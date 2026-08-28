#ifndef DRV_LIFTMASTER_H
#define DRV_LIFTMASTER_H

#include "../httpserver/new_http.h"

// LiftMaster / Chamberlain commercial-operator host-link driver.
//
// Speaks the framed serial link between the door logic board (NXP LPC, where it
// is named cgi::msg1210) and the on-board RTL8720 WiFi module (where the same
// link is named "Saturn"). This driver reproduces the RTL side so OpenBeken can
// drive the door locally instead of via the myQ cloud.
//
// Wire format (validated byte-exact against both silicon dumps, see the
// pwnLiftMaster repo docs/lpc_protocol.md + docs/rtl_saturn_protocol.md):
//
//   '<' TYPE SEQ hex(HDR[4] + LEN + PAYLOAD[LEN] + CKSUM) '>'
//
//   TYPE  : one ASCII letter ('P' for the normal packet builder)
//   SEQ   : one hex nibble (transport sequence)
//   HDR   : 4 routing/address bytes
//   LEN   : payload length byte
//   CKSUM : CRC-8, poly 0x1D, init 0xAA, over HDR+LEN+PAYLOAD
//   every field after SEQ is uppercase 2-char ASCII hex, high nibble first.
//
// Link params: 57600 8N1 (FC0).

// Enabled per-platform in obk_config.h (this driver targets one specific
// LiftMaster/Chamberlain logic board, whose WiFi module is an RTL8720C), so it
// costs nothing on chips that can never see this hardware.
#ifndef ENABLE_DRIVER_LIFTMASTER
#define ENABLE_DRIVER_LIFTMASTER 0
#endif

void LiftMaster_Init();
void LiftMaster_RunFrame();        // quick tick: drain UART + parse frames
void LiftMaster_RunEverySecond();
void LiftMaster_OnChannelChanged(int channel, int value); // UI/MQTT toggle -> door cmd
void LiftMaster_Shutdown();
void LiftMaster_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void LiftMaster_OnHassDiscovery(const char *topic); // publishes a HA garage cover

#endif // DRV_LIFTMASTER_H
