
#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"
#include "drv_local.h" 		//for DRV_IsRunning


#include "drv_deviceclock.h"
// functions for handling device time even without NTP driver present
// using "g_epochOnStartup" and "g_UTCoffset" if NTP not present (or not synced)

#include "drv_ntp.h"

// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
uint32_t g_epochOnStartup = 0;
// UTC offset
int g_UTCoffset = 0;


#if ENABLE_CLOCK_DST
static uint32_t Start_DST_epoch=0 , End_DST_epoch=0, next_DST_switch_epoch=0;
static uint8_t nthWeekEnd, monthEnd, dayEnd, hourEnd, nthWeekStart, monthStart, dayStart, hourStart;
static int8_t g_DST_offset=0, g_DST=-128;	// g_DST_offset: offset during DST; 0: unset / g_DST: actual DST_offset in hours; -128: not initialised
#define useDST (g_DST_offset)
const uint32_t SECS_PER_MIN = 60UL;
const uint32_t SECS_PER_HOUR = 3600UL;
const uint32_t SECS_PER_DAY = 3600UL * 24UL;
const uint32_t MINS_PER_HOUR = 60UL;
#define LEAP_YEAR(Y)  ((!(Y%4) && (Y%100)) || !(Y%400))
static const uint8_t DaysMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#endif

extern void CLOCK_Init_Events(void);
extern void CLOCK_RunEvents(unsigned int newTime, bool bTimeValid);

#if ENABLE_CLOCK_SUNRISE_SUNSET
extern void CLOCK_CalculateSunrise(byte *outHour, byte *outMinute);
extern void CLOCK_CalculateSunset(byte *outHour, byte *outMinute);
#endif

// leave it for the moment, until a CLOCK logging is possible ...
#define LOG_FEATURE LOG_FEATURE_NTP


void CLOCK_setDeviceTime(uint32_t time){
	ADDLOG_DEBUG(LOG_FEATURE_RAW, "CLOCK_setDeviceTime - time = %lu - g_secondsElapsed =%lu \r\n",time,g_secondsElapsed);
	g_epochOnStartup = (uint32_t)time - g_secondsElapsed;
#if ENABLE_CLOCK_DST
    setDST();	// just to be sure: recalculate DST
#endif

//	ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_setDeviceTime - time = %lu - g_secondsElapsed =%lu - g_epochOnStartup=%lu \r\n",time,g_secondsElapsed,g_epochOnStartup);
}

void CLOCK_setDeviceTimeOffset(int offs){
//	ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_setDeviceTimeOffset - offs = %i\r\n",offs);
	g_UTCoffset = offs;
#if WINDOWS
// removed command
//	NTP_SetTimesZoneOfsSeconds(g_UTCoffset);
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

}

commandResult_t CLOCK_SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a, b;
	const char *arg;
	int Ofs = 0;

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
		Ofs = useSign * (a * 60 * 60 + b * 60);
	}
	else {
		Ofs = Tokenizer_GetArgInteger(0) * 60 * 60;
	}
	CLOCK_setDeviceTimeOffset(Ofs);
	return CMD_RES_OK;
}

