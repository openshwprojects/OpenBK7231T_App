#ifndef __DRV_DEVICECLOCK_H__
#define __DRV_DEVICECLOCK_H__

#include "../httpserver/new_http.h"

//void AppendTimeInformationToHTTPIndexPage(http_request_t* request);
void setDeviceTime(uint32_t time);
void setDeviceTimeOffset(int offs);
bool IsTimeSynced();
int GetTimesZoneOfsSeconds();
void Clock_OnEverySecond();
unsigned int GetCurrentTime();
unsigned int GetCurrentTimeWithoutOffset();
int GetWeekDay();
int GetHour();
int GetMinute();
int GetSecond();
int GetMDay();
int GetMonth();
int GetYear();
int GetSunrise();
int GetSunset();
// drv_clock_events.c
int Print_Clock_EventList();
int GetClockEventTime(int id);
int RemoveClockEvent(int id);
int ClearClockEvents();

extern time_t g_deviceTime;
extern int g_timeOffsetSeconds;
extern uint32_t g_next_dst_change;
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;

#endif /* __DRV_DEVICECLOCK_H__ */

