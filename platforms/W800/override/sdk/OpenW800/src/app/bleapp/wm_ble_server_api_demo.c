#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_NIMBLE_INCLUDED == CFG_ON)

#include "wm_bt_app.h"
#include "wm_bt_util.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "wm_ble_gap.h"
#include "wm_ble_uart_if.h"
#include "wm_ble_server_api_demo.h"
#include "wm_ble_client_util.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

typedef enum {
    BLE_SERVER_MODE_IDLE     = 0x00,
    BLE_SERVER_MODE_ADVERTISING = 0x01,
    BLE_SERVER_MODE_CONNECTED,
    BLE_SERVER_MODE_INDICATING,
    BLE_SERVER_MODE_EXITING
} ble_server_state_t;
    
static struct ble_gap_event_listener ble_server_event_listener;
static uint8_t g_ble_demo_indicate_enable = 0;
static uint8_t g_ble_demo_notify_enable = 0;

static int g_mtu = 20;
static uint8_t g_ind_data[MYNEWT_VAL(BLE_ATT_PREFERRED_MTU)];
static volatile uint8_t g_send_pending = 0;
static volatile ble_server_state_t g_ble_server_state = BLE_SERVER_MODE_IDLE;
static tls_ble_uart_output_ptr g_uart_output_ptr = NULL; 
static tls_ble_uart_sent_ptr g_uart_in_and_sent_ptr = NULL;


/* ble attr write/notify handle */
uint16_t g_ble_demo_attr_indicate_handle;
uint16_t g_ble_demo_attr_notify_handle;

uint16_t g_ble_demo_attr_write_handle;
uint16_t g_ble_demo_attr_read_handle;

uint16_t g_ble_demo_conn_handle ;



#define WM_GATT_SVC_UUID      0xFFF0
#define WM_GATT_INDICATE_UUID 0xFFF1
#define WM_GATT_WRITE_UUID    0xFFF2
#define WM_GATT_READ_UUID     0xFFF3
#define WM_GATT_NOTIFICATION_UUID 0xFFF4

#define WM_INDICATE_AUTO 1


static int
gatt_svr_chr_demo_access_func(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);


/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */


static const struct ble_gatt_svc_def gatt_demo_svr_svcs[] = {
    {
        /* Service: uart */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(WM_GATT_SVC_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                .uuid = BLE_UUID16_DECLARE(WM_GATT_WRITE_UUID),
                .val_handle = &g_ble_demo_attr_write_handle,
                .access_cb = gatt_svr_chr_demo_access_func,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                .uuid = BLE_UUID16_DECLARE(WM_GATT_READ_UUID),
                .val_handle = &g_ble_demo_attr_read_handle,
                .access_cb = gatt_svr_chr_demo_access_func,
                .flags = BLE_GATT_CHR_F_READ,
            },{
                .uuid = BLE_UUID16_DECLARE(WM_GATT_INDICATE_UUID),
                .val_handle = &g_ble_demo_attr_indicate_handle,
                .access_cb = gatt_svr_chr_demo_access_func,
                .flags = BLE_GATT_CHR_F_INDICATE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(WM_GATT_NOTIFICATION_UUID),
                .val_handle = &g_ble_demo_attr_notify_handle,
                .access_cb = gatt_svr_chr_demo_access_func,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                0, /* No more characteristics in this service */
            }
        },
    },

    {
        0, /* No more services */
    },
};

