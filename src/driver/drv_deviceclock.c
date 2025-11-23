
//#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"

#include "drv_deviceclock.h"
#include "../libraries/obktime/obktime.h"
// functions for handling device time even without NTP driver present
// using "g_epochOnStartup" and "g_UTCoffset" if NTP not present (or not synced)

#include "drv_ntp.h"
#include <stdbool.h>


// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
uint32_t g_epochOnStartup = 0;
// UTC offset
int g_UTCoffset = 0;
void CLOCK_setDeviceTime(uint32_t time){
	ADDLOG_DEBUG(LOG_FEATURE_RAW, "CLOCK_setDeviceTime - time = %lu - g_secondsElapsed =%lu \r\n",time,g_secondsElapsed);
#if (ENABLE_DRIVER_DS3231)
	uint32_t temp = g_epochOnStartup;
#endif
	g_epochOnStartup = (uint32_t)time - g_secondsElapsed;
#if ENABLE_CLOCK_DST
	setDST();	// just to be sure: recalculate DST
#endif
#if (ENABLE_DRIVER_DS3231)
#include "drv_public.h"
#include "drv_ds3231.h"
//	ADDLOG_DEBUG(LOG_FEATURE_RAW, "CLOCK_setDeviceTime 1 - temp = %lu - g_epochOnStartup = %lu \r\n",temp,g_epochOnStartup);
	temp -= g_epochOnStartup;
//	ADDLOG_DEBUG(LOG_FEATURE_RAW, "CLOCK_setDeviceTime 2 - temp = %lu \r\n",temp);
	if (DRV_IsRunning("DS3231")) DS3231_informClockWasSet( (temp*temp > 25));		// use "force" if new time differs more than 5 seconds (temp*temp is allways positive)
#endif

//	ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_setDeviceTime - time = %lu - g_secondsElapsed =%lu - g_epochOnStartup=%lu \r\n",time,g_secondsElapsed,g_epochOnStartup);
}

void CLOCK_setDeviceTimeOffset(int offs){
//	ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_setDeviceTimeOffset - offs = %i\r\n",offs);
	g_UTCoffset = offs;
}

commandResult_t SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a, b;
	const char *arg;
	int oldOfs = g_UTCoffset;

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
		g_UTCoffset = useSign * (a * 60 * 60 + b * 60);
	}
	else {
		g_UTCoffset = Tokenizer_GetArgInteger(0) * 60 * 60;
	}
#if WINDOWS
	NTP_SetTimesZoneOfsSeconds(g_UTCoffset);
#endif
#if ENABLE_CLOCK_DST
    	setDST();	// check if local time is DST or not and set offset
#endif
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Time offset set to %i seconds"
#if ENABLE_CLOCK_DST
	" (DST offset %i seconds)"
#endif	
	,g_UTCoffset
#if ENABLE_CLOCK_DST
	,getDST_offset()
#endif	
	);
	return CMD_RES_OK;
}

commandResult_t SetDeviceTime(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
//	uint32_t t=Tokenizer_GetArgInteger(0);
	uint32_t t1=atol(Tokenizer_GetArg(0));
//	CLOCK_setDeviceTime(Tokenizer_GetArgInteger(0));
	CLOCK_setDeviceTime(t1);
#if ENABLE_CLOCK_DST
    	setDST();	// check if local time is DST or not and set offset
#endif	
	return CMD_RES_OK;
}



#if ENABLE_CLOCK_DST
static uint32_t next_DST_switch_epoch=0;

