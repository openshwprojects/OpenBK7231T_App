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
#include "wm_ble_client_util.h"
#include "wm_ble_gap.h"
#include "wm_bt_util.h"

static struct ble_gap_event_listener ble_mclient_event_listener;

/* BLE server srivces list*/
#define WM_BLE_SERVER_SVC_UUID              0xFFF0
#define WM_BLE_SERVER_CHR_INDICATE          0xFFF1
#define WM_BLE_SERVER_CHR_WRITE             0xFFF2


#define MAX_CONN_DEVCIE_COUNT    (MYNEWT_VAL(BLE_MAX_CONNECTIONS)-1)

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
    ble_addr_t addr;
    uint16_t conn_handle;
    uint16_t conn_mtu;
    uint8_t  remote_name[16];      /**parsed out from name filed in advertisement data, */
    uint8_t  subscribed;
    uint32_t write_counter;        /**indicate the notification counter*/

} connect_device_t;

static connect_device_t conn_devices[MAX_CONN_DEVCIE_COUNT];
static bool g_ble_client_switching_off = false;
static uint8_t g_ble_client_inited = 0;

static int ble_gap_evt_cb(struct ble_gap_event *event, void *arg);

static void wm_ble_dump_conn_status(void)
{
    int i = 0;
    printf("          =========remote device info========\r\n");
    printf("ID, STATE, HANDLE,       MAC          ,    TRANSFER    \r\n");

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        printf("%d,     %d,     %d,     %s,      %d\r\n", i + 1,
               conn_devices[i].conn_state, conn_devices[i].conn_handle,
               addr_str(conn_devices[i].addr.val),conn_devices[i].write_counter);
    }
}

static void wm_ble_update_conn_params(struct ble_gap_conn_params *conn_params)
{
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_state == DEV_DISCONNCTED) {
            conn_params->itvl_min += i ;
            conn_params->itvl_max += i ;
            conn_params->latency = 2;
            return;
        }
    }
}
static void wm_ble_update_notify_counter(uint16_t conn_handle)
{
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_handle == conn_handle) {
            conn_devices[i].write_counter++;
            return;
        }
    }
}

static bool wm_ble_check_all_connect_state(conn_state_t conn_state)
{
    bool ret = true;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_state != conn_state) {
            return false;
        }
    }

    return ret;
}
static bool wm_ble_check_any_connecting()
{
    bool ret = true;
    int i = 0;

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_state == DEV_CONNECTING) {
            return ret;
        }
    }

    return false;
}