commandResult_t CLOCK_SetDeviceTime(const void *context, const char *cmd, const char *args, int cmdFlags) {
    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CLOCK_setDeviceTime(Tokenizer_GetArgInteger(0));
#if ENABLE_CLOCK_DST
    	setDST();	// check if local time is DST or not and set offset
#endif	
	return CMD_RES_OK;
}


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
// simplify, no need for gmtime here
       return ((int)(Clock_GetCurrentTime()/ 86400 )+4) % 7;   // 1970-01-01 was a Thursday
}
int CLOCK_GetHour() {
// simplify, no need for gmtime here
	return (Clock_GetCurrentTime()/3600) % 24;
}
int CLOCK_GetMinute() {
// simplify, no need for gmtime here
	return (Clock_GetCurrentTime()/60) % 60;
}
int CLOCK_GetSecond() {
// simplify, no need for gmtime here
	return Clock_GetCurrentTime() % 60;
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

#if ENABLE_CLOCK_DST
// derived from tasmotas "RuleToTime"
uint32_t RuleToTime(uint8_t dow, uint8_t mo, uint8_t week,  uint8_t hr, int yr) {
  struct tm tm={0};
  uint32_t t=0;
  int i;
  uint8_t m=mo-1;		// we use struct tm here, so month values are 0..11
  uint8_t w=week;		// m and w are copies of mo(nth) and week

  if (0 == w) {			// for "Last XX" (w=0), we compute first occurence in following month and go back 7 days
    if (++m > 11) {		// so go to the next month ...
      m = 0;			// .. and to next year, if we need last XX in December
      yr++;
    }
    w = 1;			// search first week of next month, we will subtract 7 days later
  }

  // avoid mktime - this enlarges the image especially for BL602!
  // so calculate seconds from epoch locally
  // we start by calculating the number of days since 1970 - will also be used to get weekday
  uint16_t days;
  days = (yr - 1970) * 365;			// days per full years
  // add one day every leap year - first leap after 1970 is 1972, possible leap years every 4 years
  for (i=1972; i < yr; i+=4) days += LEAP_YEAR(i);
  for (i=0; i<m; i++){
  	if (i==1 && LEAP_YEAR(yr)){
  		days += 29 ;
  	} else {
  		days += DaysMonth[i];
  	}
  } 	
 // we search for the first day of a month, so no offset for days needed
 // calculate weekday from number of days since 1970
   uint8_t wday = (days + 4) % 7 +1 ;	// 1970-01-01 is Thursday ( = 4) -- since tasmota defines Sunday=1, add 1
 // convert days to seconds and add seconds from hours
  t = days * SECS_PER_DAY + hr * SECS_PER_HOUR;

  // we calculated the weekday of the first day in month
  // now calculate the requested day:
  // (dow - wday + 7)%7 will give first "dow" in the month:
  // if dow == wday: (0+7)%7 = 0 
  // if it's X days later, there are two cases:
  // 1.	searched dow > wday: we need to advance dow - wday (adding 7 and % 7 has no impact)
  // 2.	searched dow < wday: we need to advance the days from wday to Saturday (=day 7) and add dow
  // 		from wday to Saturday:	7 - wday
  // 		add dow: + dow		--> so its 7 - wday + dow = dow - wday + 7 
  // so the complete formular for the first wday in month is     			--> (dow - wday + 7) % 7
  // if we need the second, third ... we need to add 7 days per week after the first 	--> 7 * (w-1)
  t += (7 * (w - 1) + (dow - wday + 7) % 7) * SECS_PER_DAY;


  // sepecial case if we searched for the "last" weekday in month, we calculated the first occurence in the
  // following month. So the requested "last" in the month before is 7 days earlier 
  if (0 == week) {
    t -= 7 * SECS_PER_DAY;  // back one week if this is a "Last XX" rule
  }
  return t;
}

int getDST_offset()
{
	return (g_DST%128)*3600;	// return 0 if "unset" because -128%128 = 0 
};


// this will calculate the actual DST stage and the time of the next DST switch
uint32_t setDST()
{
   if (useDST && Clock_IsTimeSynced()){
	int year=CLOCK_GetYear();
	time_t tempt = (time_t)Clock_GetCurrentTimeWithoutOffset();
	int8_t old_DST=0;
	char tmp[40];	// to hold date string of timestamp
	Start_DST_epoch = RuleToTime(dayStart,monthStart,nthWeekStart,hourStart,year)-g_UTCoffset;	// will return the start time, which is given as local time. To get UTC time, remove offset 
	End_DST_epoch = RuleToTime(dayEnd,monthEnd,nthWeekEnd,hourEnd,year) - g_UTCoffset - g_DST_offset*3600; // will return the end time, which is given as local time. To get UTC time, remove offset and DST offset
	old_DST = g_DST%128;	// 0 if "unset" because -128%128 = 0 

	if ( Start_DST_epoch < End_DST_epoch ) {	// Northern --> begin before end
		if (tempt < Start_DST_epoch) {
			// we are in winter time before start of summer time
			next_DST_switch_epoch=Start_DST_epoch;
			g_DST=0;
//			tempt = (time_t)Start_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "Before first DST switch in %i. Info: DST starts at %lu (%.24s local time)\r\n",year,Start_DST_epoch,ctime(&tempt));
		} else if (tempt < End_DST_epoch ){
			// we in summer time
			next_DST_switch_epoch=End_DST_epoch;
			g_DST=g_DST_offset;
//			tempt = (time_t)End_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "In DST of %i. Info: DST ends at %lu (%.24s local time)\r\n",year,End_DST_epoch,ctime(&tempt));
		} else {
			// we are in winter time, after summer time --> next DST starts in next year
			Start_DST_epoch = RuleToTime(dayStart,monthStart,nthWeekStart,hourStart,year+1);
			next_DST_switch_epoch=Start_DST_epoch;
			g_DST=0;
//			tempt = (time_t)Start_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "After DST in %i. Info: Next DST start in next year at %lu (%.24s local time)\r\n",year,Start_DST_epoch,ctime(&tempt));
		}
	} else {	// so end of DST before begin of DST --> southern
			if (tempt < End_DST_epoch) {
			// we in summer time at beginning of the yeay
			next_DST_switch_epoch=End_DST_epoch;
			g_DST=g_DST_offset;
//			tempt = (time_t)End_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "In first DST period of %i. Info: DST ends at %lu (%.24s local time)\r\n",year,End_DST_epoch,ctime(&tempt));
		} else if (tempt < Start_DST_epoch ){
			// we are in winter time 
			next_DST_switch_epoch=Start_DST_epoch;
			g_DST=0;
//			tempt = (time_t)Start_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "Regular time of %i. Info: DST starts at %lu (%.24s local time)\r\n",year,Start_DST_epoch,ctime(&tempt));
		} else {
			// we in summer time at the end of the year --> DST will end next year
			End_DST_epoch = RuleToTime(dayEnd,monthEnd,nthWeekEnd,hourEnd,year+1);
			next_DST_switch_epoch=End_DST_epoch;
			g_DST=g_DST_offset;
//			tempt = (time_t)End_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "In second DST of %i. Info: DST ends next year at %lu (%.24s local time)\r\n",year,End_DST_epoch,ctime(&tempt));
		}
	}
//	g_ntpTime += (g_DST-old_DST)*60*setCLOCK;
	tempt = (time_t)next_DST_switch_epoch + g_UTCoffset + g_DST_offset*3600*(g_DST>0);

	struct tm *ltm;
	ltm = gmtime(&tempt);
	ADDLOG_INFO(LOG_FEATURE_RAW, "In %s time - next DST switch at %lu (UTC) (" LTSTR ")\r\n",
	(g_DST)?"summer":"standard", next_DST_switch_epoch, LTM2TIME(ltm));
	return (g_DST!=0);
  }
  else return 0;	// DST not (yet) set or can't be calculated (if ntp not synced)

}

int IsDST()
{
	if (( g_DST == -128) || (Clock_GetCurrentTimeWithoutOffset() > next_DST_switch_epoch)) return (setDST()!=0);	// only in case we don't know DST status, calculate it
	return g_DST!=0;									// otherwise we can safely return the prevously calculated value
}

commandResult_t CLOCK_CalcDST(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int year=CLOCK_GetYear();
	time_t te,tb;		// time_t of timestamps needed for ctime() to get string of timestamp

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 8)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

//	ADDLOG_INFO(LOG_FEATURE_RAW, "CLOCK_SetConfigs: we have %u Args\r\n", Tokenizer_GetArgsCount());
	nthWeekEnd = Tokenizer_GetArgInteger(0);
	monthEnd = Tokenizer_GetArgInteger(1);
	dayEnd = Tokenizer_GetArgInteger(2);
	hourEnd = Tokenizer_GetArgInteger(3);
	nthWeekStart = Tokenizer_GetArgInteger(4);
	monthStart = Tokenizer_GetArgInteger(5);
	dayStart = Tokenizer_GetArgInteger(6);
	hourStart = Tokenizer_GetArgInteger(7);
	g_DST_offset=Tokenizer_GetArgIntegerDefault(8, 1);
	ADDLOG_INFO(LOG_FEATURE_RAW, "read values: %u,%u,%u,%u,%u,%u,%u,%u,(%u)\r\n",  nthWeekEnd, monthEnd, dayEnd, hourEnd, nthWeekStart, monthStart, dayStart, hourStart,g_DST_offset);

/*	Start_DST_epoch = RuleToTime(dayStart,monthStart,nthWeekStart,hourStart,year);
	End_DST_epoch = RuleToTime(dayEnd,monthEnd,nthWeekEnd,hourEnd,year);
	te=(time_t)End_DST_epoch;
	tb=(time_t)Start_DST_epoch;
	
	ADDLOG_INFO(LOG_FEATURE_RAW, "Calculated DST switch epochs in %i. DST start at %lu (%.24s local time) - DST end at %lu (%.24s local time)\r\n",year,Start_DST_epoch,ctime(&tb),End_DST_epoch,ctime(&te));
*/
	return CMD_RES_OK;
};

