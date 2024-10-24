/*
* Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include "sysinit/sysinit.h"
#include "nimble/hci_common.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/ble_hci_trans.h"
#include "wm_bt.h"
#include "wm_mem.h"

#define BLE_HCI_UART_H4_NONE        0x00
#define BLE_HCI_UART_H4_CMD         0x01
#define BLE_HCI_UART_H4_ACL         0x02
#define BLE_HCI_UART_H4_SCO         0x03
#define BLE_HCI_UART_H4_EVT         0x04
#define BLE_HCI_UART_H4_SYNC_LOSS   0x80
#define BLE_HCI_UART_H4_SKIP_CMD    0x81
#define BLE_HCI_UART_H4_SKIP_ACL    0x82
#define BLE_HCI_UART_H4_LE_EVT      0x83
#define BLE_HCI_UART_H4_SKIP_EVT    0x84

#define BLE_HCI_EVENT_HDR_LEN               (2)
#define BLE_HCI_CMD_HDR_LEN                 (3)

#define HCI_VUART_RX_QUEUE MYNEWT_VAL(HCI_VUART_RX_QUEUE_ENABLE)


static volatile int ble_hci_cmd_pkts = 0;
static ble_hci_trans_rx_cmd_fn *ble_hci_rx_cmd_hs_cb;
static void *ble_hci_rx_cmd_hs_arg;
static ble_hci_trans_rx_acl_fn *ble_hci_rx_acl_hs_cb;
static void *ble_hci_rx_acl_hs_arg;


static struct os_mempool ble_hci_vuart_evt_hi_pool;

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
static os_membuf_t *ble_hci_vuart_evt_hi_buf = NULL;

#else
static os_membuf_t ble_hci_vuart_evt_hi_buf[
                 OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
                                 MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))
 ];
#endif


static struct os_mempool ble_hci_vuart_evt_lo_pool;

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
static os_membuf_t *ble_hci_vuart_evt_lo_buf = NULL;
#else
static os_membuf_t ble_hci_vuart_evt_lo_buf[
                 OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
                                 MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE))
 ];
#endif


#if HCI_VUART_RX_QUEUE
static tls_os_task_t task_vuart_rx_hdl = NULL;

static struct os_mempool ble_hci_vuart_queue_buf_pool;

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
static os_membuf_t *ble_hci_vuart_queue_buf = NULL;
#else
static os_membuf_t ble_hci_vuart_queue_buf[
                 OS_MEMPOOL_SIZE(MYNEWT_VAL(HCI_VUART_QUEUE_BUF_COUNT),
                                 MYNEWT_VAL(HCI_VUART_QUEUE_BUF_SIZE))
 ];
#endif
#endif



static struct os_mempool ble_hci_vuart_cmd_pool;

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
static os_membuf_t *ble_hci_vuart_cmd_buf = NULL;
#else
static os_membuf_t ble_hci_vuart_cmd_buf[
                 OS_MEMPOOL_SIZE(1, BLE_HCI_TRANS_CMD_SZ)
 ];
#endif

static struct os_mbuf_pool ble_hci_vuart_acl_mbuf_pool;
static struct os_mempool_ext ble_hci_vuart_acl_pool;

/*
 * The MBUF payload size must accommodate the HCI data header size plus the
 * maximum ACL data packet length. The ACL block size is the size of the
 * mbufs we will allocate.
 */
#define ACL_BLOCK_SIZE  OS_ALIGN(MYNEWT_VAL(BLE_ACL_BUF_SIZE) \
                                 + BLE_MBUF_MEMBLOCK_OVERHEAD \
                                 + BLE_HCI_DATA_HDR_SZ, OS_ALIGNMENT)

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
static os_membuf_t *ble_hci_vuart_acl_buf = NULL;
#else
static os_membuf_t ble_hci_vuart_acl_buf[
                 OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                                 ACL_BLOCK_SIZE)
 ];
#endif



#if HCI_VUART_RX_QUEUE

#define HCI_RESP_QUEUE_FULL      (20)
#define HCI_RESP_QUEUE_HALF_FULL (12)

