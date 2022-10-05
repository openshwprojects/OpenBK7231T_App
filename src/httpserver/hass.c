#include "hass.h"
#include "../new_common.h"
#include "../new_cfg.h"
#include "../logging/logging.h"

/* Sample Hass Discovery JSON
{
    "dev":{
        "ids":["espurna_9de8f9"],
        "name":"ESPURNA_9DE8F9",
        "sw":"1.15.0-dev.git0e55397a",
        "mf":"NODEMCU",
        "mdl":"LOLIN"
    },
    "avty_t":"ESPURNA-9DE8F9/status",
    "pl_on":"1",
    "pl_off":"0",
    "uniq_id":"espurna_9de8f9_relay_0",
    "name":"ESPURNA_9DE8F9 0",
    "stat_t":"ESPURNA-9DE8F9/relay/0",
    "cmd_t":"ESPURNA-9DE8F9/relay/0/set"
}

Abbreviated node names - https://www.home-assistant.io/docs/mqtt/discovery/

*/


/// @brief Populates HomeAssistant unique id for the entity.
/// @param type Entity type
/// @param index Entity index
/// @param uniq_id Array to populate (should be of size HASS_UNIQUE_ID_SIZE)
void hass_populate_unique_id(ENTITY_TYPE type, int index, char *uniq_id){
    //https://developers.home-assistant.io/docs/entity_registry_index/#unique-id-requirements
    //mentions that mac can be used for unique_id and deviceName contains that.
    const char *longDeviceName = CFG_GetDeviceName();
    
    switch(type){
        case ENTITY_LIGHT_PWM:
            sprintf(uniq_id,"%s_%s_%d", longDeviceName, "light", index);
            break;

        case ENTITY_LIGHT_RGB:
        case ENTITY_LIGHT_RGBCW:
            sprintf(uniq_id,"%s_%s", longDeviceName, "light");
            break;
        
        case ENTITY_RELAY:
            sprintf(uniq_id,"%s_%s_%d", longDeviceName, "relay", index);
            break;

        case ENTITY_SENSOR:
            addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "ENTITY_SENSOR not yet supported");
    }
}

/// @brief Prints HomeAssistant unique id for the entity.
/// @param request
/// @param fmt
/// @param type Entity type
/// @param index Entity index
void hass_print_unique_id(http_request_t *request, const char *fmt, ENTITY_TYPE type, int index){
    char uniq_id[HASS_UNIQUE_ID_SIZE];
    hass_populate_unique_id(type, index, uniq_id);
    hprintf128(request, fmt, uniq_id);
}

/// @brief Populates HomeAssistant device configuration MQTT channel e.g. switch/enbrighten_9de8f9_relay_0/config.
/// @param type Entity type
/// @param uniq_id Entity unique id
/// @param info Device info
void hass_populate_device_config_channel(ENTITY_TYPE type, char *uniq_id, HassDeviceInfo *info){
    //device_type is `switch` or `light`
    switch(type){
        case ENTITY_LIGHT_PWM:
        case ENTITY_LIGHT_RGB:
        case ENTITY_LIGHT_RGBCW:
            sprintf(info->channel, "light/%s/config", uniq_id);
            break;
        
        case ENTITY_RELAY:
            sprintf(info->channel, "switch/%s/config", uniq_id);
            break;
        
        case ENTITY_SENSOR:
            addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "ENTITY_SENSOR not yet supported");
    }
}

/// @brief Builds HomeAssistant device discovery info. The caller needs to free the returned pointer.
/// @param ids 
cJSON *hass_build_device_node(cJSON *ids) {
    cJSON *dev = cJSON_CreateObject();
    cJSON_AddItemToObject(dev, "ids", ids);     //identifiers
    cJSON_AddStringToObject(dev, "name", CFG_GetShortDeviceName());

    #ifdef USER_SW_VER
    cJSON_AddStringToObject(dev, "sw", USER_SW_VER);   //sw_version
    #endif

    cJSON_AddStringToObject(dev, "mf", MANUFACTURER);   //manufacturer
    cJSON_AddStringToObject(dev, "mdl", PLATFORM_MCU_NAME);  //Using chipset for model
    return dev;
}

/// @brief Populates common values for HomeAssistant device discovery.
/// @param root 
/// @param type Entity type 
/// @param index 
/// @param unique_id 
/// @param payload_on 
/// @param payload_off 
/// @param isRGB If true, then state_topic and command_topic are not emitted.
void hass_populate_common(cJSON *root, ENTITY_TYPE type, int index, char *unique_id, char *payload_on, char *payload_off, bool isRGB){
    const char *clientId = CFG_GetMQTTClientId();
    
    //We are stuffing CFG_GetShortDeviceName and clientId into tmp so it needs to be bigger than them. +16 for light/switch/etc.
    char tmp[CGF_MQTT_CLIENT_ID_SIZE + 16];
    
    //A device can have both relay and PWM and they would need to have separate names.
    switch(type){
        case ENTITY_LIGHT_PWM:
        case ENTITY_LIGHT_RGB:
        case ENTITY_LIGHT_RGBCW:
            sprintf(tmp,"%s light %i",CFG_GetShortDeviceName(),index);
            break;
        case ENTITY_RELAY:
            sprintf(tmp,"%s switch %i",CFG_GetShortDeviceName(),index);
            break;
        case ENTITY_SENSOR:
            addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "ENTITY_SENSOR not yet supported");
    }
    
    cJSON_AddStringToObject(root, "name", tmp);

    if (isRGB == false){
        sprintf(tmp,"%s/%i/get",clientId,index);
        cJSON_AddStringToObject(root, "stat_t", tmp);   //state_topic

        sprintf(tmp,"%s/%i/set",clientId,index);
        cJSON_AddStringToObject(root, "cmd_t", tmp);    //command_topic
    }

    sprintf(tmp,"%s/connected",clientId);
    cJSON_AddStringToObject(root, "avty_t", tmp);   //availability_topic

    cJSON_AddStringToObject(root, "pl_on", payload_on);    //payload_on
    cJSON_AddStringToObject(root, "pl_off", payload_off);   //payload_off
    cJSON_AddStringToObject(root, "uniq_id", unique_id);  //unique_id
}

