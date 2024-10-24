#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)


#include "wm_ble_client.h"
#include "wm_ble_client_api_demo.h"
#include "wm_ble_gap.h"
#include "wm_ble_gatt.h"
#include "wm_ble.h"
#include "wm_ble_uart_if.h"
#include "wm_bt_util.h"
#include "wm_mem.h"


#define BTA_GATT_AUTH_REQ_NONE           0
#define BTA_GATT_AUTH_REQ_NO_MITM        1            /* unauthenticated encryption */
#define BTA_GATT_AUTH_REQ_MITM           2               /* authenticated encryption */
#define BTA_GATT_AUTH_REQ_SIGNED_NO_MITM 3
#define BTA_GATT_AUTH_REQ_SIGNED_MITM    4



#define BTA_GATT_PERM_READ              (1 << 0) /* bit 0 */
#define BTA_GATT_PERM_READ_ENCRYPTED    (1 << 1) /* bit 1 */
#define BTA_GATT_PERM_READ_ENC_MITM     (1 << 2) /* bit 2 */
#define BTA_GATT_PERM_WRITE             (1 << 4) /* bit 4 */
#define BTA_GATT_PERM_WRITE_ENCRYPTED   (1 << 5) /* bit 5 */
#define BTA_GATT_PERM_WRITE_ENC_MITM    (1 << 6) /* bit 6 */
#define BTA_GATT_PERM_WRITE_SIGNED      (1 << 7) /* bit 7 */
#define BTA_GATT_PERM_WRITE_SIGNED_MITM (1 << 8) /* bit 8 */

static int g_client_if;
static tls_bt_addr_t g_bd_addr;
static int g_conn_id;
static uint16_t g_indication_handle = 0x00;
static uint16_t g_write_handle = 0x00;
static tls_ble_output_func_ptr g_ble_output_fptr = NULL;
static volatile uint8_t g_send_pending = 0;
static int g_mtu = 21;

/**
*Description:  anayse the adv data and return true when cared content is found
*
*
*/
static bool analyse_adv_data(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data);
static tls_bt_status_t wm_ble_client_demo_api_connect(int id);
void ble_client_demo_api_scan_result_callback(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data);


static void ble_report_evt_cb(tls_ble_dm_evt_t event, tls_ble_dm_msg_t *p_data)
{
    tls_ble_dm_scan_res_msg_t *msg = NULL;
    tls_bt_addr_t address;

    if(event == WM_BLE_DM_SCAN_RES_EVT) {
        msg = (tls_ble_dm_scan_res_msg_t *)&p_data->dm_scan_result;
        memcpy(address.address, msg->address, 6);
        ble_client_demo_api_scan_result_callback(&address, msg->rssi, msg->value);
    }
}


/** Callback invoked in response to register_client */
void ble_client_demo_api_register_client_callback(int status, int client_if,
        uint16_t app_uuid)
{
    //TLS_BT_APPL_TRACE_DEBUG("%s ,status = %d, client_if = %d\r\n", __FUNCTION__, status, client_if);
    if(status == 0) {
        g_client_if = client_if;
        status = tls_ble_scan(1);

        if(status == TLS_BT_STATUS_SUCCESS) {
            tls_ble_register_report_evt(WM_BLE_DM_SCAN_RES_EVT, ble_report_evt_cb);
        }
    }
}
void ble_client_demo_api_deregister_client_callback(int status, int client_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, client_if=%d\r\n", __FUNCTION__, client_if);
    /*clear ble output function ptr*/
    g_ble_output_fptr = NULL;
}

void ble_client_demo_api_scan_result_callback(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data)
{
    bool found = false;
    tls_bt_status_t status;
    //hci_dbg_hexstring("scan result callback:", adv_data, 32);
    found = analyse_adv_data(addr, rssi, adv_data);

    if(found) {
        status = tls_ble_scan(0);

        if(status == TLS_BT_STATUS_SUCCESS) {
            tls_ble_deregister_report_evt(WM_BLE_DM_SCAN_RES_EVT, ble_report_evt_cb);
        }

        tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_demo_api_connect);
    }
}

/** GATT open callback invoked in response to open */
void ble_client_demo_api_connect_callback(int conn_id, int status, int client_if,
        tls_bt_addr_t *bda)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, connected = %d, conn_id=%d\r\n", __FUNCTION__, status, conn_id);

    if(status == 0) {
        g_conn_id = conn_id;
        tls_ble_client_search_service(conn_id, NULL);
    } else {
        TLS_BT_APPL_TRACE_WARNING("Try to connect again...\r\n");
        tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_demo_api_connect);
    }
}

