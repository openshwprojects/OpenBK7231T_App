#include "hass.h"
#include "../new_common.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_wifi.h"
#include "../driver/drv_public.h"
#include "../new_pins.h"

/*
Abbreviated node names - https://www.home-assistant.io/docs/mqtt/discovery/
Light - https://www.home-assistant.io/integrations/light.mqtt/
Switch - https://www.home-assistant.io/integrations/switch.mqtt/
Sensor - https://www.home-assistant.io/integrations/sensor.mqtt/
*/

//Buffer used to populate values in cJSON_Add* calls. The values are based on
//CFG_GetShortDeviceName and clientId so it needs to be bigger than them. +64 for light/switch/etc.
static char g_hassBuffer[CGF_MQTT_CLIENT_ID_SIZE + 64];
const char *g_template_lowMidHigh = "{% if value == '0' %}\n"
			"	Low\n"
			"{% elif value == '1' %}\n"
			"	Medium\n"
			"{% elif value == '2' %}\n"
			"	High\n"
			"{% else %}\n"
			"	Unknown\n"
			"{% endif %}";

/// @brief Populates HomeAssistant unique id for the entity.
/// @param type Entity type
/// @param index Entity index (Ignored for RGB)
/// @param uniq_id Array to populate (should be of size HASS_UNIQUE_ID_SIZE)
void hass_populate_unique_id(ENTITY_TYPE type, int index, char* uniq_id) {
	//https://developers.home-assistant.io/docs/entity_registry_index/#unique-id-requirements
	//mentions that mac can be used for unique_id and deviceName contains that.
	const char* longDeviceName = CFG_GetDeviceName();

	switch (type) {
	case LIGHT_ON_OFF:
	case LIGHT_PWM:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "light", index);
		break;
	case LIGHT_PWMCW:
	case LIGHT_RGB:
	case LIGHT_RGBCW:
		sprintf(uniq_id, "%s_%s", longDeviceName, "light");
		break;

	case RELAY:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "relay", index);
		break;

	case POWER_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "sensor", index);
		break;

	case BINARY_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "binary_sensor", index);
		break;

	case TEMPERATURE_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "temperature", index);
		break;
	case HUMIDITY_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "humidity", index);
		break;
	case CO2_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "co2", index);
		break;
	case ILLUMINANCE_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "illuminance", index);
		break;
	case SMOKE_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "smoke", index);
		break;
	case TVOC_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "tvoc", index);
		break;
	case BATTERY_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "battery", index);
		break;
	case VOLTAGE_SENSOR:
	case BATTERY_VOLTAGE_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "voltage", index);
		break;
	case CURRENT_SENSOR:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "current", index);
		break;
	default:
		sprintf(uniq_id, "%s_%s_%d", longDeviceName, "sensor", index);
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
	case LIGHT_ON_OFF:
	case LIGHT_PWM:
	case LIGHT_PWMCW:
	case LIGHT_RGB:
	case LIGHT_RGBCW:
		sprintf(info->channel, "light/%s/config", uniq_id);
		break;
	case RELAY:
		sprintf(info->channel, "switch/%s/config", uniq_id);
		break;
	case BINARY_SENSOR:
		sprintf(info->channel, "binary_sensor/%s/config", uniq_id);
		break;
	case SMOKE_SENSOR:
	case CO2_SENSOR:
	case TVOC_SENSOR:
	case POWER_SENSOR:
	case BATTERY_SENSOR:
	case BATTERY_VOLTAGE_SENSOR:
	case TEMPERATURE_SENSOR:
	case HUMIDITY_SENSOR:
		sprintf(info->channel, "sensor/%s/config", uniq_id);
		break;
	default:
		sprintf(info->channel, "sensor/%s/config", uniq_id);
		break;
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
/// @param index This is used to generate generate unique_id and name. 
/// It is ignored for RGB. For power sensors, index corresponds to sensor_mqttNames. For regular sensor, index can be be the channel.
/// @param payload_on The payload that represents enabled state. This is not added for POWER_SENSOR.
/// @param payload_off The payload that represents disabled state. This is not added for POWER_SENSOR.
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

	bool isSensor = false;	//This does not count binary_sensor

	//Build the `name`
	switch (type) {
	case LIGHT_ON_OFF:
	case LIGHT_PWM:
	case RELAY:
	case BINARY_SENSOR:
		sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), CHANNEL_GetLabel(index));
		break;
	case LIGHT_PWMCW:
	case LIGHT_RGB:
	case LIGHT_RGBCW:
		//There can only be one RGB so we can skip including index in the name. Do the same
		//for 2 PWM case.
		sprintf(g_hassBuffer, "%s", CFG_GetShortDeviceName());
		break;
	case POWER_SENSOR:
		isSensor = true;
