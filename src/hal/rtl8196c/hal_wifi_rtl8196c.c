#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../../logging/logging.h"
#include "../../httpserver/new_http.h"
#include "../hal_wifi.h"
#include "hal_rtl8196c.h"
#include "spiflash.h"
#include "flash_layout.h"
#include "swcore.h"
#include "pcie.h"
#include "wl.h"
#include "wl_ap.h"
#include "wl_sta.h"
#include "net.h"
#include "dhcp.h"
#include "dhcpd.h"
#include "dnsd.h"
#include "timer.h"
#include "rtos.h"
#include "wifi_blobs.inc"

enum {
    WIFI_MODE_OFF = 0,
    WIFI_MODE_AP  = 1,
    WIFI_MODE_STA = 2,
};

#define WIFI_AP_DEFAULT_CHAN 7

extern void Main_OnWiFiStatusChange(int code);
extern int g_openAP;

static void (*g_wifiStatusCallback)(int code) = Main_OnWiFiStatusChange;
static struct wl_dev g_wl_dev;
static int g_wifi_hw_tried = 0;
static int g_wifi_hw_up    = 0;
static int g_wifi_mode     = WIFI_MODE_OFF;

static char g_ap_ssid[33];
static char g_sta_ssid[33];
static char g_sta_pass[64];
static obkStaticIP_t g_sta_static_ip;
static int g_sta_has_static_ip = 0;
static enum wl_sta_state g_sta_last_state = WL_STA_IDLE;
static int g_sta_dhcp_started  = 0;
static int g_sta_bound_logged  = 0;

static const uint8_t g_ap_ip[4]   = { 192, 168, 4, 1 };
static const uint8_t g_ap_mask[4] = { 255, 255, 255, 0 };

/* Keep Ethernet and HTTP alive during PBKDF2 without switching RTOS tasks */
void wpa_crypto_yield(void)
{
    net_poll();
    dhcp_poll();
    tcp_poll();
}

static int eth_any_port_up(void)
{
    for (int p = 0; p < NUM_PHY_PORTS; p++)
        if (eth_link_up(p)) return 1;
    return 0;
}

static int pmk_cache_load(const char *ssid, const char *pass, uint8_t out_pmk[32])
{
    if (!ssid || !ssid[0] || !pass || !pass[0]) return 0;
    rtl8196c_vflash_init_if_needed();
    const struct vflash_wifi_pmk *ram_c =
        (const struct vflash_wifi_pmk *)(const void *)(vflash_mem + VFLASH_PMK_OFFSET);
    if (ram_c->magic == VFLASH_PMK_MAGIC &&
        strcmp(ram_c->ssid, ssid) == 0 && strcmp(ram_c->pass, pass) == 0) {
        memcpy(out_pmk, ram_c->pmk, 32);
        return 1;
    }
    struct vflash_wifi_pmk spi_c;
    spiflash_read(FLASH_LFS_OFF + VFLASH_PMK_OFFSET, &spi_c, sizeof(spi_c));
    if (spi_c.magic == VFLASH_PMK_MAGIC &&
        strcmp(spi_c.ssid, ssid) == 0 && strcmp(spi_c.pass, pass) == 0) {
        memcpy(vflash_mem + VFLASH_PMK_OFFSET, &spi_c, sizeof(spi_c));
        memcpy(out_pmk, spi_c.pmk, 32);
        return 1;
    }
    return 0;
}

static void pmk_cache_save(const char *ssid, const char *pass, const uint8_t pmk[32])
{
    if (!ssid || !ssid[0] || !pass || !pass[0]) return;
    rtl8196c_vflash_init_if_needed();
    struct vflash_wifi_pmk c;
    memset(&c, 0, sizeof(c));
    c.magic = VFLASH_PMK_MAGIC;
    strncpy(c.ssid, ssid, 32);
    strncpy(c.pass, pass, 63);
    memcpy(c.pmk, pmk, 32);
    memcpy(vflash_mem + VFLASH_PMK_OFFSET, &c, sizeof(c));
    uint32_t spi_off = FLASH_LFS_OFF + VFLASH_PMK_OFFSET;
    if (spiflash_verify(spi_off, &c, sizeof(c)) != 0) {
        spiflash_erase_sector(spi_off);
        spiflash_write(spi_off, &c, sizeof(c));
    }
}

