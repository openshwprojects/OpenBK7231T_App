// based on MCP9808_RT by RobTillaart
// See also: https://www.elektroda.pl/rtvforum/topic3988466.html
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

#define MCP9808_RFU     0x00
#define MCP9808_CONFIG  0x01
#define MCP9808_TUPPER  0x02
#define MCP9808_TLOWER  0x03
#define MCP9808_TCRIT   0x04
#define MCP9808_TA      0x05
#define MCP9808_MID     0x06
#define MCP9808_DID     0x07
#define MCP9808_RES     0x08

static int g_targetChannel = -1;
static softI2C_t g_softI2C;
static byte g_addr = 30;
static float g_temp = 0.0f;
static int g_mcp_secondsBetweenMeasurements = 10;
static int g_mcp_secondsUntilNextMeasurement = 0;
static float g_mcp_userCalibrationDelta = 0.0f;

void MCP9808_WriteReg16(uint8_t reg, uint16_t value)
{
	if (reg > MCP9808_RES)
		return;      //  see p.16
	Soft_I2C_Start(&g_softI2C, g_addr);
	Soft_I2C_WriteByte(&g_softI2C, reg);
	Soft_I2C_WriteByte(&g_softI2C, value >> 8);
	Soft_I2C_WriteByte(&g_softI2C, value & 0xFF);
	Soft_I2C_Stop(&g_softI2C);
}
uint16_t MCP9808_ReadReg16(uint8_t reg)
{
	byte reply[2];

	Soft_I2C_Start(&g_softI2C, g_addr);
	Soft_I2C_WriteByte(&g_softI2C, reg);
	Soft_I2C_Stop(&g_softI2C);

	rtos_delay_milliseconds(20);	//give the sensor time to do the conversion

	Soft_I2C_Start(&g_softI2C, g_addr | 1);
	Soft_I2C_ReadBytes(&g_softI2C, reply, 2);
	Soft_I2C_Stop(&g_softI2C);

	uint16_t val = reply[0] << 8;
	val += reply[1];
	return val;
}
float MCP9808_ReadFloat(uint8_t reg)
{
	uint16_t val = MCP9808_ReadReg16(reg);
	if (val & 0x1000)         //  negative value
	{
		return ((val & 0x0FFF) * 0.0625) - 256.0;
	}
	return (val & 0x0FFF) * 0.0625;
}
void MCP9808_WriteFloat(uint8_t reg, float f)
{
	bool neg = (f < 0.0);
	if (neg)
		f = -f;
	uint16_t val = ((uint16_t)(f * 4 + 0.5)) * 4;
	if (neg)
		val |= 0x1000;
	MCP9808_WriteReg16(reg, val);
}

void MCP9808_SetConfigRegister(uint16_t configuration)
{
	MCP9808_WriteReg16(MCP9808_CONFIG, configuration);
}

uint16_t MCP9808_GetConfigRegister()
{
	return MCP9808_ReadReg16(MCP9808_CONFIG);
}

void  MCP9808_SetTupper(float temperature)
{
	MCP9808_WriteFloat(MCP9808_TUPPER, temperature);
}

float MCP9808_GetTupper()
{
	return MCP9808_ReadFloat(MCP9808_TUPPER);
}

void  MCP9808_SetTlower(float temperature)
{
	MCP9808_WriteFloat(MCP9808_TLOWER, temperature);
}


float MCP9808_GetTlower()
{
	return MCP9808_ReadFloat(MCP9808_TLOWER);
}
void  MCP9808_SetTcritical(float temperature)
{
	MCP9808_WriteFloat(MCP9808_TCRIT, temperature);
}


float MCP9808_GetTcritical()
{
	return MCP9808_ReadFloat(MCP9808_TCRIT);
}