int wm_ble_server_api_demo_adv(bool enable)
{
    int rc;

    if(enable) {
        struct ble_hs_adv_fields fields;
        const char *name;
        /**
         *  Set the advertisement data included in our advertisements:
         *     o Flags (indicates advertisement type and other general info).
         *     o Device name.
         *     o user specific field (winner micro).
         */
        memset(&fields, 0, sizeof fields);
        /* Advertise two flags:
         *     o Discoverability in forthcoming advertisement (general)
         *     o BLE-only (BR/EDR unsupported).
         */
        fields.flags = BLE_HS_ADV_F_DISC_GEN |
                       BLE_HS_ADV_F_BREDR_UNSUP;
        name = ble_svc_gap_device_name();
        fields.name = (uint8_t *)name;
        fields.name_len = strlen(name);
        fields.name_is_complete = 1;
        fields.uuids16 = (ble_uuid16_t[]) {
            BLE_UUID16_INIT(WM_GATT_SVC_UUID)
        };
        fields.num_uuids16 = 1;
        fields.uuids16_is_complete = 1;
        rc = ble_gap_adv_set_fields(&fields);

        if(rc != 0) {
            TLS_BT_APPL_TRACE_ERROR("error setting advertisement data; rc=%d\r\n", rc);
            return rc;
        }

        /* As own address type we use hard-coded value, because we generate
              NRPA and by definition it's random */
        rc = tls_nimble_gap_adv(WM_BLE_ADV_IND, 0);

        if(rc != 0) {
            TLS_BT_APPL_TRACE_ERROR("tls_nimble_gap_adv; rc=%d\r\n", rc);
            return rc;
        }
    } else {
        rc = tls_nimble_gap_adv(WM_BLE_ADV_STOP, 0);
    }

    return rc;
}
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint8_t gatt_svc_test_read_value[200] = {0x01, 0x02, 0x03};

static void tls_print_bytes(const char *desc, uint8_t *ptr, uint16_t len)
{
    int i = 0, j = 0;
    printf("%s:", desc);
    for(i = 0; i<len; i++)
    {
        printf("%02x", ptr[i]);
        j++;

        if(j == 16){
            j = 0;
            printf("\r\n");
        }
    }
    printf("\r\n");
}

static int
gatt_svr_chr_demo_access_func(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    struct os_mbuf *om = ctxt->om;

    switch(ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            while(om) {
                if(g_uart_output_ptr) {
                    TLS_BT_APPL_TRACE_VERBOSE("### %s len=%d\r\n", __FUNCTION__, om->om_len);
                    g_uart_output_ptr(UART_OUTPUT_DATA, (uint8_t *)om->om_data, om->om_len);
                } else {
                    tls_print_bytes("op write", om->om_data, om->om_len);
                }

                om = SLIST_NEXT(om, om_next);
            }

            return 0;
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &gatt_svc_test_read_value,
                                sizeof gatt_svc_test_read_value);
            gatt_svc_test_read_value[0]++;
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
    }
}

int
wm_ble_server_demo_gatt_svr_init(void)
{
    int rc;
    rc = ble_gatts_count_cfg(gatt_demo_svr_svcs);

    if(rc != 0) {
        goto err;
    }

    rc = ble_gatts_add_svcs(gatt_demo_svr_svcs);

    if(rc != 0) {
        return rc;
    }

err:
    return rc;
}


static uint8_t ss = 0x00;
//#define THROUGHTPUT_TEST

#ifdef THROUGHTPUT_TEST

static uint32_t g_send_bytes = 0;
static uint32_t g_time_last = 0;

static uint32_t ticks_elapsed(uint32_t ticks)
{
    uint32_t diff_ticks = 0;
    uint32_t curr_ticks = tls_os_get_time();

    if(curr_ticks >= ticks) {
        diff_ticks = curr_ticks - ticks;
    } else {
        diff_ticks = curr_ticks + (0xFFFFFFFF - ticks);
    }

    return diff_ticks;
}
#endif

static void ble_server_indication_sent_cb(int conn_id, int status)
{
#if WM_INDICATE_AUTO
    int rc;
#endif
    g_send_pending = 0;

    if(!g_ble_demo_indicate_enable) {
        TLS_BT_APPL_TRACE_DEBUG("Indicate disabled... when trying to send...\r\n");
        return;
    }

#if WM_INDICATE_AUTO

    if(g_uart_output_ptr == NULL) {
        memset(g_ind_data, ss, sizeof(g_ind_data));
        ss++;

        if(ss > 0xFE) { ss = 0x00; }

        rc = tls_ble_server_demo_api_send_msg(g_ind_data, g_mtu);
		(void)rc;
#ifdef THROUGHTPUT_TEST
        if(rc == 0)
        {
            g_send_bytes += g_mtu;
        }else
        {
            printf("indicate failed rc=%d\r\n", rc);
        }

        if(ticks_elapsed(g_time_last) >= HZ) {
            g_time_last = tls_os_get_time();
            printf("BLE Send(%d bytes)[%5.2f Kbps][mtu%d]/s\r\n", g_send_bytes, (g_send_bytes * 8.0 / 1024),g_mtu);
            g_send_bytes = 0;
        }

#endif
    } else {
        if(g_uart_in_and_sent_ptr) g_uart_in_and_sent_ptr(BLE_UART_SERVER_MODE, status);
    }
#endif

}

