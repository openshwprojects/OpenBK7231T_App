
#include "include.h"
#include "mem_pub.h"
#include "drv_model_pub.h"
#include "net_param_pub.h"
#include "flash_pub.h"
#if CFG_SUPPORT_ALIOS
#include "hal/soc/soc.h"
#else
#include "BkDriverFlash.h"
#include "BkDriverUart.h"
#endif

#include "../logging/logging.h"
#include "flash_config.h"

//#define DEBUG_FLASH_CONFIG

#ifndef DEBUG_FLASH_CONFIG
#undef ADDLOG_DEBUG
#define ADDLOG_DEBUG(x, y, ...)
#endif


// temporary storage for table.
static int *test1;
static int *test2;
static TLV_HEADER_ST *g_table = NULL;
static UINT32 flashaddr;
static UINT32 flashlen;

static int changes = 0;
static int flashwrites = -1;
static int OTAwrites = 0;
extern int g_savecfg;

static int config_compress_table();
static int config_save_table();
int config_getflashinfo();
int increment_flashcounts(int config, int ota);

static INFO_ITEM_ST *_search_item(INFO_ITEM_ST *item, UINT32 *p_usedlen);

static SemaphoreHandle_t config_mutex = 0;


char *hex = "0123456789ABCDEF";
int config_dump(void* obj, int len){
    unsigned char *addr = (unsigned char *)obj;
    char tmp[40];
    int i;
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "dump of 0x%08X", addr);
    for(i = 0; i < len; i++){
        if (i && !(i%16)){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, tmp);
        }
        tmp[(i%16) * 2] = hex[((*addr) >> 4) & 0xf];
        tmp[(i%16) * 2+1] = hex[(*addr)  & 0xf];
        tmp[(i%16) * 2+2] = '\0';
        addr++;
    }
    ADDLOG_DEBUG(LOG_FEATURE_CFG, tmp);
    return 0;
}



int config_get_item(void *container) {
    INFO_ITEM_ST *res;
    INFO_ITEM_ST *p_item = (INFO_ITEM_ST *)container;
    int ret = 0;
    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );

    res = _search_item(p_item, NULL);
    if (res){
        os_memcpy(p_item, res, sizeof(INFO_ITEM_ST) + p_item->len);
        ret = 1;
    }

    if (taken == pdTRUE){
        xSemaphoreGive( config_mutex );
    }
    return ret;
}

int config_get_tableOffsets(int tableID, int *outStart, int *outLen) {
	bk_logic_partition_t *pt;
	pt = bk_flash_get_info(tableID);
	*outStart = 0;
	*outLen = 0;
	if(pt == 0)
		return 1;
    *outStart = pt->partition_start_addr;
    *outLen = pt->partition_length;
	return 0;
}

int config_get_tbl(int readit){
    UINT32 ret = 0, status;
    DD_HANDLE flash_handle;
    TLV_HEADER_ST head;
	bk_logic_partition_t *pt = bk_flash_get_info(BK_PARTITION_NET_PARAM);
	int cfg_len = 0;

    flashaddr = pt->partition_start_addr;
    flashlen = pt->partition_length;

    if (g_table){
        if (*test1 != 0x12345678){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl test1 corrput %08X", *test1);
        }
        if (*test2 != 0x12345678){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl test2 corrput %08X", *test2);
        }

        if(INFO_TLV_HEADER != g_table->type){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl g_table corrupted");
            config_dump((unsigned char *)g_table, 32);
            cfg_len = 0;
            return cfg_len;
        }

        cfg_len = g_table->len + sizeof(TLV_HEADER_ST);
        if (cfg_len > flashlen){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl table too big? %d > %d bytes", cfg_len, flashlen);
            cfg_len = 0;
        }

        ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl got existing table len", cfg_len);

        return cfg_len;
    }

	hal_flash_lock();
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_read(flash_handle, (char *)&head, sizeof(TLV_HEADER_ST), flashaddr);
    if(INFO_TLV_HEADER == head.type)
	{
	 	cfg_len = head.len + sizeof(TLV_HEADER_ST);
        ret = cfg_len;
        if (readit){
            test1 = os_malloc(sizeof(*test1));
            *test1 = 0x12345678;
            g_table = os_malloc(cfg_len);
            test2 = os_malloc(sizeof(*test2));
            *test2 = 0x12345678;
            ddev_read(flash_handle, ((char *)g_table), cfg_len, flashaddr);
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "get_tbl read %d bytes", cfg_len);
        }
    } 
    ddev_close(flash_handle);
	hal_flash_unlock();

    // should happen only on first read.
    if (flashwrites == -1){
        flashwrites = 0;
        config_getflashinfo();
    }
    //config_dump((unsigned char *)g_table, cfg_len);
