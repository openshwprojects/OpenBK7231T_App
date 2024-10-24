#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)


#include "wm_ble_client.h"
#include "wm_ble_client_api_multi_conn_demo.h"
#include "wm_ble_gap.h"
#include "wm_ble_gatt.h"
#include "wm_ble.h"
#include "wm_bt_util.h"
#include "wm_mem.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define MAX_CONN_DEVCIE_COUNT    7

typedef enum {
    DEV_DISCONNCTED = 0,
    DEV_CONNECTING,
    DEV_CONNECTED
} conn_state_t;

typedef enum {
    DEV_IDLE = 0,
    DEV_WAITING_CONFIGURE,
    DEV_TRANSFERING,
} transfer_state_t;

typedef enum {
    DEV_SCAN_IDLE,
    DEV_SCAN_RUNNING,
    DEV_SCAN_STOPPING,
} conn_scan_t;


typedef struct {

    conn_state_t conn_state;
    transfer_state_t transfer_state;
    uint8_t conn_retry;
    uint32_t client_if;
    tls_bt_addr_t addr;
    uint32_t conn_id;
    uint16_t conn_handle;
    uint16_t conn_mtu;
    uint8_t  remote_name[16];      /**parsed out from name filed in advertisement data, */
    int      conn_min_interval;
    uint16_t indicate_handle;
    uint16_t write_handle;
    uint16_t indicate_enable_handle;

} connect_device_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static connect_device_t conn_devices[MAX_CONN_DEVCIE_COUNT];
static conn_scan_t g_scan_state = DEV_SCAN_IDLE;

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

static int multi_conn_parse_adv_data(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data);
static tls_bt_status_t wm_ble_client_multi_conn_demo_api_connect(int id);
static void ble_client_demo_api_scan_result_callback(tls_bt_addr_t *addr, int rssi,
        uint8_t *adv_data);


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int get_conn_devices_index_wait_for_configure()
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //then check if all devcies connected
        if((conn_devices[i].conn_state == DEV_CONNECTED)
                && (conn_devices[i].transfer_state == DEV_WAITING_CONFIGURE)) {
            ret = i;
            break;
        }
    }

    return ret;
}

static int get_conn_devices_index_pending()
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //then check if all devcies connected
        if(conn_devices[i].conn_state == DEV_CONNECTING) {
            ret = i;
            break;
        }
    }

    return ret;
}


static int get_conn_devices_index_to_scan()
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //then check if all devcies connected
        if(conn_devices[i].conn_state == DEV_DISCONNCTED) {
            ret = i;
            break;
        }
    }

    return ret;
}

static int get_conn_devices_index_to_connect()
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //first check if one is connecting, we connect remote device one by one;
        if(conn_devices[i].conn_state == DEV_CONNECTING) {
            return ret;
        }
    }

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //then check if all devcies connected
        if(conn_devices[i].conn_state == DEV_DISCONNCTED) {
            ret = i;
            break;
        }
    }

    return ret;
}
static int get_conn_devices_index_to_connect_by_name(uint8_t len, const char *pname)
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //first check if one is connecting, we connect remote device one by one;
        if(conn_devices[i].conn_state == DEV_CONNECTING) {
            //printf("!!!!devices [%s] is connecting...\r\n", conn_devices[i].remote_name);
            return ret;
        }
    }

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //then check if all devcies connected
        //printf("len=%d, strlen=%d,state=%d,%s\r\n", len, strlen(conn_devices[i].remote_name),conn_devices[i].conn_state,conn_devices[i].remote_name);
        if((conn_devices[i].conn_state == DEV_DISCONNCTED)
                && (strncmp(conn_devices[i].remote_name, pname, len) == 0)
                && (strlen(conn_devices[i].remote_name) == len)) {
            ret = i;
        }
    }

    return ret;
}

static int get_conn_devices_index_to_connect_by_addr(tls_bt_addr_t *addr)
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        //first check if one is connecting, we connect remote device one by one;
        if(memcmp(&conn_devices[i].addr.address[0], &addr->address[0], 6) == 0) {
            ret = i;
            break;
        }
    }

    return ret;
}
static int get_conn_devices_index_to_connect_by_conn_id(int conn_id)
{
    int ret = -1;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_id == conn_id) {
            ret = i;
            break;
        }
    }

    return ret;
}