static void wm_ble_server_demo_start_indicate(void *arg)
{
    int rc;

    /*No uart ble interface*/
    if(g_uart_in_and_sent_ptr == NULL) {
        rc = tls_ble_server_demo_api_send_msg(g_ind_data, g_mtu);
        TLS_BT_APPL_TRACE_DEBUG("Indicating sending...rc=%d\r\n", rc);
#ifdef THROUGHTPUT_TEST
        g_time_last = tls_os_get_time();
#endif
    } else {
        g_uart_in_and_sent_ptr(BLE_UART_SERVER_MODE, 0);

    }
}
static void conn_param_update_cb(uint16_t conn_handle, int status, void *arg)
{
    TLS_BT_APPL_TRACE_DEBUG("conn param update complete; conn_handle=%d status=%d\n",
                            conn_handle, status);
    if(status!=0)
    {
        
    }
}

static void wm_ble_server_demo_conn_param_update_slave()
{
    int rc;
    struct ble_l2cap_sig_update_params params;
    params.itvl_min = 0x00016;
    params.itvl_max = 0x00026;
    params.slave_latency = 0;
    params.timeout_multiplier = 500;
    rc = ble_l2cap_sig_update(g_ble_demo_conn_handle, &params,
                              conn_param_update_cb, NULL);
    if(rc != 0)
    {
        TLS_BT_APPL_TRACE_ERROR("ERROR, ble_l2cap_sig_update rc=%d\r\n", rc);
    }
}
#if 0
static int
wm_ble_server_demo_on_mtu(uint16_t conn_handle, const struct ble_gatt_error *error,
                          uint16_t mtu, void *arg)
{
    switch(error->status) {
        case 0:
            TLS_BT_APPL_TRACE_DEBUG("mtu exchange complete: conn_handle=%d mtu=%d\r\n",
                                    conn_handle, mtu);
            g_mtu = mtu-3;
            break;

        default:
            TLS_BT_APPL_TRACE_DEBUG("Update MTU failed...error->status=%d\r\n", error->status);
            break;
    }

    return 0;
}
#endif                          
static int ble_gap_evt_cb(struct ble_gap_event *event, void *arg)
{
    int rc;
    struct ble_gap_conn_desc desc;

    switch(event->type) {
        case BLE_GAP_EVENT_CONNECT:
            
            if(event->connect.status == 0) {

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                if(desc.role != BLE_GAP_ROLE_SLAVE){
                    return 0;
                }
                TLS_BT_APPL_TRACE_DEBUG("Server connected status=%d handle=%d,g_ble_demo_attr_indicate_handle=%d\r\n",
                                    event->connect.status, g_ble_demo_conn_handle, g_ble_demo_attr_indicate_handle);                
                print_conn_desc(&desc);
                g_ble_server_state = BLE_SERVER_MODE_CONNECTED;
                //re set this flag, to prevent stop adv, but connected evt reported when deinit this demo
                g_ble_demo_conn_handle = event->connect.conn_handle;
                if(g_uart_output_ptr)
                {
                    g_uart_output_ptr(UART_OUTPUT_CMD_CONNECTED, NULL, 0);
                }
                
#if MYNEWT_VAL(BLEPRPH_LE_PHY_SUPPORT)
                phy_conn_changed(event->connect.conn_handle);
#endif
#ifdef THROUGHTPUT_TEST
                g_send_bytes = 0;
                g_time_last = tls_os_get_time();
#endif
                tls_bt_async_proc_func(wm_ble_server_demo_conn_param_update_slave, NULL, 100);
                g_send_pending = 0;

            }

            TLS_BT_APPL_TRACE_DEBUG("\r\n");

            if(event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                tls_nimble_gap_adv(WM_BLE_ADV_IND, 0);
            }

            break;

        case BLE_GAP_EVENT_DISCONNECT:

            if(event->disconnect.conn.role != BLE_GAP_ROLE_SLAVE) return 0;

            TLS_BT_APPL_TRACE_DEBUG("Server disconnect reason=%d[0x%02x],state=%d\r\n", event->disconnect.reason,event->disconnect.reason-0x200,
                                    g_ble_server_state);
            g_ble_demo_indicate_enable = 0;
            g_send_pending = 0;
            g_mtu = 20;
            uint8_t err_code[1];
            if(g_uart_output_ptr)
            {
                err_code[0] = event->disconnect.reason - 0x200;
                g_uart_output_ptr(UART_OUTPUT_CMD_DISCONNECTED, (uint8_t*)&err_code, 0);
            }

            if(g_ble_server_state == BLE_SERVER_MODE_EXITING) {
                if(g_uart_output_ptr) {
                    g_uart_output_ptr = NULL;
                }
                if(g_uart_in_and_sent_ptr) {
                    g_uart_in_and_sent_ptr = NULL;
                }

                g_ble_server_state = BLE_SERVER_MODE_IDLE;
                ble_gap_event_listener_unregister(&ble_server_event_listener);
            } else {

                /**if connction is not closed by local host*/
                /**Any reason, we will continue to do advertise*/
                // 1, indicate timeout, host will terminate the connection, disconnect reason 0x16
                // 2, host stack close, negative effection, no!
                if(event->disconnect.reason != 534)
                {
                    rc = tls_nimble_gap_adv(WM_BLE_ADV_IND, 0);
                    TLS_BT_APPL_TRACE_DEBUG("Disconnect evt, and continue to do adv, rc=%d\r\n", rc);

                    if(!rc) {
                        g_ble_server_state = BLE_SERVER_MODE_ADVERTISING;
                    } else {
                        g_ble_server_state = BLE_SERVER_MODE_IDLE;
                    }
                }else
                {
                    g_ble_server_state = BLE_SERVER_MODE_IDLE;
                }
            }

            break;

        case BLE_GAP_EVENT_NOTIFY_TX:

            rc = ble_gap_conn_find(event->notify_tx.conn_handle , &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_SLAVE) return 0;
            if(event->notify_tx.attr_handle == g_ble_demo_attr_indicate_handle)
            {
            if(event->notify_tx.status == BLE_HS_EDONE) {
                ble_server_indication_sent_cb(event->notify_tx.attr_handle, event->notify_tx.status);
            } else {
                /*Application will handle other cases*/
            }
            }else if(event->notify_tx.attr_handle == g_ble_demo_attr_notify_handle)
            {
                g_send_pending = 0;
            }

            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            rc = ble_gap_conn_find(event->subscribe.conn_handle , &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_SLAVE) return 0;
            
            TLS_BT_APPL_TRACE_DEBUG("subscribe [%d]indicate(%d,%d)\r\n",event->subscribe.attr_handle,  event->subscribe.prev_indicate,
                                    event->subscribe.cur_indicate);

            if(event->subscribe.attr_handle == g_ble_demo_attr_indicate_handle)
            {
            g_ble_demo_indicate_enable = event->subscribe.cur_indicate;

            if(g_ble_demo_indicate_enable) {
                g_ble_server_state = BLE_SERVER_MODE_INDICATING;

                /*To reach the max passthrough,  in ble_uart mode, I conifg the min connection_interval*/
                if(g_uart_in_and_sent_ptr) {
                    tls_bt_async_proc_func(wm_ble_server_demo_conn_param_update_slave, NULL, 30);
                } else {
#ifdef THROUGHTPUT_TEST
                    tls_bt_async_proc_func(wm_ble_server_demo_conn_param_update_slave, NULL, 30);
#endif
                }
#if WM_INDICATE_AUTO
                if(g_uart_output_ptr == NULL)
                {
                    tls_bt_async_proc_func(wm_ble_server_demo_start_indicate, NULL, 30);
                }
#endif
                } else if(g_ble_demo_indicate_enable == 0){
                if(g_ble_server_state != BLE_SERVER_MODE_EXITING) {
                    g_ble_server_state = BLE_SERVER_MODE_CONNECTED;
                }
            }
           }else
           {
                g_ble_demo_notify_enable = event->subscribe.cur_notify;
                if(g_ble_demo_notify_enable)
                {
#ifdef THROUGHTPUT_TEST
                    tls_bt_async_proc_func(wm_ble_server_demo_conn_param_update_slave, NULL, 30);
#endif
                }else
                {
                    ;
                }
           }

            break;

        case BLE_GAP_EVENT_MTU:
            rc = ble_gap_conn_find(event->mtu.conn_handle , &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_SLAVE) return 0;

            TLS_BT_APPL_TRACE_DEBUG("wm ble dm mtu changed to(%d)\r\n", event->mtu.value);
            /*nimBLE config prefered ATT_MTU is 256. here 256-12 = 244. */
            /* preamble(1)+access address(4)+pdu(2~257)+crc*/
            /* ATT_MTU(247):pdu= pdu_header(2)+l2cap_len(2)+l2cap_chn(2)+mic(4)*/
            /* GATT MTU(244): ATT_MTU +opcode+chn*/
            //g_mtu = min(event->mtu.value - 12, 244);
            g_mtu = event->mtu.value-3;
            break;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */
            /* Delete the old bond. */
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_SLAVE) return 0;            
            ble_store_util_delete_peer(&desc.peer_id_addr);
            TLS_BT_APPL_TRACE_DEBUG("!!!BLE_GAP_EVENT_REPEAT_PAIRING\r\n");
            return BLE_GAP_REPEAT_PAIRING_RETRY;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            TLS_BT_APPL_TRACE_DEBUG(">>>BLE_GAP_EVENT_REPEAT_PAIRING\r\n");
            return 0;
        case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
            TLS_BT_APPL_TRACE_DEBUG("Server conn handle=%d,peer:[min=%d,max=%d,sup=%d],local[min=%d,max=%d,sup=%d]", event->conn_update_req.conn_handle, 
                event->conn_update_req.peer_params->itvl_min, event->conn_update_req.peer_params->itvl_max,event->conn_update_req.peer_params->supervision_timeout,
                event->conn_update_req.self_params->itvl_min, event->conn_update_req.self_params->itvl_max,event->conn_update_req.self_params->supervision_timeout);
            break;
        case BLE_GAP_EVENT_CONN_UPDATE:
            if(event->conn_update.status == 0)
            {
                rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
                assert(rc == 0);
                TLS_BT_APPL_TRACE_DEBUG("Server con_handle=%d,conn update, interval=%d, supervision=%d\r\n", event->conn_update.conn_handle, desc.conn_itvl, desc.supervision_timeout);
            }
            break;
        case BLE_GAP_EVENT_HOST_SHUTDOWN:
            TLS_BT_APPL_TRACE_DEBUG("Server BLE_GAP_EVENT_HOST_SHUTDOWN:%d\r\n", event->type);
            g_ble_server_state = BLE_SERVER_MODE_IDLE;
            g_ble_demo_indicate_enable = 0;
            g_ble_demo_notify_enable = 0;
            g_send_pending = 0;
            g_mtu = 20;
            if(g_uart_output_ptr) {
                g_uart_output_ptr = NULL;
            }
            if(g_uart_in_and_sent_ptr) {
                g_uart_in_and_sent_ptr = NULL;
            }              
            break;
        default:
            TLS_BT_APPL_TRACE_VERBOSE("Server Unhandled event:%d\r\n", event->type);
  
            break;
    }

    return 0;
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_server_demo_api_init(tls_ble_uart_output_ptr uart_output_ptr, tls_ble_uart_sent_ptr uart_in_and_sent_ptr)
{
    int rc = BLE_HS_EAPP;

    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_DEBUG("%s, state=%d\r\n", __FUNCTION__, g_ble_server_state);

    if(g_ble_server_state == BLE_SERVER_MODE_IDLE) {
        //step 0: reset other services. Note
        rc = ble_gatts_reset();

        if(rc != 0) {
            TLS_BT_APPL_TRACE_ERROR("tls_ble_server_demo_api_init failed rc=%d\r\n", rc);
            return rc;
        }

        //step 1: config/adding  the services
        rc = wm_ble_server_demo_gatt_svr_init();

        if(rc == 0) {
            ble_gap_event_listener_register(&ble_server_event_listener,
                                            ble_gap_evt_cb, NULL);

            TLS_BT_APPL_TRACE_DEBUG("tls_ble_server_demo_api_init register success\r\n");
            g_uart_output_ptr = uart_output_ptr;
            g_uart_in_and_sent_ptr = uart_in_and_sent_ptr;
            /*step 2: start the service*/
            rc = ble_gatts_start();
            assert(rc == 0);
            /*step 3: start advertisement*/
            rc = wm_ble_server_api_demo_adv(true);

            if(rc == 0) {
                g_ble_server_state = BLE_SERVER_MODE_ADVERTISING;
                if(g_uart_output_ptr)
                {
                    g_uart_output_ptr(UART_OUTPUT_CMD_ADVERTISING, NULL, 0);
                }
            }
        } else {
            TLS_BT_APPL_TRACE_ERROR("tls_ble_server_demo_api_init failed(rc=%d)\r\n", rc);
        }
    } else {
        TLS_BT_APPL_TRACE_WARNING("tls_ble_server_demo_api_init registered\r\n");
        rc = BLE_HS_EALREADY;
    }

    return rc;
}
int tls_ble_server_demo_api_deinit()
{
    int rc = BLE_HS_EAPP;

    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_DEBUG("%s, state=%d\r\n", __FUNCTION__, g_ble_server_state);

    if(g_ble_server_state == BLE_SERVER_MODE_CONNECTED
            || g_ble_server_state == BLE_SERVER_MODE_INDICATING) {
        g_ble_demo_indicate_enable = 0;
        rc = ble_gap_terminate(g_ble_demo_conn_handle, BLE_ERR_REM_USER_CONN_TERM);

        if(rc == 0) {
            g_ble_server_state = BLE_SERVER_MODE_EXITING;
        } else if(rc == BLE_HS_EDISABLED || rc == BLE_HS_ENOTCONN) {
            g_send_pending = 0;
            g_ble_server_state = BLE_SERVER_MODE_IDLE;
            ble_gap_event_listener_unregister(&ble_server_event_listener);
        }else{
            /**Force to clear state if unknown error happen*/
            if(g_uart_output_ptr) {
                g_uart_output_ptr = NULL;
            }
            if(g_uart_in_and_sent_ptr) {
                g_uart_in_and_sent_ptr = NULL;
            }

            g_send_pending = 0;
            g_ble_server_state = BLE_SERVER_MODE_IDLE;  
            ble_gap_event_listener_unregister(&ble_server_event_listener);
        }
    } else if(g_ble_server_state == BLE_SERVER_MODE_ADVERTISING) {
        rc = tls_nimble_gap_adv(WM_BLE_ADV_STOP, 0);

        if(rc == 0 || rc == BLE_HS_EDISABLED || rc == BLE_HS_EALREADY) {
            if(g_uart_output_ptr) {
                g_uart_output_ptr = NULL;
            }
            if(g_uart_in_and_sent_ptr) {
                g_uart_in_and_sent_ptr = NULL;
            }

            g_send_pending = 0;
            g_ble_server_state = BLE_SERVER_MODE_IDLE;
            ble_gap_event_listener_unregister(&ble_server_event_listener);
        }
    } else if(g_ble_server_state == BLE_SERVER_MODE_IDLE) {
        rc = 0;
    } else {
        rc = BLE_HS_EALREADY;
    }

    return rc;
}
uint32_t tls_ble_server_demo_api_get_mtu()
{
    return g_mtu;
}

