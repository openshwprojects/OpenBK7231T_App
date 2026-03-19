#!/bin/sh

LWIPOPTS="sdk/OpenXR809/include/net/lwip-2.0.3/lwipopts.h"
LWIP_MAKEFILE="sdk/OpenXR809/src/net/lwip-2.0.3/Makefile"
CONFIG_MK="sdk/OpenXR809/config.mk"

if [ -f "$CONFIG_MK" ]; then
	echo "PREBUILD XR809: selecting lwIP 2.x (disable lwIP v1)"
	if grep -q '^[[:space:]]*__CONFIG_LWIP_V1[[:space:]]*\?=' "$CONFIG_MK"; then
		sed -i 's/^[[:space:]]*__CONFIG_LWIP_V1[[:space:]]*\?=.*/__CONFIG_LWIP_V1 ?= n/' "$CONFIG_MK"
	else
		echo "__CONFIG_LWIP_V1 ?= n" >> "$CONFIG_MK"
	fi
else
	echo "PREBUILD XR809: WARN config.mk not found: $CONFIG_MK"
fi

if [ -f "$LWIPOPTS" ]; then
	echo "PREBUILD XR809: enabling lwIP mDNS options"

	if grep -q '^[[:space:]]*#define LWIP_MDNS_RESPONDER' "$LWIPOPTS"; then
		sed -i 's/^[[:space:]]*#define LWIP_MDNS_RESPONDER.*/#define LWIP_MDNS_RESPONDER             1/' "$LWIPOPTS"
	else
		sed -i '/^[[:space:]]*#define LWIP_NUM_NETIF_CLIENT_DATA/a #define LWIP_MDNS_RESPONDER             1' "$LWIPOPTS"
	fi

	if grep -q '^[[:space:]]*#define LWIP_NUM_NETIF_CLIENT_DATA' "$LWIPOPTS"; then
		sed -i 's/^[[:space:]]*#define LWIP_NUM_NETIF_CLIENT_DATA.*/#define LWIP_NUM_NETIF_CLIENT_DATA      1/' "$LWIPOPTS"
	else
		sed -i '/^[[:space:]]*#define LWIP_NETIF_TX_SINGLE_PBUF/a #define LWIP_NUM_NETIF_CLIENT_DATA      1' "$LWIPOPTS"
	fi
else
	echo "PREBUILD XR809: WARN lwipopts not found: $LWIPOPTS"
fi

if [ -f "$LWIP_MAKEFILE" ]; then
	echo "PREBUILD XR809: enabling lwIP mDNS sources"

	if ! grep -q '^[[:space:]]*SRCS[[:space:]]*+=[[:space:]]*src/apps/mdns/mdns' "$LWIP_MAKEFILE"; then
		if grep -q 'src/apps/mqtt/mqtt' "$LWIP_MAKEFILE"; then
			sed -i '/src\/apps\/mqtt\/mqtt/a SRCS += src/apps/mdns/mdns' "$LWIP_MAKEFILE"
		else
			sed -i '/^[[:space:]]*SRCS[[:space:]]*:=/a SRCS += src/apps/mdns/mdns' "$LWIP_MAKEFILE"
		fi
	fi
else
	echo "PREBUILD XR809: WARN lwip Makefile not found: $LWIP_MAKEFILE"
fi
