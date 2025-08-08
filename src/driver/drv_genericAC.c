
#include "../obk_config.h"


#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "drv_uart.h"
#include "../httpserver/hass.h"

static commandResult_t CMD_ACMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);


	return CMD_RES_OK;
}
static commandResult_t CMD_FANMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	return CMD_RES_OK;
}

static commandResult_t CMD_SwingH(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	return CMD_RES_OK;
}
static commandResult_t CMD_TargetTemperature(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float target;

	Tokenizer_TokenizeString(args, 0);

	return CMD_RES_OK;
}
static commandResult_t CMD_SwingV(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	return CMD_RES_OK;
}
void GenericAC_Init(void) {
	//cmddetail:{"name":"ACMode","args":"CMD_ACMode",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ACMode", CMD_ACMode, NULL);
	//cmddetail:{"name":"FANMode","args":"CMD_FANMode",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("FANMode", CMD_FANMode, NULL);
	//cmddetail:{"name":"SwingH","args":"CMD_SwingH",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SwingH", CMD_SwingH, NULL);
	//cmddetail:{"name":"SwingV","args":"CMD_SwingV",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SwingV", CMD_SwingV, NULL);
	//cmddetail:{"name":"TargetTemperature","args":"CMD_TargetTemperature",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TargetTemperature", CMD_TargetTemperature, NULL);
	//cmddetail:{"name":"Buzzer","args":"CMD_Buzzer",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_tclAC.c","requires":"",
	//cmddetail:"examples":""}
}
int CurrentTemperature;
int TargetTemperature;
int ACMode;
int FANMode;
int SwingH;
int SwingV;