int tls_ble_server_demo_api_send_msg_indicate()
{
    int rc;
    struct os_mbuf *om;
    
    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_VERBOSE("### %s len=%d\r\n", __FUNCTION__, 4000);
    

    if(g_send_pending) { return BLE_HS_EBUSY; }

    memset(g_ind_data, ss, sizeof(g_ind_data));
    ss++;
    
    if(ss > 0xFE) { ss = 0x00; }


    om = ble_hs_mbuf_from_flat(g_ind_data, 4000);

    if(!om) {
        return BLE_HS_ENOMEM;
    }

    rc = ble_gattc_indicate_custom(g_ble_demo_conn_handle, g_ble_demo_attr_indicate_handle, om);

    if(rc == 0) {
        g_send_pending = 1;
        
    }else
    {
        printf("!!! tls_ble_server_demo_api_send_msg , rc=%d\r\n", rc);
    }

    return rc;

    
    return 0;
}
int tls_ble_server_demo_api_send_msg(uint8_t *data, int data_len)
{
    int rc;
    struct os_mbuf *om;
    
    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_VERBOSE("### %s len=%d\r\n", __FUNCTION__, data_len);
    

    if(g_send_pending) { return BLE_HS_EBUSY; }
    if(g_ble_demo_indicate_enable == 0) { return BLE_HS_EDISABLED; }
    
    if(data_len <= 0 || data == NULL) {
        return BLE_HS_EINVAL;
    }

    om = ble_hs_mbuf_from_flat(data, data_len);

    if(!om) {
        return BLE_HS_ENOMEM;
    }

    rc = ble_gattc_indicate_custom(g_ble_demo_conn_handle, g_ble_demo_attr_indicate_handle, om);

    if(rc == 0) {
        g_send_pending = 1;
    }else
    {
        TLS_BT_APPL_TRACE_ERROR("!!! tls_ble_server_demo_api_send_msg , rc=%d\r\n", rc)
    }

    return rc;
}

