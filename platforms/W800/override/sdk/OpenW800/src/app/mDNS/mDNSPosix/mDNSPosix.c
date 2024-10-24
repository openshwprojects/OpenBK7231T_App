/*
 * Copyright (c) 2002-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
 * Formatting notes:
 * This code follows the "Whitesmiths style" C indentation rules. Plenty of discussion
 * on C indentation can be found on the web, such as <http://www.kafejo.com/komp/1tbs.htm>,
 * but for the sake of brevity here I will say just this: Curly braces are not syntactially
 * part of an "if" statement; they are the beginning and ending markers of a compound statement;
 * therefore common sense dictates that if they are part of a compound statement then they
 * should be indented to the same level as everything else in that compound statement.
 * Indenting curly braces at the same level as the "if" implies that curly braces are
 * part of the "if", which is false. (This is as misleading as people who write "char* x,y;"
 * thinking that variables x and y are both of type "char*" -- and anyone who doesn't
 * understand why variable y is not of type "char*" just proves the point that poor code
 * layout leads people to unfortunate misunderstandings about how the C language really works.)

 Change History (most recent first):

 $Log: mDNSPosix.c,v $
 Revision 1.2  2005/11/21 13:51:48  shiro
 *** empty log message ***

 Revision 1.1.1.1  2005/07/23 13:57:05  shiro
 raop_play project

 Revision 1.1.2.1  2004/09/18 03:29:20  shiro
 *** empty log message ***

 Revision 1.25.2.1  2004/04/09 17:57:31  cheshire
 Make sure to set the TxAndRx field so that duplicate suppression works correctly

 Revision 1.25  2003/10/30 19:25:49  cheshire
 Fix signed/unsigned warning on certain compilers

 Revision 1.24  2003/08/18 23:12:23  cheshire
 <rdar://problem/3382647> mDNSResponder divide by zero in mDNSPlatformTimeNow()

 Revision 1.23  2003/08/12 19:56:26  cheshire
 Update to APSL 2.0

 Revision 1.22  2003/08/06 18:46:15  cheshire
 LogMsg() errors are serious -- always report them to stderr, regardless of debugging level

 Revision 1.21  2003/08/06 18:20:51  cheshire
 Makefile cleanup

 Revision 1.20  2003/08/05 23:56:26  cheshire
 Update code to compile with the new mDNSCoreReceive() function that requires a TTL
 (Right now mDNSPosix.c just reports 255 -- we should fix this)

 Revision 1.19  2003/07/19 03:15:16  cheshire
 Add generic MemAllocate/MemFree prototypes to mDNSPlatformFunctions.h,
 and add the obvious trivial implementations to each platform support layer

 Revision 1.18  2003/07/14 18:11:54  cheshire
 Fix stricter compiler warnings

 Revision 1.17  2003/07/13 01:08:38  cheshire
 There's not much point running mDNS over a point-to-point link; exclude those

 Revision 1.16  2003/07/02 21:19:59  cheshire
 <rdar://problem/3313413> Update copyright notices, etc., in source code comments

 Revision 1.15  2003/06/18 05:48:41  cheshire
 Fix warnings

 Revision 1.14  2003/05/26 03:21:30  cheshire
 Tidy up address structure naming:
 mDNSIPAddr         => mDNSv4Addr (for consistency with mDNSv6Addr)
 mDNSAddr.addr.ipv4 => mDNSAddr.ip.v4
 mDNSAddr.addr.ipv6 => mDNSAddr.ip.v6

 Revision 1.13  2003/05/26 03:01:28  cheshire
 <rdar://problem/3268904> sprintf/vsprintf-style functions are unsafe; use snprintf/vsnprintf instead

 Revision 1.12  2003/05/21 03:49:18  cheshire
 Fix warning

 Revision 1.11  2003/05/06 00:00:50  cheshire
 <rdar://problem/3248914> Rationalize naming of domainname manipulation functions

 Revision 1.10  2003/04/25 01:45:57  cheshire
 <rdar://problem/3240002> mDNS_RegisterNoSuchService needs to include a host name

 Revision 1.9  2003/03/20 21:10:31  cheshire
 Fixes done at IETF 56 to make mDNSProxyResponderPosix run on Solaris

 Revision 1.8  2003/03/15 04:40:38  cheshire
 Change type called "mDNSOpaqueID" to the more descriptive name "mDNSInterfaceID"

 Revision 1.7  2003/03/13 03:46:21  cheshire
 Fixes to make the code build on Linux

 Revision 1.6  2003/03/08 00:35:56  cheshire
 Switched to using new "mDNS_Execute" model (see "mDNSCore/Implementer Notes.txt")

 Revision 1.5  2002/12/23 22:13:31  jgraessl
 Reviewed by: Stuart Cheshire
 Initial IPv6 support for mDNSResponder.

 Revision 1.4  2002/09/27 01:47:45  cheshire
 Workaround for Linux 2.0 systems that don't have IP_PKTINFO

 Revision 1.3  2002/09/21 20:44:53  zarzycki
 Added APSL info

 Revision 1.2  2002/09/19 21:25:36  cheshire
 mDNS_snprintf() doesn't need to be in a separate file

 Revision 1.1  2002/09/17 06:24:34  cheshire
 First checkin
*/

#include "mDNSClientAPI.h"           // Defines the interface provided to the client layer above
#include "mDNSPlatformFunctions.h"   // Defines the interface to the supporting layer below
#include "mDNSPosix.h"				 // Defines the specific types needed to run mDNS on this platform
#include <assert.h>
//#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
//#include <unistd.h>
#include <stdarg.h>
//#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/uio.h>
//#include <netinet/in.h>
#include "utils.h"
#include "APSCommonServices.h"
#include "lwip/sockets.h"
#include "wm_sockets.h"
#include "lwip/inet.h"
#include "lwip/igmp.h"
#include <time.h>                   // platform support for UTC time
#include "wm_osal.h"
#include "utils.h"
//#include "wm_overlay_common.h"

