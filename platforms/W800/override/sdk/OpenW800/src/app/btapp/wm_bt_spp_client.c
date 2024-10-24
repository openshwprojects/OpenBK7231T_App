/*****************************************************************************
**
**  Name:           wm_bt_spp_client.c
**
**  Description:    This file contains the sample functions for bluetooth spp client application
**
*****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BTA_SPPC_INCLUDED == CFG_ON)
#include "wm_bt_app.h"
#include "wm_bt_spp.h"
#include "wm_bt_util.h"
#include "wm_bt_spp_client.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define SPP_CLIENT_NAME "WM_SPP_CLIENT"
#define TLS_SPP_DATA_LEN 800

typedef enum {
    WM_SPPC_IDLE,
    WM_SPPC_GENERAL_DISCOVERY,
    WM_SPPC_SERVICE_DISCOVERY,
    WM_SPPC_CONNECTING,
    WM_SPPC_INITING,
    WM_SPPC_OPENING,
    WM_SPPC_TRANSFERING,
    WM_SPPC_CONGESTING
} wm_sppc_state_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static tls_bt_addr_t dev_found_addr;
static uint8_t uuid_spp[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
static const wm_spp_sec_t sec_mask = WM_SPP_SEC_NONE;
static const tls_spp_role_t role_master = WM_SPP_ROLE_CLIENT;
static uint8_t tls_spp_data[TLS_SPP_DATA_LEN];
static wm_sppc_state_t sppc_state = WM_SPPC_IDLE;
static bool dev_found = false;
/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

static void tls_bt_host_spp_callback_handler(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg);

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void spp_remote_device_properties_callback(tls_bt_status_t status,
        tls_bt_addr_t *bd_addr,
        int num_properties,
        tls_bt_property_t *properties)
{
    int i = 0;
    TLS_BT_APPL_TRACE_DEBUG("app_remote_device_properties_callback:\r\n");

    for(i = 0; i < num_properties; i++) {
        TLS_BT_APPL_TRACE_DEBUG("\t%s:%s\r\n", dump_property_type(properties[i].type), properties[i].val);
    }
}
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static void spp_device_found_callback(int num_properties, tls_bt_property_t *properties)
{
    int i = 0;
    uint8_t dev_type = 0;
    uint32_t class_of_device = 0;
    int8_t dev_rssi = 0;
    uint8_t *p_value;
    tls_bt_addr_t dev_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    char dev_name[64] = {0};
    int dev_name_len = 0;
    //TLS_BT_APPL_TRACE_DEBUG("app_device_found_callback\r\n");

    for(i = 0; i < num_properties; i++) {
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
                TLS_BT_APPL_TRACE_WARNING("Unknown property\r\n");
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

    /*Note the spp server name  like this WM-XX:XX:XX, the client will think it is a spp server*/
    if(dev_name[0] == 'W' && dev_name[1] == 'M' && dev_name[2] == '-' && !dev_found) {
        TLS_BT_APPL_TRACE_DEBUG("spp_device_found, stop device discovery and do servcie discovery\r\n");
        tls_bt_cancel_discovery();
        dev_found = true;
        memcpy(dev_found_addr.address, dev_addr.address, 6);
    }
}
static void spp_discovery_state_changed_callback(tls_bt_discovery_state_t state)
{
    tls_bt_uuid_t spps_uuid;
    TLS_BT_APPL_TRACE_DEBUG("%s, state:%s\r\n", __FUNCTION__,
                            (state) ? "WM_BT_DISCOVERY_STARTED" : "WM_BT_DISCOVERY_STOPPED");

    if(state == WM_BT_DISCOVERY_STOPPED) {
        if(sppc_state != WM_SPPC_GENERAL_DISCOVERY) { return; }

        /*unregister the evetn*/
        wm_bt_deregister_report_evt(WM_BT_RMT_DEVICE_PROP_EVT | WM_BT_DEVICE_FOUND_EVT |
                                    WM_BT_DISCOVERY_STATE_CHG_EVT, tls_bt_host_spp_callback_handler);
        TLS_BT_APPL_TRACE_DEBUG("spp start service discovery [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
                                dev_found_addr.address[0],
                                dev_found_addr.address[1], dev_found_addr.address[2], dev_found_addr.address[3],
                                dev_found_addr.address[4], dev_found_addr.address[5]);

        if(dev_found == true) {
            memcpy(spps_uuid.uu, uuid_spp,  16);
            tls_bt_spp_start_discovery(&dev_found_addr, &spps_uuid);
            sppc_state = WM_SPPC_SERVICE_DISCOVERY;
        } else {
            /*continue to do discovery*/
            tls_bt_start_discovery();
        }
    }
}



