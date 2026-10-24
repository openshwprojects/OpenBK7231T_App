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

#include "obk_config.h"

#if ENABLE_PING_WATCHDOG

#ifdef PLATFORM_RTL8196C

#include "new_common.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "cmnds/cmd_public.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "net.h"
#include "timer.h"
#include "rtos.h"

#define PING_ID_WDOG 0xAFAFu
#define PING_ID_CMD  0xB0B0u

static uint8_t  wdog_target_ip[4];
static uint16_t wdog_seq = 0;
static uint32_t wdog_send_ms = 0;
static unsigned int ping_lost = 0;
static unsigned int ping_received = 0;
static int bReceivedLastOneSend = -1;
static int ping_handler_active = 0;
static uint32_t wdog_last_tick_ms = 0;

static volatile int      cmd_ping_got = 0;
static volatile uint32_t cmd_ping_rtt_ms = 0;
static volatile uint16_t cmd_ping_expect_seq = 0;
static uint32_t          cmd_ping_send_ms = 0;

static void rtl8196c_on_icmp_reply(const uint8_t src_ip[4], uint16_t id, uint16_t seq, uint32_t len)
{
    (void)src_ip;
    (void)len;
    uint32_t now = timer_ms();
    if (id == PING_ID_WDOG && seq == wdog_seq) {
        uint32_t ms = now - wdog_send_ms;
        ping_received++;
        bReceivedLastOneSend = 1;
        Main_OnPingCheckerReply((int)ms);
    } else if (id == PING_ID_CMD && seq == cmd_ping_expect_seq) {
        cmd_ping_rtt_ms = now - cmd_ping_send_ms;
        cmd_ping_got = 1;
    }
}

int PING_getPingIntervalMS(void)
{
    int ret = CFG_GetPingIntervalSeconds();
    if (ret < 1)
        ret = 1;
    return ret * 1000;
}

int PingWatchDog_GetTotalLost(void)
{
    return (int)ping_lost;
}

int PingWatchDog_GetTotalReceived(void)
{
    return (int)ping_received;
}

void Main_SetupPingWatchDog(const char *target)
{
    bReceivedLastOneSend = -1;
    if (!target || !*target)
        return;
    uint32_t addr = inet_addr(target);
    if (addr == INADDR_NONE)
        return;
    memcpy(wdog_target_ip, &addr, 4);
    net_icmp_set_reply_handler(rtl8196c_on_icmp_reply);
    wdog_last_tick_ms = timer_ms();
    ping_handler_active = 1;
}

void RTL8196C_PingWatchDog_Poll(void)
{
    if (!ping_handler_active)
        return;
    uint32_t now = timer_ms();
    if ((uint32_t)(now - wdog_last_tick_ms) < (uint32_t)PING_getPingIntervalMS())
        return;
    wdog_last_tick_ms = now;
    if (bReceivedLastOneSend == 0)
        ping_lost++;
    bReceivedLastOneSend = 0;
    wdog_seq++;
    wdog_send_ms = timer_ms();
    net_icmp_send_echo(wdog_target_ip, PING_ID_WDOG, wdog_seq, 32);
}

