#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_ble_server.h"
#include "wm_ble_gatt.h"
#include "wm_ble_server_wifi_app.h"
#include "wm_ble_server_wifi_prof.h"
#include "wm_bt_util.h"

/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

typedef enum {
    ATTR_SERVICE = 1,
    ATTR_CHARACTISTIRC,
    ATTR_DESCRIPTOR_CCC,
    ATTR_NONE
} ATT_type;

typedef struct {
    unsigned int numHandles;
    uint16_t uuid;
    ATT_type attrType;             /*filled by callback*/
    uint16_t attr_handle;  /*filled by callback*/
    uint16_t properties;
    uint16_t permissions;
} gattArray_t;

#define WM_WIFI_SERVICE_INDEX       (0)
#define WM_WIFI_CHARC_INDEX         (1)
#define WM_WIFI_DESC_INDEX          (2)
#define WM_WIFI_PROF_UUID           (0x1234)

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


static gattArray_t gatt_uuid[] = {
    {5,     0x1824, ATTR_SERVICE,       0, 0,    0},
    {0,     0x2ABC, ATTR_CHARACTISTIRC, 0, 0x28, 0x11},
    {0,     0x2902, ATTR_DESCRIPTOR_CCC, 0, 0,    0x11}
};

static int g_server_if;
static int g_conn_id;
static tls_bt_addr_t g_addr;
static int g_trans_id;
static int g_offset;
static wm_ble_wifi_prof_callbacks_t  *ps_wifi_prof_callback = NULL;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void ble_server_register_app_cb(int status, int server_if, uint16_t app_uuid)
{
    if(status != 0) {
        TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, status);
        return;
    }

    if(app_uuid != WM_WIFI_PROF_UUID) {
        TLS_BT_APPL_TRACE_ERROR("ble_server_register_app_cb failed(app_uuid=0x%04x)\r\n", app_uuid);
        return;
    }

    g_server_if = server_if;
    tls_ble_server_add_service(server_if, 1, 1,
                               app_uuid16_to_uuid128(gatt_uuid[WM_WIFI_SERVICE_INDEX].uuid),
                               gatt_uuid[WM_WIFI_SERVICE_INDEX].numHandles);
}
void ble_server_deregister_app_cb(int status, int server_if)
{
    TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, 1);

    if(ps_wifi_prof_callback) {
        ps_wifi_prof_callback = NULL;
    }
}

void ble_server_connection_cb(int conn_id, int server_if, int connected, tls_bt_addr_t *bda,
                              uint16_t reason)
{
    g_conn_id = conn_id;
    memcpy(&g_addr, bda, sizeof(tls_bt_addr_t));

    if(connected) {
        TLS_BT_APPL_TRACE_DEBUG("ble_server_connection_cb status=%d, addr:%02x%02x%02x%02x%02x%02x\r\n",
                                connected,
                                bda->address[0], bda->address[1], bda->address[2], bda->address[3], bda->address[4],
                                bda->address[5]);
        TLS_HAL_CBACK(ps_wifi_prof_callback, connected_cb, 0);
        /*Update connection parameter 5s timeout*/
        tls_ble_conn_parameter_update(bda, 16, 32, 0, 300);
    } else {
        TLS_BT_APPL_TRACE_DEBUG("ble_server_connection_cb status=%d, addr:%02x%02x%02x%02x%02x%02x, reason=0x%04x\r\n",
                                connected,
                                bda->address[0], bda->address[1], bda->address[2], bda->address[3], bda->address[4],
                                bda->address[5], reason);
        TLS_HAL_CBACK(ps_wifi_prof_callback, disconnected_cb, 0);
    }
}

