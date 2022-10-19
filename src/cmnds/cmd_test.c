

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



char *cmds[16] = { NULL };
char *names[16] = { NULL };


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
static int addcmd(const void * context, const char *cmd, const char *args, int cmdFlags){
	if (!wal_strnicmp(cmd, "addcmd", 6)){
		int index = 0;
        char cmd[32];
        //int len;
		if (strlen(cmd) > 6) {
			index = atoi(cmd+6);
		}
        if ((index < 0) || (index > 16)){
            return 0;
        }

        if (cmds[index]){
            os_free(cmds[index]);
        }
        if (names[index]){
            os_free(names[index]);
        }
        cmds[index] = os_malloc(strlen(args)+1);

        strcpy(cmds[index], args);
        //len =
		get_cmd(args, cmd, 32, 1);
        names[index] = os_malloc(strlen(cmd)+1);
        strcpy(names[index], cmd);
		ADDLOG_ERROR(LOG_FEATURE_CMD, "cmd %d set to %s", index, cmd);

        CMD_RegisterCommand(names[index], "", runcmd, "custom", cmds[index]);
		return 1;
	}
	return 0;
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
    CMD_RegisterCommand("addcmd", "", addcmd, "add a custom command", NULL);
    CMD_RegisterCommand("testMallocFree", "", testMallocFree, "", NULL);
    CMD_RegisterCommand("testRealloc", "", testRealloc, "", NULL);
    CMD_RegisterCommand("testJSON", "", testJSON, "", NULL);
    return 0;
}

