/**************************************************************************
 * File Name                   : dhcp_server.c
 * Author                       :
 * Version                      :
 * Date                         :
 * Description                :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wm_mem.h"
#include "wm_debug.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "netif/ethernetif.h"
#include "dhcp_server.h"
#include "tls_common.h"
#include "wm_wifi.h"

#if TLS_CONFIG_AP

#define ETHARP_FLAG_FIND_ONLY    	2
#define ETHARP_FLAG_STATIC_ENTRY 	4



extern u8 *wpa_supplicant_get_mac(void);


/* 是否根据客户端dhcp报文中的broadcast标志来回应，不使用则统一使用广播回复 */
#define DHCPS_CHECK_BROADCAST_FLAG
#ifdef DHCPS_CHECK_BROADCAST_FLAG
#define IS_BROADCAST_SEND(x)    (((x) >> 15) == 1 ? TRUE : FALSE)
#endif

//static DHCP_SERVER DhcpServer;
static DHCP_SERVER *DhcpServer = NULL;

//static DHCP_MSG DhcpMsg;
static DHCP_MSG *DhcpMsg = NULL;


#define DHCP_SET_OPTION_SUBNET_MASK(buffer, mask, len)	\
	{	\
		INT32U tmp;	\
		*buffer++ = DHCP_OPTION_ID_SUBNET_MASK;	\
		*buffer++ = 4;	\
		tmp = mask;	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		len += 6;	\
	}

#define DHCP_SET_OPTION_GW(buffer, gw, len)	\
	{	\
		INT32U tmp;	\
		*buffer++ = DHCP_OPTION_ID_DEF_GW;	\
		*buffer++ = 4;	\
		tmp = gw;	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		len += 6;	\
	}

#define DHCP_SET_OPTION_DNS(buffer, dns1, dns2, len)	\
	{	\
		INT32U tmp;	\
		*buffer++ = DHCP_OPTION_ID_DNS_SERVER;	\
		*buffer++ = 8;	\
		tmp = dns1;	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		tmp = dns2;	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		len += 10;	\
	}

#define DHCP_SET_OPTION_SERVER_ID(buffer, ip, len)	\
	{	\
		INT32U tmp;	\
		*buffer++ = DHCP_OPTION_ID_SERVER_ID;	\
		*buffer++ = 4;	\
		tmp = ip;	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		len += 6;	\
	}

#define DHCP_SET_OPTION_MSG_TYPE(buffer, type, len)	\
	{	\
		*buffer++ = DHCP_OPTION_ID_MSG_TYPE;	\
		*buffer++ = 1;	\
		*buffer++ = type;	\
		len += 3;	\
	}

#define DHCP_SET_OPTION_LEASE_TIME(buffer, time, len)	\
	{	\
		INT32U tmp;	\
		*buffer++ = DHCP_OPTION_ID_LEASE_TIME;	\
		*buffer++ = 4;	\
		tmp = htonl(time);	\
		MEMCPY(buffer, &tmp, 4);	\
		buffer += 4;	\
		len += 6;	\
	}

#define DHCP_SET_OPTION_END(buffer, len)	\
	{	\
		*buffer++ = DHCP_OPTION_ID_END;	\
		len ++;	\
	}

#if 0
static void _PostMsgToSysQ(PDHCP_CLIENT pClient, INT16U nMsgId)
{

	TSYSC_MSG * Msg;	
	INT8U Err;
	INT8U Len;

	Len = 9;
	Msg = tls_mem_alloc(sizeof(TSYSC_MSG) + Len);
	if (Msg == NULL){return;}

	Msg->Id = nMsgId;
	Msg->Len = Len;
	Msg->Argc = 0;

	MEMCPY(&Msg->Argv[0], pClient->MacAddr, 6);
	MEMCPY(&Msg->Argv[0] + 6, (void *)&pClient->IpAddr, 4);
	
	Err = OSQPost(Que_Sys, (void *)Msg);
	assert(Err == OS_NO_ERR);
	UNUSED_ARG(Err);

}
#endif	

static bool _CheckMacIsValid(u8 *mac)
{
#define STA_MAC_BUF_LEN  64
    u32 cnt;
    u32 sta_num = 0;
    u8 *sta_buf;
    bool ret = FALSE;
    struct tls_sta_info_t *sta;

    sta_buf = tls_mem_alloc(STA_MAC_BUF_LEN);
    if (!sta_buf)
        return FALSE;/* 系统资源不足，无需再让client接入 */

    memset(sta_buf, 0, STA_MAC_BUF_LEN);
    tls_wifi_get_authed_sta_info(&sta_num, sta_buf, STA_MAC_BUF_LEN);
    sta = (struct tls_sta_info_t *)sta_buf;
    for (cnt = 0; cnt < sta_num; cnt++)
    {
        if (!compare_ether_addr(mac, sta->mac_addr))
        {
            ret = TRUE;/* 本SOFTAP下的client才予以分配IP */
            break;
        }
        sta++;
    }
    tls_mem_free(sta_buf);

    return ret;
}

