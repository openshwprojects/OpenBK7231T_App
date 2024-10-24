
#include <assert.h>
#include <string.h>

#include "wm_bt_config.h"

#if (WM_NIMBLE_INCLUDED == CFG_ON)

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"

/* Mandatory services. */
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* Application-specified header. */
#include "wm_bt_app.h"
#include "wm_bt_util.h"
#include "wm_ble_gap.h"
#include "wm_ble_uart_if.h"
#include "wm_ble_client_util.h"
#include "wm_ble_client_api_demo.h"



#define WM_BLE_SERVER_SVC_UUID              0xFFF0
#define WM_BLE_SERVER_CHR_INDICATE          0xFFF1
#define WM_BLE_SERVER_CHR_WRITE             0xFFF2
#define WM_BLE_SERVER_CHR_READ              0xFFF3

typedef enum {
    BLE_CLIENT_MODE_IDLE     = 0x00,
    BLE_CLIENT_MODE_SCANNING = 0x01,
    BLE_CLIENT_MODE_CONNECTING,
    BLE_CLIENT_MODE_CONNECTED,
    BLE_CLIENT_MODE_SUBSCRIBING,
    BLE_CLIENT_MODE_SUBSCRIBED,
    BLE_CLIENT_MODE_EXITING
} ble_client_state_t;
static struct ble_gap_event_listener ble_client_event_listener;
static uint16_t g_ble_client_conn_handle = 0;
static uint8_t g_ble_client_state = BLE_CLIENT_MODE_IDLE;
static volatile uint8_t g_send_pending = 0;
static int g_mtu = 20;
static tls_ble_uart_output_ptr g_uart_output_ptr = NULL; 
static tls_ble_uart_sent_ptr g_uart_in_and_sent_ptr = NULL;


static int ble_gap_evt_cb(struct ble_gap_event *event, void *arg);



/**
 * Application callback.  Called when the write to the [0xFFF2]
 * Control Point characteristic has completed.
 */
