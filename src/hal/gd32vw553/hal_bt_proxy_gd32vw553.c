#if PLATFORM_GD32VW553

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_bt_proxy.h"
#include "../../logging/logging.h"
#include "../../driver/drv_esphome_api.h"
#include <string.h>
#include <stdio.h>

#if ENABLE_BT_PROXY

#include "ble_adapter.h"
#include "ble_scan.h"
#include "ble_conn.h"
#include "ble_utils.h"
#include "ble_export.h"
#include "ble_gattc.h"
#include "ble_adv_data.h"
#include "ble_sec.h"
#include "wrapper_os.h"

enum
{
	BLE_STACK_TASK_PRIORITY = OS_TASK_PRIORITY(2),
	BLE_APP_TASK_PRIORITY = OS_TASK_PRIORITY(1),
};

enum
{
	BLE_STACK_TASK_STACK_SIZE = 768,
	BLE_APP_TASK_STACK_SIZE = 1024,
};

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
static uint16_t g_bleScanInterval = 100;
static uint16_t g_bleScanWindow = 30;
static bool scan_mode = false; // passive - false, active - true
static bool isFiltering = false;

static void app_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u* p_data)
{
	printf("app_scan_mgr_evt_handler %i\r\n", event);
	if(event != BLE_SCAN_EVT_ADV_RPT) return;
	g_bt_proxy.scan_total_packets++;
	uint8_t type = *(uint8_t*)(&p_data->p_adv_rpt->type);
	if(!ESPHome_API_PassScanResult(p_data->p_adv_rpt->peer_addr.addr, p_data->p_adv_rpt->rssi, type, p_data->p_adv_rpt->data.p_data, p_data->p_adv_rpt->data.len))
	{
		g_bt_proxy.scan_dropped_packets++;
	}
}

//static void app_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u* p_data)
//{
//	uint8_t i = 0;
//
//	if(event == BLE_ADP_EVT_ENABLE_CMPL_INFO)
//	{
//		if(p_data->adapter_info.status == BLE_ERR_NO_ERROR)
//		{
//			printf("=== Adapter enable success ===\r\n");
//			printf("hci_ver 0x%x, hci_subver 0x%x, lmp_ver 0x%x, lmp_subver 0x%x, manuf_name 0x%x\r\n",
//				p_data->adapter_info.version.hci_ver, p_data->adapter_info.version.hci_subver,
//				p_data->adapter_info.version.lmp_ver, p_data->adapter_info.version.lmp_subver,
//				p_data->adapter_info.version.manuf_name);
//
//			printf("adv_set_num %u, min_tx_pwr %d, max_tx_pwr %d, max_adv_data_len %d \r\n",
//				p_data->adapter_info.adv_set_num, p_data->adapter_info.tx_pwr_range.min_tx_pwr,
//				p_data->adapter_info.tx_pwr_range.max_tx_pwr, p_data->adapter_info.max_adv_data_len);
//			printf("sugg_max_tx_octets %u, sugg_max_tx_time %u \r\n",
//				p_data->adapter_info.sugg_dft_data.sugg_max_tx_octets,
//				p_data->adapter_info.sugg_dft_data.sugg_max_tx_time);
//
//			printf("loc irk:");
//
//			for(i = 0; i < BLE_GAP_KEY_LEN; i++)
//			{
//				printf(" %02x", p_data->adapter_info.loc_irk_info.irk[i]);
//			}
//
//			printf("\r\n");
//			printf("identity addr %02X:%02X:%02X:%02X:%02X:%02X \r\n ",
//				p_data->adapter_info.loc_irk_info.identity.addr[5],
//				p_data->adapter_info.loc_irk_info.identity.addr[4],
//				p_data->adapter_info.loc_irk_info.identity.addr[3],
//				p_data->adapter_info.loc_irk_info.identity.addr[2],
//				p_data->adapter_info.loc_irk_info.identity.addr[1],
//				p_data->adapter_info.loc_irk_info.identity.addr[0]);
//
//			printf("=== BLE Adapter enable complete ===\r\n");
//		}
//		else
//		{
//			printf("=== BLE Adapter enable fail ===\r\n");
//		}
//
//	}
//}

