/*****************************************************************************
**
**  Name:           wm_bt_server_api_demo.c
**
**  Description:    This file contains the  implemention of ble demo server
**
*****************************************************************************/


#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

/*
* This file is a ble server demo:
* 1, How to create one ble server?
      a, register one ble server with specific uuid and callback functions structure;
      b, adding service in server register callback function;
      c, adding character in service adding callback function;
      d, adding descriptor in character adding callback function;
      e, repeat c or d until all character or descriptor added;
      f, start the servcie;
      g, configure advertise data and enable advertise in service enable callback function;
      h, Now you can access the service with app tool on phone.
  2, How to destroy the ble server?
      a, stop the service;
      b, delete the service in callback of stop service;
      c, unregister the server in callback of delete service;
      d, disable the advertisement in callback of deregister function;
  3, About the service configure, see structer of gattArray_t;
      in this demo, I enable one service uuid 0x6789.
      and configure two characters and one descriptor within this service.
      You can access the read/write/notification on phone with ble tool apps(eg. Nrf Connect, lightblue, etc...)
*/

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_ble_server.h"
#include "wm_ble_gatt.h"
#include "wm_ble_server_api_demo.h"
#include "wm_bt_util.h"
#include "wm_ble_uart_if.h"
#include "wm_mem.h"

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


#define DEMO_SERVICE_UUID           (0x6789)
#define DEMO_SERVICE_INDEX          (0)
#define DEMO_PARAM_VALUE_INDEX      (1)
#define DEMO_KEY_VALUE_INDEX        (2)
#define DEMO_KEY_VALUE_CCCD_INDEX   (3)


#if 0
#define BTA_GATT_PERM_READ              GATT_PERM_READ              /* bit 0 -  0x0001 */
#define BTA_GATT_PERM_READ_ENCRYPTED    GATT_PERM_READ_ENCRYPTED    /* bit 1 -  0x0002 */
#define BTA_GATT_PERM_READ_ENC_MITM     GATT_PERM_READ_ENC_MITM     /* bit 2 -  0x0004 */
#define BTA_GATT_PERM_WRITE             GATT_PERM_WRITE             /* bit 4 -  0x0010 */
#define BTA_GATT_PERM_WRITE_ENCRYPTED   GATT_PERM_WRITE_ENCRYPTED   /* bit 5 -  0x0020 */
#define BTA_GATT_PERM_WRITE_ENC_MITM    GATT_PERM_WRITE_ENC_MITM    /* bit 6 -  0x0040 */
#define BTA_GATT_PERM_WRITE_SIGNED      GATT_PERM_WRITE_SIGNED      /* bit 7 -  0x0080 */
#define BTA_GATT_PERM_WRITE_SIGNED_MITM GATT_PERM_WRITE_SIGNED_MITM /* bit 8 -  0x0100 */
typedef uint16_t tBTA_GATT_PERM;

#define BTA_GATT_CHAR_PROP_BIT_BROADCAST    GATT_CHAR_PROP_BIT_BROADCAST    /* 0x01 */
#define BTA_GATT_CHAR_PROP_BIT_READ         GATT_CHAR_PROP_BIT_READ    /* 0x02 */
#define BTA_GATT_CHAR_PROP_BIT_WRITE_NR     GATT_CHAR_PROP_BIT_WRITE_NR    /* 0x04 */
#define BTA_GATT_CHAR_PROP_BIT_WRITE        GATT_CHAR_PROP_BIT_WRITE       /* 0x08 */
#define BTA_GATT_CHAR_PROP_BIT_NOTIFY       GATT_CHAR_PROP_BIT_NOTIFY      /* 0x10 */
#define BTA_GATT_CHAR_PROP_BIT_INDICATE     GATT_CHAR_PROP_BIT_INDICATE    /* 0x20 */
#define BTA_GATT_CHAR_PROP_BIT_AUTH         GATT_CHAR_PROP_BIT_AUTH        /* 0x40 */
#define BTA_GATT_CHAR_PROP_BIT_EXT_PROP     GATT_CHAR_PROP_BIT_EXT_PROP    /* 0x80 */

