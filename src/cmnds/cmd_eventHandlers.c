
#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"

// setEventHandler OnClick 5 setChannel 4 1
// This will set event handler for event name "OnClick" for pin number 5, and the executed event command will be "setChannel 4 1"
// setEventHandler OnHold 5 addChannel 4 10 
// As above, but it will require a button hold and it will add value 10 to channel 4 (it will add a value to PWM, PWM channels are <0,100> range)

//
// setEventHandler OnChannelChanged 5 ???
// setEventHandler OnWifiLost ????
//
//
//
// Full example of events.
// Requirements:
// - two buttons (pin 10 and pin 11)
// - single PWM output (channel 1)
// Description:
// - two buttons and single pwm. Buttons are used for both 100% and 0% duty setting
//	 and also for step by step pwm adjust.
// Please note:
// - this is also compatible with TuyaMCU dimmers because they are channel-based!
// Example code:
/*
setPinRole 10 Btn
setPinChannel 10 0
setPinRole 11 Btn
setPinChannel 11 0
setPinRole 26 PWM
setPinChannel 26 1
setEventHandler OnClick 10 setChannel 1 100
setEventHandler OnHold 10 addChannel 1 10 
setEventHandler OnClick 11 setChannel 1 0
setEventHandler OnHold 11 addChannel 1 -10 

//
// On change listeners 
// Full example of on change listeners:
// addChangeHandler Channel0 below 50 echo value is low
// addChangeHandler Current above 100 setChannel 0 0
//
// 
// LCD demo:
// backlog startDriver I2C; addI2CDevice_LCD_PCF8574 I2C1 0x23 0 0 0
// addChangeHandler Channel1 != 0 backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// addChangeHandler Channel1 == 0 backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Disabled


*/
//

enum {
	EVENT_DEFAULT,
	EVENT_TYPE_EQUALS,
	EVENT_TYPE_NOT_EQUALS,
	EVENT_TYPE_GREATER,
	EVENT_TYPE_LESS,
	EVENT_TYPE_EQUALS_OR_LESS,
	EVENT_TYPE_EQUALS_OR_GREATER,

};

static int EVENT_ParseRelation(const char *s) {
	if(!strcmp(s,"=="))
		return EVENT_TYPE_EQUALS;
	if(!strcmp(s,"!="))
		return EVENT_TYPE_NOT_EQUALS;
	if(!strcmp(s,">"))
		return EVENT_TYPE_GREATER;
	if(!strcmp(s,"<"))
		return EVENT_TYPE_LESS;
	if(!strcmp(s,">="))
		return EVENT_TYPE_EQUALS_OR_GREATER;
	if(!strcmp(s,"<="))
		return EVENT_TYPE_EQUALS_OR_LESS;
	return EVENT_DEFAULT;
}

static int EVENT_ParseEventName(const char *s) {
	if(!wal_strnicmp(s,"channel",7)) {
		return CMD_EVENT_CHANGE_CHANNEL0 + atoi(s+7);
	}
	if(!stricmp(s,"voltage"))
		return CMD_EVENT_CHANGE_VOLTAGE;
	if(!stricmp(s,"current"))
		return CMD_EVENT_CHANGE_CURRENT;
	if(!stricmp(s,"power"))
		return CMD_EVENT_CHANGE_POWER;
	if(!stricmp(s,"OnClick"))
		return CMD_EVENT_PIN_ONCLICK;
	if(!stricmp(s,"OnHold"))
		return CMD_EVENT_PIN_ONHOLD;
	if(!stricmp(s,"OnDblClick"))
		return CMD_EVENT_PIN_ONDBLCLICK;
	return CMD_EVENT_NONE;
}
static bool EVENT_EvaluateCondition(int code, int argument, int next) {
	switch(code) {
		case EVENT_TYPE_EQUALS:
			if(argument == next)
				return 1;
			return 0;
		break;
		case EVENT_TYPE_NOT_EQUALS:
			if(argument != next)
				return 1;
			return 0;
		break;
		case EVENT_TYPE_GREATER:
			if(next > argument)
				return 1;
			return 0;
		break;
		case EVENT_TYPE_LESS:
			if(next < argument)
				return 1;
			return 0;
		break;
		case EVENT_TYPE_EQUALS_OR_GREATER:
			if(next >= argument)
				return 1;
			return 0;
		break;
		case EVENT_TYPE_EQUALS_OR_LESS:
			if(next <= argument)
				return 1;
			return 0;
		break;


	}
	return 0;
}

