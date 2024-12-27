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

#define LTSTR "Local Time: %04d-%02d-%02d %02d:%02d:%02d"
#define LTM2TIME(T) (T)->tm_year+1900, (T)->tm_mon+1, (T)->tm_mday, (T)->tm_hour, (T)->tm_min, (T)->tm_sec

extern void NTP_Init_Events(void);
extern void NTP_RunEvents(unsigned int newTime, bool bTimeValid);

#if ENABLE_NTP_SUNRISE_SUNSET
extern void NTP_CalculateSunrise(byte *outHour, byte *outMinute);
extern void NTP_CalculateSunset(byte *outHour, byte *outMinute);
#endif

#if ENABLE_NTP_DST
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

#define LOG_FEATURE LOG_FEATURE_NTP

typedef struct
{

  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

#define MAKE_WORD(hi, lo) hi << 8 | lo

// NTP time since 1900 to unix time (since 1970)
// Number of seconds to ad
#define NTP_OFFSET 2208988800L

static int g_ntp_socket = 0;
static struct sockaddr_in g_address;
static int adrLen;
// in seconds, before next retry
static int g_ntp_delay = 0;
static bool g_synced;
// time offset (time zone?) in seconds
//#define CFG_DEFAULT_TIMEOFFSETSECONDS (-8 * 60 * 60)
static int g_timeOffsetSeconds = 0;
// current time - this may be 32 or 64 bit, depending on platform
time_t g_ntpTime;

int NTP_GetTimesZoneOfsSeconds()
{
    return g_timeOffsetSeconds;
}
commandResult_t NTP_SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a, b;
	const char *arg;
	int oldOfs;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	arg = Tokenizer_GetArg(0);
	oldOfs = g_timeOffsetSeconds;
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
	g_ntpTime -= oldOfs;
	g_ntpTime += g_timeOffsetSeconds;
#if ENABLE_NTP_DST
// in rare cases time can be decreased so time of next DST is wrong
// e.g. by mistake we set offset to two hours in EU and we have just passed start of summertime - next DST switch will be end of summertime
// if we now adjust the clock to the correct offset of one hour, we are slightliy before start of summertime
// so just to be sure, recalculate DST in any case
    	setDST(1);	// setDST will take care of all details
#endif
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP offset set"
#if ENABLE_NTP_DST
	" - DST offset set to %i hours",g_DST
#endif	
	);
	return CMD_RES_OK;
}

#if ENABLE_NTP_SUNRISE_SUNSET

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
commandResult_t NTP_SetLatlong(const void *context, const char *cmd, const char *args, int cmdFlags) {
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

int NTP_GetSunrise()
{
	byte hour, minute;
	int sunriseInSecondsFromMidnight;

	NTP_CalculateSunrise(&hour, &minute);
	sunriseInSecondsFromMidnight = ((int)hour * 3600) + ((int)minute * 60);
	return sunriseInSecondsFromMidnight;
}

int NTP_GetSunset()
{
	byte hour, minute;
	int sunsetInSecondsFromMidnight;

	NTP_CalculateSunset(&hour, &minute);
	sunsetInSecondsFromMidnight =  ((int)hour * 3600) + ((int)minute * 60);
	return sunsetInSecondsFromMidnight;
}
#endif

//Set custom NTP server
commandResult_t NTP_SetServer(const void *context, const char *cmd, const char *args, int cmdFlags) {
    const char *newValue;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
    newValue = Tokenizer_GetArg(0);
    CFG_SetNTPServer(newValue);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP server set to %s", newValue);
    return CMD_RES_OK;
}

//Display settings used by the NTP driver
commandResult_t NTP_Info(const void *context, const char *cmd, const char *args, int cmdFlags) {
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "Server=%s, Time offset=%d", CFG_GetNTPServer(), g_timeOffsetSeconds);
    return CMD_RES_OK;
}

#if ENABLE_NTP_DST

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
 // calc seconds from days and add hours
 t = days * SECS_PER_DAY + hr * SECS_PER_HOUR;

  t += (7 * (w - 1) + (dow - wday + 7) % 7) * SECS_PER_DAY;


