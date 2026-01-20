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

bl_mtd_info_t lfs_info = { 0 };
bl_mtd_handle_t lfs_handle;
bool lfs_init = false;

#elif PLATFORM_LN882H

#include "../hal/hal_flashConfig.h"
#include "hal/hal_flash.h"
#include "flash_partition_table.h"

#elif PLATFORM_LN8825

#include "../hal/hal_flashConfig.h"
#include "hal/flash.h"
#include "flash_partition_table.h"

#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "esp_partition.h"
esp_partition_t* esplfs = NULL;

#elif PLATFORM_REALTEK

#include "flash_api.h"
#include "device_lock.h"
extern uint8_t flash_size_8720;

#elif PLATFORM_ECR6600

#include "flash.h"

#elif defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "wm_internal_flash.h"
#include "wm_socket_fwup.h"
#include "wm_fwup.h"

#elif PLATFORM_XR809 || PLATFORM_XR872 || PLATFORM_XR806

#include "driver/chip/hal_flash.h"
#include <image/flash.h>

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

#if PLATFORM_REALTEK_NEW
    .lock = lfs_diskio_lock,
    .unlock = lfs_diskio_unlock,
#endif

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = LFS_BLOCK_SIZE,
    .block_count = (LFS_BLOCKS_DEFAULT_LEN/LFS_BLOCK_SIZE),
#if ENABLE_LFS_SPI
	.cache_size = 128,
	.lookahead_size = 128,
#else
	.cache_size = 16,
	.lookahead_size = 16,
#endif
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

#if PLATFORM_ESPIDF || PLATFORM_RTL8720D || PLATFORM_BL602 || PLATFORM_ESP8266 || PLATFORM_REALTEK_NEW
	ADDLOG_ERROR(LOG_FEATURE_CMD, PLATFORM_MCU_NAME" doesn't support changing LFS size");
	return CMD_RES_ERROR;
#endif


    const char *p = args;

#if ENABLE_LFS_SPI
	uint32_t newsize = strtol(args, (char **)&p, 0);
	uint32_t newstart = 0;

	newsize = (newsize / LFS_BLOCK_SIZE)*LFS_BLOCK_SIZE;
#else
	uint32_t newsize = strtol(args, (char **)&p, 0);
	uint32_t newstart = (LFS_BLOCKS_END - newsize);

	newsize = (newsize / LFS_BLOCK_SIZE)*LFS_BLOCK_SIZE;
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
#endif

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
#if ENABLE_LFS_SPI
	uint32_t newstart = 0;
#else
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
	if (newstart < LFS_BLOCKS_START_MIN) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS OUT OF BOUNDS start 0x%X too small", newstart);
		return CMD_RES_ERROR;
	}
	if ((newstart + newsize > LFS_BLOCKS_END) ||
		(newstart + newsize < LFS_BLOCKS_START_MIN)) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS OUT OF BOUNDS end 0x%X too big", newstart + newsize);
		return CMD_RES_ERROR;
	}
#endif

    LFS_Start = newstart;
    LFS_Size = newsize;

#if PLATFORM_RTL8720D

	switch(flash_size_8720)
	{
		case 2:
			LFS_Start = newstart = 0x1F0000;
			LFS_Size = newsize = 0x10000;
			break;
		default:
			LFS_Start = newstart = LFS_BLOCKS_START;
			LFS_Size = newsize = (flash_size_8720 << 20) - LFS_BLOCKS_START;
			break;
	}

#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

	if(esplfs == NULL)
	{
		esplfs = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
		if(esplfs != NULL)
		{
			ADDLOGF_INFO("Partition %s found, size: %i", esplfs->label, esplfs->size);
		}
		else
		{
			return CMD_RES_ERROR;
		}
	}
	LFS_Start = newstart = 0;
	LFS_Size = newsize = esplfs->size;

#elif PLATFORM_BL602

	LFS_Start = newstart = 0; // lfs_info.offset;
	LFS_Size = newsize = lfs_info.size;

#elif PLATFORM_REALTEK_NEW

    switch(flash_size_8720)
    {
        case 4:
            LFS_Start = newstart = 0x1F0000;
            LFS_Size = newsize = 0x24000;
            break;
        default:
            LFS_Start = newstart = 0x400000;
            LFS_Size = newsize = (flash_size_8720 << 20) - LFS_Start;
            break;
    }