static void _DhcpTickHandle(void * Arg)
{
	INT8U i;
	PDHCP_CLIENT pClient;

	if (DhcpServer == NULL)
	{
		return;
	}


	if(DhcpServer->Enable == 0)
	{
		return;
	}
	
	for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
	{
		pClient = &DhcpServer->Clients[i];
		if(pClient->State == DHCP_CLIENT_STATE_REQUEST)
		{
			if(pClient->Timeout && (--pClient->Timeout == 0))
			{
				/* Timeout for the client's request frame. */
				memset(pClient->MacAddr, 0, 6);
				pClient->State = DHCP_CLIENT_STATE_IDLE;
			}
		}
		else if(pClient->State == DHCP_CLIENT_STATE_BIND)
		{
#if 1		
			if(pClient->Lease && (--pClient->Lease == 0))
			{
				/* The lease time over. */
				pClient->State = DHCP_CLIENT_STATE_IDLE;
		//		_PostMsgToSysQ(pClient, SYSC_MSG_IP_RELEASE);
			}
			else if(_CheckMacIsValid(pClient->MacAddr) == 0)
			{
				/* The client leave the wireless network. */
				pClient->State = DHCP_CLIENT_STATE_IDLE;
		//		_PostMsgToSysQ(pClient, SYSC_MSG_IP_RELEASE);
			}
#endif			
		}	
	}
	
	if(DhcpServer->Enable) sys_timeout(DHCP_TICK_TIME, _DhcpTickHandle, NULL);
}

static INT8U _ParseDhcpOptions(PDHCP_MSG pMsg, INT8U * pMsgType, INT32U * pReqIpAddr, INT32U * pServerId)
{
	INT8U * pDhcpOptions, * pEnd;
	INT32U Len;
	INT8U Ret;

	pDhcpOptions = pMsg->Options;
	pEnd = (INT8U *)(pMsg) + sizeof(DHCP_MSG);
	Len = 0;
	Ret = 0;
	while((*pDhcpOptions != DHCP_OPTION_ID_END) && (pDhcpOptions < pEnd))
	{
		if (*pDhcpOptions == DHCP_OPTION_ID_PAD)
		{
			/* Skip the pad. */
			pDhcpOptions += 1;
		}
		else
		{
			Len = (*(pDhcpOptions + 1));
			if(*pDhcpOptions == DHCP_OPTION_ID_MSG_TYPE)
			{
				/* Get the message type. */
				pDhcpOptions += 2;
				*pMsgType = *pDhcpOptions;
				Ret |= 0x01;
			}
			else if(*pDhcpOptions == DHCP_OPTION_ID_REQ_IP_ADDR)
			{
				/* Get the client's requested ip address. */
				pDhcpOptions += 2;
				MEMCPY((INT8U*)pReqIpAddr, pDhcpOptions,4 );
				Ret |= 0x02;
			}
			else if(*pDhcpOptions == DHCP_OPTION_ID_SERVER_ID)
			{
				/* Get the server's ip address. */
				pDhcpOptions += 2;
				MEMCPY((INT8U*)pServerId, pDhcpOptions,4 );
				Ret |= 0x04;
			}
			else
			{
				/* Dropped the other options. */
				pDhcpOptions += 2;
			}
			pDhcpOptions += Len;
		}
	}
	return Ret;
}