/// @brief Initializes HomeAssistant device discovery storage.
/// @param type 
/// @param index 
/// @param payload_on 
/// @param payload_off 
/// @param isRGB If true, then state_topic and command_topic are not emitted.
/// @return 
HassDeviceInfo *hass_init_device_info(ENTITY_TYPE type, int index, char *payload_on, char *payload_off, bool isRGB){
    HassDeviceInfo *info = os_malloc(sizeof(HassDeviceInfo));
    addLogAdv(LOG_INFO, LOG_FEATURE_HASS, "hass_init_device_info=%p", info);
    
    hass_populate_unique_id(type, index, info->unique_id);
    addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "unique_id=%s", info->unique_id);
    
    hass_populate_device_config_channel(type, info->unique_id, info);
  
    info->ids = cJSON_CreateArray();
    cJSON_AddItemToArray(info->ids, cJSON_CreateString(CFG_GetDeviceName()));

    info->device = hass_build_device_node(info->ids);
    info->root = cJSON_CreateObject();

    cJSON_AddItemToObject(info->root, "dev", info->device);    //device
    
    hass_populate_common(info->root, type, index, info->unique_id, payload_on, payload_off, isRGB);
    addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "root=%p", info->root);
    return info;
}

/// @brief Initializes HomeAssistant relay device discovery storage.
/// @param index
/// @return 
HassDeviceInfo *hass_init_relay_device_info(int index){
    return hass_init_device_info(ENTITY_RELAY, index, "1", "0", false);
}

/// @brief Initializes HomeAssistant light device discovery storage.
/// @param type 
/// @param index
/// @return 
HassDeviceInfo *hass_init_light_device_info(ENTITY_TYPE type, int index){
    char tmp[CGF_MQTT_CLIENT_ID_SIZE + 64];  //Used to generate values based on CFG_GetMQTTClientId
    const char *clientId = CFG_GetMQTTClientId();
    HassDeviceInfo *info = NULL;

    switch(type){
        case ENTITY_LIGHT_RGBCW:
        case ENTITY_LIGHT_RGB:
            info = hass_init_device_info(type, index, "1", "0", true);

            cJSON_AddStringToObject(info->root, "rgb_cmd_tpl","{{'#%02x%02x%02x0000'|format(red, green, blue)}}");

            sprintf(tmp,"cmnd/%s/led_basecolor_rgb",clientId);
            cJSON_AddStringToObject(info->root, "rgb_stat_t", tmp);

            sprintf(tmp,"cmnd/%s/led_basecolor_rgb",clientId);
            cJSON_AddStringToObject(info->root, "rgb_cmd_t", tmp);

            sprintf(tmp,"cmnd/%s/led_enableAll",clientId);
            cJSON_AddStringToObject(info->root, "cmd_t", tmp);

            sprintf(tmp,"cmnd/%s/led_dimmer",clientId);
            cJSON_AddStringToObject(info->root, "bri_cmd_t", tmp);
            cJSON_AddNumberToObject(info->root, "bri_scl", 100);
            cJSON_AddStringToObject(info->root, "bri_val_tpl", "{{value_json.Dimmer}}");

            if (type == ENTITY_LIGHT_RGBCW){
                sprintf(tmp,"cmnd/%s/led_temperature",clientId);
                cJSON_AddStringToObject(info->root, "clr_temp_cmd_t", tmp);

                sprintf(tmp,"cmnd/%s/ctr",clientId);
                cJSON_AddStringToObject(info->root, "clr_temp_stat_t", tmp);
                
                cJSON_AddStringToObject(info->root, "clr_temp_val_tpl", "{{value_json.CT}}");
            }

            break;

        case ENTITY_LIGHT_PWM:
            info = hass_init_device_info(type, index, "99", "0", false);
            cJSON_AddStringToObject(info->root, "on_cmd_type", "brightness"); //on_command_type
            cJSON_AddNumberToObject(info->root, "bri_scl", 99);   //brightness_scale
            cJSON_AddNumberToObject(info->root, "bri_scl", 99);   //brightness_scale
            cJSON_AddBoolToObject(info->root, "opt", cJSON_True);   //optimistic

            sprintf(tmp,"%s/%i/set",clientId,index);
            cJSON_AddStringToObject(info->root, "bri_cmd_t", tmp);    //brightness_command_topic
            break;
        
        default:
            addLogAdv(LOG_ERROR, LOG_FEATURE_HASS, "Unsupported light type %s", type);
    }
    
    return info;
}

/// @brief Returns the discovery JSON.
/// @param info 
/// @return 
char *hass_build_discovery_json(HassDeviceInfo *info){
    cJSON_PrintPreallocated(info->root, info->json, HASS_JSON_SIZE, 0);
    return info->json;
}

/// @brief Release allocated memory.
/// @param info 
void hass_free_device_info(HassDeviceInfo *info){
    if (info == NULL) return;
    addLogAdv(LOG_DEBUG, LOG_FEATURE_HASS, "hass_free_device_info \r\n");
    
    if (info->root != NULL){
        cJSON_Delete(info->root);
    } 

    os_free(info);
}