#endif

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
static commandResult_t CMD_LFS_MakeDirectory(const void *context, const char *cmd, const char *args, int cmdFlags) {

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	lfs_mkdir(&lfs, Tokenizer_GetArg(0));

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

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	fileName = Tokenizer_GetArg(0);

	lfs_remove(&lfs, fileName);

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
	//cmddetail:"fn":"CMD_LFS_Append","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_append", CMD_LFS_Append, NULL);
	//cmddetail:{"name":"lfs_appendFloat","args":"[FileName][Float]",
	//cmddetail:"descr":"Appends a float to LFS file",
	//cmddetail:"fn":"CMD_LFS_AppendFloat","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendFloat", CMD_LFS_AppendFloat, NULL);
	//cmddetail:{"name":"lfs_appendInt","args":"[FileName][Int]",
	//cmddetail:"descr":"Appends a Int to LFS file",
	//cmddetail:"fn":"CMD_LFS_AppendInt","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendInt", CMD_LFS_AppendInt, NULL);
	//cmddetail:{"name":"lfs_appendLine","args":"[FileName][String]",
	//cmddetail:"descr":"Appends a string to LFS file with a next line marker",
	//cmddetail:"fn":"CMD_LFS_AppendLine","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_appendLine", CMD_LFS_AppendLine, NULL);
	//cmddetail:{"name":"lfs_remove","args":"[FileName]",
	//cmddetail:"descr":"Deletes a LittleFS file",
	//cmddetail:"fn":"CMD_LFS_Remove","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_remove", CMD_LFS_Remove, NULL);
	//cmddetail:{"name":"lfs_write","args":"[FileName][String]",
	//cmddetail:"descr":"Resets a LFS file and writes a new string to it",
	//cmddetail:"fn":"CMD_LFS_Write","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_write", CMD_LFS_Write, NULL);
	//cmddetail:{"name":"lfs_writeLine","args":"[FileName][String]",
	//cmddetail:"descr":"Resets a LFS file and writes a new string to it with newline",
	//cmddetail:"fn":"CMD_LFS_WriteLine","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_writeLine", CMD_LFS_WriteLine, NULL);

	//cmddetail:{"name":"lfs_mkdir","args":"CMD_LFS_MakeDirectory",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_LFS_MakeDirectory","file":"littlefs/our_lfs.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("lfs_mkdir", CMD_LFS_MakeDirectory, NULL);
}


void init_lfs(int create){
    if (!lfs_initialised){
        uint32_t newsize = CFG_GetLFS_Size();

#if ENABLE_LFS_SPI
		uint32_t newstart = 0;
		// do nothing for now?
#else
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
#endif

#if PLATFORM_BL602

		if(!lfs_init)
		{
			int ret = bl_mtd_open(BL_MTD_PARTITION_NAME_ROMFS, &lfs_handle, BL_MTD_OPEN_FLAG_BUSADDR);
			memset(&lfs_info, 0, sizeof(lfs_info));
			if(ret < 0)
			{
				ADDLOGF_WARN("LFS media partition not found %d, trying using OTA partition", ret);

				ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &lfs_handle, BL_MTD_OPEN_FLAG_BACKUP);
				if(ret < 0)
				{
					ADDLOGF_ERROR("OTA partition not found %d, LFS won't be used", ret);
					return;
				}
			}
			bl_mtd_info(lfs_handle, &lfs_info);
			lfs_init = true;
		}
		newstart = 0; // lfs_info.offset;
		newsize = lfs_info.size;
		CFG_SetLFS_Size(lfs_info.size);

#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

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
		CFG_SetLFS_Size(newsize);

#elif PLATFORM_RTL8720D

		switch(flash_size_8720)
		{
			case 2:
				newstart = 0x1F0000;
				newsize = 0x10000;
				break;
			default:
				newstart = LFS_BLOCKS_START;
				newsize = (flash_size_8720 << 20) - LFS_BLOCKS_START;
				break;
		}
		ADDLOGF_INFO("8720D Detected Flash Size: %i MB, adjusting LFS, start: 0x%X, size: 0x%X", flash_size_8720, newstart, newsize);
		CFG_SetLFS_Size(newsize);

#elif PLATFORM_REALTEK_NEW

        switch(flash_size_8720)
        {
            case 4:
                LFS_Start = newstart = 0x1F0000;
                LFS_Size = newsize = 0x24000;
                break;
            default:
                LFS_Start = newstart = 0x400000;
                LFS_Size = newsize = (flash_size_8720 << 20) - LFS_Start;
                break;
        }
        ADDLOGF_INFO("8721DA/8720E Detected Flash Size: %i MB, adjusting LFS, start: 0x%X, size: 0x%X", flash_size_8720, newstart, newsize);
        CFG_SetLFS_Size(newsize);

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

#if ENABLE_LFS_SPI


void LFS_SPI_Flash_Write(int adr, const byte *data, int cnt);
void LFS_SPI_Flash_Read(int adr, int cnt, byte *data);

void LFS_SPI_Flash_EraseSector(int addr);

// Read a region in a block. Negative error codes are propogated
// to the user.
static int lfs_read(const struct lfs_config *c, lfs_block_t block,
	lfs_off_t off, void *buffer, lfs_size_t size) {
	int res;
	unsigned int startAddr = 0;
	startAddr = block * LFS_BLOCK_SIZE;
	startAddr += off;

	LFS_SPI_Flash_Read(startAddr, size, buffer);

	return LFS_ERR_OK;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
	lfs_off_t off, const void *buffer, lfs_size_t size) {
	unsigned int startAddr;

	startAddr = block * LFS_BLOCK_SIZE;
	startAddr += off;

	LFS_SPI_Flash_Write(startAddr, buffer, size);

	return LFS_ERR_OK;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
	unsigned int startAddr;

	startAddr = block * LFS_BLOCK_SIZE;

	LFS_SPI_Flash_EraseSector(startAddr);

	return 0;
}

#elif PLATFORM_BEKEN || WINDOWS
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
	lfs_off_t off, void *buffer, lfs_size_t size)
{
	if(!lfs_init) return 0;
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	//res = bl_flash_read(startAddr, (uint8_t*)buffer, size);
	res = bl_mtd_read(lfs_handle, startAddr, size, (uint8_t*)buffer);
	return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config *c, lfs_block_t block,
	lfs_off_t off, const void *buffer, lfs_size_t size)
{
	if(!lfs_init) return 0;
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	//res = bl_flash_write(startAddr, (uint8_t*)buffer, size);
	res = bl_mtd_write(lfs_handle, startAddr, size, (uint8_t*)buffer);
	return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config *c, lfs_block_t block)
{
	if(!lfs_init) return 0;
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;

	//res = bl_flash_erase(startAddr, LFS_BLOCK_SIZE);
	res = bl_mtd_erase(lfs_handle, startAddr, LFS_BLOCK_SIZE);
	return res;
}