static int
wm_ble_client_demo_on_write(uint16_t conn_handle,
                            const struct ble_gatt_error *error,
                            struct ble_gatt_attr *attr,
                            void *arg)
{
#if 1
    TLS_BT_APPL_TRACE_DEBUG("Write complete; status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);
#endif

    int rc = 0;
    g_send_pending = 0;

    if(error->status != 0) goto err;

    if(g_uart_in_and_sent_ptr) {
        g_uart_in_and_sent_ptr(BLE_UART_CLIENT_MODE, error->status);
    }

    return 0;
err:
    /* Terminate the connection. */
    ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return rc;
}

/**
 * Application callback.  Called when the attempt to subscribe to indications
 * for the Specified characteristic Status characteristic has completed.
 */
static int
wm_ble_client_demo_on_subscribe(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                struct ble_gatt_attr *attr,
                                void *arg)
{
    TLS_BT_APPL_TRACE_DEBUG("Subscribe complete; status=%d conn_handle=%d attr_handle=%d\r\n",
                            error->status, conn_handle, attr->handle);

    if(error->status == 0) {
        if(g_ble_client_state == BLE_CLIENT_MODE_SUBSCRIBING) {
            g_ble_client_state = BLE_CLIENT_MODE_SUBSCRIBED;
        } else if(g_ble_client_state == BLE_CLIENT_MODE_EXITING) {
            TLS_BT_APPL_TRACE_DEBUG("Terminate the connection by closing client\r\n");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
    } else {
        TLS_BT_APPL_TRACE_DEBUG("Terminate the connection now by subscribe failed , next to disconnect and scanning...\r\n");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    return 0;
}
static int
wm_ble_client_demo_cancel_subscribe(const struct peer *peer)
{
    const struct peer_dsc *dsc;
    uint8_t value[2];
    int rc;
    /* Subscribe to indifications for the Specified characteristic[0xFFF1].
     * A central disables indications by writing two bytes (0, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    dsc = peer_dsc_find_uuid(peer,
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_INDICATE),
                             BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));

    if(dsc == NULL) {
        TLS_BT_APPL_TRACE_ERROR("Error: Peer lacks a CCCD for the 0xFFF1 characteristic\n");
        goto err;
    }

    value[0] = 0;
    value[1] = 0;
    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, sizeof value, wm_ble_client_demo_on_subscribe, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to subscribe to characteristic;rc=%d\n", rc);
        goto err;
    }

    return rc;
err:
    /* Terminate the connection. */
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return rc;
}
/**
 * Performs three concurrent GATT operations against the specified peer:
 * 1. Writes the 0xFFF2 characteristic.
 * 2. Subscribes to notifications for 0xFFF1
 *    characteristic.
 *
 * If the peer does not support a required service, characteristic, or
 * descriptor, then the peer lied when it claimed support for the alert
 * notification service!  When this happens, or if a GATT procedure fails,
 * this function immediately terminates the connection.
 */

static void
wm_ble_client_demo_read_write_subscribe(const struct peer *peer)
{
    const struct peer_chr *chr;
    const struct peer_dsc *dsc;
    uint8_t value[] = {'H','e', '1','l','o', ',', 'B', 'L', 'E', '!', 0x0A};
    int rc;
    /* Write two bytes (0xAA, 0xBB) tothe Specified characteristic[WM_BLE_SERVER_CHR_WRITE].
     */
    chr = peer_chr_find_uuid(peer,
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_WRITE));

    if(chr == NULL) {
        TLS_BT_APPL_TRACE_ERROR("Error: Peer doesn't support 0x%04x Control Point characteristic\r\n",WM_BLE_SERVER_CHR_WRITE);
        goto err;
    }

    rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle,
                              value, sizeof value, wm_ble_client_demo_on_write, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to write characteristic; rc=%d\n",
                                rc);
    }

    /* Subscribe to indifications for the Specified characteristic[0xFFF1].
     * A central enables indications by writing two bytes (2, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    dsc = peer_dsc_find_uuid(peer,
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_INDICATE),
                             BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));

    if(dsc == NULL) {
        TLS_BT_APPL_TRACE_ERROR("Error: Peer lacks a CCCD for the 0x%04x characteristic\r\n",WM_BLE_SERVER_CHR_INDICATE);
        goto err;
    }
    value[0] = 2;
    value[1] = 0;
    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, 2, wm_ble_client_demo_on_subscribe, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to subscribe to characteristic;rc=%d\n", rc);
        goto err;
    } else {
        g_ble_client_state = BLE_CLIENT_MODE_SUBSCRIBING;
    }

    return;
err:
    /* Terminate the connection. */
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

static int
wm_ble_client_demo_on_mtu(uint16_t conn_handle, const struct ble_gatt_error *error,
                          uint16_t mtu, void *arg)
{
    switch(error->status) {
        case 0:
            TLS_BT_APPL_TRACE_DEBUG("mtu exchange complete: conn_handle=%d mtu=%d\r\n",
                                    conn_handle, mtu);
            g_mtu = mtu - 3;
            break;

        default:
            TLS_BT_APPL_TRACE_ERROR("Update MTU failed...error->status=%d\r\n", error->status);
            break;
    }

    return 0;
}

/**
* Called when service discovery of the specified peer has completed.
*/
static void
wm_ble_client_demo_on_disc_complete(const struct peer *peer, int status, void *arg)
{
  if(status != 0) {
      /* Service discovery failed.  Terminate the connection. */
      TLS_BT_APPL_TRACE_ERROR("Error: Service discovery failed; status=%d conn_handle=%d,state=%d\n",
                              status, peer->conn_handle, g_ble_client_state);
      ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      return;
  }
  ble_gattc_exchange_mtu(g_ble_client_conn_handle, wm_ble_client_demo_on_mtu, NULL);

  /* Service discovery has completed successfully.  Now we have a complete
   * list of services, characteristics, and descriptors that the peer
   * supports.
   */
  TLS_BT_APPL_TRACE_DEBUG("Service discovery complete; status=%d conn_handle=%d\n", status,
                          peer->conn_handle);
  /* Now perform three concurrent GATT procedures against the peer: read,
   * write, and subscribe to notifications.
   */
  wm_ble_client_demo_read_write_subscribe(peer);
}

/**
 * Indicates whether we should tre to connect to the sender of the specified
 * advertisement.  The function returns a positive result if the device
 * advertises connectability and support for the Alert Notification service.
 */
static int
wm_ble_client_demo_should_connect(const struct ble_gap_disc_desc *disc)
{
    struct ble_hs_adv_fields fields;
    int rc;
    int i;

    /* The device has to be advertising connectability. */
    if(disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
            disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return 0;
    }

    rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);

    if(rc != 0) {
        return rc;
    }

    /* The device has to advertise support for the Alert Notification
     * service (WM_BLE_SERVER_SVC_UUID).
     */
    for(i = 0; i < fields.num_uuids16; i++) {
        if(ble_uuid_u16(&fields.uuids16[i].u) == WM_BLE_SERVER_SVC_UUID) {
            return 1;
        }
    }

    return 0;
}