static PDHCP_CLIENT _ClientTableLookup(INT8U * MacAddr, INT8U MsgType, INT32U ReqIpAddr, INT32U ServerId)
{
	INT8U i;
	INT8U IpUnavailable;
	PDHCP_CLIENT pClient;
	PDHCP_CLIENT pFreeClient;
	PDHCP_CLIENT pReqClient;
	PDHCP_CLIENT pMyHistoryClient;
	PDHCP_CLIENT pHistoryClient;
	PDHCP_CLIENT pMyClient;
	PDHCP_CLIENT pReturnClient;

	pFreeClient = NULL;
	pReqClient = NULL;
	pHistoryClient= NULL;
	pMyHistoryClient = NULL;
	pMyClient = NULL;
	pReturnClient = NULL;
	IpUnavailable = 0;

	if (DhcpServer == NULL)
	{
		return NULL;
	}

	for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
	{
		pClient = &DhcpServer->Clients[i];
		if(pClient->State == DHCP_CLIENT_STATE_IDLE)
		{	
			if((pMyHistoryClient == NULL) && (memcmp(pClient->MacAddr, MacAddr, 6) == 0))
			{
				/* Get my history entry. */
				pMyHistoryClient = pClient;
			}

			if((pReqClient == NULL) && ip_addr_cmp(&pClient->IpAddr, (ip_addr_t *)&ReqIpAddr) )
			{
				/* Get the idle entry that hold my requested ip address. */
				pReqClient = pClient;
			}

			if((pFreeClient == NULL) && (memcmp(pClient->MacAddr, "\x00\x00\x00\x00\x00\x00", 6) == 0) )
			{
				/* Get the first free entry. */
				pFreeClient = pClient;
			}

			if((pHistoryClient == NULL) && (memcmp(pClient->MacAddr, "\x00\x00\x00\x00\x00\x00", 6) != 0))
			{
				/* Get the first histoy entry that not belong to me. */
				pHistoryClient = pClient;
			}
		}
		else
		{
			if(memcmp(pClient->MacAddr, MacAddr, 6) == 0)
			{
				/* Is negotiating ip address or has negotiated now. */
				pMyClient = pClient;
			}
			else if(ip_addr_cmp(&pClient->IpAddr, (ip_addr_t *)&ReqIpAddr))
			{
				/* The requested ip address is allocated. */
				IpUnavailable = 1;
			}
		}
	}

	switch(MsgType)
	{
		case DHCP_MSG_DISCOVER:
			if(pMyClient)
			{
				/* Amazing!!The client restart the negotiation. */
				pMyClient->State = DHCP_CLIENT_STATE_IDLE;
				
				if(pReqClient)
				{
					/* The client request another ip address and that address is not allocated now. */
					if(pMyClient->State != DHCP_CLIENT_STATE_BIND)
					{
						memset(pMyClient->MacAddr, 0, 6);
					}
					else
					{
					//	_PostMsgToSysQ(pMyClient, SYSC_MSG_IP_RELEASE);
					}
					pReturnClient = pReqClient;
				}
				else
				{
					/* The client request the same ip address. */
					pReturnClient = pMyClient;
				}
			}
			else
			{
				/* A new negotiation! */

				/*
					The IP address allocation priority(high to low):
					1. the client's request ip address.
					2. the client's history ip address.
					3. a totally free ip address.
					4. the other client's history ip address.
				*/
				if(pReqClient)
				{
					/* The client's request ip address. */
					pReturnClient = pReqClient;
				}
				else if(pMyHistoryClient)
				{
					/* The client's history ip address. */
					pReturnClient = pMyHistoryClient;
				}
				else if(pFreeClient)
				{
					/* A totally free ip address. */
					pReturnClient = pFreeClient;
				}
				else if(pHistoryClient)
				{
					/* The other client's history ip address. */
					pReturnClient = pHistoryClient;
				}
				else
				{
					/* The IP pool is full, so return null. */
					pReturnClient = NULL;
				}
			}
			break;

		case DHCP_MSG_REQUEST:
			if(pMyClient)
			{
				if(IpUnavailable == 1)
				{
					/* The client request a new address that was allocated, so return null. */
					pReturnClient = NULL;
					
					if(pMyClient->State != DHCP_CLIENT_STATE_BIND)
					{
						memset(pMyClient->MacAddr, 0, 6);
					}
					pMyClient->State = DHCP_CLIENT_STATE_IDLE;
				}
				else
				{
					if(pReqClient)
					{
						/* The client request a new address that is not allocated. */
						if(pMyClient->State != DHCP_CLIENT_STATE_BIND)
						{
							memset(pMyClient->MacAddr, 0, 6);
						}
						else
						{
						//	_PostMsgToSysQ(pMyClient, SYSC_MSG_IP_RELEASE);
						}

						pMyClient->State = DHCP_CLIENT_STATE_IDLE;
						if((ServerId == 0) || (ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr) == ServerId))						
						{
							/* The client request the new address and that is free, allocate it. */
							pReturnClient = pReqClient;
						}
						else
						{
							/* The client refuses my offer and request another ip address. */
							pReturnClient = NULL;
						}
					}				
					else if((ServerId == 0) || (ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr) == ServerId))				
					{
						/* The client request this address that has been allocated to it(a little abnorm) or the client renew the lease time. */
						pReturnClient = pMyClient;
					}
					else
					{
						/* The client refuses my offer. */
						pReturnClient = NULL;
					}
				}
			}
			else
			{
				if(IpUnavailable == 1)
				{
					/* The requested ip address was allocated, so return null. */
					pReturnClient = NULL;
				}				
				else if((ServerId == 0) || (ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr) == ServerId))				
				{
					/* The client request my free address, so allocate it. */
					pReturnClient = pReqClient;
				}
				else
				{
					/* The client request the address from another server, but send it to me, so return null. */
					pReturnClient = NULL;
				}
			}
			break;

		case DHCP_MSG_RELEASE:
			/* The client release the ip address. */
			if(pMyClient)
			{
				if(pMyClient->State != DHCP_CLIENT_STATE_BIND)
				{
					memset(pMyClient->MacAddr, 0, 6);
				}
				else
				{
				//	_PostMsgToSysQ(pClient, SYSC_MSG_IP_RELEASE);
				}
				pMyClient->State = DHCP_CLIENT_STATE_IDLE;
			}
			pReturnClient = NULL;
			break;

		case DHCP_MSG_DECLINE:
			/* The client refuses my offer directly. */
			if(pMyClient)
			{
				if(pMyClient->State != DHCP_CLIENT_STATE_BIND)
				{
					memset(pMyClient->MacAddr, 0, 6);
				}
				else
				{
				//	_PostMsgToSysQ(pClient, SYSC_MSG_IP_RELEASE);
				}
				pMyClient->State = DHCP_CLIENT_STATE_IDLE;
			}
			pReturnClient = NULL;
			break;

		default:
			/* Dropped the other frames. */
			pReturnClient = NULL;
			break;
	}

	if(pReturnClient)
	{
		/* Updata the client's MAC address. */
		MEMCPY(pReturnClient->MacAddr, MacAddr, 6);
	}

	return pReturnClient;
}

