
#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"

#include "drv_deviceclock.h"
// functions for handling device time even without NTP driver present
// using "g_epochOnStartup" and "g_UTCoffset" if NTP not present (or not synced)

#include "drv_ntp.h"

// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
#if ENABLE_LOCAL_CLOCK
uint32_t g_epochOnStartup = 0;
// UTC offset
int g_UTCoffset = 0;
#endif

#if ENABLE_LOCAL_CLOCK_ADVANCED
// daylight saving time offset
int g_DSToffset = 0;
// epoch of next change in dst
uint32_t g_next_dst_change=0;
#endif

extern void CLOCK_Init_Events(void);
extern void CLOCK_RunEvents(unsigned int newTime, bool bTimeValid);

#if ENABLE_CLOCK_SUNRISE_SUNSET
extern void CLOCK_CalculateSunrise(byte *outHour, byte *outMinute);
extern void CLOCK_CalculateSunset(byte *outHour, byte *outMinute);
#endif

// leave it for the moment, until a CLOCK logging is possible ...
#define LOG_FEATURE LOG_FEATURE_NTP

#if ENABLE_CLOCK_SUNRISE_SUNSET

/* sunrise/sunset defaults */
#define CFG_DEFAULT_LATITUDE	43.994131
#define CFG_DEFAULT_LONGITUDE -123.095854
#define SUN_DATA_COORD_MULT		1000000

struct SUN_DATA sun_data =
	{
	.latitude = (int) (CFG_DEFAULT_LATITUDE * SUN_DATA_COORD_MULT),
	.longitude = (int) (CFG_DEFAULT_LONGITUDE * SUN_DATA_COORD_MULT),
	};

//Set Latitude and Longitude for sunrise/sunset calc
commandResult_t CLOCK_SetLatlong(const void *context, const char *cmd, const char *args, int cmdFlags) {
    const char *newValue;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
    newValue = Tokenizer_GetArg(0);
    sun_data.latitude = (int) (atof(newValue) * SUN_DATA_COORD_MULT);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK latitude set to %s", newValue);

    newValue = Tokenizer_GetArg(1);
		sun_data.longitude = (int) (atof(newValue) * SUN_DATA_COORD_MULT);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK longitude set to %s", newValue);
    return CMD_RES_OK;
}

int CLOCK_GetSunrise()
{
	byte hour, minute;
	int sunriseInSecondsFromMidnight;

	CLOCK_CalculateSunrise(&hour, &minute);
	sunriseInSecondsFromMidnight = ((int)hour * 3600) + ((int)minute * 60);
	return sunriseInSecondsFromMidnight;
}

int CLOCK_GetSunset()
{
	byte hour, minute;
	int sunsetInSecondsFromMidnight;

	CLOCK_CalculateSunset(&hour, &minute);
	sunsetInSecondsFromMidnight =  ((int)hour * 3600) + ((int)minute * 60);
	return sunsetInSecondsFromMidnight;
}
#endif

int CLOCK_GetWeekDay() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_wday;
}
int CLOCK_GetHour() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_hour;
}
int CLOCK_GetMinute() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_min;
}
int CLOCK_GetSecond() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_sec;
}
int CLOCK_GetMDay() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mday;
}
int CLOCK_GetMonth() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mon+1;
}
int CLOCK_GetYear() {
	struct tm *ltm;
	time_t act_deviceTime = (time_t)Clock_GetCurrentTime();

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&act_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_year+1900;
}


#if ENABLE_LOCAL_CLOCK
void CLOCK_setDeviceTime(uint32_t time)
{
// done in CMD_Init_Delayed()  in cmd_main.c
//	if (g_epochOnStartup < 10) CLOCK_Init();
	g_epochOnStartup = time - g_secondsElapsed;
}

void CLOCK_setDeviceTimeOffset(int offs)
{
	g_UTCoffset = offs;
}
#endif


void CLOCK_Init() {

#if ENABLE_CLOCK_SUNRISE_SUNSET
	//cmddetail:{"name":"clock_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Sets the devices latitude and longitude",
	//cmddetail:"fn":"CLOCK_SetLatlong","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":"CLOCK_SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("clock_setLatLong", CLOCK_SetLatlong, NULL);    
// and register an alias for backward compatibility
	//cmddetail:{"name":"ntp_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"obsolete! only for backward compatibility - please use 'clock_setLatLong' in the future",
	//cmddetail:"fn":"CLOCK_SetLatlong","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":"ntp_SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("ntp_setLatLong", CLOCK_SetLatlong, NULL);    
#endif
#if ENABLE_CALENDAR_EVENTS
	CLOCK_Init_Events();
#endif


    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK driver initialized.");
}

