#include "../new_pins.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_aht2x.h"

#define AHT2X_I2C_ADDR (0x38 << 1)
#define AHT2X_CRC_MODE_OFF 0
#define AHT2X_CRC_MODE_AUTO 1
#define AHT2X_CRC_MODE_REQUIRED 2

static byte max_retries = 20;
static int g_aht_secondsUntilNextMeasurement = 1, g_aht_secondsBetweenMeasurements = 10, channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0, g_calTemp = 0.0, g_calHum = 0.0;
static softI2C_t g_softI2C;
static bool isWorking = false;
static bool g_crcSupported = false;
static byte g_crcMode = AHT2X_CRC_MODE_OFF;
static uint8_t g_lastStatus = 0;

static bool AHT2X_IsReadyAndCalibrated(uint8_t status)
{
	return ((status & AHT2X_DAT_BUSY) == 0) && ((status & AHT2X_DAT_CALIBRATED) == AHT2X_DAT_CALIBRATED);
}

static bool AHT2X_WriteCommand(uint8_t cmd)
{
	bool ok = Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR);
	if(ok)
	{
		ok = Soft_I2C_WriteByte(&g_softI2C, cmd);
	}
	Soft_I2C_Stop(&g_softI2C);
	return ok;
}

static bool AHT2X_WriteCommand3(uint8_t cmd, uint8_t data1, uint8_t data2)
{
	bool ok = Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR);
	if(ok)
	{
		ok = Soft_I2C_WriteByte(&g_softI2C, cmd);
	}
	if(ok)
	{
		ok = Soft_I2C_WriteByte(&g_softI2C, data1);
	}
	if(ok)
	{
		ok = Soft_I2C_WriteByte(&g_softI2C, data2);
	}
	Soft_I2C_Stop(&g_softI2C);
	return ok;
}

static bool AHT2X_ReadBytes(uint8_t *data, int len)
{
	bool ok = Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR | 1);
	if(ok)
	{
		Soft_I2C_ReadBytes(&g_softI2C, data, len);
	}
	Soft_I2C_Stop(&g_softI2C);
	return ok;
}

static bool AHT2X_ReadStatus(uint8_t *status)
{
	bool ok = Soft_I2C_Start(&g_softI2C, AHT2X_I2C_ADDR | 1);
	if(ok)
	{
		*status = Soft_I2C_ReadByte(&g_softI2C, true);
		g_lastStatus = *status;
	}
	Soft_I2C_Stop(&g_softI2C);
	return ok;
}

static bool AHT2X_WaitReady(uint8_t *status)
{
	uint8_t attempts = 0;
	*status = AHT2X_DAT_BUSY;

	while(*status & AHT2X_DAT_BUSY)
	{
		rtos_delay_milliseconds(20);
		if(!AHT2X_ReadStatus(status))
		{
			return false;
		}
		attempts++;
		if(attempts > max_retries)
		{
			return false;
		}
	}
	return true;
}

static uint8_t AHT2X_CalcCrc8(const uint8_t *data, int len)
{
	uint8_t crc = AHT2X_CRC8_INIT;
	for(int idx = 0; idx < len; idx++)
	{
		crc ^= data[idx];
		for(uint8_t bit = 8; bit > 0; bit--)
		{
			if(crc & 0x80)
			{
				crc = (crc << 1) ^ AHT2X_CRC8_POLY;
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

static bool AHT2X_ValidateCrcIfPresent(const uint8_t *data)
{
	if(g_crcMode == AHT2X_CRC_MODE_OFF)
	{
		return true;
	}

	uint8_t crc = AHT2X_CalcCrc8(data, AHT2X_READ_LEN);
	if(crc == data[AHT2X_READ_LEN])
	{
		if(!g_crcSupported)
		{
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X: CRC byte detected and will be validated.");
		}
		g_crcSupported = true;
		return true;
	}

	if(g_crcMode == AHT2X_CRC_MODE_REQUIRED || g_crcSupported)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "AHT2X: CRC mismatch, expected 0x%02X got 0x%02X; measurement ignored.", crc, data[AHT2X_READ_LEN]);
		return false;
	}

	/*
	 * Some AHT1x/AHT2x modules expose only the six measurement bytes described
	 * by older datasheets/libraries.  When CRC auto mode is explicitly enabled,
	 * a non-matching seventh byte is treated as "CRC not present" rather than as
	 * a sensor fault.
	 */
	return true;
}

void AHT2X_SoftReset()
{
	if(!AHT2X_WriteCommand(AHT2X_CMD_RST))
	{
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "%s: Sensor did not ACK soft reset.", __func__);
	}
	rtos_delay_milliseconds(20);
}

static bool AHT2X_TryInitializationCommand(uint8_t initCmd, const char *name)
{
	uint8_t status = AHT2X_DAT_BUSY;

	if(!AHT2X_WriteCommand3(initCmd, AHT2X_DAT_INI1, AHT2X_DAT_INI2))
	{
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "%s: %s init command was not ACKed.", __func__, name);
		return false;
	}

	if(!AHT2X_WaitReady(&status))
	{
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "%s: %s init timed out or sensor did not ACK status read.", __func__, name);
		return false;
	}

	if(!AHT2X_IsReadyAndCalibrated(status))
	{
		ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "%s: %s init returned unexpected status 0x%02X.", __func__, name, status);
		return false;
	}

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Initialization successful using %s command set, status 0x%02X.", __func__, name, status);
	return true;
}