#ifndef OBK_DISABLE_ALL_DRIVERS
		if ((index >= OBK_VOLTAGE) && (index <= OBK_POWER))
			sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), sensor_mqttNames[index]);
		else if ((index >= OBK_CONSUMPTION_TOTAL) && (index <= OBK_CONSUMPTION_STATS))
			sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), counter_mqttNames[index - OBK_CONSUMPTION_TOTAL]);
#endif
		break;

	case TEMPERATURE_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Temperature", CFG_GetShortDeviceName());
		break;
	case HUMIDITY_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Humidity", CFG_GetShortDeviceName());
		break;
	case CO2_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s CO2", CFG_GetShortDeviceName());
		break;
	case SMOKE_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Smoke", CFG_GetShortDeviceName());
		break;
	case TVOC_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Tvoc", CFG_GetShortDeviceName());
		break;
	case BATTERY_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Battery", CFG_GetShortDeviceName());
		break;
	case BATTERY_VOLTAGE_SENSOR:
		isSensor = true;
		sprintf(g_hassBuffer, "%s Voltage", CFG_GetShortDeviceName());
		break;
	case VOLTAGE_SENSOR:
		sprintf(g_hassBuffer, "%s Voltage", CFG_GetShortDeviceName());
		break;
	case ILLUMINANCE_SENSOR:
		sprintf(g_hassBuffer, "%s Illuminance", CFG_GetShortDeviceName());
		break;
	default:
		sprintf(g_hassBuffer, "%s %s", CFG_GetShortDeviceName(), CHANNEL_GetLabel(index));
		break;
	}
	cJSON_AddStringToObject(info->root, "name", g_hassBuffer);
	cJSON_AddStringToObject(info->root, "~", CFG_GetMQTTClientId());      //base topic
	// remove availability information for sensor to keep last value visible on Home Assistant
	bool flagavty = false;
	flagavty = CFG_HasFlag(OBK_FLAG_NOT_PUBLISH_AVAILABILITY_SENSOR);
	// if door sensor is running, then deep sleep will be invoked mostly, then we dont want availability
#ifndef OBK_DISABLE_ALL_DRIVERS
	if (DRV_IsRunning("DoorSensor") == false && DRV_IsRunning("tmSensor") == false)
#endif
	{
		if (!isSensor || !flagavty) {
			cJSON_AddStringToObject(info->root, "avty_t", "~/connected");   //availability_topic, `online` value is broadcasted
		}
	}

	if (!isSensor) {	//Sensors (except binary_sensor) don't use payload 
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
HassDeviceInfo* hass_init_relay_device_info(int index, ENTITY_TYPE type) {
	HassDeviceInfo* info = hass_init_device_info(type, index, "1", "0");

	sprintf(g_hassBuffer, "~/%i/get", index);
	cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);   //state_topic
	sprintf(g_hassBuffer, "~/%i/set", index);
	cJSON_AddStringToObject(info->root, "cmd_t", g_hassBuffer);    //command_topic

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
	case LIGHT_RGBCW:
	case LIGHT_RGB:
		cJSON_AddStringToObject(info->root, "rgb_cmd_tpl", "{{'#%02x%02x%02x0000'|format(red,green,blue)}}");  //rgb_command_template
		cJSON_AddStringToObject(info->root, "rgb_val_tpl", "{{ value[0:2]|int(base=16) }},{{ value[2:4]|int(base=16) }},{{ value[4:6]|int(base=16) }}");  //rgb_value_template

		cJSON_AddStringToObject(info->root, "rgb_stat_t", "~/led_basecolor_rgb/get"); //rgb_state_topic
		sprintf(g_hassBuffer, "cmnd/%s/led_basecolor_rgb", clientId);
		cJSON_AddStringToObject(info->root, "rgb_cmd_t", g_hassBuffer);  //rgb_command_topic
		break;

	case LIGHT_ON_OFF:
	case LIGHT_PWM:
		brightness_scale = 100;
		break;

	case LIGHT_PWMCW:
		brightness_scale = 100;

		//Using `last` (the default) will send any style (brightness, color, etc) topics first and then a payload_on to the command_topic. 
		//Using `first` will send the payload_on and then any style topics. 
		//Using `brightness` will only send brightness commands instead of the payload_on to turn the light on.
		cJSON_AddStringToObject(info->root, "on_cmd_type", "first");	//on_command_type
		break;

	default:
		addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "Unsupported light type %s", type);
	}

	if ((type == LIGHT_PWMCW) || (type == LIGHT_RGBCW)) {
		sprintf(g_hassBuffer, "cmnd/%s/led_temperature", clientId);
		cJSON_AddStringToObject(info->root, "clr_temp_cmd_t", g_hassBuffer);    //color_temp_command_topic

		cJSON_AddStringToObject(info->root, "clr_temp_stat_t", "~/led_temperature/get");    //color_temp_state_topic

		sprintf(g_hassBuffer, "%.0f", led_temperature_min);
		cJSON_AddStringToObject(info->root, "min_mirs", g_hassBuffer);    //min_mireds

		sprintf(g_hassBuffer, "%.0f", led_temperature_max);
		cJSON_AddStringToObject(info->root, "max_mirs", g_hassBuffer);    //max_mireds
	}

	cJSON_AddStringToObject(info->root, "stat_t", "~/led_enableAll/get");  //state_topic
	sprintf(g_hassBuffer, "cmnd/%s/led_enableAll", clientId);
	cJSON_AddStringToObject(info->root, "cmd_t", g_hassBuffer);  //command_topic

	cJSON_AddStringToObject(info->root, "bri_stat_t", "~/led_dimmer/get");  //brightness_state_topic
	sprintf(g_hassBuffer, "cmnd/%s/led_dimmer", clientId);
	cJSON_AddStringToObject(info->root, "bri_cmd_t", g_hassBuffer);  //brightness_command_topic

	cJSON_AddNumberToObject(info->root, "bri_scl", brightness_scale);	//brightness_scale

	return info;
}