#ifdef DEBUG_FLASH_CONFIG
    config_dump_table();
#endif
    return ret;
}

int config_release_tbl(){
    void *table;
    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );
    table = g_table;


    if(INFO_TLV_HEADER != g_table->type){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "release_tbl g_table corrupted");
    }

    if (!table) {
        if (taken == pdTRUE){
            xSemaphoreGive( config_mutex );
        }
        return 0;
    }

    if (changes){
        config_save_table();
    }

    g_table = NULL;
    os_free(table);
    if (taken == pdTRUE){
        xSemaphoreGive( config_mutex );
    }
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "release_tbl");
    return 0;
}


static INFO_ITEM_ST *_search_item(INFO_ITEM_ST *item, UINT32 *p_usedlen)
{
    UINT32 addr, end_addr;
    INFO_ITEM_ST *head;
    INFO_ITEM_ST *target = NULL;
    UINT32 usedlen = 0;
    UINT32 type = 0;
    UINT32 tablelen = config_get_tbl(1);
    if (!tablelen){
        if (p_usedlen) *p_usedlen = usedlen;
        return NULL;
    }

    if(INFO_TLV_HEADER != g_table->type){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "_search_item g_table corrupted");
        return NULL;
    }

    if (item){
        type = item->type;
    }

    head = (INFO_ITEM_ST *) (((char *)g_table) + sizeof(TLV_HEADER_ST));
    usedlen = sizeof(TLV_HEADER_ST);

    addr = sizeof(TLV_HEADER_ST);
    end_addr = tablelen;

    while(addr < end_addr) {
        if(type == head->type) {
            target = head;
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "_search_item found %x at %d", type, addr);
        } else {
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "_search_item not %x at %d", type, addr);
        }
        addr += sizeof(INFO_ITEM_ST);
        addr += head->len;
        if (0 != head->type) {
		    usedlen += sizeof(INFO_ITEM_ST);
            usedlen += head->len;
        }
        head = (INFO_ITEM_ST *) (((char *)g_table) + addr);
    }

    if (p_usedlen) *p_usedlen = usedlen;
    return target;
}

static int tbl_used_data_len(){
    UINT32 len = 0;
    _search_item(NULL, &len);
    return (int)len;
}

INFO_ITEM_ST *config_search_item(INFO_ITEM_ST *item){
    return _search_item(item, NULL);
}

INFO_ITEM_ST *config_search_item_type(UINT32 type){
    INFO_ITEM_ST item;
    item.type = type;
    return _search_item(&item, NULL);
}

int bekken_hal_flash_read(const uint32_t addr, void *dst, const UINT32 size)
{
    UINT32 status;
    if(NULL == dst) {
        return 1;
    }
	hal_flash_lock();

    DD_HANDLE flash_handle;
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_read(flash_handle, dst, size, addr);
    ddev_close(flash_handle);
    
	hal_flash_unlock();

    return 0;
}

int config_delete_item(UINT32 type)
{
    UINT32 addr, end_addr;
    INFO_ITEM_ST *head;
    UINT32 deleted = 0;
    UINT32 tablelen;

    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );

    tablelen = config_get_tbl(1);

    if (!tablelen){
        if (taken == pdTRUE) xSemaphoreGive( config_mutex );
        return 0;
    }

    addr = sizeof(TLV_HEADER_ST);
    end_addr = tablelen;
    head = (INFO_ITEM_ST *) (((char *)g_table) + addr);

    while(addr < end_addr) {
        if(type == head->type) {
            head->type = 0;
            deleted++;
        }
        addr += sizeof(INFO_ITEM_ST);
        addr += head->len;
        head = (INFO_ITEM_ST *) (((char *)g_table) + addr);
    }

    if (deleted){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "Deleted %d items type 0x%08X", deleted, type);
        config_compress_table();
    }

    if (taken == pdTRUE) xSemaphoreGive( config_mutex );
    return deleted;
}


