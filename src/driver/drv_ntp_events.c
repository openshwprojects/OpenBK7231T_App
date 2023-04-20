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

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = localtime((time_t*)&runTime);
	
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
void NTP_AddClockEvent(int hour, int minute, int second, int weekDayFlags, int id, const char* command) {
	ntpEvent_t* newEvent = (ntpEvent_t*)malloc(sizeof(ntpEvent_t));
	if (newEvent == NULL) {
		// handle error
		return;
	}

	newEvent->hour = hour;
	newEvent->minute = minute;
	newEvent->second = second;
	newEvent->weekDayFlags = weekDayFlags;
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
commandResult_t CMD_NTP_AddClockEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int hour, minute = 0, second = 0;
	const char *s;
	int flags;
	int id;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	s = Tokenizer_GetArg(0);
	if (sscanf(s, "%i:%i:%i", &hour, &minute, &second) <= 1) {
		return CMD_RES_BAD_ARGUMENT;
	}
	flags = Tokenizer_GetArgInteger(1);
	id = Tokenizer_GetArgInteger(2);
	s = Tokenizer_GetArgFrom(3);

	NTP_AddClockEvent(hour, minute, second, flags, id, s);

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
int NTP_PrintEventList() {
	ntpEvent_t* e;
	int t;

	e = ntp_events;
	t = 0;

	while (e) {
		// Print the command
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Ev %i - %i:%i:%i, days %i, cmd %s\n", (int)e->id, (int)e->hour, (int)e->minute, (int)e->second, (int)e->weekDayFlags, e->command);

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

	//cmddetail:{"name":"addClockEvent","args":"[Time] [WeekDayFlags] [UniqueIDForRemoval][Command]",
	//cmddetail:"descr":"Schedule command to run on given time in given day of week. NTP must be running. Time is a time like HH:mm or HH:mm:ss, WeekDayFlag is a bitflag on which day to run, 0xff mean all days, 0x01 means sunday, 0x02 monday, 0x03 sunday and monday, etc, id is an unique id so event can be removede later",
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