static commandResult_t CMD_RTL8196C_Ping(const void *context, const char *cmd, const char *args, int cmdFlags)
{
    (void)context; (void)cmd; (void)cmdFlags;
    Tokenizer_TokenizeString(args, 0);
    if (Tokenizer_GetArgsCount() < 1) {
        addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Usage: ping <host> [count]");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    const char *host = Tokenizer_GetArg(0);
    int count = Tokenizer_GetArgIntegerDefault(1, 4);
    if (count < 1) count = 1;
    if (count > 10) count = 10;

    uint32_t addr = inet_addr(host);
    if (addr == INADDR_NONE) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "ping: failed to resolve %s", host);
        return CMD_RES_BAD_ARGUMENT;
    }
    uint8_t dip[4];
    memcpy(dip, &addr, 4);
    net_icmp_set_reply_handler(rtl8196c_on_icmp_reply);

    int ok = 0;
    uint32_t sum_ms = 0;
    for (int i = 1; i <= count; i++) {
        cmd_ping_got = 0;
        cmd_ping_expect_seq = (uint16_t)i;
        cmd_ping_send_ms = timer_ms();
        net_icmp_send_echo(dip, PING_ID_CMD, (uint16_t)i, 32);
        while (!cmd_ping_got && (uint32_t)(timer_ms() - cmd_ping_send_ms) < 1000u) {
            net_poll();
            rtos_delay_milliseconds(1);
        }
        if (cmd_ping_got) {
            ok++;
            sum_ms += cmd_ping_rtt_ms;
            addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Reply from %u.%u.%u.%u (%s): seq=%d time=%ums",
                      dip[0], dip[1], dip[2], dip[3], host, i, (unsigned)cmd_ping_rtt_ms);
        } else {
            addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Request timed out for %u.%u.%u.%u (%s): seq=%d",
                      dip[0], dip[1], dip[2], dip[3], host, i);
        }
    }
    addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Ping %s (%u.%u.%u.%u): sent=%d received=%d lost=%d avg=%ums",
              host, dip[0], dip[1], dip[2], dip[3], count, ok, count - ok, ok ? (unsigned)(sum_ms / (uint32_t)ok) : 0u);
    return CMD_RES_OK;
}

void RTL8196C_Ping_RegisterCommands(void)
{
    net_icmp_set_reply_handler(rtl8196c_on_icmp_reply);
    CMD_RegisterCommand("ping", CMD_RTL8196C_Ping, NULL);
}

#else /* !PLATFORM_RTL8196C */

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#ifdef PLATFORM_XR809

#else
#include "lwip/timeouts.h"
#endif
#include "lwip/inet_chksum.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "logging/logging.h"
#include "new_common.h"
#include "new_cfg.h"
#include <string.h>

#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
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
static int bReceivedLastOneSend = -1;
static bool ping_handler_active = false;
//static bool ping_handler_silent = false;

//static int g_delayBetweenPings_MS = 1000 / portTICK_PERIOD_MS;

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
		//addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping lost: (total lost %i, recv %i)",ping_lost,ping_received);
		ping_lost++;

	}
	bReceivedLastOneSend = 0;

    raw_sendto(raw, p, addr);

    ping_time = sys_now();

  }
  pbuf_free(p);
}
int PING_getPingIntervalMS() {
	int ret;

	ret = CFG_GetPingIntervalSeconds();

	if (ret < 1) {
		ret = 1;
	}
	return ret * 1000;
}
static void ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;

 // if (ping_handler_silent == false)
  {
    LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

    ping_send(pcb, &ping_target);
  }
  // void 	sys_timeout (u32_t msecs, sys_timeout_handler handler, void *arg)
  sys_timeout(PING_getPingIntervalMS(), ping_timeout, pcb);
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

	//addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping recv: %ims (total lost %i, recv %i)", ms,ping_lost,ping_received);

	//  addLogAdv(LOG_INFO,LOG_FEATURE_MAIN,"Ping recv: %ims", ms);
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

int PingWatchDog_GetTotalLost() {
	return ping_lost;
}
int PingWatchDog_GetTotalReceived() {
	return ping_received;
}

void Main_SetupPingWatchDog(const char *target/*, int delayBetweenPings_Seconds*/) 
{
	// none sent yet.
	bReceivedLastOneSend = -1;
    if (ping_handler_active == false)
    {
    	//g_delayBetweenPings_MS = delayBetweenPings_Seconds * 1000;
        ///ipaddr_aton("192.168.0.1",&ping_target)
        //ipaddr_aton("8.8.8.8",&ping_target);
        ipaddr_aton(target,&ping_target);

        ping_pcb = raw_new(IP_PROTO_ICMP);
        LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

        raw_recv(ping_pcb, ping_recv, NULL);
        raw_bind(ping_pcb, IP_ADDR_ANY);
	    // void 	sys_timeout (u32_t msecs, sys_timeout_handler handler, void *arg)
        sys_timeout(PING_getPingIntervalMS(), ping_timeout, ping_pcb);
		ping_handler_active = true;
    }

}

#endif /* !PLATFORM_RTL8196C */

#endif // ENABLE_PING_WATCHDOG
