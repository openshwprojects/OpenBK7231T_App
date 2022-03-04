

typedef void (*commandHandler_t)();

typedef struct command_s {
	const char *name;
	const char *argsFormat;
	commandHandler_t handler;
	struct command_s *next;
} command_t;

command_t *CMD_Find(const char *name);
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler);
void CMD_ExecuteCommand(const char *s);