int Time_IsDST(){ 
	return IsDST();
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
#if ENABLE_CLOCK_DST
	//cmddetail:{"name":"CLOCK_CalcDST","args":"[nthWeekEnd monthEnd dayEnd hourEnd nthWeekStart monthStart dayStart hourStart [g_DSToffset minutes - default is 60 minutes if unset]",
	//cmddetail:"descr":"Checks, if actual time is during DST or not.",
	//cmddetail:"fn":"CLOCK_CalcDST","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_calcDST",CLOCK_CalcDST, NULL);
#endif
	//cmddetail:{"name":"clock_setTZ","args":"[Value]",
	//cmddetail:"descr":"Sets the time zone offset in hours. Also supports HH:MM syntax if you want to specify value in minutes. For negative values, use -HH:MM syntax, for example -5:30 will shift time by 5 hours and 30 minutes negative.",
	//cmddetail:"fn":"SetTimeZoneOfs","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_setTZ",CLOCK_SetTimeZoneOfs, NULL);
	//cmddetail:{"name":"clock_setTTime","args":"[Value]",
	//cmddetail:"descr":"Sets the time of device in seconds after 19700101.",
	//cmddetail:"fn":"SetDeviceTime","file":"driver/drv_deviceclock.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_setTime",CLOCK_SetDeviceTime, NULL);

    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "CLOCK driver initialized.");
}

