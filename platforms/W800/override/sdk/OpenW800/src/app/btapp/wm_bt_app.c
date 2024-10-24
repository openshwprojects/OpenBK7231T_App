/*****************************************************************************
**
**  Name:           wm_bt_app.c
**
**  Description:    This file contains the sample functions for bluetooth application
**
*****************************************************************************/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"
#include "btif_util.h"
#include "wm_params.h"

#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON )

#include "wm_bt_app.h"
#include "wm_bt_util.h"
#include "wm_pmu.h"
#include "wm_mem.h"
#include "list.h"
#include "wm_osal.h"


#if (WM_BLE_INCLUDED == CFG_ON)
#include "wm_ble_server_wifi_app.h"
#include "wm_ble_server_api_demo.h"
#include "wm_ble_uart_if.h"
#include "wm_ble_client_api_multi_conn_demo.h"
#endif

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
#include "wm_audio_sink.h"
#endif

#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
#include "wm_hfp_hsp_client.h"
#endif

#if (WM_BTA_SPPS_INCLUDED == CFG_ON)
#include "wm_bt_spp_server.h"
#endif
#if (WM_BTA_SPPC_INCLUDED == CFG_ON)
#include "wm_bt_spp_client.h"
#endif


/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

typedef struct {
    struct dl_list list;
    tls_bt_host_evt_t evt;
    tls_bt_host_callback_t reg_func_ptr;
} host_report_evt_t;


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static tls_bt_host_callback_t tls_bt_host_callback_at_ptr = NULL;
volatile static tls_bt_state_t bt_adapter_state = WM_BT_STATE_OFF;
static uint8_t bt_enabled_by_at = 0;
static uint8_t host_enabled_by_at = 0;
volatile static uint8_t bt_adapter_scaning = 0;
static host_report_evt_t host_report_evt_list = {{NULL, NULL}, 0, NULL} ;

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */
void wm_bt_init_evt_report_list();
void wm_bt_deinit_evt_report_list();

int demo_ble_scan(uint8_t start);
int demo_ble_adv(uint8_t type);
int demo_async_ble_scan(uint8_t start);
static void wm_bt_notify_evt_report(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg);

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_adapter_state_changed_callback(tls_bt_state_t status)
{
    tls_bt_host_msg_t msg;
    msg.adapter_state_change.status = status;
    TLS_BT_APPL_TRACE_DEBUG("adapter status = %s\r\n",
                            status == WM_BT_STATE_ON ? "bt_state_on" : "bt_state_off");
    bt_adapter_state = status;
#if (TLS_CONFIG_BLE == CFG_ON)

    if(status == WM_BT_STATE_ON) {
        TLS_BT_APPL_TRACE_VERBOSE("init base application\r\n");
        /* those funtions should be called basiclly*/
        tls_ble_gap_init();
        tls_ble_client_init();
        tls_ble_server_init();
        //at here , user run their own applications;
        //application_run();
        //demo_ble_adv(1);
        //demo_async_ble_scan(1);
        //tls_ble_server_demo_api_init(NULL);
        //wm_ble_client_multi_conn_demo_api_init();
    } else {
        TLS_BT_APPL_TRACE_VERBOSE("deinit base application\r\n");
        tls_ble_gap_deinit();
        tls_ble_client_deinit();
        tls_ble_server_deinit();
        //here, user may free their application;
        //application_stop();
        //tls_ble_server_demo_api_deinit();
    }

#endif
#if (WM_BT_INCLUDED == CFG_ON)

    if(status == WM_BT_STATE_ON) {
        /*class bluetooth application will be enabled by user*/
        //demo_bt_app_on();
    } else {
    }

#endif

    /*Notify at level application, if registered*/
    if(tls_bt_host_callback_at_ptr) {
        tls_bt_host_callback_at_ptr(WM_BT_ADAPTER_STATE_CHG_EVT, &msg);
    }
}