void rtl8196c_wifi_init(void)
{
    if (g_wifi_hw_tried)
        return;
    g_wifi_hw_tried = 1;

    printf("[wifi] Bringing up PCIe & RTL8192CE radio...\n");
    if (pcie_init(0) < 0) {
        printf("[wifi] PCIe link down (continuing with Ethernet only)\n");
        return;
    }
    pcie_map_bar2();
    struct wl_tables t = {
        wifi_fw, WIFI_FW_LEN,
        wifi_mac, WIFI_MAC_N,
        wifi_phy_2t, WIFI_PHY_2T_N,
        wifi_phy_fix, WIFI_PHY_FIX_N,
        wifi_agc, WIFI_AGC_N,
        wifi_radio_a, WIFI_RADIO_A_N,
        wifi_radio_b, WIFI_RADIO_B_N,
    };
    int rc = wl_hw_init(&g_wl_dev, &t);
    if (rc < 0) {
        printf("[wifi] wl_hw_init failed (%d)\n", rc);
        pcie_power_down();
        return;
    }
    wl_ap_trace  = 0;
    wl_sta_trace = 0;
    g_wifi_hw_up = 1;
    uint8_t ther_raw = 0;
    g_wifi_temperature = wl_hw_read_temperature(&g_wl_dev, &ther_raw);
    printf("[wifi] RTL8192CE radio up, WLAN MAC %02x:%02x:%02x:%02x:%02x:%02x (RF temp %.1f C, raw=%u, ref=%u)\n",
           g_wl_dev.mac[0], g_wl_dev.mac[1], g_wl_dev.mac[2],
           g_wl_dev.mac[3], g_wl_dev.mac[4], g_wl_dev.mac[5],
           (double)g_wifi_temperature, ther_raw, g_wl_dev.ther_ref);
}

extern int mqtt_loopsWithDisconnected;
static int g_wifi_cfg_saved = 0;
static int g_wifi_scanning  = 0;
static char g_last_mqtt_host[64];

void rtl8196c_wifi_update_temperature(void)
{
    if (!g_wifi_hw_up || g_wifi_scanning)
        return;
    g_wifi_temperature = wl_hw_read_temperature(&g_wl_dev, NULL);
}

void rtl8196c_wifi_notify_cfg_saved(void)
{
    if (g_wifi_hw_tried)
        g_wifi_cfg_saved = 1;
}

