#ifndef __OTA_H__
#define __OTA_H__

// NOTE: this offset was taken from BkDriverFlash.c
// search for BK_PARTITION_OTA

#if PLATFORM_BK7231T
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#define SIZE_OF_BK_PARTITION_OTA 0x96000
// end is 0x1C8000
#elif PLATFORM_BK7231N
#define START_ADR_OF_BK_PARTITION_OTA 0x12A000
#define SIZE_OF_BK_PARTITION_OTA 0xA6000
// end is 0x1D0000
#else
// TODO
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#define SIZE_OF_BK_PARTITION_OTA 0x96000
#endif
#define END_ADR_BK_PARTITION_OTA (START_ADR_OF_BK_PARTITION_OTA + SIZE_OF_BK_PARTITION_OTA)

// initialise OTA flash starting at startaddr
int init_ota(unsigned int startaddr, unsigned int endaddr);

// add any length of data to OTA
void add_otadata(unsigned char *data, int len);

// finalise OTA flash (write last sector if incomplete)
void close_ota(int nofinalise);

void otarequest(const char *urlin);

int ota_progress();
int ota_total_bytes();

#endif /* __OTA_H__ */