static void _DHCPNakGenAndSend(INT8U * pClientMacAddr, INT32U Xid)
{
	INT32U Len;
	INT8U * Body;
	PDHCP_MSG pDhcpMsg;
	struct pbuf * pDhcpBuf;

	if (DhcpMsg == NULL || DhcpServer == NULL)
	{
		return;
	}

	pDhcpBuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(DHCP_MSG), PBUF_RAM);
	if(pDhcpBuf == NULL)
	{
		return;
	}

    pDhcpMsg = DhcpMsg;
	memset(pDhcpMsg, 0, sizeof(*pDhcpMsg));

	/* Initialize the DHCP message header. */
	pDhcpMsg->Op = DHCP_OP_REPLY;
	pDhcpMsg->HType = DHCP_HWTYPE_ETHERNET;
	pDhcpMsg->HLen = 6;
	pDhcpMsg->Xid = htonl(Xid);	
	pDhcpMsg->Siaddr = ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr);	
	MEMCPY(pDhcpMsg->Chaddr, pClientMacAddr, 6);
	pDhcpMsg->Magic = htonl(DHCP_MAGIC);

	Len = 240;
	Body = &pDhcpMsg->Options[0];

	/* Set the message type. */
	DHCP_SET_OPTION_MSG_TYPE(Body, DHCP_MSG_NAK, Len);

	DHCP_SET_OPTION_END(Body, Len);

	pbuf_take(pDhcpBuf, (const void *)pDhcpMsg, Len);
	pbuf_realloc(pDhcpBuf, Len);

	/* Send broadcast to the DHCP client. */
	udp_sendto(DhcpServer->Socket, pDhcpBuf, IP_ADDR_BROADCAST, DHCP_CLIENT_PORT);

	TLS_DBGPRT_INFO("sent dhcp nak, ClientMacAddr="MACSTR", ServerIp=%d.%d.%d.%d\n",
	                MAC2STR(pClientMacAddr),
	                ip4_addr1(&DhcpServer->ServerIpAddr), ip4_addr2(&DhcpServer->ServerIpAddr),
	                ip4_addr3(&DhcpServer->ServerIpAddr), ip4_addr4(&DhcpServer->ServerIpAddr));
	pbuf_free(pDhcpBuf);
}

static void _DHCPAckGenAndSend(PDHCP_CLIENT pClient, INT8U * pClientMacAddr, INT32U Xid, INT16U Flags)
{
	INT32U Len;
	INT8U * Body;
	PDHCP_MSG pDhcpMsg;
	struct pbuf * pDhcpBuf;
	
	if (DhcpMsg == NULL || DhcpServer == NULL)
	{
		return;
	}

	pDhcpBuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(DHCP_MSG), PBUF_RAM);
	if(pDhcpBuf == NULL)
	{
		return;
	}

    pDhcpMsg = DhcpMsg;
	memset(pDhcpMsg, 0, sizeof(*pDhcpMsg));

	/* Initialize the DHCP message header. */
	pDhcpMsg->Op = DHCP_OP_REPLY;
	pDhcpMsg->HType = DHCP_HWTYPE_ETHERNET;
	pDhcpMsg->HLen = 6;
	pDhcpMsg->Xid = htonl(Xid);
	pDhcpMsg->Yiaddr = ip_addr_get_ip4_u32(&pClient->IpAddr);
	pDhcpMsg->Siaddr = 0;
	MEMCPY(pDhcpMsg->Chaddr, pClientMacAddr, 6);
	pDhcpMsg->Magic = htonl(DHCP_MAGIC);

	Len = 240;
	Body = &pDhcpMsg->Options[0];
	
	/* Set the message type. */
	DHCP_SET_OPTION_MSG_TYPE(Body, DHCP_MSG_ACK, Len);

	/* Set the lease time. */
	DHCP_SET_OPTION_LEASE_TIME(Body, DHCP_DEFAULT_LEASE_TIME, Len);
	/* Set the server's ip address */
	DHCP_SET_OPTION_SERVER_ID(Body, ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr), Len);

	/* Set the subnet mask. */
	DHCP_SET_OPTION_SUBNET_MASK(Body, ip_addr_get_ip4_u32(&DhcpServer->SubnetMask), Len);

	/* Set the default gatway's ip address. */
	DHCP_SET_OPTION_GW(Body, ip_addr_get_ip4_u32(&DhcpServer->GateWay), Len);

	/* Set the dns server's ip address. */
	DHCP_SET_OPTION_DNS(Body, ip_addr_get_ip4_u32(&DhcpServer->Dns1), ip_addr_get_ip4_u32(&DhcpServer->Dns2), Len);
	DHCP_SET_OPTION_END(Body, Len);

	pbuf_take(pDhcpBuf, (const void *)pDhcpMsg, Len);
	pbuf_realloc(pDhcpBuf, Len);

#ifdef DHCPS_CHECK_BROADCAST_FLAG
    if (IS_BROADCAST_SEND(Flags))
    {
#endif
	/* Send broadcast to the DHCP client. */
    udp_sendto(DhcpServer->Socket, pDhcpBuf, IP_ADDR_BROADCAST, DHCP_CLIENT_UDP_PORT);
#ifdef DHCPS_CHECK_BROADCAST_FLAG
    }
    else
    {
//	    etharp_update_arp_entry(tls_get_netif(), (ip_addr_t *)(&pClient->IpAddr), (struct eth_addr *)pClientMacAddr, ETHARP_FLAG_FIND_ONLY);
		etharp_update_arp_entry(tls_get_netif(), (ip_addr_t *)(&pClient->IpAddr), (struct eth_addr *)pClientMacAddr, ETHARP_FLAG_STATIC_ENTRY);
	    udp_sendto(DhcpServer->Socket, pDhcpBuf, (ip_addr_t *)(&pClient->IpAddr), DHCP_CLIENT_UDP_PORT);
    }
