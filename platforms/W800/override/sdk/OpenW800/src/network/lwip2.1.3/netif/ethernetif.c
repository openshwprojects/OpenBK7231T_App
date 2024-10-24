/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include <string.h>
#include "lwip/opt.h"

#if 1 /* don't build, this is only a skeleton, see previous comment */

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "wm_mem.h"
#include "lwip/igmp.h"
#include "lwip/mld6.h"
#include "tls_common.h"
#if TLS_CONFIG_AP_OPT_FWD
#include "lwip/tcpip.h"
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

/** Maximum transfer unit */
#define NET_MTU  1500

/** Network link speed */
#define NET_LINK_SPEED  100000000

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
};

/* Forward declarations. */
int ethernetif_input(const u8 *bssid, u8 *buf, u32 buf_len);
extern struct netif *tls_get_netif(void);
extern u8* tls_wifi_buffer_acquire(int total_len);
extern void tls_wifi_buffer_release(bool is_apsta, u8* buffer);
extern u8 *wpa_supplicant_get_mac(void);
extern int tls_hw_set_multicast_key(u8* addr);
extern int tls_hw_del_multicast_key(u8* addr);
#if TLS_CONFIG_AP
extern u8 *hostapd_get_mac(void);
#endif

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{
    u8* mac_addr;

    /* Set MAC hardware address length */
    netif->hwaddr_len = ETH_ALEN;

#if TLS_CONFIG_AP
    if (netif != tls_get_netif())
    {
        mac_addr = hostapd_get_mac();
        /* Set MAC hardware address */
        MEMCPY(&netif->hwaddr[0], mac_addr, ETH_ALEN);
    }
    else
#endif
    {
        mac_addr = wpa_supplicant_get_mac();
        /* Set MAC hardware address */
        MEMCPY(&netif->hwaddr[0], mac_addr, ETH_ALEN);
    }

    /* Maximum transfer unit */
    netif->mtu = NET_MTU;
 	
#if  LWIP_IPV6_AUTOCONFIG  	
		netif_set_ip6_autoconfig_enabled(netif, 1);
#endif
    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
        NETIF_FLAG_IGMP  
#if defined(DHCP_USED)
        | NETIF_FLAG_DHCP
#endif
        ;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  /* Do whatever else is needed to initialize interface. */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q = NULL;
	int datalen = 0;
	u8* buf = tls_wifi_buffer_acquire(p->tot_len);
	if(buf == NULL)
		return ERR_MEM;

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE);    /* Drop the padding word */
#endif

	/* Check the buffer boundary */
	//if (p->tot_len > NET_RW_BUFF_SIZE) {
	//	return ERR_BUF;
	//}
	
	for (q = p; q != NULL; q = q->next) {
		/* Send data from(q->payload, q->len); */
		MEMCPY(buf + datalen, q->payload, q->len);
		datalen += q->len;
	}

#if TLS_CONFIG_AP
    if (netif != tls_get_netif())
	    tls_wifi_buffer_release(true, buf);
	else
#endif
	    tls_wifi_buffer_release(false, buf);

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE);    /* Reclaim the padding word */
#endif

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *low_level_input(struct netif *netif, u8 *buf, u32 buf_len)
{
    struct pbuf *p = NULL, *q = NULL;
    u16_t s_len;
    u8_t *bufptr;

    /* Obtain the size of the packet and put it into the "len"
     * variable. */

    s_len = buf_len; 
    bufptr = buf;

#if ETH_PAD_SIZE
    s_len += ETH_PAD_SIZE;    /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, s_len, PBUF_RAM);

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE);  /* drop the padding word */
#endif

        /* Iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
        for (q = p; q != NULL; q = q->next) {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable. */
            /* read data into(q->payload, q->len); */
            MEMCPY(q->payload, bufptr, q->len);
            bufptr += q->len;
        }
        /* Acknowledge that packet has been read(); */

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE);    /* Reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);
    } else {
        /* Drop packet(); */
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

    return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
int ethernetif_input(const u8 *bssid, u8 *buf, u32 buf_len)
{
    struct netif    *netif = tls_get_netif();
    struct pbuf       *p;

#if TLS_CONFIG_AP
    u8* mac_addr = hostapd_get_mac();
    if (0 == compare_ether_addr(bssid, mac_addr))
    {
        netif = netif->next;
    }
#endif

    /* move received packet into a new pbuf */
    p = low_level_input(netif, buf, buf_len);
    if (p) {
        if (ERR_OK != netif->input(p, netif)) {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
            p = NULL;
        }
        return 0;
    } else {
        return -1;
    }
}

#if LWIP_IGMP
/***
	action:
	IGMP_DEL_MAC_FILTER		0
	IGMP_ADD_MAC_FILTER		1
***/

static err_t ethernetif_igmp_mac_filter(struct netif *netif,
      const ip4_addr_t *group, enum netif_mac_filter_action action)
{
	u8 m_addr[6] = {0x01, 0x00, 0x5E};
	int ret = ERR_OK;
	if (netif != tls_get_netif())
	{
		return ERR_OK;
	}
	LWIP_DEBUGF(IGMP_DEBUG, ("IPaddr: %d.%d.%d.%d\n", ip4_addr1(group), 
		ip4_addr2(group),
		ip4_addr3(group),
		ip4_addr4(group)));
	
	//create group mac address:
	memcpy(m_addr+3, (u8*)&(group->addr)+1, 3);
	m_addr[3] &= 0x7F; //clear bit24

	LWIP_DEBUGF(IGMP_DEBUG, ("MACaddr: %x:%x:%x:%x:%x:%x\n",m_addr[0],
			m_addr[1],
			m_addr[2],
			m_addr[3],
			m_addr[4],
			m_addr[5]));
	
	if(action == IGMP_ADD_MAC_FILTER)
	{
		ret = tls_hw_set_multicast_key(m_addr);
	}
	else if(action == IGMP_DEL_MAC_FILTER)
	{
		ret = tls_hw_del_multicast_key(m_addr);
	}
	if (ret < 0)
		return ret;
	else
		return ERR_OK;
}
#endif

static char host_name[64] = {0};
/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
    u8* mac_addr = wpa_supplicant_get_mac();
#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
    sprintf(host_name, "WinnerMicro_%02X%02X", mac_addr[4], mac_addr[5]);
	netif->hostname = host_name;
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, NET_LINK_SPEED);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  
#if TLS_CONFIG_AP_OPT_FWD
    netif->ipfwd_output = tcpip_output;
#endif
#if  LWIP_IGMP
	netif->igmp_mac_filter = ethernetif_igmp_mac_filter;
#endif

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

#endif /* 0 */