void ble_server_service_added_cb(int status, int server_if, int inst_id, bool is_primary,
                                 uint16_t app_uuid, int srvc_handle)
{
    if(status != 0) {
        TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, status);
        return;
    }

    gatt_uuid[WM_WIFI_SERVICE_INDEX].attr_handle = srvc_handle;
    tls_ble_server_add_characteristic(server_if, srvc_handle,
                                      app_uuid16_to_uuid128(gatt_uuid[WM_WIFI_CHARC_INDEX].uuid),
                                      gatt_uuid[WM_WIFI_CHARC_INDEX].properties, gatt_uuid[WM_WIFI_CHARC_INDEX].permissions);
}

void ble_server_included_service_added_cb(int status, int server_if,
        int srvc_handle,
        int incl_srvc_handle)
{
}

void ble_server_characteristic_added_cb(int status, int server_if, uint16_t char_id,
                                        int srvc_handle, int char_handle)
{
    if(status != 0) {
        TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, status);
        return;
    }

    gatt_uuid[WM_WIFI_CHARC_INDEX].attr_handle = char_handle;
    tls_ble_server_add_descriptor(server_if, srvc_handle,
                                  app_uuid16_to_uuid128(gatt_uuid[WM_WIFI_DESC_INDEX].uuid),
                                  gatt_uuid[WM_WIFI_DESC_INDEX].permissions);
}

void ble_server_descriptor_added_cb(int status, int server_if,
                                    uint16_t descr_id, int srvc_handle,
                                    int descr_handle)
{
    if(status != 0) {
        TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, status);
        return;
    }

    gatt_uuid[WM_WIFI_DESC_INDEX].attr_handle = descr_handle;
    tls_ble_server_start_service(server_if, srvc_handle, WM_BLE_GATT_TRANSPORT_LE_BR_EDR);
}

void ble_server_service_started_cb(int status, int server_if, int srvc_handle)
{
    TLS_HAL_CBACK(ps_wifi_prof_callback, enabled_cb, status);
}

void ble_server_service_stopped_cb(int status, int server_if, int srvc_handle)
{
    tls_ble_server_delete_service(g_server_if, gatt_uuid[WM_WIFI_SERVICE_INDEX].attr_handle);
}

void ble_server_service_deleted_cb(int status, int server_if, int srvc_handle)
{
    tls_ble_server_unregister_server(g_server_if);;
}

void ble_server_request_read_cb(int conn_id, int trans_id, tls_bt_addr_t *bda,
                                int attr_handle, int offset, bool is_long)
{
    g_trans_id = trans_id;
    g_offset = offset;
    TLS_HAL_CBACK(ps_wifi_prof_callback, read_cb, offset);
    /*IOS perform a read opertion, when do searching...*/
    tls_ble_server_send_response(conn_id, trans_id, 0, offset, attr_handle, 0, "WM", 2);
}

void ble_server_request_write_cb(int conn_id, int trans_id,
                                 tls_bt_addr_t *bda, int attr_handle,
                                 int offset, int length,
                                 bool need_rsp, bool is_prep, uint8_t *value)
{
    if(value[0] == 0x00 || value[0] == 0x02) {
        TLS_BT_APPL_TRACE_DEBUG("This is an indication enable msg(%d)\r\n", value[0]);
        return;
    }

    TLS_HAL_CBACK(ps_wifi_prof_callback, write_cb, offset, value, length, is_prep);
}

void ble_server_request_exec_write_cb(int conn_id, int trans_id,
                                      tls_bt_addr_t *bda, int exec_write)
{
    TLS_HAL_CBACK(ps_wifi_prof_callback, exec_write_cb, exec_write);
}

void ble_server_response_confirmation_cb(int status, int conn_id, int trans_id)
{
}

void ble_server_indication_sent_cb(int conn_id, int status)
{
    TLS_HAL_CBACK(ps_wifi_prof_callback, indication_cb, status);
}

void ble_server_congestion_cb(int conn_id, bool congested)
{
}

