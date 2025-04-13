#pragma once

#include "../httpserver/new_http.h"
#include "../obk_config.h"

#if ENABLE_BL_SHARED

void BL_Shared_Init(void);
void BL_ProcessUpdate(float voltage, float current, float power,
                      float frequency, float energyWh);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void BL09XX_SaveEmeteringStatistics();

#define BL_SENSORS_IX_0 0
#if ENABLE_BL_TWIN
#define BL_SENSORS_IX_1 1

void BL_ProcessUpdateEx(int asensdatasetix, float voltage, float current, float power,
  float frequency, float energyWh);
void BL09XX_AppendInformationToHTTPIndexPageEx(int asensdatasetix, http_request_t* request);
void BL_ResetRecivedDataBool();

int BL_IsMeteringDeviceIndexActive(int asensdatasetix);
#endif

#endif

