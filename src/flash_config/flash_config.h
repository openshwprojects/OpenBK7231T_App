#include "net_param_pub.h"


int get_tbl(int readit);
int release_tbl();
INFO_ITEM_ST *search_item(INFO_ITEM_ST *item);
INFO_ITEM_ST *search_item_type(UINT32 type);
int compress_table();
int save_item(INFO_ITEM_ST *item);
int delete_item(UINT32 type);
int dump_table();



// config structures - only the tags really need to be here....

//
// strucutre is ITEM_URL_CONFIG
#define CONFIG_TAG_WEBAPP_ROOT ((UINT32) *((UINT32*)"TEST"))
//


#define CONFIG_URL_SIZE_MAX 64

typedef struct item_url_config
{
	INFO_ITEM_ST head;
	char url[CONFIG_URL_SIZE_MAX];
}ITEM_URL_CONFIG,*ITEM_URL_CONFIG_PTR;


