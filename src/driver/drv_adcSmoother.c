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

static int *g_samples = NULL;
static int g_samplesCount = 0;
static int g_nextSample = 0;

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
