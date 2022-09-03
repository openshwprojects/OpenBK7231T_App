
#include "../cJSON/cJSON.h"

typedef enum {
	ENTITY_RELAY = 0,
    ENTITY_LIGHT = 1
} ENTITY_TYPE;

/// @brief HomeAssistant device discovery information
typedef struct HassDeviceInfo_s{
    char *unique_id;
    char *channel;
    char *json;
    cJSON *root;
    cJSON *device;
    cJSON *ids;
} HassDeviceInfo;

char *hass_build_unique_id(ENTITY_TYPE type, int index);
char *hass_build_unique_id(ENTITY_TYPE type, int index);
char *hass_build_discovery_json(HassDeviceInfo *info);
HassDeviceInfo *hass_init_device_info(ENTITY_TYPE type, int index, char *payload_on, char *payload_off);
void hass_free_device_info(HassDeviceInfo *info);
