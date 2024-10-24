/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "syscfg/syscfg.h"
#define MESH_LOG_MODULE BLE_MESH_ADV_LOG

#include "mesh/mesh.h"
#include "host/ble_hs_adv.h"
#include "host/ble_gap.h"
#include "nimble/hci_common.h"
#include "mesh/porting.h"

#include "adv.h"
#include "net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"


#define BT_MESH_EXTENDED_DEBUG 0

/* Convert from ms to 0.625ms units */
#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL_MS 30
#define MESH_SCAN_WINDOW_MS   30
#define MESH_SCAN_INTERVAL    ADV_SCAN_UNIT(MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      ADV_SCAN_UNIT(MESH_SCAN_WINDOW_MS)

/* Pre-5.0 controllers enforce a minimum interval of 100ms
 * whereas 5.0+ controllers can go down to 20ms.
 */
#define ADV_INT_DEFAULT_MS 100
#define ADV_INT_FAST_MS    20

#define WM_ADV_FAST 1

#if WM_ADV_FAST
/**NOTE: poor controller effiency, we should give more chance to scan mode to improve receiving performance */
static s32_t adv_int_min =  ADV_INT_FAST_MS;
#else
static s32_t adv_int_min =  ADV_INT_DEFAULT_MS;
#endif

/* TinyCrypt PRNG consumes a lot of stack space, so we need to have
 * an increased call stack whenever it's used.
 */
#if MYNEWT
#define ADV_STACK_SIZE 768
OS_TASK_STACK_DEFINE(g_blemesh_stack, ADV_STACK_SIZE);
struct os_task adv_task;
#else
#define ADV_STACK_SIZE 368
static tls_os_task_t tls_mesh_adv_task_hdl = NULL;
static void *tls_mesh_adv_task_stack_ptr = NULL;
#endif

static struct ble_npl_eventq adv_queue;
extern u8_t g_mesh_addr_type;
static int adv_initialized = false;

static os_membuf_t adv_buf_mem[OS_MEMPOOL_SIZE(
        MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT),
        BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE)];

struct os_mbuf_pool adv_os_mbuf_pool;
static struct os_mempool adv_buf_mempool;

static const u8_t adv_type[] = {
    [BT_MESH_ADV_PROV]   = BLE_HS_ADV_TYPE_MESH_PROV,
    [BT_MESH_ADV_DATA]   = BLE_HS_ADV_TYPE_MESH_MESSAGE,
    [BT_MESH_ADV_BEACON] = BLE_HS_ADV_TYPE_MESH_BEACON,
    [BT_MESH_ADV_URI]    = BLE_HS_ADV_TYPE_URI,
};


static struct bt_mesh_adv adv_pool[CONFIG_BT_MESH_ADV_BUF_COUNT+1];

static struct bt_mesh_adv *adv_alloc(int id)
{
    return &adv_pool[id];
}

static inline void adv_send_start(u16_t duration, int err,
                                  const struct bt_mesh_send_cb *cb,
                                  void *cb_data)
{
    if(cb && cb->start) {
        cb->start(duration, err, cb_data);
    }
}

static inline void adv_send_end(int err, const struct bt_mesh_send_cb *cb,
                                void *cb_data)
{
    if(cb && cb->end) {
        cb->end(err, cb_data);
    }
}

static inline void adv_send(struct os_mbuf *buf, bool adv_send_once)
{
    const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
    void *cb_data = BT_MESH_ADV(buf)->cb_data;
    struct ble_gap_adv_params param = { 0 };
    u16_t duration, adv_int;
    struct bt_data ad;
    int err;
    adv_int = max(adv_int_min,
                  BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
//#if MYNEWT_VAL(BLE_CONTROLLER)
#if WM_ADV_FAST
    //increase 60ms advertise time ; 60/20=3;
    if(adv_send_once)
    {
    duration = ((adv_int + 10)*2);
    }else
    {
    duration = (MESH_SCAN_WINDOW_MS*2 + ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
                (adv_int + 10)));        
    }
#else
    duration = (MESH_SCAN_WINDOW_MS +
                ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) *
                 (adv_int + 10)));

