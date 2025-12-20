#ifndef __CMD_LOCAL_H__
#define __CMD_LOCAL_H__

#include "cmd_public.h"

#define CMD_FLAG_FREE_NAME		1
#define CMD_FLAG_FREE_CONTEXT	2

typedef struct command_s {
	const char *name;
	commandHandler_t handler;
	const void *context;
	struct command_s *next;
	int commandFlags;
} command_t;

command_t *CMD_Find(const char *name);
// for autocompletion?
void CMD_ListAllCommands(void *userData, void (*callback)(command_t *cmd, void *userData));
int get_cmd(const char *s, char *dest, int maxlen, int stripnum);


float CMD_EvaluateExpression(const char *s, const char *stop);
commandResult_t CMD_If(const void *context, const char *cmd, const char *args, int cmdFlags);
void CMD_ExpandConstantsWithinString(const char *in, char *out, int outLen);
void CMD_Script_ProcessWaitersForEvent(byte eventCode, int argument);
bool CheckEventCondition(eventWait_t *w, byte eventCode, int argument);
void CMD_FreeLabels();


#endif // __CMD_LOCAL_H__









