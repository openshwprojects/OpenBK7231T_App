//  Driver for BMP085, 180, 280, and BME280, 680, 688.
#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"

static byte g_secondsBetweenMeasurements = 1, g_secondsUntilNextMeasurement = 1;
static int32_t g_temperature, g_calTemp = 0, g_calHum = 0, g_calPres = 0;
static uint32_t g_pressure, g_humidity;
static char g_targetChannelTemperature = -1, g_targetChannelPressure = -1, g_targetChannelHumidity = -1;
static softI2C_t g_softI2C;
static char* g_chipName = "BMPI2C";
bool isConfigured = false;

#include "drv_bmpi2c.h"

commandResult_t BMPI2C_Configure(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	BMP_mode mode = GetMode(Tokenizer_GetArgIntegerDefault(0, 0));
	BMP_sampling t_sampling = GetSampling(Tokenizer_GetArgIntegerDefault(1, 1));
	BMP_sampling p_sampling = GetSampling(Tokenizer_GetArgIntegerDefault(2, 1));
	BMP_sampling h_sampling = GetSampling(Tokenizer_GetArgIntegerDefault(3, 1));
	BMP_filter filter = GetFilter(Tokenizer_GetArgIntegerDefault(4, 0));
	standby_time time = GetStandbyTime(Tokenizer_GetArgIntegerDefault(5, 1000));

	BMXI2C_Configure(mode, t_sampling, p_sampling, h_sampling, filter, time);

	isConfigured = true;
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "BMPI2C_Configure: configured");

	return CMD_RES_OK;
}

commandResult_t BMPI2C_Cycle(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int seconds = Tokenizer_GetArgInteger(0);
	if(seconds < 1)
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "BMPI2C_Cycle: You must have at least 1 second cycle.");
		return CMD_RES_BAD_ARGUMENT;
	}
	else
	{
		g_secondsBetweenMeasurements = seconds;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "BMPI2C_Cycle: Measurement will run every %i seconds.", g_secondsBetweenMeasurements);

	return CMD_RES_OK;
}

commandResult_t BMPI2C_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	g_calTemp = Tokenizer_GetArgFloatDefault(0, 0.0f) * 100;
	g_calPres = Tokenizer_GetArgFloatDefault(1, 0.0f) * 100;
	g_calHum = Tokenizer_GetArgFloatDefault(2, 0.0f) * 10;

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "BMPI2C_Calibrate: Calibration done temp %f, pressure %f and humidity %f", g_calTemp / 100, g_calPres / 100, g_calHum / 10);

	return CMD_RES_OK;
}

void BMPI2C_Measure()
{
	if(IsBMX280)
	{
		BMX280_Update();
		delay_ms(125);
		g_temperature = BMX280_ReadTemperature();
		g_pressure = BMX280_ReadPressure();
		if(IsBME280)
		{
			g_humidity = BME280_ReadHumidity();
		}
	}
	else if(IsBMP180)
	{
		g_pressure = BMP180_ReadData(&g_temperature);
	}
	else if(IsBME68X)
	{
		BME68X_Update();
		g_temperature = BME68X_ReadTemperature();
		g_pressure = BME68X_ReadPressure();
		g_humidity = BME68X_ReadHumidity();
	}
	g_temperature += g_calTemp;
	g_pressure += g_calPres;
	if(g_targetChannelTemperature != -1)
	{
		CHANNEL_Set(g_targetChannelTemperature, g_temperature, CHANNEL_SET_FLAG_SILENT);
	}
	if(g_targetChannelPressure != -1)
	{
		CHANNEL_Set(g_targetChannelPressure, g_pressure, CHANNEL_SET_FLAG_SILENT);
	}
	if(isHumidityAvail)
	{
		g_humidity += g_calHum;
		if(g_targetChannelHumidity != -1)
		{
			CHANNEL_Set(g_targetChannelHumidity, g_humidity, CHANNEL_SET_FLAG_SILENT);
		}
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "T %i, P %i, H%i", g_temperature, g_pressure, g_humidity);
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "T %i, P %i", g_temperature, g_pressure);
	}
}

