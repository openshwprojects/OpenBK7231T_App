



typedef struct flash_vars_structure
{
    unsigned short boot_count; // number of times the device has booted
    unsigned short boot_success_count; // if a device boots completely (>30s), will equal boot_success_count
    unsigned char _align[3];
    unsigned char len; // length of the whole structure (i.e. 2+2+1 = 5)  MUST NOT BE 255
} FLASH_VARS_STRUCTURE;

extern FLASH_VARS_STRUCTURE flash_vars;


int flash_vars_init();
int flash_vars_write();

// for testing only...  only done once at startup...
extern int flash_vars_offset;
int flash_vars_read(FLASH_VARS_STRUCTURE *data);