static void tls_bt_host_spp_callback_handler(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg)
{
    TLS_BT_APPL_TRACE_EVENT("%s, event:%s,%d\r\n", __FUNCTION__, tls_bt_host_evt_2_str(evt), evt);

    switch(evt) {
        case WM_BT_ADAPTER_STATE_CHG_EVT:
        case WM_BT_ADAPTER_PROP_CHG_EVT:
            break;

        case WM_BT_RMT_DEVICE_PROP_EVT:
            spp_remote_device_properties_callback(msg->remote_device_prop.status,
                                                  msg->remote_device_prop.address,
                                                  msg->remote_device_prop.num_properties, msg->remote_device_prop.properties);
            break;

        case WM_BT_DEVICE_FOUND_EVT:
            spp_device_found_callback(msg->device_found.num_properties, msg->device_found.properties);
            break;

        case WM_BT_DISCOVERY_STATE_CHG_EVT:
            spp_discovery_state_changed_callback(msg->discovery_state.state);
            break;

        case WM_BT_BOND_STATE_CHG_EVT:
            break;
        case WM_BT_ACL_STATE_CHG_EVT:
        case WM_BT_ENERGY_INFO_EVT:
        case WM_BT_SSP_REQUEST_EVT:
        case WM_BT_PIN_REQUEST_EVT:
            break;
    }
}

static void btspp_init_status_callback(uint8_t status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, start general discovery\r\n", __FUNCTION__, status);
    wm_bt_register_report_evt(WM_BT_RMT_DEVICE_PROP_EVT | WM_BT_DEVICE_FOUND_EVT |
                              WM_BT_DISCOVERY_STATE_CHG_EVT|WM_BT_ACL_STATE_CHG_EVT, tls_bt_host_spp_callback_handler);
    tls_bt_start_discovery();
#if 0
    memcpy(spps_uuid.uu, uuid_spp,  16);
    tls_bt_spp_start_discovery(&dev_found_addr, &spps_uuid);
    sppc_state = WM_SPPC_SERVICE_DISCOVERY;
#endif
}

static void wm_bt_spp_callback(tls_spp_event_t evt, tls_spp_msg_t *msg)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, event=%s(%d)\r\n", __FUNCTION__, tls_spp_evt_2_str(evt), evt);

    switch(evt) {
        case WM_SPP_INIT_EVT:
            btspp_init_status_callback(msg->init_msg.status);
            sppc_state = WM_SPPC_GENERAL_DISCOVERY;
            break;

        case WM_SPP_DISCOVERY_COMP_EVT:
            TLS_BT_APPL_TRACE_DEBUG("status=%d scn_num=%d\r\n", msg->disc_comp_msg.status,
                                    msg->disc_comp_msg.scn_num);

            if(msg->disc_comp_msg.status == TLS_BT_STATUS_SUCCESS) {
                TLS_BT_APPL_TRACE_DEBUG("spp connect to [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
                                        dev_found_addr.address[0],
                                        dev_found_addr.address[1], dev_found_addr.address[2], dev_found_addr.address[3],
                                        dev_found_addr.address[4], dev_found_addr.address[5]);
                tls_bt_spp_connect(sec_mask, role_master, msg->disc_comp_msg.scn_num, &dev_found_addr);
                sppc_state = WM_SPPC_CONNECTING;
            } else if(msg->disc_comp_msg.status == TLS_BT_STATUS_FAIL) {
                    tls_bt_uuid_t spps_uuid;
                    memcpy(spps_uuid.uu, uuid_spp,  16);
                    tls_bt_spp_start_discovery(&dev_found_addr, &spps_uuid);
                    sppc_state = WM_SPPC_SERVICE_DISCOVERY;
            }

            break;

        case WM_SPP_OPEN_EVT:
            TLS_BT_APPL_TRACE_DEBUG("status=%d,handle=%d\r\n", msg->open_msg.status, msg->open_msg.handle);

            if(msg->open_msg.status == TLS_BT_STATUS_SUCCESS) {
                tls_bt_spp_write(msg->open_msg.handle,  tls_spp_data, TLS_SPP_DATA_LEN);
                sppc_state = WM_SPPC_TRANSFERING;
            }

            break;

        case WM_SPP_CLOSE_EVT:
            break;

        case WM_SPP_START_EVT:
            break;

        case WM_SPP_CL_INIT_EVT:
            TLS_BT_APPL_TRACE_DEBUG("status=%d,handle=%d,sec_id=%d,co_rfc=%d\r\n", msg->cli_init_msg.status,
                                    msg->cli_init_msg.handle,
                                    msg->cli_init_msg.sec_id, msg->cli_init_msg.use_co_rfc);
            break;

        case WM_SPP_DATA_IND_EVT:
            break;

        case WM_SPP_CONG_EVT:
            if(msg->congest_msg.congest == 0) {
                tls_bt_spp_write(msg->congest_msg.handle, tls_spp_data, TLS_SPP_DATA_LEN);
                sppc_state = WM_SPPC_TRANSFERING;
            }

            break;

        case WM_SPP_WRITE_EVT:
            if(msg->write_msg.congest == 0) {
                tls_bt_spp_write(msg->write_msg.handle, tls_spp_data, TLS_SPP_DATA_LEN);
                sppc_state = WM_SPPC_TRANSFERING;
            }

            break;

        case WM_SPP_SRV_OPEN_EVT:
            break;

        default:
            break;
    }
}
/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

tls_bt_status_t tls_bt_enable_spp_client()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __func__);
    status = tls_bt_spp_init(wm_bt_spp_callback);
    return tls_bt_spp_enable();
}

tls_bt_status_t tls_bt_disable_spp_client()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __func__);
    tls_bt_spp_disable();
    status = tls_bt_spp_deinit();
    return status;
}

#endif

