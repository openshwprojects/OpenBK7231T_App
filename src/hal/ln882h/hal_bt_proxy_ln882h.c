#if PLATFORM_LN882H

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_bt_proxy.h"
#include "../../logging/logging.h"
#include <string.h>
#include <stdio.h>

#if ENABLE_BT_PROXY

#include "ln_utils.h"
#include "ln_misc.h"
#include "utils/power_mgmt/ln_pm.h"
#include "gapm_task.h"
#include "gapc_task.h"
#include "gattc_task.h"
#include "gattm_task.h"
#include "gap.h"

#include "ln_ble_gap.h"
#include "ln_ble_gatt.h"
#include "ln_ble_scan.h"
#include "ln_ble_rw_app_task.h"
#include "ln_ble_app_kv.h"
#include "ln_ble_connection_manager.h"
#include "ln_ble_event_manager.h"
#include "ln_ble_smp.h"

#define BT_SCAN_RING_SIZE 64

typedef struct
{
	uint8_t bda[6];
	int rssi;
	int adv_len;
	uint8_t addr_type;
	int evt_type;
	uint8_t data[31];
	uint32_t ts_ms;
} bt_scan_entry_t;

typedef struct
{
	bool init_done;
	bool scan_active;
	int scan_total_packets;
	int scan_dropped_packets;

	uint16_t scan_ring_size;
	bt_scan_entry_t* scan_ring;
	int scan_head;
	int scan_tail;
	int scan_count;

	//QueueHandle_t cmd_queue;
	SemaphoreHandle_t scan_mutex;
	TaskHandle_t task_handle;
} hal_bt_state_t;

extern void WiFI_GetMacAddress(char* mac);
extern unsigned int g_timeMs;
static hal_bt_state_t g_bt_proxy = { 0 };

static int g_bleStackReady = 0;
static uint16_t g_bleScanInterval = 0x80;//= 0x63; // 100.8ms
static uint16_t g_bleScanWindow = 0x40;//= 0x25; // 40ms
static bool scan_mode = false; // passive - false, active - true

static int scan_entry_matches(bt_scan_entry_t* e, ble_evt_scan_report_t* info)
{
	if(memcmp(e->bda, info->trans_addr, 6) != 0)
		return 0;

	if(e->adv_len != info->length)
		return 0;

	if(e->evt_type != info->info)
		return 0;

	if(memcmp(e->data, info->data, info->length) != 0)
		return 0;

	return 1;
}

static void hal_bt_gap_callback(void* param)
{
	ble_evt_scan_report_t* info = (ble_evt_scan_report_t*)param;
	HAL_BTProxy_Lock();

	int found = -1;
	
	for(int i = 0; i < g_bt_proxy.scan_count; i++)
	{
		int idx = g_bt_proxy.scan_tail + i;
		if(idx >= g_bt_proxy.scan_ring_size)
			idx -= g_bt_proxy.scan_ring_size;
	
		if(scan_entry_matches(&g_bt_proxy.scan_ring[idx], info))
		{
			found = idx;
			break;
		}
	}
	
	int pos;
	
	if(found >= 0)
	{
		pos = found;
	}
	else
	{
		pos = g_bt_proxy.scan_head;

		g_bt_proxy.scan_head = (g_bt_proxy.scan_head + 1) % g_bt_proxy.scan_ring_size;

		if(g_bt_proxy.scan_count < g_bt_proxy.scan_ring_size)
		{
			g_bt_proxy.scan_count++;
		}
		else
		{
			g_bt_proxy.scan_tail = (g_bt_proxy.scan_tail + 1) % g_bt_proxy.scan_ring_size;
			g_bt_proxy.scan_dropped_packets++;
		}
	}

	memcpy(g_bt_proxy.scan_ring[pos].bda, info->trans_addr, 6);
	g_bt_proxy.scan_ring[pos].rssi = info->rssi  -128;
	g_bt_proxy.scan_ring[pos].addr_type = info->trans_addr_type;
	g_bt_proxy.scan_ring[pos].adv_len = info->length;
	g_bt_proxy.scan_ring[pos].evt_type = info->info;

	int copy_len = info->length > 31 ? 31 : info->length;
	memcpy(g_bt_proxy.scan_ring[pos].data, info->data, copy_len);

	g_bt_proxy.scan_ring[pos].ts_ms = g_timeMs;

	g_bt_proxy.scan_total_packets++;

	HAL_BTProxy_Unlock();
}

