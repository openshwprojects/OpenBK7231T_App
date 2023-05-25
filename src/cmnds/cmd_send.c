#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../httpclient/http_client.h"
#include "cmd_local.h"

// SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
// addRepeatingEvent 5 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
// addEventHandler OnClick 8 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
static commandResult_t CMD_SendGET(const void *context, const char *cmd, const char *args, int cmdFlags){
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET received with args %s",args);

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	HTTPClient_Async_SendGet(args);
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET not supported!");

#endif
	return CMD_RES_OK;
}
// SendPOST http://localhost:3000/ 3000 "application/json" "{ \"a\":123, \"b\":77 }"
static commandResult_t CMD_SendPOST(const void *context, const char *cmd, const char *args, int cmdFlags) {
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendPOST received with args %s", args);

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS);

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	HTTPClient_Async_SendPost(Tokenizer_GetArg(0),
		Tokenizer_GetArgIntegerDefault(1,80),
		Tokenizer_GetArg(2),
		Tokenizer_GetArg(3),
		Tokenizer_GetArg(4));
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendPOST not supported!");

#endif
	return CMD_RES_OK;
}
static commandResult_t CMD_TestPOST(const void *context, const char *cmd, const char *args, int cmdFlags) {

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	HTTPClient_Async_SendPost("http://localhost:3000/",
		3000,
		"application/json",
		"{ \"a\":123, \"b\":77 }",
		0);
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendPOST not supported!");

#endif
	return CMD_RES_OK;
}


int CMD_InitSendCommands(){
	//cmddetail:{"name":"sendGet","args":"[TargetURL]",
	//cmddetail:"descr":"Sends a HTTP GET request to target URL. May include GET arguments. Can be used to control devices by Tasmota HTTP protocol. Command supports argument expansion, so $CH11 changes to value of channel 11, etc, etc.",
	//cmddetail:"fn":"CMD_SendGET","file":"cmnds/cmd_send.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("sendGet", CMD_SendGET, NULL);

	//cmddetail:{"name":"sendPost","args":"[TargetURL]",
	//cmddetail:"descr":"Sends a HTTP POST request to target URL. TODO",
	//cmddetail:"fn":"CMD_SendPOST","file":"cmnds/cmd_send.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("sendPOST", CMD_SendPOST, NULL);


	//CMD_RegisterCommand("testPost", CMD_TestPOST, NULL);
    return 0;
}
