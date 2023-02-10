
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

#if ENABLE_LITTLEFS

// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x133000
#define LFS_BLOCKS_START_MIN 0x133000

// end of OTA flash
#define LFS_BLOCKS_END 0x1B3000
// 512k MAX - i.e. no more that 0x80000
// 0x8000 = 32k
#define LFS_BLOCKS_MIN_LEN 0x4000
#define LFS_BLOCKS_MAX_LEN 0x80000
#define LFS_BLOCKS_DEFAULT_LEN 0x8000

#define LFS_BLOCK_SIZE 0x1000


extern int boot_count;
extern lfs_t lfs;
extern lfs_file_t file;
extern uint32_t LFS_Start;

void LFSAddCmds();
void init_lfs(int create);
void release_lfs();
int lfs_present();
#endif