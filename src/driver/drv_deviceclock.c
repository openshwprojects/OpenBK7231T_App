// NTP client
// Based on my previous work here:
// https://www.elektroda.pl/rtvforum/topic3712112.html

#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"

#include "drv_ntp.h"
#include "drv_deviceclock.h"
#include "drv_public.h"	// for DRV_IsRunning()


#if ENABLE_CALENDAR_EVENTS
extern void Init_Clock_Events(void);
extern void RunClockEvents(unsigned int newTime, bool bTimeValid);
#endif

#if ENABLE_SUNRISE_SUNSET
extern void CalculateSunrise(byte *outHour, byte *outMinute);
extern void CalculateSunset(byte *outHour, byte *outMinute);
#endif

#define LOG_FEATURE LOG_FEATURE_CLOCK

// time offset (time zone?) in seconds
//#define CFG_DEFAULT_TIMEOFFSETSECONDS (-8 * 60 * 60)
int g_timeOffsetSeconds = 0;
// current time - this may be 32 or 64 bit, depending on platform
time_t g_deviceTime = 0;



// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
uint32_t g_epochOnStartup = 0;
// daylight saving time offset
int g_DSToffset = 0;
// epoch of next change in dst
uint32_t g_next_dst_change=0;




int GetTimesZoneOfsSeconds()
{
    return g_timeOffsetSeconds;
}

bool IsTimeSynced(){ 				// for NTP:NTP_IsTimeSynced()
#ifdef ENABLE_NTP
	if (DRV_IsRunning("NTP")== true && NTP_IsTimeSynced() == true) {
		return true;
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return time - we might (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? true : false;

}


commandResult_t SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a, b;
	const char *arg;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	arg = Tokenizer_GetArg(0);
	if (strchr(arg, ':')) {
		int useSign = 1;
		if (*arg == '-') {
			arg++;
			useSign = -1;
		}
		sscanf(arg, "%i:%i", &a, &b);
		g_timeOffsetSeconds = useSign * (a * 60 * 60 + b * 60);
	}
	else {
		g_timeOffsetSeconds = Tokenizer_GetArgInteger(0) * 60 * 60;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_CLOCK,"Clock offset set");
	return CMD_RES_OK;
}

#if ENABLE_SUNRISE_SUNSET

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
commandResult_t SetLatlong(const void *context, const char *cmd, const char *args, int cmdFlags) {
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
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP latitude set to %s", newValue);

    newValue = Tokenizer_GetArg(1);
		sun_data.longitude = (int) (atof(newValue) * SUN_DATA_COORD_MULT);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP longitude set to %s", newValue);
    return CMD_RES_OK;
}

int GetSunrise()
{
	byte hour, minute;
	int sunriseInSecondsFromMidnight;

	CalculateSunrise(&hour, &minute);
	sunriseInSecondsFromMidnight = ((int)hour * 3600) + ((int)minute * 60);
	return sunriseInSecondsFromMidnight;
}

int GetSunset()
{
	byte hour, minute;
	int sunsetInSecondsFromMidnight;

	CalculateSunset(&hour, &minute);
	sunsetInSecondsFromMidnight =  ((int)hour * 3600) + ((int)minute * 60);
	return sunsetInSecondsFromMidnight;
}
#endif

void setDeviceTime(uint32_t time)
{
#if ENABLE_CALENDAR_EVENTS
// check, if time was allready set before init (only call once)
	if (g_epochOnStartup < 10){
	 	Init_Clock_Events();
#if ENABLE_SUNRISE_SUNSET
	//cmddetail:{"name":"ntp_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Sets the NTP latitude and longitude",
	//cmddetail:"fn":"SetLatlong","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":"SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("setLatLong", SetLatlong, NULL);
#endif
	}
#endif
	g_epochOnStartup = time - g_secondsElapsed;
}

void setDeviceTimeOffset(int offs)
{
	g_timeOffsetSeconds = offs;
}

int GetWeekDay() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_wday;
}
int GetHour() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);
addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "in GetHour with g_deviceTime=%lu ", g_deviceTime);
	if (ltm == 0) {
addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "in GetHour ltm is NULL!!!");
		return -1;
	}

	return ltm->tm_hour;
}
int GetMinute() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_min;
}
int GetSecond() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_sec;
}
int GetMDay() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mday;
}
int GetMonth() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mon+1;
}
int GetYear() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_deviceTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_year+1900;
}

