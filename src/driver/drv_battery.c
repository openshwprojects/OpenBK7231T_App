#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_adc.h"
#include "drv_battery.h"

static int g_pin_adc = -1, g_pin_rel = -1, g_val_rel = -1, g_battcycle = 1, g_battcycleref = 10;
static float g_battvoltage = 0.0, g_battlevel = 0.0;
static int g_lastbattvoltage = 0, g_lastbattlevel = 0;
static float g_vref = 2400, g_vdivider = 2.29, g_maxbatt = 3000, g_minbatt = 2000, g_adcbits = 4096;

static int Batt_Load() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		switch (g_cfg.pins.roles[i])
		{
		case IOR_BAT_ADC:
			g_pin_adc = i;
			break;
		case IOR_BAT_Relay:
			g_pin_rel = i;
			g_val_rel = 1;
			break;
		case IOR_BAT_Relay_n:
			g_pin_rel = i;
			g_val_rel = 0;
			break;
		default:
			break;
		}
	}
	return g_pin_adc;
}

static void Batt_Measure() {
	//this command has only been tested on CBU
	float batt_ref, batt_res, vref;
	if (g_pin_adc == -1) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "DRV_BATTERY : ADC pin not registered, reset device");
		return;
	}

	// if relay pin is registered use relay
	if (g_pin_rel != -1) {
		HAL_PIN_SetOutputValue(g_pin_rel, g_val_rel);
		rtos_delay_milliseconds(10);
	}
	g_battvoltage = HAL_ADC_Read(g_pin_adc);
	ADDLOG_INFO(LOG_FEATURE_DRV, "DRV_BATTERY : ADC binary Measurement : %f", g_battvoltage);
	if (g_pin_rel != -1) {
		HAL_PIN_SetOutputValue(g_pin_rel, !g_val_rel);
	}
	ADDLOG_DEBUG(LOG_FEATURE_DRV, "DRV_BATTERY : Calculation with param : %f %f %f", g_vref, g_adcbits, g_vdivider);
	// batt_value = batt_value / vref / 12bits value should be 10 un doc ... but on CBU is 12 ....
	vref = g_vref / g_adcbits;
	g_battvoltage = g_battvoltage * vref;
	// multiply by 2 cause ADC is measured after the Voltage Divider
	g_battvoltage = g_battvoltage * g_vdivider;

	// ignore values less then half the minimum but don't quit trying
	if ((g_minbatt / 2) > g_battvoltage) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "DRV_BATTERY : Reading invalid, ignoring will try again");
		++g_battcycle;
		return;
	}

	batt_ref = g_maxbatt - g_minbatt;
	batt_res = g_battvoltage - g_minbatt;
	ADDLOG_DEBUG(LOG_FEATURE_DRV, "DRV_BATTERY : Ref battery: %f, rest battery %f", batt_ref, batt_res);
	g_battlevel = (batt_res / batt_ref) * 100;
	if (g_battlevel < 0)
		g_battlevel = 0;
	if (g_battlevel > 100)
		g_battlevel = 100;

#if ENABLE_MQTT
	if (MQTT_IsReady()) {
		MQTT_PublishMain_StringInt("voltage", (int)g_battvoltage, 0);
		MQTT_PublishMain_StringInt("battery", (int)g_battlevel, 0);
	} else 	{
		char sValue[8];   // channel value as a string
		sprintf(sValue, "%i", (int)g_battvoltage);
		MQTT_QueuePublish(CFG_GetMQTTClientId(), "voltage/get", sValue, 0); // queue the publishing
		sprintf(sValue, "%i", (int)g_battlevel);
		MQTT_QueuePublish(CFG_GetMQTTClientId(), "battery/get", sValue, 0); // queue the publishing
	}
#endif
	g_lastbattlevel = (int)g_battlevel;
	g_lastbattvoltage = (int)g_battvoltage;
	ADDLOG_INFO(LOG_FEATURE_DRV, "DRV_BATTERY : battery voltage : %f and percentage %f%%", g_battvoltage, g_battlevel);
}
void Simulator_Force_Batt_Measure() {
	Batt_Measure();
}

