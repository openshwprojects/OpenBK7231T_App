
#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"

// addEventHandler OnClick 5 setChannel 4 1
// This will set event handler for event name "OnClick" for pin number 5, and the executed event command will be "setChannel 4 1"
// addEventHandler OnHold 5 addChannel 4 10
// As above, but it will require a button hold and it will add value 10 to channel 4 (it will add a value to PWM, PWM channels are <0,100> range)

/*
startDriver DGR
addEventHandler OnClick 8 DGR_SendBrightness roomLEDstrips 0
addEventHandler OnClick 10 DGR_SendBrightness roomLEDstrips 100
*/

/*
startDriver DGR
addEventHandler OnClick 8 backlog setChannel 4 0; DGR_SendBrightness roomLEDstrips $CH4
addEventHandler OnClick 10 backlog setChannel 4 255; DGR_SendBrightness roomLEDstrips $CH4
addEventHandler OnHold 8 backlog addChannel 4 10 0 255; DGR_SendBrightness roomLEDstrips $CH4
addEventHandler OnHold 10 backlog addChannel 4 -10 0 255; DGR_SendBrightness roomLEDstrips $CH4
*/
//
// addEventHandler OnChannelChange 5 ???
// addEventHandler OnWifiLost ????
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
addEventHandler OnClick 10 setChannel 1 100
addEventHandler OnHold 10 addChannel 1 10
addEventHandler OnClick 11 setChannel 1 0
addEventHandler OnHold 11 addChannel 1 -10

//
// On change listeners
// Full example of on change listeners:
// addChangeHandler Channel0 < 50 echo value is low
// addChangeHandler Current > 100 setChannel 0 0
// addChangeHandler Power > 40 setChannel 1 0
// addChangeHandler noPingTime > 600 reboot
// addChangeHandler noMQTTTime > 600 reboot
//
// LCD demo:
// backlog startDriver I2C; addI2CDevice_LCD_PCF8574 I2C1 0x23 0 0 0
// addChangeHandler Channel1 != 0 backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Enabled
// addChangeHandler Channel1 == 0 backlog lcd_clearAndGoto I2C1 0x23 1 1; lcd_print I2C1 0x23 Disabled


// when channel 1 becomes 0, send OFF
addChangeHandler Channel1 == 0 SendGet http://192.168.0.112/cm?cmnd=Power0%20OFF
// when channel 1 becomes 1, send ON
addChangeHandler Channel1 == 1 SendGet http://192.168.0.112/cm?cmnd=Power0%20ON


alias doRelayClick backlog setChannel 1 1; addRepeatingEvent 2 1 setChannel 1 0; ClearNoPingTime
addChangeHandler NoPingTime > 40 doRelayClick 


// This will automatically turn off relay after about 2 seconds
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addChangeHandler Channel0 != 0 addRepeatingEvent 2 1 setChannel 0 0

AddEventHandler OnClick 0 addChannel 1 -10 0 100 AddEventHandler OnClick 1 addChannel 1 10 0 100

// Event to fire on binary (hex) value received by TuyaMCU
AddEventHandler OnUART 55AA00FF setChannel 0 1


// IR events
addEventHandler2 Samsung 0x707 0x68 setChannel 1 0
addEventHandler2 Samsung 0x707 0x69 setChannel 1 1

addEventHandler2 Samsung 0x707 0x60 setChannel 1 0
addEventHandler2 Samsung 0x707 0x61 setChannel 1 1
// MQTT state

addEventHandler MQTTState 0 setChannel 1 0
addEventHandler MQTTState 1 setChannel 1 1


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

