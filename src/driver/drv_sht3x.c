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

#include "drv_sht3x.h"

// Some platforms have less pins than BK7231T.
// For example, BL602 doesn't have pin number 26.
// The pin code would crash BL602 while trying to access pin 26.
// This is why the default settings here a per-platform.
#if PLATFORM_BEKEN
static int g_pin_clk = 9;
static int g_pin_data = 14;
#else
static int g_pin_clk = 0;
static int g_pin_data = 1;
#endif

#define SHT3X_I2C_ADDR (0x44 << 1)

static byte channel_temp = 0, channel_humid = 0;
static float g_temp = 0.0, g_humid = 0.0, g_caltemp = 0.0, g_calhum = 0.0;


commandResult_t SHT3X_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "Calibrate SHT: require Temp and Humidity args");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_caltemp = Tokenizer_GetArgFloat(0);
	g_calhum = Tokenizer_GetArgFloat(1);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "Calibrate SHT: Calibration done temp %f and humidity %f ", g_caltemp, g_calhum);

	return CMD_RES_OK;
}

static void SHT3X_SetLow(uint8_t pin) {
	HAL_PIN_Setup_Output(pin);
	HAL_PIN_SetOutputValue(pin, 0);
}

static void SHT3X_SetHigh(uint8_t pin) {
	HAL_PIN_Setup_Input_Pullup(pin);
}

static bool SHT3X_PreInit(void) {
	HAL_PIN_SetOutputValue(g_pin_data, 0);
	HAL_PIN_SetOutputValue(g_pin_clk, 0);
	SHT3X_SetHigh(g_pin_data);
	SHT3X_SetHigh(g_pin_clk);
	return (!((HAL_PIN_ReadDigitalInput(g_pin_data) == 0 || HAL_PIN_ReadDigitalInput(g_pin_clk) == 0)));
}

static bool SHT3X_WriteByte(uint8_t value) {
	uint8_t curr;
	uint8_t ack;

	for (curr = 0x80; curr != 0; curr >>= 1)
	{
		if (curr & value)
		{
			SHT3X_SetHigh(g_pin_data);
		}
		else
		{
			SHT3X_SetLow(g_pin_data);
		}
		SHT3X_SetHigh(g_pin_clk);
		usleep(SHT3X_DELAY);
		SHT3X_SetLow(g_pin_clk);
	}
	// get Ack or Nak
	SHT3X_SetHigh(g_pin_data);
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY / 2);
	ack = HAL_PIN_ReadDigitalInput(g_pin_data);
	SHT3X_SetLow(g_pin_clk);
	usleep(SHT3X_DELAY / 2);
	SHT3X_SetLow(g_pin_data);
	return (0 == ack);
}

static bool SHT3X_Start(uint8_t addr) {
	SHT3X_SetLow(g_pin_data);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_clk);
	return SHT3X_WriteByte(addr);
}

static void SHT3X_Stop(void) {
	SHT3X_SetLow(g_pin_data);
	usleep(SHT3X_DELAY);
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetHigh(g_pin_data);
	usleep(SHT3X_DELAY);
}

static uint8_t SHT3X_ReadByte(bool nack)
{
	uint8_t val = 0;

	SHT3X_SetHigh(g_pin_data);
	for (int i = 0; i < 8; i++)
	{
		usleep(SHT3X_DELAY);
		SHT3X_SetHigh(g_pin_clk);
		val <<= 1;
		if (HAL_PIN_ReadDigitalInput(g_pin_data))
		{
			val |= 1;
		}
		SHT3X_SetLow(g_pin_clk);
	}
	if (nack)
	{
		SHT3X_SetHigh(g_pin_data);
	}
	else
	{
		SHT3X_SetLow(g_pin_data);
	}
	SHT3X_SetHigh(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_clk);
	usleep(SHT3X_DELAY);
	SHT3X_SetLow(g_pin_data);

	return val;
}


