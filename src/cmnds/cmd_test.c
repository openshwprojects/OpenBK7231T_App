

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_common.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif



char *cmds[16] = { NULL };
char *names[16] = { NULL };


// run an aliased command
static int runcmd(const void * context, const char *cmd, const char *args){
    char *c = (char *)context;
    char *p = c;

    while (*p && !isWhiteSpace(*p)) {
        p++;
	}
    if (*p) p++;
    return CMD_ExecuteCommand(p);
}

// run an aliased command
static int addcmd(const void * context, const char *cmd, const char *args){
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

int fortest_commands_init(){
    CMD_RegisterCommand("addcmd", "", addcmd, "add a custom command", NULL);
    return 0;
}
