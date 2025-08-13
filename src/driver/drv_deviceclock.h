#ifndef __DRV_DEVICECLOCK_H__
#define __DRV_DEVICECLOCK_H__

#include "../httpserver/new_http.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

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
bool Clock_IsTimeSynced();
commandResult_t CLOCK_SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags);
void CLOCK_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
#if ENABLE_CLOCK_SUNRISE_SUNSET
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;
#endif

uint32_t Clock_GetCurrentTime(); 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
uint32_t Clock_GetCurrentTimeWithoutOffset(); 		// ... same for NTP_GetCurrentTimeWithoutOffset()...
bool Clock_IsTimeSynced(); 				// ... and for NTP_IsTimeSynced()
int Clock_GetTimesZoneOfsSeconds();			// ... and for NTP_GetTimesZoneOfsSeconds()

#if ENABLE_CLOCK_DST
int Time_IsDST();
// usually we want to set/correct g_ntpTime inside setDST()	--> call setDST(1)
// only after setting g_ntpTime freshly from an NTP packet	--> call setDST(0)
// we must not alter g_ntpTime inside setDST in this case (the old offsets are no longer valid)
uint32_t setDST();
int getDST_offset();

#endif


#endif /* __DRV_DEVICECLOCK_H__ */

