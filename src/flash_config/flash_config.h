#include "net_param_pub.h"

/////////////////////////////////////////////////////
// mutex protected functions:

// copy a config item of type to 'container'
// 'container' should have been initialised with CONFIG_INIT_ITEM
// then it will have the right type and len..
int config_get_item(void *container);

// save an item to config.
// item should start with INFO_ITEM_ST, and be initialised with CONFIG_INIT_ITEM
// will trigger config save 3s later
int config_save_item(void *item);

// delete ALL items of type
int config_delete_item(UINT32 type);

// save pending changes NOW and release the config memory 
int config_commit();

int increment_OTA_count();

/////////////////////////////////////////////////////


// other functions, not protected.
// internal
int config_get_tbl(int readit);
// release memory (saves if changes)
int config_release_tbl();
// return a ptr to the item, unprotected
INFO_ITEM_ST *config_search_item(INFO_ITEM_ST *item);
// return a ptr to the item, unprotected
INFO_ITEM_ST *config_search_item_type(UINT32 type);
// list table contetn by type & len to debug
int config_dump_table();

// debug
int config_get_tableOffsets(int tableID, int *outStart, int *outLen);
// copy of tuya_hal_flash_read (in BK7231T it was in SDK source, but in BK7231N it was in tuya lib)
int bekken_hal_flash_read(const uint32_t addr, void *dst, const UINT32 size);


/////////////////////////////////////////
// config types not defined by beken
//
// structure is ITEM_URL_CONFIG
#define CONFIG_TYPE_WEBAPP_ROOT ((UINT32) *((UINT32*)"WEB"))
// structure is ITEM_FLASHIINFO_CONFIG
#define CONFIG_TYPE_FLASH_INFO ((UINT32) *((UINT32*)"FLSH"))
// 'new' structure
#define CONFIG_TYPE_WIFI ((UINT32) *((UINT32*)"WIFI"))
#define CONFIG_TYPE_MQTT ((UINT32) *((UINT32*)"MQTT"))
#define CONFIG_TYPE_PINS ((UINT32) *((UINT32*)"PINS"))

// OLD VALUES moved from SDK
#define OLD_PINS_CONFIG 0xAAAAAAAA
#define OLD_WIFI_CONFIG 0xBBBBBBBB
#define OLD_MQTT_CONFIG 0xCCCCCCCC

#define CONFIG_INIT_ITEM(t, ptr) { (ptr)->head.len = sizeof(*(ptr)) - sizeof((ptr)->head); (ptr)->head.type = t; }  


/////////////////////////////////////////
// config structures not defined by beken
#define CONFIG_URL_SIZE_MAX 64

typedef struct item_url_config
{
	INFO_ITEM_ST head;
	char url[CONFIG_URL_SIZE_MAX];
}ITEM_URL_CONFIG,*ITEM_URL_CONFIG_PTR;

typedef struct item_flashinfo_config
{
	INFO_ITEM_ST head;
	int flash_write_count;
	int OTA_count;
}ITEM_FLASHIINFO_CONFIG,*ITEM_FLASHINFO_CONFIG_PTR;

// added for OpenBK7231T
typedef struct item_new_wifi_config2
{
	INFO_ITEM_ST head;
    char scrap[8];
	char ssid[32];
	char pass[64];    
}ITEM_NEW_WIFI_CONFIG2,*ITEM_NEW_WIFI_CONFIG2_PTR;

typedef struct item_new_mqtt_config2
{
	INFO_ITEM_ST head;
    char scrap[8];
	char brokerName[64];
	char userName[64];
	int port;
	char hostName[64];
	// Home Assistant default password is 64 chars..
	char pass[128];    
}ITEM_NEW_MQTT_CONFIG2,*ITEM_NEW_MQTT_CONFIG2_PTR;


// added for OpenBK7231T
typedef struct item_new_new_wifi_config
{
    INFO_ITEM_ST head;
    char ssid[32];
    char pass[64];    
}ITEM_NEW_NEW_WIFI_CONFIG,*ITEM_NEW_NEW_WIFI_CONFIG_PTR;

// added for OpenBK7231T
typedef struct item_new_new_mqtt_config
{
    INFO_ITEM_ST head;
    char brokerName[64];
    char userName[64];
    int port;
    char hostName[64];
    // Home Assistant default password is 64 chars..
    char pass[128];    
}ITEM_NEW_NEW_MQTT_CONFIG,*ITEM_NEW_NEW_MQTT_CONFIG_PTR;