int tls_ble_server_demo_api_send_msg_notify(uint8_t *ptr, int length)
{
    int rc;
    struct os_mbuf *om;
    
    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_VERBOSE("### %s len=%d\r\n", __FUNCTION__, g_mtu);
    
    if(g_ble_demo_notify_enable == 0) return BLE_HS_EAGAIN;
    
    if(g_send_pending) { return BLE_HS_EBUSY; }


    om = ble_hs_mbuf_from_flat(ptr, length);

    if(!om) {
        return BLE_HS_ENOMEM;
    }
    g_send_pending = 1;

    rc = ble_gattc_notify_custom(g_ble_demo_conn_handle, g_ble_demo_attr_notify_handle, om);

    if(rc == 0) {
#ifdef THROUGHTPUT_TEST

        g_send_bytes += g_mtu;

    if(ticks_elapsed(g_time_last) >= HZ) {
        g_time_last = tls_os_get_time();
        printf("BLE Send(%d bytes)[%5.2f Kbps][mtu%d]/s\r\n", g_send_bytes, (g_send_bytes * 8.0 / 1024),length);
        g_send_bytes = 0;
    }    
#endif
    
    }else
    {
        TLS_BT_APPL_TRACE_DEBUG("!!! tls_ble_server_demo_api_send_msg_notify , rc=%d, err data[%02x%02x]\r\n", rc, g_ind_data[0], g_ind_data[1])
        g_send_pending = 0;    
    }

    return rc;
}

int tls_ble_server_demo_api_set_work_mode(int work_mode)
{
    int rc;
    int intv_min = 0x06;
    int intv_max = 0x06;
    struct ble_l2cap_sig_update_params params;
    
#define PIC_TRANSFERING 1
#define CONNECTING_KEEP 0
#define CONNECTING_KEEP_AND_LOW_POWER 2

    switch(work_mode)
    {
        case 0: 
            intv_min = 100;
            intv_max = 120;
            break;
        case 1:
            intv_min = 6;
            intv_max = 6;
            break;      
        case 2:
            intv_min = 200;
            intv_max = 240; 
            break;
        default:
            TLS_BT_APPL_TRACE_DEBUG("Unspported work mode %d\r\n", work_mode);
            return -1;
    }
    
    params.itvl_min = intv_min;
    params.itvl_max = intv_max;
    params.slave_latency = 0;
    params.timeout_multiplier = 500;
    rc = ble_l2cap_sig_update(g_ble_demo_conn_handle, &params,
                              conn_param_update_cb, NULL);
    if(rc != 0)
    {
        TLS_BT_APPL_TRACE_ERROR("ERROR, ble_l2cap_sig_update rc=%d\r\n", rc);
    } 

    return rc;
}

#endif


