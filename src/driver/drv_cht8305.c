#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "drv_cht83xx.h"

static byte g_cht_secondsUntilNextMeasurement = 1, g_cht_secondsBetweenMeasurements = 1;
static byte channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0;
static float g_calTemp = 0, g_calHum = 0;
static uint16_t sensor_id = 0;
static char* g_cht_sensor = "CHT8305";

static void CHT83XX_ReadEnv(float* temp, float* hum)
{
	uint8_t buff[4];

	if(IS_CHT831X)
	{
		//Oneshot measurement
		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_ONESHOT);
		Soft_I2C_WriteByte(&g_softI2C, 0x00);
		Soft_I2C_WriteByte(&g_softI2C, 0x00);
		Soft_I2C_Stop(&g_softI2C);
	}

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_TEMP);
	Soft_I2C_Stop(&g_softI2C);

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, buff, 4);
	Soft_I2C_Stop(&g_softI2C);

	//In case we have the new sensor 8310, overwrite humidity data reading it from 0x01, as it cannot be directrly read from 0x00, there is parity
	if(IS_CHT831X)
	{
		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_HUM);
		Soft_I2C_Stop(&g_softI2C);

		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR | 1);
		Soft_I2C_ReadBytes(&g_softI2C, buff + 2, 2);
		Soft_I2C_Stop(&g_softI2C);
		int16_t temp_val = (buff[0] << 8 | buff[1]);
		// >> 3 = 13bit resolution
		(*temp) = (float)(temp_val >> 3) * 0.03125 + g_calTemp;
		(*hum) = ((((buff[2] << 8 | buff[3]) & 0x7fff) / 32768.0) * 100.0) + g_calHum;
		return;
	}

	(*temp) = ((buff[0] << 8 | buff[1]) * 165.0 / 65535.0 - 40.0) + g_calTemp;
	(*hum) = ((buff[2] << 8 | buff[3]) * 100.0 / 65535.0) + g_calHum;
}

void CHT831X_ConfigureAlert(float temp_diff, float hum_diff, CHT_alert_freq freq, CHT_alert_fq fq)
{
	uint8_t mode = SRC_TEMP_AND_HUM;
	if(temp_diff > 0.05) mode ^= 0b10;
	if(hum_diff > 0.1) mode ^= 0b01;
	// technically we can support this mode
	if(mode == SRC_TEMP_AND_HUM)
	{
		ADDLOG_ERROR(LOG_FEATURE_CMD, "CHT_Alert: wrong mode, too low TempDiff && HumDiff");
		return;
	}
	if(mode != SRC_HUM_ONLY)
	{
		int16_t hight = (int)((g_temp - g_calTemp + temp_diff) / 0.03125) << 3;
		int16_t lowt = (int)((g_temp - g_calTemp - temp_diff) / 0.03125) << 3;
		WriteReg(CHT831X_REG_TEMP_HL, hight);
		WriteReg(CHT831X_REG_TEMP_LL, lowt);
	}
	if(mode != SRC_TEMP_ONLY)
	{
		int16_t humh = (int)round((g_humid - g_calHum + hum_diff) * 327.67);
		int16_t huml = (int)round((g_humid - g_calHum - hum_diff) * 327.67);

		WriteReg(CHT831X_REG_HUM_HL, humh);
		WriteReg(CHT831X_REG_HUM_LL, huml);
	}

	uint8_t conf = 0x80 | (POL_AL << 5) | (mode << 3) | (fq << 1) | 1;
	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_CFG);
	Soft_I2C_WriteByte(&g_softI2C, 0x08);
	Soft_I2C_WriteByte(&g_softI2C, conf);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_C_RATE);
	Soft_I2C_WriteByte(&g_softI2C, (uint8_t)freq);
	Soft_I2C_Stop(&g_softI2C);
}

commandResult_t CHT83XX_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_calTemp = Tokenizer_GetArgFloat(0);
	g_calHum = Tokenizer_GetArgFloat(1);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "Calibrate CHT: Calibration done temp %f and humidity %f ", g_calTemp, g_calHum);

	return CMD_RES_OK;
}

commandResult_t CHT83XX_Cycle(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_cht_secondsBetweenMeasurements = Tokenizer_GetArgInteger(0);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Measurement will run every %i seconds", g_cht_secondsBetweenMeasurements);

	return CMD_RES_OK;
}

commandResult_t CHT83XX_Alert(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);

	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	if(!IS_CHT831X)
	{
		ADDLOG_ERROR(LOG_FEATURE_CMD, "ALERT is not supported on CHT8305");
		return CMD_RES_ERROR;
	}
	float difft = Tokenizer_GetArgFloat(0);
	float diffh = Tokenizer_GetArgFloat(1);
	int tfreq = Tokenizer_GetArgInteger(2);
	int tfq = Tokenizer_GetArgInteger(3);
	CHT_alert_freq freq;
	CHT_alert_fq fq;
	switch(tfreq)
	{
	case 120:
		freq = FREQ_120S;
		break;
	case 60:
		freq = FREQ_60S;
		break;
	case 10:
		freq = FREQ_10S;
		break;
	case 5:
		freq = FREQ_5S;
		break;
	default:
		ADDLOG_WARN(LOG_FEATURE_CMD, "CHT83XX_Alert: Wrong freq, using def (1s)");
	case 1:
		freq = FREQ_1S;
		break;
	}

	switch(tfq)
	{
	case 6:
		fq = FQ_6;
		break;
	case 4:
		fq = FQ_4;
		break;
	case 2:
		fq = FQ_2;
		break;
	default:
		ADDLOG_WARN(LOG_FEATURE_CMD, "CHT83XX_Alert: Wrong fault queue, using def (1)");
	case 1:
		fq = FQ_1;
		break;
	}

	CHT831X_ConfigureAlert(difft, diffh, freq, fq);

	return CMD_RES_OK;
}