void app_adapter_properties_callback(tls_bt_status_t status,
                                     int num_properties,
                                     tls_bt_property_t *properties)
{
    tls_bt_host_msg_t msg;
    msg.adapter_prop.status = status;
    msg.adapter_prop.num_properties = num_properties;
    msg.adapter_prop.properties = properties;

    /*Notify at level application, if registered*/
    if(tls_bt_host_callback_at_ptr) { tls_bt_host_callback_at_ptr(WM_BT_ADAPTER_PROP_CHG_EVT, &msg); }

    int i = 0;
    int j = 0;
    tls_bt_addr_t *devices_list;
    uint8_t *p_value;
    TLS_BT_APPL_TRACE_DEBUG("app_adapter_properties_callback:\r\n");

    for(i = 0; i < num_properties; i++) {
        TLS_BT_APPL_TRACE_DEBUG("\t%s\r\n", dump_property_type((properties + i)->type));

        if(properties[i].type == WM_BT_PROPERTY_ADAPTER_BONDED_DEVICES) {
            TLS_BT_APPL_TRACE_DEBUG("\t\tbonded device count %d\r\n",  properties[i].len / 6);
            devices_list = (tls_bt_addr_t *)properties[i].val;

            for(j = 0; j < properties[i].len / 6; j++) {
                TLS_BT_APPL_TRACE_DEBUG("\t\tAddress:0x%02x:%02x:%02x:0x%02x:%02x:%02x\r\n",
                                        devices_list->address[0], devices_list->address[1], devices_list->address[2],
                                        devices_list->address[3], devices_list->address[4], devices_list->address[5]);
                devices_list++;
            }
        }

        if(properties[i].type == WM_BT_PROPERTY_BDNAME) {
            TLS_BT_APPL_TRACE_DEBUG("\t\t%s\r\n", (char *)properties[i].val);
        }

        if(properties[i].type == WM_BT_PROPERTY_BDADDR) {
            p_value = (uint8_t *)properties[i].val;
            TLS_BT_APPL_TRACE_DEBUG("\t\t%02x:%02x:%02x:%02x:%02x:%02x\r\n", p_value[0], p_value[1], p_value[2],
                                    p_value[3], p_value[4], p_value[5]);
        }

        if(properties[i].type == WM_BT_PROPERTY_ADAPTER_SCAN_MODE) {
            p_value = (uint8_t *)properties[i].val;

            switch(p_value[0]) {
                case 0x30:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_NONE\r\n");
                    break;

                case 0x31:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_CONNECTABLE\r\n");
                    break;

                case 0x32:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_CONNECTABLE_DISCOVERABLE\r\n");
                    break;
            }
        }

        if(properties[i].type == WM_BT_PROPERTY_TYPE_OF_DEVICE) {
            p_value = (uint8_t *)properties[i].val;
            TLS_BT_APPL_TRACE_DEBUG("\t\tDEVICE TYPE:%02x%02x%02x%02x\r\n", p_value[0], p_value[1], p_value[2],
                                    p_value[3]);
        }
    }
}
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

void app_remote_device_properties_callback(tls_bt_status_t status,
        tls_bt_addr_t *bd_addr,
        int num_properties,
        tls_bt_property_t *properties)
{
    int i = 0;
    uint8_t *p_value;
    TLS_BT_APPL_TRACE_DEBUG("app_remote_device_properties_callback:\r\n");

    for(i = 0; i < num_properties; i++) {
        TLS_BT_APPL_TRACE_DEBUG("\t%s\r\n", dump_property_type(properties[i].type));

        if(properties[i].type == WM_BT_PROPERTY_BDNAME) {
            TLS_BT_APPL_TRACE_DEBUG("\t\t%s\r\n", (char *)properties[i].val);
        }

        if(properties[i].type == WM_BT_PROPERTY_BDADDR) {
            p_value = (uint8_t *)properties[i].val;
            TLS_BT_APPL_TRACE_DEBUG("\t\t%02x:%02x:%02x:%02x:%02x:%02x\r\n", p_value[0], p_value[1], p_value[2],
                                    p_value[3], p_value[4], p_value[5]);
        }

        if(properties[i].type == WM_BT_PROPERTY_ADAPTER_SCAN_MODE) {
            p_value = (uint8_t *)properties[i].val;

            switch(p_value[0]) {
                case 0x30:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_NONE\r\n");
                    break;

                case 0x31:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_CONNECTABLE\r\n");
                    break;

                case 0x32:
                    TLS_BT_APPL_TRACE_DEBUG("\t\tBT_SCAN_MODE_CONNECTABLE_DISCOVERABLE\r\n");
                    break;
            }
        }

        if(properties[i].type == WM_BT_PROPERTY_TYPE_OF_DEVICE) {
            p_value = (uint8_t *)properties[i].val;
            TLS_BT_APPL_TRACE_DEBUG("\t\tDEVICE TYPE:%02x%02x%02x%02x\r\n", p_value[0], p_value[1], p_value[2],
                                    p_value[3]);
        }
    }
}


