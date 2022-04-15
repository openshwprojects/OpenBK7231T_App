

typedef int (*commandHandler_t)(const void *context, const char *cmd, const char *args);

typedef struct command_s {
	const char *name;
	const char *argsFormat;
	commandHandler_t handler;
	const char *userDesc;
	const void *context;
	struct command_s *next;
} command_t;

void CMD_Init();
command_t *CMD_Find(const char *name);
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context);
int CMD_ExecuteCommand(const char *s);
int CMD_ExecuteCommandArgs(const char *cmd, const char *args);
// NOTE: argsCount includes commands name, so 1 tells "only command"
int CMD_GetArgsCount() ;
// NOTE: arg0 is command name
const char *CMD_GetArg(int i);
// for autocompletion?
void CMD_ListAllCommands(void *userData, void (*callback)(command_t *cmd, void *userData));
int get_cmd(const char *s, char *dest, int maxlen, int stripnum);
bool isWhiteSpace(char ch);

//
int NewLED_InitCommands();
//

//

//