/**
 * Connects to the sender of the specified advertisement of it looks
 * interesting.  A device is "interesting" if it advertises connectability and
 * support for the Alert Notification service.
 */
static void
wm_ble_client_demo_connect_if_interesting(const struct ble_gap_disc_desc *disc)
{
    int rc;
    struct ble_gap_conn_params conn_params;

    /* Don't do anything if we don't care about this advertiser. */
    if(!wm_ble_client_demo_should_connect(disc)) {
        return;
    }

    g_ble_client_state = BLE_CLIENT_MODE_CONNECTING; //preset this flag
    /* Scanning must be stopped before a connection can be initiated. */
    rc = tls_ble_gap_scan(WM_BLE_SCAN_STOP, 0);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Failed to cancel scan; rc=%d\n", rc);
        g_ble_client_state = BLE_CLIENT_MODE_SCANNING;
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
     * timeout.
     */
    ble_gap_init_conn_params(&conn_params);
    //conn_params.itvl_min = 0x08;
    //conn_params.itvl_max = 0x0C;
    rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &disc->addr, 30000, &conn_params,
                         NULL, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to connect to device; addr_type=%d "
                                "addr=%s, continue to scan\n",
                                disc->addr.type, addr_str(disc->addr.val));
        rc = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);

        if(rc != 0) {
            TLS_BT_APPL_TRACE_ERROR("Failed to start scan when ble_gap_connect failed; rc=%d\n", rc);
            g_ble_client_state = BLE_CLIENT_MODE_IDLE;
            return;
        } else {
            g_ble_client_state = BLE_CLIENT_MODE_SCANNING;
        }

        return;
    } else {
        g_ble_client_state = BLE_CLIENT_MODE_CONNECTING;
    }
}
#define BLE_CLIENT_CONN_PARAM_UPDATE 0

#if BLE_CLIENT_CONN_PARAM_UPDATE
static void wm_ble_client_demo_conn_param_update_master(u16_t conn_handle)
{
    int rc;
    struct ble_gap_upd_params params;
    params.itvl_min = 0x0006;
    params.itvl_max = 0x0016;
    params.latency = 0;
    params.supervision_timeout = 500;
    params.min_ce_len = 16;
    params.max_ce_len = 328;
    rc = ble_gap_update_params(conn_handle, &params);
    assert(rc == 0);
}
#endif


//#define THROUGHTPUT_TEST

#ifdef THROUGHTPUT_TEST

static uint32_t g_recv_bytes = 0;
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