typedef struct {
    // DST configuration 1 (16 bits)
    unsigned week1 : 3;      // 0-7 (3 bits)
    unsigned month1 : 4;     // 1-12 (4 bits)
    unsigned day1 : 3;       // 0-6 (3 bits)	0=sunday 1=monday ...
    unsigned hour1 : 5;      // 0-23 (5 bits)
    unsigned isDST1 : 1;     // 0: no (end of DST) 1 yes (start of DST)
    
    // DST configuration 2 (16 bits)
    unsigned week2 : 3;      // 0-7 (3 bits)
    unsigned month2 : 4;     // 1-12 (4 bits)
    unsigned day2 : 3;       // 0-6 (3 bits)	0=sunday 1=monday ...
    unsigned hour2 : 5;      // 0-23 (5 bits)
    unsigned isDST2 : 1;     // 0: no (end of DST) 1 yes (start of DST)
    
    // DST offset in seconds (13 bits: 0-8191 seconds (136 minutes; maximum DST is 2 hours)
    unsigned DSToffset : 13;
    
    // DST state (3 bits)
    unsigned DSTactive : 1;      // 0=inactive, 1=active
    unsigned DSTinitialized : 1; // 0=uninitialized, 1=initialized
    unsigned : 1;                // Padding/reserved
} DSTcfg;


static DSTcfg dst_config = {0};

#define useDST (dst_config.DSTinitialized)

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
    CLOCK_setLatitude(atof(newValue));
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK latitude set to %s", newValue);

    newValue = Tokenizer_GetArg(1);
    CLOCK_setLongitude(atof(newValue));
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK longitude set to %s", newValue);
    return CMD_RES_OK;
}

void CLOCK_setLatitude(float lat){
    sun_data.latitude = (int) (lat * SUN_DATA_COORD_MULT);
};

void CLOCK_setLongitude(float longi){
	sun_data.longitude = (int) (longi * SUN_DATA_COORD_MULT);
};


float CLOCK_GetLatitude(){
    return (float)sun_data.latitude/SUN_DATA_COORD_MULT;
};

float CLOCK_GetLongitude(){
	return (float)sun_data.longitude/SUN_DATA_COORD_MULT ;
};

int CLOCK_GetSunrise()
{
	byte hour, minute;
	int sunriseInSecondsFromMidnight;

	CLOCK_CalculateSunrise(&hour, &minute);
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK sunrise is at %02i:%02i", hour, minute);
	sunriseInSecondsFromMidnight = ((int)hour * 3600) + ((int)minute * 60);
	return sunriseInSecondsFromMidnight;
}

int CLOCK_GetSunset()
{
	byte hour, minute;
	int sunsetInSecondsFromMidnight;

	CLOCK_CalculateSunset(&hour, &minute);
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK sunset is at %02i:%02i", hour, minute);
	sunsetInSecondsFromMidnight =  ((int)hour * 3600) + ((int)minute * 60);
	return sunsetInSecondsFromMidnight;
}
#endif

// Shared implementation helper
static TimeComponents getCurrentComponents(void) {
    return calculateComponents(Clock_GetCurrentTime());
}

// Individual getter functions
int CLOCK_GetSecond(void) {
    return getCurrentComponents().second;
}

int CLOCK_GetMinute(void) {
    return getCurrentComponents().minute;
}

int CLOCK_GetHour(void) {
    return getCurrentComponents().hour;
}

int CLOCK_GetMDay(void) {
    return getCurrentComponents().day;
}

int CLOCK_GetMonth(void) {
    return getCurrentComponents().month;
}

int CLOCK_GetYear(void) {
    return getCurrentComponents().year;
}

int CLOCK_GetWeekDay(void) {
    return getCurrentComponents().wday;
}


#if ENABLE_CLOCK_DST


