#pragma once

#include "../httpserver/new_http.h"
#include "../obk_config.h"

#if ENABLE_BL_SHARED

void BL_Shared_Init(void);
void BL_ProcessUpdate(float voltage, float current, float power,
                      float frequency, float energyWh);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request);
void BL09XX_SaveEmeteringStatistics();

void BL_Shared_InitEx(int asensorsix);
void BL_ProcessUpdateEx(int asensorsix, float voltage, float current, float power,
  float frequency, float energyWh);
void BL09XX_AppendInformationToHTTPIndexPageEx(int asensorsix, http_request_t* request);
void BL_ResetRecivedDataBool();

#define BL_SENSORS_IX_0 0
#define BL_SENSORS_IX_1 1

int BL_IsMeteringDeviceIndexActive(int asensdatasetix);

#endif

