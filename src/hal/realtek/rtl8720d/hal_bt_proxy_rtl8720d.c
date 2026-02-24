#ifdef PLATFORM_RTL8720D

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include <string.h>
#include <stdio.h>
#include <os_sched.h>
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

#if ENABLE_BT_PROXY

#define BT_SCAN_RING_SIZE 128
#define BT_CMD_QUEUE_SIZE 10
/** @brief Default scan window (units of 0.625ms, 0x520=820ms) */
#define DEFAULT_SCAN_INTERVAL     0x520
#define DEFAULT_SCAN_WINDOW       0x520

typedef struct
{
	uint8_t bda[6];
	int rssi;
	int adv_len;
	uint8_t addr_type;
	int evt_type;
	uint8_t data[62];
	uint32_t ts_ms;
} bt_scan_entry_t;

typedef struct
{
	bool init_done;
	bool scan_active;
	int scan_total_packets;
	int scan_dropped_packets;

	bt_scan_entry_t scan_ring[BT_SCAN_RING_SIZE];
	int scan_head;
	int scan_tail;
	int scan_count;

	QueueHandle_t cmd_queue;
	TaskHandle_t task_handle;
} hal_bt_state_t;

extern unsigned int g_timeMs;
static hal_bt_state_t g_bt_proxy = { 0 };
static void* _evtQueueHandle;
static void* _ioQueueHandle;

static int scan_entry_matches(bt_scan_entry_t* e, T_LE_SCAN_INFO* info)
{
	uint8_t tmp_mac[6];

	for(int i = 0; i < 6; i++)
		tmp_mac[i] = info->bd_addr[5 - i];

	if(memcmp(e->bda, tmp_mac, 6) != 0)
		return 0;

	if(e->adv_len != info->data_len)
		return 0;

	if(e->evt_type != info->adv_type)
		return 0;

	return 1;
}

static void hal_bt_gap_callback(uint8_t cb_type, void* p_cb_data)
{
	T_LE_CB_DATA* p_data = (T_LE_CB_DATA*)p_cb_data;
	if(!p_data || !p_data->p_le_scan_info) return;

	if(cb_type == GAP_MSG_LE_SCAN_INFO)
	{
		T_LE_SCAN_INFO* info = p_data->p_le_scan_info;
		int found = -1;

		for(int i = 0; i < g_bt_proxy.scan_count; i++)
		{
			int idx = g_bt_proxy.scan_tail + i;
			if(idx >= BT_SCAN_RING_SIZE)
				idx -= BT_SCAN_RING_SIZE;

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

			g_bt_proxy.scan_head = (g_bt_proxy.scan_head + 1) % BT_SCAN_RING_SIZE;

			if(g_bt_proxy.scan_count < BT_SCAN_RING_SIZE)
			{
				g_bt_proxy.scan_count++;
			}
			else
			{
				g_bt_proxy.scan_tail = (g_bt_proxy.scan_tail + 1) % BT_SCAN_RING_SIZE;
				g_bt_proxy.scan_dropped_packets++;
			}
		}

		//memcpy(g_bt_proxy.scan_ring[pos].bda, info->bd_addr, 6);
		// reverse mac bytes
		for(int i = 0; i < 6; i++)
		{
			g_bt_proxy.scan_ring[pos].bda[i] = info->bd_addr[5 - i];
		}
		g_bt_proxy.scan_ring[pos].rssi = info->rssi;
		g_bt_proxy.scan_ring[pos].addr_type = info->remote_addr_type;
		g_bt_proxy.scan_ring[pos].adv_len = info->data_len;
		g_bt_proxy.scan_ring[pos].evt_type = info->adv_type;

		int copy_len = info->data_len > 62 ? 62 : info->data_len;
		memcpy(g_bt_proxy.scan_ring[pos].data, info->data, copy_len);

		g_bt_proxy.scan_ring[pos].ts_ms = g_timeMs;

		g_bt_proxy.scan_total_packets++;
	}
}

void HAL_BTProxy_StartScan()
{
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
	while(true)
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
}
static void hal_bt_task(void* p_param)
{
	(void)p_param;
	uint8_t event;

	while(!(wifi_is_up(RTW_STA_INTERFACE)))
	{
		delay_ms(100);
	}

	T_GAP_DEV_STATE new_state;
	bt_trace_init();
	bte_init();
	// Create command queue
	gap_config_max_le_link_num(2);
	//gap_config_max_le_paired_device(3);
	le_gap_init(2);

	char* device_name = CFG_GetDeviceName();
	uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

	uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
	uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
	uint16_t scan_window = DEFAULT_SCAN_WINDOW;
	uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
	uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;

	le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
	le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

	le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
	le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
	le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
		&scan_filter_policy);
	le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
		&scan_filter_duplicate);
	//client_init(2);

	le_register_app_cb(hal_bt_gap_callback);

	xTaskCreate(hal_btq_task, "HAL_BTQ_Task", 2048, NULL, 1, &g_bt_proxy.task_handle);

	bt_coex_init();
	do
	{
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	} while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);
	wifi_btcoex_set_bt_on();
	delay_ms(50);
	HAL_BTProxy_StartScan();
	vTaskDelete(NULL);
}

void HAL_BTProxy_PreInit(void)
{
	if(g_bt_proxy.init_done) return;

	g_bt_proxy.cmd_queue = xQueueCreate(BT_CMD_QUEUE_SIZE, sizeof(bt_proxy_cmd_t));
	if(!g_bt_proxy.cmd_queue)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "BT proxy: failed to create cmd queue");
		return;
	}
	// Create HAL task
	xTaskCreate(hal_bt_task, "HAL_BT_Task", 2048, NULL, 1, NULL);

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_OnEverySecond(void)
{

}

int HAL_BTProxy_EnqueueCommand(bt_proxy_cmd_t* cmd)
{
	if(!g_bt_proxy.cmd_queue) return -1;

	//if(xQueueSend(g_bt_proxy.cmd_queue, cmd, pdMS_TO_TICKS(100)) != pdPASS)
	//{
	//    return -2;
	//}
	return 0;
}

int HAL_BTProxy_PopScanResult(uint8_t* mac, int* rssi, uint8_t* addr_type, uint8_t* data, int* data_len)
{
	if(g_bt_proxy.scan_count == 0) return 0;

	int pos = g_bt_proxy.scan_tail;
	memcpy(mac, g_bt_proxy.scan_ring[pos].bda, 6);
	if(rssi) *rssi = g_bt_proxy.scan_ring[pos].rssi;
	if(addr_type) *addr_type = g_bt_proxy.scan_ring[pos].addr_type;
	if(data_len) *data_len = g_bt_proxy.scan_ring[pos].adv_len > 62 ? 62 : g_bt_proxy.scan_ring[pos].adv_len;
	if(data) memcpy(data, g_bt_proxy.scan_ring[pos].data, *data_len);

	g_bt_proxy.scan_tail = (g_bt_proxy.scan_tail + 1) % BT_SCAN_RING_SIZE;
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
		idx += BT_SCAN_RING_SIZE;
	}
	pos = idx % BT_SCAN_RING_SIZE;
	e = g_bt_proxy.scan_ring[pos];

	snprintf(mac_buf, mac_buf_len, MACSTR, MAC2STR(e.bda));

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