struct QUEUE_ITEM {
    int size;
    uint8_t payload[256];
    TAILQ_ENTRY(QUEUE_ITEM) entries;
};

static TAILQ_HEAD(, QUEUE_ITEM) hci_resp_queue;
static volatile uint8_t hci_resp_queue_counter = 0;

static struct ble_npl_mutex hci_resp_queue_mutex;
static struct ble_npl_event ble_hs_ev_hci_rx;
static struct ble_npl_eventq hci_evt_rx_tx_queue;

#endif



#define HCI_DBG FALSE


#ifndef HCI_DBG
#define HCI_DBG TRUE
#endif

#if (HCI_DBG == TRUE)
#define HCIDBG(fmt, ...)  \
    do{\
        if(1) \
            printf("%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)
#else
#define HCIDBG(param, ...)
#endif


#if (HCI_DBG == TRUE)
static void  hci_dbg_hexstring(const char *msg, const uint8_t *ptr, int a_length)
{
#define DBG_TRACE_WARNING_MAX_SIZE_W  252
    //123
#define DBG_TRACE_WARNING_MAX_SIZE    256
    //128
    char sbuffer[DBG_TRACE_WARNING_MAX_SIZE];
    uint8_t offset = 0;
    int i = 0;
    int length = 0;

    if(msg) {
        printf("[%d]%s", tls_os_get_time(), msg);
    }

    if(a_length <= 0 || ptr == NULL) {
        return;
    }

    //length = MIN(40, a_length);
    length = a_length;

    do {
        for(; i < length; i++) {
            offset += sprintf(sbuffer + offset, "%02X ", (uint8_t)ptr[i]);

            if(offset > DBG_TRACE_WARNING_MAX_SIZE_W) {
                break;
            }
        }

        sbuffer[offset - 1] = '\r';
        sbuffer[offset] = '\n';
        sbuffer[offset + 1] = 0;

        if(offset > DBG_TRACE_WARNING_MAX_SIZE_W) {
            sbuffer[offset - 2] = '.';
            sbuffer[offset - 3] = '.';
        }

        printf("%s", sbuffer);
        offset = 0;
    } while(i < length);
}
#endif


void ble_hci_trans_cfg_hs(ble_hci_trans_rx_cmd_fn *cmd_cb,
                          void *cmd_arg,
                          ble_hci_trans_rx_acl_fn *acl_cb,
                          void *acl_arg)
{
    ble_hci_rx_cmd_hs_cb = cmd_cb;
    ble_hci_rx_cmd_hs_arg = cmd_arg;
    ble_hci_rx_acl_hs_cb = acl_cb;
    ble_hci_rx_acl_hs_arg = acl_arg;
}
int ble_hci_trans_hs_cmd_tx(uint8_t *cmd)
{
    uint16_t len;
    assert(cmd != NULL);
    *cmd = BLE_HCI_UART_H4_CMD;
    len = BLE_HCI_CMD_HDR_LEN + cmd[3] + 1;

    while(ble_hci_cmd_pkts <= 0) {
        tls_os_time_delay(1000 / /*configTICK_RATE_HZ*/HZ);
    }

    //hci_dbg_hexstring(">>>[CMD]", cmd, len);
    tls_bt_vuart_host_send_packet(cmd, len);
    ble_hci_trans_buf_free(cmd);
    return 0;
}

int ble_hci_trans_ll_evt_tx(uint8_t *hci_ev)
{
    int rc = -1;

    if(ble_hci_rx_cmd_hs_cb) {
        rc = ble_hci_rx_cmd_hs_cb(hci_ev, ble_hci_rx_cmd_hs_arg);
    }

    return rc;
}

int ble_hci_trans_hs_acl_tx(struct os_mbuf *om)
{
    uint16_t len = 0;
    uint8_t data[MYNEWT_VAL(BLE_ACL_BUF_SIZE) + 1];

    /* If this packet is zero length, just free it */
    if(OS_MBUF_PKTLEN(om) == 0) {
        os_mbuf_free_chain(om);
        return 0;
    }

    data[0] = BLE_HCI_UART_H4_ACL;
    len++;

    while(ble_hci_cmd_pkts <= 0) {
        tls_os_time_delay(1000 / /*configTICK_RATE_HZ*/HZ);
    }

    os_mbuf_copydata(om, 0, OS_MBUF_PKTLEN(om), &data[1]);
    len += OS_MBUF_PKTLEN(om);
    tls_bt_vuart_host_send_packet(data, len);
    //hci_dbg_hexstring(">>>[ACL]", data, len);
    os_mbuf_free_chain(om);
    return 0;
}


uint8_t *ble_hci_trans_buf_alloc(int type)
{
    uint8_t *buf;

    switch(type) {
        case BLE_HCI_TRANS_BUF_CMD:
            buf = os_memblock_get(&ble_hci_vuart_cmd_pool);
            break;

        case BLE_HCI_TRANS_BUF_EVT_HI:
            buf = os_memblock_get(&ble_hci_vuart_evt_hi_pool);

            if(buf == NULL) {
                /* If no high-priority event buffers remain, try to grab a
                 * low-priority one.
                 */
                buf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_LO);
            }

            break;

        case BLE_HCI_TRANS_BUF_EVT_LO:
            buf = os_memblock_get(&ble_hci_vuart_evt_lo_pool);
            break;
#if HCI_VUART_RX_QUEUE        
        case BLE_HCI_TRANS_QUEUE_EVT:
            buf = os_memblock_get(&ble_hci_vuart_queue_buf_pool);
            break;
#endif        
        default:
            assert(0);
            buf = NULL;
    }

    return buf;
}

void ble_hci_trans_buf_free(uint8_t *buf)
{
    int rc;

    /*
     * XXX: this may look a bit odd, but the controller uses the command
     * buffer to send back the command complete/status as an immediate
     * response to the command. This was done to insure that the controller
     * could always send back one of these events when a command was received.
     * Thus, we check to see which pool the buffer came from so we can free
     * it to the appropriate pool
     */
    if(os_memblock_from(&ble_hci_vuart_evt_hi_pool, buf)) {
        rc = os_memblock_put(&ble_hci_vuart_evt_hi_pool, buf);
        assert(rc == 0);
    } else if(os_memblock_from(&ble_hci_vuart_evt_lo_pool, buf)) {
        rc = os_memblock_put(&ble_hci_vuart_evt_lo_pool, buf);
        assert(rc == 0);
    }
#if HCI_VUART_RX_QUEUE    
    else if(os_memblock_from(&ble_hci_vuart_queue_buf_pool, buf)) {
        rc = os_memblock_put(&ble_hci_vuart_queue_buf_pool, buf);
        assert(rc == 0);
    }
#endif    
    else {
        assert(os_memblock_from(&ble_hci_vuart_cmd_pool, buf));
        rc = os_memblock_put(&ble_hci_vuart_cmd_pool, buf);
        assert(rc == 0);
    }
}

/**
 * Unsupported; the RAM transport does not have a dedicated ACL data packet
 * pool.
 */
int ble_hci_trans_set_acl_free_cb(os_mempool_put_fn *cb, void *arg)
{
    return BLE_ERR_UNSUPPORTED;
}

int ble_hci_trans_reset(void)
{
    /* No work to do.  All allocated buffers are owned by the host or
     * controller, and they will get freed by their owners.
     */
    return 0;
}

/**
 * Allocates a buffer (mbuf) for ACL operation.
 *
 * @return                      The allocated buffer on success;
 *                              NULL on buffer exhaustion.
 */
static struct os_mbuf *ble_hci_trans_acl_buf_alloc(void)
{
    struct os_mbuf *m;
    uint8_t usrhdr_len;
#if MYNEWT_VAL(BLE_DEVICE)
    usrhdr_len = sizeof(struct ble_mbuf_hdr);
#elif MYNEWT_VAL(BLE_HS_FLOW_CTRL)
    usrhdr_len = BLE_MBUF_HS_HDR_LEN;
#else
    usrhdr_len = 0;
#endif
    m = os_mbuf_get_pkthdr(&ble_hci_vuart_acl_mbuf_pool, usrhdr_len);
    return m;
}

static void ble_hci_rx_acl(uint8_t *data, uint16_t len)
{
    struct os_mbuf *m;
    int sr;

    if(len < BLE_HCI_DATA_HDR_SZ || len > MYNEWT_VAL(BLE_ACL_BUF_SIZE)) {
        assert(0);
        return;
    }

    m = ble_hci_trans_acl_buf_alloc();

    if(!m) {
        assert(0);
        return;
    }

    if(os_mbuf_append(m, data, len)) {
        assert(0);
        os_mbuf_free_chain(m);
        return;
    }

    OS_ENTER_CRITICAL(sr);

    if(ble_hci_rx_acl_hs_cb) {
        ble_hci_rx_acl_hs_cb(m, NULL);
    }

    OS_EXIT_CRITICAL(sr);
}

void notify_host_send_available(int cnt)
{
    ble_hci_cmd_pkts = cnt;
}

static int ble_hci_trans_hs_rx(uint8_t *data, uint16_t len)
{
    int rc = 0;
    
    if(data[0] == BLE_HCI_UART_H4_EVT) {
        uint8_t *evbuf;
        int totlen;

        totlen = BLE_HCI_EVENT_HDR_LEN + data[2];
        assert(totlen <= UINT8_MAX + BLE_HCI_EVENT_HDR_LEN);

        if(data[1] == BLE_HCI_EVCODE_HW_ERROR) {
            assert(0);
        }

        /* Allocate LE Advertising Report Event from lo pool only */
        if((data[1] == BLE_HCI_EVCODE_LE_META) && (data[3] == BLE_HCI_LE_SUBEV_ADV_RPT)) {
            evbuf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_LO);

            /* Skip advertising report if we're out of memory */
            if(!evbuf) {
                BLE_HS_LOG(WARN, "alloc BLE_HCI_TRANS_BUF_EVT_LO buffer failed\r\n");
                return BLE_HS_ENOMEM;
            }
        } else {
            evbuf = ble_hci_trans_buf_alloc(BLE_HCI_TRANS_BUF_EVT_HI);
            if(!evbuf) {
                BLE_HS_LOG(WARN, "alloc BLE_HCI_TRANS_BUF_EVT_HI buffer failed\r\n");
                return BLE_HS_ENOMEM;
            }

        }
        assert(totlen <= MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE));        
        memcpy(evbuf, &data[1], totlen);
        rc = ble_hci_trans_ll_evt_tx(evbuf);
    } else if(data[0] == BLE_HCI_UART_H4_ACL) {
        ble_hci_rx_acl(data + 1, len - 1);
    }

    return rc;
}

