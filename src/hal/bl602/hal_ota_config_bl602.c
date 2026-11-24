#if PLATFORM_BL602

#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "hal_ota_config_bl602.h"
#include <easyflash.h>
#include <string.h>

#define KV_KEY_OTA_CFG_CRC "obkcfg_crc"

__attribute__((used, section(".rodata")))
const obk_ota_config_v1_t obk_ota_config_v1 = {
    .magic     = OBK_OTA_CONFIG_MAGIC,
    .version   = OBK_OTA_CONFIG_VERSION,
    .flags     = 0,
    .total_len = sizeof(obk_ota_config_v1_t),
};

extern void InitEasyFlashIfNeeded(void);

static uint32_t crc32_compute(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1));
    }
    return ~crc;
}

static uint32_t obk_ota_config_crc32(const obk_ota_config_v1_t *cfg) {
    obk_ota_config_v1_t tmp;
    memcpy(&tmp, cfg, sizeof(tmp));
    tmp.crc32 = 0;
    return crc32_compute((const uint8_t *)&tmp, sizeof(tmp));
}

static void obk_ota_config_read(obk_ota_config_v1_t *out) {
    volatile const uint8_t *src = (volatile const uint8_t *)&obk_ota_config_v1;
    uint8_t *dst = (uint8_t *)out;
    for (size_t i = 0; i < sizeof(*out); i++) {
        dst[i] = src[i];
    }
}

void OBK_OtaConfig_TryImport(void) {
    obk_ota_config_v1_t cfg_storage;
    const obk_ota_config_v1_t *cfg = &cfg_storage;

    obk_ota_config_read(&cfg_storage);

    addLogAdv(LOG_WARN, LOG_FEATURE_CFG,
        "OtaCfg: block v=%d flags=0x%02x len=%d crc=0x%08x",
        cfg->version, cfg->flags, cfg->total_len, cfg->crc32);

    if (memcmp(cfg->magic, OBK_OTA_CONFIG_MAGIC "\0", 8) != 0) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: bad magic, skip");
        return;
    }
    if (cfg->version != OBK_OTA_CONFIG_VERSION) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: unknown version %d, skip", cfg->version);
        return;
    }
    if (!(cfg->flags & OBK_OTA_FLAG_VALID)) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: not marked valid (flags=0x%02x), skip", cfg->flags);
        return;
    }
    if (cfg->total_len != sizeof(obk_ota_config_v1_t)) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: wrong total_len %d (expect %d), skip",
            cfg->total_len, (int)sizeof(obk_ota_config_v1_t));
        return;
    }

    uint32_t computed = obk_ota_config_crc32(cfg);
    if (computed != cfg->crc32) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: CRC mismatch (got 0x%08x expect 0x%08x), skip",
            computed, cfg->crc32);
        return;
    }
    if (cfg->ssid_len == 0) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: empty SSID, skip import");
        return;
    }

    InitEasyFlashIfNeeded();
    uint32_t saved_crc = 0;
    size_t read_len = 0;
    ef_get_env_blob(KV_KEY_OTA_CFG_CRC, &saved_crc, sizeof(saved_crc), &read_len);
    if (read_len == sizeof(saved_crc) && saved_crc == cfg->crc32) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: already imported (crc=0x%08x), skip", saved_crc);
        return;
    }

    if (g_cfg.wifi_ssid[0] != 0) {
        addLogAdv(LOG_WARN, LOG_FEATURE_CFG, "OtaCfg: WiFi already set ('%s'), skip import", g_cfg.wifi_ssid);
        ef_set_env_blob(KV_KEY_OTA_CFG_CRC, (void *)&cfg->crc32, sizeof(cfg->crc32));
        return;
    }

    if (cfg->ssid_len > 0 && cfg->ssid_len < sizeof(g_cfg.wifi_ssid)) {
        memcpy(g_cfg.wifi_ssid, cfg->ssid, cfg->ssid_len);
        g_cfg.wifi_ssid[cfg->ssid_len] = 0;
    }
    if (cfg->pass_len > 0 && cfg->pass_len < sizeof(g_cfg.wifi_pass)) {
        memcpy(g_cfg.wifi_pass, cfg->pass, cfg->pass_len);
        g_cfg.wifi_pass[cfg->pass_len] = 0;
    }
    if (cfg->device_name[0] != 0) {
        CFG_SetShortDeviceName(cfg->device_name);
        CFG_SetDeviceName(cfg->device_name);
    }

    ef_set_env_blob(KV_KEY_OTA_CFG_CRC, (void *)&cfg->crc32, sizeof(cfg->crc32));
    g_cfg_pendingChanges++;

    addLogAdv(LOG_WARN, LOG_FEATURE_CFG,
        "OtaCfg: IMPORTED ssid='%s' pass_len=%d name='%s'",
        g_cfg.wifi_ssid, cfg->pass_len, g_cfg.shortDeviceName);
}

#endif // PLATFORM_BL602
