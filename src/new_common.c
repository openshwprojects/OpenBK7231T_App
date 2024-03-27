#include "new_common.h"
#include <stdio.h>
#include <stdlib.h>

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
uint32_t g_epochOnStartup = 0;
// UTC offset
int g_UTCoffset = 0;
// daylight saving time offset
int g_DSToffset = 0;
// epoch of next change in dst
uint32_t g_next_dst_change=0;

TickType_t lastTick=0;
uint8_t timer_rollovers=0; // I don't expect uptime > 35 years ...



// g_secondsElapsed is drifting of after some time (for me it was ~ 1 to 2 minutes (!) a day)
// when using rtos ticks, it was reduced to 1 to 2 seconds(!) a day
// if we want to use this for emulating an RTC, we should get the time as good as possible 

uint32_t getSecondsElapsed(){
 TickType_t actTick=xTaskGetTickCount();
 // to make this work, getSecondsElapsed() must be called once before rollover, which is 
 // no problem for the usual choice of TickType_t = uint32_t:
 // 	rollover will take place after 4294967295 ms (almost 50 days)
 // but it might be a more of challenge for uint16_t its with only 65535 ms (one Minute and 5 seconds)!! 
 if (actTick < lastTick ) timer_rollovers++;
 lastTick = actTick;
 // 
 // version 1 :
 // use the time also to adjust g_secondsElapsed 
 g_secondsElapsed = (uint32_t)(((uint64_t) timer_rollovers << 32 | actTick) / 1000 );
 return  g_secondsElapsed;
 // 
 // possible version 2 :
 // without adjusting g_secondsElapsed :
// return (uint32_t)(((uint64_t) timer_rollovers << 32 | actTick) / 1000 );
}



uint32_t GetCurrentTime(){ 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
#ifdef ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return (uint32_t)NTP_GetCurrentTime();
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return it - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset + g_DSToffset : 0;
};

uint32_t GetCurrentTimeNew(){ 			// might replace for NTP_GetCurrentTime() to return time regardless of NTP present/running
	// if g_epochOnStartup is set, return it - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? g_epochOnStartup + g_secondsElapsed + g_UTCoffset + g_DSToffset : 0;
};

uint32_t GetCurrentTimeWithoutOffset(){ 	// ... same forNTP_GetCurrentTimeWithoutOffset()...
#ifdef ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return (uint32_t)NTP_GetCurrentTimeWithoutOffset();
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return time - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? g_epochOnStartup  + g_secondsElapsed  : 0;
};



bool IsTimeSynced(){ 				// ... and for NTP_IsTimeSynced()
#ifdef ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return true;
	}
	// even if NTP is enabled, but NTP is not synced, we might have g_epochOnStartup set, so go on
#endif
	// if g_epochOnStartup is set, return time - we migth (mis-)use a very small value as status,
	// so check for > 10. A hack, but as we will not go back in time ... 
	return g_epochOnStartup > 10 ? true : false;

}


int GetTimesZoneOfsSeconds()			// ... and for NTP_GetTimesZoneOfsSeconds()
{
#ifdef ENABLE_NTP
	if (NTP_IsTimeSynced() == true) {
		return NTP_GetTimesZoneOfsSeconds();
	}
	// even if NTP is enabled, but NTP is not synced, we might have UTC offset set with g_epochOnStartup set, so go on
#endif
	// ...a nd again: check if g_epochOnStartup is set  ... 
	return g_epochOnStartup > 10 ? g_UTCoffset + g_DSToffset: 0;

}