#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static gattArray_t gatt_uuid[] = {
#if 0
    {8,     0x1910, ATTR_SERVICE,       0, 0,    0},
    {0,     0x2B11, ATTR_CHARACTISTIRC, 0, 0x08, 0x11},
    {0,     0x2B10, ATTR_CHARACTISTIRC, 0, 0x20, 0x01},
    {0,     0x2902, ATTR_DESCRIPTOR_CCC, 0, 0,   0x11},
#else
    {8,     0xFFF0, ATTR_SERVICE,       0, 0,    0},
    {0,     0xFFF2, ATTR_CHARACTISTIRC, 0, 0x08, 0x20},
    {0,     0xFFF1, ATTR_CHARACTISTIRC, 0, 0x20, 0x01},
    {0,     0x2902, ATTR_DESCRIPTOR_CCC, 0, 0,   0x11},

#endif
};

static int g_server_if;
static int g_conn_id = -1;
static tls_bt_addr_t g_addr;
static int g_trans_id;
static int g_offset;
static int g_service_index = 0;
static int demo_server_notification_timer_id = -1;
static int g_mtu = 21;
static uint8_t g_ind_data[255];
static tls_ble_output_func_ptr g_ble_output_fptr = NULL;
static volatile uint8_t g_send_pending = 0;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void dumphex(const char *info, uint8_t *p, int len)
{
    int i = 0;
    printf("%s", info);

    for(i = 0; i < len; i++) {
        printf("%02x ", p[i]);
    }

    printf("\r\n");
}

static void ble_server_adv_enable_cb(uint8_t triger_id)
{
    tls_ble_adv(1);
}
static void ble_server_cfg_and_enable_adv()
{
    tls_ble_dm_adv_data_t data;
    uint8_t bt_mac[6] = {0};
    uint8_t dev_name[31] = {0};
    uint8_t adv_data[31] = {
        0x0C, 0x09, 'T', 'M', '-', '0', '0', '0', '0', '0', '0', '0', '0',
        0x02, 0x01, 0x05,
        0x03, 0x19, 0xc1, 0x03
    };
    memset(&data, 0, sizeof(data));
    extern int tls_get_bt_mac_addr(uint8_t *mac);
    tls_get_bt_mac_addr(bt_mac);
    sprintf(adv_data + 5, "%02X:%02X:%02X", bt_mac[3], bt_mac[4], bt_mac[5]);
    /*for updating device name, stack use it*/
    sprintf(dev_name, "TM-%02X:%02X:%02X", bt_mac[3], bt_mac[4], bt_mac[5]);
    adv_data[13] = 0x02;  //byte 13 was overwritten to zero by sprintf; recover it;
    data.set_scan_rsp = false;   //advertisement data;
    data.pure_data = true;       //only manufacture data is inclucded in the advertisement payload
    data.manufacturer_len = 20;   //configure payload length;
    memcpy(data.manufacturer_data, adv_data, 20);//copy payload ;
    tls_ble_set_adv_data(&data); //configure advertisement data;
    tls_ble_dm_adv_param_t adv_param;
    adv_param.adv_int_min = 0x40; //interval min;
    adv_param.adv_int_max = 0x40; //interval max;
    adv_param.dir_addr = NULL;    //directed address NULL;
    tls_ble_set_adv_param(&adv_param); //configure advertisement parameters;
    tls_ble_gap_set_name(dev_name, 0);
    /*enable advertisement*/
    tls_dm_evt_triger(0, ble_server_adv_enable_cb);
}


