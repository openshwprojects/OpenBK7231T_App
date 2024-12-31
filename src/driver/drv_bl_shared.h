#pragma once

#include "../httpserver/new_http.h"

void BL_Shared_Init(void);
void BL_ProcessUpdate(float voltage, float current, float power);
void BL_ProcessUpdateExt(float voltage, float current, float power,
                         float frequency, float importWh, float exportWh);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request);
