#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_bt_proxy.h"
#include "../../logging/logging.h"
#include "../hal_wifi.h"

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

int __attribute__((weak)) HAL_BTProxy_PopScanResult(uint8_t* mac, int* rssi, uint8_t* addr_type, uint8_t* data, int* data_len)
{
	return 0;
}

int __attribute__((weak)) HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets, int* buffered_packets)
{
	return 0;
}

int __attribute__((weak)) HAL_BTProxy_GetScanEntry(int newest_index, char* mac_buf, int mac_buf_len, int* rssi, int* adv_len, int* evt_type, int* age_ms)
{
	return 0;
}

void __attribute__((weak)) HAL_BTProxy_Lock(void)
{

}

void __attribute__((weak)) HAL_BTProxy_Unlock(void)
{

}
