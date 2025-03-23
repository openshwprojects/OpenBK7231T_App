//
#include <time.h>
#include <math.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"

#include "drv_ntp.h"

#define M_PI   3.14159265358979323846264338327950288
#define LOG_FEATURE LOG_FEATURE_NTP

time_t  ntp_eventsTime = 0;

typedef struct ntpEvent_s {
	byte hour;
	byte minute;
	byte second;
	byte weekDayFlags;
#if ENABLE_NTP_SUNRISE_SUNSET
	byte lastDay;  /* used so we don't repeat sunrise sunset events the same day */
	byte sunflags;  /* flags for sunrise/sunset as follows: */
#define SUNRISE_FLAG (1 << 0)
#define SUNSET_FLAG (1 << 1)
#endif
	int id;
	char *command;
	struct ntpEvent_s *next;
} ntpEvent_t;

ntpEvent_t *ntp_events = 0;

#if ENABLE_NTP_SUNRISE_SUNSET
/* Sunrise/sunset algorithm, somewhat based on https://edwilliams.org/sunrise_sunset_algorithm.htm and tasmota code */
const float pi2 = (M_PI * 2);
const float pi = M_PI;
const float RAD = (M_PI / 180.0);

/* Force a float value between two ranges, and add or substract the range until we fit */
static inline float ModulusRangef(float val, float min, float max)
	{
	if (max <= min) {
		return min;   /* inconsistent, do what we can */
		}
	float range = max - min;
	val = val - min;       /* now range of val should be 0..range */
	val = fmodf(val, range);    /* actual range is now -range..range */
	if (val < 0.0f) {
		val += range;
		}   /* actual range is now 0..range */
	return val + min;    /* returns range min..max */
	}

/* Force value in the 0..pi2 range */
static inline float InPi(float val)
	{
	return ModulusRangef(val, 0.0f, pi2);
	}

#define CENTURY_DAYS 36524.0f  /* days in century (1900's = 36525, 2000's = 36524 */
/* Time formula */
/* Tdays is the number of days since Jan 1 2000, and replaces T as the Tropical Century. T = Tdays / CENTURY_DAYS */
static inline float TimeFormula(float *declination, uint32_t Tdays)
	{
	float RA_Mean = 18.71506921f + (2400.0513369f / CENTURY_DAYS) * Tdays;    /* we keep only first order value as T is between 0.20 and 0.30 */
	float meanAnomaly = InPi( (pi2 * 0.993133f) + (pi2 * 99.997361f / CENTURY_DAYS) * Tdays);
	float trueLongitude = InPi( (pi2 * 0.7859453f) + meanAnomaly + (6893.0f * sinf(meanAnomaly)
			+ 72.0f * sinf(meanAnomaly + meanAnomaly) + (6191.2f / CENTURY_DAYS) * Tdays) * (pi2 / 1296.0e3f));

	float cos_eps = 0.91750f;     /* precompute cos(eps) */
	float sin_eps = 0.39773f;     /* precompute sin(eps) */

	float rightAscension = atanf(tanf(trueLongitude) * cos_eps);
	if (rightAscension < 0.0f)
		rightAscension += pi;
	if (trueLongitude > pi)
		rightAscension += pi;
	rightAscension = rightAscension * (24.0f / pi2);
	*declination = asinf(sin_eps * sinf(trueLongitude));
	RA_Mean = ModulusRangef(RA_Mean, 0.0f, 24.0f);
	float dRA = ModulusRangef(RA_Mean - rightAscension, -12.0f, 12.0f);
	dRA = dRA * 1.0027379f;
	return dRA;
	}

/* Compute the Julian day number from the Calendar date, using only unsigned ints for code compactness */
static inline uint32_t JulianDay(void)
	{
	/* https://en.wikipedia.org/wiki/Julian_day */

	uint32_t Year = NTP_GetYear();    /* Year ex:2020 */
	uint32_t Month = NTP_GetMonth();       /* 1..12 */
	uint32_t Day = NTP_GetMDay();      /* 1..31 */
	uint32_t Julian;               /* Julian day number */

	if (Month <= 2) {
		Month += 12;
		Year -= 1;
		}
	/* NOTE: 1461 = 365.25 * 4, Julian = (1461 * Year + 6884472) / 4 + (153 * Month - 457) / 5 + Day -1 -13; */
	Julian = (1461 * Year + 6884416) / 4 + (153 * Month - 457) / 5 + Day;   /* -1 -13 included in 6884472 - 14*4 = 6884416 */
	return Julian;
	}

/* Sunrise and Sunset DawnType */
#define DAWN_NORMAL            -0.8333
#define DAWN_CIVIL             -6.0
#define DAWN_NAUTIC            -12.0
#define DAWN_ASTRONOMIC        -18.0

