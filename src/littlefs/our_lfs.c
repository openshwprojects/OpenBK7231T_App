/*****************************************************************************
* Implements littlefs in the area used for OTA upgrades
* this means that OTA will always wipe the filesystem
* so we may implement a FS backup and restore around the OTA function.
*
* but, this does have the advantage that we can have >512k of FS space
*
*****************************************************************************/

#include "../new_common.h"
#include "our_lfs.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"

#if PLATFORM_BEKEN || WINDOWS

#include "typedef.h"
#include "flash_pub.h"

#elif PLATFORM_BL602

#include <bl_flash.h>
#include <bl_mtd.h>

#elif PLATFORM_LN882H

#include "../hal/hal_flashConfig.h"
#include "hal/hal_flash.h"
#include "flash_partition_table.h"

#elif PLATFORM_ESPIDF

#include "esp_partition.h"
esp_partition_t* esplfs = NULL;

#endif


//https://github.com/littlefs-project/littlefs


#if ENABLE_LITTLEFS
// define the feature ADDLOGF_XXX will use
#define LOG_FEATURE LOG_FEATURE_LFS

// variables used by the filesystem
int lfs_initialised = 0;
lfs_t lfs;
lfs_file_t file;

// from flash.c
#if PLATFORM_BEKEN
extern UINT32 flash_read(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_write(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_ctrl(UINT32 cmd, void *parm);
#endif 

#ifdef LFS_BOOTCOUNT
int boot_count = -1;
#endif

// Read a region in a block. Negative error codes are propogated
// to the user.
static int lfs_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block);

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int lfs_sync(const struct lfs_config *c);


uint32_t LFS_Start = LFS_BLOCKS_END - LFS_BLOCKS_DEFAULT_LEN;
uint32_t LFS_Size = LFS_BLOCKS_DEFAULT_LEN;

// configuration of the filesystem is provided by this struct
struct lfs_config cfg = {
    // block device operations
    .read  = lfs_read,
    .prog  = lfs_write,
    .erase = lfs_erase,
    .sync  = lfs_sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = LFS_BLOCK_SIZE,
    .block_count = (LFS_BLOCKS_DEFAULT_LEN/LFS_BLOCK_SIZE),
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

int lfs_present(){
    return lfs_initialised;
}

static commandResult_t CMD_LFS_Size(const void *context, const char *cmd, const char *args, int cmdFlags){
    if (!args || !args[0]){
        ADDLOG_INFO(LOG_FEATURE_CMD, "unchanged LFS size 0x%X configured 0x%X", LFS_Size, CFG_GetLFS_Size());
        return CMD_RES_OK;
    }
#ifdef PLATFORM_ESPIDF
    ADDLOG_ERROR(LOG_FEATURE_CMD, "ESP doesn't support changing LFS size");
    return CMD_RES_ERROR;
#endif // PLATFORM_ESPIDF


    const char *p = args;

    uint32_t newsize = strtol(args, (char **)&p, 0);
    uint32_t newstart = (LFS_BLOCKS_END - newsize);

    newsize = (newsize/LFS_BLOCK_SIZE)*LFS_BLOCK_SIZE;

    if ((newsize < LFS_BLOCKS_MIN_LEN) || (newsize > LFS_BLOCKS_MAX_LEN)) {
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFSSize OUT OF BOUNDS 0x%X (range 0x%X-0x%X)", 
            newsize,
            LFS_BLOCKS_MIN_LEN,
            LFS_BLOCKS_MAX_LEN
            );
        return CMD_RES_ERROR;
    }

    // double check again that we're within bounds - don't want
    // boot overwrite or anything nasty....
    if ((newstart < LFS_BLOCKS_START_MIN) || (newstart >= LFS_BLOCKS_END)){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFSSize OUT OF BOUNDS start 0x%X ", newstart);
        return CMD_RES_ERROR;
    }
    if ((newstart + newsize > LFS_BLOCKS_END) ||
        (newstart + newsize < LFS_BLOCKS_START_MIN)){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFSSize OUT OF BOUNDS end 0x%X", newstart + newsize);
        return CMD_RES_ERROR;
    }


    CFG_SetLFS_Size(newsize);
    ADDLOG_INFO(LOG_FEATURE_CMD, "LFS size 0x%X new configured 0x%X", LFS_Size, CFG_GetLFS_Size());
    return CMD_RES_OK;
}

static commandResult_t CMD_LFS_Unmount(const void *context, const char *cmd, const char *args, int cmdFlags){
    if (lfs_initialised){
        release_lfs();
        ADDLOG_INFO(LOG_FEATURE_CMD, "unmounted LFS size 0x%X", LFS_Size);
    } else {
        ADDLOG_INFO(LOG_FEATURE_CMD, "LFS was not mounted - size 0x%X", LFS_Size);
    }
    return CMD_RES_OK;
}

static commandResult_t CMD_LFS_Mount(const void *context, const char *cmd, const char *args, int cmdFlags){
    if (lfs_initialised){
        ADDLOG_INFO(LOG_FEATURE_CMD, "LFS already mounted size 0x%X", LFS_Size);
    } else {
        init_lfs(1);
        ADDLOG_INFO(LOG_FEATURE_CMD, "LFS mounted size 0x%X", LFS_Size);
    }
    return CMD_RES_OK;
}

static commandResult_t CMD_LFS_Format(const void *context, const char *cmd, const char *args, int cmdFlags){
    if (lfs_initialised){
        release_lfs();
        ADDLOG_INFO(LOG_FEATURE_CMD, "LFS released size 0x%X", LFS_Size);
    }

    if (args && args[0]){
        // if we have a size, set the size first
        CMD_LFS_Size(context, cmd, args, cmdFlags);
    }

    uint32_t newsize = CFG_GetLFS_Size();

    newsize = (newsize/LFS_BLOCK_SIZE)*LFS_BLOCK_SIZE;
    if ((newsize < LFS_BLOCKS_MIN_LEN) || (newsize > LFS_BLOCKS_MAX_LEN)) {
        ADDLOGF_ERROR("LFSSize OUT OF BOUNDS 0x%X (range 0x%X-0x%X) - defaulting to 0x%X", 
            newsize,
            LFS_BLOCKS_MIN_LEN,
            LFS_BLOCKS_MAX_LEN,
            LFS_BLOCKS_DEFAULT_LEN
            );
        newsize = LFS_BLOCKS_DEFAULT_LEN;
    }
    uint32_t newstart = (LFS_BLOCKS_END - newsize);

    // double check again that we're within bounds - don't want
    // boot overwrite or anything nasty....
    if (newstart < LFS_BLOCKS_START_MIN){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS OUT OF BOUNDS start 0x%X too small", newstart);
        return CMD_RES_ERROR;
    }
    if ((newstart + newsize > LFS_BLOCKS_END) ||
        (newstart + newsize < LFS_BLOCKS_START_MIN)){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS OUT OF BOUNDS end 0x%X too big", newstart + newsize);
        return CMD_RES_ERROR;
    }

    LFS_Start = newstart;
    LFS_Size = newsize;
    cfg.block_count = (newsize/LFS_BLOCK_SIZE);

    int err  = lfs_format(&lfs, &cfg);
    ADDLOG_INFO(LOG_FEATURE_CMD, "LFS formatted size 0x%X (err %d)", LFS_Size, err);
    init_lfs(1);
    if (!lfs_initialised){
        ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS error");
    } else {
        ADDLOG_INFO(LOG_FEATURE_CMD, "LFS mounted");
    }
    return CMD_RES_OK;
}

static commandResult_t CMD_LFS_Append_Internal(lcdPrintType_t type, bool bLine, bool bAppend, const char *args) {
	const char *fileName;
	const char *str;
	float f;
	int i;
	char buffer[8];

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "Not enough arguments.");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	fileName = Tokenizer_GetArg(0);
	if (type == LCD_PRINT_FLOAT) {
		f = Tokenizer_GetArgFloat(1);
		snprintf(buffer, sizeof(buffer), "%f", f);
		str = buffer;
	}
	else if (type == LCD_PRINT_INT) {
		i = Tokenizer_GetArgInteger(1);
		snprintf(buffer, sizeof(buffer), "%i", i);
		str = buffer;
	}
	else {
		str = Tokenizer_GetArgFrom(1);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "Writing %s to %s", str, fileName);

	// read current count
	lfs_file_open(&lfs, &file, fileName, LFS_O_RDWR | LFS_O_CREAT);
	if (bAppend) {
		lfs_file_seek(&lfs, &file, 0, LFS_SEEK_END);
	}
	else {
		lfs_file_truncate(&lfs, &file, 0);
	}
	lfs_file_write(&lfs, &file, str, strlen(str));
	if (bLine) {
		lfs_file_write(&lfs, &file, "\r\n", 2);
	}
	lfs_file_close(&lfs, &file);


	return CMD_RES_OK;
}
static commandResult_t CMD_LFS_Write(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_DEFAULT, false, false, args);
}
static commandResult_t CMD_LFS_WriteLine(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_DEFAULT, true, false, args);
}
static commandResult_t CMD_LFS_AppendInt(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_INT, false, true, args);
}
static commandResult_t CMD_LFS_AppendFloat(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_FLOAT, false, true, args);
}
static commandResult_t CMD_LFS_Append(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_DEFAULT, false, true, args);
}
static commandResult_t CMD_LFS_AppendLine(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return CMD_LFS_Append_Internal(LCD_PRINT_DEFAULT, true, true, args);
}
static commandResult_t CMD_LFS_Remove(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *fileName;
	int res;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	fileName = Tokenizer_GetArg(0);

	res = lfs_remove(&lfs, fileName);


	return CMD_RES_OK;
}
void LFSAddCmds(){
	//cmddetail:{"name":"lfs_size","args":"[MaxSize]",
	//cmddetail:"descr":"Log or Set LFS size - will apply and re-format next boot, usage setlfssize 0x10000",
	//cmddetail:"fn":"CMD_LFS_Size","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("lfs_size", CMD_LFS_Size, NULL);	
	//cmddetail:{"name":"lfs_unmount","args":"",
	//cmddetail:"descr":"Un-mount LFS",
	//cmddetail:"fn":"CMD_LFS_Unmount","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("lfs_unmount", CMD_LFS_Unmount, NULL);	
	//cmddetail:{"name":"lfs_mount","args":"",
	//cmddetail:"descr":"Mount LFS",
	//cmddetail:"fn":"CMD_LFS_Mount","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("lfs_mount", CMD_LFS_Mount, NULL);	
	//cmddetail:{"name":"lfs_format","args":"",
	//cmddetail:"descr":"Unmount and format LFS.  Optionally add new size as argument",
	//cmddetail:"fn":"CMD_LFS_Format","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("lfs_format", CMD_LFS_Format, NULL);
	//cmddetail:{"name":"lfs_append","args":"[FileName][String]",
	//cmddetail:"descr":"Appends a string to LFS file",
	//cmddetail:"fn":"CMD_LFS_Append","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_append", CMD_LFS_Append, NULL);
	//cmddetail:{"name":"lfs_appendFloat","args":"[FileName][Float]",
	//cmddetail:"descr":"Appends a float to LFS file",
	//cmddetail:"fn":"CMD_LFS_AppendFloat","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendFloat", CMD_LFS_AppendFloat, NULL);
	//cmddetail:{"name":"lfs_appendInt","args":"[FileName][Int]",
	//cmddetail:"descr":"Appends a Int to LFS file",
	//cmddetail:"fn":"CMD_LFS_AppendInt","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendInt", CMD_LFS_AppendInt, NULL);
	//cmddetail:{"name":"lfs_appendLine","args":"[FileName][String]",
	//cmddetail:"descr":"Appends a string to LFS file with a next line marker",
	//cmddetail:"fn":"CMD_LFS_AppendLine","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendLine", CMD_LFS_AppendLine, NULL);
	//cmddetail:{"name":"lfs_remove","args":"[FileName]",
	//cmddetail:"descr":"Deletes a LittleFS file",
	//cmddetail:"fn":"CMD_LFS_Remove","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_remove", CMD_LFS_Remove, NULL);
	//cmddetail:{"name":"lfs_write","args":"[FileName][String]",
	//cmddetail:"descr":"Resets a LFS file and writes a new string to it",
	//cmddetail:"fn":"CMD_LFS_Write","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_write", CMD_LFS_Write, NULL);
	//cmddetail:{"name":"lfs_writeLine","args":"[FileName][String]",
	//cmddetail:"descr":"Resets a LFS file and writes a new string to it with newline",
	//cmddetail:"fn":"CMD_LFS_WriteLine","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_writeLine", CMD_LFS_WriteLine, NULL);

}