#endif

    TLS_DBGPRT_INFO("sent dhcp ack, ClientMacAddr="MACSTR", GivenIpAddr=%d.%d.%d.%d, ServerIp=%d.%d.%d.%d\n",
	                MAC2STR(pClientMacAddr),
	                ip4_addr1(&pClient->IpAddr), ip4_addr2( &pClient->IpAddr),
	                ip4_addr3(&pClient->IpAddr), ip4_addr4( &pClient->IpAddr),
	                ip4_addr1(&DhcpServer->ServerIpAddr), ip4_addr2(&DhcpServer->ServerIpAddr),
	                ip4_addr3(&DhcpServer->ServerIpAddr), ip4_addr4(&DhcpServer->ServerIpAddr) );

	pbuf_free(pDhcpBuf);
}

static void _DHCPOfferGenAndSend(PDHCP_CLIENT pClient, INT8U * pClientMacAddr, INT32U Xid, INT16U Flags)
{
	INT32U Len;
	INT8U * Body;
	PDHCP_MSG pDhcpMsg;
	struct pbuf * pDhcpBuf;

	if (DhcpMsg == NULL || DhcpServer == NULL)
	{
		return;
	}

	pDhcpBuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(DHCP_MSG), PBUF_RAM);
	if(pDhcpBuf == NULL)
	{
		return;
	}

    pDhcpMsg = DhcpMsg;
	memset(pDhcpMsg, 0, sizeof(*pDhcpMsg));

	/* Initialize the DHCP message header. */
	pDhcpMsg->Op = DHCP_OP_REPLY;
	pDhcpMsg->HType = DHCP_HWTYPE_ETHERNET;
	pDhcpMsg->HLen = 6;
	pDhcpMsg->Xid = htonl(Xid);	
	pDhcpMsg->Yiaddr = ip_addr_get_ip4_u32(&pClient->IpAddr);
	pDhcpMsg->Siaddr = 0;
	MEMCPY(pDhcpMsg->Chaddr, pClientMacAddr, 6);
	pDhcpMsg->Magic = htonl(DHCP_MAGIC);

	Len = 240;
	Body = &pDhcpMsg->Options[0];

	/* Set the message type. */
	DHCP_SET_OPTION_MSG_TYPE(Body, DHCP_MSG_OFFER, Len);

	/* Set the lease time. */
	DHCP_SET_OPTION_LEASE_TIME(Body, DHCP_DEFAULT_LEASE_TIME, Len);
	/* Set the server's ip address */
	DHCP_SET_OPTION_SERVER_ID(Body, ip_addr_get_ip4_u32(&DhcpServer->ServerIpAddr), Len);

	/* Set the subnet mask. */
	DHCP_SET_OPTION_SUBNET_MASK(Body, ip_addr_get_ip4_u32(&DhcpServer->SubnetMask), Len);

	/* Set the default gatway's ip address. */
	DHCP_SET_OPTION_GW(Body, ip_addr_get_ip4_u32(&DhcpServer->GateWay), Len);

	/* Set the dns server's ip address. */
	DHCP_SET_OPTION_DNS(Body, ip_addr_get_ip4_u32(&DhcpServer->Dns1), ip_addr_get_ip4_u32(&DhcpServer->Dns2), Len);
	DHCP_SET_OPTION_END(Body, Len);

	pbuf_take(pDhcpBuf, (const void *)pDhcpMsg, Len);
	pbuf_realloc(pDhcpBuf, Len);

#ifdef DHCPS_CHECK_BROADCAST_FLAG
    if (IS_BROADCAST_SEND(Flags))
    {
#endif
	/* Send broadcast to the DHCP client. */
	udp_sendto(DhcpServer->Socket, pDhcpBuf, IP_ADDR_BROADCAST, DHCP_CLIENT_UDP_PORT);
#ifdef DHCPS_CHECK_BROADCAST_FLAG
    }
    else
    {
//        etharp_update_arp_entry(tls_get_netif(), (ip_addr_t *)(&pClient->IpAddr), (struct eth_addr *)pClientMacAddr, ETHARP_FLAG_FIND_ONLY);
		etharp_update_arp_entry(tls_get_netif(), (ip_addr_t *)(&pClient->IpAddr), (struct eth_addr *)pClientMacAddr, ETHARP_FLAG_STATIC_ENTRY);
	    udp_sendto(DhcpServer->Socket, pDhcpBuf, (ip_addr_t *)(&pClient->IpAddr), DHCP_CLIENT_UDP_PORT); 
    }
#endif

    TLS_DBGPRT_INFO("sent dhcp offer, ClientMacAddr="MACSTR", GivenIpAddr=%d.%d.%d.%d, ServerIp=%d.%d.%d.%d\n",
	                MAC2STR(pClientMacAddr),
	                ip4_addr1(&pClient->IpAddr), ip4_addr2(&pClient->IpAddr),
	                ip4_addr3(&pClient->IpAddr), ip4_addr4(&pClient->IpAddr),
	                ip4_addr1( &DhcpServer->ServerIpAddr), ip4_addr2( &DhcpServer->ServerIpAddr),
	                ip4_addr3( &DhcpServer->ServerIpAddr), ip4_addr4( &DhcpServer->ServerIpAddr)
		   );				   
	
	pbuf_free(pDhcpBuf);
}