static void ble_server_register_app_cb(int status, int server_if, uint16_t app_uuid)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);

    if(status != 0) {
        return;
    }

    if(app_uuid != DEMO_SERVICE_UUID) {
        TLS_BT_APPL_TRACE_ERROR("%s failed(app_uuid=0x%04x)\r\n", __FUNCTION__, app_uuid);
        return;
    }

    g_server_if = server_if;

    if(gatt_uuid[g_service_index].attrType != ATTR_SERVICE) {
        TLS_BT_APPL_TRACE_ERROR("%s failed(g_service_index=%d)\r\n", __FUNCTION__, g_service_index);
        return;
    }

    tls_ble_server_add_service(server_if, 1, 1, app_uuid16_to_uuid128(gatt_uuid[g_service_index].uuid),
                               gatt_uuid[g_service_index].numHandles);
}
static void ble_server_deregister_app_cb(int status, int server_if)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);
    g_service_index = 0;
    /*clear ble output function ptr*/
    g_ble_output_fptr = NULL;
    /*disable advertisement*/
    tls_ble_adv(false);
}
static void update_data_length_cb(uint8_t id)
{
    tls_dm_set_data_length(&g_addr, 0xFB);
}
static void update_conn_param(uint8_t id)
{
    tls_ble_conn_parameter_update(&g_addr, 30, 30, 0, 600);
    tls_dm_free_timer_id(id);
}
static void ble_server_connection_cb(int conn_id, int server_if, int connected, tls_bt_addr_t *bda,
                                     uint16_t reason)
{
    g_conn_id = conn_id;
    memcpy(&g_addr, bda, sizeof(tls_bt_addr_t));

    if(connected) {
        TLS_BT_APPL_TRACE_API("%s , connected=%d,%02x:%02x:%02x:%02x:%02x:%02x\r\n", __FUNCTION__,
                              connected, bda->address[0],
                              bda->address[1], bda->address[2], bda->address[3], bda->address[4], bda->address[5]);
        /*Update connection parameter 3s timeout, if you need */
        //tls_ble_conn_parameter_update(bda, 10, 10, 0, 500);
        //tls_dm_start_timer(tls_dm_get_timer_id(), 1000, update_conn_param);
        //tls_dm_set_data_length(&g_addr, 0xFB);
        tls_ble_adv(0);
    } else {
        TLS_BT_APPL_TRACE_API("%s , connected=%d,%02x:%02x:%02x:%02x:%02x:%02x, reason=0x%04x\r\n",
                              __FUNCTION__, connected, bda->address[0],
                              bda->address[1], bda->address[2], bda->address[3], bda->address[4], bda->address[5], reason);
        g_conn_id = -1;
        tls_ble_adv(1);
    }
}

static void ble_server_service_added_cb(int status, int server_if, int inst_id, bool is_primary,
                                        uint16_t app_uuid, int srvc_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);

    if(status != 0) {
        return;
    }

    gatt_uuid[g_service_index].attr_handle = srvc_handle;
    g_service_index++;

    if(gatt_uuid[g_service_index].attrType != ATTR_CHARACTISTIRC) {
        TLS_BT_APPL_TRACE_ERROR("tls_ble_server_add_characteristic failed(g_service_index=%d)\r\n",
                                g_service_index);
        return;
    }

    tls_ble_server_add_characteristic(server_if, srvc_handle,
                                      app_uuid16_to_uuid128(gatt_uuid[g_service_index].uuid),  gatt_uuid[g_service_index].properties,
                                      gatt_uuid[g_service_index].permissions);
}

static void ble_server_included_service_added_cb(int status, int server_if,
        int srvc_handle,
        int incl_srvc_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);
}

static void ble_server_characteristic_added_cb(int status, int server_if, uint16_t char_id,
        int srvc_handle, int char_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);

    if(status != 0) {
        return;
    }

    gatt_uuid[g_service_index].attr_handle = char_handle;
    g_service_index++;

    if(gatt_uuid[g_service_index].attrType != ATTR_CHARACTISTIRC) {
        tls_ble_server_add_descriptor(server_if, srvc_handle,
                                      app_uuid16_to_uuid128(gatt_uuid[g_service_index].uuid), gatt_uuid[g_service_index].permissions);
    } else {
        tls_ble_server_add_characteristic(server_if, srvc_handle,
                                          app_uuid16_to_uuid128(gatt_uuid[g_service_index].uuid),  gatt_uuid[g_service_index].properties,
                                          gatt_uuid[g_service_index].permissions);
    }
}




static void ble_server_descriptor_added_cb(int status, int server_if,
        uint16_t descr_id, int srvc_handle,
        int descr_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);

    if(status != 0) {
        return;
    }

    gatt_uuid[g_service_index].attr_handle = descr_handle;
    g_service_index++;

    if(g_service_index > DEMO_KEY_VALUE_CCCD_INDEX) {
        tls_ble_server_start_service(server_if, srvc_handle, WM_BLE_GATT_TRANSPORT_LE_BR_EDR);
    } else {
        tls_ble_server_add_characteristic(server_if, srvc_handle,
                                          app_uuid16_to_uuid128(gatt_uuid[g_service_index].uuid),  gatt_uuid[g_service_index].properties,
                                          gatt_uuid[g_service_index].permissions);
    }
}