void app_device_found_callback(int num_properties, tls_bt_property_t *properties)
{
    int i = 0;
    uint8_t dev_type = 0;
    uint32_t class_of_device = 0;
    int8_t dev_rssi = 0;
    uint8_t *p_value;
    tls_bt_addr_t dev_addr = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    char dev_name[64] = {0};
    int dev_name_len = 0;

    //TLS_BT_APPL_TRACE_DEBUG("app_device_found_callback\r\n");

    for(i = 0; i < num_properties; i++) {
        //TLS_BT_APPL_TRACE_DEBUG("\t%s:", dump_property_type(properties[i].type));
        p_value = (uint8_t *)properties[i].val;

        switch(properties[i].type) {
            case WM_BT_PROPERTY_BDADDR:
                memcpy(&dev_addr.address[0], p_value, 6);
                break;

            case WM_BT_PROPERTY_BDNAME:
                dev_name_len = MIN(sizeof(dev_name) - 1, properties[i].len);
                memcpy(dev_name, p_value, dev_name_len);
                dev_name[dev_name_len] = '\0';
                break;

            case WM_BT_PROPERTY_CLASS_OF_DEVICE:
                class_of_device = devclass2uint(p_value);
                break;

            case WM_BT_PROPERTY_TYPE_OF_DEVICE:
                dev_type = p_value[0];
                break;

            case WM_BT_PROPERTY_REMOTE_RSSI:
                dev_rssi = (char)p_value[0];
                break;

            default:
                TLS_BT_APPL_TRACE_DEBUG("Unknown property\r\n");
                break;
        }
    }

    if(num_properties) {
        TLS_BT_APPL_TRACE_DEBUG("[%d,%02x:%02x:%02x:%02x:%02x:%02x, 0x%04x, %03d, %s]\r\n", dev_type,
                                dev_addr.address[0],
                                dev_addr.address[1], dev_addr.address[2], dev_addr.address[3], dev_addr.address[4],
                                dev_addr.address[5],
                                class_of_device, dev_rssi, dev_name);
    }
}
void app_discovery_state_changed_callback(tls_bt_discovery_state_t state)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, state:%s\r\n", __FUNCTION__,
                            (state) ? "WM_BT_DISCOVERY_STARTED" : "WM_BT_DISCOVERY_STOPPED");
}

void app_bond_state_changed_callback(tls_bt_status_t status,
                                     tls_bt_addr_t *remote_bd_addr,
                                     tls_bt_bond_state_t state)
{
    TLS_BT_APPL_TRACE_DEBUG("app_bond_state_changed_callback is called, state=(%d)\r\n", state);
}

#if 0
static void app_flush_bonded_params(uint8_t id)
{
    //tls_dm_free_timer_id(id);
    TLS_BT_APPL_TRACE_DEBUG("flush bonded params to flash\r\n");
    tls_param_to_flash(-1);
}
#endif

void app_acl_state_changed_callback(tls_bt_status_t status, tls_bt_addr_t *remote_bd_addr,
                                    tls_bt_acl_state_t state, uint8_t link_type)
{
    TLS_BT_APPL_TRACE_DEBUG("app_acl_state_changed_callback is called,[%02x:%02x:%02x:%02x:%02x:%02x],type=%s, state=(%s)\r\n",
                            remote_bd_addr->address[0], remote_bd_addr->address[1], remote_bd_addr->address[2],
                            remote_bd_addr->address[3],
                            remote_bd_addr->address[4], remote_bd_addr->address[5], (link_type == 1) ? "BR_EDR" : "BLE",
                            (state) ? "DISCONNECTED" : "CONNECTED");

    if((state == WM_BT_ACL_STATE_CONNECTED) && (link_type == 1)) {
        //demo_bt_scan_mode(0);
        //TODO , flush bonding information to flash
        //tls_dm_start_timer(tls_dm_get_timer_id(),3000,app_flush_bonded_params);
    } else {
        //demo_bt_scan_mode(2);
    }
}
void app_dut_mode_recv_callback(uint16_t opcode, uint8_t *buf, uint8_t len)
{
    TLS_BT_APPL_TRACE_DEBUG("%s %s %d is called, attention..\r\n", __FILE__, __FUNCTION__, __LINE__);
}

void app_energy_info_callback(tls_bt_activity_energy_info *energy_info)
{
    TLS_BT_APPL_TRACE_DEBUG("%s %s %d is called, attention..\r\n", __FILE__, __FUNCTION__, __LINE__);
}

void app_ssp_request_callback(tls_bt_addr_t *remote_bd_addr,
                              tls_bt_bdname_t *bd_name,
                              uint32_t cod,
                              tls_bt_ssp_variant_t pairing_variant,
                              uint32_t pass_key)
{
    TLS_BT_APPL_TRACE_DEBUG("app_ssp_request_callback, attention...(%s) cod=0x%08x, ssp_variant=%d, pass_key=0x%08x\r\n",
                            bd_name->name, cod, pairing_variant, pass_key);
    tls_bt_ssp_reply(remote_bd_addr, pairing_variant, 1, pass_key);
}

/** Bluetooth Legacy PinKey Request callback */
void app_pin_request_callback(tls_bt_addr_t *remote_bd_addr,
                              tls_bt_bdname_t *bd_name, uint32_t cod, uint8_t min_16_digit)
{
    TLS_BT_APPL_TRACE_DEBUG("app_request_callback\r\n");
}