//#include "mDNSUNP.h"

// ***************************************************************************
// Structures

// PosixNetworkInterface is a record extension of the core NetworkInterfaceInfo
// type that supports extra fields needed by the Posix platform.
//
// IMPORTANT: coreIntf must be the first field in the structure because
// we cast between pointers to the two different types regularly.

typedef struct PosixNetworkInterface PosixNetworkInterface;

struct PosixNetworkInterface
{
	NetworkInterfaceInfo    coreIntf;
	const char *            intfName;
	PosixNetworkInterface * aliasIntf;
	int                     index;
	int                     multicastSocket;
	int                     multicastSocketv6;
};

// ***************************************************************************
// Globals (for debugging)

static int num_registered_interfaces = 0;
//static int num_pkts_accepted = 0;
//static int num_pkts_rejected = 0;

// ***************************************************************************
// Functions

int gMDNSPlatformPosixVerboseLevel = 2;

extern u8 *wpa_supplicant_get_mac(void);
#if 1
// Note, this uses mDNS_vsnprintf instead of standard "vsnprintf", because mDNS_vsnprintf knows
// how to print special data types like IP addresses and length-prefixed domain names
mDNSexport void debugf_(const char *format, ...)
{
	unsigned char buffer[512];
	va_list ptr;
	va_start(ptr,format);
	buffer[mDNS_vsnprintf((char *)buffer, sizeof(buffer), format, ptr)] = 0;
	va_end(ptr);
	if (gMDNSPlatformPosixVerboseLevel >= 1)
		printf("%s\n", buffer);
	//fflush(stderr);
}

mDNSexport void verbosedebugf_(const char *format, ...)
{
	unsigned char buffer[512];
	va_list ptr;
	va_start(ptr,format);
	buffer[mDNS_vsnprintf((char *)buffer, sizeof(buffer), format, ptr)] = 0;
	va_end(ptr);
	if (gMDNSPlatformPosixVerboseLevel >= 2)
		printf("%s\n", buffer);
	//fflush(stderr);
}

mDNSexport void LogMsg_(const char *format, ...)
{
	unsigned char buffer[512];
	va_list ptr;
	va_start(ptr,format);
	buffer[mDNS_vsnprintf((char *)buffer, sizeof(buffer), format, ptr)] = 0;
	va_end(ptr);
	printf("%s\n", buffer);
	//fflush(stderr);
}
#endif
#define PosixErrorToStatus(errNum) ((errNum) == 0 ? mStatus_NoError : mStatus_UnknownErr)

static int gethostname(char * hostname, int len)
{
	u8* mac = wpa_supplicant_get_mac();
	snprintf(hostname, len, "WM_Hos_%02X%02X", mac[4], mac[5]);
	return 0;//random_get_bytes(hostname, len);
}
static void SockAddrTomDNSAddr(const struct sockaddr *const sa, mDNSAddr *ipAddr, mDNSIPPort *ipPort)
{
	//LOG("SockAddrTomDNSAddr: sa_family %d\n", sa->sa_family);
	switch (sa->sa_family)
	{
	case AF_INET:
	{
		struct sockaddr_in* sin          = (struct sockaddr_in*)sa;
		ipAddr->type                     = mDNSAddrType_IPv4;
		ipAddr->ip.v4.NotAnInteger       = sin->sin_addr.s_addr;
		if (ipPort) ipPort->NotAnInteger = sin->sin_port;
		break;
	}

#ifdef mDNSIPv6Support
	case AF_INET6:
	{
		struct sockaddr_in6* sin6        = (struct sockaddr_in6*)sa;
#ifndef NOT_HAVE_SA_LEN
		assert(sin6->sin6_len == sizeof(*sin6));
#endif		
		ipAddr->type                     = mDNSAddrType_IPv6;
		ipAddr->ip.v6                    = *(mDNSv6Addr*)&sin6->sin6_addr;
		if (ipPort) ipPort->NotAnInteger = sin6->sin6_port;
		break;
	}
#endif

	default:
		LOG("SockAddrTomDNSAddr: Uknown address family %d\n", sa->sa_family);
		ipAddr->type = mDNSAddrType_None;
		if (ipPort) ipPort->NotAnInteger = 0;
		break;
	}
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Send and Receive
#endif

// mDNS core calls this routine when it needs to send a packet.
mDNSexport mStatus mDNSPlatformSendUDP(const mDNS *const m, const DNSMessage *const msg, const mDNSu8 *const end,
				       mDNSInterfaceID InterfaceID, mDNSIPPort srcPort, const mDNSAddr *dst, mDNSIPPort dstPort)
{
	int                     err;
	struct sockaddr_storage to;
	PosixNetworkInterface * thisIntf;

	assert(m != NULL);
	assert(msg != NULL);
	assert(end != NULL);
	assert( (((char *) end) - ((char *) msg)) > 0 );
	assert(InterfaceID != 0); // Can't send from zero source address
	assert(srcPort.NotAnInteger != 0);     // Nor from a zero source port
	assert(dstPort.NotAnInteger != 0);     // Nor from a zero source port

	if (dst->type == mDNSAddrType_IPv4)
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)&to;
#ifndef NOT_HAVE_SA_LEN
		sin->sin_len            = sizeof(*sin);
#endif
		sin->sin_family         = AF_INET;
		sin->sin_port           = dstPort.NotAnInteger;
		sin->sin_addr.s_addr    = dst->ip.v4.NotAnInteger;
	}

