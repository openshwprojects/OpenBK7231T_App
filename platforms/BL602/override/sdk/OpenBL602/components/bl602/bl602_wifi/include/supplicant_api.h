/*
 * Copyright (c) 2016-2022 Bouffalolab.
 *
 * This file is part of
 *     *** Bouffalolab Software Dev Kit ***
 *      (see www.bouffalolab.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Bouffalo Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SUPPLICANT_API_H
#define SUPPLICANT_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BL_SAE_INVALID_PACKET -8

// XXX: this struct should be compatible with internal(FULLMAC)
typedef struct bl_wifi_timer {
    void *_storage_0[3];
    uint32_t exp_time;
} bl_wifi_timer_t;

/** Configuration structure for Protected Management Frame */
typedef struct {
    bool capable;            /**< Advertizes support for Protected Management Frame. Device will prefer to connect in PMF mode if other device also advertizes PMF capability. */
    bool required;           /**< Advertizes that Protected Management Frame is required. Device will not associate to non-PMF capable devices. */
} wifi_pmf_config_t;

typedef enum {
    WIFI_CIPHER_TYPE_NONE = 0,   /**< the cipher type is none */
    WIFI_CIPHER_TYPE_WEP40,      /**< the cipher type is WEP40 */
    WIFI_CIPHER_TYPE_WEP104,     /**< the cipher type is WEP104 */
    WIFI_CIPHER_TYPE_TKIP,       /**< the cipher type is TKIP */
    WIFI_CIPHER_TYPE_CCMP,       /**< the cipher type is CCMP */
    WIFI_CIPHER_TYPE_TKIP_CCMP,  /**< the cipher type is TKIP and CCMP */
    WIFI_CIPHER_TYPE_AES_CMAC128,/**< the cipher type is AES-CMAC-128 */
    WIFI_CIPHER_TYPE_SMS4,       /**< the cipher type is SMS4 */
    WIFI_CIPHER_TYPE_UNKNOWN,    /**< the cipher type is unknown */
} wifi_cipher_type_t;

typedef enum {
    WIFI_APPIE_WPA_RSN = 0, // WPA and/or RSN
    WIFI_APPIE_WPS_PR,
    WIFI_APPIE_WPS_AR,
    WIFI_APPIE_CUSTOM,
    WIFI_APPIE_MAX,
} wifi_appie_t;

typedef enum wpa_alg {
    WIFI_WPA_ALG_NONE   = 0,
    WIFI_WPA_ALG_WEP40  = 1,
    WIFI_WPA_ALG_TKIP   = 2,
    WIFI_WPA_ALG_CCMP   = 3,
    WIFI_WAPI_ALG_SMS4  = 4,
    WIFI_WPA_ALG_WEP104 = 5,
    WIFI_WPA_ALG_WEP    = 6,
    WIFI_WPA_ALG_IGTK   = 7,
    WIFI_WPA_ALG_PMK    = 8,
    WIFI_WPA_ALG_GCMP   = 9,
} wpa_alg_t;

typedef struct {
    int proto;
    int pairwise_cipher;
    int group_cipher;
    int key_mgmt;
    int capabilities;
    size_t num_pmkid;
    const uint8_t *pmkid;
    int mgmt_group_cipher;
} wifi_wpa_ie_t;

#ifndef BIT
#define BIT(x) (1U << (x))
#endif

#define WPA_PROTO_WPA BIT(0)
#define WPA_PROTO_RSN BIT(1)

#define WPA_KEY_MGMT_IEEE8021X BIT(0)
#define WPA_KEY_MGMT_PSK BIT(1)
#define WPA_KEY_MGMT_NONE BIT(2)
#define WPA_KEY_MGMT_IEEE8021X_NO_WPA BIT(3)
#define WPA_KEY_MGMT_WPA_NONE BIT(4)
#define WPA_KEY_MGMT_FT_IEEE8021X BIT(5)
#define WPA_KEY_MGMT_FT_PSK BIT(6)
#define WPA_KEY_MGMT_IEEE8021X_SHA256 BIT(7)
#define WPA_KEY_MGMT_PSK_SHA256 BIT(8)
#define WPA_KEY_MGMT_WPS BIT(9)
#define WPA_KEY_MGMT_SAE BIT(10)
#define WPA_KEY_MGMT_FT_SAE BIT(11)
#define WPA_KEY_MGMT_WAPI_PSK BIT(12)
#define WPA_KEY_MGMT_WAPI_CERT BIT(13)
#define WPA_KEY_MGMT_CCKM BIT(14)
#define WPA_KEY_MGMT_OSEN BIT(15)
#define WPA_KEY_MGMT_IEEE8021X_SUITE_B BIT(16)
#define WPA_KEY_MGMT_IEEE8021X_SUITE_B_192 BIT(17)

