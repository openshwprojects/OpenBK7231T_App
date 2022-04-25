/*
 *  ping.c
 *
 *  Created on: 13 may. 2019
 *  Author: RICHARD
 */
//
//	TODO: convert it to structure-based code instead of using global variables
// so we can have multiple ping instances running.
// One ping for "ping watchdog", second ran from console by user, etc etc
//

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#ifdef PLATFORM_XR809

#else
#include "lwip/timeouts.h"
#include "lwip/prot/ip4.h"
#endif
#include "lwip/inet_chksum.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "logging/logging.h"
#include "new_common.h"
#include <string.h>

#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif


/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

static unsigned int ping_time;
static ip_addr_t ping_target;
static unsigned short ping_seq_num;
static struct raw_pcb *ping_pcb;
static unsigned int ping_lost = 0;
static unsigned int ping_received = 0;
static int bReceivedLastOneSend = 0;

static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = lwip_htons(++ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}

static void ping_send(struct raw_pcb *raw, const ip_addr_t *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  //LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
  //ip_addr_debug_print(PING_DEBUG, addr);
  //LWIP_DEBUGF( PING_DEBUG, ("\n"));
  LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return;
  }
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ping_prepare_echo(iecho, (u16_t)ping_size);

	if(bReceivedLastOneSend == 0) {
		addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping lost: (total lost %i, recv %i)\r\n",ping_lost,ping_received);
		ping_lost++;
	}
	bReceivedLastOneSend = 0;

    raw_sendto(raw, p, addr);

    ping_time = sys_now();

  }
  pbuf_free(p);
}

static void ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;

  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  ping_send(pcb, &ping_target);

	// void 	sys_timeout (u32_t msecs, sys_timeout_handler handler, void *arg)
  sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
	unsigned int ms;
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
      pbuf_header(p, -PBUF_IP_HLEN) == 0) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
    //  LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
    // ip_addr_debug_print(PING_DEBUG, addr);
	  ms = (sys_now()-ping_time);
    //  LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", ms));
	  ping_received++;
	bReceivedLastOneSend = 1;
	 addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping recv: %ims (total lost %i, recv %i)\r\n", ms,ping_lost,ping_received);
	//  addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping recv: %ims\r\n", ms);
	Main_OnPingCheckerReply(ms);
      PING_RESULT(1);
      pbuf_free(p);
      return 1; /* eat the packet */
    }
    /* not eaten, restore original packet */
    pbuf_header(p, PBUF_IP_HLEN);
  }

  return 0; /* don't eat the packet */
}


void Main_SetupPingWatchDog(const char *target, int delayBetweenPings) {

    ///ipaddr_aton("192.168.0.1",&ping_target)
    //ipaddr_aton("8.8.8.8",&ping_target);
    ipaddr_aton(target,&ping_target);

    ping_pcb = raw_new(IP_PROTO_ICMP);
    LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
	// void 	sys_timeout (u32_t msecs, sys_timeout_handler handler, void *arg)
    sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}