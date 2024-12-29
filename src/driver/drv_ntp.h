#ifndef __DRV_NTP_H__
#define __DRV_NTP_H__

#include "../httpserver/new_http.h"

#define LTSTR "Local Time: %04d-%02d-%02d %02d:%02d:%02d"
#define LTM2TIME(T) (T)->tm_year+1900, (T)->tm_mon+1, (T)->tm_mday, (T)->tm_hour, (T)->tm_min, (T)->tm_sec

void NTP_Init();
void NTP_Stop();
void NTP_OnEverySecond();
// returns number of seconds passed after 1900
unsigned int NTP_GetCurrentTime();
unsigned int NTP_GetCurrentTimeWithoutOffset();
void NTP_AppendInformationToHTTPIndexPage(http_request_t* request);
bool NTP_IsTimeSynced();
int NTP_GetTimesZoneOfsSeconds();
void NTP_SetTimesZoneOfsSeconds(int o);
// for Simulator only, on Windows, for unit testing
void NTP_SetSimulatedTime(unsigned int timeNow);
// drv_ntp_events.c
extern time_t g_ntpTime;

#endif /* __DRV_NTP_H__ */

