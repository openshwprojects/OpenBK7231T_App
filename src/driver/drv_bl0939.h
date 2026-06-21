#pragma once

#include "../httpserver/new_http.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_BL0939SPI

void BL0939_SPI_Init(void);
void BL0939_SPI_Stop(void);
void BL0939_SPI_RunEverySecond(void);
void BL0939_OnHassDiscovery(const char *topic);
void BL0939_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void BL0939_Save_Statistics(void);

#endif