//////////////////////////////////////////
// remove redundant entries
static int config_compress_table()
{
    UINT32 addr1, addr2, end_addr;
    INFO_ITEM_ST *head;
    UINT32 usedlen = 0;
    UINT32 tablelen;

    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );

    tablelen = config_get_tbl(1);

    if (!tablelen){
        if (taken == pdTRUE) xSemaphoreGive( config_mutex );
        return 0;
    }

    head = (INFO_ITEM_ST *) (((char *)g_table) + sizeof(TLV_HEADER_ST));
    usedlen = sizeof(TLV_HEADER_ST);

    addr1 = sizeof(TLV_HEADER_ST);
    addr2 = sizeof(TLV_HEADER_ST);
    end_addr = tablelen;

    while(addr1 < end_addr) {
        int len = head->len;
        int type = head->type;
        if (addr1 != addr2){
            INFO_ITEM_ST *newhead = (INFO_ITEM_ST *) (((char *)g_table) + addr2);
            os_memmove(newhead, head, sizeof(INFO_ITEM_ST)+head->len);
        }
        if (0 != type) {
            addr2 += sizeof(INFO_ITEM_ST);
            addr2 += len;
            usedlen += sizeof(INFO_ITEM_ST);
            usedlen += len;
        }
        addr1 += sizeof(INFO_ITEM_ST);
        addr1 += len;
        head = (INFO_ITEM_ST *) (((char *)g_table) + addr1);
    }

    if (addr1 != addr2) {
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "Compress table from %d to %d bytes", g_table->len, usedlen);
        changes++;
    }
    g_table->len = addr2 - sizeof(TLV_HEADER_ST);

    if (taken == pdTRUE) xSemaphoreGive( config_mutex );

    if (addr1 != addr2) {
        return addr2; // maybe should save
    }

    return 0;
}


static int config_save_table(){
    UINT32 tablelen = 0;
	bk_logic_partition_t *pt = bk_flash_get_info(BK_PARTITION_NET_PARAM);

    flashaddr = pt->partition_start_addr;
    flashlen = pt->partition_length;

    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );

    if (!g_table) {
        ADDLOG_ERROR(LOG_FEATURE_CFG, "save_table - no table to save");
        if (taken == pdTRUE) xSemaphoreGive( config_mutex );
        return 0;
    }
    // should already have it...
    increment_flashcounts(1, 0);
    tablelen = config_get_tbl(1);

    if (tablelen > flashlen){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "save_table - table too big - can't save");
        if (taken == pdTRUE) xSemaphoreGive( config_mutex );
        return 0;
    }


	hal_flash_lock();
	bk_flash_enable_security(FLASH_PROTECT_NONE);
	bk_flash_erase(BK_PARTITION_NET_PARAM,0,tablelen);
	bk_flash_write(BK_PARTITION_NET_PARAM,0,(uint8_t *)g_table,tablelen);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
	hal_flash_unlock();
    changes = 0;
    if (taken == pdTRUE) xSemaphoreGive( config_mutex );
    
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "would save_table %d bytes", tablelen);
    return 1;
}


