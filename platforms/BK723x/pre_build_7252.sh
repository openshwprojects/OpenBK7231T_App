#!/bin/sh

BEKEN_SRC_MK="sdk/beken_freertos_sdk/beken378/beken_src.mk"
LWIPOPTS_20="sdk/beken_freertos_sdk/beken378/func/lwip_intf/lwip-2.0.2/port/lwipopts.h"
LWIPOPTS_21="sdk/beken_freertos_sdk/beken378/func/lwip_intf/lwip-2.1.3/port/lwipopts.h"

patch_lwipopts() {
	LWIPOPTS="$1"

	if [ ! -f "$LWIPOPTS" ]; then
		return
	fi

	echo "PREBUILD BK7252: enabling lwIP mDNS options in $LWIPOPTS"

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
}

patch_lwipopts "$LWIPOPTS_20"
patch_lwipopts "$LWIPOPTS_21"

if [ ! -f "$LWIPOPTS_20" ] && [ ! -f "$LWIPOPTS_21" ]; then
	echo "PREBUILD BK7252: WARN no lwipopts.h found for known lwIP paths"
fi

if [ -f "$BEKEN_SRC_MK" ]; then
	if grep -q 'src/apps/mdns/mdns.c' "$BEKEN_SRC_MK"; then
		echo "PREBUILD BK7252: mdns.c already present in beken_src.mk"
	else
		echo "PREBUILD BK7252: adding mdns.c to lwIP source list"
		sed -i '/src\/apps\/mqtt\/mqtt\.c/a SRC_LWIP_C += ./beken378/func/lwip_intf/$(LWIP_VERSION)/src/apps/mdns/mdns.c' "$BEKEN_SRC_MK"
	fi
else
	echo "PREBUILD BK7252: WARN beken_src.mk not found: $BEKEN_SRC_MK"
fi
