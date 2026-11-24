#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../logging/logging.h"
#include "../hal_bt_proxy.h"
#include "../hal_wifi.h"

//static SemaphoreHandle_t scan_mutex = NULL;

void __attribute__((weak)) HAL_BTProxy_StartScan()
{

}

void __attribute__((weak)) HAL_BTProxy_StopScan()
{

}

void __attribute__((weak)) HAL_BTProxy_Init(void)
{

}

void __attribute__((weak)) HAL_BTProxy_Deinit(void)
{
	HAL_BTProxy_StopScan();
}

void __attribute__((weak)) HAL_BTProxy_GetMAC(uint8_t* mac)
{
	WiFI_GetMacAddress((char*)mac);
}

void __attribute__((weak)) HAL_BTProxy_SetScanMode(bool isActive)
{

}

bool __attribute__((weak)) HAL_BTProxy_GetScanMode(void)
{
	return false;
}

void __attribute__((weak)) HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{

}

void __attribute__((weak)) HAL_BTProxy_OnEverySecond(void)
{

}

int __attribute__((weak)) HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets)
{
	return 0;
}

//void __attribute__((weak)) HAL_BTProxy_Lock(void)
//{
//	if(!scan_mutex) scan_mutex = xSemaphoreCreateMutex();
//	xSemaphoreTake(scan_mutex, 0xFFFFFFFF);
//}

//void __attribute__((weak)) HAL_BTProxy_Unlock(void)
//{
//	xSemaphoreGive(scan_mutex);
//}

void __attribute__((weak)) HAL_BTProxy_RegisterPlatformCommands(void)
{

}

bool __attribute__((weak)) HAL_BTProxy_IsInit(void)
{
	return false;
}

void __attribute__((weak)) HAL_BTProxy_FilterDuplicates(bool isActive)
{

}

#if ENABLE_BT_PROXY

static commandResult_t CMD_SetBTScanMode(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);

	int isActive = Tokenizer_GetArgIntegerDefault(0, 0);
	if(isActive != 0 && isActive != 1)
		return CMD_RES_BAD_ARGUMENT;
	HAL_BTProxy_SetScanMode(isActive);
	return CMD_RES_OK;
}

static commandResult_t CMD_SetWindowInterval(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);

	int window = Tokenizer_GetArgIntegerDefault(0, 30);
	int interval = Tokenizer_GetArgIntegerDefault(1, 50);
	if(window <= 0 || interval <= 0)
		return CMD_RES_BAD_ARGUMENT;
	HAL_BTProxy_SetWindowInterval(window, interval);
	return CMD_RES_OK;
}

static commandResult_t CMD_StartScan(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_StartScan();
	return CMD_RES_OK;
}

static commandResult_t CMD_StopScan(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_StopScan();
	return CMD_RES_OK;
}

static commandResult_t CMD_BTInit(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_Init();
	return CMD_RES_OK;
}

static commandResult_t CMD_BTDeinit(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_Deinit();
	return CMD_RES_OK;
}

static commandResult_t CMD_BTFilterDuplicates(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);

	int isActive = Tokenizer_GetArgIntegerDefault(0, 0);
	if(isActive != 0 && isActive != 1)
		return CMD_RES_BAD_ARGUMENT;
	HAL_BTProxy_FilterDuplicates(isActive);
	return CMD_RES_OK;
}

