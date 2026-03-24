#!/bin/sh

LWIP_MAKEDEFS="sdk/OpenECR6600/components/lwip/Make.defs"
LWIPOPTS="sdk/OpenECR6600/components/lwip/contrib/port/lwipopts.h"

if [ -f "$LWIPOPTS" ]; then
	echo "PREBUILD ECR6600: enabling lwIP mDNS options"

	if grep -q '^[[:space:]]*#define LWIP_MDNS_RESPONDER' "$LWIPOPTS"; then
		sed -i 's/^[[:space:]]*#define LWIP_MDNS_RESPONDER.*/#define LWIP_MDNS_RESPONDER             1/' "$LWIPOPTS"
	else
		sed -i '/^[[:space:]]*#define LWIP_IGMP[[:space:]]\+1/a #define LWIP_MDNS_RESPONDER             1' "$LWIPOPTS"
	fi

	if grep -q '^[[:space:]]*#define LWIP_NUM_NETIF_CLIENT_DATA' "$LWIPOPTS"; then
		sed -i 's/^[[:space:]]*#define LWIP_NUM_NETIF_CLIENT_DATA.*/#define LWIP_NUM_NETIF_CLIENT_DATA      1/' "$LWIPOPTS"
	else
		sed -i '/^[[:space:]]*#define LWIP_IGMP[[:space:]]\+1/a #define LWIP_NUM_NETIF_CLIENT_DATA      1' "$LWIPOPTS"
	fi
else
	echo "PREBUILD ECR6600: WARN lwipopts not found: $LWIPOPTS"
fi

if [ -f "$LWIP_MAKEDEFS" ]; then
	if grep -q 'src/apps/mdns' "$LWIP_MAKEDEFS"; then
		echo "PREBUILD ECR6600: lwIP mdns source path already present"
	else
		echo "PREBUILD ECR6600: adding lwIP mdns source path"
		sed -i '/:lwip\/lwip-2.1.0\/src\/apps\/sntp \\/a \            :lwip/lwip-2.1.0/src/apps/mdns \\' "$LWIP_MAKEDEFS"
	fi

	if grep -q '[[:space:]]mdns\.c' "$LWIP_MAKEDEFS"; then
		echo "PREBUILD ECR6600: mdns.c already present in lwIP source list"
	else
		echo "PREBUILD ECR6600: adding mdns.c to lwIP source list"
		sed -i 's/mqtt\.c sntp\.c \\/mqtt.c sntp.c mdns.c \\/' "$LWIP_MAKEDEFS"
	fi
else
	echo "PREBUILD ECR6600: WARN lwip Make.defs not found: $LWIP_MAKEDEFS"
fi