void ble_server_mtu_changed_cb(int conn_id, int mtu)
{
    TLS_BT_APPL_TRACE_DEBUG("ble_server_mtu_changed_cb, conn_id=%d, mtu=%d\r\n", conn_id, mtu);
    TLS_HAL_CBACK(ps_wifi_prof_callback, mtu_changed_cb, mtu);
}

static const wm_ble_server_callbacks_t servercb = {
    ble_server_register_app_cb,
    ble_server_deregister_app_cb,
    ble_server_connection_cb,
    ble_server_service_added_cb,
    ble_server_included_service_added_cb,
    ble_server_characteristic_added_cb,
    ble_server_descriptor_added_cb,
    ble_server_service_started_cb,
    ble_server_service_stopped_cb,
    ble_server_service_deleted_cb,
    ble_server_request_read_cb,
    ble_server_request_write_cb,
    ble_server_request_exec_write_cb,
    ble_server_response_confirmation_cb,
    ble_server_indication_sent_cb,
    ble_server_congestion_cb,
    ble_server_mtu_changed_cb
};

int tls_ble_wifi_prof_init(wm_ble_wifi_prof_callbacks_t *callback)
{
    tls_bt_status_t status;

    if(ps_wifi_prof_callback == NULL) {
        ps_wifi_prof_callback = callback;
        status = tls_ble_server_register_server(0x1234, &servercb);

        if(status == TLS_BT_STATUS_SUCCESS) {
            TLS_BT_APPL_TRACE_DEBUG("### wm_wifi_prof_init success\r\n");
        } else {
            //strange logical, at cmd task , bt host task, priority leads to this situation;
            ps_wifi_prof_callback = NULL;
            TLS_BT_APPL_TRACE_ERROR("### wm_ble_server_register_server failed\r\n");
        }
    } else {
        TLS_BT_APPL_TRACE_WARNING("wm_ble_server_register_server registered\r\n");
        status = TLS_BT_STATUS_DONE;
    }

    return status;
}
int tls_ble_wifi_prof_deinit()
{
    tls_bt_status_t status;

    if(ps_wifi_prof_callback) {
        //ps_wifi_prof_callback = NULL;   //this ptr will be cleared, when got deregister event
        status = tls_ble_server_unregister_server(g_server_if);

        if(status != TLS_BT_STATUS_SUCCESS) {
            TLS_BT_APPL_TRACE_ERROR("wm_wifi_prof_deinit failed\r\n");
        }
    } else {
        TLS_BT_APPL_TRACE_WARNING("wm_wifi_prof_deinit deinited already\r\n");
        status = TLS_BT_STATUS_DONE;
    }

    return status;
}
int tls_ble_wifi_prof_connect(int status)
{
    return tls_ble_server_connect(g_server_if, (tls_bt_addr_t *)&g_addr, 1, 0);
}

int tls_ble_wifi_prof_disconnect(int status)
{
    return tls_ble_server_disconnect(g_server_if, (tls_bt_addr_t *)&g_addr, g_conn_id);
}

int tls_ble_wifi_prof_send_msg(uint8_t *ptr, int length)
{
    return tls_ble_server_send_indication(g_server_if, gatt_uuid[WM_WIFI_CHARC_INDEX].attr_handle,
                                          g_conn_id, length, 1, ptr);
}

int tls_ble_wifi_prof_send_response(uint8_t *ptr, int length)
{
    return tls_ble_server_send_response(g_conn_id, g_trans_id, 0, g_offset,
                                        gatt_uuid[WM_WIFI_CHARC_INDEX].attr_handle, 0, ptr, length);
}


int tls_ble_wifi_prof_clean_up(int status)
{
    return tls_ble_server_delete_service(g_server_if, gatt_uuid[WM_WIFI_SERVICE_INDEX].attr_handle);
}

int tls_ble_wifi_prof_disable(int status)
{
    return tls_ble_server_stop_service(g_server_if, gatt_uuid[WM_WIFI_SERVICE_INDEX].attr_handle);
}

#endif



