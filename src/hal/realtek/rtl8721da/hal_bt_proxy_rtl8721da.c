#if PLATFORM_REALTEK_NEW

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include "../../../driver/drv_esphome_api.h"

#if ENABLE_BT_PROXY

#include <string.h>
#include <stdio.h>

#include <bt_api_config.h>
#include <rtk_bt_def.h>
#include <rtk_bt_common.h>
#include <rtk_bt_gap.h>
#include <rtk_bt_le_gap.h>
#include <rtk_bt_gattc.h>
#include <rtk_bt_gatts.h>
#include <rtk_bt_device.h>

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

#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
static rtk_bt_le_ext_scan_param_t scan_param =
{
	.own_addr_type = RTK_BT_LE_ADDR_TYPE_PUBLIC,
	.phys = { true, true },
	.type = { RTK_BT_LE_SCAN_TYPE_PASSIVE, RTK_BT_LE_SCAN_TYPE_PASSIVE },
	.interval = { DEFAULT_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL },
	.window = { DEFAULT_SCAN_WINDOW, DEFAULT_SCAN_WINDOW },
	.duration = 0,
	.period = 0,
	.filter_policy = RTK_BT_LE_SCAN_FILTER_ALLOW_ALL,
	.duplicate_opt = RTK_BT_LE_SCAN_DUPLICATE_DISABLE,
};
#else
static rtk_bt_le_scan_param_t scan_param =
{
	.type = RTK_BT_LE_SCAN_TYPE_PASSIVE,
	.interval = DEFAULT_SCAN_INTERVAL,
	.window = DEFAULT_SCAN_WINDOW,
	.own_addr_type = RTK_BT_LE_ADDR_TYPE_PUBLIC,
	.filter_policy = RTK_BT_LE_SCAN_FILTER_ALLOW_ALL,
	.duplicate_opt = RTK_BT_LE_SCAN_DUPLICATE_DISABLE,
};
#endif

extern void WiFI_GetMacAddress(char* mac);
static hal_bt_state_t g_bt_proxy = { 0 };

#if PLATFORM_RTL8721DA && RTK_BLE_ISO_SUPPORT

uint16_t bt_stack_le_iso_act_handle(rtk_bt_cmd_t* p_cmd)
{
	bt_stack_pending_cmd_delete(p_cmd);
	p_cmd->ret = 1;
	osif_sem_give(p_cmd->psem);
	return 1;
}
uint16_t bt_stack_le_iso_init(void* p_conf)
{
	return RTK_BT_FAIL;
}

void bt_stack_le_iso_deinit(void)
{ }

#endif

static rtk_bt_evt_cb_ret_t hal_bt_gap_callback(uint8_t evt_code, void* param, uint32_t len)
{
	(void)len;
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	if(evt_code == RTK_BT_LE_GAP_EVT_EXT_SCAN_RES_IND)
	{
		rtk_bt_le_ext_scan_res_ind_t* scan_res_ind = (rtk_bt_le_ext_scan_res_ind_t*)param;
		g_bt_proxy.scan_total_packets++;
		if(!ESPHome_API_PassScanResult(scan_res_ind->addr.addr_val, scan_res_ind->rssi, scan_res_ind->evt_type, scan_res_ind->data, scan_res_ind->len))
		{
			g_bt_proxy.scan_dropped_packets++;
		}
	}
#else
	if(evt_code == RTK_BT_LE_GAP_EVT_SCAN_RES_IND)
	{
		rtk_bt_le_scan_res_ind_t* scan_res_ind = (rtk_bt_le_scan_res_ind_t*)param;

		g_bt_proxy.scan_total_packets++;
		if(!ESPHome_API_PassScanResult(scan_res_ind->adv_report.addr.addr_val, scan_res_ind->adv_report.rssi, scan_res_ind->adv_report.evt_type, scan_res_ind->adv_report.data, scan_res_ind->adv_report.len))
		{
			g_bt_proxy.scan_dropped_packets++;
		}
	}
#endif
	return RTK_BT_EVT_CB_OK;
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	uint16_t cause;
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		g_bt_proxy.scan_active = true;
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
		cause = rtk_bt_le_gap_start_ext_scan();
#else
		cause = rtk_bt_le_gap_start_scan();
#endif
		if(cause != 0)
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
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
		rtk_bt_le_gap_stop_ext_scan();
#else
		rtk_bt_le_gap_stop_scan();
#endif
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
		rtk_bt_evt_register_callback(RTK_BT_LE_GP_GAP, hal_bt_gap_callback);
		return;
	}
	rtk_bt_app_conf_t bt_app_conf = { 0 };
	bt_app_conf.app_profile_support = RTK_BT_PROFILE_GATTC;
	bt_app_conf.mtu_size = 180;
	bt_app_conf.master_init_mtu_req = true;
	bt_app_conf.slave_init_mtu_req = false;
	bt_app_conf.prefer_all_phy = RTK_BT_LE_PHYS_PREFER_ALL;
	bt_app_conf.prefer_tx_phy = RTK_BT_LE_PHYS_PREFER_1M | RTK_BT_LE_PHYS_PREFER_2M | RTK_BT_LE_PHYS_PREFER_CODED;
	bt_app_conf.prefer_rx_phy = RTK_BT_LE_PHYS_PREFER_1M | RTK_BT_LE_PHYS_PREFER_2M | RTK_BT_LE_PHYS_PREFER_CODED;
	bt_app_conf.max_tx_octets = 0x40;
	bt_app_conf.max_tx_time = 0x200;
	bt_app_conf.user_def_service = false;
	bt_app_conf.cccd_not_check = false;
	rtk_bt_enable(&bt_app_conf);
	rtk_bt_evt_register_callback(RTK_BT_LE_GP_GAP, hal_bt_gap_callback);
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	rtk_bt_le_gap_set_ext_scan_param(&scan_param);
#else
	rtk_bt_le_gap_set_scan_param(&scan_param);