static void ble_server_service_started_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);
    /*config advertise data and enable advertisement*/
    ble_server_cfg_and_enable_adv();
}

static void ble_server_service_stopped_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);
    tls_ble_server_delete_service(g_server_if, gatt_uuid[DEMO_SERVICE_INDEX].attr_handle);
}

static void ble_server_service_deleted_cb(int status, int server_if, int srvc_handle)
{
    TLS_BT_APPL_TRACE_API("%s , status=%d\r\n", __FUNCTION__, status);
    tls_ble_server_unregister_server(g_server_if);
}

static void ble_server_request_read_cb(int conn_id, int trans_id, tls_bt_addr_t *bda,
                                       int attr_handle, int offset, bool is_long)
{
    TLS_BT_APPL_TRACE_API("%s , conn_id=%d, trans_id=%d, attr_handle=%d\r\n", __FUNCTION__, conn_id,
                          trans_id, attr_handle);
    g_trans_id = trans_id;
    g_offset = offset;
    tls_ble_server_send_response(conn_id, trans_id, 0, offset, attr_handle, 0, "Hello", 5);
}

static uint8_t ss = 0x00;
static void ble_demo_server_notification_started(int id)
{
    int len = 0;

    if(g_conn_id < 0) { return; }

    //len = sprintf(ind, "BLE, %d\r\n", tls_os_get_time());
    memset(g_ind_data, ss, sizeof(g_ind_data));
    ss++;

    if(ss > 0xFE) { ss = 0x00; }

    tls_ble_server_send_indication(g_server_if, gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle, g_conn_id,
                                   g_mtu, 1, g_ind_data);
    //tls_dm_start_timer(demo_server_notification_timer_id, 1000, ble_demo_server_notification_started);
}
static uint8_t indication_enable = 0;
static void ble_server_request_write_cb(int conn_id, int trans_id,
                                        tls_bt_addr_t *bda, int attr_handle,
                                        int offset, int length,
                                        bool need_rsp, bool is_prep, uint8_t *value)
{
    int len = 0;
    tls_bt_status_t status;
    uint8_t *tmp_ptr = NULL;
    TLS_BT_APPL_TRACE_API("%s, conn_id=%d, trans_id=%d, attr_handle=%d, length=%d\r\n", __FUNCTION__,
                          conn_id, trans_id, attr_handle, length);

    if((value[0] == 0x00 || value[0] == 0x02 || value[0] == 0x01)
            && (attr_handle == gatt_uuid[DEMO_KEY_VALUE_CCCD_INDEX].attr_handle)) {
        TLS_BT_APPL_TRACE_DEBUG("This is an notification enable msg(%d),handle=%d\r\n", value[0],
                                attr_handle);

        if(value[0] == 0x01 || value[0] == 0x02) {
            indication_enable = 1;

            /*No uart ble interface*/
            if(g_ble_output_fptr == NULL) {
                demo_server_notification_timer_id = tls_dm_get_timer_id();
                tls_dm_start_timer(demo_server_notification_timer_id, 1000, ble_demo_server_notification_started);
            } else {
                /*check and send*/
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
                    status = tls_ble_server_send_indication(g_server_if, gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle,
                                                            conn_id, len, 1, tmp_ptr);
                    tls_mem_free(tmp_ptr);

                    if(status == TLS_BT_STATUS_SUCCESS) {
                        tls_ble_uart_buffer_delete(len);
                    } else {
                        TLS_BT_APPL_TRACE_DEBUG("No enough memory, retry...");
                    }
                }
            }
        } else {
            indication_enable = 0;

            /*No uart ble interface*/
            if(g_ble_output_fptr == NULL) {
                if(demo_server_notification_timer_id >= 0) {
                    tls_dm_stop_timer(demo_server_notification_timer_id);
                    tls_dm_free_timer_id(demo_server_notification_timer_id);
                };
            }
        }

        return;
    } else {
        if(g_ble_output_fptr) { g_ble_output_fptr(value, length); }
    }

    dumphex("###write cb:", value, length);
}
static void ble_server_indication_sent_cb(int conn_id, int status)
{
    int len = 0;
    uint8_t *tmp_ptr = NULL;
    tls_bt_status_t ret;

    if(g_conn_id < 0) { return; }

    g_send_pending = 0;

    if(g_ble_output_fptr == NULL) {
        //len = sprintf(ind, "BLE, %d\r\n", tls_os_get_time());
        memset(g_ind_data, ss, sizeof(g_ind_data));
        ss++;

        if(ss > 0xFE) { ss = 0x00; }

        //tls_dm_stop_timer(demo_server_notification_timer_id);
        if(!indication_enable) { return; }

        tls_ble_server_send_indication(g_server_if, gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle, conn_id,
                                       g_mtu, 1, g_ind_data);
        //tls_dm_start_timer(demo_server_notification_timer_id, 1000, ble_demo_server_notification_started);
    } else {
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
            ret = tls_ble_server_send_indication(g_server_if, gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle,
                                                 conn_id, len, 1, tmp_ptr);
            tls_mem_free(tmp_ptr);

            if(ret == TLS_BT_STATUS_SUCCESS) {
                tls_ble_uart_buffer_delete(len);
            } else {
                TLS_BT_APPL_TRACE_DEBUG("No enough memory, retry...");
            }
        }
    }
}

