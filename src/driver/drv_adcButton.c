#include "../new_common.h"
#include "../new_pins.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "drv_local.h"
#include "drv_public.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_adc.h"
#include "../mqtt/new_mqtt.h"

// Like: 450 1250 2900
static int *g_ranges = 0;
static int g_numRanges = 0;
static int g_prevButton = -1;
static int g_timeAccum = 0;

static int chooseButton(int value) {
	int i;

	for (i = 0; i < g_numRanges; i++) {
		if (g_ranges[i] > value) {
			return i;
		}
	}
	return g_numRanges;
}
commandResult_t Cmd_ADCButtonMap(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int cnt, i;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	if (g_ranges)
		free(g_ranges);
	cnt = Tokenizer_GetArgsCount();
	g_ranges = (int*)malloc(sizeof(int)*cnt);
	for (i = 0; i < cnt; i++) {
		g_ranges[i] = Tokenizer_GetArgInteger(i);
	}
	g_numRanges = cnt;

	return CMD_RES_OK;
}

void DRV_ADCButton_Init() {


	//cmddetail:{"name":"AB_Map","args":"[int]",
	//cmddetail:"descr":"Sets margines for ADC button codes. For given N margins, there are N+1 possible ADC button values (one should be reserved for 'no button')",
	//cmddetail:"fn":"Cmd_ADCButtonMap","file":"drv/drv_adcButton.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("AB_Map", Cmd_ADCButtonMap, NULL);
}

void DRV_ADCButtonDoMeasurement() {
	int newButton;
	int adcValue;
	int adcPin;

	adcPin = PIN_FindPinIndexForRole(IOR_ADC_Button, -1);

	if (adcPin == -1) {
		return;
	}
	adcValue = HAL_ADC_Read(adcPin);

	newButton = chooseButton(adcValue);

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "ADC %i -> button %i (total %i)\r\n", adcValue, newButton, g_numRanges);

	if (newButton != g_prevButton) {
		EventHandlers_FireEvent(CMD_EVENT_ADC_BUTTON, newButton);
		g_prevButton = newButton;
	}
}
void DRV_ADCButton_RunFrame() {
	g_timeAccum += g_deltaTimeMS;
	if (g_timeAccum > 100) {
		DRV_ADCButtonDoMeasurement();
		g_timeAccum = 0;
	}
}

