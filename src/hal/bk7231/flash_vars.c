/*
    This module saves variable data to a flash region in an erase effient way.

    Design:
    variables to be small - we want as many writes between erases as possible.
    sector will be all FF (erased), and data added into it.
    reading consists of searching the first non FF byte from the end, then 
    reading the preceding bytes as the variables.
    last byte of data is len (!== 0xFF!) 

*/

#ifndef PLATFORM_XR809

#include "include.h"
#include "mem_pub.h"
#include "drv_model_pub.h"
#include "net_param_pub.h"
#include "flash_pub.h"

#if WINDOWS
#elif PLATFORM_XR809
#else
    #include "BkDriverFlash.h"
    #include "BkDriverUart.h"
#endif

#include "../logging/logging.h"
#include "flash_vars.h"


//#define TEST_MODE
//#define debug_delay(x) rtos_delay_milliseconds(x)
#define debug_delay(x) 

#define FLASH_VARS_MAGIC 0xfefefefe
// NOTE: Changed below according to partitions in SDK!!!!
static unsigned int flash_vars_start = 0x1e3000; //0x1e1000 + 0x1000 + 0x1000; // after netconfig and mystery SSID
static unsigned int flash_vars_len = 0x2000; // two blocks in BK7231 
static unsigned int flash_vars_sector_len = 0x1000; // erase size in BK7231 

FLASH_VARS_STRUCTURE flash_vars;
int flash_vars_offset = 0; // offset to first FF in our area
static int flash_vars_initialised = 0; // offset to first FF in our area

static int flash_vars_valid();
static int flash_vars_write_magic();
static int _flash_vars_write(void *data, unsigned int off_set, unsigned int size);
static int flash_vars_erase(unsigned int off_set, unsigned int size);
int flash_vars_read(FLASH_VARS_STRUCTURE *data);

#if WINDOWS
    #define TEST_MODE
#elif PLATFORM_XR809
    #define TEST_MODE
#else

#endif

#ifdef TEST_MODE

static char test_flash_area[0x2000];

#endif


static void alignOffset(int *offs){
    // do nothing - should work with any offsets
    /*
    if ((*offs) % 32){
        (*offs) += (32- ((*offs)%32));
    }*/
}


// initialise and read variables from flash
int flash_vars_init(){
#if WINDOWS
#elif PLATFORM_XR809
#else
	bk_logic_partition_t *pt;
#endif
    //direct_serial_log = 1;

    if (!flash_vars_initialised){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars not initialised - reading");
        debug_delay(200);


#if WINDOWS
#elif PLATFORM_XR809
#else
        pt = bk_flash_get_info(BK_PARTITION_NET_PARAM);
        // there is an EXTRA sctor used for some form of wifi?
        // on T variety, this is 0x1e3000
        flash_vars_start = pt->partition_start_addr + pt->partition_length + 0x1000;
        flash_vars_len = 0x2000; // two blocks in BK7231 
        flash_vars_sector_len = 0x1000; // erase size in BK7231 
#endif
        //ADDLOG_DEBUG(LOG_FEATURE_CFG, "got part info");
        debug_delay(200);

        os_memset(&flash_vars, 0, sizeof(flash_vars));
        flash_vars.len = sizeof(flash_vars);

        //ADDLOG_DEBUG(LOG_FEATURE_CFG, "cleared structure");
        debug_delay(200);
        // read any existing
        flash_vars_read(&flash_vars);
        flash_vars_initialised = 1;
        //ADDLOG_DEBUG(LOG_FEATURE_CFG, "read structure");
        debug_delay(200);
    } else {
        //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars already initialised");

    }
    return 0;
}

static int flash_vars_valid(){
    //uint32_t i;
    //uint32_t param;
    UINT32 status;
#ifndef TEST_MODE        
    DD_HANDLE flash_hdl;
#endif
    uint32_t start_addr;
    unsigned int tmp = 0xffffffff;
    GLOBAL_INT_DECLARATION();
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash_vars_valid()");
    debug_delay(200);

#ifndef TEST_MODE        
    flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
    ASSERT(DD_HANDLE_UNVALID != flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_NONE);
#endif
    start_addr = flash_vars_start;

    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash_vars_valid() flash open");
    debug_delay(200);

#ifdef TEST_MODE        
    os_memcpy(&tmp, &test_flash_area[start_addr - flash_vars_start], sizeof(tmp));
#else
    GLOBAL_INT_DISABLE();
    ddev_read(flash_hdl, (char *)&tmp, sizeof(tmp), start_addr);
    GLOBAL_INT_RESTORE();
    ddev_close(flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash_vars_valid() copied header 0x%x", tmp);
    debug_delay(200);

    // if not our magic, then erase and write magic
    if (tmp != FLASH_VARS_MAGIC){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "flash_vars_valid - not our magic, erase");
        debug_delay(200);
        if (flash_vars_write_magic() >= 0){
            ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash initialised");
        } else {
            ADDLOG_ERROR(LOG_FEATURE_CFG, "flash initialise failed");
            return -1;
        }
        return 0;
    }
    return 1;
}