int Battery_lastreading(int type)
{
	if (type == OBK_BATT_VOLTAGE)
	{
		return g_lastbattvoltage;
	}
	else if (type == OBK_BATT_LEVEL)
	{
		return g_lastbattlevel;
	}
	return 0;
}
commandResult_t Battery_Setup(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_minbatt = Tokenizer_GetArgFloat(0);
	g_maxbatt = Tokenizer_GetArgFloat(1);
	if (Tokenizer_GetArgsCount() > 2) {
		g_vdivider = Tokenizer_GetArgFloat(2);
	}
	if (Tokenizer_GetArgsCount() > 3) {
		g_vref = Tokenizer_GetArgFloat(3);
	}
	if (Tokenizer_GetArgsCount() > 4) {
		g_adcbits = Tokenizer_GetArgFloat(4);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Battery Setup : Min %f Max %f Vref %f adcbits %f vdivider %f", g_minbatt, g_maxbatt, g_vref, g_adcbits, g_vdivider);

	return CMD_RES_OK;
}


commandResult_t Battery_cycle(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_battcycleref = Tokenizer_GetArgFloat(0);

	ADDLOG_INFO(LOG_FEATURE_CMD, "Battery Cycle : Measurement will run every %i seconds", g_battcycleref);

	return CMD_RES_OK;
}


// startDriver Battery
void Batt_Init() {

	//cmddetail:{"name":"Battery_Setup","args":"[minbatt][maxbatt][V_divider][Vref][AD Bits]",
	//cmddetail:"descr":"measure battery based on ADC. <br />req. args: minbatt in mv, maxbatt in mv. <br />optional: V_divider(2), Vref(default 2400), ADC bits(4096)",
	//cmddetail:"fn":"Battery_Setup","file":"driver/drv_battery.c","requires":"",
	//cmddetail:"examples":"Battery_Setup 1500 3000 2 2400 4096"}
	CMD_RegisterCommand("Battery_Setup", Battery_Setup, NULL);

	//cmddetail:{"name":"Battery_cycle","args":"[int]",
	//cmddetail:"descr":"change cycle of measurement by default every 10 seconds",
	//cmddetail:"fn":"Battery_cycle","file":"driver/drv_battery.c","requires":"",
	//cmddetail:"examples":"Battery_cycle 60"}
	CMD_RegisterCommand("Battery_cycle", Battery_cycle, NULL);

	// do a quick and dirty to make the first reading valid
	if ((Batt_Load() != -1) && (g_pin_rel != -1)) {
		HAL_PIN_SetOutputValue(g_pin_rel, g_val_rel);
		HAL_ADC_Read(g_pin_adc);
		HAL_PIN_SetOutputValue(g_pin_rel, !g_val_rel);
	}
}

void Batt_OnEverySecond() {

	// Do nothing if cycle is set to zero and the last cycle is complete
	if ((g_battcycleref == 0) && (g_battcycle == 0)) {
		return;
	}

	ADDLOG_DEBUG(LOG_FEATURE_DRV, "DRV_BATTERY : Measurement will run in %i cycle(s)", g_battcycle - 1);
	if (g_battcycle == 1) {
		// End of the cycle, poll battery
		Batt_Measure();
	}
	if (g_battcycle > 1) {
		// In middle of cycle, reduce counter
		--g_battcycle;
	} else {
		// Cycle changed/ended, start new cycle
		g_battcycle = g_battcycleref;
	}
}


void Batt_StopDriver() {
	g_pin_adc = -1;
	g_pin_rel = -1;
	g_val_rel = -1;
}
void Batt_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (bPreState) {
		return;
	}
	hprintf255(request, "<h2>Battery level=%.2f%%, voltage=%.2fmV</h2>", g_battlevel, g_battvoltage);
}

