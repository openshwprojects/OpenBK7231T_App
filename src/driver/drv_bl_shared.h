#pragma once

#include "../httpserver/new_http.h"
#include "../new_common.h"

void BL_Shared_Init(void);
void BL_ProcessUpdate(float voltage, float current, float power,
                      float frequency, float energyWh);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request);
void BL09XX_SaveEmeteringStatistics();
bool BL09XX_PublishHASSDevices(const char* topic);

