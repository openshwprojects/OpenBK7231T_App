#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_aht2x.h"

#define AHT2X_I2C_ADDR (0x38 << 1)
#define MAX_AHT2X_SENSORS 4	// Max number of sensors
#define MAX_RETRIES  20		// Maimum number of retries

typedef struct {
    softI2C_t softI2C;
    byte secondsUntilNextMeasurement;
    byte secondsBetweenMeasurements;
    int channel_temp;
    int channel_humid;
    float temp;
    float humid;
    float calTemp;
    float calHum;
    bool isWorking;
} AHT2X_Sensor;

static AHT2X_Sensor g_aht2x_sensors[MAX_AHT2X_SENSORS];
static int g_num_aht2x_sensors = 0;

// get sensor by index
static AHT2X_Sensor* AHT2X_GetSensor(int idx) {
    if(idx < 0 || idx >= g_num_aht2x_sensors)
        return NULL;
    return &g_aht2x_sensors[idx];
}

void AHT2X_SoftReset(AHT2X_Sensor* sensor) {
    ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X_SoftReset: Resetting sensor");
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_CMD_RST);
    Soft_I2C_Stop(&sensor->softI2C);
    rtos_delay_milliseconds(20);
}

void AHT2X_Initialization(AHT2X_Sensor* sensor) {
    AHT2X_SoftReset(sensor);

    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_CMD_INI);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_DAT_INI1);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_DAT_INI2);
    Soft_I2C_Stop(&sensor->softI2C);

    uint8_t data = AHT2X_DAT_BUSY;
    uint8_t attempts = 0;

    while(data & AHT2X_DAT_BUSY) {
        rtos_delay_milliseconds(20);
        Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR | 1);
        data = Soft_I2C_ReadByte(&sensor->softI2C, true);
        Soft_I2C_Stop(&sensor->softI2C);
        attempts++;
        if(attempts > MAX_RETRIES) {
            ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Sensor timed out.");
            sensor->isWorking = false;
            break;
        }
    }
    if((data & 0x68) != 0x08) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Initialization failed.");
        sensor->isWorking = false;
    } else {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Initialization successful.");
        sensor->isWorking = true;
    }
}


void AHT2X_StopDriver(AHT2X_Sensor* sensor) {
    AHT2X_SoftReset(sensor);
}

// find channels used by driver
bool aht2x_used_channel(int ch){
    for(int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* s = &g_aht2x_sensors[i];
        if (s->channel_temp == ch || s->channel_humid == ch) return true;
    }
    return false;
}

void AHT2X_Measure(AHT2X_Sensor* sensor) {
    uint8_t data[6] = {0,};
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_CMD_TMS);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_DAT_TMS1);
    Soft_I2C_WriteByte(&sensor->softI2C, AHT2X_DAT_TMS2);
    Soft_I2C_Stop(&sensor->softI2C);

ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Measuring sensor clk=%d data=%d) .",sensor->softI2C.pin_clk, sensor->softI2C.pin_data);
    bool ready = false;
    rtos_delay_milliseconds(80);

    for(uint8_t i = 0; i < 10; i++) {
        Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR | 1);
        Soft_I2C_ReadBytes(&sensor->softI2C, data, 6);
        Soft_I2C_Stop(&sensor->softI2C);
        if((data[0] & AHT2X_DAT_BUSY) != AHT2X_DAT_BUSY) {
            ready = true;
            break;
        } else {
            ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X_Measure: Sensor is busy, waiting... (%ims)", i * 20);
            rtos_delay_milliseconds(20);
        }
    }

    if(!ready) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Measurements reading timed out.");
        return;
    }

    if(data[1] == 0x0 && data[2] == 0x0 && (data[3] >> 4) == 0x0) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Unrealistic humidity, will not update values.");
        return;
    }

    uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;

    sensor->humid = ((float)raw_humidity * 100.0f / 1048576.0f) + sensor->calHum;
    sensor->temp = (((200.0f * (float)raw_temperature) / 1048576.0f) - 50.0f) + sensor->calTemp;

    if(sensor->channel_temp > -1) {
        CHANNEL_Set(sensor->channel_temp, (int)(sensor->temp * 10), 0);
    }
    if(sensor->channel_humid > -1) {
        CHANNEL_Set(sensor->channel_humid, (int)(sensor->humid), 0);
    }

    ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Sensor[%ld] Temperature:%fC Humidity:%f%%",
                (long)(sensor - g_aht2x_sensors), sensor->temp, sensor->humid);
}