/*
* update the connected devices table, and return true, if there is disconnected device. which means the client will do
* ble scan and connect . otherwise all devcies connected , do nothing;
*/
static bool wm_ble_update_conn_devices(uint16_t conn_handle, conn_state_t conn_state,
                                       ble_addr_t *p_addr)
{
    bool ret = false;
    int i = 0;

    switch(conn_state) {
        case DEV_CONNECTING:
            for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
                if(conn_devices[i].conn_state == DEV_DISCONNCTED) { break; }
            }

            if(i < MAX_CONN_DEVCIE_COUNT) {
                conn_devices[i].conn_state = conn_state;
                conn_devices[i].conn_handle = conn_handle;
                memcpy(&conn_devices[i].addr, p_addr, sizeof(ble_addr_t));
            } else {
                assert(0);
            }

            break;

        case DEV_CONNECTED:
            for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
                if((conn_devices[i].conn_state == DEV_CONNECTING) && (conn_devices[i].conn_handle == conn_handle)) {
                    break;
                }
            }

            if(i < MAX_CONN_DEVCIE_COUNT) {
                conn_devices[i].conn_state = conn_state;
                conn_devices[i].conn_handle = conn_handle;
            } else {
                assert(0);
            }

            break;

        case DEV_DISCONNCTED:
            for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
                #if 0
                if(((conn_devices[i].conn_state == DEV_CONNECTED)
                        || (conn_devices[i].conn_state == DEV_CONNECTING))
                        && (conn_devices[i].conn_handle == conn_handle)) {
                    break;
                }
                #else
                if(conn_devices[i].conn_handle == conn_handle) {
                    break;
                }
                #endif
            }

            if(i < MAX_CONN_DEVCIE_COUNT) {
                conn_devices[i].conn_state = DEV_DISCONNCTED;
                conn_devices[i].conn_handle = 0xFFFF;
            } else {
                //assert(0);
            }

            break;
    }

    for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
        if(conn_devices[i].conn_state == DEV_DISCONNCTED) {
            ret = true;
            break;
        }
    }

    return ret;
}



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
    TLS_BT_APPL_TRACE_DEBUG("Write complete; status=%d conn_handle=%d attr_handle=%d\r\n",
                            error->status, conn_handle, attr->handle);
    if(error->status == 261 || error->status == 271)
    {
        return ble_gap_security_initiate(conn_handle);
    }
    return 0;
}
static void
wm_ble_client_async_gap_scan(void *app_arg)
{
    tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);
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
    bool ret = false;
    struct ble_gap_conn_desc desc;
    TLS_BT_APPL_TRACE_DEBUG("Subscribe complete; status=%d conn_handle=%d "
                            "attr_handle=%d\r\n",
                            error->status, conn_handle, attr->handle);

    if(error->status == 0) {
        int rc = ble_gap_conn_find(conn_handle, &desc);
        assert(rc == 0);
        ret = wm_ble_update_conn_devices(conn_handle, DEV_CONNECTED, &desc.peer_id_addr);
    } else {
         ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    wm_ble_dump_conn_status();

    if(ret) {
        TLS_BT_APPL_TRACE_API("Continue to scan next device to connect...\r\n");
        tls_bt_async_proc_func(wm_ble_client_async_gap_scan, NULL, 200);
    }

    return 0;
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
    uint8_t value[2];
    int rc;
    /* Write two bytes (0xAA, 0xBB) to the alert-notification-control-point
     * characteristic.
     */
    chr = peer_chr_find_uuid(peer,
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_SVC_UUID),
                             BLE_UUID16_DECLARE(WM_BLE_SERVER_CHR_WRITE));

    if(chr == NULL) {
        TLS_BT_APPL_TRACE_ERROR("Error: Peer doesn't support 0xFFF2 characteristic\r\n");
        goto err;
    }

    value[0] = 0xAA;
    value[1] = 0xBB;
    rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle,
                              value, sizeof value, wm_ble_client_demo_on_write, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to write characteristic; rc=%d\r\n",
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
        TLS_BT_APPL_TRACE_ERROR("Error: Peer lacks a CCCD for the 0xFFF1 characteristic\r\n");
        goto err;
    }

    value[0] = 2;
    value[1] = 0;
    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, sizeof value, wm_ble_client_demo_on_subscribe, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to subscribe to characteristic,rc=%d\r\n", rc);
        goto err;
    }

    return;
err:
    /* Terminate the connection. */
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}



/**
 * Called when service discovery of the specified peer has completed.
 */