void CLOCK_OnEverySecond()
{

#if ENABLE_CALENDAR_EVENTS
	CLOCK_RunEvents(Clock_GetCurrentTime(), Clock_IsTimeSynced());
#endif
}





uint32_t Clock_GetCurrentTime(){ 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
#if ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return (uint32_t)NTP_GetCurrentTime();
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return it - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
#if ENABLE_LOCAL_CLOCK_ADVANCED
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset + g_DSToffset : 0;
#elif ENABLE_LOCAL_CLOCK
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset : 0;
#else
	return  0;
#endif
};

uint32_t Clock_GetCurrentTimeWithoutOffset(){ 	// ... same forNTP_GetCurrentTimeWithoutOffset()...
#if ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return (uint32_t)NTP_GetCurrentTimeWithoutOffset();
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
#if ENABLE_LOCAL_CLOCK
	// if g_epochOnStartup is set, return time - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? g_epochOnStartup  + g_secondsElapsed  : 0;
#else
	return  0;
#endif
};


//
// ###################################################################  START  ########################################################################
// ##################################################### temp. function to get local time, ############################################################
// ##################################################### even if NTP is running and synced ############################################################ 
//
uint32_t Clock_GetDeviceTime(){ 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
#if ENABLE_LOCAL_CLOCK_ADVANCED
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset + g_DSToffset : 0;
#elif ENABLE_LOCAL_CLOCK
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset : 0;
#else
	return  0;
#endif
};

uint32_t Clock_GetDeviceTimeWithoutOffset(){ 	// ... same forNTP_GetCurrentTimeWithoutOffset()...
#if ENABLE_LOCAL_CLOCK
	// if g_epochOnStartup is set, return time - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? g_epochOnStartup  + g_secondsElapsed  : 0;
#else
	return  0;
#endif
};

//
// ###################################################################   END   ########################################################################
// ##################################################### temp. function to get local time, ############################################################
// ##################################################### even if NTP is running and synced ############################################################ 
//


bool Clock_IsTimeSynced(){ 				// ... and for NTP_IsTimeSynced()
#if ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return true;
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return time - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
#if ENABLE_LOCAL_CLOCK
	return g_epochOnStartup > 10 ? true : false;
#else
	return  false;
#endif

}


int Clock_GetTimesZoneOfsSeconds()			// ... and for NTP_GetTimesZoneOfsSeconds()
{
#if ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return NTP_GetTimesZoneOfsSeconds();
	}
	// even if NTP is enabled, but NTP is not synced, we might have UTC offset set with g_epochOnStartup set, so go on
#endif
	// ...and again: check if g_epochOnStartup is set  ... 
#if ENABLE_LOCAL_CLOCK_ADVANCED
	return g_epochOnStartup > 10 ? g_UTCoffset + g_DSToffset: 0;
#elif  ENABLE_LOCAL_CLOCK
	return g_epochOnStartup > 10 ? g_UTCoffset  : 0;
#else
	return 0;
#endif

}


#if ENABLE_LOCAL_CLOCK_ADVANCED
// handle daylight saving time and so on

union DST g_clocksettings;

const uint32_t SECS_PER_DAY = 3600UL * 24UL;

