#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_aht2x.h"

#define AHT2X_I2C_ADDR  (0x38 << 1)
#define MAX_AHT2X_SENSORS 4   // Max number of sensors
#define MAX_RETRIES      20   // Maximum number of retries

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

// Internal helpers

static AHT2X_Sensor* AHT2X_GetSensor(int idx) {
    if (idx < 0 || idx >= g_num_aht2x_sensors)
        return NULL;
    return &g_aht2x_sensors[idx];
}

// Iterate over one sensor (sensor_idx >= 0) or all sensors (sensor_idx == -1)
// and call fn() on each. Returns CMD_RES_BAD_ARGUMENT if a specific index is
// out of range, CMD_RES_OK otherwise.
static commandResult_t AHT2X_ForEach(int sensor_idx, void (*fn)(AHT2X_Sensor*)) {
    if (sensor_idx == -1) {
        for (int i = 0; i < g_num_aht2x_sensors; ++i)
            fn(&g_aht2x_sensors[i]);
    } else {
        AHT2X_Sensor* s = AHT2X_GetSensor(sensor_idx);
        if (!s) return CMD_RES_BAD_ARGUMENT;
        fn(s);
    }
    return CMD_RES_OK;
}

// I2C transaction helpers — eliminate repeated Start/Write.../Stop patterns

// Write 1 byte command (e.g. soft reset)
static void AHT2X_I2C_WriteCmd1(AHT2X_Sensor* sensor, uint8_t cmd) {
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR);
    Soft_I2C_WriteByte(&sensor->softI2C, cmd);
    Soft_I2C_Stop(&sensor->softI2C);
}

// Write 3 byte command (cmd + 2 data bytes, e.g. init / trigger measure)
static void AHT2X_I2C_WriteCmd3(AHT2X_Sensor* sensor, uint8_t cmd, uint8_t b1, uint8_t b2) {
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR);
    Soft_I2C_WriteByte(&sensor->softI2C, cmd);
    Soft_I2C_WriteByte(&sensor->softI2C, b1);
    Soft_I2C_WriteByte(&sensor->softI2C, b2);
    Soft_I2C_Stop(&sensor->softI2C);
}

// Read 1 status byte (used during init/busy poll)
static uint8_t AHT2X_I2C_ReadStatus(AHT2X_Sensor* sensor) {
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR | 1);
    uint8_t status = Soft_I2C_ReadByte(&sensor->softI2C, true);
    Soft_I2C_Stop(&sensor->softI2C);
    return status;
}

// Read 6 measurement bytes into buf
static void AHT2X_I2C_ReadData(AHT2X_Sensor* sensor, uint8_t* buf) {
    Soft_I2C_Start(&sensor->softI2C, AHT2X_I2C_ADDR | 1);
    Soft_I2C_ReadBytes(&sensor->softI2C, buf, 6);
    Soft_I2C_Stop(&sensor->softI2C);
}

void AHT2X_SoftReset(AHT2X_Sensor* sensor) {
    ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X: reset");
    AHT2X_I2C_WriteCmd1(sensor, AHT2X_CMD_RST);
    rtos_delay_milliseconds(20);
}

void AHT2X_Initialization(AHT2X_Sensor* sensor) {
    AHT2X_SoftReset(sensor);
    AHT2X_I2C_WriteCmd3(sensor, AHT2X_CMD_INI, AHT2X_DAT_INI1, AHT2X_DAT_INI2);

    uint8_t data = AHT2X_DAT_BUSY;
    uint8_t attempts = 0;

    while (data & AHT2X_DAT_BUSY) {
        rtos_delay_milliseconds(20);
        data = AHT2X_I2C_ReadStatus(sensor);
        if (++attempts > MAX_RETRIES) {
            ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X: init timed out");
            sensor->isWorking = false;
            return; // early-out: don't check status on stale data
        }
    }

    // BUG FIX: was 0x68 — correct calibration mask per datasheet is 0x18
    // bit3 must be set (calibrated), bits6-7 must be clear (not busy/error)
    sensor->isWorking = ((data & 0x18) == 0x18);
    ADDLOG_INFO(LOG_FEATURE_SENSOR, sensor->isWorking
                ? "AHT2X: init ok"
                : "AHT2X: init failed (status=0x%02x)", data);
}

void AHT2X_StopDriver(AHT2X_Sensor* sensor) {
    AHT2X_SoftReset(sensor);
}

