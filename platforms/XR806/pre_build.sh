#!/bin/sh

set_mdns_lwipopts() {
	LWIPOPTS="$1"

	if [ ! -f "$LWIPOPTS" ]; then
		echo "PREBUILD XR806: WARN lwipopts not found: $LWIPOPTS"
		return
	fi

	echo "PREBUILD XR806: enabling lwIP mDNS options in $LWIPOPTS"

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
}

set_mdns_lwip_makefile() {
	LWIP_MAKEFILE="$1"

	if [ ! -f "$LWIP_MAKEFILE" ]; then
		echo "PREBUILD XR806: WARN lwip Makefile not found: $LWIP_MAKEFILE"
		return
	fi

	echo "PREBUILD XR806: enabling lwIP mDNS sources in $LWIP_MAKEFILE"

	if grep -q '^[[:space:]]*SRCS[[:space:]]*+=[[:space:]]*src/apps/mdns/mdns' "$LWIP_MAKEFILE"; then
		return
	fi

	if grep -q 'src/apps/mqtt/mqtt' "$LWIP_MAKEFILE"; then
		sed -i '/src\/apps\/mqtt\/mqtt/a SRCS += src/apps/mdns/mdns' "$LWIP_MAKEFILE"
	else
		sed -i '/^[[:space:]]*SRCS[[:space:]]*:=/a SRCS += src/apps/mdns/mdns' "$LWIP_MAKEFILE"
	fi
}

set_mdns_lwipopts "sdk/OpenXR806/include/net/lwip-2.1.2/lwipopts.h"
set_mdns_lwipopts "sdk/OpenXR806/include/net/lwip-2.0.3/lwipopts.h"
set_mdns_lwip_makefile "sdk/OpenXR806/src/net/lwip-2.1.2/Makefile"
set_mdns_lwip_makefile "sdk/OpenXR806/src/net/lwip-2.0.3/Makefile"