static void
wm_ble_client_demo_on_disc_complete(const struct peer *peer, int status, void *arg)
{
    if(status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        TLS_BT_APPL_TRACE_ERROR("Error: Service discovery failed; status=%d "
                                "conn_handle=%d\r\n", status, peer->conn_handle);
        wm_ble_update_conn_devices(peer->conn_handle, DEV_DISCONNCTED, NULL);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    TLS_BT_APPL_TRACE_API("Service discovery complete; status=%d "
                          "conn_handle=%d\r\n", status, peer->conn_handle);
    /* Now perform three concurrent GATT procedures against the peer: read,
     * write, and subscribe to notifications.
     */
    wm_ble_client_demo_read_write_subscribe(peer);
}


static int
wm_ble_client_demo_on_mtu(uint16_t conn_handle, const struct ble_gatt_error *error,
                          uint16_t mtu, void *arg)
{
    int rc;
    
    switch(error->status) {
        case 0:
            TLS_BT_APPL_TRACE_API("mtu exchange complete: conn_handle=%d mtu=%d\n", conn_handle, mtu);
            break;

        default:
            TLS_BT_APPL_TRACE_ERROR("Update MTU failed...\r\n");
            break;
    }
    
    /* Perform service discovery. */
    rc = peer_disc_all(conn_handle,
               wm_ble_client_demo_on_disc_complete, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Failed to discover services; rc=%d\n", rc);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    return 0;
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
     * service (0xFFF0).
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

    /* Scanning must be stopped before a connection can be initiated. */
    rc = tls_ble_gap_scan(WM_BLE_SCAN_STOP, 0);;

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Failed to cancel scan; rc=%d\r\n", rc);
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
     * timeout.
     */
    ble_gap_init_conn_params(&conn_params);
    //conn_params.scan_itvl = 0x20;
    //conn_params.scan_window = 0x20;
    //conn_params.supervision_timeout = 500;
    //conn_params.max_ce_len = 512;
    //conn_params.min_ce_len = 0;
    /*Update connection interval*/
    wm_ble_update_conn_params(&conn_params);
    rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &disc->addr, 30000, &conn_params,
                         NULL, NULL);

    if(rc != 0) {
        TLS_BT_APPL_TRACE_ERROR("Error: Failed to connect to device; addr_type=%d "
                                "addr=%s, rc=%d(0x%04x)\r\n",
                                disc->addr.type, addr_str(disc->addr.val), rc, rc);
        return;
    }else
    {
        TLS_BT_APPL_TRACE_DEBUG("MC conntecting to %s\r\n", addr_str(disc->addr.val));
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
                if(desc.role != BLE_GAP_ROLE_MASTER) return 0;
                /* Connection successfully established. */
                TLS_BT_APPL_TRACE_API("MC Connection established \r\n");                
                print_conn_desc(&desc);
                TLS_BT_APPL_TRACE_DEBUG("\r\n");
                
                wm_ble_update_conn_devices(event->connect.conn_handle, DEV_CONNECTING, &desc.peer_id_addr);
                wm_ble_dump_conn_status();
                /* Remember peer. */
                rc = peer_add(event->connect.conn_handle);

                if(rc != 0) {
                    TLS_BT_APPL_TRACE_ERROR("MC Failed to add peer; rc=%d\r\n", rc);
                    return 0;
                }

                ble_gattc_exchange_mtu(event->connect.conn_handle, wm_ble_client_demo_on_mtu, NULL);
            } else {
                /* Connection attempt failed; resume scanning. */
                TLS_BT_APPL_TRACE_ERROR("Error:MC Connection failed; status=%d\r\n",
                                        event->connect.status);
                tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);
            }

            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            if(event->disconnect.conn.role != BLE_GAP_ROLE_MASTER) return 0;
            /* Connection terminated. */
            TLS_BT_APPL_TRACE_API("MC disconnect; handle=%d reason=%d \r\n", event->disconnect.conn.conn_handle, event->disconnect.reason);
            print_conn_desc(&event->disconnect.conn);
            TLS_BT_APPL_TRACE_DEBUG("\r\n");
            /* Forget about peer. */
            rc = peer_delete(event->disconnect.conn.conn_handle);

            if(rc != 0) {
                TLS_BT_APPL_TRACE_ERROR("MC peer_delete (conn_handle=%d)failed\r\n",
                                        event->disconnect.conn.conn_handle);
            }

            wm_ble_update_conn_devices(event->disconnect.conn.conn_handle, DEV_DISCONNCTED, NULL);

            /* Resume scanning. If the termination is not issued by local host */
            if(g_ble_client_switching_off) {
                //peer_deinit();
            } else {
                bool connecting = wm_ble_check_any_connecting();

                if(!connecting) { tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false); }
            }

            wm_ble_dump_conn_status();

            if(g_ble_client_switching_off)
            {
                bool ret = wm_ble_check_all_connect_state(DEV_DISCONNCTED);
                if(ret)
                {
                    ble_gap_event_listener_unregister(&ble_mclient_event_listener);
                    g_ble_client_switching_off = false;
                    g_ble_client_inited = 0;
                }
                
            }
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            TLS_BT_APPL_TRACE_API("MC discovery complete; reason=%d\r\n",
                                  event->disc_complete.reason);
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            /* Encryption has been enabled or disabled for this connection. */
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;
            TLS_BT_APPL_TRACE_API("MC encryption change event; status=%d \r\n",
                        event->enc_change.status);            
            return 0;

        case BLE_GAP_EVENT_NOTIFY_RX:
            /* Peer sent us a notification or indication. */
            rc = ble_gap_conn_find(event->notify_rx.conn_handle, &desc);
            assert(rc == 0);
            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;  
            TLS_BT_APPL_TRACE_VERBOSE("MC received %s; conn_handle=%d attr_handle=%d "
                                    "attr_len=%d\n",
                                    event->notify_rx.indication ?
                                    "indication" :
                                    "notification",
                                    event->notify_rx.conn_handle,
                                    event->notify_rx.attr_handle,
                                    OS_MBUF_PKTLEN(event->notify_rx.om));                      
            /* Attribute data is contained in event->notify_rx.attr_data. */
            wm_ble_update_notify_counter(event->notify_rx.conn_handle);
            return 0;

        case BLE_GAP_EVENT_MTU:
            TLS_BT_APPL_TRACE_API("MC mtu update event; conn_handle=%d cid=%d mtu=%d\r\n",
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
       case BLE_GAP_EVENT_CONN_UPDATE_REQ:
       case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
            
            rc = ble_gap_conn_find(event->conn_update_req.conn_handle, &desc);
            assert(rc == 0);

            if(desc.role != BLE_GAP_ROLE_MASTER) return 0;

            TLS_BT_APPL_TRACE_WARNING("MultiClient Decline:%d[handle=%d]\r\n", event->type,event->conn_update_req.conn_handle);            
            return 1;
        case BLE_GAP_EVENT_HOST_SHUTDOWN:
            TLS_BT_APPL_TRACE_API("MC BLE_GAP_EVENT_HOST_SHUTDOWN\r\n");
            g_ble_client_switching_off = false;
            g_ble_client_inited = 0;
            return 0;
        default:
            TLS_BT_APPL_TRACE_VERBOSE("MultiClient Unhandled event:%d\r\n", event->type);
            return 0;
    }
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