#endif

	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	if(!g_bt_proxy.init_done) return;

	HAL_BTProxy_StopScan();
	rtk_bt_evt_unregister_callback(RTK_BT_LE_GP_GAP);
	rtk_bt_disable();
	g_bt_proxy.init_done = false;
}

void HAL_BTProxy_GetMAC(uint8_t* mac)
{
	rtk_bt_le_addr_t bd_addr = { (rtk_bt_le_addr_type_t)0, {0} };
	rtk_bt_le_gap_get_bd_addr(&bd_addr);
	for(int i = 0; i < 6; i++) mac[i] = bd_addr.addr_val[5 - i];
	uint8_t zero_mac[6] = { 0 };
	uint8_t ff_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	if(memcmp(mac, zero_mac, 6) == 0 || memcmp(mac, ff_mac, 6) == 0)
	{
		WiFI_GetMacAddress((char*)mac);
	}
}

void HAL_BTProxy_SetScanMode(bool isActive)
{
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	scan_param.type[0] = isActive == true ? RTK_BT_LE_SCAN_TYPE_ACTIVE : RTK_BT_LE_SCAN_TYPE_PASSIVE;
	scan_param.type[1] = scan_param.type[0];
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_ext_scan_param(&scan_param);
#else
	scan_param.type = isActive == true ? RTK_BT_LE_SCAN_TYPE_ACTIVE : RTK_BT_LE_SCAN_TYPE_PASSIVE;
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_scan_param(&scan_param);
#endif
	if(g_bt_proxy.scan_active)
	{
		HAL_BTProxy_StopScan();
		rtos_delay_milliseconds(10);
		HAL_BTProxy_StartScan();
	}
}

bool HAL_BTProxy_GetScanMode(void)
{
	if(!g_bt_proxy.init_done) return false;
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	if(scan_param.type[0] == RTK_BT_LE_SCAN_TYPE_ACTIVE) return true;
	return false;
#else
	rtk_bt_le_scan_param_t current_scan;
	uint16_t cause = rtk_bt_le_gap_get_scan_param(&current_scan);
	if(cause) return false;
	if(current_scan.type == RTK_BT_LE_SCAN_TYPE_ACTIVE) return true;
	else return false;
#endif
}

void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{
	// convert from ms
	uint16_t scan_interval = (uint16_t)((interval * 16) / 10);
	uint16_t scan_window = (uint16_t)((window * 16) / 10);
	
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	scan_param.window[0] = scan_window;
	scan_param.interval[0] = scan_interval;
	scan_param.window[1] = scan_window;
	scan_param.interval[1] = scan_interval;
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_ext_scan_param(&scan_param);
#else
	scan_param.window = scan_window;
	scan_param.interval = scan_interval;
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_scan_param(&scan_param);
#endif
	if(g_bt_proxy.scan_active)
	{
		HAL_BTProxy_StopScan();
		rtos_delay_milliseconds(10);
		HAL_BTProxy_StartScan();
	}
}


void HAL_BTProxy_FilterDuplicates(bool isActive)
{
	scan_param.duplicate_opt = isActive == true ? RTK_BT_LE_SCAN_DUPLICATE_ENABLE : RTK_BT_LE_SCAN_DUPLICATE_DISABLE;
#if defined(RTK_BLE_5_0_USE_EXTENDED_ADV) && RTK_BLE_5_0_USE_EXTENDED_ADV
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_ext_scan_param(&scan_param);
#else
	if(g_bt_proxy.init_done) rtk_bt_le_gap_set_scan_param(&scan_param);
#endif
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
