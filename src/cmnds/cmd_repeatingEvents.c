
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
	char *command;
	//char *condition;
	int intervalSeconds;
	int currentInterval;
	int times;
	int userID;
	struct repeatingEvent_s *next;
} repeatingEvent_t;

static repeatingEvent_t *g_repeatingEvents = 0;

void RepeatingEvents_CancelRepeatingEvents(int userID)
{
	repeatingEvent_t *ev;

	// reuse existing
	for(ev = g_repeatingEvents; ev; ev = ev->next) {
		if(ev->userID == userID) {
			// mark as finished
			ev->times = 0;
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"Event with id %i and cmd %s has been canceled\n",ev->userID,ev->command);
		}
	}

}
void RepeatingEvents_AddRepeatingEvent(const char *command, int secondsInterval, int times, int userID)
{
	repeatingEvent_t *ev;
	char *cmd_copy;

	// reuse existing
	for(ev = g_repeatingEvents; ev; ev = ev->next) {
		if(ev->times <= 0) {
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
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: failed to malloc new event\n");
		return;
	}
	cmd_copy = strdup(command);
	if(cmd_copy == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: failed to malloc command text copy\n");
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
	ev->currentInterval = 1;
}
void RepeatingEvents_OnEverySecond() {
	repeatingEvent_t *cur;
	int c_checked = 0;
	int c_ran = 0;

	cur = g_repeatingEvents;
	while(cur) {
		c_checked++;
		// debug only check
		if(cur == cur->next) {
			addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond: single linked list was broken?\n");
			cur->next = 0;
			return;
		}
		// -1 means 'forever'
		if(cur->times > 0 || cur->times == -1) {
			cur->currentInterval--;
			if(cur->currentInterval<=0){
				c_ran++;
				// -1 means 'forever'
				if(cur->times != -1) {
					cur->times -= 1;
				}
				cur->currentInterval = cur->intervalSeconds;
				CMD_ExecuteCommand(cur->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		cur = cur->next;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond checked %i events, ran %i\n",c_checked,c_ran);
}
int RepeatingEvents_Cmd_AddRepeatingEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int interval;
	int times;
	const char *cmdToRepeat;
	int userID;

	// linkTuyaMCUOutputToChannel dpID channelID [varType]
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: will tokenize %s\n",args);
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: requires 2 arguments\n");
		return -1;
	}
	interval = Tokenizer_GetArgInteger(0);
	times = Tokenizer_GetArgInteger(1);
	if(!stricmp(cmd,"addRepeatingEventID")) {
		userID = Tokenizer_GetArgInteger(2);
		cmdToRepeat = Tokenizer_GetArgFrom(3);
	} else { 
		userID = 255;
		cmdToRepeat = Tokenizer_GetArgFrom(2);
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: interval %i, repeats %i, command [%s]\n",interval,times,cmdToRepeat);

	RepeatingEvents_AddRepeatingEvent(cmdToRepeat,interval, times, userID);

	return 1;
}
int RepeatingEvents_Cmd_CancelRepeatingEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int userID;

	// linkTuyaMCUOutputToChannel dpID channelID [varType]
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"cancelRepeatingEvent: will tokenize %s\n",args);
	Tokenizer_TokenizeString(args,0);

	if(Tokenizer_GetArgsCount() < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"cancelRepeatingEvent: requires 1 argument\n");
		return -1;
	}
	userID = Tokenizer_GetArgInteger(0);

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"cancelRepeatingEvent: will cancel events with id %i\n",userID);

	RepeatingEvents_CancelRepeatingEvents(userID);

	return 1;
}
void RepeatingEvents_Init() {
	// addRepeatingEvent [DelaySeconds] [Repeats] [Command With Spaces Allowed]
	// addRepeatingEvent 5 -1 Power0 Toggle
	//cmddetail:{"name":"addRepeatingEvent","args":"",
	//cmddetail:"descr":"qqqq",
	//cmddetail:"fn":"RepeatingEvents_Cmd_AddRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addRepeatingEvent","",RepeatingEvents_Cmd_AddRepeatingEvent, "qqqq", NULL);
	// addRepeatingEventID [DelaySeconds] [Repeats] [UserIDInteger] [Command With Spaces Allowed]
	// addRepeatingEventID 2 -1 123 Power0 Toggle
	//cmddetail:{"name":"addRepeatingEventID","args":"",
	//cmddetail:"descr":"qqqq",
	//cmddetail:"fn":"RepeatingEvents_Cmd_AddRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addRepeatingEventID","",RepeatingEvents_Cmd_AddRepeatingEvent, "qqqq", NULL);
	// cancelRepeatingEvent [UserIDInteger]
	//cmddetail:{"name":"cancelRepeatingEvent","args":"",
	//cmddetail:"descr":"qqqq",
	//cmddetail:"fn":"RepeatingEvents_Cmd_CancelRepeatingEvent","file":"cmnds/cmd_repeatingEvents.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("cancelRepeatingEvent","",RepeatingEvents_Cmd_CancelRepeatingEvent, "qqqq", NULL);


}


