
#ifndef PLATFORM_XR809

#include "flash_vars.h"
#include "../logging/logging.h"


//#define DISABLE_FLASH_VARS_VARS


// call at startup
void increment_boot_count(){
#ifndef DISABLE_FLASH_VARS_VARS
    FLASH_VARS_STRUCTURE data;

    flash_vars_init();
    flash_vars.boot_count ++;
    ADDLOG_INFO(LOG_FEATURE_CFG, "####### Boot Count %d #######", flash_vars.boot_count);
    flash_vars_write();

    flash_vars_read(&data);
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "re-read - offset %d, boot count %d, boot success %d, bootfailures %d", 
        flash_vars_offset, 
        data.boot_count, 
        data.boot_success_count,
        data.boot_count - data.boot_success_count );
#endif
}

// call once started (>30s?)
void boot_complete(){
#ifndef DISABLE_FLASH_VARS_VARS
    FLASH_VARS_STRUCTURE data;
    // mark that we have completed a boot.
    ADDLOG_INFO(LOG_FEATURE_CFG, "####### Set Boot Complete #######");

    flash_vars.boot_success_count = flash_vars.boot_count;
    flash_vars_write();

    flash_vars_read(&data);
    ADDLOG_DEBUG(LOG_FEATURE_CFG, "re-read - offset %d, boot count %d, boot success %d, bootfailures %d", 
        flash_vars_offset, 
        data.boot_count, 
        data.boot_success_count,
        data.boot_count - data.boot_success_count );
#endif
}

// call to return the number of boots since a boot_complete
int boot_failures(){
    int diff = 0;
#ifndef DISABLE_FLASH_VARS_VARS
    diff = flash_vars.boot_count - flash_vars.boot_success_count;
#endif
    return diff;
}

#endif 