#if HCI_VUART_RX_QUEUE
static void ble_hs_event_hci_rx_func(struct ble_npl_event *ev)
{

#if 0
    struct QUEUE_ITEM *item = NULL;
    struct QUEUE_ITEM *item_next = NULL;
    ble_npl_mutex_pend(&hci_resp_queue_mutex, 0);
    item = TAILQ_FIRST(&hci_resp_queue);

    if(item) {
        TAILQ_REMOVE(&hci_resp_queue, item, entries);
        item_next = TAILQ_FIRST(&hci_resp_queue);
        hci_resp_queue_counter--;
    }

    ble_npl_mutex_release(&hci_resp_queue_mutex);

    if(item) {      
#if (HCI_DBG == TRUE)
        hci_dbg_hexstring("***", item->payload, item->size);
#endif
        ble_hci_trans_hs_rx(item->payload, item->size);
        ble_hci_trans_buf_free((uint8_t*)item);
    }

    /*More hci response available, notify the host statck for reading again*/

    if(item_next) {
        ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
    }
#else
    int rc;
    struct QUEUE_ITEM *item = NULL;
    struct QUEUE_ITEM *item_next = NULL;
    ble_npl_mutex_pend(&hci_resp_queue_mutex, 0);
    item = TAILQ_FIRST(&hci_resp_queue);
    ble_npl_mutex_release(&hci_resp_queue_mutex);

    if(item)
    {
        #if (HCI_DBG == TRUE)
        hci_dbg_hexstring("***", item->payload, item->size);
        #endif
        rc = ble_hci_trans_hs_rx(item->payload, item->size);
        if(rc == 0)
        {
            
            ble_npl_mutex_pend(&hci_resp_queue_mutex, 0);            
            TAILQ_REMOVE(&hci_resp_queue, item, entries);  
            item_next = TAILQ_FIRST(&hci_resp_queue);
            ble_npl_mutex_release(&hci_resp_queue_mutex);
            ble_hci_trans_buf_free((uint8_t*)item);
            hci_resp_queue_counter--;
            if(item_next) {
                //printf("ble_hci_trans_hs_rx. rc=%d, process more\r\n", rc);
                ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
            }            
            
        }else
        {
            /*try again to notify host to read*/
            //printf("ble_hci_trans_hs_rx. rc=%d\r\n", rc);
            //ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
        }
    }

#endif
}