static void dump_conn_devices_status()
{
    int i = 0;
    TLS_BT_APPL_TRACE_DEBUG("=============================conn device information==============================\r\n");

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        TLS_BT_APPL_TRACE_DEBUG("%s[%02x:%02x:%02x:%02x:%02x:%02x],conn_state[%d], conn_id[%04d], conn_mtu[%03d]\r\n",
                                conn_devices[i].remote_name, conn_devices[i].addr.address[0], conn_devices[i].addr.address[1],
                                conn_devices[i].addr.address[2],
                                conn_devices[i].addr.address[3], conn_devices[i].addr.address[4], conn_devices[i].addr.address[5],
                                conn_devices[i].conn_state, conn_devices[i].conn_id, conn_devices[i].conn_mtu);
    }

    TLS_BT_APPL_TRACE_DEBUG("scanning state=%d\r\n", g_scan_state);
}
/**
*Description:  anayse the adv data and return device index need to be connected, return -1 when all devices connected ;
*
*
*/

static int multi_conn_parse_adv_data(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data)
{
    uint8_t offset = 0, len = 0, type = 0;  //MAX_BLE_DEV_COUNT
    int ret = -1;
    int scan_ret = -1;
    uint8_t dev_name[64];

    //02 01 02 07 09 48 55 41 57 45 49 00 00
    while(offset < 62) {
        len = adv_data[offset++];
        type = adv_data[offset++];

        if(len == 0) { break; }

        if(type == 0x09) {
            memcpy(dev_name, adv_data + offset, len - 1);
            dev_name[len - 1] = 0x00;
            //TLS_BT_APPL_TRACE_DEBUG("parsing device name:%s\r\n", dev_name);
            ret = get_conn_devices_index_to_connect_by_name(len - 1, adv_data + offset);

            if(ret >= 0) {
                TLS_BT_APPL_TRACE_DEBUG("%s, Found device(%s)\r\n", __FUNCTION__, conn_devices[ret].remote_name);
                memcpy(conn_devices[ret].addr.address, &addr->address[0], 6);
                break;
            } else {
                offset += (len - 1);
            }
        } else {
            offset += (len - 1);
        }
    }

    if(ret < 0) {
        scan_ret = get_conn_devices_index_to_scan();

        if(scan_ret >= 0) {
            ret = -2; //need to scan;
        }
    }

    return ret;
}


static void ble_report_evt_cb(tls_ble_dm_evt_t event, tls_ble_dm_msg_t *p_data)
{
    tls_ble_dm_scan_res_msg_t *msg = NULL;
    tls_bt_addr_t address;

    if(event == WM_BLE_DM_SCAN_RES_EVT) {
        msg = (tls_ble_dm_scan_res_msg_t *)&p_data->dm_scan_result;
        memcpy(address.address, msg->address, 6);
        ble_client_demo_api_scan_result_callback(&address, msg->rssi, msg->value);
    } else if(event == WM_BLE_DM_SCAN_RES_CMPL_EVT) {
        g_scan_state = DEV_SCAN_IDLE;
    }
}


/** Callback invoked in response to register_client */
static void ble_client_demo_api_register_client_callback(int status, int client_if,
        uint16_t app_uuid)
{
    TLS_BT_APPL_TRACE_DEBUG("%s ,status = %d, client_if = %d\r\n", __FUNCTION__, status, client_if);
    int i = 0;

    if(status == 0) {
        ///all conn_devices share with one client if;
        for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
            conn_devices[i].client_if = client_if;
        }

        TLS_BT_APPL_TRACE_DEBUG("Start to scan...\r\n");
        tls_ble_set_scan_param(0x08, 0x10, 0);
        g_scan_state = DEV_SCAN_RUNNING;
        status = tls_ble_scan(1);

        if(status == TLS_BT_STATUS_SUCCESS) {
            tls_ble_register_report_evt(WM_BLE_DM_SCAN_RES_EVT | WM_BLE_DM_SCAN_RES_CMPL_EVT,
                                        ble_report_evt_cb);
        }
    }
}
static void ble_client_demo_api_deregister_client_callback(int status, int client_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, client_if=%d\r\n", __FUNCTION__, client_if);
    /*clear ble output function ptr*/
}