// function derived from tasmotas "RuleToTime"
uint32_t RuleToTime(uint8_t dow, uint8_t mo, uint8_t week,  uint8_t hr, int yr) {
  struct tm tm={0};
  uint32_t t;
  uint8_t m=mo-1;		// we use struct tm here, so month values 0..11
  uint8_t w=week;		// temp copies of mo(nth) and week

  if (0 == w) {			// Last week = 0
    if (++m > 11) {		// for "Last", go to the next month ...
      m = 0;			// .. and to next year, if we need last XY in December
      yr++;
    }
    w = 1;			// search first week of next month, we will subtract 7 days later
  }
  tm.tm_hour = hr;
// tm is set to {0}, so we shouldn't need to set values equal to "0"
//  tm.tm_min = 0;
//  tm.tm_sec = 0;
  tm.tm_mday = 1;		// first day of (next) month
  tm.tm_mon = m ;
  tm.tm_year = yr - 1900;
//  t = (uint32_t)timegm(&tm);         // First day of the month, or first day of next month for "Last" rules
  t = (uint32_t)mktime(&tm);         // First day of the month, or first day of next month for "Last" rules
//  since we use time functions, weekdays are 0 to 6 but tasmote settings use 1 to 7 (e.g. Sunday = 1)
// so we have to subtract 1 from dow to match tm_weekday here
  t += (7 * (w - 1) + (dow - 1 - tm.tm_wday + 7) % 7) * SECS_PER_DAY;
  if (0 == week) {
    t -= 7 * SECS_PER_DAY;  // back one week if this is a "Last" rule
  }
  return t;
}

void printtime(uint32_t time, char* string){
	time_t tmpt=(time_t)time;
	struct tm * tm=gmtime(&tmpt);
	// e.g. "Sun, 2024-10-27 01:00:00 UTC"
	strftime(string,30,"%a, %F %T UTC",tm);
}

// helper, for printf won't print uint64_t (%llu) correctly
void dump_u64(uint64_t x,char *ret)
{
    int i=0;
    uint64_t n=x;
    while (n != 0)
    {
        i++;
        n /= 10;
    }
    ret[i] = '\0';
    do
    {
        ret[--i] = '0' + (x % 10);
        x /= 10;
    } while (x && i>=1);
}


// return 1 if checked time is during DST period 
// set g_next_dst_change to next DST event
int testNsetDST(uint32_t val)
{
	// DST calculation can be split in three phases during a year:
	// 1. at the beginning the time before DST starts/ends
	// 2. after first DST change - but before second (during DST in northern or during regular time in southern hemisphere)
	// 3. after second DST change (after DST in northern or during DST in southern hemisphere)
	// For this three cases, we known
	// 1. next switch will take place at first DST change of this year - no DST for nothern hemisphere, in DST for southern
	// 2. we are between the two DST changes, next switch will take place at second DST change of this year - DST in N, regular time in S
	// 3. after second DST switch, next switch will take place at first DST change of next year - regular time in N, DST in S
	// so we will need mimimum the first DST change of this year, we calculate both and check for minimum.
	// This check is needed, for "H" setting will not work in every case:
	// e.g. in Ireland (northern / H=0), which uses "DST" in winter for a "negative" DST, so DST "starts" in fall and not in spring as expected for N
	//
	// we can be gratious for calculating the actual year by dividing epoch
	// if we are off a day (leap year), it won't matter, as long as around the switch of the year there is no DST event...
	// so we use 1 Year = 365.24 days = 31556926 Seconds
	// and can be sure to have the correct year for a date if it's > Jan 1st and < Dec 31st which is true for all known DST dates


	// so lets try to find the year corresponding to val
	int year=(int)(val/31556926)+1970;


	uint32_t b= RuleToTime(g_clocksettings.DST_Ds,g_clocksettings.DST_Ms,g_clocksettings.DST_Ws,g_clocksettings.DST_hs-g_clocksettings.Tstd/60,year);
	uint32_t e= RuleToTime(g_clocksettings.DST_De,g_clocksettings.DST_Me,g_clocksettings.DST_We,g_clocksettings.DST_he-(g_clocksettings.Tstd+g_clocksettings.Tdst)/60,year);
	char tmp[30];	// to hold date string of timestamp
	dump_u64(g_clocksettings.value,tmp);  // printf with %llu doesn't work :-( so we print it to a string by our own
//	ADDLOG_INFO(LOG_FEATURE_RAW, "INFO: in testNsetDST with ClockSettings: %i  %i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i -- g_clocksettings.value as uint64_t:%s \r\n",g_clocksettings.TZ,  g_clocksettings.DST_H, g_clocksettings.DST_We, g_clocksettings.DST_Me, g_clocksettings.DST_De, g_clocksettings.DST_he, g_clocksettings.Tstd, g_clocksettings.DST_Ws, g_clocksettings.DST_Ms, g_clocksettings.DST_Ds, g_clocksettings.DST_hs, g_clocksettings.Tdst,tmp);
	ADDLOG_INFO(LOG_FEATURE_RAW, "Info: epoch %lu in year %i -- b=%lu -- e=%lu\n\r\n\r",val,year,b,e);
	if ( b < e ) {	// Northern --> begin before end
		if (val < b) {
			printtime(b,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "Before first DST switch in %i. Next switch at %lu (%s)\r\n",year,b,tmp);
			g_next_dst_change=b;
			g_DSToffset = 0;
			return 0;
		} else if ( val < e ){
			printtime(e,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "In DST of %i. Next switch at %lu (%s)\r\n",year,e,tmp);
			g_next_dst_change=e;
			g_DSToffset = 60*g_clocksettings.Tdst;
			return 1;
		} else {
			b=RuleToTime(g_clocksettings.DST_Ds,g_clocksettings.DST_Ms,g_clocksettings.DST_Ws,g_clocksettings.DST_hs-g_clocksettings.Tstd/60,year+1);
			printtime(b,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "After DST in %i. Next switch at %lu (%s - thats next year)\r\n",year,b,tmp);
			g_next_dst_change=b;
			g_DSToffset = 0;
			return 0;
		}
	} else {	// so end of DST before begin of DST --> southern
			if (val < e) {
			printtime(e,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "In first DST period of %i. Next switch at %lu (%s)\r\n",year,e,tmp);
			g_next_dst_change=e;
			g_DSToffset = 60*g_clocksettings.Tdst;
			return 1;
		} else if ( val < b ){
			printtime(b,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "Regular time of %i. Next switch at %lu (%s)\r\n",year,b,tmp);
			g_next_dst_change=b;
			g_DSToffset = 0;
			return 0;
		} else {
			e=RuleToTime(g_clocksettings.DST_De,g_clocksettings.DST_Me,g_clocksettings.DST_We,g_clocksettings.DST_he-(g_clocksettings.Tstd+g_clocksettings.Tdst)/60,year+1);
			printtime(e,tmp);
			ADDLOG_INFO(LOG_FEATURE_RAW, "In second DST of %i. Next switch at %lu (%s - thats next year)\r\n",year,e,tmp);
			g_next_dst_change=e;
			g_DSToffset = 60*g_clocksettings.Tdst;
			return 1;
		}
	}


}