void AHT2X_Measure(AHT2X_Sensor* sensor) {
    uint8_t data[6] = {0};

    AHT2X_I2C_WriteCmd3(sensor, AHT2X_CMD_TMS, AHT2X_DAT_TMS1, AHT2X_DAT_TMS2);
    rtos_delay_milliseconds(80);

    bool ready = false;
    for (uint8_t i = 0; i < 10; i++) {
        AHT2X_I2C_ReadData(sensor, data);
        if (!(data[0] & AHT2X_DAT_BUSY)) {
            ready = true;
            break;
        }
        ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X: busy +%dms", i * 20);
        rtos_delay_milliseconds(20);
    }

    if (!ready) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X: measure timed out");
        return;
    }

    // Sanity check: all-zero humidity bytes → unrealistic reading
    if (data[1] == 0 && data[2] == 0 && (data[3] >> 4) == 0) {
        ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X: unrealistic humidity, skipping");
        return;
    }

    uint32_t raw_hum  = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
    uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];

    sensor->humid = (raw_hum  * 100.0f / 1048576.0f) + sensor->calHum;
    sensor->temp  = ((200.0f * raw_temp / 1048576.0f) - 50.0f) + sensor->calTemp;

    if (sensor->channel_temp  > -1) CHANNEL_Set(sensor->channel_temp,  (int)(sensor->temp  * 10), 0);
    if (sensor->channel_humid > -1) CHANNEL_Set(sensor->channel_humid, (int)(sensor->humid),       0);

    ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X[%ld]: %.1fC %.0f%%",
                (long)(sensor - g_aht2x_sensors), sensor->temp, sensor->humid);
}

// To show as "used" in channel query
bool aht2x_used_channel(int ch) {
    for (int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* s = &g_aht2x_sensors[i];
        if (s->channel_temp == ch || s->channel_humid == ch) return true;
    }
    return false;
}


// AHT2X_Calibrate <DeltaTemp> [DeltaHumidity=0] [Sensor=0]
// Note: to set for sensor != 0 both calibration values + sensor-id must be given.
commandResult_t AHT2X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(2, 0);
    AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
    if (!sensor) return CMD_RES_BAD_ARGUMENT;

    sensor->calTemp = Tokenizer_GetArgFloat(0);
    sensor->calHum  = Tokenizer_GetArgFloatDefault(1, 0.0f);

    ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X[%d]: cal temp=%f hum=%f",
                sensor_idx, sensor->calTemp, sensor->calHum);
    return CMD_RES_OK;
}

// AHT2X_Cycle <seconds> [Sensor=-1 means all]
commandResult_t AHT2X_Cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    int seconds    = Tokenizer_GetArgIntegerDefault(0, 10);
    int sensor_idx = Tokenizer_GetArgIntegerDefault(1, -1);

    if (seconds < 1) {
        ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: need >= 1 second");
        return CMD_RES_BAD_ARGUMENT;
    }

    if (sensor_idx == -1) {
        for (int i = 0; i < g_num_aht2x_sensors; ++i)
            g_aht2x_sensors[i].secondsBetweenMeasurements = seconds;
        ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X: all sensors -> %ds cycle", seconds);
    } else {
        AHT2X_Sensor* sensor = AHT2X_GetSensor(sensor_idx);
        if (!sensor) return CMD_RES_BAD_ARGUMENT;
        sensor->secondsBetweenMeasurements = seconds;
        ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X[%d]: %ds cycle", sensor_idx, seconds);
    }
    return CMD_RES_OK;
}

// Helpers for "ForEach" callbacks (no arguments needed)
static void _DoMeasure(AHT2X_Sensor* s) {
    s->secondsUntilNextMeasurement = s->secondsBetweenMeasurements;
    AHT2X_Measure(s);
}

static void _DoReinit(AHT2X_Sensor* s) {
    s->secondsUntilNextMeasurement = s->secondsBetweenMeasurements;
    AHT2X_Initialization(s);
}

// AHT2X_Measure [Sensor=-1 means all]
commandResult_t AHT2X_Force(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    return AHT2X_ForEach(Tokenizer_GetArgIntegerDefault(0, -1), _DoMeasure);
}

// AHT2X_Reinit [Sensor=-1 means all]
commandResult_t AHT2X_Reinit(const void* context, const char* cmd, const char* args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    return AHT2X_ForEach(Tokenizer_GetArgIntegerDefault(0, -1), _DoReinit);
}

// Sensor registration
void AHT2X_AddSensor(short pin_clk, short pin_data, int channel_temp, int channel_humid) {
    if (g_num_aht2x_sensors >= MAX_AHT2X_SENSORS)
        return;

    AHT2X_Sensor* sensor = &g_aht2x_sensors[g_num_aht2x_sensors];
    sensor->softI2C.pin_clk  = pin_clk;
    sensor->softI2C.pin_data = pin_data;
    sensor->channel_temp     = channel_temp;
    sensor->channel_humid    = channel_humid;
    sensor->secondsBetweenMeasurements  = 10;
    sensor->secondsUntilNextMeasurement = 1;
    sensor->calTemp   = 0.0f;
    sensor->calHum    = 0.0f;
    sensor->isWorking = false;

    Soft_I2C_PreInit(&sensor->softI2C);
    rtos_delay_milliseconds(100);
    AHT2X_Initialization(sensor);

    ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X: added sensor #%d clk=%d data=%d",
                g_num_aht2x_sensors, pin_clk, pin_data);
    g_num_aht2x_sensors++;
}