static void SHT3X_ReadBytes(uint8_t* buf, int numOfBytes)
{

	for (int i = 0; i < numOfBytes - 1; i++)
	{

		buf[i] = SHT3X_ReadByte(false);

	}

	buf[numOfBytes - 1] = SHT3X_ReadByte(true); //Give NACK on last byte read
}

void SHT3X_StopPer() {
	SHT3X_Start(SHT3X_I2C_ADDR);
	// Stop Periodic Data
	SHT3X_WriteByte(0x30);
	// medium repeteability
	SHT3X_WriteByte(0x93);
	SHT3X_Stop();
}

void SHT3X_StartPer(uint8_t msb, uint8_t lsb) {
	// Start Periodic Data capture
	SHT3X_Start(SHT3X_I2C_ADDR);
	// Measure per seconds
	SHT3X_WriteByte(msb);
	// repeteability
	SHT3X_WriteByte(lsb);
	SHT3X_Stop();
}

void SHT3X_ChangePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	uint8_t g_msb, g_lsb;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Change Per: require MSB and LSB");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_msb = Tokenizer_GetArgInteger(0);
	g_lsb = Tokenizer_GetArgInteger(1);
	SHT3X_StopPer();
	//give some time for SHT to stop Periodicity
	rtos_delay_milliseconds(25);
	SHT3X_StartPer(g_msb, g_lsb);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Change Per : change done");

	return CMD_RES_OK;

}

void SHT3X_Heater(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int g_state_heat;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater: 1 or 0 to activate");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_state_heat = Tokenizer_GetArgInteger(0);
	SHT3X_Start(SHT3X_I2C_ADDR);

	if (g_state_heat > 0) {
		// medium repeteability
		SHT3X_WriteByte(0x30);
		SHT3X_WriteByte(0x6D);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater activated");
	}
	else {
		// medium repeteability
		SHT3X_WriteByte(0x30);
		SHT3X_WriteByte(0x66);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater deactivated");
	}
	SHT3X_Stop();
	return CMD_RES_OK;
}