#ifdef mDNSIPv6Support
	else if (dst->type == mDNSAddrType_IPv6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&to;
		mDNSPlatformMemZero(sin6, sizeof(*sin6));
#ifndef NOT_HAVE_SA_LEN
		sin6->sin6_len            = sizeof(*sin6);
#endif
		sin6->sin6_family         = AF_INET6;
		sin6->sin6_port           = dstPort.NotAnInteger;
		sin6->sin6_addr           = *(struct in6_addr*)&dst->ip.v6;
	}
#endif

	err = 0;
	thisIntf = (PosixNetworkInterface *)(InterfaceID);
	if (dst->type == mDNSAddrType_IPv4)
		err = sendto(thisIntf->multicastSocket, msg, (char*)end - (char*)msg, 0, (struct sockaddr *)&to, GET_SA_LEN(to));

#ifdef mDNSIPv6Support
	else if (dst->type == mDNSAddrType_IPv6)
		err = sendto(thisIntf->multicastSocketv6, msg, (char*)end - (char*)msg, 0, (struct sockaddr *)&to, GET_SA_LEN(to));
#endif

	if (err > 0) err = 0;
	else if (err < 0)
	{
		//LOG("mDNSPlatformSendUDP got error %d (%s) sending packet to %#a on interface %#a/%s/%d",
		//	      errno, strerror(errno), dst, &thisIntf->coreIntf.ip, thisIntf->intfName, thisIntf->index);
		LOG("mDNSPlatformSendUDP got err %d, dst->type %ld\n", err, dst->type);
	}
	return PosixErrorToStatus(err);
}

