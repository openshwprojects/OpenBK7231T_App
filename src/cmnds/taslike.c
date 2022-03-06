
#include "../logging/logging.h"
#include "../new_cmd.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

static int power(const void *context, const char *cmd, const char *args){
	if (!wal_strnicmp(cmd, "POWER", 5)){
		int channel = 0;
		int iVal = 0;
		if (strlen(cmd) > 5) {
			channel = atoi(cmd+5);
		}
		iVal = atoi(args);
		CHANNEL_Set(channel, iVal, false);
		return 1;
	}
	return 0;
}

static int color(const void *context, const char *cmd, const char *args){
    if (!wal_strnicmp(cmd, "COLOR", 5)){
        if (args[0] != '#'){
            ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd COLOR expected a # prefixed color");
            return 0;
        } else {
            char *c = args;
            int val = 0;
            int channel = 0;
            c++;
            while (*c){
                char tmp[3];
                int r;
                tmp[0] = *(c++);
                if (!*c) break;
                tmp[1] = *(c++);
                r = sscanf(tmp, "%x", &val);
                if (!r) break;
                // if this channel is not PWM, find a PWM channel;
                while ((channel < 32) && (IOR_PWM != CHANNEL_GetRoleForChannel(channel))) {
                    channel ++;
                }

                if (channel >= 32) break;

                val = (val * 100)/255;
                CHANNEL_Set(channel, val, 0);
                // move to next channel.
                channel ++;
            }
        }
        return 1;
    }
    return 0;
}

static int cmnd_backlog(const void * context, const char *cmd, const char *args){
	char *subcmd;
	char *p;
	int count = 0;
    char copy[128];
    char *c;
	if (stricmp(cmd, "backlog")){
		return -1;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog [%s]", args);

	subcmd = args;
	p = args;
	while (*subcmd){
        c = copy;
		while (*p){
			if (*p == ';'){
				*c = '\0';
				p++;
				break;
			}
            *(c) = *(p++);
            if (c - copy < 127){
                c++;
            }
		}
		count++;
		CMD_ExecuteCommand(copy);
		subcmd = p;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog executed %d", count);

	return 1;
}


static int cmnd_lfsexec(const void * context, const char *cmd, const char *args){
#ifdef BK_LITTLEFS
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "exec %s", args);
	if (lfs_present()){
		lfs_file_t *file = os_malloc(sizeof(lfs_file_t));
		if (file){
			int lfsres;
			char line[256];
			char *fname = "autoexec.bat";
		    memset(file, 0, sizeof(lfs_file_t));
			if (args && *args){
				fname = args;
			}
			lfsres = lfs_file_open(&lfs, file, fname, LFS_O_RDONLY);
			if (lfsres >= 0) {
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "openned file %s", fname);
				do {
					char *p = line;
					do {
						lfsres = lfs_file_read(&lfs, file, p, 1);
						if ((lfsres <= 0) || (*p < 0x20) || (p - line) == 255){
							*p = 0;
							break;
						}
						p++;
					} while ((p - line) < 255);
					ADDLOG_DEBUG(LOG_FEATURE_CMD, "line is %s", line);

					if (lfsres >= 0){
						if (*line && (*line != '#')){
							CMD_ExecuteCommand(line);
						}
					}
				} while (lfsres > 0);

				lfs_file_close(&lfs, file);
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "closed file %s", fname);
			} else {
				ADDLOG_ERROR(LOG_FEATURE_CMD, "no file %s err %d", fname, lfsres);
			}
			os_free(file);
			file = NULL;
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "lfs is absent");
	}
#endif
	return 1;
}


int taslike_commands_init(){
    CMD_RegisterCommand("power", "", power, "set output POWERn 0..100", NULL);
    CMD_RegisterCommand("color", "", color, "set PWN color using #RRGGBB[cw][ww]", NULL);
	CMD_RegisterCommand("backlog", "", cmnd_backlog, "run a sequence of ; separated commands", NULL);
	CMD_RegisterCommand("exec", "", cmnd_lfsexec, "exec <file> - run autoexec.bat or other file from LFS if present", NULL);
}