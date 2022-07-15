#ifndef __CMD_PUBLIC_H__
#define __CMD_PUBLIC_H__

typedef int (*commandHandler_t)(const void *context, const char *cmd, const char *args, int flags);

// command was entered in console (web app etc)
#define COMMAND_FLAG_SOURCE_CONSOLE		1
// command was entered by script
#define COMMAND_FLAG_SOURCE_SCRIPT		2
// command was sent by MQTT
#define COMMAND_FLAG_SOURCE_MQTT		4
// command was sent by HTTP cmnd GET
#define COMMAND_FLAG_SOURCE_HTTP		8



//
void CMD_Init();
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context);
int CMD_ExecuteCommand(const char *s, int cmdFlags);
int CMD_ExecuteCommandArgs(const char *cmd, const char *args, int cmdFlags);

enum EventCode {
	CMD_EVENT_NONE,
	// per-pins event (no value, only trigger action)
	// This is for Buttons
	CMD_EVENT_PIN_ONCLICK,
	CMD_EVENT_PIN_ONDBLCLICK,
	CMD_EVENT_PIN_ONHOLD,
	// change events (with a value, so event can trigger only when
	// argument becomes larger than a given threshold, or lower, etc)
	CMD_EVENT_CHANGE_CHANNEL0,
	CMD_EVENT_CHANGE_CHANNEL63 = CMD_EVENT_CHANGE_CHANNEL0 + 63,
	// change events for custom values (non-channels)
	CMD_EVENT_CHANGE_VOLTAGE, // must match order in drv_bl0942.c
	CMD_EVENT_CHANGE_CURRENT,
	CMD_EVENT_CHANGE_POWER,

	// this is for ToggleChannelOnToggle
	CMD_EVENT_PIN_ONTOGGLE,

	// must be lower than 256
	CMD_EVENT_MAX_TYPES
};

// cmd_tokenizer.c
int Tokenizer_GetArgsCount();
const char *Tokenizer_GetArg(int i);
const char *Tokenizer_GetArgFrom(int i);
int Tokenizer_GetArgInteger(int i);
void Tokenizer_TokenizeString(const char *s);
// cmd_repeatingEvents.c
void RepeatingEvents_Init();
void RepeatingEvents_OnEverySecond();
// cmd_eventHandlers.c
void EventHandlers_Init();
void EventHandlers_FireEvent(byte eventCode, int argument);
void EventHandlers_ProcessVariableChange_Integer(byte eventCode, int oldValue, int newValue);
// cmd_tasmota.c
int taslike_commands_init();
// cmd_newLEDDriver.c
int NewLED_InitCommands();
float LED_GetDimmer();
void LED_SetDimmer(int iVal);
void LED_SetEnableAll(int bEnable);
int LED_GetEnableAll();
// cmd_test.c
int fortest_commands_init();
// cmd_channels.c
void CMD_InitChannelCommands();
// cmd_send.c
int CMD_InitSendCommands();

#endif // __CMD_PUBLIC_H__
