
#include "../new_common.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"

void TuyaMCUTmSensor_AliasCommand(const char* oldName, const char* newName) {
    command_t *oldCmd = CMD_Find(oldName);
    if (!oldCmd) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "%s command not found!\n", oldName);
        return;
    }
    command_t *newCmd = CMD_Find(newName);
    if (!newCmd) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "%s command not found!\n", newName);
        return;
    }
	oldCmd->handler = newCmd->handler;
	oldCmd->context = newCmd->context;
}

void TuyaMCUTmSensor_ReplaceCommand(const char* name, commandHandler_t handler) {
    command_t *cmd = CMD_Find(name);
    if (!cmd) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "%s command not found!\n", name);
        return;
    }
	cmd->handler = handler;
}

commandResult_t TuyaMCUTmSensor_UnsupportedCommand(const void* context, const char* cmd, const char* args, int cmdFlags) {
    addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "This command isn't supported by low-power TuyaMCU\n");

    return CMD_RES_ERROR;
}

void TuyaMCUTmSensor_Init() {
	CMD_ExecuteCommand("stopDriver TuyaMCU", 0);
	CMD_ExecuteCommand("startDriver TuyaMCULE", 0);

	TuyaMCUTmSensor_AliasCommand("tuyaMcu_sendCurTime", "tuyaMcuLE_sendCurTime");
	TuyaMCUTmSensor_AliasCommand("linkTuyaMCUOutputToChannel", "tuyaMCULE_SetDpConfig");
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_setDimmerRange", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_sendHeartbeat", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_sendQueryState", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_AliasCommand("tuyaMcu_sendProductInformation", "tuyaMcuLE_getProductInformation");
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_sendState", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_sendMCUConf", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_AliasCommand("tuyaMcu_sendCmd", "tuyaMcuLE_sendCmd");
	TuyaMCUTmSensor_AliasCommand("tuyaMcu_setBaudRate", "tuyaMcuLE_setBaudRate");
	TuyaMCUTmSensor_AliasCommand("tuyaMcu_sendRSSI", "tuyaMcuLE_sendRSSI");
	TuyaMCUTmSensor_AliasCommand("tuyaMcu_defWiFiState", "tuyaMcuLE_defWiFiState");
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_sendColor", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_setupLED", TuyaMCUTmSensor_UnsupportedCommand);
	TuyaMCUTmSensor_ReplaceCommand("tuyaMcu_enableAutoSend", TuyaMCUTmSensor_UnsupportedCommand);
}

void TuyaMCUTmSensor_Shutdown() {

}