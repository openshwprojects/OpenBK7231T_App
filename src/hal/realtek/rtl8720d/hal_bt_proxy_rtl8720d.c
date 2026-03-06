#if PLATFORM_RTL8720D || PLATFORM_RTL87X0C

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include <string.h>
#include <stdio.h>
#include <os_sched.h>
#include "wifi_conf.h"
#include "trace_app.h"
#include "gap.h"
#include "gap_adv.h"
#include "gap_bond_le.h"
#include "gap_callback_le.h"
#include "gap_config.h"
#include "gap_conn_le.h"
#include "gap_le.h"
#include "gap_le_types.h"
#include "gap_msg.h"
#include "gap_privacy.h"
#include "gap_scan.h"
#include "gap_storage_le.h"
#include "os_task.h"
#include "os_msg.h"
#include "app_msg.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "bte.h"

#if ENABLE_BT_PROXY

#if PLATFORM_RTL8720D
#define BT_SCAN_RING_SIZE 128
#else
#define BT_SCAN_RING_SIZE 32
#endif
#define BT_CMD_QUEUE_SIZE 10
#define DEFAULT_SCAN_INTERVAL     0x520	// 820ms
#define DEFAULT_SCAN_WINDOW       0x520	// 820ms

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
	TaskHandle_t task_handle;
} hal_bt_state_t;

extern int bt_get_mac_address(uint8_t* mac);
extern void WiFI_GetMacAddress(char* mac);
extern unsigned int g_timeMs;
static hal_bt_state_t g_bt_proxy = { 0 };
static void* _evtQueueHandle;
static void* _ioQueueHandle;
static bool btq_task_running = true;
static uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
static uint16_t scan_window = DEFAULT_SCAN_WINDOW;
static uint8_t scan_mode = GAP_SCAN_MODE_PASSIVE;

static int scan_entry_matches(bt_scan_entry_t* e, T_LE_SCAN_INFO* info)
{
	if(memcmp(e->bda, info->bd_addr, 6) != 0)
		return 0;

	if(e->adv_len != info->data_len)
		return 0;

	if(e->evt_type != info->adv_type)
		return 0;

	if(memcmp(e->data, info->data, info->data_len) != 0)
		return 0;

	return 1;
}

static T_APP_RESULT hal_bt_gap_callback(uint8_t cb_type, void* p_cb_data)
{
	T_LE_CB_DATA* p_data = (T_LE_CB_DATA*)p_cb_data;
	if(!p_data || !p_data->p_le_scan_info) return APP_RESULT_SUCCESS;

	if(cb_type == GAP_MSG_LE_SCAN_INFO)
	{
		HAL_BTProxy_Lock();
		if(!g_bt_proxy.scan_ring)
		{
			HAL_BTProxy_Unlock();
			return;
		}

		T_LE_SCAN_INFO* info = p_data->p_le_scan_info;
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

		memcpy(g_bt_proxy.scan_ring[pos].bda, info->bd_addr, 6);
		g_bt_proxy.scan_ring[pos].rssi = info->rssi;
		g_bt_proxy.scan_ring[pos].addr_type = info->remote_addr_type;
		g_bt_proxy.scan_ring[pos].adv_len = info->data_len;
		g_bt_proxy.scan_ring[pos].evt_type = info->adv_type;

		int copy_len = info->data_len > 31 ? 31 : info->data_len;
		memcpy(g_bt_proxy.scan_ring[pos].data, info->data, copy_len);

		g_bt_proxy.scan_ring[pos].ts_ms = g_timeMs;

		g_bt_proxy.scan_total_packets++;

		HAL_BTProxy_Unlock();
	}
	return APP_RESULT_SUCCESS;
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	T_GAP_CAUSE cause;
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		g_bt_proxy.scan_active = true;
		cause = le_scan_start();
		if(cause != GAP_CAUSE_SUCCESS)
		{
			ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan error");
			g_bt_proxy.scan_active = false;
		}
	}
}