/*
  tm.tm_hour = hr;
// tm is set to {0}, so no need to set minute or second values equal to "0"
  tm.tm_mday = 1;		// first day of (next) month
  tm.tm_mon = m ;
  tm.tm_year = yr - 1900;
  t = (uint32_t)mktime(&tm);         // First day of the month, or first day of next month for "Last" rules
//  since we use time functions, weekdays are 0 to 6 but tasmota settings use 1 to 7 (e.g. Sunday = 1)
// so we have to subtract 1 from dow to match tm_weekday here
  t += (7 * (w - 1) + (dow - 1 - tm.tm_wday + 7) % 7) * SECS_PER_DAY;

*/

  if (0 == week) {
    t -= 7 * SECS_PER_DAY;  // back one week if this is a "Last XX" rule
  }
  return t;
}

uint32_t setDST(bool setNTP)
{
   if (useDST && g_synced){
	int year=NTP_GetYear();
	time_t tempt;
	int8_t old_DST=0;
	char tmp[40];	// to hold date string of timestamp
	Start_DST_epoch = RuleToTime(dayStart,monthStart,nthWeekStart,hourStart,year);
	End_DST_epoch = RuleToTime(dayEnd,monthEnd,nthWeekEnd,hourEnd,year);
	old_DST = g_DST%128;	// 0 if "unset" because -128%128 = 0 

	if ( Start_DST_epoch < End_DST_epoch ) {	// Northern --> begin before end
		if (g_ntpTime < Start_DST_epoch) {
			// we are in winter time before start of summer time
			next_DST_switch_epoch=Start_DST_epoch;
			g_DST=0;
//			tempt = (time_t)Start_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "Before first DST switch in %i. Info: DST starts at %lu (%.24s local time)\r\n",year,Start_DST_epoch,ctime(&tempt));
		} else if (g_ntpTime < End_DST_epoch ){
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
			if (g_ntpTime < End_DST_epoch) {
			// we in summer time at beginning of the yeay
			next_DST_switch_epoch=End_DST_epoch;
			g_DST=g_DST_offset;
//			tempt = (time_t)End_DST_epoch;
//			ADDLOG_INFO(LOG_FEATURE_RAW, "In first DST period of %i. Info: DST ends at %lu (%.24s local time)\r\n",year,End_DST_epoch,ctime(&tempt));
		} else if (g_ntpTime < Start_DST_epoch ){
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
	g_ntpTime += (g_DST-old_DST)*3600*setNTP;
	tempt = (time_t)next_DST_switch_epoch;

	struct tm *ltm;
	ltm = gmtime(&tempt);
	ADDLOG_INFO(LOG_FEATURE_RAW, "In %s time - next DST switch at %lu (" LTSTR ")\r\n",
	(g_DST)?"summer":"standard", next_DST_switch_epoch, LTM2TIME(ltm));
	return g_DST;
  }
  else return 0;	// DST not (yet) set or can't be calculated (if ntp not synced)

}

int IsDST()
{
	if (( g_DST == -128) || (g_ntpTime > next_DST_switch_epoch)) return (setDST(1));	// only in case we don't know DST status, calculate it - and while at it: set ntpTime correctly...
	return (g_DST);										// otherwise we can safely return the prevously calkulated value
}

commandResult_t CLOCK_CalcDST(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int year=NTP_GetYear();
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



#endif



int NTP_GetWeekDay() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_wday;
}
int NTP_GetHour() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_hour;
}
int NTP_GetMinute() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_min;
}
int NTP_GetSecond() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_sec;
}
int NTP_GetMDay() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mday;
}
int NTP_GetMonth() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_mon+1;
}
int NTP_GetYear() {
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&g_ntpTime);

	if (ltm == 0) {
		return 0;
	}

	return ltm->tm_year+1900;
}
#if ENABLE_NTP_DST
int Time_IsDST(){ 
	return IsDST();
}
#endif
#if WINDOWS
bool b_ntp_simulatedTime = false;
void NTP_SetSimulatedTime(unsigned int timeNow) {
	g_ntpTime = timeNow;
	g_ntpTime += g_timeOffsetSeconds;
#if ENABLE_NTP_DST
    	g_ntpTime += setDST(0)*3600;	
#endif
	g_synced = true;
	b_ntp_simulatedTime = true;
}
#endif
void NTP_Init() {

#if WINDOWS
	b_ntp_simulatedTime = false;
#endif
	//cmddetail:{"name":"ntp_timeZoneOfs","args":"[Value]",
	//cmddetail:"descr":"Sets the time zone offset in hours. Also supports HH:MM syntax if you want to specify value in minutes. For negative values, use -HH:MM syntax, for example -5:30 will shift time by 5 hours and 30 minutes negative.",
	//cmddetail:"fn":"NTP_SetTimeZoneOfs","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_timeZoneOfs",NTP_SetTimeZoneOfs, NULL);
	//cmddetail:{"name":"ntp_setServer","args":"[ServerIP]",
	//cmddetail:"descr":"Sets the NTP server",
	//cmddetail:"fn":"NTP_SetServer","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_setServer", NTP_SetServer, NULL);
#if ENABLE_NTP_SUNRISE_SUNSET
	//cmddetail:{"name":"ntp_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Sets the NTP latitude and longitude",
	//cmddetail:"fn":"NTP_SetLatlong","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":"NTP_SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("ntp_setLatLong", NTP_SetLatlong, NULL);
#endif
	//cmddetail:{"name":"ntp_info","args":"",
	//cmddetail:"descr":"Display NTP related settings",
	//cmddetail:"fn":"NTP_Info","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_info", NTP_Info, NULL);

#if ENABLE_CALENDAR_EVENTS
	NTP_Init_Events();
#endif
#if ENABLE_NTP_DST
	//cmddetail:{"name":"CLOCK_CalcDST","args":"[nthWeekEnd monthEnd dayEnd hourEnd nthWeekStart monthStart dayStart hourStart [g_DSToffset hours - default is 1 if unset]",
	//cmddetail:"descr":"Checks, if actual time is during DST or not.",
	//cmddetail:"fn":"CLOCK_CalcDST","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clock_calcDST",CLOCK_CalcDST, NULL);
#endif

    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP driver initialized with server=%s, offset=%d", CFG_GetNTPServer(), g_timeOffsetSeconds);
    g_synced = false;
}