void MCP9808_Measure() {
	g_temp = MCP9808_ReadFloat(MCP9808_TA) + g_mcp_userCalibrationDelta;
	if (g_targetChannel >= 0) {
		int type = CHANNEL_GetType(g_targetChannel);
		if (type == ChType_Temperature_div10) {
			CHANNEL_Set(g_targetChannel, g_temp * 10, 0);
		}
		else {
			CHANNEL_Set(g_targetChannel, g_temp, 0);
		}
	}
}
commandResult_t MCP9808_Adr(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_addr = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
commandResult_t MCP9808_Calibrate(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_mcp_userCalibrationDelta = Tokenizer_GetArgFloat(0);


	return CMD_RES_OK;
}
commandResult_t MCP9808_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_mcp_secondsBetweenMeasurements = Tokenizer_GetArgInteger(0);


	return CMD_RES_OK;
}
// MCP9808_AlertRange [MinT] [MaxT] [OptionalBActiveHigh]
// MCP9808_AlertRange 20 26 1
commandResult_t MCP9808_AlertRange(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float min, max;
	int bActiveHigh;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	min = Tokenizer_GetArgFloat(0);
	max = Tokenizer_GetArgFloat(1);
	bActiveHigh = Tokenizer_GetArgIntegerDefault(2, 0);
	MCP9808_SetTlower(min);
	MCP9808_SetTupper(max);
	MCP9808_SetTcritical(max);

	// SET ALERT PARAMETERS
	uint16_t cfg = MCP9808_GetConfigRegister();
	cfg &= ~0x0001;      // set comparator mode
	if (bActiveHigh) {
		cfg &= ~0x0002;      // set polarity HIGH
	}
	else {
		cfg |= 0x0002;       // set polarity LOW
	}
	cfg &= ~0x0004;      // use upper lower and critical
	cfg |= 0x0008;       // enable alert
	MCP9808_SetConfigRegister(cfg);

	return CMD_RES_OK;
}
// MCP9808_AlertMin [MinT] [OptionalBActiveHigh]
// MCP9808_AlertMin 20 1
commandResult_t MCP9808_AlertMin(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float min;
	int bActiveHigh;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	min = Tokenizer_GetArgFloat(0);
	bActiveHigh = Tokenizer_GetArgIntegerDefault(1, 0);
	MCP9808_SetTcritical(min);

	// SET ALERT PARAMETERS
	uint16_t cfg = MCP9808_GetConfigRegister();
	cfg &= ~0x0001;      // set comparator mode
	if (bActiveHigh) {
		cfg &= ~0x0002;      // set polarity HIGH
	}
	else {
		cfg |= 0x0002;       // set polarity LOW
	}
	// TA > TCRIT only
	cfg |= 0x0004;      // use only critical
	cfg |= 0x0008;       // enable alert
	MCP9808_SetConfigRegister(cfg);

	return CMD_RES_OK;
}
// driver initialization
// startDriver MCP9808 [ClkPin] [DatPin] [OptionalTargetChannel]
// startDriver MCP9808 26 24
// startDriver MCP9808 26 24 1
// startDriver MCP9808 12 13 1

// Sample config for module from here: https://www.elektroda.pl/rtvforum/topic3988466.html
/*
startDriver MCP9808 7 8 1
MCP9808_Adr 0x30
MCP9808_Cycle 1
*/
void MCP9808_Init() {

	//uint8_t buff[4];

	g_softI2C.pin_clk = Tokenizer_GetArgIntegerDefault(1, 26);
	g_softI2C.pin_data = Tokenizer_GetArgIntegerDefault(2, 24);
	g_targetChannel = Tokenizer_GetArgIntegerDefault(3,-1);

	Soft_I2C_PreInit(&g_softI2C);

	//cmddetail:{"name":"MCP9808_Adr","args":"MCP9808_Adr",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_mcp9808.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP9808_Adr", MCP9808_Adr, NULL);
	
	//cmddetail:{"name":"MCP9808_AlertRange","args":"MCP9808_AlertRange",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_mcp9808.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP9808_AlertRange", MCP9808_AlertRange, NULL);
	//cmddetail:{"name":"MCP9808_AlertMin","args":"MCP9808_AlertMin",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_mcp9808.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP9808_AlertMin", MCP9808_AlertMin, NULL);


	//cmddetail:{"name":"MCP9808_Calibrate","args":"[DeltaTemperature]",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_mcp9808.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP9808_Calibrate", MCP9808_Calibrate, NULL);
	//cmddetail:{"name":"MCP9808_Cycle","args":"[DelayInSeconds]",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_mcp9808.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP9808_Cycle", MCP9808_cycle, NULL);

}
void MCP9808_OnEverySecond() {

	if (g_mcp_secondsUntilNextMeasurement <= 0) {
		MCP9808_Measure();
		g_mcp_secondsUntilNextMeasurement = g_mcp_secondsBetweenMeasurements;
	}
	if (g_mcp_secondsUntilNextMeasurement > 0) {
		g_mcp_secondsUntilNextMeasurement--;
	}
}

void MCP9808_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>MCP9808 Temperature=%f</h2>", g_temp);
}

