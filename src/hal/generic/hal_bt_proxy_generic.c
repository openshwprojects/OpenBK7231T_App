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

#endif

void HAL_BTProxy_RegisterCommands(void)
{
#if ENABLE_BT_PROXY
	//cmddetail:{"name":"BTSetScanMode","args":"[isActive]",
	//cmddetail:"descr":"Set BT scan mode",
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
	//cmddetail:{"name":"BTStartScan","args":"",
	//cmddetail:"descr":"BT start scan",
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
#endif

	HAL_BTProxy_RegisterPlatformCommands();
}