#if ENABLE_DRIVER_ESPHOME_API
#include "../../driver/drv_esphome_api.h"
#if ENABLE_BT_PROXY_GATT
static commandResult_t CMD_BTProxyConnect(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	// BTProxyConnect AA:BB:CC:DD:EE:FF [addrType] - debug: start a GATT connect locally
	Tokenizer_TokenizeString(args, 0);
	const char* mac = Tokenizer_GetArg(0);
	if(!mac) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	unsigned int b[6];
	if(sscanf(mac, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return CMD_RES_BAD_ARGUMENT;
	uint64_t addr = 0;
	for(int i = 0; i < 6; i++) addr = (addr << 8) | (b[i] & 0xFF);
	int type = Tokenizer_GetArgIntegerDefault(1, 0);
	HAL_BTProxy_GattConnect(addr, (uint8_t)type);
	return CMD_RES_OK;
}
static commandResult_t CMD_BTProxyDisconnect(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_GattDisconnectAll();
	return CMD_RES_OK;
}
static commandResult_t CMD_BTProxyStatus(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	HAL_BTProxy_LogStatus();
	return CMD_RES_OK;
}
#endif
static commandResult_t CMD_BTProxyLeds(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	int red = Tokenizer_GetArgIntegerDefault(0, -1);
	int blue = Tokenizer_GetArgIntegerDefault(1, -1);
	ESPHome_API_SetStatusLeds(red, blue);
	return CMD_RES_OK;
}
#endif

#endif

void HAL_BTProxy_RegisterCommands(void)
{
#if ENABLE_BT_PROXY
	//cmddetail:{"name":"BTSetScanMode","args":"[isActive]",
	//cmddetail:"descr":"Set BT scan mode: 1 = active (request scan responses; needed e.g. for Tuya BLE devices, which only advertise their protocol version in the scan response), 0 = passive. Can be used before the stack is ready (e.g. in the startup command); it is applied once BT is up.",
	//cmddetail:"fn":"CMD_SetBTScanMode","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTSetScanMode", CMD_SetBTScanMode, NULL);
	//cmddetail:{"name":"BTSetWindowInterval","args":"[window] [interval]",
	//cmddetail:"descr":"Set BT window and interval",
	//cmddetail:"fn":"CMD_SetWindowInterval","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTSetWindowInterval", CMD_SetWindowInterval, NULL);
	//cmddetail:{"name":"BTStartScan","args":"",
	//cmddetail:"descr":"BT start scan",
	//cmddetail:"fn":"CMD_StartScan","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTStartScan", CMD_StartScan, NULL);
	//cmddetail:{"name":"BTStopScan","args":"",
	//cmddetail:"descr":"BT stop scan",
	//cmddetail:"fn":"CMD_StopScan","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTStopScan", CMD_StopScan, NULL);
	//cmddetail:{"name":"BTInit","args":"",
	//cmddetail:"descr":"BT init",
	//cmddetail:"fn":"CMD_BTInit","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTInit", CMD_BTInit, NULL);
	//cmddetail:{"name":"BTDeinit","args":"",
	//cmddetail:"descr":"BT deinit",
	//cmddetail:"fn":"CMD_BTDeinit","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTDeinit", CMD_BTDeinit, NULL);
	//cmddetail:{"name":"BTFilterDuplicates","args":"[isActive]",
	//cmddetail:"descr":"BT filter duplicates",
	//cmddetail:"fn":"CMD_BTFilterDuplicates","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTFilterDuplicates", CMD_BTFilterDuplicates, NULL);
#if ENABLE_DRIVER_ESPHOME_API
	//cmddetail:{"name":"BTProxyLeds","args":"[redChannel] [blueChannel]",
	//cmddetail:"descr":"Status LEDs for the ESPHome BT proxy. Blue: Home Assistant connected. Red: blinking while connecting/discovering, solid while a BLE device is connected. -1 disables.",
	//cmddetail:"fn":"CMD_BTProxyLeds","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_DRIVER_ESPHOME_API",
	//cmddetail:"examples":"BTProxyLeds 60 61"}
	CMD_RegisterCommand("BTProxyLeds", CMD_BTProxyLeds, NULL);
#if ENABLE_BT_PROXY_GATT
	//cmddetail:{"name":"BTProxyStatus","args":"",
	//cmddetail:"descr":"Log BT proxy scan/link/task diagnostics",
	//cmddetail:"fn":"CMD_BTProxyStatus","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY_GATT",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTProxyStatus", CMD_BTProxyStatus, NULL);
	//cmddetail:{"name":"BTProxyConnect","args":"[MAC] [addrType]",
	//cmddetail:"descr":"Debug: open a BLE GATT connection locally (no API client needed)",
	//cmddetail:"fn":"CMD_BTProxyConnect","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY_GATT",
	//cmddetail:"examples":"BTProxyConnect AA:BB:CC:DD:EE:FF 0"}
	CMD_RegisterCommand("BTProxyConnect", CMD_BTProxyConnect, NULL);
	//cmddetail:{"name":"BTProxyDisconnect","args":"",
	//cmddetail:"descr":"Debug: disconnect all BLE GATT links",
	//cmddetail:"fn":"CMD_BTProxyDisconnect","file":"hal/generic/hal_bt_proxy_generic.c","requires":"ENABLE_BT_PROXY_GATT",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("BTProxyDisconnect", CMD_BTProxyDisconnect, NULL);
#endif
#endif
#endif

	HAL_BTProxy_RegisterPlatformCommands();
}