static void HAL_BTScan_EnsureStackReady()
{
	if(g_bleStackReady)
	{
		return;
	}
	ln_kv_ble_app_init();
	*(volatile int*)(0x400121F8) = 0x003F; // BLE default PTI == 0, wifi first
	soc_module_clk_gate_enable(CLK_G_BLE);
	extern void ble_entry(void);
	ln_bd_addr_t bt_addr = { 0 };
	ln_bd_addr_t* kv_addr = ln_kv_ble_pub_addr_get();
	memcpy(&bt_addr, kv_addr, sizeof(ln_bd_addr_t));
	ln_bd_addr_t static_addr = { BLE_DEFAULT_PUBLIC_ADDR };
	
	if(!memcmp(kv_addr->addr, static_addr.addr, LN_BD_ADDR_LEN))
	{
		ln_generate_random_mac(bt_addr.addr);
		bt_addr.addr[5] |= 0xC0;
		ln_kv_ble_addr_store(bt_addr);
	}
	//sysparam_sta_mac_get(bt_addr.addr);

	extern void rw_init(uint8_t mac[6]);
	rw_init(bt_addr.addr);

	ln_gap_app_init();
	ln_gatt_app_init();

	ln_ble_conn_mgr_init();
	ln_ble_evt_mgr_init();
	ln_ble_smp_init();

	//ln_ble_adv_mgr_init();
	//ln_ble_trans_svr_init();
	ln_ble_scan_mgr_init();

	ln_rw_app_task_init();

	ln_gap_reset();

	ln_ble_evt_mgr_reg_evt(BLE_EVT_ID_SCAN_REPORT, hal_bt_gap_callback);

	rtos_delay_milliseconds(100);

	ln_ble_scan_actv_creat();
	//ln_ble_init_actv_creat();

	rtos_delay_milliseconds(10);
	ln_ble_scan_start(&le_scan_mgr_info_get()->scan_param);
	rtos_delay_milliseconds(10);
	ln_ble_scan_stop();
	g_bleStackReady = 1;
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BLE scan stack initialized");
}

void HAL_BTProxy_StartScan()
{
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		HAL_BTScan_EnsureStackReady();
		le_scan_parameters_t* scan_p = &le_scan_mgr_info_get()->scan_param;

		scan_p->dup_filt_pol = GAPM_DUP_FILT_DIS;
		scan_p->scan_intv = g_bleScanInterval;
		scan_p->scan_wd = g_bleScanWindow;
		scan_p->type = GAPM_SCAN_TYPE_OBSERVER;
		scan_p->prop = GAPM_SCAN_PROP_PHY_1M_BIT | GAPM_SCAN_PROP_PHY_CODED_BIT;
		if(scan_mode)
		{
			scan_p->prop |= GAPM_SCAN_PROP_ACTIVE_1M_BIT | GAPM_SCAN_PROP_ACTIVE_CODED_BIT;
		}
		ln_ble_scan_start(&le_scan_mgr_info_get()->scan_param);
		g_bt_proxy.scan_active = true;
	}
}

void HAL_BTProxy_StopScan()
{
	if(g_bt_proxy.scan_active)
	{
		ln_ble_scan_stop();
		g_bt_proxy.scan_active = false;
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "There is no scan");
	}
}

void HAL_BTProxy_Init(void)
{
	if(g_bt_proxy.init_done) return;
	HAL_BTProxy_Lock();

	memset(&g_bt_proxy, 0, sizeof(g_bt_proxy));
	g_bt_proxy.scan_ring_size = BT_SCAN_RING_SIZE;
	g_bt_proxy.scan_ring = os_malloc(sizeof(bt_scan_entry_t) * g_bt_proxy.scan_ring_size);

	HAL_BTScan_EnsureStackReady();
	HAL_BTProxy_StartScan();

	HAL_BTProxy_Unlock();

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	if(!g_bt_proxy.init_done) return;

	HAL_BTProxy_StopScan();

	g_bt_proxy.init_done = false;
}

