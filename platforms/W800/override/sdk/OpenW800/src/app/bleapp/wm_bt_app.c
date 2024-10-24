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

#if ( WM_NIMBLE_INCLUDED == CFG_ON )
#include "wm_bt.h"
#include "wm_bt_app.h"
#include "wm_bt_util.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "wm_pmu.h"
#include "wm_ble_gap.h"

#if ( WM_MESH_INCLUDED == CFG_ON )
#include "wm_ble_mesh.h"
#endif
#include "wm_ble_uart_if.h"
#include "wm_ble_client_util.h"
#include "wm_ble_server_wifi_app.h"
#include "wm_ble_server_api_demo.h"
#include "wm_ble_client_api_demo.h"
#include "wm_ble_client_api_multi_conn_demo.h"

/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

volatile tls_bt_state_t bt_adapter_state = WM_BT_STATE_OFF;
volatile bt_system_action_t bt_system_action = WM_BT_SYSTEM_ACTION_IDLE;


/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */



/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void app_adapter_state_changed_callback(tls_bt_state_t status)
{
    TLS_BT_APPL_TRACE_DEBUG("adapter status = %s\r\n",
                            status == WM_BT_STATE_ON ? "bt_state_on" : "bt_state_off");
    bt_system_action = WM_BT_SYSTEM_ACTION_IDLE;
#if (TLS_CONFIG_BLE == CFG_ON)

    if(status == WM_BT_STATE_ON) {
        TLS_BT_APPL_TRACE_VERBOSE("init base application\r\n");
        bt_adapter_state = status;
        //at here , user run their own applications;

        //tls_ble_wifi_cfg_init();
        //tls_ble_server_demo_api_init(NULL,NULL);
        //tls_ble_client_demo_api_init(NULL, NULL);
        //tls_ble_server_demo_hid_init();
        //tls_ble_server_hid_uart_init();
        //tls_ble_client_multi_conn_demo_api_init();
        //tls_ble_mesh_init(NULL, MESH_ROLE_NODE, true);

    } else {
        TLS_BT_APPL_TRACE_VERBOSE("deinit base application\r\n");
        //here, user may free their application;

        //tls_ble_wifi_cfg_deinit(2);
        //tls_ble_server_demo_api_deinit();
        //tls_ble_client_demo_api_deinit();
        //tls_ble_client_multi_conn_demo_api_deinit();
        //tls_ble_mesh_deinit();

        bt_adapter_state = status;
    }

#endif
}