int tls_ble_client_multi_conn_demo_api_init()
{
    int rc = 0;
    int i = 0;

    CHECK_SYSTEM_READY();

    if(g_ble_client_inited) { return BLE_HS_EALREADY; }

    rc = peer_init(MAX_CONN_DEVCIE_COUNT, 64, 64, 64);

    if(rc == 0) {
        ble_gap_event_listener_register(&ble_mclient_event_listener,
                                    ble_gap_evt_cb, NULL);
        TLS_BT_APPL_TRACE_DEBUG("### %s success\r\n", __FUNCTION__);
        memset(&conn_devices, 0, sizeof(connect_device_t)*MAX_CONN_DEVCIE_COUNT);

        for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
            conn_devices[i].conn_state = DEV_DISCONNCTED;
            conn_devices[i].conn_handle = 0xFFFF;
        }

        rc = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);

        if(rc == 0) {
            g_ble_client_inited = 1;
        }
    } else {
        g_ble_client_inited = 0;
        TLS_BT_APPL_TRACE_ERROR("### %s failed\r\n", __FUNCTION__);
    }

    g_ble_client_switching_off = false;
    return rc;
}

int tls_ble_client_multi_conn_demo_api_deinit()
{
    int rc = 0, i = 0;
    bool ret = false;
    TLS_BT_APPL_TRACE_DEBUG("### %s \r\n", __FUNCTION__);

    CHECK_SYSTEM_READY();

    if(g_ble_client_inited) {
        ret = wm_ble_check_all_connect_state(DEV_CONNECTED);

        if(!ret) {
            tls_ble_gap_scan(WM_BLE_SCAN_STOP, 0);
        }

        g_ble_client_switching_off = true;//indicating the demo is switching off;

        for(i = 0; i < MAX_CONN_DEVCIE_COUNT; i++) {
            if(conn_devices[i].conn_state == DEV_CONNECTED || conn_devices[i].conn_state == DEV_CONNECTING) {
                ble_gap_terminate(conn_devices[i].conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            } 
        }

        g_ble_client_inited = 0;
    } else {
        rc = BLE_HS_EALREADY;
    }

    return rc;
}
#endif