/*
Similar to tasmota settings, but all in one command

Set policies for the beginning of daylight saving time (DST) and return back to standard time (STD) 
Use the Tasmota timezone table to find the commands for your time zone.
0 = reset parameters to firmware defaults
H,W,M,D,h,T
H = hemisphere (0 = northern hemisphere / 1 = southern hemisphere)
W = week (0 = last week of month, 1..4 = first .. fourth)
M = month (1..12)
D = day of week (1..7 1 = Sunday 7 = Saturday)
h = hour (0..23) in local time
T = time zone (-780..780) (offset from UTC in MINUTES - 780min / 60min=13hrs)
Example: TIMEDST 1,1,10,1,2,660
_If time zone is NOT 99, DST is not used (even if displayed)

Since we keep everything in one struct, we have dst start and ending formulas,
the regular UTC offset and the ammount of minutes to add during dst

So here is a matching of the variables as used in tasmota commands and our struct/union

TimeStd H,W,M,D,h,T --> He=H; We=W; Me=M; De=D; he=h; Tstd=T
TimeDST H,W,M,D,h,T --> Hs=H; Ws=W; Ms=M; Ds=D; hs=h; Tdst=T-Tstd

We use an all in on setting command:

Clock_SetConfig TZ,H,We,Me,De,he,Tstd,Ws,Ms,Ds,hs,Tdst
TZ:	Timezone (+/-HH or +/-HH:MM or 99 for DST)
<x>=s: 	Values for daylight saving time "start of DTS"
<x>=e: 	Values for standard time "end of DTS"
	W<x> = week (0 = last week of month, 1..4 = first .. fourth)
	M<x> = month (1..12)
	D<x> = day of week (1..7 1 = Sunday 7 = Saturday)
	h<x> = hour (0..23) in local time
	Tsdt = standard offset during regular time
	Tdst = offset to add during DST (allmost everywhere 60 [minutes], -60 in Eire ("negative DST") and seldom 120 or 30) 

example for EU DST (with UTC+1):
	TZ=99 --> use DST settings
	northern hmisphere				(H=0)
	start last sun in march at 2 local time 	(Ws=0 [last ...] Ds=1 [Sunday] Ms=3 [March] hs=2 [2:00] Tdst=60 [add 60 minutes=1 hour for DST])
	end last sun in October at 3 local (dst) time	(We=0 [last ...] De=1 [Sunday] Me=10 [Oct] he=3 [3:00] Tdstd=60 [standard time is UTC+1 (UTC+60 minutes)])
clock_setConfig 99,0,0,10,1,3,60,0,3,2,1,60



We can also derive all settings from Tasmota settings

Settings for EU
Tasmota: 	Europe/Berlin 	Backlog0 Timezone 99; TimeStd 0,0,10,1,3,60; TimeDst 0,0,3,1,2,120
Our config:	clock_setConfig 99 0,0,10,1,3,60,0,3,1,2,60

Settings for NYC
Tasmota: 	USA/NYC		Backlog0 Timezone 99; TimeStd 0,1,11,1,2,-300; TimeDst 0,2,3,1,2,-240
Our config:	clock_setConfig 99 0,1,11,1,2,-300,2,3,1,2,60


*/
// CLOCK_SetConfig [TimeZone] [DST-Settings]
commandResult_t CMD_CLOCK_SetConfig(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a,b;
	uint8_t H,We,Ws,Me,Ms,De,Ds,he,hs;
	int8_t Tdst;
	int16_t Tstd,TZ=1;
	const char *s;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_GetArgsCount() < 1) {	// so we have no args
//	ADDLOG_INFO(LOG_FEATURE_CMD, "Actual ClockSettings: %i  %i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i \r\n",g_clocksettings.TZ,  g_clocksettings.DST_H, g_clocksettings.DST_We, g_clocksettings.DST_Me, g_clocksettings.DST_De, g_clocksettings.DST_he, g_clocksettings.Tstd, g_clocksettings.DST_Ws, g_clocksettings.DST_Ms, g_clocksettings.DST_Ds, g_clocksettings.DST_hs, g_clocksettings.Tdst);
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Actual ClockSettings %i %i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i\r\n", g_clocksettings.TZ,  g_clocksettings.DST_H, g_clocksettings.DST_We, g_clocksettings.DST_Me, g_clocksettings.DST_De, g_clocksettings.DST_he, g_clocksettings.Tstd, g_clocksettings.DST_Ws, g_clocksettings.DST_Ms, g_clocksettings.DST_Ds, g_clocksettings.DST_hs, g_clocksettings.Tdst);
	}
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