static void HAL_BTScan_EnsureStackReady()
{
	if(g_bleStackReady)
	{
		return;
	}

	ble_init_param_t param = { 0 };
	ble_os_api_t os_interface = {
		.os_malloc = sys_malloc,
		.os_calloc = sys_calloc,
		.os_mfree = sys_mfree,
		.os_memset = sys_memset,
		.os_memcpy = sys_memcpy,
		.os_memcmp = sys_memcmp,
		.os_task_create = sys_task_create,
		.os_task_init_notification = sys_task_init_notification,
		.os_task_wait_notification = sys_task_wait_notification,
		.os_task_notify = sys_task_notify,
		.os_task_delete = sys_task_delete,
		.os_ms_sleep = sys_ms_sleep,
		.os_current_task_handle_get = sys_current_task_handle_get,
		.os_queue_init = sys_queue_init,
		.os_queue_free = sys_queue_free,
		.os_queue_write = sys_queue_write,
		.os_queue_read = sys_queue_read,
		.os_random_bytes_get = sys_random_bytes_get,
	};

	ble_power_on();

	param.role = BLE_GAP_SCAN_TYPE_OBSERVER;
	param.keys_user_mgr = app_sec_user_key_mgr_get();
	param.pairing_mode = 0;
	param.privacy_cfg = BLE_GAP_PRIV_CFG_PRIV_EN_BIT;
	param.ble_task_stack_size = BLE_STACK_TASK_STACK_SIZE;
	param.ble_task_priority = BLE_STACK_TASK_PRIORITY;
	param.ble_app_task_stack_size = BLE_APP_TASK_STACK_SIZE;
	param.ble_app_task_priority = BLE_APP_TASK_PRIORITY;
	param.en_cfg = 0;
	param.p_os_api = &os_interface;
	param.name_perm = BLE_GAP_WRITE_NOT_ENC;
	param.appearance_perm = BLE_GAP_WRITE_NOT_ENC;
	ble_sw_init(&param);
	//ble_adp_callback_register(app_adp_evt_handler);
	//app_adapter_init();
	//ble_adp_init();
	//ble_app_init();
	//ble_scan_init(BLE_GAP_LOCAL_ADDR_STATIC);
	ble_scan_callback_register(app_scan_mgr_evt_handler);
	ble_irq_enable();
	//ble_scan_reinit();
	g_bleStackReady = 1;
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BLE scan stack initialized");
	extern void coex_ble_event_notify(uint32_t event_start, uint32_t event_window, uint32_t iso_event);
	ble_coex_evt_notify_register(coex_ble_event_notify);
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	ble_status_t ret;
	if(g_bt_proxy.scan_active)
	{
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "Scan is processing, please stop it first");
	}
	else
	{
		HAL_BTScan_EnsureStackReady();
		//ble_scan_disable();
		ble_gap_scan_param_t scan_param = {
			.type = BLE_GAP_SCAN_TYPE_OBSERVER,
			.prop = BLE_GAP_SCAN_PROP_PHY_1M_BIT/* | BLE_GAP_SCAN_PROP_PHY_CODED_BIT | (scan_mode ? BLE_GAP_SCAN_PROP_ACTIVE_1M_BIT | BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT : 0)*/,
			.dup_filt_pol = isFiltering ? BLE_GAP_DUP_FILT_EN : BLE_GAP_DUP_FILT_DIS,
			.scan_intv_1m = g_bleScanInterval,
			.scan_win_1m = g_bleScanWindow,
			.scan_intv_coded = g_bleScanInterval,
			.scan_win_coded = g_bleScanWindow,
			.duration = 0,
			.period = 0,
		};
		ret = ble_scan_param_set(BLE_GAP_LOCAL_ADDR_STATIC, &scan_param);
		if(ret != BLE_ERR_NO_ERROR)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE set scan params (err %d)", ret);
			return;
		}

		ret = ble_scan_enable();
		if(ret != BLE_ERR_NO_ERROR)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE scan start failed (err %d)", ret);
			return;
		}

		g_bt_proxy.scan_active = true;
	}
}

void HAL_BTProxy_StopScan()
{
	if(!g_bt_proxy.init_done) return;
	ble_status_t ret;
	if(g_bt_proxy.scan_active)
	{
		ret = ble_scan_disable();
		if(ret != BLE_ERR_NO_ERROR)
		{
			ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BLE scan stop failed (err %d)", ret);
			return;
		}
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
	g_bleScanInterval = (uint16_t)((interval * 16) / 10);
	g_bleScanWindow = (uint16_t)((window * 16) / 10);
	HAL_BTProxy_StopScan();
	delay_ms(1);
	HAL_BTProxy_StartScan();
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

void HAL_BTProxy_FilterDuplicates(bool isActive)
{
	isFiltering = isActive;
	HAL_BTProxy_StopScan();
	delay_ms(1);
	HAL_BTProxy_StartScan();
}

#endif

#endif
