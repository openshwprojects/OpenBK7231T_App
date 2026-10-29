#ifndef __OTA_H__
#define __OTA_H__

#if PLATFORM_BEKEN
// The Beken SDK keeps one partition table per flash size (BkDriverFlash.c) and
// selects it by CFG_FLASH_SELECTION_TYPE, so the OTA slot is not one address:
// 0x132000 on the 2 MB table, 0x286000 on the 4 MB one. Read it from the table
// the firmware was built with rather than hard-coding a layout.
unsigned int HAL_Beken_OTAPartitionStart(void);
// The flash size the partition table implies (end of the highest partition,
// rounded up to a power of two). The flash-ID table is not used for this: it
// does not know every part and answers 2 MB for the ones it does not.
unsigned int HAL_Beken_FlashLength(void);
#define START_ADR_OF_BK_PARTITION_OTA (HAL_Beken_OTAPartitionStart())
#else
// TODO
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#endif

/// @brief Handle OTA request. Only used for Beken SDK.
/// @param urlin 
void otarequest(const char *urlin);


/***** SDK independent code from this point. ******/

/// @brief Indicates current OTA progress status. A non -ve value indicates active OTA.
/// @return 
int OTA_GetProgress();

/// @brief Reset OTA progress status to -1. This can be called from other SDKs.
/// @param value 
void OTA_ResetProgress();

/// @brief Increment OTA progress status. This can be called from other SDKs.
/// @param value 
void OTA_IncrementProgress(int value);

/// @brief Get total OTA size.
/// @return 
int OTA_GetTotalBytes();

/// @brief Set total OTA size. This can be called from other SDKs.
/// @param value 
void OTA_SetTotalBytes(int value);

int HAL_FlashRead(char*buffer, int readlen, int startaddr);

#endif /* __OTA_H__ */

