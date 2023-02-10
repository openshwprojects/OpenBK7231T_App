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





void SHT3X_StopPer() {
	Soft_I2C_Start(SHT3X_I2C_ADDR);
	// Stop Periodic Data
	Soft_I2C_WriteByte(0x30);
	// medium repeteability
	Soft_I2C_WriteByte(0x93);
	Soft_I2C_Stop();
}

void SM2135_StartPer(uint8_t msb, uint8_t lsb) {
	// Start Periodic Data capture
	Soft_I2C_Start(SHT3X_I2C_ADDR);
	// Measure per seconds
	Soft_I2C_WriteByte(msb);
	// repeteability
	Soft_I2C_WriteByte(lsb);
	Soft_I2C_Stop();
}

commandResult_t SHT3X_ChangePer(const void* context, const char* cmd, const char* args, int cmdFlags) {
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
	SM2135_StartPer(g_msb, g_lsb);

	ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Change Per : change done");

	return CMD_RES_OK;

}

commandResult_t SHT3X_Heater(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int g_state_heat;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater: 1 or 0 to activate");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_state_heat = Tokenizer_GetArgInteger(0);
	Soft_I2C_Start(SHT3X_I2C_ADDR);

	if (g_state_heat > 0) {
		// medium repeteability
		Soft_I2C_WriteByte(0x30);
		Soft_I2C_WriteByte(0x6D);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater activated");
	}
	else {
		// medium repeteability
		Soft_I2C_WriteByte(0x30);
		Soft_I2C_WriteByte(0x66);
		ADDLOG_INFO(LOG_FEATURE_SENSOR, "SHT Heater deactivated");
	}
	Soft_I2C_Stop();
	return CMD_RES_OK;
}