unsigned int NTP_GetCurrentTime() {
    return g_ntpTime;
}
unsigned int NTP_GetCurrentTimeWithoutOffset() {
#if ENABLE_NTP_DST
	return g_ntpTime - g_timeOffsetSeconds - g_DST%128;	// if g_DST is unset, it's -128 --> -128%128 = 0 as needed
#else
	return g_ntpTime - g_timeOffsetSeconds;
#endif	

}



void NTP_Shutdown() {
    if(g_ntp_socket != 0) {
#if WINDOWS
        closesocket(g_ntp_socket);
#else
        lwip_close(g_ntp_socket);
#endif
    }
    g_ntp_socket = 0;
    // can attempt in next 10 seconds
    g_ntp_delay = 60;
}
void NTP_SendRequest(bool bBlocking) {
    byte *ptr;
	const char *adrString;
    //int i, recv_len;
    //char buf[64];
    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    adrLen = sizeof(g_address);
    memset( &packet, 0, sizeof( ntp_packet ) );
    ptr = (byte*)&packet;
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    ptr[0] = 0xE3;   // LI, Version, Mode
    ptr[1] = 0;     // Stratum, or type of clock
    ptr[2] = 6;     // Polling Interval
    ptr[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    ptr[12]  = 49;
    ptr[13]  = 0x4E;
    ptr[14]  = 49;
    ptr[15]  = 52;


    //create a UDP socket
    if ((g_ntp_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1)
    {
        g_ntp_socket = 0;
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: failed to create socket");
        return;
    }

    memset((char *) &g_address, 0, sizeof(g_address));

	adrString = CFG_GetNTPServer();
	if (adrString == 0 || adrString[0] == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP_SendRequest: somehow ntp server in config was empty, setting non-empty");
		CFG_SetNTPServer(DEFAULT_NTP_SERVER);
		adrString = CFG_GetNTPServer();
	}

    g_address.sin_family = AF_INET;
    g_address.sin_addr.s_addr = inet_addr(adrString);
    g_address.sin_port = htons(123);


    // Send the message to server:
    if(sendto(g_ntp_socket, &packet, sizeof(packet), 0,
         (struct sockaddr*)&g_address, adrLen) < 0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: Unable to send message");
        NTP_Shutdown();
		// quick next frame attempt
		if (g_secondsElapsed < 60) {
			g_ntp_delay = 0;
		}
        return;
    }

    // https://github.com/tuya/tuya-iotos-embeded-sdk-wifi-ble-bk7231t/blob/5e28e1f9a1a9d88425f3fd4b658e895a8ee7b83b/platforms/bk7231t/tuya_os_adapter/src/system/tuya_hal_network.c
    //
    if(bBlocking == false) {
#if WINDOWS
#else
        if(fcntl(g_ntp_socket, F_SETFL, O_NONBLOCK)) {
            addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: failed to make socket non-blocking!");
        }
#endif
    }

    // can attempt in next 10 seconds
    g_ntp_delay = 10;
}
void NTP_CheckForReceive() {
    byte *ptr;
    int i, recv_len;
    //struct tm * ptm;
    unsigned short highWord;
    unsigned short lowWord;
    unsigned int secsSince1900;
    struct tm *ltm;
    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ptr = (byte*)&packet;

    // Receive the server's response:
    i = sizeof(packet);
#if 0
    recv_len = recvfrom(g_ntp_socket, ptr, i, 0,
         (struct sockaddr*)&g_address, &adrLen);
#else
    recv_len = recv(g_ntp_socket, ptr, i, 0);
#endif

    if(recv_len < 0){
			addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_CheckForReceive: Error while receiving server's msg");
        return;
    }
    highWord = MAKE_WORD(ptr[40], ptr[41]);
    lowWord = MAKE_WORD(ptr[42], ptr[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    secsSince1900 = highWord << 16 | lowWord;
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Seconds since Jan 1 1900 = %u",secsSince1900);

    g_ntpTime = secsSince1900 - NTP_OFFSET;
    g_ntpTime += g_timeOffsetSeconds;
#if ENABLE_NTP_DST
    	g_ntpTime += setDST(0)*3600;	// just to be sure: recalculate DST before setting, in case we somehow "moved back in time" we start with freshly set g_ntpTime, so don't change it inside setDST()!
#endif
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Unix time  : %u",(unsigned int)g_ntpTime);
    ltm = gmtime(&g_ntpTime);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, LTSTR, LTM2TIME(ltm));

	if (g_synced == false) {
		EventHandlers_FireEvent(CMD_EVENT_NTP_STATE, 1);
	}
    g_synced = true;
#if 0
    //ptm = gmtime (&g_ntpTime);
    ptm = gmtime(&g_ntpTime);
    if(ptm == 0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime somehow returned 0\n");
    } else {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_year: %i\n",ptm->tm_year);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mon: %i\n",ptm->tm_mon);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mday: %i\n",ptm->tm_mday);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_hour: %i\n",ptm->tm_hour  );
    }
#endif
    NTP_Shutdown();

}

void NTP_SendRequest_BlockingMode() {
    NTP_Shutdown();
    NTP_SendRequest(true);
    NTP_CheckForReceive();

}

void NTP_OnEverySecond()
{
    g_ntpTime++;
#if ENABLE_NTP_DST
    if (useDST && (g_ntpTime >= next_DST_switch_epoch)){
    	int8_t old_DST=g_DST;
	setDST(1);
    	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Passed DST switch time - recalculated DST offset. Was:%i - now:%i",old_DST,g_DST);
    }
#endif

#if ENABLE_CALENDAR_EVENTS
	NTP_RunEvents(g_ntpTime, g_synced);
#endif
    if(Main_IsConnectedToWiFi()==0)
    {
        return;
    }
#if WINDOWS
	if (b_ntp_simulatedTime) {
		return;
	}
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
    if (ota_progress() != -1)
    {
        return;
    }
#endif
    if(g_ntp_socket == 0) {
        // if no socket, this is a reconnect delay
        if(g_ntp_delay > 0) {
            g_ntp_delay--;
            return;
        }
        NTP_SendRequest(false);
    } else {
        NTP_CheckForReceive();
        // if socket exists, this is a disconnect timeout
        if(g_ntp_delay > 0) {
            g_ntp_delay--;
            if(g_ntp_delay<=0) {
                // disconnect and force reconnect
                NTP_Shutdown();
            }
        }
    }
}

void NTP_AppendInformationToHTTPIndexPage(http_request_t* request)
{
    struct tm *ltm;

    ltm = gmtime(&g_ntpTime);

    if (g_synced == true)
        hprintf255(request, "<h5>NTP (%s): " LTSTR " </h5>",
			CFG_GetNTPServer(),LTM2TIME(ltm));
    else 
        hprintf255(request, "<h5>NTP: Syncing with %s....</h5>",CFG_GetNTPServer());
}

bool NTP_IsTimeSynced()
{
    return g_synced;
}

