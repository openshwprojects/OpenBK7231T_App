/**************************************************************************
 * File Name                   : dhcp_server.h
 * Author                       :
 * Version                      :
 * Date                         :
 * Description                :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#ifndef __DHCP_SERVER_H_175137__
#define __DHCP_SERVER_H_175137__

#include "wm_config.h"

#if TLS_CONFIG_AP
#include "wm_sockets.h"

#define DHCPS_ERR_SUCCESS      0
#define DHCPS_ERR_LINKDOWN      -1
#define DHCPS_ERR_PARAM      -2
#define DHCPS_ERR_MEM      -3
#define DHCPS_ERR_NOT_BIND      -4
#define DHCPS_ERR_NOT_FOUND      -5
#define DHCPS_ERR_INACTIVE      -6

#define DHCPS_HISTORY_CLIENT_NUM      8

#define DHCP_DEFAULT_LEASE_TIME      7200 /* 2 Hours. */
#define DHCP_DEFAULT_LEASE_TIME_MS      7200000 /* 7200000ms */
#define DHCP_TICK_TIME      1000 /* 1s. */
#define DHCP_DEFFAULT_TIMEOUT      10 /* 500ms */

#define DHCP_HWTYPE_ETHERNET      1

/* udp port number of dhcp. */
#define DHCP_CLIENT_UDP_PORT 68  
#define DHCP_SERVER_UDP_PORT 67

/* Magic number in DHCP packet. */
#define DHCP_MAGIC 0x63825363

#define DHCP_OP_REQUEST  1
#define DHCP_OP_REPLY    2

#define DHCP_OPTION_ID_PAD      0
#define DHCP_OPTION_ID_SUBNET_MASK      1
#define DHCP_OPTION_ID_DEF_GW      3
#define DHCP_OPTION_ID_DNS_SERVER      6
#define DHCP_OPTION_ID_LEASE_TIME      51
#define DHCP_OPTION_ID_MSG_TYPE      53
#define DHCP_OPTION_ID_SERVER_ID      54
#define DHCP_OPTION_ID_REQ_IP_ADDR      50
#define DHCP_OPTION_ID_END      255

#define DHCP_MSG_DISCOVER      1
#define DHCP_MSG_OFFER      2
#define DHCP_MSG_REQUEST      3
#define DHCP_MSG_DECLINE      4
#define DHCP_MSG_ACK      5
#define DHCP_MSG_NAK      6
#define DHCP_MSG_RELEASE      7
#define DHCP_MSG_INFORM      8

#ifdef INT8U
#undef INT8U
#endif
typedef unsigned char INT8U;

#ifdef INT8S
#undef INT8S
#endif
typedef signed char INT8S;

#ifdef INT16U
#undef INT16U
#endif
typedef unsigned short INT16U;

#ifdef INT32U
#undef INT32U
#endif
typedef unsigned int INT32U;

typedef enum __DHCP_CLIENT_STATE
{
	DHCP_CLIENT_STATE_IDLE=0,
	DHCP_CLIENT_STATE_SELECT,
	DHCP_CLIENT_STATE_REQUEST,
	DHCP_CLIENT_STATE_BIND
}DHCP_CLIENT_STATE;

typedef struct __DHCP_CLIENT
{
	DHCP_CLIENT_STATE State;
	INT32U Timeout;
	/* Attention!!! MUST BE __align(4) */
	ip_addr_t IpAddr;
	INT8U MacAddr[6];
	INT32U Lease;
}DHCP_CLIENT, *PDHCP_CLIENT;

typedef struct __DHCP_SERVER
{
	INT8U Enable;
	struct udp_pcb * Socket;
	/* Attention!!! MUST BE __align(4) */
	ip_addr_t ServerIpAddr;
	ip_addr_t StartIpAddr;
	ip_addr_t SubnetMask;
	ip_addr_t GateWay;
	ip_addr_t Dns1;
	ip_addr_t Dns2;	
	INT32U LeaseTime;
	DHCP_CLIENT Clients[DHCPS_HISTORY_CLIENT_NUM];
}DHCP_SERVER, *PDHCP_SERVER;

#define DHCPS_HADDR_SIZE      16
#define DHCPS_SNAME_SIZE      64
#define DHCPS_FILENAME_SIZE      128
#define DHCPS_OPTIONS_LEN      312
typedef struct __DHCP_MSG
{
	INT8U Op;
	INT8U HType;
	INT8U HLen;
	INT8U HOps;

	INT32U Xid;
	
	INT16U Secs;
	INT16U Flags;

	INT32U Ciaddr;

	INT32U Yiaddr;

	INT32U Siaddr;

	INT32U Giaddr;

	INT8U Chaddr[DHCPS_HADDR_SIZE];

	INT8U Sname[DHCPS_SNAME_SIZE];

	INT8U File[DHCPS_FILENAME_SIZE];

	INT32U Magic;

	INT8U Options[DHCPS_OPTIONS_LEN - 4];
}DHCP_MSG, *PDHCP_MSG;

INT8S DHCPS_Start(struct netif *Netif);
void DHCPS_Stop(void);
INT8S DHCPS_ClientDelete(INT8U * MacAddr);
void DHCPS_RecvCb(void *Arg, struct udp_pcb *Pcb, struct pbuf *P, ip_addr_t *Addr, INT16U Port);
void DHCPS_SetDns(INT8U numdns, INT32U dns);
ip_addr_t *DHCPS_GetIpByMac(const INT8U *mac_addr);
INT8U *DHCPS_GetMacByIp(const ip_addr_t *ipaddr);
#endif
#endif /* __DHCP_SERVER_H_175137__ */
 
