/**************************************************************************
 * File Name                  : dns_server.h
 * Author                     :
 * Version                    :
 * Date                       :
 * Description                :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#ifndef __DNS_SERVER_H_175233__
#define __DNS_SERVER_H_175233__

#if TLS_CONFIG_AP
#define DNSS_ERR_SUCCESS      0
#define DNSS_ERR_LINKDOWN      1
#define DNSS_ERR_PARAM      2
#define DNSS_ERR_MEM      3

#define DNS_DEFAULT_TTL      3600 /* a day. */

#define DNS_SERVER_PORT      53

/* DNS field TYPE used for "Resource Records". */
#define DNS_RRTYPE_A              1     /* a host address. */
#define DNS_RRTYPE_NS             2     /* an authoritative name server. */
#define DNS_RRTYPE_MD             3     /* a mail destination (Obsolete - use MX). */
#define DNS_RRTYPE_MF             4     /* a mail forwarder (Obsolete - use MX) */
#define DNS_RRTYPE_CNAME          5     /* the canonical name for an alias. */
#define DNS_RRTYPE_SOA            6     /* marks the start of a zone of authority. */
#define DNS_RRTYPE_MB             7     /* a mailbox domain name (EXPERIMENTAL). */
#define DNS_RRTYPE_MG             8     /* a mail group member (EXPERIMENTAL). */
#define DNS_RRTYPE_MR             9     /* a mail rename domain name (EXPERIMENTAL). */
#define DNS_RRTYPE_NULL           10    /* a null RR (EXPERIMENTAL). */
#define DNS_RRTYPE_WKS            11    /* a well known service description. */
#define DNS_RRTYPE_PTR            12    /* a domain name pointer. */
#define DNS_RRTYPE_HINFO          13    /* host information. */
#define DNS_RRTYPE_MINFO          14    /* mailbox or mail list information. */
#define DNS_RRTYPE_MX             15    /* mail exchange. */
#define DNS_RRTYPE_TXT            16    /* text strings. */

/* DNS field CLASS used for "Resource Records". */
#define DNS_RRCLASS_IN            1     /* the Internet */
#define DNS_RRCLASS_CS            2     /* the CSNET class (Obsolete - used only for examples in some obsolete RFCs).*/
#define DNS_RRCLASS_CH            3     /* the CHAOS class. */
#define DNS_RRCLASS_HS            4     /* Hesiod [Dyer 87].*/
#define DNS_RRCLASS_FLUSH         0x800 /* Flush bit. */

/* DNS protocol flags */
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

#define DNS_NAME_OFFSET      0xC000

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

#ifdef INT32S
#undef INT32S
#endif
typedef signed int INT32S;

typedef struct __DNS_SERVER
{
	struct udp_pcb * Socket;
	INT8U DnsName[32];
	/* Attention!!! MUST BE __align(4) */
	ip_addr_t HostIp;
}DNS_SERVER;

typedef struct __DNS_HEADER
{
	INT16U TansactionId;
	INT8U DnsFlag1;
	INT8U DnsFlag2;
	INT16U Quentions;
	INT16U AnswerRR;
	INT16U AuthorityRR;
	INT16U AdditionalRR;
}DNS_HEADER, *PDNS_HEADER;

typedef struct _DNS_QUERY
{
	INT16U Type;
	INT16U Class;
}DNS_QUERY, *PDNS_QUERY;

typedef struct __DNS_ANSWER
{
	INT16U Type;
	INT16U Class;
	INT32U Ttl;
}DNS_ANSWER, *PDNS_ANSWER;

void DNSS_RecvCb(void *Arg, struct udp_pcb *Pcb, struct pbuf *P, ip_addr_t *Addr, INT16U Port);
INT8S DNSS_Config(INT8U * DnsName);
INT8S DNSS_Start(struct netif *Netif, INT8U * DnsName);
void DNSS_Stop(void);
#endif
#endif /* __DNS_SERVER_H_175233__ */
 