/** Callback invoked in response to close */
void ble_client_demo_api_disconnect_callback(int conn_id, int status, int reason,
        int client_if, tls_bt_addr_t *bda)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, disconnect,status = %d,reason=%d,  conn_id=%d\r\n", __FUNCTION__,
                            status, reason, conn_id);
}

/**
 * Invoked in response to search_service when the GATT service search
 * has been completed.
 */
void ble_client_demo_api_search_complete_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
    tls_ble_client_get_gatt_db(conn_id);
}

void ble_client_demo_api_search_service_result_callback(int conn_id, tls_bt_uuid_t *p_uuid,
        uint8_t inst_id)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}
static tls_bt_status_t wm_ble_client_demo_api_enable_indication(int id)
{
    uint8_t ind_enable[2];
    TLS_BT_APPL_TRACE_DEBUG(">>>>%s, conn_id=%d, handle=%d\r\n", __FUNCTION__, g_conn_id, 45);
    tls_dm_free_timer_id(id);
    ind_enable[0] = 0x02;
    ind_enable[1] = 0x00;
    tls_ble_client_write_characteristic(g_conn_id, 45, 2, 2, 0, ind_enable);
    tls_ble_client_write_characteristic(g_conn_id, 45, 2, 2, 0, ind_enable);
}

/** Callback invoked in response to [de]register_for_notification */
void ble_client_demo_api_register_for_notification_callback(int conn_id,
        int registered, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d, handle=%d,status=%d\r\n", __FUNCTION__, conn_id, handle,
                            status);
    tls_ble_client_configure_mtu(conn_id, 247);
    tls_dm_start_timer(tls_dm_get_timer_id(), 1000, wm_ble_client_demo_api_enable_indication);
}

/**
 * Remote device notification callback, invoked when a remote device sends
 * a notification or indication that a client has registered for.
 */
static uint32_t g_recv_size = 0;
static uint32_t g_recv_count = 0;
void ble_client_demo_api_notify_callback(int conn_id, uint8_t *value, tls_bt_addr_t *addr,
        uint16_t handle, uint16_t len, uint8_t is_notify)
{
    g_recv_size += len;
    g_recv_count++;

    if(g_recv_count > 100) {
        g_recv_count = 0;
        printf("Recv bytes(%d)(is_notify=%d)\r\n", g_recv_size, is_notify);
    }

    if(g_ble_output_fptr) {
        g_ble_output_fptr(value, len);
    }

#if 0
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
void ble_client_demo_api_read_characteristic_callback(int conn_id, int status,
        uint16_t handle, uint8_t *value, int length, uint16_t value_type, uint8_t p_status)
{
    //hci_dbg_hexstring("read out:", value, length);
}

/** GATT write characteristic operation callback */
void ble_client_demo_api_write_characteristic_callback(int conn_id, int status, uint16_t handle)
{
    uint32_t len = 0;
    uint8_t *tmp_ptr;
    tls_bt_status_t ret;
    g_send_pending = 0;
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);

    if(g_ble_output_fptr) {
        len = tls_ble_uart_buffer_size();
        len = MIN(len, g_mtu);

        if(len) {
            tmp_ptr = tls_mem_alloc(len);

            if(tmp_ptr == NULL) {
                TLS_BT_APPL_TRACE_WARNING("!!!ble_server_indication_sent_cb NO enough memory\r\n");
                return;
            }

            tls_ble_uart_buffer_peek(tmp_ptr, len);
            g_send_pending = 1;
            ret = tls_ble_client_write_characteristic(g_conn_id, g_write_handle, 2, len, 0, tmp_ptr);
            tls_mem_free(tmp_ptr);

            if(ret == TLS_BT_STATUS_SUCCESS) {
                tls_ble_uart_buffer_delete(len);
            }
        }
    }
}