static void
on_sync(void)
{
    //int rc;
    /* Make sure we have proper identity address set (public preferred) */
    //rc = ble_hs_util_ensure_addr(1);
    //assert(rc == 0);
    app_adapter_state_changed_callback(WM_BT_STATE_ON);
}
static void
on_reset(int reason)
{
    TLS_BT_APPL_TRACE_DEBUG("Resetting state; reason=%d\r\n", reason);
    app_adapter_state_changed_callback(WM_BT_STATE_OFF);
}
static void
on_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch(ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            TLS_BT_APPL_TRACE_DEBUG("service,uuid16 %s handle=%d (%04X)\r\n",
                                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle, ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            TLS_BT_APPL_TRACE_DEBUG("charact,uuid16 %s arg %d def_handle=%d (%04X) val_handle=%d (%04X)\r\n",
                                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                                    (int)ctxt->chr.chr_def->arg,
                                    ctxt->chr.def_handle, ctxt->chr.def_handle,
                                    ctxt->chr.val_handle, ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            TLS_BT_APPL_TRACE_DEBUG("descrip, uuid16 %s arg %d handle=%d (%04X)\r\n",
                                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                                    (int)ctxt->dsc.dsc_def->arg,
                                    ctxt->dsc.handle, ctxt->dsc.handle);
            break;
    }

    return;
}


int
tls_bt_init(uint8_t uart_idx)
{
    if(bt_adapter_state == WM_BT_STATE_ON) {
        return BLE_HS_EALREADY;
    }

    if(bt_system_action != WM_BT_SYSTEM_ACTION_IDLE) { return BLE_HS_EBUSY; }

    bt_system_action = WM_BT_SYSTEM_ACTION_ENABLING;
    memset(&ble_hs_cfg, 0, sizeof(ble_hs_cfg));
    /** Security manager settings. */
    ble_hs_cfg.sm_io_cap = MYNEWT_VAL(BLE_SM_IO_CAP),
    ble_hs_cfg.sm_oob_data_flag = MYNEWT_VAL(BLE_SM_OOB_DATA_FLAG),
    ble_hs_cfg.sm_bonding = MYNEWT_VAL(BLE_SM_BONDING),
    ble_hs_cfg.sm_mitm = MYNEWT_VAL(BLE_SM_MITM),
    ble_hs_cfg.sm_sc = MYNEWT_VAL(BLE_SM_SC),
    ble_hs_cfg.sm_keypress = MYNEWT_VAL(BLE_SM_KEYPRESS),
    ble_hs_cfg.sm_our_key_dist = MYNEWT_VAL(BLE_SM_OUR_KEY_DIST),
    ble_hs_cfg.sm_their_key_dist = MYNEWT_VAL(BLE_SM_THEIR_KEY_DIST),
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.shutdown_cb = on_reset; /*same callback as on_reset */
    ble_hs_cfg.gatts_register_cb = on_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    /* Initialize all packages. */
    nimble_port_init();
    /*Application levels code entry*/
    tls_ble_gap_init();
    tls_bt_util_init();
    /*Initialize the vuart interface and enable controller*/
    ble_hci_vuart_init(uart_idx);
    /* As the last thing, process events from default event queue. */
    tls_nimble_start();

    while(bt_adapter_state == WM_BT_STATE_OFF) {
        tls_os_time_delay(10);
    }

    return 0;
}


int
tls_bt_deinit(void)
{
    int rc = 0;

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        return BLE_HS_EALREADY;
    }

    if(bt_system_action != WM_BT_SYSTEM_ACTION_IDLE) { return BLE_HS_EBUSY; }

    bt_system_action = WM_BT_SYSTEM_ACTION_DISABLING;
    /*Stop hs system*/
    rc = nimble_port_stop();
    assert(rc == 0);
    /*Stop controller and free vuart resource */
    rc = ble_hci_vuart_deinit();
    assert(rc == 0);
    /*Free hs system resource*/
    nimble_port_deinit();
    /*Free task stack ptr and free hs task*/
    tls_nimble_stop();
    /*Application levels resource cleanup*/
    tls_ble_gap_deinit();
    tls_bt_util_deinit();

    while(bt_adapter_state == WM_BT_STATE_ON) {
        tls_os_time_delay(10);
    }

    return rc;
}


#define SYSTEM_SLEEP_ENABLE 0


#if SYSTEM_SLEEP_ENABLE
static void bt_pmu_timer1_irq(uint8_t *arg)
{
    tls_pmu_timer0_stop();
    //tls_os_queue_send(app_schedule_queue, (void *)(uint32_t)APP_MSG_BLE_WAKEUPED, 4);
}
#endif


void tls_bt_sleep_enter(uint32_t duration_ms)
{
    /**Do not do anything that blocking cpu running, eg. printf*/

#if SYSTEM_SLEEP_ENABLE

#define SLEEP_TO_WAKEUP_DELAY 7
        /**Do not do printf here, blocking is forbidden*/ 
    
        if(duration_ms > 10)
        {
            tls_pmu_timer1_isr_register((tls_pmu_irq_callback)bt_pmu_timer1_irq, NULL);
            tls_pmu_timer1_start((duration_ms-SLEEP_TO_WAKEUP_DELAY));
            tls_pmu_sleep_start();
        }
#endif

}

void tls_bt_sleep_exit(void)
{
    /**Do not do anything that blocking cpu running, eg. printf*/
}

/*This function is called at wm_main.c*/
void tls_bt_entry()
{
    //tls_bt_init(0xFF);    //enable it if you want to turn on bluetooth after system booting
    //tls_bt_ctrl_sleep(true);
    //tls_bt_register_sleep_callback(tls_bt_sleep_enter, tls_bt_sleep_exit);
}

