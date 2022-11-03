

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_common.h"
#include "../obk_config.h"
#include "../cJSON/cJSON.h"
#include <ctype.h>
#include "cmd_local.h"
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif


// run an aliased command
static int runcmd(const void * context, const char *cmd, const char *args, int cmdFlags){
    char *c = (char *)context;
    char *p = c;

    while (*p && !isWhiteSpace(*p)) {
        p++;
	}
    if (*p) p++;
    return CMD_ExecuteCommand(p,cmdFlags);
}

// run an aliased command
static int alias(const void * context, const char *cmd, const char *args, int cmdFlags){
	const char *alias;
	const char *ocmd;
	char *cmdMem;
	char *aliasMem;
	command_t *existing;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_Alias: command require 2 args");
		return 1;
	}
	Tokenizer_TokenizeString(args,0);
	if(Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_Alias: command require 2 args");
		return 1;
	}

	alias = Tokenizer_GetArg(0);
	ocmd = Tokenizer_GetArgFrom(1);

	existing = CMD_Find(alias);

	if(existing!=0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_Alias: the alias you are trying to use is already in use (as an alias or as a command)");
		return 1;
	}

	cmdMem = strdup(ocmd);
	aliasMem = strdup(alias);

	ADDLOG_INFO(LOG_FEATURE_CMD, "New alias has been set: %s runs %s", alias, ocmd);

    CMD_RegisterCommand(aliasMem, "", runcmd, "custom", cmdMem);
	return 1;
}