// new approach for "RuleToTime" to find epoch of DST transitions
// we will use "normal" weekday count here (sun=0, so if we use Tasmota-style sun=1, 
// do fix that when calling  this function !!!
uint32_t RuleToTime(uint8_t dayOfWeek, uint8_t month, uint8_t weekNum, uint8_t hour, uint16_t year) {
    // check for valid input
    if (month < 1 || month > 12 || weekNum > 5 || dayOfWeek > 6) return 0;
    // when looking for "last" occurrence, we search the first in following month and then substract 7 days
    // else we search the first occurence in given month. If we look for second, third ... occurrenc, advance multiple of 7 days )  
    int8_t daysToAdd = (weekNum == 0) ? -7 : (weekNum - 1) * 7;


    // not nice, but working: if rule is "last day" in December, we will ask for a date in January of next year.
    // E.g. to get the "last sunday in December 2025 at 4", we would call e.g. "dateToEpoch(2026, 1, 0, 4, 0, 0);"
    // 
    // BUT we can also call "dateToEpoch(2025, 13, 0, 4, 0, 0);" in this special case, knowing the implementation ao dateToEpoch:  
    // 		We add all days of the years before and then the "days per month" _before_ the actual month +  the days in the actual month
    // 		In this special case (month=13), we only add all days per month up to 12, which is a valid operation (days in a year = sum of days in january to december)
    //
    // 		So we can avoid another special case for December and only take the disadvantage not to add 365 (or 366) in one but in 12 parts...
    //
    uint32_t retval = dateToEpoch(year, month + (weekNum == 0), 1, hour, 0, 0);
    
    uint8_t firstWeekday = (retval / SECS_PER_DAY + 4) % 7;		// 1970-01-01 is Thursday ( = 4)
    
    // if the weekday of the first day in a month is the one we are looking for (dayOfWeek == firstWeekday), we are done (0+7)%7=0 .
    // else we need to advance to that day 
    daysToAdd += (dayOfWeek - firstWeekday + 7) % 7;
    
    
    return retval + daysToAdd * SECS_PER_DAY;
}



int getDST_offset() {
    return dst_config.DSTactive ? dst_config.DSToffset : 0;
}

int IsDST() {
    return dst_config.DSTactive;
}

bool IsDST_initialized() {
    return dst_config.DSTinitialized;
}


// make sure pointer is e.g. uint32_t DST[2];  
void getDSTtransition(uint32_t * DST){
	// if first entry starts DST, isDST2 will be 0 else it will be 1
	DST[dst_config.isDST2] = (RuleToTime(dst_config.day1, dst_config.month1, dst_config.week1, dst_config.hour1, CLOCK_GetYear()));
	// if second entry starts DST, isDST1 will be 0 else it will be 1
	DST[dst_config.isDST1] = (RuleToTime(dst_config.day2, dst_config.month2, dst_config.week2, dst_config.hour2, CLOCK_GetYear()));
}

// this will calculate the actual DST stage and the time of the next DST switch

uint32_t setDST() {
    if (!dst_config.DSTinitialized || !Clock_IsTimeSynced()) {
        return 0;
    }
    
    uint16_t year = (uint16_t)CLOCK_GetYear();
    time_t tempt = (time_t)Clock_GetCurrentTimeWithoutOffset();
    int old_DST_active = dst_config.DSTactive;

    next_DST_switch_epoch = (RuleToTime(dst_config.day1, dst_config.month1, dst_config.week1, dst_config.hour1, year) - g_UTCoffset - dst_config.isDST2 * dst_config.DSToffset);
    if ( tempt < next_DST_switch_epoch){
    	dst_config.DSTactive = dst_config.isDST2;
    }
    else {
    	next_DST_switch_epoch = (RuleToTime(dst_config.day2, dst_config.month2, dst_config.week2, dst_config.hour2, year) - g_UTCoffset - dst_config.isDST1 * dst_config.DSToffset);
    	if (tempt <  next_DST_switch_epoch){
		dst_config.DSTactive = dst_config.isDST1;
    	}
    	else {
    		next_DST_switch_epoch = RuleToTime(dst_config.day1, dst_config.month1, dst_config.week1, dst_config.hour1, year+1) - g_UTCoffset - (dst_config.isDST2) * dst_config.DSToffset;
		dst_config.DSTactive = dst_config.isDST2;
    	}
    }

    tempt = next_DST_switch_epoch + (uint32_t)g_UTCoffset + (dst_config.DSTactive)*dst_config.DSToffset;	// DST calculation is in UTC, so add offset for local time
#if ENABLE_CLOCK_SUNRISE_SUNSET
    if (old_DST_active != dst_config.DSTactive){
	// if we had a DST switch, we might corect sunset/sunrise events, which were calculated before (with "previous" DST settings)
	// if we changed to DST, we need to add DST_offset (old_DST_active = 0)
	// if we were in DST before switch, we need to sub DST_offset (old_DST_active = 1)
	int fix_DSTMinutes = (dst_config.DSToffset / 60);
	fix_DSTMinutes *= old_DST_active ? -1 : 1;
	ADDLOG_INFO(LOG_FEATURE_RAW, "DST switch - calling  fix_DSTforEvents(%d)\r\n", fix_DSTMinutes );
	fix_DSTforEvents(fix_DSTMinutes);
    }
#endif
    ADDLOG_INFO(LOG_FEATURE_RAW, "In %s time - next DST switch at %u (%s) \r\n", (dst_config.DSTactive)?"summer":"standard",  (uint32_t)tempt, TS2STR(tempt,TIME_FORMAT_LONG));

    return dst_config.DSTactive;
}