// handle first parameter: timezone
// can be a string like "-10:30" or an integer like -10
// we accept a single parameter (TZ) or a complete set of daylight saving time settings
	s = Tokenizer_GetArg(0);
	if (strchr(s, ':')) {
//		printf("CLOCK_SetConfigs: TZ with : time TZ=%s\r\n", s);
		ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: TZ with : time TZ=%s\r\n", s);
		if (*s == '-') {
			s++;
//			printf("CLOCK_SetConfigs: TZ negative (-)\r\n");
			ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: TZ negative (-)\r\n");
			TZ = -1;
		} else if  (*s == '+') {
//			printf("CLOCK_SetConfigs: TZ posiive (+)\r\n");
			ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: TZ posiive (+)\r\n");
			s++;
		}
		sscanf(s, "%d:%2d", &a, &b);
//			printf("CLOCK_SetConfigs: a=%i b=%i (s=%s)\r\n",a,b,s);
			ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: a=%i b=%i (s=%s)\r\n",a,b,s);
		TZ *= (a * 100 + b); // we use time as signed number <hour><minutes> of the time zone eg: -1400 for -14:00
		// set offset for clock hours and minutes
		g_UTCoffset=3600*a+60*b;
	}
	else {
		TZ = Tokenizer_GetArgInteger(0);
		// set offset for clock [hours] (if it's no DST line, then we use Tstd later)
		if (TZ != 99) g_UTCoffset=3600*TZ;
	}

	g_clocksettings.TZ=TZ;

	if (Tokenizer_GetArgsCount() > 1) {	// so we have more than 1 argument (TZ)
		s = Tokenizer_GetArg(1);
//		ADDLOG_INFO(LOG_FEATURE_CMD, "CLOCK_SetConfigs: we have %i Args, TZ is set to %i", a,TZ);
//		if (TZ <> 99) ADDLOG_INFO(LOG_FEATURE_CMD, "!!!Warning: Setting DST but TZ is not 99 but set to %i!!!\n",TZ);
//		printf("CLOCK_SetConfigs: we have %u Args, TZ is set to %i -- S=%s\r\n", Tokenizer_GetArgsCount(),TZ,s);
		ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: we have %u Args, TZ is set to %i -- S=%s\r\n", Tokenizer_GetArgsCount(),TZ,s);

//		g_clocksettings.H=Tokenizer_GetArgInteger(1);
// 		do this later, so we can do some sanity checks before ...
//		so use local vars here
//		Clock_SetConfig TZ,H,We,Me,De,he,Tstd,Ws,Ms,Ds,hs,Tdst

		H = Tokenizer_GetArgInteger(1);
		We = Tokenizer_GetArgInteger(2);
		Me = Tokenizer_GetArgInteger(3);
		De = Tokenizer_GetArgInteger(4);
		he = Tokenizer_GetArgInteger(5);
		Tstd = Tokenizer_GetArgInteger(6);
		Ws = Tokenizer_GetArgInteger(7);
		Ms = Tokenizer_GetArgInteger(8);
		Ds = Tokenizer_GetArgInteger(9);
		hs = Tokenizer_GetArgInteger(10);
		Tdst = Tokenizer_GetArgInteger(11);
ADDLOG_INFO(LOG_FEATURE_RAW, "read values: %u,%u,%u,%u,%u,%i,%u,%u,%u,%u,%i\r\n", H, We, Me, De, he, Tstd, Ws, Ms, Ds, hs, Tdst);
// todo: check numbers before assigning them
		g_clocksettings.DST_H=H;
		g_clocksettings.DST_We=We;
		g_clocksettings.DST_Me=Me;
		g_clocksettings.DST_De=De;
		g_clocksettings.DST_he=he;
		g_clocksettings.Tstd=Tstd;
		// set offset for clock (minutes)
		g_UTCoffset=60*Tstd;
		g_clocksettings.DST_Ws=Ws;
		g_clocksettings.DST_Ms=Ms;
		g_clocksettings.DST_Ds=Ds;
		g_clocksettings.DST_hs=hs;
		g_clocksettings.Tdst=Tdst;
	}; //
	// save settings
	CFG_SetCLOCK_SETTINGS(g_clocksettings.value);
	  return CMD_RES_OK;
};

