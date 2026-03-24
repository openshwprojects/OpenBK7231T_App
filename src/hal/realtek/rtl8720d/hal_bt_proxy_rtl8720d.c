#if PLATFORM_RTL8720D || PLATFORM_RTL87X0C

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include "../../../driver/drv_esphome_api.h"

#if ENABLE_BT_PROXY

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

#define DEFAULT_SCAN_INTERVAL     0x520	// 820ms
#define DEFAULT_SCAN_WINDOW       0x520	// 820ms

typedef struct
{
	bool init_done;
	bool scan_active;
	int scan_total_packets;
	int scan_dropped_packets;

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

static T_APP_RESULT hal_bt_gap_callback(uint8_t cb_type, void* p_cb_data)
{
	T_LE_CB_DATA* p_data = (T_LE_CB_DATA*)p_cb_data;
	if(!p_data || !p_data->p_le_scan_info) return APP_RESULT_SUCCESS;

	if(cb_type == GAP_MSG_LE_SCAN_INFO)
	{
		T_LE_SCAN_INFO* info = p_data->p_le_scan_info;

		g_bt_proxy.scan_total_packets++;
		if(!ESPHome_API_PassScanResult(info->bd_addr, info->rssi, info->remote_addr_type, info->data, info->data_len))
		{
			g_bt_proxy.scan_dropped_packets++;
		}
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
		le_register_app_cb(hal_bt_gap_callback);
		return;
	}
	btq_task_running = true;
	xTaskCreate(hal_bt_task, "HAL_BT_Task", 1024, NULL, 1, NULL);


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
