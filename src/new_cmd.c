#include "new_cmd.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "logging/logging.h"
#include "obk_config.h"
#include <ctype.h>
#ifdef BK_LITTLEFS
	#include "littlefs/our_lfs.h"
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

void CMD_Init() {
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
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context) {
	int hash;
	command_t *newCmd;

	// check
	newCmd = CMD_Find(name);
	if(newCmd != 0) {
		printf("ERROR: command with name %s already exists!\n",name);
		return;
	}

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

#define MAX_CMD_LEN 512
#define MAX_ARGS 32

//static char g_buffer[MAX_CMD_LEN];
//static char *g_args[MAX_ARGS];
//static int g_numArgs = 0;

bool isWhiteSpace(char ch) {
	if(ch == ' ')
		return true;
	if(ch == '\t')
		return true;
	if(ch == '\n')
		return true;
	if(ch == '\r')
		return true;
	return false;
}
// NOTE: arg0 is command name
//int CMD_GetArgsCount() {
//	return g_numArgs;
//}
// NOTE: arg0 is command name
//const char *CMD_GetArg(int i) {
//	return g_args[i];
//}

// get a string up to whitespace.
// if stripnum is set, stop at numbers.
int get_cmd(const char *s, char *dest, int maxlen, int stripnum){
	int i;
	int count = 0;
	for (i = 0; i < maxlen-1; i++){
		if (isWhiteSpace(*s)) {
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
int CMD_ExecuteCommandArgs(const char *cmd, const char *args) {
	command_t *newCmd;
	int len;
	char *org;
	char *s = cmd;

	// look for complete commmand
	newCmd = CMD_Find(cmd);
	if (!newCmd) {
		// not found, so...
		char nonums[32];
		// get the complete string up to numbers.
		len = get_cmd(cmd, nonums, 32, 1);
		newCmd = CMD_Find(nonums);
		if (!newCmd) {
			// if still not found, then error
			ADDLOG_ERROR(LOG_FEATURE_CMD, "cmd %s NOT found", cmd);
			return 0;
		}
	} else {
	}

	if (newCmd->handler){
		int res;
		res = newCmd->handler(newCmd->context, cmd, args);
		return res;
	}
	return 0;
}


// execute a raw command - single string
int CMD_ExecuteCommand(const char *s) {
	char *p;
	char *args;
	command_t *newCmd;

	char copy[32];
	int len;
	char *org;

	ADDLOG_DEBUG(LOG_FEATURE_CMD, "cmd [%s]", s);

	while(isWhiteSpace(*s)) {
		s++;
	}
	org = s;

	// get the complete string up to whitespace.
	len = get_cmd(s, copy, 32, 0);
	s += len;

	p = s;

	while(*p && isWhiteSpace(*p)) {
		p++;
	}
	args = p;

	return CMD_ExecuteCommandArgs(copy, args);
}


