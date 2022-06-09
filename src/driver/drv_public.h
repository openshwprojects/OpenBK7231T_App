


void DRV_Generic_Init();
#include "../httpserver/new_http.h"
void DRV_AppendInformationToHTTPIndexPage(http_request_t *request);
void DRV_OnEverySecond();
void DRV_RunQuickTick();
void DRV_StartDriver(const char *name);
void DRV_StopDriver(const char *name);

