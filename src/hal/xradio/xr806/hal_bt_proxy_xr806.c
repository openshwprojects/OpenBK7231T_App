#if PLATFORM_XR806

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include "../../../driver/drv_esphome_api.h"
#include <string.h>
#include <stdio.h>

#if ENABLE_BT_PROXY

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

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
static uint16_t g_bleScanInterval = 0x80;//= 0x63; // 100.8ms
static uint16_t g_bleScanWindow = 0x40;//= 0x25; // 40ms
static bool scan_mode = false; // passive - false, active - true
static bool filter_duplicates = false;

static void hal_bt_gap_callback(const bt_addr_le_t* addr, int8_t rssi, uint8_t type, struct net_buf_simple* ad)
{
	g_bt_proxy.scan_total_packets++;
	if(!ESPHome_API_PassScanResult(addr->a.val, rssi, type, ad->data, ad->len))
	{
		g_bt_proxy.scan_dropped_packets++;
	}
}

static void bt_ready(int err)
{
	if(err)
	{
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Bluetooth init failed %i", err);
		g_bleStackReady = 0;
		return;
	}
	bt_le_scan_cb_register(hal_bt_gap_callback);
	g_bleStackReady = 1;
}

static void HAL_BTScan_EnsureStackReady()
{
	if(g_bleStackReady)
	{
		return;
	}
	bt_ctrl_enable();
	bt_enable(bt_ready);
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		HAL_BTScan_EnsureStackReady();
		struct bt_le_scan_param param =
		{
			.options = BT_LE_SCAN_OPT_CODED,
			.interval = g_bleScanInterval,
			.window = g_bleScanWindow,
		};
		param.type = scan_mode == true ? BT_LE_SCAN_TYPE_ACTIVE : BT_LE_SCAN_TYPE_PASSIVE;
		param.options |= filter_duplicates == true ? BT_LE_SCAN_OPT_FILTER_DUPLICATE : 0;
		int err = bt_le_scan_start(&param, NULL);
		if(err)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "Scan failed to start %i", err);
			return;
		}
		g_bt_proxy.scan_active = true;
	}
}

void HAL_BTProxy_StopScan()
{
	if(!g_bt_proxy.init_done) return;
	if(g_bt_proxy.scan_active)
	{
		bt_le_scan_stop();
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
		bt_le_scan_cb_register(hal_bt_gap_callback);
		return;
	}

	HAL_BTScan_EnsureStackReady();

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	if(!g_bt_proxy.init_done) return;

	HAL_BTProxy_StopScan();

	bt_le_scan_cb_unregister(hal_bt_gap_callback);
}

void HAL_BTProxy_SetScanMode(bool isActive)
{
	scan_mode = isActive;
	HAL_BTProxy_StopScan();
	rtos_delay_milliseconds(10);
	HAL_BTProxy_StartScan();
}

bool HAL_BTProxy_GetScanMode(void)
{
	return scan_mode;
}

void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{
	// convert from ms
	g_bleScanInterval = (uint16_t)((interval * 16) / 10);
	g_bleScanWindow   = (uint16_t)((window * 16) / 10);
	HAL_BTProxy_StopScan();
	rtos_delay_milliseconds(10);
	HAL_BTProxy_StartScan();
}

void HAL_BTProxy_FilterDuplicates(bool isActive)
{
	filter_duplicates = isActive;
	if(g_bt_proxy.scan_active)
	{
		HAL_BTProxy_StopScan();
		rtos_delay_milliseconds(10);
		HAL_BTProxy_StartScan();
	}
}

bool HAL_BTProxy_IsInit(void)
{
	return g_bt_proxy.init_done;
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