int EVENT_ParseEventName(const char *s) {
	if(!wal_strnicmp(s,"channel",7)) {
		return CMD_EVENT_CHANGE_CHANNEL0 + atoi(s+7);
	}
	if (!stricmp(s, "noPingTime"))
		return CMD_EVENT_CHANGE_NOPINGTIME;
	if (!stricmp(s, "NoMQTTTime"))
		return CMD_EVENT_CHANGE_NOMQTTTIME;
	if(!stricmp(s,"voltage"))
		return CMD_EVENT_CHANGE_VOLTAGE;
	if(!stricmp(s,"current"))
		return CMD_EVENT_CHANGE_CURRENT;
	if(!stricmp(s,"power"))
		return CMD_EVENT_CHANGE_POWER;
	if(!stricmp(s,"OnRelease"))
		return CMD_EVENT_PIN_ONRELEASE;
	if (!stricmp(s, "OnPress"))
		return CMD_EVENT_PIN_ONPRESS;
	if(!stricmp(s,"OnClick"))
		return CMD_EVENT_PIN_ONCLICK;
	if(!stricmp(s,"OnToggle"))
		return CMD_EVENT_PIN_ONTOGGLE;
	if (!stricmp(s, "IPChange"))
		return CMD_EVENT_IPCHANGE;
	if(!stricmp(s,"OnHold"))
		return CMD_EVENT_PIN_ONHOLD;
	if(!stricmp(s,"OnHoldStart"))
		return CMD_EVENT_PIN_ONHOLDSTART;
	if(!stricmp(s,"OnDblClick"))
		return CMD_EVENT_PIN_ONDBLCLICK;
	if (!stricmp(s, "On3Click"))
		return CMD_EVENT_PIN_ON3CLICK;
	if (!stricmp(s, "On4Click"))
		return CMD_EVENT_PIN_ON4CLICK;
	if (!stricmp(s, "On5Click"))
		return CMD_EVENT_PIN_ON5CLICK;
	if(!stricmp(s,"OnChannelChange"))
		return CMD_EVENT_CHANNEL_ONCHANGE;
	if(!stricmp(s,"OnUART"))
		return CMD_EVENT_ON_UART;
	if(!stricmp(s,"MQTTState"))
		return CMD_EVENT_MQTT_STATE;
	if (!stricmp(s, "NTPState"))
		return CMD_EVENT_NTP_STATE;
	if (!stricmp(s, "LEDState"))
		return CMD_EVENT_LED_STATE;
	if (!stricmp(s, "LEDMode"))
		return CMD_EVENT_LED_MODE;
    if(!stricmp(s,"energycounter") || !stricmp(s, "energy"))
        return CMD_EVENT_CHANGE_CONSUMPTION_TOTAL;
    if(!stricmp(s,"energycounter_last_hour"))
        return CMD_EVENT_CHANGE_CONSUMPTION_LAST_HOUR;
    if(!stricmp(s,"IR_RC5"))
		return CMD_EVENT_IR_RC5;
    if(!stricmp(s,"IR_RC6"))
		return CMD_EVENT_IR_RC6;
    if(!stricmp(s,"IR_Samsung"))
		return CMD_EVENT_IR_SAMSUNG;
    if(!stricmp(s,"IR_PANASONIC"))
		return CMD_EVENT_IR_PANASONIC;
    if(!stricmp(s,"IR_NEC"))
		return CMD_EVENT_IR_NEC;
    if(!stricmp(s,"IR_SAMSUNG_LG"))
		return CMD_EVENT_IR_SAMSUNG_LG;
    if(!stricmp(s,"IR_SHARP"))
		return CMD_EVENT_IR_SHARP;
    if(!stricmp(s,"IR_SONY"))
		return CMD_EVENT_IR_SONY;
	// WiFi state has single argument: HALWifiStatus_t
	if (!stricmp(s, "WiFiState"))
		return CMD_EVENT_WIFI_STATE;
	if (!stricmp(s, "TuyaMCUParsed"))
		return CMD_EVENT_TUYAMCU_PARSED;
	if (!stricmp(s, "OnADCButton"))
		return CMD_EVENT_ADC_BUTTON;
	if (!stricmp(s, "OnCustomDown"))
		return CMD_EVENT_CUSTOM_DOWN;
	if (!stricmp(s, "OnCustomUP"))
		return CMD_EVENT_CUSTOM_UP;
	if (isdigit((unsigned char)*s)) {
		return atoi(s);
	}
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
	// For example, you can do "addEventHandler OnClick 5"
	// and it will only fire for pin 5
	int requiredArgument;
	int requiredArgument2;
	int requiredArgument3;
	// command to execute when it happens
	char *command;
	// for UART event handlers?
	char *requiredArgumentText;

	struct eventHandler_s *next;
} eventHandler_t;

static eventHandler_t *g_eventHandlers = 0;


