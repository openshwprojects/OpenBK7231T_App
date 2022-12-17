
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../obk_config.h"
#include "../driver/drv_public.h"
#include <ctype.h>
#include "cmd_local.h"

static commandResult_t CMD_SetChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetChannel: command requires argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetChannel: command requires 2 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	CHANNEL_Set(ch,val, false);

	return CMD_RES_OK;
}
static commandResult_t CMD_AddChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires 2 arguments (next 2, min and max, are optionsl)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires 2 arguments (next 2, min and max, are optionsl)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
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


	return CMD_RES_OK;
}
static commandResult_t CMD_ToggleChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ToggleChannel: command requires 1 argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ToggleChannel: command requires 1 argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);

	CHANNEL_Toggle(ch);

	return CMD_RES_OK;
}
static commandResult_t CMD_ClampChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, max, min;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClampChannel: command requires argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClampChannel: command requires 3 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);
	min = Tokenizer_GetArgInteger(1);
	max = Tokenizer_GetArgInteger(2);

	CHANNEL_AddClamped(ch,0, min, max);

	return CMD_RES_OK;
}

static commandResult_t CMD_SetPinRole(const void *context, const char *cmd, const char *args, int cmdFlags){
	int pin, roleIndex;
	const char *role;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: command requires argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: command requires 2 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	pin = Tokenizer_GetArgInteger(0);
	role = Tokenizer_GetArg(1);

	roleIndex = PIN_ParsePinRoleName(role);
	if(roleIndex == IOR_Total_Options) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinRole: This role is not known");
	} else {
		PIN_SetPinRoleForPinIndex(pin,roleIndex);
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_SetPinChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int pin, ch;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinChannel: command requires argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SetPinChannel: command requires 2 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	pin = Tokenizer_GetArgInteger(0);
	ch = Tokenizer_GetArgInteger(1);

	PIN_SetPinChannelForPinIndex(pin,ch);

	return CMD_RES_OK;
}
static commandResult_t CMD_GetChannel(const void *context, const char *cmd, const char *args, int cmdFlags){
	int ch, val;

	if(args==0||*args==0) {
		return 1;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = CHANNEL_Get(ch);

	if(cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
		ADDLOG_INFO(LOG_FEATURE_RAW, "%i", val);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GetChannel: channel %i is %i",ch, val);
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_GetReadings(const void *context, const char *cmd, const char *args, int cmdFlags){
#ifndef OBK_DISABLE_ALL_DRIVERS
	char tmp[96];
	float v, c, p;
    float e, elh;

	v = DRV_GetReading(OBK_VOLTAGE);
	c = DRV_GetReading(OBK_CURRENT);
	p = DRV_GetReading(OBK_POWER);
    e = DRV_GetReading(OBK_CONSUMPTION_TOTAL);
    elh = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);

	snprintf(tmp, sizeof(tmp), "%f %f %f %f %f",v,c,p,e,elh);

	if(cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
		ADDLOG_INFO(LOG_FEATURE_RAW, tmp);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_GetReadings: readings are %s",tmp);
	}
#endif
	return CMD_RES_OK;
}
static commandResult_t CMD_ShortName(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *s;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	s = CFG_GetShortDeviceName();
	if (Tokenizer_GetArgsCount() == 0) {
		if (cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
			ADDLOG_INFO(LOG_FEATURE_RAW, s);
		}
		else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ShortName: name is %s", s);
		}
	}
	else {
		CFG_SetShortDeviceName(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t CMD_FriendlyName(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *s;
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	s = CFG_GetDeviceName();
	if (Tokenizer_GetArgsCount() == 0) {
		if (cmdFlags & COMMAND_FLAG_SOURCE_TCP) {
			ADDLOG_INFO(LOG_FEATURE_RAW, s);
		}
		else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_FriendlyName: name is %s", s);
		}
	}
	else {
		CFG_SetDeviceName(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t CMD_SetFlag(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *s;
	int flag;
	int bOn;
	Tokenizer_TokenizeString(args, 0);

	s = CFG_GetDeviceName();
	if (Tokenizer_GetArgsCount() <= 1) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Usage: [flag] [1or0]");
		return CMD_RES_BAD_ARGUMENT;
	}
	flag = Tokenizer_GetArgInteger(0);
	bOn = Tokenizer_GetArgInteger(1);

	CFG_SetFlag(flag, bOn);
	ADDLOG_INFO(LOG_FEATURE_CMD, "Flag %i set to %i",flag,bOn);

	return CMD_RES_OK;
}
extern int g_bWantDeepSleep;
static commandResult_t CMD_StartDeepSleep(const void *context, const char *cmd, const char *args, int cmdFlags){
	g_bWantDeepSleep = 1;
	return CMD_RES_OK;
}
void CMD_InitChannelCommands(){
	//cmddetail:{"name":"SetChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_SetChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, NULL, NULL);
	//cmddetail:{"name":"ToggleChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ToggleChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ToggleChannel", "", CMD_ToggleChannel, NULL, NULL);
	//cmddetail:{"name":"AddChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_AddChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("AddChannel", "", CMD_AddChannel, NULL, NULL);
	//cmddetail:{"name":"ClampChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ClampChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ClampChannel", "", CMD_ClampChannel, NULL, NULL);
	//cmddetail:{"name":"SetPinRole","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_SetPinRole","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SetPinRole", "", CMD_SetPinRole, NULL, NULL);
	//cmddetail:{"name":"SetPinChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_SetPinChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("SetPinChannel", "", CMD_SetPinChannel, NULL, NULL);
	//cmddetail:{"name":"GetChannel","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_GetChannel","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("GetChannel", "", CMD_GetChannel, NULL, NULL);
	//cmddetail:{"name":"GetReadings","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_GetReadings","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("GetReadings", "", CMD_GetReadings, NULL, NULL);
	//cmddetail:{"name":"ShortName","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ShortName","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ShortName", "", CMD_ShortName, NULL, NULL);
	//cmddetail:{"name":"ShortName","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ShortName","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("FriendlyName", "", CMD_FriendlyName, NULL, NULL);
	//cmddetail:{"name":"ShortName","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ShortName","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("startDeepSleep", "", CMD_StartDeepSleep, NULL, NULL);
	//cmddetail:{"name":"ShortName","args":"",
	//cmddetail:"descr":"qqqqq0",
	//cmddetail:"fn":"CMD_ShortName","file":"cmnds/cmd_channels.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetFlag", "", CMD_SetFlag, NULL, NULL);

}
