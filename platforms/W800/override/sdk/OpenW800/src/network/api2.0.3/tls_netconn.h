/**************************************************************************
 * File Name                   : tls_netconn.h
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef TLS_NETCONN_H
#define TLS_NETCONN_H

#include "tls_common.h"
#include "lwip/ip_addr.h"
#include "list.h"
#include "lwip/err.h"
#include "wm_socket.h"
#include "arch/sys_arch.h"
#include "lwip/pbuf.h"
#include "wm_netif.h"

#define TLS_NETCONN_TCP       0
#define TLS_NETCONN_UDP       1

#define RAW_SOCKET_USE_CUSTOM_PBUF	LWIP_SUPPORT_CUSTOM_PBUF

#define CONN_SEM_NOT_FREE       1

/** A netconn descriptor */
struct tls_netconn {
    struct dl_list list;
    struct tls_socket_desc *skd;
    u8        skt_num;
    bool      used;

    /* client or server mode */
    bool      client;
    ip_addr_t addr;
    u16       port;
    u16       localport;
    u16       proto;
    /* the lwIP internal protocol control block */
    union {
        struct ip_pcb  *ip;
        struct tcp_pcb *tcp;
        struct udp_pcb *udp;
        struct raw_pcb *raw;
    } pcb;
    bool   write_state;
    u16    write_offset;
    struct tls_net_msg *current_msg;
    u8     tcp_connect_cnt;
    u8     state;
    err_t  last_err; 
#if !CONN_SEM_NOT_FREE
	sys_sem_t op_completed;
#endif
	u32 idle_time;
	
};

struct tls_net_msg {
    struct dl_list list;
    //struct tls_netconn *conn;
    void *dataptr;
    struct pbuf *p;
    ip_addr_t addr;
    u16       port;
    u32   len;
    u32   write_offset;
    err_t err;
	int skt_no;
};
#if (RAW_SOCKET_USE_CUSTOM_PBUF)
struct raw_sk_pbuf_custom{
  /** 'base class' */
  struct pbuf_custom pc;
  /** pointer to the original pbuf that is referenced */
  struct pbuf *original;
  /*custom pbuf free function used*/
  void *conn;
  void *pcb;
};
#endif

int tls_net_init(void);
struct tls_netconn * tls_net_get_socket(u8 socket);

#endif /* end of TLS_NETCONN_H */