typedef enum {
    NONE_AUTH            = 0x01,
    WPA_AUTH_UNSPEC      = 0x02,
    WPA_AUTH_PSK         = 0x03,
    WPA2_AUTH_ENT        = 0x04,
    WPA2_AUTH_PSK        = 0x05,
    WPA_AUTH_CCKM        = 0x06,
    WPA2_AUTH_CCKM       = 0x07,
    WPA2_AUTH_PSK_SHA256 = 0x08,
    WPA3_AUTH_PSK        = 0x09,
    WPA2_AUTH_ENT_SHA256 = 0x0a,
    WAPI_AUTH_PSK        = 0x0b,
    WAPI_AUTH_CERT       = 0x0c,
    WPA2_AUTH_INVALID    = 0x0d,
} wpa_auth_mode_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,         /**< authenticate mode : open */
    WIFI_AUTH_WEP,              /**< authenticate mode : WEP */
    WIFI_AUTH_WPA_PSK,          /**< authenticate mode : WPA_PSK */
    WIFI_AUTH_WPA2_PSK,         /**< authenticate mode : WPA2_PSK */
    WIFI_AUTH_WPA_WPA2_PSK,     /**< authenticate mode : WPA_WPA2_PSK */
    WIFI_AUTH_WPA2_ENTERPRISE,  /**< authenticate mode : WPA2_ENTERPRISE */
    WIFI_AUTH_WPA3_PSK,         /**< authenticate mode : WPA3_PSK */
    WIFI_AUTH_WPA2_WPA3_PSK,    /**< authenticate mode : WPA2_WPA3_PSK */
    WIFI_AUTH_WAPI_PSK,         /**< authenticate mode : WAPI_PSK */
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

struct wifi_ssid {
    int len;
    uint8_t ssid[32];
};


#define WPA_AES_KEY_LEN    16
#define WPA_TKIP_KEY_LEN   32
#define WPA_WEP104_KEY_LEN 13
#define WPA_WEP40_KEY_LEN  5

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

// XXX order matters!
typedef enum {
    SEC_PROTO_NONE,
    SEC_PROTO_WEP_STATIC,
    SEC_PROTO_WPA,
    SEC_PROTO_WPA2,
    SEC_PROTO_WPA3,
    SEC_PROTO_WAPI,
} sec_proto_t;

typedef enum {
    EAPOL_FRAME_4_1,
    EAPOL_FRAME_4_2,
    EAPOL_FRAME_4_3,
    EAPOL_FRAME_4_4,
    EAPOL_FRAME_2_1,
    EAPOL_FRAME_2_2,
} eapol_frame_id_t;

typedef struct {
    uint8_t vif_idx;
    uint8_t sta_idx;
    uint8_t mac[ETH_ALEN];
    uint8_t bssid[ETH_ALEN];
    struct wifi_ssid ssid;
    sec_proto_t proto;
    uint16_t key_mgmt;
    uint8_t pairwise_cipher;
    uint8_t group_cipher;
    char passphrase[64 + 1];
    bool pmf_required;
    uint8_t mgmt_group_cipher; // should always be WPA_CIPHER_AES_128_CMAC if PMFR=1
} wifi_connect_parm_t;

typedef struct {
    uint8_t vif_idx;
    uint8_t mac[ETH_ALEN];
    struct wifi_ssid ssid;
    wifi_auth_mode_t auth_mode;
    wifi_cipher_type_t pairwise_cipher;
    char passphrase[64 + 1];
} wifi_ap_parm_t;

struct wpa_funcs {
    bool (*wpa_sta_init)(void);
    bool (*wpa_sta_deinit)(void);
    void (*wpa_sta_config)(wifi_connect_parm_t *bssid);
    void (*wpa_sta_connect)(wifi_connect_parm_t *bssid);
    void (*wpa_sta_disconnected_cb)(uint8_t reason_code);
    int (*wpa_sta_rx_eapol)(uint8_t *src_addr, uint8_t *buf, uint32_t len);
    void *(*wpa_ap_init)(wifi_ap_parm_t *parm);
    bool (*wpa_ap_deinit)(void *data);
    bool (*wpa_ap_join)(void **sm, uint8_t *mac, uint8_t *wpa_ie, uint8_t wpa_ie_len);
    void (*wpa_ap_sta_associated)(void *wpa_sm, uint8_t sta_idx);
    bool (*wpa_ap_remove)(void *sm);
    bool (*wpa_ap_rx_eapol)(void *hapd_data, void *sm, uint8_t *data, size_t data_len);
    int (*wpa_parse_wpa_ie)(const uint8_t *wpa_ie, size_t wpa_ie_len, wifi_wpa_ie_t *data);
    void (*wpa_reg_diag_tlv_cb)(void *tlv_pack_cb);
    // XXX MIC failure countermeasure disabled for now
    /* int (*wpa_michael_mic_failure)(uint16_t is_unicast); */
    uint8_t *(*wpa3_build_sae_msg)(uint8_t *bssid, uint8_t *mac, uint8_t *passphrase, uint32_t sae_msg_type, size_t *sae_msg_len);
    int (*wpa3_parse_sae_msg)(uint8_t *buf, size_t len, uint32_t type, uint16_t status);
    void (*wpa3_clear_sae)(void);
};

