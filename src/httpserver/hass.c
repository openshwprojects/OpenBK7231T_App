#include "hass.h"
#include "../new_common.h"
#include "../new_cfg.h"

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
*/

/// @brief Returns HomeAssistant unique id for the entity. The caller needs to free the returned pointer.
/// @param type Entity type
/// @param index Entity index
/// @return 
char *hass_build_unique_id(ENTITY_TYPE type, int index){
    //https://developers.home-assistant.io/docs/entity_registry_index/#unique-id-requirements mentions that mac can be used for
    //unique_id and I think that longDeviceName should contain that e.g. longDeviceName_relay_1

    const char *longDeviceName = CFG_GetDeviceName();

    //Entity type is `relay` or `light` - 5 char
    char *uniq_id = (char *)os_malloc(strlen(longDeviceName) + 1 + 5 + 1 + 4);    //4 for index and nul char
    if (type == ENTITY_LIGHT){
        sprintf(uniq_id,"%s_%s_%d", longDeviceName, "light", index);
    }
    else{
        sprintf(uniq_id,"%s_%s_%d", longDeviceName, "relay", index);
    }

    return uniq_id;
}

/// @brief Returns HomeAssistant device configuration MQTT channel e.g. switch/enbrighten_9de8f9_relay_0/config. The caller needs to free the returned pointer.
/// @param type Entity type
/// @param uniq_id Entity unique id
/// @return 
char *hass_get_device_config_channel(ENTITY_TYPE type, char *uniq_id){
    //device_type is `switch` or `light` - 6 char
    char *channel = (char *)os_malloc(6 + 1 + strlen(uniq_id) + 7 + 1);

    if (type == ENTITY_LIGHT){
        sprintf(channel, "light/%s/config", uniq_id);
    }
    else{
        sprintf(channel, "switch/%s/config", uniq_id);
    }

    return channel;
}

/// @brief Builds HomeAssistant device discovery info. The caller needs to free the returned pointer.
/// @param baseName 
/// @param ids 
cJSON *hass_build_device_node(const char *baseName, cJSON *ids) {
    cJSON *dev = cJSON_CreateObject();
    cJSON_AddItemToObject(dev, "ids", ids);     //identifiers
    cJSON_AddStringToObject(dev, "name", baseName);

    #ifdef USER_SW_VER
    cJSON_AddStringToObject(dev, "sw", USER_SW_VER);   //sw_version
    #endif

    cJSON_AddStringToObject(dev, "mf", MANUFACTURER);   //manufacturer
    cJSON_AddStringToObject(dev, "mdl", PLATFORM_MCU_NAME);  //Using chipset for model
    return dev;
}

/// @brief Populates common values for HomeAssistant device discovery.
/// @param root 
/// @param baseName 
/// @param index 
/// @param unique_id 
/// @param payload_on 
/// @param payload_off 
void hass_populate_common(cJSON *root, const char *baseName, int index, char *unique_id, char *payload_on, char *payload_off){
    char tmp[64];  //baseName is g_cfg.shortDeviceName which reserves 32 char so 64 would be enough

    //Using abbreviated node names as per https://www.home-assistant.io/docs/mqtt/discovery/
    sprintf(tmp,"%s %i",baseName,index);
    cJSON_AddStringToObject(root, "name", tmp);

    sprintf(tmp,"%s/%i/get",baseName,index);
    cJSON_AddStringToObject(root, "stat_t", tmp);   //state_topic

    sprintf(tmp,"%s/%i/set",baseName,index);
    cJSON_AddStringToObject(root, "cmd_t", tmp);    //command_topic

    sprintf(tmp,"%s/connected",baseName);
    cJSON_AddStringToObject(root, "avty_t", tmp);   //availability_topic

    cJSON_AddStringToObject(root, "pl_on", "1");    //payload_on
    cJSON_AddStringToObject(root, "pl_off", "0");   //payload_off
    cJSON_AddStringToObject(root, "uniq_id", unique_id);  //unique_id
}

/// @brief Initializes HomeAssistant device discovery storage.
/// @param type 
/// @param index 
/// @param payload_on 
/// @param payload_off 
/// @return 
HassDeviceInfo *hass_init_device_info(ENTITY_TYPE type, int index, char *payload_on, char *payload_off){
    const char *baseName = CFG_GetShortDeviceName();
    HassDeviceInfo *info = os_malloc(sizeof(HassDeviceInfo));

    info->unique_id = hass_build_unique_id(type, index);
    info->channel = hass_get_device_config_channel(type, info->unique_id);

    info->ids = cJSON_CreateArray();
    cJSON_AddItemToArray(info->ids, cJSON_CreateString(CFG_GetDeviceName()));

    info->device = hass_build_device_node(baseName, info->ids);

    info->root = cJSON_CreateObject();
    cJSON_AddItemToObject(info->root, "dev", info->device);    //device
    
    hass_populate_common(info->root, baseName, index, info->unique_id, payload_on, payload_off);
    return info;
}

/// @brief Returns the discovery JSON.
/// @param info 
/// @return 
char *hass_build_discovery_json(HassDeviceInfo *info){
    info->json = cJSON_PrintUnformatted(info->root);
    return info->json;
}

/// @brief Release allocated memory.
/// @param info 
void hass_free_device_info(HassDeviceInfo *info){
    if (info == NULL) return;

    os_free(info->unique_id);
    os_free(info->channel);
    os_free(info->json);
    
    cJSON_Delete(info->root);
    cJSON_Delete(info->device);
    cJSON_Delete(info->ids);

    os_free(info);
}