void rtl8196c_wifi_poll(void)
{
    const char *mhost = CFG_GetMQTTHost();
    if (mhost && strcmp(g_last_mqtt_host, mhost) != 0) {
        strncpy(g_last_mqtt_host, mhost, sizeof(g_last_mqtt_host) - 1);
        g_last_mqtt_host[sizeof(g_last_mqtt_host) - 1] = 0;
        if (mhost[0])
            mqtt_loopsWithDisconnected = 9999;
    }

    if (!g_wifi_hw_up || g_wifi_scanning)
        return;

    if (g_wifi_cfg_saved) {
        g_wifi_cfg_saved = 0;
        const char *ssid = CFG_GetWiFiSSID();
        const char *pass = CFG_GetWiFiPass();
        if (!ssid || !ssid[0] || strcmp(ssid, "RTL8196C-Ethernet") == 0) {
            if (g_wifi_mode != WIFI_MODE_AP)
                HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
        } else {
            if (g_wifi_mode != WIFI_MODE_STA ||
                strcmp(g_sta_ssid, ssid) != 0 ||
                strcmp(g_sta_pass, pass ? pass : "") != 0) {
                HAL_ConnectToWiFi(ssid, pass, &g_cfg.staticIP);
            } else {
                int want_static = (g_cfg.staticIP.localIPAddr[0] | g_cfg.staticIP.localIPAddr[1] |
                                   g_cfg.staticIP.localIPAddr[2] | g_cfg.staticIP.localIPAddr[3]) != 0;
                if (want_static) {
                    g_sta_static_ip = g_cfg.staticIP;
                    g_sta_has_static_ip = 1;
                    if (g_sta_dhcp_started) {
                        dhcp_wifi_stop();
                        g_sta_dhcp_started = 0;
                    }
                    if (wl_sta_info()->state == WL_STA_CONNECTED) {
                        net_set_wifi_ip(g_sta_static_ip.localIPAddr);
                        net_set_wifi_netmask(g_sta_static_ip.netMask);
                        net_set_wifi_gateway(g_sta_static_ip.gatewayIPAddr);
                        net_set_wifi_dns(g_sta_static_ip.dnsServerIpAddr);
                    }
                } else if (g_sta_has_static_ip) {
                    g_sta_has_static_ip = 0;
                    if (wl_sta_info()->state == WL_STA_CONNECTED && !g_sta_dhcp_started) {
                        dhcp_wifi_start(CFG_GetShortDeviceName());
                        g_sta_dhcp_started = 1;
                        g_sta_bound_logged = 0;
                    }
                }
            }
        }
    }

    if (g_wifi_mode == WIFI_MODE_AP) {
        wl_ap_poll();
    } else if (g_wifi_mode == WIFI_MODE_STA) {
        wl_sta_poll();
        const struct wl_sta_info *s = wl_sta_info();
        if (s->state != g_sta_last_state) {
            printf("[wifi] STA state -> %s", wl_sta_state_name(s->state));
            if (s->state == WL_STA_AUTHENTICATING) {
                printf(" (BSSID %02x:%02x:%02x:%02x:%02x:%02x ch=%u rssi=%d dBm)",
                       s->bssid[0], s->bssid[1], s->bssid[2],
                       s->bssid[3], s->bssid[4], s->bssid[5],
                       s->chan, s->rssi);
            } else if (s->state == WL_STA_CONNECTED) {
                printf(" (AID %u, %s)", s->aid, s->secured ? "WPA2-CCMP" : "open");
            } else if (s->state == WL_STA_FAILED && s->fail_reason) {
                printf(" (%s)", s->fail_reason);
            }
            printf("\n");

            if (s->state == WL_STA_CONNECTED) {
                if (g_sta_has_static_ip) {
                    net_set_wifi_ip(g_sta_static_ip.localIPAddr);
                    net_set_wifi_netmask(g_sta_static_ip.netMask);
                    net_set_wifi_gateway(g_sta_static_ip.gatewayIPAddr);
                    net_set_wifi_dns(g_sta_static_ip.dnsServerIpAddr);
                    g_sta_bound_logged = 1;
                    printf("[wifi] STA static IP: IP=%u.%u.%u.%u GW=%u.%u.%u.%u DNS=%u.%u.%u.%u\n",
                           g_sta_static_ip.localIPAddr[0], g_sta_static_ip.localIPAddr[1],
                           g_sta_static_ip.localIPAddr[2], g_sta_static_ip.localIPAddr[3],
                           g_sta_static_ip.gatewayIPAddr[0], g_sta_static_ip.gatewayIPAddr[1],
                           g_sta_static_ip.gatewayIPAddr[2], g_sta_static_ip.gatewayIPAddr[3],
                           g_sta_static_ip.dnsServerIpAddr[0], g_sta_static_ip.dnsServerIpAddr[1],
                           g_sta_static_ip.dnsServerIpAddr[2], g_sta_static_ip.dnsServerIpAddr[3]);
                } else if (!g_sta_dhcp_started) {
                    dhcp_wifi_start(CFG_GetShortDeviceName());
                    g_sta_dhcp_started = 1;
                    g_sta_bound_logged = 0;
                }
                if (g_wifiStatusCallback)
                    g_wifiStatusCallback(WIFI_STA_CONNECTED);
            } else {
                g_sta_dhcp_started = 0;
                g_sta_bound_logged = 0;
            }
            g_sta_last_state = s->state;
        }
        if (g_sta_dhcp_started) {
            dhcp_wifi_poll();
            if (dhcp_wifi_bound() && !g_sta_bound_logged) {
                const struct dhcp_lease *l = dhcp_wifi_info();
                g_sta_bound_logged = 1;
                printf("[wifi] STA DHCP bound: IP=%u.%u.%u.%u GW=%u.%u.%u.%u DNS=%u.%u.%u.%u\n",
                       l->ip[0], l->ip[1], l->ip[2], l->ip[3],
                       l->gw[0], l->gw[1], l->gw[2], l->gw[3],
                       l->dns[0], l->dns[1], l->dns[2], l->dns[3]);
            }
        }
    }
}

