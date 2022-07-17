
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include <ctype.h>
#include "cmd_local.h"

static int CMD_SetChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetChannel: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetChannel: command requires 2 arguments");
		return 1;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	CHANNEL_Set(ch,val, false);

	return 1;
}
static int CMD_AddChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires 2 arguments (next 2, min and max, are optionsl)");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires 2 arguments (next 2, min and max, are optionsl)");
		return 1;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);
	if(Tokenizer_GetArgsCount() == 4) {
		int min, max;

		min = Tokenizer_GetArgInteger(2);
		max = Tokenizer_GetArgInteger(3);

		CHANNEL_AddClamped(ch,val,min,max);
	} else {
		CHANNEL_Add(ch,val);
	}


	return 1;
}
static int CMD_ClampChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, max, min;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClampChannel: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClampChannel: command requires 3 arguments");
		return 1;
	}

	ch = Tokenizer_GetArgInteger(0);
	min = Tokenizer_GetArgInteger(1);
	max = Tokenizer_GetArgInteger(2);

	CHANNEL_AddClamped(ch,0, min, max);

	return 1;
}

static int CMD_SetPinRole(const void *context, const char *cmd, const char *args, int cmdFlags){
	int pin, roleIndex;
	const char *role;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: command requires 2 arguments");
		return 1;
	}

	pin = Tokenizer_GetArgInteger(0);
	role = Tokenizer_GetArg(1);

	roleIndex = PIN_ParsePinRoleName(role);
	if(roleIndex == IOR_Total_Options) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: This role is not known");
	} else {
		PIN_SetPinRoleForPinIndex(pin,roleIndex);
	}

	return 1;
}
static int CMD_SetPinChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int pin, ch;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinChannel: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinChannel: command requires 2 arguments");
		return 1;
	}

	pin = Tokenizer_GetArgInteger(0);
	ch = Tokenizer_GetArgInteger(1);

	PIN_SetPinChannelForPinIndex(pin,ch);

	return 1;
}
static int CMD_GetChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 1) {
		return 1;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = CHANNEL_Get(ch);

	if(cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
		ADDLOG_INFO(LOG_FEATURE_RAW, "%i", val);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GetChannel: channel %i is %i",ch, val);
	}

	return 1;
}
static int CMD_GetReadings(const void *context, const char *cmd, const char *args, int cmdFlags){
#ifndef OBK_DISABLE_ALL_DRIVERS
	char tmp[64];
	float v, c, p;

	v = DRV_GetReading(OBK_VOLTAGE);
	c = DRV_GetReading(OBK_CURRENT);
	p = DRV_GetReading(OBK_POWER);

	sprintf(tmp, "%f %f %f",v,c,p);

	if(cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
		ADDLOG_INFO(LOG_FEATURE_RAW, tmp);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GetReadings: readings are %s",tmp);
	}
#endif
	return 1;
}
static int CMD_ShortName(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *s;

	s = CFG_GetShortDeviceName();

	if(cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
		ADDLOG_INFO(LOG_FEATURE_RAW, s);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ShortName: name is %s", s);
	}
	return 1;
}
void CMD_InitChannelCommands(){
    CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("AddChannel", "", CMD_AddChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("ClampChannel", "", CMD_ClampChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("SetPinRole", "", CMD_SetPinRole, "qqqqq0", NULL);
    CMD_RegisterCommand("SetPinChannel", "", CMD_SetPinChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("GetChannel", "", CMD_GetChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("GetReadings", "", CMD_GetReadings, "qqqqq0", NULL);
    CMD_RegisterCommand("ShortName", "", CMD_ShortName, "qqqqq0", NULL);

}