static int flash_vars_write_magic(){
    //uint32_t i;
    //uint32_t param;
    UINT32 status;
#ifndef TEST_MODE        
    DD_HANDLE flash_hdl;
#endif
    uint32_t start_addr;
    unsigned int tmp = FLASH_VARS_MAGIC;
    GLOBAL_INT_DECLARATION();
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash_vars_magic write");
    debug_delay(200);

    start_addr = flash_vars_start;
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash open");
    debug_delay(200);

    if (flash_vars_erase(0, flash_vars_len) >= 0){
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash erased");
        debug_delay(200);
    } else {
        ADDLOG_ERROR(LOG_FEATURE_CFG, "flash erase failed");
        debug_delay(200);
        return -1;
    }
#ifdef TEST_MODE        
    os_memcpy(&test_flash_area[start_addr - flash_vars_start], &tmp, sizeof(tmp));
#else
	bk_flash_enable_security(FLASH_PROTECT_NONE);

    flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
    ASSERT(DD_HANDLE_UNVALID != flash_hdl);
    GLOBAL_INT_DISABLE();
    ddev_write(flash_hdl, (char *)&tmp, sizeof(tmp), start_addr);
    GLOBAL_INT_RESTORE();
    ddev_close(flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "header written");
    // next write point
    flash_vars_offset = sizeof(tmp);
    alignOffset(&flash_vars_offset);

    ADDLOG_DEBUG(LOG_FEATURE_CFG, "header written %d", flash_vars_offset);
    debug_delay(200);

    // write a new structure
    flash_vars_write();

    return 0;
}


// read data from flash vars area.
// design:
// search from end of flash until we find a non-zero byte.
// this is length of existing data.
// clear structure to 00
// read existing data (excluding len) into structure.
// set len to current structure defn len.
// remember first FF byte offset
int flash_vars_read(FLASH_VARS_STRUCTURE *data){
    //uint32_t i;
    //uint32_t param;
    UINT32 status;
#ifndef TEST_MODE        
    DD_HANDLE flash_hdl;
#endif
    uint32_t start_addr;
    int loops = 0x2100/4;
    unsigned int tmp = 0xffffffff;
    GLOBAL_INT_DECLARATION();
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash_vars_read len %d", sizeof(*data));

    // check for magic, and reset sector(s) if not.
    if (flash_vars_valid() >= 0){
        //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars valid or initialised");
    } else {
        ADDLOG_ERROR(LOG_FEATURE_CFG, "flash_vars_validity error");
        os_memset(data, 0, sizeof(*data));
        data->len = sizeof(*data);
        debug_delay(200);
        return -1;
    }
    debug_delay(200);

#ifndef TEST_MODE        
    flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
    ASSERT(DD_HANDLE_UNVALID != flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_NONE);
#endif
    start_addr = flash_vars_start + flash_vars_len;

    do {
        start_addr -= sizeof(tmp);
#ifdef TEST_MODE        
        os_memcpy(&tmp, &test_flash_area[start_addr - flash_vars_start], sizeof(tmp));
#else
        GLOBAL_INT_DISABLE();
        ddev_read(flash_hdl, (char *)&tmp, sizeof(tmp), start_addr);
        GLOBAL_INT_RESTORE();
#endif
    } while ((tmp == 0xFFFFFFFF) && (start_addr > flash_vars_start + 4) && (loops--));
    if (!loops){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "loops over addr 0x%X", start_addr);
    }

    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "found at %u 0x%X", start_addr, tmp);
    start_addr += sizeof(tmp);
    debug_delay(200);

    if (tmp == 0xffffffff){
        // no data found, all erased
        // clear result.
        os_memset(data, 0, sizeof(*data));
        // set the len to the latest revision's len
        data->len = sizeof(*data);
#ifndef TEST_MODE        
        ddev_close(flash_hdl);
    	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
        ADDLOG_INFO(LOG_FEATURE_CFG, "new flash vars");
        return 0;
    } else {
        int shifts = 0;
        int len = 0;
        while ((tmp & 0xFF000000) == 0xFF000000){
            tmp <<= 8;
            shifts ++;
        }
        len = (tmp >> 24) & 0xff;
        start_addr -= shifts;
        start_addr -= len;

        if (len > sizeof(*data)){
            ADDLOG_ERROR(LOG_FEATURE_CFG, "len (%d) in flash_var greater than current structure len (%d)", len, sizeof(*data));
#ifndef TEST_MODE        
            ddev_close(flash_hdl);
        	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
            // erase it all and write a new one
            flash_vars_write_magic();

        } else {
            // clear result.
            os_memset(data, 0, sizeof(*data));
            // read the DATA portion into the structure
#ifdef TEST_MODE        
            os_memcpy(data, &test_flash_area[start_addr - flash_vars_start], len-1);
#else
            GLOBAL_INT_DISABLE();
            ddev_read(flash_hdl, (char *)data, len-1, start_addr);
            GLOBAL_INT_RESTORE();
            ddev_close(flash_hdl);
        	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
            // set the len to the latest revision's len
            data->len = sizeof(*data);
            flash_vars_offset = (start_addr - flash_vars_start) + len;
            alignOffset(&flash_vars_offset);

            ADDLOG_DEBUG(LOG_FEATURE_CFG, "new offset after read %d, boot_count %d, success count %d",
                flash_vars_offset,
                data->boot_count,
                data->boot_success_count
                );

        }
        return 1;
    }
}