static void ble_server_request_exec_write_cb(int conn_id, int trans_id,
        tls_bt_addr_t *bda, int exec_write)
{
}

static void ble_server_response_confirmation_cb(int status, int conn_id, int trans_id)
{
}

static void ble_server_congestion_cb(int conn_id, bool congested)
{
}

static void ble_server_mtu_changed_cb(int conn_id, int mtu)
{
    TLS_BT_APPL_TRACE_DEBUG("!!!ble_server_mtu_changed_cb, conn_id=%d, mtu=%d\r\n", conn_id, mtu);
    g_mtu = mtu - 3;
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


int tls_ble_server_demo_api_init(tls_ble_output_func_ptr output_func_ptr)
{
    tls_bt_status_t status;
    status = tls_ble_server_register_server(DEMO_SERVICE_UUID, &servercb);

    if(status == TLS_BT_STATUS_SUCCESS) {
        TLS_BT_APPL_TRACE_DEBUG("### %s success\r\n", __FUNCTION__);
        g_ble_output_fptr = output_func_ptr;
    } else {
        //strange logical, at cmd task , bt host task, priority leads to this situation;
        TLS_BT_APPL_TRACE_ERROR("### %s failed\r\n", __FUNCTION__);
    }

    return status;
}
int tls_ble_server_demo_api_deinit()
{
    tls_bt_status_t status;
    return tls_ble_server_stop_service(g_server_if, gatt_uuid[DEMO_SERVICE_INDEX].attr_handle);
}
int tls_ble_server_demo_api_connect(int status)
{
    return tls_ble_server_connect(g_server_if, (tls_bt_addr_t *)&g_addr, 1, 0);
}

int tls_ble_server_demo_api_disconnect(int status)
{
    return tls_ble_server_disconnect(g_server_if, (tls_bt_addr_t *)&g_addr, g_conn_id);
}

int tls_ble_server_demo_api_send_msg(uint8_t *ptr, int length)
{
    if(g_send_pending) { return TLS_BT_STATUS_BUSY; }

    g_send_pending = 1;
    return tls_ble_server_send_indication(g_server_if, gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle,
                                          g_conn_id, length, 1, ptr);
}

int tls_ble_server_demo_api_send_response(uint8_t *ptr, int length)
{
    return tls_ble_server_send_response(g_conn_id, g_trans_id, 0, g_offset,
                                        gatt_uuid[DEMO_KEY_VALUE_INDEX].attr_handle, 0, ptr, length);
}


int tls_ble_server_demo_api_clean_up(int status)
{
    return tls_ble_server_delete_service(g_server_if, gatt_uuid[DEMO_SERVICE_INDEX].attr_handle);
}

int tls_ble_server_demo_api_disable(int status)
{
    return tls_ble_server_stop_service(g_server_if, gatt_uuid[DEMO_SERVICE_INDEX].attr_handle);
}
int tls_ble_server_demo_api_read_remote_rssi()
{
    return tls_dm_read_remote_rssi(&g_addr);
}
uint32_t tls_ble_server_demo_api_get_mtu()
{
    return g_mtu;
}
#endif