void init_lfs(int create){
    if (!lfs_initialised){
        uint32_t newsize = CFG_GetLFS_Size();

        // double check again that we're within bounds - don't want
        // boot overwrite or anything nasty....
        newsize = (newsize/LFS_BLOCK_SIZE)*LFS_BLOCK_SIZE;
        if ((newsize < LFS_BLOCKS_MIN_LEN) || (newsize > LFS_BLOCKS_MAX_LEN)) {
            ADDLOGF_ERROR("LFSSize OUT OF BOUNDS 0x%X (range 0x%X-0x%X) - defaulting to 0x%X", 
                newsize,
                LFS_BLOCKS_MIN_LEN,
                LFS_BLOCKS_MAX_LEN,
                LFS_BLOCKS_DEFAULT_LEN
                );
            newsize = LFS_BLOCKS_DEFAULT_LEN;
        }
        uint32_t newstart = (LFS_BLOCKS_END - newsize);

        if (newstart < LFS_BLOCKS_START_MIN){
            ADDLOGF_ERROR("LFS OUT OF BOUNDS start 0x%X too small", newstart);
            return;
        }
        if ((newstart + newsize > LFS_BLOCKS_END) ||
            (newstart + newsize < LFS_BLOCKS_START_MIN)){
            ADDLOGF_ERROR("LFS OUT OF BOUNDS end 0x%X too big", newstart + newsize);
            return;
        }

	#if PLATFORM_BL602
	// ensure we have media partition at correct place, because it can change depending on bldevcube version
	// this supports 1.4.8
	    
	int ret;
    	bl_mtd_info_t info;
	bl_mtd_handle_t handle;

    	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_ROMFS, &handle, BL_MTD_OPEN_FLAG_BUSADDR);
    	if (ret < 0) {
		ADDLOGF_ERROR("LFS media partition not found %d", ret);
		return;
	}
    	memset(&info, 0, sizeof(info));
        bl_mtd_info(handle, &info);
	if (info.offset != LFS_BLOCKS_START ){
		ADDLOGF_ERROR("LFS media partition position 0x%X while expected is 0x%X", info.offset, LFS_BLOCKS_START);
		return;
	}
	#endif

#if PLATFORM_ESPIDF
        if(esplfs == NULL)
        {
            esplfs = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
            if(esplfs != NULL)
            {
                ADDLOGF_INFO("Partition %s found, size: %i", esplfs->label, esplfs->size);
            }
            else
            {
                return;
            }
        }
        newstart = 0;
        newsize = esplfs->size;
#endif

        LFS_Start = newstart;
        LFS_Size = newsize;
        cfg.block_count = (newsize/LFS_BLOCK_SIZE);

        int err = lfs_mount(&lfs, &cfg);

        // reformat if we can't mount the filesystem
        // this should only happen on the first boot
        if (err){
            if (create) {
                ADDLOGF_INFO("Formatting LFS");
                err  = lfs_format(&lfs, &cfg);
                if (err){
                    ADDLOGF_ERROR("Format LFS failed %d", err);
                    return;
                }
                ADDLOGF_INFO("Formatted LFS");
                err = lfs_mount(&lfs, &cfg);
                if (err){
                    ADDLOGF_ERROR("Mount LFS failed %d", err);
                    return;
                }
                lfs_initialised = 1;
            } else {
                ADDLOGF_INFO("LFS not present - not creating");
            }
        } else {
            // mounted existing
            ADDLOGF_INFO("Mounted existing LFS");
            lfs_initialised = 1;
        }
#ifdef LFS_BOOTCOUNT
        // read current count
        lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

        // update boot count
        boot_count += 1;
        lfs_file_rewind(&lfs, &file);
        lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

        // remember the storage is not updated until the file is closed successfully
        lfs_file_close(&lfs, &file);
        ADDLOGF_INFO("boot count %d", boot_count);
#endif
    }
}