int config_save_item(void *itemin)
{
	UINT32 item_len;
	INFO_ITEM_ST_PTR item_head_ptr;
    INFO_ITEM_ST *item;

    BaseType_t taken;
    if (!config_mutex) {
        config_mutex = xSemaphoreCreateMutex( );
    }
    taken = xSemaphoreTake( config_mutex, 100 );
    item = itemin;

	item_len = sizeof(INFO_ITEM_ST) + item->len;
	
    UINT32 tablelen = config_get_tbl(1);

    if(g_table && (INFO_TLV_HEADER != g_table->type)){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "save_item g_table corrupted");
        if (taken == pdTRUE){
            xSemaphoreGive( config_mutex );
        }
        return 0;
    }

    ADDLOG_DEBUG(LOG_FEATURE_CFG, "save_item type %08X len %d, tablelen %d", item->type, item->len, tablelen);

    if (!tablelen){
        // no table, creat it.
        int cfg_len = item_len + sizeof(TLV_HEADER_ST);

        g_table = os_malloc(cfg_len);
        g_table->type = INFO_TLV_HEADER;
        g_table->len = item_len;
        item_head_ptr = (INFO_ITEM_ST *) (((char *)g_table) + sizeof(TLV_HEADER_ST));
        os_memcpy(item_head_ptr, item, item_len);
        tablelen = cfg_len;
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "save_item, new table created");
        changes++;
    } else {
        // have table - do we have existing item?
        item_head_ptr = config_search_item(item);
        ADDLOG_DEBUG(LOG_FEATURE_API, "save search found %x len %d, our len %d", item_head_ptr, (item_head_ptr?item_head_ptr->len:0), item->len);
        if (item_head_ptr){
            // if length mismatch, then zap this entry, and add on end
            if (item_head_ptr->len != item->len){
                ADDLOG_WARN(LOG_FEATURE_CFG, "save_item - item length mismatch type 0x%08X %d != %d",
                    item->type, item->len, item_head_ptr->len);
                item_head_ptr->type = 0;
                item_head_ptr = NULL;
            } else {
                ADDLOG_DEBUG(LOG_FEATURE_CFG, "save_item - will replace item - same length type 0x%08X %d",
                    item->type, item->len);
            }
        } else {
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "save_item new item");
        }

        // if we STILL have an item, lengths match.
        // just copy in data and write whole table
        if (item_head_ptr){
            if (os_memcmp(item_head_ptr, item, item_len)){
                os_memcpy(item_head_ptr, item, item_len);
                changes++;
            }
        } else {
            UINT32 newlen = 0;
            // add to end
            TLV_HEADER_ST *oldtable;
            TLV_HEADER_ST *newtable;
            oldtable = g_table;
            newtable = os_malloc(tablelen + item_len);
            if(!newtable){
                ADDLOG_DEBUG(LOG_FEATURE_CFG, "allocation failure for %d bytes - save aborted",
                    tablelen + item_len);
                if (taken == pdTRUE){
                    xSemaphoreGive( config_mutex );
                }
                return 0;
            }
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "copy from %x to %x len %d",
                g_table, newtable, tablelen);
            os_memcpy(newtable, g_table, tablelen);
            item_head_ptr = (INFO_ITEM_ST *) (((char *)newtable) + tablelen);
            os_memcpy(item_head_ptr, item, item_len);
            tablelen += item_len;
            newtable->len = tablelen - sizeof(TLV_HEADER_ST);
            g_table = newtable;
            os_free(oldtable);
            newlen = config_compress_table();
            if (newlen){
                tablelen = newlen;
            }
            changes++;
        }
    }

    if (taken == pdTRUE){
        xSemaphoreGive( config_mutex );
    }

    if (changes){
        // save config in 3 seconds....
        if (!g_savecfg){
            g_savecfg = 3;
        }
    }

	return 1;
}


int config_dump_table()
{
    UINT32 addr, end_addr;
    INFO_ITEM_ST *head;
    UINT32 usedlen = 0;
    UINT32 tablelen = config_get_tbl(1);
    if (!tablelen){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "dump_table - no table");
        return 0;
    }
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "dump_table - table len %d(0x%X)", tablelen, tablelen);

    head = (INFO_ITEM_ST *) (((char *)g_table) + sizeof(TLV_HEADER_ST));
    usedlen = sizeof(TLV_HEADER_ST);

    addr = sizeof(TLV_HEADER_ST);
    end_addr = tablelen;

    while(addr < end_addr) {
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "item type 0x%08X len %d at 0x%04X", 
            head->type, head->len, addr);
        config_dump(head, head->len);

        addr += sizeof(INFO_ITEM_ST);
        addr += head->len;
        if (0 != head->type) {
		    usedlen += sizeof(INFO_ITEM_ST);
            usedlen += head->len;
        }
        head = (INFO_ITEM_ST *) (((char *)g_table) + addr);
    }
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "dump_table end - table len %d(0x%X) used len %d(0x%X)", 
        tablelen, tablelen,
        usedlen, usedlen
        );

    return 1;
}

int config_commit(){
    if (changes){
        config_save_table();
        return 1;
    }
    // free config memory;
    config_release_tbl();
    return 0;
}

int config_getflashinfo(){
    ITEM_FLASHIINFO_CONFIG flash_info;
    CONFIG_INIT_ITEM(CONFIG_TYPE_FLASH_INFO, &flash_info);
    if (config_get_item(&flash_info)){
        flashwrites = flash_info.flash_write_count;
	    OTAwrites = flash_info.OTA_count;
    }
    return 0;
}

int increment_flashcounts(int config, int ota){
    ITEM_FLASHIINFO_CONFIG flash_info;
    CONFIG_INIT_ITEM(CONFIG_TYPE_FLASH_INFO, &flash_info);
    flashwrites += config;
    OTAwrites += ota;
    flash_info.flash_write_count = flashwrites;
    flash_info.OTA_count = OTAwrites;
    config_save_item(&flash_info);
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "Flash info - config writes %d, OTA %d", 
        flashwrites,
        OTAwrites
        );
    return 1;
}

int increment_OTA_count(){
    return increment_flashcounts(0, 1);
}