#ifndef __DRV_NTP_H__
#define __DRV_NTP_H__

#include "../httpserver/new_http.h"

void NTP_Init();
void NTP_OnEverySecond();
void NTP_StopDriver();
// returns number of seconds passed after 1900
void AppendTimeInformationToHTTPIndexPage(http_request_t* request);
bool NTP_IsTimeSynced();
// for Simulator only, on Windows, for unit testing
void NTP_SetSimulatedTime(unsigned int timeNow);
#endif /* __DRV_NTP_H__ */

