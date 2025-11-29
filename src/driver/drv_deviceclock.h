#ifndef __DRV_DEVICECLOCK_H__
#define __DRV_DEVICECLOCK_H__

#include "../httpserver/new_http.h"
#include "../cmnds/cmd_public.h"
#include "../new_common.h"
#include <stdbool.h>


int TIME_GetWeekDay();
int TIME_GetHour();
int TIME_GetMinute();
int TIME_GetSecond();
int TIME_GetMDay();
int TIME_GetMonth();
int TIME_GetYear();
int TIME_GetSunrise();
int TIME_GetSunset();
// drv_timed_events.c
int TIME_Print_EventList();
void TIME_setDeviceTime(uint32_t time);
void TIME_setDeviceTimeOffset(int offs);
int TIME_GetEventTime(int id);
int TIME_RemoveEvent(int id);
int TIME_ClearEvents();
void TIME_Init();
void TIME_OnEverySecond();
commandResult_t SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags);
commandResult_t SetDeviceTime(const void *context, const char *cmd, const char *args, int cmdFlags);

#if ENABLE_TIME_SUNRISE_SUNSET
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;
int TIME_GetSunrise();
int TIME_GetSunset();
float TIME_GetLatitude();
float TIME_GetLongitude();
void TIME_setLatitude(float lat);
void TIME_setLongitude(float longi);
#if ENABLE_CALENDAR_EVENTS
void TIME_CalculateSunrise(byte *outHour, byte *outMinute);	
void TIME_CalculateSunset(byte *outHour, byte *outMinute);
#endif	// to #if ENABLE_CALENDAR_EVENTS
#endif
uint32_t TIME_GetCurrentTime(); 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
uint32_t TIME_GetCurrentTimeWithoutOffset(); 		// ... same for NTP_GetCurrentTimeWithoutOffset()...
bool TIME_IsTimeSynced(); 				// ... and for NTP_IsTimeSynced()

int TIME_GetTimesZoneOfsSeconds();			// ... and for NTP_GetTimesZoneOfsSeconds()

#if ENABLE_TIME_DST
int Time_IsDST();
bool IsDST_initialized();
uint32_t setDST();
int getDST_offset();
#if ENABLE_TIME_SUNRISE_SUNSET
// since we calculate time "in the future", which might be after a DST switch, we migth need to fix sunset/sunrise time (add ore sub DST_offset)
// in case a DST switch happens, we should change future events of sunset/sunrise, since this will be different after a switch
// since we calculated the events in advance, we need to "fix" all events, postulating the DST switch is allways before a days sunrise and sunset
void fix_DSTforEvents(int minutes);	// inside "drv_timed_events.c"
#endif
uint32_t RuleToTime(uint8_t dayOfWeek, uint8_t month, uint8_t weekNum, uint8_t hour, uint16_t year);
void getDSTtransition(uint32_t * DST);
#endif
void TIME_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);


#endif /* __DRV_DEVICECLOCK_H__ */

