#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"

#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

#define HASH_SIZE 128

static int generateHashValue(const char *fname) {
	int		i;
	int		hash;
	int		letter;
	unsigned char *f = (unsigned char *)fname;

	hash = 0;
	i = 0;
	while (f[i]) {
		letter = tolower(f[i]);
		hash+=(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (HASH_SIZE-1);
	return hash;
}

command_t *g_commands[HASH_SIZE] = { NULL };
bool g_powersave;

static commandResult_t CMD_PowerSave(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_PowerSave: enable power save");
#ifdef PLATFORM_BEKEN
	extern int bk_wlan_power_save_set_level(BK_PS_LEVEL level);
    bk_wlan_power_save_set_level(/*PS_DEEP_SLEEP_BIT */  PS_RF_SLEEP_BIT | PS_MCU_SLEEP_BIT);	
	g_powersave = true;
#elif defined(PLATFORM_W600)
	tls_wifi_set_psflag(1, 0);	//Enable powersave but don't save to flash
	g_powersave = true;
#endif

	return CMD_RES_OK;
}


static commandResult_t CMD_ScheduleHADiscovery(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int delay;

	if (args && *args) {
		delay = atoi(args);
	}
	else {
		delay = 5;
	}

	Main_ScheduleHomeAssistantDiscovery(delay);

	return CMD_RES_OK;
}
static commandResult_t CMD_Flags(const void *context, const char *cmd, const char *args, int cmdFlags) {
	union {
		long long newValue;
		struct {
			int ints[2];
			int dummy[2]; // just to be safe
		};
	} u;
	// TODO: check on other platforms, on Beken it's 8, 64 bit
	// On Windows simulator it's 8 as well
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Flags: sizeof(newValue) = %i",sizeof(u.newValue));
	if (args && *args) {
		if (1 != sscanf(args, "%lld", &u.newValue)) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "Argument/sscanf error!");
			return CMD_RES_BAD_ARGUMENT;
		}
	}
	else {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	CFG_SetFlags(u.ints[0], u.ints[1]);
	ADDLOG_INFO(LOG_FEATURE_CMD, "New flags set!");

	return CMD_RES_OK;
}
static commandResult_t CMD_HTTPOTA(const void *context, const char *cmd, const char *args, int cmdFlags) {

	if (args && *args) {
		OTA_RequestDownloadFromHTTP(args);
	}
	else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Command requires 1 argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_SimonTest(const void *context, const char *cmd, const char *args, int cmdFlags){
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SimonTest: ir test routine");

#ifdef PLATFORM_BK7231T
	//stackCrash(0);
	//CrashMalloc();
	// anything
#endif

	
	return CMD_RES_OK;
}

static commandResult_t CMD_Restart(const void *context, const char *cmd, const char *args, int cmdFlags){
	int delaySeconds;

	if(args==0||*args==0) {
		delaySeconds = 3;
	} else {
		delaySeconds = atoi(args);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Restart: will reboot in %i",delaySeconds);

	RESET_ScheduleModuleReset(delaySeconds);

	return CMD_RES_OK;
}
static commandResult_t CMD_ClearAll(const void *context, const char *cmd, const char *args, int cmdFlags) {

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	CHANNEL_ClearAllChannels();
	CMD_ClearAllHandlers(0, 0, 0, 0);
	RepeatingEvents_Cmd_ClearRepeatingEvents(0, 0, 0, 0); 
#if defined(WINDOWS) || defined(PLATFORM_BL602) || defined(PLATFORM_BEKEN)
	CMD_resetSVM(0, 0, 0, 0);
#endif

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearAll: all clear");

	return CMD_RES_OK;
}
static commandResult_t CMD_ClearNoPingTime(const void *context, const char *cmd, const char *args, int cmdFlags) {
	g_timeSinceLastPingReply = 0;
	return CMD_RES_OK;
}
static commandResult_t CMD_ClearConfig(const void *context, const char *cmd, const char *args, int cmdFlags){

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearConfig: whole device config has been cleared, restart device to connect to clear AP");

	return CMD_RES_OK;
}
static commandResult_t CMD_Echo(const void *context, const char *cmd, const char *args, int cmdFlags){


	ADDLOG_INFO(LOG_FEATURE_CMD, args);

	return CMD_RES_OK;
}
static commandResult_t CMD_SetStartValue(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int ch, val;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not enough arguments.");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ch = Tokenizer_GetArgInteger(0);
	val = Tokenizer_GetArgInteger(1);

	CFG_SetChannelStartupValue(ch, val);

	return CMD_RES_OK;
}


void CMD_Init_Early() {
	//cmddetail:{"name":"echo","args":"[Message]",
	//cmddetail:"descr":"Sends given message back to console.",
	//cmddetail:"fn":"CMD_Echo","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("echo", "", CMD_Echo, NULL, NULL);
	//cmddetail:{"name":"restart","args":"",
	//cmddetail:"descr":"Reboots the module",
	//cmddetail:"fn":"CMD_Restart","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("restart", "", CMD_Restart, NULL, NULL);
	//cmddetail:{"name":"clearConfig","args":"",
	//cmddetail:"descr":"Clears all config, including WiFi data",
	//cmddetail:"fn":"CMD_ClearConfig","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clearConfig", "", CMD_ClearConfig, NULL, NULL);
	//cmddetail:{"name":"clearAll","args":"",
	//cmddetail:"descr":"Clears config and all remaining features, like runtime scripts, events, etc",
	//cmddetail:"fn":"CMD_ClearAll","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("clearAll", "", CMD_ClearAll, NULL, NULL);
	//cmddetail:{"name":"PowerSave","args":"",
	//cmddetail:"descr":"Enable power save on N & T",
	//cmddetail:"fn":"CMD_PowerSave","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerSave", "", CMD_PowerSave, NULL, NULL);	
	//cmddetail:{"name":"simonirtest","args":"",
	//cmddetail:"descr":"Simons Special Test",
	//cmddetail:"fn":"CMD_SimonTest","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("simonirtest", "", CMD_SimonTest, NULL, NULL);
	//cmddetail:{"name":"if","args":"[Condition]['then'][CommandA]['else'][CommandB]",
	//cmddetail:"descr":"Executed a conditional. Condition should be single line. You must always use 'then' after condition. 'else' is optional. Use aliases or quotes for commands with spaces",
	//cmddetail:"fn":"CMD_If","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("if", NULL, CMD_If, NULL, NULL);
	//cmddetail:{"name":"ota_http","args":"[HTTP_URL]",
	//cmddetail:"descr":"Starts the firmware update procedure, the argument should be a reachable HTTP server file. You can easily setup HTTP server with Xampp, or Visual Code, or Python, etc. Make sure you are using OTA file for a correct platform (getting N platform RBL on T will brick device, etc etc)",
	//cmddetail:"fn":"CMD_HTTPOTA","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ota_http", "", CMD_HTTPOTA, NULL, NULL);
	//cmddetail:{"name":"scheduleHADiscovery","args":"[Seconds]",
	//cmddetail:"descr":"This will schedule HA discovery, the discovery will happen with given number of seconds, but timer only counts when MQTT is connected. It will not work without MQTT online, so you must set MQTT credentials first.",
	//cmddetail:"fn":"CMD_ScheduleHADiscovery","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("scheduleHADiscovery", "", CMD_ScheduleHADiscovery, NULL, NULL);
	//cmddetail:{"name":"flags","args":"[IntegerValue]",
	//cmddetail:"descr":"Sets the device flags",
	//cmddetail:"fn":"CMD_Flags","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("flags", "", CMD_Flags, NULL, NULL);
	//cmddetail:{"name":"ClearNoPingTime","args":"",
	//cmddetail:"descr":"Command for ping watchdog; it sets the 'time since last ping reply' to 0 again",
	//cmddetail:"fn":"CMD_ClearNoPingTime","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ClearNoPingTime", "", CMD_ClearNoPingTime, NULL, NULL);
	//cmddetail:{"name":"SetStartValue","args":"[Channel][Value]",
	//cmddetail:"descr":"Sets the startup value for a channel. Used for start values for relays. Use 1 for High, 0 for low and -1 for 'remember last state'",
	//cmddetail:"fn":"CMD_SetStartValue","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetStartValue", "", CMD_SetStartValue, NULL, NULL);
	
#if (defined WINDOWS) || (defined PLATFORM_BEKEN)
	CMD_InitScripting();
#endif
}

void CMD_Init_Delayed() {
	if(CFG_HasFlag(OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER)) {
		CMD_StartTCPCommandLine();
	}
}


void CMD_ListAllCommands(void *userData, void (*callback)(command_t *cmd, void *userData)) {
	int i;
	command_t *newCmd;

	for(i = 0; i < HASH_SIZE; i++) {
		newCmd = g_commands[i];
		while(newCmd) {
			callback(newCmd,userData);
			newCmd = newCmd->next;
		}
	}

}
void CMD_FreeAllCommands() {
	int i;
	command_t *cmd, *next;

	for (i = 0; i < HASH_SIZE; i++) {
		cmd = g_commands[i];
		while (cmd) {
			next = cmd->next;
			free(cmd);
			cmd = next;
		}
		g_commands[i] = 0;
	}

}
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context) {
	int hash;
	command_t *newCmd;

	// check
	newCmd = CMD_Find(name);
	if(newCmd != 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "command with name %s already exists!",name);
		return;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "Adding command %s",name);

	hash = generateHashValue(name);
	newCmd = (command_t*)malloc(sizeof(command_t));
	newCmd->argsFormat = args;
	newCmd->handler = handler;
	newCmd->name = name;
	newCmd->next = g_commands[hash];
	newCmd->userDesc = userDesc;
	newCmd->context = context;
	g_commands[hash] = newCmd;
}

command_t *CMD_Find(const char *name) {
	int hash;
	command_t *newCmd;

	hash = generateHashValue(name);

	newCmd = g_commands[hash];
	while(newCmd != 0) {
		if(!stricmp(newCmd->name,name)) {
			return newCmd;
		}
		newCmd = newCmd->next;
}
	return 0;
}

// get a string up to whitespace.
// if stripnum is set, stop at numbers.
int get_cmd(const char *s, char *dest, int maxlen, int stripnum){
	int i;
	int count = 0;
	for (i = 0; i < maxlen-1; i++){
		if (isWhiteSpace(*s)) {
			break;
		}
		if(*s == 0) {
			break;
		}
		if (stripnum && *s >= '0' && *s <= '9'){
			break;
		}
		*(dest++) = *(s++);
		count++;
	}
	*dest = '\0';
	return count;
}


// execute a command from cmd and args - used below and in MQTT
commandResult_t CMD_ExecuteCommandArgs(const char *cmd, const char *args, int cmdFlags) {
	command_t *newCmd;
	//int len;

	// look for complete commmand
	newCmd = CMD_Find(cmd);
	if (!newCmd) {
		// not found, so...
		char nonums[32];
		// get the complete string up to numbers.
		//len =
		get_cmd(cmd, nonums, 32, 1);
		newCmd = CMD_Find(nonums);
		if (!newCmd) {
			// if still not found, then error
			ADDLOG_ERROR(LOG_FEATURE_CMD, "cmd %s NOT found (args %s)", cmd, args);
			return CMD_RES_UNKNOWN_COMMAND;
		}
	} else {
	}

	if (newCmd->handler){
		commandResult_t res;
		res = newCmd->handler(newCmd->context, cmd, args, cmdFlags);
		return res;
	}
	return CMD_RES_UNKNOWN_COMMAND;
}


// execute a raw command - single string
commandResult_t CMD_ExecuteCommand(const char *s, int cmdFlags) {
	const char *p;
	const char *args;

	char copy[128];
	int len;
	//const char *org;

	if(s == 0 || *s == 0) {
		return CMD_RES_EMPTY_STRING;
	}

	while(isWhiteSpace(*s)) {
		s++;
	}
	if(*s == 0) {
		return CMD_RES_EMPTY_STRING;
	}
	if((cmdFlags & COMMAND_FLAG_SOURCE_TCP)==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "cmd [%s]", s);
	}
	//org = s;

	// get the complete string up to whitespace.
	len = get_cmd(s, copy, sizeof(copy), 0);
	s += len;

	p = s;

	while(*p && isWhiteSpace(*p)) {
		p++;
	}
	args = p;

	return CMD_ExecuteCommandArgs(copy, args,cmdFlags);
}


