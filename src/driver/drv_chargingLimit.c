#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

static int chl_targetChannel = -1;
static int chl_targetChannelStartValue = -1;
static int chl_targetChannelMaxDelta = -1;
static int chl_timeLeftSeconds = -1;
static int chl_timeMax = -1;
static char *chl_commandToRun = 0;
static int chl_lastDelteForDisplay = -1;
static bool chl_running = false;

// startDriver ChargingLimit
// chSetupLimit [limitChannelIndex] [maxAllowedLimitChannelDelta] [timeoutOr-1] [commandToRun]
// chSetupLimit 5 5000 3600 "POWER OFF"
// or:
// alias myOff backlog POWER OFF; and smth else; and delse;
// chSetupLimit 5 5000 3600 myOff
commandResult_t ChargingLimit_SetupCommand(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char *newCmd;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	chl_targetChannel = Tokenizer_GetArgInteger(0);
	chl_targetChannelStartValue = CHANNEL_Get(chl_targetChannel);
	chl_targetChannelMaxDelta = Tokenizer_GetArgInteger(1);
	chl_timeMax = chl_timeLeftSeconds = Tokenizer_GetArgInteger(2);
	newCmd = Tokenizer_GetArg(3);
	if (chl_commandToRun != 0 && !strcmp(chl_commandToRun, newCmd)) {
		// no change, avoid free + malloc
	}
	else {
		if (chl_commandToRun)
			free(chl_commandToRun);
		chl_commandToRun = strdup(newCmd);
	}
	chl_running = true;
	return CMD_RES_OK;
}
void ChargingLimit_Init() {

	//cmddetail:{"name":"chSetupLimit","args":"[limitChannelIndex] [maxAllowedLimitChannelDelta] [timeoutOr-1] [commandToRun]",
	//cmddetail:"descr":"After executing this command, chargingLimit driver will watch channel for changes and count down timer. When a timer runs our or channel change (from the initial state) is larger than given margin, given command is run",
	//cmddetail:"fn":"ChargingLimit_SetupCommand);","file":"driver/drv_chargingLimit.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("chSetupLimit", ChargingLimit_SetupCommand, NULL);
}
void ChargingLimit_OnLimitReached(bool bTimedOut) {
	// cast event
	CMD_ExecuteCommand(chl_commandToRun, 0);
	// reset variables
	chl_running = false;
	// do not reset command because we may want to reuse the strdup allocation
}
void ChargingLimit_OnEverySecond() {
	int newValue;

	// if off
	if (chl_running == false)
		return;
	// tick time
	if (chl_timeLeftSeconds != -1) {
		chl_timeLeftSeconds--;
		if (chl_timeLeftSeconds <= 0) {
			ChargingLimit_OnLimitReached(true);
			return;
		}
	}
	// check channel
	newValue = CHANNEL_Get(chl_targetChannel);
	chl_lastDelteForDisplay = newValue - chl_targetChannelStartValue;
	// absolute
	if (chl_lastDelteForDisplay < 0)
		chl_lastDelteForDisplay = -chl_lastDelteForDisplay;
	// has exceed?
	if (chl_lastDelteForDisplay > chl_targetChannelMaxDelta) {
		ChargingLimit_OnLimitReached(false);
	}
}

void ChargingLimit_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	const char *rStr;
	int type;

	if (chl_targetChannel == -1) {
		hprintf255(request, "<h2>Charging limit not started yet</h2>");
		return;
	}
	if (chl_running)
		rStr = "Running";
	else
		rStr = "Finished";

	type = CHANNEL_GetType(chl_targetChannel);

	hprintf255(request, "<h2>Charging limit on channel %i: Status: %s, Time: %i of %i seconds",
			chl_targetChannel, rStr,
		chl_timeLeftSeconds, chl_timeMax);

	if (type == ChType_EnergyTotal_kWh_div100) {
		hprintf255(request, ", Power: %.2f of %.2f kWh</h2>",
			chl_lastDelteForDisplay*0.01f, chl_targetChannelMaxDelta*0.01f);
	}
	else if (type == ChType_EnergyTotal_kWh_div1000) {
		hprintf255(request, ", Power: %.2f of %.2f kWh</h2>",
			chl_lastDelteForDisplay*0.001f, chl_targetChannelMaxDelta*0.001f);
	}
	else {
		hprintf255(request, ", Power: %i of %i </h2>",
			chl_lastDelteForDisplay, chl_targetChannelMaxDelta);
	}


}






