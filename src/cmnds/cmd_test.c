

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_common.h"
#include "../obk_config.h"
#include "../cJSON/cJSON.h"
#include <ctype.h>
#include "cmd_local.h"
#if ENABLE_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

#if ENABLE_TEST_COMMANDS

static commandResult_t CMD_TimeSize(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "sizeof(time_t) = %i, sizeof(int) = %i", sizeof(time_t), sizeof(int));
	return CMD_RES_OK;
}
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
	float b = 3.14f;

	ADDLOG_INFO(LOG_FEATURE_CMD, "This is an int - %i",a);
	ADDLOG_INFO(LOG_FEATURE_CMD, "This is a float - %f",b);
	
	return CMD_RES_OK;
}

static commandResult_t testFloats(const void * context, const char *cmd, const char *args, int cmdFlags){
	int a = 123;
	float b = 3.14f;
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
		cJSON_AddNumberToObject(root, "uptime", g_secondsElapsed);
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


// Usage for continous test: addRepeatingEvent 1 -1 lfs_test1 ir.bat
static commandResult_t cmnd_lfs_test1(const void * context, const char *cmd, const char *args, int cmdFlags) {
#if ENABLE_LITTLEFS
	if (lfs_present()) {
		lfs_file_t file;
		int lfsres;
		char a;
		int cnt;

		cnt = 0;

		memset(&file, 0, sizeof(lfs_file_t));
		lfsres = lfs_file_open(&lfs, &file, args, LFS_O_RDONLY);

		ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: sizeof(lfs_file_t) %i", sizeof(lfs_file_t));
		if (lfsres >= 0) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: openned file %s", args);
			do {
				lfsres = lfs_file_read(&lfs, &file, &a, 1);
				cnt++;
			} while (lfsres > 0);
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: Stopped at char %i\n", cnt);

			lfs_file_close(&lfs, &file);
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: closed file %s", args);
		}
		else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: failed to file %s", args);
		}
	}
	else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "cmnd_lfs_test1: lfs is absent");
	}
#endif
	return CMD_RES_OK;
}
// Usage for continous test: addRepeatingEvent 1 -1 lfs_test2 ir.bat
static commandResult_t cmnd_lfs_test2(const void * context, const char *cmd, const char *args, int cmdFlags) {
#if ENABLE_LITTLEFS
	if (lfs_present()) {
		lfs_file_t *file;
		int lfsres;
		char a;
		int cnt;

		cnt = 0;

		file = malloc(sizeof(lfs_file_t));
		if (file == 0) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: failed to malloc for %s", args);
		}
		else {
			memset(file, 0, sizeof(lfs_file_t));
			lfsres = lfs_file_open(&lfs, file, args, LFS_O_RDONLY);

			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: sizeof(lfs_file_t) %i", sizeof(lfs_file_t));
			if (lfsres >= 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: openned file %s", args);
				do {
					lfsres = lfs_file_read(&lfs, file, &a, 1);
					cnt++;
				} while (lfsres > 0);
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: Stopped at char %i\n", cnt);

				lfs_file_close(&lfs, file);
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: closed file %s", args);
			}
			else {
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: failed to file %s", args);
			}
			free(file);
		}
	}
	else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "cmnd_lfs_test2: lfs is absent");
	}
#endif
	return CMD_RES_OK;
}

