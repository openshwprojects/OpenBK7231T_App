#ifndef __DRV_DEVICECLOCK_H__
#define __DRV_DEVICECLOCK_H__

#include "../httpserver/new_http.h"
#include "../cmnds/cmd_public.h"
#include "../new_common.h"
#include <stdbool.h>


int CLOCK_GetWeekDay();
int CLOCK_GetHour();
int CLOCK_GetMinute();
int CLOCK_GetSecond();
int CLOCK_GetMDay();
int CLOCK_GetMonth();
int CLOCK_GetYear();
int CLOCK_GetSunrise();
int CLOCK_GetSunset();
// drv_timed_events.c
int CLOCK_Print_EventList();
void CLOCK_setDeviceTime(uint32_t time);
void CLOCK_setDeviceTimeOffset(int offs);
int CLOCK_GetEventTime(int id);
int CLOCK_RemoveEvent(int id);
int CLOCK_ClearEvents();
void CLOCK_Init();
void CLOCK_OnEverySecond();
#if ENABLE_LOCAL_CLOCK || ENABLE_NTP
commandResult_t SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags);
#endif

#if ENABLE_CLOCK_SUNRISE_SUNSET
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;
int CLOCK_GetSunrise();
int CLOCK_GetSunset();
float CLOCK_GetLatitude();
float CLOCK_GetLongitude();
void CLOCK_setLatitude(float lat);
void CLOCK_setLongitude(float longi);
#endif
uint32_t Clock_GetCurrentTime(); 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
uint32_t Clock_GetCurrentTimeWithoutOffset(); 		// ... same for NTP_GetCurrentTimeWithoutOffset()...
bool Clock_IsTimeSynced(); 				// ... and for NTP_IsTimeSynced()

int Clock_GetTimesZoneOfsSeconds();			// ... and for NTP_GetTimesZoneOfsSeconds()

#if ENABLE_CLOCK_DST
int Time_IsDST();
uint32_t setDST();
int getDST_offset();
uint32_t RuleToTime(uint8_t dayOfWeek, uint8_t month, uint8_t weekNum, uint8_t hour, uint16_t year);
void getDSTtransition(uint32_t * DST);
#endif
void CLOCK_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);


#endif /* __DRV_DEVICECLOCK_H__ */

