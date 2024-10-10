#ifndef __DRV_DEVICECLOCK_H__
#define __DRV_DEVICECLOCK_H__

#include "../httpserver/new_http.h"

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
extern struct SUN_DATA {  /* sunrise / sunset globals */
	int latitude;  /* latitude * 1000000 */
	int longitude;  /* longitude * 1000000 */
	} sun_data;

#if ENABLE_LOCAL_CLOCK_ADVANCED
// handle daylight saving time

// define a union to hold all settings regarding local clock (timezone, dst settings ...)
// we will use somthing simmilar to tasmotas handling of dst with the commands:
// TimeStd
// TimeDst
// use a union, to be able to save/restore all values as one 64 bit integer
union DST{
	struct {
		uint32_t DST_H : 1;	// 0-1		only once - you can only be in one DST_hemisphere ;-)
		uint32_t DST_Ws : 3; // 0-7
		uint32_t DST_Ms : 4; // 0-15
		uint32_t DST_Ds : 3; // 0-7
		uint32_t DST_hs : 5; // 0-31
		// up to here: 16 bits
		int32_t Tstd : 11; // -1024 - +1023 	( regular (non DST)  offset to UTC max 14 h = 840 Min) 
		uint32_t DST_he : 5; // 0-31
		// up to here: 32 bits
		uint32_t DST_We : 3; // 0-7
		uint32_t DST_Me : 4; // 0-15
		uint32_t DST_De : 3; // 0-7
		int32_t Tdst : 8; // -128 - +127  (Eire has "negative" DST in winter (-60) ...; max DST offset is 120 minutes)
		int32_t TZ : 12; //  -2048 - 2047 for TZ hours (max offset is +14h and - 12h  -- we will use 99 as indicator for "DST" like tasmota, all otDST_hers values are interpreted as the <hour><minutes> of the time zone eg: -1400 for -14:00)
		// up to here: 62 bits
		};
	uint64_t value;	// to save/store all values in one large integer 
};

extern union DST g_clocksettings;

#endif

int testNsetDST(uint32_t val);

// to use ticks for time keeping
// since usual ticktimer (uint32_t) rolls over after approx 50 days,
// we need to count this rollovers
#ifndef WINDOWS
extern TickType_t lastTick;
extern uint8_t timer_rollover; // I don't expect uptime > 35 years ...
#endif
void set_UTCoffset_from_Config();

uint32_t Clock_GetCurrentTime(); 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
uint32_t Clock_GetCurrentTimeWithoutOffset(); 		// ... same for NTP_GetCurrentTimeWithoutOffset()...
bool Clock_IsTimeSynced(); 				// ... and for NTP_IsTimeSynced()
int Clock_GetTimesZoneOfsSeconds();			// ... and for NTP_GetTimesZoneOfsSeconds()



//
// ###################################################################  START  ########################################################################
// ##################################################### temp. function to get local time, ############################################################
// ##################################################### even if NTP is running and synced ############################################################ 
//
uint32_t Clock_GetDeviceTime();
uint32_t Clock_GetDeviceTimeWithoutOffset();

//
// ###################################################################   END   ########################################################################
// ##################################################### temp. function to get local time, ############################################################
// ##################################################### even if NTP is running and synced ############################################################ 
//

#endif /* __DRV_DEVICECLOCK_H__ */