static void _CleanClientHistory(INT8U * pClientMacAddr)
{
	INT8U i;
	PDHCP_CLIENT pClient;
	if (DhcpServer == NULL)
	{
		return;
	}

	for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
	{
		pClient = &DhcpServer->Clients[i];
		if((pClient->State == DHCP_CLIENT_STATE_IDLE) && (memcmp(pClient->MacAddr, pClientMacAddr, 6) == 0))
		{
			/* Clean the history client's Mac address. */
			memset(pClient->MacAddr, 0, 6);
		}
	}
}

static void _DhcpClientSMEHandle(PDHCP_CLIENT pClient, INT8U MsgType, INT32U Xid, INT8U * MacAddr, INT16U Flags)
{
	switch(pClient->State)
	{
		case DHCP_CLIENT_STATE_IDLE:
			if(MsgType == DHCP_MSG_DISCOVER)
			{
				/* Receive the "DISCOVER" frame, switch the state to "SELECT". */
				pClient->State = DHCP_CLIENT_STATE_SELECT;
			}
			else if(MsgType == DHCP_MSG_REQUEST)
			{
				/* If the requested ip is not allocated, allocate it. */
				_DHCPAckGenAndSend(pClient, pClient->MacAddr, Xid, Flags);
				pClient->Lease = DHCP_DEFAULT_LEASE_TIME;
				pClient->State = DHCP_CLIENT_STATE_BIND;
				_CleanClientHistory(pClient->MacAddr);
//				_PostMsgToSysQ(pClient, SYSC_MSG_IP_ALLOCATED);
				break;
			}

		case DHCP_CLIENT_STATE_SELECT:
			/* Receive the "DISCOVER" frame, send "OFFER" to the client. */
			_DHCPOfferGenAndSend(pClient, pClient->MacAddr, Xid, Flags);
			pClient->Timeout = DHCP_DEFFAULT_TIMEOUT;
			pClient->State = DHCP_CLIENT_STATE_REQUEST;
			break;

		case DHCP_CLIENT_STATE_REQUEST:
//			_PostMsgToSysQ(pClient, SYSC_MSG_IP_ALLOCATED);
		case DHCP_CLIENT_STATE_BIND:
			/* Send ACK to the client, if receive the "REQUEST" frame to select the offer or renew the DHCP lease. */
			_DHCPAckGenAndSend(pClient, pClient->MacAddr, Xid, Flags);
			pClient->Lease = DHCP_DEFAULT_LEASE_TIME;
			pClient->State = DHCP_CLIENT_STATE_BIND;
			_CleanClientHistory(pClient->MacAddr);
			break;

		default:
			break;
	}
}

ip_addr_t *DHCPS_GetIpByMac(const INT8U *MacAddr)
{
    INT8U i;
    PDHCP_CLIENT pClient;
    ip_addr_t *IpAddr = NULL;
	if (DhcpServer == NULL)
	{
		return NULL;
	}

    for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
    {
        pClient = &DhcpServer->Clients[i];
        if (0 == compare_ether_addr(MacAddr, pClient->MacAddr))
        { 
            IpAddr = &pClient->IpAddr;
            break;
        }
    }

    return IpAddr;
} 

INT8U *DHCPS_GetMacByIp(const ip_addr_t *ipaddr)
{
    INT8U i;
    PDHCP_CLIENT pClient;
    INT8U *macaddr = NULL;
	if (DhcpServer == NULL)
	{
		return NULL;
	}

    for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
    {
        pClient = &DhcpServer->Clients[i];
        if (pClient->IpAddr.addr == ipaddr->addr)
        { 
            macaddr = pClient->MacAddr;
            break;
        }
    }

    return macaddr;
}

/* numdns 0/1  --> dns 1/2 */
void DHCPS_SetDns(INT8U numdns, INT32U dns)
{
	if (DhcpServer == NULL)
	{
		return;
	}

    if (0 == numdns)
        ip_addr_set_ip4_u32(&DhcpServer->Dns1, dns);
    if (1 == numdns)
        ip_addr_set_ip4_u32(&DhcpServer->Dns2, dns);
    return;
}

