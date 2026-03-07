#ifndef OBK_RTL87X0C_LWIPOPTS_WRAPPER_H
#define OBK_RTL87X0C_LWIPOPTS_WRAPPER_H

/*
 * Pull in SDK defaults first, then override only the knobs required for
 * stable mDNS operation on RTL87X0C.
 */
#include_next "lwipopts.h"

#undef TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE 1024

#undef DEFAULT_THREAD_STACKSIZE
#define DEFAULT_THREAD_STACKSIZE 512

#ifdef LWIP_NUM_NETIF_CLIENT_DATA
#if (LWIP_NUM_NETIF_CLIENT_DATA < 1)
#undef LWIP_NUM_NETIF_CLIENT_DATA
#define LWIP_NUM_NETIF_CLIENT_DATA 1
#endif
#else
#define LWIP_NUM_NETIF_CLIENT_DATA 1
#endif

#endif