//
//	             |         |-- 1st rule: last_week March sunday 2_o_clock 60_minutes_DST_after_this_time  
//	CLOCK_setDST 0 3 1 2 60 0 10 1 3 0	
//	                       |         |-- 2nd_rule: last_week October sunday 3_o_clock 0_minutes_DST_after_this_time 
commandResult_t CLOCK_SetDST(const int *args) {
    // d1/d2 for day and DST values, needed more than once, so store values to avoid multiple "GetArg" calls
    uint8_t d1,d2;

    // DST1 must be before DST2
    int add = (args[1] < args[6])? 0 : 5;		// check if first entry is "before" second (compare month number)
    dst_config.week1 = args[add];
    dst_config.month1 = args[1+add];
    dst_config.day1 = args[2+add]-1;
    dst_config.hour1 = args[3+add];
    d1 = args[4+add];	// first DST offset
    // Second DST configuration - if we added 5 for first DST, we need to sub 5 now (no change for add=0)
    add *= -1;
    dst_config.week2 = args[5+add];
    dst_config.month2 = args[6+add];
    dst_config.day2 = args[7+add]-1;
    dst_config.hour2 = args[8+add];
    d2 = args[9+add];	// second DST offset

    // no case needed for "DSToffset": one entry must be 0, one != 0, so if valid, the sum is the DST offset 
    dst_config.DSToffset = 60*(d1+d2)*(d1*d2 == 0);
    if ( dst_config.DSToffset == 0 )  return CMD_RES_BAD_ARGUMENT;		// neither two times 0 nor two times != 0 is valid !
    

    dst_config.isDST1 = (d1 != 0);
    dst_config.isDST2 = (d2 != 0);

    if (Clock_IsTimeSynced()) setDST();
    
    dst_config.DSTinitialized = 1;
    
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "DST config set: offset=%d seconds", 
             dst_config.DSToffset);
    return CMD_RES_OK;
}

//
//	             |         |-- 1st rule: last_week March sunday 2_o_clock 60_minutes_DST_after_this_time  
//	CLOCK_setDST 0 3 1 2 60 0 10 1 3 0	
//	                       |         |-- 2nd_rule: last_week October sunday 3_o_clock 0_minutes_DST_after_this_time 
commandResult_t CMD_CLOCK_SetDST(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 10)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    int clkargs[10];
    for (int i=0; i<10; i++){
    	clkargs[i]=Tokenizer_GetArgInteger(i);
    }
    return CLOCK_SetDST(clkargs);
}