/*-------------------------------------------------------------------------
	Description:	
		When an incoming DHCP message is to me, this function process it and trigger the state machine.
	Arguments:
		Arg: Pointer to the user supplied argument.
		Pcb: Pointer to the udp_pcb which received data.
		P: Pointer to the packet buffer that was received.
		Addr: The remote IP address from which the packet was received.
		Port: The remote port from which the packet was received	.
	Return Value:
		None.
	Note:	
-------------------------------------------------------------------------*/
void DHCPS_RecvCb(void *Arg, struct udp_pcb *Pcb, struct pbuf *P, ip_addr_t *Addr, INT16U Port)
{
#if 1
    INT16U Flags;
	INT32U Xid;
	INT8U MsgType;
	PDHCP_MSG pDhcpMsg;
	INT32U MsgLen;

	ip_addr_t ReqIpAddr;
	ip_addr_t ServerId;
	
	INT8U ClientMacAddr[6];
	PDHCP_CLIENT pClient;
    INT8U* MacAddr;

	if (DhcpMsg == NULL)
	{
		return;
	}

	do
	{
	    pDhcpMsg = DhcpMsg;
        memset(pDhcpMsg, 0, sizeof(DHCP_MSG));
	    
		/* Copy the DHCP message. */
		MsgLen = pbuf_copy_partial(P, (void *)pDhcpMsg, sizeof(DHCP_MSG), 0);

		/* Filter out the frame that is not request frame or has wrong magic number or has wrong hardware address type. */
		if((MsgLen == 0) ||
			(pDhcpMsg->Op != DHCP_OP_REQUEST) || 
			(ntohl(pDhcpMsg->Magic) != DHCP_MAGIC) ||
			(pDhcpMsg->HType != DHCP_HWTYPE_ETHERNET))
		{
			break;
		}


        MacAddr = wpa_supplicant_get_mac();
        if (0 == memcmp(MacAddr, pDhcpMsg->Chaddr, 6))
        {
            TLS_DBGPRT_INFO("drop form ours dhcp packet\n");
            break;
        }

        if (!_CheckMacIsValid(pDhcpMsg->Chaddr))
        {
            TLS_DBGPRT_INFO("drop form router's dhcp packet\n");
            break;
        }

		/* Parse the packet to get message type, ip address requested by client and server ID. */
		MsgType = 0xff;			
		memset(&ReqIpAddr, 0, sizeof(ip_addr_t));
		memset(&ServerId, 0, sizeof(ip_addr_t));		
		if((_ParseDhcpOptions(pDhcpMsg, &MsgType, &ip4_addr_get_u32(&ReqIpAddr), &ip4_addr_get_u32(&ServerId)) & 0x01) == 0)		
		{
			break;
		}

         if (DHCP_MSG_INFORM == MsgType)
         {
            TLS_DBGPRT_INFO("drop dhcp inform packet\n");
            break;
         }

		/* Get the Xid and client's MAC address. */
		Xid = ntohl(pDhcpMsg->Xid);
		Flags = ntohs(pDhcpMsg->Flags);
		MEMCPY(ClientMacAddr, pDhcpMsg->Chaddr, 6);
		TLS_DBGPRT_INFO("ClientMacAddr=%x, MsgType=%x, ReqIpAddr=%x, ServerId=%x\n", (u32)ClientMacAddr, (u32)MsgType, (u32)&ReqIpAddr, (u32)&ServerId);

		TLS_DBGPRT_INFO("ClientMacAddr="MACSTR", MsgType=%x, ReqIpAddr=%d.%d.%d.%d, ServerIp=%d.%d.%d.%d\n",
		                MAC2STR(ClientMacAddr), MsgType,
		                ip4_addr1(&ReqIpAddr), ip4_addr2(&ReqIpAddr), ip4_addr3(&ReqIpAddr), ip4_addr4(&ReqIpAddr), 
		                ip4_addr1(&ServerId), ip4_addr2(&ServerId), ip4_addr3(&ServerId), ip4_addr4(&ServerId));

		/* Get the client entry that is free or negotiating. */		
		pClient = _ClientTableLookup(ClientMacAddr, MsgType, ip4_addr_get_u32(&ReqIpAddr), ip4_addr_get_u32(&ServerId));	
		if(pClient == NULL)
		{
			if((MsgType != DHCP_MSG_RELEASE) && (MsgType != DHCP_MSG_DECLINE))
			{
				/* Ip is already allocated, so send nack. */
				_DHCPNakGenAndSend(ClientMacAddr, Xid);
			}
			break;
		}
		
		/* Push to client state machine. */
		_DhcpClientSMEHandle(pClient, MsgType, Xid, ClientMacAddr, Flags);
	}while(0);

	pbuf_free(P);
#endif	
}

/*-------------------------------------------------------------------------
	Description:	
		This function is used to delete the dhcp client entry on the server.
	Arguments:
		MacAddr: Specify the MAC Address of the dhcp client entry to be delete.
	Return Value:
		The DHCP Server error code:
			DHCPS_ERR_SUCCESS - No error
			DHCPS_ERR_INACTIVE - The Server is inactive
			DHCPS_ERR_PARAM - The input parameter error
			DHCPS_ERR_NOT_FOUND - Not fount this client	
	Note:	
-------------------------------------------------------------------------*/
INT8S DHCPS_ClientDelete(INT8U * MacAddr)
{
	INT8U i;
	PDHCP_CLIENT pClient;

	if (DhcpServer == NULL)
	{
		return DHCPS_ERR_MEM;
	}
	/* Check the server is active now. */
	if(DhcpServer->Enable == 0)
	{
		return DHCPS_ERR_INACTIVE;
	}

	if(MacAddr == NULL)
	{
		return DHCPS_ERR_PARAM;
	}
	
	for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
	{
		pClient = &DhcpServer->Clients[i];
		if((pClient->State != DHCP_CLIENT_STATE_IDLE) && (memcmp(pClient->MacAddr, MacAddr, 6) == 0))
		{
			if(pClient->State != DHCP_CLIENT_STATE_BIND)
			{
				/* For negotiating client, return. */
				return DHCPS_ERR_NOT_BIND;
			}
			else
			{
				/* For bind client, delete it directly. */
				pClient->State = DHCP_CLIENT_STATE_IDLE;
				return DHCPS_ERR_SUCCESS;
			}
		}
	}

	return DHCPS_ERR_NOT_FOUND;
}