DNSMessage packet;
// This routine is called when the main loop detects that data is available on a socket.
static void SocketDataReady(mDNS *const m, PosixNetworkInterface *intf, int skt)
{
	mDNSAddr   senderAddr, destAddr;
	mDNSIPPort senderPort;
	ssize_t                 packetLen = 0;
	//struct my_in_pktinfo    packetInfo;
	struct sockaddr_storage from;
	socklen_t               fromLen;
	int                     flags;
//	mDNSu8					ttl;
//	mDNSBool                reject;
//	const mDNSInterfaceID InterfaceID = intf ? intf->coreIntf.InterfaceID : NULL;

	assert(m    != NULL);
	assert(intf != NULL);
	assert(skt  >= 0);

	fromLen = sizeof(from);
	flags   = 0;
	memset(&packet, 0, sizeof(DNSMessage));
#if 0
	packetLen = recvfrom_flags(skt, &packet, sizeof(packet), &flags, (struct sockaddr *) &from, &fromLen, &packetInfo);

#else
	packetLen = recvfrom(skt, &packet, sizeof(DNSMessage), flags, (struct sockaddr *) &from, &fromLen);
#endif
	if (packetLen >= 0)
	{
		SockAddrTomDNSAddr((struct sockaddr*)&from, &senderAddr, &senderPort);
#if 0
		SockAddrTomDNSAddr((struct sockaddr*)&packetInfo.ipi_addr, &destAddr, NULL);
#else
		{
			struct tls_ethif * ethif = tls_netif_get_ethif();
			destAddr.type = mDNSAddrType_IPv4;
			destAddr.ip.v4.NotAnInteger = ip_addr_get_ip4_u32(&ethif->ip_addr);
		}
#endif

		// If we have broken IP_RECVDSTADDR functionality (so far
		// I've only seen this on OpenBSD) then apply a hack to
		// convince mDNS Core that this isn't a spoof packet.
		// Basically what we do is check to see whether the
		// packet arrived as a multicast and, if so, set its
		// destAddr to the mDNS address.
		//
		// I must admit that I could just be doing something
		// wrong on OpenBSD and hence triggering this problem
		// but I'm at a loss as to how.
		//
		// If this platform doesn't have IP_PKTINFO or IP_RECVDSTADDR, then we have
		// no way to tell the destination address or interface this packet arrived on,
		// so all we can do is just assume it's a multicast

#if 0//HAVE_BROKEN_RECVDSTADDR || (!defined(IP_PKTINFO) && !defined(IP_RECVDSTADDR))
		if ( (destAddr.NotAnInteger == 0) && (flags & MSG_MCAST) )
		{
			destAddr.type = senderAddr.type;
			if      (senderAddr.type == mDNSAddrType_IPv4) destAddr.ip.v4 = AllDNSLinkGroup;
			else if (senderAddr.type == mDNSAddrType_IPv6) destAddr.ip.v6 = AllDNSLinkGroupv6;
		}
#endif

		// We only accept the packet if the interface on which it came
		// in matches the interface associated with this socket.
		// We do this match by name or by index, depending on which
		// information is available.  recvfrom_flags sets the name
		// to "" if the name isn't available, or the index to -1
		// if the index is available.  This accomodates the various
		// different capabilities of our target platforms.

#if 0
		reject = mDNSfalse;
		if      ( packetInfo.ipi_ifname[0] != 0 ) reject = (strcmp(packetInfo.ipi_ifname, intf->intfName) != 0);
		else if ( packetInfo.ipi_ifindex != -1 )  reject = (packetInfo.ipi_ifindex != intf->index);

		if (reject)
		{
			LOG("SocketDataReady ignored a packet from %#a to %#a on interface %s/%d expecting %#a/%s/%d",
				      &senderAddr, &destAddr, packetInfo.ipi_ifname, packetInfo.ipi_ifindex,
				      &intf->coreIntf.ip, intf->intfName, intf->index);
			packetLen = -1;
			num_pkts_rejected++;
			if (num_pkts_rejected > (num_pkts_accepted + 1) * (num_registered_interfaces + 1) * 2)
			{
				fprintf(stderr,
					"*** WARNING: Received %d packets; Accepted %d packets; Rejected %d packets because of interface mismatch\n",
					num_pkts_accepted + num_pkts_rejected, num_pkts_accepted, num_pkts_rejected);
				num_pkts_accepted = 0;
				num_pkts_rejected = 0;
			}
		}
		else
#endif
		{
#if 0
			int i;
			mDNSu8 * ptr = (mDNSu8 *)&packet;
			//LOG("SocketDataReady got a packet from %#a to %#a on interface %#a/%s/%d",
			//	      &senderAddr, &destAddr, &intf->coreIntf.ip, intf->intfName, intf->index);
			LOG("packetLen=%d\n", packetLen);
			for(i = 0; i < packetLen; i++)
			{
				if(ptr[i] < 127 && ptr[i] > 31)
					LOG("%c", ptr[i]);
				else
					LOG("%02X ", ptr[i]);
			}
			LOG("\n");
#endif
//			num_pkts_accepted++;
		}
	}

	if (packetLen >= 0 && packetLen < (ssize_t)sizeof(DNSMessageHeader))
	{
		LOG("SocketDataReady packet length (%d) too short", packetLen);
		packetLen = -1;
	}

	if (packetLen >= 0)
		mDNSCoreReceive(m, &packet, (mDNSu8 *)&packet + packetLen,
				&senderAddr, senderPort, &destAddr, MulticastDNSPort, intf->coreIntf.InterfaceID, 255);
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Init and Term
#endif

// On OS X this gets the text of the field labelled "Computer Name" in the Sharing Prefs Control Panel
// Other platforms can either get the information from the appropriate place,
// or they can alternatively just require all registering services to provide an explicit name
mDNSlocal void GetUserSpecifiedFriendlyComputerName(domainlabel *const namelabel)
{
	MakeDomainLabelFromLiteralString(namelabel, "Fill in Default Service Name Here");
}

// This gets the current hostname, truncating it at the first dot if necessary
mDNSlocal void GetUserSpecifiedRFC1034ComputerName(domainlabel *const namelabel)
{
	int len = 0;
	gethostname((char *)(&namelabel->c[1]), MAX_DOMAIN_LABEL);
	while (len < MAX_DOMAIN_LABEL && namelabel->c[len+1] && namelabel->c[len+1] != '.') len++;
	namelabel->c[0] = len;
}

// Searches the interface list looking for the named interface.
// Returns a pointer to if it found, or NULL otherwise.
static PosixNetworkInterface *SearchForInterfaceByName(mDNS *const m, const char *intfName)
{
	PosixNetworkInterface *intf;

	assert(m != NULL);
	assert(intfName != NULL);

	intf = (PosixNetworkInterface*)(m->HostInterfaces);
	while ( (intf != NULL) && (strcmp(intf->intfName, intfName) != 0) )
		intf = (PosixNetworkInterface *)(intf->coreIntf.next);

	return intf;
}

// Frees the specified PosixNetworkInterface structure. The underlying
// interface must have already been deregistered with the mDNS core.
static void FreePosixNetworkInterface(PosixNetworkInterface *intf)
{
	assert(intf != NULL);
	if (intf->intfName != NULL)        tls_mem_free((void *)intf->intfName);
	if (intf->multicastSocket   != -1) assert(close(intf->multicastSocket) == 0);
	if (intf->multicastSocketv6 != -1) assert(close(intf->multicastSocketv6) == 0);
	tls_mem_free(intf);
}

// Grab the first interface, deregister it, free it, and repeat until done.
static void ClearInterfaceList(mDNS *const m)
{
	assert(m != NULL);

	while (m->HostInterfaces)
	{
		PosixNetworkInterface *intf = (PosixNetworkInterface*)(m->HostInterfaces);
		mDNS_DeregisterInterface(m, &intf->coreIntf);
		if (gMDNSPlatformPosixVerboseLevel > 0) LOG("Deregistered interface %s\n", intf->intfName);
		FreePosixNetworkInterface(intf);
	}
	num_registered_interfaces = 0;
//	num_pkts_accepted = 0;
//	num_pkts_rejected = 0;
}

// Sets up a multicast send/receive socket for the specified
// port on the interface specified by the IP addrelss intfAddr.
static int SetupSocket(struct sockaddr *intfAddr, mDNSIPPort port, int interfaceIndex, int *sktPtr)
{
	int err = 0;
	static const int kOn = 1;
	static const int kIntTwoFiveFive = 255;
	static const unsigned char kByteTwoFiveFive = 255;
	
	(void) interfaceIndex;	// Unused
	assert(intfAddr != NULL);
	assert(sktPtr != NULL);
	assert(*sktPtr == -1);

	// Open the socket...
	if       (intfAddr->sa_family == AF_INET) *sktPtr = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef mDNSIPv6Support
	else if (intfAddr->sa_family == AF_INET6) *sktPtr = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
#endif
	else return EINVAL;

	if (*sktPtr < 0) { err = errno; LOG("socket"); }

	// ... with a shared UDP port
	if (err == 0)
	{
#if defined(SO_REUSEPORT)
		err = setsockopt(*sktPtr, SOL_SOCKET, SO_REUSEPORT, &kOn, sizeof(kOn));
#elif defined(SO_REUSEADDR)
		err = setsockopt(*sktPtr, SOL_SOCKET, SO_REUSEADDR, &kOn, sizeof(kOn));
#else
#error This platform has no way to avoid address busy errors on multicast.
#endif
		if (err < 0) { err = errno; LOG("setsockopt - SO_REUSExxxx err %d intfAddr->sa_family %d\n", err, intfAddr->sa_family); }
	}

	// We want to receive destination addresses and interface identifiers.
	if (intfAddr->sa_family == AF_INET)
	{
		struct ip_mreq imr;
		struct sockaddr_in bindAddr;
		if (err == 0)
		{
#if defined(IP_PKTINFO)									// Linux
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_PKTINFO, &kOn, sizeof(kOn));
			if (err < 0) { err = errno; LOG("setsockopt - IP_PKTINFO"); }
#elif defined(IP_RECVDSTADDR) || defined(IP_RECVIF)		// BSD and Solaris
#if defined(IP_RECVDSTADDR)
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_RECVDSTADDR, &kOn, sizeof(kOn));
			if (err < 0) { err = errno; LOG("setsockopt - IP_RECVDSTADDR"); }
#endif
#if defined(IP_RECVIF)
			if (err == 0)
			{
				err = setsockopt(*sktPtr, IPPROTO_IP, IP_RECVIF, &kOn, sizeof(kOn));
				if (err < 0) { err = errno; LOG("setsockopt - IP_RECVIF"); }
			}
#endif
#else
//#warning This platform has no way to get the destination interface information -- will only work for single-homed hosts
#endif
		}

		// Add multicast group membership on this interface
		if (err == 0)
		{
			imr.imr_multiaddr.s_addr = AllDNSLinkGroup.NotAnInteger;
			imr.imr_interface        = ((struct sockaddr_in*)intfAddr)->sin_addr;
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr));
			if (err < 0) { err = errno; LOG("setsockopt - IP_ADD_MEMBERSHIP"); }
		}

		// Specify outgoing interface too
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in*)intfAddr)->sin_addr, sizeof(struct in_addr));
			if (err < 0) { err = errno; LOG("setsockopt - IP_MULTICAST_IF"); }
		}

		// Per the mDNS spec, send unicast packets with TTL 255
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_TTL, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
			if (err < 0) { err = errno; LOG("setsockopt - IP_TTL"); }
		}

		// and multicast packets with TTL 255 too
		// There's some debate as to whether IP_MULTICAST_TTL is an int or a byte so we just try both.
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IP, IP_MULTICAST_TTL, &kByteTwoFiveFive, sizeof(kByteTwoFiveFive));
			if (err < 0 && errno == EINVAL)
				err = setsockopt(*sktPtr, IPPROTO_IP, IP_MULTICAST_TTL, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
			if (err < 0) { err = errno; LOG("setsockopt - IP_MULTICAST_TTL"); }
		}

		// And start listening for packets
		if (err == 0)
		{
			bindAddr.sin_family      = AF_INET;
			bindAddr.sin_port        = port.NotAnInteger;
			bindAddr.sin_addr.s_addr = INADDR_ANY; // Want to receive multicasts AND unicasts on this socket
			err = bind(*sktPtr, (struct sockaddr *) &bindAddr, sizeof(bindAddr));
			if (err < 0) { err = errno; LOG("bind"); /*fflush(stderr); */}
		}
	} // endif (intfAddr->sa_family == AF_INET)

