
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../new_pins.h"
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif



int parsePowerArgument(const char *s) {
	if(!stricmp(s,"ON"))
		return 255;
	if(!stricmp(s,"OFF"))
		return 0;
	return atoi(s);
}

static int power(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWER", 5)){
		int channel = 0;
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWER (%s) received with args %s",cmd,args);

		if (strlen(cmd) > 5) {
			channel = atoi(cmd+5);
		}
#if 0
		// it seems that my Home Assistant expects RGB etc light bulbs to be turned off entirely
		// with this commands with no arguments, so... no arguments = set all channels?
		else
		{
			CHANNEL_SetAll(iVal, false);
			return 1;
		}
#endif
		if(!stricmp(args,"TOGGLE")) {
			CHANNEL_Toggle(channel);
		} else {
			iVal = parsePowerArgument(args);
			CHANNEL_Set(channel, iVal, false);
		}
		return 1;
	//}
	//return 0;
}

static int powerAll(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWERALL (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		CHANNEL_SetAll(iVal, false);
		return 1;
	//}
	//return 0;
}


static int powerStateOnly(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd powerStateOnly (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		CHANNEL_SetStateOnly(iVal);
		return 1;
	//}
	//return 0;
}

static int color(const void *context, const char *cmd, const char *args, int cmdFlags){
   // if (!wal_strnicmp(cmd, "COLOR", 5)){
        if (args[0] != '#'){
            ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd COLOR expected a # prefixed color, you sent %s",args);
            return 0;
        } else {
            const char *c = args;
            int val = 0;
            int channel = 0;
            ADDLOG_DEBUG(LOG_FEATURE_CMD, "tasCmnd COLOR got %s", args);
            c++;
            while (*c){
                char tmp[3];
                int r;
                int val100;
                tmp[0] = *(c++);
                if (!*c) break;
                tmp[1] = *(c++);
                tmp[2] = '\0';
                r = sscanf(tmp, "%x", &val);
                if (!r) {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "COLOR no sscanf hex result from %s", tmp);
                    break;
                }
                // if this channel is not PWM, find a PWM channel;
                while ((channel < 32) && (IOR_PWM != CHANNEL_GetRoleForOutputChannel(channel))) {
                    channel ++;
                }

                if (channel >= 32) {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "COLOR channel >= 32");
                    break;
                }

                val100 = (val * 100)/255;
            //    ADDLOG_DEBUG(LOG_FEATURE_CMD, "COLOR found chan %d -> val255 %d -> val100 %d (from %s)", channel, val, val100, tmp);
                CHANNEL_Set(channel, val100, 0);
                // move to next channel.
                channel ++;
            }
            if (!(*c)){
              //  ADDLOG_DEBUG(LOG_FEATURE_CMD, "COLOR arg ended");
            }
        }
        return 1;
  //  }
   // return 0;
}


static int cmnd_backlog(const void * context, const char *cmd, const char *args, int cmdFlags){
	const char *subcmd;
	const char *p;
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
				p++;
				break;
			}
            *(c) = *(p++);
            if (c - copy < 127){
                c++;
            }
		}
		*c = 0;
		count++;
		CMD_ExecuteCommand(copy, cmdFlags);
		subcmd = p;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog executed %d", count);

	return 1;
}


static int cmnd_lfsexec(const void * context, const char *cmd, const char *args, int cmdFlags){
#ifdef BK_LITTLEFS
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "exec %s", args);
	if (lfs_present()){
		lfs_file_t *file = os_malloc(sizeof(lfs_file_t));
		if (file){
			int lfsres;
			char line[256];
			const char *fname = "autoexec.bat";
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
							CMD_ExecuteCommand(line, cmdFlags);
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
    CMD_RegisterCommand("powerStateOnly", "", powerStateOnly, "ensures that device is on or off without changing pwm values", NULL);
    CMD_RegisterCommand("powerAll", "", powerAll, "set all outputs", NULL);
    CMD_RegisterCommand("color", "", color, "set PWN color using #RRGGBB[cw][ww]", NULL);
	CMD_RegisterCommand("backlog", "", cmnd_backlog, "run a sequence of ; separated commands", NULL);
	CMD_RegisterCommand("exec", "", cmnd_lfsexec, "exec <file> - run autoexec.bat or other file from LFS if present", NULL);
    return 0;
}
