#include "hass.h"
#include "../new_common.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_wifi.h"
#include "../driver/drv_public.h"

/*
Abbreviated node names - https://www.home-assistant.io/docs/mqtt/discovery/
Light - https://www.home-assistant.io/integrations/light.mqtt/
Switch - https://www.home-assistant.io/integrations/switch.mqtt/
Sensor - https://www.home-assistant.io/integrations/sensor.mqtt/
*/

//Buffer used to populate values in cJSON_Add* calls. The values are based on
//CFG_GetShortDeviceName and clientId so it needs to be bigger than them. +64 for light/switch/etc.
static char g_hassBuffer[CGF_MQTT_CLIENT_ID_SIZE + 64];

static char* STATE_TOPIC_KEY = "stat_t";
static char* COMMAND_TOPIC_KEY = "cmd_t";

/// @brief Populates HomeAssistant unique id for the entity.
/// @param type Entity type
/// @param index Entity index (Ignored for RGB)
/// @param uniq_id Array to populate (should be of size HASS_UNIQUE_ID_SIZE)
void hass_populate_unique_id(ENTITY_TYPE type, int index, char* uniq_id) {
	//https://developers.home-assistant.io/docs/entity_registry_index/#unique-id-requirements
	//mentions that mac can be used for unique_id and deviceName contains that.
	const char* longDeviceName = CFG_GetDeviceName();

	switch (type) {
	case ENTITY_LIGHT_PWM:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "light", index);
		break;

	case ENTITY_LIGHT_PWMCW:
	case ENTITY_LIGHT_RGB:
	case ENTITY_LIGHT_RGBCW:
		sprintf(uniq_id, "%s_%s", longDeviceName, "light");
		break;

	case ENTITY_RELAY:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "relay", index);
		break;

	case ENTITY_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "sensor", index);
		break;

	case ENTITY_BINARY_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "binary_sensor", index);
		break;
	}
}

/// @brief Prints HomeAssistant unique id for the entity.
/// @param request
/// @param fmt
/// @param type Entity type
/// @param index Entity index
void hass_print_unique_id(http_request_t* request, const char* fmt, ENTITY_TYPE type, int index) {
	char uniq_id[HASS_UNIQUE_ID_SIZE];
	hass_populate_unique_id(type, index, uniq_id);
	hprintf255(request, fmt, uniq_id);
}

/// @brief Populates HomeAssistant device configuration MQTT channel e.g. switch/enbrighten_9de8f9_relay_0/config.
/// @param type Entity type
/// @param uniq_id Entity unique id
/// @param info Device info
void hass_populate_device_config_channel(ENTITY_TYPE type, char* uniq_id, HassDeviceInfo* info) {
	switch (type) {
	case ENTITY_LIGHT_PWM:
	case ENTITY_LIGHT_PWMCW:
	case ENTITY_LIGHT_RGB:
	case ENTITY_LIGHT_RGBCW:
		sprintf(info->channel, "light/%s/config", uniq_id);
		break;

	case ENTITY_RELAY:
		sprintf(info->channel, "switch/%s/config", uniq_id);
		break;

	case ENTITY_SENSOR:
		sprintf(info->channel, "sensor/%s/config", uniq_id);
		break;

	case ENTITY_BINARY_SENSOR:
		sprintf(info->channel, "binary_sensor/%s/config", uniq_id);
	}
}

/// @brief Builds HomeAssistant device discovery info. The caller needs to free the returned pointer.
/// @param ids 
cJSON* hass_build_device_node(cJSON* ids) {
	cJSON* dev = cJSON_CreateObject();
	cJSON_AddItemToObject(dev, "ids", ids);     //identifiers
	cJSON_AddStringToObject(dev, "name", CFG_GetShortDeviceName());

#ifdef USER_SW_VER
	cJSON_AddStringToObject(dev, "sw", USER_SW_VER);   //sw_version
#endif

	cJSON_AddStringToObject(dev, "mf", MANUFACTURER);   //manufacturer
	cJSON_AddStringToObject(dev, "mdl", PLATFORM_MCU_NAME);  //Using chipset for model

	sprintf(g_hassBuffer, "http://%s/index", HAL_GetMyIPString());
	cJSON_AddStringToObject(dev, "cu", g_hassBuffer);  //configuration_url

	return dev;
}