// check if date is in DST or not
// test against (GMT)epoch dates of start and end in EU
// from 2024 up to 2030
bool is_EU_dst(uint32_t val){
	// 2024: 2024-03-31 02:0:00 +0000 to 2024-10-26 03:0:00 +0000 
	if ( val >= 1711850400 && val <= 1729911600) return 1 ;
	// 2025: 2025-03-30 02:0:00 +0000 to 2025-10-26 03:0:00 +0000 
	if ( val >= 1743300000 && val <= 1761447600) return 1 ;
	// 2026: 2026-03-29 02:0:00 +0000 to 2026-10-25 03:0:00 +0000 
	if ( val >= 1774749600 && val <= 1792897200) return 1 ;
	// 2027: 2027-03-28 02:0:00 +0000 to 2027-10-31 03:0:00 +0000 
	if ( val >= 1806199200 && val <= 1824951600) return 1 ;
	// 2028: 2028-03-26 02:0:00 +0000 to 2028-10-29 03:0:00 +0000 
	if ( val >= 1837648800 && val <= 1856401200) return 1 ;
	// 2029: 2029-03-25 02:0:00 +0000 to 2029-10-28 03:0:00 +0000 
	if ( val >= 1869098400 && val <= 1887850800) return 1 ;
	// 2030: 2030-03-31 02:0:00 +0000 to 2030-10-27 03:0:00 +0000 
	if ( val >= 1901152800 && val <= 1919300400) return 1 ;
};


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
2550704400, 2531955600,  // 2050: 30.10.2050 (2550704400) , 27.3.2050 (2531955600)
2519254800, 2500506000,  // 2049: 31.10.2049 (2519254800) , 28.3.2049 (2500506000)
2487200400, 2469056400,  // 2048: 25.10.2048 (2487200400) , 29.3.2048 (2469056400)
2455750800, 2437606800,  // 2047: 27.10.2047 (2455750800) , 31.3.2047 (2437606800)
2424301200, 2405552400,  // 2046: 28.10.2046 (2424301200) , 25.3.2046 (2405552400)
2392851600, 2374102800,  // 2045: 29.10.2045 (2392851600) , 26.3.2045 (2374102800)
2361402000, 2342653200,  // 2044: 30.10.2044 (2361402000) , 27.3.2044 (2342653200)
2329347600, 2311203600,  // 2043: 25.10.2043 (2329347600) , 29.3.2043 (2311203600)
2297898000, 2279754000,  // 2042: 26.10.2042 (2297898000) , 30.3.2042 (2279754000)
2266448400, 2248304400,  // 2041: 27.10.2041 (2266448400) , 31.3.2041 (2248304400)
2234998800, 2216250000,  // 2040: 28.10.2040 (2234998800) , 25.3.2040 (2216250000)
2203549200, 2184800400,  // 2039: 30.10.2039 (2203549200) , 27.3.2039 (2184800400)
2172099600, 2153350800,  // 2038: 31.10.2038 (2172099600) , 28.3.2038 (2153350800)
2140045200, 2121901200,  // 2037: 25.10.2037 (2140045200) , 29.3.2037 (2121901200)
2108595600, 2090451600,  // 2036: 26.10.2036 (2108595600) , 30.3.2036 (2090451600)
2077146000, 2058397200,  // 2035: 28.10.2035 (2077146000) , 25.3.2035 (2058397200)
2045696400, 2026947600,  // 2034: 29.10.2034 (2045696400) , 26.3.2034 (2026947600)
2014246800, 1995498000,  // 2033: 30.10.2033 (2014246800) , 27.3.2033 (1995498000)
1982797200, 1964048400,  // 2032: 31.10.2032 (1982797200) , 28.3.2032 (1964048400)
1950742800, 1932598800,  // 2031: 26.10.2031 (1950742800) , 30.3.2031 (1932598800)
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


char tempmsg[200]="";

int testNsetDST(uint32_t val)
{
	// no information about times after last or before first date, so we return 0 = "no DST"
	int arlength= sizeof(dst_switch_times) / sizeof(dst_switch_times[0]);
//		printf("Array hat %i Elemente: [0]=%u - [%i]=%u\n",arlength,dst_switch_times[0],arlength-1,dst_switch_times[arlength-1]);
	if ( val >= dst_switch_times[0] )  return 10;
	if ( val <= dst_switch_times[arlength-1] ) { 
		g_next_dst_change = dst_switch_times[arlength-1]; 
//		ADDLOGF_INFO("Tested time (%u) is before my fist entry (%u), which will also be the next switch time. So let's assume its normal time.\n",val,g_next_dst_change);
		return 11;
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



