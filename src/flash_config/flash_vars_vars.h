

//#define DISABLE_FLASH_VARS_VARS

#define BOOT_COMPLETE_SECONDS 30

// call at startup
void increment_boot_count();
// call once started (>30s?)
void boot_complete();
// call to return the number of boots since a boot_complete
int boot_failures();