#endif

#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04
#define BT_HCI_EVT_LE_META_EVENT                0x3e
#define BT_HCI_EVT_LE_ADVERTISING_REPORT        0x02

static void notify_host_recv(uint8_t *data, uint16_t len)
{
#if HCI_VUART_RX_QUEUE

    if(data == NULL || len == 0) { return; }

#if (HCI_DBG == TRUE)
    hci_dbg_hexstring("<<<", data, len);
    //HCIDBG("%s, pending_counter=%d\r\n", __func__, hci_resp_queue_counter);
#endif

    if(data[0] == HCI_EVENT_PKT && data[1] == BT_HCI_EVT_LE_META_EVENT
            && data[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
        if(hci_resp_queue_counter > HCI_RESP_QUEUE_HALF_FULL) {
            BLE_HS_LOG(WARN, "Too much hci_adv_report_evt, discard it\r\n");
            ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
            return;
        }
    }

    if(hci_resp_queue_counter > HCI_RESP_QUEUE_FULL) {
        BLE_HS_LOG(WARN, "Too much(%d) hci_response_evt, discard it\r\n", hci_resp_queue_counter);
        ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
        return;
    }

    if(len > 256)
    {
        BLE_HS_LOG(WARN, "HCI msg length[%d] illegal\r\n", len);
        return ;
    }
    struct QUEUE_ITEM *item = (struct QUEUE_ITEM *)ble_hci_trans_buf_alloc(BLE_HCI_TRANS_QUEUE_EVT);
     if(item == NULL) {
        BLE_HS_LOG(WARN, "Alloc queue item failed, no memory available\r\n");
        return;
    }  
    memcpy(item->payload, data, len);
    item->payload[len] = 0;
    item->size = len;
    ble_npl_mutex_pend(&hci_resp_queue_mutex, 0);
    TAILQ_INSERT_TAIL(&hci_resp_queue, item, entries);
    hci_resp_queue_counter++;
    ble_npl_mutex_release(&hci_resp_queue_mutex);
    ble_npl_eventq_put(&hci_evt_rx_tx_queue, &ble_hs_ev_hci_rx);
#else
    //hci_dbg_hexstring("<<<", data, len);
    if(len > 256)
    {
        BLE_HS_LOG(WARN, "HCI msg length[%d] illegal\r\n", len);
        return ;
    }
    ble_hci_trans_hs_rx(data, len);
#endif
    return ;
}

static tls_bt_host_if_t vuart_hci_cb = {
	.notify_controller_avaiable_hci_buffer = notify_host_send_available,
	.notify_host_recv_h4 = notify_host_recv,
};


static int wm_ble_controller_init(uint8_t uart_idx)
{
    tls_bt_hci_if_t hci_if;
    tls_bt_status_t status;

    status = tls_bt_ctrl_if_register(&vuart_hci_cb);

    if(status != TLS_BT_STATUS_SUCCESS) {
        return TLS_BT_STATUS_CTRL_ENABLE_FAILED;
    }

    
    hci_if.uart_index = uart_idx;
    status = tls_bt_ctrl_enable(&hci_if, 0);

    if((status != TLS_BT_STATUS_SUCCESS) && (status != TLS_BT_STATUS_DONE)) {
        return TLS_BT_STATUS_CTRL_ENABLE_FAILED;
    }



    return 0;
}
static int wm_ble_controller_deinit()
{
    return tls_bt_ctrl_disable();
}

/**
 * Initializes the VUART HCI transport module.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
#if HCI_VUART_RX_QUEUE

static void *tls_host_vuart_task_stack_ptr = NULL;
static void nimble_vhci_task(void *parg)
{
    struct ble_npl_event *ev;
    //int arg;
    (void)parg;

    while(1) {
        ev = ble_npl_eventq_get(&hci_evt_rx_tx_queue, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
        //arg = (int)ble_npl_event_get_arg(ev);
        //if (arg == HCI_EXIT_EV_ARG) {
        //    ;//break;
        //}
    }
}
#endif
int
ble_hci_vuart_init(uint8_t uart_idx)
{
    int rc;
    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    ble_hci_vuart_acl_buf = (os_membuf_t *)tls_mem_alloc(
                                            sizeof(os_membuf_t) *
                                            OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_ACL_BUF_COUNT), ACL_BLOCK_SIZE));
    assert(ble_hci_vuart_acl_buf != NULL);
#endif
    rc = os_mempool_ext_init(&ble_hci_vuart_acl_pool,
                             MYNEWT_VAL(BLE_ACL_BUF_COUNT),
                             ACL_BLOCK_SIZE,
                             ble_hci_vuart_acl_buf,
                             "ble_hci_vuart_acl_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);
    rc = os_mbuf_pool_init(&ble_hci_vuart_acl_mbuf_pool,
                           &ble_hci_vuart_acl_pool.mpe_mp,
                           ACL_BLOCK_SIZE,
                           MYNEWT_VAL(BLE_ACL_BUF_COUNT));
    SYSINIT_PANIC_ASSERT(rc == 0);
    /*
     * Create memory pool of HCI command buffers. NOTE: we currently dont
     * allow this to be configured. The controller will only allow one
     * outstanding command. We decided to keep this a pool in case we allow
     * allow the controller to handle more than one outstanding command.
     */
#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    ble_hci_vuart_cmd_buf = (os_membuf_t *)tls_mem_alloc(
                                            sizeof(os_membuf_t) *
                                            OS_MEMPOOL_SIZE(1, BLE_HCI_TRANS_CMD_SZ));
    assert(ble_hci_vuart_cmd_buf != NULL);
