#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../httpclient/http_client.h"
#include "cmd_local.h"

// SendGet http://192.168.0.106/cm?cmnd=Power1%20Toggle
// addRepeatingEvent 5 SendGet http://192.168.0.106/cm?cmnd=Power1%20Toggle
static int CMD_SendGET(const void *context, const char *cmd, const char *args){
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET received with args %s",args);

#if PLATFORM_BEKEN
	HTTPClient_Async_SendGet(args);
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET not supported!");

#endif
	return 1;
}


int CMD_InitSendCommands(){
    CMD_RegisterCommand("sendGet", "", CMD_SendGET, "qq", NULL);

    return 0;
}