static void tls_bt_host_callback_handler(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg)
{
    TLS_BT_APPL_TRACE_EVENT("%s, event:%s,%d\r\n", __FUNCTION__, tls_bt_host_evt_2_str(evt), evt);

    switch(evt) {
        case WM_BT_ADAPTER_STATE_CHG_EVT:
            app_adapter_state_changed_callback(msg->adapter_state_change.status);
            break;

        case WM_BT_ADAPTER_PROP_CHG_EVT:
            app_adapter_properties_callback(msg->adapter_prop.status, msg->adapter_prop.num_properties,
                                            msg->adapter_prop.properties);
            break;

        case WM_BT_RMT_DEVICE_PROP_EVT:
            app_remote_device_properties_callback(msg->remote_device_prop.status,
                                                  msg->remote_device_prop.address,
                                                  msg->remote_device_prop.num_properties, msg->remote_device_prop.properties);
            break;

        case WM_BT_DEVICE_FOUND_EVT:
            app_device_found_callback(msg->device_found.num_properties, msg->device_found.properties);
            break;

        case WM_BT_DISCOVERY_STATE_CHG_EVT:
            app_discovery_state_changed_callback(msg->discovery_state.state);
            break;

        case WM_BT_BOND_STATE_CHG_EVT:
            app_bond_state_changed_callback(msg->bond_state.status, msg->bond_state.remote_bd_addr,
                                            msg->bond_state.state);
            break;

        case WM_BT_ACL_STATE_CHG_EVT:
            app_acl_state_changed_callback(msg->acl_state.status, msg->acl_state.remote_address,
                                           msg->acl_state.state, msg->acl_state.link_type);
            break;

        case WM_BT_ENERGY_INFO_EVT:
            app_energy_info_callback(msg->energy_info.energy_info);
            break;

        case WM_BT_SSP_REQUEST_EVT:
            app_ssp_request_callback(msg->ssp_request.remote_bd_addr, msg->ssp_request.bd_name,
                                     msg->ssp_request.cod, msg->ssp_request.pairing_variant, msg->ssp_request.pass_key);
            break;

        case WM_BT_PIN_REQUEST_EVT:
            app_pin_request_callback(msg->pin_request.remote_bd_addr, msg->pin_request.bd_name,
                                     msg->pin_request.cod, msg->pin_request.min_16_digit);
            break;
		default:
			break;
    }

#if (WM_BT_INCLUDED == CFG_ON)
    //Notify applications who cares those event;
    wm_bt_notify_evt_report(evt, msg);
#endif
}


void tls_bt_entry()
{
    //demo_bt_enable(); //turn on bluetooth system;
}

void tls_bt_exit()
{
    //demo_bt_destroy(); //turn off bluetooth system;
}

