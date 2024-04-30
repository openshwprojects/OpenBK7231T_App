#include "new_common.h"
#if ENABLE_LOCAL_CLOCK_ADVANCED
#include "new_cfg.h" // for CFG_SetCLOCK_SETTINGS() - used in CMD_CLOCK_SetConfig()
#include <time.h>
// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include "logging/logging.h"

const char *str_rssi[] = { "N/A", "Weak", "Fair", "Good", "Excellent" };


#ifdef WIN32

#else

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION

#ifndef ssize_t
#define ssize_t int
#endif

#include "nanoprintf.h"


#ifdef WRAP_PRINTF
#define vsnprintf3 __wrap_vsnprintf
#define snprintf3 __wrap_snprintf
#define sprintf3 __wrap_sprintf
#define vsprintf3 __wrap_vsprintf
#endif

int vsnprintf3(char *buffer, size_t bufsz, const char *fmt, va_list val) {
    int const rv = npf_vsnprintf(buffer, bufsz, fmt, val);
    return rv;
}

int snprintf3(char *buffer, size_t bufsz, const char *fmt, ...) {
   	va_list val;
    va_start(val, fmt);
    int const rv = npf_vsnprintf(buffer, bufsz, fmt, val);
    va_end(val);
    return rv;
}

#define SPRINTFMAX 100
int sprintf3(char *buffer, const char *fmt, ...) {
   	va_list val;
    va_start(val, fmt);
    int const rv = npf_vsnprintf(buffer, SPRINTFMAX, fmt, val);
    va_end(val);
    return rv;
}

int vsprintf3(char *buffer, const char *fmt, va_list val) {
    int const rv = npf_vsnprintf(buffer, SPRINTFMAX, fmt, val);
    return rv;
}

#endif

#if WINDOWS
const char* strcasestr(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
		{
			if (r == 0)
			{
				r = p1;
			}

			p2++;
		}
		else
		{
			p2 = str2;
			if (r != 0)
			{
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1;
				p2++;
			}
			else
			{
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? (char*)r : 0;
}
#endif

// Why strdup breaks strings?
// backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// it got broken around 64 char
// where is buffer with [64] bytes?
// 2022-11-02 update: It was also causing crash on OpenBL602. Original strdup was crashing while my strdup works.
// Let's just rename test_strdup to strdup and let it be our main correct strdup
#if !defined(PLATFORM_W600) && !defined(PLATFORM_W800)
// W600 and W800 already seem to have a strdup?
char *strdup(const char *s)
{
    char *res;
    size_t len;

    if (s == NULL)
        return NULL;

    len = strlen(s);
    res = malloc(len + 1);
    if (res)
        memcpy(res, s, len + 1);

    return res;
}
#endif

int strIsInteger(const char *s) {
	if(s==0)
		return 0;
	if(*s == 0)
		return 0;
	if(s[0]=='0' && s[1] == 'x'){
		return 1;
	}
	while(*s) {
		if(isdigit((unsigned char)*s)==false)
			return 0;
		s++;
	}
	return 1;
}
// returns amount of space left in buffer (0=overflow happened)
int strcat_safe(char *tg, const char *src, int tgMaxLen) {
	int curOfs = 1;

	// skip
	while(*tg != 0) {
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}


int strcpy_safe_checkForChanges(char *tg, const char *src, int tgMaxLen) {
	int changesFound = 0;
	int curOfs = 0;
	// copy
	while(*src != 0) {
		if(*tg != *src) {
			changesFound++;
			*tg = *src;
		}
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			if(*tg != 0) {
				changesFound++;
				*tg = 0;
			}
			return 0;
		}
	}
	if(*tg != 0) {
		changesFound++;
		*tg = 0;
	}
	return changesFound;
}
int strcpy_safe(char *tg, const char *src, int tgMaxLen) {
	int curOfs = 0;
	// copy
	while(*src != 0) {
		*tg = *src;
		src++;
		tg++;
		curOfs++;
		if(curOfs >= tgMaxLen - 1) {
			*tg = 0;
			return 0;
		}
	}
	*tg = 0;
	return tgMaxLen-curOfs;
}

void urldecode2_safe(char *dst, const char *srcin, int maxDstLen)
{
	int curLen = 1;
        int a = 0, b = 0;
	// avoid signing issues in conversion to int for isxdigit(int c)
	const unsigned char *src = (const unsigned char *)srcin;
        while (*src) {
		if(curLen >= (maxDstLen - 1)){
			break;
		}
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
		curLen++;
        }
        *dst++ = '\0';
}

void stripDecimalPlaces(char *p, int maxDecimalPlaces) {
	while (1) {
		if (*p == '.')
			break;
		if (*p == 0)
			return;
		p++;
	}
	if (maxDecimalPlaces == 0) {
		*p = 0;
		return;
	}
	p++;
	while (maxDecimalPlaces > 0) {
		if (*p == 0)
			return;
		maxDecimalPlaces--;
		p++;
	}
	*p = 0;
}
int wal_stricmp(const char* a, const char* b) {
	int ca, cb;
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
	} while ((ca == cb) && (ca != '\0'));
	return ca - cb;
}
int wal_strnicmp(const char* a, const char* b, int count) {
	int ca, cb;
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
		count--;
	} while ((ca == cb) && (ca != '\0') && (count > 0));
	return ca - cb;
}