#endif
    rc = os_mempool_init(&ble_hci_vuart_cmd_pool,
                         1,
                         BLE_HCI_TRANS_CMD_SZ,
                         ble_hci_vuart_cmd_buf,
                         "ble_hci_vuart_cmd_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);
#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    ble_hci_vuart_evt_hi_buf = (os_membuf_t *)tls_mem_alloc(
            sizeof(os_membuf_t) *
            OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
                            MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE)));
    assert(ble_hci_vuart_evt_hi_buf != NULL);
#endif
    rc = os_mempool_init(&ble_hci_vuart_evt_hi_pool,
                         MYNEWT_VAL(BLE_HCI_EVT_HI_BUF_COUNT),
                         MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE),
                         ble_hci_vuart_evt_hi_buf,
                         "ble_hci_vuart_evt_hi_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);
#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    ble_hci_vuart_evt_lo_buf = (os_membuf_t *)tls_mem_alloc(
            sizeof(os_membuf_t) *
            OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
                            MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE)));
    assert(ble_hci_vuart_evt_lo_buf != NULL);
#endif
    rc = os_mempool_init(&ble_hci_vuart_evt_lo_pool,
                         MYNEWT_VAL(BLE_HCI_EVT_LO_BUF_COUNT),
                         MYNEWT_VAL(BLE_HCI_EVT_BUF_SIZE),
                         ble_hci_vuart_evt_lo_buf,
                         "ble_hci_vuart_evt_lo_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

#if HCI_VUART_RX_QUEUE

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    ble_hci_vuart_queue_buf = (os_membuf_t *)tls_mem_alloc(
            sizeof(os_membuf_t) *
            OS_MEMPOOL_SIZE(MYNEWT_VAL(HCI_VUART_QUEUE_BUF_COUNT),
                            MYNEWT_VAL(HCI_VUART_QUEUE_BUF_SIZE)));
    assert(ble_hci_vuart_queue_buf != NULL);
#endif
    rc = os_mempool_init(&ble_hci_vuart_queue_buf_pool,
                         MYNEWT_VAL(HCI_VUART_QUEUE_BUF_COUNT),
                         MYNEWT_VAL(HCI_VUART_QUEUE_BUF_SIZE),
                         ble_hci_vuart_queue_buf,
                         "ble_hci_vuart_queue_buf_pool");
    SYSINIT_PANIC_ASSERT(rc == 0);

    /* Initialize default event queue */
    ble_npl_eventq_init(&hci_evt_rx_tx_queue);
    ble_npl_event_init(&ble_hs_ev_hci_rx, ble_hs_event_hci_rx_func, NULL);
    rc = ble_npl_mutex_init(&hci_resp_queue_mutex);
    assert(rc == BLE_NPL_OK);
    TAILQ_INIT(&hci_resp_queue);
    hci_resp_queue_counter = 0;
    tls_host_vuart_task_stack_ptr = (void *)tls_mem_alloc(MYNEWT_VAL(OS_HS_VUART_STACK_SIZE) * sizeof(
            uint32_t));
    assert(tls_host_vuart_task_stack_ptr != NULL);
    tls_os_task_create(&task_vuart_rx_hdl, "btv",
                       nimble_vhci_task,
                       (void *)0,
                       (void *)tls_host_vuart_task_stack_ptr,
                       MYNEWT_VAL(OS_HS_VUART_STACK_SIZE)*sizeof(uint32_t),
                       MYNEWT_VAL(OS_HS_VUART_TASK_PRIO),
                       0);
#endif
    rc = wm_ble_controller_init(uart_idx);
    assert(rc == 0);	
	return rc;
}