int tls_at_bt_enable(int uart_no, tls_bt_log_level_t log_level,
                     tls_bt_host_callback_t at_callback_ptr)
{
    tls_bt_status_t status;
    bt_enabled_by_at = 1;
    tls_appl_trace_level = log_level;
    tls_bt_hci_if_t hci_if;

    if(host_enabled_by_at) {
        TLS_BT_APPL_TRACE_WARNING("bt host stack enabled by at+btcfghost=1, please do at+btcfghost=0, then continue...\r\n");
        return TLS_BT_STATUS_UNSUPPORTED;
    }

    if(tls_bt_host_callback_at_ptr) {
        TLS_BT_APPL_TRACE_WARNING("bt system already enabled\r\n");
        return TLS_BT_STATUS_DONE;
    }

    tls_bt_host_callback_at_ptr = at_callback_ptr;
    TLS_BT_APPL_TRACE_VERBOSE("bt system running, uart_no=%d, log_level=%d\r\n", uart_no, log_level);
    hci_if.uart_index = uart_no;
    hci_if.band_rate = 115200;
    hci_if.data_bit = 8;
    hci_if.stop_bit = 1;
    hci_if.verify_bit = 0;
#if (WM_BT_INCLUDED == CFG_ON)
    wm_bt_init_evt_report_list();
#endif
    status = tls_bt_enable(tls_bt_host_callback_handler, &hci_if, TLS_BT_LOG_NONE);

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        tls_bt_host_callback_at_ptr = NULL;
        TLS_BT_APPL_TRACE_ERROR("tls_bt_enable, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

    return status;
}
int tls_at_bt_destroy()
{
    tls_bt_status_t status;

    if(host_enabled_by_at) {
        TLS_BT_APPL_TRACE_WARNING("do not support, bt system enabled by at+btcfghost=1,n\r\n");
        return TLS_BT_STATUS_UNSUPPORTED;
    }

    bt_enabled_by_at = 0;

    if(tls_bt_host_callback_at_ptr == NULL) {
        TLS_BT_APPL_TRACE_WARNING("bt system already destroyed\r\n");
        return TLS_BT_STATUS_DONE;
    }

    TLS_BT_APPL_TRACE_VERBOSE("bt system destroy\r\n");
    status = tls_bt_disable();

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_disable, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

    return TLS_BT_STATUS_SUCCESS;
}

int tls_at_bt_cleanup_host()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("cleanup bluedroid\r\n");
    tls_bt_host_callback_at_ptr = NULL;
    status = tls_bt_host_cleanup();

    if(status != TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_host_cleanup, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

#if (WM_BT_INCLUDED == CFG_ON)
    wm_bt_deinit_evt_report_list();
#endif
    return status;
}



/*
*bluetooth api demo
*/
int demo_bt_enable()
{
    tls_bt_status_t status;
    uint8_t uart_no = 1;    //default we use uart 1 for testing;
    tls_appl_trace_level = TLS_BT_LOG_VERBOSE;
    tls_bt_hci_if_t hci_if;

    if(bt_adapter_state == WM_BT_STATE_ON) {
        TLS_BT_APPL_TRACE_VERBOSE("bt system enabled already");
        return TLS_BT_STATUS_SUCCESS;
    }

    TLS_BT_APPL_TRACE_VERBOSE("bt system running, uart_no=%d, log_level=%d\r\n", uart_no,
                              tls_appl_trace_level);
    hci_if.uart_index = uart_no;
    hci_if.band_rate = 115200;
    hci_if.data_bit = 8;
    hci_if.stop_bit = 1;
    hci_if.verify_bit = 0;
#if (WM_BT_INCLUDED == CFG_ON)
    wm_bt_init_evt_report_list();
#endif
    status = tls_bt_enable(tls_bt_host_callback_handler, &hci_if, TLS_BT_LOG_NONE);

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_enable, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

    return status;
}

int demo_bt_destroy()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_VERBOSE("bt system destroy\r\n");

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("bt system destroyed already");
        return TLS_BT_STATUS_SUCCESS;
    }

    status = tls_bt_disable();

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_disable, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

    while(bt_adapter_state == WM_BT_STATE_ON) {
        tls_os_time_delay(500);
    }

    TLS_BT_APPL_TRACE_VERBOSE("bt system cleanup host\r\n");
    status = tls_bt_host_cleanup();

    if(status != TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_host_cleanup, ret:%s,%d\r\n", tls_bt_status_2_str(status), status);
    }

#if (WM_BT_INCLUDED == CFG_ON)
    wm_bt_deinit_evt_report_list();
#endif
    return status;
}

#if (TLS_CONFIG_BLE == CFG_ON)


static uint8_t get_valid_adv_length_and_name(uint8_t *ptr, uint8_t *pname)
{
    uint8_t ret = 0, seg_len = 0, min_len = 0;
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

    if(ptr == NULL) { return ret; }

    seg_len = ptr[0];

    while(seg_len != 0) {
        if(ptr[ret + 1] == 0x09 || ptr[ret + 1] == 0x08) {
            min_len = MIN(64, seg_len - 1);
            memcpy(pname, ptr + ret + 2, min_len);
            pname[min_len] = 0;
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
        }

        ret += (seg_len + 1); //it self 1 byte;
        seg_len = ptr[ret];

        if(ret >= 64) { break; } //sanity check;
    }

    return ret;
}

