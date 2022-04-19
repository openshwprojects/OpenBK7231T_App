#ifndef __CMD_PUBLIC_H__
#define __CMD_PUBLIC_H__

typedef int (*commandHandler_t)(const void *context, const char *cmd, const char *args);



//
void CMD_Init();
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context);
int CMD_ExecuteCommand(const char *s);
int CMD_ExecuteCommandArgs(const char *cmd, const char *args);


// cmd_tokenizer.c
int Tokenizer_GetArgsCount();
const char *Tokenizer_GetArg(int i);
const char *Tokenizer_GetArgFrom(int i);
int Tokenizer_GetArgInteger(int i);
void Tokenizer_TokenizeString(const char *s);
// cmd_repeatingEvents.c
void RepeatingEvents_Init();
void RepeatingEvents_OnEverySecond();
// cmd_tasmota.c
int taslike_commands_init();
// cmd_newLEDDriver.c
int NewLED_InitCommands();
// cmd_test.c
int fortest_commands_init();

#endif // __CMD_PUBLIC_H__
