#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"

#if WINDOWS

#include <ctype.h>
#include "cmd_local.h"

// exits Windows application
static commandResult_t CMD_ExitSimulator(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	exit(0);

	return CMD_RES_OK;
}


void CMD_InitSimulatorOnlyCommands() {
	//cmddetail:{"name":"ExitSimulator","args":"",
	//cmddetail:"descr":"[SIMULATOR ONLY] Exits the application instance",
	//cmddetail:"fn":"CMD_ExitSimulator","file":"cmnds/cmd_simulatorOnly.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ExitSimulator", CMD_ExitSimulator, NULL);
}

#endif