void HAL_BTProxy_StopScan()
{
	if(!g_bt_proxy.init_done) return;
	if(g_bt_proxy.scan_active)
	{
		le_scan_stop();
		g_bt_proxy.scan_active = false;
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "There is no scan");
	}
}

static void hal_btq_task(void* p_param)
{
	(void)p_param;
	uint8_t event;
	os_msg_queue_create(&_ioQueueHandle, 0x20, sizeof(T_IO_MSG));
	os_msg_queue_create(&_evtQueueHandle, 0x40, sizeof(uint8_t));
	gap_start_bt_stack(_evtQueueHandle, _ioQueueHandle, 0x20);
	while(btq_task_running)
	{
		if(os_msg_recv(_evtQueueHandle, &event, 0xFFFFFFFF) == true)
		{
			if(event == EVENT_IO_TO_APP)
			{
				T_IO_MSG io_msg;
				if(os_msg_recv(_ioQueueHandle, &io_msg, 0) == true)
				{

				}
			}
			else
			{
				gap_handle_msg(event);
			}
		}
	}
	vTaskDelete(NULL);
}

static void hal_bt_task(void* p_param)
{
	(void)p_param;

	while(!(wifi_is_up(RTW_STA_INTERFACE)))
	{
		delay_ms(100);
	}

	T_GAP_DEV_STATE new_state;
	bt_trace_init();
	le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	if(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY)
	{
		bte_init();
		// Create command queue
		gap_config_max_le_link_num(2);
		//gap_config_max_le_paired_device(3);
		le_gap_init(2);
	}

	char* device_name = (char*)CFG_GetDeviceName();
	uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

	uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
	uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;

	le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
	le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

	le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
	le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);

	le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
		&scan_filter_policy);
	le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
		&scan_filter_duplicate);
	//client_init(2);

	le_register_app_cb(hal_bt_gap_callback);

	xTaskCreate(hal_btq_task, "HAL_BTQ_Task", 1024, NULL, 1, &g_bt_proxy.task_handle);

	bt_coex_init();
	do
	{
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	} while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);
	wifi_btcoex_set_bt_on();
	delay_ms(50);
	vTaskDelete(NULL);
}

void HAL_BTProxy_Init(void)
{
	if(g_bt_proxy.init_done)
	{
		HAL_BTProxy_Lock();
		if(g_bt_proxy.scan_ring) os_free(g_bt_proxy.scan_ring);
		g_bt_proxy.scan_ring = os_malloc(sizeof(bt_scan_entry_t) * g_bt_proxy.scan_ring_size);
		HAL_BTProxy_Unlock();
		return;
		le_register_app_cb(hal_bt_gap_callback);
		return;
	}
	HAL_BTProxy_Lock();
	memset(&g_bt_proxy, 0, sizeof(g_bt_proxy));
	g_bt_proxy.scan_ring_size = BT_SCAN_RING_SIZE;
	g_bt_proxy.scan_ring = os_malloc(sizeof(bt_scan_entry_t) * g_bt_proxy.scan_ring_size);
	//g_bt_proxy.cmd_queue = xQueueCreate(BT_CMD_QUEUE_SIZE, sizeof(bt_proxy_cmd_t));
	//if(!g_bt_proxy.cmd_queue)
	//{
	//	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BT proxy: failed to create cmd queue");
	//	return;
	//}
	btq_task_running = true;
	xTaskCreate(hal_bt_task, "HAL_BT_Task", 1024, NULL, 1, NULL);

	HAL_BTProxy_Unlock();

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	if(!g_bt_proxy.init_done) return;

	HAL_BTProxy_StopScan();
	//btq_task_running = false;
	le_register_app_cb(NULL);

	//if(_evtQueueHandle)
	//{
	//	uint8_t dummy = EVENT_IO_TO_APP;
	//	os_msg_send(_evtQueueHandle, &dummy, 0);
	//	delay_ms(100);
	//	os_msg_queue_delete(_evtQueueHandle);
	//	_evtQueueHandle = NULL;
	//}
	//if(_ioQueueHandle)
	//{
	//	os_msg_queue_delete(_ioQueueHandle);
	//	_ioQueueHandle = NULL;
	//}

	//bte_deinit(); // causes crash
	//bt_trace_uninit();

	HAL_BTProxy_Lock();

	g_bt_proxy.scan_total_packets = 0;
	g_bt_proxy.scan_dropped_packets = 0;
	g_bt_proxy.scan_head = 0;
	g_bt_proxy.scan_tail = 0;
	g_bt_proxy.scan_count = 0;

	if(g_bt_proxy.scan_ring)
	{
		os_free(g_bt_proxy.scan_ring);
		g_bt_proxy.scan_ring = NULL;
	}

	HAL_BTProxy_Unlock();

	//g_bt_proxy.init_done = false;
}