static bool EVENT_EvaluateChangeCondition(int code, int argument, int prev, int next) {
	int prev_ch, next_ch;

	prev_ch = EVENT_EvaluateCondition(code, argument, prev);
	next_ch = EVENT_EvaluateCondition(code, argument, next);

	if(prev_ch == 0 && next_ch == 1) {
		return 1;
	}

	return 0;
}
typedef struct eventHandler_s {
	// event name, eg. "OnClick" or "OnDoubleClick"
	//char *eventName;
	byte eventCode;
	byte eventType;
	// optional argument for event.
	// The handler will only happen if it
	// catches event with a certain argument.
	// For example, you can do "setEventHandler OnClick 5"
	// and it will only fire for pin 5
	short requiredArgument;
	// command to execute when it happens
	char *command;

	struct eventHandler_s *next;
} eventHandler_t;

static eventHandler_t *g_eventHandlers = 0;

// Why strdup breaks strings?
// backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// it got broken around 64 char
// where is buffer with [64] bytes?
static char *test_strdup(const char *s)
{
    char *res;
    size_t len;

    if (s == NULL)
        return NULL;

    len = os_strlen(s);
    res = os_malloc(len + 1);
    if (res)
        os_memcpy(res, s, len + 1);

    return res;
}

void EventHandlers_ProcessVariableChange_Integer(byte eventCode, int oldValue, int newValue) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;
	
	while(ev) {
		if(eventCode==ev->eventCode) {
			if(EVENT_EvaluateChangeCondition(ev->eventType, ev->requiredArgument, oldValue, newValue)) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_ProcessVariableChange_Integer: executing command %s",ev->command);
				CMD_ExecuteCommand(ev->command);
			}
		}
		ev = ev->next;
	}
}

void EventHandlers_AddEventHandler_Integer(byte eventCode, int type, int requiredArgument, const char *commandToRun)
{
	eventHandler_t *ev = malloc(sizeof(eventHandler_t));

	ev->next = g_eventHandlers;
	g_eventHandlers = ev;
	ev->eventType = type;
	ev->command = test_strdup(commandToRun);
	ev->eventCode = eventCode;
	ev->requiredArgument = requiredArgument;
}
void EventHandlers_FireEvent(byte eventCode, int argument) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;
	
	while(ev) {
		if(eventCode==ev->eventCode) {
			if(argument == ev->requiredArgument) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_ProcessVariableChange_Integer: executing command %s",ev->command);
				CMD_ExecuteCommand(ev->command);
			}
		}
		ev = ev->next;
	}
	
}

static int CMD_AddEventHandler(const void *context, const char *cmd, const char *args){
	const char *eventName;
	int reqArg;
	const char *cmdToCall;
	int eventCode;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddEventHandler: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 3) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddEventHandler: command requires 3 arguments");
		return 1;
	}
	
	eventName = Tokenizer_GetArg(0);
	reqArg = Tokenizer_GetArgInteger(1);
	cmdToCall = Tokenizer_GetArgFrom(2);
	
	eventCode = EVENT_ParseEventName(eventName);
	if(eventCode == CMD_EVENT_NONE) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddEventHandler: %s is not a valid event",eventName);
		return 1;
	}
	
	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddEventHandler: added %s with cmd %s",eventName,cmdToCall);
	EventHandlers_AddEventHandler_Integer(eventCode,EVENT_DEFAULT,reqArg,cmdToCall);

	return 1;
}

static int CMD_AddChangeHandler(const void *context, const char *cmd, const char *args){
	const char *eventName;
	const char *relation;
	int reqArg;
	const char *cmdToCall;
	int relationCode;
	int eventCode;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 4) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: command requires 4 arguments");
		return 1;
	}
	
	eventName = Tokenizer_GetArg(0);
	relation = Tokenizer_GetArg(1);
	reqArg = Tokenizer_GetArgInteger(2);
	cmdToCall = Tokenizer_GetArgFrom(3);

	relationCode = EVENT_ParseRelation(relation);

	if(relationCode == EVENT_DEFAULT) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: %s is not a valid relation",relation);
		return 1;
	}
	eventCode = EVENT_ParseEventName(eventName);
	if(eventCode == CMD_EVENT_NONE) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: %s is not a valid event",eventName);
		return 1;
	}
	
		
	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: added %s with cmd %s",eventName,cmdToCall);
	EventHandlers_AddEventHandler_Integer(eventCode,relationCode,reqArg,cmdToCall);

	return 1;
}

static int CMD_ListEvents(const void *context, const char *cmd, const char *args){
	struct eventHandler_s *ev;
	int c;

	ev = g_eventHandlers;
	c = 0;
	
	while(ev) {
		
		ADDLOG_INFO(LOG_FEATURE_EVENT, "Event %i has code %i and command %s",c,ev->eventCode,ev->command);
		ev = ev->next;
		c++;
	}

	return 1;
}
void EventHandlers_Init() {

    CMD_RegisterCommand("AddEventHandler", "", CMD_AddEventHandler, "qqqqq0", NULL);
    CMD_RegisterCommand("AddChangeHandler", "", CMD_AddChangeHandler, "qqqqq0", NULL);
    CMD_RegisterCommand("listEvents", "", CMD_ListEvents, "qqqqq0", NULL);
	
}