int flash_vars_write(){
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars write");
    flash_vars_init();
    alignOffset(&flash_vars_offset);

    FLASH_VARS_STRUCTURE *data = &flash_vars;
    if (flash_vars_offset + data->len > flash_vars_len){
        if (flash_vars_write_magic() >= 0){
            ADDLOG_ERROR(LOG_FEATURE_CFG, "flash reinitialised");
        } else {
            ADDLOG_ERROR(LOG_FEATURE_CFG, "flash reinitialise failed");
            return -1;
        }
    }

    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars write at offset %d len %d", flash_vars_offset, data->len);
    _flash_vars_write(data, flash_vars_offset, data->len);
    flash_vars_offset += data->len;
    alignOffset(&flash_vars_offset);

    ADDLOG_DEBUG(LOG_FEATURE_CFG, "new offset %d, boot_count %d, success count %d", 
        flash_vars_offset, 
        data->boot_count,
        data->boot_success_count
        );
    return 1;
}


// write updated data to flash vars area.
// off_set is zero based.  size in bytes
// TODO - test we CAN write at a byte boundary?
// answer - the flash driver in theroy deals with than... writes are always in chunks of 32 bytes
// on 32 byte boundaries.
int _flash_vars_write(void *data, unsigned int off_set, unsigned int size){
    //uint32_t i;
    //uint32_t param;
    UINT32 status;
#ifndef TEST_MODE        
    DD_HANDLE flash_hdl;
#endif
    uint32_t start_addr;
    GLOBAL_INT_DECLARATION();
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "_flash vars write offset %d, size %d", off_set, size);

#ifndef TEST_MODE        
    flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
    ASSERT(DD_HANDLE_UNVALID != flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_NONE);
#endif
    start_addr = flash_vars_start + off_set;

    if (start_addr <= flash_vars_start){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "_flash vars write invalid addr 0x%X", start_addr);
        ddev_close(flash_hdl);
    	bk_flash_enable_security(FLASH_PROTECT_ALL);
        return -1;
    }
    if (start_addr + size > flash_vars_start + flash_vars_len){
        ADDLOG_ERROR(LOG_FEATURE_CFG, "_flash vars write invalid addr 0x%X len 0x%X", start_addr, size);
        ddev_close(flash_hdl);
    	bk_flash_enable_security(FLASH_PROTECT_ALL);
        return -1;
    }

#ifdef TEST_MODE        
    os_memcpy(&test_flash_area[start_addr - flash_vars_start], data, size);
#else
    GLOBAL_INT_DISABLE();
    ddev_write(flash_hdl, data, size, start_addr);
    GLOBAL_INT_RESTORE();
    ddev_close(flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif

    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "_flash vars write wrote offset %d, size %d", off_set, size);

    return 0;
}

// erase one or more of the sectors we are using.
// off_set is zero based.  size in bytes
// in theory, can't erase outside of OUR area.
int flash_vars_erase(unsigned int off_set, unsigned int size){
    uint32_t i;
    uint32_t param;
    UINT32 status;
#ifndef TEST_MODE        
    DD_HANDLE flash_hdl;
#endif
    uint32_t start_sector, end_sector;
    GLOBAL_INT_DECLARATION();
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars erase at offset %d len %d", off_set, size);

#ifndef TEST_MODE        
    flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
    ASSERT(DD_HANDLE_UNVALID != flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_NONE);
#endif
    start_sector = off_set >> 12;
    end_sector = (off_set + size - 1) >> 12;

    for(i = start_sector; i <= end_sector; i ++)
    {
        param = flash_vars_start + (i << 12);
        if (param < flash_vars_start){
            ADDLOG_ERROR(LOG_FEATURE_CFG, "flash vars erase invalid addr 0x%X < 0x%X", param, flash_vars_start);
#ifndef TEST_MODE        
            ddev_close(flash_hdl);
        	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
            return -1;
        }
        if (param + flash_vars_sector_len > flash_vars_start + flash_vars_len){
            ADDLOG_ERROR(LOG_FEATURE_CFG, "flash vars erase invalid addr 0x%X+0x%X > 0x%X", param, flash_vars_sector_len, flash_vars_start + flash_vars_len);
#ifndef TEST_MODE        
            ddev_close(flash_hdl);
        	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
            return -1;
        }
        ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars erase block at addr 0x%X", param);
#ifdef TEST_MODE        
        os_memset(&test_flash_area[param - flash_vars_start], 0xff, 0x1000);
#else
        GLOBAL_INT_DISABLE();
        ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&param);
        GLOBAL_INT_RESTORE();
#endif
    }
#ifndef TEST_MODE        
    ddev_close(flash_hdl);
	bk_flash_enable_security(FLASH_PROTECT_ALL);
#endif
    //ADDLOG_DEBUG(LOG_FEATURE_CFG, "flash vars erase end");

    return 0;
}

#endif 
