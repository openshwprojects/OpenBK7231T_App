
#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"

// addRepeatingEvent	interval_seconds	  repeats	command top run
// addRepeatingEvent		1				 -1			led_basecolor_rgb rand

// turn off TuyaMCU after 5 seconds
// addRepeatingEvent 5 1 setChannel 1 0
typedef struct repeatingEvent_s {
	// command string to execute
	char *command;
	//char *condition;
	// how often event repeats
	float intervalSeconds;
	// current value until next repeat (decremented every second)
	float currentInterval;
	// number of times to repeat.
	// If set to -1, then it's infinite repeater
	// If set to EVENT_CANCELED_TIMES, then event structure is ready to be reused
	int times;
	// user can set an ID and then cancel repeating event by ID
	int userID;
	struct repeatingEvent_s *next;
} repeatingEvent_t;

#define EVENT_CANCELED_TIMES -999

static repeatingEvent_t *g_repeatingEvents = 0;

void RepeatingEvents_CancelRepeatingEvents(int userID)
{
	repeatingEvent_t *ev;

	// reuse existing
	for(ev = g_repeatingEvents; ev; ev = ev->next) {
		if(ev->userID == userID) {
			// mark as finished
			ev->times = EVENT_CANCELED_TIMES;
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"Event with id %i and cmd %s has been canceled",ev->userID,ev->command);
		}
	}

}
void RepeatingEvents_AddRepeatingEvent(const char *command, float secondsInterval, int times, int userID)
{
	repeatingEvent_t *ev;
	char *cmd_copy;

	// reuse existing
	for(ev = g_repeatingEvents; ev; ev = ev->next) {
		// is this event canceled/empty?
		if(ev->times == EVENT_CANCELED_TIMES) {
			if(!strcmp(ev->command,command)) {
				ev->intervalSeconds = secondsInterval;
				// fire after delay
				ev->currentInterval = secondsInterval;
				ev->times = times;
				return;
			}
		}
	}
	// create new
	ev = malloc(sizeof(repeatingEvent_t));
	if(ev == 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: failed to malloc new event");
		return;
	}
	cmd_copy = strdup(command);
	if(cmd_copy == 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: failed to malloc command text copy");
		free(ev);
		return;
	}

	ev->next = g_repeatingEvents;
	g_repeatingEvents = ev;
	ev->command = cmd_copy;
	ev->intervalSeconds = secondsInterval;
	ev->times = times;
	ev->userID = userID;
	// fire next frame
	// TODO: is this what we want? or do we want to fire after full interval?
	//ev->currentInterval = 1;
	// fire after full interval
	ev->currentInterval = secondsInterval;
}
void SIM_GenerateRepeatingEventsDesc(char *o, int outLen) {
	repeatingEvent_t *cur;
	//int ci = 0;
	char buffer[32];
	cur = g_repeatingEvents;
	while (cur) {
		// -1 means 'forever'
		if (cur->times > 0 || cur->times == -1) {
			//ci++;
			snprintf(buffer, outLen,"ID %i, repeats %i",(int) cur->userID, (int)cur->times);
			strcat_safe(o, buffer, outLen);
			snprintf(buffer, outLen, ", interval %i", (int)cur->intervalSeconds);
			snprintf(buffer, outLen, " (cur left %i), cmd: ", (int)cur->currentInterval);
			strcat_safe(o, buffer, outLen);
			strcat_safe(o, cur->command, outLen);
		}
		cur = cur->next;
	}
}
int RepeatingEvents_GetActiveCount() {
	repeatingEvent_t *cur;
	int c_active;
	
	c_active = 0;
	cur = g_repeatingEvents;
	while (cur) {
		// -1 means 'forever'
		if (cur->times > 0 || cur->times == -1) {
			c_active++;
		}
		cur = cur->next;
	}
	return c_active;
}
void RepeatingEvents_RunUpdate(float deltaTimeSeconds) {
	repeatingEvent_t *cur;
	int c_checked = 0;
	int c_ran = 0;

	cur = g_repeatingEvents;
	while(cur) {
		c_checked++;
		// debug only check
		if(cur == cur->next) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: single linked list was broken?");
			cur->next = 0;
			return;
		}
		// -1 means 'forever'
		if(cur->times > 0 || cur->times == -1) {
			cur->currentInterval-= deltaTimeSeconds;
			if(cur->currentInterval<=0){
				c_ran++;
				// -1 means 'forever'
				if(cur->times != -1) {
					cur->times -= 1;
					if (cur->times <= 0) {
						// if finished all calls, mark as empty so we can reuse later
						cur->times = EVENT_CANCELED_TIMES;
					}
				}
				cur->currentInterval = cur->intervalSeconds;
				CMD_ExecuteCommand(cur->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		cur = cur->next;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond checked %i events, ran %i\n",c_checked,c_ran);
}
// addRepeatingEventID 1234 5 -1 DGR_SendPower "testgr" 1 1 
// cancelRepeatingEvent 1234
#define MIN_REPEATING_INTERVAL 0.001f
commandResult_t RepeatingEvents_Cmd_AddRepeatingEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	float interval;
	int times;
	const char *cmdToRepeat;
	int userID;

	Tokenizer_TokenizeString(args,0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	interval = Tokenizer_GetArgFloat(0);
	times = Tokenizer_GetArgInteger(1);
	if(!stricmp(cmd,"addRepeatingEventID")) {
		userID = Tokenizer_GetArgInteger(2);
		cmdToRepeat = Tokenizer_GetArgFrom(3);
	} else { 
		userID = 255;
		cmdToRepeat = Tokenizer_GetArgFrom(2);
	}
	if (interval <= MIN_REPEATING_INTERVAL) {
		interval = MIN_REPEATING_INTERVAL;
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Interval was too small!");
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: interval %f, repeats %i, command [%s]",interval,times,cmdToRepeat);

	RepeatingEvents_AddRepeatingEvent(cmdToRepeat,interval, times, userID);

	return CMD_RES_OK;
}
commandResult_t RepeatingEvents_Cmd_ClearRepeatingEvents(const void *context, const char *cmd, const char *args, int cmdFlags) {
	repeatingEvent_t *cur;
	repeatingEvent_t *rem;
	int c = 0;

	cur = g_repeatingEvents;
	while (cur) {
		rem = cur;
		cur = cur->next;
		free(rem->command);
		free(rem);
		c++;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Fried %i rep. events", c);
	g_repeatingEvents = 0;
	return CMD_RES_OK;
}
commandResult_t RepeatingEvents_Cmd_CancelRepeatingEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int userID;

	Tokenizer_TokenizeString(args,0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	userID = Tokenizer_GetArgInteger(0);

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"cancelRepeatingEvent: will cancel events with id %i",userID);

	RepeatingEvents_CancelRepeatingEvents(userID);

	return CMD_RES_OK;
}
static commandResult_t RepeatingEvents_Cmd_ListRepeatingEvents(const void *context, const char *cmd, const char *args, int cmdFlags) {
	repeatingEvent_t *ev;
	int c;

	ev = g_repeatingEvents;
	c = 0;

	while (ev) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "Repeater %i has ID %i, interval %i, reps %i, and command %s",
			c,  ev->userID, ev->intervalSeconds, ev->times, ev->command);
		ev = ev->next;
		c++;
	}

	return CMD_RES_OK;
}
void RepeatingEvents_Init() {
	// addRepeatingEvent [DelaySeconds] [Repeats] [Command With Spaces Allowed]
	// addRepeatingEvent 5 -1 Power0 Toggle
	//cmddetail:{"name":"addRepeatingEvent","args":"[IntervalSeconds][RepeatsOr-1][CommandToRun]",
	//cmddetail:"descr":"Starts a timer/interval command. Use 'backlog' to fit multiple commands in a single string.",
	//cmddetail:"fn":"RepeatingEvents_Cmd_AddRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addRepeatingEvent",RepeatingEvents_Cmd_AddRepeatingEvent, NULL);
	//cmddetail:{"name":"addRepeatingEventID","args":"[IntervalSeconds][RepeatsOr-1][UserID][CommandToRun]",
	//cmddetail:"descr":"as addRepeatingEvent, but with a given ID. You can later cancel it with cancelRepeatingEvent.",
	//cmddetail:"fn":"RepeatingEvents_Cmd_AddRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":"addRepeatingEventID 2 -1 123 Power0 Toggle"}
	CMD_RegisterCommand("addRepeatingEventID",RepeatingEvents_Cmd_AddRepeatingEvent, NULL); 
	//cmddetail:{"name":"cancelRepeatingEvent","args":"[UserIDInteger]",
	//cmddetail:"descr":"Stops a given repeating event with a specified ID",
	//cmddetail:"fn":"RepeatingEvents_Cmd_CancelRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("cancelRepeatingEvent",RepeatingEvents_Cmd_CancelRepeatingEvent, NULL);

	//cmddetail:{"name":"clearRepeatingEvents","args":"",
	//cmddetail:"descr":"Clears all repeating events.",
	//cmddetail:"fn":"RepeatingEvents_Cmd_ClearRepeatingEvents","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearRepeatingEvents", RepeatingEvents_Cmd_ClearRepeatingEvents, NULL);
	//cmddetail:{"name":"listRepeatingEvents","args":"",
	//cmddetail:"descr":"lists all repeating events",
	//cmddetail:"fn":"RepeatingEvents_Cmd_ListRepeatingEvents","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("listRepeatingEvents", RepeatingEvents_Cmd_ListRepeatingEvents, NULL);


}