const char* skipToNextWord(const char* p) {
	while (isWhiteSpace(*p) == false) {
		if (*p == 0)
			return p;
		p++;
	}
	while (isWhiteSpace(*p)) {
		if (*p == 0)
			return p;
		p++;
	}
	return p;
}

int STR_ReplaceWhiteSpacesWithUnderscore(char *p) {
	int r = 0;
	while (*p) {
		if (*p == ' ' || *p == '\t') {
			r++;
			*p = '_';
		}
		p++;
	}
	return r;
}


WIFI_RSSI_LEVEL wifi_rssi_scale(int8_t rssi_value)
{
    #define LEVEL_WEAK      -70     //-70
    #define LEVEL_FAIR      -60     //-60
    #define LEVEL_GOOD      -50     //-50

    WIFI_RSSI_LEVEL retVal = NOT_CONNECTED;

    if (rssi_value <= LEVEL_WEAK)
        retVal = WEAK;
    else if ((rssi_value <= LEVEL_FAIR) && (rssi_value > LEVEL_WEAK))
        retVal = FAIR;
    else if ((rssi_value <= LEVEL_GOOD) && (rssi_value > LEVEL_FAIR))
        retVal = GOOD; 
    else if (rssi_value > LEVEL_GOOD)
        retVal = EXCELLENT;
    return retVal;
}

// functions for handling device time even without NTP driver present
// using "g_epochOnStartup" and "g_UTCoffset" if NTP not present (or not synced)

#include "driver/drv_ntp.h"

// "eoch" on startup of device; If we add g_secondsElapsed we get the actual time  
#if ENABLE_LOCAL_CLOCK
uint32_t g_epochOnStartup = 0;


#endif
// UTC offset
int g_UTCoffset = 0;
#if ENABLE_LOCAL_CLOCK_ADVANCED
// daylight saving time offset
int g_DSToffset = 0;
// epoch of next change in dst
uint32_t g_next_dst_change=0;
#endif

TickType_t lastTick=0;
// it's a pitty we cant use rtos' "xNumOfOverflows" here, but its not accessable, so we need to take care of owerflows here
// a 32 bit TickType_t counter of ms will rollover after (4294836225÷1000÷3600÷24=49,7088) ~ 49,7 days we should expect uptimes 
// bigger than this value!
//
// I don't expect uptime > 35 years, so uint8_t could be sufficient if TickType_t is 32 bit ( 255×4294967295 ms = 1,09521666×10¹² ms = 1095216660 s ~ 12676 days ~ 34,7 years)
// but just to be sure use uint16_t (65535 x 4294967295 ms = 2,814706817×10¹⁴ ms = 281470681677,825 s ~ 3257762 days ~ 8919 years)
// if TickType_t is also uint16_t it will last for approx 50 days, while uint8_t will rollover after 4,65 hours !! 
uint16_t timer_rollovers=0; 


// g_secondsElapsed is drifting of after some time (for me it was ~ 1 to 2 minutes (!) a day)
// when using rtos ticks, it was reduced to 1 to 2 seconds(!) a day
// if we want to use this for emulating an RTC, we should get the time as good as possible 

uint32_t getSecondsElapsed(){

	// xTicks are not bound to be in ms, 
	// but for all plattforms xTicks can be converted to MS with "portTICK_RATE_MS"

 	TickType_t actTick=portTICK_RATE_MS*xTaskGetTickCount();

	 // to make this work, getSecondsElapsed() must be called once before rollover, which is 
	 // no problem for the usual choice of TickType_t = uint32_t:
	 // 	rollover will take place after 4294967295 ms (almost 50 days)
	 // but it might be a more of challenge for uint16_t its with only 65535 ms (one Minute and 5 seconds)!! 
	 if (actTick < lastTick ){
		timer_rollovers++;
		ADDLOG_INFO(LOG_FEATURE_RAW, "\r\n\r\nCLOCK: Rollover of tick counter! Actual value of timer_rollovers=%u \r\n\r\n",timer_rollovers);

	 }
	 lastTick = actTick;
	 // 
	 // version 1 :
	 // use the time also to adjust g_secondsElapsed 
	 g_secondsElapsed = (uint32_t)(((uint64_t) timer_rollovers << (sizeof(TickType_t)*8) | actTick) / 1000 );
	 return  g_secondsElapsed;
	 // 
	 // possible version 2 :
	 // without adjusting g_secondsElapsed :
	// return (uint32_t)(((uint64_t) timer_rollovers << 32 | actTick) / 1000 );
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
	// DST calculation can be split in three pases during a year:
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
the regular UTC offset aund the ammount of minutes to add during dst

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
	}
	else {
		TZ = Tokenizer_GetArgInteger(0);
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

#endif	// to #if ENABLE_LOCAL_CLOCK_ADVANCED


