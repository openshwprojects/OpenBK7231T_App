/*****************************************************************************
* Implements littlefs in the area used for OTA upgrades
* this means that OTA will always wipe the filesystem
* so we may implement a FS backup and restore around the OTA function.
*
* but, this does have the advantage that we can have >512k of FS space
*
*****************************************************************************/

#include "../new_common.h"
#include "typedef.h"
#include "our_lfs.h"
#include "../logging/logging.h"
#include "flash_pub.h"


//https://github.com/littlefs-project/littlefs


#ifdef BK_LITTLEFS
// define the feature ADDLOGF_XXX will use
#define LOG_FEATURE LOG_FEATURE_LFS

// variables used by the filesystem
int lfs_initialised = 0;
lfs_t lfs;
//lfs_file_t file;

// from flash.c
extern UINT32 flash_read(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_write(char *user_buf, UINT32 count, UINT32 address);
extern UINT32 flash_ctrl(UINT32 cmd, void *parm);

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

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = lfs_read,
    .prog  = lfs_write,
    .erase = lfs_erase,
    .sync  = lfs_sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = LFS_BLOCK_SIZE,
    .block_count = (LFS_BLOCKS_LEN/LFS_BLOCK_SIZE),
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

int lfs_present(){
    return lfs_initialised;
}


void init_lfs(int create){
    if (!lfs_initialised){
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
    lfs_unmount(&lfs);
    lfs_initialised = 0;
}


// Read a region in a block. Negative error codes are propogated
// to the user.
static int lfs_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size){
    int res;
    unsigned int startAddr = LFS_BLOCKS_START;
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
    unsigned int startAddr = LFS_BLOCKS_START;
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
    unsigned int startAddr = LFS_BLOCKS_START;
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

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int lfs_sync(const struct lfs_config *c){
    // nothing to do
    return 0;
}

#endif