static void wm_ble_client_read_service(void *arg)
{
	int rc;
    /* Perform service discovery. */
    rc = peer_disc_all(g_ble_client_conn_handle,
               wm_ble_client_demo_on_disc_complete, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Failed to discover services; rc=%d\n", rc);
        ble_gap_terminate(g_ble_client_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }    
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that is
 * established.  blecent uses the same callback for all connections.
 *
 * @param event                 The event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  blecent.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int
ble_gap_evt_cb(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    struct os_mbuf *om;
    int rc;

    switch(event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                         event->disc.length_data);

            if(rc != 0) {
                return 0;
            }

            /* Try to connect to the advertiser if it looks interesting. */
            wm_ble_client_demo_connect_if_interesting(&event->disc);
            return 0;

        case BLE_GAP_EVENT_CONNECT:

            /* A new connection was established or a connection attempt failed. */
            if(event->connect.status == 0) {

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                print_conn_desc(&desc);
                MODLOG_DFLT(INFO, "\n");

                if(desc.role != BLE_GAP_ROLE_MASTER)
                {
                    return 0;
                }
                /* Connection successfully established. */
                TLS_BT_APPL_TRACE_API("Client Connection established ");                
                g_ble_client_state = BLE_CLIENT_MODE_CONNECTED;
                g_ble_client_conn_handle = event->connect.conn_handle;
#ifdef THROUGHTPUT_TEST
                g_recv_bytes = 0;
                g_time_last = tls_os_get_time();
#endif
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                print_conn_desc(&desc);
                MODLOG_DFLT(INFO, "\n");
                /* Remember peer. */
                rc = peer_add(event->connect.conn_handle);

                if(rc != 0) {
                    TLS_BT_APPL_TRACE_ERROR("Failed to add peer; rc=%d\n", rc);
                    return 0;
                }
                //defer 100ticks, then find service;
                tls_bt_async_proc_func(wm_ble_client_read_service, NULL, 100);

                //ble_gattc_exchange_mtu(event->connect.conn_handle, wm_ble_client_demo_on_mtu, NULL);
                #if BLE_CLIENT_CONN_PARAM_UPDATE
                wm_ble_client_demo_conn_param_update_master(event->connect.conn_handle);
                #endif
            } else {
                /* Connection attempt failed; resume scanning. */
                TLS_BT_APPL_TRACE_ERROR("Error: Connection failed; status=%d\n",
                                        event->connect.status);
                //g_ble_client_state = BLE_CLIENT_MODE_IDLE;
                rc = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);

                if(rc == 0) {
                    g_ble_client_state = BLE_CLIENT_MODE_SCANNING;
                }
            }

            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            
            if(event->disconnect.conn.role != BLE_GAP_ROLE_MASTER) return 0;
            /* Connection terminated. */
            TLS_BT_APPL_TRACE_API("Client disconnect[state=%d]; reason=%d 0x%02x ", g_ble_client_state,event->disconnect.reason,event->disconnect.reason - 0x200);
            print_conn_desc(&event->disconnect.conn);
            MODLOG_DFLT(INFO, "\n");
            /* Forget about peer. */
            rc = peer_delete(event->disconnect.conn.conn_handle);

            if(rc != 0) {
                TLS_BT_APPL_TRACE_WARNING("peer_delete (conn_handle=%d), rc=%d\r\n",
                                        event->disconnect.conn.conn_handle,rc);
            }

            /* Resume scanning. If the termination is not issued by local host */

            if(g_ble_client_state == BLE_CLIENT_MODE_EXITING ) {
                peer_deinit();
                g_ble_client_state = BLE_CLIENT_MODE_IDLE;
                g_uart_output_ptr = NULL;
                g_uart_in_and_sent_ptr = NULL;
                ble_gap_event_listener_unregister(&ble_client_event_listener);
            } else {
                /*check disconnect reason is not trigger by local host*/
                if(event->disconnect.reason != 534)
                {
                    g_ble_client_state = BLE_CLIENT_MODE_SCANNING;
                    tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);
                }
            }

            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            TLS_BT_APPL_TRACE_DEBUG("discovery complete; reason=%d\n",
                                    event->disc_complete.reason);

            if(g_ble_client_state == BLE_CLIENT_MODE_SCANNING) {
                g_uart_output_ptr = NULL;
                g_uart_in_and_sent_ptr = NULL;
                g_ble_client_state = BLE_CLIENT_MODE_IDLE;
                TLS_BT_APPL_TRACE_ERROR("Occurs only when limited scanning\r\n");
            }

            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;
            /* Encryption has been enabled or disabled for this connection. */
            TLS_BT_APPL_TRACE_DEBUG("encryption change event; status=%d ",
                                    event->enc_change.status);
            print_conn_desc(&desc);
            return 0;

        case BLE_GAP_EVENT_NOTIFY_RX:
            rc = ble_gap_conn_find(event->notify_rx.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;            
            /* Peer sent us a notification or indication. */

            TLS_BT_APPL_TRACE_VERBOSE("received %s; conn_handle=%d attr_handle=%d "
                                    "attr_len=%d\n",
                                    event->notify_rx.indication ?
                                    "indication" :
                                    "notification",
                                    event->notify_rx.conn_handle,
                                    event->notify_rx.attr_handle,
                                    OS_MBUF_PKTLEN(event->notify_rx.om));                       
            /* Attribute data is contained in event->notify_rx.attr_data. */
            om = event->notify_rx.om;

            while(om != NULL) {
                if(g_uart_output_ptr) {
                    g_uart_output_ptr(UART_OUTPUT_DATA, om->om_data, om->om_len);
                } else {
                    print_bytes(om->om_data, om->om_len);
                }

#ifdef THROUGHTPUT_TEST
                g_recv_bytes += om->om_len;
#endif
                om = SLIST_NEXT(om, om_next);
            }

#ifdef THROUGHTPUT_TEST

            if(ticks_elapsed(g_time_last) >= HZ) {
                g_time_last = tls_os_get_time();
                printf("BLE Recv(%d bytes)[%5.2f Kbps]/s\r\n", g_recv_bytes, (g_recv_bytes * 8.0 / 1024));
                g_recv_bytes = 0;
            }

#endif
            return 0;

        case BLE_GAP_EVENT_MTU:
            rc = ble_gap_conn_find(event->mtu.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;            
            TLS_BT_APPL_TRACE_DEBUG("mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                                    event->mtu.conn_handle,
                                    event->mtu.channel_id,
                                    event->mtu.value);
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */
            /* Delete the old bond. */
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;   
            ble_store_util_delete_peer(&desc.peer_id_addr);
            /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
             * continue with the pairing operation.
             */
            return BLE_GAP_REPEAT_PAIRING_RETRY;
        case BLE_GAP_EVENT_HOST_SHUTDOWN:
            TLS_BT_APPL_TRACE_DEBUG("Client BLE_GAP_EVENT_HOST_SHUTDOWN:%d\r\n", event->type);
            g_ble_client_state = BLE_CLIENT_MODE_IDLE;
            g_send_pending = 0;
            g_mtu = 20;
            peer_deinit();
            g_uart_output_ptr = NULL;
            g_uart_in_and_sent_ptr = NULL;            
            break;
        default:
            TLS_BT_APPL_TRACE_VERBOSE("Client Unhandled event:%d\r\n", event->type);
            return 0;
    }

    return 0;
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_client_demo_api_init(tls_ble_uart_output_ptr uart_output_ptr, tls_ble_uart_sent_ptr uart_in_and_sent_ptr)
{
    int rc = BLE_HS_EAPP ;

    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_DEBUG("%s state=%d\r\n", __FUNCTION__, g_ble_client_state);

    if(g_ble_client_state == BLE_CLIENT_MODE_IDLE) {
        rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);

        if(rc == 0) {
            ble_gap_event_listener_register(&ble_client_event_listener,
                                    ble_gap_evt_cb, NULL);
            TLS_BT_APPL_TRACE_DEBUG("%s success\r\n", __FUNCTION__);
            g_uart_output_ptr = uart_output_ptr;
            g_uart_in_and_sent_ptr = uart_in_and_sent_ptr;
            rc = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);

            if(rc == 0) {
                g_ble_client_state = BLE_CLIENT_MODE_SCANNING;
            } else {
                TLS_BT_APPL_TRACE_ERROR("%s failed, tls_ble_gap_scan passive\r\n", __FUNCTION__);
            }
        }
    } else {
        return BLE_HS_EALREADY;
    }

    return rc;
}