static commandResult_t cmnd_json_test(const void * context, const char *cmd, const char *args, int cmdFlags) {
	int i;
	cJSON* root;
	cJSON* stats;
	char *msg;
	float dailyStats[4] = { 95.44071197f, 171.84954833f, 181.58737182f, 331.35061645f };

	root = cJSON_CreateObject();
	{
		stats = cJSON_CreateArray();
		for (i = 0; i < 4; i++)
		{
			cJSON_AddItemToArray(stats, cJSON_CreateNumber(dailyStats[i]));
		}
		cJSON_AddItemToObject(root, "consumption_daily", stats);
	}

	msg = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	ADDLOG_INFO(LOG_FEATURE_CMD, "Test JSON reads: %s", msg);
	free(msg);
	return CMD_RES_OK;
}
// Usage for continous test: addRepeatingEvent 1 -1 lfs_test3 ir.bat
static commandResult_t cmnd_lfs_test3(const void * context, const char *cmd, const char *args, int cmdFlags) {
	byte *res;

	res = LFS_ReadFile(args);

	if (res) {
		free(res);
	}
	return CMD_RES_OK;
}
static void stackOverflow(int a) {
	char lala[64];
	int i;

	for (i = 0; i < sizeof(lala); i++) {
		lala[i] = a;
	}
	stackOverflow(a + 1);
}
static commandResult_t CMD_StackOverflow(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_StackOverflow: Will overflow soon");

	stackOverflow(0);

	return CMD_RES_OK;
}
static commandResult_t CMD_CrashNull(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int *p = (int*)0;

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_CrashNull: Will crash soon");

	while (1) {
		*p = 0;
		p++;
	}



	return CMD_RES_OK;
}
static commandResult_t CMD_SimonTest(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SimonTest: ir test routine");

#ifdef PLATFORM_BK7231T
	//stackCrash(0);
	//CrashMalloc();
	// anything
#endif

	return CMD_RES_OK;
}

int CMD_InitTestCommands(){
	//cmddetail:{"name":"testMallocFree","args":"",
	//cmddetail:"descr":"Test malloc and free functionality to see if the device crashes",
	//cmddetail:"fn":"testMallocFree","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testMallocFree", testMallocFree, NULL);
	//cmddetail:{"name":"testRealloc","args":"",
	//cmddetail:"descr":"Test realloc and free functions to see if the device crashes",
	//cmddetail:"fn":"testRealloc","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testRealloc", testRealloc, NULL);
	//cmddetail:{"name":"testJSON","args":"",
	//cmddetail:"descr":"Test the JSON library",
	//cmddetail:"fn":"testJSON","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testJSON", testJSON, NULL);
	//cmddetail:{"name":"testLog","args":"",
	//cmddetail:"descr":"Do some test printfs to log with integer and a float",
	//cmddetail:"fn":"testLog","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testLog", testLog, NULL);
	//cmddetail:{"name":"testFloats","args":"",
	//cmddetail:"descr":"Do some more test printfs with floating point numbers",
	//cmddetail:"fn":"testFloats","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testFloats", testFloats, NULL);
	//cmddetail:{"name":"testArgs","args":"",
	//cmddetail:"descr":"Test tokenizer for args and print back all the given args to console",
	//cmddetail:"fn":"testArgs","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testArgs", testArgs, NULL);
	//cmddetail:{"name":"testStrdup","args":"",
	//cmddetail:"descr":"Test strdup function to see if it allocs news string correctly, also test freeing the string",
	//cmddetail:"fn":"testStrdup","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testStrdup", testStrdup, NULL);
	//cmddetail:{"name":"lfs_test1","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test1", cmnd_lfs_test1, NULL);
	//cmddetail:{"name":"lfs_test2","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test2","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test2", cmnd_lfs_test2, NULL);
	//cmddetail:{"name":"lfs_test3","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test3","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test3", cmnd_lfs_test3, NULL);
	//cmddetail:{"name":"json_test","args":"cmnd_json_test",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("json_test", cmnd_json_test, NULL);
	//cmddetail:{"name":"TimeSize","args":"",
	//cmddetail:"descr":"Prints the size of time_t on current platform - sizeof(time_t), only for testing purposes",
	//cmddetail:"fn":"NULL);","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TimeSize", CMD_TimeSize, NULL);
	//cmddetail:{"name":"stackOverflow","args":"",
	//cmddetail:"descr":"Causes a stack overflow",
	//cmddetail:"fn":"CMD_StackOverflow","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("stackOverflow", CMD_StackOverflow, NULL);
	//cmddetail:{"name":"crashNull","args":"",
	//cmddetail:"descr":"Causes a crash",
	//cmddetail:"fn":"CMD_CrashNull","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("crashNull", CMD_CrashNull, NULL);
	//cmddetail:{"name":"simonirtest","args":"",
	//cmddetail:"descr":"Simons Special Test",
	//cmddetail:"fn":"CMD_SimonTest","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("simonirtest", CMD_SimonTest, NULL);

	
    return 0;
}

#endif // ENABLE_TEST_COMMANDS
