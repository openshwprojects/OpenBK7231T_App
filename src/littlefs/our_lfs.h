
/*****************************************************************************
* Implements littlefs in the area used for OTA upgrades
* this means that OTA will always wipe the filesystem
* so we may implement a FS backup and restore around the OTA function.
*
* but, this does have the advantage that we can have >512k of FS space
*
*****************************************************************************/

#include "../obk_config.h"
#include "lfs.h"

#ifdef BK_LITTLEFS

// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x133000
// 512k
#define LFS_BLOCKS_LEN 0x80000
#define LFS_BLOCK_SIZE 0x1000


extern int boot_count;
extern lfs_t lfs;
extern lfs_file_t file;

void init_lfs();
#endif