void EventHandlers_ProcessVariableChange_Integer(byte eventCode, int oldValue, int newValue) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;

	while(ev) {
		if(eventCode==ev->eventCode) {
			if(EVENT_EvaluateChangeCondition(ev->eventType, ev->requiredArgument, oldValue, newValue)) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_ProcessVariableChange_Integer: executing command %s",ev->command);
				CMD_ExecuteCommand(ev->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		ev = ev->next;
	}

#if defined(PLATFORM_BEKEN) || defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_LN882H) 
	CMD_Script_ProcessWaitersForEvent(eventCode, newValue);
#endif
}

void EventHandlers_AddEventHandler_Integer(byte eventCode, int type, int requiredArgument, int requiredArgument2, int requiredArgument3, const char *commandToRun)
{
	eventHandler_t *ev = malloc(sizeof(eventHandler_t));
	memset(ev,0,sizeof(eventHandler_t));

	ev->next = g_eventHandlers;
	g_eventHandlers = ev;

	ev->requiredArgumentText = NULL;
	ev->eventType = type;
	ev->command = strdup(commandToRun);
	ev->eventCode = eventCode;
	ev->requiredArgument = requiredArgument;
	ev->requiredArgument2 = requiredArgument2;
	ev->requiredArgument3 = requiredArgument3;
}

void EventHandlers_AddEventHandler_String(byte eventCode, int type, const char *requiredArgument, const char *commandToRun)
{
	eventHandler_t *ev = malloc(sizeof(eventHandler_t));
	memset(ev,0,sizeof(eventHandler_t));

	ev->next = g_eventHandlers;
	g_eventHandlers = ev;

	ev->requiredArgumentText = strdup(requiredArgument);
	ev->eventType = type;
	ev->command = strdup(commandToRun);
	ev->eventCode = eventCode;
	ev->requiredArgument = 0;
	ev->requiredArgument2 = 0;
}
void EventHandlers_FireEvent3(byte eventCode, int argument, int argument2, int argument3) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;

	while (ev) {
		if (eventCode == ev->eventCode) {
			if (argument == ev->requiredArgument && argument2 == ev->requiredArgument2 && argument3 == ev->requiredArgument3) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_FireEvent3: executing command %s", ev->command);
				CMD_ExecuteCommand(ev->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		ev = ev->next;
	}
}
void EventHandlers_FireEvent2(byte eventCode, int argument, int argument2) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;

	while(ev) {
		if(eventCode==ev->eventCode) {
			if(argument == ev->requiredArgument && argument2 == ev->requiredArgument2) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_FireEvent2: executing command %s",ev->command);
				CMD_ExecuteCommand(ev->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		ev = ev->next;
	}
}


void EventHandlers_FireEvent(byte eventCode, int argument) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;

	while(ev) {
		if(eventCode==ev->eventCode) {
			if(argument == ev->requiredArgument) {
				ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_FireEvent: executing command %s",ev->command);
				CMD_ExecuteCommand(ev->command, COMMAND_FLAG_SOURCE_SCRIPT);
			}
		}
		ev = ev->next;
	}

#if defined(PLATFORM_BEKEN) || defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_LN882H)
	CMD_Script_ProcessWaitersForEvent(eventCode, argument);
#endif
}
void EventHandlers_FireEvent_String(byte eventCode, const char *argument) {
	struct eventHandler_s *ev;

	ev = g_eventHandlers;

	while(ev) {
		if(eventCode==ev->eventCode) {
			if(ev->requiredArgumentText != 0) {
				if(!stricmp(argument,ev->requiredArgumentText)) {
					ADDLOG_INFO(LOG_FEATURE_EVENT, "EventHandlers_FireEvent_String: executing command %s",ev->command);
					CMD_ExecuteCommand(ev->command, COMMAND_FLAG_SOURCE_SCRIPT);
				}
			}
		}
		ev = ev->next;
	}

}

// NOTE: this also handles addEventHandler2, an event handler with two arguments
static commandResult_t CMD_AddEventHandler(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *eventName;
	int reqArg;
	int arg2;
	int arg3;
	const char *cmdToCall;
	int eventCode;
	const char *reqArgStr;
	char argsCnt;

	Tokenizer_TokenizeString(args,0);
	
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	argsCnt = cmd[strlen("addEventHandler2") - 1];
	if (argsCnt == '2' || argsCnt == '3') {
		argsCnt = argsCnt - '0';
	}
	else {
		argsCnt = 1;
	}
	arg2 = 0;
	arg3 = 0;

	eventName = Tokenizer_GetArg(0);
	if(false==Tokenizer_IsArgInteger(1)) {
		reqArg = 0;
		reqArgStr = Tokenizer_GetArg(1);
	} else {
		reqArgStr = 0;
		reqArg = Tokenizer_GetArgInteger(1);
		if(argsCnt>1) {
			arg2 = Tokenizer_GetArgInteger(2);
			ADDLOG_DEBUG(LOG_FEATURE_EVENT, "CMD_AddEventHandler: arg2 = %i.",arg2);
			if (argsCnt > 2) {
				arg3 = Tokenizer_GetArgInteger(3);
				ADDLOG_DEBUG(LOG_FEATURE_EVENT, "CMD_AddEventHandler: arg3= %i.", arg3);
			}
		}
		ADDLOG_DEBUG(LOG_FEATURE_EVENT, "CMD_AddEventHandler: arg1 = %i.",reqArg);
	}
	cmdToCall = Tokenizer_GetArgFrom(1+ argsCnt);

	eventCode = EVENT_ParseEventName(eventName);
	if(eventCode == CMD_EVENT_NONE) {
		ADDLOG_ERROR(LOG_FEATURE_EVENT, "%s is not a valid event",eventName);
		return CMD_RES_BAD_ARGUMENT;
	}

	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddEventHandler: added %s with cmd %s",eventName,cmdToCall);
	if(reqArgStr) {
		EventHandlers_AddEventHandler_String(eventCode,EVENT_DEFAULT,reqArgStr,cmdToCall);
	} else {
		EventHandlers_AddEventHandler_Integer(eventCode,EVENT_DEFAULT,reqArg,arg2, arg3,cmdToCall);
	}

	return CMD_RES_OK;
}
commandResult_t CMD_ClearAllHandlers(const void *context, const char *cmd, const char *args, int cmdFlags){

	int c = 0;
	eventHandler_t *ev, *next;

	ev = g_eventHandlers;

	while(ev != 0) {
		next = ev->next;

		free(ev->command);
		free(ev);

		ev = next;
		c++;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Fried %i handlers", c);
	g_eventHandlers = 0;

	return CMD_RES_OK;
}
static commandResult_t CMD_AddChangeHandler(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *eventName;
	const char *relation;
	int reqArg;
	const char *cmdToCall;
	int relationCode;
	int eventCode;

	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	eventName = Tokenizer_GetArg(0);
	relation = Tokenizer_GetArg(1);
	reqArg = Tokenizer_GetArgInteger(2);
	cmdToCall = Tokenizer_GetArgFrom(3);

	relationCode = EVENT_ParseRelation(relation);

	if(relationCode == EVENT_DEFAULT) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: %s is not a valid relation",relation);
		return CMD_RES_BAD_ARGUMENT;
	}
	eventCode = EVENT_ParseEventName(eventName);
	if(eventCode == CMD_EVENT_NONE) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: %s is not a valid event",eventName);
		return CMD_RES_BAD_ARGUMENT;
	}


	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_AddChangeHandler: added %s with cmd %s",eventName,cmdToCall);
	EventHandlers_AddEventHandler_Integer(eventCode,relationCode,reqArg,0,0,cmdToCall);

	return CMD_RES_OK;
}

static commandResult_t CMD_ListEventHandlers(const void *context, const char *cmd, const char *args, int cmdFlags){
	struct eventHandler_s *ev;
	int c;

	ev = g_eventHandlers;
	c = 0;

	while(ev) {

		ADDLOG_INFO(LOG_FEATURE_EVENT, "Event %i has code %i and command %s",c,ev->eventCode,ev->command);
		ev = ev->next;
		c++;
	}

	return CMD_RES_OK;
}
int EventHandlers_GetActiveCount() {
	struct eventHandler_s *ev;
	int c;

	ev = g_eventHandlers;
	c = 0;

	while (ev) {
		ev = ev->next;
		c++;
	}
	return c;
}
void EventHandlers_Init() {

	//cmddetail:{"name":"AddEventHandler","args":"[EventName][EventArgument][CommandToRun]",
	//cmddetail:"descr":"This can be used to trigger an action on a button click, long press, etc",
	//cmddetail:"fn":"CMD_AddEventHandler","file":"cmnds/cmd_eventHandlers.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("AddEventHandler", CMD_AddEventHandler, NULL);
	//cmddetail:{"name":"AddChangeHandler","args":"[Variable][Relation][Constant][Command]",
	//cmddetail:"descr":"This can listen to change in channel value (for example channel 0 becoming 100), or for a voltage/current/power change for BL0942/BL0937. This supports multiple relations, like ==, !=, >=, < etc. The Variable name for channel is Channel0, Channel2, etc, for BL0XXX it can be 'Power', or 'Current' or 'Voltage'",
	//cmddetail:"fn":"CMD_AddChangeHandler","file":"cmnds/cmd_eventHandlers.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("AddChangeHandler", CMD_AddChangeHandler, NULL);
	//cmddetail:{"name":"listEventHandlers","args":"",
	//cmddetail:"descr":"Prints full list of added event handlers",
	//cmddetail:"fn":"CMD_ListEventHandlers","file":"cmnds/cmd_eventHandlers.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("listEventHandlers", CMD_ListEventHandlers, NULL);
	//cmddetail:{"name":"clearAllHandlers","args":"",
	//cmddetail:"descr":"This clears all added event handlers",
	//cmddetail:"fn":"CMD_ClearAllHandlers","file":"cmnds/cmd_eventHandlers.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clearAllHandlers", CMD_ClearAllHandlers, NULL);

}



