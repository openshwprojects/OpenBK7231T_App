#if PLATFORM_BEKEN_NEW

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_bt_proxy.h"
#include "../../logging/logging.h"
#include "../../driver/drv_esphome_api.h"
#include <string.h>
#include <stdio.h>

#include "include.h"

#if ENABLE_BT_PROXY

#include "ble_api.h"

#if !PLATFORM_BK7238
#include "common_bt_defines.h"
extern struct bd_addr common_default_bdaddr;
#endif

typedef struct
{
	bool init_done;
	bool scan_active;
	int scan_total_packets;
	int scan_dropped_packets;

	//QueueHandle_t cmd_queue;
	TaskHandle_t task_handle;
} hal_bt_state_t;

extern void WiFI_GetMacAddress(char* mac);
extern unsigned int g_timeMs;
static hal_bt_state_t g_bt_proxy = { 0 };

static int g_bleStackReady = 0;
static uint8_t g_bleScanActvIdx = 0xFF;
static uint16_t g_bleScanInterval = 100;
static uint16_t g_bleScanWindow = 30;
static bool scan_mode = false; // passive - false, active - true

static void hal_bt_gap_callback(ble_notice_t notice, void* param)
{
	if(!param) return;
	if(notice == BLE_5_REPORT_ADV)
	{
		recv_adv_t* info = (recv_adv_t*)param;
		g_bt_proxy.scan_total_packets++;
		if(!ESPHome_API_PassScanResult(info->adv_addr, (int)(int8_t)info->rssi, info->adv_addr_type, info->data, info->data_len))
		{
			g_bt_proxy.scan_dropped_packets++;
		}
	}
}

static void HAL_BTScan_EnsureStackReady()
{
	if(g_bleStackReady)
	{
		return;
	}

	extern void ble_entry(void);
	ble_set_notice_cb(hal_bt_gap_callback);
	ble_entry();
	g_bleStackReady = 1;
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BLE scan stack initialized");
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	struct scan_param scan;
	ble_err_t ret;
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		HAL_BTScan_EnsureStackReady();
		memset(&scan, 0, sizeof(scan));
		//scan.filter_en = SCAN_FILT_DUPLIC_EN;
		scan.channel_map = 7;
		scan.interval = g_bleScanInterval;
		scan.window = g_bleScanWindow;
		scan.active = (uint8_t)scan_mode;

		g_bleScanActvIdx = app_ble_get_idle_actv_idx_handle(SCAN_ACTV);
		if(g_bleScanActvIdx == 0xFF)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE scan failed - no idle activity");
			return;
		}

		ret = bk_ble_scan_start(g_bleScanActvIdx, &scan, NULL);
		if(ret != ERR_SUCCESS)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE scan start failed (err %d)", ret);
			g_bleScanActvIdx = 0xFF;
			return;
		}

		g_bt_proxy.scan_active = true;
	}
}

void HAL_BTProxy_StopScan()
{
	if(!g_bt_proxy.init_done) return;
	ble_err_t ret;
	if(g_bt_proxy.scan_active)
	{
		ret = bk_ble_scan_stop(g_bleScanActvIdx, NULL);
		if(ret != ERR_SUCCESS)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE scan stop failed (err %d)", ret);
			return;
		}
		g_bleScanActvIdx = 0xFF;
		g_bt_proxy.scan_active = false;
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "There is no scan");
	}
}

void HAL_BTProxy_Init(void)
{
	if(g_bt_proxy.init_done)
	{
		return;
	}

	HAL_BTScan_EnsureStackReady();

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	HAL_BTProxy_StopScan();

	//g_bt_proxy.init_done = false;
}

void HAL_BTProxy_GetMAC(uint8_t* mac)
{
	uint8_t* btmac = &common_default_bdaddr.addr[0];
	for(int i = 0; i < 6; i++)
	{
		mac[i] = btmac[5 - i];
	}
	uint8_t zero_mac[6] = { 0 };
	uint8_t ff_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	if(memcmp(mac, zero_mac, 6) == 0 || memcmp(mac, ff_mac, 6) == 0)
	{
		WiFI_GetMacAddress((char*)mac);
	}
}

void HAL_BTProxy_SetScanMode(bool isActive)
{
	scan_mode = isActive;
	HAL_BTProxy_StopScan();
	delay_ms(1);
	HAL_BTProxy_StartScan();
}

bool HAL_BTProxy_GetScanMode(void)
{
	return scan_mode;
}

void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{
	g_bleScanInterval = interval;
	g_bleScanWindow = window;
	HAL_BTProxy_StopScan();
	delay_ms(1);
	HAL_BTProxy_StartScan();
}

bool HAL_BTProxy_IsInit(void)
{
	return g_bt_proxy.init_done;
}

void HAL_BTProxy_OnEverySecond(void)
{
	// add check if there were no scan results in 60 seconds, if so - restart scan
}

int HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets)
{
	if(init_done) *init_done = g_bt_proxy.init_done;
	if(scan_active) *scan_active = g_bt_proxy.scan_active;
	if(total_packets) *total_packets = g_bt_proxy.scan_total_packets;
	if(dropped_packets) *dropped_packets = g_bt_proxy.scan_dropped_packets;
	return 1;
}

#endif

#endif