void release_lfs(){
	if (lfs_initialised) {
		lfs_unmount(&lfs);
		lfs_initialised = 0;
	}
}


#if PLATFORM_BEKEN || WINDOWS
// Read a region in a block. Negative error codes are propogated
// to the user.
static int lfs_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_Start;
    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    res = flash_read((char *)buffer, size, startAddr);
    GLOBAL_INT_RESTORE();
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size){
    int res;
    int protect = FLASH_PROTECT_NONE;
    unsigned int startAddr = LFS_Start;
    GLOBAL_INT_DECLARATION();

    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;

    GLOBAL_INT_DISABLE();
    flash_ctrl(CMD_FLASH_SET_PROTECT, &protect);
    flash_ctrl(CMD_FLASH_WRITE_ENABLE, (void *)0);
    res = flash_write((char *)buffer , size, startAddr);
    protect = FLASH_PROTECT_ALL;
    flash_ctrl(CMD_FLASH_SET_PROTECT, &protect);
    GLOBAL_INT_RESTORE();

    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block){
    int res;
    int protect = FLASH_PROTECT_NONE;
    unsigned int startAddr = LFS_Start;
    GLOBAL_INT_DECLARATION();

    startAddr += block*LFS_BLOCK_SIZE;
    GLOBAL_INT_DISABLE();
    flash_ctrl(CMD_FLASH_SET_PROTECT, &protect);
    flash_ctrl(CMD_FLASH_WRITE_ENABLE, (void *)0);
    res = flash_ctrl(CMD_FLASH_ERASE_SECTOR, &startAddr);
    protect = FLASH_PROTECT_ALL;
    flash_ctrl(CMD_FLASH_SET_PROTECT, &protect);
    GLOBAL_INT_RESTORE();
    return res;
}
#elif PLATFORM_BL602

