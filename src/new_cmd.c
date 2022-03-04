#include "new_cmd.h"
#include "new_pins.h"
#include "new_cfg.h"
#include <ctype.h>

#define HASH_SIZE 128

static int generateHashValue(const char *fname) {
	int		i;
	int		hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(int)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (HASH_SIZE-1);
	return hash;
}

command_t *g_commands[HASH_SIZE];

void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler) {
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
	}
	return 0;
}

#define MAX_CMD_LEN 512
#define MAX_ARGS 32

static char g_buffer[MAX_CMD_LEN];
static char *g_args[MAX_ARGS];
static int g_numArgs = 0;

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
int CMD_ExecuteCommand(const char *s) {
	int r = 0;
	char *p;
	int i;
	command_t *newCmd;

	while(isWhiteSpace(*s)) {
		s++;
	}

	strcpy(g_buffer,s);
	p = g_buffer;
	g_numArgs = 0;
	g_args[g_numArgs] = p;
	g_numArgs++;
	while(*p != 0) {
		if(isWhiteSpace(*p)) {
			*p = 0;
			if((p[1] != 0)) {
				g_args[g_numArgs] = p+1;
				g_numArgs++;
			}
		}
		if(*p == ',') {
			*p = 0;
			g_args[g_numArgs] = p+1;
			g_numArgs++;
		}
		p++;
	}

	if(1){
		printf("Command parsed debug out! %i args\n",g_numArgs);
		for(i = 0; i < g_numArgs; i++) {
			printf("Arg %i is %s\n",i,g_args[i]);
		}
	}

	newCmd = CMD_Find(g_args[0]);
	if(newCmd != 0) {
		r++;
		newCmd->handler();
	}

	return r;
}