void AHT2X_Initialization()
{
	isWorking = false;
	g_crcSupported = false;
	g_lastStatus = 0;

	AHT2X_SoftReset();

	/*
	 * Keep the original AHT2X/AHT20-style init first for OTA compatibility.
	 * AHT20/AHT21-style datasheets document 0xBE 0x08 0x00; AHT10/AHT15
	 * datasheets document 0xE1 0x08 0x00.  Falling back to 0xE1 fixes
	 * stricter AHT1x parts without changing existing AHT2X start commands.
	 */
	if(AHT2X_TryInitializationCommand(AHT2X_CMD_INI, "AHT2X-compatible"))
	{
		isWorking = true;
		return;
	}

	AHT2X_SoftReset();
	if(AHT2X_TryInitializationCommand(AHT2X_CMD_INI_AHT1X, "AHT1X-compatible"))
	{
		isWorking = true;
		return;
	}

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Initialization failed. Last status 0x%02X.", __func__, g_lastStatus);
}

void AHT2X_StopDriver()
{
	AHT2X_SoftReset();
}

void AHT2X_Measure()
{
	uint8_t data[AHT2X_READ_LEN_CRC] = { 0, };
	int readLen = (g_crcMode == AHT2X_CRC_MODE_OFF) ? AHT2X_READ_LEN : AHT2X_READ_LEN_CRC;
	bool ready = false;

	/* AHT-family measurement trigger documented as 0xAC 0x33 0x00. */
	if(!AHT2X_WriteCommand3(AHT2X_CMD_TMS, AHT2X_DAT_TMS1, AHT2X_DAT_TMS2))
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Sensor did not ACK measurement command.", __func__);
		isWorking = false;
		return;
	}

	/* AHT20/AHT30 datasheets specify an 80 ms conversion wait after trigger. */
	rtos_delay_milliseconds(80);

	for(uint8_t i = 0; i < 10; i++)
	{
		if(!AHT2X_ReadBytes(data, readLen))
		{
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Sensor did not ACK measurement read.", __func__);
			isWorking = false;
			return;
		}

		g_lastStatus = data[0];
		if((data[0] & AHT2X_DAT_BUSY) != AHT2X_DAT_BUSY)
		{
			ready = true;
			break;
		}
		else
		{
			ADDLOG_DEBUG(LOG_FEATURE_SENSOR, "%s: Sensor is busy, waiting... (%ims)", __func__, i * 20);
			rtos_delay_milliseconds(20);
		}
	}

	if(!ready)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Measurements reading timed out.", __func__);
		return;
	}

	if(!AHT2X_ValidateCrcIfPresent(data))
	{
		return;
	}

	if(data[1] == 0x0 && data[2] == 0x0 && (data[3] >> 4) == 0x0)
	{
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Unrealistic humidity, will not update values.", __func__);
		return;
	}

	uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
	uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];

	g_humid = ((float)raw_humidity * 100.0f / AHT2X_RAW_DIVISOR) + g_calHum;
	g_temp = (((200.0f * (float)raw_temperature) / AHT2X_RAW_DIVISOR) - 50.0f) + g_calTemp;

	/* Preserve the existing MQTT/HA path: same channel indexes and same scaling. */
	if(channel_temp >= 0)
	{
		CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	}
	if(channel_humid >= 0)
	{
		CHANNEL_Set(channel_humid, (int)(g_humid), 0);
	}

	isWorking = true;
	ADDLOG_INFO(LOG_FEATURE_SENSOR, "%s: Temperature:%fC Humidity:%f%%", __func__, g_temp, g_humid);
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
		ADDLOG_INFO(LOG_FEATURE_CMD, "%s: You must have at least 1 second cycle.", __func__);
		return CMD_RES_BAD_ARGUMENT;
	}
	else
	{
		g_aht_secondsBetweenMeasurements = seconds;
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "%s: Measurement will run every %i seconds.", __func__, g_aht_secondsBetweenMeasurements);

	return CMD_RES_OK;
}