void tls_bt_exit()
{
    //tls_bt_deinit();      //enable it if you want to turn off bluetooth when system reseting;
}


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_at_bt_enable(int uart_no, tls_bt_log_level_t log_level)
{
    int rc = 0;
    tls_appl_trace_level = log_level;
    TLS_BT_APPL_TRACE_VERBOSE("bt system running, uart_no=%d, log_level=%d\r\n", uart_no, log_level);
    rc = tls_bt_init(uart_no);

    if((rc != 0) && (rc != BLE_HS_EALREADY)) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_enable, ret:%s,%d\r\n", tls_bt_rc_2_str(rc), rc);
        return rc;
    }

    return 0;
}

int tls_at_bt_destroy()
{
    int rc = 0;
    TLS_BT_APPL_TRACE_VERBOSE("bt system destroy\r\n");
    rc = tls_bt_deinit();

    if((rc != 0) && (rc != BLE_HS_EALREADY)) {
        TLS_BT_APPL_TRACE_ERROR("tls_bt_disable, ret:%s,%d\r\n", tls_bt_rc_2_str(rc), rc);
    }else
    {
        rc = 0;
    }

    return rc;
}
/**
 * Called                        1) AT cmd; 2)demo show;
 *
 * @param type              0: advertise stop; 1: adv_ind; 2: adv_nonconn_ind;
 *
 *
 * @return                      0 on success; nonzero on failure.
 */

int tls_ble_demo_adv(uint8_t type)
{
    int rc = 0;
    TLS_BT_APPL_TRACE_DEBUG("### %s type=%d\r\n", __FUNCTION__, type);

    CHECK_SYSTEM_READY();

    if(type) {
        uint8_t bt_mac[6] = {0};
        uint8_t adv_data[] = {
            0x0C, 0x09, 'W', 'M', '-', '0', '0', '0', '0', '0', '0', '0', '0',
            0x02, 0x01, 0x05,
            0x03, 0x19, 0xc1, 0x03
        };
        extern int tls_get_bt_mac_addr(uint8_t *mac);
        tls_get_bt_mac_addr(bt_mac);
        sprintf((char *)adv_data + 5, "%02X:%02X:%02X", bt_mac[3], bt_mac[4], bt_mac[5]);
        adv_data[13] = 0x02;  //byte 13 was overwritten to zero by sprintf; recover it;
        rc = tls_ble_gap_set_data(WM_BLE_ADV_DATA, adv_data, 20);

        switch(type) {
            case 1:
                rc = tls_nimble_gap_adv(WM_BLE_ADV_IND, 0);
                break;

            case 2:
                rc = tls_nimble_gap_adv(WM_BLE_ADV_NONCONN_IND, 0);
                break;
            default:
                /*AT/DEMO cmd only support adv_ind mode*/
                return BLE_HS_EINVAL;
        }
    } else {
        rc = tls_nimble_gap_adv(WM_BLE_ADV_STOP, 0);
    }

    return rc;
}

/**This funcation was used by at command at+blescan=1/0 only, */
typedef void scan_resp_cb_fn(int type, int8_t rssi, uint8_t *addr, const uint8_t *name, int name_len, const uint8_t *raw_scan_resp, int raw_scan_resp_length);
static scan_resp_cb_fn *p_scan_resp_cb = NULL;

/**General gap event listener*/
static struct ble_gap_event_listener ble_scan_event_listener;

static int
ble_gap_evt_cb(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields = {0};
    int rc = 0;
    int i = 0;
    char adv_name[64];

    switch(event->type) {
        case BLE_GAP_EVENT_DISC:

            rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                         event->disc.length_data);

            if(rc != 0) {
                return 0;
            }
#if 0

            /* An advertisment report was received during GAP discovery. */
            print_adv_fields(&fields);
#endif 
            /**check gap event triggered by at command first*/
            if(p_scan_resp_cb)
            {
                p_scan_resp_cb(WM_BLE_GAP_EVENT_DISC, event->disc.rssi, event->disc.addr.val, fields.name, fields.name_len, event->disc.data,
                                         event->disc.length_data);
                return 0;
            }

            /**local gap event processing, only print the result*/
            if(fields.name_len)
            {
                memcpy(adv_name, fields.name, fields.name_len);
                adv_name[fields.name_len] = 0;
                printf(">>>>>>name[%d]%s, rssi=%d\r\n", fields.name_len, adv_name, event->disc.rssi);
            }
            for(i = 0; i < event->disc.length_data; i++) {
                printf("%02x", event->disc.data[i]);
            }
            printf("\r\n");
            
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            if(p_scan_resp_cb)
            {
                p_scan_resp_cb(WM_BLE_GAP_EVENT_DISC_COMPLETE, 0, NULL, NULL, 0, NULL,0);
                return 0;
            }            
            break;
        case BLE_GAP_EVENT_HOST_SHUTDOWN:
            printf("Host is shutting down...\r\n");
            break;
        default:
            break;
    }

    return rc;
}


