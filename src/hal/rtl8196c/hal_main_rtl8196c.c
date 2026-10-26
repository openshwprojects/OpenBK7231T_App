#ifdef PLATFORM_RTL8196C

#include "../../new_common.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../hal_wifi.h"
#include "hal_rtl8196c.h"
#include "board_config.h"
#include "irq.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "spiflash.h"
#include "flash_layout.h"
#include "swcore.h"
#include "net.h"
#include "dhcp.h"
#include "tcp.h"
#include "ota.h"
#include "heap.h"
#include "rtos.h"

extern char _start[];
extern char _bss_start[];
extern void mqtt_poll(void);

static char uart_cmd_buf[256];
static int  uart_cmd_len = 0;

static void poll_uart_console(void)
{
    int ch;
    while ((ch = uart_getc_nb()) >= 0) {
        if (ch == '\r' || ch == '\n') {
            if (uart_cmd_len > 0) {
                uart_cmd_buf[uart_cmd_len] = 0;
                uart_cmd_len = 0;
                printf("\n[cmd] %s\n", uart_cmd_buf);
                CMD_ExecuteCommand(uart_cmd_buf, COMMAND_FLAG_SOURCE_CONSOLE);
            }
        } else if (ch == 0x08 || ch == 0x7F) {
            if (uart_cmd_len > 0)
                uart_cmd_len--;
        } else if (ch >= 0x20 && uart_cmd_len < (int)sizeof(uart_cmd_buf) - 1) {
            uart_cmd_buf[uart_cmd_len++] = (char)ch;
        }
    }
}

extern void rtl8196c_usb_vfs_init(void);
extern void rtl8196c_usb_vfs_poll(void);

static void rtl8196c_bg_poll(void)
{
    static int in_bg_poll = 0;
    if (in_bg_poll)
        return;
    in_bg_poll = 1;
    net_poll();
    dhcp_poll();
    rtl8196c_wifi_poll();
    ota_poll();
    tcp_poll();
    mqtt_poll();
    poll_uart_console();
    in_bg_poll = 0;
}

extern int g_openAP;
extern int bSafeMode;
extern void Main_ConnectToWiFiNow(void);
extern void Main_OnWiFiStatusChange(int code);
#include "../../driver/drv_public.h"

int main(void)
{
    /* Save clean boot image (.text+.rodata+.data) before any .data variable is mutated */
    uint32_t img_len = (uint32_t)((uintptr_t)_bss_start - (uintptr_t)_start);
    if (img_len > 0 && img_len <= SNAPSHOT_MAX_SIZE) {
        memcpy((void *)(uintptr_t)SNAPSHOT_BASE_ADDR,
               (const void *)(uintptr_t)_start, img_len);
    }
    irq_init();
    uart_init();
    timer_delay_init();
    gpio_leds_init();
    heap_init();
    rtos_init();

    rtl8196c_vflash_init_if_needed();
    vflash_hdr->snapshot_len = img_len;
    vflash_hdr->pending_ota_len = 0;

    printf("\n============================================================\n");
    printf("OpenRTL8196C — OpenBeken (%s) on Realtek RTL8196C\n", USER_SW_VER);
    printf("============================================================\n");

    uint8_t mac[6];
    static const uint8_t static_ip[4] = BOARD_IP;
    static const uint8_t peer_ip[4]   = BOARD_PEER_IP;
    spiflash_factory_mac(mac);

    net_init(mac, static_ip);
    ota_init(BOARD_OTA_PORT);

    /* On cold boot from SPI flash, PHY auto-negotiation takes ~1.8s after
     * swcore_hw_init(). Wait up to 3.0s for any port link to come up before
     * sending gratuitous ARP and initial DHCP DISCOVER. */
    uint32_t link_wait_ms = timer_ms();
    while ((uint32_t)(timer_ms() - link_wait_ms) < 3000u) {
        int up = 0;
        for (int p = 0; p < NUM_PHY_PORTS; p++) {
            if (eth_link_up(p)) { up = 1; break; }
        }
        if (up) break;
        delay_ms(20);
    }

    net_arp_request(peer_ip);
    dhcp_start("OpenRTL8196C");

    /* Wait up to 2.5s for initial DHCP lease so IP is ready when Main_Init logs it */
    uint32_t dhcp_start_ms = timer_ms();
    while (!dhcp_bound() && (uint32_t)(timer_ms() - dhcp_start_ms) < 2500u) {
        net_poll();
        dhcp_poll();
        ota_poll();
        tcp_poll();
    }

    rtos_register_poll(rtl8196c_bg_poll);

    /* Ethernet is already up before Main_Init(), so mark network connected so
     * network drivers started during Main_Init() (SSDP, MDNS, DGR) bind immediately. */
    Main_OnWiFiStatusChange(WIFI_AP_CONNECTED);
    Main_Init();
    rtl8196c_usb_vfs_init();
    /* Start Wi-Fi AP (when unconfigured) or STA (when configured) immediately without 5s delay */
    if (g_openAP > 0) {
        g_openAP = 0;
        HAL_SetupWiFiOpenAccessPoint(CFG_GetDeviceName());
    } else {
        Main_ConnectToWiFiNow();
    }
    if (!bSafeMode) {
        const char *ntp_srv = CFG_GetNTPServer();
        if (!ntp_srv || !ntp_srv[0] || strcmp(ntp_srv, "192.168.0.114:12300") == 0) {
            CFG_SetNTPServer(DEFAULT_NTP_SERVER);
        }
        if (!DRV_IsRunning("SSDP")) {
            DRV_StartDriver("SSDP");
        }
        if (!DRV_IsRunning("NTP")) {
            DRV_StartDriver("NTP");
        }
        if (!DRV_IsRunning("FTP")) {
            DRV_StartDriver("FTP");
        }
        if (!DRV_IsRunning("SMB")) {
            DRV_StartDriver("SMB");
        }
    }
    bootstate_confirm(FLASH_BOOTSTATE_OFF);

    printf("[obk-rtl8196c] Ready: HTTP on http://%s/ (and http://192.168.0.196/, AP http://192.168.4.1/), TCP OTA on :%u\n",
           HAL_GetMyIPString(), BOARD_OTA_PORT);

    extern void rtl8196c_wifi_update_temperature(void);
    uint32_t last_sec = timer_ms();
    for (;;) {
        rtos_delay_milliseconds(10);
        uint32_t now = timer_ms();
        while ((uint32_t)(now - last_sec) >= 1000u) {
            last_sec += 1000u;
            rtl8196c_wifi_update_temperature();
            rtl8196c_usb_vfs_poll();
            Main_OnEverySecond();
        }
    }
}

#endif /* PLATFORM_RTL8196C */