static void demo_ble_scan_report_cb(tls_ble_dm_evt_t event, tls_ble_dm_msg_t *p_data)
{
    if((event != WM_BLE_DM_SCAN_RES_EVT) && (event != WM_BLE_DM_SCAN_RES_CMPL_EVT)) { return; }

#define BLE_SCAN_RESULT_LEN 256
    int len = 0, i = 0;
    char *buf;
    buf = tls_mem_alloc(BLE_SCAN_RESULT_LEN);

    if(!buf) {
        TLS_BT_APPL_TRACE_ERROR("alloc failed\r\n");
        return;
    }

    switch(event) {
        case WM_BLE_DM_SCAN_RES_EVT: {
            tls_ble_dm_scan_res_msg_t *msg = (tls_ble_dm_scan_res_msg_t *)&p_data->dm_scan_result;
            u8 valid_len;
            u8 device_name[64] = {0};
            memset(buf, 0, BLE_SCAN_RESULT_LEN);
            memset(device_name, 0, sizeof(device_name));
            valid_len = get_valid_adv_length_and_name(msg->value, device_name);

            if(valid_len > 62) {
                //printf("###warning(%d)###\r\n", valid_len);
                valid_len = 62;
            }

            len = sprintf(buf, "%02X%02X%02X%02X%02X%02X,%d,",
                          msg->address[0], msg->address[1], msg->address[2],
                          msg->address[3], msg->address[4], msg->address[5], msg->rssi);

            if(device_name[0] != 0x00) {
                len += sprintf(buf + len, "\"%s\",", device_name);
            } else {
                len += sprintf(buf + len, "\"\",");
            }

            for(i = 0; i < valid_len; i++) {
                len += sprintf(buf + len, "%02X", msg->value[i]);
            }

            len += sprintf(buf + len, "\r\n");
            buf[len++] = '\0';
            TLS_BT_APPL_TRACE_VERBOSE("%s\r\n", buf);
        }
        break;

        case WM_BLE_DM_SCAN_RES_CMPL_EVT: {
            tls_ble_dm_scan_res_cmpl_msg_t *msg = (tls_ble_dm_scan_res_cmpl_msg_t *)
                                                  &p_data->dm_scan_result_cmpl;
            TLS_BT_APPL_TRACE_VERBOSE("scan ended, ret=%d\r\n", msg->num_responses);
            bt_adapter_scaning = 0;
        }
        break;

        default:
            break;
    }

    if(buf)
    { tls_mem_free(buf); }
}
int tls_ble_demo_scan(uint8_t start)
{
    tls_bt_status_t ret;
    TLS_BT_APPL_TRACE_VERBOSE("demo_ble_scan=%d\r\n", start);

    if(start) {
        tls_ble_set_scan_param(0x40, 0x60, 0);
        //tls_ble_set_scan_param(0x100, 0x100, 0);
        ret = tls_ble_scan(TRUE);

        if(ret == TLS_BT_STATUS_SUCCESS) {
            bt_adapter_scaning = 1;
            ret = tls_ble_register_report_evt(WM_BLE_DM_SCAN_RES_EVT | WM_BLE_DM_SCAN_RES_CMPL_EVT,
                                              demo_ble_scan_report_cb);
        }
    } else {
        ret = tls_ble_scan(FALSE);

        if(ret == TLS_BT_STATUS_SUCCESS) {
            //wait scan stop;
            while(bt_adapter_scaning) {
                tls_os_time_delay(500);
            }

            //unregister the callback
            ret = tls_ble_deregister_report_evt(WM_BLE_DM_SCAN_RES_EVT | WM_BLE_DM_SCAN_RES_CMPL_EVT,
                                                demo_ble_scan_report_cb);
        }
    }

    return ret;
}

static void ble_scan_enable_cb(uint8_t triger_id)
{
    TLS_BT_APPL_TRACE_VERBOSE("tls_ble_adv=%d\r\n", triger_id);
    demo_ble_scan(triger_id);
}

int demo_async_ble_scan(uint8_t start)
{
    tls_dm_evt_triger(start, ble_scan_enable_cb);
}

static void ble_adv_enable_cb(uint8_t triger_id)
{
    TLS_BT_APPL_TRACE_VERBOSE("tls_ble_adv=%d\r\n", triger_id);
    tls_ble_adv(triger_id);
}

int tls_ble_demo_adv(uint8_t type)
{
    TLS_BT_APPL_TRACE_VERBOSE("demo_ble_adv=%d\r\n", type);

    if(type) {
        tls_ble_dm_adv_data_t data;
        uint8_t bt_mac[6] = {0};
        uint8_t adv_data[31] = {
            0x0C, 0x09, 'W', 'M', '-', '0', '0', '0', '0', '0', '0', '0', '0',
            0x02, 0x01, 0x05,
            0x03, 0x19, 0xc1, 0x03
        };
        memset(&data, 0, sizeof(data));
        extern int tls_get_bt_mac_addr(uint8_t *mac);
        tls_get_bt_mac_addr(bt_mac);
        sprintf(adv_data + 5, "%02X:%02X:%02X", bt_mac[3], bt_mac[4], bt_mac[5]);
        adv_data[13] = 0x02;  //byte 13 was overwritten to zero by sprintf; recover it;
        data.set_scan_rsp = false;   //advertisement data;
        data.pure_data = true;       //only manufacture data is inclucded in the advertisement payload
        data.manufacturer_len = 20;   //configure payload length;
        memcpy(data.manufacturer_data, adv_data, 20);//copy payload ;
        tls_ble_set_adv_data(&data); //configure advertisement data;
#if 0
        uint8_t scan_resp_data[31] = {0x0C, 0x09, 'W', 'M', '-', '0', '0', '0', '0', '0', '0', '0', '0'};
        memset(&data, 0, sizeof(data));
        sprintf(scan_resp_data + 5, "%02X:%02X:%02X", bt_mac[3], bt_mac[4], bt_mac[5]);
        data.set_scan_rsp = true;    //scan response data;
        data.pure_data = true;       //only manufacture data is inclucded in the scan response payload
        data.manufacturer_len = 13;   //configure payload length;
        memcpy(data.manufacturer_data, scan_resp_data, 13);//copy payload ;
        tls_ble_set_adv_data(&data); //configure advertisement data;
#endif
        tls_ble_dm_adv_param_t adv_param;

        if(type == 1) {
            adv_param.adv_int_min = 0x64; //interval min;
            adv_param.adv_int_max = 0x64; //interval max;
        } else {
            adv_param.adv_int_min = 0xA0; //for nonconnectable advertisement, interval min is 0xA0;
            adv_param.adv_int_max = 0xA0; //interval max;
        }

        adv_param.dir_addr = NULL;    //directed address NULL;
        tls_ble_set_adv_param(&adv_param); //configure advertisement parameters;
        /*enable advertisement*/
        tls_dm_evt_triger(type, ble_adv_enable_cb);
    } else {
        tls_ble_adv(0);
    }

    return TLS_BT_STATUS_SUCCESS;
}