#endif
    BT_DBG("type %u om_len %u: %s", BT_MESH_ADV(buf)->type,
           buf->om_len, bt_hex(buf->om_data, buf->om_len));
    BT_DBG("count %u interval %ums duration %ums",
           BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1, adv_int,
           duration);
    ad.type = adv_type[BT_MESH_ADV(buf)->type];
    ad.data_len = buf->om_len;
    ad.data = buf->om_data;
    param.itvl_min = ADV_SCAN_UNIT(adv_int);
    param.itvl_max = param.itvl_min;
    param.conn_mode = BLE_GAP_CONN_MODE_NON;
    err = bt_le_adv_start(&param, &ad, 1, NULL, 0);
    net_buf_unref(buf);
    adv_send_start(duration, err, cb, cb_data);

    if(err) {
        BT_ERR("Advertising failed: err %d", err);
        return;
    }

    BT_DBG("Advertising started. Sleeping %u ms", duration);
    k_sleep(K_MSEC(duration));
    err = bt_le_adv_stop(false);
    adv_send_end(err, cb, cb_data);

    if(err) {
        BT_ERR("Stopping advertising failed: err %d", err);
        return;
    }

    BT_DBG("Advertising stopped");
}

void
mesh_adv_thread(void *args)
{
    static struct ble_npl_event *ev;
    struct os_mbuf *buf;
    uint8_t space_available = 0;
    uint8_t clear_count = 0;
    bool adv_send_once = false;
    
#if (MYNEWT_VAL(BLE_MESH_PROXY))
    s32_t timeout;
#endif
    BT_DBG("started");

    while(1) {
#if (MYNEWT_VAL(BLE_MESH_PROXY))

#if 0
        bool ev_empty = ble_npl_eventq_is_empty(&adv_queue);
        if(ev_empty)
        {
            ev = NULL;
        }else
        {
            ev = ble_npl_eventq_get(&adv_queue, 1);
        }
#else
        space_available = ble_npl_eventq_space_available(&adv_queue);
        if(space_available == 128)
        {
            ev = NULL;
            adv_send_once = false;
        }else
        {
            BT_WARN("[%d]ADV QUEUE AVAILABLE %d\r\n", tls_os_get_time(), space_available);
            if(space_available < 16)
            {
                /**Flush all pending queue*/
                clear_count = 128 - space_available;
                while(clear_count){
                    ev = ble_npl_eventq_get(&adv_queue, 1);
                    if(!ev || !ble_npl_event_get_arg(ev)) {
                        break;
                    }
                    buf = ble_npl_event_get_arg(ev);
                    net_buf_unref(buf);
                    clear_count--;
                }
                
            }else if(space_available < 64)
            {
                adv_send_once = true;
            }else
            {
                adv_send_once = false;
            }
            ev = ble_npl_eventq_get(&adv_queue, 1);
        }
#endif
        while(!ev) {
            timeout = bt_mesh_proxy_adv_start();
            BT_DBG("Proxy Advertising up to %d ms", (int) timeout);

            // FIXME: should we redefine K_SECONDS macro instead in glue?
            if(timeout != K_FOREVER) {
                timeout = ble_npl_time_ms_to_ticks32(timeout);
            }

            ev = ble_npl_eventq_get(&adv_queue, timeout);
            bt_mesh_proxy_adv_stop();
        }

#else
        ev = ble_npl_eventq_get(&adv_queue, BLE_NPL_TIME_FOREVER);
#endif

        if(!ev || !ble_npl_event_get_arg(ev)) {
            continue;
        }

        buf = ble_npl_event_get_arg(ev);

        /* busy == 0 means this was canceled */
        if(BT_MESH_ADV(buf)->busy) {
            BT_MESH_ADV(buf)->busy = 0;
            adv_send(buf,adv_send_once);
        } else {
            net_buf_unref(buf);
        }

        /**ADV END ->>DELAY(5ms)->>ADV START*/
        k_sleep(K_MSEC(10));

        /* os_sched(NULL); */
    }
}

