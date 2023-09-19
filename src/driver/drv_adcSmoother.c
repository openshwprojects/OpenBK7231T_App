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

// smoothinng
static int *g_samples = NULL;
static int g_samplesCount = 0;
static int g_nextSample = 0;
static int g_sampleIntervalMS = 100;
static int g_timeAccum = 0;
// generic
static int g_adcPin = 0;
static int g_margin = 0;
static int g_smoothed = -1;
static int g_lh = -1;
// targets
static int g_channel_smoothed = 0;
static int g_channel_lh = 0;

void ADCSmoother_SetupWindow(int size) {
	g_samples = realloc(g_samples, sizeof(int) * size);
	memset(g_samples,0, sizeof(int) * size);
	g_samplesCount = size;
}
void ADCSmoother_AppendSample(int val) {
	if (g_samples == 0)
		return;
	g_samples[g_nextSample] = val;
	g_nextSample++;
	g_nextSample %= g_samplesCount;
}
float ADCSmoother_Sample() {
	float s = 0;
	for (int i = 0; i < g_samplesCount; i++) {
		s += g_samples[i];
	}
	s /= g_samplesCount;
	return s;
}

// ADCSmoother [Pindex] [TotalSamples] [SampleIntervalMS] [TargetChannelADCValue] [MarginValue] [TargetChannel0or1]
// TargetChannelADCValue is a channel which will get smoother value, like 640, etc
// MarginValue is a value that used to tell which smoothed adc values are considered high and which are low
// TargetChannel0or1 will be set depending on MarginValue to either 0 or 1
// something like:
// StartDriver ADCSmoother
// ADCSmoother 27 10 50 10 2048 11
// 
commandResult_t Cmd_SetupADCSmoother(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 6))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_adcPin = Tokenizer_GetArgInteger(0);
	int size = Tokenizer_GetArgInteger(1);
	ADCSmoother_SetupWindow(size);
	g_sampleIntervalMS = Tokenizer_GetArgInteger(2);
	g_channel_smoothed = Tokenizer_GetArgInteger(3);
	g_margin = Tokenizer_GetArgInteger(4);
	g_channel_lh = Tokenizer_GetArgInteger(5);

	HAL_ADC_Init(g_adcPin);

	return CMD_RES_OK;
}

void DRV_ADCSmoother_Init() {

	//cmddetail:{"name":"ADCSmoother","args":"[Pindex] [TotalSamples] [SampleIntervalMS] [TargetChannelADCValue] [MarginValue] [TargetChannel0or1]",
	//cmddetail:"descr":"Starts the ADC smoother with given configuration",
	//cmddetail:"fn":"NULL);","file":"driver/drv_adcSmoother.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ADCSmoother", Cmd_SetupADCSmoother, NULL);
}
void DRV_ADCSmootherDoSmooth() {
	int raw = HAL_ADC_Read(g_adcPin);
	ADCSmoother_AppendSample(raw);
	int smoothed = ADCSmoother_Sample();
	int lowHigh = smoothed > g_margin;
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "AS: raw %i, smoothed %i, LowHigh %i\n", raw, smoothed, lowHigh);
	
	if (smoothed != g_smoothed) {
		g_smoothed = smoothed;
		CHANNEL_Set(g_channel_smoothed, g_smoothed, 0);
	}
	if (lowHigh != g_lh) {
		g_lh = lowHigh;
		CHANNEL_Set(g_channel_lh, g_lh, 0);
	}
}

void DRV_ADCSmoother_RunFrame() {
	g_timeAccum += g_deltaTimeMS;
	if (g_timeAccum > g_sampleIntervalMS) {
		DRV_ADCSmootherDoSmooth();
		g_timeAccum = 0;
	}
}