#ifdef mDNSIPv6Support
	else if (intfAddr->sa_family == AF_INET6)
	{
#if 1
		struct ipv6_mreq imr6;
		struct sockaddr_in6 bindAddr6;
		if (err == 0)
		{
#if defined(IPV6_PKTINFO)
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_PKTINFO, &kOn, sizeof(kOn));
			if (err < 0) { err = errno; LOG("setsockopt - IPV6_PKTINFO"); }
#else
//#warning This platform has no way to get the destination interface information for IPv6 -- will only work for single-homed hosts
#endif
		}
		// Add multicast group membership on this interface
		if (err == 0)
		{
			imr6.ipv6mr_multiaddr       = *(const struct in6_addr*)&AllDNSLinkGroupv6;
			imr6.ipv6mr_interface       = interfaceIndex;
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_JOIN_GROUP, &imr6, sizeof(imr6));
			if (err < 0)
			{
				err = errno;
				LOG("setsockopt - IPV6_JOIN_GROUP err %d\n", err);
			}
		}
#if 0
		// Specify outgoing interface too
		if (err == 0)
		{
			u_int	multicast_if = interfaceIndex;
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_MULTICAST_IF, &multicast_if, sizeof(multicast_if));
			if (err < 0) { err = errno; LOG("setsockopt - IPV6_MULTICAST_IF"); }
		}
#endif
#else

		struct sockaddr_in6 bindAddr6;
		struct sockaddr_in6 ipv6_multiaddr;

		ipv6_multiaddr.sin6_addr = *(const struct in6_addr*)&AllDNSLinkGroupv6;
#endif
		// We want to receive only IPv6 packets on this socket.
		// Without this option, we may get IPv4 addresses as mapped addresses.
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_V6ONLY, &kOn, sizeof(kOn));
			if (err < 0) { err = errno; LOG("setsockopt - IPV6_V6ONLY"); }
		}
#if 0
		// Per the mDNS spec, send unicast packets with TTL 255
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
			if (err < 0) { err = errno; LOG("setsockopt - IPV6_UNICAST_HOPS"); }
		}

		// and multicast packets with TTL 255 too
		// There's some debate as to whether IPV6_MULTICAST_HOPS is an int or a byte so we just try both.
		if (err == 0)
		{
			err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &kByteTwoFiveFive, sizeof(kByteTwoFiveFive));
			if (err < 0 && errno == EINVAL)
				err = setsockopt(*sktPtr, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
			if (err < 0) { err = errno; LOG("setsockopt - IPV6_MULTICAST_HOPS"); }
		}
