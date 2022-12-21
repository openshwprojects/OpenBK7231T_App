
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif



int parsePowerArgument(const char *s) {
	if(!stricmp(s,"ON"))
		return 1;
	if(!stricmp(s,"OFF"))
		return 0;
	return atoi(s);
}

static commandResult_t power(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWER", 5)){
		int channel = 0;
		int iVal = 0;
		int i;
		bool bRelayIndexingStartsWithZero;
		
		bRelayIndexingStartsWithZero  = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n);

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWER (%s) received with args %s",cmd,args);

		if (strlen(cmd) > 5) {
			channel = atoi(cmd+5);

			if (LED_IsLEDRunning()) {
				channel = SPECIAL_CHANNEL_LEDPOWER;
			}
			else {
				if (bRelayIndexingStartsWithZero) {
					channel--;
					if (channel < 0)
						channel = 0;
				}
			}
		} else {
			// if new LED driver active
			if(LED_IsLEDRunning()) {
				channel = SPECIAL_CHANNEL_LEDPOWER;
			} else {
				// find first active channel, because some people index with 0 and some with 1
				for(i = 0; i < CHANNEL_MAX; i++) {
					if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
						channel = i;
						break;
					}
				}
			}
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
		if(args == 0 || *args == 0) {
			// this should only check status
		}
		else if (!stricmp(args, "STATUS")) {
			// this should only check status
		} else if(!stricmp(args,"TOGGLE")) {
			CHANNEL_Toggle(channel);
		} else {
			iVal = parsePowerArgument(args);
			CHANNEL_Set(channel, iVal, false);
		}
		return CMD_RES_OK;
	//}
	//return 0;
}

