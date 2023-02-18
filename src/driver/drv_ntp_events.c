//
#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"

#include "drv_ntp.h"

#define LOG_FEATURE LOG_FEATURE_NTP

unsigned int ntp_eventsTime = 0;

typedef struct ntpEvent_s {
	byte hour;
	byte minute;
	byte second;
	byte weekDayFlags;
	int id;
	char *command;
	struct ntpEvent_s *next;
} ntpEvent_t;

ntpEvent_t *ntp_events = 0;


void NTP_RunEventsForSecond(unsigned int runTime) {
	ntpEvent_t *e;
	struct tm *ltm;

	ltm = localtime((time_t*)&runTime);
	
	e = ntp_events;

	while (e) {
		if (e->command) {
			// base check
			if (e->hour == ltm->tm_hour && e->second == ltm->tm_sec && e->minute == ltm->tm_min) {
				// weekday check
				if (BIT_CHECK(e->weekDayFlags, ltm->tm_wday)) {
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
		ntp_eventsTime = newTime;
		return;
	}
	// time went backwards
	if (newTime < ntp_eventsTime) {
		ntp_eventsTime = newTime;
		return;
	}
	if (ntp_events) {
		// NTP resynchronization could cause us to skip some seconds in some rare cases?
		delta = newTime - ntp_eventsTime;
		// a large shift in time is not expected, so limit to a constant number of seconds
		if (delta > 100)
			delta = 100;
		for (i = 0; i < delta; i++) {
			NTP_RunEventsForSecond(ntp_eventsTime + i);
		}
	}
	ntp_eventsTime = newTime;
}
// addClockEvent [Time] [WeekDayFlags] [UniqueIDForRemoval]
// Bit flag 0 - sunday, bit flag 1 - monday, etc...
// Example: do event on 15:30 every weekday, unique ID 123
// addClockEvent 15:30:00 0xff 123 POWER0 ON
// Example: do event on 8:00 every sunday
// addClockEvent 8:00:00 0x01 234 backlog led_temperature 500; led_dimmer 100; led_enableAll 1;

commandResult_t NTP_AddClockEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int hour, minute, second;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

    return CMD_RES_OK;
}

commandResult_t NTP_RemoveClockEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {

	return CMD_RES_OK;
}
}
void NTP_Init_Events() {

    CMD_RegisterCommand("addClockEvent",NTP_AddClockEvent, NULL);
	CMD_RegisterCommand("removeClockEvent", NTP_RemoveClockEvent, NULL);
}