// return 0 if time is not set !
unsigned int GetCurrentTime() {
    return IsTimeSynced() == true ?  g_deviceTime + g_timeOffsetSeconds + g_DSToffset : 0 ;
}
unsigned int GetCurrentTimeWithoutOffset() {
	return IsTimeSynced() == true ? g_deviceTime : 0;
}


//
// table of epoch timestamps, when DST events take place (DST starts or ends)
// there are some constraints to the table to work:
// - the timestamps in the table must be in desending order to work properly
// - for every Dst period, both timestamps (end and start) must be present
// --> since the entries are in desneding order, every pair is "DST end" "DST start"
//
// so every time on an "even" index is "DST end", every "odd" entry is "DST start" 
// 
// timestamps for EU 
uint32_t dst_switch_times[]={
1919293200, 1901149200,  // 2030: 27.10.2030 (1919293200) , 31.3.2030 (1901149200)
1887843600, 1869094800,  // 2029: 28.10.2029 (1887843600) , 25.3.2029 (1869094800)
1856394000, 1837645200,  // 2028: 29.10.2028 (1856394000) , 26.3.2028 (1837645200)
1824944400, 1806195600,  // 2027: 31.10.2027 (1824944400) , 28.3.2027 (1806195600)
1792890000, 1774746000,  // 2026: 25.10.2026 (1792890000) , 29.3.2026 (1774746000)
1761440400, 1743296400,  // 2025: 26.10.2025 (1761440400) , 30.3.2025 (1743296400)
1729990800, 1711846800   // 2024: 27.10.2024 (1729990800) , 31.3.2024 (1711846800)
};

/*
//timestapms for US/NYC, just as another example
uint32_t dst_switch_times_NYC[]={
 2551330800, 2530767600,  // 2050:  2050/11/6 (2551330800) -- 2050/3/13 (2530767600)
 2519881200, 2499318000,  // 2049:  2049/11/7 (2519881200) -- 2049/3/14 (2499318000)
 2487826800, 2467263600,  // 2048:  2048/11/1 (2487826800) -- 2048/3/8 (2467263600)
 2456377200, 2435814000,  // 2047:  2047/11/3 (2456377200) -- 2047/3/10 (2435814000)
 2424927600, 2404364400,  // 2046:  2046/11/4 (2424927600) -- 2046/3/11 (2404364400)
 2393478000, 2372914800,  // 2045:  2045/11/5 (2393478000) -- 2045/3/12 (2372914800)
 2362028400, 2341465200,  // 2044:  2044/11/6 (2362028400) -- 2044/3/13 (2341465200)
 2329974000, 2309410800,  // 2043:  2043/11/1 (2329974000) -- 2043/3/8 (2309410800)
 2298524400, 2277961200,  // 2042:  2042/11/2 (2298524400) -- 2042/3/9 (2277961200)
 2267074800, 2246511600,  // 2041:  2041/11/3 (2267074800) -- 2041/3/10 (2246511600)
 2235625200, 2215062000,  // 2040:  2040/11/4 (2235625200) -- 2040/3/11 (2215062000)
 2204175600, 2183612400,  // 2039:  2039/11/6 (2204175600) -- 2039/3/13 (2183612400)
 2172726000, 2152162800,  // 2038:  2038/11/7 (2172726000) -- 2038/3/14 (2152162800)
 2140671600, 2120108400,  // 2037:  2037/11/1 (2140671600) -- 2037/3/8 (2120108400)
 2109222000, 2088658800,  // 2036:  2036/11/2 (2109222000) -- 2036/3/9 (2088658800)
 2077772400, 2057209200,  // 2035:  2035/11/4 (2077772400) -- 2035/3/11 (2057209200)
 2046322800, 2025759600,  // 2034:  2034/11/5 (2046322800) -- 2034/3/12 (2025759600)
 2014873200, 1994310000,  // 2033:  2033/11/6 (2014873200) -- 2033/3/13 (1994310000)
 1983423600, 1962860400,  // 2032:  2032/11/7 (1983423600) -- 2032/3/14 (1962860400)
 1951369200, 1930806000,  // 2031:  2031/11/2 (1951369200) -- 2031/3/9 (1930806000)
 1919919600, 1899356400,  // 2030:  2030/11/3 (1919919600) -- 2030/3/10 (1899356400)
 1888470000, 1867906800,  // 2029:  2029/11/4 (1888470000) -- 2029/3/11 (1867906800)
 1857020400, 1836457200,  // 2028:  2028/11/5 (1857020400) -- 2028/3/12 (1836457200)
 1825570800, 1805007600,  // 2027:  2027/11/7 (1825570800) -- 2027/3/14 (1805007600)
 1793516400, 1772953200,  // 2026:  2026/11/1 (1793516400) -- 2026/3/8 (1772953200)
 1762066800, 1741503600,  // 2025:  2025/11/2 (1762066800) -- 2025/3/9 (1741503600)
 1730617200, 1710054000   // 2024:  2024/11/3 (1730617200) -- 2024/3/10 (1710054000)
}
*/