// Calibration - defaults to first sensor 
// AHT2X_Calibrate <temperature cal> [<hum cal>] [<sensor number>]
// not nice - to set sensor != 0, we need three args, for first sensor, only temp_cal is needed, hum_cal is defaulted
commandResult_t AHT2X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(2, 0);
	
    AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
    if(!sensor) return CMD_RES_BAD_ARGUMENT;

    sensor->calTemp = Tokenizer_GetArgFloat(0);
    sensor->calHum = Tokenizer_GetArgFloatDefault(1, 0.0f);

    ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Calibrate: Sensor[%d] Calibration done temp %f and humidity %f",
                sensor_idx, sensor->calTemp, sensor->calHum);

    return CMD_RES_OK;
}

// Set measurement interval - no sensor given --> change for all
commandResult_t AHT2X_Cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int seconds = Tokenizer_GetArgIntegerDefault(0, 10);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(1, -1);
    if(seconds < 1) {
        ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: You must have at least 1 second cycle.");
        return CMD_RES_BAD_ARGUMENT;
    }

	if (sensor_idx == -1) {
		 for(int i = 0; i < g_num_aht2x_sensors; ++i) {
			 AHT2X_Sensor* sensor = AHT2X_GetSensor(i);
			sensor->secondsBetweenMeasurements = seconds;
			ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: Sensor[%d] Measurement will run every %i seconds.", i, seconds);
		 }
	} else {
		AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
		if(!sensor) return CMD_RES_BAD_ARGUMENT;
		sensor->secondsBetweenMeasurements = seconds;
		ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: Sensor[%d] Measurement will run every %i seconds.", sensor_idx, seconds);
	}

    return CMD_RES_OK;
}

// Force measurement - no sensor given --> all sensors
commandResult_t AHT2X_Force(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(0, -1);

	if (sensor_idx == -1) {
		 for(int i = 0; i < g_num_aht2x_sensors; ++i) {
			AHT2X_Sensor* sensor = AHT2X_GetSensor(i);
			sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
			AHT2X_Measure(sensor);
		 }
	} else {
		AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
		if(!sensor) return CMD_RES_BAD_ARGUMENT;
		sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
		AHT2X_Measure(sensor);
	}
    return CMD_RES_OK;
}

commandResult_t AHT2X_Reinit(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(0, -1);
	if (sensor_idx == -1) {
		 for(int i = 0; i < g_num_aht2x_sensors; ++i) {
			AHT2X_Sensor* sensor = AHT2X_GetSensor(i);
			sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
			AHT2X_Initialization(sensor);
		 }
	} else {
		AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
		if(!sensor) return CMD_RES_BAD_ARGUMENT;
		sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
		AHT2X_Initialization(sensor);
	}
    return CMD_RES_OK;
}



// Add a single sensor
void AHT2X_AddSensor(short pin_clk,short pin_data,int channel_temp,int channel_humid) {
    if (g_num_aht2x_sensors >= MAX_AHT2X_SENSORS)
        return;

    // g_num_aht2x_sensors is the actual number of sensors.
    // since array starts with index 0, the next (unused) is index g_num_aht2x_sensors
    AHT2X_Sensor* sensor = &g_aht2x_sensors[g_num_aht2x_sensors];
    sensor->softI2C.pin_clk = pin_clk;
    sensor->softI2C.pin_data = pin_data;
    sensor->channel_temp = channel_temp;
    sensor->channel_humid = channel_humid;
    sensor->secondsBetweenMeasurements = 10;
    sensor->secondsUntilNextMeasurement = 1;
    sensor->calTemp = 0.0f;
    sensor->calHum = 0.0f;
    sensor->isWorking = false;
    Soft_I2C_PreInit(&sensor->softI2C);
    rtos_delay_milliseconds(100);
    AHT2X_Initialization(sensor);
    ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_AddSensor: Added AHT2X on pins clk=%d data=%d as sensor # %d!",sensor->softI2C.pin_clk, sensor->softI2C.pin_data,g_num_aht2x_sensors);
    g_num_aht2x_sensors++;
}

commandResult_t CMD_AHT2X_AddSensor(const void* context, const char* cmd, const char* args, int cmdFlags){
    if (g_num_aht2x_sensors >= MAX_AHT2X_SENSORS){
        ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AHT2X_AddSensor: Maximum number of sensors (%d) reached!", MAX_AHT2X_SENSORS);
        return CMD_RES_ERROR;
    }
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    short pin_clk = Tokenizer_GetPin(0, 9);
    short pin_data = Tokenizer_GetPin(1, 14);
    int channel_temp = Tokenizer_GetArgIntegerDefault(2, -1);
    int channel_humid = Tokenizer_GetArgIntegerDefault(3, -1);
    AHT2X_AddSensor(pin_clk,pin_data,channel_temp,channel_humid);
    return CMD_RES_OK;
}


