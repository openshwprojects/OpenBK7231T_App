
#include "../obk_config.h"


#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "drv_uart.h"
#include "../httpserver/hass.h"

void GenericAC_DoDiscovery(const char *topic) {

	HassDeviceInfo* dev_info = NULL;

	float min = 15;
	float max = 30;
	float step = 0.5f;


	HassDeviceInfo* info = hass_init_device_info(HASS_HVAC, 0, NULL, NULL, 0, 0);

	// Set the name for the HVAC device
	cJSON_AddStringToObject(info->root, "name", "Smart Thermostat");

	// Set temperature unit
	cJSON_AddStringToObject(info->root, "temperature_unit", "C");

	// Set temperature topics
	cJSON_AddStringToObject(info->root, "current_temperature_topic", "~/CurrentTemperature/get");
	sprintf(g_hassBuffer, "cmnd/%s/TargetTemperature", CFG_GetMQTTClientId());
	cJSON_AddStringToObject(info->root, "temperature_command_topic", g_hassBuffer);
	cJSON_AddStringToObject(info->root, "temperature_state_topic", "~/TargetTemperature/get");

	// Set temperature range and step
	cJSON_AddNumberToObject(info->root, "min_temp", min);
	cJSON_AddNumberToObject(info->root, "max_temp", max);
	cJSON_AddNumberToObject(info->root, "temp_step", step);

	// Set mode topics
	cJSON_AddStringToObject(info->root, "mode_state_topic", "~/ACMode/get");
	sprintf(g_hassBuffer, "cmnd/%s/ACMode", CFG_GetMQTTClientId());
	cJSON_AddStringToObject(info->root, "mode_command_topic", g_hassBuffer);

	// Add supported modes
	cJSON* modes = cJSON_CreateArray();
	cJSON_AddItemToArray(modes, cJSON_CreateString("off"));
	cJSON_AddItemToArray(modes, cJSON_CreateString("heat"));
	cJSON_AddItemToArray(modes, cJSON_CreateString("cool"));
	// fan does not work, it has to be fan_only
	cJSON_AddItemToArray(modes, cJSON_CreateString("fan_only"));
	cJSON_AddItemToObject(info->root, "modes", modes);

	if (fanOptions && numFanOptions) {
		// Add fan mode topics
		cJSON_AddStringToObject(info->root, "fan_mode_state_topic", "~/FanMode/get");
		sprintf(g_hassBuffer, "cmnd/%s/FanMode", CFG_GetMQTTClientId());
		cJSON_AddStringToObject(info->root, "fan_mode_command_topic", g_hassBuffer);

		// Add supported fan modes
		cJSON* fan_modes = cJSON_CreateArray();
		for (int i = 0; i < numFanOptions; i++) {
			const char *mode = fanOptions[i];
			cJSON_AddItemToArray(fan_modes, cJSON_CreateString(mode));
		}
		cJSON_AddItemToObject(info->root, "fan_modes", fan_modes);
	}
	if (numSwingHOptions) {
		// Add Swing Horizontal
		cJSON_AddStringToObject(info->root, "swing_horizontal_mode_state_topic", "~/SwingH/get");
		sprintf(g_hassBuffer, "cmnd/%s/SwingH", CFG_GetMQTTClientId());
		cJSON_AddStringToObject(info->root, "swing_horizontal_mode_command_topic", g_hassBuffer);

		cJSON* swingH_modes = cJSON_CreateArray();
		for (int i = 0; i < numSwingHOptions; i++) {
			const char *mode = swingHOptions[i];
			cJSON_AddItemToArray(swingH_modes, cJSON_CreateString(mode));
		}
		cJSON_AddItemToObject(info->root, "swing_horizontal_modes", swingH_modes);
	}
	if (numSwingOptions) {
		// Add Swing Vertical
		cJSON_AddStringToObject(info->root, "swing_mode_state_topic", "~/SwingV/get");
		sprintf(g_hassBuffer, "cmnd/%s/SwingV", CFG_GetMQTTClientId());
		cJSON_AddStringToObject(info->root, "swing_mode_command_topic", g_hassBuffer);

		cJSON* swing_modes = cJSON_CreateArray();
		for (int i = 0; i < numSwingOptions; i++) {
			const char *mode = swingOptions[i];
			cJSON_AddItemToArray(swing_modes, cJSON_CreateString(mode));
		}
		cJSON_AddItemToObject(info->root, "swing_modes", swing_modes);

	}
	// Set availability topic
	cJSON_AddStringToObject(info->root, "availability_topic", "~/status");
	cJSON_AddStringToObject(info->root, "payload_available", "online");
	cJSON_AddStringToObject(info->root, "payload_not_available", "offline");

	// Update device configuration channel for HVAC
	sprintf(info->channel, "climate/%s/config", info->unique_id);

	// Update device info
	cJSON* dev = info->device;
	cJSON_ReplaceItemInObject(dev, "manufacturer", cJSON_CreateString("Custom"));
	cJSON_ReplaceItemInObject(dev, "model", cJSON_CreateString("C-Thermo"));

	MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);
}