static const uint8_t *active_report_ip(void)
{
    const uint8_t *wip = net_wifi_ip();
    if (g_wifi_mode == WIFI_MODE_STA && wl_sta_info()->state == WL_STA_CONNECTED &&
        (wip[0] | wip[1] | wip[2] | wip[3]))
        return wip;
    if (!eth_any_port_up() && (wip[0] | wip[1] | wip[2] | wip[3]))
        return wip;
    return net_ip();
}

const char *HAL_GetMyIPString(void)
{
    static char ip_str[16];
    const uint8_t *ip = active_report_ip();
    snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return ip_str;
}

const char *HAL_GetMyGatewayString(void)
{
    static char gw_str[16];
    const uint8_t *gw = (active_report_ip() == net_wifi_ip()) ? net_wifi_gateway() : net_gateway();
    snprintf(gw_str, sizeof(gw_str), "%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
    return gw_str;
}

const char *HAL_GetMyDNSString(void)
{
    static char dns_str[16];
    const uint8_t *dns = (active_report_ip() == net_wifi_ip()) ? net_wifi_dns() : net_dns();
    snprintf(dns_str, sizeof(dns_str), "%u.%u.%u.%u", dns[0], dns[1], dns[2], dns[3]);
    return dns_str;
}

const char *HAL_GetMyMaskString(void)
{
    static char mask_str[16];
    const uint8_t *m = (active_report_ip() == net_wifi_ip()) ? net_wifi_netmask() : net_netmask();
    snprintf(mask_str, sizeof(mask_str), "%u.%u.%u.%u", m[0], m[1], m[2], m[3]);
    return mask_str;
}

void WiFI_GetMacAddress(char *mac)
{
    if (mac)
        memcpy(mac, net_mac(), 6);
}

int WiFI_SetMacAddress(char *mac)
{
    (void)mac;
    return 0;
}

const char *HAL_GetMACStr(char *macstr)
{
    const uint8_t *m = net_mac();
    sprintf(macstr, MACSTR, MAC2STR(m));
    return macstr;
}

char *HAL_GetWiFiBSSID(char *bssid)
{
    const uint8_t *m = net_mac();
    if (g_wifi_hw_up) {
        if (g_wifi_mode == WIFI_MODE_STA && wl_sta_info()->state == WL_STA_CONNECTED)
            m = wl_sta_info()->bssid;
        else
            m = g_wl_dev.mac;
    }
    sprintf(bssid, MACSTR, MAC2STR(m));
    return bssid;
}

uint8_t HAL_GetWiFiChannel(uint8_t *chan)
{
    uint8_t ch = WIFI_AP_DEFAULT_CHAN;
    if (g_wifi_hw_up) {
        if (g_wifi_mode == WIFI_MODE_STA && wl_sta_info()->chan)
            ch = wl_sta_info()->chan;
        else if (g_wl_dev.chan)
            ch = g_wl_dev.chan;
    }
    if (chan) *chan = ch;
    return ch;
}

int HAL_GetWifiStrength(void)
{
    if (g_wifi_hw_up) {
        if (g_wifi_mode == WIFI_MODE_STA && wl_sta_info()->state == WL_STA_CONNECTED)
            return wl_sta_info()->rssi;
        if (g_wifi_mode == WIFI_MODE_AP) {
            const struct wl_ap_client *c;
            int nc = wl_ap_clients(&c);
            if (nc > 0)
                return c[0].rssi;
            return -35;
        }
    }
    return -40;
}

void HAL_PrintNetworkInfo(void)
{
    const uint8_t *eip = net_ip();
    const uint8_t *wip = net_wifi_ip();
    if (g_wifi_mode == WIFI_MODE_AP) {
        const struct wl_ap_client *c;
        int nc = wl_ap_clients(&c);
        ADDLOG_INFO(LOG_FEATURE_GENERAL,
                    "RTL8196C Eth=%u.%u.%u.%u | WiFi AP \"%s\" IP=%u.%u.%u.%u ch=%u clients=%d",
                    eip[0], eip[1], eip[2], eip[3],
                    g_ap_ssid, wip[0], wip[1], wip[2], wip[3],
                    (unsigned)g_wl_dev.chan, nc);
    } else if (g_wifi_mode == WIFI_MODE_STA) {
        const struct wl_sta_info *s = wl_sta_info();
        ADDLOG_INFO(LOG_FEATURE_GENERAL,
                    "RTL8196C Eth=%u.%u.%u.%u | WiFi STA \"%s\" (%s) IP=%u.%u.%u.%u ch=%u rssi=%d tx=%u rx=%u(u=%u,b=%u,undec=%u)",
                    eip[0], eip[1], eip[2], eip[3],
                    g_sta_ssid, wl_sta_state_name(s->state),
                    wip[0], wip[1], wip[2], wip[3],
                    (unsigned)s->chan, s->rssi,
                    (unsigned)s->data_tx, (unsigned)s->data_rx,
                    (unsigned)s->rx_unicast, (unsigned)s->rx_bcast, (unsigned)s->rx_undecrypted);
    } else {
        ADDLOG_INFO(LOG_FEATURE_GENERAL, "RTL8196C Eth=%u.%u.%u.%u",
                    eip[0], eip[1], eip[2], eip[3]);
    }
}

void HAL_WiFi_SetupStatusCallback(void (*cb)(int code))
{
    g_wifiStatusCallback = cb;
    if (cb)
        cb(g_wifi_mode == WIFI_MODE_AP ? WIFI_AP_CONNECTED : WIFI_STA_CONNECTED);
}

void HAL_DisconnectFromWifi(void)
{
    if (g_wifi_mode == WIFI_MODE_STA) {
        dhcp_wifi_stop();
        wl_sta_stop();
    }
}

void HAL_DisableEnhancedFastConnect(void)
{
}

int HAL_SetupWiFiOpenAccessPoint(const char *ssid)
{
    g_openAP = 0;
    rtl8196c_wifi_init();
    const char *use_ssid = (ssid && ssid[0]) ? ssid : "OpenRTL8196C";
    if (!g_wifi_hw_up) {
        if (g_wifiStatusCallback)
            g_wifiStatusCallback(WIFI_STA_CONNECTED);
        return 0;
    }

    if (g_wifi_mode == WIFI_MODE_AP && strcmp(g_ap_ssid, use_ssid) == 0) {
        if (g_wifiStatusCallback)
            g_wifiStatusCallback(WIFI_AP_CONNECTED);
        return 0;
    }

    if (g_wifi_mode == WIFI_MODE_STA) {
        dhcp_wifi_stop();
        wl_sta_stop();
    }

    strncpy(g_ap_ssid, use_ssid, 32);
    g_ap_ssid[32] = 0;

    wl_ap_start(&g_wl_dev, g_ap_ssid, WIFI_AP_DEFAULT_CHAN, 0);
    net_set_wifi_link(wl_ap_link(), g_wl_dev.mac);
    net_set_wifi_ip(g_ap_ip);
    net_set_wifi_netmask(g_ap_mask);
    net_set_wifi_gateway(g_ap_ip);
    net_set_wifi_dns(g_ap_ip);
    dhcpd_start(g_ap_ip, g_ap_mask, 2, 8);
    dnsd_start(g_ap_ip);

    g_wifi_mode = WIFI_MODE_AP;
    printf("[wifi] Soft-AP active: SSID=\"%s\" ch=%d IP=192.168.4.1 (DHCP 192.168.4.2..9)\n",
           g_ap_ssid, WIFI_AP_DEFAULT_CHAN);

    if (g_wifiStatusCallback)
        g_wifiStatusCallback(WIFI_AP_CONNECTED);
    return 0;
}

void HAL_ConnectToWiFi(const char *oob_ssid, const char *connect_key, obkStaticIP_t *ip)
{
    g_openAP = 0;
    if (!oob_ssid || !oob_ssid[0] || strcmp(oob_ssid, "RTL8196C-Ethernet") == 0) {
        HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
        return;
    }

    rtl8196c_wifi_init();
    const char *use_pass = connect_key ? connect_key : "";

    if (!g_wifi_hw_up) {
        if (g_wifiStatusCallback)
            g_wifiStatusCallback(WIFI_STA_CONNECTED);
        return;
    }

    if (g_wifi_mode == WIFI_MODE_STA &&
        strcmp(g_sta_ssid, oob_ssid) == 0 &&
        strcmp(g_sta_pass, use_pass) == 0 &&
        wl_sta_info()->state != WL_STA_IDLE) {
        if (g_wifiStatusCallback)
            g_wifiStatusCallback(WIFI_STA_CONNECTED);
        return;
    }

    if (g_wifi_mode == WIFI_MODE_AP) {
        dhcpd_stop();
        dnsd_stop();
        wl_ap_stop();
    } else if (g_wifi_mode == WIFI_MODE_STA) {
        dhcp_wifi_stop();
        wl_sta_stop();
    }

    strncpy(g_sta_ssid, oob_ssid, 32);
    g_sta_ssid[32] = 0;
    strncpy(g_sta_pass, use_pass, 63);
    g_sta_pass[63] = 0;

    if (ip && (ip->localIPAddr[0] | ip->localIPAddr[1] | ip->localIPAddr[2] | ip->localIPAddr[3])) {
        g_sta_static_ip = *ip;
        g_sta_has_static_ip = 1;
    } else {
        g_sta_has_static_ip = 0;
    }

    static const uint8_t zero_ip[4] = { 0, 0, 0, 0 };
    net_set_wifi_link(wl_sta_link(), g_wl_dev.mac);
    net_set_wifi_ip(zero_ip);

    uint8_t pre_pmk[32];
    int cached = pmk_cache_load(g_sta_ssid, g_sta_pass, pre_pmk);
    uint32_t t0 = timer_ms();
    if (g_sta_pass[0] && !cached)
        printf("[wifi] Deriving WPA2 PMK for SSID \"%s\"...\n", g_sta_ssid);

    wl_sta_start_pmk(&g_wl_dev, g_sta_ssid, g_sta_pass, cached ? pre_pmk : 0);

    if (g_sta_pass[0] && !cached) {
        wl_sta_get_pmk(pre_pmk);
        pmk_cache_save(g_sta_ssid, g_sta_pass, pre_pmk);
        printf("[wifi] WPA2 PMK derived & cached in %u ms\n", timer_ms() - t0);
    } else if (g_sta_pass[0] && cached) {
        printf("[wifi] Using cached WPA2 PMK for SSID \"%s\"\n", g_sta_ssid);
    }

    g_sta_last_state   = WL_STA_IDLE;
    g_sta_dhcp_started = 0;
    g_sta_bound_logged = 0;
    g_wifi_mode        = WIFI_MODE_STA;

    if (g_wifiStatusCallback)
        g_wifiStatusCallback(WIFI_STA_CONNECTED);
}

void HAL_FastConnectToWiFi(const char *oob_ssid, const char *connect_key, obkStaticIP_t *ip)
{
    HAL_ConnectToWiFi(oob_ssid, connect_key, ip);
}

#define MAX_SCAN_APS 32

struct scan_ap_entry {
    uint8_t bssid[6];
    char    ssid[33];
    uint8_t chan;
    int     rssi;
    uint8_t sec; /* 0=Open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA/WPA2 */
};

struct scan_ctx {
    struct scan_ap_entry aps[MAX_SCAN_APS];
    int count;
    uint16_t seq;
};

static void scan_send_probe(uint16_t *seq)
{
    uint8_t f[64];
    memset(f, 0, sizeof(f));
    f[0] = 0x40; /* Probe Request */
    memset(f + 4, 0xFF, 6);
    memcpy(f + 10, g_wl_dev.mac, 6);
    memset(f + 16, 0xFF, 6);
    uint16_t s = (*seq)++;
    f[22] = (uint8_t)(s << 4);
    f[23] = (uint8_t)(s >> 4);
    uint32_t n = 24;
    /* Wildcard SSID IE (id=0, len=0) */
    f[n++] = 0;
    f[n++] = 0;
    /* Supported rates (id=1, len=8) */
    static const uint8_t rates[8] = { 0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24 };
    f[n++] = 1; f[n++] = 8; memcpy(f + n, rates, 8); n += 8;
    /* Extended supported rates (id=50, len=4) */
    static const uint8_t ext[4] = { 0x30, 0x48, 0x60, 0x6C };
    f[n++] = 50; f[n++] = 4; memcpy(f + n, ext, 4); n += 4;
    wl_hw_tx(&g_wl_dev, f, n, 0, 20);
}

static void scan_rx_cb(void *ctx, const uint8_t *f, uint32_t len, uint8_t ch, int rssi)
{
    struct scan_ctx *sc = (struct scan_ctx *)ctx;
    if (len < 36)
        return;
    uint8_t type = f[0] & 0x0C, sub = f[0] & 0xF0;
    if (type != 0x00 || (sub != 0x80 && sub != 0x50))
        return;

    const uint8_t *bssid = f + 16;
    uint16_t cap = (uint16_t)(f[34] | (f[35] << 8));
    int privacy = (cap & 0x0010) != 0;

    const uint8_t *ssid_p = 0;
    uint8_t ssid_l = 0;
    uint8_t chan = ch;
    int has_wpa = 0, has_rsn = 0;

    for (uint32_t o = 36; o + 2 <= len;) {
        uint8_t id = f[o], l = f[o + 1];
        const uint8_t *v = f + o + 2;
        if (o + 2 + l > len)
            break;
        if (id == 0 && l <= 32) {
            ssid_p = v;
            ssid_l = l;
        } else if (id == 3 && l >= 1 && v[0] >= 1 && v[0] <= 14) {
            chan = v[0];
        } else if (id == 48 && l >= 2) {
            has_rsn = 1;
        } else if (id == 221 && l >= 4 && v[0] == 0x00 && v[1] == 0x50 && v[2] == 0xF2 && v[3] == 0x01) {
            has_wpa = 1;
        }
        o += 2u + l;
    }

    uint8_t sec = 0;
    if (has_wpa && has_rsn) sec = 4;
    else if (has_rsn) sec = 3;
    else if (has_wpa) sec = 2;
    else if (privacy) sec = 1;

    int idx = -1;
    for (int i = 0; i < sc->count; i++) {
        if (memcmp(sc->aps[i].bssid, bssid, 6) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        if (sc->count >= MAX_SCAN_APS) {
            int worst = 0;
            for (int i = 1; i < sc->count; i++)
                if (sc->aps[i].rssi < sc->aps[worst].rssi)
                    worst = i;
            if (rssi <= sc->aps[worst].rssi)
                return;
            idx = worst;
        } else {
            idx = sc->count++;
        }
        memset(&sc->aps[idx], 0, sizeof(sc->aps[idx]));
        memcpy(sc->aps[idx].bssid, bssid, 6);
        sc->aps[idx].rssi = rssi;
    }

    if (ssid_p && ssid_l > 0 && ssid_p[0] != 0) {
        memcpy(sc->aps[idx].ssid, ssid_p, ssid_l);
        sc->aps[idx].ssid[ssid_l] = 0;
    }
    if (chan)
        sc->aps[idx].chan = chan;
    if (rssi > sc->aps[idx].rssi)
        sc->aps[idx].rssi = rssi;
    if (sec > sc->aps[idx].sec)
        sc->aps[idx].sec = sec;
}

static const char *scan_sec_name(uint8_t sec)
{
    switch (sec) {
    case 0: return "Open";
    case 1: return "WEP";
    case 2: return "WPA";
    case 3: return "WPA2";
    case 4: return "WPA/WPA2";
    default: return "Unknown";
    }
}

void HAL_WiFi_Scan_HTML(http_request_t *request)
{
    rtl8196c_wifi_init();
    if (!g_wifi_hw_up) {
        poststr(request, "Wi-Fi radio not available.<br>");
        return;
    }

    static struct scan_ctx sc;
    memset(&sc, 0, sizeof(sc));

    uint8_t orig_chan = g_wl_dev.chan ? g_wl_dev.chan : WIFI_AP_DEFAULT_CHAN;
    uint32_t orig_rcr = wl_rd32(&g_wl_dev, 0x0608);
    uint8_t orig_bcn  = wl_rd8(&g_wl_dev, 0x0550);

    g_wifi_scanning = 1;
    /* Pause AP beacon TX while hopping channels and accept all BSSIDs */
    wl_wr8(&g_wl_dev, 0x0550, orig_bcn & ~0x08u);
    wl_wr32(&g_wl_dev, 0x0608, orig_rcr & ~((1u << 6) | (1u << 7)));

    for (uint8_t ch = 1; ch <= 13; ch++) {
        wl_hw_set_channel(&g_wl_dev, ch);
        scan_send_probe(&sc.seq);
        uint32_t t0 = timer_ms();
        while ((uint32_t)(timer_ms() - t0) < 95) {
            wl_hw_rx_poll(&g_wl_dev, scan_rx_cb, &sc);
            net_poll();
            tcp_poll();
        }
    }

    wl_hw_set_channel(&g_wl_dev, orig_chan);
    wl_wr32(&g_wl_dev, 0x0608, orig_rcr);
    wl_wr8(&g_wl_dev, 0x0550, orig_bcn);
    g_wifi_scanning = 0;

    /* Sort by RSSI descending */
    for (int i = 0; i < sc.count - 1; i++) {
        for (int j = i + 1; j < sc.count; j++) {
            if (sc.aps[j].rssi > sc.aps[i].rssi) {
                struct scan_ap_entry tmp = sc.aps[i];
                sc.aps[i] = sc.aps[j];
                sc.aps[j] = tmp;
            }
        }
    }

    if (sc.count <= 0) {
        poststr(request, "No networks found.<br>");
        return;
    }

    hprintf255(request, "<table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Signal</th><th>Security</th></tr>");
    for (int i = 0; i < sc.count; i++) {
        hprintf255(request, "<tr><td>");
        poststr_escaped(request, sc.aps[i].ssid[0] ? sc.aps[i].ssid : "<hidden>");
        hprintf255(request,
                   "</td><td>" MACSTR "</td><td>%u</td><td>%ddBm</td><td>%s</td></tr>",
                   MAC2STR(sc.aps[i].bssid),
                   (unsigned)sc.aps[i].chan,
                   sc.aps[i].rssi,
                   scan_sec_name(sc.aps[i].sec));
    }
    hprintf255(request, "</table><br>");
}

#endif /* PLATFORM_RTL8196C */
