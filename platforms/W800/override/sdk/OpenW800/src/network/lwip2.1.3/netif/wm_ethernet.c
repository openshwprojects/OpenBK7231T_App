/**
 * @file
 * Ethernet common functions
 * 
 * @defgroup ethernet Ethernet
 * @ingroup callbackstyle_api
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
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
 */

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/netifapi.h"
#include "netif/ethernetif.h"
#include "netif/ethernet.h"
#include "wm_params.h"
#include "wm_mem.h"
#include <string.h>
#if TLS_CONFIG_AP
#include "dhcp_server.h"
#include "dns_server.h"
#include "lwip/alg.h"
#include "tls_wireless.h"
#endif
#if TLS_CONFIG_SOCKET_RAW
#include "tls_netconn.h"
#endif
#if TLS_CONFIG_RMMS
#include "tls_sys.h"
#include "wm_rmms.h"
#endif
#include "wm_wifi.h"
//extern int tls_wifi_get_oneshot_flag(void);
extern int tls_dhcp_get_ip_timeout_flag(void);
static struct tls_ethif *ethif = NULL;
static struct netif *nif = NULL;
static struct tls_netif_status_event netif_status_event;
static void netif_status_changed(struct netif *netif, u8_t netif_state)
{
	struct tls_netif_status_event *status_event;

	if (netif_is_up(netif))
	{
		dl_list_for_each(status_event, &netif_status_event.list, struct tls_netif_status_event, list)
		{
			if(status_event->status_callback != NULL)
			{
				//if (0 == tls_wifi_get_oneshot_flag())
				{
					if (ip_addr_isany(netif_ip4_addr(netif)))
					{
						if (tls_dhcp_get_ip_timeout_flag())
						{
							status_event->status_callback(NETIF_WIFI_JOIN_FAILED);
						}
					}
					else
					{
						if(netif_state & NETIF_STATE_IPV4)
						    status_event->status_callback(NETIF_IP_NET_UP);
						else if(netif_state & NETIF_STATE_IPV6)
						    status_event->status_callback(NETIF_IPV6_NET_UP);
					}
				}
			}
		}
	}
}

#if TLS_CONFIG_AP
static struct tls_ethif *ethif2 = NULL;
static void netif_status_changed2(struct netif *netif, u8_t netif_state)
{
    struct tls_netif_status_event *status_event;

    if (netif_is_up(netif))
    {
        dl_list_for_each(status_event, &netif_status_event.list, struct tls_netif_status_event, list)
        {
            if(status_event->status_callback)
            {
           	if(netif_state & NETIF_STATE_IPV4)
           		status_event->status_callback(NETIF_IP_NET2_UP);
		else if(netif_state & NETIF_STATE_IPV6)
               	status_event->status_callback(NETIF_IPV6_NET_UP);
            }
        }
    }
}
#endif

static void wifi_status_changed(u8 status)
{
    struct tls_netif_status_event *status_event;
    dl_list_for_each(status_event, &netif_status_event.list, struct tls_netif_status_event, list)
    {
        if(status_event->status_callback != NULL)
        {
            switch(status)
            {
                case WIFI_JOIN_SUCCESS:             
                    status_event->status_callback(NETIF_WIFI_JOIN_SUCCESS);
                    break;
                case WIFI_JOIN_FAILED:
                    status_event->status_callback(NETIF_WIFI_JOIN_FAILED);
                    break;
                case WIFI_DISCONNECTED:
                    status_event->status_callback(NETIF_WIFI_DISCONNECTED);
                    break;
#if TLS_CONFIG_AP
                case WIFI_SOFTAP_SUCCESS:
                    status_event->status_callback(NETIF_WIFI_SOFTAP_SUCCESS);
                    break;
                case WIFI_SOFTAP_FAILED:
                    status_event->status_callback(NETIF_WIFI_SOFTAP_FAILED);
                    break;
                case WIFI_SOFTAP_CLOSED:
                    status_event->status_callback(NETIF_WIFI_SOFTAP_CLOSED);
                    break;
#endif
                default:
                    break;
            }
        }
    }
}

struct netif *wm_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src)
{
    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        /* is the netif up, does it have a link and a valid address? */
        if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
            if (NULL != src)
            {
                if (ip_addr_isany(src))
                {
                    /* broadcast? */
                    if (ip_addr_isbroadcast(dest, netif))
                        return netif;
                }
                else
                {
                    if (ip4_addr_netcmp(src, netif_ip4_addr(netif), netif_ip4_netmask(netif))) {
                        /* return netif on which to forward IP packet */
                        return netif;
                    }
                }
            }
            else
            {
                /* broadcast? */
                if (ip_addr_isbroadcast(dest, netif))
                    return netif;
            }
        }
  }

  return NULL;
}