/** GATT execute prepared write callback */
void ble_client_demo_api_execute_write_callback(int conn_id, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to read_descriptor */
void ble_client_demo_api_read_descriptor_callback(int conn_id, int status,
        uint16_t handle, uint8_t *p_value, uint16_t length, uint16_t value_type, uint8_t pa_status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback invoked in response to write_descriptor */
void ble_client_demo_api_write_descriptor_callback(int conn_id, int status, uint16_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** Callback triggered in response to read_remote_rssi */
void ble_client_demo_api_read_remote_rssi_callback(int client_if, tls_bt_addr_t *bda,
        int rssi, int status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, client_if=%d\r\n", __FUNCTION__, client_if);
}

/**
 * Callback indicating the status of a listen() operation
 */
void ble_client_demo_api_listen_callback(int status, int server_if)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, server_if=%d\r\n", __FUNCTION__, server_if);
}

/** Callback invoked when the MTU for a given connection changes */
void ble_client_demo_api_configure_mtu_callback(int conn_id, int status, int mtu)
{
    TLS_BT_APPL_TRACE_DEBUG("!!!!%s, conn_id=%d,mtu=%d\r\n", __FUNCTION__, conn_id, mtu);
    g_mtu = mtu - 3;
}

/**
 * Callback notifying an application that a remote device connection is currently congested
 * and cannot receive any more data. An application should avoid sending more data until
 * a further callback is received indicating the congestion status has been cleared.
 */
void ble_client_demo_api_congestion_callback(int conn_id, uint8_t congested)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT get database callback */
void ble_client_demo_api_get_gatt_db_callback(int status, int conn_id, tls_btgatt_db_element_t *db,
        int count)
{
    int i = 0;
    uint16_t cared_handle, tmp_uuid;
    unsigned char ind_enable[1];
    //hci_dbg_msg("===========btgattc_get_gatt_db_callback(count=%d)(conn_id=%d)================\r\n", count, conn_id);
#if 1

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
    g_indication_handle = 44;
    g_write_handle = 42;
    //TLS_BT_APPL_TRACE_DEBUG("read handle=%d\r\n", cared_handle);
    //tls_ble_client_read_characteristic(conn_id, cared_handle, 0);
    tls_ble_client_register_for_notification(g_client_if, &g_bd_addr, g_indication_handle, conn_id);
}

/** GATT services between start_handle and end_handle were removed */
void ble_client_demo_api_services_removed_callback(int conn_id, uint16_t start_handle,
        uint16_t end_handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

/** GATT services were added */
void ble_client_demo_api_services_added_callback(int conn_id, tls_btgatt_db_element_t *added,
        int added_count)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, conn_id=%d\r\n", __FUNCTION__, conn_id);
}

static tls_bt_status_t wm_ble_client_demo_api_connect(int id)
{
    tls_dm_free_timer_id(id);
    return tls_ble_client_connect(g_client_if, &g_bd_addr, 1, WM_BLE_GATT_TRANSPORT_LE);
}

static bool analyse_adv_data(tls_bt_addr_t *addr, int rssi, uint8_t *adv_data)
{
    bool status = false;
    uint8_t index = 0, len = 0, type = 0;  //MAX_BLE_DEV_COUNT
    static uint8_t index_cur = 0;
    int i = 0;
    bool found = false;
    uint16_t cid = 0;

    //02 01 02 07 09 48 55 41 57 45 49 00 00
    while(index < 62) {
        len = adv_data[index++];
        type = adv_data[index++];

        if(!len) { break; }

#if 0

        //search by name;
        if(type == 0x09) {
            if((adv_data[index++] == 'W') && (adv_data[index++] == 'M') && (adv_data[index++] == 'B')
                    && (adv_data[index++] == 'L') && (adv_data[index++] == 'E')) {
                found = true;
                memcpy(&g_bd_addr.address[0], &addr->address[0], 6);
                //hci_dbg_hexstring("!!! Found device:", &g_bd_addr.address[0], 6);
                TLS_BT_APPL_TRACE_DEBUG("%s, Found device\r\n", __FUNCTION__);
                status = true;
                break;
            } else {
                index += (len - 1);
            }

            break;
        }

#else

        //search by service uuid;
        //0x03,0x19,0xc1, 0x03 , this field will be in our adv data, see wm_ble_server_api_demo.c
        if(type == 0x19) {
            if((adv_data[index++] == 0xc1) && (adv_data[index++] == 0x03)) {
                found = true;
                memcpy(&g_bd_addr.address[0], &addr->address[0], 6);
                //hci_dbg_hexstring("!!! Found device:", &g_bd_addr.address[0], 6);
                TLS_BT_APPL_TRACE_DEBUG("%s, Found device\r\n", __FUNCTION__);
                status = true;
                break;
            } else {
                index += (len - 1);
            }

            break;
        }

#endif
        else {
            index += (len - 1);
        }
    }

    return status;
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

int tls_ble_client_demo_api_init(tls_ble_output_func_ptr output_func_ptr)
{
    tls_bt_status_t status;
    status = tls_ble_client_register_client(0x1234, &swmbleclientcb);

    if(status == TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_DEBUG("### %s success\r\n", __FUNCTION__);
        g_ble_output_fptr = output_func_ptr;
    } else {
        //strange logical, at cmd task , bt host task, priority leads to this situation;
        TLS_BT_APPL_TRACE_ERROR("### %s failed\r\n", __FUNCTION__);
    }

    return status;
}

int tls_ble_client_demo_api_deinit()
{
    return tls_ble_client_unregister_client(g_client_if);
}


int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length)
{
    g_send_pending = 1;
    return tls_ble_client_write_characteristic(g_conn_id, g_write_handle, 2, length, 0, ptr);
}



#endif