/**
 * Called                        1) AT cmd; 2)demo show;
 *
 * @param type              0: scan stop; 1: scan start, default passive;
 *
 *
 * @return                      0 on success; nonzero on failure.
 */
int tls_ble_demo_scan(uint8_t type)
{
    int rc;
    TLS_BT_APPL_TRACE_DEBUG("### %s type=%d\r\n", __FUNCTION__, type);

    CHECK_SYSTEM_READY();

    if(type) {
        ble_gap_event_listener_register(&ble_scan_event_listener,
                            ble_gap_evt_cb, NULL);
        rc = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);
    } else {
        rc = tls_ble_gap_scan(WM_BLE_SCAN_STOP, false);
        ble_gap_event_listener_unregister(&ble_scan_event_listener);
    }

    return rc;
}

void tls_ble_demo_scan_at_cmd_register(scan_resp_cb_fn *cb)
{
    TLS_BT_APPL_TRACE_DEBUG("### %s type=%d\r\n", __FUNCTION__);
    ble_gap_event_listener_register(&ble_scan_event_listener,
                        ble_gap_evt_cb, NULL);    
    p_scan_resp_cb = cb; 
}
void tls_ble_demo_scan_at_cmd_unregister()
{
    TLS_BT_APPL_TRACE_DEBUG("### %s type=%d\r\n", __FUNCTION__);
    p_scan_resp_cb = NULL;
    ble_gap_event_listener_unregister(&ble_scan_event_listener);
}

/*
*bluetooth api demo
*/
int demo_bt_enable()
{
    int rc;
    uint8_t uart_no = 0xFF;
    tls_appl_trace_level = TLS_BT_LOG_VERBOSE;

    if(bt_adapter_state == WM_BT_STATE_ON) {
        TLS_BT_APPL_TRACE_VERBOSE("bt system enable already");
        return TLS_BT_STATUS_SUCCESS;
    }

    TLS_BT_APPL_TRACE_DEBUG("bt system running, uart_no=%d, log_level=%d\r\n", uart_no,
                            tls_appl_trace_level);
    rc = tls_bt_init(uart_no);

    if((rc != 0) && (rc != BLE_HS_EALREADY)) {
        TLS_BT_APPL_TRACE_ERROR("demo_bt_enable, ret:%s,%d\r\n", tls_bt_rc_2_str(rc), rc);
    }

    return rc;
}

int demo_bt_destroy()
{
    int rc;
    TLS_BT_APPL_TRACE_DEBUG("bt system destroy\r\n");

    if(bt_adapter_state == WM_BT_STATE_OFF) {
        TLS_BT_APPL_TRACE_VERBOSE("bt system destroyed already");
        return TLS_BT_STATUS_SUCCESS;
    }

    rc = tls_bt_deinit();

    if((rc != 0) && (rc != BLE_HS_EALREADY)) {
        TLS_BT_APPL_TRACE_ERROR("demo_bt_destroy, ret:%s,%d\r\n", tls_bt_rc_2_str(rc), rc);
    }

    return rc;
}

int demo_ble_server_on()
{
    CHECK_SYSTEM_READY();

    tls_ble_server_demo_api_init(NULL,NULL);
    return 0;
}
int demo_ble_server_off()
{
    CHECK_SYSTEM_READY();

    tls_ble_server_demo_api_deinit();
    return 0;
}
int demo_ble_client_on()
{
    CHECK_SYSTEM_READY();

    tls_ble_client_demo_api_init(NULL,NULL);
    return 0;
}
int demo_ble_client_off()
{
    CHECK_SYSTEM_READY();

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