void AHT2X_Init() {
	// Default: parse args for a single/first sensor on driver start
	short pin_clk = Tokenizer_GetPin(1, 9);
	short pin_data = Tokenizer_GetPin(2, 14);
	int channel_temp = Tokenizer_GetArgIntegerDefault(3, -1);
	int channel_humid = Tokenizer_GetArgIntegerDefault(4, -1);

	g_num_aht2x_sensors=0; // AddSensor will increase to 1 after first sensor is registered

	AHT2X_AddSensor(pin_clk,pin_data,channel_temp,channel_humid);
	
	// register commands
	
	//cmddetail:{"name":"AHT2X_Calibrate","args":"<DeltaTemp> [DeltaHumidity - default 0] [Sensor - default 0]",
	//cmddetail:"descr":"Calibrate the AHT2X Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"AHT2X_Calibrate","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Calibrate -4 10 <br /> meaning -4 on current temp reading and +10 on current humidity reading sor first sensor"}
	CMD_RegisterCommand("AHT2X_Calibrate", AHT2X_Calibrate, NULL);
	//cmddetail:{"name":"AHT2X_Cycle","args":"<IntervalSeconds> [Sensor - default: all sensors]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
	//cmddetail:"fn":"AHT2X_cycle","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Cycle 60 <br /> measurement is taken every 60 seconds"}
	CMD_RegisterCommand("AHT2X_Cycle", AHT2X_Cycle, NULL);
	//cmddetail:{"name":"AHT2X_Measure","args":"[Sensor - default: all sensors]",
	//cmddetail:"descr":"Retrieve OneShot measurement.",
	//cmddetail:"fn":"AHT2X_Measure","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Measure"}
	CMD_RegisterCommand("AHT2X_Measure", AHT2X_Force, NULL);
	//cmddetail:{"name":"AHT2X_Reinit","args":"[Sensor - default: all sensors]",
	//cmddetail:"descr":"Reinitialize sensor.",
	//cmddetail:"fn":"AHT2X_Reinit","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Reinit"}
	CMD_RegisterCommand("AHT2X_Reinit", AHT2X_Reinit, NULL);
	//cmddetail:{"name":"AHT2X_Addsensor","args":"<CLK Pin> <DATA Pin> [Channel temperature] [Channel humidity]",
	//cmddetail:"descr":"Add another AHT2X with CLK and DATA pin and (optional) channels for temp and hum",
	//cmddetail:"fn":"AHT2X_Addsensor","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Addsensor 10 11 4 5<br /> Add sensor with CLK-pin 10 and DATA-pin 11, channel for temp 4 channel for hum 5"}
	CMD_RegisterCommand("AHT2X_Addsensor", CMD_AHT2X_AddSensor, NULL);
		
}

// Call every second for all sensors
void AHT2X_OnEverySecond() {
    for(int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* sensor = &g_aht2x_sensors[i];
        if(sensor->secondsUntilNextMeasurement <= 0) {
            if(sensor->isWorking){
                AHT2X_Measure(sensor);
            }
/*
            else {
 		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_OnEverySecond: Sensor %i (clk=%d data=%d) not working, trying to re-init",i,sensor->softI2C.pin_clk, sensor->softI2C.pin_data);
		Soft_I2C_PreInit(&sensor->softI2C);
		rtos_delay_milliseconds(100);
		AHT2X_Initialization(sensor);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Reinit done for sensor %i (clk=%d data=%d) - sensor is %s working!",i,sensor->softI2C.pin_clk, sensor->softI2C.pin_data, sensor->isWorking?"now":"still not");
		if(sensor->isWorking)
          	      AHT2X_Measure(sensor);
            }
*/
            sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
        }
        if(sensor->secondsUntilNextMeasurement > 0) {
            sensor->secondsUntilNextMeasurement--;
        }
    }
}

// HTTP info: show all sensors
void AHT2X_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
    if (bPreState)
        return;
    for(int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* sensor = &g_aht2x_sensors[i];
        hprintf255(request, "<h2>AHT2X[%d] Temperature=%.1fC, Humidity=%.0f%%</h2>", i, sensor->temp, sensor->humid);
        if(!sensor->isWorking) {
            hprintf255(request, "WARNING: AHT sensor[%d] appears to have failed initialization, check if configured pins are correct!<br>", i);
        }
        if(sensor->channel_humid == sensor->channel_temp) {
            hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results for sensor[%d]!<br>", i);
        }
    }
}
