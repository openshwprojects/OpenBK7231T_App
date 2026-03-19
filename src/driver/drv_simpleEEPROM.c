/*
startDriver SimpleEEPROM 5 4
EEPROM_WriteHex 0 AABBCCDDEEFFBAADF00DAABB
EEPROM_Read 0 32

*/
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"

#define EEPROM_ADDR (0x50<<1)
#define EEPROM_PAGE 16
#define EEPROM_ADDR_BITS 2

static softI2C_t g_eepI2C;
static int g_eepBusy = 0;

static int EEP_WritePage(uint16_t addr, const uint8_t*data, int len) {
	if (len <= 0)return 0;
	Soft_I2C_Start(&g_eepI2C, EEPROM_ADDR);
	if (EEPROM_ADDR_BITS == 2) {
		Soft_I2C_WriteByte(&g_eepI2C, (addr >> 8) & 0xFF);
	}
	Soft_I2C_WriteByte(&g_eepI2C, addr & 0xFF);
	for (int i = 0; i < len; i++)Soft_I2C_WriteByte(&g_eepI2C, data[i]);
	Soft_I2C_Stop(&g_eepI2C);
	rtos_delay_milliseconds(5);
	return len;
}

static int EEPROM_Write(uint16_t addr, const uint8_t*data, int len) {
	int w = 0;
	while (len > 0) {
		int p = EEPROM_PAGE - (addr%EEPROM_PAGE);
		if (p > len)p = len;
		int r = EEP_WritePage(addr, data, p);
		if (r <= 0)break;
		addr += r;
		data += r;
		len -= r;
		w += r;
	}
	return w;
}

static int EEPROM_Read(uint16_t addr, uint8_t*data, int len) {
	Soft_I2C_Start(&g_eepI2C, EEPROM_ADDR);
	if (EEPROM_ADDR_BITS == 2) {
		Soft_I2C_WriteByte(&g_eepI2C, (addr >> 8) & 0xFF);
	}
	Soft_I2C_WriteByte(&g_eepI2C, addr & 0xFF);
	Soft_I2C_Stop(&g_eepI2C);
	rtos_delay_milliseconds(1);
	Soft_I2C_Start(&g_eepI2C, EEPROM_ADDR | 1);
	Soft_I2C_ReadBytes(&g_eepI2C, data, len);
	Soft_I2C_Stop(&g_eepI2C);
	return len;
}

commandResult_t EEPROM_ReadCmd(const void*ctx, const char*cmd, const char*args, int f) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_GetArgsCount() < 2) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	uint16_t addr = Tokenizer_GetArgInteger(0);
	int len = Tokenizer_GetArgInteger(1);

	uint8_t *buf = (uint8_t*)malloc(len);
	if (!buf) return CMD_RES_ERROR;

	EEPROM_Read(addr, buf, len);

	int outLen = len * 3 + 1;
	char *out = (char*)malloc(outLen);
	if (!out) {
		free(buf);
		return CMD_RES_ERROR;
	}

	int p = 0;
	for (int i = 0; i < len; i++) p += sprintf(out + p, "%02X ", buf[i]);

	ADDLOG_INFO(LOG_FEATURE_CMD, "EEPROM_Read %s", out);

	free(buf);
	free(out);

	return CMD_RES_OK;
}


commandResult_t EEPROM_WriteHexCmd(const void*ctx, const char*cmd, const char*args, int f) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_GetArgsCount() < 2)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	uint16_t addr = Tokenizer_GetArgInteger(0);
	const char* hex = args+strlen(Tokenizer_GetArg(0));
	while (*hex == ' ')
		hex++;

	const char* p = hex;
	int count = 0;
	while (p[0] && p[1]) {
		unsigned int b;
		if (sscanf(p, "%02x", &b) != 1) break;
		count++;
		p += 2;
		while (*p == ' ')
			p++;
	}

	if (count == 0)
		return CMD_RES_BAD_ARGUMENT;

	uint8_t* buf = (uint8_t*)malloc(count);
	if (!buf)
		return CMD_RES_ERROR;

	p = hex;
	int len = 0;
	while (p[0] && p[1] && len < count) {
		unsigned int b;
		if (sscanf(p, "%02x", &b) != 1) break;
		buf[len++] = b;
		p += 2;
		while (*p == ' ')
			p++;
	}

	EEPROM_Write(addr, buf, len);
	free(buf);

	ADDLOG_INFO(LOG_FEATURE_CMD, "EEPROM_Write %d bytes", len);
	return CMD_RES_OK;
}

commandResult_t EEPROM_DumpCmd(const void*ctx, const char*cmd, const char*args, int f) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2)return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	uint16_t addr = Tokenizer_GetArgInteger(0);
	int len = Tokenizer_GetArgInteger(1);
	if (len <= 0 || len > 512)return CMD_RES_BAD_ARGUMENT;
	uint8_t buf[512];
	EEPROM_Read(addr, buf, len);
	for (int i = 0; i < len; i += 16) {
		char out[128]; int p = 0;
		p += sprintf(out + p, "%04X: ", addr + i);
		for (int j = 0; j < 16 && i + j < len; j++)p += sprintf(out + p, "%02X ", buf[i + j]);
		ADDLOG_INFO(LOG_FEATURE_CMD, "%s", out);
	}
	return CMD_RES_OK;
}

// startDriver SimpleEEPROM 5 4
// EEPROM_Write 0 AABBCC
// EEPROM_Read 0 16
void EEPROM_Init() {
//	g_eepI2C.pin_clk = Tokenizer_GetArgIntegerDefault(1, g_eepI2C.pin_clk); 
//	g_eepI2C.pin_data = Tokenizer_GetArgIntegerDefault(2, g_eepI2C.pin_data);
	g_eepI2C.pin_clk = Tokenizer_GetPin(1, g_eepI2C.pin_clk); 
	g_eepI2C.pin_data = Tokenizer_GetPin(2, g_eepI2C.pin_data);
	Soft_I2C_PreInit(&g_eepI2C);
	//cmddetail:{"name":"EEPROM_Read","args":"TODO",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"EEPROM_ReadCmd","file":"driver/drv_simpleEEPROM.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("EEPROM_Read", EEPROM_ReadCmd, NULL);
	//cmddetail:{"name":"EEPROM_WriteHex","args":"TODO",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"EEPROM_WriteHexCmd","file":"driver/drv_simpleEEPROM.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("EEPROM_WriteHex", EEPROM_WriteHexCmd, NULL);
	//cmddetail:{"name":"EEPROM_Dump","args":"TODO",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"EEPROM_DumpCmd","file":"driver/drv_simpleEEPROM.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("EEPROM_Dump", EEPROM_DumpCmd, NULL);
}

void EEPROM_OnEverySecond() {
}

void EEPROM_AppendInformationToHTTPIndexPage(http_request_t*r, int pre) {
	if (pre)return;
	hprintf255(r, "<h2>EEPROM driver active</h2>");
}