#if HCI_VUART_RX_QUEUE  
static void free_vuart_rx_task_stack()
{
    if(tls_host_vuart_task_stack_ptr) {
        tls_mem_free(tls_host_vuart_task_stack_ptr);
        tls_host_vuart_task_stack_ptr = NULL;
    }
}
#endif

int ble_hci_vuart_deinit()
{
    int rc;
    rc = wm_ble_controller_deinit();
    assert(rc == 0);

#if HCI_VUART_RX_QUEUE  
    struct QUEUE_ITEM *item = NULL;

    //delete task
    if(task_vuart_rx_hdl) {
        tls_os_task_del_by_task_handle(task_vuart_rx_hdl, free_vuart_rx_task_stack);
    }
    //free resp queue resource if not empty
    item = TAILQ_FIRST(&hci_resp_queue);
    while(item)
    {
        TAILQ_REMOVE(&hci_resp_queue, item, entries);
        ble_hci_trans_buf_free((uint8_t*)item);
        item = TAILQ_FIRST(&hci_resp_queue);
    }
    
    ble_npl_eventq_deinit(&hci_evt_rx_tx_queue);
    ble_npl_mutex_deinit(&hci_resp_queue_mutex);
    
#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
    if(ble_hci_vuart_queue_buf)
    {
        tls_mem_free(ble_hci_vuart_queue_buf);
        ble_hci_vuart_queue_buf = NULL;
    }
#endif

#endif

#if MYNEWT_VAL(SYS_MEM_DYNAMIC)
        
        if(ble_hci_vuart_acl_buf) {
            tls_mem_free(ble_hci_vuart_acl_buf);
            ble_hci_vuart_acl_buf = NULL;
        }
    
        if(ble_hci_vuart_cmd_buf) {
            tls_mem_free(ble_hci_vuart_cmd_buf);
            ble_hci_vuart_cmd_buf = NULL;
        }
    
        if(ble_hci_vuart_evt_hi_buf) {
            tls_mem_free(ble_hci_vuart_evt_hi_buf);
            ble_hci_vuart_evt_hi_buf = NULL;
        }
    
        if(ble_hci_vuart_evt_lo_buf) {
            tls_mem_free(ble_hci_vuart_evt_lo_buf);
            ble_hci_vuart_evt_lo_buf = NULL;
        }
#endif

    return rc;
}

