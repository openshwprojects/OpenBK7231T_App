
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"

static int CMD_SetChannel(const void *context, const char *cmd, const char *args){
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
static int CMD_AddChannel(const void *context, const char *cmd, const char *args){
	int ch, val;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires argument");
		return 1;
	}
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_AddChannel: command requires 2 arguments");
		return 1;
	}
	
	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);
	
	CHANNEL_Add(ch,val);

	return 1;
}

void CMD_InitChannelCommands(){
    CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, "qqqqq0", NULL);
    CMD_RegisterCommand("AddChannel", "", CMD_AddChannel, "qqqqq0", NULL);

}