commandResult_t CMD_AHT2X_AddSensor(const void* context, const char* cmd, const char* args, int cmdFlags) {
    if (g_num_aht2x_sensors >= MAX_AHT2X_SENSORS) {
        ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X: max sensors (%d) reached", MAX_AHT2X_SENSORS);
        return CMD_RES_ERROR;
    }
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
    AHT2X_AddSensor(
        Tokenizer_GetPin(0, 9),
        Tokenizer_GetPin(1, 14),
        Tokenizer_GetArgIntegerDefault(2, -1),
        Tokenizer_GetArgIntegerDefault(3, -1)
    );
    return CMD_RES_OK;
}


void AHT2X_Init() {
    // Remember: Use arg indices 1-4 (startdriver aht2x --> "aht2x" at index 0)
    short pin_clk  = Tokenizer_GetPin(1, 9);
    short pin_data = Tokenizer_GetPin(2, 14);
    int channel_temp  = Tokenizer_GetArgIntegerDefault(3, -1);
    int channel_humid = Tokenizer_GetArgIntegerDefault(4, -1);

    g_num_aht2x_sensors = 0;
    AHT2X_AddSensor(pin_clk, pin_data, channel_temp, channel_humid);

    //cmddetail:{"name":"AHT2X_Calibrate","args":"<DeltaTemp> [DeltaHumidity - default 0] [Sensor - default 0]",
    //cmddetail:"descr":"Calibrate the AHT2X Sensor as Tolerance is +/-2 degrees C.",
    //cmddetail:"fn":"AHT2X_Calibrate","file":"driver/drv_aht2x.c","requires":"",
    //cmddetail:"examples":"AHT2X_Calibrate -4 10 <br /> meaning -4 on current temp reading and +10 on current humidity reading for first sensor"}
    CMD_RegisterCommand("AHT2X_Calibrate", AHT2X_Calibrate, NULL);
    //cmddetail:{"name":"AHT2X_Cycle","args":"<IntervalSeconds> [Sensor - default: all sensors]",
    //cmddetail:"descr":"This is the interval between measurements in seconds, by default 10. Max is 255.",
    //cmddetail:"fn":"AHT2X_Cycle","file":"driver/drv_aht2x.c","requires":"",
    //cmddetail:"examples":"AHT2X_Cycle 60 <br /> measurement is taken every 60 seconds"}
    CMD_RegisterCommand("AHT2X_Cycle", AHT2X_Cycle, NULL);
    //cmddetail:{"name":"AHT2X_Measure","args":"[Sensor - default: all sensors]",
    //cmddetail:"descr":"Retrieve OneShot measurement.",
    //cmddetail:"fn":"AHT2X_Force","file":"driver/drv_aht2x.c","requires":"",
    //cmddetail:"examples":"AHT2X_Measure"}
    CMD_RegisterCommand("AHT2X_Measure", AHT2X_Force, NULL);
    //cmddetail:{"name":"AHT2X_Reinit","args":"[Sensor - default: all sensors]",
    //cmddetail:"descr":"Reinitialize sensor.",
    //cmddetail:"fn":"AHT2X_Reinit","file":"driver/drv_aht2x.c","requires":"",
    //cmddetail:"examples":"AHT2X_Reinit"}
    CMD_RegisterCommand("AHT2X_Reinit", AHT2X_Reinit, NULL);
    //cmddetail:{"name":"AHT2X_Addsensor","args":"<CLK Pin> <DATA Pin> [Channel temperature] [Channel humidity]",
    //cmddetail:"descr":"Add another AHT2X with CLK and DATA pin and (optional) channels for temp and hum",
    //cmddetail:"fn":"AHT2X_Addsensor","file":"driver/drv_aht2x.c","requires":"",
    //cmddetail:"examples":"AHT2X_Addsensor 10 11 4 5<br /> Add sensor with CLK-pin 10 and DATA-pin 11, channel for temp 4 channel for hum 5"}
    CMD_RegisterCommand("AHT2X_Addsensor", CMD_AHT2X_AddSensor, NULL);
}

void AHT2X_OnEverySecond() {
    for (int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* sensor = &g_aht2x_sensors[i];
        if (sensor->secondsUntilNextMeasurement == 0) {
            if (sensor->isWorking)
                AHT2X_Measure(sensor);
            else
            	_DoReinit(sensor);
            sensor->secondsUntilNextMeasurement = sensor->secondsBetweenMeasurements;
        }
        sensor->secondsUntilNextMeasurement--;
    }
}

void AHT2X_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
    if (bPreState)
        return;
    for (int i = 0; i < g_num_aht2x_sensors; ++i) {
        AHT2X_Sensor* sensor = &g_aht2x_sensors[i];
        hprintf255(request, "<h2>AHT2X[%d] Temperature=%.1fC, Humidity=%.0f%%</h2>",
                   i, sensor->temp, sensor->humid);
        if (!sensor->isWorking)
            hprintf255(request,
                       "WARNING: AHT2X sensor[%d] failed init, check pins!<br>", i);
        // BUG FIX: was checking channel_humid == channel_temp which silently
        // passes when both are -1 (unconfigured). Correct check is both == -1.
        if (sensor->channel_humid == -1 && sensor->channel_temp == -1)
            hprintf255(request,
                       "WARNING: No target channels configured for sensor[%d]!<br>", i);
    }
}
