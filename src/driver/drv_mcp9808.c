// based on MCP9808_RT by RobTillaart
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

static softI2C_t g_softI2C;
static byte g_addr = 30;
static float g_temp = 0.0f;
static int g_mcp_secondsBetweenMeasurements = 10;
static int g_mcp_secondsUntilNextMeasurement = 0;


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
float MCP9808_WriteFloat(uint8_t reg, float f)
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
	g_temp = MCP9808_ReadFloat(MCP9808_TA);

}
commandResult_t MCP9808_Adr(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_addr = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
// startDriver MCP9808
void MCP9808_Init() {

	uint8_t buff[4];


	g_softI2C.pin_clk = 26;
	g_softI2C.pin_data = 24;

	Soft_I2C_PreInit(&g_softI2C);

	CMD_RegisterCommand("MCP9808_Adr", MCP9808_Adr, NULL);

	//cmddetail:{"name":"MCP9808_Calibrate","args":"",
	//cmddetail:"descr":"Calibrate the MCP9808 Sensor as Tolerance is +/-2 degrees C.",
	//cmddetail:"fn":"MCP9808_Calibrate","file":"driver/drv_MCP98088305.c","requires":"",
	//cmddetail:"examples":"SHT_Calibrate -4 10"}
	//CMD_RegisterCommand("MCP9808_Calibrate", MCP9808_Calibrate, NULL);
	//cmddetail:{"name":"MCP9808_Cycle","args":"[int]",
	//cmddetail:"descr":"This is the interval between measurements in seconds, by default 1. Max is 255.",
	//cmddetail:"fn":"MCP9808_cycle","file":"drv/drv_MCP98088305.c","requires":"",
	//cmddetail:"examples":"MCP9808_Cycle 60"}
	//CMD_RegisterCommand("MCP9808_Cycle", MCP9808_cycle, NULL);

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

