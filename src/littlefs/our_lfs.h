
/*****************************************************************************
* Implements littlefs in the area used for OTA upgrades
* this means that OTA will always wipe the filesystem
* so we may implement a FS backup and restore around the OTA function.
*
* but, this does have the advantage that we can have >512k of FS space
*
*****************************************************************************/

#ifndef __OUR_LFS_H__
#define __OUR_LFS_H__


#include "../obk_config.h"

// we need that even if LFS is disabled

#if WINDOWS

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

#elif PLATFORM_BK7231T

// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x133000
#define LFS_BLOCKS_START_MIN 0x133000
// end of OTA flash
#define LFS_BLOCKS_END 0x1B3000
#elif PLATFORM_BK7231N
// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x12B000
#define LFS_BLOCKS_START_MIN 0x12B000
// end of OTA flash
#define LFS_BLOCKS_END 0x1D0000
#elif PLATFORM_BL602
// start media partition in bldevcube 1.4.8 partition config
#define LFS_BLOCKS_START 0x192000
#define LFS_BLOCKS_START_MIN 0x192000
// end media partition
#define LFS_BLOCKS_END 0x1E9000

#elif PLATFORM_LN882H
// start0x1000 after OTA addr (OTA, start_addr: 0x00133000, size_KB: 0x000AA000)
// (OTA, start_addr: 0x00133000, size_KB: 0x000AA000)
// --> OTA-start + 0x1000 = 0x00133000 + 0x1000 = 0x00134000
// --> OTA-end = 0x00133000 +  0x000AA000 = 0x001DD000
#define LFS_BLOCKS_START 0x00134000
#define LFS_BLOCKS_START_MIN 0x00134000
// end media partition
#define LFS_BLOCKS_END 0x001DD000

#elif PLATFORM_ESPIDF

#define LFS_BLOCKS_START 0x0
#define LFS_BLOCKS_START_MIN 0x0
#define LFS_BLOCKS_END 0x50000

#else
// TODO
// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x12B000
#define LFS_BLOCKS_START_MIN 0x12B000
// end of OTA flash
#define LFS_BLOCKS_END 0x1AB000
#endif

#if ENABLE_LITTLEFS

#include "lfs.h"

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
#endif
