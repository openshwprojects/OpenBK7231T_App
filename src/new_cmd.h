

typedef int (*commandHandler_t)(const void *context, const char *cmd, char *args);

typedef struct command_s {
	const char *name;
	const char *argsFormat;
	commandHandler_t handler;
	const char *userDesc;
	const void *context;
	struct command_s *next;
} command_t;

command_t *CMD_Find(const char *name);
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context);
// allow modification of s
int CMD_ExecuteCommand(char *s);
// NOTE: argsCount includes commands name, so 1 tells "only command"
int CMD_GetArgsCount() ;
// NOTE: arg0 is command name
const char *CMD_GetArg(int i);
// for autocompletion?
void CMD_ListAllCommands(void *userData, void (*callback)(command_t *cmd, void *userData));