// return 1 if checked time is during DST period 
// set g_next_dst_change to next DST event
int testNsetDST(uint32_t val)
{
	// no information about times after last or before first date, so we return 0 = "no DST"
	// in cas of "before the earliest entry" we also set the next time of switch event to the lowest entry
	// if time is above "latest entry", we set it to MAX, so we don't check any more
	int arlength= sizeof(dst_switch_times) / sizeof(dst_switch_times[0]);
	if ( val >= dst_switch_times[0] ){
		g_next_dst_change = -1;		// g_next_dst_change is unsigned type, so "-1" should be MAX Value
		return 0;
	}
	if ( val <= dst_switch_times[arlength-1] ) { 
		g_next_dst_change = dst_switch_times[arlength-1]; 
		return 0;
	}
	for (int i=1; i < arlength; i++) {
//		printf("prüfe %u gegen %u\n",val,dst_switch_times[i]);
		if (val >= dst_switch_times[i]) {
//			ADDLOGF_INFO("Next switch at %u (was %u before)  - its %s\n",dst_switch_times[i-1], g_next_dst_change, i%2 == 0 ? "normal time" : "daylight saving time");
			g_DSToffset = i%2 == 1 ? 3600 : 0;
			g_next_dst_change = dst_switch_times[i-1];
			return i%2 == 1 ;
		}
	}
return 0;

}




void Clock_OnEverySecond()
{
// we will use this also for a device time independent from NTP
// so we change this variable here, in a common place ...
// device time is kept as "time at startup", so we add the uptime to
// get actual time.
// it's easier to do this here once so "g_deviceTime" is set to actual time
// instead of calculating every time we need the actual time 
    g_deviceTime = (time_t)(g_epochOnStartup + g_secondsElapsed);

#if ENABLE_CALENDAR_EVENTS
	RunClockEvents(g_deviceTime, IsTimeSynced());
#endif
}


/*
void AppendTimeInformationToHTTPIndexPage(http_request_t* request)
{
    struct tm *ltm;

    ltm = gmtime(&g_deviceTime);

    if (g_synced == true)
        hprintf255(request, "<h5>NTP (%s): Local Time: %04d/%02d/%02d %02d:%02d:%02d </h5>",
			CFG_GetNTPServer(),ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    else 
        hprintf255(request, "<h5>NTP: Syncing with %s....</h5>",CFG_GetNTPServer());
}

*/