void bt_mesh_adv_update(void)
{
    static struct ble_npl_event ev = { };
    BT_DBG("");
    ble_npl_eventq_put(&adv_queue, &ev);
}

struct os_mbuf *bt_mesh_adv_create_from_pool(struct os_mbuf_pool *pool,
        bt_mesh_adv_alloc_t get_id,
        enum bt_mesh_adv_type type,
        u8_t xmit, s32_t timeout)
{
    struct bt_mesh_adv *adv;
    struct os_mbuf *buf;

    if(atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
        BT_WARN("Refusing to allocate buffer while suspended");
        return NULL;
    }

    buf = os_mbuf_get_pkthdr(pool, BT_MESH_ADV_USER_DATA_SIZE);

    if(!buf) {
        BT_ERR("No free adv buffer available\r\n");
        return NULL;
    }
    if(net_buf_id(buf) >= CONFIG_BT_MESH_ADV_BUF_COUNT)
    {
        BT_ERR("ADV buffer corrupted %d\r\n",net_buf_id(buf));
        os_mbuf_free_chain(buf);
        return NULL;
        
    }
    adv = get_id(net_buf_id(buf));
    BT_MESH_ADV(buf) = adv;
    memset(adv, 0, sizeof(*adv));
    adv->type         = type;
    adv->xmit         = xmit;
    adv->ref_cnt = 1;
    ble_npl_event_set_arg(&adv->ev, buf);
    return buf;
}

uint8_t bt_mesh_adv_avaiable_for_relay(void)
{
    uint8_t ret = 0;
    int t_total, t_free, t_thresh;

    t_total = os_mbuf_num_total(&adv_os_mbuf_pool);
    t_free = os_mbuf_num_free(&adv_os_mbuf_pool);
    t_thresh = CONFIG_BT_MESH_ADV_BUF_COUNT*1/4;
    BT_INFO("Total=%d, free=%d, thresh=%d\r\n", t_total,t_free, t_thresh );

    if(t_free >= t_thresh) ret = 1;

    return ret;
}
struct os_mbuf *bt_mesh_adv_create(enum bt_mesh_adv_type type, u8_t xmit,
                                   s32_t timeout)
{
    return bt_mesh_adv_create_from_pool(&adv_os_mbuf_pool, adv_alloc, type,
                                        xmit, timeout);
}
/**Default mesh msg except for relay, we process it with as soon as possible*/
void bt_mesh_adv_send(struct os_mbuf *buf, const struct bt_mesh_send_cb *cb,
                      void *cb_data)
{
    BT_DBG("buf %p, type 0x%02x len %u: %s", buf, BT_MESH_ADV(buf)->type, buf->om_len,
           bt_hex(buf->om_data, buf->om_len));
    BT_MESH_ADV(buf)->cb = cb;
    BT_MESH_ADV(buf)->cb_data = cb_data;
    BT_MESH_ADV(buf)->busy = 1;
    net_buf_put_to_front(&adv_queue, net_buf_ref(buf));
}
/**Relay mesh msg processing*/
void bt_mesh_adv_send_to_tail(struct os_mbuf *buf, const struct bt_mesh_send_cb *cb,
                    void *cb_data)
{
  BT_DBG("buf %p, type 0x%02x len %u: %s", buf, BT_MESH_ADV(buf)->type, buf->om_len,
         bt_hex(buf->om_data, buf->om_len));
  BT_MESH_ADV(buf)->cb = cb;
  BT_MESH_ADV(buf)->cb_data = cb_data;
  BT_MESH_ADV(buf)->busy = 1;
  net_buf_put(&adv_queue, net_buf_ref(buf));
}