void HAL_BTProxy_GetMAC(uint8_t* mac)
{
	ln_bd_addr_t* kv_addr = ln_kv_ble_pub_addr_get();
	memcpy(mac, kv_addr->addr, 6);
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
	g_bleScanInterval =  (uint16_t)((interval * 16) / 10);
	g_bleScanWindow = (uint16_t)((window * 16) / 10);
	HAL_BTProxy_StopScan();
	rtos_delay_milliseconds(10);
	HAL_BTProxy_StartScan();
}

void HAL_BTProxy_SetScanRingBufSize(uint16_t new_size)
{
	HAL_BTProxy_Lock();
	g_bt_proxy.scan_ring_size = new_size;
	g_bt_proxy.scan_head = 0;
	g_bt_proxy.scan_tail = 0;
	g_bt_proxy.scan_count = 0;
	if(g_bt_proxy.scan_ring) os_free(g_bt_proxy.scan_ring);
	g_bt_proxy.scan_ring = os_malloc(sizeof(bt_scan_entry_t) * g_bt_proxy.scan_ring_size);
	HAL_BTProxy_Unlock();
}

void HAL_BTProxy_OnEverySecond(void)
{
	// add check if there were no scan results in 60 seconds, if so - restart scan
}

int HAL_BTProxy_PopScanResult(uint8_t* mac, int* rssi, uint8_t* addr_type, uint8_t* data, int* data_len)
{
	if(g_bt_proxy.scan_count == 0) return 0;

	int pos = g_bt_proxy.scan_tail;

	//memcpy(mac, g_bt_proxy.scan_ring[pos].bda, 6);
	if(mac) for(int i = 0; i < 6; i++)
	{
		mac[i] = g_bt_proxy.scan_ring[pos].bda[5 - i];
	}
	if(rssi) *rssi = g_bt_proxy.scan_ring[pos].rssi;
	if(addr_type) *addr_type = g_bt_proxy.scan_ring[pos].addr_type;
	if(data_len) *data_len = g_bt_proxy.scan_ring[pos].adv_len > 62 ? 62 : g_bt_proxy.scan_ring[pos].adv_len;
	if(data && data_len) memcpy(data, g_bt_proxy.scan_ring[pos].data, *data_len);

	g_bt_proxy.scan_tail = (g_bt_proxy.scan_tail + 1) % g_bt_proxy.scan_ring_size;
	g_bt_proxy.scan_count--;

	return 1;
}

int HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets, int* buffered_packets)
{
	if(init_done) *init_done = g_bt_proxy.init_done;
	if(scan_active) *scan_active = g_bt_proxy.scan_active;
	if(total_packets) *total_packets = g_bt_proxy.scan_total_packets;
	if(dropped_packets) *dropped_packets = g_bt_proxy.scan_dropped_packets;
	if(buffered_packets) *buffered_packets = g_bt_proxy.scan_count;
	return 1;
}

int HAL_BTProxy_GetScanEntry(int newest_index, char* mac_buf, int mac_buf_len, int* rssi, int* adv_len, int* evt_type, int* age_ms)
{
	int idx;
	int pos;
	bt_scan_entry_t e;
	uint32_t now_ms;

	if(newest_index < 0 || newest_index >= g_bt_proxy.scan_count)
	{
		return 0;
	}
	if(!mac_buf || mac_buf_len < 18)
	{
		return 0;
	}

	idx = g_bt_proxy.scan_head - 1 - newest_index;
	while(idx < 0)
	{
		idx += g_bt_proxy.scan_ring_size;
	}
	pos = idx % g_bt_proxy.scan_ring_size;
	e = g_bt_proxy.scan_ring[pos];

	snprintf(mac_buf, mac_buf_len, MACSTR, MAC2STRINV(e.bda));

	if(rssi) *rssi = e.rssi;
	if(adv_len) *adv_len = e.adv_len;
	if(evt_type) *evt_type = e.evt_type;
	if(age_ms)
	{
		now_ms = g_timeMs;
		*age_ms = (int)(now_ms - e.ts_ms);
	}
	return 1;
}

void HAL_BTProxy_Lock(void)
{
	if(g_bt_proxy.scan_mutex)
	{
		xSemaphoreTake(g_bt_proxy.scan_mutex, portMAX_DELAY);
	}
}

void HAL_BTProxy_Unlock(void)
{
	if(g_bt_proxy.scan_mutex)
	{
		xSemaphoreGive(g_bt_proxy.scan_mutex);
	}
}

#endif

#endif