void HAL_BTProxy_GetMAC(uint8_t* mac)
{
	bt_get_mac_address(mac);
	uint8_t zero_mac[6] = { 0 };
	uint8_t ff_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	if(memcmp(mac, zero_mac, 6) == 0 || memcmp(mac, ff_mac, 6) == 0)
	{
		WiFI_GetMacAddress((char*)mac);
	}
}

void HAL_BTProxy_SetScanMode(bool isActive)
{
	scan_mode = isActive == true ? GAP_SCAN_MODE_ACTIVE : GAP_SCAN_MODE_PASSIVE;
	if(g_bt_proxy.init_done)
	{
		le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
	}
}

bool HAL_BTProxy_GetScanMode(void)
{
	if(!g_bt_proxy.init_done) return false;
	uint8_t scan_mode;
	T_GAP_CAUSE cause = le_scan_get_param(GAP_PARAM_SCAN_MODE, &scan_mode);
	if(cause) return false;
	if(scan_mode == GAP_SCAN_MODE_ACTIVE) return true;
	else return false;
}

void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{
	// convert from ms
	scan_interval = (uint16_t)((interval * 16) / 10);
	scan_window = (uint16_t)((window * 16) / 10);

	if(g_bt_proxy.init_done)
	{
		le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
		le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	}
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

bool HAL_BTProxy_IsInit(void)
{
	return g_bt_proxy.init_done;
}

void HAL_BTProxy_OnEverySecond(void)
{
	// add check if there were no scan results in 60 seconds, if so - restart scan
}

//int HAL_BTProxy_EnqueueCommand(bt_proxy_cmd_t* cmd)
//{
//	if(!g_bt_proxy.cmd_queue) return -1;
//
//	//if(xQueueSend(g_bt_proxy.cmd_queue, cmd, pdMS_TO_TICKS(100)) != pdPASS)
//	//{
//	//	return -2;
//	//}
//	return 0;
//}

int HAL_BTProxy_PopScanResult(uint8_t* mac, int* rssi, uint8_t* addr_type, uint8_t* data, int* data_len)
{
	if(g_bt_proxy.scan_count == 0 || !g_bt_proxy.scan_ring) return 0;

	int pos = g_bt_proxy.scan_tail;
	//memcpy(mac, g_bt_proxy.scan_ring[pos].bda, 6);
	if(mac) for(int i = 0; i < 6; i++)
	{
		mac[i] = g_bt_proxy.scan_ring[pos].bda[5 - i];
	}
	if(rssi) *rssi = g_bt_proxy.scan_ring[pos].rssi;
	if(addr_type) *addr_type = g_bt_proxy.scan_ring[pos].addr_type;
	if(data_len) *data_len = g_bt_proxy.scan_ring[pos].adv_len > 31 ? 31 : g_bt_proxy.scan_ring[pos].adv_len;
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

	if(g_bt_proxy.scan_count == 0 || !g_bt_proxy.scan_ring) return 0;

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

#endif

#endif