// to keep compatibility to prior command
commandResult_t CMD_CLOCK_CalcDST(const void *context, const char *cmd, const char *args, int cmdFlags) {
	// arguments:  nthWeekEnd, monthEnd, dayEnd, hourEnd, nthWeekStart, monthStart, dayStart, hourStart,g_DST_offset
	// we know that new command "CLOCK_setDST" will take care or finding first or last DST event, so only add "0" as DST-Offset for "ending" (arg[4])
	// and use 1 as default DST if non given (arg[9])
	//
	//  for reference
	//
	//
	//	             |         |-- 1st rule: last_week March sunday 2_o_clock 60_minutes_DST_after_this_time  
	//	CLOCK_setDST 0 3 1 2 60 0 10 1 3 0	
	//	                       |         |-- 2nd_rule: last_week October sunday 3_o_clock 0_minutes_DST_after_this_time 
	//
	
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 8)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	
	int clkargs[10];
	clkargs[4]=0;	// starts with "DST-End", so after this DST offset is 0
	for (int i=0; i<4; i++){
		clkargs[i]=Tokenizer_GetArgInteger(i);
	}
	for (int i=4; i<8; i++){
		clkargs[i+1]=Tokenizer_GetArgInteger(i);
	}
	clkargs[9]=Tokenizer_GetArgIntegerDefault(8, 1)*60;	// we'll use minutes with CLOCK_SetDST
	return CLOCK_SetDST(clkargs);
}


int Time_IsDST(){ 
	return IsDST();
}
#endif


void CLOCK_Init() {

#if ENABLE_CLOCK_SUNRISE_SUNSET
	//cmddetail:{"name":"clock_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Sets the devices latitude and longitude",
	//cmddetail:"fn":"CLOCK_SetLatlong","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":"CLOCK_SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("clock_setLatLong", CLOCK_SetLatlong, NULL);    
// and register an alias for backward compatibility
	//cmddetail:{"name":"ntp_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Depreciated! Only for backward compatibility! Please use 'clock_setLatLong' in the future!",
	//cmddetail:"fn":"CLOCK_SetLatlong","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":"ntp_SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("ntp_setLatLong", CLOCK_SetLatlong, NULL);    
#endif
#if ENABLE_CALENDAR_EVENTS
	CLOCK_Init_Events();
#endif
#if ENABLE_CLOCK_DST
	//cmddetail:{"name":"CLOCK_setDST","args":" Rule# [1/2] nthWeek month day hour DSToffset [additional minutes _after_ this Point: <DST-Offset> for "start" of DST, 0 for "end" of DST]",
	//cmddetail:"descr":"Checks, if actual time is during DST or not.",
	//cmddetail:"fn":"CLOCK_CalcDST","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":"CLOCK_setDST 0 3 1 2 1 0 10 1 3 0	-- 1st rule: last_week March sunday 2_o_clock 1_hour_DST_after_this_time -- 2nd_rule: last_week October sunday 3_o_clock 0_hours_DST_after_this_time "}
    CMD_RegisterCommand("clock_setDST",CMD_CLOCK_SetDST, NULL);
	//cmddetail:{"name":"CLOCK_calcDST","args":" Depreciated! Only for backward compatibility! Please use 'CLOCK_setDST' in the future!",
	//cmddetail:"descr":"Sets DST settings.",
	//cmddetail:"fn":"CLOCK_CalcDST","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":"CLOCK_calcDST 0 10 1 3 0 3 1 2 1 	-- DST-End: last_week(0) October(10) sunday(1) 3_o_clock(3)  - DST-Start: last_week(0) March(3) sunday(1) 2_o_clock(2) 1_hour_DST_offset"}
    CMD_RegisterCommand("clock_calcDST",CMD_CLOCK_CalcDST, NULL);
    
    dst_config.DSTinitialized = 0;
    
#endif
	//cmddetail:{"name":"clock_setTZ","args":"[Value]",
	//cmddetail:"descr":"Sets the time zone offset in hours. Also supports HH:MM syntax if you want to specify value in minutes. For negative values, use -HH:MM syntax, for example -5:30 will shift time by 5 hours and 30 minutes negative.",
	//cmddetail:"fn":"SetTimeZoneOfs","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_setTZ",SetTimeZoneOfs, NULL);
	//cmddetail:{"name":"clock_setTime","args":"[Value]",
	//cmddetail:"descr":"Sets the time of device in seconds after 19700101.",
	//cmddetail:"fn":"SetDeviceTime","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_setTime",SetDeviceTime, NULL);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK driver initialized.");
}

