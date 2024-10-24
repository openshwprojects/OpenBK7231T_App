/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hci_h4.c
 *
 *  Description:   Contains HCI transport send/receive functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_h4"
#include <string.h>
#include <stdio.h>
#include "bt_target.h"
#include "bt_hci_bdroid.h"
#include "bt_types.h"
#include "hcih.h"
#include "bt_utils.h"
#include "bt_buffer_q.h"
#include "data_types.h"
#include "wm_bt.h"



/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef HCI_DBG
#define HCI_DBG FALSE
#endif

#if (HCI_DBG == TRUE)
#define HCIDBG(fmt, ...)  \
    do{\
        if(1) \
            LogMsg(0xffff, "%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__); \
    }while(0)
#else
#define HCIDBG(param, ...) {}
#endif


/* Preamble length for HCI Commands:
**      2-bytes for opcode and 1 byte for length
*/
#define HCI_CMD_PREAMBLE_SIZE   3

/* Preamble length for HCI Events:
**      1-byte for opcode and 1 byte for length
*/
#define HCI_EVT_PREAMBLE_SIZE   2

/* Preamble length for SCO Data:
**      2-byte for Handle and 1 byte for length
*/
#define HCI_SCO_PREAMBLE_SIZE   3

/* Preamble length for ACL Data:
**      2-byte for Handle and 2 byte for length
*/
#define HCI_ACL_PREAMBLE_SIZE   4

/* Table of HCI preamble sizes for the different HCI message types */
static const uint8_t hci_preamble_table[] = {
    HCI_CMD_PREAMBLE_SIZE,
    HCI_ACL_PREAMBLE_SIZE,
    HCI_SCO_PREAMBLE_SIZE,
    HCI_EVT_PREAMBLE_SIZE
};

/* HCI H4 message type definitions */
#define H4_TYPE_COMMAND         1
#define H4_TYPE_ACL_DATA        2
#define H4_TYPE_SCO_DATA        3
#define H4_TYPE_EVENT           4

static const uint16_t msg_evt_table[] = {
    MSG_HC_TO_STACK_HCI_ERR,       /* H4_TYPE_COMMAND */
    MSG_HC_TO_STACK_HCI_ACL,       /* H4_TYPE_ACL_DATA */
    MSG_HC_TO_STACK_HCI_SCO,       /* H4_TYPE_SCO_DATA */
    MSG_HC_TO_STACK_HCI_EVT        /* H4_TYPE_EVENT */
};

#define ACL_RX_PKT_START        2
#define ACL_RX_PKT_CONTINUE     1
#define L2CAP_HEADER_SIZE       4

/* Maximum numbers of allowed internal
** outstanding command packets at any time
*/
#define INT_CMD_PKT_MAX_COUNT       8
#define INT_CMD_PKT_IDX_MASK        0x07

#define HCI_COMMAND_COMPLETE_EVT    0x0E
#define HCI_COMMAND_STATUS_EVT      0x0F
#define HCI_READ_BUFFER_SIZE        0x1005
#define HCI_LE_READ_BUFFER_SIZE     0x2002

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* H4 Rx States */
typedef enum {
    H4_RX_MSGTYPE_ST,
    H4_RX_LEN_ST,
    H4_RX_DATA_ST,
    H4_RX_IGNORE_ST
} tHCI_H4_RCV_STATE;

typedef struct {
    uint16_t opcode;        /* OPCODE of outstanding internal commands */
    tINT_CMD_CBACK cback;   /* Callback function when return of internal
                             * command is received */
} tINT_CMD_Q;

/* Control block for HCISU_H4 */
typedef struct {
    HC_BT_HDR *p_rcv_msg;          /* Buffer to hold current rx HCI message */
    uint16_t rcv_len;               /* Size of current incoming message */
    uint8_t rcv_msg_type;           /* Current incoming message type */
    tHCI_H4_RCV_STATE rcv_state;    /* Receive state of current rx message */
    uint16_t hc_acl_data_size;      /* Controller's max ACL data length */
    uint16_t hc_ble_acl_data_size;  /* Controller's max BLE ACL data length */
    BUFFER_Q acl_rx_q;      /* Queue of base buffers for fragmented ACL pkts */
    uint8_t preload_count;          /* Count numbers of preload bytes */
    uint8_t preload_buffer[6];      /* HCI_ACL_PREAMBLE_SIZE + 2 */
    int int_cmd_rsp_pending;        /* Num of internal cmds pending for ack */
    uint8_t int_cmd_rd_idx;         /* Read index of int_cmd_opcode queue */
    uint8_t int_cmd_wrt_idx;        /* Write index of int_cmd_opcode queue */
    tINT_CMD_Q int_cmd[INT_CMD_PKT_MAX_COUNT]; /* FIFO queue */
} tHCI_H4_CB;




/******************************************************************************
**  Externs
******************************************************************************/

uint8_t hci_h4_send_int_cmd(uint16_t opcode, HC_BT_HDR *p_buf, \
                            tINT_CMD_CBACK p_cback);
#ifdef WM_LPM_ENABLED
void lpm_wake_assert(void);
void lpm_tx_done(uint8_t is_tx_done);
#endif

/******************************************************************************
**  Variables
******************************************************************************/

/* Num of allowed outstanding HCI CMD packets */
volatile int num_hci_cmd_pkts = 1;

/******************************************************************************
**  Static variables
******************************************************************************/


static tHCI_H4_CB       h4_cb;
static void controller_rcv_pkt_ready(int cnt);
static void host_rcv_pkt(uint8_t *msg, uint16_t len);

static const tls_bt_host_if_t vuart_host_cb = {
    .notify_controller_avaiable_hci_buffer = controller_rcv_pkt_ready,
    .notify_host_recv_h4 = host_rcv_pkt,
};

/******************************************************************************
**  Static functions
******************************************************************************/

/*******************************************************************************
**
** Function         get_acl_data_length_cback
**
** Description      Callback function for HCI_READ_BUFFER_SIZE and
**                  HCI_LE_READ_BUFFER_SIZE commands if they were sent because
**                  of internal request.
**
** Returns          None
**
*******************************************************************************/
void get_acl_data_length_cback(void *p_mem)
{
}


/*******************************************************************************
**
** Function         internal_event_intercept
**
** Description      This function is called to parse received HCI event and
**                  - update the Num_HCI_Command_Packets
**                  - intercept the event if it is the result of an early
**                    issued internal command.
**
** Returns          TRUE : if the event had been intercepted for internal process
**                  FALSE : send this event to core stack
**
*******************************************************************************/
uint8_t internal_event_intercept(void)
{
    uint8_t     *p;
    uint8_t     event_code, len;
    uint16_t    opcode;
    tHCI_H4_CB  *p_cb = &h4_cb;
    p = (uint8_t *)(p_cb->p_rcv_msg + 1);
    event_code = *p++;
    len = *p++;
	(void)len;

    //HCIDBG("event_code:0x%08x, int_cmd_rsp_pending:%d",event_code, p_cb->int_cmd_rsp_pending);
    //dumphex("2 internal_event_interpret", p, len);
    if(event_code == HCI_COMMAND_COMPLETE_EVT) {
        num_hci_cmd_pkts = *p++;

        //printf("num_hci_cmd_pkts=%d, opcode=0x%08x\n", num_hci_cmd_pkts,p_cb->int_cmd[p_cb->int_cmd_rd_idx].opcode);

        if(p_cb->int_cmd_rsp_pending > 0) {
            STREAM_TO_UINT16(opcode, p)

            if(opcode == p_cb->int_cmd[p_cb->int_cmd_rd_idx].opcode) {
                printf("Intercept CommandCompleteEvent for internal command (0x%04X)", opcode);

                if(p_cb->int_cmd[p_cb->int_cmd_rd_idx].cback != NULL) {
                    p_cb->int_cmd[p_cb->int_cmd_rd_idx].cback(p_cb->p_rcv_msg);
                } else {
                    // Missing cback function!
                    // Release the p_rcv_msg buffer.
                    if(bt_hc_cbacks) {
                        bt_hc_cbacks->dealloc(p_cb->p_rcv_msg);
                    }
                }

                p_cb->int_cmd_rd_idx = ((p_cb->int_cmd_rd_idx + 1) & \
                                        INT_CMD_PKT_IDX_MASK);
                p_cb->int_cmd_rsp_pending--;
                return TRUE;
            }
        }
    } else if(event_code == HCI_COMMAND_STATUS_EVT) {
        num_hci_cmd_pkts = *(++p);
    }

    return FALSE;
}

/*******************************************************************************
**
** Function         acl_rx_frame_buffer_alloc
**
** Description      This function is called from the HCI transport when the
**                  first 4 or 6 bytes of an HCI ACL packet have been received:
**                  - Allocate a new buffer if it is a start pakcet of L2CAP
**                    message.
**                  - Return the buffer address of the starting L2CAP message
**                    frame if the packet is the next segment of a fragmented
**                    L2CAP message.
**
** Returns          the address of the receive buffer H4 RX should use
**                  (CR419: Modified to return NULL in case of error.)
**
** NOTE             This assumes that the L2CAP MTU size is less than the size
**                  of an HCI ACL buffer, so the maximum L2CAP message will fit
**                  into one buffer.
**
*******************************************************************************/
static HC_BT_HDR *acl_rx_frame_buffer_alloc(void)
{
    uint8_t     *p;
    uint16_t    handle;
    uint16_t    hci_len;
    uint16_t    total_len;
    uint8_t     pkt_type;
    HC_BT_HDR  *p_return_buf = NULL;
    tHCI_H4_CB  *p_cb = &h4_cb;
    p = p_cb->preload_buffer;
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT16(hci_len, p);
    STREAM_TO_UINT16(total_len, p);
    //printf("handle:%04x, hci_len=%04x, total_len=%04x\n", handle, hci_len, total_len);
    pkt_type = (uint8_t)(((handle) >> 12) & 0x0003);
    handle   = (uint16_t)((handle) & 0x0FFF);

    if(p_cb->acl_rx_q.count) {
        uint16_t save_handle;
        HC_BT_HDR *p_hdr = p_cb->acl_rx_q.p_first;

        while(p_hdr != NULL) {
            p = (uint8_t *)(p_hdr + 1);
            STREAM_TO_UINT16(save_handle, p);
            save_handle   = (uint16_t)((save_handle) & 0x0FFF);

            if(save_handle == handle) {
                p_return_buf = p_hdr;
                break;
            }

            p_hdr = utils_getnext(p_hdr);
        }
    }

    if(pkt_type == ACL_RX_PKT_START) {      /*** START PACKET ***/
        /* Might have read 2 bytes for the L2CAP payload length */
        p_cb->rcv_len = (hci_len) ? (hci_len - 2) : 0;

        /* Start of packet. If we were in the middle of receiving */
        /* a packet on the same ACL handle, the original packet is incomplete.
         * Drop it. */
        if(p_return_buf) {
            printf("H4 - dropping incomplete ACL frame");
            utils_remove_from_queue(&(p_cb->acl_rx_q), p_return_buf);

            if(bt_hc_cbacks) {
                bt_hc_cbacks->dealloc(p_return_buf);
            }

            p_return_buf = NULL;
        }

        /* Allocate a buffer for message */
        if(bt_hc_cbacks) {
            int len = total_len + HCI_ACL_PREAMBLE_SIZE + L2CAP_HEADER_SIZE + \
                      BT_HC_HDR_SIZE;
            p_return_buf = (HC_BT_HDR *) bt_hc_cbacks->alloc(len);
        }

        if(p_return_buf) {
            /* Initialize buffer with preloaded data */
            p_return_buf->offset = 0;
            p_return_buf->layer_specific = 0;
            p_return_buf->event = MSG_HC_TO_STACK_HCI_ACL;
            p_return_buf->len = p_cb->preload_count;
            wm_memcpy((uint8_t *)(p_return_buf + 1), p_cb->preload_buffer, \
                      p_cb->preload_count);

            //dumphex("prepare acl buffer", p_cb->preload_buffer, p_cb->preload_count);
            if(hci_len && ((total_len + L2CAP_HEADER_SIZE) > hci_len)) {
                /* Will expect to see fragmented ACL packets */
                /* Keep the base buffer address in the watching queue */
                utils_enqueue(&(p_cb->acl_rx_q), p_return_buf);
            }
        }
    } else {                                /*** CONTINUATION PACKET ***/
        p_cb->rcv_len = hci_len;

        if(p_return_buf) {
            /* Packet continuation and found the original rx buffer */
            uint8_t *p_f = p = (uint8_t *)(p_return_buf + 1) + 2;
            STREAM_TO_UINT16(total_len, p);
            /* Update HCI header of first segment (base buffer) with new len */
            total_len += hci_len;
            UINT16_TO_STREAM(p_f, total_len);
        }
    }

    return (p_return_buf);
}

/*******************************************************************************
**
** Function         acl_rx_frame_end_chk
**
** Description      This function is called from the HCI transport when the last
**                  byte of an HCI ACL packet has been received. It checks if
**                  the L2CAP message is complete, i.e. no more continuation
**                  packets are expected.
**
** Returns          TRUE if message complete, FALSE if continuation expected
**
*******************************************************************************/
static uint8_t acl_rx_frame_end_chk(void)
{
    uint8_t     *p;
    uint16_t    handle, hci_len, l2cap_len;
    HC_BT_HDR  *p_buf;
    tHCI_H4_CB  *p_cb = &h4_cb;
    uint8_t     frame_end = TRUE;
    p_buf = p_cb->p_rcv_msg;
    p = (uint8_t *)(p_buf + 1);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT16(hci_len, p);
    STREAM_TO_UINT16(l2cap_len, p);

    if(hci_len > 0) {
        if(l2cap_len > (p_buf->len - (HCI_ACL_PREAMBLE_SIZE + L2CAP_HEADER_SIZE))) {
            /* If the L2CAP length has not been reached, tell H4 not to send
             * this buffer to stack */
            frame_end = FALSE;
        } else {
            /*
             * The current buffer coulb be in the watching list.
             * Remove it from the list if it is in.
             */
            if(p_cb->acl_rx_q.count) {
                utils_remove_from_queue(&(p_cb->acl_rx_q), p_buf);
            }
        }
    }

    /****
     ** Print snoop trace
     ****/
    if(p_buf->offset) {
        /* CONTINUATION PACKET */
        /* save original p_buf->len content */
        uint16_t tmp_u16 = p_buf->len;
        /* borrow HCI_ACL_PREAMBLE_SIZE bytes from the payload section */
        p = (uint8_t *)(p_buf + 1) + p_buf->offset - HCI_ACL_PREAMBLE_SIZE;
        /* save contents */
        wm_memcpy(p_cb->preload_buffer, p, HCI_ACL_PREAMBLE_SIZE);
        /* Set packet boundary flags to "continuation packet" */
        handle = (handle & 0xCFFF) | 0x1000;
        /* write handl & length info */
        UINT16_TO_STREAM(p, handle);
        UINT16_TO_STREAM(p, (p_buf->len - p_buf->offset));
        /* roll pointer back */
        p = p - HCI_ACL_PREAMBLE_SIZE;
        /* adjust `p_buf->offset` & `p_buf->len`
         * before calling btsnoop_capture() */
        p_buf->offset = p_buf->offset - HCI_ACL_PREAMBLE_SIZE;
        p_buf->len = p_buf->len - p_buf->offset;
        //btsnoop_capture(p_buf, true);
        /* restore contents */
        wm_memcpy(p, p_cb->preload_buffer, HCI_ACL_PREAMBLE_SIZE);
        /* restore p_buf->len */
        p_buf->len = tmp_u16;
    } else {
        /* START PACKET */
        //btsnoop_capture(p_buf, true);
    }

    if(frame_end == TRUE) {
        p_buf->offset = 0;
    } else {
        p_buf->offset = p_buf->len;    /* save current buffer-end position */
    }

    return frame_end;
}

/*****************************************************************************
**   HCI H4 INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        hci_h4_init
**
** Description     Initialize H4 module
**
** Returns         None
**
*******************************************************************************/
void hci_h4_init(void)
{
    wm_memset(&h4_cb, 0, sizeof(tHCI_H4_CB));
    utils_queue_init(&(h4_cb.acl_rx_q));
    /* Per HCI spec., always starts with 1 */
    num_hci_cmd_pkts = 1;
    /* Give an initial values of Host Controller's ACL data packet length
     * Will update with an internal HCI(_LE)_Read_Buffer_Size request
     */
    h4_cb.hc_acl_data_size = 1021;
    h4_cb.hc_ble_acl_data_size = 251;
    tls_bt_ctrl_if_register(&vuart_host_cb);
}

/*******************************************************************************
**
** Function        hci_h4_cleanup
**
** Description     Clean H4 module
**
** Returns         None
**
*******************************************************************************/
void hci_h4_cleanup(void)
{
    HCIDBG("hci_h4_cleanup");
}

/*******************************************************************************
**
** Function        hci_h4_send_msg
**
** Description     Determine message type, set HCI H4 packet indicator, and
**                 send message through USERIAL driver
**
** Returns         None
**
*******************************************************************************/
void hci_h4_send_msg(HC_BT_HDR *p_msg)
{
    uint8_t type = 0;
    uint16_t handle;
    uint16_t bytes_to_send, lay_spec;
    uint8_t *p = ((uint8_t *)(p_msg + 1)) + p_msg->offset;
    uint16_t event = p_msg->event & MSG_EVT_MASK;
    uint16_t sub_event = p_msg->event & MSG_SUB_EVT_MASK;
    uint16_t acl_pkt_size = 0, acl_data_size = 0;
    /* wake up BT device if its in sleep mode */
    //lpm_wake_assert();

    if(event == MSG_STACK_TO_HC_HCI_ACL) {
        type = H4_TYPE_ACL_DATA;
    } else if(event == MSG_STACK_TO_HC_HCI_SCO) {
        type = H4_TYPE_SCO_DATA;
    } else if(event == MSG_STACK_TO_HC_HCI_CMD) {
        type = H4_TYPE_COMMAND;
    }

    if(sub_event == LOCAL_BR_EDR_CONTROLLER_ID) {
        acl_data_size = h4_cb.hc_acl_data_size;
        acl_pkt_size = h4_cb.hc_acl_data_size + HCI_ACL_PREAMBLE_SIZE;
    } else {
        acl_data_size = h4_cb.hc_ble_acl_data_size;
        acl_pkt_size = h4_cb.hc_ble_acl_data_size + HCI_ACL_PREAMBLE_SIZE;
    }

    /* Check if sending ACL data that needs fragmenting */
    if((event == MSG_STACK_TO_HC_HCI_ACL) && (p_msg->len > acl_pkt_size)) {
        /* Get the handle from the packet */
        STREAM_TO_UINT16(handle, p);
        /* Set packet boundary flags to "continuation packet" */
        handle = (handle & 0xCFFF) | 0x1000;

        /* Do all the first chunks */
        while(p_msg->len > acl_pkt_size) {
            /* remember layer_specific because uart borrow
               one byte from layer_specific for packet type */
            lay_spec = p_msg->layer_specific;
            p = ((uint8_t *)(p_msg + 1)) + p_msg->offset - 1;
            *p = type;
            bytes_to_send = acl_pkt_size + 1; /* packet_size + message type */

            //bytes_sent = userial_write(event,(uint8_t *) p,bytes_to_send);
            //bytes_sent = wm_host_2_controller_write((uint8_t *) p, bytes_to_send);
            if(tls_bt_vuart_host_check_send_available()) {
                tls_bt_vuart_host_send_packet((uint8_t *) p, bytes_to_send);
            } else {
                //TODO WARNING; controller can not handle hci cmds now!!!
                printf("Warning, controller congested\r\n");
            }

            /* generate snoop trace message */
            //btsnoop_capture(p_msg, false);
            p_msg->layer_specific = lay_spec;
            /* Adjust offset and length for what we just sent */
            p_msg->offset += acl_data_size;
            p_msg->len    -= acl_data_size;
            p = ((uint8_t *)(p_msg + 1)) + p_msg->offset;
            UINT16_TO_STREAM(p, handle);

            if(p_msg->len > acl_pkt_size) {
                UINT16_TO_STREAM(p, acl_data_size);
            } else {
                UINT16_TO_STREAM(p, p_msg->len - HCI_ACL_PREAMBLE_SIZE);
            }

            /* If we were only to send partial buffer, stop when done.    */
            /* Send the buffer back to L2CAP to send the rest of it later */
            if(p_msg->layer_specific) {
                if(--p_msg->layer_specific == 0) {
                    p_msg->event = MSG_HC_TO_STACK_L2C_SEG_XMIT;

                    if(bt_hc_cbacks) {
                        bt_hc_cbacks->tx_result((TRANSAC) p_msg, \
                                                (char *)(p_msg + 1), \
                                                BT_HC_TX_FRAGMENT);
                    }

                    return;
                }
            }
        }
    }

    /* remember layer_specific because uart borrow
       one byte from layer_specific for packet type */
    lay_spec = p_msg->layer_specific;
    /* Put the HCI Transport packet type 1 byte before the message */
    p = ((uint8_t *)(p_msg + 1)) + p_msg->offset - 1;
    *p = type;
    bytes_to_send = p_msg->len + 1;     /* message_size + message type */

    //bytes_sent = userial_write(event,(uint8_t *) p, bytes_to_send);
    //bytes_sent = wm_host_2_controller_write((uint8_t *) p, bytes_to_send);
    if(tls_bt_vuart_host_check_send_available()) {
        tls_bt_vuart_host_send_packet((uint8_t *) p, bytes_to_send);
    } else {
        //TODO WARNING; controller can not handle hci cmds now!!!
        printf("Warning, controller congested\r\n");
    }

    p_msg->layer_specific = lay_spec;

    if(event == MSG_STACK_TO_HC_HCI_CMD) {
        num_hci_cmd_pkts--;
        /* If this is an internal Cmd packet, the layer_specific field would
         * have stored with the opcode of HCI command.
         * Retrieve the opcode from the Cmd packet.
         */
        p++;
        STREAM_TO_UINT16(lay_spec, p);
    }

    /* generate snoop trace message */
    //btsnoop_capture(p_msg, false);

    if(bt_hc_cbacks) {
        //HCIDBG("bt_hc_cbacksbt_hc_cbacks...,event=0x%04x int_cmd_rsp_pending:%x, p_msg->lay_spec:0x%08x, lay_spec:0x%08x\n",event, h4_cb.int_cmd_rsp_pending ,p_msg->layer_specific,lay_spec );
        if((event == MSG_STACK_TO_HC_HCI_CMD) && \
                (h4_cb.int_cmd_rsp_pending > 0) && \
                (p_msg->layer_specific == lay_spec)) {
            printf("bt_hc_cbacks-->dealloc...");
            /* dealloc buffer of internal command */
            bt_hc_cbacks->dealloc(p_msg);
        } else {
            //HCIDBG("bt_hc_cbacks-->tx_result...");
            bt_hc_cbacks->tx_result((TRANSAC) p_msg, (char *)(p_msg + 1), \
                                    BT_HC_TX_SUCCESS);
        }
    }

    //lpm_tx_done(TRUE);
    return;
}
static void controller_rcv_pkt_ready(int cnt)
{
    UNUSED(cnt);
}

static void host_rcv_pkt(uint8_t *msg, uint16_t len)
{
    uint8_t type;
    uint16_t msg_len = 0;
    tHCI_H4_CB  *p_cb = &h4_cb;
    int offset = 0;
    uint8_t msg_received = FALSE;
    type = msg[offset++];

    switch(p_cb->rcv_state) {
        case H4_RX_MSGTYPE_ST:
            if((type < H4_TYPE_ACL_DATA) || (type > H4_TYPE_EVENT)) {
                /* Unknown HCI message type */
                /* Drop this byte */
                printf("[h4] Unknown HCI message type drop this byte 0x%x", type);
                break;
            }

            p_cb->rcv_msg_type = type;
            p_cb->rcv_len = hci_preamble_table[type - 1];
            wm_memset(p_cb->preload_buffer, 0, 6);
            wm_memcpy(p_cb->preload_buffer, msg + offset, p_cb->rcv_len);
            offset += p_cb->rcv_len;
            p_cb->preload_count = p_cb->rcv_len;

            if(type == H4_TYPE_ACL_DATA) {
                //hci_dbg_hexstring("LMP:", msg, len);
                /* ACL data lengths are 16-bits */
                msg_len = p_cb->preload_buffer[3];
                msg_len = (msg_len << 8) + p_cb->preload_buffer[2];

                if(msg_len && (p_cb->preload_count == 4)) {
                    /* Check if this is a start packet */
                    type = ((p_cb->preload_buffer[1] >> 4) & 0x03);

                    if(type == ACL_RX_PKT_START) {
                        /*
                                  * A start packet & with non-zero data payload length.
                                  * We want to read 2 more bytes to get L2CAP payload
                                  * length.
                                  */
                        wm_memcpy(p_cb->preload_buffer + p_cb->preload_count, msg + offset, 2);
                        offset += 2;
                        p_cb->preload_count = 6;
                    }
                }

                /*
                         * Check for segmented packets. If this is a continuation
                         * packet, then we will continue appending data to the
                         * original rcv buffer.
                         */
                p_cb->p_rcv_msg = acl_rx_frame_buffer_alloc();
            } else {
                msg_len = p_cb->preload_buffer[p_cb->rcv_len - 1];
                p_cb->rcv_len = msg_len;

                /* Allocate a buffer for message */
                if(bt_hc_cbacks) {
                    len = msg_len + p_cb->preload_count + BT_HC_HDR_SIZE;
                    p_cb->p_rcv_msg = (HC_BT_HDR *) bt_hc_cbacks->alloc(len);
                }

                if(p_cb->p_rcv_msg) {
                    /* Initialize buffer with preloaded data */
                    p_cb->p_rcv_msg->offset = 0;
                    p_cb->p_rcv_msg->layer_specific = 0;
                    p_cb->p_rcv_msg->event = msg_evt_table[p_cb->rcv_msg_type - 1];
                    p_cb->p_rcv_msg->len = p_cb->preload_count;
                    wm_memcpy((uint8_t *)(p_cb->p_rcv_msg + 1), \
                              p_cb->preload_buffer, p_cb->preload_count);
                } else {
                    printf("bt_hc_cbacks error\n");
                }
            }

            if(p_cb->p_rcv_msg == NULL) {
                break;
            }

            if(msg_len == 0) {
                /* Message has no additional parameters.
                         * (Entire message has been received) */
                if(p_cb->rcv_msg_type == H4_TYPE_ACL_DATA) {
                    acl_rx_frame_end_chk();    /* to print snoop trace */
                }

                msg_received = TRUE;
                /* Next, wait for next message */
                p_cb->rcv_state = H4_RX_MSGTYPE_ST;
            } else {
                //if (p_cb->rcv_msg_type == H4_TYPE_ACL_DATA) printf("copy data(%d),to offset=%d\n",p_cb->rcv_len ,p_cb->p_rcv_msg->len);
                wm_memcpy((uint8_t *)(p_cb->p_rcv_msg + 1) + p_cb->p_rcv_msg->len, msg + offset, p_cb->rcv_len);
                p_cb->p_rcv_msg->len += p_cb->rcv_len;
                p_cb->rcv_len = 0;

                /* Received entire packet. */
                /* Check for segmented l2cap packets */
                if((p_cb->rcv_msg_type == H4_TYPE_ACL_DATA) &&
                        !acl_rx_frame_end_chk()) {
                    /* Not the end of packet yet. */
                    /* Next, wait for next message */
                    p_cb->rcv_state = H4_RX_MSGTYPE_ST;
                } else {
                    msg_received = TRUE;
                    //if(p_cb->rcv_msg_type == H4_TYPE_ACL_DATA) hci_dbg_hexstring("EEE\r\n", NULL, 0);
                    /* Next, wait for next message */
                    p_cb->rcv_state = H4_RX_MSGTYPE_ST;
                }
            }

            /* If we received entire message, then send it to the task */
            if(msg_received) {
                uint8_t intercepted = FALSE;

                /* generate snoop trace message */
                /* ACL packet tracing had done in acl_rx_frame_end_chk() */
                //if (p_cb->p_rcv_msg->event != MSG_HC_TO_STACK_HCI_ACL)
                //    btsnoop_capture(p_cb->p_rcv_msg, true);
                //if(p_cb->p_rcv_msg->event == MSG_HC_TO_STACK_HCI_EVT) hci_dbg_hexstring("XXX:", NULL, 0);

                if(p_cb->p_rcv_msg->event == MSG_HC_TO_STACK_HCI_EVT) {
                    intercepted = internal_event_intercept();
                }

                if((bt_hc_cbacks) && (intercepted == FALSE)) {
                    //if(p_cb->rcv_msg_type == H4_TYPE_ACL_DATA) hci_dbg_hexstring("ACL:", (char *) (p_cb->p_rcv_msg + 1), p_cb->p_rcv_msg->len);
                    bt_hc_cbacks->data_ind((TRANSAC) p_cb->p_rcv_msg, \
                                           (char *)(p_cb->p_rcv_msg + 1), \
                                           p_cb->p_rcv_msg->len + BT_HC_HDR_SIZE);
                }

                p_cb->p_rcv_msg = NULL;
            }

            break;

        default:
            printf("[h4] Unknown HCI message type drop this state 0x%x", p_cb->rcv_state);
            break;
    }
}
/*******************************************************************************
**
** Function        hci_h4_receive_msg
**
** Description     Construct HCI EVENT/ACL packets and send them to stack once
**                 complete packet has been received.
**
** Returns         Number of read bytes
**
*******************************************************************************/
uint16_t hci_h4_receive_msg(void)
{
    return 0;
}


/*******************************************************************************
**
** Function        hci_h4_send_int_cmd
**
** Description     Place the internal commands (issued internally by vendor lib)
**                 in the tx_q.
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t hci_h4_send_int_cmd(uint16_t opcode, HC_BT_HDR *p_buf, \
                            tINT_CMD_CBACK p_cback)
{
    HCIDBG("hci_h4_send_int_cmd...");
    return TRUE;
}


/*******************************************************************************
**
** Function        hci_h4_get_acl_data_length
**
** Description     Issue HCI_READ_BUFFER_SIZE command to retrieve Controller's
**                 ACL data length setting
**
** Returns         None
**
*******************************************************************************/
void hci_h4_get_acl_data_length(void)
{
}


/******************************************************************************
**  HCI H4 Services interface table
******************************************************************************/

const tHCI_IF hci_h4_func_table = {
    hci_h4_init,
    hci_h4_cleanup,
    hci_h4_send_msg,
    hci_h4_send_int_cmd,
    hci_h4_get_acl_data_length,
    hci_h4_receive_msg
};

