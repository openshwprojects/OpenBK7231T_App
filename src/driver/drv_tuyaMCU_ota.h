#ifndef __DRV_TUYAMCU_OTA_H__
#define __DRV_TUYAMCU_OTA_H__

// Firmware update of the TuyaMCU (the external MCU) using the standard Tuya
// MCU upgrade commands 0x0A (start) and 0x0B (data). The image is read from
// a LittleFS file. Part of the TuyaMCU driver; see drv_tuyaMCU_ota.c.

#include "../new_common.h"
#include "../httpserver/new_http.h"

// Called by the TuyaMCU driver
void TuyaMCU_OTA_Init();
void TuyaMCU_OTA_RunFrame();
void TuyaMCU_OTA_Shutdown();
void TuyaMCU_OTA_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

// Offer a received TuyaMCU packet (payload without header and checksum).
// Returns true if it was an OTA reply that the TuyaMCU driver should not
// process further.
bool TuyaMCU_OTA_OnPacket(byte cmd, const byte *payload, int len);

// True while an update is being sent
bool TuyaMCU_OTA_IsActive();

// "idle", "starting", "sending", "waiting for MCU restart", "done" or "failed"
const char *TuyaMCU_OTA_GetStateName();

#endif