/*************************************************************************** 
* Function: Tcpip_stack_init
*
* Description: This function is init ip stack. 
* 
* Input: 
*		ipaddr:  
*		netmask: 
*       gateway: 
* Output: 
* 
* Return: 
*		netif: Init IP Stack OK
*       NULL : Init IP Statck Fail Because no memory
* Date : 2014-6-4 
****************************************************************************/ 
struct netif *Tcpip_stack_init()
{
//	err_t err;
#if TLS_CONFIG_AP
    struct netif *nif4apsta = NULL;
#endif
	
    /* Setup lwIP. */
    tcpip_init(NULL, NULL);

#if TLS_CONFIG_AP
    /* add net info for apsta's ap */
    nif4apsta = (struct netif *)tls_mem_alloc(sizeof(struct netif));
    if (nif4apsta == NULL)
        return NULL;
#endif

    /*Add Net Info to Netif, default */
    nif = (struct netif *)tls_mem_alloc(sizeof(struct netif));
    if (nif == NULL)
    {
#if TLS_CONFIG_AP
        tls_mem_free(nif4apsta);
#endif
        return NULL;
    }

#if TLS_CONFIG_AP
    memset(nif4apsta, 0, sizeof(struct netif));
    //nif->next = nif4apsta;
    netifapi_netif_add(nif4apsta, IPADDR_ANY, IPADDR_ANY, IPADDR_ANY, NULL, ethernetif_init, tcpip_input);
    netif_set_status_callback(nif4apsta, netif_status_changed2);
#endif

    memset(nif, 0, sizeof(struct netif));
    netifapi_netif_add(nif, IPADDR_ANY,IPADDR_ANY,IPADDR_ANY,NULL,ethernetif_init,tcpip_input);
    netifapi_netif_set_default(nif);
    dl_list_init(&netif_status_event.list);
    netif_set_status_callback(nif, netif_status_changed);
    tls_wifi_status_change_cb_register(wifi_status_changed);

	/*Register Ethernet Rx Data callback From wifi*/
	tls_ethernet_data_rx_callback(ethernetif_input);
#if TLS_CONFIG_AP_OPT_FWD
    alg_napt_init(); 
	tls_ethernet_ip_rx_callback(alg_input);
#endif

    return nif;
}

#ifndef TCPIP_STACK_INIT
#define TCPIP_STACK_INIT Tcpip_stack_init
#endif
int tls_ethernet_init()
{
    if(ethif)
        tls_mem_free(ethif);
    ethif = tls_mem_alloc(sizeof(struct tls_ethif));
    memset(ethif, 0, sizeof(struct tls_ethif));

#if TLS_CONFIG_AP
    if(ethif2)
        tls_mem_free(ethif2);
    ethif2 = tls_mem_alloc(sizeof(struct tls_ethif));
    memset(ethif2, 0, sizeof(struct tls_ethif));
#endif
    TCPIP_STACK_INIT();
#if TLS_CONFIG_SOCKET_RAW
    tls_net_init();
#endif
    return 0;
}

void tls_netif_set_status(u8 status)
{
    if (ethif)
    {
        ethif->status = status;
    }

    return;
}

struct tls_ethif * tls_netif_get_ethif(void)
{
#if TLS_CONFIG_IPV6
    int i;
#endif    
	ip_addr_t * addr = NULL;
    ip_addr_t dns1,dns2;
    if (nif && ethif)
    {
//    MEMCPY((char *)&ethif->ip_addr.addr, &nif->ip_addr.addr, 4);
    ip_addr_copy(ethif->ip_addr, nif->ip_addr);
   // MEMCPY((char *)&ethif->netmask.addr, &nif->netmask.addr, 4);
    ip_addr_copy(ethif->netmask, nif->netmask);
//    MEMCPY((char *)&ethif->gw.addr, &nif->gw.addr, 4);
    ip_addr_copy(ethif->gw, nif->gw);
#if TLS_CONFIG_IPV6
    for(i = 0;i < IPV6_ADDR_MAX_NUM;i ++)
    {
        ip_addr_copy(ethif->ip6_addr[i], nif->ip6_addr[i]);
    }
#endif
    
    addr = (ip_addr_t *)dns_getserver(0);
	dns1 = *addr;
    //MEMCPY(&ethif->dns1.addr, (char *)&dns1.addr, 4);
    ip_addr_copy(ethif->dns1, dns1);
    addr = (ip_addr_t *)dns_getserver(1);
	dns2 = *addr;
    //MEMCPY(&ethif->dns2.addr, (char *)&dns2.addr, 4);
    ip_addr_copy(ethif->dns2, dns2);
	//ethif->status = nif->flags&NETIF_FLAG_UP;
#if TLS_CONFIG_IPV6
	for(i = 0; i < IPV6_ADDR_MAX_NUM; i++)
	{
    	ethif->ipv6_status[i] = (nif->ip6_addr_state[i] == IP6_ADDR_PREFERRED ? 1 : 0);
	}
#endif	
    }
    return ethif;
}