commandResult_t AHT2X_CRC(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	int mode = Tokenizer_GetArgInteger(0);
	if(mode < AHT2X_CRC_MODE_OFF || mode > AHT2X_CRC_MODE_REQUIRED)
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "%s: Use 0=off, 1=auto, 2=require CRC.", __func__);
		return CMD_RES_BAD_ARGUMENT;
	}

	g_crcMode = (byte)mode;
	g_crcSupported = false;
	ADDLOG_INFO(LOG_FEATURE_CMD, "%s: CRC mode is %i (0=off, 1=auto, 2=require).", __func__, g_crcMode);

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

// startDriver AHT2X 4 5 3 4
void AHT2X_Init()
{
	g_softI2C.pin_clk = Tokenizer_GetPin(1, 9);
	g_softI2C.pin_data = Tokenizer_GetPin(2, 14);
	channel_temp = Tokenizer_GetArgIntegerDefault(3, -1);
	channel_humid = Tokenizer_GetArgIntegerDefault(4, -1);

	Soft_I2C_PreInit(&g_softI2C);
	rtos_delay_milliseconds(100);

	AHT2X_Initialization();

	//cmddetail:{"name":"AHT2X_Calibrate","args":"[DeltaTemp][DeltaHumidity]",
	//cmddetail:"descr":"Calibrate the AHT-family sensor reading without changing channel scaling.",
	//cmddetail:"fn":"AHT2X_Calibrate","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Calibrate -4 10 <br /> meaning -4 on current temp reading and +10 on current humidity reading"}
	CMD_RegisterCommand("AHT2X_Calibrate", AHT2X_Calibrate, NULL);
	//cmddetail:{"name":"AHT2X_Cycle","args":"[IntervalSeconds]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 10.",
	//cmddetail:"fn":"AHT2X_Cycle","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Cycle 60 <br /> measurement is taken every 60 seconds"}
	CMD_RegisterCommand("AHT2X_Cycle", AHT2X_Cycle, NULL);
	//cmddetail:{"name":"AHT2X_CRC","args":"[0/1/2]",
	//cmddetail:"descr":"Set AHT CRC handling. 0 = off/legacy six-byte read (default), 1 = automatic when a valid CRC byte is detected, 2 = require CRC validation.",
	//cmddetail:"fn":"AHT2X_CRC","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_CRC 1 <br /> enable automatic CRC validation where the sensor provides a valid CRC byte"}
	CMD_RegisterCommand("AHT2X_CRC", AHT2X_CRC, NULL);
	//cmddetail:{"name":"AHT2X_Measure","args":"",
	//cmddetail:"descr":"Retrieve OneShot measurement.",
	//cmddetail:"fn":"AHT2X_Force","file":"driver/drv_aht2x.c","requires":"",
	//cmddetail:"examples":"AHT2X_Measure"}
	CMD_RegisterCommand("AHT2X_Measure", AHT2X_Force, NULL);
	//cmddetail:{"name":"AHT2X_Reinit","args":"",
	//cmddetail:"descr":"Reinitialize sensor.",
	//cmddetail:"fn":"AHT2X_Reinit","file":"driver/drv_aht2x.c","requires":"",
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

void AHT2X_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState)
		return;
	hprintf255(request, "<h2>AHT2X Temperature=%.1fC, Humidity=%.0f%%</h2>", g_temp, g_humid);
	if(!isWorking)
	{
		hprintf255(request, "WARNING: AHT sensor appears to have failed initialization or communication, check if configured pins are correct!");
	}
	if(channel_humid == channel_temp)
	{
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}