int tls_ble_client_demo_api_deinit()
{
    int rc = BLE_HS_EAPP ;
    const struct peer *peer;

    CHECK_SYSTEM_READY();

    TLS_BT_APPL_TRACE_DEBUG("%s state=%d\r\n", __FUNCTION__, g_ble_client_state);

    if(g_ble_client_state == BLE_CLIENT_MODE_CONNECTING
            || g_ble_client_state == BLE_CLIENT_MODE_CONNECTED
            || g_ble_client_state == BLE_CLIENT_MODE_SUBSCRIBING
            || g_ble_client_state == BLE_CLIENT_MODE_SUBSCRIBED) {
        if(g_ble_client_state == BLE_CLIENT_MODE_SUBSCRIBED) {
            g_ble_client_state = BLE_CLIENT_MODE_EXITING; //here I only preset the flag;
            peer = get_peer_by_conn_handle(g_ble_client_conn_handle);

            if(peer) {
                TLS_BT_APPL_TRACE_DEBUG("cancel subsribe and terminate the connection\r\n");
                //NOTE , The disconnect process will be issued after cancel subscribe complete;
                rc = wm_ble_client_demo_cancel_subscribe(peer);
            } else {
                rc = ble_gap_terminate(g_ble_client_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        } else {
            g_ble_client_state = BLE_CLIENT_MODE_EXITING; //here I only preset the flag;
            rc = ble_gap_terminate(g_ble_client_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
    } else if(g_ble_client_state == BLE_CLIENT_MODE_SCANNING) {
        rc = tls_ble_gap_scan(WM_BLE_SCAN_STOP, 0);

        if(rc == 0) {
            g_ble_client_state = BLE_CLIENT_MODE_IDLE;
            peer_deinit();
            g_uart_output_ptr = NULL;
            g_uart_in_and_sent_ptr = NULL;
        }

        TLS_BT_APPL_TRACE_DEBUG("tls_ble_gap_scan rc=%d\r\n", rc);
        ble_gap_event_listener_unregister(&ble_client_event_listener);
    } else if(g_ble_client_state == BLE_CLIENT_MODE_IDLE) {
        rc = 0;
        ble_gap_event_listener_unregister(&ble_client_event_listener);
    } else {
        rc = BLE_HS_EALREADY;
        TLS_BT_APPL_TRACE_DEBUG("%s busy, state=%d\r\n", __FUNCTION__, g_ble_client_state);
    }

    return rc;
}


static int
tls_ble_client_demo_on_read(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                struct ble_gatt_attr *attr,
                void *arg)
{
    TLS_BT_APPL_TRACE_DEBUG("Read complete; status=%d conn_handle=%d\r\n", error->status,
                conn_handle);
    const struct os_mbuf *om = attr->om;
    
    if (error->status == 0) {
        while(om != NULL) {

            print_bytes(om->om_data, om->om_len);
            om = SLIST_NEXT(om, om_next);
        }        
    }

    return 0;
}
int tls_ble_client_demo_api_read_msg()
{
    const struct peer *peer;
    const struct peer_chr *chr;
    int rc = BLE_HS_ENOTCONN;

    CHECK_SYSTEM_READY();
    
    if(g_ble_client_state) {
        peer = get_peer_by_conn_handle(g_ble_client_conn_handle);

        if(peer) {
            chr = peer_chr_find_uuid(peer,
                                     BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                                     BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_READ));

            if(chr == NULL) {
                TLS_BT_APPL_TRACE_ERROR("Error: Peer doesn't Control Point characteristic[0xFFF3]\r\n");
                return BLE_HS_EAPP;
            }

            rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle,
                                tls_ble_client_demo_on_read, NULL);
            if (rc != 0) {
                TLS_BT_APPL_TRACE_ERROR("Error: Failed to read characteristic; rc=%d\r\n",rc);
            }

        }
    } else {
        return BLE_HS_ENOTCONN;
    }    

    return rc;
}

int tls_ble_client_demo_api_send_msg(uint8_t *ptr, int length)
{
    const struct peer *peer;
    const struct peer_chr *chr;
    int rc = 0;

    TLS_BT_APPL_TRACE_VERBOSE("### %s len=%d\r\n", __FUNCTION__, length);

    CHECK_SYSTEM_READY();
    
    if(g_send_pending) { return BLE_HS_EBUSY; }

    if(g_ble_client_state) {
        peer = get_peer_by_conn_handle(g_ble_client_conn_handle);

        if(peer) {
            chr = peer_chr_find_uuid(peer,
                                     BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                                     BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_WRITE));

            if(chr == NULL) {
                TLS_BT_APPL_TRACE_ERROR("Error: Peer doesn't Control Point characteristic[0xFFF2]\n");
                return BLE_HS_EAPP;
            }

            rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle,
                                      ptr, length, wm_ble_client_demo_on_write, NULL);

            if(rc != 0) {
                TLS_BT_APPL_TRACE_ERROR("Error: Failed to write characteristic; rc=%d\r\n",
                                        rc);
                return rc;
            } else {
                g_send_pending = 1;
            }
        }
    } else {
        return BLE_HS_ENOTCONN;
    }

    return rc;
}
#endif

