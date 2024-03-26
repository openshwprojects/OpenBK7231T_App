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
int NTP_GetMinute();
int NTP_GetSecond();
int NTP_GetMDay();
int NTP_GetMonth();
int NTP_GetYear();
int NTP_GetSunrise();
int NTP_GetSunset();
// for Simulator only, on Windows, for unit testing
void NTP_SetSimulatedTime(unsigned int timeNow);
// drv_ntp_events.c
int NTP_PrintEventList();
int NTP_GetEventTime(int id);
int NTP_RemoveClockEvent(int id);
int NTP_ClearEvents();

extern time_t g_ntpTime;
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;

#endif /* __DRV_NTP_H__ */