// startDriver CHT83XX
void CHT83XX_Init()
{
	uint8_t buff[4];

	g_softI2C.pin_clk = 9;
	g_softI2C.pin_data = 14;
	g_softI2C.pin_clk = PIN_FindPinIndexForRole(IOR_CHT83XX_CLK, g_softI2C.pin_clk);
	g_softI2C.pin_data = PIN_FindPinIndexForRole(IOR_CHT83XX_DAT, g_softI2C.pin_data);

	Soft_I2C_PreInit(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_ID); //manufacturer ID 2 bytes
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, buff, 2);
	Soft_I2C_Stop(&g_softI2C);

	//Read Sensor version separately on the last 2 bytes of the buffer
	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
	Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_ID + 1);
	Soft_I2C_Stop(&g_softI2C);

	Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(&g_softI2C, buff + 2, 2);
	Soft_I2C_Stop(&g_softI2C);

	//Identify chip ID and keep if for later use
	sensor_id = (buff[2] << 8 | buff[3]);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "DRV_CHT83XX_Init: ID: %02X %02X %04X", buff[0], buff[1], sensor_id);

	if(IS_CHT831X)
	{
		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_STATUS);
		Soft_I2C_Stop(&g_softI2C);

		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR | 1);
		uint8_t status = Soft_I2C_ReadByte(&g_softI2C, 1);
		Soft_I2C_Stop(&g_softI2C);
		if(status != 0)
		{
			ADDLOG_INFO(LOG_FEATURE_SENSOR, "CHT83XX wake: th:%i,tl:%i,hh:%i,hl:%i", (status & (1 << 6)) > 0,
				(status & (1 << 5)) > 0, (status & (1 << 4)) > 0, (status & (1 << 3)) > 0);
		}
		//it should be 8310 id is 0x8215, we enable low power mode, so only 50nA are drawn from sensor,
		//but need to write something to one shot register to trigger new measurement
		Soft_I2C_Start(&g_softI2C, CHT83XX_I2C_ADDR);
		Soft_I2C_WriteByte(&g_softI2C, CHT831X_REG_CFG);	//config register
		Soft_I2C_WriteByte(&g_softI2C, 0x48);	//enable shutdown default is 0x08
		//setting bit 6 enables low power mode,
		//to get new measurement need to write to one shot register
		Soft_I2C_WriteByte(&g_softI2C, 0x80);	//as default 0x80
		Soft_I2C_Stop(&g_softI2C);
	}

	switch(sensor_id)
	{
	case 0x8215:
		g_cht_sensor = "CHT8310";
		break;
	case 0x8315:
		g_cht_sensor = "CHT8315";
		break;
	default:
		g_cht_sensor = "CHT8305";
		break;
	}

	//cmddetail:{"name":"CHT_Calibrate","args":"[DeltaTemp][DeltaHumidity]",
	//cmddetail:"descr":"Calibrate the CHT Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"CHT_Calibrate","file":"driver/drv_cht8305.c","requires":"",
	//cmddetail:"examples":"CHT_Calibrate -4 10 <br /> meaning -4 on current temp reading and +10 on current humidity reading"}
	CMD_RegisterCommand("CHT_Calibrate", CHT83XX_Calibrate, NULL);
	//cmddetail:{"name":"CHT_Cycle","args":"[IntervalSeconds]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
	//cmddetail:"fn":"CHT_cycle","file":"drv/drv_cht8305.c","requires":"",
	//cmddetail:"examples":"CHT_Cycle 60 <br /> measurement is taken every 60 seconds"}
	CMD_RegisterCommand("CHT_Cycle", CHT83XX_Cycle, NULL);
	//cmddetail:{"name":"CHT_Alert","args":"[TempDiff][HumDiff][Freq][FQ]",
	//cmddetail:"descr":"Enable alert pin. TempDif (temperature difference is any float higher than 0.05°C) = set detected difference in temperature required for device to wake. HumDiff (humidity difference is any float higher than 0.1%). Freq (time per measurement in s) = 1, 5, 10, 60, 120 (default if wrong = 1). FQ (fault queue number) = 1, 2, 4, 6 (default if wrong = 1)",
	//cmddetail:"fn":"CHT_Alert","file":"drv/drv_cht8305.c","requires":"",
	//cmddetail:"examples":"CHT_Alert 0.5 0 5 2 <br /> alert pin will trigger when temperature deviates by more than 0.5°C. Humidity will be ignored. Sensor measures every 5s with fault queue number 2."}
	CMD_RegisterCommand("CHT_Alert", CHT83XX_Alert, NULL);
}

void CHT83XX_Measure()
{
	CHT83XX_ReadEnv(&g_temp, &g_humid);

	channel_temp = g_cfg.pins.channels[g_softI2C.pin_data];
	channel_humid = g_cfg.pins.channels2[g_softI2C.pin_data];
	// don't want to loose accuracy, so multiply by 10
	// We have a channel types to handle that
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "DRV_CHT83XX_ReadEnv: Temperature:%fC Humidity:%f%%", g_temp, g_humid);
}

void CHT83XX_OnEverySecond()
{
	if(g_cht_secondsUntilNextMeasurement <= 0)
	{
		CHT83XX_Measure();
		g_cht_secondsUntilNextMeasurement = g_cht_secondsBetweenMeasurements;
	}
	if(g_cht_secondsUntilNextMeasurement > 0)
	{
		g_cht_secondsUntilNextMeasurement--;
	}
}

void CHT83XX_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>%s Temperature=%f, Humidity=%f</h2>", g_cht_sensor, g_temp, g_humid);
	if(channel_humid == channel_temp)
	{
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}