/// @brief Initializes HomeAssistant device discovery storage with common values.
/// @param type 
/// @param index This is used to generate generate unique_id and name. It is ignored for RGB. For sensor this corresponds to sensor_mqttNames.
/// @param payload_on The payload that represents enabled state. This is not added for ENTITY_SENSOR.
/// @param payload_off The payload that represents disabled state. This is not added for ENTITY_SENSOR.
/// @return 
HassDeviceInfo* hass_init_device_info(ENTITY_TYPE type, int index, char* payload_on, char* payload_off) {
	HassDeviceInfo* info = os_malloc(sizeof(HassDeviceInfo));
	addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "hass_init_device_info=%p", info);

	hass_populate_unique_id(type, index, info->unique_id);
	hass_populate_device_config_channel(type, info->unique_id, info);

	info->ids = cJSON_CreateArray();
	cJSON_AddItemToArray(info->ids, cJSON_CreateString(CFG_GetDeviceName()));

	info->device = hass_build_device_node(info->ids);

	info->root = cJSON_CreateObject();
	cJSON_AddItemToObject(info->root, "dev", info->device);    //device

	//Build the `name`
	switch (type) {
	case ENTITY_LIGHT_PWM:
	case ENTITY_RELAY:
	case ENTITY_BINARY_SENSOR:
		sprintf(g_hassBuffer, "%s %i", CFG_GetShortDeviceName(), index);
		break;
	case ENTITY_LIGHT_PWMCW:
	case ENTITY_LIGHT_RGB:
	case ENTITY_LIGHT_RGBCW:
		//There can only be one RGB so we can skip including index in the name. Do the same
		//for 2 PWM case.
		sprintf(g_hassBuffer, "%s", CFG_GetShortDeviceName());
		break;
	case ENTITY_SENSOR:
#ifndef OBK_DISABLE_ALL_DRIVERS
		if ((index >= OBK_VOLTAGE) && (index <= OBK_POWER))
			sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), sensor_mqttNames[index]);
		else if ((index >= OBK_CONSUMPTION_TOTAL) && (index <= OBK_CONSUMPTION_STATS))
			sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), counter_mqttNames[index - OBK_CONSUMPTION_TOTAL]);
#endif
		break;
	}
	cJSON_AddStringToObject(info->root, "name", g_hassBuffer);
	cJSON_AddStringToObject(info->root, "~", CFG_GetMQTTClientId());      //base topic
	cJSON_AddStringToObject(info->root, "avty_t", "~/connected");   //availability_topic, `online` value is broadcasted

	if (type != ENTITY_SENSOR) {
		cJSON_AddStringToObject(info->root, "pl_on", payload_on);    //payload_on
		cJSON_AddStringToObject(info->root, "pl_off", payload_off);   //payload_off
	}

	cJSON_AddStringToObject(info->root, "uniq_id", info->unique_id);  //unique_id
	cJSON_AddNumberToObject(info->root, "qos", 1);

	addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "root=%p", info->root);
	return info;
}

/// @brief Initializes HomeAssistant relay device discovery storage.
/// @param index
/// @return 
HassDeviceInfo* hass_init_relay_device_info(int index) {
	HassDeviceInfo* info = hass_init_device_info(ENTITY_RELAY, index, "1", "0");

	sprintf(g_hassBuffer, "~/%i/get", index);
	cJSON_AddStringToObject(info->root, STATE_TOPIC_KEY, g_hassBuffer);   //state_topic
	sprintf(g_hassBuffer, "~/%i/set", index);
	cJSON_AddStringToObject(info->root, COMMAND_TOPIC_KEY, g_hassBuffer);    //command_topic

	return info;
}