#elif PLATFORM_LN882H || PLATFORM_LN8825

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

#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

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

#elif PLATFORM_TR6260

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = hal_spiflash_read(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = hal_spiflash_write(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;

    res = hal_spiflash_erase(startAddr, LFS_BLOCK_SIZE);
    return res;
}

#elif PLATFORM_REALTEK

extern flash_t flash;

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    device_mutex_lock(RT_DEV_LOCK_FLASH);
    flash_stream_read(&flash, startAddr, size, (uint8_t*)buffer);
    device_mutex_unlock(RT_DEV_LOCK_FLASH);
    return LFS_ERR_OK;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    device_mutex_lock(RT_DEV_LOCK_FLASH);
    flash_stream_write(&flash, startAddr, size, (uint8_t*)buffer);
    device_mutex_unlock(RT_DEV_LOCK_FLASH);
    return LFS_ERR_OK;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;

    device_mutex_lock(RT_DEV_LOCK_FLASH);
    flash_erase_sector(&flash, startAddr);
    device_mutex_unlock(RT_DEV_LOCK_FLASH);
    return LFS_ERR_OK;
}

#elif PLATFORM_ECR6600

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = drv_spiflash_read(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = drv_spiflash_write(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;

    res = drv_spiflash_erase(startAddr, LFS_BLOCK_SIZE);
    return res;
}

#elif PLATFORM_W800 || PLATFORM_W600

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = tls_fls_read(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;
    startAddr += off;

    res = tls_fls_write(startAddr, (unsigned char*)buffer, size);
    return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
    int res;

    unsigned int startAddr = LFS_Start;
    startAddr += block * LFS_BLOCK_SIZE;

    res = tls_fls_erase(startAddr / LFS_BLOCK_SIZE);
    return res;
}

#elif PLATFORM_XRADIO

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
	lfs_off_t off, void* buffer, lfs_size_t size)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	res = flash_rw(0, startAddr, buffer, size, 0);
	if(res == 0) res = LFS_ERR_IO;
	res = LFS_ERR_OK;
	return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
	lfs_off_t off, const void* buffer, lfs_size_t size)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	res = flash_rw(0, startAddr, buffer, size, 1);
	if(res == 0) res = LFS_ERR_IO;
	res = LFS_ERR_OK;
	return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;

	res = flash_erase(0, startAddr, LFS_BLOCK_SIZE);
	if(res != 0) res = LFS_ERR_IO;
	res = LFS_ERR_OK;
	return res;
}

#elif PLATFORM_TXW81X || PLATFORM_RDA5981

static int lfs_read(const struct lfs_config* c, lfs_block_t block,
	lfs_off_t off, void* buffer, lfs_size_t size)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	HAL_FlashRead(buffer, size, startAddr);
	res = LFS_ERR_OK;
	return res;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_write(const struct lfs_config* c, lfs_block_t block,
	lfs_off_t off, const void* buffer, lfs_size_t size)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;
	startAddr += off;

	HAL_FlashWrite(buffer, size, startAddr);
	res = LFS_ERR_OK;
	return res;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int lfs_erase(const struct lfs_config* c, lfs_block_t block)
{
	int res;

	unsigned int startAddr = LFS_Start;
	startAddr += block * LFS_BLOCK_SIZE;

	HAL_FlashEraseSector(startAddr);
	res = LFS_ERR_OK;
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
