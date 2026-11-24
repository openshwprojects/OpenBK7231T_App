#pragma once
#if PLATFORM_BL602

#include <stdint.h>

#define OBK_OTA_CONFIG_MAGIC "OBKCFG1"
#define OBK_OTA_CONFIG_VERSION 1

#define OBK_OTA_FLAG_VALID    0x01
#define OBK_OTA_FLAG_ONE_SHOT 0x02

typedef struct __attribute__((packed)) {
    char     magic[8];       // "OBKCFG1\0"
    uint8_t  version;        // 1
    uint8_t  flags;          // OBK_OTA_FLAG_*
    uint16_t total_len;      // sizeof(obk_ota_config_v1_t)
    uint32_t crc32;          // CRC32 of whole struct with this field = 0

    uint8_t  ssid_len;
    uint8_t  pass_len;
    uint8_t  reserved0[2];

    char     ssid[32];
    char     pass[64];
    char     device_name[32];
    uint8_t  reserved1[128];
} obk_ota_config_v1_t;

extern const obk_ota_config_v1_t obk_ota_config_v1;

void OBK_OtaConfig_TryImport(void);

#endif // PLATFORM_BL602