#endif
		// And start listening for packets
		if (err == 0)
		{
			mDNSPlatformMemZero(&bindAddr6, sizeof(bindAddr6));
#ifndef NOT_HAVE_SA_LEN
			bindAddr6.sin6_len         = sizeof(bindAddr6);
#endif			
			bindAddr6.sin6_family      = AF_INET6;
			bindAddr6.sin6_port        = port.NotAnInteger;
			bindAddr6.sin6_flowinfo    = 0;
//			bindAddr6.sin6_addr.s_addr = IN6ADDR_ANY_INIT; // Want to receive multicasts AND unicasts on this socket
			bindAddr6.sin6_addr = in6addr_any; // Want to receive multicasts AND unicasts on this socket
			bindAddr6.sin6_scope_id    = 0;
			err = bind(*sktPtr, (struct sockaddr *) &bindAddr6, sizeof(bindAddr6));
			if (err < 0) { err = errno; LOG("bind"); /*fflush(stderr); */}
		}
	} // endif (intfAddr->sa_family == AF_INET6)
#endif

	// Set the socket to non-blocking.
	if (err == 0)
	{
		err = fcntl(*sktPtr, F_GETFL, 0);
		if (err < 0) err = errno;
		else
		{
			err = fcntl(*sktPtr, F_SETFL, err | O_NONBLOCK);
			if (err < 0) err = errno;
		}
	}

	// Clean up
	if (err != 0 && *sktPtr != -1) { assert(close(*sktPtr) == 0); *sktPtr = -1; }
	assert( (err == 0) == (*sktPtr != -1) );
	return err;
}

// Creates a PosixNetworkInterface for the interface whose IP address is
// intfAddr and whose name is intfName and registers it with mDNS core.
static int SetupOneInterface(mDNS *const m, struct sockaddr *intfAddr, const char *intfName)
{
	int err = 0;
	PosixNetworkInterface *intf;
	PosixNetworkInterface *alias = NULL;

	assert(m != NULL);
	assert(intfAddr != NULL);
	assert(intfName != NULL);

	// Allocate the interface structure itself.
	intf = tls_mem_alloc(sizeof(*intf));
	if (intf == NULL) { assert(0); err = ENOMEM; }

	// And make a copy of the intfName.
	if (err == 0)
	{
		intf->intfName = (char *)strdup(intfName);
		if (intf->intfName == NULL) { assert(0); err = ENOMEM; }
	}

	if (err == 0)
	{
		// Set up the fields required by the mDNS core.
		SockAddrTomDNSAddr(intfAddr, &intf->coreIntf.ip, NULL);
		intf->coreIntf.Advertise = m->AdvertiseLocalAddresses;
		intf->coreIntf.TxAndRx   = mDNStrue;

		// Set up the extra fields in PosixNetworkInterface.
		assert(intf->intfName != NULL);         // intf->intfName already set up above
		intf->index                = 0;//if_nametoindex(intf->intfName);
		intf->multicastSocket      = -1;
		intf->multicastSocketv6    = -1;
		alias                      = SearchForInterfaceByName(m, intf->intfName);
		if (alias == NULL) alias   = intf;
		intf->coreIntf.InterfaceID = (mDNSInterfaceID)alias;

		if (alias != intf)
			LOG("SetupOneInterface: %s %#a is an alias of %#a", intfName, (double)(int)&intf->coreIntf.ip, (double)(int)&alias->coreIntf.ip);
	}
	// Set up the multicast socket
	if (err == 0)
	{
		if (alias->multicastSocket == -1 && intfAddr->sa_family == AF_INET)
			err = SetupSocket(intfAddr, MulticastDNSPort, intf->index, &alias->multicastSocket);
#ifdef mDNSIPv6Support
		else if (alias->multicastSocketv6 == -1 && intfAddr->sa_family == AF_INET6)
			err = SetupSocket(intfAddr, MulticastDNSPort, intf->index, &alias->multicastSocketv6);
#endif
	}

	// The interface is all ready to go, let's register it with the mDNS core.
	if (err == 0)
		err = mDNS_RegisterInterface(m, &intf->coreIntf);

	// Clean up.
	if (err == 0)
	{
		num_registered_interfaces++;
		LOG("SetupOneInterface: %s %#a Registered", intf->intfName, (double)(int)&intf->coreIntf.ip);
		if (gMDNSPlatformPosixVerboseLevel > 0)
			LOG("Registered interface %s\n", intf->intfName);
	}
	else
	{
		// Use intfName instead of intf->intfName in the next line to avoid dereferencing NULL.
		LOG("SetupOneInterface: %s %#a failed to register %d", intfName, (double)(int)&intf->coreIntf.ip, err);
		if (intf) { FreePosixNetworkInterface(intf); intf = NULL; }
	}

	assert( (err == 0) == (intf != NULL) );

	return err;
}