/// @brief Initializes HomeAssistant binary sensor device discovery storage.
/// @param index
/// @return
HassDeviceInfo* hass_init_binary_sensor_device_info(int index, bool bInverse) {
	const char *payload_on;
	const char *payload_off;
	if (bInverse) {
		payload_on = "1";
		payload_off = "0";
	} else {
		payload_off = "1";
		payload_on = "0";
	}
	HassDeviceInfo* info = hass_init_device_info(BINARY_SENSOR, index, payload_on, payload_off);

	sprintf(g_hassBuffer, "~/%i/get", index);
	cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);   //state_topic

	return info;
}

#ifndef OBK_DISABLE_ALL_DRIVERS

/// @brief Initializes HomeAssistant power sensor device discovery storage.
/// @param index Index corresponding to sensor_mqttNames.
/// @return 
HassDeviceInfo* hass_init_power_sensor_device_info(int index) {
	HassDeviceInfo* info = hass_init_device_info(POWER_SENSOR, index, NULL, NULL);

	//https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
	//device_class automatically assigns unit,icon
	if ((index >= OBK_VOLTAGE) && (index <= OBK_POWER))
	{
		cJSON_AddStringToObject(info->root, "dev_cla", sensor_mqtt_device_classes[index]);   //device_class=voltage,current,power
		cJSON_AddStringToObject(info->root, "unit_of_meas", sensor_mqtt_device_units[index]);   //unit_of_measurement

		sprintf(g_hassBuffer, "~/%s/get", sensor_mqttNames[index]);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);

		cJSON_AddStringToObject(info->root, "stat_cla", "measurement");
	}
	else if ((index >= OBK_CONSUMPTION_TOTAL) && (index <= OBK_CONSUMPTION_STATS))
	{
		const char* device_class_value = counter_devClasses[index - OBK_CONSUMPTION_TOTAL];
		if (strlen(device_class_value) > 0) {
			cJSON_AddStringToObject(info->root, "dev_cla", device_class_value);  //device_class=energy
			if (CFG_HasFlag(OBK_FLAG_MQTT_ENERGY_IN_KWH)) {
				cJSON_AddStringToObject(info->root, "unit_of_meas", "kWh");   //unit_of_measurement
			}
			else {
				cJSON_AddStringToObject(info->root, "unit_of_meas", "Wh");   //unit_of_measurement
			}

			//state_class can be measurement, total or total_increasing. Energy values should be total_increasing.
			cJSON_AddStringToObject(info->root, "stat_cla", "total_increasing");
		}

		sprintf(g_hassBuffer, "~/%s/get", counter_mqttNames[index - OBK_CONSUMPTION_TOTAL]);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
	}

	return info;
}

#endif

// generate string like "{{ float(value)*0.1|round(2) }}"
// {{ float(value)*0.1 }} for value=12 give 1.2000000000000002, using round() to limit the decimal places
char *hass_generate_multiplyAndRound_template(int decimalPlacesForRounding, int decimalPointOffset) {
	char tmp[8];
	int i;

	strcpy(g_hassBuffer, "{{ float(value)*");
	if (decimalPointOffset != 0) {
		strcat(g_hassBuffer, "0.");
		for (i = 1; i < decimalPointOffset; i++) {
			strcat(g_hassBuffer, "0");
		}
	}
	strcat(g_hassBuffer, "1|round(");
	sprintf(tmp, "%i", decimalPlacesForRounding);
	strcat(g_hassBuffer, tmp);
	strcat(g_hassBuffer, ") }}");

	return g_hassBuffer;
}