void CLOCK_OnEverySecond()
{

#if ENABLE_CALENDAR_EVENTS
	CLOCK_RunEvents(Clock_GetCurrentTime(), Clock_IsTimeSynced());
#endif
#if ENABLE_CLOCK_DST
    if (useDST && (Clock_GetCurrentTimeWithoutOffset() >= next_DST_switch_epoch)){
    	int8_t old_DST=g_DST;
	setDST();
    	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Passed DST switch time - recalculated DST offset. Was:%ih - now:%ih",old_DST,g_DST);
    }
#endif


}



uint32_t Clock_GetCurrentTime(){ 			// replacement for NTP_GetCurrentTime() to return time regardless of NTP present/running
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



bool Clock_IsTimeSynced(){ 				// ... and for NTP_IsTimeSynced()
	return  (g_epochOnStartup > 10);
}

void CLOCK_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	if (bPreState)
		return;
	struct tm *ltm;
	time_t tempt = (time_t)Clock_GetCurrentTime();
	ltm = gmtime(&tempt);
	char temp[128]={0};
	if (DRV_IsRunning("NTP")){
		NTP_Server_Status(temp,sizeof(temp)-1);
	}
	if (Clock_IsTimeSynced()) hprintf255(request, "<h5>" LTSTR " (%u) %s</h5>",LTM2TIME(ltm),(uint32_t)tempt,temp);
}