static commandResult_t powerAll(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWERALL (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		CHANNEL_SetAll(iVal, false);
		return CMD_RES_OK;
	//}
	//return 0;
}


static commandResult_t powerStateOnly(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd powerStateOnly (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		CHANNEL_SetStateOnly(iVal);
		return CMD_RES_OK;
	//}
	//return 0;
}

static commandResult_t color(const void *context, const char *cmd, const char *args, int cmdFlags){
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
                while ((channel < 32) && (IOR_PWM != CHANNEL_GetRoleForOutputChannel(channel) && IOR_PWM_n != CHANNEL_GetRoleForOutputChannel(channel))) {
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
        return CMD_RES_OK;
  //  }
   // return 0;
}


static commandResult_t cmnd_backlog(const void * context, const char *cmd, const char *args, int cmdFlags){
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

	return CMD_RES_OK;
}

// Our wrapper for LFS.
// Returns a buffer created with malloc.
// You must free it later.
byte *LFS_ReadFile(const char *fname) {
#ifdef BK_LITTLEFS
	if (lfs_present()){
		lfs_file_t file;
		int lfsres;
		int len;
		int cnt;
		byte *res, *at;

		cnt = 0;

		memset(&file, 0, sizeof(lfs_file_t));
		lfsres = lfs_file_open(&lfs, &file, fname, LFS_O_RDONLY);

		if (lfsres >= 0) {
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: openned file %s", fname);
			//lfs_file_seek(&lfs,&file,0,LFS_SEEK_END);
			//len = lfs_file_tell(&lfs,&file);
			//lfs_file_seek(&lfs,&file,0,LFS_SEEK_SET);

			len = lfs_file_size(&lfs,&file);

			lfs_file_seek(&lfs,&file,0,LFS_SEEK_SET);

			res = malloc(len+1);
			at = res;

			if(res == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: openned file %s but malloc failed for %i", fname, len);
			} else {
#if 0
				char buffer[32];
				while(at - res < len) {
					lfsres = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
					if(lfsres <= 0)
						break;
					memcpy(at,buffer,lfsres);

					at += lfsres;
				}
#elif 1
				lfsres = lfs_file_read(&lfs, &file, res, len);
#elif 0
				int ofs;
				for(ofs = 0; ofs < len; ofs++) {
					lfsres = lfs_file_read(&lfs, &file, &res[ofs], 1);
					if(lfsres <= 0)
						break;

				}
#else
				while(at - res < len) {
					lfsres = lfs_file_read(&lfs, &file, at, 1);
					if(lfsres <= 0)
						break;
					at++;
				}
#endif
				res[len] = 0;
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: Loaded %i bytes\n",len);
				//ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: Loaded %s\n",res);
			}
			lfs_file_close(&lfs, &file);
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: closed file %s", fname);
			return res;
		} else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: failed to file %s", fname);
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS_ReadFile: lfs is absent");
	}
#endif
	return 0;
}

static commandResult_t cmnd_lfsexec(const void * context, const char *cmd, const char *args, int cmdFlags){
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
	return CMD_RES_OK;
}


// Usage for continous test: addRepeatingEvent 1 -1 lfs_test1 ir.bat
static commandResult_t cmnd_lfs_test1(const void * context, const char *cmd, const char *args, int cmdFlags){
#ifdef BK_LITTLEFS
	if (lfs_present()){
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
			} while (lfsres > 0) ;
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: Stopped at char %i\n",cnt);

			lfs_file_close(&lfs, &file);
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: closed file %s", args);
		} else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test1: failed to file %s", args);
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "cmnd_lfs_test1: lfs is absent");
	}
#endif
	return CMD_RES_OK;
}
// Usage for continous test: addRepeatingEvent 1 -1 lfs_test2 ir.bat
static commandResult_t cmnd_lfs_test2(const void * context, const char *cmd, const char *args, int cmdFlags){
#ifdef BK_LITTLEFS
	if (lfs_present()){
		lfs_file_t *file;
		int lfsres;
		char a;
		int cnt;

		cnt = 0;

		file = malloc(sizeof(lfs_file_t));
		if(file == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: failed to malloc for %s", args);
		} else {
			memset(file, 0, sizeof(lfs_file_t));
			lfsres = lfs_file_open(&lfs, file, args, LFS_O_RDONLY);

			ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: sizeof(lfs_file_t) %i", sizeof(lfs_file_t));
			if (lfsres >= 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: openned file %s", args);
				do {
					lfsres = lfs_file_read(&lfs, file, &a, 1);
					cnt++;
				} while (lfsres > 0) ;
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: Stopped at char %i\n",cnt);

				lfs_file_close(&lfs, file);
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: closed file %s", args);
			} else {
				ADDLOG_INFO(LOG_FEATURE_CMD, "cmnd_lfs_test2: failed to file %s", args);
			}
			free(file);
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "cmnd_lfs_test2: lfs is absent");
	}
#endif
	return CMD_RES_OK;
}
// Usage for continous test: addRepeatingEvent 1 -1 lfs_test3 ir.bat
static commandResult_t cmnd_lfs_test3(const void * context, const char *cmd, const char *args, int cmdFlags){
	byte *res;

	res = LFS_ReadFile(args);

	if(res) {
		free(res);
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_SSID1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_GetArgsCount()==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetWiFiSSID(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_Password1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetWiFiPass(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttHost(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetMQTTHost(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttUser(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetMQTTUserName(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttClient(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetMQTTClientId(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttPassword(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() == 0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "This command requires 1 argument");
	}
	else {
		CFG_SetMQTTPass(Tokenizer_GetArg(0));
	}
	return CMD_RES_OK;
}
int taslike_commands_init(){
	//cmddetail:{"name":"power","args":"",
	//cmddetail:"descr":"set output POWERn 0..100",
	//cmddetail:"fn":"power","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("power", "", power, NULL, NULL);
	//cmddetail:{"name":"powerStateOnly","args":"",
	//cmddetail:"descr":"ensures that device is on or off without changing pwm values",
	//cmddetail:"fn":"powerStateOnly","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("powerStateOnly", "", powerStateOnly, NULL, NULL);
	//cmddetail:{"name":"powerAll","args":"",
	//cmddetail:"descr":"set all outputs",
	//cmddetail:"fn":"powerAll","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("powerAll", "", powerAll, NULL, NULL);
	//cmddetail:{"name":"color","args":"",
	//cmddetail:"descr":"set PWN color using #RRGGBB[cw][ww]",
	//cmddetail:"fn":"color","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("color", "", color, NULL, NULL);
	//cmddetail:{"name":"backlog","args":"",
	//cmddetail:"descr":"run a sequence of ; separated commands",
	//cmddetail:"fn":"cmnd_backlog","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("backlog", "", cmnd_backlog, NULL, NULL);
	//cmddetail:{"name":"exec","args":"",
	//cmddetail:"descr":"exec <file> - run autoexec.bat or other file from LFS if present",
	//cmddetail:"fn":"cmnd_lfsexec","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("exec", "", cmnd_lfsexec, NULL, NULL);
	//cmddetail:{"name":"lfs_test1","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"cmnd_lfs_test1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test1", NULL, cmnd_lfs_test1, NULL, NULL);
	//cmddetail:{"name":"lfs_test2","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"cmnd_lfs_test2","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test2", NULL, cmnd_lfs_test2, NULL, NULL);
	//cmddetail:{"name":"lfs_test3","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"cmnd_lfs_test3","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_test3", NULL, cmnd_lfs_test3, NULL, NULL);
	//cmddetail:{"name":"SSID1","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_SSID1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SSID1", NULL, cmnd_SSID1, NULL, NULL);
	//cmddetail:{"name":"Password1","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_Password1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Password1", NULL, cmnd_Password1, NULL, NULL);
	//cmddetail:{"name":"MqttHost","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_MqttHost","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttHost", NULL, cmnd_MqttHost, NULL, NULL);
	//cmddetail:{"name":"MqttUser","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_MqttUser","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttUser", NULL, cmnd_MqttUser, NULL, NULL);
	//cmddetail:{"name":"MqttPassword","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_MqttPassword","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttPassword", NULL, cmnd_MqttPassword, NULL, NULL);
	//cmddetail:{"name":"MqttClient","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_MqttClient","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttClient", NULL, cmnd_MqttClient, NULL, NULL);
    return 0;
}
