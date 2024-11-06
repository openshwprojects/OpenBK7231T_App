
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#if ENABLE_LITTLEFS
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



static commandResult_t cmnd_backlog(const void * context, const char *cmd, const char *args, int cmdFlags){
	const char *subcmd;
	const char *p;
	int count = 0;
    char copy[128];
    char *c;
	int localRes;
	int res = CMD_RES_OK;
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
            if (c - copy < (sizeof(copy)-1)){
                c++;
            }
		}
		*c = 0;
		count++;
		localRes = CMD_ExecuteCommand(copy, cmdFlags);
		if (localRes != CMD_RES_OK && localRes != CMD_RES_EMPTY_STRING) {
			res = localRes;
		}
		subcmd = p;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog executed %d", count);

	return res;
}

// Our wrapper for LFS.
// Returns a buffer created with malloc.
// You must free it later.
byte *LFS_ReadFile(const char *fname) {
#if ENABLE_LITTLEFS
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
#if WINDOWS
		// sstop sim spam
#else
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS_ReadFile: lfs is absent");
#endif
	}
#endif
	return 0;
}
int LFS_WriteFile(const char *fname, const byte *data, int len, bool bAppend) {
#if ENABLE_LITTLEFS
	init_lfs(1);
	if (lfs_present()) {
		lfs_file_t file;
		int lfsres;

		memset(&file, 0, sizeof(lfs_file_t));
		if (bAppend) {
			lfsres = lfs_file_open(&lfs, &file, fname, LFS_O_APPEND | LFS_O_WRONLY);
		}
		else {
			lfs_remove(&lfs, fname);
			lfsres = lfs_file_open(&lfs, &file, fname, LFS_O_CREAT | LFS_O_WRONLY);
		}

		if (lfsres >= 0) {
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: openned file %s", fname);

			lfsres = lfs_file_write(&lfs, &file, data, len);
			lfs_file_close(&lfs, &file);
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: closed file %s", fname);
			return lfsres;
		}
		else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: failed to file %s", fname);
		}
	}
	else {
#if WINDOWS
		// sstop sim spam
#else
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS_WriteFile: lfs is absent");
#endif
	}
#endif
	return 1;
}

static commandResult_t cmnd_lfsexec(const void * context, const char *cmd, const char *args, int cmdFlags){
#if ENABLE_LITTLEFS
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
							if (!(line[0] == '/' && line[1] == '/')) {
								CMD_ExecuteCommand(line, cmdFlags);
							}
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

static commandResult_t cmnd_SSID1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetWiFiSSID(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_Password1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetWiFiPass(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttHost(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTHost(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttUser(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTUserName(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttClient(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTClientId(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_stub(const void * context, const char *cmd, const char *args, int cmdFlags) {

	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttPassword(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTPass(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
int taslike_commands_init(){
	//cmddetail:{"name":"power","args":"[OnorOfforToggle]",
	//cmddetail:"descr":"Tasmota-style POWER command. Should work for both LEDs and relay-based devices. You can write POWER0, POWER1, etc to access specific relays.",
	//cmddetail:"fn":"power","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("power", power, NULL);
	//cmddetail:{"name":"powerAll","args":"",
	//cmddetail:"descr":"set all outputs",
	//cmddetail:"fn":"powerAll","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("powerAll", powerAll, NULL);
	//cmddetail:{"name":"backlog","args":"[string of commands separated with ;]",
	//cmddetail:"descr":"run a sequence of ; separated commands",
	//cmddetail:"fn":"cmnd_backlog","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("backlog", cmnd_backlog, NULL);
	//cmddetail:{"name":"exec","args":"[Filename]",
	//cmddetail:"descr":"exec <file> - run autoexec.bat or other file from LFS if present",
	//cmddetail:"fn":"cmnd_lfsexec","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("exec", cmnd_lfsexec, NULL);
	//cmddetail:{"name":"SSID1","args":"[ValueString]",
	//cmddetail:"descr":"Sets the SSID of target WiFi. Command keeps Tasmota syntax.",
	//cmddetail:"fn":"cmnd_SSID1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SSID1", cmnd_SSID1, NULL);
	//cmddetail:{"name":"Password1","args":"[ValueString]",
	//cmddetail:"descr":"Sets the Pass of target WiFi. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_Password1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Password1", cmnd_Password1, NULL);
	//cmddetail:{"name":"MqttHost","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT host. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttHost","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttHost", cmnd_MqttHost, NULL);
	//cmddetail:{"name":"MqttUser","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT user. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttUser","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttUser", cmnd_MqttUser, NULL);
	//cmddetail:{"name":"MqttPassword","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT pass. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttPassword","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttPassword", cmnd_MqttPassword, NULL);
	//cmddetail:{"name":"MqttClient","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT client. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttClient","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttClient", cmnd_MqttClient, NULL);

	// those are stubs, they are handled elsewhere so we can have Tasmota style replies

	//cmddetail:{"name":"State","args":"NULL",
	//cmddetail:"descr":"A stub for Tasmota",
	//cmddetail:"fn":"cmnd_stub","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("State", cmnd_stub, NULL);
	//cmddetail:{"name":"Sensor","args":"NULL",
	//cmddetail:"descr":"A stub for Tasmota",
	//cmddetail:"fn":"cmnd_stub","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Sensor", cmnd_stub, NULL);
	//cmddetail:{"name":"Status","args":"NULL",
	//cmddetail:"descr":"A stub for Tasmota",
	//cmddetail:"fn":"cmnd_stub","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Status", cmnd_stub, NULL);
	//cmddetail:{"name":"Result","args":"NULL",
	//cmddetail:"descr":"A stub for Tasmota",
	//cmddetail:"fn":"cmnd_stub","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Result", cmnd_stub, NULL);
    return 0;
}
