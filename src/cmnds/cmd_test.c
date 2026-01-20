

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

static commandResult_t CMD_getPin(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args,0);
	
	ADDLOG_INFO(LOG_FEATURE_CMD, "Pin index for %s is %i", Tokenizer_GetArg(0), Tokenizer_GetPin(0,-1));
	return CMD_RES_OK;
}
static commandResult_t CMD_TimeSize(const void* context, const char* cmd, const char* args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, "sizeof(time_t) = %i, sizeof(int) = %i", sizeof(time_t), sizeof(int));
	return CMD_RES_OK;
}
static commandResult_t CMD_TestSprintfForInteger(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char dummy[32];
	sprintf(dummy, "%i", 123);
	if(strcmp(dummy,"123")==0)
		return CMD_RES_OK;
	return CMD_RES_ERROR;
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
	static int reallocBroken = 0;

	repeats = atoi(args);
	if(repeats < 1)
		repeats = 1;

	totalCalls++;

	for(rep = 0; rep < repeats; rep++) {
		ra1 = 1 + abs(rand() % 1000);

		msg = malloc(ra1);
		if (!msg) {
			break;
		}
		int initialra1 = ra1;
		for (int i = 0; i < initialra1; i++) {
			msg[i] = i % 100;
		}
		
		for(i = 0; i < 3; i++) {
			ra1 += 1 + abs(rand()%10);
			char* msgr = realloc(msg,ra1);
			if (!msgr) {
				break;
			}
			msg = msgr;

			for (int j = 0; j < initialra1; j++) {
				if (msg[j] != (j % 100)) {
					ADDLOG_INFO(LOG_FEATURE_CMD,
							"Realloc difference: rep %d, i %d j %d initialra1 %d ra1 %d msg[j] %d (j % 100) %d",
							rep, i, j, initialra1, ra1, (int) msg[j], j % 100);
					reallocBroken = 1;
				}
			}
		}

		os_free(msg);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Realloc has been tested! Total calls %i, reps now %i, reallocBroken %i",totalCalls,repeats,reallocBroken);

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

#define TEST_ATOI(x) if (atoi(#x) != x) return CMD_RES_ERROR;

static commandResult_t testAtoi(const void * context, const char *cmd, const char *args, int cmdFlags) {

	TEST_ATOI(123421);
	TEST_ATOI(3223);
	TEST_ATOI(-3223);

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
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: opened file %s", args);
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
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: opened file %s", args);
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
// ESP will refuse compilation ( error: infinite recursion detected [-Werror=infinite-recursion])
#ifndef PLATFORM_ESPIDF
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
#endif
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
static commandResult_t CMD_TestSprintfForHex(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char dummy[32];
	sprintf(dummy, "%X", 255);
	return (strcmp(dummy, "FF") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestSprintfForFloat(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char dummy[32];
	sprintf(dummy, "%.2f", 3.14159);
	return (strcmp(dummy, "3.14") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestStrcpy(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char src[] = "hello";
	char dest[10];
	strcpy(dest, src);
	return (strcmp(dest, "hello") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestStrncpy(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char src[] = "world";
	char dest[10];
	strncpy(dest, src, 5);
	dest[5] = '\0';
	return (strcmp(dest, "world") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestStrcmp(const void* context, const char* cmd, const char* args, int cmdFlags) {
	return (strcmp("test", "test") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestStrlen(const void* context, const char* cmd, const char* args, int cmdFlags) {
	return (strlen("abcd") == 4) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestStrcat(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char dest[20] = "Hello, ";
	strcat(dest, "World!");
	return (strcmp(dest, "Hello, World!") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestMemset(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char buf[10];
	memset(buf, 'A', sizeof(buf) - 1);
	buf[9] = '\0';
	return (strcmp(buf, "AAAAAAAAA") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestMemcpy(const void* context, const char* cmd, const char* args, int cmdFlags) {
	char src[] = "copy";
	char dest[10];
	memcpy(dest, src, 5);
	return (strcmp(dest, "copy") == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t CMD_TestMemcmp(const void* context, const char* cmd, const char* args, int cmdFlags) {
	return (memcmp("abc", "abc", 3) == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}
static commandResult_t CMD_TestChannelSetToExpression(const void* context, const char* cmd, const char* args, int cmdFlags) {
	CMD_ExecuteCommand("setChannel 50 2*5+3", 0);
	if (CHANNEL_Get(50) == (2 * 5 + 3)) {
		return CMD_RES_OK;
	}
	return CMD_RES_ERROR;
}
static commandResult_t CMD_TestParseIP(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte ip[4];
	str_to_ip("192.168.0.123", ip);
	if (ip[0] != 192 || ip[1] != 168 || ip[2] != 0 || ip[3] != 123) {
		return CMD_RES_ERROR;
	}
	return CMD_RES_OK;
}
static commandResult_t CMD_TestIPtoStr(const void* context, const char* cmd, const char* args, int cmdFlags) {
	byte ip[4];
	ip[0] = 192;
	ip[1] = 168;
	ip[2] = 0;
	ip[3] = 123;
	char buff[64];
	convert_IP_to_string(buff, ip);
	if (strcmp(buff,"192.168.0.123")) {
		return CMD_RES_ERROR;
	}
	return CMD_RES_OK;
}


int CMD_InitTestCommands(){
	//cmddetail:{"name":"getPin","args":"string",
	//cmddetail:"descr":"find pin index for a pin alias",
	//cmddetail:"fn":"CMD_getPin","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("getPin", CMD_getPin, NULL);
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
	//cmddetail:{"name":"testAtoi","args":"",
	//cmddetail:"descr":"Test atoi function",
	//cmddetail:"fn":"testAtoi","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("testAtoi", testAtoi, NULL);
	//cmddetail:{"name":"testStrdup","args":"",
	//cmddetail:"descr":"Test strdup function to see if it allocs news string correctly, also test freeing the string",
	//cmddetail:"fn":"testStrdup","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("testStrdup", testStrdup, NULL);
	//cmddetail:{"name":"lfs_test1","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test1","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test1", cmnd_lfs_test1, NULL);
	//cmddetail:{"name":"lfs_test2","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test2","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test2", cmnd_lfs_test2, NULL);
	//cmddetail:{"name":"lfs_test3","args":"[FileName]",
	//cmddetail:"descr":"Tests the LFS file reading feature.",
	//cmddetail:"fn":"cmnd_lfs_test3","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test3", cmnd_lfs_test3, NULL);
	//cmddetail:{"name":"json_test","args":"",
	//cmddetail:"descr":"Developer-only command used to test CJSON library",
	//cmddetail:"fn":"cmnd_json_test","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("json_test", cmnd_json_test, NULL);
	//cmddetail:{"name":"TimeSize","args":"",
	//cmddetail:"descr":"Prints the size of time_t on current platform - sizeof(time_t), only for testing purposes",
	//cmddetail:"fn":"CMD_TimeSize","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TimeSize", CMD_TimeSize, NULL);
#ifndef PLATFORM_ESPIDF
	//cmddetail:{"name":"stackOverflow","args":"",
	//cmddetail:"descr":"Causes a stack overflow",
	//cmddetail:"fn":"CMD_StackOverflow","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("stackOverflow", CMD_StackOverflow, NULL);
#endif
	//cmddetail:{"name":"crashNull","args":"",
	//cmddetail:"descr":"Causes a crash",
	//cmddetail:"fn":"CMD_CrashNull","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("crashNull", CMD_CrashNull, NULL);
	//cmddetail:{"name":"simonirtest","args":"",
	//cmddetail:"descr":"Simons Special Test",
	//cmddetail:"fn":"CMD_SimonTest","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("simonirtest", CMD_SimonTest, NULL);

	//cmddetail:{"name":"TestSprintfForInteger","args":"CMD_TestSprintfForInteger",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestSprintfForInteger","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestSprintfForInteger", CMD_TestSprintfForInteger, NULL);
	//cmddetail:{"name":"TestSprintfForHex","args":"CMD_TestSprintfForHex",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestSprintfForHex","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestSprintfForHex", CMD_TestSprintfForHex, NULL);
	//cmddetail:{"name":"TestSprintfForFloat","args":"CMD_TestSprintfForFloat",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestSprintfForFloat","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestSprintfForFloat", CMD_TestSprintfForFloat, NULL);
	//cmddetail:{"name":"TestStrcpy","args":"CMD_TestStrcpy",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestStrcpy","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestStrcpy", CMD_TestStrcpy, NULL);
	//cmddetail:{"name":"TestStrncpy","args":"CMD_TestStrncpy",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestStrncpy","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestStrncpy", CMD_TestStrncpy, NULL);
	//cmddetail:{"name":"TestStrcmp","args":"CMD_TestStrcmp",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestStrcmp","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestStrcmp", CMD_TestStrcmp, NULL);
	//cmddetail:{"name":"TestStrlen","args":"CMD_TestStrlen",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestStrlen","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestStrlen", CMD_TestStrlen, NULL);
	//cmddetail:{"name":"TestStrcat","args":"CMD_TestStrcat",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestStrcat","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestStrcat", CMD_TestStrcat, NULL);
	//cmddetail:{"name":"TestMemset","args":"CMD_TestMemset",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestMemset","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestMemset", CMD_TestMemset, NULL);
	//cmddetail:{"name":"TestMemcpy","args":"CMD_TestMemcpy",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestMemcpy","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestMemcpy", CMD_TestMemcpy, NULL);
	//cmddetail:{"name":"TestMemcmp","args":"CMD_TestMemcmp",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestMemcmp","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestMemcmp", CMD_TestMemcmp, NULL);
	//cmddetail:{"name":"TestChannelSetToExpression","args":"CMD_TestChannelSetToExpression",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestChannelSetToExpression","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestChannelSetToExpression", CMD_TestChannelSetToExpression, NULL);
	//cmddetail:{"name":"TestParseIP","args":"CMD_TestParseIP",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestParseIP","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestParseIP", CMD_TestParseIP, NULL);
	//cmddetail:{"name":"TestIPtoStr","args":"CMD_TestIPtoStr",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_TestIPtoStr","file":"cmnds/cmd_test.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TestIPtoStr", CMD_TestIPtoStr, NULL);
	
    return 0;
}

#endif // ENABLE_TEST_COMMANDS