void publishSelf() {
	if (-1 != CurrentTemperature)
	{
		MQTT_PublishMain_StringInt("CurrentTemperature", CHANNEL_Get(CurrentTemperature), 0);
	}
	if (-1 != TargetTemperature)
	{
		MQTT_PublishMain_StringInt("TargetTemperature", CHANNEL_Get(TargetTemperature), 0);
	}
	if (-1 != ACMode)
	{
		const char *s = "";
		MQTT_PublishMain_StringString("ACMode", s, 0);
	}
	if (-1 != FANMode)
	{
		const char *s = "";
		MQTT_PublishMain_StringString("FANMode", s, 0);
	}
	if (-1 != SwingH)
	{
		const char *s = "";
		MQTT_PublishMain_StringString("SwingH", s, 0);
	}
	if (-1 != SwingV)
	{
		const char *s = "";
		MQTT_PublishMain_StringString("SwingV", s, 0);
	}
}
void GenericAC_OnChannelChange(int ch, int value) {
	//if (ch == CurrentTemperature)
	//{
	//	MQTT_PublishMain_StringInt("CurrentTemperature", (int)value, 0);
	//}
	//if (ch == TargetTemperature)
	//{
	//	MQTT_PublishMain_StringInt("TargetTemperature", (int)value, 0);
	//}
	//if (ch == ACMode)
	//{
	//	MQTT_PublishMain_StringString("ACMode", value, 0);
	//}
	//if (ch == FANMode)
	//{
	//	MQTT_PublishMain_StringString("FANMode", fanModeToStr(value), 0);
	//}
	//if (ch == SwingH)
	//{
	//	MQTT_PublishMain_StringString("SwingH", getSwingHLabel(value), 0);
	//}
	//if (ch == SwingV)
	//{
	//	MQTT_PublishMain_StringString("SwingV", getSwingHLabel(value), 0);
	//}
}
void GenericAC_DoDiscovery(const char *topic) {

	HassDeviceInfo* dev_info;
	const char *fanOptions[] = {"q"};
	const char *vertical_swing_options[] = { "q" };
	const char *horizontal_swing_options[] = { "q" };

	dev_info = hass_createHVAC(15, 30, 0.5f, fanOptions, sizeof(fanOptions) / sizeof(fanOptions[0]),
		vertical_swing_options, sizeof(vertical_swing_options) / sizeof(vertical_swing_options[0]),
		horizontal_swing_options, sizeof(horizontal_swing_options) / sizeof(horizontal_swing_options[0])
	);
	MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);



	//float min = 15;
	//float max = 30;
	//float step = 0.5f;

	//int channelCurTemp = 0;
	//int channelTargetTemp = 0;
	//int channelMode = 0;

	//HassDeviceInfo* info = hass_init_device_info(HASS_HVAC, 0, NULL, NULL, 0, 0);

	//// Set the name for the HVAC device
	//cJSON_AddStringToObject(info->root, "name", "Smart Thermostat");

	//// Set temperature unit
	//cJSON_AddStringToObject(info->root, "temperature_unit", "C");

	//// Set temperature topics
	//cJSON_AddStringToObject(info->root, "current_temperature_topic", formatGetter(channelCurTemp));
	//cJSON_AddStringToObject(info->root, "temperature_command_topic", formatSetter(channelTargetTemp));
	//cJSON_AddStringToObject(info->root, "temperature_state_topic", formatGetter(channelTargetTemp));

	//// Set temperature range and step
	//cJSON_AddNumberToObject(info->root, "min_temp", min);
	//cJSON_AddNumberToObject(info->root, "max_temp", max);
	//cJSON_AddNumberToObject(info->root, "temp_step", step);

	//// Set mode topics
	//cJSON_AddStringToObject(info->root, "mode_state_topic", formatGetter(channelMode));
	//cJSON_AddStringToObject(info->root, "mode_command_topic", formatSetter(channelMode));

	//// Add supported modes
	//cJSON* modes = cJSON_CreateArray();
	//cJSON_AddItemToArray(modes, cJSON_CreateString("off"));
	//cJSON_AddItemToArray(modes, cJSON_CreateString("heat"));
	//cJSON_AddItemToArray(modes, cJSON_CreateString("cool"));
	//// fan does not work, it has to be fan_only
	//cJSON_AddItemToArray(modes, cJSON_CreateString("fan_only"));
	//cJSON_AddItemToObject(info->root, "modes", modes);

	//const char *fanOptions = 0;
	//int numFanOptions = 0;
	//if (fanOptions && numFanOptions) {
	//	// Add fan mode topics
	//	cJSON_AddStringToObject(info->root, "fan_mode_state_topic", "~/FanMode/get");
	//	sprintf(g_acBuffer, "cmnd/%s/FanMode", CFG_GetMQTTClientId());
	//	cJSON_AddStringToObject(info->root, "fan_mode_command_topic", g_acBuffer);

	//	// Add supported fan modes
	//	cJSON* fan_modes = cJSON_CreateArray();
	//	for (int i = 0; i < numFanOptions; i++) {
	//		const char *mode = fanOptions[i];
	//		cJSON_AddItemToArray(fan_modes, cJSON_CreateString(mode));
	//	}
	//	cJSON_AddItemToObject(info->root, "fan_modes", fan_modes);
	//}
	//const char *swingHOptions = 0;
	//int numSwingHOptions = 0;
	//if (numSwingHOptions) {
	//	// Add Swing Horizontal
	//	cJSON_AddStringToObject(info->root, "swing_horizontal_mode_state_topic", "~/SwingH/get");
	//	sprintf(g_acBuffer, "cmnd/%s/SwingH", CFG_GetMQTTClientId());
	//	cJSON_AddStringToObject(info->root, "swing_horizontal_mode_command_topic", g_acBuffer);

	//	cJSON* swingH_modes = cJSON_CreateArray();
	//	for (int i = 0; i < numSwingHOptions; i++) {
	//		const char *mode = swingHOptions[i];
	//		cJSON_AddItemToArray(swingH_modes, cJSON_CreateString(mode));
	//	}
	//	cJSON_AddItemToObject(info->root, "swing_horizontal_modes", swingH_modes);
	//}
	//const char *swingOptions = 0;
	//int numSwingOptions = 0;
	//if (numSwingOptions) {
	//	// Add Swing Vertical
	//	cJSON_AddStringToObject(info->root, "swing_mode_state_topic", "~/SwingV/get");
	//	sprintf(g_acBuffer, "cmnd/%s/SwingV", CFG_GetMQTTClientId());
	//	cJSON_AddStringToObject(info->root, "swing_mode_command_topic", g_acBuffer);

	//	cJSON* swing_modes = cJSON_CreateArray();
	//	for (int i = 0; i < numSwingOptions; i++) {
	//		const char *mode = swingOptions[i];
	//		cJSON_AddItemToArray(swing_modes, cJSON_CreateString(mode));
	//	}
	//	cJSON_AddItemToObject(info->root, "swing_modes", swing_modes);

	//}
	//// Set availability topic
	//cJSON_AddStringToObject(info->root, "availability_topic", "~/status");
	//cJSON_AddStringToObject(info->root, "payload_available", "online");
	//cJSON_AddStringToObject(info->root, "payload_not_available", "offline");

	//// Update device configuration channel for HVAC
	//sprintf(info->channel, "climate/%s/config", info->unique_id);

	//// Update device info
	//cJSON* dev = info->device;
	//cJSON_ReplaceItemInObject(dev, "manufacturer", cJSON_CreateString("Custom"));
	//cJSON_ReplaceItemInObject(dev, "model", cJSON_CreateString("C-Thermo"));

	//MQTT_QueuePublish(topic, info->channel, hass_build_discovery_json(info), OBK_PUBLISH_FLAG_RETAIN);
	//hass_free_device_info(info);
}