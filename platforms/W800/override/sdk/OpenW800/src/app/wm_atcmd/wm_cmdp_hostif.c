
/**************************************************************************
 * File Name                   : wm_cmdp_hostif.c
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#if (GCC_COMPILE==1)
#include "wm_cmdp_hostif_gcc.h"
#else
#include "wm_cmdp_hostif.h"
#endif
#include "wm_debug.h"
#include "wm_hspi.h"
#include "list.h"
#include "wm_mem.h"
#include "wm_regs.h"
#include "wm_params.h"
#include "wm_wl_task.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "wm_efuse.h"
#include "wm_fwup.h"
#include "wm_flash.h"
#include "litepoint.h"
#include "wm_irq.h"
#include "wm_config.h"
#include "wm_http_client.h"
#include "wm_rmms.h"
#include "ping.h"
#include "iperf.h"
#include "iperf_units.h"
#include "wm_cpu.h"
#include "wm_wifi_oneshot.h"
#include "wm_internal_flash.h"
#include "wm_cmdp.h"
#include "wm_bt_config.h"
#include "wm_bt_api.h"
#include "wm_uart_task.h"
#include "wm_bt_def.h"

const u8 SysCreatedDate[] = __DATE__;
const u8 SysCreatedTime[] = __TIME__;

#if TLS_CONFIG_HOSTIF
#include "wm_osal.h"
#include "wm_uart.h"
#include "wm_sockets.h"


extern u32 rf_spi_read(u32 reg);
extern void rf_spi_write(u32 reg);
#define RFR_REG_MAX_NUM (28)

extern u8* ieee80211_get_tx_gain(void);
extern u8 *wpa_supplicant_get_mac(void);
extern void wpa_supplicant_set_mac(u8 *mac);
#if TLS_CONFIG_AP
extern u8 *hostapd_get_mac(void);
#endif
extern void tls_wifi_get_oneshot_customdata(u8 *data);

#if TLS_CONFIG_CMD_USE_RAW_SOCKET
extern u32 tls_net_get_sourceip(void);
extern void tls_net_set_sourceip(u32 ipvalue);
#else
static u32 source_ip = 0;
#endif
//static u32 temAtStartUp = 0;

extern void wm_cmdp_oneshot_status_event(u8 status);
extern	int wm_cmdp_oneshot_task_init(void);
extern int wm_cmdp_oneshot_task_del(void);
extern void tls_set_hspi_fwup_mode(u8 ifenable);

extern void wm_rf_set_channel(u16 chan, int channel_type);
extern int t_http_fwup(char *url);
extern int adc_temp(void);

extern void CreateThroughputTask(void);

struct tls_hostif g_hostif;
struct tls_hostif *tls_get_hostif(void)
{
	return &g_hostif;
}

u8 default_socket = 0;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR
struct tls_uart_net_msg *sockrecvmit[TLS_MAX_NETCONN_NUM];
#else
struct tls_uart_circ_buf * sockrecvmit[TLS_MAX_NETCONN_NUM];
#endif
#else
#define SOCK_RECV_TIMEOUT    100
struct tls_uart_circ_buf * sockrecvmit[MEMP_NUM_NETCONN];
fd_set fdatsockets;
static struct sockaddr  sock_cmdp_addrs[MEMP_NUM_NETCONN];
static u32 sock_cmdp_timeouts[MEMP_NUM_NETCONN] = {0};
#endif

#ifndef MIN
   #define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR
struct tls_uart_net_msg * tls_hostif_get_recvmit(int socket_num)
{
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
	TLS_DBGPRT_INFO("socket_num=%d, precvmit=0x%x\n", socket_num, (u32)sockrecvmit[socket_num-1]);
	return sockrecvmit[socket_num-1];
#else
	return sockrecvmit[socket_num-LWIP_SOCKET_OFFSET];
#endif
}
#else
struct tls_uart_circ_buf * tls_hostif_get_recvmit(int socket_num)
{
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
	TLS_DBGPRT_INFO("socket_num=%d, precvmit=0x%x\n", socket_num, (u32)sockrecvmit[socket_num-1]);
	return sockrecvmit[socket_num-1];
#else
	return sockrecvmit[socket_num-LWIP_SOCKET_OFFSET];
#endif
}
#endif

	
void tls_hostif_fill_cmdrsp_hdr(struct tls_hostif_cmdrsp *cmdrsp,
        u8 code, u8 err, u8 ext)
{
    cmdrsp->cmd_hdr.code = code;
    cmdrsp->cmd_hdr.err = err;
    cmdrsp->cmd_hdr.ext = ext;
    cmdrsp->cmd_hdr.msg_type = HOSTIF_MSG_TYPE_RSP;
}

void tls_hostif_fill_event_hdr(struct tls_hostif_event *event,
        u8 code, u8 err, u8 ext)
{
    event->cmd_hdr.code = code;
    event->cmd_hdr.err = err;
    event->cmd_hdr.ext = ext;
    event->cmd_hdr.msg_type = HOSTIF_MSG_TYPE_EVENT; 
}

void tls_hostif_fill_hdr(struct tls_hostif *hif,
        struct tls_hostif_hdr *hdr,
        u8 type, u16 length, u8 flag, u8 dest_addr, u8 chk)
{
    hdr->sync = 0xAA;
    hdr->type = type;
    hdr->length = host_to_be16(length);
    hdr->seq_num = hif->hspi_tx_seq++;
    hdr->flag = flag;
    hdr->dest_addr = dest_addr;
    hdr->chk = chk;
}

struct tls_hostif_tx_msg *tls_hostif_get_tx_msg(void)
{
    struct tls_hostif_tx_msg *tx_msg = tls_mem_alloc(sizeof(struct tls_hostif_tx_msg));
    return tx_msg; 
}
struct tls_hostif_tx_msg *tls_hostif_get_tx_event_msg(struct tls_hostif *hif)
{
    return tls_hostif_get_tx_msg(); 
}
void free_tx_msg_buffer(struct tls_hostif_tx_msg *tx_msg){
	switch (tx_msg->type) {
            case HOSTIF_TX_MSG_TYPE_EVENT:
            case HOSTIF_TX_MSG_TYPE_CMDRSP:
                tls_mem_free(tx_msg->u.msg_event.buf);
                break;
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
            //Tcp and Udp both use the below case.
            case HOSTIF_TX_MSG_TYPE_UDP:
            case HOSTIF_TX_MSG_TYPE_TCP:
				if (tx_msg->u.msg_tcp.p)
				{
                	pbuf_free(tx_msg->u.msg_tcp.p);
				}
                break;
#endif //TLS_CONFIG_SOCKET_RAW
            default:
                break;
        }
}

int tls_hostif_atcmd_loopback(u8 hostif_type,
        char *buf, u32 buflen)
{
    struct tls_hostif_tx_msg *tx_msg;
    struct tls_hostif *hif = tls_get_hostif();
    

    if (buf == NULL || buflen == 0)
        return -1;

    switch (hostif_type) {
        case HOSTIF_MODE_UART0:
            tx_msg = tls_hostif_get_tx_event_msg(hif);
            if (tx_msg == NULL)
                return -1;

            tx_msg->offset = 0;
            tx_msg->u.msg_cmdrsp.buf = buf;
            tx_msg->type = HOSTIF_TX_MSG_TYPE_CMDRSP;
            tx_msg->u.msg_cmdrsp.buflen = buflen;

            if(hif->uart_send_tx_msg_callback != NULL)
                hif->uart_send_tx_msg_callback(hostif_type, tx_msg, FALSE);
            break;
        case HOSTIF_MODE_UART1_LS:
            tx_msg = tls_hostif_get_tx_event_msg(hif);
            if (tx_msg == NULL)
                return -1;

            tx_msg->offset = 0;
            tx_msg->u.msg_cmdrsp.buf = buf;
            tx_msg->type = HOSTIF_TX_MSG_TYPE_CMDRSP;
            tx_msg->u.msg_cmdrsp.buflen = buflen;

            if(hif->uart_send_tx_msg_callback != NULL)
                hif->uart_send_tx_msg_callback(hostif_type, tx_msg, FALSE);
            break;
#if TLS_CONFIG_RMMS
        case HOSTIF_MODE_RMMS_AT:
            /* rmms do nothing */
            break;
#endif
        default:
            break;
    }
    return 0; 
}

extern struct tls_uart_port uart_port[];
int tls_hostif_process_cmdrsp(u8 hostif_type, char *cmdrsp, u32 cmdrsp_size)
{
    struct tls_hostif_tx_msg *tx_msg;
    struct tls_hostif *hif = tls_get_hostif();
    u16 remain_len=0;
	extern int tls_uart_tx_remain_len(struct tls_uart_port *port);

 
    //TLS_DBGPRT_INFO("===>\n");
    if (cmdrsp == NULL || cmdrsp_size == 0)
        return -1;

    switch (hostif_type) 
	{
        case HOSTIF_MODE_HSPI:
            tx_msg = tls_hostif_get_tx_event_msg(hif);
            if (tx_msg == NULL)
            {
                return -1;
            }
            tx_msg->offset = 0;
            tx_msg->u.msg_cmdrsp.buf = cmdrsp;
            tx_msg->type = HOSTIF_TX_MSG_TYPE_CMDRSP;
            tx_msg->u.msg_cmdrsp.buflen = cmdrsp_size;
            if(hif->hspi_send_tx_msg_callback != NULL)
            {
                hif->hspi_send_tx_msg_callback(hostif_type, tx_msg, FALSE);
            }
            break;
        case HOSTIF_MODE_UART0:
        case HOSTIF_MODE_UART1_LS:
        case HOSTIF_MODE_UART1_HS:
            tx_msg = tls_hostif_get_tx_event_msg(hif);
            if (tx_msg == NULL)
                return -1;

            tx_msg->offset = 0;
            tx_msg->u.msg_cmdrsp.buf = cmdrsp;
            tx_msg->type = HOSTIF_TX_MSG_TYPE_CMDRSP;
            tx_msg->u.msg_cmdrsp.buflen = cmdrsp_size;

            if(hif->uart_send_tx_msg_callback != NULL)
            {
                while(tx_msg->u.msg_cmdrsp.buflen > remain_len)
                {
                    if(hostif_type == HOSTIF_MODE_UART0)
                        remain_len = tls_uart_tx_remain_len(&uart_port[0]);
                    else
                        remain_len = tls_uart_tx_remain_len(&uart_port[tls_uart_get_at_cmd_port()]);
                    tls_os_time_delay(2);
                }
                hif->uart_send_tx_msg_callback(hostif_type, tx_msg, FALSE);
            }
            break;
#if TLS_CONFIG_RMMS
        case HOSTIF_MODE_RMMS_AT:
            RMMS_SendHedAtRsp((struct rmms_msg *)cmdrsp);
            break;
#endif
        default:
            break;
    }
    return 0;
}

int tls_hostif_cmd_handler(u8 hostif_cmd_type, char *buf, u32 length)
{
    char *cmdrsp_buf;
    u32 cmdrsp_size;
    struct tls_atcmd_token_t *atcmd_tok = NULL;
    int err;
    int i, name_len;
    struct tls_hostif_hdr *hdr = (struct tls_hostif_hdr *)buf;
    u8 hostif_type;
	struct tls_hostif *hif = tls_get_hostif();

    //TLS_DBGPRT_INFO("===>\n");
    cmdrsp_size = AT_CMD_RSP_BUF_SIZE;
    atcmd_tok = tls_mem_alloc(sizeof(struct tls_atcmd_token_t));
    if (NULL == atcmd_tok)
        return -1;

    switch (hostif_cmd_type) {
        case HOSTIF_HSPI_RI_CMD:
        case HOSTIF_UART1_RI_CMD:
            cmdrsp_buf = tls_mem_alloc(AT_CMD_RSP_BUF_SIZE);
            if (!cmdrsp_buf)
            {
                tls_mem_free(atcmd_tok);
                return -1;
            }
            err = tls_hostif_ricmd_exec(buf + sizeof(struct tls_hostif_hdr), 
                    be_to_host16(hdr->length), cmdrsp_buf, &cmdrsp_size);
            tls_hostif_fill_hdr(hif, 
                    (struct tls_hostif_hdr *)cmdrsp_buf,
                    PACKET_TYPE_RI_CMD,
                    cmdrsp_size, 0, 0, 0); 
            cmdrsp_size += sizeof(struct tls_hostif_hdr);
            if (hostif_cmd_type == HOSTIF_HSPI_RI_CMD)
                hostif_type = HOSTIF_MODE_HSPI;
            else
                hostif_type = HOSTIF_MODE_UART1_HS;
            break;
        case HOSTIF_HSPI_AT_CMD:

            memset(atcmd_tok, 0, sizeof(struct tls_atcmd_token_t));
            err = tls_atcmd_parse(atcmd_tok, buf + sizeof(struct tls_hostif_hdr),
                    length); 

            if (err) {
                //TODO:
            }
            cmdrsp_buf = tls_mem_alloc(AT_CMD_RSP_BUF_SIZE);
            if (!cmdrsp_buf)
            {
                tls_mem_free(atcmd_tok);
                return -1;
            }

//            err = tls_atcmd_exec(&atcmd_tok, cmdrsp_buf, &cmdrsp_size);
            if (err) {
                //TODO:
            }
            hostif_type = HOSTIF_MODE_HSPI;

            break;
        case HOSTIF_UART1_AT_CMD:
        case HOSTIF_UART0_AT_CMD:
#if TLS_CONFIG_RMMS
        case HOSTIF_RMMS_AT_CMD:
            if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
                hostif_type = HOSTIF_MODE_RMMS_AT;
            else 
#endif
            if (hostif_cmd_type == HOSTIF_UART1_AT_CMD)
                hostif_type = HOSTIF_MODE_UART1_LS;
            else
                hostif_type = HOSTIF_MODE_UART0;

            /* at cmd loopback */
            if (hif->uart_insdisp) {
                u8 *atcmd_loopback_buf = tls_mem_alloc(length+1);
                if (!atcmd_loopback_buf)
                {
                    tls_mem_free(atcmd_tok);
                    return -1;
                }
                MEMCPY(atcmd_loopback_buf, buf, length);
                atcmd_loopback_buf[length-1] = '\r';
                atcmd_loopback_buf[length] = '\n';
                err = tls_hostif_atcmd_loopback(hostif_type,
                        (char *)atcmd_loopback_buf, length+1);
                if (err)
                    tls_mem_free(atcmd_loopback_buf);
            }

            cmdrsp_buf = tls_mem_alloc(AT_CMD_RSP_BUF_SIZE);
            if (!cmdrsp_buf)
            {
                tls_mem_free(atcmd_tok);
                return -1;
            }
            memset(atcmd_tok, 0, sizeof(struct tls_atcmd_token_t));
#if TLS_CONFIG_RMMS
            if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
                atcmd_tok->cmd_mode = CMD_MODE_RMMS_ATCMD;
            else 
#endif
            if (hostif_cmd_type == HOSTIF_UART0_AT_CMD)
                atcmd_tok->cmd_mode = CMD_MODE_UART0_ATCMD;
            else
                atcmd_tok->cmd_mode = CMD_MODE_UART1_ATCMD;
            //TLS_DBGPRT_DUMP(buf, length);
            //TLS_DBGPRT_INFO("at cmd :%s\n", buf);
#if TLS_CONFIG_RMMS
            if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
                err = tls_atcmd_parse(atcmd_tok, buf + 6 + 3, length - 3 - 6);
            else
#endif
                err = tls_atcmd_parse(atcmd_tok, buf+3, length - 3);


#if 0
            TLS_DBGPRT_INFO("atcmd = %s\n", atcmd_tok.name);
            TLS_DBGPRT_INFO("atcmd_tok: argc = %d, op = %d \n", 
                    atcmd_tok.arg_found,
                    atcmd_tok.op);
            for (i=0;i<atcmd_tok.arg_found;i++)
                TLS_DBGPRT_INFO("argv[%d] length = %d\n", i,
                        atcmd_tok.arg[i+1] - atcmd_tok.arg[i] - 1);
#endif

            if (err) {
                TLS_DBGPRT_INFO("err parse cmd, code = %d\n", err);
                cmdrsp_size = sprintf(cmdrsp_buf, "+ERR=%d\r\n", err);
            } else {
                name_len = strlen(atcmd_tok->name);
                for (i = 0; i < name_len; i++)
                    atcmd_tok->name[i] = toupper(atcmd_tok->name[i]);
                cmdrsp_size = AT_CMD_RSP_BUF_SIZE;
#if TLS_CONFIG_RMMS
                if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
                {
                    if (strcmp("WSCAN", atcmd_tok->name) == 0)/* ç®ååªæwscanè¿ä¸ä¸ªå¼æ­?*/
                        memcpy(hif->rmms_addr, buf, 6);
                }
#endif
                err = tls_hostif_atcmd_exec(atcmd_tok, cmdrsp_buf, &cmdrsp_size);
                if (err) {
                    //TODO:
                } 	
                if(err != -CMD_ERR_SKT_RPT && err != -CMD_ERR_SKT_SND){
                /* TODO: send cmd response */		
                    cmdrsp_buf[cmdrsp_size] = '\r';
                    cmdrsp_buf[cmdrsp_size+1] = '\n';
    		        cmdrsp_buf[cmdrsp_size+2] = '\r';
                    cmdrsp_buf[cmdrsp_size+3] = '\n';
                    cmdrsp_buf[cmdrsp_size+4] = '\0';
                    cmdrsp_size += 4;
                }
                //tls_mem_free(cmdrsp_buf);
            }
            break;
        default:
            TLS_DBGPRT_ERR("illegal command type\n");
             tls_mem_free(atcmd_tok);
			return -1;
            //break;
    }

#if TLS_CONFIG_RMMS
    if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
    {
        memcpy(buf + 6, cmdrsp_buf, cmdrsp_size + 1);
        memcpy(cmdrsp_buf, buf, 6 + cmdrsp_size + 1);
        cmdrsp_size = cmdrsp_size + 6;
        tls_mem_free(buf);
    }
#endif

    err = tls_hostif_process_cmdrsp(hostif_type, cmdrsp_buf, cmdrsp_size);
    if (err)
    {
        tls_mem_free(cmdrsp_buf);
    }
#if TLS_CONFIG_RMMS
    else
    {
        if (hostif_cmd_type == HOSTIF_RMMS_AT_CMD)
        {
            if (strcmp("WSCAN", atcmd_tok->name) != 0)/* ç®ååªæwscanè¿ä¸ä¸ªå¼æ­?*/
                hif->rmms_status = 0;
        }
    }
#endif

    tls_mem_free(atcmd_tok);
    return err;

}

int tls_hostif_hdr_check(u8 *buf, u32 length)
{
    if (!buf)
        return -1;

#if 0

    hdr = (struct tls_hostif_hdr *)buf;
    payload_len = be_to_host16(*(u16 __packed *)hdr->length);
    chksum = hdr->flag & 0x1;
    type = hdr->type;

    if (payload_len != (length - sizeof(struct tls_hostif_hdr))) {
        return -1;
    }
    /* check comand type */
    if (type > HOSTCMD_TYPE_AT_CMD) 
        return -1;
#endif

    //TODO: è®¡ç®æ ¡éªåå¹¶æ¯è¾ 
    //

    return 0;
}

int tls_hostif_send_event_port_check(void)
{
    struct tls_hostif *hif = tls_get_hostif();

    if (hif->hostif_mode == HOSTIF_MODE_UART1_HS) {
        return 0; 
    } 
    if (hif->hostif_mode == HOSTIF_MODE_HSPI) {
        return 0;
    }

    return -1;
}

int tls_hostif_send_event(char *buf, u32 buflen, u8 type)
{
    struct tls_hostif_tx_msg *tx_msg;
    struct tls_hostif *hif = tls_get_hostif();
    u8 ext;
    struct tls_hostif_event *event = (struct tls_hostif_event *)buf;

    tx_msg = tls_hostif_get_tx_event_msg(hif); 
    if (!tx_msg) {
        return -1;
    }

    tls_hostif_fill_hdr(hif, &event->hdr,
            PACKET_TYPE_RI_CMD,
            buflen - 8, 0, 0, 0); 
    if (buflen == 12)
        ext = 0;
    else {
        ext = 1;
    }
    tls_hostif_fill_event_hdr(event, type, 0, ext); 

    tx_msg->u.msg_event.buf = buf;
    tx_msg->u.msg_event.buflen = buflen;
    tx_msg->type = HOSTIF_TX_MSG_TYPE_EVENT;

    //TLS_DBGPRT_DUMP(buf, buflen);

    if (hif->hostif_mode == HOSTIF_MODE_HSPI) {
        if(hif->hspi_send_tx_msg_callback != NULL)
            hif->hspi_send_tx_msg_callback(HOSTIF_MODE_HSPI, tx_msg, TRUE);
    }
    else if (hif->hostif_mode == HOSTIF_MODE_UART1_HS) {
        if(hif->uart_send_tx_msg_callback != NULL)
            hif->uart_send_tx_msg_callback(HOSTIF_MODE_UART1_HS, tx_msg, TRUE);
    } else {
        return -1;
    }
    return 0;
}

int tls_hostif_send_event_init_cmplt(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;

    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_INIT_END); 

    if (err)
        tls_mem_free(buf);

    return 0;
}

static int tls_hostif_send_event_linkup(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_LINKUP); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_wjoin_success(void)
{
    char *buf;
    u16 buflen;
    int err;
    char *p;
	struct tls_curr_bss_t bss;


    err = tls_hostif_send_event_port_check();
    if (err)
    return 0;

	tls_wifi_get_current_bss(&bss);
	
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr) + 12 + bss.ssid_len;
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    p = &buf[12];
    *p++ = 1;


    MEMCPY(p, bss.bssid, ETH_ALEN);
    p += ETH_ALEN;
    *p++ = (char)bss.type;
    *p++ = (char)bss.channel;
    *p++ = (char)bss.encryptype;
    *p++ = (char)bss.ssid_len;
    MEMCPY(p, bss.ssid, bss.ssid_len);
    p += bss.ssid_len;
	*p = bss.rssi;
    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_JOIN_RES); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_wjoin_failed(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr) + 1;
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    buf[12] = 0;
    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_JOIN_RES); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_linkdown(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_LINKDOWN); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_sta_join(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_STA_JOIN); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_sta_leave(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_STA_LEAVE); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_crc_err(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_CRC_ERR); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_tx_fail(void)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr);
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_TX_ERR); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_tcp_conn(
        u8 socket, u8 res)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr) + 2;
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;
    buf[12] = socket;
    buf[13] = res;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_TCP_CONN); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_tcp_join(u8 socket)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr) + 1;
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;
    buf[12] = socket;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_TCP_JOIN); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_tcp_dis(u8 socket)
{
    char *buf;
    u16 buflen;
    int err;

    err = tls_hostif_send_event_port_check();
    if (err)
        return 0;
    buflen = sizeof(struct tls_hostif_hdr) +
        sizeof(struct tls_hostif_cmd_hdr) + 1;
    buf = (char *)tls_mem_alloc(buflen);
    if (!buf)
        return 0;
    buf[12] = socket;

    err = tls_hostif_send_event(buf, buflen,
            HOSTIF_EVENT_TCP_DIS); 
    if (err)
        tls_mem_free(buf);

    return 0;
}

int tls_hostif_send_event_scan_cmplt(struct tls_scan_bss_t *scan_res,
        enum tls_cmd_mode cmd_mode)
{
    char *buf = NULL;
    u32 buflen, remain_len;
    int err = 0; 
    int i, j;
    struct tls_bss_info_t *bss_info;
    char *p;
    u8 hostif_type;
    u32 strlen;
#if TLS_CONFIG_RMMS
    struct tls_hostif *hif = tls_get_hostif();
#endif

    if (scan_res == NULL)
    {
#if TLS_CONFIG_RMMS
        if (CMD_MODE_RMMS_ATCMD == cmd_mode)
            hif->rmms_status = 0;
#endif
        return -1;
    }

    switch (cmd_mode) {
        case CMD_MODE_HSPI_RICMD:
        case CMD_MODE_UART1_RICMD:
            buflen = 1450; 
            buf = (char *)tls_mem_alloc(buflen);
            if (!buf)
                return 0;
            if (scan_res->count == 0) {
                buflen = 13;
                p = buf + 12;
                *p++ = 0;
            }
            else {
                remain_len = buflen;
                p = buf + 12;
                buflen = 12;
                remain_len -= 12;
                *p++ = (u8)scan_res->count;
                remain_len--;
                buflen++;
                bss_info = scan_res->bss;
                for (i = 0; i < scan_res->count; i++) {
                    if (remain_len < 43)
                        break;
                    MEMCPY(p, bss_info->bssid, ETH_ALEN);
                    p += ETH_ALEN;
                    *p++ = bss_info->mode;
                    *p++ = bss_info->channel;
                    *p++ = bss_info->privacy;
                    *p++ = bss_info->ssid_len;
                    MEMCPY(p, bss_info->ssid, bss_info->ssid_len);
                    p += bss_info->ssid_len;
                    *p++ = (signed char)bss_info->rssi;
                    buflen += (11 + bss_info->ssid_len);
                    remain_len = remain_len - (11 + bss_info->ssid_len);
                    bss_info++; 
                }
            }

            err = tls_hostif_send_event(buf, buflen,
                    HOSTIF_EVENT_SCAN_RES); 
            break;
        case CMD_MODE_UART0_ATCMD:
        case CMD_MODE_UART1_ATCMD:
#if TLS_CONFIG_RMMS
        case CMD_MODE_RMMS_ATCMD:
#endif
            buf = (char *)tls_mem_alloc(2500);
            if (!buf)
            {
#if TLS_CONFIG_RMMS
                if (CMD_MODE_RMMS_ATCMD == cmd_mode)
                    hif->rmms_status = 0;
#endif
                return 0;
            }
            p = buf;
#if TLS_CONFIG_RMMS
            if (CMD_MODE_RMMS_ATCMD == cmd_mode)
            {
                memcpy(p, hif->rmms_addr, 6);
                p += 6;
            }
#endif
            buflen = sprintf(p, "+OK=");
            p += buflen;
            bss_info = scan_res->bss;
#if TLS_CONFIG_RMMS
            if (CMD_MODE_RMMS_ATCMD == cmd_mode)
            {
               buflen += 6;
            }
#endif
            for (i = 0; i < scan_res->count; i++) {
				if (buflen > (2500 - sizeof(struct tls_bss_info_t)))
				{
					break;
				}
                strlen = sprintf(p, "%02X%02X%02X%02X%02X%02X,%u,%u,%u,\"",
                        bss_info->bssid[0], bss_info->bssid[1], bss_info->bssid[2],
                        bss_info->bssid[3], bss_info->bssid[4], bss_info->bssid[5],
                        bss_info->mode, bss_info->channel, bss_info->privacy);
                buflen += strlen;
                p = buf + buflen;
                for (j = 0; j < bss_info->ssid_len; j++) {
                    strlen = sprintf(p, "%c", bss_info->ssid[j]);
                    buflen += strlen;
                    p = buf + buflen;
                }
                strlen = sprintf(p, "\",%d\r\n", (signed char)bss_info->rssi);
                buflen += strlen;
                p = buf + buflen;
                bss_info++; 
            }
#if TLS_CONFIG_RMMS
            if (cmd_mode == CMD_MODE_RMMS_ATCMD)
                hostif_type = HOSTIF_MODE_RMMS_AT;
            else 
#endif
            if (cmd_mode == CMD_MODE_UART0_ATCMD)
                hostif_type = HOSTIF_MODE_UART0;
            else
                hostif_type = HOSTIF_MODE_UART1_LS;

            err = tls_hostif_process_cmdrsp(hostif_type, buf, buflen);

            break;
        default:
            break;
    }
    if (err && buf)
        tls_mem_free(buf);

#if TLS_CONFIG_RMMS
    if (CMD_MODE_RMMS_ATCMD == cmd_mode)
        hif->rmms_status = 0;
#endif

    return 0; 
}

void tls_hostif_tx_timeout(void *ptmr, void *parg)
{
    struct tls_hostif *hif = (struct tls_hostif *)parg;

    if (hif->hostif_mode == HOSTIF_MODE_HSPI)
    {
        if(hif->hspi_send_tx_msg_callback != NULL)
            hif->hspi_send_tx_msg_callback(HOSTIF_MODE_HSPI, NULL, FALSE);
    }
    else if (hif->hostif_mode == HOSTIF_MODE_UART0) {
        if(hif->uart_send_tx_msg_callback != NULL)
            hif->uart_send_tx_msg_callback(HOSTIF_MODE_UART0, NULL, FALSE);
    } else if ((hif->hostif_mode == HOSTIF_MODE_UART1_LS) ||
                (hif->hostif_mode == HOSTIF_MODE_UART1_HS)) {
        if(hif->uart_send_tx_msg_callback != NULL)
            hif->uart_send_tx_msg_callback(hif->hostif_mode, NULL, FALSE);
    } else 
        ;
}

void hostif_wscan_cmplt(void)
{
    char *buf;
    u32 buflen;
    int err;    
    enum tls_cmd_mode cmd_mode;
    struct tls_hostif *hif = tls_get_hostif();

    if (hif->last_scan) {

        cmd_mode = hif->last_scan_cmd_mode;
        hif->last_scan = 0; 

        buflen = 2000;
        buf = tls_mem_alloc(buflen);
        if (!buf)
            return;

        err = tls_wifi_get_scan_rslt((u8 *)buf, buflen);
        if (err) {
            tls_mem_free(buf);
            return;
        }
        switch (cmd_mode) {
            case CMD_MODE_HSPI_RICMD:
            case CMD_MODE_UART1_RICMD:
                tls_hostif_send_event_scan_cmplt((struct tls_scan_bss_t *)buf, cmd_mode);
                tls_mem_free(buf); 
                break;
            case CMD_MODE_UART0_ATCMD:
            case CMD_MODE_UART1_ATCMD:
#if TLS_CONFIG_RMMS
            case CMD_MODE_RMMS_ATCMD:
#endif
                tls_hostif_send_event_scan_cmplt((struct tls_scan_bss_t *)buf, cmd_mode);
                tls_mem_free(buf); 
				hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WSCAN);
                tls_os_sem_release(hif->uart_atcmd_sem); 
                break;
            default:
                tls_mem_free(buf);
        }
    } 
}
#if TLS_CONFIG_UART || TLS_CONFIG_HS_SPI
#define HOSTIF_TASK_USE_DYNAMIC_MALLOC_FTR  1
#define HOSTIF_TASK_STK_SIZE    800
#if HOSTIF_TASK_USE_DYNAMIC_MALLOC_FTR
u32 *hostif_stk;
#else
u32 hostif_stk[HOSTIF_TASK_STK_SIZE];
#endif
struct task_parameter wl_task_param_hostif = {
		.mbox_size = 32,
		.name = "uart spi task",

#if HOSTIF_TASK_USE_DYNAMIC_MALLOC_FTR
		.stk_size = 0,
		.stk_start = NULL,
#else
		.stk_size = HOSTIF_TASK_STK_SIZE,
		.stk_start = (u8 *)hostif_stk,
#endif		
		.task_id = TLS_HOSTIF_TASK_PRIO,
		.mbox_id = TLS_MBOX_ID_HOSTIF_TASK,
		.timeo_id = TLS_TIMEO_ID_HOSTIF_TASK,
};

int tls_hostif_task_init(void){
#if HOSTIF_TASK_USE_DYNAMIC_MALLOC_FTR
	hostif_stk = tls_mem_alloc(HOSTIF_TASK_STK_SIZE *sizeof(u32));
	if (hostif_stk == NULL)
	{
		return -1;
	}
	wl_task_param_hostif.stk_size = HOSTIF_TASK_STK_SIZE;
	wl_task_param_hostif.stk_start = (u8 *)hostif_stk;
#endif
	return tls_wl_task_run(&wl_task_param_hostif);
}
#endif
int tls_hostif_init(void)
{
    struct tls_hostif *hif;
//    struct tls_hostif_tx_msg *tx_msg;
//    int i;
    int err;

	u16 transparent_trigger_length;
	u8 mode;

    hif= &g_hostif;
    memset(hif, 0, sizeof(struct tls_hostif));
    tls_param_get(TLS_PARAM_ID_AUTO_TRIGGER_LENGTH, &transparent_trigger_length, FALSE);
    hif->uart_atlt = transparent_trigger_length;

	//cfg_param.user_port_mode = TLS_PARAM_USR_INTF_LUART; /*set default LUART MODE*/
	tls_param_get(TLS_PARAM_ID_USRINTF, (void *)&mode, TRUE);
    if ((mode == TLS_PARAM_USR_INTF_HSPI) 
		|| (mode == TLS_PARAM_USR_INTF_HSDIO)){
        hif->hostif_mode = HOSTIF_MODE_HSPI;
    }
    else if (mode == TLS_PARAM_USR_INTF_HUART)
        hif->hostif_mode = HOSTIF_MODE_UART1_HS;
    else if (mode == TLS_PARAM_USR_INTF_LUART)
        hif->hostif_mode = HOSTIF_MODE_UART1_LS;
    else
        hif->hostif_mode = HOSTIF_MODE_HSPI;

    err = tls_os_sem_create(&hif->uart_atcmd_sem, 0);
    if (err)
        return err;

    err = tls_os_timer_create(&hif->tx_timer,
            tls_hostif_tx_timeout,
            hif,
            60*HZ,  /* 60 seconds */
            TRUE,
            NULL);

    if (!err)
        tls_os_timer_start(hif->tx_timer); 

#if TLS_CONFIG_UART || TLS_CONFIG_HS_SPI
    err = tls_hostif_task_init();
#endif


    return err; 
}

#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR
static void tls_hostif_set_recvmit(int socket_num, struct tls_uart_net_msg * precvmit)
{
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
	TLS_DBGPRT_INFO("socket_num=%d, precvmit=0x%x\n",socket_num, (u32)precvmit);
	sockrecvmit[socket_num-1] = precvmit;
#else
	sockrecvmit[socket_num-LWIP_SOCKET_OFFSET] = precvmit;
#endif
}

static void alloc_recvmit(int socket_num)
{

	struct tls_uart_net_msg * precvmit = tls_hostif_get_recvmit(socket_num);
	if(precvmit != NULL)
		return;
	precvmit = tls_mem_alloc(sizeof(struct tls_uart_net_msg));
	if(precvmit == NULL)
		return;

	dl_list_init(&precvmit->tx_msg_pending_list);
	tls_hostif_set_recvmit(socket_num, precvmit);
}

static void free_recvmit(int socket_num)
{
	struct tls_uart_net_msg * precvmit = tls_hostif_get_recvmit(socket_num);
	tls_uart_net_buf_t *net_buf;
	if(precvmit == NULL)
	return;

	dl_list_for_each(net_buf,&precvmit->tx_msg_pending_list,struct tls_uart_net_buf,list)
	{
		if (net_buf->pbuf)
		{
			pbuf_free(net_buf->pbuf);
			net_buf->buflen = 0;
			net_buf->offset = 0;
			net_buf->buf = NULL;
		}
	}
	tls_mem_free(precvmit);
	precvmit = NULL;
	tls_hostif_set_recvmit(socket_num, precvmit);
}
#else
static void tls_hostif_set_recvmit(int socket_num, struct tls_uart_circ_buf * precvmit)
{
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
	TLS_DBGPRT_INFO("socket_num=%d, precvmit=0x%x\n",socket_num, (u32)precvmit);
	sockrecvmit[socket_num-1] = precvmit;
#else
	sockrecvmit[socket_num-LWIP_SOCKET_OFFSET] = precvmit;
#endif
}

static void alloc_recvmit(int socket_num)
{
	char * buf;
	struct tls_uart_circ_buf * precvmit = tls_hostif_get_recvmit(socket_num);
	if(precvmit != NULL)
		return;
	precvmit = tls_mem_alloc(sizeof(struct tls_uart_circ_buf));
	if(precvmit == NULL)
		return;
	memset(precvmit, 0, sizeof(struct tls_uart_circ_buf));
	buf = tls_mem_alloc(TLS_SOCKET_RECV_BUF_SIZE);
	if(buf == NULL)
	{
		tls_mem_free(precvmit);
		precvmit = NULL;
		tls_hostif_set_recvmit(socket_num, precvmit);
		return;
	}
	precvmit->buf = (u8 *)buf;
	tls_hostif_set_recvmit(socket_num, precvmit);
}

static void free_recvmit(int socket_num)
{
	struct tls_uart_circ_buf * precvmit = tls_hostif_get_recvmit(socket_num);
	if(precvmit == NULL)
	return;
	if(precvmit->buf != NULL)
	tls_mem_free((void *)precvmit->buf);
	tls_mem_free(precvmit);
	precvmit = NULL;
	tls_hostif_set_recvmit(socket_num, precvmit);
}
#endif

int tls_hostif_recv_data(struct tls_hostif_tx_msg *tx_msg) 
{
    struct tls_hostif *hif = &g_hostif;

    if (hif->hostif_mode == HOSTIF_MODE_UART0) {
        if(hif->uart_send_tx_msg_callback != NULL)
            hif->uart_send_tx_msg_callback(HOSTIF_MODE_UART0, tx_msg, FALSE);
    } else if ((hif->hostif_mode == HOSTIF_MODE_UART1_LS) ||
            (hif->hostif_mode == HOSTIF_MODE_UART1_HS)) {
        if(hif->uart_send_tx_msg_callback != NULL)
            hif->uart_send_tx_msg_callback(hif->hostif_mode, tx_msg, FALSE);
    } else {
        /* HSPI */
        if(hif->hspi_send_tx_msg_callback != NULL)
            hif->hspi_send_tx_msg_callback(HOSTIF_MODE_HSPI, tx_msg, FALSE);
    }

    return 0; 
}

int tls_hostif_send_data(struct tls_hostif_socket_info *skt_info, 
        char *buf, u32 buflen)
{
    int err = 0;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    if(skt_info->socket)
        err = tls_socket_send(skt_info->socket, buf, buflen);
    else if(skt_info->proto == 1)//udp
        err = tls_socket_udp_sendto(skt_info->local_port, (u8 *)(&skt_info->remote_ip), skt_info->remote_port, buf, buflen);
#else
	if(FD_ISSET(skt_info->socket, &fdatsockets))
	{
		struct tls_skt_status_t *skt_status = 0;
		struct tls_skt_status_ext_t *ext;
		u32 skt_len;
		int n;
		skt_len = sizeof(struct tls_skt_status_ext_t) * 5 +
	                        sizeof(u32);
		skt_status = (struct tls_skt_status_t *)
	                    tls_mem_alloc(skt_len);
		if(skt_status == NULL)
			return ERR_MEM;
		memset(skt_status, 0, skt_len);
		err = tls_cmd_get_socket_status(skt_info->socket, (u8 *)skt_status, skt_len);
		if(err || skt_status->socket_cnt < 1)
			return ERR_ARG;
		ext = &skt_status->skts_ext[0];
		if(ext->protocol == SOCKET_PROTO_UDP)
		{
			err = sendto(skt_info->socket, buf, buflen, 0, &sock_cmdp_addrs[skt_info->socket - LWIP_SOCKET_OFFSET], (socklen_t)sizeof(struct sockaddr));
		}
		else if(skt_status->socket_cnt > 1)
		{
			for(n = 1; n < skt_status->socket_cnt; n++)
			{
				ext = &skt_status->skts_ext[n];
				err = send(ext->socket, buf, buflen, 0);
				if(err < 0)
					break;
			}
		}
		else
		{
			err = send(skt_info->socket, buf, buflen, 0);
		}
	}
    else if(skt_info->proto == 1)//udp
    {
        //TODO
    }
#endif
    
    return err;
}

static void hostif_default_socket_setup(void *ptmr, void *parg)
{
    tls_hostif_close_default_socket();
	default_socket = 0;
	tls_hostif_create_default_socket();
}

static tls_os_timer_t *default_sock_tmr = NULL;
static void hostif_default_socket_create_tmr(int ticks)
{
    tls_os_status_t err;
    if(default_sock_tmr != NULL)
    {
        tls_os_timer_change(default_sock_tmr, ticks);
        return;
    }
    err = tls_os_timer_create(&default_sock_tmr,
            hostif_default_socket_setup,
            (void *)0,
            HZ/100,  /* 10 ms */
            FALSE,
            NULL);

    if (!err)
        tls_os_timer_start(default_sock_tmr); 
}

static void hostif_default_socket_stop_tmr()
{
	if(default_sock_tmr != NULL)
	{
       	tls_os_timer_stop(default_sock_tmr); 
    	}
	tls_hostif_close_default_socket();
}

static void hostif_default_socket_err(u8 skt_num, s8 err)
{
	if (tls_cmd_get_auto_mode() && default_socket == skt_num){
		if(default_sock_tmr != NULL)
		{
			tls_os_timer_change(default_sock_tmr, 10*HZ);
		}
	}
}
#if 0
static s8  hostif_socket_rpt_handle(void* arg){
	int err1 = 0;
	struct tls_uart_circ_buf * precvmit  = NULL;
	char *cmdind_buf = NULL;
	u32 cmdind_size = 0, ret = 0, maxsize;
	struct tls_hostif_tx_msg *tx_msg = (struct tls_hostif_tx_msg *)arg;
	struct tls_hostif *hif = tls_get_hostif();
	cmdind_size = tx_msg->u.msg_event.buflen;
	cmdind_buf = tx_msg->u.msg_event.buf;
	maxsize = tx_msg->time;
	precvmit = tls_hostif_get_recvmit(tx_msg->offset);
	if(precvmit)
	{
		ret = CIRC_CNT(precvmit->head, precvmit->tail, TLS_SOCKET_RECV_BUF_SIZE);
		if(ret < maxsize)
			maxsize = ret;
	}	
	else{
		err1 = -1;
		goto err;
	}
	while(1)
	{
		ret = CIRC_CNT_TO_END(precvmit->head, precvmit->tail, TLS_SOCKET_RECV_BUF_SIZE);
		if(ret == 0)
		{
			break;
		}
		if(ret > maxsize)
		{
			ret = maxsize;
		}
		memcpy(cmdind_buf + cmdind_size,(char *)(precvmit->buf+precvmit->tail),ret);
		cmdind_size += ret;
		precvmit->tail = (precvmit->tail + ret) & (TLS_SOCKET_RECV_BUF_SIZE - 1);
		maxsize -= ret;
		if(maxsize <= 0)
			break;
	}
	cmdind_buf[cmdind_size] = '\0';
	
	err1 = tls_hostif_process_cmdrsp(hif->hostif_mode, cmdind_buf, cmdind_size);
err:
		if(err1)
			tls_mem_free(cmdind_buf);
		tls_mem_free(tx_msg);
		return err1;
}
#endif
static s8 hostif_socket_rpt(u8 skt_num, u16 datalen, u8 *ipaddr, u16 port, s8 err)
{
	#undef CMDIND_BUF_SIZE
	#define CMDIND_BUF_SIZE 600//128
	char *cmdind_buf = NULL;
	u32 cmdind_size = 0;
	struct tls_hostif *hif = tls_get_hostif();

	if (hif->rptmode){		
		cmdind_buf = tls_mem_alloc(CMDIND_BUF_SIZE);
		if(cmdind_buf){
			cmdind_size = sprintf(cmdind_buf,"+SKTRPT=%d,%d,%d.%d.%d.%d,%d\r\n\r\n",skt_num,datalen,ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3],port);
			if (tls_hostif_process_cmdrsp(hif->hostif_mode, cmdind_buf, cmdind_size))
			{
				tls_mem_free(cmdind_buf);
			}
		}
	}
	return ERR_OK;
}

static s8  hostif_socket_recv(u8 skt_num, struct pbuf *p, s8 err)
{
    struct tls_hostif_tx_msg *tx_msg;
    u8 state;
    struct tls_skt_status_ext_t skt_ext;


    tx_msg = tls_hostif_get_tx_msg();
    err = tls_cmd_get_socket_state(skt_num, &state, &skt_ext);
    if (tx_msg == NULL || err || NETCONN_STATE_NONE == state) {
        pbuf_free(p);
        return ERR_OK;
    } else {
        tx_msg->type = HOSTIF_TX_MSG_TYPE_TCP;
        tx_msg->u.msg_tcp.p = p;
        tx_msg->u.msg_tcp.sock = skt_num;
        if(skt_ext.protocol == SOCKET_PROTO_UDP)
        {
            tx_msg->type = HOSTIF_TX_MSG_TYPE_UDP;
            tx_msg->u.msg_udp.p = p;
            tx_msg->u.msg_udp.sock = skt_num;
            tx_msg->u.msg_udp.port = skt_ext.remote_port;
            tx_msg->u.msg_udp.localport = skt_ext.local_port;
			MEMCPY(ip_2_ip4(&tx_msg->u.msg_udp.ip_addr), &skt_ext.host_ipaddr[0], 4); 
        }
        tx_msg->offset = 0;
        tx_msg->time = tls_os_get_time();
    }
//TLS_DBGPRT_INFO("tx_msg->u.msg_tcp.p=0x%x\n", tx_msg->u.msg_tcp.p);
    tls_hostif_recv_data(tx_msg);
    return ERR_OK;
}
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
static void  hostif_default_socket_state_changed(u8 skt_num, u8 event, u8 state)
{
    //cmd_set_uart1_mode_callback callback;
    TLS_DBGPRT_INFO("event=%d, state=%d\n", event, state);
    switch (event) {
        case NET_EVENT_TCP_JOINED:
            alloc_recvmit(skt_num);
            break;
        case NET_EVENT_TCP_DISCONNECT:               
            free_recvmit(skt_num);
            break; 
        case NET_EVENT_TCP_CONNECTED:
            alloc_recvmit(skt_num);
            break;
        case NET_EVENT_TCP_CONNECT_FAILED:
            free_recvmit(skt_num);
            break;
        case NET_EVENT_UDP_START:
            alloc_recvmit(skt_num);
        default:
            break;
    }
    //callback = tls_cmd_get_set_uart1_mode();
    //if(callback!=NULL)
    //    callback(UART_TRANS_MODE);
}

struct tls_socket_desc skt_desc_def;
#endif
int tls_hostif_create_default_socket(void)
{
    int ret = 0;
#if !TLS_CONFIG_CMD_USE_RAW_SOCKET
	struct tls_cmd_socket_t skt_cmd;
#endif
    struct tls_socket_cfg *skt_cfg = tls_cmd_get_socket_cfg();
    if (tls_cmd_get_auto_mode() && (0 == tls_wifi_get_oneshot_flag())){
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
		memset(&skt_desc_def, 0, sizeof(struct tls_socket_desc));
		skt_desc_def.cs_mode = skt_cfg->client ? SOCKET_CS_MODE_CLIENT : SOCKET_CS_MODE_SERVER;
		MEMCPY(ip_2_ip4(&skt_desc_def.ip_addr), skt_cfg->ip_addr, sizeof(ip_addr_t));
		skt_desc_def.localport = 0;//skt_cfg->port;
		skt_desc_def.port = skt_cfg->port;
		skt_desc_def.protocol = (enum tls_socket_protocol)skt_cfg->proto;
		skt_desc_def.timeout = skt_cfg->timeout;
		skt_desc_def.recvf = hostif_socket_recv;
		skt_desc_def.errf = hostif_default_socket_err;
		skt_desc_def.state_changed = hostif_default_socket_state_changed;
		skt_desc_def.recvwithipf = hostif_socket_rpt;
		if (default_socket == 0){
			ret = tls_socket_create(&skt_desc_def);
			if (ret < 0){
				//hostif_default_socket_setup((void *)0, (void *)0);
				hostif_default_socket_create_tmr(100);
			}else{
				TLS_DBGPRT_INFO("create socket:%d\n", ret);
				default_socket = ret;
			}
		}				
#else
		skt_cmd.client = skt_cfg->client;
		skt_cmd.proto = skt_cfg->proto;
		memcpy(skt_cmd.ip_addr, skt_cfg->ip_addr, 4);
		skt_cmd.port = skt_cfg->port;
		skt_cmd.localport = skt_cfg->port;
		skt_cmd.host_len = skt_cfg->host_len;
		memcpy(skt_cmd.host_name, skt_cfg->host, 32);
		skt_cmd.timeout = skt_cfg->timeout;
		if (default_socket == 0){
			ret = tls_cmd_create_socket(&skt_cmd, CMD_MODE_UART1_ATCMD);
			if(ret < 0)
			{
				hostif_default_socket_create_tmr(100);
			}
			else
			{
				TLS_DBGPRT_INFO("create socket:%d\n", ret);
				default_socket = ret;
			}
		}
#endif
    }
	return ret;
}

int tls_hostif_close_default_socket(void)
{
	int ret = 0;
	
	if (tls_cmd_get_auto_mode() && ((0 == tls_wifi_get_oneshot_flag())||default_socket)){
		if(!tls_cmd_close_socket(default_socket))
		{
	  		default_socket = 0;
		}
	}
	return ret;
}
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
static void  hostif_socket_state_changed_ATCMD(u8 skt_num, u8 event, u8 state)
{
    struct tls_hostif *hif = tls_get_hostif();
    TLS_DBGPRT_INFO("event=%d, state=%d\n", event, state);
    switch (event) {
        case NET_EVENT_TCP_JOINED:
            alloc_recvmit(skt_num);
            tls_hostif_send_event_tcp_join(skt_num);
            break;
        case NET_EVENT_TCP_DISCONNECT:               
            free_recvmit(skt_num);
            tls_hostif_send_event_tcp_dis(skt_num);
            break; 
        case NET_EVENT_TCP_CONNECTED:
            alloc_recvmit(skt_num);
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_SKCT);
            tls_os_sem_release(hif->uart_atcmd_sem);
            break;
        case NET_EVENT_TCP_CONNECT_FAILED:
            free_recvmit(skt_num);
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_SKCT);
            tls_os_sem_release(hif->uart_atcmd_sem);
            break;
        case NET_EVENT_UDP_START:
            alloc_recvmit(skt_num);
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_SKCT);
            tls_os_sem_release(hif->uart_atcmd_sem);
            break;
        case NET_EVENT_UDP_START_FAILED:
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_SKCT);
            tls_os_sem_release(hif->uart_atcmd_sem);
            break;
        default:
            break;
    }
}

static void  hostif_socket_state_changed_RICMD(u8 skt_num, u8 event, u8 state)
{

	TLS_DBGPRT_INFO("event=%d, state=%d\n", event, state);
    switch (event) {
        case NET_EVENT_TCP_JOINED:
            tls_hostif_send_event_tcp_join(skt_num);
            break;
        case NET_EVENT_TCP_DISCONNECT:
            tls_hostif_send_event_tcp_dis(skt_num);
            break; 
        case NET_EVENT_TCP_CONNECTED:
            tls_hostif_send_event_tcp_conn(skt_num, 1);
            break;
        case NET_EVENT_TCP_CONNECT_FAILED:
            tls_hostif_send_event_tcp_conn(skt_num, 0);
            break;
        default:
            break;
    }
}

struct tls_socket_desc skt_desc;

#else
static void sock_recv_timeout_handler(void * arg)
{
	int maxsock = 0;
	int ret;
	int sock;
	fd_set  rdset;
	struct timeval timeout;
	struct pbuf * p = NULL;
	int optval;
    int optlen = sizeof(int);
	struct sockaddr_in from;
	socklen_t fromlen = (socklen_t)sizeof(struct sockaddr);

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	FD_ZERO(&rdset);
	for(sock = LWIP_SOCKET_OFFSET; sock < (MEMP_NUM_NETCONN + LWIP_SOCKET_OFFSET); sock++)
	{
		if(FD_ISSET(sock, &fdatsockets))
		{
			if(maxsock < sock)
				maxsock = sock;
			FD_SET(sock, &rdset);
			maxsock ++;
		}
	}
	if(maxsock == 0)
		goto exit;
	ret = select(maxsock, &rdset, NULL, NULL, &timeout);
	//printf("select maxsock %d\n", maxsock);
	if(ret <= 0)
		goto exit;
	for(sock = LWIP_SOCKET_OFFSET; sock < (MEMP_NUM_NETCONN + LWIP_SOCKET_OFFSET); sock++)
	{
		if(FD_ISSET(sock, &fdatsockets) && FD_ISSET(sock, &rdset))
		{
			getsockopt(sock, SOL_SOCKET, SO_TYPE, &optval, (socklen_t *)&optlen);
			if(optval == SOCK_STREAM)//tcp
			{
				ret = getsockopt(sock, SOL_SOCKET, SO_ACCEPTCONN, &optval, (socklen_t *)&optlen);
				//printf("get socket %d SO_ACCEPTCONN optval %d\n", sock, optval);
				if(ret)
					optval = 0;
			}
			//printf("sock %d, optval = %d\n", sock, optval);
			if (1 != optval) //is udp or tcp client
			{
				p = pbuf_alloc(PBUF_RAW, TLS_SOCKET_RECV_BUF_SIZE, PBUF_RAM);
				ret = recvfrom(sock, p->payload, TLS_SOCKET_RECV_BUF_SIZE, 0, (struct sockaddr *)&from, &fromlen);
				if(ret <= 0)
				{
					pbuf_free(p);
					hostif_default_socket_err(sock, ERR_CLSD);
					free_recvmit(sock);
					FD_CLR(sock, &fdatsockets);
					closesocket(sock);
				}
				p->tot_len = p->len = ret;
				source_ip = from.sin_addr.s_addr;
				hostif_socket_rpt(sock, ret, (u8*)&from.sin_addr.s_addr, htons(from.sin_port), ERR_OK);
				hostif_socket_recv(sock, p, 0);
			}
			else
			{
				optval = sock_cmdp_timeouts[sock-LWIP_SOCKET_OFFSET];
				//printf("get socket %d TCP_KEEPIDLE optval %d\n", sock, optval);
				ret = accept(sock, NULL, NULL);
				if (ret < 0) {
					goto exit;
				}
				FD_SET(ret, &fdatsockets);
				alloc_recvmit(ret);
				if(optval > 0)
				{
					setsockopt(ret, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen);
					optval = 0;
					setsockopt(ret, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen);
					optval = 1;
					setsockopt(ret, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
				}
			}
		}
	}
exit:
	tls_wl_task_add_timeout(&wl_task_param_hostif, SOCK_RECV_TIMEOUT, sock_recv_timeout_handler, NULL);
}
#endif
/* 
 * @return: value 1-20 is socket num
 *          value 0 : socket is not availble
 *          value <0 is error
 * */
int tls_cmd_create_socket(struct tls_cmd_socket_t *skt,
        enum tls_cmd_mode cmd_mode)
{
    int ret = 0;
#if !TLS_CONFIG_CMD_USE_RAW_SOCKET
	int sock;
	struct sockaddr_in *sock_addr = NULL;
	struct sockaddr_in local_addr;
//	int optval;
//  int optlen = sizeof(int);
#endif
    TLS_DBGPRT_INFO("=====>\n");
    TLS_DBGPRT_INFO("skt proto = %d\n", skt->proto);
    TLS_DBGPRT_INFO("skt client = %d\n", skt->client);
    TLS_DBGPRT_INFO("skt port = %d\n", skt->port);
    TLS_DBGPRT_INFO("skt ipaddr = 0x%x\n", get_unaligned_le32(skt->ip_addr));
    TLS_DBGPRT_INFO("cmd_mode = %d\n", cmd_mode);
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    memset(&skt_desc, 0, sizeof(struct tls_socket_desc));
    skt_desc.cs_mode = skt->client ? SOCKET_CS_MODE_CLIENT : SOCKET_CS_MODE_SERVER;
    MEMCPY(skt_desc.host_name, skt->host_name, 32);
    skt_desc.host_len = skt->host_len;
    MEMCPY(ip_2_ip4(&skt_desc.ip_addr), skt->ip_addr, 4);
    skt_desc.localport = skt->localport;
    skt_desc.port = skt->port;
    skt_desc.protocol = (enum tls_socket_protocol)skt->proto;
    skt_desc.timeout = skt->timeout;
    skt_desc.recvf = hostif_socket_recv;
	skt_desc.recvwithipf = hostif_socket_rpt;
    if ((cmd_mode == CMD_MODE_UART0_ATCMD) ||
        (cmd_mode == CMD_MODE_UART1_ATCMD) 
#if TLS_CONFIG_RMMS
        || (cmd_mode == CMD_MODE_RMMS_ATCMD)
#endif
       )
    {
        skt_desc.state_changed = hostif_socket_state_changed_ATCMD;
        TLS_DBGPRT_INFO("skt_desc.state_changed: 0x%x\n", (u32)skt_desc.state_changed);
    }
    else
    {
        skt_desc.state_changed = hostif_socket_state_changed_RICMD;
        TLS_DBGPRT_INFO("==>skt_desc.state_changed: 0x%x\n", (u32)skt_desc.state_changed);
    }
    ret = tls_socket_create(&skt_desc);
    if (ret <= 0)
        return -1;
    else 
        return ret;
#else
	if(SOCKET_PROTO_TCP == skt->proto)
	{
 		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	else
	{
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
    if(sock < 0)
    {
    	return -1;
    }
	
	if((skt->localport && (SOCKET_PROTO_UDP == skt->proto || skt->client)) || (SOCKET_PROTO_TCP == skt->proto && !skt->client))
	{
		memset(&local_addr, 0, sizeof(struct sockaddr));
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = IPADDR_ANY;
		if(SOCKET_PROTO_TCP == skt->proto && !skt->client)
		{
			local_addr.sin_port = htons(skt->port);
			//printf("bind port %d\n", skt->port);
		}
		else
		{
			local_addr.sin_port = htons(skt->localport);
			//printf("bind port %d\n", skt->localport);
		}
		ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
		if(ret < 0)
		{
			closesocket(sock);
			return ret;
		}
	}
	if(SOCKET_PROTO_UDP == skt->proto || skt->client)
	{
		sock_addr = (struct sockaddr_in *)&sock_cmdp_addrs[sock - LWIP_SOCKET_OFFSET];
		memset(sock_addr, 0, sizeof(struct sockaddr));
		sock_addr->sin_family = AF_INET;
		if(SOCKET_PROTO_UDP == skt->proto && !skt->client)
		{
			sock_addr->sin_addr.s_addr = IPADDR_BROADCAST;
		}
		else
		{
			sock_addr->sin_addr.s_addr = *((u32 *)skt->ip_addr);
		}
		sock_addr->sin_port = htons(skt->port);
		if(SOCKET_PROTO_TCP == skt->proto)
		{
			//printf("connect addr %x\n", sock_addr->sin_addr.s_addr);
			ret = connect(sock, (struct sockaddr *)sock_addr, sizeof(struct sockaddr));
		}
	}
	else
	{
		//printf("set socket %d TCP_KEEPIDLE timeout %d\n", sock, skt->timeout);
		sock_cmdp_timeouts[sock-LWIP_SOCKET_OFFSET] = skt->timeout;
		ret = listen(sock, 4);
	}
	if(ret < 0)
	{
		closesocket(sock);
		return ret;
	}
	//printf("cretea sock %d\n", sock);
	FD_SET(sock, &fdatsockets);
	alloc_recvmit(sock);
	tls_wl_task_untimeout(&wl_task_param_hostif, sock_recv_timeout_handler, NULL);
	tls_wl_task_add_timeout(&wl_task_param_hostif, SOCK_RECV_TIMEOUT, sock_recv_timeout_handler, NULL);
	return sock;
#endif
}


int tls_cmd_close_socket(u8 skt_num)
{
    int err;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    err = tls_socket_close(skt_num);
#else
	err = closesocket(skt_num);
	if(!err)
		FD_CLR(skt_num, &fdatsockets);
#endif
    if(!err)
        free_recvmit(skt_num);

    return err;
}
        
int tls_cmd_get_socket_status(u8 socket, u8 *buf, u32 bufsize)
{
    int err;
#if !TLS_CONFIG_CMD_USE_RAW_SOCKET
	struct tls_skt_status_t *skt_status;
    struct tls_skt_status_ext_t *skts_ext;
	struct sockaddr_in sock_addr;
	struct sockaddr_in * peer_addr = (struct sockaddr_in *)&sock_cmdp_addrs[socket - LWIP_SOCKET_OFFSET];
	int optval;
    int optlen = sizeof(int);
    int remain_len;
	int s;
	socklen_t namelen = sizeof(struct sockaddr_in);

    skt_status = (struct tls_skt_status_t *)buf;
    if (bufsize < sizeof(struct tls_skt_status_t))
	{
		TLS_DBGPRT_ERR("\nbufsize=%d\n",bufsize);
    	return ERR_VAL; 
	}
    memset(buf, 0, bufsize);

    remain_len = bufsize - sizeof(u32);
    skts_ext = (struct tls_skt_status_ext_t *)(buf + sizeof(u32));
	if(!FD_ISSET(socket, &fdatsockets) || getsockname(socket, (struct sockaddr *)&sock_addr, &namelen)){
		TLS_DBGPRT_ERR("\nskt num=%d\n",socket);
		skt_status->socket_cnt = 1;
		skts_ext->protocol = SOCKET_PROTO_TCP;
		skts_ext->status = NETCONN_STATE_NONE;
		skts_ext->socket = socket;
		skts_ext->host_ipaddr[0] = 0;
		skts_ext->host_ipaddr[1] = 0;
		skts_ext->host_ipaddr[2] = 0;
		skts_ext->host_ipaddr[3] = 0;
		return ERR_OK;
	}
	getsockopt(socket, SOL_SOCKET, SO_TYPE, &optval, (socklen_t *)&optlen);
    skts_ext->protocol = optval == SOCK_STREAM ? SOCKET_PROTO_TCP : SOCKET_PROTO_UDP;
	if(optval == SOCK_STREAM)//tcp
	{
		err = getsockopt(socket, SOL_SOCKET, SO_ACCEPTCONN, &optval, (socklen_t *)&optlen);
		if(err)
			optval = 0;
	}
	
    if (1 != optval) //is udp or tcp client
	{
        skt_status->socket_cnt = 1;
		skts_ext->socket = socket;
		skts_ext->status = NETCONN_STATE_CONNECTED;
		if(SOCK_DGRAM == optval)
		{
			err = 0;
		}
		else
		{
			namelen = sizeof(struct sockaddr_in);
			err = getpeername(socket, (struct sockaddr *)peer_addr, &namelen);
		}
		if(err)
		{
			memset(skts_ext->host_ipaddr, 0, 4);
			skts_ext->remote_port = 0;
			skts_ext->status = NETCONN_STATE_NONE;
		}
		else
		{
			MEMCPY(skts_ext->host_ipaddr, (u8 *)ip_2_ip4(&peer_addr->sin_addr), 4);
			skts_ext->remote_port = htons(peer_addr->sin_port);
		}
		skts_ext->local_port = htons(sock_addr.sin_port);
    } else {
    	struct tls_ethif * ethif;
		u16 listen_port;
		ethif = tls_netif_get_ethif();
        skt_status->socket_cnt = 1;
		skts_ext->socket = socket;
		skts_ext->status = NETCONN_STATE_WAITING;/* listen */
		MEMCPY(skts_ext->host_ipaddr, (char *)(ip_2_ip4(&ethif->ip_addr)), 4);
        skts_ext->remote_port = 0;
        skts_ext->local_port = htons(sock_addr.sin_port);
		listen_port = sock_addr.sin_port;
		remain_len -= sizeof(struct tls_skt_status_ext_t);
		
        for(s = LWIP_SOCKET_OFFSET; s < (MEMP_NUM_NETCONN + LWIP_SOCKET_OFFSET); s++)
        {
        	if(remain_len < sizeof(struct tls_skt_status_ext_t))
				break;
        	if(s == socket)
				continue;
			namelen = sizeof(struct sockaddr_in);
			err = getsockname(s, (struct sockaddr *)&sock_addr, &namelen);
			if(err)
				continue;
			getsockopt(s, SOL_SOCKET, SO_TYPE, &optval, (socklen_t *)&optlen);
			if(optval != SOCK_STREAM)
				continue;
			err = getsockopt(s, SOL_SOCKET, SO_ACCEPTCONN, &optval, (socklen_t *)&optlen);
			if(err || optval)
				continue;
			//printf("listen_port %x, sock_addr.sin_port %x\n", listen_port, sock_addr.sin_port);
			if(listen_port != sock_addr.sin_port)
				continue;
			skts_ext++;
			skts_ext->status = NETCONN_STATE_CONNECTED; /* connect */
            skt_status->socket_cnt++;
			namelen = sizeof(struct sockaddr_in);
			err = getpeername(s, (struct sockaddr *)peer_addr, &namelen);
			if(err)
			{
				memset(skts_ext->host_ipaddr, 0, 4);
				skts_ext->remote_port = 0;
				skts_ext->status = NETCONN_STATE_NONE;
			}
			else
			{
				MEMCPY(skts_ext->host_ipaddr, (u8 *)ip_2_ip4(&peer_addr->sin_addr), 4);
				skts_ext->remote_port = htons(peer_addr->sin_port);
			}
            skts_ext->local_port = htons(listen_port);
            skts_ext->socket = s;
            skts_ext->protocol = SOCKET_PROTO_TCP;
			remain_len -= sizeof(struct tls_skt_status_ext_t);
        }
        
    }
    err = 0;
#else
    err = tls_socket_get_status(socket, buf, bufsize);
#endif
    return err;
}

int tls_cmd_get_socket_state(u8 socket, u8 * state, struct tls_skt_status_ext_t *skt_ext)
{
	struct tls_skt_status_t *skt_status = 0;
	struct tls_skt_status_ext_t *ext;
	int err;
	u32 buflen;
	buflen = sizeof(struct tls_skt_status_ext_t) * 5 +
                        sizeof(u32);
	skt_status = (struct tls_skt_status_t *)
                    tls_mem_alloc(buflen);
	if(skt_status == NULL)
		return NETCONN_STATE_NONE;
	memset(skt_status, 0, buflen);
	err = tls_cmd_get_socket_status(socket, (u8 *)skt_status, buflen);
	//TLS_DBGPRT_INFO("err=%d\n", err);
	if(err)
	{
		*state = NETCONN_STATE_NONE;
	}
	else
	{
		ext = &skt_status->skts_ext[0];
		if(skt_ext != NULL)
			MEMCPY(skt_ext, ext, sizeof(struct tls_skt_status_ext_t));
		*state = ext->status;
	}
	tls_mem_free(skt_status);
	//TLS_DBGPRT_INFO("state=%d\n", *state);
	return err;
}

u8 tls_cmd_get_default_socket(void)
{
    return default_socket;
}

int tls_cmd_set_default_socket(u8 socket)
{
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    if (socket < 1 || socket > 20)
        return -1;
#endif
    default_socket = socket;
    return 0;
}

#endif //TLS_CONFIG_SOCKET_RAW

static void tls_hostif_wjoin_success(void)
{
    struct tls_hostif *hif = tls_get_hostif();
    if (hif->last_join) {
        hif->last_join = 0;
        if ((hif->last_join_cmd_mode == CMD_MODE_HSPI_RICMD) ||
                (hif->last_join_cmd_mode == CMD_MODE_UART1_RICMD)){ 
            tls_hostif_send_event_wjoin_success(); 
        }
        else if (hif->last_join_cmd_mode == CMD_MODE_UART1_ATCMD) {
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        } else if (hif->last_join_cmd_mode == CMD_MODE_UART0_ATCMD) {
        	hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        } 
#if TLS_CONFIG_RMMS
        else if (hif->last_join_cmd_mode == CMD_MODE_RMMS_ATCMD) {
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        }
#endif
        else
            ;
    } 
}

static void tls_hostif_wjoin_failed(void)
{ 
    struct tls_hostif *hif = tls_get_hostif();
    if (hif->last_join) {
        if ((hif->last_join_cmd_mode == CMD_MODE_HSPI_RICMD) ||
                (hif->last_join_cmd_mode == CMD_MODE_UART1_RICMD)){
            tls_hostif_send_event_wjoin_failed();
	 	}
        else if (hif->last_join_cmd_mode == CMD_MODE_UART1_ATCMD) {
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        } else if (hif->last_join_cmd_mode == CMD_MODE_UART0_ATCMD) {
        	hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        } 
#if TLS_CONFIG_RMMS
        else if (hif->last_join_cmd_mode == CMD_MODE_RMMS_ATCMD) {
			hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_WJOIN);
            tls_os_sem_release(hif->uart_atcmd_sem); 
        } 
#endif
        else
            ;
        hif->last_join = 0;
    }
}

static void tls_hostif_net_status_changed(u8 status)
{
	struct tls_hostif *hif = tls_get_hostif();
    switch(status)
    {
        case NETIF_WIFI_JOIN_FAILED:
            if (tls_cmd_get_net_up())/* è§£å³å ç½ååå ç½åºç°join failedä¹åè¿æ¥æ¾ç¤ºä¸æ­å¼çé®é¢?*/
            {
                tls_cmd_set_net_up(0);
                tls_hostif_send_event_linkdown(); 
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
                hostif_default_socket_stop_tmr();
#endif //TLS_CONFIG_SOCKET_RAW
            }
            tls_hostif_wjoin_failed();
            break;
        case NETIF_WIFI_JOIN_SUCCESS:
	case NETIF_WIFI_SOFTAP_SUCCESS:

            tls_cmd_set_net_up(1);
            tls_hostif_wjoin_success();
            break;
        case NETIF_IP_NET_UP:
	case NETIF_IP_NET2_UP:

            tls_hostif_send_event_linkup(); 
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
            hostif_default_socket_create_tmr(1);
#endif //TLS_CONFIG_SOCKET_RAW
#if TLS_CONFIG_RMMS
			tls_rmms_start();
#endif
            break;
        case NETIF_WIFI_DISCONNECTED:
			{

				if (hif->last_join) 
				{		
					hif->last_join = 0;
				}
			}
        case NETIF_WIFI_SOFTAP_CLOSED:
            tls_cmd_set_net_up(0);
            tls_hostif_send_event_linkdown(); 
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
            hostif_default_socket_stop_tmr();
#endif //TLS_CONFIG_SOCKET_RAW
            break;
        default:
            break;
    }
	
	return;
}
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
int tls_hostif_set_net_status_callback(void)
{
    return tls_netif_add_status_event(tls_hostif_net_status_changed);
}
#endif
#if TLS_CONFIG_HTTP_CLIENT_TASK
u8  pSession_flag=0;
void tls_hostif_http_client_recv_callback(HTTP_SESSION_HANDLE pSession, CHAR * data, u32 totallen, u32 datalen)
{
	#undef CMDIND_BUF_SIZE
	#define CMDIND_BUF_SIZE 64
	char *cmdind_buf = NULL;
	int err1 = 0;
	u32 cmdind_size = 0;

	struct tls_hostif *hif = tls_get_hostif();
	if ((hif->rptmode) || (HOSTIF_MODE_HSPI == hif->hostif_mode))
	{	
        if(pSession_flag==0)
        {
            cmdind_buf = tls_mem_alloc(CMDIND_BUF_SIZE);
    		if (cmdind_buf)
    		{
			    cmdind_size = sprintf(cmdind_buf,"+HTTPCRPT=%d,%d\r\n\r\n",pSession, totallen);
    			err1 = tls_hostif_process_cmdrsp(hif->hostif_mode, cmdind_buf, cmdind_size);
    			if (err1)
    			{
    				tls_mem_free(cmdind_buf);
    				cmdind_buf = NULL;
    				tls_mem_free(data);
    				return;
    			}
                pSession_flag=1;
            }else
                tls_mem_free(data);
        }
		err1 = tls_hostif_process_cmdrsp(hif->hostif_mode, (char *)data, datalen);
		if (err1)
		{
			tls_mem_free(data);
		}
	}
	else
		tls_mem_free(data);
}

void tls_hostif_http_client_err_callback(HTTP_SESSION_HANDLE pSession, int err)
{
	#undef CMDIND_BUF_SIZE
	#define CMDIND_BUF_SIZE 64
	char *cmdind_buf = NULL;
	int err1 = 0;
	u32 cmdind_size = 0;

	struct tls_hostif *hif = tls_get_hostif();

	if ((hif->rptmode) || (HOSTIF_MODE_HSPI == hif->hostif_mode))
	{		
		cmdind_buf = tls_mem_alloc(CMDIND_BUF_SIZE);
		if (cmdind_buf)
		{
			cmdind_size = sprintf(cmdind_buf,"+HTTPCERRRPT=%d,%d\r\n\r\n",pSession, err);
			err1 = tls_hostif_process_cmdrsp(hif->hostif_mode, cmdind_buf, cmdind_size);
			if (err1)
			{
				tls_mem_free(cmdind_buf);
				cmdind_buf = NULL;
			}
		}
	}
}

#endif

int atcmd_filter_quotation(u8 **keyInfo, u8 *inbuf)
{
    u8 len=strlen((char *)inbuf);
    int i;
    if (*inbuf == '"'){/* argument such as  "xxxx" */
        inbuf++; /* skip 1st <"> */
        len -= 1;
		*keyInfo = inbuf;
        if((*(inbuf+len-1) == '"') && (*(inbuf+len) == '\0')){
            *(inbuf + len-1) = '\0';
            len -= 1;
            for(i=0;i<len;i++){
                if(inbuf[i] == '\\'){
                    len -= 1;
                    memcpy(inbuf+i, inbuf+i+1, len-i);
                }
            }
            inbuf[len] = '\0';
        }else
            return 1;
	}else{
		*keyInfo = inbuf;
	}

	return 0;
}

#if 1

int z_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    tls_cmd_reset_sys();
    return 0;
}
int e_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    struct tls_hostif *hif = tls_get_hostif();
    if (hif->uart_insdisp)
        hif->uart_insdisp = 0;
    else
        hif->uart_insdisp = 1;
    return 0; 
}

int ents_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0;
    struct tls_cmd_ps_t ps;

	if (cmd->ps.ps_type > 2 || (cmd->ps.wake_type > 1))
	{
		return -CMD_ERR_INV_PARAMS;
	}

	if ((cmd->ps.ps_type == 1 || cmd->ps.ps_type == 2) 
		&& (((cmd->ps.wake_type == 1) && (cmd->ps.wake_time > 65535 || cmd->ps.wake_time < 1000))
		|| (cmd->ps.delay_time < 100 || cmd->ps.delay_time > 10000)))
	{
		return -CMD_ERR_INV_PARAMS;
	}
	
    ps.ps_type=cmd->ps.ps_type;
    ps.wake_type = cmd->ps.wake_type;
    ps.delay_time=cmd->ps.delay_time;
    ps.wake_time = cmd->ps.wake_time;
    ret = tls_cmd_ps(&ps);
    return ret ? -CMD_ERR_OPS : 0;
}

int rstf_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
    ret = tls_cmd_reset_flash();
    if(ret)
        return -CMD_ERR_OPS;
//    tls_cmd_reset_sys();
    return 0;
}

int pmtf_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
    ret = tls_cmd_pmtf();
    return ret ? -CMD_ERR_OPS : 0;
}

int ioc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

int wjoin_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int err; 
    struct tls_hostif *hif = tls_get_hostif();
    struct tls_curr_bss_t *bss;
	
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_WJOIN);

    err = tls_cmd_join(cmd->wjoin.mode, NULL);

	/*clear ip net status for bug 1465*/
	tls_netif_set_status(0);
    if(cmd->wjoin.mode!=CMD_MODE_HSPI_RICMD && cmd->wjoin.mode!=CMD_MODE_UART1_RICMD)
    {
        if (err == CMD_ERR_OK) {
sem_acquire:
            /* waiting for ever: infact 20s, determind by wpa_supplicant_connect_timeout */
            err = tls_os_sem_acquire(hif->uart_atcmd_sem, 20 * HZ);
            if (err) 
            {
				if (hif->last_join)
				{
					hif->last_join = 0;
				}
                return -CMD_ERR_JOIN; 
            }
            else {
                if(!(hif->uart_atcmd_bits & (1 << UART_ATCMD_BIT_WJOIN)))
                {
                    goto sem_acquire;
                }
                if (tls_cmd_get_net_up()) 
                {
                    bss = tls_mem_alloc(sizeof(struct tls_curr_bss_t));
                    if(!bss)
                    {
                        return -CMD_ERR_MEM;
                    }
                    memset(bss, 0, sizeof(struct tls_curr_bss_t));

                    tls_wifi_get_current_bss(bss);
                    MEMCPY(cmdrsp->join.bssid, bss->bssid, ETH_ALEN);
                    tls_cmd_get_wireless_mode(&cmdrsp->join.type);					
                    cmdrsp->join.encrypt = bss->encryptype;
                    cmdrsp->join.ssid_len = bss->ssid_len;
                    MEMCPY(cmdrsp->join.ssid, bss->ssid, bss->ssid_len);
                    cmdrsp->join.channel = bss->channel;
                    cmdrsp->join.rssi = bss->rssi;
                    tls_mem_free(bss);
                } else {
                    return -CMD_ERR_JOIN; 
                }
            }
        }
        else 
        {
        	if (err < 0)
				return err;
			else
	            return -err;
        }
    }
    else
    {
        cmdrsp->join.result = 0;
    }

    return 0;
}

int wleav_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
    if (cmd->wreg.region == 2){
    	ret = tls_cmd_disconnect_network(IEEE80211_MODE_AP);
    }else{
    	ret = tls_cmd_disconnect_network(IEEE80211_MODE_INFRA);    	
    }
    return ret ? -CMD_ERR_FLASH : 0;
}

int wscan_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
	u32 time, offset = 0;
	u32 expiredtime = 0;
	int i = 0;
	
    struct tls_hostif *hif = tls_get_hostif();

	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_WSCAN);
    ret = tls_cmd_scan_by_param(cmd->scanparam.mode, cmd->scanparam.chlist, cmd->scanparam.scantimes, cmd->scanparam.switchinterval, cmd->scanparam.scantype);
    if(ret){
        return -ret;
    }
    if(cmd->wscan.mode!=CMD_MODE_HSPI_RICMD && cmd->wscan.mode!=CMD_MODE_UART1_RICMD)
    {
    	/*calculate timeout value according to channellist scantimes,switchinterval, 
    	   plus 2second as gap to protect*/
    	time = 0;
    	for (i = 0; i < 14; i++)
    	{
    		if (cmd->scanparam.chlist & (1<<i))
    		{
    			time++;
    		}
    	}
		expiredtime = (time == 0)?14:time;
		expiredtime *= (cmd->scanparam.scantimes > 0?(cmd->scanparam.scantimes):1);
		expiredtime *= (cmd->scanparam.switchinterval > 100?(cmd->scanparam.switchinterval):200); 
		expiredtime += 5*HZ;
		
        time = tls_os_get_time();
sem_acquire:
        ret = tls_os_sem_acquire(hif->uart_atcmd_sem, expiredtime - offset);
        if (ret == TLS_OS_SUCCESS)
        {
			if(!(hif->uart_atcmd_bits & (1 << UART_ATCMD_BIT_WSCAN)))
			{
			    offset = tls_os_get_time() - time;
			    if (offset < expiredtime)
			    {
			        goto sem_acquire;    
			    }

			    if (hif->last_scan)
			    {
			        hif->last_scan = 0;
			    }
			}
        }
        else
        {
			hif->last_scan = 0;
        }
	
    }
    
    return ret ? -CMD_ERR_OPS : 0;
}


#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
int lkstt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    struct tls_cmd_link_status_t lk;
    
    memset(&lk, 0, sizeof(struct tls_cmd_link_status_t));
    tls_cmd_get_link_status(&lk);
    cmdrsp->lkstt.status = lk.status;
    if (cmdrsp->lkstt.status == 0)
        return 0;
    else{
        memcpy(cmdrsp->lkstt.ip, lk.ip, 4);
        memcpy(cmdrsp->lkstt.nm, lk.netmask, 4);
        memcpy(cmdrsp->lkstt.gw, lk.gw, 4);
        memcpy(cmdrsp->lkstt.dns1, lk.dns1, 4);
        memcpy(cmdrsp->lkstt.dns2, lk.dns2, 4);
    }
    return 0;
}

int entm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u16 rx_fifocnt;
    u8 ch;
    int i;
#if TLS_CONFIG_UART
	cmd_set_uart1_mode_callback callback;
	struct tls_uart_port *uart1_port;
	cmd_get_uart1_port_callback port_callback;
	extern void tls_uart_rx_disable(struct tls_uart_port *port);
	extern void tls_uart_rx_enable(struct tls_uart_port *port);

    port_callback = tls_cmd_get_uart1_port();
    if(port_callback!=NULL)
        port_callback(&uart1_port);
    if (!uart1_port) {
        return -CMD_ERR_NOT_ALLOW;
    }	
    callback = tls_cmd_get_set_uart1_mode();
    if(callback!=NULL)
        callback(UART_TRANS_MODE);

    tls_irq_disable(uart1_port->uart_irq_no);
    tls_uart_rx_disable(uart1_port);
    /* read all data from uart rx fifo */
    rx_fifocnt = (uart1_port->regs->UR_FIFOS >> 6) & 0x3F;
    for (i = 0; i < rx_fifocnt; i++)
        ch = (u8)uart1_port->regs->UR_RXW;

    (void)ch;

    /* reset uart rx ring buffer */
    uart1_port->recv.tail = uart1_port->recv.head; 

    tls_uart_rx_enable(uart1_port);
    tls_irq_enable(uart1_port->uart_irq_no); 
#endif   

    return 0;
}

int skct_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){

    int socket_num;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    int err = 0;
    u8 state;
	u32 time, offset = 0;
#endif
	struct tls_hostif *hif = tls_get_hostif();
    struct tls_cmd_socket_t socket;
    
    socket.timeout = cmd->skct.timeout; 
    memcpy(socket.ip_addr, cmd->skct.ip_addr, 4);
    socket.proto = cmd->skct.proto;
    socket.client = cmd->skct.client;
    socket.port = cmd->skct.port;
    socket.host_len = cmd->skct.host_len;
    memcpy(socket.host_name, cmd->skct.host_name, socket.host_len);
    socket.localport = cmd->skct.localport;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_SKCT);
    socket_num = tls_cmd_create_socket(&socket, cmd->skct.mode);
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    if (socket_num > 0 && socket_num <= TLS_MAX_NETCONN_NUM) {
        if(cmd->skct.mode!= CMD_MODE_HSPI_RICMD && cmd->skct.mode!= CMD_MODE_UART1_RICMD){
            time = tls_os_get_time();
sem_acquire:
            /* waiting for 25 seconds */
            err = tls_os_sem_acquire(hif->uart_atcmd_sem, 25*HZ - offset);
            if (err) {
                tls_cmd_close_socket(socket_num);
                return -CMD_ERR_SKT_CONN; 
            } else {
                if(!(hif->uart_atcmd_bits & (1 << UART_ATCMD_BIT_SKCT)))
                {
                    offset = tls_os_get_time() - time;
                    if (offset < 25*HZ)
                    {
                        goto sem_acquire;
                    }
                }

                tls_cmd_get_socket_state(socket_num, &state, NULL);
                if (state != NETCONN_STATE_NONE)
                    cmdrsp->skct.socket = socket_num;
                else
                {
                    tls_cmd_close_socket(socket_num);
                    return -CMD_ERR_SKT_CONN; 
                }
            }
        }else
            cmdrsp->skct.socket = socket_num;
    } else if (socket_num == 0) {
        return -CMD_ERR_NO_SKT;
    } else
        return -CMD_ERR_SKT_CONN; 
#else
	if(socket_num < 0)
		return -1;
	cmdrsp->skct.socket = socket_num;
#endif
    return 0;
}

int skstt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0,i=0; 
    u32 buflen;
    u8 socket = cmd->skstt.socket;
    struct tls_skt_status_t *skt_status = 0;
    if (set_opt) {
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    	if(cmd->skclose.socket<1 || cmd->skclose.socket>TLS_MAX_NETCONN_NUM)
    		return -CMD_ERR_INV_PARAMS;
#endif
        buflen = sizeof(struct tls_skt_status_ext_t) * 5 + sizeof(u32);
        skt_status = (struct tls_skt_status_t *)tls_mem_alloc(buflen);            
        if (!skt_status) {
            return -CMD_ERR_MEM;
        } else  {
            memset(skt_status, 0, buflen);
            ret = tls_cmd_get_socket_status(socket, (u8 *)skt_status, buflen);
            if (ret)
            {
                if (skt_status)
                    tls_mem_free(skt_status);
                return -CMD_ERR_INV_PARAMS;
            }
            cmdrsp->skstt.number = skt_status->socket_cnt;
            for (i=0;i<skt_status->socket_cnt;i++) {
                cmdrsp->skstt.ext[i].status = skt_status->skts_ext[i].status;
                cmdrsp->skstt.ext[i].socket= skt_status->skts_ext[i].socket;
                memcpy(cmdrsp->skstt.ext[i].host_ipaddr, skt_status->skts_ext[i].host_ipaddr, 4);
                cmdrsp->skstt.ext[i].remote_port = skt_status->skts_ext[i].remote_port;
                cmdrsp->skstt.ext[i].local_port = skt_status->skts_ext[i].local_port;
            }
            if (skt_status)
                tls_mem_free(skt_status);
        }
    } 

    return ret ? -CMD_ERR_OPS : 0;
}

int skcls_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    	if(cmd->skclose.socket<1 || cmd->skclose.socket>TLS_MAX_NETCONN_NUM)
    		return -CMD_ERR_INV_PARAMS;
#endif
        ret = tls_cmd_close_socket(cmd->skclose.socket);
    } 

    return ret ? -CMD_ERR_OPS : 0;
}

int sksdf_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
    	if(cmd->sksdf.socket<1 || cmd->sksdf.socket>20)
    		return -CMD_ERR_INV_PARAMS;
        tls_cmd_set_default_socket(cmd->sksdf.socket);
    }

    return 0;
}

int sksnd_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int err = 0;

	cmd_set_uart1_mode_callback callback;
	cmd_set_uart1_sock_param_callback sock_callback;
	struct tls_hostif *hif = tls_get_hostif();
    u8 state;
    if (set_opt) {
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    	if(cmd->sksnd.socket<1 || cmd->sksnd.socket>TLS_MAX_NETCONN_NUM)
    		return -CMD_ERR_INV_PARAMS;
#endif
        if (cmd->sksnd.size>512)
            cmd->sksnd.size = 512;
        err = tls_cmd_get_socket_state(cmd->sksnd.socket, &state, NULL);
        if (err || state != NETCONN_STATE_CONNECTED)
            return -CMD_ERR_INV_PARAMS;
        else {
            cmdrsp->sksnd.size = cmd->sksnd.size;
			tls_cmd_set_default_socket(cmd->sksnd.socket);
			if (hif->hostif_mode == HOSTIF_MODE_UART1_LS){
				sock_callback = tls_cmd_get_set_uart1_sock_param();
				if(sock_callback!=NULL)
					sock_callback(cmd->sksnd.size, FALSE);
				callback = tls_cmd_get_set_uart1_mode();
				if(callback!=NULL)
					callback(UART_ATSND_MODE);
			}
        } 
    } 
    return 0;
//    return -CMD_ERR_SKT_SND;
}

int skrcv_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 socket = 0, size = 0, maxsize;
    u8 state;
    if (set_opt) {
        socket = cmd->skrcv.socket;
        size = cmd->skrcv.size;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    	if(socket<1 || socket>TLS_MAX_NETCONN_NUM)
    		return -CMD_ERR_INV_PARAMS;
#endif
        if (size > AT_SKRCV_CMD_RECV_MAX_SIZE_PER_TIME)
            size = AT_SKRCV_CMD_RECV_MAX_SIZE_PER_TIME;
        tls_cmd_get_socket_state(socket, &state, NULL);
        if (state != NETCONN_STATE_CONNECTED) {
            return -CMD_ERR_INV_PARAMS;
        } else {
        	maxsize = size;
		
        cmdrsp->skrcv.socket = socket;
        cmdrsp->skrcv.size = maxsize;
        }
    } 

    return 0;
}

int skrptm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_hostif *hif = tls_get_hostif();
    if (set_opt) {
    	if(cmd->skrptm.mode>1)
    		return -CMD_ERR_INV_PARAMS;
        hif->rptmode = cmd->skrptm.mode;
    } else{
        cmdrsp->skrptm.mode = hif->rptmode;
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int sksrcip_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 ipvalue;
#if TLS_CONFIG_CMD_USE_RAW_SOCKET
    ipvalue = tls_net_get_sourceip();
    memcpy(cmdrsp->sksrcip.ipvalue, (u8 *)&ipvalue, 4);
	tls_net_set_sourceip(0);
#else
    ipvalue = source_ip;
    memcpy(cmdrsp->sksrcip.ipvalue, (u8 *)&ipvalue, 4);
	source_ip = 0;
#endif
    return 0;
}

int skghbn_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8 *ipstr = NULL;
    int i=0;
	struct hostent* HostEntry;
    if (set_opt) {
        ipstr = cmd->skghbn.ipstr;
    	HostEntry = gethostbyname((char *)ipstr); 
        if(HostEntry)
            for(i=0;i<4;i++)
                cmdrsp->skghbn.h_addr_list[i]=*(HostEntry->h_addr_list[0]+i);
        else 
            return -CMD_ERR_INV_PARAMS;
    }

    return 0;
}

#endif

int wprt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->wprt.type>3)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_wireless_mode(cmd->wprt.type, update_flash);
    } else {
        ret = tls_cmd_get_wireless_mode(&cmdrsp->wprt.type); 
    }
    return ret ? -CMD_ERR_OPS : 0;
}

int ssid_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_ssid_t ssid;
    
    memset(&ssid, 0, sizeof(struct tls_cmd_ssid_t));
    if (set_opt) {
        if((cmd->ssid.ssid_len>32)||(cmd->ssid.ssid_len == 0))
            return -CMD_ERR_INV_PARAMS;
        ssid.ssid_len = cmd->ssid.ssid_len;
        MEMCPY(ssid.ssid, cmd->ssid.ssid, ssid.ssid_len);
        ret = tls_cmd_set_ssid(&ssid, update_flash);
    } else {
        ret = tls_cmd_get_ssid(&ssid);
        if (!ret){
	        cmdrsp->ssid.ssid_len = ssid.ssid_len;
	        MEMCPY(cmdrsp->ssid.ssid, ssid.ssid, ssid.ssid_len);
        }
    }
    return ret ? -CMD_ERR_OPS : 0;
}

int key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_key_t key;
    if (set_opt) {
    	if(cmd->key.format > 1 || cmd->key.index > 4 || cmd->key.key_len > 64)
    		return -CMD_ERR_INV_PARAMS;
        key.format = cmd->key.format;
        key.index = cmd->key.index;
        memcpy(key.key, cmd->key.key, cmd->key.key_len);
        key.key_len = cmd->key.key_len;
        ret = tls_cmd_set_key(&key, update_flash);
    } else {
        ret = tls_cmd_get_key(&key);
        if(!ret){
            cmdrsp->key.format = key.format;
            cmdrsp->key.index = key.index;
            cmdrsp->key.key_len = key.key_len;
            memcpy(cmdrsp->key.key, key.key, cmdrsp->key.key_len);
        }
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int encry_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->encrypt.mode>8)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_encrypt(cmd->encrypt.mode, update_flash);
    } else {
        ret = tls_cmd_get_encrypt(&cmdrsp->encrypt.mode); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int bssid_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_bssid_t bssid;
    if (set_opt) {
    	if(cmd->bssid.enable>1)
    		return -CMD_ERR_INV_PARAMS;
        bssid.enable = cmd->bssid.enable;
        if(bssid.enable==1) {
            memcpy(bssid.bssid, cmd->bssid.bssid, 6);
        }
        ret = tls_cmd_set_bssid(&bssid, update_flash);
    } else {
        ret = tls_cmd_get_bssid(&bssid);
        cmdrsp->bssid.enable = bssid.enable;
        memcpy(cmdrsp->bssid.bssid, bssid.bssid, 6);
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int brdssid_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->brd_ssid.enable>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_hide_ssid(cmd->brd_ssid.enable, update_flash);
    } else {
        ret = tls_cmd_get_hide_ssid(&cmdrsp->brd_ssid.enable); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int cntparam_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
	struct tls_param_ssid original_ssid;
	struct tls_param_original_key original_key;
    struct tls_cmd_bssid_t bssid;

    memset(&original_ssid, 0, sizeof(struct tls_param_ssid));
    memset(&original_key, 0, sizeof(struct tls_param_original_key));
    memset(&bssid, 0, sizeof(struct tls_cmd_bssid_t));
    tls_cmd_get_bssid(&bssid);    
    tls_cmd_get_original_key(&original_key);
    if (bssid.enable) {
        cmdrsp->cntparam_bssid_en.bssid_enable = bssid.enable;
        memcpy(cmdrsp->cntparam_bssid_en.bssid, bssid.bssid, 6);
        cmdrsp->cntparam_bssid_en.key_len = original_key.key_length;
        memcpy(cmdrsp->cntparam_bssid_en.key, original_key.psk, original_key.key_length);
    }else{
        tls_cmd_get_original_ssid(&original_ssid);
        cmdrsp->cntparam_bssid_dis.bssid_enable = bssid.enable;
        cmdrsp->cntparam_bssid_dis.ssid_len = original_ssid.ssid_len;
        memcpy(cmdrsp->cntparam_bssid_dis.ssid_key, original_ssid.ssid, original_ssid.ssid_len);
        cmdrsp->cntparam_bssid_dis.key_len = original_key.key_length;
        memcpy(cmdrsp->cntparam_bssid_dis.ssid_key+cmdrsp->cntparam_bssid_dis.ssid_len, original_key.psk, original_key.key_length);
    }
    return 0;
}

int chl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if((cmd->channel.enable>1) ||((cmd->channel.enable==1)&&((cmd->channel.channel>14) ||(cmd->channel.channel<1))))
    		return -CMD_ERR_INV_PARAMS;
        if(cmd->channel.enable==0)
            cmd->channel.channel = 1;
        ret = tls_cmd_set_channel(cmd->channel.channel, cmd->channel.enable, update_flash);
    } else {
        ret = tls_cmd_get_channel(&cmdrsp->channel.channel, &cmdrsp->channel.enable);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int chll_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
        if((cmd->channel_list.channellist & (~0x3fff))||(cmd->channel_list.channellist == 0)) {
            return -CMD_ERR_INV_PARAMS;
        }
    	tls_cmd_set_channellist(cmd->channel_list.channellist, update_flash);
    } else {
        tls_cmd_get_channellist((u16 *)&(cmdrsp->channel_list.channellist));
    }

    return 0;
}

int wreg_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
    	tls_cmd_set_region(cmd->wreg.region, update_flash);
    } else {
        tls_cmd_get_region((u16 *)&cmdrsp->wreg.region);
    }
    return 0;
}

int wbgr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
	int limit_rate;
    struct tls_cmd_wl_hw_mode_t hw_mode;
    memset(&hw_mode, 0, sizeof(struct tls_cmd_wl_hw_mode_t));
    
    if (set_opt) {
        if(cmd->wbgr.mode>3 || cmd->wbgr.rate >28)
            return -CMD_ERR_INV_PARAMS;
        limit_rate = (cmd->wbgr.mode == 1)?3:((cmd->wbgr.mode == 2)? 28:11);
        hw_mode.max_rate = (cmd->wbgr.rate > limit_rate)?limit_rate: cmd->wbgr.rate;
        hw_mode.hw_mode = cmd->wbgr.mode;
    	ret = tls_cmd_set_hw_mode(&hw_mode, update_flash);
    } else {
        ret = tls_cmd_get_hw_mode(&hw_mode);
        cmdrsp->wbgr.mode = hw_mode.hw_mode;
        cmdrsp->wbgr.rate = hw_mode.max_rate;
    }
    return ret ? -CMD_ERR_OPS : 0;
}

int watc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->watc.enable>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_adhoc_create_mode(cmd->watc.enable, update_flash);
    } else {
        ret = tls_cmd_get_adhoc_create_mode(&cmdrsp->watc.enable); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int wpsm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->wpsm.enable>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_wl_ps_mode(cmd->wpsm.enable, update_flash);
    } else {
        ret = tls_cmd_get_wl_ps_mode(&cmdrsp->wpsm.enable); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int warc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->warc.autoretrycnt>255)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_warc(cmd->warc.autoretrycnt, update_flash);
    } else {
        ret = tls_cmd_get_warc(&cmdrsp->warc.autoretrycnt); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int warm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->warm.enable>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_roaming_mode(cmd->warm.enable, update_flash);
    } else {
        ret = tls_cmd_get_roaming_mode(&cmdrsp->warm.enable); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
int nip_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_ip_params_t ip_info;
    if (set_opt) {
    	if(cmd->nip.type>1)
    		return -CMD_ERR_INV_PARAMS;
        ip_info.type = cmd->nip.type;
        memcpy(ip_info.ip_addr, cmd->nip.ip, 4);
        memcpy(ip_info.netmask, cmd->nip.nm, 4);
        memcpy(ip_info.gateway, cmd->nip.gw, 4);
        memcpy(ip_info.dns, cmd->nip.dns, 4);
        ret = tls_cmd_set_ip_info(&ip_info, update_flash);
    } else {
        ret = tls_cmd_get_ip_info(&ip_info); 
        cmdrsp->nip.type = ip_info.type;
        memcpy(cmdrsp->nip.ip, ip_info.ip_addr, 4);
        memcpy(cmdrsp->nip.nm, ip_info.netmask, 4);
        memcpy(cmdrsp->nip.gw, ip_info.gateway, 4);
        memcpy(cmdrsp->nip.dns, ip_info.dns, 4);
    }

    return ret ? -CMD_ERR_OPS : 0;
}
#endif
int atm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->atm.mode>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_work_mode(cmd->atm.mode, update_flash);
    } else {
        ret = tls_cmd_get_work_mode(&cmdrsp->atm.mode); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int atrm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt)
    	tls_cmd_set_default_socket_params((struct tls_cmd_socket_t *)&cmd->atrm, update_flash);
    else
        tls_cmd_get_default_socket_params((struct tls_cmd_socket_t *)&cmdrsp->atrm);
    return 0;
}

int aolm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

int portm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->portm.mode>3)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_hostif_mode(cmd->portm.mode, update_flash);
    } else {
        ret = tls_cmd_get_hostif_mode(&cmdrsp->portm.mode);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int uart_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_uart_params_t uart_cfg;
    memset(&uart_cfg, 0, sizeof(struct tls_cmd_uart_params_t));
    if (set_opt) {
        memcpy((u8 *)&uart_cfg.baud_rate, cmd->uart.baud_rate, 3);
        uart_cfg.charlength = cmd->uart.char_len;
        uart_cfg.stop_bit = cmd->uart.stopbit;
        uart_cfg.parity = cmd->uart.parity;
        uart_cfg.flow_ctrl = cmd->uart.flow_ctrl;
        ret = tls_cmd_set_uart_params(&uart_cfg, update_flash);
    } else {
        ret = tls_cmd_get_uart_params(&uart_cfg);
        memcpy(cmdrsp->uart.baud_rate, &uart_cfg.baud_rate, 3);
        cmdrsp->uart.char_len = uart_cfg.charlength;
        cmdrsp->uart.flow_ctrl = uart_cfg.flow_ctrl;
        cmdrsp->uart.parity = uart_cfg.parity;
        cmdrsp->uart.stopbit = uart_cfg.stop_bit;
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int atlt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->atlt.length>1024 || cmd->atlt.length<32)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_atlt(cmd->atlt.length, update_flash);
    } else {
        ret = tls_cmd_get_atlt((u16 *)&cmdrsp->atlt.length);  
    }
    
    return ret ? -CMD_ERR_OPS : 0;
}

int dns_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->dns.length>31 || cmd->dns.length==0)
    		return -CMD_ERR_INV_PARAMS;
        cmd->dns.name[cmd->dns.length] = '\0';
        ret = tls_cmd_set_dnsname(cmd->dns.name, update_flash);
    } else {
        ret = tls_cmd_get_dnsname(cmdrsp->dns.name);
        cmdrsp->dns.length = strlen((char *)cmdrsp->dns.name);
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int ddns_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

int upnp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

int dname_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

int atpt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->portm.mode>10000)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_atpt(cmd->atpt.period, update_flash);
    } else {
        ret = tls_cmd_get_atpt((u16 *)&cmdrsp->atpt.period);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int dbg_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
    	tls_cmd_set_dbg(cmd->dbg.dbg_level);
    }

    return 0;
}

int espc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->espc.escapechar>0xFF)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_espc(cmd->espc.escapechar, update_flash);
    } else {
        ret = tls_cmd_get_espc(&cmdrsp->espc.escapechar);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int espt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->espt.escapeperiod>10000 || cmd->espt.escapeperiod<100)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_espt(cmd->espt.escapeperiod, update_flash);
    } else {
        ret = tls_cmd_get_espt((u16 *)&cmdrsp->espt.escapeperiod);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int webs_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_webs_cfg stWebsCfg;
    if (set_opt) {
    	if(cmd->webs.autorun>1 || cmd->webs.portnum<1 || cmd->webs.portnum>65535)
    		return -CMD_ERR_INV_PARAMS;
        stWebsCfg.AutoRun = cmd->webs.autorun;
        stWebsCfg.PortNum = cmd->webs.portnum;
        ret = tls_cmd_set_webs(stWebsCfg, update_flash);
    } else {
        ret = tls_cmd_get_webs(&stWebsCfg);
        cmdrsp->webs.autorun = stWebsCfg.AutoRun;
        cmdrsp->webs.portnum = stWebsCfg.PortNum;
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int iom_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->iom.mode>2)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_iom(cmd->iom.mode, update_flash);
    } else {
        ret = tls_cmd_get_iom(&cmdrsp->iom.mode);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int cmdm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->iom.mode>1)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_cmdm(cmd->cmdm.mode, update_flash);
    } else {
        ret = tls_cmd_get_cmdm(&cmdrsp->cmdm.mode);  
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int pass_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->pass.length!=6)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_pass(cmd->pass.password, update_flash);
    } else {
        ret = tls_cmd_get_pass(cmdrsp->pass.password); 
        cmdrsp->pass.length = 6;
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int oneshot_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0;
    uint8_t flag = 0;
    
    if (set_opt) {
#if (WM_BLE_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON)
        if(cmd->oneshot.status>4)
#else
        if(cmd->oneshot.status>3)
#endif
            return -CMD_ERR_INV_PARAMS;

        /*check oneshot flag, if busy, clear it*/
        tls_cmd_get_oneshot(&flag);
        if(flag != 0)
        {
            tls_cmd_set_oneshot(0, 0);
        }
        
        ret = tls_cmd_set_oneshot(cmd->oneshot.status, update_flash);
    } else {
        ret = tls_cmd_get_oneshot(&cmdrsp->oneshot.status);
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int updp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
    	if(cmd->updp.mode>1)
    		return -CMD_ERR_INV_PARAMS;
        if (cmd->updp.mode == 1) {
            extern struct tls_sys_param user_default_param;
    		extern int tls_param_load_user(struct tls_sys_param *param);
    		struct tls_sys_param *param = &user_default_param;
    		
    		tls_param_set_updp_mode(cmd->updp.mode);
    		tls_param_load_user(param);
        }
        else {
            tls_param_set_updp_mode(0);
		    tls_param_save_user_default();
        }
    }

    return 0;
}

#if TLS_CONFIG_HTTP_CLIENT_TASK
int httpc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
	http_client_msg msg;
/*    if (set_opt) {
        if((cmd->httpc.url_len>255)||(cmd->httpc.data_len>512))
            return -CMD_ERR_INV_PARAMS;
    	memset(&msg, 0, sizeof(http_client_msg));
    	msg.param.Uri = (char *)cmd->httpc.url;
    	msg.method = (HTTP_VERB)cmd->httpc.verb;
    	if(cmd->httpc.verb == VerbPost || cmd->httpc.verb == VerbPut)
    	{
    		msg.dataLen = cmd->httpc.data_len;
    		msg.sendData = (char *)(cmd->httpc.url + cmd->httpc.url_len);
    	}
    	msg.recv_fn = tls_hostif_http_client_recv_callback;
    	msg.err_fn = tls_hostif_http_client_err_callback;
    	http_client_post(&msg);
        cmdrsp->httpc.psession = msg.pSession;
    }*/

    if (set_opt)
	{     
        if((cmd->httpc.url_len>255)||(cmd->httpc.data_len>512))
            return -CMD_ERR_INV_PARAMS;
    	memset(&msg, 0, sizeof(http_client_msg));
    	msg.param.Uri = (CHAR *)cmd->httpc.url;
    	msg.method = (HTTP_VERB)cmd->httpc.verb;
    	if(cmd->httpc.verb == VerbPost || cmd->httpc.verb == VerbPut)
    	{
    		msg.dataLen = cmd->httpc.data_len;
    		msg.sendData = (CHAR *)cmd->httpc.data;
    	}
    	msg.recv_fn = tls_hostif_http_client_recv_callback;
    	msg.err_fn = tls_hostif_http_client_err_callback;
    	http_client_post(&msg);
        cmdrsp->httpc.psession = msg.pSession;
    }

    return 0;
}

int fwup_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    if (set_opt) 
    {       
#if TLS_CONFIG_HTTP_CLIENT
        t_http_fwup((char*)cmd->httpc.url);
#endif
    }    
    return 0;
}

#endif
#endif

int tem_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0;
    struct tls_cmd_tem_t tem = { 0 };

    memset(&tem, 0, sizeof(struct tls_cmd_tem_t));
    if( set_opt ) {
		return -CMD_ERR_UNSUPP;
    }
    else {
        char temperature[8] = {0};
        int temp = adc_temp();
        s32 offset = 0;

		ret = tls_get_rx_iq_gain((u8 *)&offset);
		if (offset != -1) {
			temp = temp - offset;
        }
        if (!ret){
			if (temp < 0)
			{
				temp = 0- temp;
	            sprintf(temperature, "-%d.%d", temp/1000, (temp%1000)/100);			
			}
			else
	            sprintf(temperature, "%d.%d", temp/1000, (temp%1000)/100);
            memcpy((char *)cmdrsp->tem.offset, temperature, strlen(temperature));
        }
    }
    
   return ret ? -CMD_ERR_OPS : 0;
}

int qmac_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8 *mac = NULL;
    mac = wpa_supplicant_get_mac();
	if (mac)
	{
    	memcpy(cmdrsp->mac.addr, mac, 6);
	}
	else
	{
		tls_get_mac_addr(&cmdrsp->mac.addr[0]);
	}

    return 0;
}

#if TLS_CONFIG_AP
int slist_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
#define STA_DETAIL_BUF_LEN  320
    u32 sta_num = 0;
    u8 *sta_detail;
    int ret = -1;

    sta_detail = tls_mem_alloc(STA_DETAIL_BUF_LEN);
    if (NULL == sta_detail)
    {
        return -CMD_ERR_MEM;
    }

    memset(sta_detail, 0, STA_DETAIL_BUF_LEN);
    ret = tls_cmd_get_sta_detail(&sta_num, sta_detail);
    if (ret)
    {
        tls_mem_free(sta_detail);
        return -CMD_ERR_MEM;
    }

    cmdrsp->stalist.sta_num = sta_num;
    memcpy(cmdrsp->stalist.data, sta_detail, strlen((char *)sta_detail) + 1);
    tls_mem_free(sta_detail);

    return 0;
}

int softap_lkstt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    struct tls_cmd_link_status_t lk;
    
    memset(&lk, 0, sizeof(struct tls_cmd_link_status_t));
    tls_cmd_get_softap_link_status(&lk);
    cmdrsp->lkstt.status = lk.status;
    if (cmdrsp->lkstt.status == 0)
        return 0;
    else{
        memcpy(cmdrsp->lkstt.ip, lk.ip, 4);
        memcpy(cmdrsp->lkstt.nm, lk.netmask, 4);
        memcpy(cmdrsp->lkstt.gw, lk.gw, 4);
        memcpy(cmdrsp->lkstt.dns1, lk.dns1, 4);
        memcpy(cmdrsp->lkstt.dns2, lk.dns2, 4);
    }
    return 0;
}

int softap_ssid_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_ssid_t ssid;
    
    memset(&ssid, 0, sizeof(struct tls_cmd_ssid_t));
    if (set_opt) {
        if((cmd->ssid.ssid_len>32)||(cmd->ssid.ssid_len == 0))
            return -CMD_ERR_INV_PARAMS;
        ssid.ssid_len = cmd->ssid.ssid_len;
        MEMCPY(ssid.ssid, cmd->ssid.ssid, ssid.ssid_len);
        ret = tls_cmd_set_softap_ssid(&ssid, update_flash);
    } else {
        ret = tls_cmd_get_softap_ssid(&ssid);
        if (!ret){
	        cmdrsp->ssid.ssid_len = ssid.ssid_len;
	        MEMCPY(cmdrsp->ssid.ssid, ssid.ssid, ssid.ssid_len);
        }
    }
    return ret ? -CMD_ERR_OPS : 0;
}

int softap_qmac_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8 *mac = NULL;
    mac = hostapd_get_mac();
    memcpy(cmdrsp->mac.addr, mac, 6);
    return 0;
}

int softap_encry_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if(cmd->encrypt.mode>8)
    		return -CMD_ERR_INV_PARAMS;
        ret = tls_cmd_set_softap_encrypt(cmd->encrypt.mode, update_flash);
    } else {
        ret = tls_cmd_get_softap_encrypt(&cmdrsp->encrypt.mode); 
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int softap_key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_key_t key;
    if (set_opt) {
    	if(cmd->key.format > 1 || cmd->key.index > 4 || cmd->key.key_len > 64)
    		return -CMD_ERR_INV_PARAMS;
        key.format = cmd->key.format;
        key.index = cmd->key.index;
        memcpy(key.key, cmd->key.key, cmd->key.key_len);
        key.key_len = cmd->key.key_len;
        ret = tls_cmd_set_softap_key(&key, update_flash);
    } else {
        ret = tls_cmd_get_softap_key(&key);
        if(!ret){
            cmdrsp->key.format = key.format;
            cmdrsp->key.index = key.index;
            cmdrsp->key.key_len = key.key_len;
            memcpy(cmdrsp->key.key, key.key, cmdrsp->key.key_len);
        }
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int softap_chl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    if (set_opt) {
    	if((cmd->channel.enable>1) ||((cmd->channel.enable==1)&&((cmd->channel.channel>14) ||(cmd->channel.channel<1))))
    		return -CMD_ERR_INV_PARAMS;
        if(cmd->channel.enable==0)
            cmd->channel.channel = 1;
        ret = tls_cmd_set_softap_channel(cmd->channel.channel, update_flash);
    } else {
        ret = tls_cmd_get_softap_channel(&cmdrsp->channel.channel);  
		cmdrsp->channel.enable = 1;
    }

    return ret ? -CMD_ERR_OPS : 0;
}

int softap_wbgr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret=0;
	int limit_rate;
    struct tls_cmd_wl_hw_mode_t hw_mode;
    memset(&hw_mode, 0, sizeof(struct tls_cmd_wl_hw_mode_t));
    
    if (set_opt) {
        if(cmd->wbgr.mode>3 || cmd->wbgr.rate >28)
            return -CMD_ERR_INV_PARAMS;
        limit_rate = (cmd->wbgr.mode == 1)?3:((cmd->wbgr.mode == 2)? 28:11);
        hw_mode.max_rate = (cmd->wbgr.rate > limit_rate)?limit_rate: cmd->wbgr.rate;
        hw_mode.hw_mode = cmd->wbgr.mode;
    	ret = tls_cmd_set_softap_hw_mode(&hw_mode, update_flash);
    } else {
        ret = tls_cmd_get_softap_hw_mode(&hw_mode);
        cmdrsp->wbgr.mode = hw_mode.hw_mode;
        cmdrsp->wbgr.rate = hw_mode.max_rate;
    }
    return ret ? -CMD_ERR_OPS : 0;
}

int softap_nip_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret = 0; 
    struct tls_cmd_ip_params_t ip_info;
    if (set_opt) {
    	if(cmd->nip.type>1)
    		return -CMD_ERR_INV_PARAMS;
        ip_info.type = cmd->nip.type;
        memcpy(ip_info.ip_addr, cmd->nip.ip, 4);
        memcpy(ip_info.netmask, cmd->nip.nm, 4);
        memcpy(ip_info.gateway, cmd->nip.gw, 4);
        memcpy(ip_info.dns, cmd->nip.dns, 4);
        ret = tls_cmd_set_softap_ip_info(&ip_info, update_flash);
    } else {
        ret = tls_cmd_get_softap_ip_info(&ip_info); 
        cmdrsp->nip.type = ip_info.type;
        memcpy(cmdrsp->nip.ip, ip_info.ip_addr, 4);
        memcpy(cmdrsp->nip.nm, ip_info.netmask, 4);
        memcpy(cmdrsp->nip.gw, ip_info.gateway, 4);
        memcpy(cmdrsp->nip.dns, ip_info.dns, 4);
    }

    return ret ? -CMD_ERR_OPS : 0;
}


#endif

int qver_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    tls_cmd_get_ver((struct tls_cmd_ver_t *)&cmdrsp->ver);
    return 0;
}

int updm_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if(0 == tls_get_fwup_mode())
	{
		if(1 == cmd->updm.mode)
		{
			tls_set_fwup_mode(cmd->updm.mode);
			tls_cmd_disconnect_network((IEEE80211_MODE_INFRA | IEEE80211_MODE_AP));/*åçº§æ¶æ­å¼ç½ç»*/
			if (0 == cmd->updm.src)
			    tls_fwup_enter(TLS_FWUP_IMAGE_SRC_LUART);
			else
			    tls_fwup_enter(TLS_FWUP_IMAGE_SRC_HSPI);
		}
	}
	else
	{
		if(0 == cmd->updm.mode)
		{
			tls_set_fwup_mode(0);
			tls_fwup_exit(tls_fwup_get_current_session_id());
		}
	}
    return 0;
}

int updd_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int datasize, session_id;	
    int err = 0;

    session_id = tls_fwup_get_current_session_id();
	if((0 == session_id) || (TLS_FWUP_STATUS_OK != tls_fwup_current_state(session_id)))
		return -CMD_ERR_INV_PARAMS;

    datasize = cmd->updd.size;
    if (0 == cmd->updd.data[0])/* at */
    {
        if(datasize != sizeof(struct tls_fwup_block)) 
    	        err = -CMD_ERR_INV_PARAMS;
    }
    else if (1 == cmd->updd.data[0])/* ri */
    {
#if TLS_CONFIG_HS_SPI
        tls_set_hspi_fwup_mode(1);
#endif
    }

    return err;
}

/******************************************************************
* Description:	Read register or memory

* Format:		AT+&REGR=<address>,[num]<CR>
			+OK=<value1>,[value2]...<CR><LF><CR><LF>
		
* Argument:	address: num:

* Author: 	kevin 2014-03-19
******************************************************************/
int regr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 Addr, Num;
    int i=0;
//printf("addr = 0x%x  length = %d\n",cmd->regr.reg_base_addr,Num = cmd->regr.length);
    Addr = cmd->regr.reg_base_addr;
    Num = cmd->regr.length;
    for(i=0;i<Num;i++){
        cmdrsp->regr.value[i] = tls_reg_read32(Addr);
        Addr += 4;
    }
    cmdrsp->regr.length = Num;
    return 0;
}

/******************************************************************
* Description:	Write register or memory

* Format:		AT+&REGW=<address>,<value1>,[value2]...<CR>
			+OK=<CR><LF><CR><LF>
		
* Argument:	address: value:

* Author: 	kevin 2014-03-19
******************************************************************/
int regw_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 Addr, i;
    u32 cpu_sr;

    cpu_sr = tls_os_set_critical();
    Addr = cmd->regw.reg_base_addr;
    for(i = 0; i < cmd->regw.length; i++)
    {
        tls_reg_write32(Addr, cmd->regw.v[i]);
        Addr += 4;
    }
    tls_os_release_critical(cpu_sr);	
    return 0;
}

/******************************************************************
* Description:	Read RF register

* Format:		AT+&RFR=<address>,[num]<CR>
			+OK=<value1>,[value2]...<CR><LF><CR><LF>
		
* Argument:	address: size:

* Author: 	kevin 2014-03-19
******************************************************************/
int rfr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int i;
	u32 Addr, Num;

    Addr = cmd->rfr.reg_base_addr;
    Num = cmd->rfr.length;

    if((Num < 1) || (Num > 8) || (Addr+Num-1) > RFR_REG_MAX_NUM)
	{
		return -CMD_ERR_INV_PARAMS;
	}
	
	for(i = 0; i < Num; i++)
	{
		cmdrsp->rfr.value[i] = (u16)rf_spi_read(Addr);
		Addr += 1;
	}
    cmdrsp->rfr.length = Num;
    return 0;
}

/******************************************************************
* Description:	Write RF registers

* Format:		AT+&RFW=<address>,<value1>,[value2]...<CR>
			+OK<CR><LF><CR><LF>
		
* Argument:	address: value:

* Author: 	kevin 2014-03-19
******************************************************************/
int rfw_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int i;
	u32 Addr, Num;
	u16 databuf[8];
    
    Num = 0;
    Addr = cmd->rfw.reg_base_addr;
	for(i = 0; i < cmd->rfw.length; i++)
	{
		databuf[Num++] = cmd->rfw.v[i];
	}
	if((Num < 1) || (Num > 8) || (Addr+Num-1) > RFR_REG_MAX_NUM)
	{
		return -CMD_ERR_INV_PARAMS;
	}
	Addr = Addr*2;
	for(i = 0; i < Num; i++)
	{
		rf_spi_write((Addr << 16) | databuf[i]);
		Addr += 2;
	}
    return 0;
}

int flsr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 addr, len, ret = 0;
	u8 buff[32];
    addr = cmd->flsr.reg_base_addr;
	len = cmd->flsr.length;
	if((len > 8) || (len < 1))
	{
		TLS_DBGPRT_INFO("ret = 0x%x, len = 0x%x\r\n", ret, len);
		return -CMD_ERR_INV_PARAMS;
	}
	
	TLS_DBGPRT_INFO("addr = 0x%x, len = 0x%x\r\n", addr, len);
	
	memset(buff, 0, sizeof(buff));
	ret = tls_fls_read(addr, buff, 4 * len);
	if(ret)
	{
		TLS_DBGPRT_INFO("ret = 0x%x\r\n", ret);
		return -CMD_ERR_INV_PARAMS;
	}
	memcpy((u8 *)cmdrsp->flsr.value, buff, 4 * len);
    cmdrsp->flsr.length = len;
    return 0;
}

int flsw_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u32 addr, num, data, ret, i;
	u8 buff[32];
    addr = cmd->flsw.reg_base_addr;
    num = cmd->flsw.length;
    memset(buff, 0, sizeof(buff));
	for(i = 0; i < num; i++)
	{
		data = cmd->flsw.v[i];
		MEMCPY(&buff[4 * i], &data, sizeof(u32));
		TLS_DBGPRT_INFO("data = 0x%x\r\n", data);
	}
	
	ret = tls_fls_write(addr, buff, 4 * num);
    if(ret)
	{
		TLS_DBGPRT_INFO("ret = 0x%x\r\n", ret);
		return -CMD_ERR_INV_PARAMS;
	}
    return 0;
}

/******************************************************************
* Description:	set/get system tx gain

* Format:		AT+&TXG=[!?][gain]<CR>
			+OK[=gain]<CR><LF><CR><LF>
		
* Argument:	12 byte hex ascii

* Author: 	kevin 2014-03-12
******************************************************************/
int txg_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8* tx_gain = ieee80211_get_tx_gain();
    if(set_opt){
         memcpy(tx_gain, cmd->txg.tx_gain, TX_GAIN_LEN);
		TLS_DBGPRT_INFO("save tx gain!\r\n");
		return tls_set_tx_gain(tx_gain);
    }else{
		MEMCPY(cmdrsp->txg.tx_gain, tx_gain, TX_GAIN_LEN);
    }
    return 0;
}


/******************************************************************
* Description:	set/get system tx gain index

* Format:		
				Save Gain:	AT+&TXGI=[!][gainindex]<CR>
							+OK[=gainindex]<CR><LF><CR><LF>
* Argument:	
			gain value:28 byte hex ascii

* Author: 	
******************************************************************/
int txgi_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8 tx_gain[TX_GAIN_LEN];
	u8* param_tx_gain = ieee80211_get_tx_gain();

	int ret = 0;
	int i = 0;
	if(set_opt)
	{
		extern int tls_tx_gainindex_map2_gainvalue(u8 *dst_txgain, u8 *srcgain_index);
		tls_tx_gainindex_map2_gainvalue(tx_gain, cmd->txg.tx_gain);
		for (i = 0; i < TX_GAIN_LEN/3; i++)
		{
			if (cmd->txg.tx_gain[i] == 0xFF)
			{
				tx_gain[i] = 0xFF;
				tx_gain[i+TX_GAIN_LEN/3] = 0xFF;
				tx_gain[i+TX_GAIN_LEN*2/3] = 0xFF;
			}
			else
			{
				param_tx_gain[i] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN/3] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN*2/3] = tx_gain[i];
			}
		}
		ret = tls_set_tx_gain(tx_gain);
	}
	else
	{  
		/*ÃÃ§ÃÂµÂ·Â´ÃÂ³flashÂ²ÃÃÃ½ÃÃ¸ÂµÃÃÂµÂ¼ÃÂ´Ã¦Â´Â¢ÃÃ©Â¿Ã¶*/
		ret = tls_get_tx_gain(tx_gain);
		if (ret == 0)
		{
			extern int tls_tx_gainvalue_map2_gainindex(u8 *dst_txgain_index, u8 *srcgain);
			memset(cmdrsp->txg.tx_gain, 0xFF, TX_GAIN_LEN/3);
			ret = tls_tx_gainvalue_map2_gainindex(cmdrsp->txg.tx_gain, tx_gain);
		}
	}
	return ret;
}


int txg_rate_set_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8* tx_gain = ieee80211_get_tx_gain();
    if(set_opt){
         tx_gain[cmd->txgr.tx_rate] = cmd->txgr.txr_gain[0];
         tx_gain[cmd->txgr.tx_rate+TX_GAIN_LEN/3] = cmd->txgr.txr_gain[1];		 
         tx_gain[cmd->txgr.tx_rate+TX_GAIN_LEN*2/3] = cmd->txgr.txr_gain[2];			 
         return tls_set_tx_gain(tx_gain);
    }
    return 0;
}

int txg_rate_get_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8* tx_gain = ieee80211_get_tx_gain();
    cmdrsp->txgr.tx_rate = cmd->txgr.tx_rate;
    cmdrsp->txgr.txr_gain[0] = tx_gain[cmd->txgr.tx_rate];
    cmdrsp->txgr.txr_gain[1] =  tx_gain[cmd->txgr.tx_rate+TX_GAIN_LEN/3];
    cmdrsp->txgr.txr_gain[2] =   tx_gain[cmd->txgr.tx_rate+TX_GAIN_LEN*2/3];			 
    return 0;
}



int mac_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if (set_opt) {
    	if(cmd->mac.length>12)
    	{
   		    return -CMD_ERR_INV_PARAMS;
        }
		wpa_supplicant_set_mac(cmd->mac.macaddr);
		return tls_set_mac_addr(cmd->mac.macaddr);
    }else{
        u8 *mac = NULL;
        mac = wpa_supplicant_get_mac();
		if (mac)
		{
        	memcpy(cmdrsp->mac.addr, mac, 6);
		}
		else
		{
			tls_get_mac_addr(&cmdrsp->mac.addr[0]);
		}
    }
    return 0;
}

int hwv_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    tls_cmd_get_ver((struct tls_cmd_ver_t *)&cmdrsp->ver);
    return 0;
}

/******************************************************************
* Description:	Set/Get spi flash's parameter

* Format:		AT+&SPIF=[!?][size]<CR>[data stream]
			+OK<CR><LF><CR><LF>[data stream]
		
* Argument:	hex

* Author: 	kevin 2014-03-17
******************************************************************/
int spif_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

/******************************************************************
* Description:	For litepoint init

* Format:		
		
* Argument:	none

* Author: 	kevin 2014-03-13
******************************************************************/
static void atcmd_lpinit(void)
{
        tls_cmd_disconnect_network(IEEE80211_MODE_INFRA | IEEE80211_MODE_AP);
        tls_litepoint_start();
}

/******************************************************************
* Description:	For litepoint test, set wireless channel

* Format:		AT+&LPCHL=[!?]<channel><CR>
			+OK<CR><LF><CR><LF>
		
* Argument:	channel:1-14

* Author: 	kevin 2014-03-12
******************************************************************/
int lpchl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){

    if((cmd->lpchl.channel<1) || (cmd->lpchl.channel >14)){
            TLS_DBGPRT_INFO("kevin params err! %x\r\n", cmd->lpchl.channel);
            return -CMD_ERR_INV_PARAMS;
    }

    atcmd_lpinit();
    tls_set_test_channel(cmd->lpchl.channel, cmd->lpchl.bandwidth);

    return 0;
}

/******************************************************************
* Description:	For litepoint test, start tx process

* Format:		AT+&LPTSTR=<temperaturecompensation>,<PacketCount>,<PsduLen>,<TxGain>,<DataRate>[,rifs][,greenfield][,gimode]<CR>
			+OK<CR><LF><CR><LF>
		
* Argument:	hex <temperaturecompensation>:255-compensation for different temperature;other value-no temperature;
				<PacketCount>, 0è¡¨ç¤ºé¿å
				<PsduLen>,     åé¿åº?				<TxGain>,      å¢çï¼ç´æ¥åå°Mac bd
				<DataRate>,    S2M = 0x0000,	S5M5 = 0x0001,	S11M = 0x0002,	L1M = 0x0003,	L2M = 0x0004,
	                             L5M5 = 0x0005,	L11M = 0x0006,
	                             R06M = 0x0100,	R09M = 0x0101,	R12M = 0x0102,	R18M = 0x0103, 	R24M = 0x0104,
	                             R36M = 0x0105,	R48M = 0x0106,	R54M = 0x0107,
	                             MCS0 = 0x0200,	MCS1 = 0x0201,	MCS2 = 0x0202,	MCS3 = 0x0203,	MCS4 = 0x0204,
	                             MCS5 = 0x0205,	MCS6 = 0x0206,	MCS7 = 0x0207,	MCS8 = 0x0208,	MCS9 = 0x0209,
	                             MCS10 = 0x0210,	MCS11 = 0x0211,	MCS12 = 0x0212,	MCS13 = 0x0213,	MCS14 = 0x0214,
	                             MCS15 = 0x0215,	MCS32 = 0x0232,
				  [rifs],        1: mac_txbd->ctrl1 |= (1 << 12)
				  [greenfield],  1: mac_txbd->ctrl4 |= (1 << 18)
				  [gimode],      1: mac_txbd->ctrl4 |= (1 << 19)
				  [timedelay]    åé´éæ¶é´ï¼åä½å¾®å¦ï¼?6è¿å¶
******************************************************************/
int lptstr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
	
    TLS_DBGPRT_INFO("tempcomp = 0x%x, PacketCount = 0x%x, PsduLen = 0x%x, TxGain = 0x%x, DataRate = 0x%x"
	    "rifs:0x%x,greenfield:0x%x, gimode:0x%x \r\n",
            cmd->lptstr.tempcomp, cmd->lptstr.packetcount, cmd->lptstr.psdulen,
            cmd->lptstr.txgain, cmd->lptstr.datarate, cmd->lptstr.rifs, cmd->lptstr.greenfield, cmd->lptstr.gimode);

    atcmd_lpinit();
    tls_tx_litepoint_test_start(cmd->lptstr.tempcomp,cmd->lptstr.packetcount, cmd->lptstr.psdulen, cmd->lptstr.txgain, cmd->lptstr.datarate, cmd->lptstr.gimode, cmd->lptstr.greenfield, cmd->lptstr.rifs);
    return 0;
}
int tls_lptperiod_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION *cmdrsp)
{
	if (set_opt)
	{
		tls_set_tx_litepoint_period(cmd->rxsin.rxlen);
	}
    return 0;
}

/******************************************************************
* Description:	For litepoint test, stop tx process

* Format:		AT+&LPTSTP<CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-13
******************************************************************/
int lptstp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    tls_txrx_litepoint_test_stop();	
    tls_lp_notify_lp_tx_data();
    return 0;
}

/******************************************************************
* Description:	For litepoint test, query tx infomation

* Format:		AT+&LPTSTT<CR>
			+OK=<TransCnt><CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-13
******************************************************************/
int lptstt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

/******************************************************************
* Description:	For litepoint test, start rx process

* Format:		AT+&LPRSTR=channel<CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	channel:1-14
			
* Author: 	kevin 2014-03-13
******************************************************************/
int lprstr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    if((cmd->lpchl.channel< 1) || (cmd->lpchl.channel > 14))
	{
		TLS_DBGPRT_INFO("Channel = 0x%x \r\n", cmd->lpchl.channel);
		return -CMD_ERR_INV_PARAMS;
	}
	atcmd_lpinit();
	tls_rx_litepoint_test_start(cmd->lpchl.channel, cmd->lpchl.bandwidth);
    return 0;
}

/******************************************************************
* Description:	For litepoint test, stop rx process

* Format:		AT+&LPRSTP<CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-13
******************************************************************/
int lprstp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    tls_txrx_litepoint_test_stop();

    return 0;
}

/******************************************************************
* Description:	For litepoint test, query rx infomation

* Format:		AT+&LPRSTT<CR>
			+OK=<TotalRecvCnt>,<CorrectRecvCnt>,<FcsErrorCnt><CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-13
******************************************************************/
int lprstt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

/******************************************************************
* Description:	For litepoint test, start the calibration process of rf's parameter(LO-Leakage)

* Format:		AT+&LPPSTR=<init_param>,<flag_start><CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	hex init_param: flag_start: 
			
* Author: 	kevin 2014-03-14
******************************************************************/
u8 gulCalFlag = 0;
int lppstr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{	
    return 0;
}

/******************************************************************
* Description:	For litepoint test, stop the calibration and return the result (IQ-Mismatch)

* Format:		AT+&LPPSTP=<result_param><CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	hex result_param: IQ-Mismatch
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lppstp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    return 0;
}

/******************************************************************
* Description:	For litepoint test, setting the parameter of RF

* Format:		AT+&LPRFPS=< rftype ><size><CR>[data stream] 
			+OK=<CR><LF><CR><LF>
			
* Argument:	ftypeï¼rfç±»å 0ï¼?230ï¼?ï¼?829ï¼?ï¼HEDrf
              	data stream ä¸­åå?6ä¸ªrfå¯å­å¨ï¼å?8ä¸ªä¿¡éå¯å­å¨
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lprfps_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

/******************************************************************
* Description:	For litepoint test,  receive and set channel

* Format:		AT+&LPCHRS =<channel>,< rxcbw ><CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	channel: æ çº¿ä¿¡éå·ï¼ææèå´1ï½?4
            		rxcbw: æ¥æ¶å¯¹åºä¿¡éå¸¦å®½0:  20Mï¼?ï¼?0M
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lpchrs_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return lpchl_proc(set_opt, update_flash, cmd, cmdrsp);
}

/******************************************************************
* Description:	For litepoint test,  BD Tx process

* Format:		AT+&LPTBD =< psdulen >,< txgain >,< datarate >< txcbw >,<gi>,<gf>,< rifs ><CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	psdulen: æ°æ®é¿åº¦ï¼ææèå?4ï½?5535
			txgain: åå°å¢ç
			datarate: æ°æ®æ°ç
			txcbw: åå°å¸¦å®½0:20M;1:40M
			gi:  0:normal gi;1:short gi
			gf:  0:no green field;1: green field
			rifs:  0:no rifs;1:rifs
			Data Rate: 
			S2M = 0x0000, S5.5M = 0x0001, S11M = 0x0002, L1M = 0x0003,
			L2M = 0x0004, L5M5 = 0x0005, L11M = 0x0006, 06M = 0x0100,
			09M = 0x0101, 12M = 0x0102, 18M = 0x0103, 24M = 0x0104,
			36M = 0x0105, 48M = 0x0106, 54M = 0x0107, MCS0 = 0x200,
			MCS1 = 0x201, MCS2 = 0x202, MCS3 = 0x203, MCS4 = 0x204,
			MCS5 = 0x205, MCS6 = 0x206, MCS7 = 0x207,
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lptbd_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return lptstr_proc(set_opt, update_flash, cmd, cmdrsp);
}

/******************************************************************
* Description:	For litepoint test,  stop tx process

* Format:		AT+&LPSTPT<CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lpstpt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return lptstp_proc(set_opt, update_flash, cmd, cmdrsp);
}

/******************************************************************
* Description:	For litepoint test, receive channel

* Format:		AT+&LPCHLR =<channel>,< rxcbw ><CR>
			+OK<CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lpchlr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return lprstr_proc(set_opt, update_flash, cmd, cmdrsp);
}

/******************************************************************
* Description:	For litepoint test,  stop rx process

* Format:		AT+&LPSTPR<CR>
			+OK<CR><LF><CR><LF>
	
* Argument:	
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lpstpr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return lprstp_proc(set_opt, update_flash, cmd, cmdrsp);
}

/******************************************************************
* Description:	For litepoint test, For query rx frame information

* Format:		AT+&LPRAGC <CR>
			+OK=<TotalRecvCnt>,<CorrectRecvCnt>,<FcsErrorCnt><CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lpragc_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

/******************************************************************
* Description:	For litepoint test,  For query rx frame information

* Format:		AT+&LPRSR [=?]<CR>
			+OK[=valid,rcpi,snr]<CR><LF><CR><LF>
			
* Argument:	
			
* Author: 	kevin 2014-03-14
******************************************************************/
int lprsr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    return 0;
}

extern int tls_tx_wave_start(u32 freq, u32 dividend);
int tls_tx_sin(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = -1;
	
	ret = tls_tx_wave_start(cmd->width.freq, cmd->width.dividend);

    return ret ? -CMD_ERR_OPS : 0;	
}

extern int tls_rx_data_from_adc(u32 datalen, char showtouart);
int tls_rx_wave(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = -1;
	
	ret = tls_rx_data_from_adc(cmd->rxsin.rxlen, cmd->rxsin.isprint);

    return ret;	
}


int tls_tx_lo_proc(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = -1;

	if (set_opt)
	{
		ret = tls_set_tx_lo((u8 *) &cmd->txLO.txlo);
	}else{
		ret = tls_get_tx_lo((u8 *) &cmdrsp->txLO.txlo);
	}

	return ret;
}


int tls_tx_iq_mismatch_proc(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = -1;

	if (set_opt)
	{
		ret = tls_set_tx_iq_gain((u8 *) &cmd->txIQ.txiqgain);
		ret = tls_set_tx_iq_phase((u8 *) &cmd->txIQ.txiqphase);
	}else{
		ret = tls_get_tx_iq_gain((u8 *) &cmdrsp->txIQ.txiqgain);
		ret = tls_get_tx_iq_phase((u8 *) &cmdrsp->txIQ.txiqphase);
	}

	return ret;
}

int tls_freq_error_proc(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = -1;

	if (set_opt)
	{
		ret = tls_freq_err_op((u8 *) &cmd->FreqErr.freqerr, 1);
		if (0 == ret)
		{
			extern void tls_rf_freqerr_adjust(int freqerr);
			tls_rf_freqerr_adjust(cmd->FreqErr.freqerr);
		}
	}else{
		ret = tls_freq_err_op((u8 *) &cmdrsp->FreqErr.freqerr, 0);
	}

	return ret;
}

int tls_rf_cal_finish_proc(u8 set_opt, u8 update_flah, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	int ret = 0;
    if (set_opt)
    {
		ret = tls_rf_cal_finish_op((u8 *) &cmd->calfin.val, 1);
    }
	else
	{
		ret = tls_rf_cal_finish_op((u8 *) &cmdrsp->calfin.val, 0);
		if (cmdrsp->calfin.val != 1)
		{
			cmdrsp->calfin.val = 0;
		}
	}

	return ret;
}


#if TLS_CONFIG_WIFI_PERF_TEST
/******************************************************************
* Description:	
As server: TEST UDP & TCP RX
AT+THT=Ss,-i=1
AT+THT=Ss

As client:
UDP TX:  AT+THT=Cc,192.168.1.100, UDP, -b=10K,-t=10,-i=1
			-b=0: full speed test
			K for kilo bps
			M for Mega bps

TCP TX: AT+THT=Cc,192.168.1.100, TCP, -l=1024,-t=10,-i=1
			-l: 1024 block size; prefer to x * 1024, l < 32
			
			
* Argument:	
			
******************************************************************/
int tht_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	extern int tht_start_iperf_test(char *arg[]);
	int ret = -1;
	ret = tht_start_iperf_test(((struct tls_atcmd_token_t *)cmd->tht.tok)->arg);
	switch(ret)
	{
		case 0:
			ret = 0;
		break;
		case -1:
			ret = -CMD_ERR_MEM;	
		break;
		default:
			ret = -CMD_ERR_INV_PARAMS;
		break;
	}
	return ret;
	
}
#endif

#if TLS_CONFIG_WIFI_PING_TEST
static int ping_parse_param(struct ping_param *para,
                            union HOSTIF_CMD_PARAMS_UNION *cmd)
{
    int ret = -1;

	strcpy(para->host, (char*)cmd->ping.ip);
	para->interval = cmd->ping.timeLimt;
	para->cnt = cmd->ping.cnt;
	para->src = cmd->ping.src;
	ret = cmd->ping.start;
	
//    strcpy(para->host, tok->arg[0]);
//    para->interval = atoi(tok->arg[1]);
//    para->flags = atoi(tok->arg[2]);
//    ret = atoi(tok->arg[3]);

    return ret;
}

/* AT+PING=HOST,INTERVAL(ms),T(0|1),START(1)
   AT+PING=HOST,INTERVAL(ms),T(0|1),STOP(0)
*/
static int ping_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    int ret = -1;
    struct ping_param para;

    memset(&para, 0, sizeof(para));

    cmdrsp->ping.ext = cmd->ping.ext;

    if (cmd->ping.ext)
    {
        ping_test_create_task();

	    ret = ping_parse_param(&para, cmd);
	    if (1 == ret)
	    {
	        ping_test_start(&para);
	        ret = 0;
	    }
	    else if(0 == ret)
	    {
	        ping_test_stop();
	        ret = 0;
	    }
	    else
	    {
	        ret = -CMD_ERR_INV_PARAMS;
	    }
    }
    else
    {
        ping_parse_param(&para, cmd);
        ret = ping_test_sync(&para);
        if (ret >= 0)
        {
            cmdrsp->ping.ttl = ret;
            ret = 0;
        }
    }

    return ret;
}
#endif


/*
For PIN:
1:
Step1:	   AT+WWPS=get_pin
		   Pin code will be responsed; User should input this Pin to AP; 
Step2: 	   AT+WWPS=start_pin
___________________________ 
2:
Step1: 	  AT+WWPS=!set_pin,xxxx
		  User can set an Pin code to device; User should input this Pin to AP ;
Step2: 	  AT+WWPS=start_pin

___________________________	
3: 	
Step1: 	  AT+WWPS=start_pin
		  Pin code is the default value, and stored in system during manufacturing;User should input this Pin to AP;

___________________________
For PBC:
4:
Step1:	 AT+WWPS=start_pbc

*/
#if TLS_CONFIG_WPS
int wwps_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    struct tls_param_wps tmp_wps;
    int err=0;
    
    if(set_opt){
        memset(&tmp_wps, 0, sizeof(struct tls_param_wps));
        if(cmd->wps.mode == 0){
            #if 0 ////Generate Pin Code randomly
			unsigned int rpin = 0;
			char val[13];
			int val_len;
			
			rpin = wps_generate_pin();
			val_len = os_snprintf(val, sizeof(val), "pin=%08d", rpin);
			#endif
			tls_cmd_get_wps_pin(&tmp_wps);
            memcpy(cmdrsp->wps.pin, tmp_wps.pin, WPS_PIN_LEN);
            cmdrsp->wps.pin_len = WPS_PIN_LEN;
            cmdrsp->wps.result = 1;
        }else if(cmd->wps.mode == 1){
            if(cmd->wps.pin_len != 8)
                return -CMD_ERR_INV_PARAMS;
            memcpy(tmp_wps.pin, cmd->wps.pin, cmd->wps.pin_len);
			tls_cmd_set_wps_pin(&tmp_wps, update_flash);
            cmdrsp->wps.result = 0;
        }else if(cmd->wps.mode == 2){
            err = tls_wps_start_pin();
            cmdrsp->wps.result = 0;
        }else if(cmd->wps.mode == 3){
            err = tls_wps_start_pbc();
            cmdrsp->wps.result = 0;
        }else
            err = 1;
    }
	else
	{
		err = 1;
	}
/*    else{
        err = tls_cmd_get_wps_params(&wps);
        cmdrsp->wps.mode=wps.mode;
        cmdrsp->wps.pin_len = wps.pin_len;
        memcpy(cmdrsp->wps.pin, wps.pin, cmdrsp->wps.pin_len);
        cmdrsp->wps.result = 2;
    }*/
    return err ? -CMD_ERR_INV_PARAMS : 0;
}
#endif

int custdata_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    u8 *data = NULL;
    data = tls_mem_alloc(65);
	if (data){
		memset(data, 0, 65);
		tls_wifi_get_oneshot_customdata(data);
       		cmdrsp->custdata.length = strlen((char *)data);
		memcpy(cmdrsp->custdata.data, data, cmdrsp->custdata.length);
		tls_mem_free(data);
		data = NULL;
		return 0;
	}else{
		return -CMD_ERR_MEM;
	}
}
int stand_by_power_down(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
#define CMD_START_Pos                         8U                                          /*!< CMD start position */
#define CMD_START_Msk                         (1UL << CMD_START_Pos)                         /*!< CMD start Mask */
	uint32_t val = 0;
	val = tls_reg_read32(HR_PMU_PS_CR);
//	TLS_DBGPRT_INFO("goto standby here\n");
	val |= BIT(0);
	tls_reg_write32(HR_PMU_PS_CR, val);

	/*Enter Deep Power Down Mode*/
	M32(HR_FLASH_CMD_ADDR) = 0xB9;
	M32(HR_FLASH_CMD_START) = CMD_START_Msk;
	return 0;
}
int cpu_state_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	csi_vic_set_wakeup_irq(UART0_IRQn);
	switch(cmd->width.freq)
	{
	case 0:
		__WAIT();
		break;
	case 1:
		__DOZE();
		break;
	case 2:
		__STOP();
		break;
	default:
		return -1;
	}
	printf("cpu state %d out\n", cmd->width.freq);
	return 0;
}
s32 tls_uart_bps_set(u8 portNum, u32 bdrate)
{
	u32 val, reg_addr;
	tls_sys_clk sysclk;

    reg_addr = HR_UART0_BAUD_RATE_CTRL + portNum*0x200;
	tls_sys_clk_get(&sysclk);
	val = (sysclk.apbclk*1000000/(16*bdrate) - 1)|(((sysclk.apbclk*1000000%(bdrate*16))*16/(bdrate*16))<<16);

	tls_reg_write32(reg_addr, val);

	return 0;
}
#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON  || WM_NIMBLE_INCLUDED == CFG_ON)
tls_bt_status_t bt_wait_rsp_timeout(enum tls_cmd_mode cmd_mode, union HOSTIF_CMD_PARAMS_UNION *cmd, struct tls_hostif *hif, int timeout_s)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;
	u32 offset = 0, time = 0;

	if(cmd_mode != CMD_MODE_HSPI_RICMD && cmd_mode != CMD_MODE_UART1_RICMD)
    {
        time = tls_os_get_time();
sem_acquire:
        ret = (tls_bt_status_t)tls_os_sem_acquire(hif->uart_atcmd_sem, timeout_s * HZ - offset);
        if (ret == (tls_bt_status_t)TLS_OS_SUCCESS)
        {
			if(!(hif->uart_atcmd_bits & (1 << UART_ATCMD_BIT_BT)))
			{
			    offset = tls_os_get_time() - time;
			    if (offset < timeout_s * HZ)
			    {
			        goto sem_acquire;    
			    }
			}
        }
	}

	return ret;
}

#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON)

static void bt_evt_cback(tls_bt_host_evt_t evt, tls_bt_host_msg_t *msg)
{
#define BLE_EVT_BUF_LEN 1248
	int ret = -1;
	int len = 0;
	char *buf = NULL;
	u8 hostif_type;
	int passive_response = 0;
	struct tls_hostif *hif = tls_get_hostif();

#if TLS_CONFIG_RMMS
	if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
	{
		hostif_type = HOSTIF_MODE_RMMS_AT;
	}
	else
#endif
	{
		if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
		{
			hostif_type = HOSTIF_MODE_UART0;
		}
		else
		{
			hostif_type = HOSTIF_MODE_UART1_LS;
		}
	}

	buf = tls_mem_alloc(BLE_EVT_BUF_LEN);
	if (!buf)
	{
		return;
	}


	switch(evt)
	{
		case WM_BT_ADAPTER_STATE_CHG_EVT:
			len = sprintf(buf, "+OK=0,%hhu\r\n", msg->adapter_state_change.status);
		    ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT_DM))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
			}else
			{
				passive_response =1;
			}
			break;
	    case WM_BT_ADAPTER_PROP_CHG_EVT:

			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT_DM_EXT))
			{
				if(msg->adapter_prop.status == TLS_BT_STATUS_SUCCESS)
				{
					if(msg->adapter_prop.num_properties == 1)
					{
						len = sprintf(buf, "+OK=0,");
						memcpy(buf+len, msg->adapter_prop.properties->val, msg->adapter_prop.properties->len);
						len += msg->adapter_prop.properties->len;
					}else
					{
						len = sprintf(buf, "+OK=0");
					}
				}else
				{
					len = sprintf(buf, "+OK=0,%hhu", msg->adapter_prop.status);
				}
				buf[len++] = '\r';
				buf[len++] = '\n';
				ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT_DM_EXT);
			}
			break;
	default:
		break;
		
	}

	if (ret)
		tls_mem_free(buf);

	if(passive_response == 1) return;

	hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_BT);
	tls_os_sem_release(hif->uart_atcmd_sem); 
	
}

#endif

int bt_enable_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON)
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btctrl.cmd_mode;
    
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);


	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM);

	ret = tls_at_bt_enable(cmd->btctrl.type, (tls_bt_log_level_t)cmd->btctrl.level, bt_evt_cback);
      
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT_DM);
		goto end_tag;
	}
	
	ret = bt_wait_rsp_timeout(cmd->btctrl.cmd_mode, cmd, hif, 5);
    
end_tag:
    
#else
    ret = tls_at_bt_enable(cmd->btctrl.type, (tls_bt_log_level_t)cmd->btctrl.level);
#endif 


	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;	

}

int bt_destory_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
	uint8_t flag = 0x00;
#if (WM_BLE_INCLUDED == CFG_ON)
	
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;
    
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
	
    ret = tls_at_bt_destroy();   
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT_DM);
		goto end_tag;
	}
	
	ret = bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 10);
    //cleanup bluedroid, free memory; note, when got adapter off evt, cleanup bluedroid
	tls_at_bt_cleanup_host();
	tls_cmd_get_oneshot(&flag);
	if(flag == 0x04)
	{
		//clear ble wifi config flag;
		tls_cmd_set_oneshot(0, 0);
	}
end_tag:
    
#else
     ret = tls_at_bt_destroy();
	tls_cmd_get_oneshot(&flag);
	if(flag == 0x04)
	{
		//clear ble wifi config flag;
		tls_cmd_set_oneshot(0, 0);
	}

#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;	
}

int bt_ctrl_get_status_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    cmdrsp->bt.status = (u16)tls_bt_controller_get_status();
	return 0;
}

int bt_sleep_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    if (cmd->bt.cmd)
    {
        ret = tls_bt_ctrl_sleep(TRUE);
    }
    else
    {
        ret = tls_bt_ctrl_sleep(FALSE);
    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_tx_power_set_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    ret = tls_ble_set_tx_power((tls_ble_power_type_t)cmd->btctrl.type, cmd->btctrl.level);

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_tx_power_get_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    cmdrsp->bt.status = (u8)tls_ble_get_tx_power((tls_ble_power_type_t)cmd->btctrl.type);

	return 0;
}

int bt_tx_power_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    if (set_opt)
    {
        ret = tls_bredr_set_tx_power((int8_t)cmd->btctrl.type, (int8_t)cmd->btctrl.level);
    }
    else
    {
    	tls_bredr_get_tx_power((int8_t *)&cmdrsp->blepow.min, (int8_t *)&cmdrsp->blepow.max);
        ret = TLS_BT_STATUS_SUCCESS;
    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int bt_test_mode_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;
    tls_bt_hci_if_t hci_if;

    if (cmd->bt.cmd == 1)
    {
        /*default uart1 will be used*/
        hci_if.uart_index = 1;
        ret = enable_bt_test_mode(&hci_if);
    }
    else if (cmd->bt.cmd == 0)
    {
        ret = exit_bt_test_mode();

    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int bt_mac_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){
    int ret;
    
    if (set_opt) {
    	if(cmd->mac.length>12)
    	{
   		    return -CMD_ERR_INV_PARAMS;
        }
		ret = tls_set_bt_mac_addr(cmd->mac.macaddr);
    }else{
        ret = tls_get_bt_mac_addr(cmdrsp->mac.addr);
    }
    return ret;
}
int bt_name_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp){

    int ret = TLS_BT_STATUS_SUCCESS;
    
    #if (WM_NIMBLE_INCLUDED == CFG_ON)
    
    if (set_opt) {
		ret = tls_ble_gap_set_name(cmd->btname.name, update_flash);
    }else{
        ret = tls_ble_gap_get_name(cmdrsp->btname.name);
        if(ret > 0)
        {
            ret = TLS_BT_STATUS_SUCCESS;
        }
    }
    #else
	struct tls_hostif *hif = tls_get_hostif();
    tls_bt_property_t prop;
	prop.type = WM_BT_PROPERTY_BDNAME;
	prop.len = cmd->btname.len;     ////name length;
    prop.val = cmd->btname.name;  ////name value;
    

    hif->last_bt_cmd_mode = cmd->btname.cmd_mode;
    
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM_EXT);
    if (set_opt) {    
	    ret = tls_bt_set_adapter_property(&prop, update_flash);
    }else
    {
        ret = tls_bt_get_adapter_property(WM_BT_PROPERTY_BDNAME);
    }
      
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT_DM_EXT);
		goto end_tag;
	}
	
	ret = bt_wait_rsp_timeout(cmd->btctrl.cmd_mode, cmd, hif, 5);  
end_tag:     
    #endif
    
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int bt_rf_mode_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;
    tls_rf_bt_mode(cmd->bt.cmd);
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;    
}


#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)

int bt_audio_sink_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    if (cmd->bt.cmd == 1)
    {
        ret = tls_bt_enable_a2dp_sink();
    }
    else if (cmd->bt.cmd == 0)
    {
        ret = tls_bt_disable_a2dp_sink();

    }
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
#endif

#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)

int bt_hfp_client_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    if (cmd->bt.cmd == 1)
    {
        ret = tls_bt_enable_hfp_client();
    }
    else if (cmd->bt.cmd == 0)
    {
        ret = tls_bt_disable_hfp_client();

    }
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int bt_dial_up_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    ret = tls_bt_dial_number(cmd->btname.name);
    
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

#endif

#if (WM_BTA_SPPC_INCLUDED == CFG_ON)

int bt_spp_client_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    if (cmd->bt.cmd == 1)
    {
        ret = tls_bt_enable_spp_client();
    }
    else if (cmd->bt.cmd == 0)
    {
       ret = tls_bt_disable_spp_client();

    }
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
#endif

#if (WM_BTA_SPPS_INCLUDED == CFG_ON)

int bt_spp_server_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    if (cmd->bt.cmd == 1)
    {
        ret = tls_bt_enable_spp_server();
    }
    else if (cmd->bt.cmd == 0)
    {
       ret = tls_bt_disable_spp_server();

    }
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
#endif

#if (WM_BT_INCLUDED == CFG_ON)

int bt_scan_mode_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    ret = demo_bt_scan_mode(cmd->bt.cmd);

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int bt_inquiry_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

    ret = demo_bt_inquiry(cmd->bt.cmd);

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

#endif

#endif

#if (WM_BLE_INCLUDED == CFG_ON  || WM_NIMBLE_INCLUDED == CFG_ON)

#if (WM_MESH_INCLUDED == CFG_ON )

void ble_mesh_evt_cback(tls_mesh_event_t event, tls_mesh_msg_t *p_data)
{
#define RESP_BUF_LEN 256

    int ret = -1;
    int len = 0;
    u16 i=0;
    char *buf;
    u8 hostif_type;
    int passive_response = 0;
    
    struct tls_hostif *hif = tls_get_hostif();
    u32 time = tls_os_get_time();
    u32 hour,min,sec,ms = 0;

    sec = time/HZ;
    ms = (time%HZ)*2;
    hour = sec/3600;
    min = (sec%3600)/60;
    sec = (sec%3600)%60;

    
#if TLS_CONFIG_RMMS
    if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
    {
        hostif_type = HOSTIF_MODE_RMMS_AT;
    }
    else
#endif
    {
        if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
        {
            hostif_type = HOSTIF_MODE_UART0;
        }
        else
        {
            hostif_type = HOSTIF_MODE_UART1_LS;
        }
    }

    buf = tls_mem_alloc(RESP_BUF_LEN);
    if (!buf)
    {
        printf("alloc failed\r\n");
        return;
    }

    switch(event)
    {
       
        case WM_MESH_UNPROVISION_BEACON_EVT:
            {
                tls_mesh_unprov_msg_t *msg = (tls_mesh_unprov_msg_t *)&p_data->unprov_msg;
   
                len = sprintf(buf, "+OK=<%d:%02d:%02d.%03d>%02X%02X%02X%02X%02X%02X,", hour,min, sec, ms,
                              msg->addr[0], msg->addr[1], msg->addr[2],
                              msg->addr[3], msg->addr[4], msg->addr[5]);
                for(i = 0; i<16; i++)
                {
                    len += sprintf(buf+len, "%02x", msg->uuid[i]);
                }

                len += sprintf(buf+len, ",%04x\r\n\r\n" , msg->oob_info);

                buf[len++] = 0x00;

                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =1;
            }
            break;  
        case WM_MESH_NODE_ADDED_EVT:
            {
                tls_mesh_node_added_msg_t *msg = (tls_mesh_node_added_msg_t *)&p_data->node_added_msg;

                len = sprintf(buf, "+OK=%04X,%04X,%02X\r\n\r\n", msg->net_idx, msg->addr, msg->num_elem);
                
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response = 1;

            }
            break;  
        case WM_MESH_OOB_STRING_EVT:
            {
                tls_mesh_oob_output_str_msg_t *msg = (tls_mesh_oob_output_str_msg_t *)&p_data->oob_output_string_msg;

                len = sprintf(buf, "+OK=%s\r\n\r\n", msg->str);
                
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =1;

            }
            break;  
        case WM_MESH_OOB_NUMBER_EVT:
            {
                tls_mesh_oob_output_number_msg_t *msg = (tls_mesh_oob_output_number_msg_t *)&p_data->oob_output_number_msg;

                len = sprintf(buf, "+OK=%d\r\n\r\n", msg->number);
                
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =1;

            }
            break; 
        case WM_MESH_OOB_INPUT_EVT:
            {
                tls_mesh_oob_input_msg_t *msg = (tls_mesh_oob_input_msg_t *)&p_data->oob_input_msg;

                len = sprintf(buf, "+OK=%08x\r\n\r\n", msg->act);
                
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =1;
            }
        case WM_MESH_PROV_CMPLT_EVT: //Node role, was provisioned complete;
            {
                tls_mesh_prov_complete_msg_t *msg = (tls_mesh_prov_complete_msg_t *)&p_data->prov_cmplt_msg;

                len = sprintf(buf, "+OK=%04X,0x%04X\r\n\r\n", msg->net_idx, msg->addr);
                
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response = 1;

            }
            break;  
        case WM_MESH_PROV_END_EVT: //Provisioner role, provision process ended;
            {
                tls_mesh_prov_end_msg_t *msg = (tls_mesh_prov_end_msg_t *)&p_data->prov_end_msg;
                len = sprintf(buf, "+OK=%hu,%04X,%04X,%02X\r\n\r\n",(uint8_t)!msg->success, msg->net_idx, msg->addr, msg->num_elem);             
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
                {
                    passive_response =0;
                    hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
                }else
                {
                    passive_response =1;
                }                
            }
            break;
        default:
            break;
    }
   
    if (ret)
        tls_mem_free(buf);
    
    if(passive_response == 1) {
        return;
    }
    hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_BT);
    tls_os_sem_release(hif->uart_atcmd_sem); 
    
}
int ble_mesh_uart_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_erase_cfg_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_erase_cfg();
#else
    ret = CMD_ERR_OPS;
#endif
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_read_cfg_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();
    tls_mesh_primary_cfg_t primary_cfg;

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_get_cfg(&primary_cfg);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_primary_cfg.status = ret;
    cmdrsp->mesh_primary_cfg.net_transmit_count = primary_cfg.net_transmit_count;
    cmdrsp->mesh_primary_cfg.net_transmit_intvl = primary_cfg.net_transmit_intvl;
    cmdrsp->mesh_primary_cfg.relay = primary_cfg.relay;
    cmdrsp->mesh_primary_cfg.relay_retransmit_count = primary_cfg.relay_retransmit_count;
    cmdrsp->mesh_primary_cfg.relay_retransmit_intvl = primary_cfg.relay_retransmit_intvl;
    cmdrsp->mesh_primary_cfg.beacon = primary_cfg.beacon;
    cmdrsp->mesh_primary_cfg.gatt_proxy = primary_cfg.gatt_proxy;
    cmdrsp->mesh_primary_cfg.frnd = primary_cfg.frnd;
    cmdrsp->mesh_primary_cfg.default_ttl = primary_cfg.default_ttl;
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_read_primary_addr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();
    u16_t primary_addr;

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_get_primary_addr((u16_t*)&primary_addr);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = primary_addr;
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_change_default_ttl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_change_ttl((u8_t)cmd->btparamnum.param);
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_change_primary_addr_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_change_primary_addr((u16_t)cmd->btparamnum.param);
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_clear_rpl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_clear_local_rpl();
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_vnd_send_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_gen_set_level_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    s16 state;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamonoffset.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_gen_level_set(cmd->btparamonoffset.net_idx, cmd->btparamonoffset.dst, cmd->btparamonoffset.app_idx, cmd->btparamonoffset.val ,&state);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = state;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}  

int ble_mesh_gen_get_level_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    s16 state;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamonoffget.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_gen_level_get(cmd->btparamonoffget.net_idx, cmd->btparamonoffget.dst, cmd->btparamonoffget.app_idx,&state);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = state;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}    

int ble_mesh_gen_pub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamonoffget.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_gen_off_publish((uint8_t)cmd->bt.cmd);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = ret;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_gen_get_onoff_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 state;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamonoffget.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_gen_onoff_get(cmd->btparamonoffget.net_idx, cmd->btparamonoffget.dst, cmd->btparamonoffget.app_idx,&state);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = state;


    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_gen_set_onoff_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 state;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamonoffset.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_gen_onoff_set(cmd->btparamonoffset.net_idx, cmd->btparamonoffset.dst, cmd->btparamonoffset.app_idx,cmd->btparamonoffset.val, &state);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = state;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_set_hb_sub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_hb_sub hb_sub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamhbsubset.cmd_mode;

    hb_sub.max = cmd->btparamhbsubset.hb_sub_max;
    hb_sub.min = cmd->btparamhbsubset.hb_sub_min;
    hb_sub.count = cmd->btparamhbsubset.hb_sub_count;
    hb_sub.period = cmd->btparamhbsubset.hb_sub_period;
    hb_sub.dst = cmd->btparamhbsubset.hb_sub_dst;
    hb_sub.src = cmd->btparamhbsubset.hb_sub_src;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_hb_sub_set(cmd->btparamhbsubset.net_idx, cmd->btparamhbsubset.net_dst, &hb_sub, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_get_hb_sub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_hb_sub hb_sub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamhbsubset.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_hb_sub_get(cmd->btparamhbsubset.net_idx, cmd->btparamhbsubset.net_dst, &hb_sub, &status);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->mesh_hb_sub.status = status;
    cmdrsp->mesh_hb_sub.min = hb_sub.min;
    cmdrsp->mesh_hb_sub.max = hb_sub.max;
    cmdrsp->mesh_hb_sub.count = hb_sub.count;
    cmdrsp->mesh_hb_sub.period = hb_sub.period;
    cmdrsp->mesh_hb_sub.dst = hb_sub.dst;
    cmdrsp->mesh_hb_sub.src = hb_sub.src;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}



int ble_mesh_set_hb_pub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_hb_pub hb_pub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamhbpubset.cmd_mode;

    hb_pub.dst = cmd->btparamhbpubset.hb_pub_dst;
    hb_pub.count = cmd->btparamhbpubset.hb_pub_count;
    hb_pub.period = cmd->btparamhbpubset.hb_pub_period;
    hb_pub.ttl = cmd->btparamhbpubset.hb_pub_ttl;
    hb_pub.feat = cmd->btparamhbpubset.hb_pub_feat;
    hb_pub.net_idx = cmd->btparamhbpubset.hb_pub_net_idx;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_hb_pub_set(cmd->btparamhbpubset.net_idx, cmd->btparamhbpubset.net_dst, &hb_pub, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_get_hb_pub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_hb_pub hb_pub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamhbpubset.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_hb_pub_get(cmd->btparamhbpubset.net_idx, cmd->btparamhbpubset.net_dst, &hb_pub, &status);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->mesh_hb_pub.status = status;
    cmdrsp->mesh_hb_pub.net_idx = hb_pub.net_idx;
    cmdrsp->mesh_hb_pub.feat = hb_pub.feat;
    cmdrsp->mesh_hb_pub.ttl = hb_pub.ttl;
    cmdrsp->mesh_hb_pub.period = hb_pub.period;
    cmdrsp->mesh_hb_pub.count = hb_pub.count;
    cmdrsp->mesh_hb_pub.dst = hb_pub.dst;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}




int ble_mesh_set_pub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_mod_pub pub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparampubset.cmd_mode;

    pub.addr = cmd->btparampubset.pub_addr;
    pub.app_idx = cmd->btparampubset.pub_app_idx;
    pub.cred_flag = cmd->btparampubset.pub_cred_flag;
    pub.ttl = cmd->btparampubset.pub_ttl;
    pub.period = cmd->btparampubset.pub_period;
    pub.transmit = TLS_BT_MESH_PUB_TRANSMIT(cmd->btparampubset.pub_count, cmd->btparampubset.pub_interval);


#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_pub_set(cmd->btparampubset.net_idx, cmd->btparampubset.dst, cmd->btparampubset.elem_addr, 
                    cmd->btparampubset.mod_id, cmd->btparampubset.cid, &pub, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_get_pub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    tls_bt_mesh_cfg_mod_pub pub;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamsubadd.cmd_mode;


#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_pub_get(cmd->btparamsubget.net_idx, cmd->btparamsubget.dst, cmd->btparamsubget.elem_addr, 
                    cmd->btparamsubget.mod_id, cmd->btparamsubget.cid, &pub, &status);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->mesh_pub.status = status;
    cmdrsp->mesh_pub.addr = pub.addr;
    cmdrsp->mesh_pub.app_idx = pub.app_idx;
    cmdrsp->mesh_pub.cred_flag = pub.cred_flag;
    cmdrsp->mesh_pub.ttl = pub.ttl;
    cmdrsp->mesh_pub.period = pub.period;
    cmdrsp->mesh_pub.transmit = pub.transmit;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_add_sub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamsubadd.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_sub_add(cmd->btparamsubadd.net_idx, cmd->btparamsubadd.dst, cmd->btparamsubadd.elem_addr, 
                    cmd->btparamsubadd.sub_addr, cmd->btparamsubadd.mod_id, cmd->btparamsubadd.cid, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_del_sub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamsubadd.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_sub_del(cmd->btparamsubadd.net_idx, cmd->btparamsubadd.dst, cmd->btparamsubadd.elem_addr, 
                    cmd->btparamsubadd.sub_addr,cmd->btparamsubadd.mod_id, cmd->btparamsubadd.cid, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_get_sub_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status;
    u16 sub[16];
    u32 i, sub_cnt = 16;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamsubget.cmd_mode;    

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_sub_get(cmd->btparamsubget.net_idx, cmd->btparamsubget.dst, cmd->btparamsubget.elem_addr, 
                    cmd->btparamsubget.mod_id, cmd->btparamsubget.cid, &status, &sub[0], &sub_cnt);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->mesh_sub.status = status;
    cmdrsp->mesh_sub.sub_cnt = sub_cnt;

    for(i = 0; i<sub_cnt; i++)
    {
        cmdrsp->mesh_sub.subs[i] = sub[i];
    }

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_get_friend_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 val;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt2param.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_friend_get(cmd->bt2param.param1, cmd->bt2param.param2,&val);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = val;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

    
int ble_mesh_set_friend_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 val;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamproxy.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_friend_set(cmd->btparamproxy.net_idx, cmd->btparamproxy.dst, cmd->btparamproxy.val, &val);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = val;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_get_proxy_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 proxy;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt2param.cmd_mode;

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_proxy_get(cmd->bt2param.param1, cmd->bt2param.param2,&proxy);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = proxy;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

    
int ble_mesh_set_proxy_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 proxy;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamproxy.cmd_mode;
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_proxy_set(cmd->btparamproxy.net_idx, cmd->btparamproxy.dst, cmd->btparamproxy.val, &proxy);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->mesh_resp.status = ret;
    cmdrsp->mesh_resp.state = proxy;


    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_get_relay_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 relay;
    u8 transmit;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt2param.cmd_mode;
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_relay_get(cmd->bt2param.param1, cmd->bt2param.param2, &relay, &transmit);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_relay.status = ret;
    cmdrsp->mesh_relay.state = relay;
    cmdrsp->mesh_relay.count = TLS_BT_MESH_TRANSMIT_COUNT(transmit);
    cmdrsp->mesh_relay.interval = TLS_BT_MESH_TRANSMIT_INT(transmit);

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

    
int ble_mesh_set_relay_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    u8 status= -1;
    u8 transmit;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamrelay.cmd_mode;   
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_relay_set(cmd->btparamrelay.net_idx, cmd->btparamrelay.dst, cmd->btparamrelay.val, cmd->btparamrelay.count,cmd->btparamrelay.interval, &status, &transmit);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->mesh_relay.status = ret;
    cmdrsp->mesh_relay.state = cmd->btparamrelay.val;
    cmdrsp->mesh_relay.count = TLS_BT_MESH_TRANSMIT_COUNT(transmit);
    cmdrsp->mesh_relay.interval = TLS_BT_MESH_TRANSMIT_INT(transmit);

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_unbind_app_key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    uint8_t status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparambak.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON) 
    ret = tls_ble_mesh_unbind_app_key(cmd->btparambak.net_idx, cmd->btparambak.dst, cmd->btparambak.elem_addr, cmd->btparambak.app_key_idx, cmd->btparambak.mod_id,cmd->btparambak.cid, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_bind_app_key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    uint8_t status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparambak.cmd_mode;
    
#if (WM_NIMBLE_INCLUDED == CFG_ON) 
    ret = tls_ble_mesh_bind_app_key(cmd->btparambak.net_idx, cmd->btparambak.dst, cmd->btparambak.elem_addr, cmd->btparambak.app_key_idx, cmd->btparambak.mod_id,cmd->btparambak.cid, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_add_app_key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    uint8_t status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamark.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_add_app_key(cmd->btparamark.net_idx, cmd->btparamark.addr, cmd->btparamark.key_net_idx, cmd->btparamark.key_app_idx,cmd->btparamark.param, &status);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_add_local_app_key_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamalk.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_add_local_app_key(cmd->btparamalk.net_idx, cmd->btparamalk.app_idx, cmd->btparamalk.param);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->bt.status = ret;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_oob_string_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamstr.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_input_oob_string((const char*)cmd->btparamstr.param);
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_oob_number_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btparamnum.cmd_mode;    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_input_oob_number(cmd->btparamnum.param);
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_reset_node_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    uint8_t status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt2param.cmd_mode;
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_node_reset(cmd->bt2param.param1, cmd->bt2param.param2, &status);
#else
    ret = CMD_ERR_OPS;
#endif
    cmdrsp->bt.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_config_node_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_prov_node_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->btprov.cmd_mode;
    hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
    hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT); 
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_provisioner_prov_adv(cmd->btprov.uuid, cmd->btprov.net_idx, cmd->btprov.addr, cmd->btprov.attention);
#else
    ret = CMD_ERR_OPS;
#endif
 
    if(ret != TLS_BT_STATUS_SUCCESS)
    {
        hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT);
        goto end_tag;        
    }  
    
    bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 30);

end_tag:
    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_get_comp_data_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    uint8_t status;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt2param.cmd_mode;
    
    cmdrsp->comp_data.data_len = sizeof(cmdrsp->comp_data.data);

#if (WM_NIMBLE_INCLUDED == CFG_ON)
    ret = tls_ble_mesh_get_comp(cmd->bt2param.param1, cmd->bt2param.param2, &status, cmdrsp->comp_data.data, &cmdrsp->comp_data.data_len);
#else
    ret = CMD_ERR_OPS;
#endif

    cmdrsp->comp_data.status = status;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_scan_prov_node_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;    
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_ble_mesh_provisioner_scan(false);
            break;
        case 1:
            ret = tls_ble_mesh_provisioner_scan(true);
            break;
    }
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_scan_unprov_node_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;    
    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_ble_mesh_provisioner_scan(false);
            break;
        case 1:
            ret = tls_ble_mesh_provisioner_scan(true);
            break;
    }
#else
    ret = CMD_ERR_OPS;
#endif

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_mesh_node_demo_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;

    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_ble_mesh_node_deinit(0);
            break;
        case 1:
            ret = tls_ble_mesh_node_init();
            break;
    }
 #else
    ret = CMD_ERR_OPS;
 #endif
 
    cmdrsp->bt.status = ret;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_node_enable_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;

    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_ble_mesh_deinit();
            break;
        case 1:
            ret = tls_ble_mesh_init(ble_mesh_evt_cback, MESH_ROLE_NODE, true);
            break;
    }
 #else
    ret = CMD_ERR_OPS;
 #endif
 
    cmdrsp->bt.status = ret;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_mesh_provisioner_enable_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;

    
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_ble_mesh_deinit();
            break;
        case 1:
            ret = tls_ble_mesh_init(ble_mesh_evt_cback, MESH_ROLE_PROVISIONER, true);
            break;
    }
 #else
    ret = CMD_ERR_OPS;
 #endif
 
    cmdrsp->bt.status = ret;

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
#endif


int ble_adv_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
#if (WM_NIMBLE_INCLUDED == CFG_ON)
    switch(cmd->bt.cmd)
    {
        case 0:
            ret = tls_nimble_gap_adv(/*WM_BLE_ADV_STOP*/0, 0);
            break;
        case 1:
            ret = tls_nimble_gap_adv(/*WM_BLE_ADV_IND*/1, 0);
            break;
    }
 #else
    ret = tls_ble_gap_adv(cmd->bt.cmd, 0);
 #endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int ble_demo_adv_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    ret = tls_ble_demo_adv(cmd->bt.cmd);

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_adv_data_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
#if (WM_BLE_INCLUDED == CFG_ON)
	tls_ble_dm_adv_data_t adv_data;
	memset(&adv_data, 0, sizeof(adv_data));
	adv_data.pure_data = true;
	if(cmd->bleadv.len >0)
	{
		adv_data.manufacturer_len = cmd->bleadv.len;
		memcpy(adv_data.manufacturer_data, cmd->bleadv.data, cmd->bleadv.len);
	}
    ret = tls_ble_set_adv_data(&adv_data);
#else
	if(cmd->bleadv.len >0)
	{
        ret = tls_ble_gap_set_data(WM_BLE_ADV_DATA, cmd->bleadv.data, cmd->bleadv.len);
	}    
#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int ble_scan_rsp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
#if (WM_BLE_INCLUDED == CFG_ON)
	tls_ble_dm_adv_data_t adv_data;
	memset(&adv_data, 0, sizeof(adv_data));
	adv_data.set_scan_rsp = true;
	adv_data.pure_data = true;
	if(cmd->bleadv.len >0)
	{
		adv_data.manufacturer_len = cmd->bleadv.len;
		memcpy(adv_data.manufacturer_data, cmd->bleadv.data, cmd->bleadv.len);
	}
    ret = tls_ble_set_adv_data(&adv_data);
#else
	if(cmd->bleadv.len >0)
	{
        ret = tls_ble_gap_set_data(WM_BLE_ADV_RSP_DATA, cmd->bleadv.data, cmd->bleadv.len);
	}
#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_adv_param_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;
#if (WM_BLE_INCLUDED == CFG_ON)

    tls_ble_dm_adv_ext_param_t adv_ext_param;
    adv_ext_param.adv_int_min = cmd->bleprm.adv_int_min;
    adv_ext_param.adv_int_max = cmd->bleprm.adv_int_max;
    adv_ext_param.adv_type = cmd->bleprm.adv_type;
    adv_ext_param.own_addr_type = cmd->bleprm.own_addr_type;
    adv_ext_param.peer_addr_type = cmd->bleprm.peer_addr_type;
    adv_ext_param.chnl_map = cmd->bleprm.channel_map;
    adv_ext_param.afp = cmd->bleprm.adv_filter_policy;
    if(0 == (cmd->bleprm.peer_addr[0]|cmd->bleprm.peer_addr[1]|cmd->bleprm.peer_addr[2]|cmd->bleprm.peer_addr[3]|cmd->bleprm.peer_addr[4]|cmd->bleprm.peer_addr[5]))
    {
        adv_ext_param.dir_addr = NULL; //cmd->bleprm.peer_addr;
    }else
    {
        adv_ext_param.dir_addr = cmd->bleprm.peer_addr;
    }

    ret = tls_ble_set_adv_ext_param(&adv_ext_param);
#else
    ret = tls_ble_gap_set_adv_param(cmd->bleprm.adv_type, cmd->bleprm.adv_int_min, cmd->bleprm.adv_int_max, cmd->bleprm.channel_map, cmd->bleprm.adv_filter_policy, cmd->bleprm.peer_addr, cmd->bleprm.peer_addr_type);
#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_scan_param_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
#if (WM_BLE_INCLUDED == CFG_ON)
    ret = tls_ble_set_scan_param((int)cmd->bleprm.adv_int_min, (int)cmd->bleprm.adv_int_max, cmd->bleprm.adv_type);
#else
    ret = tls_ble_gap_set_scan_param(cmd->bleprm.adv_int_min, cmd->bleprm.adv_int_max, 0, 0, 1, 0);
#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_scan_filter_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
//TODO: filter ???
	return 0;
}



#if (WM_BLE_INCLUDED == CFG_ON)
static u8 get_valid_adv_length_and_name(const uint8_t *ptr, uint8_t *pname)
{
	u8 ret = 0, seg_len = 0, min_len = 0;
    
	if(ptr == NULL) return ret;
    
	seg_len = ptr[0];
	while(seg_len != 0)
	{
	    if(ptr[ret+1] == 0x09 || ptr[ret+1] == 0x08)
	    {
	    	min_len = MIN(64, seg_len-1);
	    	memcpy(pname, ptr+ret+2, min_len);
			pname[min_len] = 0;
			asm("nop");asm("nop");asm("nop");asm("nop");
	    }
		ret += (seg_len+1); //it self 1 byte;
		seg_len = ptr[ret];
		if(ret >=64) break; //sanity check;
	}
	
	return ret;
}

static void ble_report_evt_cb(tls_ble_dm_evt_t event, tls_ble_dm_msg_t *p_data)
{
    if((event != WM_BLE_DM_SCAN_RES_EVT) && (event != WM_BLE_DM_SCAN_RES_CMPL_EVT) && (event != WM_BLE_DM_REPORT_RSSI_EVT)) return;
    
#define BLE_SCAN_RESULT_LEN 256

    int ret = -1, index = 0;
    int len = 0, tmp_len= 0;
    u16 uuid = 0, i=0;
    char *buf;
    u8 hostif_type;
    int passive_response = 0;
    
    struct tls_hostif *hif = tls_get_hostif();
    u32 time = tls_os_get_time();
    u32 hour,min,sec,ms = 0;

    sec = time/HZ;
    ms = (time%HZ)*2;
    hour = sec/3600;
    min = (sec%3600)/60;
    sec = (sec%3600)%60;

    
#if TLS_CONFIG_RMMS
    if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
    {
        hostif_type = HOSTIF_MODE_RMMS_AT;
    }
    else
#endif
    {
        if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
        {
            hostif_type = HOSTIF_MODE_UART0;
        }
        else
        {
            hostif_type = HOSTIF_MODE_UART1_LS;
        }
    }

    buf = tls_mem_alloc(BLE_SCAN_RESULT_LEN);
    if (!buf)
    {
        printf("alloc failed\r\n");
        return;
    }

    switch(event)
    {
       
        case WM_BLE_DM_SCAN_RES_EVT:
            {
                tls_ble_dm_scan_res_msg_t *msg = (tls_ble_dm_scan_res_msg_t *)&p_data->dm_scan_result;

                u8 valid_len;
            	u8 device_name[64] = {0};
                struct tls_hostif *hif = tls_get_hostif();
                
                memset(buf, 0, BLE_SCAN_RESULT_LEN);
            	memset(device_name, 0, sizeof(device_name));
            	valid_len = get_valid_adv_length_and_name(msg->value, device_name);
                if(valid_len > 62)
                {
                	//printf("###warning(%d)###\r\n", valid_len);
                	valid_len = 62;
                }
            	len = sprintf(buf, "<%d:%02d:%02d.%03d>%02X%02X%02X%02X%02X%02X,%d,", hour,min, sec, ms,
                              msg->address[0], msg->address[1], msg->address[2],
                              msg->address[3], msg->address[4], msg->address[5], msg->rssi);

            	if(device_name[0] != 0x00)
                {
                	len += sprintf(buf +len, "\"%s\",", device_name);
                }else
                {
                	len += sprintf(buf+len, "\"\",");
                }
                
                for (i = 0; i < valid_len; i++)
                {
                    len += sprintf(buf + len, "%02X", msg->value[i]);
                }

                len += sprintf(buf + len, "\r\n");
            	buf[len++] = '\0';
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =1;
            }
            break;  
        case WM_BLE_DM_SCAN_RES_CMPL_EVT:
            {
                tls_ble_dm_scan_res_cmpl_msg_t *msg = (tls_ble_dm_scan_res_cmpl_msg_t *)&p_data->dm_scan_result_cmpl;
                if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT_DM))
                {
                    len = sprintf(buf, "+OK=%hhu,%hhu\r\n", 0,msg->num_responses);
                    ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                    passive_response =0;
                    hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
                }else
                {
                    passive_response =1;
                }
            }
            break;  
        case WM_BLE_DM_REPORT_RSSI_EVT:
            {
                tls_ble_report_rssi_msg_t *msg = (tls_ble_report_rssi_msg_t*)&p_data->dm_scan_result_cmpl;
                if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT_DM))
                {
                    len = sprintf(buf, "+OK=%hhu,%d\r\n", msg->status,msg->rssi);
                    ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                    passive_response =0;
                    hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
                }else
                {
                    passive_response =1;
                }
            }
            break;            
        default:
            break;
    }
   
    if (ret)
        tls_mem_free(buf);
    
    if(passive_response == 1) {
        return;
    }
    hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_BT);
    tls_os_sem_release(hif->uart_atcmd_sem); 
	
}
#else

static void ble_gap_scan_at_callback(int type, int8_t rssi, uint8_t *addr, const uint8_t *name, int name_len, const uint8_t *raw_scan_resp, int raw_scan_resp_length)
{
#define BLE_SCAN_RESULT_LEN 256
    
    int ret = -1;
    int len = 0;
    u16 i=0;
    char *buf;
    u8 hostif_type;
    int passive_response = 0;
    
    struct tls_hostif *hif = tls_get_hostif();
    u32 time = tls_os_get_time();
    u32 hour,min,sec,ms = 0;

    sec = time/HZ;
    ms = (time%HZ)*2;
    hour = sec/3600;
    min = (sec%3600)/60;
    sec = (sec%3600)%60;
    
        
#if TLS_CONFIG_RMMS
    if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
    {
        hostif_type = HOSTIF_MODE_RMMS_AT;
    }
    else
#endif
    {
        if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
        {
            hostif_type = HOSTIF_MODE_UART0;
        }
        else
        {
            hostif_type = HOSTIF_MODE_UART1_LS;
        }
    }

    buf = tls_mem_alloc(BLE_SCAN_RESULT_LEN);
    if (!buf)
    {
        printf("alloc failed\r\n");
        return ;
    }

    switch (type) {
    case WM_BLE_GAP_EVENT_DISC:
        {
            u8 device_name[64] = {0};
            
            memset(device_name, 0, sizeof(device_name));
            if(name_len)
            {
                memcpy(device_name, name, MIN(62, name_len));
            }
            
            len = sprintf(buf, "<%d:%02d:%02d.%03d>%02X%02X%02X%02X%02X%02X,%d,", hour,min, sec, ms,
                          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], rssi);
            
            if(device_name[0] != 0x00)
            {
                len += sprintf(buf +len, "\"%s\",", device_name);
            }else
            {
                len += sprintf(buf+len, "\"\",");
            }
            
            for (i = 0; i < raw_scan_resp_length; i++)
            {
                len += sprintf(buf + len, "%02X", raw_scan_resp[i]);
            }
            
            len += sprintf(buf + len, "\r\n");
            buf[len++] = '\0';
            ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
            passive_response =1;

            break;
        }
    case WM_BLE_GAP_EVENT_DISC_COMPLETE:
        {
            if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT_DM))
            {
                len = sprintf(buf, "+OK=%hhu,\r\n",0);
                ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
                passive_response =0;
                hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
            }else
            {
                passive_response =1;
            }            
        }
        break;
    default:
        break;
    }

    if (ret)
        tls_mem_free(buf);
    
    if(passive_response == 1) {
        return;
    }
    hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_BT);
    tls_os_sem_release(hif->uart_atcmd_sem); 

    return ;

}

static void ble_uart_server_cb(tls_uart_msg_out_t msg_type,  uint8_t *ptr, int length)
{
#define BLE_UART_OUTPUT_MAX 768
        
        int ret = -1;
        int len = 0;
        u16 i=0;
        char *buf;
        u8 hostif_type;
        struct tls_hostif *hif = tls_get_hostif();       
#if TLS_CONFIG_RMMS
        if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
        {
            hostif_type = HOSTIF_MODE_RMMS_AT;
        }
        else
#endif
        {
            if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
            {
                hostif_type = HOSTIF_MODE_UART0;
            }
            else
            {
                hostif_type = HOSTIF_MODE_UART1_LS;
            }
        }
    
        buf = tls_mem_alloc(BLE_UART_OUTPUT_MAX);
        if (!buf)
        {
            printf("alloc failed\r\n");
            return ;
        }
        switch(msg_type)
        {
            case UART_OUTPUT_DATA:
                len = sprintf(buf, "%s", "+DATA:");
                
                for(i=0; i<length; i++)
                {
                    len += sprintf(buf+len, "%02x", ptr[i]);
                }
                break;
            case UART_OUTPUT_CMD_ADVERTISING:
                len = sprintf(buf, "%s", "+STATE:ADVERTISING");
                break;
            case UART_OUTPUT_CMD_CONNECTED:
                len = sprintf(buf, "%s", "+STATE:CONNECTED");
                break;
            case UART_OUTPUT_CMD_DISCONNECTED:
                len = sprintf(buf, "%s(%d)", "+STATE:DISCONNECTED", ptr[0]);
                break;
        }
        buf[len++] = '\n';
        buf[len++] = 0;       
        ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);        
    
        if (ret)
            tls_mem_free(buf);

    
        return ;

}

#endif
int ble_read_rssi_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;
#if (WM_BLE_INCLUDED == CFG_ON)
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
    
    hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
    hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM); 

    ret = tls_dm_read_remote_rssi(cmd->bleconn.addr);

	if(ret == TLS_BT_STATUS_SUCCESS)
	{
		ret = tls_ble_register_report_evt(WM_BLE_DM_REPORT_RSSI_EVT, ble_report_evt_cb);
	}else
    {
	    hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT_DM);
	    goto end_tag;        
    }  
    bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 5);
    
end_tag:    
#else
    printf("TODO TODO\r\n");
#endif  
    
 	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;   
}

int ble_demo_scan_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    ret = tls_ble_demo_scan(cmd->blescan.cmd);
    
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;    
}
int ble_scan_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();
#if (WM_BLE_INCLUDED == CFG_ON)
    if (cmd->blescan.cmd)
    {
        hif->last_bt_cmd_mode = cmd->blescan.cmd_mode;

        ret = tls_ble_scan(TRUE);

		if(ret == TLS_BT_STATUS_SUCCESS)
		{
			ret = tls_ble_register_report_evt(WM_BLE_DM_SCAN_RES_EVT|WM_BLE_DM_SCAN_RES_CMPL_EVT, ble_report_evt_cb);
		}
    }
    else
    {
    	hif->last_bt_cmd_mode = cmd->blescan.cmd_mode;
    	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
    	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM);        
		
        ret = tls_ble_scan(FALSE);
        
        if (ret != TLS_BT_STATUS_SUCCESS)
        {
		    hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_ACTIVE_BT_DM);
		    goto end_tag;
	    }
	
	    ret = bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 5);
        
		if(ret == TLS_BT_STATUS_SUCCESS)
		{
			ret = tls_ble_deregister_report_evt(WM_BLE_DM_SCAN_RES_EVT|WM_BLE_DM_SCAN_RES_CMPL_EVT, ble_report_evt_cb);
        }
    }
    
end_tag:
#else
    if(cmd->blescan.cmd)
    {
        hif->last_bt_cmd_mode = cmd->blescan.cmd_mode;
        tls_ble_demo_scan_at_cmd_register(ble_gap_scan_at_callback);
        ret = tls_ble_gap_scan(WM_BLE_SCAN_PASSIVE, false);    
    }else
    {
        hif->last_bt_cmd_mode = cmd->blescan.cmd_mode;
    	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
    	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT_DM);
        ret = tls_ble_gap_scan(WM_BLE_SCAN_STOP, false);
        tls_ble_demo_scan_at_cmd_unregister();
    }  
#endif

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
   
}

int ble_demo_server_hid_uart_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_demo_server_hid_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_SUCCESS;

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_demo_server_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    if (cmd->bt.cmd)
    {
#if (WM_NIMBLE_INCLUDED == CFG_ON)    
        ret = tls_ble_server_demo_api_init(NULL,NULL);
#else
        ret = tls_ble_server_demo_api_init(NULL);
#endif
    }
    else
    {
        ret = tls_ble_server_demo_api_deinit();
    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int ble_demo_uart_server_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;    

    if (cmd->bt.cmd)
    {
        ret = tls_ble_server_demo_api_init((void *)ble_uart_server_cb , NULL);
    }
    else
    {
        ret = tls_ble_server_demo_api_deinit();
    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_demo_uart_server_send_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = 0;

    ret = tls_ble_server_demo_api_send_msg(cmd->btparamudata.param, cmd->btparamudata.param_len);

    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}


int ble_demo_client_multi_conn_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    if (cmd->bt.cmd)
    {
        ret = tls_ble_client_multi_conn_demo_api_init();
    }
    else
    {
        ret = tls_ble_client_multi_conn_demo_api_deinit();
    }


    return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_demo_client_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    if (cmd->bt.cmd)
    {
#if (WM_NIMBLE_INCLUDED == CFG_ON)     
        ret = tls_ble_client_demo_api_init(NULL, NULL);
#else
        ret = tls_ble_client_demo_api_init(NULL);
#endif
    }
    else
    {
        ret = tls_ble_client_demo_api_deinit();
    }


	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int ble_scan_chnl_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;

    ret = tls_ble_set_scan_chnl_map(cmd->bt.cmd);
    
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
int ble_uart_mode_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret = TLS_BT_STATUS_FAIL;

    /*enable, type: 1, server mode; 2 client mode;  level: uart index*/
    if(cmd->btctrl.type)
    {
        if(cmd->btctrl.type ==1)
        {
            ret = tls_ble_uart_init(BLE_UART_SERVER_MODE, cmd->btctrl.level, NULL);
        }else if(cmd->btctrl.type ==2)
        { 
           ret = tls_ble_uart_init(BLE_UART_CLIENT_MODE, cmd->btctrl.level, NULL);
        }
    }else
    {
        /*disable function*/
        if(cmd->btctrl.level==1)
        {
            ret = tls_ble_uart_deinit(BLE_UART_SERVER_MODE, 0x01);
        }else if(cmd->btctrl.level==2)
        {
           ret = tls_ble_uart_deinit(BLE_UART_CLIENT_MODE, 0x01); 
        }        
    }

	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

#if (WM_BLE_INCLUDED == CFG_ON )
void ble_gatt_evt_cback(tls_ble_evt_t event, tls_ble_msg_t *msg)
{
#define BLE_EVT_BUF_LEN 1248
    int ret = -1, index = 0;
    int len = 0, tmp_len= 0;
	u16 uuid = 0;
    char *buf;
    u8 hostif_type;
	int passive_response = 0;
	struct tls_hostif *hif = tls_get_hostif();
#if TLS_CONFIG_RMMS
    if (hif->last_bt_cmd_mode == CMD_MODE_RMMS_ATCMD)
    {
        hostif_type = HOSTIF_MODE_RMMS_AT;
    }
    else
#endif
    {
        if (hif->last_bt_cmd_mode == CMD_MODE_UART0_ATCMD)
        {
            hostif_type = HOSTIF_MODE_UART0;
        }
        else
        {
            hostif_type = HOSTIF_MODE_UART1_LS;
        }
    }

    buf = tls_mem_alloc(BLE_EVT_BUF_LEN);
    if (!buf)
    {
        return;
    }

	switch(event)
	{
		case WM_BLE_SE_REGISTER_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu\r\n", msg->ser_register.status,msg->ser_register.server_if);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_SE_CREATE_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu\r\n",msg->ser_create.status, msg->ser_create.server_if, msg->ser_create.service_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_SE_ADD_CHAR_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu,%hhu\r\n",msg->ser_add_char.status, msg->ser_add_char.server_if, msg->ser_add_char.service_id, msg->ser_add_char.attr_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);	
			break;
		case WM_BLE_SE_ADD_CHAR_DESCR_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu,%hhu\r\n",msg->ser_add_char_descr.status,msg->ser_add_char_descr.server_if, msg->ser_add_char_descr.service_id, msg->ser_add_char_descr.attr_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len); 
			break;
		case WM_BLE_SE_START_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu\r\n",msg->ser_start_srvc.status,msg->ser_start_srvc.server_if, msg->ser_start_srvc.service_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;		
		case WM_BLE_SE_STOP_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu\r\n", msg->ser_stop_srvc.status, msg->ser_stop_srvc.server_if, msg->ser_stop_srvc.service_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;	
		case WM_BLE_SE_DELETE_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hhu\r\n", msg->ser_delete_srvc.status, msg->ser_delete_srvc.server_if, msg->ser_delete_srvc.service_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;	
		case WM_BLE_SE_DEREGISTER_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu\r\n", msg->ser_register.status,msg->ser_register.server_if);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}

			break;
		case WM_BLE_SE_CONNECT_EVT:
			len = sprintf(buf, "+OK=0,%hhu,%hu,%hhu,%02X%02X%02X%02X%02X%02X\r\n",
				msg->ser_connect.server_if, msg->ser_connect.conn_id,msg->ser_connect.connected,msg->ser_connect.addr[0],
				msg->ser_connect.addr[1],msg->ser_connect.addr[2],msg->ser_connect.addr[3],
				msg->ser_connect.addr[4],msg->ser_connect.addr[5]);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;
		case WM_BLE_SE_DISCONNECT_EVT:
			len = sprintf(buf, "+OK=0,%hhu,%hhu\r\n",
				msg->ser_connect.server_if,msg->ser_connect.connected);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;
		case WM_BLE_SE_CONFIRM_EVT:
			len = sprintf(buf, "+OK=%hhu\r\n", msg->ser_confirm.status);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			passive_response =1;
			break;
		case WM_BLE_SE_WRITE_EVT:

			len = sprintf(buf, "+OK=0,%hu,%hhu,%hhu,%hhu,", msg->ser_write.conn_id,msg->ser_write.handle, msg->ser_write.trans_id,
								msg->ser_write.len);

			tmp_len = MIN(256, msg->ser_write.len);
			for(index = 0; index<tmp_len; index++)
			{
				len += sprintf(buf+len, "%02x", msg->ser_write.value[index]);
			}
			buf[len++] = '\r';
			buf[len++] = '\n';
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;

        /**client notificaiton*/			
	    case WM_BLE_CL_REGISTER_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hhu\r\n",msg->cli_register.status,msg->cli_register.client_if);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_CL_OPEN_EVT:
			len = sprintf(buf, "+OK=%hhu,%hhu,%hu\r\n", msg->cli_open.status, msg->cli_open.client_if, msg->cli_open.conn_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;
		case WM_BLE_CL_CLOSE_EVT:
			/**Note, if w800 triger the disconnect event, status and reason always 0;
			*  otherwise, status is 0, reason indicates the actual disconnect reason
			*/
			len = sprintf(buf, "+OK=%hhu,%hu,%hhu,%hhu\r\n",0, msg->cli_close.client_if, msg->cli_close.conn_id, msg->cli_close.reason);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;
        case WM_BLE_CL_SEARCH_CMPL_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hu, CMPLT\r\n",msg->cli_search_cmpl.status,msg->cli_search_cmpl.conn_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_CL_REPORT_DB_EVT:
			len = sprintf(buf, "+OK=%hhu,%hu,%hhu\r\n",msg->cli_db.status, msg->cli_db.conn_id, msg->cli_db.count);
			for(index = 0; index<msg->cli_db.count; index++)
			{   
				if(msg->cli_db.db)
				{
					memcpy(&uuid, msg->cli_db.db->uuid.uu+12, 2);
					if(msg->cli_db.db->type == 0)
					{
						len += sprintf(buf+len, "0x%04x,T=0x%02x,HDL=%hhu,PROP=0x%02x \r\n",uuid, msg->cli_db.db->type, msg->cli_db.db->attribute_handle, msg->cli_db.db->properties);
					}else
					{
						len += sprintf(buf+len, "\t0x%04x,T=0x%02x,HDL=%hhu,PROP=0x%02x \r\n",uuid, msg->cli_db.db->type, msg->cli_db.db->attribute_handle, msg->cli_db.db->properties);
					}
					msg->cli_db.db++;
				}
			}
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_CL_REG_NOTIFY_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hu,%hhu\r\n", msg->cli_reg_notify.status, msg->cli_reg_notify.conn_id, msg->cli_reg_notify.handle);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;	
		case WM_BLE_CL_READ_CHAR_EVT:
		case WM_BLE_CL_READ_DESCR_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hu,%hhu,", msg->cli_read.status, msg->cli_read.conn_id, msg->cli_read.len);
			tmp_len = MIN(256, msg->cli_read.len);
			for(index = 0; index<tmp_len; index++)
			{
				len += sprintf(buf+len, "%02x",msg->cli_read.value[index] );
			}
			buf[len++] = '\r';
			buf[len++] = '\n';
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;	
		case WM_BLE_CL_WRITE_CHAR_EVT:
		case WM_BLE_CL_WRITE_DESCR_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hu,%hhu\r\n",msg->cli_write.status,msg->cli_write.conn_id, msg->cli_write.handle);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_CL_DEREG_NOTIFY_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hhu,%hu\r\n",msg->cli_reg_notify.status, msg->cli_reg_notify.handle, msg->cli_reg_notify.conn_id);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			break;
		case WM_BLE_CL_DEREGISTER_EVT:
	        len = sprintf(buf, "+OK=%hhu,%hhu\r\n",msg->cli_register.status,msg->cli_register.client_if);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}
			break;
		case WM_BLE_CL_CFG_MTU_EVT:
			len = sprintf(buf, "+OK=%hhu,%hu,%hu\r\n",msg->cli_cfg_mtu.status,msg->cli_cfg_mtu.conn_id, msg->cli_cfg_mtu.mtu);
			ret = tls_hostif_process_cmdrsp(hostif_type, buf, len);
			if(hif->uart_atcmd_bits & (1<<UART_ATCMD_BIT_ACTIVE_BT))
			{
				passive_response =0;
				hif->uart_atcmd_bits &= ~(1<<UART_ATCMD_BIT_ACTIVE_BT);
			}else
			{
				passive_response =1;
			}

			break;
		default:
			break;
	}
   
    if (ret)
        tls_mem_free(buf);
	
	if(passive_response == 1) {
		return;
	}
	hif->uart_atcmd_bits |= (1 << UART_ATCMD_BIT_BT);
    tls_os_sem_release(hif->uart_atcmd_sem); 
}

int ble_create_server_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blesv.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

	ret = tls_ble_demo_prof_init(cmd->blesv.uuid, ble_gatt_evt_cback);
    if (ret != TLS_BT_STATUS_SUCCESS)
    {
        goto end_tag;
    }

	ret = bt_wait_rsp_timeout(cmd->blesv.cmd_mode, cmd, hif, 1);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_add_service_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();
	
 	if(cmd->blesc.uuid == 0x1824)
 	{
 		ret = TLS_BT_STATUS_UNSUPPORTED;
		printf("0x1824 service is not available\r\n");
		goto end_tag;
 	}
    hif->last_bt_cmd_mode = cmd->blesc.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_add_service(cmd->blesc.server_if, cmd->blesc.inst_id, 1, app_uuid16_to_uuid128(cmd->blesc.uuid), cmd->blesc.handles);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blesc.cmd_mode, cmd, hif, 1);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;

}

int ble_add_characteristic_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blech.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_add_characteristic(cmd->blech.server_if, cmd->blech.service_handle, app_uuid16_to_uuid128(cmd->blech.uuid), cmd->blech.prop, cmd->blech.perm);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blech.cmd_mode, cmd, hif, 1);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_add_descript_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blech.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_add_descriptor(cmd->blech.server_if, cmd->blech.service_handle, app_uuid16_to_uuid128(cmd->blech.uuid), cmd->blech.perm);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blech.cmd_mode, cmd, hif, 1);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_start_service_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blestt.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_start_service(cmd->blestt.server_if, cmd->blestt.service_handle, cmd->blestt.tran_type);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blestt.cmd_mode, cmd, hif, 2);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_stop_service_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blestt.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_stop_service(cmd->blestt.server_if, cmd->blestt.service_handle);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);

	ret = bt_wait_rsp_timeout(cmd->blestt.cmd_mode, cmd, hif, 3);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_del_service_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blestt.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_delete_service(cmd->blestt.server_if, cmd->blestt.service_handle);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);

	ret = bt_wait_rsp_timeout(cmd->blestt.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_destory_server_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_demo_prof_deinit(cmd->bt.cmd);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);

	ret = bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_server_connect_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
	tls_bt_addr_t addr;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

	memcpy(addr.address, cmd->bleconn.addr, 6);asm("nop");asm("nop");asm("nop");
    ret = tls_ble_server_connect(cmd->bleconn.server_if, (const tls_bt_addr_t *)&addr, 1, 2);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	/**Indicate the command is trigged by AT, 
	*  why set this flag;
	*  because the phone will connect to me also.
	*/
    hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);
	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_server_disconnect_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
	tls_bt_addr_t addr;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

	memcpy(addr.address, cmd->bleconn.addr, 6);asm("nop");asm("nop");asm("nop");

    ret = tls_ble_server_disconnect(cmd->bleconn.server_if, (const tls_bt_addr_t *)&addr, cmd->bleconn.conn_id);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);
	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 8);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_server_send_indication_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blesndind.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_send_indication(cmd->blesndind.server_if, cmd->blesndind.attr_handle, cmd->blesndind.conn_id, strlen((const char *)cmd->blesndind.value), 1, (char *)cmd->blesndind.value);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blesndind.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_server_send_resp_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blesndrsp.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_server_send_response(cmd->blesndrsp.conn_id, cmd->blesndrsp.trans_id, 0, 0, cmd->blesndrsp.attr_handle, cmd->blesndrsp.auth_req, cmd->blesndrsp.value, strlen((const char *)cmd->blesndrsp.value));
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blesndrsp.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_create_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blesv.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);
	
	ret = tls_ble_demo_cli_init(cmd->blesv.uuid, ble_gatt_evt_cback);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blesv.cmd_mode, cmd, hif, 1);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_connect_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
	tls_bt_addr_t addr;
    struct tls_hostif *hif = tls_get_hostif();
    printf("The connection cmd will last 30s, if connect failed\r\n");
    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    memcpy(&addr.address, cmd->bleconn.addr, 6);

    ret = tls_ble_client_connect(cmd->bleconn.server_if, &addr, 1,2);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);
	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 31);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_scan_service_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

	ret = tls_ble_client_search_service(cmd->bleconn.conn_id, NULL);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_get_gatt_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_client_get_gatt_db(cmd->bleconn.conn_id);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_reg_notify_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blenty.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_client_register_for_notification(cmd->blenty.client_if, (const tls_bt_addr_t *)cmd->blenty.addr, cmd->blenty.attr_handle, cmd->blenty.conn_id);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blenty.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_dereg_notify_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->blenty.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_client_deregister_for_notification(cmd->blenty.client_if, (const tls_bt_addr_t *)cmd->blenty.addr, cmd->blenty.attr_handle, cmd->blenty.conn_id);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->blenty.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_access_characteristic_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleacc.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

	/*character write operation*/
	if (cmd->bleacc.mode == 0)
	{
		ret = tls_ble_client_write_characteristic(cmd->bleacc.conn_id, cmd->bleacc.handle, 2, cmd->bleacc.data_len, cmd->bleacc.auth_req, (char *)cmd->bleacc.data);
	}
	else
	{
		ret = tls_ble_client_read_characteristic(cmd->bleacc.conn_id, cmd->bleacc.handle,cmd->bleacc.auth_req);
	}
	
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}

	ret = bt_wait_rsp_timeout(cmd->bleacc.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_disconnect_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bleconn.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_client_disconnect(cmd->bleconn.server_if, (const tls_bt_addr_t *)cmd->bleconn.addr, cmd->bleconn.conn_id);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
    hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);
	ret = bt_wait_rsp_timeout(cmd->bleconn.cmd_mode, cmd, hif, 11);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_destory_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();

    hif->last_bt_cmd_mode = cmd->bt.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_demo_cli_deinit(cmd->bt.cmd);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);

	ret = bt_wait_rsp_timeout(cmd->bt.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}

int ble_client_cfg_mtu_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
    tls_bt_status_t ret;
    struct tls_hostif *hif = tls_get_hostif();
    hif->last_bt_cmd_mode = cmd->blecmtu.cmd_mode;
	hif->uart_atcmd_bits &= ~(1 << UART_ATCMD_BIT_BT);

    ret = tls_ble_client_configure_mtu(cmd->blecmtu.conn_id, cmd->blecmtu.mtu);
	if (ret != TLS_BT_STATUS_SUCCESS)
	{
		goto end_tag;
	}
	hif->uart_atcmd_bits |= (1<<UART_ATCMD_BIT_ACTIVE_BT);
	ret = bt_wait_rsp_timeout(cmd->blecmtu.cmd_mode, cmd, hif, 5);

end_tag:
	return (ret == TLS_BT_STATUS_SUCCESS) ? 0 : -CMD_ERR_OPS;
}
#endif



#endif

int cpu_clock_proc(u8 set_opt, u8 update_flash, union HOSTIF_CMD_PARAMS_UNION *cmd, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp)
{
	u8 div = (u8)cmd->width.freq;
	if (set_opt)
	{
		tls_sys_clk_set(div);
		tls_uart_bps_set(0, 115200);
	}
	else
	{
		clk_div_reg clk_div;
		clk_div.w = tls_reg_read32(HR_CLK_DIV_CTL);
		cmdrsp->pass.length = clk_div.b.CPU;
		div = clk_div.b.CPU;
	}
	printf("cpu clk is %d\n", 480000000/div);
	return 0;
}

static struct tls_cmd_t  at_ri_cmd_tbl[] = {
#if 1
    { "Z", HOSTIF_CMD_RESET, 0x11, 0, 0,z_proc},
    { "E", HOSTIF_CMD_NOP, 0x1, 0, 0,e_proc},
    { "ENTS", HOSTIF_CMD_PS, 0x22, 4, 4,ents_proc},
    { "RSTF", HOSTIF_CMD_RESET_FLASH, 0x11, 0, 0,rstf_proc},
    { "PMTF", HOSTIF_CMD_PMTF, 0x11, 0, 0,pmtf_proc},
    { "IOC", HOSTIF_CMD_GPIO, 0x11, 0, 0, ioc_proc},
    { "WJOIN", HOSTIF_CMD_WJOIN, 0x11, 0, 0,wjoin_proc},
    { "WLEAV", HOSTIF_CMD_WLEAVE, 0x33, 1, 0,wleav_proc},
    { "WSCAN", HOSTIF_CMD_WSCAN, 0x13, 0, 0,wscan_proc},
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    { "LKSTT", HOSTIF_CMD_LINK_STATUS, 0x19, 0, 0,lkstt_proc},
    { "ENTM", HOSTIF_CMD_NOP, 0x1, 0, 0, entm_proc},
    { "SKCT", HOSTIF_CMD_SKCT, 0x22, 4, 6, skct_proc},
    { "SKSTT", HOSTIF_CMD_SKSTT, 0x22, 1, 1, skstt_proc},
    { "SKCLS", HOSTIF_CMD_SKCLOSE, 0x22, 1, 1, skcls_proc},
    { "SKSDF", HOSTIF_CMD_SKSDF, 0x22, 1, 1, sksdf_proc},
    { "SKSND", HOSTIF_CMD_NOP, 0x02, 2, 0, sksnd_proc},
    { "SKRCV", HOSTIF_CMD_NOP, 0x02, 2, 0, skrcv_proc},
    { "SKRPTM", HOSTIF_CMD_NOP, 0xA, 1, 0, skrptm_proc},
    { "SKSRCIP", HOSTIF_CMD_SKSRCIP, 0x18, 0, 0, sksrcip_proc},
    { "SKGHBN", HOSTIF_CMD_SKGHBN, 0x22, 1, 1, skghbn_proc},
#endif
    { "WPRT", HOSTIF_CMD_WPRT, 0x7F, 1, 1,wprt_proc},
    { "SSID", HOSTIF_CMD_SSID, 0x7F, 1, 1,ssid_proc},
    { "KEY", HOSTIF_CMD_KEY, 0x7F, 3, 3,key_proc},
    { "ENCRY", HOSTIF_CMD_ENCRYPT, 0x7F, 1, 1,encry_proc},
    { "BSSID", HOSTIF_CMD_BSSID, 0x7F, 1, 1,bssid_proc},
    { "BRDSSID", HOSTIF_CMD_BRD_SSID, 0x7F, 1, 1,brdssid_proc},
    { "CNTPARAM", HOSTIF_CMD_CNTPARAM, 0x19, 0, 0,cntparam_proc},
    { "CHL", HOSTIF_CMD_CHNL, 0x7F, 1, 2,chl_proc},
    { "CHLL", HOSTIF_CMD_CHLL, 0x7F, 1, 2,chll_proc},
    { "WREG", HOSTIF_CMD_WREG, 0x7F, 1, 2,wreg_proc},
    { "WBGR", HOSTIF_CMD_WBGR, 0x7F, 2, 2, wbgr_proc},
    { "WATC", HOSTIF_CMD_WATC, 0x7F, 1, 1, watc_proc},
    { "WPSM", HOSTIF_CMD_WPSM, 0x7F, 1, 1, wpsm_proc},
    { "WARC", HOSTIF_CMD_WARC, 0x7F, 1, 1, warc_proc},
    { "WARM", HOSTIF_CMD_WARM, 0x7F, 1, 1, warm_proc},
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    { "NIP", HOSTIF_CMD_NIP, 0x7F, 1, 17, nip_proc},
#endif
    { "ATM", HOSTIF_CMD_ATM, 0x7F, 1, 1, atm_proc},
    { "ATRM", HOSTIF_CMD_ATRM, 0x7F, 4, 6, atrm_proc},
    { "AOLM", HOSTIF_CMD_AOLM, 0x7F, 0, 0, aolm_proc},
    { "PORTM", HOSTIF_CMD_PORTM, 0x7F, 1, 1, portm_proc},
    { "UART", HOSTIF_CMD_UART, 0x7F, 4, 7, uart_proc},
    { "ATLT", HOSTIF_CMD_ATLT, 0x7F, 1, 2, atlt_proc},
    { "DNS", HOSTIF_CMD_DNS, 0x7F, 1, 2, dns_proc},
    { "DDNS", HOSTIF_CMD_DDNS, 0x7F, 0, 0, ddns_proc},
    { "UPNP", HOSTIF_CMD_UPNP, 0x7F, 0, 0, upnp_proc},
    { "DNAME", HOSTIF_CMD_DNAME, 0x7F, 0, 0, dname_proc},
    { "ATPT", HOSTIF_CMD_ATPT, 0x7F, 1, 2, atpt_proc},
    { "&DBG", HOSTIF_CMD_DBG, 0x22, 1, 4, dbg_proc},
    { "ESPC", HOSTIF_CMD_NOP, 0xF, 1, 0, espc_proc},
    { "ESPT", HOSTIF_CMD_NOP, 0xF, 1, 0, espt_proc},
    { "WEBS", HOSTIF_CMD_WEBS, 0x7F, 1, 1, webs_proc},
    { "IOM", HOSTIF_CMD_IOM, 0x7F, 1, 1, iom_proc},
    { "CMDM", HOSTIF_CMD_CMDM, 0x7F, 1, 1, cmdm_proc},
    { "PASS", HOSTIF_CMD_PASS, 0x7F, 1, 7, pass_proc},
    { "ONESHOT", HOSTIF_CMD_ONESHOT, 0x7F, 1, 1, oneshot_proc},
    { "&UPDP", HOSTIF_CMD_UPDP, 0x22, 1, 1, updp_proc},
#if TLS_CONFIG_HTTP_CLIENT_TASK
    { "HTTPC", HOSTIF_CMD_HTTPC, 0x22, 2, 3, httpc_proc},
    { "FWUP", HOSTIF_CMD_FWUP, 0x22, 1, 0, fwup_proc},
#endif
    { "TEM", HOSTIF_CMD_TEM, 0x7F, 1, 1, tem_proc},
#endif
    { "QMAC", HOSTIF_CMD_MAC, 0x19, 0, 0, qmac_proc},
    { "QVER", HOSTIF_CMD_VER, 0x19, 0, 0, qver_proc},
    { "&UPDM", HOSTIF_CMD_UPDM, 0x22, 1, 1, updm_proc},
	{ "&UPDD", HOSTIF_CMD_UPDD, 0x22, 1, 2, updd_proc},
	{ "&REGR", HOSTIF_CMD_REGR, 0x22, 2, 5, regr_proc},
	{ "&REGW", HOSTIF_CMD_REGW, 0x22, 2, 5, regw_proc},
	{ "&RFR", HOSTIF_CMD_RFR, 0x22, 2, 3, rfr_proc},
	{ "&RFW", HOSTIF_CMD_RFW, 0x22, 2, 3, rfw_proc},
	{ "&FLSR", HOSTIF_CMD_FLSR, 0x22, 2, 5, flsr_proc},
	{ "&FLSW", HOSTIF_CMD_FLSW, 0x22, 2, 5, flsw_proc},
    { "&TXG", HOSTIF_CMD_NOP, 0xF, 1, 0, txg_proc},
	{ "&TXGI", HOSTIF_CMD_NOP, 0xF, 1, 0, txgi_proc},
    { "&TXGS", HOSTIF_CMD_NOP, 0xF, 1, 0, txg_rate_set_proc},
    { "&TXGG", HOSTIF_CMD_NOP, 0xF, 1, 0, txg_rate_get_proc},
	{ "&MAC", HOSTIF_CMD_NOP, 0xF, 1, 0, mac_proc},
	{ "&HWV", HOSTIF_CMD_NOP, 0x9, 0, 0, hwv_proc},
	{ "&SPIF", HOSTIF_CMD_NOP, 0x2, 1, 0, spif_proc},
    { "&LPCHL", HOSTIF_CMD_NOP, 0xB, 1, 0, lpchl_proc},
    { "&LPTSTR", HOSTIF_CMD_NOP, 0x2, 5, 0, lptstr_proc},
    { "&LPTSTP", HOSTIF_CMD_NOP, 0x1, 0, 0, lptstp_proc},
    { "&LPTSTT", HOSTIF_CMD_NOP, 0xF, 0, 0, lptstt_proc},
    { "&LPRSTR", HOSTIF_CMD_NOP, 0x2, 1, 0, lprstr_proc},
    { "&LPRSTP", HOSTIF_CMD_NOP, 0x1, 0, 0, lprstp_proc},
    { "&LPRSTT", HOSTIF_CMD_NOP, 0x1, 0, 0, lprstt_proc},
    { "&LPPSTR", HOSTIF_CMD_NOP, 0x3, 2, 0, lppstr_proc},
    { "&LPPSTP", HOSTIF_CMD_NOP, 0x2, 1, 0, lppstp_proc},
    { "&LPRFPS", HOSTIF_CMD_NOP, 0x1, 0, 0, lprfps_proc},
    { "&LPCHRS", HOSTIF_CMD_NOP, 0x2, 1, 0, lpchrs_proc},
    { "&LPTBD", HOSTIF_CMD_NOP, 0x2, 7, 0, lptbd_proc},
    { "&LPSTPT", HOSTIF_CMD_NOP, 0x1, 0, 0, lpstpt_proc},
    { "&LPCHLR", HOSTIF_CMD_NOP, 0x2, 1, 0, lpchlr_proc},
    { "&LPSTPR", HOSTIF_CMD_NOP, 0x1, 0, 0, lpstpr_proc},
    { "&LPRAGC", HOSTIF_CMD_NOP, 0x1, 0, 0, lpragc_proc},
    { "&LPRSR", HOSTIF_CMD_NOP, 0x9, 0, 0, lprsr_proc},
#if TLS_CONFIG_AP
    { "SLIST", HOSTIF_CMD_STA_LIST, 0x19, 0, 0, slist_proc},
    { "APLKSTT", HOSTIF_CMD_AP_LINK_STATUS, 0x19, 0, 0,softap_lkstt_proc},
    { "APSSID", HOSTIF_CMD_AP_SSID, 0x7F, 1, 1, softap_ssid_proc},
    { "APMAC", HOSTIF_CMD_AP_MAC, 0x19, 0, 0, softap_qmac_proc},
    { "APENCRY", HOSTIF_CMD_AP_ENCRYPT, 0x7F, 1, 1,softap_encry_proc },
    { "APKEY", HOSTIF_CMD_AP_KEY, 0x7F, 3, 3, softap_key_proc },
    { "APCHL", HOSTIF_CMD_AP_CHL, 0x7F, 1, 2,softap_chl_proc},
    { "APWBGR", HOSTIF_CMD_AP_WBGR, 0x7F, 2, 2, softap_wbgr_proc},
    { "APNIP", HOSTIF_CMD_AP_NIP, 0x7F, 1, 17, softap_nip_proc },            
#endif
#if TLS_CONFIG_WIFI_PERF_TEST
	{ "THT", HOSTIF_CMD_NOP, 0x2, 0, 0, tht_proc},
#endif
#if TLS_CONFIG_WIFI_PING_TEST
	{ "PING", HOSTIF_CMD_NOP, 0x2, 0, 0, ping_proc},
#endif
#if TLS_CONFIG_WPS    
    { "WWPS", HOSTIF_CMD_WPS, 0x7F, 1, 1, wwps_proc},
#endif
	{ "CUSTDATA", HOSTIF_CMD_CUSTDATA, 0x19, 0, 0, custdata_proc},

	{ "WIDTH", HOSTIF_CMD_NOP, 0x2, 2, 0, tls_tx_sin},
	{ "&RXSIN", HOSTIF_CMD_NOP, 0x2, 2, 0, tls_rx_wave},

	{ "TXLO", HOSTIF_CMD_NOP, 0x7F, 1,  0,  tls_tx_lo_proc},
	{ "TXIQ", HOSTIF_CMD_NOP, 0x7F, 2,  0,  tls_tx_iq_mismatch_proc},
	{ "FREQ", HOSTIF_CMD_NOP, 0x7F, 1,  0,  tls_freq_error_proc},
	{ "&CALFIN",    HOSTIF_CMD_NOP, 0x7F, 0, 0 , tls_rf_cal_finish_proc},
	{ "&LPTPD", HOSTIF_CMD_NOP, 0x7F, 1, 0, tls_lptperiod_proc},
	
	{ "STDBY", HOSTIF_CMD_NOP, 0x1, 0, 0,stand_by_power_down},
	{ "CPUSTA", HOSTIF_CMD_NOP, 0x2, 1, 0,cpu_state_proc},
	{ "CPUDIV", HOSTIF_CMD_NOP, 0xb, 1, 0,cpu_clock_proc},
#if (WM_BT_INCLUDED == CFG_ON || WM_BLE_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON)

	{ "BTEN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, bt_enable_proc},
	{ "BTDES", HOSTIF_CMD_NOP, ATCMD_OP_NULL, 0, 0, bt_destory_proc},
	{ "&BTMAC", HOSTIF_CMD_NOP, 0xF, 1, 0, bt_mac_proc},
	{ "&BTNAME", HOSTIF_CMD_NOP, 0xF, 1, 0, bt_name_proc},
	{ "BTCTRLGS", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_QU | RICMD_OP_GET, 0, 0, bt_ctrl_get_status_proc},
	{ "BTSLEEP", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_sleep_proc},
	{ "BLETPS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_tx_power_set_proc},
	{ "BLETPG", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_QU | RICMD_OP_GET, 1, 0, ble_tx_power_get_proc},
	{ "BTTXPOW", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_GET | RICMD_OP_SET, 2, 0, bt_tx_power_proc},
	{ "BTTEST", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_test_mode_proc},
    { "BTRF", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_rf_mode_proc},

#if (WM_BTA_AV_SINK_INCLUDED == CFG_ON)
    { "BTAVS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_audio_sink_proc},
#endif  
#if (WM_BTA_HFP_HSP_INCLUDED == CFG_ON)
    { "BTHFP", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_hfp_client_proc},
    { "BTDIAL",HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_dial_up_proc},
#endif
#if (WM_BTA_SPPS_INCLUDED == CFG_ON)
    { "BTSPPS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_spp_server_proc},
#endif 
#if (WM_BTA_SPPC_INCLUDED == CFG_ON)
    { "BTSPPC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_spp_client_proc},
#endif 

#if (WM_BT_INCLUDED == CFG_ON)
    { "BTSCM", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_scan_mode_proc},
    { "BTINQUIRY", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, bt_inquiry_proc},
#endif 

#if (WM_BLE_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON)    
	{ "BLEADV", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_adv_proc},
	{ "BLEADATA", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_adv_data_proc},
	{ "BLESCRSP", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_scan_rsp_proc},
	{ "BLEAPRM", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET | RICMD_OP_SET, 5, 0, ble_adv_param_proc},
	{ "BLESCPRM", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_scan_param_proc},
	{ "BLESFLT", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_scan_filter_proc},
	{ "BLESCAN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_scan_proc},
	{ "BLERDRSSI", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_read_rssi_proc},
	{ "BLEDS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_server_proc},
	{ "BLEDC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_client_proc},
	{ "BLEDCMC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_client_multi_conn_proc},
	{ "BLESSCM", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_scan_chnl_proc},
	{ "BLEUM", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_uart_mode_proc},
	{ "BLEDMADV", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_adv_proc},
	{ "BLEDMSCAN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_scan_proc},
	{ "BLEHIDS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_server_hid_proc},
	{ "BLEHIDU", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_server_hid_uart_proc},
	{ "BLEUDS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_uart_server_proc},
	{ "BLEUSND", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_demo_uart_server_send_proc},
#if (WM_BLE_INCLUDED == CFG_ON)
	{ "BLECTSV", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_create_server_proc},
	{ "BLEADDSC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_add_service_proc},
	{ "BLEADDCH", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 5, 0, ble_add_characteristic_proc},
	{ "BLEADESC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_add_descript_proc},
	{ "BLESTTSC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_start_service_proc},
	{ "BLESTPSC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_stop_service_proc},
	{ "BLEDELSC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_del_service_proc},
	{ "BLEDESSV", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_destory_server_proc},
	{ "BLESCONN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_server_connect_proc},
	{ "BLESVDIS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_server_disconnect_proc},
	{ "BLESIND", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_server_send_indication_proc},
	{ "BLESRSP", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 5, 0, ble_server_send_resp_proc},

	{ "BLECCT", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_client_create_proc},
	{ "BLECCONN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_client_connect_proc},
	{ "BLECSSC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_client_scan_service_proc},
	{ "BLECGDB", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_client_get_gatt_proc},
	{ "BLECRNTY", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_client_reg_notify_proc},
	{ "BLECDNTY", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_client_dereg_notify_proc},
	{ "BLECACH", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_client_access_characteristic_proc},
	{ "BLECDIS", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_client_disconnect_proc},
	{ "BLECDES", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_client_destory_proc},
	{ "BLECMTU", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_client_cfg_mtu_proc},
#endif

#if (WM_NIMBLE_INCLUDED== CFG_ON)
#if (WM_MESH_INCLUDED == CFG_ON)
    { "MSNODEEN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_node_enable_proc},        /*v*/
    { "MSPROVEN", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_provisioner_enable_proc}, /*v*/
    { "MSSCANPD", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_scan_unprov_node_proc},   /*v*/
    { "MSSCANND", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_scan_prov_node_proc},     /*n*/
    { "MSGETCPD", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_mesh_get_comp_data_proc},      /*v*/
    { "MSPROVAD", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_mesh_prov_node_proc},          /*v*/
    { "MSADDLAKEY", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_mesh_add_local_app_key_proc},/*v*/ 
    { "MSADDRAKEY", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 5, 0, ble_mesh_add_app_key_proc},      /*v*/
    { "MSBNDAKEY", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET , 5, 0, ble_mesh_bind_app_key_proc},      /*v*/
    { "MSUBNDAKEY", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 5, 0, ble_mesh_unbind_app_key_proc},    /*v*/
    { "MSRELAYSET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 5, 0, ble_mesh_set_relay_proc},    /*v*/
    { "MSRELAYGET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_mesh_get_relay_proc},    /*v*/
    
    { "MSPROXYSET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_mesh_set_proxy_proc},    /*v*/
    { "MSPROXYGET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_mesh_get_proxy_proc},    /*v*/

    { "MSFRDSET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_mesh_set_friend_proc},     /*v*/
    { "MSFRDGET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_mesh_get_friend_proc},     /*v*/

    { "MSSUBGET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 4, 0, ble_mesh_get_sub_proc},
    { "MSSUBADD", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 5, 0, ble_mesh_add_sub_proc},
    { "MSSUBDEL", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 5, 0, ble_mesh_del_sub_proc},

    { "MSPUBGET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 4, 0, ble_mesh_get_pub_proc},
    { "MSPUBSET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 11, 0, ble_mesh_set_pub_proc},

    { "MSHBPUBGET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 2, 0, ble_mesh_get_hb_pub_proc},
    { "MSHBPUBSET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 8, 0, ble_mesh_set_hb_pub_proc},

    { "MSHBSUBGET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 2, 0, ble_mesh_get_hb_sub_proc},
    { "MSHBSUBSET", HOSTIF_CMD_NOP, ATCMD_OP_NULL | ATCMD_OP_EQ | ATCMD_OP_EP | ATCMD_OP_QU | RICMD_OP_SET, 8, 0, ble_mesh_set_hb_sub_proc},

    { "MSONOFFSET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_mesh_gen_set_onoff_proc}, /*V*/
    { "MSONOFFGET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_mesh_gen_get_onoff_proc}, /*v*/
    { "MSONOFFPUB", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_gen_pub_proc},  /*v*/

    { "MSLVLSET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_mesh_gen_set_level_proc},
    { "MSLVLGET", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 3, 0, ble_mesh_gen_get_level_proc},

    
    { "MSCLRRPL", HOSTIF_CMD_NOP, ATCMD_OP_NULL, 0, 0, ble_mesh_clear_rpl_proc},
    { "MSAC", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_config_node_proc},            /*u*/
    { "MSRST", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 2, 0, ble_mesh_reset_node_proc},            /*v*/   
    { "MSOOBNUM", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_oob_number_proc}, 
    { "MSOOBSTR", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_oob_string_proc},
    { "MSVNDSND", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_mesh_vnd_send_proc},    

    /**node demo */
    { "MSNODEDM",  HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_node_demo_proc},/*v*/
    { "MSUART",    HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 4, 0, ble_mesh_uart_proc},
    { "MSWRADDR", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_change_primary_addr_proc}, 
    { "MSRDADDR", HOSTIF_CMD_NOP, ATCMD_OP_NULL, 0, 0, ble_mesh_read_primary_addr_proc},
    { "MSRDCFG", HOSTIF_CMD_NOP, ATCMD_OP_NULL, 0, 0, ble_mesh_read_cfg_proc},
    { "MSERASE", HOSTIF_CMD_NOP, ATCMD_OP_NULL, 0, 0, ble_mesh_erase_cfg_proc},
    { "MSWRTTL", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_change_default_ttl_proc}, 
    //{ "MSLVLMV", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_client_gen_level_move_proc},
    //{ "MSLVLDLTA", HOSTIF_CMD_NOP, ATCMD_OP_EQ | ATCMD_OP_EP | RICMD_OP_SET, 1, 0, ble_mesh_client_gen_level_delta_proc},

#endif
#endif

#endif	
#endif

	{ NULL, HOSTIF_CMD_NOP, 0, 0 , 0, NULL},
};

int at_parse_func(char *at_name, struct tls_atcmd_token_t *tok, union HOSTIF_CMD_PARAMS_UNION *cmd){
    if(strcmp("QMAC", at_name) == 0 || strcmp("QVER", at_name) == 0 || strcmp("&HWV", at_name) == 0 ||
       strcmp("&LPTSTP", at_name) == 0 || strcmp("&LPTSTT", at_name) == 0 || strcmp("&LPRSTP", at_name) == 0 ||
       strcmp("&LPRSTT", at_name) == 0 || strcmp("&LPRFPS", at_name) == 0 || strcmp("&LPSTPT", at_name) == 0 ||
       strcmp("&LPSTPR", at_name) == 0 || strcmp("&LPRAGC", at_name) == 0 || strcmp("&LPRSR", at_name) == 0 ||
       strcmp("CUSTDATA", at_name) == 0 || strcmp("STDBY", at_name) == 0
#if TLS_CONFIG_AP
       || strcmp("APMAC", at_name) == 0 ||
          strcmp("APLKSTT", at_name) == 0 ||
          strcmp("SLIST", at_name) == 0
#endif
       ){
        ;
    }
#if TLS_CONFIG_AT_CMD
    else if ((strcmp("Z", at_name) == 0) || (strcmp("E", at_name) == 0) || (strcmp("RSTF", at_name) == 0) ||
        (strcmp("PMTF", at_name) == 0) || (strcmp("IOC", at_name) == 0)  ||
        (strcmp("LKSTT", at_name) == 0) || (strcmp("CNTPARAM", at_name) == 0)
#if (WM_BLE_INCLUDED == CFG_ON) || (WM_BT_INCLUDED == CFG_ON)
        || (strcmp("BTCTRLGS", at_name) == 0)
#endif
        ){
        ;
    }else if (strcmp("ENTS", at_name) == 0) {
        int err = 0, ret = 0;
        u32 params;
        if(tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        do {
            ret = string_to_uint(tok->arg[0], &params);
            if (ret){
                err = 1;
                break;
            }
            cmd->ps.ps_type = (u8)params;

            ret = string_to_uint(tok->arg[1], &params);
            if (ret){
                err = 1;
                break;
            }
            cmd->ps.wake_type = (u8)params;

            ret = string_to_uint(tok->arg[2], &params);
            if (ret){
                err = 1;
                break;
            }
            cmd->ps.delay_time = (u16)params;

            ret = string_to_uint(tok->arg[3], &params);
            if (ret){
                err = 1;
                break;
            }
            cmd->ps.wake_time = (u16)params;
        }while(0);
        if (err)
            return -CMD_ERR_INV_PARAMS;
    }
    else if (strcmp("WJOIN", at_name) == 0) {
		cmd->wscan.mode = tok->cmd_mode;
    }
	else if (strcmp("WSCAN", at_name) == 0)
	{
        int ret;
		u32 value = 0;
		cmd->scanparam.mode = tok->cmd_mode;
		cmd->scanparam.switchinterval = 0;
		cmd->scanparam.scantimes = 0;
		cmd->scanparam.chlist = 0;
		cmd->scanparam.scantype = 0;

		switch(tok->arg_found)
		{
			case 4:
				ret = string_to_uint(tok->arg[3], &value);
		        if(ret)
		            return -CMD_ERR_INV_PARAMS;
				cmd->scanparam.scantype = value;

			case 3:
				ret = string_to_uint(tok->arg[2], &value);
		        if(ret)
		            return -CMD_ERR_INV_PARAMS;
				cmd->scanparam.switchinterval = value;
			case 2:
				ret = string_to_uint(tok->arg[1], &value);
		        if(ret)
		            return -CMD_ERR_INV_PARAMS;
				cmd->scanparam.scantimes = value;
			case 1:
				ret = hexstr_to_unit(tok->arg[0], &value);
		        if(ret)
		            return -CMD_ERR_INV_PARAMS;
				cmd->scanparam.chlist = value;
			break;
			default:
				cmd->scanparam.switchinterval = 0;
				cmd->scanparam.scantimes = 0;
				cmd->scanparam.chlist = 0;
				cmd->scanparam.scantype = 0;
				break;
		}		
    }
	else if (strcmp("SSID", at_name) == 0 || strcmp("DNS", at_name) == 0  || strcmp("PASS", at_name) == 0
#if TLS_CONFIG_AP
              || strcmp("APSSID", at_name) == 0
#endif
             ){
        int ret=0;
        u8 *tmpssid;
        if(tok->arg_found>1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found==1){
            ret = atcmd_filter_quotation(&tmpssid, (u8 *)tok->arg[0]);
    		if (ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->ssid.ssid_len = strlen((char *)tmpssid);
            memcpy(cmd->ssid.ssid, tmpssid, cmd->ssid.ssid_len);
        }
    }
    else if( strcmp("TEM", at_name) == 0 )
    {
        int ret=0;
        u8 *tmpssid;
        if(tok->arg_found>1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found==1) {
            ret = atcmd_filter_quotation(&tmpssid, (u8 *)tok->arg[0]);
    		if (ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->tem.offsetLen = strlen((char *)tmpssid);
            memcpy(cmd->tem.offset, tmpssid, cmd->tem.offsetLen);
        }
    }
    else if((strcmp("WPRT", at_name) == 0) || (strcmp("ENCRY", at_name) == 0) || (strcmp("BRDSSID", at_name) == 0) ||
             (strcmp("WATC", at_name) == 0) || (strcmp("WPSM", at_name) == 0) || (strcmp("WARC", at_name) == 0) ||
             (strcmp("WARM", at_name) == 0) || (strcmp("ATM", at_name) == 0) || (strcmp("PORTM", at_name) == 0) ||
             (strcmp("IOM", at_name) == 0) || (strcmp("CMDM", at_name) == 0) || (strcmp("ONESHOT", at_name) == 0) ||
             (strcmp("&UPDP", at_name) == 0) 
#if TLS_CONFIG_AP
	||(strcmp("APENCRY", at_name) == 0)
#endif
             ){
        int ret = 0; 
        u32 param;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 1){
            ret = string_to_uint(tok->arg[0], &param);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->wprt.type = (u8)param;
        }
    }else if((strcmp("KEY", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APKEY", at_name) == 0)
#endif
    ){
        int ret;
        u32 params;
        u8 *keyInfo;
        
        if(tok->arg_found != 0 && tok->arg_found != 3)
            return  -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 3){
            ret= strtodec((int *)&params, tok->arg[0]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->key.format = (u8)params;
            ret = strtodec((int *)&params, tok->arg[1]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->key.index = (u8)params;
            ret = atcmd_filter_quotation(&keyInfo,(u8 *)tok->arg[2]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->key.key_len = strlen((char *)keyInfo);
            memcpy(cmd->key.key, keyInfo, cmd->key.key_len);
        }
    }
    else if (strcmp("BSSID", at_name) == 0) {
		int len;
	    int i, j;
	    int h, l;
        int ret;
        u32 params;
	    if(tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
	    if (tok->arg_found >= 1) {
			ret = string_to_uint(tok->arg[0], &params);
            if(ret)
				return -CMD_ERR_INV_PARAMS;
			cmd->bssid.enable = (u8)params;
	        if(((cmd->bssid.enable==0)&&(tok->arg_found==2)) ||((cmd->bssid.enable==1)&&(tok->arg_found==1)))
	                return -CMD_ERR_INV_PARAMS;
	        if(tok->arg_found==2)
	        {
	            len = tok->arg[2] - tok->arg[1] - 1;
	            if (len == 12) {
	                for (i = 0, j=0; i<len; i+= 2, j++) {
	                    h = hex_to_digit(tok->arg[1][i]);
	                    l = hex_to_digit(tok->arg[1][i+1]);
	                    if (h < 0 || l < 0) {
	                        return -CMD_ERR_INV_PARAMS;
	                    }
	                    cmd->bssid.bssid[j] = h<<4 | l; 
	                } 
				}else {
	                return -CMD_ERR_INV_PARAMS;
	            }
            }
        }
    }
    else if((strcmp("CHL", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APCHL", at_name) == 0)
#endif
	)
	{
        int ret;
        u32 params;
        if(tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found > 0){
            ret = string_to_uint(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->channel.enable = (u8)params;
            if(cmd->channel.enable == 0 && tok->arg_found > 1)
                return -CMD_ERR_INV_PARAMS;
            if(cmd->channel.enable==0){
                cmd->channel.channel = 1;
                return 0;
            }
            ret = string_to_uint(tok->arg[1], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->channel.channel = (u8)params;                
        }
    }
    else if(strcmp("CHLL", at_name) == 0){
        int ret;
	    u32 params;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 1){
            ret = strtohex(&params, tok->arg[0]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->channel_list.channellist = (u16)params;
        }
    }else if(strcmp("WREG", at_name) == 0 || strcmp("ATLT", at_name) == 0 || strcmp("ATPT", at_name) == 0 ||
             strcmp("ESPT", at_name) == 0
             || (strcmp("WLEAV", at_name) == 0)){
        int ret;
	    u32 params;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 1){
            ret = string_to_uint(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->wreg.region = (u16)params;
            //printf("params = %d  region = %d\n",params, cmd->wreg.region);
        }
    }else if((strcmp("WBGR", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APWBGR", at_name) == 0)
#endif
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON ||WM_NIMBLE_INCLUDED == CFG_ON)
    || (strcmp("BLETPS", at_name) == 0)
	|| (strcmp("BTEN", at_name) == 0) || (strcmp("BLEUM", at_name) == 0)
#endif
)
    {
        int ret;
	    u32 params;
        if(tok->arg_found != 0 && tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 2){
            ret = string_to_uint(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btctrl.type = (u8)params;
            ret = string_to_uint(tok->arg[1], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON ||WM_NIMBLE_INCLUDED == CFG_ON)
			if(strcmp("BTEN", at_name) == 0)
			{
				if(params>(u32)TLS_BT_LOG_VERBOSE)
					return -CMD_ERR_INV_PARAMS;
			}
#endif
            cmd->btctrl.level = (u8)params;
        }
		cmd->btctrl.cmd_mode = tok->cmd_mode;
    }
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON ||WM_NIMBLE_INCLUDED == CFG_ON)

#if (WM_MESH_INCLUDED == CFG_ON)
     else if((strcmp("MSNODEEN", at_name) == 0) ||(strcmp("MSPROVEN", at_name) == 0)||(strcmp("MSSCANPD", at_name) == 0)||(strcmp("MSSCANND", at_name) == 0)
        ||(strcmp("MSNODEDM", at_name) == 0)||(strcmp("MSONOFFPUB", at_name) == 0))
     {
            int ret = 0; 
            u32 param;
            if(tok->arg_found != 1)
                return -CMD_ERR_INV_PARAMS;
            ret = string_to_uint(tok->arg[0], &param);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->bt.cmd = (u8)param;

            if(cmd->bt.cmd >1)
                return -CMD_ERR_INV_PARAMS;

            cmd->bt.cmd_mode = tok->cmd_mode;
    }else if(strcmp("MSOOBNUM", at_name) == 0 || strcmp("MSWRADDR", at_name) == 0 || strcmp("MSWRTTL", at_name) == 0)
    {
            int ret = 0; 
            u32 param;
            if(tok->arg_found != 1)
                return -CMD_ERR_INV_PARAMS;
            ret = string_to_uint(tok->arg[0], &param);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btparamnum.param = param;
   
            cmd->btparamnum.cmd_mode = tok->cmd_mode;
    }else if(strcmp("MSOOBSTR", at_name) == 0)
    {
            if(tok->arg_found != 1)
                return -CMD_ERR_INV_PARAMS;
          
            strncpy((char*)cmd->btparamstr.param, tok->arg[0], 15);
            cmd->btparamstr.cmd_mode = tok->cmd_mode;
            
    }else if (strcmp("MSGETCPD", at_name) == 0 ||strcmp("MSRST", at_name) == 0 || strcmp("MSRELAYGET", at_name) == 0
                || strcmp("MSPROXYGET", at_name) == 0 || strcmp("MSFRDGET", at_name) == 0)
    {
        int ret;
        u32 params;
        
        if (tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
        
        if(tok->arg_found == 2) {
            ret = hexstr_to_unit(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            
            cmd->bt2param.param1 = (u16)params;
            ret = hexstr_to_unit(tok->arg[1], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->bt2param.param2 = (u16)params;
            cmd->bt2param.cmd_mode = tok->cmd_mode;
        }
    }else if (strcmp("MSPROVAD", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 4) {
            ret = strtohexarray(cmd->btprov.uuid, 16, tok->arg[0]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            
            ret = hexstr_to_unit(tok->arg[1], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btprov.net_idx = (u16)params;
            
            ret = hexstr_to_unit(tok->arg[2], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btprov.addr = (u16)params;

            ret = hexstr_to_unit(tok->arg[3], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btprov.attention = (u8)params;           

            cmd->btprov.cmd_mode = tok->cmd_mode;
        }
    }else if (strcmp("MSADDLAKEY", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        

        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamalk.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamalk.app_idx = (u16)params;

        ret = strtohexarray(cmd->btparamalk.param, 16, tok->arg[2]);
        if(ret)
            return -CMD_ERR_INV_PARAMS;          

        cmd->btparamalk.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSADDRAKEY", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamark.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamark.addr = (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamark.key_net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamark.key_app_idx = (u16)params;        

        ret = strtohexarray(cmd->btparamark.param, 16, tok->arg[4]);
        if(ret)
            return -CMD_ERR_INV_PARAMS;          

        cmd->btparamark.cmd_mode = tok->cmd_mode;
    }else if ((strcmp("MSBNDAKEY", at_name) == 0) || (strcmp("MSUBNDAKEY", at_name) == 0))
    {
        int ret;
        u32 params;
        if (tok->arg_found < 5 || tok->arg_found > 6)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparambak.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparambak.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparambak.elem_addr= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparambak.app_key_idx= (u16)params;  
        
        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparambak.mod_id= (u16)params;    

        

        if(tok->arg_found == 6)
        {
            ret = hexstr_to_unit(tok->arg[5], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btparambak.cid= (u16)params;            
        }else
        {
            cmd->btparambak.cid = 0xFFFF;
        }

        cmd->btparambak.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSRELAYSET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamrelay.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamrelay.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamrelay.val= (u8)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamrelay.count= (u8)params;        

        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamrelay.interval= (u8)params;        

        cmd->btparamrelay.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSSUBADD", at_name) == 0 || strcmp("MSSUBDEL", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 5 && tok->arg_found != 6)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubadd.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubadd.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubadd.elem_addr= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubadd.sub_addr= (u16)params;        

        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubadd.mod_id = (u16)params; 

        if(tok->arg_found == 6)
        {
            ret = hexstr_to_unit(tok->arg[5], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btparamsubadd.cid = (u16)params;         
        }else
        {
            cmd->btparamsubadd.cid = 0xFFFF;
        }

        cmd->btparamsubadd.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSSUBGET", at_name) == 0 || strcmp("MSPUBGET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 4 && tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubget.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubget.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubget.elem_addr= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamsubget.mod_id = (u16)params; 

        if(tok->arg_found == 5)
        {
            ret = hexstr_to_unit(tok->arg[4], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btparamsubget.cid = (u16)params;         
        }else
        {
            cmd->btparamsubget.cid = 0xFFFF;
        }

        cmd->btparamsubget.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSHBSUBGET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.net_dst= (u16)params;

        cmd->btparamhbsubset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSHBSUBSET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 8)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.net_dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_src = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_dst = (u16)params; 

        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_period = (u8)params;    

        ret = hexstr_to_unit(tok->arg[5], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_count = (u8)params; 
        
        ret = hexstr_to_unit(tok->arg[6], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_min = (u8)params; 

        ret = hexstr_to_unit(tok->arg[7], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbsubset.hb_sub_max = (u8)params;

        cmd->btparamhbsubset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSHBPUBGET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.net_dst= (u16)params;

        cmd->btparamhbpubset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSHBPUBSET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 8)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.net_dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_dst= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_count = (u8)params; 

        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_period = (u8)params;    

        ret = hexstr_to_unit(tok->arg[5], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_ttl = (u8)params; 
        
        ret = hexstr_to_unit(tok->arg[6], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_feat = (u16)params; 

        ret = hexstr_to_unit(tok->arg[7], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamhbpubset.hb_pub_net_idx = (u16)params;

        cmd->btparamhbpubset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSPUBSET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 11 && tok->arg_found != 12)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.elem_addr= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.mod_id = (u16)params; 

        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_addr = (u16)params;    

        ret = hexstr_to_unit(tok->arg[5], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_app_idx = (u16)params; 
        
        ret = hexstr_to_unit(tok->arg[6], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_cred_flag= (u8)params; 

        ret = hexstr_to_unit(tok->arg[7], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_ttl = (u8)params;

        ret = hexstr_to_unit(tok->arg[8], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_period = (u8)params;
        
        ret = hexstr_to_unit(tok->arg[9], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_count = (u8)params;

        ret = hexstr_to_unit(tok->arg[10], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparampubset.pub_interval = (u8)params;

        if(tok->arg_found == 12)
        {
            ret = hexstr_to_unit(tok->arg[11], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btparampubset.cid = (u16)params;         
        }else
        {
            cmd->btparampubset.cid = 0xFFFF;
        }

        cmd->btparampubset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSPROXYSET", at_name) == 0 || strcmp("MSFRDSET", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamproxy.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamproxy.dst= (u16)params;

        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamproxy.val= (u8)params;       

        cmd->btparamproxy.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSONOFFSET", at_name) == 0 || strcmp("MSLVLSET", at_name) == 0 || strcmp("MSVNDSND", at_name) == 0 || strcmp("MSUART", at_name) == 0)
    {
        int ret;
        u32 params;
        if (tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffset.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffset.dst= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffset.app_idx= (u16)params; 

        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffset.val= (s16)params;       

        cmd->btparamonoffset.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSONOFFGET", at_name) == 0 || strcmp("MSLVLGET", at_name) == 0 )
    {
        int ret;
        u32 params;
        if (tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffget.net_idx = (u16)params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffget.dst= (u16)params;
        
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamonoffget.app_idx = (u16)params;      

        cmd->btparamonoffget.cmd_mode = tok->cmd_mode;
    }else if (strcmp("MSCLRRPL", at_name) == 0 ||strcmp("MSRDADDR", at_name) == 0 ||strcmp("MSRDCFG", at_name) == 0 ||strcmp("MSERASE", at_name) == 0)
    {
        if(tok->arg_found != 0)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd_mode = tok->cmd_mode;
    }

#endif

    else if (strcmp("BTTXPOW", at_name) == 0)
    {
        int ret;
	    u32 params;
        if (tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 2) {
            ret = string_to_uint(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btctrl.type = (u8)params;
            ret = string_to_uint(tok->arg[1], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btctrl.level = (u8)params;
            cmd->btctrl.cmd_mode = tok->cmd_mode;
        }
    }
    else if (strcmp("BTDES", at_name) == 0)
    {
        if(tok->arg_found != 0)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd_mode = tok->cmd_mode;
    }
#if (WM_BT_INCLUDED == CFG_ON)    
    else if (
            (strcmp("BTAVS", at_name) == 0)|| (strcmp("BTHFP", at_name) == 0)
         	|| (strcmp("BTSPPS", at_name) == 0)|| (strcmp("BTSPPC", at_name) == 0))
    {
        int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd = (u8)param;


	    if(cmd->bt.cmd >1)
		return -CMD_ERR_INV_PARAMS;

        cmd->bt.cmd_mode = tok->cmd_mode;
    }
    else if ((strcmp("BTSCM", at_name) == 0)||(strcmp("BTINQUIRY", at_name) == 0))
    {
        int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd = (u8)param;
	
        if(cmd->bt.cmd >2)
			  return -CMD_ERR_INV_PARAMS;

        cmd->bt.cmd_mode = tok->cmd_mode;
    }
    else if(strcmp("BTDIAL", at_name) == 0){
        u8 *namestr = NULL;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        atcmd_filter_quotation(&namestr, (u8 *)tok->arg[0]);
        cmd->btname.len = strlen((char *)namestr);
        memcpy(cmd->btname.name, namestr, cmd->btname.len);
        cmd->btname.cmd_mode = tok->cmd_mode;
    }

#endif    
    else if (
         	(strcmp("BTSLEEP", at_name) == 0) || (strcmp("BTTEST", at_name) == 0)
         	|| (strcmp("BLEDS", at_name) == 0)|| (strcmp("BLEDC", at_name) == 0)|| (strcmp("BLEDCMC", at_name) == 0)
         	|| (strcmp("BLESSCM", at_name) == 0) ||(strcmp("BLEUDS", at_name) == 0)
            || (strcmp("BTRF", at_name) == 0) || (strcmp("BLEHIDS", at_name) == 0)|| (strcmp("BLEHIDU", at_name) == 0)
            )
    {
        int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd = (u8)param;
        if(strcmp("BLESSCM", at_name) == 0)
        {
            if((cmd->bt.cmd&0x80) ||(!cmd->bt.cmd))
                return -CMD_ERR_INV_PARAMS;
        }
        else
        {
		    if(cmd->bt.cmd >1)
			return -CMD_ERR_INV_PARAMS;
        }
        cmd->bt.cmd_mode = tok->cmd_mode;
    }
    else if(strcmp("BLEUSND", at_name) == 0)
    {
        int ret = 0;
        int data_length = 0;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        
        data_length = strlen(tok->arg[0]);
        #if 1
        ret = strtohexarray(cmd->btparamudata.param, data_length>>1, tok->arg[0]);
        if(ret != 0)
            return -CMD_ERR_INV_PARAMS;
        cmd->btparamudata.param_len = data_length>>1;
        #else
        data_length = MIN(data_length, sizeof(cmd->btparamudata.param)-1);
        strncpy(cmd->btparamudata.param, tok->arg[0], data_length);
        cmd->btparamudata.param_len = data_length;
        #endif
        cmd->btparamudata.cmd_mode = tok->cmd_mode;
    }            
    else if ((strcmp("BLEADV", at_name) == 0)|| (strcmp("BLEDMADV", at_name) == 0))
    {
        int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd = (u8)param;
	
        if(cmd->bt.cmd >2)
			  return -CMD_ERR_INV_PARAMS;

        cmd->bt.cmd_mode = tok->cmd_mode;
    }
	else if ((strcmp("BLEDESSV", at_name) == 0) || (strcmp("BLECDES", at_name) == 0))
	{
		int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bt.cmd = (u8)param;
        cmd->bt.cmd_mode = tok->cmd_mode;
	}
    else if (strcmp("BLETPG", at_name) == 0)
    {
        int ret;
	    u32 params;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if (tok->arg_found == 1)
        {
            ret = string_to_uint(tok->arg[0], &params);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->btctrl.type = (u8)params;
            cmd->btctrl.cmd_mode = tok->cmd_mode;
        }
    }
    else if (strcmp("BLEAPRM", at_name) == 0)
    {
        int ret;
        u32 params;
        u8 *tmpmac = NULL;
        if((tok->arg_found < 5) || (tok->arg_found > 8))
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->bleprm.adv_int_min);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[1], (u32 *)&cmd->bleprm.adv_int_max);
        if(ret)
            return -CMD_ERR_INV_PARAMS;

		/*see LE command 7.8.5*/
		if(cmd->bleprm.adv_int_min < 0x20 || cmd->bleprm.adv_int_min > 0x4000) 
			return -CMD_ERR_INV_PARAMS;
		if(cmd->bleprm.adv_int_max < 0x20 || cmd->bleprm.adv_int_max > 0x4000) 
			return -CMD_ERR_INV_PARAMS;	
		if(cmd->bleprm.adv_int_max < cmd->bleprm.adv_int_min)
			return -CMD_ERR_INV_PARAMS;
		
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleprm.adv_type = (u8)params;
        ret = string_to_uint(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleprm.own_addr_type = (u8)params;
        ret = string_to_uint(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleprm.channel_map = (u8)params;
        memset(cmd->bleprm.peer_addr, 0 , ETH_ALEN);
    	switch (tok->arg_found)
    	{
    		case 8:
                 if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[7]))
	                return -CMD_ERR_INV_PARAMS;
    	        if (strtohexarray(cmd->bleprm.peer_addr, ETH_ALEN, (char *)tmpmac)< 0)  
    	            return -CMD_ERR_INV_PARAMS;
    		case 7:
    			ret = string_to_uint(tok->arg[6], &params);
                if(ret)
                    return -CMD_ERR_INV_PARAMS;
                cmd->bleprm.peer_addr_type = (u8)params;
    		case 6:
    			ret = string_to_uint(tok->arg[5], &params);
                if(ret)
                    return -CMD_ERR_INV_PARAMS;
                cmd->bleprm.adv_filter_policy = (u8)params;
    		    break;
    		default:
    			break;
    	}
    }
    else if (strcmp("BLESCAN", at_name) == 0 ||strcmp("BLEDMSCAN", at_name) == 0)
    {
        int ret = 0; 
        u32 param;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &param);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
		/**O or 1 is accepted*/
		if(param>1) 
			return -CMD_ERR_INV_PARAMS;
		
        cmd->blescan.cmd = (u8)param;
        cmd->blescan.cmd_mode = tok->cmd_mode;
    }
	else if (strcmp("BLEADATA", at_name) == 0 || strcmp("BLESCRSP", at_name) == 0)
    {
        int ret = 0; 
		int len = 0;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
		
		len = strlen(tok->arg[0]);
		len = MIN(len, 62);

        if(len&0x01)   //ascii string len must be even length;
            return -CMD_ERR_INV_PARAMS;
		
		cmd->bleadv.len = len/2; //asciistring to hexstring, expected length;
		ret = strtohexarray(cmd->bleadv.data,len/2,tok->arg[0]); 
		if(ret)
			return -CMD_ERR_INV_PARAMS;
		cmd->bleadv.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLECTSV", at_name) == 0 || strcmp("BLECCT", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
		/*Only support bit uuid format*/
		if(params&0xFFFF0000)
			return -CMD_ERR_INV_PARAMS;
        cmd->blesv.uuid = (u16)params;
        cmd->blesv.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLEADDSC", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesc.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesc.inst_id = (u16)params;
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesc.uuid = (u16)params;
        ret = string_to_uint(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesc.handles = (u16)params;
        cmd->blesc.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLEADDCH", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.service_handle = (u16)params;
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.uuid = (u16)params;
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.prop = params;
        ret = hexstr_to_unit(tok->arg[4], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.perm = params;
        cmd->blech.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLEADESC", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.service_handle = (u16)params;
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.uuid = (u16)params;
        ret = hexstr_to_unit(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blech.perm = params;
        cmd->blech.cmd_mode = tok->cmd_mode;
    }
	else if(strcmp("BLESIND", at_name) == 0)
    {
    	int ret;
		u32 params;
        if(tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndind.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndind.attr_handle= (u16)params;
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndind.conn_id= (u16)params;
        strncpy((char *)cmd->blesndind.value, tok->arg[3], sizeof(cmd->blesndind.value));
        cmd->blesndind.cmd_mode = tok->cmd_mode;
    }
	else if(strcmp("BLESRSP", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndrsp.conn_id = (u16)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndrsp.trans_id = (u16)params;
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndrsp.attr_handle = (u16)params;
        ret = string_to_uint(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blesndrsp.auth_req = (u8)params;
        strncpy((char *)cmd->blesndrsp.value, tok->arg[4], sizeof(cmd->blesndind.value));
        cmd->blesndrsp.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLESTTSC", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blestt.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blestt.service_handle = (u16)params;
        ret = hexstr_to_unit(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blestt.tran_type = (u8)params;
        cmd->blestt.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLESTPSC", at_name) == 0 || strcmp("BLEDELSC", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blestt.server_if = (u8)params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blestt.service_handle = (u16)params;
        cmd->blestt.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLESCONN", at_name) == 0 || strcmp("BLECCONN", at_name) == 0)
    {
        int ret;
        u32 params;
        u8 *tmpmac = NULL;
        if(tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleconn.server_if = (u8)params;
        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[1]))
            return -CMD_ERR_INV_PARAMS;
        if (strtohexarray(cmd->bleconn.addr, ETH_ALEN, (char *)tmpmac)< 0)  
            return -CMD_ERR_INV_PARAMS;
        cmd->bleconn.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLERDRSSI", at_name) == 0 )
    {
        u8 *tmpmac = NULL;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        
        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[0]))
            return -CMD_ERR_INV_PARAMS;
        if (strtohexarray(cmd->bleconn.addr, ETH_ALEN, (char *)tmpmac)< 0)  
            return -CMD_ERR_INV_PARAMS;
        
        cmd->bleconn.cmd_mode = tok->cmd_mode;
    }
	else if (strcmp("BLECMTU", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blecmtu.conn_id = (u16)params;

        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blecmtu.mtu = (u16)params;

        cmd->blecmtu.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLESVDIS", at_name) == 0 || strcmp("BLECDIS", at_name) == 0)
    {
        int ret;
        u32 params;
        u8 *tmpmac = NULL;
        if(tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleconn.server_if = (u8)params;
        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[1]))
            return -CMD_ERR_INV_PARAMS;
        if (strtohexarray(cmd->bleconn.addr, ETH_ALEN, (char *)tmpmac)< 0)  
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleconn.conn_id = (u16)params;
        cmd->bleconn.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLECRNTY", at_name) == 0 || strcmp("BLECDNTY", at_name) == 0)
    {
        int ret;
        u32 params;
        u8 *tmpmac = NULL;
        if(tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blenty.client_if = (u8)params;
        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[1]))
            return -CMD_ERR_INV_PARAMS;
        if (strtohexarray(cmd->blenty.addr, ETH_ALEN, (char *)tmpmac)< 0)  
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blenty.attr_handle= (u16)params;

        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->blenty.conn_id = (u16)params; 
        
        cmd->blenty.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLECSSC", at_name) == 0 || strcmp("BLECGDB", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleconn.conn_id = (u16)params;
        cmd->bleconn.cmd_mode = tok->cmd_mode;
    }else if(strcmp("&BTMAC", at_name) == 0){
        u8 *tmpmac = NULL;
        if(tok->arg_found == 1){
	        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[0]))
	            return -CMD_ERR_INV_PARAMS;
	        cmd->mac.length = strlen((char *)tmpmac);
	        if (strtohexarray(cmd->mac.macaddr, ETH_ALEN, (char *)tmpmac)< 0)  
	            return -CMD_ERR_INV_PARAMS;
        	}
    }else if(strcmp("&BTNAME", at_name) == 0){
        u8 *namestr = NULL;
        if(tok->arg_found == 1){
                atcmd_filter_quotation(&namestr, (u8 *)tok->arg[0]);
                cmd->btname.len = strlen((char *)namestr);
                if(cmd->btname.len>16)
                {
                    return -CMD_ERR_INV_PARAMS;
                }
                memcpy(cmd->btname.name, namestr, cmd->btname.len);
                cmd->btname.cmd_mode = tok->cmd_mode;   
        	}
    }
    else if (strcmp("BLECACH", at_name) == 0)
    {
        int ret;
        u32 params;
        u8 *tmpdata;

        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleacc.mode = (u8)params;
        /*read opertion, 4 parameters */
		if(cmd->bleacc.mode == 1)
		{
			if(tok->arg_found != 4)
            	return -CMD_ERR_INV_PARAMS;
		}else if(cmd->bleacc.mode == 0)
		{
			if(tok->arg_found != 5)
            	return -CMD_ERR_INV_PARAMS;			
		}else
		{
			return -CMD_ERR_INV_PARAMS;
		}
		
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleacc.conn_id = (u16)params;
        ret = string_to_uint(tok->arg[2], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleacc.handle = (u16)params;
        ret = string_to_uint(tok->arg[3], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleacc.auth_req = (u8)params;

        /*write operation, handle the input data*/
		if(cmd->bleacc.mode == 0)
		{
	        ret = atcmd_filter_quotation(&tmpdata, (u8 *)tok->arg[4]);
			if (ret)
	            return -CMD_ERR_INV_PARAMS;

	        cmd->bleacc.data_len = strlen((char *)tmpdata);
	        memcpy(cmd->bleacc.data, tmpdata, cmd->bleacc.data_len);
		}
        cmd->bleacc.cmd_mode = tok->cmd_mode;
    }
    else if (strcmp("BLESCPRM", at_name) == 0)
    {
        int ret;
        u32 params;
        if(tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleprm.adv_int_min = params;
        
        ret = hexstr_to_unit(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->bleprm.adv_int_max = params;

        ret = string_to_uint(tok->arg[2], &params);  //adding passive and active scan parametes;
        if(ret)
            return -CMD_ERR_INV_PARAMS; 
        cmd->bleprm.adv_type = (u8)params;

		if(cmd->bleprm.adv_int_min < 0x0004 || cmd->bleprm.adv_int_min > 0x4000)
			return -CMD_ERR_INV_PARAMS;
		if(cmd->bleprm.adv_int_max < 0x0004 || cmd->bleprm.adv_int_max > 0x4000)
			return -CMD_ERR_INV_PARAMS;
		if(cmd->bleprm.adv_int_min > cmd->bleprm.adv_int_max)
			return -CMD_ERR_INV_PARAMS;
        if(cmd->bleprm.adv_type > 1)
            return -CMD_ERR_INV_PARAMS;
    }
#endif
    else if((strcmp("NIP", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APNIP", at_name) == 0)
#endif
	)
	{
        int ret=0;
        int err = 0;
	    u32 params;
		u8 *tmpbuf=NULL;
        if(tok->arg_found !=0 && tok->arg_found !=1 && tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found > 0){
            do{
                ret = string_to_uint(tok->arg[0], &params);
                if(ret){
                    err = 1;
                    break;
                }
                cmd->nip.type = (u8)params;
                if ((tok->arg_found == 1 && cmd->nip.type != 0) ||
                        (tok->arg_found == 5 && cmd->nip.type !=1)) {
                    err = 1;
                    break;
                } 
                if (tok->arg_found == 1)
                    break;
                
                ret = atcmd_filter_quotation(&tmpbuf,(u8 *)tok->arg[1]);
    			if (ret){
    				err = 1;
    				break;
    			}
                ret = string_to_ipaddr((char *)tmpbuf, (u8 *)&params);
                if (ret) {
                    err = 1;
                    break;
                }
                MEMCPY(cmd->nip.ip, (u8 *)&params, 4);
                /* netmask */
    			ret = atcmd_filter_quotation(&tmpbuf,(u8 *)tok->arg[2]);
    			if (ret){
    				err = 1;
    				break;
    			}			
                ret = string_to_ipaddr((char *)tmpbuf, (u8 *)&params);
                if (ret) {
                    err = 1;
                    break;
                }
                MEMCPY(cmd->nip.nm, (u8 *)&params, 4);
                /* gateway */
    			ret = atcmd_filter_quotation(&tmpbuf,(u8 *)tok->arg[3]);
    			if (ret){
    				err = 1;
    				break;
    			}			
                ret = string_to_ipaddr((char *)tmpbuf, (u8 *)&params);
                if (ret) {
                    err = 1;
                    break;
                }
                MEMCPY(cmd->nip.gw, (u8 *)&params, 4);
                /* dns */
    			ret = atcmd_filter_quotation(&tmpbuf,(u8 *)tok->arg[4]);
    			if (ret){
    				err = 1;
    				break;
    			}			
                ret = string_to_ipaddr((char *)tmpbuf, (u8 *)&params);
                if (ret) {
                    err = 1;
                    break;
                }
                MEMCPY(cmd->nip.dns, (u8 *)&params, 4);

                err = 0;
            }while(0);
            if(err)
                return -CMD_ERR_INV_PARAMS;
        }
    }
    else if(strcmp("ATRM", at_name) == 0){
        u32  params;
        int err = 0;
        int ret;
        u8 *tmp;
        struct tls_cmd_socket_t socket;
        if(tok->arg_found != 0 && tok->arg_found != 4)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 4){
            do {
                memset(&socket, 0, sizeof(struct tls_cmd_socket_t));
                /* check protol argument */
                ret = string_to_uint(tok->arg[0], &params);
                if (ret || params > 1) {
                    err = 1;
                    break;
                }
                socket.proto = (u8)params;
                /* check clinet/sever argument */
                ret = string_to_uint(tok->arg[1], &params);
                if (ret || params > 1) {
                    err = 1;
                    break;
                }
                socket.client = (u8)params ? 0 : 1;
    			ret = atcmd_filter_quotation(&tmp, (u8 *)tok->arg[2]);
    			if (ret){
    				err = 1;
    				break;
    			}

                socket.host_len = strlen((char *)tmp);
                if (socket.host_len > 32) {
                    err = 1;
                    break;
                }
                /* check ip or timeout  */
                if (socket.client) {
                    ret = string_to_ipaddr((char *)tmp, (u8 *)&params);
                    if (!ret) {
                        MEMCPY(socket.ip_addr, (u8 *)&params, 4); 
                    }
                    strcpy(socket.host_name, (char *)tmp);
                } else {
                    if (socket.proto == 0) {
                        ret = string_to_uint((char *)tmp, &params);
                        if (ret || params > 10000000) {
                            err = 1;
                            break;
                        }
                        socket.timeout = params; 
    					strcpy(socket.host_name, (char *)tmp);
                    }
                }
                /* check port */
                ret = string_to_uint(tok->arg[3], &params);
                if (ret || (params > 0xFFFF)) {
                    err = 1;
                    break;
                }
                socket.port = params; 

                err = 0;
            } while (0);
            if (err){
    	        return -CMD_ERR_INV_PARAMS;
    	    }else{
    	        cmd->atrm.timeout = socket.timeout; 
    	        memcpy(cmd->atrm.ip_addr, socket.ip_addr, 4);
    	        cmd->atrm.proto = socket.proto;
    	        cmd->atrm.client = socket.client;
    	        cmd->atrm.port = socket.port;
    	        memcpy(cmd->atrm.host_name, socket.host_name, socket.host_len);
    	        cmd->atrm.host_len = socket.host_len;
    	    }
        }
    }
    else if(strcmp("AOLM", at_name) == 0 || strcmp("DDNS", at_name) == 0 || strcmp("UPNP", at_name) == 0 ||
            strcmp("DNAME", at_name) == 0){
        if(tok->arg_found)
            return -CMD_ERR_INV_PARAMS;
    }else if(strcmp("UART", at_name) == 0){
        int err = 0;
        int ret;
        u32 params;
        if(tok->arg_found != 0 && tok->arg_found != 4 && tok->arg_found != 5)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found >= 4){
            do {
                /* baud rate */
                ret = string_to_uint(tok->arg[0], &params);
                if (ret) {
                    err = 1;
                    break;
                }
                MEMCPY(cmd->uart.baud_rate, (u8 *)&params, 3);
                /* char length */
                ret = string_to_uint(tok->arg[1], &params);
                if (ret) {
                    err = 1;
                    break;
                }
                cmd->uart.char_len = params;
                /* stopbit */
                ret = string_to_uint(tok->arg[2], &params);
                if (ret) {
                    err = 1;
                    break;
                }
                cmd->uart.stopbit = params;
                /* parity */
                ret = string_to_uint(tok->arg[3], &params);
                if (ret) {
                    err = 1;
                    break;
                }
                cmd->uart.parity = params;
                /* flow control */
    			if (tok->arg_found == 5){
    	            ret = string_to_uint(tok->arg[4], &params);
    	            if (ret) {
    	                err = 5;
    	                break;
    	            }
    	            cmd->uart.flow_ctrl = params;
    			}else{
    				cmd->uart.flow_ctrl = 0;
    			}

                err = 0;
            } while (0);
            if(err)
                return -CMD_ERR_INV_PARAMS;
        }
    }
    else if(strcmp("&DBG", at_name) == 0){
        u32 dbg;
        int ret = 0;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &dbg);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->dbg.dbg_level = dbg;
    }else if(strcmp("ESPC", at_name) == 0){
        int ret;
	    u32 params;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 1){
            ret = strtohex(&params, tok->arg[0]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->espc.escapechar = (u8)params;
        }
    }else if(strcmp("WEBS", at_name) == 0){
        u32 params;
	    int ret = 0; 
        if(tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found >= 1){
            ret = strtodec((int *)&params, tok->arg[0]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->webs.autorun = (u8)params;
            cmd->webs.portnum = 80;
        }

        if(tok->arg_found >= 2){
            ret = strtodec((int *)&params, tok->arg[1]);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->webs.portnum = (u16)params;
        }
    }
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    else if ((strcmp("ENTM", at_name) == 0) || (strcmp("SKSRCIP", at_name) == 0)){
        ;
    }else if (strcmp("SKCT", at_name) == 0){
		struct tls_cmd_socket_t socket;
	    u32 params;
	    int err = 0;
	    int ret;
        int host_len;
	    u8 *ipstr = NULL;
		struct hostent* HostEntry;

	    if((tok->arg_found != 4) && (tok->arg_found != 5))
	        return -CMD_ERR_INV_PARAMS;
	    do {
	        memset(&socket, 0, sizeof(struct tls_cmd_socket_t));
	        ret = string_to_uint(tok->arg[0], &params);
            if (ret || params > 1) {
                err = 1;
                break;
            }
            socket.proto = (u8)params;
            /* check clinet/sever argument */
            ret = string_to_uint(tok->arg[1], &params);
            if (ret || params > 1) {
                err = 1;
                break;
            }
            socket.client = (u8)params ? 0 : 1;
            host_len = tok->arg[3] - tok->arg[2] - 1;
            if (host_len > 32) {
                err = 1;
                break;
            }
            /* check ip or timeout  */
            if (socket.client) {
    			ret = string_to_ipaddr(tok->arg[2], (u8 *)&params);
    			if (!ret){
    				MEMCPY(socket.ip_addr, (u8 *)&params, 4); 
    			}else
    			{					
    				atcmd_filter_quotation(&ipstr, (u8 *)tok->arg[2]);	
    				HostEntry = gethostbyname((char *)ipstr); 
    				if(HostEntry)
    				{
    					MEMCPY(socket.ip_addr, HostEntry->h_addr_list[0], 4);
                    } else {
                        err = 1;
                        break;
                    }
    			}
                MEMCPY(socket.host_name, tok->arg[2], host_len);
            } else {
                if (socket.proto == 0) {
    				if (*tok->arg[2] != '\0'){
                        ret = string_to_uint(tok->arg[2], &params);
                        if (ret || params > 10000000) {
                            err = 1;
                            break;
                        }
                        socket.timeout = params; 
    				}
                }
            }
            /* check port */
            ret = string_to_uint(tok->arg[3], &params);
            if (ret || (params > 0xFFFF)) {
                err = 1;
                break;
            }
            if((tok->arg_found == 4) && (params == 0))
            {
                err = 1;
                break;
            }
            socket.port = params; 
            socket.host_len = host_len;
        /* check local port */
            if(tok->arg_found == 5)
            {
                ret = string_to_uint(tok->arg[4], &params);
                if (ret || (params > 0xFFFF)) {
                    err = 1;
                    break;
                }
                if((socket.proto == 0) && (socket.client == 0))
                {
                    if(params != 0)
                    {
                        socket.port = params;
                    }
                    else
                    {
                        if(socket.port == 0)
                        {
                            err = 1;
                            break;
                        }
                    }
                }
                else
                {
                    if((params == 0) || (socket.port == 0))
                    {
                        err = 1;
                        break;
                    }
                }
                socket.localport = params;
            }
//            if((socket.proto == 1) && (socket.client == 1) && (tok->arg_found == 4))
//            {
//                socket.localport = socket.port;
//            }

            err = 0;
	    } while (0);
	    if (err){
	        return -CMD_ERR_INV_PARAMS;
	    }else{
	        cmd->skct.timeout = socket.timeout; 
	        memcpy(cmd->skct.ip_addr, socket.ip_addr, 4);
	        cmd->skct.proto = socket.proto;
	        cmd->skct.client = socket.client;
	        cmd->skct.port = socket.port;
	        memcpy(cmd->skct.host_name, socket.host_name, socket.host_len);
	        cmd->skct.host_len = socket.host_len;
	        cmd->skct.localport = socket.localport;
            cmd->skct.mode = tok->cmd_mode;
	    }
    }
    else if((strcmp("SKSTT", at_name) == 0) || (strcmp("SKCLS", at_name) == 0) || (strcmp("SKSDF", at_name) == 0)){
        int err;
        u32 params;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        err = string_to_uint(tok->arg[0], &params);
        if(err)
            return -CMD_ERR_INV_PARAMS;
        cmd->skstt.socket = params;
    }else if ((strcmp("SKSND", at_name) == 0) || (strcmp("SKRCV", at_name) == 0)){
        int ret;
        u32 params;
		if (tok->cmd_mode == CMD_MODE_UART0_ATCMD)
		{
			return -CMD_ERR_UNSUPP;
		}

        if(tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->sksnd.socket = params;
        ret = string_to_uint(tok->arg[1], &params);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->sksnd.size = params;
    }else if(strcmp("SKRPTM", at_name) == 0){
        int err;
        u32 params;
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found==1){
            err = string_to_uint(tok->arg[0], &params);
            if(err)
                return -CMD_ERR_INV_PARAMS;
            cmd->skrptm.mode = params;
        }
    }else if(strcmp("SKGHBN", at_name) == 0){
        u8 *ipstr = NULL;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        atcmd_filter_quotation(&ipstr, (u8 *)tok->arg[0]);
        memcpy(cmd->skghbn.ipstr, ipstr, strlen((char *)ipstr));
    }
#endif
#if TLS_CONFIG_HTTP_CLIENT_TASK
    else if(strcmp("HTTPC", at_name) == 0){

    int ret, verb;
	    u8 * uri;
        if(tok->arg_found != 2 && tok->arg_found != 3)
            return -CMD_ERR_INV_PARAMS;
        ret = atcmd_filter_quotation(&uri,(u8 *)tok->arg[0]);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->httpc.url_len = strlen((char *)uri);
        cmd->httpc.url = uri;
        ret = string_to_uint(tok->arg[1], (u32 *)&verb);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->httpc.verb = (u8)verb;
        if(verb == VerbPost || verb == VerbPut){
            if(tok->arg_found != 3){
                return -CMD_ERR_INV_PARAMS;
            }
            cmd->httpc.data_len = strlen(tok->arg[2]);
            cmd->httpc.data = (u8 *)tok->arg[2];
        }
    }
    else if(strcmp("FWUP", at_name) == 0){

        int ret;
	    u8 * uri;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = atcmd_filter_quotation(&uri,(u8 *)tok->arg[0]);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->httpc.url_len = strlen((char *)uri);
        cmd->httpc.url = uri;
    }
#endif
#endif
    else if(strcmp("&UPDM", at_name) == 0){
        int ret, mode;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], (u32 *)&mode);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->updm.mode = (u8)mode;
        cmd->updm.src = 0;
    }else if(strcmp("&UPDD", at_name) == 0){
        int ret, datasize;
        cmd_set_uart1_mode_callback callback;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], (u32 *)&datasize);
        if(ret)
            return -CMD_ERR_INV_PARAMS;

        if (tls_get_fwup_mode())
        {
            cmd->updd.size = (u16)datasize;
            cmd->updd.data[0] = 0;/* æ è¯æ¯atæä»¤ */
            if(tok->cmd_mode == CMD_MODE_UART1_ATCMD)
            {
                callback = tls_cmd_get_set_uart1_mode();
                if(callback!=NULL)
                    callback(UART_ATDATA_MODE);
            }else if (tok->cmd_mode == CMD_MODE_UART0_ATCMD){
                callback = tls_cmd_get_set_uart0_mode();
                if (callback != NULL)
                    callback(UART_ATDATA_MODE);
            }
        }
    }
    else if(strcmp("&REGR", at_name) == 0 || strcmp("&FLSR", at_name) == 0){
        int ret;
        u32 Addr, Num;
        if(tok->arg_found != 2)
            return -CMD_ERR_OPS;
        ret = hexstr_to_unit(tok->arg[0], &Addr);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->regr.reg_base_addr = Addr;
        ret = hexstr_to_unit(tok->arg[1], &Num);
    	if(ret)
    		return -CMD_ERR_INV_PARAMS;
        cmd->regr.length = Num;
    }else if(strcmp("&REGW", at_name) == 0 || strcmp("&FLSW", at_name) == 0){
        int ret;
	    u32 Addr, Value, i;
        if (tok->arg_found <2 || tok->arg_found>9)
            return -CMD_ERR_OPS;
        ret = hexstr_to_unit(tok->arg[0], &Addr);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->regw.reg_base_addr = Addr;
        cmd->regw.length = tok->arg_found - 1;
        for(i=0;i<cmd->regw.length;i++){
            ret = hexstr_to_unit(tok->arg[i+1], &Value);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->regw.v[i] = Value;
        }
    }else if(strcmp("&RFR", at_name) == 0){
        int ret;
        u32 Addr, Num;
        if(tok->arg_found != 2)
            return -CMD_ERR_OPS;
        ret = hexstr_to_unit(tok->arg[0], &Addr);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->rfr.reg_base_addr = (u16)Addr;
        ret = hexstr_to_unit(tok->arg[1], &Num);
    	if(ret)
    		return -CMD_ERR_INV_PARAMS;
        cmd->rfr.length = Num;
    }else if(strcmp("&RFW", at_name) == 0){
        int ret;
	    u32 Addr, Value, i;
        if (tok->arg_found <2 || tok->arg_found>9)
            return -CMD_ERR_OPS;
        ret = hexstr_to_unit(tok->arg[0], &Addr);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->rfw.reg_base_addr = (u16)Addr;
        cmd->rfw.length = tok->arg_found - 1;
        for(i=0;i<cmd->rfw.length;i++){
            ret = hexstr_to_unit(tok->arg[i+1], &Value);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            cmd->rfw.v[i] = (u16)Value;
        }
    }else if(strcmp("&TXG", at_name) == 0){
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 1){
            if (strtohexarray(cmd->txg.tx_gain, TX_GAIN_LEN, tok->arg[0]) < 0)
    		{
    			return -CMD_ERR_INV_PARAMS;
    		}
        }
    }
	else if (strcmp("&TXGI", at_name) == 0){
        if(tok->arg_found > 1)
            return -CMD_ERR_INV_PARAMS;

		if (tok->arg_found == 1)
		{
			if (strtohexarray(cmd->txg.tx_gain, TX_GAIN_LEN/3, tok->arg[0]) < 0)
			{
				return -CMD_ERR_INV_PARAMS;
			}
		}
    }	
	else if (strcmp("&TXGS", at_name) == 0){
	if (tok->arg_found >=1)
	{
		u32 rate;
		if (0 != string_to_uint(tok->arg[0], (u32 *)&rate))
			return -CMD_ERR_INV_PARAMS;
		cmd->txgr.tx_rate = rate;
	}

        if (tok->arg_found ==2)
        {
            if (strtohexarray(cmd->txgr.txr_gain, 3, tok->arg[1]) < 0)
    	   {
    		return -CMD_ERR_INV_PARAMS;
    	   }
        }
    }
    else  if (strcmp("&TXGG", at_name) == 0)
    {
	if (tok->arg_found >=1)
	{
		u32 rate;
		if (0 != string_to_uint(tok->arg[0], (u32 *)&rate))
			return -CMD_ERR_INV_PARAMS;
		cmd->txgr.tx_rate = rate;
	}
    }

    else if(strcmp("&MAC", at_name) == 0){
        u8 *tmpmac = NULL;
        if(tok->arg_found == 1){
	        if (atcmd_filter_quotation(&tmpmac, (u8 *)tok->arg[0]))
	            return -CMD_ERR_INV_PARAMS;
	        cmd->mac.length = strlen((char *)tmpmac);
	        if (strtohexarray(cmd->mac.macaddr, ETH_ALEN, (char *)tmpmac)< 0)  
	            return -CMD_ERR_INV_PARAMS;
        	}
    }
    else if (strcmp("TXLO", at_name) == 0)
    {
    	int ret = 0;
	u32 value = 0;

        if (tok->arg_found == 1){
            ret = hexstr_to_unit(tok->arg[0], &value);
	   if (ret)
	   {
	      return -CMD_ERR_INV_PARAMS;
	   }
	   cmd->txLO.txlo = value;
       }
    }
    else if (strcmp("TXIQ", at_name) == 0)
    {
    	int ret = 0;
    	u32 value = 0;

        if (tok->arg_found == 2){    
    	    ret = hexstr_to_unit(tok->arg[0],  &value);
	    if (ret)
	    {
		    return -CMD_ERR_INV_PARAMS;
	    }
	    cmd->txIQ.txiqgain = value;

	    ret = hexstr_to_unit(tok->arg[1],  &value);
	    if (ret)
	    {
		    return -CMD_ERR_INV_PARAMS;
	    }
	    cmd->txIQ.txiqphase = value;
        	}
    }
    else if (strcmp("FREQ", at_name) == 0)
    {
    	int ret = 0;
    	int value = 0;

        if (tok->arg_found == 1){   
		    ret = strtodec(&value, tok->arg[0]);
		    if (ret)
		    {
			    return -CMD_ERR_INV_PARAMS;
		    }
		    cmd->FreqErr.freqerr = value;
        }
    }
   else if(strcmp("&SPIF", at_name) == 0){
        int ret, len;
        if(tok->arg_found != 1 && tok->arg_found != 2)
            return -CMD_ERR_INV_PARAMS;
        ret = string_to_uint(tok->arg[0], (u32 *)&len);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        cmd->spif.len = (u8)len;
        if(tok->arg_found == 2){
            if (strtohexarray(cmd->spif.data, cmd->spif.len, (char *)tok->arg[1]) < 0)
                return -CMD_ERR_INV_PARAMS;
            cmd->spif.mode = 1;
        }else
            cmd->spif.mode =0;
    }else if(strcmp("&LPCHL", at_name) == 0){
        int ret;

        if(tok->arg_found == 1){
            ret = string_to_uint(tok->arg[0], (u32 *)&cmd->lpchl.channel);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
			cmd->lpchl.bandwidth = 0;
        }else if(tok->arg_found == 2){
            ret = string_to_uint(tok->arg[0], (u32 *)&cmd->lpchl.channel);
            if(ret)
                return -CMD_ERR_INV_PARAMS;            
			ret = string_to_uint(tok->arg[1], (u32 *)&cmd->lpchl.bandwidth);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
        }else{
			return -CMD_ERR_INV_PARAMS;
        }
    }else if(strcmp("&LPTSTR", at_name) == 0){
        int ret;
        if((tok->arg_found < 5) || (tok->arg_found > 8))
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->lptstr.tempcomp);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[1], (u32 *)&cmd->lptstr.packetcount);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[2], (u32 *)&cmd->lptstr.psdulen);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[3], (u32 *)&cmd->lptstr.txgain);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[4], (u32 *)&cmd->lptstr.datarate);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
	switch (tok->arg_found)
	{
		case 8:
			ret = hexstr_to_unit(tok->arg[7], (u32 *)&cmd->lptstr.gimode);
			if(ret)
				return -CMD_ERR_INV_PARAMS;
		case 7:
			ret = hexstr_to_unit(tok->arg[6], (u32 *)&cmd->lptstr.greenfield);
			if(ret)
				return -CMD_ERR_INV_PARAMS;
		case 6:
			ret = hexstr_to_unit(tok->arg[5], (u32 *)&cmd->lptstr.rifs);
			if(ret)
				return -CMD_ERR_INV_PARAMS;
		break;
		default:
			break;
	}	
    }
    else if(strcmp("&LPRSTR", at_name) == 0 || strcmp("&LPCHRS", at_name) == 0 || strcmp("&LPCHLR", at_name) == 0){
        int ret;
        if((tok->arg_found != 1)&&(tok->arg_found != 2))
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->lpchl.channel);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
		if (tok->arg_found == 2){
	        ret = hexstr_to_unit(tok->arg[1], (u32 *)&cmd->lpchl.bandwidth);
	        if(ret)
	            return -CMD_ERR_INV_PARAMS;
		}
    }else if(strcmp("&LPPSTR", at_name) == 0){
        int ret;
        if(tok->arg_found != 0 && tok->arg_found !=2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found == 2){
            ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->lppstr.param);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
            ret = hexstr_to_unit(tok->arg[1], (u32 *)&cmd->lppstr.start);
            if(ret)
                return -CMD_ERR_INV_PARAMS;
        }
    }else if(strcmp("&LPPSTP", at_name) == 0){
        int ret;
        if(tok->arg_found != 1)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->lppstp.mismatch);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
    }else if(strcmp("&LPTBD", at_name) == 0){
        int ret;
        if (tok->arg_found != 7) 
            return -CMD_ERR_INV_PARAMS;
        cmd->lptstr.packetcount = 0;
        ret = hexstr_to_unit(tok->arg[0], (u32 *)&cmd->lptstr.psdulen);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[1], (u32 *)&cmd->lptstr.txgain);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
        ret = hexstr_to_unit(tok->arg[2], (u32 *)&cmd->lptstr.datarate);
        if(ret)
            return -CMD_ERR_INV_PARAMS;
    }else if (strcmp("WIDTH", at_name) == 0){
        int ret;
        if (tok->arg_found != 2) 
            return -CMD_ERR_INV_PARAMS;

		ret = string_to_uint(tok->arg[0], (u32 *)&cmd->width.freq);
        if(ret)
            return -CMD_ERR_INV_PARAMS;

		ret = string_to_uint(tok->arg[1], (u32 *)&cmd->width.dividend);
        if(ret)
            return -CMD_ERR_INV_PARAMS;

    }
	else if (strcmp("&RXSIN", at_name) == 0){
		int ret;
		if (tok->arg_found != 2) 
		   return -CMD_ERR_INV_PARAMS;

		ret = string_to_uint(tok->arg[0], (u32 *)&cmd->rxsin.rxlen);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;

		ret = string_to_uint(tok->arg[1], (u32 *)&cmd->rxsin.isprint);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;

	}
	else if (strcmp("CPUSTA", at_name) == 0){
		int ret;
		if (tok->arg_found != 1)
			return -CMD_ERR_INV_PARAMS;
		ret = string_to_uint(tok->arg[0], (u32 *)&cmd->width.freq);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;
	}
	else if (strcmp("CPUDIV", at_name) == 0)
	{
		int ret;
		if (tok->arg_found > 1)
			return -CMD_ERR_INV_PARAMS;
		if(tok->arg_found == 1)
		{
			ret = string_to_uint(tok->arg[0], (u32 *)&cmd->width.freq);
			if(ret)
			   return -CMD_ERR_INV_PARAMS;
		}
	}
	else if (strcmp("&LPTPD", at_name) == 0)
	{
		int ret;
		if (tok->arg_found == 1)
        {	
			ret = string_to_uint(tok->arg[0], (u32 *)&cmd->rxsin.rxlen);
			if(ret)
			   return -CMD_ERR_INV_PARAMS;
		}
	}
	else if (strcmp("&CALFIN", at_name) == 0)
	{
		int ret;
		if (tok->arg_found)
        {	
			ret = strtodec(&cmd->calfin.val, tok->arg[0]);
			if(ret || ((cmd->calfin.val != 1) && (cmd->calfin.val != 0)))
			   return -CMD_ERR_INV_PARAMS;
		}
	}
#if TLS_CONFIG_WIFI_PERF_TEST
    else if(strcmp("THT", at_name) == 0){	
		cmd->tht.tok = (u32 *)tok;
		return 0;
#if 0		
        struct tht_param* tht = (struct tht_param*)(&gThtSys);
        CreateThroughputTask();
        memset(tht, 0, sizeof(struct tht_param));
        if(tht_parse_parameter(tht, tok) == 0){
		    OSQPost(tht_q,TLS_MSG_WIFI_PERF_TEST_START);
            return 0;
        }else
            return -CMD_ERR_INV_PARAMS;
#endif
    }
#endif
#if TLS_CONFIG_WPS 
    else if(strcmp("WWPS", at_name) == 0){
        if(tok->arg_found > 2)
            return -CMD_ERR_INV_PARAMS;
        if(tok->arg_found >= 1){
            if(!strcmp(tok->arg[0], "get_pin"))
                cmd->wps.mode = 0;
            else if(!strcmp(tok->arg[0], "set_pin")){
                if(tok->arg_found != 2)
                    return -CMD_ERR_INV_PARAMS;
                cmd->wps.mode = 1;
                cmd->wps.pin_len = strlen(tok->arg[1]);
                MEMCPY(cmd->wps.pin, tok->arg[1], cmd->wps.pin_len);
            }else if(!strcmp(tok->arg[0], "start_pin"))
                cmd->wps.mode = 2;
            else if(!strcmp(tok->arg[0], "start_pbc"))
                cmd->wps.mode = 3;
            else
                return -CMD_ERR_INV_PARAMS;
        }
    }
#endif
#if TLS_CONFIG_WIFI_PING_TEST
	else if(strcmp("PING", at_name) == 0){
	    if (CMD_MODE_UART0_ATCMD == tok->cmd_mode)
	    {
	        cmd->ping.src = 0;
	    }
	    else
	    {
	        cmd->ping.src = 1;
	    }
		if (tok->arg_found == 4)
        {
		int ret;
		cmd->ping.ip = (u8 *)tok->arg[0];
		ret = string_to_uint(tok->arg[1], (u32 *)&cmd->ping.timeLimt);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;
		ret = string_to_uint(tok->arg[2], (u32 *)&cmd->ping.cnt);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;
		ret = string_to_uint(tok->arg[3], (u32 *)&cmd->ping.start);
		if(ret)
		   return -CMD_ERR_INV_PARAMS;
            cmd->ping.ext = 1;
	    }
	    else if (tok->arg_found == 1)
	    {
            cmd->ping.ip = (u8 *)tok->arg[0];
    		cmd->ping.ext = 0;
	    }
	    else
	    {
            return -CMD_ERR_INV_PARAMS;
	    }
	}
#endif
//    else{
//        return -CMD_ERR_UNSUPP;
//    }
    return 0;
}

int at_format_func(char *at_name, u8 set_opt, u8 update_flash, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp, char *res_resp, u32 *res_len){
    if(strcmp("QMAC", at_name) == 0 || strcmp("&HWV", at_name) == 0
#if TLS_CONFIG_AP
       || strcmp("APMAC", at_name) == 0
#endif
      ){
        *res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x", 
                cmdrsp->mac.addr[0], cmdrsp->mac.addr[1], cmdrsp->mac.addr[2], 
                cmdrsp->mac.addr[3], cmdrsp->mac.addr[4], cmdrsp->mac.addr[5]);
    }
    else if(strcmp("TEM", at_name) == 0)
    {
        if (set_opt) {
    		*res_len = atcmd_ok_resp(res_resp);
    	}
        else {
            *res_len = sprintf(res_resp, "+OK=%s", cmdrsp->tem.offset);
    	}
    }
#if TLS_CONFIG_AT_CMD
    else if ((strcmp("Z", at_name) == 0) || (strcmp("E", at_name) == 0) || (strcmp("ENTS", at_name) == 0) ||
        (strcmp("RSTF", at_name) == 0) || (strcmp("PMTF", at_name) == 0) || (strcmp("IOC", at_name) == 0) ||
        (strcmp("WLEAV", at_name) == 0) || (strcmp("AOLM", at_name) == 0) || (strcmp("DDNS", at_name) == 0) ||
        (strcmp("UPNP", at_name) == 0) || (strcmp("DNAME", at_name) == 0) || (strcmp("&DBG", at_name) == 0) ||
        (strcmp("&UPDP", at_name) == 0) || (strcmp("STDBY", at_name) == 0)
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON)
	    || (strcmp("BTSLEEP", at_name) == 0)
	    || (strcmp("BLETPS", at_name) == 0) || (strcmp("BLESSCM", at_name) == 0)|| (strcmp("BLEUDS", at_name) == 0)|| (strcmp("BLEUSND", at_name) == 0)
	    || (strcmp("BTTEST", at_name) == 0)|| (strcmp("BLEDS", at_name) == 0)|| (strcmp("BLEDC", at_name) == 0) || (strcmp("BLEUM", at_name) == 0)
	    || (strcmp("BLEADV", at_name) == 0) || (strcmp("BLEADATA", at_name) == 0) || (strcmp("BLESCRSP", at_name) == 0)
	    || (strcmp("BLESCPRM", at_name) == 0) || (strcmp("BLESFLT", at_name) == 0) || (strcmp("BLESCAN", at_name) == 0)
	    || (strcmp("BTAVS", at_name) == 0)|| (strcmp("BTHFP", at_name) == 0)|| (strcmp("BTSPPS", at_name) == 0)|| (strcmp("BTSPPC", at_name) == 0)
	    || (strcmp("BTSCM", at_name) == 0) || (strcmp("BLEDMADV", at_name) == 0) || (strcmp("BLEDMSCAN", at_name) == 0) || (strcmp("BTRF", at_name) == 0)
        ||(strcmp("BTINQUIRY", at_name) == 0)|| (strcmp("BLEDCMC", at_name) == 0)|| (strcmp("BLEHIDS", at_name) == 0)|| (strcmp("BLEHIDU", at_name) == 0)
#if (WM_MESH_INCLUDED == CFG_ON)
        ||(strcmp("MSSCANPD", at_name) == 0)||(strcmp("MSSCANND", at_name) == 0)
        ||(strcmp("MSRST", at_name) == 0)||(strcmp("MSOOBNUM", at_name) == 0)||(strcmp("MSWRADDR", at_name) == 0) ||(strcmp("MSWRTTL", at_name) == 0)
        ||(strcmp("MSOOBSTR", at_name) == 0)||(strcmp("MSCLRRPL", at_name) == 0) || (strcmp("MSVNDSND", at_name) == 0)|| (strcmp("MSUART", at_name) == 0)  
#endif
#endif
        ){
        *res_len = atcmd_ok_resp(res_resp);
    }else if(strcmp("MSPROVAD", at_name) == 0)
    {
        *res_len = 0;
    }else if (strcmp("WJOIN", at_name) == 0) {
		int len=0,i=0;
	    len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x,%d,%d,%d,\"",
	                    cmdrsp->join.bssid[0],cmdrsp->join.bssid[1], cmdrsp->join.bssid[2],
	                    cmdrsp->join.bssid[3],cmdrsp->join.bssid[4], cmdrsp->join.bssid[5],
	                    cmdrsp->join.type, cmdrsp->join.channel,
	                    (cmdrsp->join.encrypt?1:0));
	    for (i = 0; i < cmdrsp->join.ssid_len; i++)
	        sprintf(res_resp+len+i, "%c", cmdrsp->join.ssid[i]);
	    *res_len = len + cmdrsp->join.ssid_len;
	    len = sprintf(res_resp+len + cmdrsp->join.ssid_len, "\",%d", (signed char)cmdrsp->join.rssi);
	    *res_len += len;
    }else if (strcmp("WSCAN", at_name) == 0) {
		*res_len = 0;
    }else if (strcmp("LKSTT", at_name) == 0
#if TLS_CONFIG_AP
              || strcmp("APLKSTT", at_name) == 0
#endif
             ) {
		if (cmdrsp->lkstt.status == 0) {
	        *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->lkstt.status);
	    } else {
	        *res_len = sprintf(res_resp, "+OK=%d,\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"",
                cmdrsp->lkstt.status,
	            cmdrsp->lkstt.ip[0], cmdrsp->lkstt.ip[1], cmdrsp->lkstt.ip[2], cmdrsp->lkstt.ip[3],
	            cmdrsp->lkstt.nm[0], cmdrsp->lkstt.nm[1], cmdrsp->lkstt.nm[2], cmdrsp->lkstt.nm[3],
	            cmdrsp->lkstt.gw[0], cmdrsp->lkstt.gw[1], cmdrsp->lkstt.gw[2], cmdrsp->lkstt.gw[3],
	            cmdrsp->lkstt.dns1[0], cmdrsp->lkstt.dns1[1], cmdrsp->lkstt.dns1[2], cmdrsp->lkstt.dns1[3],
	            cmdrsp->lkstt.dns2[0], cmdrsp->lkstt.dns2[1], cmdrsp->lkstt.dns2[2], cmdrsp->lkstt.dns2[3]);
	    }
    }else if (strcmp("DNS", at_name) == 0 || strcmp("PASS", at_name) == 0){
        if (set_opt) {
    		*res_len = atcmd_ok_resp(res_resp);
    	}else{
            *res_len = sprintf(res_resp, "+OK=\"%s\"", cmdrsp->ssid.ssid);
    	}
    }else if(strcmp("SSID", at_name) == 0
#if TLS_CONFIG_AP
             || strcmp("APSSID", at_name) == 0
#endif
            ){
        if (set_opt) {
    		*res_len = atcmd_ok_resp(res_resp);
    	}else{
            *res_len = sprintf(res_resp, "+OK=%s", cmdrsp->ssid.ssid);
    	}
    }else if((strcmp("WPRT", at_name) == 0) || (strcmp("ENCRY", at_name) == 0) || (strcmp("BRDSSID", at_name) == 0) ||
             (strcmp("WATC", at_name) == 0) || (strcmp("WPSM", at_name) == 0) || (strcmp("WARC", at_name) == 0) ||
             (strcmp("WARM", at_name) == 0) || (strcmp("ATM", at_name) == 0) || (strcmp("PORTM", at_name) == 0) ||
             (strcmp("IOM", at_name) == 0) || (strcmp("CMDM", at_name) == 0) || (strcmp("ONESHOT", at_name) == 0) 
#if TLS_CONFIG_AP
	    ||(strcmp("APENCRY", at_name) == 0)
#endif
    ){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%hhu", cmdrsp->bt.status);
		}
    }
    else if((strcmp("KEY", at_name) == 0)
#if TLS_CONFIG_AP
		||(strcmp("APKEY", at_name) == 0)
#endif
    )
    {
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%u,%u,", cmdrsp->key.format, cmdrsp->key.index);
            MEMCPY(res_resp + *res_len, cmdrsp->key.key, cmdrsp->key.key_len);
            *res_len += cmdrsp->key.key_len;
		}
    }else if (strcmp("BSSID", at_name) == 0) {
		if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        if(cmdrsp->bssid.enable)
	        {
	            *res_len = sprintf(res_resp, "+OK=%u,%02x%02x%02x%02x%02x%02x", 
	                    cmdrsp->bssid.enable,
	                    cmdrsp->bssid.bssid[0],cmdrsp->bssid.bssid[1],cmdrsp->bssid.bssid[2],
	                    cmdrsp->bssid.bssid[3],cmdrsp->bssid.bssid[4],cmdrsp->bssid.bssid[5]);
	        }
	        else
	        {
	            *res_len = sprintf(res_resp, "+OK=%u",cmdrsp->bssid.enable);
	        }
		}
    }else if(strcmp("CNTPARAM", at_name) == 0){
        int i=0;
        if(!set_opt){
            if(cmdrsp->cntparam_bssid_en.bssid_enable){
                *res_len = sprintf(res_resp, "+OK=%u,%02x%02x%02x%02x%02x%02x,",
                    cmdrsp->cntparam_bssid_en.bssid_enable,
                    cmdrsp->cntparam_bssid_en.bssid[0],cmdrsp->cntparam_bssid_en.bssid[1],cmdrsp->cntparam_bssid_en.bssid[2],
                    cmdrsp->cntparam_bssid_en.bssid[3],cmdrsp->cntparam_bssid_en.bssid[4],cmdrsp->cntparam_bssid_en.bssid[5]);
                MEMCPY(res_resp + *res_len, cmdrsp->cntparam_bssid_en.key, cmdrsp->cntparam_bssid_en.key_len);
                *res_len += cmdrsp->cntparam_bssid_en.key_len;
            }else{
                *res_len = sprintf(res_resp, "+OK=%u,",cmdrsp->cntparam_bssid_dis.bssid_enable);
                for(i=0;i<cmdrsp->cntparam_bssid_dis.ssid_len;i++)
                    *res_len += sprintf(res_resp + (*res_len), "%c", cmdrsp->cntparam_bssid_dis.ssid_key[i]);
                *res_len += sprintf(res_resp + *res_len,",");
                MEMCPY(res_resp + *res_len, cmdrsp->cntparam_bssid_dis.ssid_key+cmdrsp->cntparam_bssid_dis.ssid_len, cmdrsp->cntparam_bssid_dis.key_len);
                *res_len += cmdrsp->cntparam_bssid_dis.key_len;
            }
        }
    }else if((strcmp("CHL", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APCHL", at_name) == 0)
#endif
	){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        if(cmdrsp->channel.enable)
	        {
	            *res_len = sprintf(res_resp, "+OK=%u,%u", cmdrsp->channel.enable, cmdrsp->channel.channel);
	        }
	        else
	        {
	            *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->channel.enable);
	        }
	}
    }else if(strcmp("CHLL", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%04x", cmdrsp->channel_list.channellist);
		}
    }
#if (WM_BLE_INCLUDED == CFG_ON || WM_BT_INCLUDED == CFG_ON || WM_NIMBLE_INCLUDED == CFG_ON)

    if((strcmp("BTCTRLGS", at_name) == 0) || (strcmp("BLETPG", at_name) == 0)){
        if (set_opt) {
            *res_len = atcmd_ok_resp(res_resp);
        }else{
            *res_len = sprintf(res_resp, "+OK=%hu", cmdrsp->bt.status);
        }
     }

    else if (strcmp("BTTXPOW", at_name) == 0)
    {
        if (set_opt) {
    		*res_len = atcmd_ok_resp(res_resp);
    	}
        else {
            *res_len = sprintf(res_resp, "+OK=%hhd,%hhd", cmdrsp->blepow.min, cmdrsp->blepow.max);
    	}
    }else if(strcmp("&BTMAC", at_name) == 0){
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
            *res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x", 
                cmdrsp->mac.addr[0], cmdrsp->mac.addr[1], cmdrsp->mac.addr[2], 
                cmdrsp->mac.addr[3], cmdrsp->mac.addr[4], cmdrsp->mac.addr[5]); 
    }
 #if (WM_NIMBLE_INCLUDED == CFG_ON)   
     else if (strcmp("BTEN", at_name) == 0|| strcmp("BTDES", at_name) == 0 )
     {
         *res_len = atcmd_ok_resp(res_resp);
     }
     else if(strcmp("&BTNAME", at_name) == 0){
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
            *res_len = sprintf(res_resp, "+OK=%s", cmdrsp->btname.name); 
    }
#if (WM_MESH_INCLUDED == CFG_ON)
    else if( 
        strcmp("MSSUBADD", at_name) == 0 || strcmp("MSSUBDEL", at_name) == 0 || strcmp("MSPUBSET", at_name) == 0 || strcmp("MSLVLGET", at_name) == 0 
        || strcmp("MSLVLSET", at_name) == 0 || strcmp("MSRDADDR", at_name) == 0 || strcmp("MSHBPUBSET", at_name) == 0 || strcmp("MSHBSUBSET", at_name) == 0
        || strcmp("MSSUBGET", at_name) == 0 ||strcmp("MSNODEDM", at_name) == 0 ||strcmp("MSONOFFPUB", at_name) == 0 ||strcmp("MSERASE", at_name) == 0
        ||strcmp("MSADDRAKEY", at_name) == 0 ||strcmp("MSBNDAKEY", at_name) == 0||strcmp("MSADDLAKEY", at_name) == 0
        ||strcmp("MSUBNDAKEY", at_name) == 0 ||strcmp("MSNODEEN", at_name) == 0||strcmp("MSPROVEN", at_name) == 0
        )
    {
        *res_len = sprintf(res_resp, "+OK=%hu", cmdrsp->bt.status);
    }else if(strcmp("MSHBSUBGET", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%04x,%04x,%02x,%02x,%02x,%02x",cmdrsp->mesh_hb_sub.status,cmdrsp->mesh_hb_sub.src, cmdrsp->mesh_hb_sub.dst,
                    cmdrsp->mesh_hb_sub.period, cmdrsp->mesh_hb_sub.count, cmdrsp->mesh_hb_sub.min, cmdrsp->mesh_hb_sub.max);
    } if(strcmp("MSHBPUBGET", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%04x,%02x,%02x,%02x,%04x,%04x",cmdrsp->mesh_hb_pub.status,cmdrsp->mesh_hb_pub.dst, cmdrsp->mesh_hb_pub.count,
                    cmdrsp->mesh_hb_pub.period, cmdrsp->mesh_hb_pub.ttl, cmdrsp->mesh_hb_pub.feat, cmdrsp->mesh_hb_pub.net_idx);
    }else if(strcmp("MSPUBGET", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%04x,%04x,%02x,%02x,%02x,%02x,%02x",cmdrsp->mesh_pub.status,cmdrsp->mesh_pub.addr, cmdrsp->mesh_pub.app_idx,
                    cmdrsp->mesh_pub.cred_flag, cmdrsp->mesh_pub.ttl, cmdrsp->mesh_pub.period, TLS_BT_MESH_PUB_TRANSMIT_COUNT(cmdrsp->mesh_pub.transmit),TLS_BT_MESH_PUB_TRANSMIT_COUNT(cmdrsp->mesh_pub.transmit));
    }else if(strcmp("MSRDCFG", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",cmdrsp->mesh_primary_cfg.status,cmdrsp->mesh_primary_cfg.net_transmit_count,cmdrsp->mesh_primary_cfg.net_transmit_intvl,
                    cmdrsp->mesh_primary_cfg.relay,cmdrsp->mesh_primary_cfg.relay_retransmit_count, cmdrsp->mesh_primary_cfg.relay_retransmit_intvl,cmdrsp->mesh_primary_cfg.beacon,
                    cmdrsp->mesh_primary_cfg.gatt_proxy, cmdrsp->mesh_primary_cfg.frnd, cmdrsp->mesh_primary_cfg.default_ttl);
    }else if(strcmp("MSSUBGET", at_name) == 0 )
    {
        int index = 0;
        int buf_offset = 0;

        buf_offset = sprintf(res_resp, "+OK=%hu,", cmdrsp->mesh_sub.status);

        for(index = 0; index<cmdrsp->mesh_sub.sub_cnt; index++)
        {
            buf_offset += sprintf(res_resp+buf_offset, ",%04x", cmdrsp->mesh_sub.subs[index]);
        }
                
        *res_len = buf_offset;
    }else if(strcmp("MSGETCPD", at_name) == 0)
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%s", cmdrsp->comp_data.status, cmdrsp->comp_data.data);
    }else if(strcmp("MSRELAYSET", at_name) == 0 || strcmp("MSRELAYGET", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%hu,%hu", cmdrsp->mesh_relay.status, cmdrsp->mesh_relay.count, cmdrsp->mesh_relay.interval);
    }else if(strcmp("MSPROXYSET", at_name) == 0 || strcmp("MSPROXYGET", at_name) == 0|| strcmp("MSFRDSET", at_name) == 0 || strcmp("MSFRDGET", at_name) == 0
        ||strcmp("MSONOFFSET", at_name) == 0 || strcmp("MSONOFFGET", at_name) == 0 )
    {
        *res_len = sprintf(res_resp, "+OK=%hu,%hu", cmdrsp->mesh_resp.status, cmdrsp->mesh_resp.state);
    }
#endif

 #else
    else if (strcmp("BLECTSV", at_name) == 0 || strcmp("BLECCT", at_name) == 0|| strcmp("BTEN", at_name) == 0
		|| strcmp("BTDES", at_name) == 0  || (strcmp("BLEADDSC", at_name) == 0) 
		|| (strcmp("BLEADDCH", at_name) == 0) || (strcmp("BLEADESC", at_name) == 0) ||(strcmp("BLESTTSC", at_name) == 0) 
		|| (strcmp("BLESTPSC", at_name) == 0) || (strcmp("BLEDELSC", at_name) == 0)|| (strcmp("BLEDESSV", at_name) == 0) 
		|| (strcmp("BLESVDIS", at_name) == 0) || (strcmp("BLESIND", at_name) == 0)||(strcmp("BLECMTU", at_name) == 0)
	    || (strcmp("BLESRSP", at_name) == 0) ||(strcmp("BLECDIS", at_name) == 0) ||(strcmp("BLESCONN", at_name) == 0)
	    || (strcmp("BLECSSC", at_name) == 0) || (strcmp("BLECRNTY", at_name) == 0) || (strcmp("BLECDNTY", at_name) == 0)
	    || (strcmp("BLECGDB", at_name) == 0) || (strcmp("BLECCONN", at_name) == 0)||(strcmp("BLECACH", at_name) == 0)
	    ||(strcmp("BLECDES", at_name) == 0) 
	    || (strcmp("BLERDRSSI", at_name) == 0) || (strcmp("&BTNAME", at_name) == 0)
	    )
    {
		*res_len = 0;
    }
 #endif   
    else if((strcmp("BTDIAL", at_name) == 0))
    {
        *res_len = atcmd_ok_resp(res_resp);
    }
    else if (strcmp("BLEAPRM", at_name) == 0)
    {
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%04x,%04x,%hhu,%hhu,%hhu,%hhu,%hhu,%02X%02X%02X%02X%02X%02X", 
	        cmdrsp->bleprm.adv_int_min, cmdrsp->bleprm.adv_int_max,
	        cmdrsp->bleprm.adv_type, cmdrsp->bleprm.own_addr_type, cmdrsp->bleprm.channel_map, 
	        cmdrsp->bleprm.adv_filter_policy, cmdrsp->bleprm.peer_addr_type, 
	        cmdrsp->bleprm.peer_addr[0], cmdrsp->bleprm.peer_addr[1], cmdrsp->bleprm.peer_addr[2],
	        cmdrsp->bleprm.peer_addr[3], cmdrsp->bleprm.peer_addr[4], cmdrsp->bleprm.peer_addr[5]);
		}
    }
#endif
    else if(strcmp("WREG", at_name) == 0 || strcmp("ATLT", at_name) == 0 || strcmp("ATPT", at_name) == 0 ||
             strcmp("ESPT", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->wreg.region);
		}
    }else if((strcmp("WBGR", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APWBGR", at_name) == 0)
#endif
){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%u,%u", cmdrsp->wbgr.mode,
	                    cmdrsp->wbgr.rate);
		}
    }else if((strcmp("NIP", at_name) == 0)
#if TLS_CONFIG_AP
	||(strcmp("APNIP", at_name) == 0)
#endif
    ){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, 
	                    "+OK=%u,%u.%u.%u.%u,%u.%u.%u.%u,%u.%u.%u.%u,%u.%u.%u.%u",
	                    cmdrsp->nip.type,
	                    cmdrsp->nip.ip[0], cmdrsp->nip.ip[1],
	                    cmdrsp->nip.ip[2], cmdrsp->nip.ip[3],
	                    cmdrsp->nip.nm[0], cmdrsp->nip.nm[1],
	                    cmdrsp->nip.nm[2], cmdrsp->nip.nm[3],
	                    cmdrsp->nip.gw[0], cmdrsp->nip.gw[1],
	                    cmdrsp->nip.gw[2], cmdrsp->nip.gw[3],
	                    cmdrsp->nip.dns[0], cmdrsp->nip.dns[1],
	                    cmdrsp->nip.dns[2], cmdrsp->nip.dns[3]);
		}
    }else if (strcmp("ATRM", at_name) == 0) {
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else{
            *res_len = sprintf(res_resp, 
                    "+OK=%u,%u,", cmdrsp->atrm.proto,
                    cmdrsp->atrm.client ? 0 : 1);
            if (cmdrsp->atrm.client) {
    			*res_len += sprintf(res_resp + (*res_len), "\"%s\"", cmdrsp->atrm.host_name);
            } else {
                if (cmdrsp->atrm.proto == 0) {
                    /* TCP */
                    *res_len += sprintf(res_resp + (*res_len),
                            "%d", cmdrsp->atrm.timeout);
                }
            }
            *res_len += sprintf(res_resp + (*res_len), ",%u", cmdrsp->atrm.port); 
        }
    }else if(strcmp("UART", at_name) == 0){
        u32 baud_rate=0;
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
		    memcpy(&baud_rate, cmdrsp->uart.baud_rate, 3);
            
	        *res_len = sprintf(res_resp, 
	                    "+OK=%u,%u,%u,%u,%u",
	                    baud_rate,
	                    cmdrsp->uart.char_len,
	                    cmdrsp->uart.stopbit, cmdrsp->uart.parity,
	                    cmdrsp->uart.flow_ctrl);
		}
    }else if(strcmp("ESPC", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=0x%02x", cmdrsp->espc.escapechar);
		}
    }else if(strcmp("WEBS", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        if (cmdrsp->webs.autorun == 1)
	            *res_len = sprintf(res_resp, "+OK=%d,%d",cmdrsp->webs.autorun, cmdrsp->webs.portnum);
	        else
	        	*res_len = sprintf(res_resp, "+OK=%d", cmdrsp->webs.autorun);
		}
    }
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    else if (strcmp("ENTM", at_name) == 0 || strcmp("SKCLS", at_name) == 0 || strcmp("SKSDF", at_name) == 0){
        *res_len = atcmd_ok_resp(res_resp);
    }else if (strcmp("SKCT", at_name) == 0){
		if (set_opt) {
			*res_len = sprintf(res_resp, "+OK=%d", cmdrsp->skct.socket);
		}
    }else if (strcmp("SKSTT", at_name) == 0){
        struct hostif_cmdrsp_skstt_ext *ext;
	    int i=0;
	    u32 buflen;
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR		
		u32 cpu_sr = 0;
	    struct tls_uart_net_msg * precvmit = NULL;
		struct tls_uart_net_buf *uartnetbuf = NULL;
#else
	    struct tls_uart_circ_buf * precvmit = NULL;		
#endif		
	    if (set_opt) {
			*res_len = sprintf(res_resp, "+OK=");
	        ext = &cmdrsp->skstt.ext[0];
	        
	        for (i = 0; i < cmdrsp->skstt.number; i++) {
	            precvmit =tls_hostif_get_recvmit(ext->socket);
	            if(precvmit == NULL)
	                buflen = 0;
	            else
	            {
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR				
	            	buflen = 0;
					cpu_sr = tls_os_set_critical();
					if (!dl_list_empty(&precvmit->tx_msg_pending_list))
					{
						dl_list_for_each(uartnetbuf,&precvmit->tx_msg_pending_list, struct tls_uart_net_buf,list)
						{					
							if (uartnetbuf->buf)
							{
								buflen += uartnetbuf->buflen - uartnetbuf->offset;
							}
						}
					}
					tls_os_release_critical(cpu_sr);
#else
					buflen = CIRC_CNT(precvmit->head, precvmit->tail, TLS_SOCKET_RECV_BUF_SIZE);
#endif					
	            }				
	            *res_len += sprintf(res_resp + (*res_len), 
	                    "%d,%d,\"%d.%d.%d.%d\",%d,%d,%d\r\n",
	                ext->socket, ext->status, 
	                ext->host_ipaddr[0], ext->host_ipaddr[1], 
	                ext->host_ipaddr[2], ext->host_ipaddr[3],
	                ext->remote_port,ext->local_port, buflen); 
	            ext++;
	        } 
		}
    }
    else if (strcmp("SKSND", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->sksnd.size); 
    }else if(strcmp("SKRCV", at_name) == 0){
        int ret = 0;
	    u32 maxsize=0;
	    u8 socket;
	   
	    if (set_opt) {
	        maxsize = cmdrsp->skrcv.size;
	        socket = cmdrsp->skrcv.socket;
#if TLS_CONFIG_CMD_NET_USE_LIST_FTR	
			struct tls_uart_net_msg * precvmit;
			struct tls_uart_net_buf *uartnetbuf;


			u32 cpu_sr = 0;
			int copylen = 0;
			int remainlen = 0;

			precvmit = tls_hostif_get_recvmit(socket);
			if(precvmit)
			{
				ret = 0;
				cpu_sr = tls_os_set_critical();
				if (!dl_list_empty(&precvmit->tx_msg_pending_list))
				{
					dl_list_for_each(uartnetbuf,&precvmit->tx_msg_pending_list, struct tls_uart_net_buf,list)
					{
						if (uartnetbuf->buf)
						{
							ret += uartnetbuf->buflen - uartnetbuf->offset;
						}
					}
				}
				tls_os_release_critical(cpu_sr);
				/*total buf len*/
				if(ret < maxsize)
					maxsize = ret;
			}	
			else{
				return -CMD_ERR_INV_PARAMS;
			}
			*res_len = sprintf(res_resp, "+OK=%d\r\n\r\n", maxsize);
			copylen = 0;
			remainlen = maxsize;
		    cpu_sr = tls_os_set_critical();
			while (!dl_list_empty(&precvmit->tx_msg_pending_list)) {
				uartnetbuf= dl_list_first(&precvmit->tx_msg_pending_list, struct tls_uart_net_buf, list);
				tls_os_release_critical(cpu_sr);
				if (uartnetbuf->buf == NULL) {
					cpu_sr = tls_os_set_critical();
					break;
				}

				if (remainlen <= (uartnetbuf->buflen - uartnetbuf->offset))
				{
					copylen = remainlen;
					memcpy(res_resp + *res_len, uartnetbuf->buf + uartnetbuf->offset, remainlen);
					remainlen = 0;
				}
				else
				{
					copylen = (uartnetbuf->buflen - uartnetbuf->offset);
					memcpy(res_resp + *res_len, uartnetbuf->buf + uartnetbuf->offset, copylen);
					remainlen -= (uartnetbuf->buflen - uartnetbuf->offset);
				}

				uartnetbuf->offset += copylen;	

				if (uartnetbuf->offset == uartnetbuf->buflen)
				{
					cpu_sr = tls_os_set_critical();
					dl_list_del(&uartnetbuf->list);
					tls_os_release_critical(cpu_sr);
					pbuf_free((struct pbuf *)uartnetbuf->pbuf);
					uartnetbuf->buf = NULL;
					uartnetbuf->buflen = 0;
					uartnetbuf->offset = 0;
					tls_mem_free(uartnetbuf);						
				}
				*res_len += copylen;
				cpu_sr = tls_os_set_critical();
				if (remainlen == 0)
				{
					break;
				}
			}
		    tls_os_release_critical(cpu_sr);	
#else
		    struct tls_uart_circ_buf * precvmit;
	        precvmit = tls_hostif_get_recvmit(socket);
			if(precvmit)
			{
				ret = CIRC_CNT(precvmit->head, precvmit->tail, TLS_SOCKET_RECV_BUF_SIZE);
				if(ret < maxsize)
					maxsize = ret;
			}	
			else{
				return -CMD_ERR_INV_PARAMS;
			}
			*res_len = sprintf(res_resp, "+OK=%d\r\n\r\n", maxsize);
	        
	        while(1)
			{
				ret = CIRC_CNT_TO_END(precvmit->head, precvmit->tail, TLS_SOCKET_RECV_BUF_SIZE);
				if(ret == 0)
				{
					break;
				}
				if(ret > maxsize)
				{
					ret = maxsize;
				}
				memcpy(res_resp + *res_len,(char *)(precvmit->buf+precvmit->tail),ret);
				*res_len += ret;
				precvmit->tail = (precvmit->tail + ret) & (TLS_SOCKET_RECV_BUF_SIZE - 1);
				if (maxsize > ret)
				{
					maxsize -= ret;
				}
				else
					break;
			}
#endif			
			res_resp[*res_len] = '\0';
	        return -CMD_ERR_SKT_RPT;
	    }
    }
    else if(strcmp("SKRPTM", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%d\n", cmdrsp->skrptm.mode);
	    }
    }else if(strcmp("SKSRCIP", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=%d.%d.%d.%d", cmdrsp->sksrcip.ipvalue[0], cmdrsp->sksrcip.ipvalue[1], cmdrsp->sksrcip.ipvalue[2],cmdrsp->sksrcip.ipvalue[3]);
    }else if(strcmp("SKGHBN", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=\"%d.%d.%d.%d\"", \
					cmdrsp->skghbn.h_addr_list[0], cmdrsp->skghbn.h_addr_list[1], \
					cmdrsp->skghbn.h_addr_list[2], cmdrsp->skghbn.h_addr_list[3]);
    }
#endif
#if TLS_CONFIG_HTTP_CLIENT_TASK
    else if(strcmp("HTTPC", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=%d", cmdrsp->httpc.psession);
    }
    else if(strcmp("FWUP", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=%d", cmdrsp->httpc.psession);
    }
#endif
#endif
    else if(strcmp("QVER", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=%c%x.%02x.%02x.%02x%02x,%c%x.%02x.%02x@ %s %s",
                cmdrsp->ver.hw_ver[0], cmdrsp->ver.hw_ver[1], cmdrsp->ver.hw_ver[2],
                cmdrsp->ver.hw_ver[3], cmdrsp->ver.hw_ver[4], cmdrsp->ver.hw_ver[5],
                cmdrsp->ver.fw_ver[0], cmdrsp->ver.fw_ver[1], cmdrsp->ver.fw_ver[2],
                cmdrsp->ver.fw_ver[3],SysCreatedTime, SysCreatedDate);
    }else if(strcmp("&UPDM", at_name) == 0 || strcmp("&REGW", at_name) == 0 || strcmp("&RFW", at_name) == 0 ||
             strcmp("&FLSW", at_name) == 0 || strcmp("&LPTSTR", at_name) == 0 || strcmp("&LPTSTP", at_name) == 0 ||
             strcmp("&LPRSTR", at_name) == 0 || strcmp("&LPRSTP", at_name) == 0 || strcmp("&LPPSTP", at_name) == 0 ||
             strcmp("&LPRFPS", at_name) == 0 || strcmp("&LPTBD", at_name) == 0 || strcmp("&LPSTPT", at_name) == 0 ||
             strcmp("&LPCHLR", at_name) == 0 || strcmp("&LPSTPR", at_name) == 0
             || strcmp("WIDTH", at_name) == 0
             || strcmp("&RXSIN", at_name) == 0){
        *res_len = atcmd_ok_resp(res_resp);
    }else if(strcmp("&UPDD", at_name) == 0) {
		*res_len = sprintf(res_resp, "+OK=%d", tls_fwup_get_current_update_numer());
    }else if (strcmp("&LPTPD", at_name) == 0){
    	if (set_opt)
    	{
    		*res_len = sprintf(res_resp, "+OK");
    	}
		else
    	{
    		*res_len = sprintf(res_resp, "+OK=%d", tls_get_tx_litepoint_period());
    	}
    }
	else if(strcmp("&REGR", at_name) == 0) {
        int i=0;
        *res_len = sprintf(res_resp, "+OK=%08x", cmdrsp->regr.value[0]);
        for(i=1;i<cmdrsp->regr.length;i++)
            *res_len += sprintf(res_resp + *res_len, ",%08x", cmdrsp->regr.value[i]);
    }else if(strcmp("&RFR", at_name) == 0) {
        int i=0;
        *res_len = sprintf(res_resp, "+OK=%04x", cmdrsp->rfr.value[0]);
        for(i=1;i<cmdrsp->rfr.length;i++){
            *res_len += sprintf(res_resp + *res_len, ",%04x", cmdrsp->rfr.value[i]);
        }
    }else if(strcmp("&FLSR", at_name) == 0) {
        u8 temp[16];
        int i=0;
        u8 buff[32];
        u32 len;
        u32 regv32;
        len = cmdrsp->flsr.length;
        memcpy(buff, (u8 *)&cmdrsp->flsr.value[0], 4*len);
        MEMCPY(&regv32, &buff[0], 4);
        *res_len = sprintf(res_resp, "+OK=%08x", regv32);
    	for(i = 1; i < len; i++)
    	{
    	    MEMCPY(&regv32, &buff[i * 4], 4);
    		sprintf((char *)temp, ",%08x", regv32);
    		strcat(res_resp, (char *)temp);
    		*res_len += 9;
    	}
    }else if(strcmp("&TXG", at_name) == 0) {
        if (set_opt){
            *res_len = atcmd_ok_resp(res_resp); 
        }else{
            *res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        	"%02x%02x", \
        		cmdrsp->txg.tx_gain[0], cmdrsp->txg.tx_gain[1], cmdrsp->txg.tx_gain[2], \
        		cmdrsp->txg.tx_gain[3], cmdrsp->txg.tx_gain[4], cmdrsp->txg.tx_gain[5], \
        		cmdrsp->txg.tx_gain[6], cmdrsp->txg.tx_gain[7], cmdrsp->txg.tx_gain[8], \
        		cmdrsp->txg.tx_gain[9], cmdrsp->txg.tx_gain[10], cmdrsp->txg.tx_gain[11],\
        		cmdrsp->txg.tx_gain[12], cmdrsp->txg.tx_gain[13], cmdrsp->txg.tx_gain[14],\
        		cmdrsp->txg.tx_gain[15], cmdrsp->txg.tx_gain[16], cmdrsp->txg.tx_gain[17],\
        		cmdrsp->txg.tx_gain[18], cmdrsp->txg.tx_gain[19], cmdrsp->txg.tx_gain[20],\
        		cmdrsp->txg.tx_gain[21], cmdrsp->txg.tx_gain[22], cmdrsp->txg.tx_gain[23],\
        		cmdrsp->txg.tx_gain[24], cmdrsp->txg.tx_gain[25], cmdrsp->txg.tx_gain[26],\
        		cmdrsp->txg.tx_gain[27], cmdrsp->txg.tx_gain[28],      	
        		cmdrsp->txg.tx_gain[29], cmdrsp->txg.tx_gain[30], cmdrsp->txg.tx_gain[31], \
        		cmdrsp->txg.tx_gain[32], cmdrsp->txg.tx_gain[33], cmdrsp->txg.tx_gain[34],\
        		cmdrsp->txg.tx_gain[35], cmdrsp->txg.tx_gain[36], cmdrsp->txg.tx_gain[37],\
        		cmdrsp->txg.tx_gain[38], cmdrsp->txg.tx_gain[39], cmdrsp->txg.tx_gain[40], cmdrsp->txg.tx_gain[41],\
        		cmdrsp->txg.tx_gain[42], cmdrsp->txg.tx_gain[43], cmdrsp->txg.tx_gain[44],\
        		cmdrsp->txg.tx_gain[45], cmdrsp->txg.tx_gain[46], cmdrsp->txg.tx_gain[47],\
        		cmdrsp->txg.tx_gain[48], cmdrsp->txg.tx_gain[49], cmdrsp->txg.tx_gain[50],\
        		cmdrsp->txg.tx_gain[51], cmdrsp->txg.tx_gain[52], cmdrsp->txg.tx_gain[53],\
        		cmdrsp->txg.tx_gain[54], cmdrsp->txg.tx_gain[55], cmdrsp->txg.tx_gain[56],\
        		cmdrsp->txg.tx_gain[57], cmdrsp->txg.tx_gain[58],  
        		cmdrsp->txg.tx_gain[59], cmdrsp->txg.tx_gain[60], 
        		cmdrsp->txg.tx_gain[61], cmdrsp->txg.tx_gain[62], \
        		cmdrsp->txg.tx_gain[63], cmdrsp->txg.tx_gain[64], cmdrsp->txg.tx_gain[65], \
        		cmdrsp->txg.tx_gain[66], cmdrsp->txg.tx_gain[67], cmdrsp->txg.tx_gain[68], \
        		cmdrsp->txg.tx_gain[69], cmdrsp->txg.tx_gain[70], cmdrsp->txg.tx_gain[71],\
        		cmdrsp->txg.tx_gain[72], cmdrsp->txg.tx_gain[73], cmdrsp->txg.tx_gain[74],\
        		cmdrsp->txg.tx_gain[75], cmdrsp->txg.tx_gain[76], cmdrsp->txg.tx_gain[77],\
        		cmdrsp->txg.tx_gain[78], cmdrsp->txg.tx_gain[79], cmdrsp->txg.tx_gain[80],\
        		cmdrsp->txg.tx_gain[81], cmdrsp->txg.tx_gain[82], cmdrsp->txg.tx_gain[83]);	
        }
    }else if (strcmp("&TXGI", at_name) == 0) {
        if (set_opt){
            *res_len = atcmd_ok_resp(res_resp);
        }else{
            *res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        									 "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
        									 "%02x%02x%02x%02x", \
        		cmdrsp->txg.tx_gain[0], cmdrsp->txg.tx_gain[1], cmdrsp->txg.tx_gain[2], \
        		cmdrsp->txg.tx_gain[3], cmdrsp->txg.tx_gain[4], cmdrsp->txg.tx_gain[5], \
        		cmdrsp->txg.tx_gain[6], cmdrsp->txg.tx_gain[7], cmdrsp->txg.tx_gain[8], \
        		cmdrsp->txg.tx_gain[9], cmdrsp->txg.tx_gain[10], cmdrsp->txg.tx_gain[11],\
        		cmdrsp->txg.tx_gain[12], cmdrsp->txg.tx_gain[13], cmdrsp->txg.tx_gain[14],\
        		cmdrsp->txg.tx_gain[15], cmdrsp->txg.tx_gain[16], cmdrsp->txg.tx_gain[17],\
        		cmdrsp->txg.tx_gain[18], cmdrsp->txg.tx_gain[19], cmdrsp->txg.tx_gain[20],\
        		cmdrsp->txg.tx_gain[21], cmdrsp->txg.tx_gain[22], cmdrsp->txg.tx_gain[23],\
        		cmdrsp->txg.tx_gain[24], cmdrsp->txg.tx_gain[25], cmdrsp->txg.tx_gain[26],\
        		cmdrsp->txg.tx_gain[27]);
        }
    }
	
    if (strcmp("&TXGS", at_name) == 0)
    {
       *res_len = atcmd_ok_resp(res_resp); 
    }
    if (strcmp("&TXGG", at_name) == 0)
    {
        *res_len = sprintf(res_resp, "+OK=%d,%02x%02x%02x", cmdrsp->txgr.tx_rate, cmdrsp->txgr.txr_gain[0], cmdrsp->txgr.txr_gain[1], cmdrsp->txgr.txr_gain[2] );
    }

   if(strcmp("&MAC", at_name) == 0){
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
            *res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x", 
                cmdrsp->mac.addr[0], cmdrsp->mac.addr[1], cmdrsp->mac.addr[2], 
                cmdrsp->mac.addr[3], cmdrsp->mac.addr[4], cmdrsp->mac.addr[5]); 
    }
    else if (strcmp("TXLO", at_name) == 0)
    {
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
	        *res_len = sprintf(res_resp, "+OK=%08x", cmdrsp->txLO.txlo);
    }
    else if (strcmp("TXIQ", at_name) == 0)
    {
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
	        *res_len = sprintf(res_resp, "+OK=%08x,%08x", cmdrsp->txIQ.txiqgain, cmdrsp->txIQ.txiqphase);
    }
    else if (strcmp("FREQ", at_name) == 0)
    {
        if(set_opt)
            *res_len = atcmd_ok_resp(res_resp);
        else
	        *res_len = sprintf(res_resp, "+OK=%d", cmdrsp->FreqErr.freqerr);
    }
    else if (strcmp("&CALFIN", at_name) == 0)
    {
    	if (set_opt)
    	{
    		*res_len = atcmd_ok_resp(res_resp);
    	}
		else
		{
	    	*res_len = sprintf(res_resp, "+OK=%d", cmdrsp->calfin.val);
		}
    }	
    else if(strcmp("&SPIF", at_name) == 0){
        if(cmdrsp->spif.mode==0)
            *res_len = sprintf(res_resp, "+OK=%s", cmdrsp->spif.data);
        else
            *res_len = atcmd_ok_resp(res_resp);
    }else if(strcmp("&LPCHL", at_name) == 0 || strcmp("&LPCHRS", at_name) == 0){
        if (set_opt) {
			*res_len = atcmd_ok_resp(res_resp);
		}else{
	        *res_len = sprintf(res_resp, "+OK=%d", cmdrsp->lpchl.channel);
		}
    }else if(strcmp("&LPTSTT", at_name) == 0){
	*res_len = sprintf(res_resp, "+OK=%x", tls_tx_litepoint_test_get_totalsnd());
    }else if((strcmp("&LPRSTT", at_name) == 0) || (strcmp("&LPRAGC", at_name) == 0)) {
        u32 cnt_total = 0, cnt_good = 0, cnt_bad = 0;
	tls_rx_litepoint_test_result(&cnt_total, &cnt_good, &cnt_bad);
        *res_len = sprintf(res_resp, "+OK=%x,%x,%x", cnt_total, cnt_good, cnt_bad);
    }else if(strcmp("&LPPSTR", at_name) == 0) {
        if (gulCalFlag){
	        *res_len = sprintf(res_resp, "+OK=%x", rf_spi_read(11));
	    }else
	        *res_len = atcmd_ok_resp(res_resp);
    }else if(strcmp("&LPRSR", at_name) == 0) {
        u32 rx_valid, rx_snr, rx_rcpi = 0;
	tls_rx_litepoint_pwr_result(&rx_valid, &rx_snr, &rx_rcpi);
        if (rx_valid)
    	{
    		*res_len = sprintf(res_resp, "+OK=%d,%x,%x", rx_valid, rx_rcpi, rx_snr);
    	}
    	else
    	{
    		*res_len = sprintf(res_resp, "+OK=%d", rx_valid);
    	}
    }
#if TLS_CONFIG_WIFI_PERF_TEST
    else if(strcmp("THT", at_name) == 0){
        *res_len = atcmd_ok_resp(res_resp);
    }
#endif
#if TLS_CONFIG_WPS
    else if(strcmp("WWPS", at_name) == 0){
        if(set_opt){
            if(cmdrsp->wps.result==0)
                *res_len = atcmd_ok_resp(res_resp);
            else if(cmdrsp->wps.result==1){
                *res_len = sprintf(res_resp, "+OK=");
                for(int i=0;i<WPS_PIN_LEN;i++)
                    *res_len += sprintf(res_resp + *res_len, "%c", cmdrsp->wps.pin[i]);
            }
        }
		else
		{
			*res_len = atcmd_ok_resp(res_resp);
		}
/*        else{
            if(cmdrsp->wps.result==2){
                *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->wps.mode);
                if(cmdrsp->wps.mode==1){
                    *res_len += sprintf(res_resp + *res_len, ",");
                    for(int i=0;i<8;i++)
                        res_len += sprintf(res_resp+*res_len,"%c",cmdrsp->wps.pin[i]);
                }
            }
        }*/
    }
#endif
    else if(strcmp("CUSTDATA", at_name) == 0){
        *res_len = sprintf(res_resp, "+OK=\"%s\"", cmdrsp->custdata.data);
    }
#if TLS_CONFIG_AP
    else if(strcmp("SLIST", at_name) == 0){
        if (0 == cmdrsp->stalist.sta_num)
        {
            *res_len = sprintf(res_resp, "+OK=%hhu", cmdrsp->stalist.sta_num);
        }
        else
        {
            *res_len = sprintf(res_resp, "+OK=%hhu%s", cmdrsp->stalist.sta_num, cmdrsp->stalist.data);
        }
    }
#endif
	else if(strcmp("PING", at_name) == 0)
	{
        if (cmdrsp->ping.ext)
            *res_len = atcmd_ok_resp(res_resp);
        else
            *res_len = sprintf(res_resp, "+OK=%u", cmdrsp->ping.ttl);
	}
	else if (strcmp("CPUSTA", at_name) == 0){
		*res_len = atcmd_ok_resp(res_resp);
	}
	else if (strcmp("CPUDIV", at_name) == 0){
		*res_len = sprintf(res_resp, "+OK=%hhu", cmdrsp->pass.length);
	}
    //else{
//        return -CMD_ERR_UNSUPP;
//    }
    return 0;
}

int ri_parse_func(s16 ri_cmd_id, char *buf, u32 length, union HOSTIF_CMD_PARAMS_UNION *cmd){
    if(ri_cmd_id == HOSTIF_CMD_REGR || ri_cmd_id == HOSTIF_CMD_FLSR){
        if(length > 9)
            return CMD_ERR_INV_PARAMS;
    }else if(ri_cmd_id == HOSTIF_CMD_RFR){
        if(length > 7)
            return CMD_ERR_INV_PARAMS;
    }else if(ri_cmd_id == HOSTIF_CMD_REGW || ri_cmd_id == HOSTIF_CMD_FLSW){
        if(length > 41)
            return CMD_ERR_INV_PARAMS;
    }else if(ri_cmd_id == HOSTIF_CMD_RFW){
        if(length > 23)
            return CMD_ERR_INV_PARAMS;
    }
    else if(ri_cmd_id == HOSTIF_CMD_UPDM){
        cmd->updm.src = 1;
    }
    else if(ri_cmd_id == HOSTIF_CMD_UPDD){
        cmd->updd.data[0] = 1;/* æ è¯æ¯riæä»¤ */
        //tls_set_hspi_fwup_mode(1);
    }
#if TLS_CONFIG_RI_CMD
    else if(ri_cmd_id == HOSTIF_CMD_WSCAN || ri_cmd_id == HOSTIF_CMD_WJOIN){ 
        struct tls_hostif *hif = tls_get_hostif();
        if (hif->hostif_mode == HOSTIF_MODE_HSPI)
            cmd->scanparam.mode = CMD_MODE_HSPI_RICMD;
        else
            cmd->scanparam.mode = CMD_MODE_UART1_RICMD;

		cmd->scanparam.chlist = 0;
		cmd->scanparam.scantimes = 0;
		cmd->scanparam.switchinterval = 0;
    }
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    else if(ri_cmd_id == HOSTIF_CMD_SKCT){
        u8 *p = (u8 *)buf + sizeof(struct tls_hostif_cmd_hdr);
        u32 len= length - sizeof(struct tls_hostif_cmd_hdr);
        int err = CMD_ERR_INV_PARAMS;
        u16 params;
        struct tls_cmd_socket_t socket;
        struct tls_hostif *hif = tls_get_hostif();
        memset(&socket, 0, sizeof(struct tls_cmd_socket_t));
        do{
            if(len >1){
                socket.proto = *p;
                if(socket.proto>1)
                    break;
            }else
                break;
            len -= 1;
            p += 1;

            if(len > 1){
                if(*p >1)
                    break;
                socket.client = *p ? 0 : 1;
            }else
                break;
            len -= 1;
            p += 1;

            if(len > 1){
                if(*p > 31)
                    break;
                else
                    socket.host_len = *p;
            }else
                break;
            len -= 1;
            p += 1;

            if(len > socket.host_len){
                if(socket.client == 1 && socket.host_len == 4){
                    //*(u32 *)&socket.ip_addr[0] = get_unaligned_le32(p);
                    u32 ipaddr = get_unaligned_le32(p);
                    MEMCPY(socket.ip_addr, &ipaddr, 4);
                } else if(socket.client == 1){
                    MEMCPY(socket.host_name, p, socket.host_len);
                }else if(socket.client == 0 && socket.proto == 0){
                    socket.timeout = get_unaligned_be32(p);
                }else
                    ;
            }else
                break;
            len -= socket.host_len;
            p += socket.host_len;

            if(len >= 2){
                socket.port = get_unaligned_be16(p);
            }else
                break;
            len -= 2;
            p += 2;
            
            if(len <2 && socket.port==0)
                break;
            if(len >=2){
                params = get_unaligned_be16(p);
                if((socket.proto == 0) && (socket.client == 0))
                {
                    if(params != 0)
                        socket.port = params;
                    else{
                        if(socket.port == 0)
                            break;
                    }
                }
                else{
                    if((params == 0) || (socket.port == 0))
                        break;
                }
                socket.localport = params;
            }
//            else if(socket.proto == 1 && socket.client == 1){
//                socket.localport = socket.port;
//            } 
            err = 0;
        }while(0);
        if(err){
            return CMD_ERR_INV_PARAMS;
        }
            
        if (hif->hostif_mode == HOSTIF_MODE_HSPI)
            cmd->skct.mode = CMD_MODE_HSPI_RICMD;
        else
            cmd->skct.mode = CMD_MODE_UART1_RICMD;
        cmd->skct.proto = socket.proto;
        cmd->skct.client = socket.client;
        cmd->skct.host_len = socket.host_len;
        memcpy(cmd->skct.host_name, socket.host_name, socket.host_len);
        cmd->skct.port = socket.port;
        cmd->skct.localport = socket.localport;
        memcpy(cmd->skct.ip_addr, socket.ip_addr, 4);
        cmd->skct.timeout = socket.timeout;
    }
    else if(ri_cmd_id == HOSTIF_CMD_SKGHBN){
        cmd->skghbn.ipstr[length-4] = '\0';
    }
#endif
    else if(ri_cmd_id == HOSTIF_CMD_ATRM){
        struct tls_cmd_socket_t socket;
        u8 *p = (u8 *)buf + sizeof(struct tls_hostif_cmd_hdr);
        u32 len= length - sizeof(struct tls_hostif_cmd_hdr);
        int err = CMD_ERR_INV_PARAMS;
        u16 params;
        memset(&socket, 0, sizeof(struct tls_cmd_socket_t));
        do{
            if(len >1){
                socket.proto = *p;
                if(socket.proto>1)
                    break;
            }else
                return 0;
            len -= 1;
            p += 1;

            if(len > 1){
                if(*p >1)
                    break;
                socket.client = *p ? 0 : 1;
            }else
                break;
            len -= 1;
            p += 1;

            if(len > 1){
                if(*p > 31)
                    break;
                else
                    socket.host_len = *p;
            }else
                break;
            len -= 1;
            p += 1;

            if(len > socket.host_len){
                if(socket.client == 1 && socket.host_len == 4){
                    //*(u32 *)socket.ip_addr = get_unaligned_le32(p);
                    u32 ipaddr = get_unaligned_le32(p);
                    MEMCPY(socket.ip_addr, &ipaddr, 4);
                } else if(socket.client == 1){
                    MEMCPY(socket.host_name, p, socket.host_len);
                }else if(socket.client == 0 && socket.proto == 0){
                    socket.timeout = get_unaligned_be32(p);
                }else
                    MEMCPY(socket.host_name, p, socket.host_len);
            }else
                break;
            len -= socket.host_len;
            p += socket.host_len;

            if(len >= 2){
                socket.port = get_unaligned_be16(p);
            }else
                break;
            len -= 2;
            p += 2;
            
            if(len <2 && socket.port==0)
                break;
            if(len >=2){
                params = get_unaligned_be16(p);
                if((socket.proto == 0) && (socket.client == 0))
                {
                    if(params != 0)
                        socket.port = params;
                    else{
                        if(socket.port == 0)
                            break;
                    }
                }
                else{
                    if((params == 0) || (socket.port == 0))
                        break;
                }
                socket.localport = params;
            }
//            else if(socket.proto == 1 && socket.client == 1){
//                socket.localport = socket.port;
//            } 
            err = 0;
        }while(0);
        if(err){
            return CMD_ERR_INV_PARAMS;
        }
        cmd->atrm.proto = socket.proto;
        cmd->atrm.client = socket.client;
        cmd->atrm.host_len = socket.host_len;
        memcpy(cmd->atrm.host_name, socket.host_name, socket.host_len);
        cmd->atrm.port = socket.port;
        cmd->atrm.localport = socket.localport;
        memcpy(cmd->atrm.ip_addr, socket.ip_addr, 4);
        cmd->atrm.timeout = socket.timeout;
    }
    else if(ri_cmd_id == HOSTIF_CMD_WEBS){
        if(length == 5)
            cmd->webs.portnum = 80;
    }
#if TLS_CONFIG_HTTP_CLIENT_TASK
    else if(ri_cmd_id == HOSTIF_CMD_HTTPC || ri_cmd_id == HOSTIF_CMD_FWUP){
        u8 *p = (u8 *)buf + sizeof(struct tls_hostif_cmd_hdr);
        u8 *url=NULL;
        if(cmd->httpc.url_len>255)
            return CMD_ERR_INV_PARAMS;
        if(cmd->httpc.data_len>512)
            return CMD_ERR_INV_PARAMS;
        url = (u8 *)tls_mem_alloc(sizeof(u8) * (cmd->httpc.url_len + cmd->httpc.data_len));
        if(url==NULL)
            return CMD_ERR_MEM;
        memset(url, 0, cmd->httpc.url_len + cmd->httpc.data_len);
        p += 4;
        memcpy(url, p, cmd->httpc.url_len+cmd->httpc.data_len);
        cmd->httpc.url = p+8;
        cmd->httpc.data = p + 8 + cmd->httpc.url_len + 1;
        memcpy(p+8, url, cmd->httpc.url_len);
        p[cmd->httpc.url_len + 8] = '\0';
        memcpy(p+9+cmd->httpc.url_len, url+cmd->httpc.url_len, cmd->httpc.data_len);
        p[9+cmd->httpc.url_len+cmd->httpc.data_len] = '\0';
        tls_mem_free(url);
        url=NULL;
    }	
#endif
#endif
    return 0;
}

int ri_format_func(s16 ri_cmd_id, u8 set_opt, u8 update_flash, union HOSTIF_CMDRSP_PARAMS_UNION * cmdrsp, char *res_resp, u32 *res_len){
    if(ri_cmd_id == HOSTIF_CMD_MAC
#if TLS_CONFIG_AP 
	|| ri_cmd_id == HOSTIF_CMD_AP_MAC
#endif	
		){
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 6;
    }else if(ri_cmd_id == HOSTIF_CMD_VER){
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 10;
    }
#if TLS_CONFIG_RI_CMD
    else if (ri_cmd_id == HOSTIF_CMD_PS || ri_cmd_id == HOSTIF_CMD_DBG || ri_cmd_id == HOSTIF_CMD_UPDP){
        ;
    }else if(ri_cmd_id == HOSTIF_CMD_RESET_FLASH || ri_cmd_id == HOSTIF_CMD_RESET || ri_cmd_id == HOSTIF_CMD_PMTF ||
             ri_cmd_id == HOSTIF_CMD_GPIO || ri_cmd_id == HOSTIF_CMD_WLEAVE || ri_cmd_id == HOSTIF_CMD_WSCAN ||
             ri_cmd_id == HOSTIF_CMD_AOLM || ri_cmd_id == HOSTIF_CMD_DDNS ||
             ri_cmd_id == HOSTIF_CMD_UPNP || ri_cmd_id == HOSTIF_CMD_DNAME){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x00;
    }else if(ri_cmd_id == HOSTIF_CMD_WJOIN){
        if(cmdrsp->join.result==1){
            u8 *p=cmdrsp->join.ssid + cmdrsp->join.ssid_len;
            *p = cmdrsp->join.rssi;
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 11 + cmdrsp->join.ssid_len;
        }else{
            struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
            cmd_rsp->cmd_hdr.ext=0x00;
        }
    }else if(ri_cmd_id == HOSTIF_CMD_LINK_STATUS
#if TLS_CONFIG_AP    
    || ri_cmd_id == HOSTIF_CMD_AP_LINK_STATUS
#endif    
    ){
        if (cmdrsp->lkstt.status == 1)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) +
                sizeof(struct _HOSTIF_CMDRSP_PARAMS_LKSTT); 
        else 
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1;
    }else if (ri_cmd_id == HOSTIF_CMD_SSID || ri_cmd_id == HOSTIF_CMD_DNS
#if TLS_CONFIG_AP     
    || ri_cmd_id == HOSTIF_CMD_AP_SSID
#endif    
    ){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->ssid.ssid_len;
    }else if(ri_cmd_id == HOSTIF_CMD_WPRT || ri_cmd_id == HOSTIF_CMD_ENCRYPT || ri_cmd_id == HOSTIF_CMD_BRD_SSID ||
             ri_cmd_id == HOSTIF_CMD_WATC || ri_cmd_id == HOSTIF_CMD_WPSM || ri_cmd_id == HOSTIF_CMD_WARM ||
             ri_cmd_id == HOSTIF_CMD_ATM || ri_cmd_id == HOSTIF_CMD_PORTM || ri_cmd_id == HOSTIF_CMD_ONESHOT ||
             ri_cmd_id == HOSTIF_CMD_WARC || ri_cmd_id == HOSTIF_CMD_IOM || ri_cmd_id == HOSTIF_CMD_CMDM 
#if TLS_CONFIG_AP             
           ||  ri_cmd_id == HOSTIF_CMD_AP_ENCRYPT
#endif           
             ){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1;
    }else if(ri_cmd_id == HOSTIF_CMD_KEY
#if TLS_CONFIG_AP
	|| ri_cmd_id == HOSTIF_CMD_AP_KEY
#endif	
	){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 3 + cmdrsp->key.key_len;
    }
	else if (ri_cmd_id == HOSTIF_CMD_BSSID)
	{
        if(!set_opt){
			*res_len = sizeof(struct tls_hostif_cmd_hdr) + 7;
        }
	}
	else if(ri_cmd_id == HOSTIF_CMD_UART || ri_cmd_id == HOSTIF_CMD_PASS){
        if(!set_opt){
            u8 baud_rate[3];
            memcpy(baud_rate, cmdrsp->uart.baud_rate, 3);
            cmdrsp->uart.baud_rate[0] = baud_rate[2];
            cmdrsp->uart.baud_rate[1] = baud_rate[1];
            cmdrsp->uart.baud_rate[2] = baud_rate[0];
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 7;
        }
    }else if(ri_cmd_id == HOSTIF_CMD_CHNL || ri_cmd_id == HOSTIF_CMD_WBGR 
#if TLS_CONFIG_AP
	|| ri_cmd_id == HOSTIF_CMD_AP_CHL || ri_cmd_id == HOSTIF_CMD_AP_WBGR
#endif
	) {
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 2;
    }else if(ri_cmd_id == HOSTIF_CMD_WREG || ri_cmd_id == HOSTIF_CMD_ATLT || 
             ri_cmd_id == HOSTIF_CMD_CHLL || ri_cmd_id == HOSTIF_CMD_ATPT){
        if(!set_opt){
            cmdrsp->wreg.region = host_to_be16(cmdrsp->wreg.region);
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 2;
        }
    }else if(ri_cmd_id == HOSTIF_CMD_NIP
#if TLS_CONFIG_AP
	|| ri_cmd_id == HOSTIF_CMD_AP_NIP
#endif
	){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 17;
    }else if(ri_cmd_id == HOSTIF_CMD_WEBS){
        if(!set_opt){
            cmdrsp->webs.portnum = host_to_be16(cmdrsp->webs.portnum);
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 3;
        }
            
    }else if(ri_cmd_id == HOSTIF_CMD_ATRM){
        u8 *p=(u8 *)res_resp+sizeof(struct tls_hostif_hdr) + sizeof(struct tls_hostif_cmd_hdr);
        u8  proto, cs, host_len;
        u16 port;
        char host_name[32];
        
        if(!set_opt){
            proto = cmdrsp->atrm.proto;
            cs = cmdrsp->atrm.client ? 0: 1;
            host_len = cmdrsp->atrm.host_len;
            if(cmdrsp->atrm.client && cmdrsp->atrm.host_len == 4)
                memcpy(host_name, cmdrsp->atrm.ip_addr, cmdrsp->atrm.host_len);
            else if(cmdrsp->atrm.client)
                memcpy(host_name, cmdrsp->atrm.host_name, cmdrsp->atrm.host_len);
            else if(!cmdrsp->atrm.client && cmdrsp->atrm.proto==0)
                put_unaligned_be32(cmdrsp->atrm.timeout, (u8 *)host_name);
            port = cmdrsp->atrm.port;
            
            *p = proto;
            p += 1;
            *p = cs;
            p += 1;
            *p = host_len;
            p += 1;
            memcpy(p, host_name, host_len);
            p += host_len;
            put_unaligned_be16(port, (u8 *)p);
            
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 5 + cmdrsp->atrm.host_len;
        }
    }
    else if(ri_cmd_id == HOSTIF_CMD_CNTPARAM){
        if(!set_opt){
            if(cmdrsp->cntparam_bssid_en.bssid_enable){
                *res_len = sizeof(struct tls_hostif_cmd_hdr) + 8 + cmdrsp->cntparam_bssid_en.key_len;
            }else{
                *res_len = sizeof(struct tls_hostif_cmd_hdr) + 3 + cmdrsp->cntparam_bssid_dis.ssid_len + cmdrsp->cntparam_bssid_dis.key_len;
            }
        }
    }
#if TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD
    else if(ri_cmd_id == HOSTIF_CMD_SKCT){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x01;
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1;
    }else if(ri_cmd_id == HOSTIF_CMD_SKSTT){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x01;
        for(int i=0;i<cmdrsp->skstt.number;i++){
            put_unaligned_le32(get_unaligned_le32(cmdrsp->skstt.ext[i].host_ipaddr), 
                    (u8 *)cmdrsp->skstt.ext[i].host_ipaddr);
            put_unaligned_be16(get_unaligned_le16((u8 *)&cmdrsp->skstt.ext[i].remote_port), 
                    (u8 *)&cmdrsp->skstt.ext[i].remote_port);
            put_unaligned_be16(get_unaligned_le16((u8 *)&cmdrsp->skstt.ext[i].local_port), 
                    (u8 *)&cmdrsp->skstt.ext[i].local_port);
        }
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->skstt.number * sizeof(struct hostif_cmdrsp_skstt_ext);
    }else if(ri_cmd_id == HOSTIF_CMD_SKCLOSE || ri_cmd_id ==HOSTIF_CMD_SKSDF){
        ;
    }else if(ri_cmd_id == HOSTIF_CMD_SKSRCIP){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 4;
    }else if(ri_cmd_id == HOSTIF_CMD_SKGHBN){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x01;
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 4;
    }
    
#endif
#if TLS_CONFIG_HTTP_CLIENT_TASK
    else if(ri_cmd_id == HOSTIF_CMD_HTTPC){
        ;
    }
	else if(ri_cmd_id == HOSTIF_CMD_FWUP){
        ;
    }
#endif
#endif
    else if(ri_cmd_id == HOSTIF_CMD_UPDM || ri_cmd_id == HOSTIF_CMD_UPDD ||
             ri_cmd_id == HOSTIF_CMD_REGW ||  ri_cmd_id == HOSTIF_CMD_RFW || 
             ri_cmd_id == HOSTIF_CMD_FLSW){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x00;
    }else if(ri_cmd_id == HOSTIF_CMD_REGR || ri_cmd_id == HOSTIF_CMD_FLSR){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x01;
        for(int i=0;i<cmdrsp->regr.length;i++)
            cmdrsp->regr.value[i] = host_to_be32(cmdrsp->regr.value[i]);
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->regr.length * 4;
    }else if(ri_cmd_id == HOSTIF_CMD_RFR){
        struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
        cmd_rsp->cmd_hdr.ext=0x01;
        for(int i=0;i<cmdrsp->rfr.length;i++)
            cmdrsp->rfr.value[i] = host_to_be16(cmdrsp->rfr.value[i]);
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->regr.length * 2;
    }else if(ri_cmd_id == HOSTIF_CMD_CUSTDATA){
        if(!set_opt)
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->custdata.length;
    }
#if TLS_CONFIG_WPS    
    else if(ri_cmd_id == HOSTIF_CMD_WPS){
        if(cmdrsp->wps.result == 1){
            struct tls_hostif_cmdrsp *cmd_rsp = (struct tls_hostif_cmdrsp *)res_resp;
            cmd_rsp->cmd_hdr.ext=0x01;
            memcpy((u8 *)&cmdrsp->wps.result, (u8 *)&cmdrsp->wps.pin_len, cmdrsp->wps.pin_len + 1);
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 2 + cmdrsp->custdata.length;
        }
    }
#endif
#if TLS_CONFIG_AP
    else if(ri_cmd_id == HOSTIF_CMD_STA_LIST){
        if (0 == cmdrsp->stalist.sta_num)
        {
            //*res_len = sprintf(res_resp, "+OK=%hhu", cmdrsp->stalist.sta_num);
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + 1 + cmdrsp->stalist.sta_num;
        }
        else
        {
            //*res_len = sprintf(res_resp, "+OK=%hhu,%s", cmdrsp->stalist.sta_num, cmdrsp->stalist.data);
            *res_len = sizeof(struct tls_hostif_cmd_hdr) + cmdrsp->stalist.sta_num + strlen((char *)cmdrsp->stalist.data);
        }
    }
#endif

	else if(ri_cmd_id == HOSTIF_CMD_TEM)
    {
        *res_len = sizeof(struct tls_hostif_cmd_hdr) + strlen((char *)cmdrsp->mac.addr);
    }
    else
        return CMD_ERR_UNSUPP;
    return 0;
}

int atcmd_err_resp(char *buf, int err_code)
{
    int len;
    len = sprintf(buf, "+ERR=%d", -err_code);
    return len;
}

int atcmd_ok_resp(char *buf)
{
    int len;
    len = sprintf(buf, "+OK");
    return len;
}


int atcmd_nop_proc(struct tls_atcmd_token_t *tok, 
        char *res_resp, u32 *res_len)
{
    if (!tok->arg_found && (tok->op == ATCMD_OP_NULL)) {
        *res_len = atcmd_ok_resp(res_resp);
    } else {
        *res_len = atcmd_err_resp(res_resp, CMD_ERR_OPS);
    }

    return 0; 
}

static int hostif_check_atcmd_opt(u8 op, u8 arg_found, u8 opt_flag, u8 arg_len, u8 *set_opt, u8 *update_flash){
//	int i = -1;
	u8 r = opt_flag&op;
	if( r == 0){
		return -CMD_ERR_UNSUPP;
	}
	if(r == ATCMD_OP_EQ || r == ATCMD_OP_EP){
		if(arg_len > arg_found)
			return -CMD_ERR_UNSUPP;
		*set_opt = 1;
		if(op == ATCMD_OP_EP)
			*update_flash = 1;
	}
	return 0;
}

char* get_first_comma(char* buf, int len){
	char prec='\0', curc;
	int n=0;
	if(len <= 0)
		return NULL;
	if(*buf == '"'){
		for(n=1;n<len;n++){
			curc = *(buf + n);
			if(curc == ',' && prec == '"'){
				if(n<3 || *(buf + n - 2) != '\\'){
					return buf + n;
				}
			}
			prec = curc;
		}
		return NULL;
	}
	else
		return strchr(buf, ',');
}

int tls_atcmd_parse(struct tls_atcmd_token_t *tok, char *buf, u32 len)
{
    char *c, *end_line, *comma;
    int remain_len, ret = 0;
    char *buf_start = buf;
//    int ssid_len;

    /* at command "AT+", NULL OP */
    if (len == 0) {
        *tok->name = '\0';
        tok->arg_found = 0;
        return -1;
    }
#if 0
    /* at command "+SSID" must process specially, 
     * because ssid include ASCII ',', or '?'  */
    if ((buf[0] == 'S' || buf[0] == 's') &&
            (buf[1] == 'S' || buf[1] == 's') &&
            (buf[2] == 'I' || buf[2] == 'i') &&
            (buf[3] == 'D' || buf[3] == 'd')) {
        MEMCPY(tok->name, buf, 4); 
        buf += 4;
        if (*buf != '=') {
            if (*buf == '\n') {
                *buf = '\0';
                tok->op = ATCMD_OP_NULL;
                tok->arg_found = 0;
                return 0;
            } else {
                return -CMD_ERR_INV_PARAMS;
            }
        }
        buf++;
        switch(*buf) {
            case '!':
                tok->op = ATCMD_OP_EP;
                buf++;
                break;
            case '?':
                tok->op = ATCMD_OP_QU;
                buf++;
                break;
            default:
                tok->op = ATCMD_OP_EQ;
                break;
        }
        tok->arg[0] = buf;
        c = strchr(buf, '\n');
        ssid_len = c - buf;
        if (ssid_len > 34) {
            return -CMD_ERR_INV_PARAMS;
        } else {
            if ((ssid_len == 0) && (tok->op == ATCMD_OP_QU)) {
                tok->arg_found = 0;
            } else if ((tok->op == ATCMD_OP_QU) && (ssid_len != 0)){
                return -CMD_ERR_INV_PARAMS;
            } else {
                tok->arg_found = 1;
                tok->arg[1] = c + 1; 
            }
            return 0;
        }
    }
#endif
    /* parse command name */
    c = strchr(buf, '=');
    if (!c) {
        /* format :  at+wprt */
        c = strchr(buf, '\n');
        if (!c)
            return -CMD_ERR_INV_FMT;
        if ((c - buf) > (ATCMD_NAME_MAX_LEN - 1)) 
            return -CMD_ERR_UNSUPP;
        MEMCPY(tok->name, buf, c-buf);
        *(tok->name + (c-buf)) = '\0';
        tok->op = ATCMD_OP_NULL;
        tok->arg_found = 0;
        return 0;
    } else {
        /* format : at+wprt=0 
         *          at+skct=0,0,192.168.1.4,80 */
        if ((c - buf) > (ATCMD_NAME_MAX_LEN - 1)) 
            return -CMD_ERR_UNSUPP;
        MEMCPY(tok->name, buf, c-buf);
        *(tok->name + (c-buf)) = '\0';
        tok->op = ATCMD_OP_NULL;
        buf += (c-buf + 1);
        switch(*buf) {
            case '!':
                tok->op = ATCMD_OP_EP;
                buf++;
                break;
            case '?':
                tok->op = ATCMD_OP_QU;
                buf++;
                break;
            default:
                tok->op = ATCMD_OP_EQ;
                break;
        }
        tok->arg[0]= buf;
        tok->arg_found = 0;
	if(tok->op & 0x9)
		return 0;
        remain_len = len - (buf - buf_start);
//printf("remain_len=%d\n", remain_len);
        end_line = strchr(buf, '\n');
        if (!end_line)
            return -CMD_ERR_INV_FMT;
        while (remain_len > 0) {
            comma = get_first_comma(buf, remain_len);
            if (end_line && !comma) {
                if (tok->arg_found >= (ATCMD_MAX_ARG - 1))
                    return -CMD_ERR_INV_PARAMS;
                /* last parameter */
                *(u8 *)end_line = '\0';
//                ret = atcmd_parse_parameter(&(tok->argument[tok->arg_found]), &(tok->argument_type[tok->arg_found]), (u8*)arg);
//printf("ret=%d\n", ret);
//                if(ret)
//                    return -CMD_ERR_INV_PARAMS;
                if (end_line != buf)
                    tok->arg_found++;
                //arg = end_line + 1;
                tok->arg[tok->arg_found] = end_line + 1;
                remain_len -= (end_line - buf);
                if (remain_len > 1)
                    return -CMD_ERR_NOT_ALLOW;
                else 
                    return 0;
            } else {
                if (tok->arg_found >= (ATCMD_MAX_ARG - 1)) 
                    return -CMD_ERR_INV_PARAMS;
                *(u8 *)comma = '\0';
//                ret = atcmd_parse_parameter(&(tok->argument[tok->arg_found]), &(tok->argument_type[tok->arg_found]), (u8*)arg);
                if(ret)
                    return -CMD_ERR_INV_PARAMS;
                tok->arg_found++;
                tok->arg[tok->arg_found] = comma + 1;
                remain_len -= (comma - buf + 1);
                buf = comma + 1;
            }
        } 
        return 0;
    } 
}

int tls_hostif_atcmd_exec(
        struct tls_atcmd_token_t *tok,
        char *res_rsp, u32 *res_len)
{
    int err = 0;
	struct tls_cmd_t *atcmd, *match = NULL;
	u8 set_opt=0, update_flash=0;
	union HOSTIF_CMD_PARAMS_UNION *cmd = NULL;
	union HOSTIF_CMDRSP_PARAMS_UNION *cmdrsp = NULL;

    if (strlen(tok->name) == 0) {
        err = atcmd_nop_proc(tok, res_rsp, res_len);
        return err;
    }

    /* look for AT CMD handle table */
	atcmd = at_ri_cmd_tbl;
	while (atcmd->at_name) {
		if (strcmp(atcmd->at_name, tok->name) == 0) {
			match = atcmd;
            break;
		}
		atcmd++;
	}
    /* at command handle */
    if (match) {
    cmd = tls_mem_alloc(sizeof(union HOSTIF_CMD_PARAMS_UNION));
    if (NULL == cmd)
    {
        err = -CMD_ERR_MEM;
        goto err;
    }
    cmdrsp = tls_mem_alloc(sizeof(union HOSTIF_CMDRSP_PARAMS_UNION));
    if (NULL == cmdrsp)
    {
        err = -CMD_ERR_MEM;
        goto err;
    }
    
    	err = hostif_check_atcmd_opt(tok->op, tok->arg_found, match->op_flag, match->at_arg_len, &set_opt, &update_flash);
//        printf("err1 = %d\n",err);
        if(err)
            goto err;
            //printf("set_opt=%d, update_flash=%d\n", set_opt, update_flash);
        memset(cmd, 0, sizeof(union HOSTIF_CMD_PARAMS_UNION));
    	err = at_parse_func(match->at_name, tok, cmd);
//        printf("err2 = %d\n",err);
        if(err)
    		goto err;
        memset(cmdrsp, 0, sizeof(union HOSTIF_CMDRSP_PARAMS_UNION));
        err = match->proc_func(set_opt, update_flash, cmd, cmdrsp);
//        printf("err3 = %d\n",err);
        if(err)
            goto err;
    	err = at_format_func(match->at_name, set_opt, update_flash, cmdrsp, res_rsp, res_len); 
//        printf("err4 = %d\n",err);
        if(err){
            if(err != -CMD_ERR_SKT_RPT){
                goto err;
            }
        }
     if (NULL != cmd)
        tls_mem_free(cmd);
     if (NULL != cmdrsp)
        tls_mem_free(cmdrsp);
    	 return err;
    }else
    {
        err = -CMD_ERR_UNSUPP;
    }
err:
    /* at command not found */
    *res_len = sprintf(res_rsp, "+ERR=%d", err); 
    if (NULL != cmd)
        tls_mem_free(cmd);
    if (NULL != cmdrsp)
        tls_mem_free(cmdrsp);
    return err;
}

int ricmd_default_proc(
        char *buf, u32 length, int err,
        char *cmdrsp_buf, u32 *cmdrsp_size)
{
#if TLS_CONFIG_HOSTIF
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;

    /* if cmd is not suppost, return this cmd and err code  */
    tls_hostif_fill_cmdrsp_hdr(cmdrsp, cmd->cmd_hdr.code, err, ((err || (cmd->cmd_hdr.ext & 0x1))? 0 : 1));
    *cmdrsp_size = sizeof(struct tls_hostif_cmd_hdr);
#endif
    return 0;
}

int tls_hostif_ricmd_exec(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size)
{
    struct tls_hostif_cmd *cmd = (struct tls_hostif_cmd *)buf;
    struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    //struct tls_hostif_cmdrsp *cmdrsp = (struct tls_hostif_cmdrsp *)cmdrsp_buf;
    int err = 0;
    struct tls_cmd_t * match = NULL;
    int cmdcnt = sizeof(at_ri_cmd_tbl)/ sizeof(struct tls_cmd_t);
	int i = 0, set_opt=0, update_flash = 0;
    
    //TLS_DBGPRT_INFO("========>\n");

    int cmd_code = cmd->cmd_hdr.code;
    
 	/*find cmdId*/
    if(cmd_code == 0){
        cmd->cmd_hdr.ext = 1;
        goto erred;
    }
	for (i = 0; i< cmdcnt; i++){
		if (cmd_code == at_ri_cmd_tbl[i].ri_cmd_id){
			match = &at_ri_cmd_tbl[i];
			break;
		}
	}
	if (match){
		if(cmd->cmd_hdr.ext & 0x2){
			if((cmd->cmd_hdr.ext & 0x1) == 0 || (match->op_flag & 0x40) == 0){
				err = CMD_ERR_INV_PARAMS;
//				printf("errA=%d\n",err);
                goto erred;
			}
			update_flash = 1;
			set_opt = 1;
		}
		else if(cmd->cmd_hdr.ext & 0x1){
			if((match->op_flag & 0x20) == 0){
				err = CMD_ERR_INV_PARAMS;
//				printf("errB=%d\n",err);
                goto erred;
			}
			set_opt = 1;
		}
		else if((match->op_flag & 0x10) == 0){
			err = CMD_ERR_INV_PARAMS;
//			printf("errC=%d\n",err);
            goto erred;
		}
		if ((cmd->cmd_hdr.msg_type != 0x01) || 
		      ((set_opt == 0) && (length != sizeof(struct tls_hostif_cmd_hdr))) ||
		      ((set_opt == 1) && (length < sizeof(struct tls_hostif_cmd_hdr) + match->ri_set_len))) {
			err = CMD_ERR_INV_PARAMS;
//			printf("errD=%d\n",err);
            goto erred;
		}
		err = ri_parse_func(cmd_code, buf, length, &cmd->params);        
		if(err){
//			printf("err1=%d\n",err);
            goto erred;
		}
		err = match->proc_func(set_opt, update_flash, &cmd->params, &cmdrsp->params);		
        if(err){
            err = -err;
//			printf("err2=%d\n",err);
            goto erred;
		}
		ricmd_default_proc(buf, length, err, cmdrsp_buf, cmdrsp_size);
		err = ri_format_func(cmd_code, set_opt, update_flash, &cmdrsp->params, cmdrsp_buf, cmdrsp_size);		
        if(err){
//			printf("err3=%d\n",err);
            goto erred;
		}
		return err;
	}
	else
		err = CMD_ERR_OPS;
erred:
	ricmd_default_proc(buf, length, err, cmdrsp_buf, cmdrsp_size);
	return err;
}

#endif /*TLS_CONFIG_HOSTIF*/