void CLOCK_OnEverySecond()
{

#if ENABLE_CALENDAR_EVENTS
	CLOCK_RunEvents(Clock_GetCurrentTime(), Clock_IsTimeSynced());
#endif
#if ENABLE_CLOCK_DST
    if (useDST &&  Clock_IsTimeSynced() && (Clock_GetCurrentTimeWithoutOffset() >= next_DST_switch_epoch)){
    	int old_DST=getDST_offset();
	setDST();
    	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Passed DST switch time - recalculated DST offset. Was:%i - now:%i",old_DST,getDST_offset());
    }
#endif


}


uint32_t Clock_GetCurrentTime(){ 			// replacement for NTP_GetCurrentTime() to return time regardless of NTP present/running
// if we use "LOCAL_CLOCK", NTP will set this clock if enabled, so no further check needed
uint32_t temp=0;
	if (g_epochOnStartup > 10) {
		temp = g_epochOnStartup + g_secondsElapsed + g_UTCoffset;
#if ENABLE_CLOCK_DST
		temp +=  getDST_offset();
#endif 
	}	// no "else" needed, will return 0 anyway if we don't return here
//    	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Clock_GetCurrentTime - returning :%lu (g_epochOnStartup=%lu g_secondsElapsed=%lu g_UTCoffset=%i DST=%i)",temp,g_epochOnStartup, g_secondsElapsed, g_UTCoffset,getDST_offset());
	return temp;					// we will report 1970-01-01 if no time present - avoids "hack" e.g. in json status ...

};

uint32_t Clock_GetCurrentTimeWithoutOffset(){ 	// ... same forNTP_GetCurrentTimeWithoutOffset()...
	if (g_epochOnStartup > 10) {
		return g_epochOnStartup + g_secondsElapsed;
	}
	return  0;
};

bool Clock_IsTimeSynced(){ 				// ... and for NTP_IsTimeSynced()
	if (g_epochOnStartup > 10) {
		return true;
	}
#if ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return true;
	}
#endif
	return  false;
}

int Clock_GetTimesZoneOfsSeconds()			// ... and for NTP_GetTimesZoneOfsSeconds()
{
	if (g_epochOnStartup > 10) {
		return g_UTCoffset 
#if ENABLE_CLOCK_DST
		+ getDST_offset()
#endif 
		;
	}	// no "else" needed, will return 0 anyway if we don't return here
	return 0;
}
void CLOCK_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	if (bPreState)
		return;
	uint32_t tempt=Clock_GetCurrentTime();
#if ENABLE_NTP
	char ntpinfo[50]={0};
	if (NTP_IsTimeSynced()){
		sprintf(ntpinfo," (NTP-Server: %s)",CFG_GetNTPServer());
	}
#endif
	if (Clock_IsTimeSynced()) hprintf255(request, "<h5>Local clock: %s"
#if ENABLE_CLOCK_DST
	"%s"
#endif 
#if ENABLE_NTP
	"%s"
#endif
	"</h5>",TS2STR(tempt,TIME_FORMAT_LONG)
#if ENABLE_CLOCK_DST
	, ! dst_config.DSTinitialized ? "" : IsDST()?" (summer-time)":" (winter-time)"
#endif 
#if ENABLE_NTP
	, ntpinfo
#endif
	);
}