void SHT3X_MeasurePer(const void* context, const char* cmd, const char* args, int cmdFlags) {

	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	SHT3X_Start(SHT3X_I2C_ADDR);
	// Ask for fetching data
	SHT3X_WriteByte(0xE0);
	// medium repeteability
	SHT3X_WriteByte(0x00);
	SHT3X_Stop();

	SHT3X_Start(SHT3X_I2C_ADDR | 1);
	SHT3X_ReadBytes(buff, 6);
	SHT3X_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	g_temp = 175 * ((th * 256 + tl) / 65535.0) - 45.0;

	g_humid = 100 * ((hh * 256 + hl) / 65535.0);

	g_temp = g_temp + g_caltemp;
	g_humid = g_humid + g_calhum;

	channel_temp = g_cfg.pins.channels[g_pin_data];
	channel_humid = g_cfg.pins.channels2[g_pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X_Measure: Period Temperature:%fC Humidity:%f%%\n", g_temp, g_humid);
	return CMD_RES_OK;

}
void SHT3X_Measure(const void* context, const char* cmd, const char* args, int cmdFlags) {

	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	SHT3X_Start(SHT3X_I2C_ADDR);
	// no clock stretching
	SHT3X_WriteByte(0x24);
	// medium repeteability
	SHT3X_WriteByte(0x16);
	SHT3X_Stop();

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	SHT3X_Start(SHT3X_I2C_ADDR | 1);
	SHT3X_ReadBytes(buff, 6);
	SHT3X_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	g_temp = 175 * ((th * 256 + tl) / 65535.0) - 45.0;

	g_humid = 100 * ((hh * 256 + hl) / 65535.0);

	g_temp = g_temp + g_caltemp;
	g_humid = g_humid + g_calhum;

	channel_temp = g_cfg.pins.channels[g_pin_data];
	channel_humid = g_cfg.pins.channels2[g_pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X_Measure: Temperature:%fC Humidity:%f%%\n", g_temp, g_humid);
	return CMD_RES_OK;
}
// StopDriver SHT3X
void SHT3X_StopDriver() {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping Driver and reset sensor");
	SHT3X_StopPer();
	// Reset the sensor
	SHT3X_Start(SHT3X_I2C_ADDR);
	SHT3X_WriteByte(0x30);
	SHT3X_WriteByte(0xA2);
	SHT3X_Stop();
}

void SHT3X_StopPerCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping periodical capture");
	SHT3X_StopPer();
	return CMD_RES_OK;
}

void SHT3X_GetStatus()
{
	uint8_t status[2];
	SHT3X_Start(SHT3X_I2C_ADDR);
	SHT3X_WriteByte(0xf3);			//Get Status should be 00000xxxxx00x0x0
	SHT3X_WriteByte(0x2d);          //Cheksum/Cmd_status/x/reset/res*5/Talert/RHalert/x/Heater/x/Alert
	SHT3X_Stop();
	SHT3X_Start(SHT3X_I2C_ADDR | 1);
	SHT3X_ReadBytes(status, 2);
	SHT3X_Stop();
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Status : %02X %02X\n", status[0], status[1]);
}
void SHT3X_GetStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SHT3X_GetStatus();
	return CMD_RES_OK;
}
// startDriver SHT3X
void SHT3X_Init() {

	SHT3X_PreInit();

	g_pin_clk = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, g_pin_clk);
	g_pin_data = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, g_pin_data);

	SHT3X_GetStatus();

	//cmddetail:{"name":"SHT_Calibrate","args":"",
	//cmddetail:"descr":"Calibrate the SHT Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"SHT3X_Calibrate","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Calibrate -4 10"}
	CMD_RegisterCommand("SHT_Calibrate", "", SHT3X_Calibrate, NULL, NULL);
	//cmddetail:{"name":"SHT_MeasurePer","args":"",
	//cmddetail:"descr":"Retrieve Periodical measurement for SHT",
	//cmddetail:"fn":"SHT3X_MeasurePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_MeasurePer", "", SHT3X_MeasurePer, NULL, NULL);
	//cmddetail:{"name":"SHT_Launch","args":"[msb][lsb]",
	//cmddetail:"descr":"Launch/Change periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_ChangePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_ChangePer 0x21 0x26"}
	CMD_RegisterCommand("SHT_LaunchPer", "", SHT3X_ChangePer, NULL, NULL);
	//cmddetail:{"name":"SHT_Launch","args":"",
	//cmddetail:"descr":"Stop periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_StopPerCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_StopPer"}
	CMD_RegisterCommand("SHT_StopPer", "", SHT3X_StopPerCmd, NULL, NULL);
	//cmddetail:{"name":"SHT_Measure","args":"",
	//cmddetail:"descr":"Retrieve OneShot measurement for SHT",
	//cmddetail:"fn":"SHT3X_Measure","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_Measure", "", SHT3X_Measure, NULL, NULL);
	//cmddetail:{"name":"SHT_Heater","args":"",
	//cmddetail:"descr":"Activate or Deactivate Heater (0 / 1)",
	//cmddetail:"fn":"SHT3X_Heater","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Heater 1"}
	CMD_RegisterCommand("SHT_Heater", "", SHT3X_Heater, NULL, NULL);
	//cmddetail:{"name":"SHT_GetStatus","args":"",
	//cmddetail:"descr":"Get Sensor Status",
	//cmddetail:"fn":"SHT3X_GetStatus","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_GetStatusCmd"}
	CMD_RegisterCommand("SHT_GetStatus", "", SHT3X_GetStatusCmd, NULL, NULL);
}

void SHT3X_OnChannelChanged(int ch, int value) {
}

void SHT3X_OnEverySecond() {

}

void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>SHT3X Temperature=%f, Humidity=%f</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}

