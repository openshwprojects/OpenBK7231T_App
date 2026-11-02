#ifndef __OTA_H__
#define __OTA_H__

// NOTE: this offset was taken from BkDriverFlash.c
// search for BK_PARTITION_OTA

#if PLATFORM_BK7231T
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#elif PLATFORM_BK7231N
#define START_ADR_OF_BK_PARTITION_OTA 0x12A000
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

/// @brief Called once an HTTP OTA download has ended and what it received is
/// written, just before the restart that applies it. Does nothing unless a
/// driver defines it: a driver can leave the device as the new firmware should
/// find it. The download can end early without an error being seen, so a
/// hook that acts on it checks the image it was to write.
/// @param ok 1 when no error was seen during the download
void HAL_OTA_BeforeReboot(int ok);

#endif /* __OTA_H__ */

