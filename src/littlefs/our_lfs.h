
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

#if ENABLE_LFS_SPI

#define LFS_BLOCKS_START 0x0
#define LFS_BLOCKS_START_MIN 0x0
#define LFS_BLOCKS_END 0x200000

#elif WINDOWS

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

#define LFS_BLOCKS_START 0x0
#define LFS_BLOCKS_START_MIN 0x0
#define LFS_BLOCKS_END 0x80000000
#define LFS_BLOCKS_MAX_LEN 0x80000000

#elif PLATFORM_LN882H || PLATFORM_LN8825
// start0x1000 after OTA addr (OTA, start_addr: 0x00133000, size_KB: 0x000AA000)
// (OTA, start_addr: 0x00133000, size_KB: 0x000AA000)
// --> OTA-start + 0x1000 = 0x00133000 + 0x1000 = 0x00134000
// --> OTA-end = 0x00133000 +  0x000AA000 = 0x001DD000
#define LFS_BLOCKS_START 0x00134000
#define LFS_BLOCKS_START_MIN 0x00134000
// end media partition
#define LFS_BLOCKS_END 0x001DD000

#elif PLATFORM_ESPIDF || PLATFORM_ESP8266

#define LFS_BLOCKS_START 0x0
#define LFS_BLOCKS_START_MIN 0x0
#define LFS_BLOCKS_END 0x80000000
#define LFS_BLOCKS_MAX_LEN 0x80000000

#elif PLATFORM_TR6260

#define LFS_BLOCKS_START 0xB6000
#define LFS_BLOCKS_START_MIN 0xB6000
#define LFS_BLOCKS_END 0xB6000 + 0x36000

#elif PLATFORM_RTL87X0C

#define LFS_BLOCKS_START 0x1D0000
#define LFS_BLOCKS_START_MIN 0x1D0000
#define LFS_BLOCKS_END LFS_BLOCKS_START + 0x20000

#elif PLATFORM_RTL8710B

#define LFS_BLOCKS_START 0x19D000
#define LFS_BLOCKS_START_MIN 0x19D000
#define LFS_BLOCKS_END 0x1E0000

#elif PLATFORM_RTL8710A

#define LFS_BLOCKS_START 0x300000
#define LFS_BLOCKS_START_MIN 0x300000
#define LFS_BLOCKS_END 0x380000

#elif PLATFORM_RTL8720D

#define LFS_BLOCKS_START 0x3C6000
#define LFS_BLOCKS_START_MIN 0x3C6000
#define LFS_BLOCKS_END 0x80000000
#define LFS_BLOCKS_MAX_LEN 0x80000000

#elif PLATFORM_REALTEK_NEW

#define LFS_BLOCKS_START 0x1F0000
#define LFS_BLOCKS_START_MIN 0x1F0000
#define LFS_BLOCKS_END 0x80000000
#define LFS_BLOCKS_MAX_LEN 0x80000000

#elif PLATFORM_ECR6600

#define LFS_BLOCKS_START 0x1D5000
#define LFS_BLOCKS_START_MIN 0x1D5000
#define LFS_BLOCKS_END 0x1D5000 + 0x22000

#elif PLATFORM_W600

#define LFS_BLOCKS_START 0xE8000
#define LFS_BLOCKS_START_MIN 0xE8000
#define LFS_BLOCKS_END 0xF0000

#elif PLATFORM_W800

// tuya config offset
#define LFS_BLOCKS_START 0x1C0000
#define LFS_BLOCKS_START_MIN 0x1C0000
#define LFS_BLOCKS_END 0x1DB000

#elif PLATFORM_XR809

#define LFS_BLOCKS_START 0x1F0000
#define LFS_BLOCKS_START_MIN 0x1F0000
#define LFS_BLOCKS_END 0x200000

#elif PLATFORM_XR806

#define LFS_BLOCKS_START 0x1C8000
#define LFS_BLOCKS_START_MIN 0x1C8000
#define LFS_BLOCKS_END 0x1F0000

#elif PLATFORM_XR872

#define LFS_BLOCKS_START 0xF7000
#define LFS_BLOCKS_START_MIN 0xF7000
#define LFS_BLOCKS_END 0xFF000

#elif PLATFORM_TXW81X

#define LFS_BLOCKS_START 0xF5000
#define LFS_BLOCKS_START_MIN 0xF5000
#define LFS_BLOCKS_END 0x100000

#elif PLATFORM_RDA5981

#define LFS_BLOCKS_START 0xE0000
#define LFS_BLOCKS_START_MIN 0xE0000
#define LFS_BLOCKS_END 0xF4000

#else
// TODO
// start 0x1000 after OTA addr
#define LFS_BLOCKS_START 0x12B000
#define LFS_BLOCKS_START_MIN 0x12B000
// end of OTA flash
#define LFS_BLOCKS_END 0x1AB000
#endif

#if ENABLE_LITTLEFS

#if PLATFORM_REALTEK_NEW
#include "littlefs_adapter.h"
#else
#include "lfs.h"
#endif

// 512k MAX - i.e. no more that 0x80000
// 0x8000 = 32k
#define LFS_BLOCKS_MIN_LEN 0x4000
#ifndef LFS_BLOCKS_MAX_LEN
#define LFS_BLOCKS_MAX_LEN (LFS_BLOCKS_END - LFS_BLOCKS_START)
#endif
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
