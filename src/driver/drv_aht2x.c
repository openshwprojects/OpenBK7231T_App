#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_aht2x.h"

#define AHT2X_I2C_ADDR (0x38 << 1)

static byte g_aht_secondsUntilNextMeasurement = 1, g_aht_secondsBetweenMeasurements = 10,
	max_retries = 20, channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0, g_calTemp = 0.0, g_calHum = 0.0;
static softI2C_t g_softI2C;
static bool isWorking = false;

void AHT2X_SoftReset()
{
	ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X_SoftReset: Resetting sensor");
	Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_CMD_RST);
	Soft_I2C_Stop(&g_softI2C);
	rtos_delay_milliseconds(20);
}

void AHT2X_Initialization()
{
	AHT2X_SoftReset();

	Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_CMD_INI);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_DAT_INI1);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_DAT_INI2);
	Soft_I2C_Stop(&g_softI2C);

	uint8_t data = AHT2X_DAT_BUSY;
	uint8_t attempts = 0;

	while(data & AHT2X_DAT_BUSY)
	{
		rtos_delay_milliseconds(20);
		Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR | 1);
		data = Soft_I2C_ReadByte(&g_softI2C, true);
		Soft_I2C_Stop(&g_softI2C);
		attempts++;
		if(attempts > max_retries)
		{
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Sensor timed out.");
			isWorking = false;
			break;
		}
	}
	if((data & 0x68) != 0x08)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Initialization failed.");
		isWorking = false;
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Initialization: Initialization successful.");
		isWorking = true;
	}
}

void AHT2X_StopDriver()
{
	AHT2X_SoftReset();
}

void AHT2X_Measure()
{
	uint8_t data[6] = { 0, };

	Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_CMD_TMS);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_DAT_TMS1);
	Soft_I2C_WriteByte(&g_softI2C, AHT2X_DAT_TMS2);
	Soft_I2C_Stop(&g_softI2C);

	bool ready = false;

	rtos_delay_milliseconds(80);

	for(uint8_t i = 0; i < 10; i++)
	{
		Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR | 1);
		Soft_I2C_ReadBytes(&g_softI2C, data, 6);
		Soft_I2C_Stop(&g_softI2C);
		if((data[0] & AHT2X_DAT_BUSY) != AHT2X_DAT_BUSY)
		{
			ready = true;
			break;
		}
		else
		{
			ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "AHT2X_Measure: Sensor is busy, waiting... (%ims)", i * 20);
			rtos_delay_milliseconds(20);
		}
	}

	if(!ready)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Measurements reading timed out.");
		return;
	}

	if(data[1] == 0x0 && data[2] == 0x0 && (data[3] >> 4) == 0x0)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Unrealistic humidity, will not update values.");
		return;
	}

	uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
	uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;

	g_humid = ((float)raw_humidity * 100.0f / 1048576.0f) + g_calHum;
	g_temp = (((200.0f * (float)raw_temperature) / 1048576.0f) - 50.0f) + g_calTemp;

	if(channel_temp > -1)
	{
		CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	}
	if(channel_humid > -1)
	{
		CHANNEL_Set(channel_humid, (int)(g_humid), 0);
	}

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Measure: Temperature:%fC Humidity:%f%%", g_temp, g_humid);
}

commandResult_t AHT2X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_calTemp = Tokenizer_GetArgFloat(0);
	g_calHum = Tokenizer_GetArgFloatDefault(1, 0.0f);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X_Calibrate: Calibration done temp %f and humidity %f", g_calTemp, g_calHum);

	return CMD_RES_OK;
}

commandResult_t AHT2X_Cycle(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int seconds = Tokenizer_GetArgInteger(0);
	if(seconds < 1)
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: You must have at least 1 second cycle.");
		return CMD_RES_BAD_ARGUMENT;
	}
	else
	{
		g_aht_secondsBetweenMeasurements = seconds;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "AHT2X_Cycle: Measurement will run every %i seconds.", g_aht_secondsBetweenMeasurements);

	return CMD_RES_OK;
}

commandResult_t AHT2X_Force(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	g_aht_secondsUntilNextMeasurement = g_aht_secondsBetweenMeasurements;

	AHT2X_Measure();

	return CMD_RES_OK;
}

commandResult_t AHT2X_Reinit(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	g_aht_secondsUntilNextMeasurement = g_aht_secondsBetweenMeasurements;

	AHT2X_Initialization();

	return CMD_RES_OK;
}

void AHT2X_Init()
{
	g_softI2C.pin_clk = Tokenizer_GetArgIntegerDefault(1, 9);
	g_softI2C.pin_data = Tokenizer_GetArgIntegerDefault(2, 14);
	channel_temp = Tokenizer_GetArgIntegerDefault(3, -1);
	channel_humid = Tokenizer_GetArgIntegerDefault(4, -1);

	Soft_I2C_PreInit(&g_softI2C);
	rtos_delay_milliseconds(100);

	AHT2X_Initialization();

	//cmddetail:{"name":"AHT2X_Calibrate","args":"[DeltaTemp][DeltaHumidity]",
	//cmddetail:"descr":"Calibrate the AHT2X Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"AHT2X_Calibrate","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Calibrate -4 10 <br /> meaning -4 on current temp reading and +10 on current humidity reading"}
	CMD_RegisterCommand("AHT2X_Calibrate", AHT2X_Calibrate, NULL);
	//cmddetail:{"name":"AHT2X_Cycle","args":"[IntervalSeconds]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
	//cmddetail:"fn":"AHT2X_cycle","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Cycle 60 <br /> measurement is taken every 60 seconds"}
	CMD_RegisterCommand("AHT2X_Cycle", AHT2X_Cycle, NULL);
	//cmddetail:{"name":"AHT2X_Measure","args":"",
	//cmddetail:"descr":"Retrieve OneShot measurement.",
	//cmddetail:"fn":"AHT2X_Measure","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Measure"}
	CMD_RegisterCommand("AHT2X_Measure", AHT2X_Force, NULL);
	//cmddetail:{"name":"AHT2X_Reinit","args":"",
	//cmddetail:"descr":"Reinitialize sensor.",
	//cmddetail:"fn":"AHT2X_Reinit","file":"drv/drv_aht2X.c","requires":"",
	//cmddetail:"examples":"AHT2X_Reinit"}
	CMD_RegisterCommand("AHT2X_Reinit", AHT2X_Reinit, NULL);
}

void AHT2X_OnEverySecond()
{
	if(g_aht_secondsUntilNextMeasurement <= 0)
	{
		if(isWorking) AHT2X_Measure();
		g_aht_secondsUntilNextMeasurement = g_aht_secondsBetweenMeasurements;
	}
	if(g_aht_secondsUntilNextMeasurement > 0)
	{
		g_aht_secondsUntilNextMeasurement--;
	}
}

void AHT2X_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>AHT2X Temperature=%f, Humidity=%f</h2>", g_temp, g_humid);
	if(!isWorking)
	{
		hprintf255(request, "WARNING: AHT sensor appears to have failed initialization, check if configured pins are correct!");
	}
	if(channel_humid == channel_temp)
	{
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}