int demo_ble_server_on()
{
    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("please enable bluetooth system first\r\n");
        return -1;
    }

    tls_ble_server_demo_api_init(NULL);
    return 0;
}
int demo_ble_server_off()
{
    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("bluetooth system stopped\r\n");
        return -1;
    }

    tls_ble_server_demo_api_deinit();
    return 0;
}
int demo_ble_client_on()
{
    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("please enable bluetooth system first\r\n");
        return -1;
    }

    tls_ble_client_demo_api_init(NULL);
    return 0;
}
int demo_ble_client_off()
{
    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("bluetooth system stopped\r\n");
        return -1;
    }

    tls_ble_client_demo_api_deinit();
    return 0;
}

int demo_ble_uart_server_on(uint8_t uart_no)
{
    return tls_ble_uart_init(BLE_UART_SERVER_MODE, uart_no, NULL);
}

int demo_ble_uart_server_off()
{
    return tls_ble_uart_deinit(BLE_UART_SERVER_MODE, 0xFF);
}
int demo_ble_uart_client_on(uint8_t uart_no)
{
    return tls_ble_uart_init(BLE_UART_CLIENT_MODE, uart_no, NULL);
}

int demo_ble_uart_client_off()
{
    return tls_ble_uart_deinit(BLE_UART_CLIENT_MODE, 0xFF);
}
int demo_ble_adv(uint8_t type)
{
    return tls_ble_demo_adv(type);
}
int demo_ble_scan(uint8_t start)
{
    return tls_ble_demo_scan(start);
}

#endif

#if (WM_BT_INCLUDED == CFG_ON)

int demo_bt_app_on()
{
    tls_bt_property_t btp;
    TLS_BT_APPL_TRACE_DEBUG("demo_bt_app_on\r\n");

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("please enable bluetooth system first\r\n");
        return -1;
    }

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
    tls_bt_enable_a2dp_sink();
#endif
#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
    tls_bt_enable_hfp_client();
#endif
#if (WM_BTA_SPPS_INCLUDED == CFG_ON)
    tls_bt_enable_spp_server();
#endif
    /*
        BT_SCAN_MODE_NONE,                     0
        BT_SCAN_MODE_CONNECTABLE,              1
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE  2
        */
    btp.type = WM_BT_PROPERTY_ADAPTER_SCAN_MODE;
    btp.len = 1;
    btp.val = "2";
    tls_bt_set_adapter_property(&btp, 0);
    return 0;
}

int demo_bt_app_off()
{
    tls_bt_property_t btp;
    TLS_BT_APPL_TRACE_DEBUG("demo_bt_app_off\r\n");

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("please enable bluetooth system first\r\n");
        return -1;
    }

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
    tls_bt_disable_a2dp_sink();
#endif
#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
    tls_bt_disable_hfp_client();
#endif
#if (WM_BTA_SPPS_INCLUDED == CFG_ON)
    tls_bt_disable_spp_server();
#endif
    /*
        BT_SCAN_MODE_NONE,                     0
        BT_SCAN_MODE_CONNECTABLE,              1
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE  2
        */
    btp.type = WM_BT_PROPERTY_ADAPTER_SCAN_MODE;
    btp.len = 1;
    btp.val = "0";
    tls_bt_set_adapter_property(&btp, 0);
    return 0;
}

int demo_bt_scan_mode(int type)
{
    tls_bt_property_t btp;

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("please enable bluetooth system first\r\n");
        return -1;
    }

    /*
        BT_SCAN_MODE_NONE,                     0
        BT_SCAN_MODE_CONNECTABLE,              1
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE  2
        */
    btp.type = WM_BT_PROPERTY_ADAPTER_SCAN_MODE;
    btp.len = 1;

    if(type == 2) {
        btp.val = "2";
    } else if(type == 1) {
        btp.val = "1";
    } else {
        btp.val = "0";
    }

    TLS_BT_APPL_TRACE_DEBUG("bt_scan_mode=%d\r\n", type);
    tls_bt_set_adapter_property(&btp, 0);
	return 0;
}