/// @brief Initializes HomeAssistant light device discovery storage.
/// @param type 
/// @return 
HassDeviceInfo* hass_init_light_device_info(ENTITY_TYPE type) {
	const char* clientId = CFG_GetMQTTClientId();
	HassDeviceInfo* info = NULL;
	double brightness_scale = 100;

	//We can just use 1 to generate unique_id and name for single PWM.
	//The payload_on/payload_off have to match the state_topic/command_topic values.
	info = hass_init_device_info(type, 1, "1", "0");

	switch (type) {
	case ENTITY_LIGHT_RGBCW:
	case ENTITY_LIGHT_RGB:
		cJSON_AddStringToObject(info->root, "rgb_cmd_tpl", "{{'#%02x%02x%02x0000'|format(red,green,blue)}}");  //rgb_command_template
		cJSON_AddStringToObject(info->root, "rgb_val_tpl", "{{ value[0:2]|int(base=16) }},{{ value[2:4]|int(base=16) }},{{ value[4:6]|int(base=16) }}");  //rgb_value_template

		cJSON_AddStringToObject(info->root, "rgb_stat_t", "~/led_basecolor_rgb/get"); //rgb_state_topic
		sprintf(g_hassBuffer, "cmnd/%s/led_basecolor_rgb", clientId);
		cJSON_AddStringToObject(info->root, "rgb_cmd_t", g_hassBuffer);  //rgb_command_topic
		break;

	case ENTITY_LIGHT_PWM:
		brightness_scale = 100;
		break;

	case ENTITY_LIGHT_PWMCW:
		brightness_scale = 100;

		//Using `last` (the default) will send any style (brightness, color, etc) topics first and then a payload_on to the command_topic. 
		//Using `first` will send the payload_on and then any style topics. 
		//Using `brightness` will only send brightness commands instead of the payload_on to turn the light on.
		cJSON_AddStringToObject(info->root, "on_cmd_type", "first");	//on_command_type
		break;

	default:
		addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "Unsupported light type %s", type);
	}

	if ((type == ENTITY_LIGHT_PWMCW) || (type == ENTITY_LIGHT_RGBCW)) {
		sprintf(g_hassBuffer, "cmnd/%s/led_temperature", clientId);
		cJSON_AddStringToObject(info->root, "clr_temp_cmd_t", g_hassBuffer);    //color_temp_command_topic

		cJSON_AddStringToObject(info->root, "clr_temp_stat_t", "~/led_temperature/get");    //color_temp_state_topic
	}

	cJSON_AddStringToObject(info->root, STATE_TOPIC_KEY, "~/led_enableAll/get");  //state_topic
	sprintf(g_hassBuffer, "cmnd/%s/led_enableAll", clientId);
	cJSON_AddStringToObject(info->root, COMMAND_TOPIC_KEY, g_hassBuffer);  //command_topic

	cJSON_AddStringToObject(info->root, "bri_stat_t", "~/led_dimmer/get");  //brightness_state_topic
	sprintf(g_hassBuffer, "cmnd/%s/led_dimmer", clientId);
	cJSON_AddStringToObject(info->root, "bri_cmd_t", g_hassBuffer);  //brightness_command_topic

	cJSON_AddNumberToObject(info->root, "bri_scl", brightness_scale);	//brightness_scale

	return info;
}

#ifndef OBK_DISABLE_ALL_DRIVERS

/// @brief Initializes HomeAssistant sensor device discovery storage.
/// @param index Index corresponding to sensor_mqttNames.
/// @return 
HassDeviceInfo* hass_init_sensor_device_info(int index) {
	HassDeviceInfo* info = hass_init_device_info(ENTITY_SENSOR, index, NULL, NULL);

	//https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
	//device_class automatically assigns unit,icon
	if ((index >= OBK_VOLTAGE) && (index <= OBK_POWER))
	{
		cJSON_AddStringToObject(info->root, "dev_cla", sensor_mqtt_device_classes[index]);   //device_class=voltage,current,power
		cJSON_AddStringToObject(info->root, "unit_of_meas", sensor_mqtt_device_units[index]);   //unit_of_measurement

		sprintf(g_hassBuffer, "~/%s/get", sensor_mqttNames[index]);
		cJSON_AddStringToObject(info->root, STATE_TOPIC_KEY, g_hassBuffer);

		cJSON_AddStringToObject(info->root, "stat_cla", "measurement");
	}
	else if ((index >= OBK_CONSUMPTION_TOTAL) && (index <= OBK_CONSUMPTION_STATS))
	{
		const char* device_class_value = counter_devClasses[index - OBK_CONSUMPTION_TOTAL];
		if (strlen(device_class_value) > 0) {
			cJSON_AddStringToObject(info->root, "dev_cla", device_class_value);  //device_class=energy
			cJSON_AddStringToObject(info->root, "unit_of_meas", "Wh");   //unit_of_measurement

			//state_class can be measurement, total or total_increasing. Energy values should be total_increasing.
			cJSON_AddStringToObject(info->root, "stat_cla", "total_increasing");
		}

		sprintf(g_hassBuffer, "~/%s/get", counter_mqttNames[index - OBK_CONSUMPTION_TOTAL]);
		cJSON_AddStringToObject(info->root, STATE_TOPIC_KEY, g_hassBuffer);
	}

	return info;
}

/// @brief Initializes HomeAssistant binary sensor device discovery storage.
/// @param index
/// @return
HassDeviceInfo* hass_init_binary_sensor_device_info(int index) {
	HassDeviceInfo* info = hass_init_device_info(ENTITY_BINARY_SENSOR, index, "1", "0");

	sprintf(g_hassBuffer, "~/%i/get", index);
	cJSON_AddStringToObject(info->root, STATE_TOPIC_KEY, g_hassBuffer);   //state_topic

	return info;
}

#endif

/// @brief Returns the discovery JSON.
/// @param info 
/// @return 
char* hass_build_discovery_json(HassDeviceInfo* info) {
	cJSON_PrintPreallocated(info->root, info->json, HASS_JSON_SIZE, 0);
	return info->json;
}

/// @brief Release allocated memory.
/// @param info 
void hass_free_device_info(HassDeviceInfo* info) {
	if (info == NULL) return;
	addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "hass_free_device_info \r\n");

	if (info->root != NULL) {
		cJSON_Delete(info->root);
	}

	os_free(info);
}