commandResult_t CMD_CLOCK_GetConfig(const void *context, const char *cmd, const char *args, int cmdFlags) {
//	ADDLOG_INFO(LOG_FEATURE_CMD, "ClockSettings: %i  %i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i \r\n",g_clocksettings.TZ,  g_clocksettings.DST_H, g_clocksettings.DST_We, g_clocksettings.DST_Me, g_clocksettings.DST_De, g_clocksettings.DST_he, g_clocksettings.Tstd, g_clocksettings.DST_Ws, g_clocksettings.DST_Ms, g_clocksettings.DST_Ds, g_clocksettings.DST_hs, g_clocksettings.Tdst);
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "ClockSettings: %i  %i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i \r\n",g_clocksettings.TZ,  g_clocksettings.DST_H, g_clocksettings.DST_We, g_clocksettings.DST_Me, g_clocksettings.DST_De, g_clocksettings.DST_he, g_clocksettings.Tstd, g_clocksettings.DST_Ws, g_clocksettings.DST_Ms, g_clocksettings.DST_Ds, g_clocksettings.DST_hs, g_clocksettings.Tdst);




	return CMD_RES_OK;
}

void set_UTCoffset_from_Config(){
	if (g_clocksettings.TZ != 99) {
		int TZ=g_clocksettings.TZ;
		// value hours only? Then |TZ| < 24 (or TZ² < 576 regardless if it's positive or negative)
		if (TZ*TZ < 576 ) g_UTCoffset=3600*TZ;		// TZ is hours only
		else {
			g_UTCoffset=3600*(int)TZ/100+60*(TZ%100);		// TZ/100 = hours 	T/%100 = minutes
		}
	}
	else {
		g_UTCoffset=60*g_clocksettings.Tstd; 	// set offset for clock (from minutes)
		g_DSToffset=60*g_clocksettings.Tdst;	// set DST offset for (from minutes)
	}
		
}

#endif	// to #if ENABLE_LOCAL_CLOCK_ADVANCED