static void bt_mesh_scan_cb(const bt_addr_le_t *addr, s8_t rssi,
                            u8_t adv_type, struct os_mbuf *buf)
{
    if(adv_type != BLE_HCI_ADV_TYPE_ADV_NONCONN_IND) {
        return;
    }

#if BT_MESH_EXTENDED_DEBUG
    BT_DBG("len %u: %s", buf->om_len, bt_hex(buf->om_data, buf->om_len));
#endif

    while(buf->om_len > 1) {
        struct net_buf_simple_state state;
        u8_t len, type;
        len = net_buf_simple_pull_u8(buf);

        /* Check for early termination */
        if(len == 0) {
            return;
        }

        if(len > buf->om_len) {
            BT_WARN("AD malformed[%d][%d]\r\n", len, buf->om_len);
            return;
        }

        net_buf_simple_save(buf, &state);
        type = net_buf_simple_pull_u8(buf);

        switch(type) {
            case BLE_HS_ADV_TYPE_MESH_MESSAGE:
                bt_mesh_net_recv(buf, rssi, BT_MESH_NET_IF_ADV);
                break;
#if MYNEWT_VAL(BLE_MESH_PB_ADV)

            case BLE_HS_ADV_TYPE_MESH_PROV:
                bt_mesh_pb_adv_recv(buf);
                break;
#endif

            case BLE_HS_ADV_TYPE_MESH_BEACON:
                bt_mesh_beacon_recv(addr, buf);
                break;

            default:
                break;
        }

        net_buf_simple_restore(buf, &state);
        net_buf_simple_pull(buf, len);
    }
}

void bt_mesh_adv_init(void)
{
    int rc;

    /* Advertising should only be initialized once. Calling
     * os_task init the second time will result in an assert. */
    if(adv_initialized) {
        return;
    }

    rc = os_mempool_init(&adv_buf_mempool, MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT),
                         BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE+1,
                         adv_buf_mem, "adv_buf_pool");
    assert(rc == 0);
    rc = os_mbuf_pool_init(&adv_os_mbuf_pool, &adv_buf_mempool,
                           BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE+1,
                           MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT));
    assert(rc == 0);
    ble_npl_eventq_init(&adv_queue);
#if MYNEWT
    os_task_init(&adv_task, "mesh_adv", mesh_adv_thread, NULL,
                 MYNEWT_VAL(BLE_MESH_ADV_TASK_PRIO), OS_WAIT_FOREVER,
                 g_blemesh_stack, ADV_STACK_SIZE);
#else
    tls_mesh_adv_task_stack_ptr = (void *)tls_mem_alloc(ADV_STACK_SIZE * sizeof(u32_t));
    assert(tls_mesh_adv_task_stack_ptr != NULL);
    tls_os_task_create(&tls_mesh_adv_task_hdl, "bma",
                       mesh_adv_thread,
                       (void *)0,
                       (void *)tls_mesh_adv_task_stack_ptr,
                       ADV_STACK_SIZE * sizeof(u32_t),
                       MYNEWT_VAL(BLE_MESH_ADV_TASK_PRIO),
                       0);
#endif

    /* For BT5 controllers we can have fast advertising interval */
    if(ble_hs_hci_get_hci_version() >= BLE_HCI_VER_BCS_5_0) {
        adv_int_min = ADV_INT_FAST_MS;
    }

    adv_initialized = true;
}

void bt_mesh_adv_deinit(void)
{
    static struct ble_npl_event *ev;
    struct os_mbuf *buf;
    int rc;

    /* Advertising should only be initialized once. Calling
     * os_task init the second time will result in an assert. */
    if(!adv_initialized) {
        return;
    }

#if 1
    rc = os_mempool_init(&adv_buf_mempool, MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT),
                         BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE +1,
                         adv_buf_mem, "adv_buf_pool");
    assert(rc == 0);
    rc = os_mbuf_pool_init(&adv_os_mbuf_pool, &adv_buf_mempool,
                           BT_MESH_ADV_DATA_SIZE + BT_MESH_MBUF_HEADER_SIZE +1,
                           MYNEWT_VAL(BLE_MESH_ADV_BUF_COUNT));
    assert(rc == 0);   
#endif
#if MYNEWT
    os_task_deinit(&adv_task, "mesh_adv", mesh_adv_thread, NULL,
                   MYNEWT_VAL(BLE_MESH_ADV_TASK_PRIO), OS_WAIT_FOREVER,
                   g_blemesh_stack, ADV_STACK_SIZE);