static void dusk2Dawn(struct SUN_DATA *Settings, byte sunflags, uint8_t *hour, uint8_t *minute, int day_offset)
	{
	float eventTime, declination, localTime;
	const uint32_t JD2000 = 2451545;
	uint32_t Tdays = JulianDay() - JD2000 + day_offset;  /* number of days since Jan 1 2000, plus offset */

	/* ex 2458977 (2020 May 7) - 2451545 -> 7432 -> 0,2034 */
	const float sin_h = sinf(DAWN_NORMAL * RAD);    /* let GCC pre-compute the sin() at compile time */

	float geoLatitude = Settings->latitude / (1000000.0f / RAD);
	float geoLongitude = ((float) Settings->longitude) / 1000000;
	float timeZone = ((float) NTP_GetTimesZoneOfsSeconds()) / 3600;  /* convert to hours */
	float timeEquation = TimeFormula(&declination, Tdays);
	float timeDiff = acosf((sin_h - sinf(geoLatitude) * sinf(declination)) / (cosf(geoLatitude) * cosf(declination))) * (12.0f / pi);

	if (sunflags & SUNRISE_FLAG) {
		localTime = 12.0f - timeDiff - timeEquation;
		}
	else {
		localTime = 12.0f + timeDiff - timeEquation;
		}
	float worldTime = localTime - geoLongitude / 15.0f;
	eventTime = worldTime + timeZone + (1 / 120.0f);  /* In Hours, with rounding to nearest minute (1/60 * .5) */
	eventTime = ModulusRangef(eventTime, 0.0f, 24.0f);   /* force 0 <= x < 24.0 */
	*hour = (uint8_t) eventTime;
	*minute = (uint8_t)(60.0f * fmodf(eventTime, 1.0f));
	}

/* calc number of days until next sun event */
static int calc_day_offset(int tm_wday, int weekDayFlags)
	{
	int day_offset, mask;

	if (tm_wday > 6)  /* we can be called with 0-7, 0 and 7 represent Sunday (0) */
		tm_wday = 0;
	mask = 1 << tm_wday;
	for (day_offset = 0; day_offset < 7; day_offset++) {
		if (mask & weekDayFlags)
			break;
		mask <<= 1;
		if (mask & 0x80)  /* mask is only 7 bits */
			mask = 1;  /* start over on Sunday */
		}
	return (day_offset);
	}
void NTP_CalculateSunrise(byte *outHour, byte *outMinute) {
	dusk2Dawn(&sun_data, SUNRISE_FLAG, outHour, outMinute, 0);
}
void NTP_CalculateSunset(byte *outHour, byte *outMinute) {
	dusk2Dawn(&sun_data, SUNSET_FLAG, outHour, outMinute, 0);
}
#endif

void NTP_RunEventsForSecond(time_t runTime) {
	ntpEvent_t *e;
	struct tm *ltm;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&runTime);
	
	if (ltm == 0) {
		return;
	}

	e = ntp_events;

	while (e) {
		if (e->command) {
			// base check
			if (e->hour == ltm->tm_hour && e->second == ltm->tm_sec && e->minute == ltm->tm_min) {
				// weekday check
				if (BIT_CHECK(e->weekDayFlags, ltm->tm_wday)) {
#if ENABLE_NTP_SUNRISE_SUNSET
					if (e->sunflags & (SUNRISE_FLAG || SUNSET_FLAG)) {
						if (e->lastDay != ltm->tm_wday) {
							e->lastDay = ltm->tm_wday;  /* stop any further sun events today */
							dusk2Dawn(&sun_data, e->sunflags, &e->hour, &e->minute,
								calc_day_offset(ltm->tm_wday + 1, e->weekDayFlags));  /* setup for tomorrow */
							CMD_ExecuteCommand(e->command, 0);
							}
						else {
							e->lastDay = -1;  /* mark with anything but a valid day of week */
							}
						}
					else
#endif
					CMD_ExecuteCommand(e->command, 0);
				}
			}
		}
		e = e->next;
	}
}

void NTP_RunEvents(unsigned int newTime, bool bTimeValid) {
	unsigned int delta;
	unsigned int i;

	// new time invalid?
	if (bTimeValid == false) {
		ntp_eventsTime = 0;
		return;
	}
	// old time invalid, but new one ok?
	if (ntp_eventsTime == 0) {
		ntp_eventsTime = (time_t)newTime;
		return;
	}
	// time went backwards
	if (newTime < ntp_eventsTime) {
		ntp_eventsTime = (time_t)newTime;
		return;
	}
	if (ntp_events) {
		// NTP resynchronization could cause us to skip some seconds in some rare cases?
		delta = (unsigned int)((time_t)newTime - ntp_eventsTime);
		// a large shift in time is not expected, so limit to a constant number of seconds
		if (delta > 100)
			delta = 100;
		for (i = 0; i < delta; i++) {
			NTP_RunEventsForSecond(ntp_eventsTime + i);
		}
	}
	ntp_eventsTime = (time_t)newTime;
}