static void ble_client_demo_api_scan_result_callback(tls_bt_addr_t *addr, int rssi,
        uint8_t *adv_data)
{
    int index = -1;
    int pending_index = -1;
    tls_bt_status_t status;
    index = multi_conn_parse_adv_data(addr, rssi, adv_data);

    if(index >= 0) {
        status = tls_ble_scan(0);

        if(status == TLS_BT_STATUS_SUCCESS) {
        }

        pending_index = get_conn_devices_index_pending();

        if(pending_index >= 0) {
            //do not support multi connect at the same time; we need to one by one???
            TLS_BT_APPL_TRACE_DEBUG("%s is  pending return...\r\n", conn_devices[pending_index].remote_name);
            return;
        }

        conn_devices[index].conn_state = DEV_CONNECTING;
        tls_dm_start_timer(tls_dm_get_timer_id(), 2000, wm_ble_client_multi_conn_demo_api_connect);
    } else if(index == -1) {
        //all devices connected
        TLS_BT_APPL_TRACE_DEBUG("wm_ble_deregister_report_evt\r\n");
        status = tls_ble_scan(0);

        if(status == TLS_BT_STATUS_SUCCESS) {
            tls_ble_deregister_report_evt(WM_BLE_DM_SCAN_RES_EVT | WM_BLE_DM_SCAN_RES_CMPL_EVT,
                                          ble_report_evt_cb);
        }
    } else {
        //contine to parse scan data;
    }
}
static void wm_ble_client_multi_conn_scan(int id)
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("Continue to SCAN next device to connect...\r\n");
    tls_dm_free_timer_id(id);
    tls_ble_set_scan_param(0x10, 0x20, 0);
    g_scan_state = DEV_SCAN_RUNNING;
    status = tls_ble_scan(true);

    if(status == TLS_BT_STATUS_SUCCESS) {
        tls_ble_register_report_evt(WM_BLE_DM_SCAN_RES_EVT | WM_BLE_DM_SCAN_RES_CMPL_EVT,
                                    ble_report_evt_cb);
    }
}

/** GATT open callback invoked in response to open */
static void ble_client_demo_api_connect_callback(int conn_id, int status, int client_if,
        tls_bt_addr_t *bda)
{
    int index = -1;
    TLS_BT_APPL_TRACE_DEBUG("%s, connected = %d, conn_id=%d\r\n", __FUNCTION__, status, conn_id);
    index = get_conn_devices_index_to_connect_by_addr(bda);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("we can not which device is connecting...\r\n");
        return;
    }

    if(status == 0) {
        conn_devices[index].conn_id = conn_id;
        conn_devices[index].conn_state = DEV_CONNECTED;
        tls_ble_client_search_service(conn_id, NULL);
        //try to check all devices connected, otherwise do scaning
        index = get_conn_devices_index_to_connect();

        if(index >= 0) {
            g_scan_state = DEV_SCAN_RUNNING;
            tls_dm_start_timer(tls_dm_get_timer_id(), 7000, wm_ble_client_multi_conn_scan);
        } else {
            //unregister report evt;
            TLS_BT_APPL_TRACE_DEBUG("All devices connected, unregister scan report callback\r\n");
            //There is no need to unregister it; if disconnect, we need to scan and connect again;
            //wm_ble_deregister_report_evt(WM_BLE_DM_SCAN_RES_EVT|WM_BLE_DM_SCAN_RES_CMPL_EVT, ble_report_evt_cb);
        }
    } else {
        TLS_BT_APPL_TRACE_WARNING("!!!Try to connect(%s) again, retry=%d, scanning__state=%d\r\n",
                                  conn_devices[index].remote_name, conn_devices[index].conn_retry, g_scan_state);
        //tls_dm_evt_triger(index,wm_ble_client_multi_conn_demo_api_connect);
        conn_devices[index].conn_state = DEV_DISCONNCTED;
        conn_devices[index].conn_retry++;

        if(conn_devices[index].conn_retry < 5) {
            if(g_scan_state != DEV_SCAN_RUNNING) {
                conn_devices[index].conn_state = DEV_CONNECTING;
                tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_multi_conn_demo_api_connect);
            }
        } else {
            conn_devices[index].conn_retry = 0;
            conn_devices[index].conn_state = DEV_DISCONNCTED;
            g_scan_state = DEV_SCAN_RUNNING;
            tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_multi_conn_scan);
        }
    }

    dump_conn_devices_status();
}

