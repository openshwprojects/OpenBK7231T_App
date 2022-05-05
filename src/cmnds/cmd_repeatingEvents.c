
#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"

typedef struct repeatingEvent_s {
	char *command;
	//char *condition;
	int intervalSeconds;
	int currentInterval;
	struct repeatingEvent_s *next;
} repeatingEvent_t;

static repeatingEvent_t *g_repeatingEvents = 0;

void RepeatingEvents_AddRepeatingEvent(const char *command, int secondsInterval)
{
	repeatingEvent_t *ev = malloc(sizeof(repeatingEvent_t));

	ev->next = g_repeatingEvents;
	g_repeatingEvents = ev;
	ev->command = strdup(command);
	ev->intervalSeconds = secondsInterval;
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
		cur->currentInterval--;
		if(cur->currentInterval<=0){
			c_ran++;
			cur->currentInterval = cur->intervalSeconds;
			CMD_ExecuteCommand(cur->command, COMMAND_FLAG_SOURCE_SCRIPT);
		}
		cur = cur->next;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"RepeatingEvents_OnEverySecond checked %i events, ran %i\n",c_checked,c_ran);
}
int RepeatingEvents_Cmd_AddRepeatingEvent(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int interval;
	const char *cmdToRepeat;
	// linkTuyaMCUOutputToChannel dpID channelID [varType]
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: will tokenize %s\n",args);
	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: requires 2 arguments\n");
		return -1;
	}
	interval = Tokenizer_GetArgInteger(0);
	cmdToRepeat = Tokenizer_GetArgFrom(1);

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,"addRepeatingEvent: interval %i, command [%s]\n",interval,cmdToRepeat);

	RepeatingEvents_AddRepeatingEvent(cmdToRepeat,interval);

	return 1;
}
void RepeatingEvents_Init() {
	// addRepeatingEvent 5 dsfsdfsdfds
	CMD_RegisterCommand("addRepeatingEvent","",RepeatingEvents_Cmd_AddRepeatingEvent, "qqqq", NULL);


}


