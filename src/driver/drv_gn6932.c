// NOTE: qqq
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

int g_clk = 17;
int g_stb = 28;
int g_din = 15;

#define GN6932_DELAY usleep(1);

static void GN6932_WriteBytes(byte *bytes, int cnt) {
	int i, byteIndex, ack;
	byte b;

	HAL_PIN_SetOutputValue(g_clk, true);
	HAL_PIN_SetOutputValue(g_stb, false);
	GN6932_DELAY;
	for (byteIndex = 0; byteIndex < cnt; byteIndex++) {
		b = bytes[byteIndex];
		for (i = 0; i < 8; i++) {
			HAL_PIN_SetOutputValue(g_clk, false);
			GN6932_DELAY;
			HAL_PIN_SetOutputValue(g_din, b & 0x01);
			GN6932_DELAY;
			b >>= 1;
			HAL_PIN_SetOutputValue(g_clk, true);

		}
	}
	HAL_PIN_SetOutputValue(g_stb, true);

}
static void GN6932_WriteByte(byte b) {
	GN6932_WriteBytes(&b, 1);
}

static commandResult_t CMD_GN6932_TestAddressIncreaseMode(const void *context, const char *cmd, const char *args, int flags) {
	byte tmp[17];
	int i;

	for (i = 0; i < sizeof(tmp); i++) {
		tmp[i] = rand();
	}
	HAL_PIN_Setup_Output(g_stb);
	HAL_PIN_Setup_Output(g_clk);
	HAL_PIN_Setup_Output(g_din);
	HAL_PIN_SetOutputValue(g_clk, true);
	HAL_PIN_SetOutputValue(g_stb, true);
	HAL_PIN_SetOutputValue(g_din, true);
	usleep(100);
	// 0x40 is  100 0000
	// 0x44 is  100 0100
	// 0x8F is 1000 1111
	// 0xC0 is 1100 0000
	// 0xC1 is 1100 0001
	// 0xC2 is 1100 0010
	// Command 1
	GN6932_WriteByte(0x40);
	// Command 2 + data
	tmp[0] = 0xC0;
	// C0 66 3F 5B 6D 6F 3F 3F 3F 3F 8F 0F 00 00 00 00 00
	// first byte is offset, then 16 bytes of data
	GN6932_WriteBytes(tmp,sizeof(tmp));
	// Command 3
	GN6932_WriteByte(0x8F);

	return CMD_RES_OK;
}
static commandResult_t CMD_GN6932_Power(const void *context, const char *cmd, const char *args, int flags) {
	int bOn;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	bOn = Tokenizer_GetArgInteger(0);
	return CMD_RES_OK;
}
static commandResult_t CMD_GN6932_Dimmer(const void *context, const char *cmd, const char *args, int flags) {
	int iDimmer;
	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	iDimmer = Tokenizer_GetArgInteger(0);
	return CMD_RES_OK;
}
void GN6932_Init() {

	CMD_RegisterCommand("GN6932_TestAddressIncreaseMode", CMD_GN6932_TestAddressIncreaseMode, NULL);
	CMD_RegisterCommand("GN6932_Power", CMD_GN6932_Power, NULL);
	CMD_RegisterCommand("GN6932_Dimmer", CMD_GN6932_Dimmer, NULL);
}