/** Callback invoked in response to close */
static void ble_client_demo_api_disconnect_callback(int conn_id, int status, int reason,
        int client_if, tls_bt_addr_t *bda)
{
    int index = -1;
    int index_next = -1;
    index = get_conn_devices_index_to_connect_by_conn_id(conn_id);
    TLS_BT_APPL_TRACE_DEBUG("%s, [%s]disconnected,status = %d,reason=%d,  conn_id=%d\r\n", __FUNCTION__,
                            conn_devices[index].remote_name, status, reason, conn_id);

    if(index >= 0) {
        conn_devices[index].conn_state = DEV_DISCONNCTED;
        /*check connection pending devices, if no, so scan*/
        index_next = get_conn_devices_index_pending();

        if(index_next < 0) {
            g_scan_state = DEV_SCAN_RUNNING;
            TLS_BT_APPL_TRACE_DEBUG("%s changed to DISCONNECTED, and continue to scan\r\n",
                                    conn_devices[index].remote_name);
            tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_multi_conn_scan);
        } else {
            TLS_BT_APPL_TRACE_DEBUG("%s changed to DISCONNECTED, but %s is connecting\r\n",
                                    conn_devices[index].remote_name, conn_devices[index_next].remote_name);
        }
    }

    dump_conn_devices_status();
}

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
static void ble_client_demo_api_search_complete_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    tls_ble_client_get_gatt_db(conn_id);
}

static void ble_client_demo_api_search_service_result_callback(int conn_id, tls_bt_uuid_t *p_uuid,
        uint8_t inst_id)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}
static tls_bt_status_t wm_ble_client_multi_demo_api_enable_indication(int id)
{
    int index = -1;
    uint8_t ind_enable[2] = {0x02, 0x00};
    tls_dm_free_timer_id(id);
    index = get_conn_devices_index_wait_for_configure();

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s failed, cannot found valid device\r\n", __FUNCTION__);
        return ;
    }

    conn_devices[index].transfer_state = DEV_TRANSFERING;
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, handle=%d\r\n", __FUNCTION__, conn_devices[index].conn_id,
                            conn_devices[index].indicate_enable_handle);
    tls_ble_client_write_characteristic(conn_devices[index].conn_id,
                                        conn_devices[index].indicate_enable_handle, 2, 2, 0, ind_enable);
    //tls_ble_client_write_characteristic(conn_devices[index].conn_id, conn_devices[index].indicate_enable_handle, 2, 2,0,ind_enable);
}

/** Callback invoked in response to [de]register_for_notification */
static void ble_client_demo_api_register_for_notification_callback(int conn_id,
        int registered, int status, uint16_t handle)
{
    int index = -1;
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, handle=%d,status=%d\r\n", __FUNCTION__, conn_id, handle,
                            status);
    index = get_conn_devices_index_to_connect_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s failed, cannot found valid device\r\n", __FUNCTION__);
        return ;
    }

    tls_ble_client_configure_mtu(conn_id, 247);
    conn_devices[index].transfer_state = DEV_WAITING_CONFIGURE;
    tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_multi_demo_api_enable_indication);
    //tls_dm_evt_triger(conn_id, wm_ble_client_demo_api_enable_indication);
}

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
static uint32_t g_recv_size = 0;
static uint32_t g_recv_count = 0;
static void ble_client_demo_api_notify_callback(int conn_id, uint8_t *value, tls_bt_addr_t *addr,
        uint16_t handle, uint16_t len, uint8_t is_notify)
{
#if 0
    g_recv_size += len;
    g_recv_count++;

    if(g_recv_count > 100) {
        g_recv_count = 0;
        printf("Recv bytes(%d)(is_notify=%d)\r\n", g_recv_size, is_notify);
    }

    int i = 0;
    int j = 0;
    printf("recv indication, len=%d(%d)\r\n", len, is_notify);

    for(i = 0; i < len; i++) {
        printf("%02x ", value[i]);
        j++;

        if(j == 16) {
            printf("\r\n");
            j = 0;
        }
    }

    printf("\r\n");
#endif
}