#if ENABLE_NTP_SUNRISE_SUNSET
void NTP_AddClockEvent(int hour, int minute, int second, int weekDayFlags, int id, int sunflags, const char* command) {
#else
void NTP_AddClockEvent(int hour, int minute, int second, int weekDayFlags, int id, const char* command) {
#endif
	ntpEvent_t* newEvent = (ntpEvent_t*)malloc(sizeof(ntpEvent_t));
	if (newEvent == NULL) {
		// handle error
		return;
	}

	newEvent->hour = hour;
	newEvent->minute = minute;
	newEvent->second = second;
	newEvent->weekDayFlags = weekDayFlags;
#if ENABLE_NTP_SUNRISE_SUNSET
	newEvent->lastDay = -1;  /* mark with anything but a valid day of week */
	newEvent->sunflags = sunflags;
#endif
	newEvent->id = id;
	newEvent->command = strdup(command);
	newEvent->next = ntp_events;

	ntp_events = newEvent;
}
int NTP_RemoveClockEvent(int id) {
	int ret = 0;
	ntpEvent_t* curr = ntp_events;
	ntpEvent_t* prev = NULL;

	while (curr != NULL) {
		if (curr->id == id) {
			if (prev == NULL) {
				ntp_events = curr->next;
			}
			else {
				prev->next = curr->next;
			}
			free(curr->command);
			free(curr);
			ret++;
			if (prev == NULL) {
				curr = ntp_events;
			}
			else {
				curr = prev->next;
			}
		}
		else {
			prev = curr;
			curr = curr->next;
		}
	}
	return ret;
}
// addClockEvent [Time] [WeekDayFlags] [UniqueIDForRemoval]
// Bit flag 0 - sunday, bit flag 1 - monday, etc...
// Example: do event on 15:30 every weekday, unique ID 123
// addClockEvent 15:30:00 0xff 123 POWER0 ON
// Example: do event on 8:00 every sunday
// addClockEvent 8:00:00 0x01 234 backlog led_temperature 500; led_dimmer 100; led_enableAll 1;
// Example
// addClockEvent 15:06:00 0xff 123 POWER TOGGLE
// Example: do event every Wednesday at sunrise
// addClockEvent sunrise 0x08 12 POWER OFF
commandResult_t CMD_NTP_AddClockEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int hour, minute = 0, second = 0;
	const char *s;
	int flags;
	int id;
#if ENABLE_NTP_SUNRISE_SUNSET
	uint8_t hour_b, minute_b;
	int sunflags = 0;
	struct tm *ltm = gmtime(&ntp_eventsTime);
#endif

	Tokenizer_TokenizeString(args, TOKENIZER_ALTERNATE_EXPAND_AT_START);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	s = Tokenizer_GetArg(0);
	flags = Tokenizer_GetArgInteger(1);

	if (sscanf(s, "%2d:%2d:%2d", &hour, &minute, &second) >= 2) {
		// hour, minute and second has correct value parsed
	}
#if ENABLE_NTP_SUNRISE_SUNSET
	else if (strcasestr(s, "sunrise")) {
		sunflags |= SUNRISE_FLAG;
	}
	else if (strcasestr(s, "sunset")) {
		sunflags |= SUNSET_FLAG;
	}
#endif
	else {
		hour = Tokenizer_GetArgInteger(0);

		// single integer value indicates the clock value is TimerSeconds from midnight
		second = hour % 60;
		minute = (hour / 60) % 60;
		hour = (hour / 3600) % 24;
	}

	id = Tokenizer_GetArgInteger(2);
	s = Tokenizer_GetArgFrom(3);
#if ENABLE_NTP_SUNRISE_SUNSET
	if (sunflags) {
		dusk2Dawn(&sun_data, sunflags, &hour_b, &minute_b, calc_day_offset(ltm->tm_wday, flags));
		hour = hour_b;
		minute = minute_b;
	}

	NTP_AddClockEvent(hour, minute, second, flags, id, sunflags, s);
#else
	NTP_AddClockEvent(hour, minute, second, flags, id, s);
#endif
	  return CMD_RES_OK;
}
// addPeriodValue [ChannelIndex] [Start_DayOfWeek] [Start_HH:MM:SS] [End_DayOfWeek] [End_HH:MM:SS] [Value] [UniqueID] [Flags]
//commandResult_t CMD_NTP_AddPeriodValue(const void *context, const char *cmd, const char *args, int cmdFlags) {
//	int start_hour, start_minute, start_second, start_day;
//	int end_hour, end_minute, end_second, end_day;
//	const char *s;
//	int flags;
//	int id;
//	int channel;
//	int value;
//
//	Tokenizer_TokenizeString(args, 0);
//	// following check must be done after 'Tokenizer_TokenizeString',
//	// so we know arguments count in Tokenizer. 'cmd' argument is
//	// only for warning display
//	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
//		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
//	}
//	channel = Tokenizer_GetArgInteger(0);
//	start_day = Tokenizer_GetArgInteger(1);
//	s = Tokenizer_GetArg(2);
//	if (sscanf(s, "%i:%i:%i", &start_hour, &start_minute, &start_second) <= 1) {
//		return CMD_RES_BAD_ARGUMENT;
//	}
//	end_day = Tokenizer_GetArgInteger(3);
//	s = Tokenizer_GetArg(4);
//	if (sscanf(s, "%i:%i:%i", &end_hour, &end_minute, &end_second) <= 1) {
//		return CMD_RES_BAD_ARGUMENT;
//	}
//	value = Tokenizer_GetArgInteger(5);
//	id = Tokenizer_GetArgInteger(6);
//	flags = Tokenizer_GetArgInteger(7);
//
//
//	return CMD_RES_OK;
//}

