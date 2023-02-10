

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

// Usage: addRepeatingEvent 1 -1 testMallocFree 100
static commandResult_t testMallocFree(const void * context, const char *cmd, const char *args, int cmdFlags){
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

    return CMD_RES_OK;
}
// Usage: addRepeatingEvent 1 -1 testStrdup
static commandResult_t testStrdup(const void * context, const char *cmd, const char *args, int cmdFlags){
	int repeats;
	int rep;
    char *msg;
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

    return CMD_RES_OK;
}
// Usage: addRepeatingEvent 1 -1 testRealloc 100
static commandResult_t testRealloc(const void * context, const char *cmd, const char *args, int cmdFlags){
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

    return CMD_RES_OK;
}

static commandResult_t testLog(const void * context, const char *cmd, const char *args, int cmdFlags){
	int a = 123;
	float b = 3.14;

	ADDLOG_INFO(LOG_FEATURE_CMD, "This is an int - %i",a);
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a float - %f",b);
	
	return CMD_RES_OK;
}

static commandResult_t testFloats(const void * context, const char *cmd, const char *args, int cmdFlags){
	int a = 123;
	float b = 3.14;
	double d;

	ADDLOG_INFO(LOG_FEATURE_CMD, "This is an int - %i",a);
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a float float - %f %f",b,b);
	d = (double)b;
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a double double - %f %f",d, d);
	
	return CMD_RES_OK;
}
// testArgs "one" "tw o" "thr ee" "fo ur" five
// testArgs one "tw o" "thr ee" "fo ur" five
static commandResult_t testArgs(const void * context, const char *cmd, const char *args, int cmdFlags){
	int i, cnt;

	
	Tokenizer_TokenizeString(args,1);
	cnt = Tokenizer_GetArgsCount();
	ADDLOG_INFO(LOG_FEATURE_CMD, "Args count: %i",cnt);
	for(i = 0; i < cnt; i++) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Arg %i is %s",i,Tokenizer_GetArg(i));
	}


	return CMD_RES_OK;
}
// Usage: addRepeatingEvent 1 -1 testJSON 100
static commandResult_t testJSON(const void * context, const char *cmd, const char *args, int cmdFlags){
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

    return CMD_RES_OK;
}
int CMD_InitTestCommands(){
	//cmddetail:{"name":"testMallocFree","args":"",
	//cmddetail:"descr":"Test malloc and free functionality to see if the device crashes",
	//cmddetail:"fn":"testMallocFree","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testMallocFree", NULL, testMallocFree, NULL, NULL);
	//cmddetail:{"name":"testRealloc","args":"",
	//cmddetail:"descr":"Test realloc and free functions to see if the device crashes",
	//cmddetail:"fn":"testRealloc","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testRealloc", NULL, testRealloc, NULL, NULL);
	//cmddetail:{"name":"testJSON","args":"",
	//cmddetail:"descr":"Test the JSON library",
	//cmddetail:"fn":"testJSON","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testJSON", NULL, testJSON, NULL, NULL);
	//cmddetail:{"name":"testLog","args":"",
	//cmddetail:"descr":"Do some test printfs to log with integer and a float",
	//cmddetail:"fn":"testLog","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testLog", NULL, testLog, NULL, NULL);
	//cmddetail:{"name":"testFloats","args":"",
	//cmddetail:"descr":"Do some more test printfs with floating point numbers",
	//cmddetail:"fn":"testFloats","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testFloats", NULL, testFloats, NULL, NULL);

	//cmddetail:{"name":"testArgs","args":"",
	//cmddetail:"descr":"Test tokenizer for args and print back all the given args to console",
	//cmddetail:"fn":"testArgs","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testArgs", NULL, testArgs, NULL, NULL);
	//cmddetail:{"name":"testStrdup","args":"",
	//cmddetail:"descr":"Test strdup function to see if it allocs news string correctly, also test freeing the string",
	//cmddetail:"fn":"testStrdup","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testStrdup", NULL, testStrdup, NULL, NULL);
    return 0;
}