HassDeviceInfo* hass_init_light_singleColor_onChannels(int toggle, int dimmer, int brightness_scale) {
	HassDeviceInfo*dev_info;
	const char* clientId;
	
	clientId = CFG_GetMQTTClientId();
	dev_info = hass_init_device_info(LIGHT_PWM, toggle, "1", "0");

	sprintf(g_hassBuffer, "~/%i/get", toggle);
	cJSON_AddStringToObject(dev_info->root, "stat_t", g_hassBuffer);  //state_topic
	sprintf(g_hassBuffer, "~/%i/set", toggle);
	cJSON_AddStringToObject(dev_info->root, "cmd_t", g_hassBuffer);  //command_topic

	sprintf(g_hassBuffer, "~/%i/get", dimmer);
	cJSON_AddStringToObject(dev_info->root, "bri_stat_t", g_hassBuffer);  //brightness_state_topic
	sprintf(g_hassBuffer, "~/%i/set", dimmer);
	cJSON_AddStringToObject(dev_info->root, "bri_cmd_t", g_hassBuffer);  //brightness_command_topic

	cJSON_AddNumberToObject(dev_info->root, "bri_scl", brightness_scale);	//brightness_scale

	return dev_info;
}
/// @brief Initializes HomeAssistant sensor device discovery storage.
/// @param type
/// @param channel
/// @return 
HassDeviceInfo* hass_init_sensor_device_info(ENTITY_TYPE type, int channel, int decPlaces, int decOffset) {
	int i;

	//Assuming that there is only one DHT setup per device which keeps uniqueid/names simpler
	HassDeviceInfo* info = hass_init_device_info(type, channel, NULL, NULL);	//using channel as index to generate uniqueId

	//https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
	switch (type) {
	case TEMPERATURE_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "temperature");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "°C");

		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case HUMIDITY_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "humidity");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "%");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case SMOKE_SENSOR:
		// there is no "smoke" class!
		//cJSON_AddStringToObject(info->root, "dev_cla", "smoke");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "%");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case CO2_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "carbon_dioxide");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "ppm");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case TVOC_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "volatile_organic_compounds");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "ppb");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case ILLUMINANCE_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "illuminance");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "lx");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case BATTERY_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "battery");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "%");
		cJSON_AddStringToObject(info->root, "stat_t", "~/battery/get");
		break;
	case BATTERY_VOLTAGE_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "voltage");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "mV");
		cJSON_AddStringToObject(info->root, "stat_t", "~/voltage/get");
		break;
	case VOLTAGE_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "voltage");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "V");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case CURRENT_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "current");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "A");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case POWER_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "power");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "W");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case ENERGY_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "energy");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "kWh");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_cla", "total_increasing");
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case POWERFACTOR_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "power_factor");
		//cJSON_AddStringToObject(info->root, "unit_of_meas", "W");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case FREQUENCY_SENSOR:
		cJSON_AddStringToObject(info->root, "dev_cla", "frequency");
		cJSON_AddStringToObject(info->root, "unit_of_meas", "Hz");
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case CUSTOM_SENSOR:
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		break;
	case READONLYLOWMIDHIGH_SENSOR:
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		cJSON_AddStringToObject(info->root, "val_tpl", g_template_lowMidHigh);

		break;
	default:
		sprintf(g_hassBuffer, "~/%d/get", channel);
		cJSON_AddStringToObject(info->root, "stat_t", g_hassBuffer);
		return NULL;
	}

	if (type != READONLYLOWMIDHIGH_SENSOR && type != ENERGY_SENSOR) {
		cJSON_AddStringToObject(info->root, "stat_cla", "measurement");
	}


	if (decPlaces != -1 && decOffset != -1) {
		//https://www.home-assistant.io/integrations/sensor.mqtt/ refers to value_template (val_tpl)
		cJSON_AddStringToObject(info->root, "val_tpl", hass_generate_multiplyAndRound_template(decPlaces, decOffset));
	}

	return info;
}

/// @brief Returns the discovery JSON.
/// @param info 
/// @return 
const char* hass_build_discovery_json(HassDeviceInfo* info) {
	if (info == NULL) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "ERROR: someone passed NULL pointer to hass_build_discovery_json\r\n");
		return "";
	}
	cJSON_PrintPreallocated(info->root, info->json, HASS_JSON_SIZE, 0);
	return info->json;
}

/// @brief Release allocated memory.
/// @param info 
void hass_free_device_info(HassDeviceInfo* info) {
	if (info == NULL)
		return;
	//addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "hass_free_device_info \r\n");

	if (info->root != NULL) {
		cJSON_Delete(info->root);
	}

	os_free(info);
}