commandResult_t SHT3X_MeasurePer(const void* context, const char* cmd, const char* args, int cmdFlags) {

	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	Soft_I2C_Start(SHT3X_I2C_ADDR);
	// Ask for fetching data
	Soft_I2C_WriteByte(0xE0);
	// medium repeteability
	Soft_I2C_WriteByte(0x00);
	Soft_I2C_Stop();

	Soft_I2C_Start(SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(buff, 6);
	Soft_I2C_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	g_temp = 175 * ((th * 256 + tl) / 65535.0) - 45.0;

	g_humid = 100 * ((hh * 256 + hl) / 65535.0);

	g_temp = g_temp + g_caltemp;
	g_humid = g_humid + g_calhum;

	channel_temp = g_cfg.pins.channels[g_i2c_pin_data];
	channel_humid = g_cfg.pins.channels2[g_i2c_pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X_Measure: Period Temperature:%fC Humidity:%f%%", g_temp, g_humid);
	return CMD_RES_OK;

}
commandResult_t SHT3X_Measure(const void* context, const char* cmd, const char* args, int cmdFlags) {

	uint8_t buff[6];
	unsigned int th, tl, hh, hl;

	Soft_I2C_Start(SHT3X_I2C_ADDR);
	// no clock stretching
	Soft_I2C_WriteByte(0x24);
	// medium repeteability
	Soft_I2C_WriteByte(0x16);
	Soft_I2C_Stop();

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	Soft_I2C_Start(SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(buff, 6);
	Soft_I2C_Stop();

	th = buff[0];
	tl = buff[1];
	hh = buff[3];
	hl = buff[4];

	g_temp = 175 * ((th * 256 + tl) / 65535.0) - 45.0;

	g_humid = 100 * ((hh * 256 + hl) / 65535.0);

	g_temp = g_temp + g_caltemp;
	g_humid = g_humid + g_calhum;

	channel_temp = g_cfg.pins.channels[g_i2c_pin_data];
	channel_humid = g_cfg.pins.channels2[g_i2c_pin_data];
	CHANNEL_Set(channel_temp, (int)(g_temp * 10), 0);
	CHANNEL_Set(channel_humid, (int)(g_humid), 0);

	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X_Measure: Temperature:%fC Humidity:%f%%", g_temp, g_humid);
	return CMD_RES_OK;
}
// StopDriver SHT3X
void SHT3X_StopDriver() {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping Driver and reset sensor");
	SHT3X_StopPer();
	// Reset the sensor
	Soft_I2C_Start(SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(0x30);
	Soft_I2C_WriteByte(0xA2);
	Soft_I2C_Stop();
}

commandResult_t SHT3X_StopPerCmd(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT3X : Stopping periodical capture");
	SHT3X_StopPer();
	return CMD_RES_OK;
}

void SHT3X_GetStatus()
{
	uint8_t status[2];
	Soft_I2C_Start(SHT3X_I2C_ADDR);
	Soft_I2C_WriteByte(0xf3);			//Get Status should be 00000xxxxx00x0x0
	Soft_I2C_WriteByte(0x2d);          //Cheksum/Cmd_status/x/reset/res*5/Talert/RHalert/x/Heater/x/Alert
	Soft_I2C_Stop();
	Soft_I2C_Start(SHT3X_I2C_ADDR | 1);
	Soft_I2C_ReadBytes(status, 2);
	Soft_I2C_Stop();
	addLogAdv(LOG_INFO, LOG_FEATURE_SENSOR, "SHT : Status : %02X %02X", status[0], status[1]);
}
commandResult_t SHT3X_GetStatusCmd(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	SHT3X_GetStatus();
	return CMD_RES_OK;
}
// startDriver SHT3X
void SHT3X_Init() {



	g_i2c_pin_clk = 9;
	g_i2c_pin_data = 14;

	g_i2c_pin_clk = PIN_FindPinIndexForRole(IOR_SHT3X_CLK, g_i2c_pin_clk);
	g_i2c_pin_data = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, g_i2c_pin_data);

	Soft_I2C_PreInit();

	SHT3X_GetStatus();

	//cmddetail:{"name":"SHT_Calibrate","args":"",
	//cmddetail:"descr":"Calibrate the SHT Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"SHT3X_Calibrate","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Calibrate -4 10"}
	CMD_RegisterCommand("SHT_Calibrate", SHT3X_Calibrate, NULL);
	//cmddetail:{"name":"SHT_MeasurePer","args":"",
	//cmddetail:"descr":"Retrieve Periodical measurement for SHT",
	//cmddetail:"fn":"SHT3X_MeasurePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_MeasurePer", SHT3X_MeasurePer, NULL);
	//cmddetail:{"name":"SHT_LaunchPer","args":"[msb][lsb]",
	//cmddetail:"descr":"Launch/Change periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_ChangePer","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SHT_LaunchPer", SHT3X_ChangePer, NULL);
	//cmddetail:{"name":"SHT_StopPer","args":"",
	//cmddetail:"descr":"Stop periodical capture for SHT Sensor",
	//cmddetail:"fn":"SHT3X_StopPerCmd","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SHT_StopPer", SHT3X_StopPerCmd, NULL);
	//cmddetail:{"name":"SHT_Measure","args":"",
	//cmddetail:"descr":"Retrieve OneShot measurement for SHT",
	//cmddetail:"fn":"SHT3X_Measure","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Measure"}
	CMD_RegisterCommand("SHT_Measure", SHT3X_Measure, NULL);
	//cmddetail:{"name":"SHT_Heater","args":"",
	//cmddetail:"descr":"Activate or Deactivate Heater (0 / 1)",
	//cmddetail:"fn":"SHT3X_Heater","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_Heater 1"}
	CMD_RegisterCommand("SHT_Heater", SHT3X_Heater, NULL);
	//cmddetail:{"name":"SHT_GetStatus","args":"",
	//cmddetail:"descr":"Get Sensor Status",
	//cmddetail:"fn":"SHT3X_GetStatus","file":"driver/drv_sht3x.c","requires":"",
	//cmddetail:"examples":"SHT_GetStatusCmd"}
	CMD_RegisterCommand("SHT_GetStatus", SHT3X_GetStatusCmd, NULL);
}


void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>SHT3X Temperature=%f, Humidity=%f</h2>", g_temp, g_humid);
	if (channel_humid == channel_temp) {
		hprintf255(request, "WARNING: You don't have configured target channels for temp and humid results, set the first and second channel index in Pins!");
	}
}

