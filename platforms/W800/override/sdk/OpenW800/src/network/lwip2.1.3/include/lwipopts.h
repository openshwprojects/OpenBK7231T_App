

#ifndef __LWIP_OPTS_H
#define __LWIP_OPTS_H

#include <stdlib.h>
#include "wm_config.h"
#include "wm_mem.h"


//#include "wm_sockets.h"
/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEM_LIBC_MALLOC                 1
#define MEMP_MEM_MALLOC                 1
#define MEM_USE_POOLS                   0
#define MEMP_USE_CUSTOM_POOLS           0
#define MEMP_OVERFLOW_CHECK             2


#define LWIP_COMPAT_SOCKETS             2

#define LWIP_SOCKET                       (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)
#define LWIP_NETCONN                    TLS_CONFIG_SOCKET_STD
#define LWIP_TCP                        (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)
#define LWIP_UDP                        (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)
#define LWIP_ARP                        (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)

#define MEM_SIZE                        30000

/**
 * TCP_WND: The size of a TCP window.  This must be at least 
 * (2 * TCP_MSS) for things to work well
 */
#define TCP_WND                         (8* TCP_MSS)

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#define TCP_MSS                   1460
//#define TCP_MSS                 (1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#define TCP_SND_BUF             (15*TCP_MSS)

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
//#define TCP_SND_QUEUELEN                ((2 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_SND_QUEUELEN                30 

#define TCP_MSL 2000UL /* The maximum segment lifetime in milliseconds */

#define MEMP_NUM_TCP_SEG                32

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#define TCPIP_MBOX_SIZE               64 

/**
 * LWIP_TCPIP_CORE_LOCKING: (EXPERIMENTAL!)
 * Don't use it if you're not an active lwIP project member
 */
#define LWIP_TCPIP_CORE_LOCKING         0

/**
 * LWIP_TCPIP_CORE_LOCKING_INPUT: (EXPERIMENTAL!)
 * Don't use it if you're not an active lwIP project member
 */
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0

#define LWIP_DHCP                       (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)

#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0

#define LWIP_NETIF_API                    (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)
/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 */
#define MEMP_NUM_NETCONN                8
#if 0

#define TCP_DEBUG                       LWIP_DBG_ON
//#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#define TCP_CWND_DEBUG                  LWIP_DBG_ON

#define TCP_WND_DEBUG                   LWIP_DBG_ON
//#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#define TCP_QLEN_DEBUG                  LWIP_DBG_ON
#define TCP_RTO_DEBUG                   LWIP_DBG_ON

#endif

#define LWIP_RAW                        1
#define LWIP_IGMP                       TLS_CONFIG_IGMP
#define LWIP_RAND                   rand
#define LWIP_SO_RCVTIMEO         1
#define LWIP_SO_RCVBUF 			1
#define LWIP_SO_SNDTIMEO                1

#define DHCP_DOES_ARP_CHECK     0

#define LWIP_IPV4    TLS_CONFIG_IPV4
#define LWIP_IPV6    TLS_CONFIG_IPV6
#define LWIP_DNS      (TLS_CONFIG_SOCKET_RAW || TLS_CONFIG_SOCKET_STD)
#define LWIP_NETIF_STATUS_CALLBACK      1
#define TCP_LISTEN_BACKLOG              1
#define TCP_DEFAULT_LISTEN_BACKLOG      8
#define SO_REUSE                        1
#define SO_REUSE_RXTOALL                1
#define LWIP_ND6_MAX_MULTICAST_SOLICIT  10

#define LWIP_HAVE_LOOPIF 1
#define ETHARP_SUPPORT_STATIC_ENTRIES   1
#define ETHARP_TABLE_MATCH_NETIF        0
#define ARP_QUEUEING					1
#define TCPIP_THREAD_STACKSIZE          1000
#define MEM_ALIGNMENT                   4
#define LWIP_TCPIP_TIMEOUT              1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_TCP_KEEPALIVE              1



#define LWIP_NETCONN_SEM_PER_THREAD     1
#define LWIP_NETCONN_FULLDUPLEX 1

#if LWIP_NETCONN_SEM_PER_THREAD
#include "wm_osal.h"
#include "arch/sys_arch.h"
extern sys_sem_t* sys_lwip_netconn_thread_sem_get();

#define LWIP_NETCONN_THREAD_SEM_ALLOC()  
#define LWIP_NETCONN_THREAD_SEM_GET()     sys_lwip_netconn_thread_sem_get()
#define LWIP_NETCONN_THREAD_SEM_FREE()  do { } while(0)
#endif

#define LWIP_NO_INTTYPES_H         1
#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_SOCKET_OFFSET              1
#include "lwip/ip4_addr.h"

extern struct netif *wm_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src);
#define LWIP_HOOK_IP4_ROUTE_SRC         wm_ip4_route_src


#endif /* end of __LWIP_OPTS_H */
