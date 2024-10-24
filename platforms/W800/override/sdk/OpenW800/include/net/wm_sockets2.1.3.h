/**
 * @file    wm_sockets2.1.3.h
 *
 * @brief   sockets2.1.3 apis
 *
 * @author  winnermicro
 *
 * @copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_SOCKET_API2_1_3_H
#define WM_SOCKET_API2_1_3_H

#include <stdio.h>
#include "wm_type_def.h"
#include "wm_config.h"
#include <time.h>

#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

extern void print_ipaddr(ip_addr_t *ip);
extern struct netif *wm_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src);

#endif