// startDriver BMPI2C 8 14 1 2 3 0
// startDriver BMPI2C [CLK] [DATA] [ChannelForTemp] [ChannelForPressure] [ChannelForHumidity] [Addr]
// Adr8bit 0 for 0x77, 1 for 0x76
void BMPI2C_Init()
{
	g_softI2C.pin_clk = Tokenizer_GetArgIntegerDefault(1, 8);
	g_softI2C.pin_data = Tokenizer_GetArgIntegerDefault(2, 14);
	g_targetChannelTemperature = Tokenizer_GetArgIntegerDefault(3, -1);
	g_targetChannelPressure = Tokenizer_GetArgIntegerDefault(4, -1);
	g_targetChannelHumidity = Tokenizer_GetArgIntegerDefault(5, -1);
	if(Tokenizer_GetArgIntegerDefault(6, 0) == 1)
	{
		g_softI2C.address8bit = I2C_ALT_ADDR;
	}
	else
	{
		g_softI2C.address8bit = I2C_MAIN_ADDR;
	}
	isConfigured = false;

	Soft_I2C_PreInit(&g_softI2C);

	usleep(100);

	if(BMP_BasicInit())
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s initialized!", g_chipName);
		if(IsBMX280 || IsBME68X)
		{
			BMXI2C_Configure(MODE_NORMAL, SAMPLING_X1, SAMPLING_X1, SAMPLING_X1, FILTER_OFF, STANDBY_0_5);
			isConfigured = true;
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s done basic configuration", g_chipName);
		}
		else if(IsBMP180)
		{
			isConfigured = true;
		}

		//cmddetail:{"name":"BMPI2C_Configure","args":"[Mode][TempSampling][PressureSampling][HumSampling][IIRFilter][StandbyTime]",
		//cmddetail:"descr":"Manual sensor configuration. Modes: 0 - normal, 1 - forced, 2 - sleep. Overampling range: -1 - skipped, 2^0 to 2^4. Default is X1
		//cmddetail:IIRFilter range: 0 - off, 2^1 to 2^4 for most, up to 2^7 for BME68X",
		//cmddetail:StandbyTime: 1 for 0.5ms, 63 for 62.5ms, 125, 250, 500, 1000, 2000, 4000. Mode and StandbyTime are not needed on BME68X
		//cmddetail:All values will be rounded down to closest available (like sampling 10 will choose 8x)
		//cmddetail:"fn":"BMPI2C_Configure","file":"driver/drv_bmpi2c.c","requires":"",
		//cmddetail:"examples":"BMPI2C_Configure 0 8 2 4 16 125 <br />"}
		CMD_RegisterCommand("BMPI2C_Configure", BMPI2C_Configure, NULL);
		//cmddetail:{"name":"BMPI2C_Calibrate","args":"[DeltaTemp][DeltaPressure][DeltaHumidity]",
		//cmddetail:"descr":"Calibrate the BMPI2C Sensor.",
		//cmddetail:"fn":"BMPI2C_Calibrate","file":"driver/drv_bmpi2c.c","requires":"",
		//cmddetail:"examples":"BMPI2C_Calibrate -4 0 10 <br /> meaning -4 on current temp reading, 0 on current pressure reading and +10 on current humidity reading"}
		CMD_RegisterCommand("BMPI2C_Calibrate", BMPI2C_Calibrate, NULL);
		//cmddetail:{"name":"BMPI2C_Cycle","args":"[IntervalSeconds]",
		//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
		//cmddetail:"fn":"BMPI2C_cycle","file":"drv/drv_bmpi2c.c","requires":"",
		//cmddetail:"examples":"BMPI2C_Cycle 60 <br /> measurement is taken every 60 seconds"}
		CMD_RegisterCommand("BMPI2C_Cycle", BMPI2C_Cycle, NULL);
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_SENSOR, "%s failed!", g_chipName);
	}
}

void BMPI2C_OnEverySecond()
{
	if(g_secondsUntilNextMeasurement <= 0)
	{
		if(isConfigured) BMPI2C_Measure();
		g_secondsUntilNextMeasurement = g_secondsBetweenMeasurements;
	}
	if(g_secondsUntilNextMeasurement > 0)
	{
		g_secondsUntilNextMeasurement--;
	}
}

void BMPI2C_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>%s Temperature=%.2f C, Pressure=%.2f hPa", g_chipName, g_temperature * 0.01f, g_pressure * 0.01f);
	if(isHumidityAvail)
	{
		hprintf255(request, ", Humidity=%.1f%%", g_humidity * 0.1f);
	}
	hprintf255(request, "</h2>");
}