static int SetupInterfaceList(mDNS *const m)
{
	int             err            = 0;
#if 0
	mDNSBool        foundav4       = mDNSfalse;
	struct ifi_info *intfList      = get_ifi_info(AF_INET, mDNStrue);
	struct ifi_info *firstLoopback = NULL;

	assert(m != NULL);
	LOG("SetupInterfaceList");

	if (intfList == NULL) err = ENOENT;

#ifdef mDNSIPv6Support
	if (err == 0)		/* Link the IPv6 list to the end of the IPv4 list */
	{
		struct ifi_info **p = &intfList;
		while (*p) p = &(*p)->ifi_next;
		*p = get_ifi_info(AF_INET6, mDNStrue);
	}
#endif

	if (err == 0)
	{
		struct ifi_info *i = intfList;
		while (i)
		{
			if (     ((i->ifi_addr->sa_family == AF_INET)
#ifdef mDNSIPv6Support
				  || (i->ifi_addr->sa_family == AF_INET6)
#endif
					 ) &&  (i->ifi_flags & IFF_UP) && !(i->ifi_flags & IFF_POINTOPOINT) )
			{
				if (i->ifi_flags & IFF_LOOPBACK)
				{
					if (firstLoopback == NULL)
						firstLoopback = i;
				}
				else
				{
					if (SetupOneInterface(m, i->ifi_addr, i->ifi_name) == 0)
						if (i->ifi_addr->sa_family == AF_INET)
							foundav4 = mDNStrue;
				}
			}
			i = i->ifi_next;
		}

		// If we found no normal interfaces but we did find a loopback interface, register the
		// loopback interface.  This allows self-discovery if no interfaces are configured.
		// Temporary workaround: Multicast loopback on IPv6 interfaces appears not to work.
		// In the interim, we skip loopback interface only if we found at least one v4 interface to use
		// if ( (m->HostInterfaces == NULL) && (firstLoopback != NULL) )
		if ( !foundav4 && firstLoopback )
			(void) SetupOneInterface(m, firstLoopback->ifi_addr, firstLoopback->ifi_name);
	}

	// Clean up.
	if (intfList != NULL) free_ifi_info(intfList);
#else
	struct tls_ethif * ethif = tls_netif_get_ethif();
	if(ethif->status){
		struct sockaddr_in sin_ip; 
		struct sockaddr_in sin_mask;
		memset(&sin_ip, 0, sizeof(struct sockaddr_in));
		memset(&sin_mask, 0, sizeof(struct sockaddr_in));
		sin_ip.sin_family = AF_INET;
		sin_ip.sin_addr.s_addr = ip_addr_get_ip4_u32(&ethif->ip_addr);
		
		sin_mask.sin_family = AF_INET;
		sin_mask.sin_addr.s_addr = ip_addr_get_ip4_u32(&ethif->netmask);
		
		err = SetupOneInterface(m, (struct sockaddr*)&sin_ip, "netif0");
#ifdef mDNSIPv6Support
	{
		struct sockaddr_in6 sin6_ip; 
		memset(&sin6_ip, 0, sizeof(struct sockaddr_in6));
		sin6_ip.sin6_family = AF_INET6;
#ifndef NOT_HAVE_SA_LEN
		sin6_ip.sin6_len = sizeof(struct sockaddr_in6);
#endif
		MEMCPY((char *)&sin6_ip.sin6_addr.un.u32_addr, (char *)&ethif->ip6_addr[0], 16);
		LOG("start setup ipv6 interface\n");
		err = SetupOneInterface(m, (struct sockaddr*)&sin6_ip, "netif0");
		LOG("setup ipv6 interface err %d\n", err);
	}
#endif
	}
#endif
	return err;
}

// mDNS core calls this routine to initialise the platform-specific data.
mDNSexport mStatus mDNSPlatformInit(mDNS *const m)
{
	int err;
	assert(m != NULL);

	// Tell mDNS core the names of this machine.

	// Set up the nice label
	m->nicelabel.c[0] = 0;
	GetUserSpecifiedFriendlyComputerName(&m->nicelabel);
	if (m->nicelabel.c[0] == 0) MakeDomainLabelFromLiteralString(&m->nicelabel, "Macintosh");

	// Set up the RFC 1034-compliant label
	m->hostlabel.c[0] = 0;
	GetUserSpecifiedRFC1034ComputerName(&m->hostlabel);
	if (m->hostlabel.c[0] == 0) MakeDomainLabelFromLiteralString(&m->hostlabel, "Macintosh");

	mDNS_GenerateFQDN(m);

	// Tell mDNS core about the network interfaces on this machine.
	err = SetupInterfaceList(m);

	// We don't do asynchronous initialization on the Posix platform, so by the time
	// we get here the setup will already have succeeded or failed.  If it succeeded,
	// we should just call mDNSCoreInitComplete() immediately.
	if (err == 0)
		mDNSCoreInitComplete(m, mStatus_NoError);

	return PosixErrorToStatus(err);
}

// mDNS core calls this routine to clean up the platform-specific data.
// In our case all we need to do is to tear down every network interface.
mDNSexport void mDNSPlatformClose(mDNS *const m)
{
	assert(m != NULL);
	ClearInterfaceList(m);
}