// Usage: addRepeatingEvent 1 -1 testMallocFree 100
static int testMallocFree(const void * context, const char *cmd, const char *args, int cmdFlags){
	int repeats;
	int rep;
    char *msg;
	//int i;
	int ra1;
	static int totalCalls = 0;

	repeats = atoi(args);
	if(repeats < 1)
		repeats = 1;

	totalCalls++;

	for(rep = 0; rep < repeats; rep++) {
		ra1 = 1 + abs(rand() % 1000);

		msg = malloc(ra1);
		memset(msg,rand()%255,ra1);
		os_free(msg);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Malloc has been tested! Total calls %i, reps now %i",totalCalls,repeats);

    return 0;
}
// Usage: addRepeatingEvent 1 -1 testStrdup
static int testStrdup(const void * context, const char *cmd, const char *args, int cmdFlags){
	int repeats;
	int rep;
    char *msg;
	int i;
	int ra1;
	static int totalCalls = 0;
	const char *s = "Strdup test123";

	repeats = atoi(args);
	if(repeats < 1)
		repeats = 1;

	totalCalls++;

	for(rep = 0; rep < repeats; rep++) {
		ra1 = 1 + abs(rand() % 1000);

		msg = strdup(s);
		
	

		os_free(msg);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "testStrdup has been tested! Total calls %i, reps now %i",totalCalls,repeats);

    return 0;
}
// Usage: addRepeatingEvent 1 -1 testRealloc 100
static int testRealloc(const void * context, const char *cmd, const char *args, int cmdFlags){
	int repeats;
	int rep;
    char *msg;
	int i;
	int ra1;
	static int totalCalls = 0;

	repeats = atoi(args);
	if(repeats < 1)
		repeats = 1;

	totalCalls++;

	for(rep = 0; rep < repeats; rep++) {
		ra1 = 1 + abs(rand() % 1000);

		msg = malloc(ra1);
		
		for(i = 0; i < 3; i++) {
			ra1 += 1 + abs(rand()%10);
			msg = realloc(msg,ra1);
		}

		os_free(msg);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Realloc has been tested! Total calls %i, reps now %i",totalCalls,repeats);

    return 0;
}

extern void ftoa_fixed(char *buffer, double value);
static int testLog(const void * context, const char *cmd, const char *args, int cmdFlags){
	int a = 123;
	float b = 3.14;

	ADDLOG_INFO(LOG_FEATURE_CMD, "This is an int - %i",a);
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a float - %f",b);
	
	return 1;
}

static int testFloats(const void * context, const char *cmd, const char *args, int cmdFlags){
	int a = 123;
	float b = 3.14;

	ADDLOG_INFO(LOG_FEATURE_CMD, "This is an int - %i",a);

	ADDLOG_INFO(LOG_FEATURE_CMD, "will do ftoa_fixed(buffer, float=0.01);");
	char buff[40];
	float t = 0.01;
	ftoa_fixed(buff, t);
	buff[39] = 0;
	ADDLOG_INFO(LOG_FEATURE_CMD, "result %s, will do ftoa_fixed(buffer, double=0.01);", buff);
	double q = 0.01;
	ftoa_fixed(buff, q);
	buff[39] = 0;
	ADDLOG_INFO(LOG_FEATURE_CMD, "result %s, will do ftoa_fixed(buffer, double=1/3);", buff);
	q = 1.0;
	q = q/3.0;
	ftoa_fixed(buff, q);
	buff[39] = 0;
	ADDLOG_INFO(LOG_FEATURE_CMD, "result %s, will do ftoa_fixed(buffer, double=2/3);", buff);
	q = 2.0;
	q = q/3.0;
	ftoa_fixed(buff, q);
	buff[39] = 0;
	ADDLOG_INFO(LOG_FEATURE_CMD, "result %s", buff);
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a float float - %f %f",b,b);
	double d = (double)b;
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a double double - %f %f",d, d);
	
	return 1;
}
// testArgs "one" "tw o" "thr ee" "fo ur" five
// testArgs one "tw o" "thr ee" "fo ur" five
static int testArgs(const void * context, const char *cmd, const char *args, int cmdFlags){
	int i, cnt;

	
	Tokenizer_TokenizeString(args,1);
	cnt = Tokenizer_GetArgsCount();
	ADDLOG_INFO(LOG_FEATURE_CMD, "Args count: %i",cnt);
	for(i = 0; i < cnt; i++) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Arg %i is %s",i,Tokenizer_GetArg(i));
	}


	return 1;
}
// Usage: addRepeatingEvent 1 -1 testJSON 100
static int testJSON(const void * context, const char *cmd, const char *args, int cmdFlags){
    cJSON* root;
    cJSON* stats;
	int repeats;
	int rep;
    char *msg;
	int i;
	int ra1, ra2, ra3, ra4, ra5;
	static int totalCalls = 0;

	repeats = atoi(args);
	if(repeats < 1)
		repeats = 1;

	totalCalls++;

	for(rep = 0; rep < repeats; rep++) {
		ra1 = rand() % 1000;
		ra2 = rand() % 1000;
		ra3 = rand() % 1000;
		ra4 = rand() % 1000;
		ra5 = rand() % 1000;


		root = cJSON_CreateObject();
		cJSON_AddNumberToObject(root, "uptime", Time_getUpTimeSeconds());
		cJSON_AddNumberToObject(root, "consumption_total", ra1 );
		cJSON_AddNumberToObject(root, "consumption_last_hour",  ra2);
		cJSON_AddNumberToObject(root, "consumption_stat_index", ra3);
		cJSON_AddNumberToObject(root, "consumption_sample_count", ra4);
		cJSON_AddNumberToObject(root, "consumption_sampling_period", ra5);

			stats = cJSON_CreateArray();
		for(i = 0; i < 23; i++)
		{
			cJSON_AddItemToArray(stats, cJSON_CreateNumber(rand()%10));
		}
		cJSON_AddItemToObject(root, "consumption_samples", stats);

		msg = cJSON_Print(root);
		cJSON_Delete(root);
		os_free(msg);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "testJSON has been tested! Total calls %i, reps now %i",totalCalls,repeats);

    return 0;
}
int fortest_commands_init(){
    CMD_RegisterCommand("alias", "", alias, "add a custom command", NULL);
    CMD_RegisterCommand("testMallocFree", "", testMallocFree, "", NULL);
    CMD_RegisterCommand("testRealloc", "", testRealloc, "", NULL);
    CMD_RegisterCommand("testJSON", "", testJSON, "", NULL);
    CMD_RegisterCommand("testLog", "", testLog, "", NULL);
    CMD_RegisterCommand("testFloats", "", testFloats, "", NULL);

    CMD_RegisterCommand("testArgs", "", testArgs, "", NULL);
    CMD_RegisterCommand("testStrdup", "", testStrdup, "", NULL);
    return 0;
}