#else

    if(tls_mesh_adv_task_hdl) {
        tls_os_task_del_by_task_handle(tls_mesh_adv_task_hdl, NULL);
    }

    tls_mesh_adv_task_hdl = NULL;

    if(tls_mesh_adv_task_stack_ptr) { tls_mem_free(tls_mesh_adv_task_stack_ptr); }

    tls_mesh_adv_task_stack_ptr = NULL;

    /**Flush unhandled msg*/
    while(1) {
        ev = ble_npl_eventq_get(&adv_queue, 1);

        if(ev) {
            buf = ble_npl_event_get_arg(ev);
            net_buf_unref(buf);
        } else {
            break;
        }
    }

    bt_le_adv_stop(true);
#endif

    /* For BT5 controllers we can have fast advertising interval */
    if(ble_hs_hci_get_hci_version() >= BLE_HCI_VER_BCS_5_0) {
        adv_int_min = ADV_INT_FAST_MS;
    }

    ble_npl_eventq_deinit(&adv_queue);
    adv_initialized = false;
}


int
ble_adv_gap_mesh_cb(struct ble_gap_event *event, void *arg)
{
#if MYNEWT_VAL(BLE_EXT_ADV)
    struct ble_gap_ext_disc_desc *ext_desc;
#endif
    struct ble_gap_disc_desc *desc;
    struct os_mbuf *buf = NULL;
#if BT_MESH_EXTENDED_DEBUG
    BT_DBG("event->type %d", event->type);
#endif

    switch(event->type) {
#if MYNEWT_VAL(BLE_EXT_ADV)

        case BLE_GAP_EVENT_EXT_DISC:
            ext_desc = &event->ext_disc;
            buf = os_mbuf_get_pkthdr(&adv_os_mbuf_pool, 0);

            if(!buf || os_mbuf_append(buf, ext_desc->data, ext_desc->length_data)) {
                BT_ERR("Could not append data");
                goto done;
            }

            bt_mesh_scan_cb(&ext_desc->addr, ext_desc->rssi,
                            ext_desc->legacy_event_type, buf);
            break;
#endif

        case BLE_GAP_EVENT_DISC:
            desc = &event->disc;
            buf = os_mbuf_get_pkthdr(&adv_os_mbuf_pool, 0);

            if(!buf || os_mbuf_append(buf, desc->data, desc->length_data)) {
                BT_ERR("Could not append data");
                goto done;
            }

            bt_mesh_scan_cb(&desc->addr, desc->rssi, desc->event_type, buf);
            break;

        default:
            break;
    }

done:

    if(buf) {
        os_mbuf_free_chain(buf);
    }

    return 0;
}

int bt_mesh_scan_enable(void)
{
    int err;
#if MYNEWT_VAL(BLE_EXT_ADV)
    struct ble_gap_ext_disc_params uncoded_params = {
        .itvl = MESH_SCAN_INTERVAL, .window = MESH_SCAN_WINDOW,
        .passive = 1
    };
    BT_DBG("");
    err =  ble_gap_ext_disc(g_mesh_addr_type, 0, 0, 0, 0, 0,
                            &uncoded_params, NULL, NULL, NULL);
#else
    struct ble_gap_disc_params scan_param = {
        .passive = 1, .filter_duplicates = 0, .itvl =
        MESH_SCAN_INTERVAL, .window = MESH_SCAN_WINDOW
    };
    BT_DBG("");
    err =  ble_gap_disc(g_mesh_addr_type, BLE_HS_FOREVER, &scan_param,
                        NULL, NULL);
#endif

    if(err && err != BLE_HS_EALREADY) {
        BT_ERR("starting scan failed (err %d)", err);
        return err;
    }

    return 0;
}

int bt_mesh_scan_disable(void)
{
    int err;
    BT_DBG("");
    err = ble_gap_disc_cancel();

    if(err && err != BLE_HS_EALREADY) {
        BT_ERR("stopping scan failed (err %d)", err);
        return err;
    }

    return 0;
}