static int lfs_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_Start;
    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;
    res = bl_flash_read(startAddr, (uint8_t *)buffer, size );
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_Start;


    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;

	res = bl_flash_write(startAddr, (uint8_t *)buffer, size );


    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block){
    int res;
    unsigned int startAddr = LFS_Start;
    startAddr += block*LFS_BLOCK_SIZE;
    res =  bl_flash_erase(startAddr, LFS_BLOCK_SIZE);
    return res;
}



#elif PLATFORM_LN882H

static int lfs_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_Start;
    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;
    res = hal_flash_read(startAddr, size, (uint8_t *)buffer);
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_Start;


    startAddr += block*LFS_BLOCK_SIZE;
    startAddr += off;

	res = hal_flash_program(startAddr, size, (uint8_t *)buffer);


    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block){
    int res = LFS_ERR_OK;
    unsigned int startAddr = LFS_Start;
    startAddr += block*LFS_BLOCK_SIZE;
    hal_flash_erase(startAddr, LFS_BLOCK_SIZE);		// it's a void function for LN882H, so no return value
    return res;
}

#elif PLATFORM_ESPIDF

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    int res = LFS_ERR_OK;
    unsigned int startAddr = block * LFS_BLOCK_SIZE;
    startAddr += off;
    res = esp_partition_read(esplfs, startAddr, (uint8_t*)buffer, size);
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    int res = LFS_ERR_OK;

    unsigned int startAddr = block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = esp_partition_write(esplfs, startAddr, (uint8_t*)buffer, size);

    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    int res = LFS_ERR_OK;
    unsigned int startAddr = block * LFS_BLOCK_SIZE;
    res = esp_partition_erase_range(esplfs, startAddr, LFS_BLOCK_SIZE);
    return res;
}


#endif 

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int lfs_sync(const struct lfs_config *c){
    // nothing to do
    return 0;
}

#endif