int demo_bt_inquiry(int type)
{
    tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;
    ret = tls_bt_start_discovery();
    return ret;
}
void wm_bt_init_evt_report_list()
{
    /*check initialized or not*/
    if(host_report_evt_list.list.next != NULL)
    { return; }

    dl_list_init(&host_report_evt_list.list);
}
void wm_bt_deinit_evt_report_list()
{
    uint32_t cpu_sr;
    host_report_evt_t *evt = NULL;
    host_report_evt_t *evt_next = NULL;

    if(host_report_evt_list.list.next == NULL)
    { return; }

    if(dl_list_empty(&host_report_evt_list.list))
    { return ; }

    cpu_sr = tls_os_set_critical();
    dl_list_for_each_safe(evt, evt_next, &host_report_evt_list.list, host_report_evt_t, list) {
        dl_list_del(&evt->list);
        tls_mem_free(evt);
    }
    tls_os_release_critical(cpu_sr);
}
void wm_bt_notify_evt_report(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg)
{
    uint32_t cpu_sr;
    host_report_evt_t *report_evt = NULL;
    host_report_evt_t *report_evt_next = NULL;

    if(host_report_evt_list.list.next == NULL)
    { return; }

    cpu_sr = tls_os_set_critical();

    if(!dl_list_empty(&host_report_evt_list.list)) {
        dl_list_for_each_safe(report_evt, report_evt_next, &host_report_evt_list.list, host_report_evt_t,
                              list) {
            tls_os_release_critical(cpu_sr);

            if((report_evt) && (report_evt->evt & evt) && (report_evt->reg_func_ptr)) {
                report_evt->reg_func_ptr(evt, msg);
            }

            cpu_sr = tls_os_set_critical();
        }
    }

    tls_os_release_critical(cpu_sr);
}
tls_bt_status_t wm_bt_register_report_evt(tls_bt_host_evt_t rpt_evt,
        tls_bt_host_callback_t rpt_callback)
{
    uint32_t cpu_sr;
    host_report_evt_t *evt = NULL;

    if(host_report_evt_list.list.next == NULL)
    { return TLS_BT_STATUS_NOMEM; }

    cpu_sr = tls_os_set_critical();
    dl_list_for_each(evt, &host_report_evt_list.list, host_report_evt_t, list) {
        if(evt->reg_func_ptr == rpt_callback) {
            if(evt->evt & rpt_evt) {
                /*Already in the list, do nothing*/
                tls_os_release_critical(cpu_sr);
                return TLS_BT_STATUS_SUCCESS;
            } else {
                /*Appending this evt to monitor list*/
                tls_os_release_critical(cpu_sr);
                evt->evt |= rpt_evt;
                return TLS_BT_STATUS_SUCCESS;
            }
        }
    }
    tls_os_release_critical(cpu_sr);
    evt = tls_mem_alloc(sizeof(host_report_evt_t));

    if(evt == NULL) {
        return TLS_BT_STATUS_NOMEM;
    }

    memset(evt, 0, sizeof(host_report_evt_t));
    evt->reg_func_ptr = rpt_callback;
    evt->evt = rpt_evt;
    cpu_sr = tls_os_set_critical();
    dl_list_add_tail(&host_report_evt_list.list, &evt->list);
    tls_os_release_critical(cpu_sr);
    return TLS_BT_STATUS_SUCCESS;
}
tls_bt_status_t wm_bt_deregister_report_evt(tls_bt_host_evt_t rpt_evt,
        tls_bt_host_callback_t rpt_callback)
{
    uint32_t cpu_sr;
    host_report_evt_t *evt = NULL;
    host_report_evt_t *evt_next = NULL;

    if(host_report_evt_list.list.next == NULL)
    { return TLS_BT_STATUS_NOMEM; }

    cpu_sr = tls_os_set_critical();

    if(!dl_list_empty(&host_report_evt_list.list)) {
        dl_list_for_each_safe(evt, evt_next, &host_report_evt_list.list, host_report_evt_t, list) {
            tls_os_release_critical(cpu_sr);

            if((evt->reg_func_ptr == rpt_callback)) {
                evt->evt &= ~rpt_evt; //clear monitor bit;

                if(evt->evt == 0) {  //no evt left;
                    dl_list_del(&evt->list);
                    tls_mem_free(evt);
                    evt = NULL;
                }
            }

            cpu_sr = tls_os_set_critical();
        }
    }

    tls_os_release_critical(cpu_sr);
    return TLS_BT_STATUS_SUCCESS;
}

#endif

#endif