extern mStatus mDNSPlatformPosixRefreshInterfaceList(mDNS *const m)
{
	int err;
	ClearInterfaceList(m);
	err = SetupInterfaceList(m);
	return PosixErrorToStatus(err);
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Locking
#endif

// On the Posix platform, locking is a no-op because we only ever enter
// mDNS core on the main thread.

// mDNS core calls this routine when it wants to prevent
// the platform from reentering mDNS core code.
mDNSexport void    mDNSPlatformLock   (const mDNS *const m)
{
	(void) m;	// Unused
}

// mDNS core calls this routine when it release the lock taken by
// mDNSPlatformLock and allow the platform to reenter mDNS core code.
mDNSexport void    mDNSPlatformUnlock (const mDNS *const m)
{
	(void) m;	// Unused
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Strings
#endif

// mDNS core calls this routine to copy C strings.
// On the Posix platform this maps directly to the ANSI C strcpy.
mDNSexport void    mDNSPlatformStrCopy(const void *src,       void *dst)
{
	strcpy((char *)dst, (char *)src);
}

// mDNS core calls this routine to get the length of a C string.
// On the Posix platform this maps directly to the ANSI C strlen.
mDNSexport mDNSu32  mDNSPlatformStrLen (const void *src)
{
	return strlen((char*)src);
}

// mDNS core calls this routine to copy memory.
// On the Posix platform this maps directly to the ANSI C memcpy.
mDNSexport void    mDNSPlatformMemCopy(const void *src,       void *dst, mDNSu32 len)
{
	memcpy(dst, src, len);
}

// mDNS core calls this routine to test whether blocks of memory are byte-for-byte
// identical. On the Posix platform this is a simple wrapper around ANSI C memcmp.
mDNSexport mDNSBool mDNSPlatformMemSame(const void *src, const void *dst, mDNSu32 len)
{
	return memcmp(dst, src, len) == 0;
}

// mDNS core calls this routine to clear blocks of memory.
// On the Posix platform this is a simple wrapper around ANSI C memset.
mDNSexport void    mDNSPlatformMemZero(                       void *dst, mDNSu32 len)
{
	memset(dst, 0, len);
}

mDNSexport void *  mDNSPlatformMemAllocate(mDNSu32 len) { return(tls_mem_alloc(len)); }
mDNSexport void    mDNSPlatformMemFree    (void *mem)   { tls_mem_free(mem); }

mDNSexport mDNSs32  mDNSPlatformOneSecond = 1024;

mDNSexport mStatus mDNSPlatformTimeInit(mDNSs32 *timenow)
{
	// No special setup is required on Posix -- we just use gettimeofday();
	// This is not really safe, because gettimeofday can go backwards if the user manually changes the date or time
	// We should find a better way to do this
	*timenow = mDNSPlatformTimeNow();
	return(mStatus_NoError);
}
mDNSexport mDNSs32  mDNSPlatformTimeNow()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	// tv.tv_sec is seconds since 1st January 1970 (GMT, with no adjustment for daylight savings time)
	// tv.tv_usec is microseconds since the start of this second (i.e. values 0 to 999999)
	// We use the lower 22 bits of tv.tv_sec for the top 22 bits of our result
	// and we multiply tv.tv_usec by 16 / 15625 to get a value in the range 0-1023 to go in the bottom 10 bits.
	// This gives us a proper modular (cyclic) counter that has a resolution of roughly 1ms (actually 1/1024 second)
	// and correctly cycles every 2^22 seconds (4194304 seconds = approx 48 days).
	return( (tv.tv_sec << 10) | (tv.tv_usec * 16 / 15625) );
}
static mDNSs32 last_report_time = 0;
mDNSexport void mDNSPosixGetFDSet(mDNS *const m, int *nfds, fd_set *readfds, struct timeval *timeout)
{
	mDNSs32 ticks;
	struct timeval interval;

	// 1. Call mDNS_Execute() to let mDNSCore do what it needs to do
	mDNSs32 nextevent = mDNS_Execute(m);

	// 2. Build our list of active file descriptors
	PosixNetworkInterface *info = (PosixNetworkInterface *)(m->HostInterfaces);
	while (info)
	{
		if (info->multicastSocket != -1)
		{
			if (*nfds < info->multicastSocket + 1)
				*nfds = info->multicastSocket + 1;
			FD_SET(info->multicastSocket, readfds);
		}
		if (info->multicastSocketv6 != -1)
		{
			if (*nfds < info->multicastSocketv6 + 1)
				*nfds = info->multicastSocketv6 + 1;
			FD_SET(info->multicastSocketv6, readfds);
		}
		info = (PosixNetworkInterface *)(info->coreIntf.next);
	}

	// 3. Calculate the time remaining to the next scheduled event (in struct timeval format)
	ticks = nextevent - mDNSPlatformTimeNow();
	if (ticks < 1) ticks = 1;
	interval.tv_sec  = ticks >> 10;						// The high 22 bits are seconds
	interval.tv_usec = ((ticks & 0x3FF) * 15625) / 16;	// The low 10 bits are 1024ths

	// 4. If client's proposed timeout is more than what we want, then reduce it
	if (timeout->tv_sec > interval.tv_sec ||
	    (timeout->tv_sec == interval.tv_sec && timeout->tv_usec > interval.tv_usec))
		*timeout = interval;
	if(timeout->tv_sec == 0 || timeout->tv_sec > 3)
		timeout->tv_sec = 3;
	if(tls_os_get_time() - last_report_time > (HZ * 10))
	{
		last_report_time = tls_os_get_time();
		//LOG("\n@@@@@ REPORT GROUPS !!!!!\n");
		igmp_report_groups(tls_get_netif());
	}
}

mDNSexport void mDNSPosixProcessFDSet(mDNS *const m, fd_set *readfds)
{
	PosixNetworkInterface *info;
	assert(m       != NULL);
	assert(readfds != NULL);
	info = (PosixNetworkInterface *)(m->HostInterfaces);
	while (info)
	{
		if (info->multicastSocket != -1 && FD_ISSET(info->multicastSocket, readfds))
		{
			FD_CLR(info->multicastSocket, readfds);
			SocketDataReady(m, info, info->multicastSocket);
		}
		if (info->multicastSocketv6 != -1 && FD_ISSET(info->multicastSocketv6, readfds))
		{
			FD_CLR(info->multicastSocketv6, readfds);
			SocketDataReady(m, info, info->multicastSocketv6);
		}
		info = (PosixNetworkInterface *)(info->coreIntf.next);
	}
}