/** Reports result of a GATT read operation */
static void ble_client_demo_api_read_characteristic_callback(int conn_id, int status,
        uint16_t handle, uint8_t *value, int length, uint16_t value_type, uint8_t p_status)
{
    //hci_dbg_hexstring("read out:", value, length);
}

/** GATT write characteristic operation callback */
static void ble_client_demo_api_write_characteristic_callback(int conn_id, int status,
        uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT execute prepared write callback */
static void ble_client_demo_api_execute_write_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to read_descriptor */
static void ble_client_demo_api_read_descriptor_callback(int conn_id, int status,
        uint16_t handle, uint8_t *p_value, uint16_t length, uint16_t value_type, uint8_t pa_status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to write_descriptor */
static void ble_client_demo_api_write_descriptor_callback(int conn_id, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback triggered in response to read_remote_rssi */
static void ble_client_demo_api_read_remote_rssi_callback(int client_if, tls_bt_addr_t *bda,
        int rssi, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, client_if=%d\r\n", __FUNCTION__, client_if);
}

/**
 * Callback indicating the status of a listen() operation
 */
static void ble_client_demo_api_listen_callback(int status, int server_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, server_if=%d\r\n", __FUNCTION__, server_if);
}

/** Callback invoked when the MTU for a given connection changes */
static void ble_client_demo_api_configure_mtu_callback(int conn_id, int status, int mtu)
{
    int index = -1;
    index = get_conn_devices_index_to_connect_by_conn_id(conn_id);
    TLS_BT_APPL_TRACE_DEBUG("!!!!%s, conn_id=%d,mtu=%d\r\n", __FUNCTION__, conn_id, mtu);

    if(index >= 0) {
        conn_devices[index].conn_mtu = mtu - 3;
    }
}

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
static void ble_client_demo_api_congestion_callback(int conn_id, uint8_t congested)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT get database callback */
static void ble_client_demo_api_get_gatt_db_callback(int status, int conn_id,
        tls_btgatt_db_element_t *db, int count)
{
    int i = 0;
    int index = -1;
    uint16_t cared_handle, tmp_uuid;
    //hci_dbg_msg("===========btgattc_get_gatt_db_callback(count=%d)(conn_id=%d)================\r\n", count, conn_id);
    index = get_conn_devices_index_to_connect_by_conn_id(conn_id);

    if(index < 0) {
        TLS_BT_APPL_TRACE_ERROR("%s, can not found conn_id=%d in conn_devices table\r\n", __FUNCTION__,
                                conn_id);
        return;
    }

#if 0

    for(i = 0; i < count; i++) {
        if(db->type == 0) {
            //hci_dbg_hexstring("#", db->uuid.uu + 12, 2);
            TLS_BT_APPL_TRACE_DEBUG("type:%d, attr_handle:%d, properties:0x%02x, s=%d, e=%d\r\n", db->type,
                                    db->attribute_handle, db->properties, db->start_handle, db->end_handle);
        } else {
            //hci_dbg_hexstring("\t#", db->uuid.uu + 12, 2);
            tmp_uuid = db->uuid.uu[12] << 8 | db->uuid.uu[13];

            if(tmp_uuid == 0xBC2A) {
                cared_handle = db->attribute_handle;
            }

            TLS_BT_APPL_TRACE_DEBUG("\ttype:%d, attr_handle:%d, properties:0x%02x, s=%d, e=%d\r\n", db->type,
                                    db->attribute_handle, db->properties, db->start_handle, db->end_handle);
        }

        db++;
    }

#endif
    //Normally, these value should be get from the db; here I force to it because I know the server demo`s db already;
    conn_devices[index].indicate_handle = 44;
    conn_devices[index].write_handle = 42;
    conn_devices[index].indicate_enable_handle = 45;
    TLS_BT_APPL_TRACE_DEBUG("tls_ble_client_register_for_notification,[%s]\r\n",
                            conn_devices[index].remote_name);
    tls_ble_client_register_for_notification(conn_devices[index].client_if, &conn_devices[index].addr,
            conn_devices[index].indicate_handle, conn_id);
}

/** GATT services between start_handle and end_handle were removed */
static void ble_client_demo_api_services_removed_callback(int conn_id, uint16_t start_handle,
        uint16_t end_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT services were added */
static void ble_client_demo_api_services_added_callback(int conn_id, tls_btgatt_db_element_t *added,
        int added_count)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

static const wm_ble_client_callbacks_t  swmbleclientcb = {
    ble_client_demo_api_register_client_callback,
    ble_client_demo_api_deregister_client_callback,
    ble_client_demo_api_connect_callback,
    ble_client_demo_api_disconnect_callback,
    ble_client_demo_api_search_complete_callback,
    ble_client_demo_api_search_service_result_callback,
    ble_client_demo_api_register_for_notification_callback,
    ble_client_demo_api_notify_callback,
    ble_client_demo_api_read_characteristic_callback,
    ble_client_demo_api_write_characteristic_callback,
    ble_client_demo_api_read_descriptor_callback,
    ble_client_demo_api_write_descriptor_callback,
    ble_client_demo_api_execute_write_callback,
    ble_client_demo_api_read_remote_rssi_callback,
    ble_client_demo_api_listen_callback,
    ble_client_demo_api_configure_mtu_callback,
    ble_client_demo_api_congestion_callback,
    ble_client_demo_api_get_gatt_db_callback,
    ble_client_demo_api_services_removed_callback,
    ble_client_demo_api_services_added_callback,
} ;
static tls_bt_status_t wm_ble_client_multi_conn_demo_api_connect(int id)
{
    int index = -1;
    tls_dm_free_timer_id(id);
    index = get_conn_devices_index_pending();

    if(index < 0) {
        TLS_BT_APPL_TRACE_DEBUG("%s, invalid device index\r\n", __FUNCTION__);
        return;
    }

    TLS_BT_APPL_TRACE_DEBUG("%s  to:%s, address:%02x:%02x:%02x:%02x:%02x:%02x\r\n", __FUNCTION__,
                            conn_devices[index].remote_name,
                            conn_devices[index].addr.address[0], conn_devices[index].addr.address[1],
                            conn_devices[index].addr.address[2],
                            conn_devices[index].addr.address[3], conn_devices[index].addr.address[4],
                            conn_devices[index].addr.address[5]);
    tls_ble_conn_parameter_update(&conn_devices[index].addr, conn_devices[index].conn_min_interval,
                                  conn_devices[index].conn_min_interval + 2, 0, 0xA00);
    return tls_ble_client_connect(conn_devices[index].client_if, &conn_devices[index].addr, 1,
                                  WM_BLE_GATT_TRANSPORT_LE);
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_client_multi_conn_demo_api_init()
{
    tls_bt_status_t status;
    int i = 0;
    status = tls_ble_client_register_client(0x1234, &swmbleclientcb);

    if(status == TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_DEBUG("### %s success\r\n", __FUNCTION__);
        memset(&conn_devices, 0, sizeof(connect_device_t)*MAX_CONN_DEVCIE_COUNT);

        /**filled with cared device name, the client will connect them*/
        //for demo, I run 7 ble servers, the name is WMBL1,WMBL2,.....WMBL8;
        for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
            sprintf(conn_devices[i].remote_name, "WMBLEDEMO0%d", i + 1);
            conn_devices[i].conn_state = DEV_DISCONNCTED;
            conn_devices[i].conn_min_interval = 0x20 + i * 16;
            conn_devices[i].conn_mtu = 21; /*default GATT mtu*/
        }
    } else {
        //strange logical, at cmd task , bt host task, priority leads to this situation;
        TLS_BT_APPL_TRACE_ERROR("### %s failed\r\n", __FUNCTION__);
    }

    return status;
}

int tls_ble_client_multi_conn_demo_api_deinit()
{
    /*All connected devices share the same client if, anyone will be ok for unregistering;*/
    return tls_ble_client_unregister_client(conn_devices[0].client_if);
}
int tls_ble_client_multi_conn_demo_send_msg(uint8_t *ptr, int length)
{
}


#endif