err_t tls_dhcp_start(void)
{
#if 0
	if (nif->flags & NETIF_FLAG_UP) 
	  nif->flags &= ~NETIF_FLAG_UP;
#endif
	if (nif)
	{
        return netifapi_dhcp_start(nif);
	}
	return -1;
}

err_t tls_dhcp_stop(void)
{
	if (nif)
	{
        return netifapi_dhcp_stop(nif);
	}
	return -1;	
}

err_t tls_netif_set_addr(ip_addr_t *ipaddr,
                        ip_addr_t *netmask,
                        ip_addr_t *gw)
{
    return netifapi_netif_set_addr(nif, (ip4_addr_t *)ipaddr, (ip4_addr_t *)netmask, (ip4_addr_t *)gw);
}
void tls_netif_dns_setserver(u8 numdns, ip_addr_t *dnsserver)
{
    dns_setserver(numdns, dnsserver);
}
err_t tls_netif_set_up(void)
{
	netif_set_link_up(nif);
    return netifapi_netif_set_up(nif);
}
err_t tls_netif_set_down(void)
{
    return netifapi_netif_set_down(nif);
}
err_t tls_netif_add_status_event(tls_netif_status_event_fn event_fn)
{
    u32 cpu_sr;
    struct tls_netif_status_event *evt;
	if (nif)
	{
	    //if exist, remove from event list first.
	    tls_netif_remove_status_event(event_fn);
	    evt = tls_mem_alloc(sizeof(struct tls_netif_status_event));
	    if(evt==NULL)
	        return -1;
	    memset(evt, 0, sizeof(struct tls_netif_status_event));
	    evt->status_callback = event_fn;
	    cpu_sr = tls_os_set_critical();
	    dl_list_add_tail(&netif_status_event.list, &evt->list);
	    tls_os_release_critical(cpu_sr);
	}

	return 0;
}
err_t tls_netif_remove_status_event(tls_netif_status_event_fn event_fn)
{
    struct tls_netif_status_event *status_event;
    bool is_exist = FALSE;
    u32 cpu_sr;
	if (nif)
	{
	    if(dl_list_empty(&netif_status_event.list))
	        return 0;
	    dl_list_for_each(status_event, &netif_status_event.list, struct tls_netif_status_event, list)
	    {
	        if(status_event->status_callback == event_fn)
	        {
	            is_exist = TRUE;
	            break;
	        }
	    }
	    if(is_exist)
	    {
	        cpu_sr = tls_os_set_critical();
	        dl_list_del(&status_event->list);
	        tls_os_release_critical(cpu_sr);
	        tls_mem_free(status_event);
	    }
	}
	return 0;
}

#if TLS_CONFIG_RMMS
INT8S tls_rmms_start(void)
{
    return RMMS_Init(nif);
}
void tls_rmms_stop(void)
{
    RMMS_Fini();
}
#endif

#if TLS_CONFIG_AP
INT8S tls_dhcps_start(void)
{
        return DHCPS_Start(nif->next);
}
void tls_dhcps_stop(void)
{
    DHCPS_Stop();
}

INT8S tls_dnss_start(INT8U * DnsName)
{
    return DNSS_Start(nif->next, DnsName);
}
void tls_dnss_stop(void)
{
    DNSS_Stop();
}
ip_addr_t *tls_dhcps_getip(const u8_t *mac)
{
	return DHCPS_GetIpByMac(mac);
}
u8 *tls_dhcps_getmac(const ip_addr_t *ip)
{
	return DHCPS_GetMacByIp(ip);
}
#endif

#if TLS_CONFIG_AP
err_t tls_netif2_set_up(void)
{
	netif_set_link_up(nif->next);
    return netifapi_netif_set_up(nif->next);
}
err_t tls_netif2_set_down(void)
{
    return netifapi_netif_set_down(nif->next);
}
err_t tls_netif2_set_addr(ip_addr_t *ipaddr,
                          ip_addr_t *netmask,
                          ip_addr_t *gw)
{
    return netifapi_netif_set_addr(nif->next, (ip4_addr_t *)ipaddr, (ip4_addr_t *)netmask, (ip4_addr_t *)gw);
}
/* numdns 0/1  --> dns 1/2 */
void tls_dhcps_setdns(u8_t numdns)
{
    const ip_addr_t* dns;
	dns = dns_getserver(numdns);
	DHCPS_SetDns(numdns, ip_addr_get_ip4_u32(dns));

	return;
}
#endif

struct netif *tls_get_netif(void)
{
    return nif;
}