/*-------------------------------------------------------------------------
	Description:	
		This function is used to start DHCP Server for a network interface.
	Arguments:
		Netif: Pointer to the Lwip network interface.
	Return Value:
		The DHCP Server error code:
			DHCPS_ERR_SUCCESS - No error
			DHCPS_ERR_MEM - Out of memory
			DHCPS_ERR_LINKDOWN - The NI is inactive
	Note:	
		The dhcp server must be started after the network interface was actived.
-------------------------------------------------------------------------*/
INT8S DHCPS_Start(struct netif *Netif)
{
	INT32U Val, Mask, tmp, i;
	PDHCP_CLIENT pClient;

	/* Check the network interface is active now. */
	if(netif_is_up(Netif) == 0)
	{
		return DHCPS_ERR_LINKDOWN;
	}
	if (DhcpServer == NULL)
	{
		DhcpServer = tls_mem_alloc(sizeof(*DhcpServer));
	}

	if (DhcpMsg == NULL)
	{
		DhcpMsg = tls_mem_alloc(sizeof(*DhcpMsg));
	}

	if (DhcpServer == NULL || DhcpMsg == NULL)
	{
		if (DhcpServer)
		{
			tls_mem_free(DhcpServer);
			DhcpServer = NULL;
		}
		if (DhcpMsg)
		{
			tls_mem_free(DhcpMsg);
			DhcpMsg = NULL;
		}		
		return DHCPS_ERR_MEM;
	}
	memset(DhcpServer, 0, sizeof(*DhcpServer));
	
	/* Calculate the start ip address of the server's ip pool. */	
	Val = ntohl(ip_addr_get_ip4_u32(&Netif->ip_addr));
	Mask = ntohl(ip_addr_get_ip4_u32(&Netif->netmask));	
	tmp = (Val & (~Mask));
	tmp = ((tmp + 1) % (~Mask)) ? ((tmp + 1) % (~Mask)) : 1;
	Val = htonl((Val & Mask) | tmp);
	
	/* Configure the DHCP Server. */
	ip_addr_set(&DhcpServer->ServerIpAddr, &Netif->ip_addr);
	ip_addr_set(&DhcpServer->StartIpAddr, (ip_addr_t *)&Val);
	ip_addr_set(&DhcpServer->SubnetMask, &Netif->netmask);
	ip_addr_set(&DhcpServer->GateWay, &Netif->ip_addr);
	ip_addr_set(&DhcpServer->Dns1, &Netif->ip_addr);

	/* Set the default lease time - 2 hours. */
	DhcpServer->LeaseTime = DHCP_DEFAULT_LEASE_TIME;
	
	/* Initialize the free DHCP clients. */
	for(i = 0; i < DHCPS_HISTORY_CLIENT_NUM; i++)
	{
		pClient = &DhcpServer->Clients[i];
		/* Set the initial client state is "IDLE". */
		pClient->State = DHCP_CLIENT_STATE_IDLE;
		
		/* Set the ip address to the client. */	
		Val = ntohl(ip_addr_get_ip4_u32(&DhcpServer->StartIpAddr));		
		tmp = (Val & (~Mask));
		tmp = ((tmp + i) % (~Mask)) ? ((tmp + i) % (~Mask)) : 1;
		Val = htonl((Val & Mask) | tmp);
		ip_addr_set(&pClient->IpAddr, (ip_addr_t *)&Val);
		
		/* Set the default lease time. */
		pClient->Lease = DHCP_DEFAULT_LEASE_TIME;
	}
	
	/* Allocate a UDP PCB. */
	DhcpServer->Socket = udp_new();
	if(DhcpServer->Socket == NULL)
	{
		return DHCPS_ERR_MEM;
	}
	
	/* Set up local and remote port for the pcb. */
	udp_bind(DhcpServer->Socket, IP_ADDR_ANY, DHCP_SERVER_UDP_PORT);

	/* bind multicast&broadcast netif */
	//udp_bind_multicast_netif(DhcpServer->Socket, &Netif->ip_addr);
	udp_set_multicast_netif_addr(DhcpServer->Socket, &Netif->ip_addr);

	/* Set up the recv callback and argument. */
	udp_recv(DhcpServer->Socket, (udp_recv_fn)DHCPS_RecvCb, Netif);
	
	/* Start the DHCP Server tick timer. */
	sys_timeout(DHCP_TICK_TIME, _DhcpTickHandle, NULL);
	
	/* Enable the DHCP Server. */
	DhcpServer->Enable = 1;
	
	return DHCPS_ERR_SUCCESS;
}

/*-------------------------------------------------------------------------
	Description:	
		This function is used to disable dhcp server.
	Arguments:
		None.		
	Return Value:
		None.
	Note:	
-------------------------------------------------------------------------*/
void DHCPS_Stop(void)
{
	if (DhcpServer)
	{
		/* Disable the dhcp server's service. */
		DhcpServer->Enable = 0;
		
		/* Stop the tick timer. */
		sys_untimeout(_DhcpTickHandle, NULL);
		
		/* Release the socket. */
		if(DhcpServer->Socket) 
		{
			udp_remove(DhcpServer->Socket);
		}

		memset(DhcpServer, 0, sizeof(*DhcpServer));

		tls_mem_free(DhcpServer);
		DhcpServer = NULL;
	}
	if (DhcpMsg)
	{
		tls_mem_free(DhcpMsg);
		DhcpMsg = NULL;
	}
}
#endif