commandResult_t CMD_NTP_RemoveClockEvent(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int id;

	// tokenize the args string
	Tokenizer_TokenizeString(args, 0);

	// Check the number of arguments
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	// Parse the id parameter
	id = Tokenizer_GetArgInteger(0);

	// Remove the clock event with the given id
	NTP_RemoveClockEvent(id);

	return CMD_RES_OK;
}

int NTP_GetEventTime(int id) {
	for (ntpEvent_t* e = ntp_events; e; e = e->next)
	{
		if (e->id == id)
		{
			return (int)e->hour * 3600 + (int)e->minute * 60 + (int)e->second;
		}
	}

	return -1;
}

int NTP_PrintEventList() {
	ntpEvent_t* e;
	int t;

	e = ntp_events;
	t = 0;

	while (e) {
		// Print the command
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Ev %i - %i:%i:%i, days 0x%02x, cmd %s\n", (int)e->id, (int)e->hour, (int)e->minute, (int)e->second, (int)e->weekDayFlags, e->command);

		t++;
		e = e->next;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Total %i events", t);
	return t;
}
commandResult_t CMD_NTP_ListEvents(const void* context, const char* cmd, const char* args, int cmdFlags) {

	NTP_PrintEventList();
	return CMD_RES_OK;
}

int NTP_ClearEvents() {
	ntpEvent_t* e;
	int t;

	e = ntp_events;
	t = 0;

	while (e) {
		ntpEvent_t *p = e;

		t++;
		e = e->next;

		free(p->command);
		free(p);
	}
	ntp_events = 0;
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Removed %i events", t);
	return t;
}
commandResult_t CMD_NTP_ClearEvents(const void* context, const char* cmd, const char* args, int cmdFlags) {

	NTP_ClearEvents();

	return CMD_RES_OK;
}
void NTP_Init_Events() {

	//cmddetail:{"name":"addClockEvent","args":"[TimerSeconds or Time or sunrise or sunset] [WeekDayFlags] [UniqueIDForRemoval][Command]",
	//cmddetail:"descr":"Schedule command to run on given time in given day of week. NTP must be running. TimerSeconds is seconds from midnight, Time is a time like HH:mm or HH:mm:ss, WeekDayFlag is a bitflag on which day to run, 0xff mean all days, 0x01 means sunday, 0x02 monday, 0x03 sunday and monday, etc, id is an unique id so event can be removed later. (NOTE: Use of sunrise/sunset requires compiling with ENABLE_NTP_SUNRISE_SUNSET set which adds about 11k of code)",
	//cmddetail:"fn":"CMD_NTP_AddClockEvent","file":"driver/drv_ntp_events.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addClockEvent",CMD_NTP_AddClockEvent, NULL);
	//cmddetail:{"name":"removeClockEvent","args":"[ID]",
	//cmddetail:"descr":"Removes clock event wtih given ID",
	//cmddetail:"fn":"CMD_NTP_RemoveClockEvent","file":"driver/drv_ntp_events.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("removeClockEvent", CMD_NTP_RemoveClockEvent, NULL);
	//cmddetail:{"name":"listClockEvents","args":"",
	//cmddetail:"descr":"Print the complete set clock events list",
	//cmddetail:"fn":"CMD_NTP_ListEvents","file":"driver/drv_ntp_events.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("listClockEvents", CMD_NTP_ListEvents, NULL);
	//cmddetail:{"name":"clearClockEvents","args":"",
	//cmddetail:"descr":"Removes all set clock events",
	//cmddetail:"fn":"CMD_NTP_ClearEvents","file":"driver/drv_ntp_events.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearClockEvents", CMD_NTP_ClearEvents, NULL);
	//CMD_RegisterCommand("addPeriodValue", CMD_NTP_AddPeriodValue, NULL);
}

