#ifndef __DRV_NTP_H__
#define __DRV_NTP_H__

#include "../httpserver/new_http.h"

void NTP_Init();
void NTP_OnEverySecond();
// returns number of seconds passed after 1900
unsigned int NTP_GetCurrentTime();
unsigned int NTP_GetCurrentTimeWithoutOffset();
void NTP_AppendInformationToHTTPIndexPage(http_request_t* request);
bool NTP_IsTimeSynced();
int NTP_GetTimesZoneOfsSeconds();
int NTP_GetWeekDay();
int NTP_GetHour();
// for Simulator only, on Windows, for unit testing
void NTP_SetSimulatedTime(unsigned int timeNow);

#endif /* __DRV_NTP_H__ */