struct wps_scan_ie {
    uint8_t    *bssid;
    uint8_t    chan;
    uint16_t   capinfo;
    uint8_t    *ssid;
    uint8_t    ssid_len;
    uint8_t    *wpa;
    uint8_t    *rsn;
    uint8_t    *wps;
};

struct wps_funcs {
    bool (*wps_parse_scan_result)(struct wps_scan_ie *scan);
    int (*wps_sm_rx_eapol)(uint8_t *src_addr, uint8_t *buf, uint32_t len);
    int (*wps_config)(uint8_t vif_idx, uint8_t sta_idx);
    int (*wps_start_pending)(void);
};

typedef enum wps_status {
    WPS_STATUS_DISABLE = 0,
    WPS_STATUS_SCANNING,
    WPS_STATUS_PENDING,
    WPS_STATUS_SUCCESS,
    WPS_STATUS_MAX,
} wps_status_t;

typedef void bl_wifi_timer_func_t(void *arg);
typedef void(*wifi_tx_cb_t)(void *);

int bl_wifi_ap_deauth_internal(uint8_t vif_idx, uint8_t sta_idx, uint32_t reason);
bool bl_wifi_auth_done_internal(uint8_t sta_idx, uint16_t reason_code);
void *bl_wifi_get_hostap_private_internal(void);
void bl_wifi_timer_arm(bl_wifi_timer_t *ptimer, uint32_t time_ms, bool repeat_flag);
void bl_wifi_timer_disarm(bl_wifi_timer_t *ptimer);
void bl_wifi_timer_done(bl_wifi_timer_t *ptimer);
void bl_wifi_timer_setfn(bl_wifi_timer_t *ptimer, bl_wifi_timer_func_t *pfunction, void *parg);
int bl_wifi_set_appie_internal(uint8_t vif_idx, wifi_appie_t type, uint8_t *ie, uint16_t len, bool sta);
int bl_wifi_unset_appie_internal(uint8_t vif_idx, wifi_appie_t type);
bool bl_wifi_wpa_ptk_init_done_internal(uint8_t sta_idx);
int bl_wifi_register_wpa_cb_internal(const struct wpa_funcs *cb);
int bl_wifi_unregister_wpa_cb_internal(void);
bool bl_wifi_skip_supp_pmkcaching(void);
int bl_wifi_sta_update_ap_info_internal(void);
uint8_t bl_wifi_sta_set_reset_param_internal(uint8_t reset_flag);
bool bl_wifi_sta_is_ap_notify_completed_rsne_internal(void);
bool bl_wifi_sta_is_running_internal(void);
int bl_wifi_set_ap_key_internal(uint8_t vif_idx, uint8_t sta_idx, wpa_alg_t alg, int key_idx, uint8_t *key, size_t key_len, bool pairwise);
int bl_wifi_set_sta_key_internal(uint8_t vif_idx, uint8_t sta_idx, wpa_alg_t alg, int key_idx, int set_tx, uint8_t *seq, size_t seq_len, uint8_t *key, size_t key_len, bool pairwise);

int bl_wifi_set_igtk_internal(uint8_t vif_idx, uint8_t sta_idx, uint16_t key_idx, const uint8_t *pn, const uint8_t *key);
int bl_wifi_get_sta_gtk(uint8_t vif_idx, uint8_t *out_buf, uint8_t *out_len);
int bl_wifi_get_assoc_bssid_internal(uint8_t vif_idx, uint8_t *bssid);
int bl_wifi_set_wps_cb_internal(const struct wps_funcs *wps_cb);
wps_status_t bl_wifi_get_wps_status_internal(void);
void bl_wifi_set_wps_status_internal(wps_status_t status);

#endif /* end of include guard: SUPPLICANT_API_H */
