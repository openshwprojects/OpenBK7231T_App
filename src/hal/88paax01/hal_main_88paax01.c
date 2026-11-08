#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../driver/drv_public.h"
#include "../../logging/logging.h"
#include "hal_88paax01.h"
#include "hal_pinmap_88paax01.h"

#include "hw.h"
#include "layout.h"
#include "image.h"
#include "crc.h"
#include "cache.h"
#include "spi_flash.h"
#include "slots.h"
#include "bootinfo.h"
#include "uart.h"
#include "timer.h"
#include "periph.h"
#include "periph_cmds.h"
#include "common_cmds.h"
#include "usbcon.h"
#include "ui.h"
#include "wifi.h"
#include "scan.h"
#include "ramrun.h"
#include "net.h"
#include "tcp.h"
#include "dhcp.h"
#include "dhcpd.h"
#include "heap.h"
#include "rtos.h"

/* Default Wi-Fi network from the git-ignored fw/app/wifi_secrets.h (HPdeskJet2620
   repository); without it OBK starts its own access point as usual. */
#if defined(__has_include)
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#endif
#endif
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""
#endif

extern void jump_to(uint32_t entry, uint32_t arg) __attribute__((noreturn));

static uint32_t g_last_second_ms = 0;
static char     g_cli_line[256];
static int      g_cli_len = 0;

/* Helper to bridge (argc, argv) HPDJ CLI commands into OpenBeken's console */
static int split_argv(char *buf, char **argv, int max_argv)
{
	int argc = 0;
	char *p = buf;
	while (*p && argc < max_argv) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;
		if (*p == '"') {
			p++;
			argv[argc++] = p;
			while (*p && *p != '"')
				p++;
			if (*p == '"')
				*p++ = '\0';
		} else {
			argv[argc++] = p;
			while (*p && *p != ' ' && *p != '\t')
				p++;
			if (*p)
				*p++ = '\0';
		}
	}
	return argc;
}

static commandResult_t run_hpdj_cmd(int (*fn)(int argc, char **argv), const char *cmd, const char *args)
{
	char line[256];
	char *argv[16];
	snprintf(line, sizeof(line), "%s %s", cmd ? cmd : "", args ? args : "");
	int argc = split_argv(line, argv, 16);
	if (argc <= 0) {
		argv[0] = (char *)(cmd ? cmd : "");
		argc = 1;
	}
	fn(argc, argv);
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPDJ_Version(const void *c, const char *cmd, const char *args, int f)
{
	(void)c; (void)cmd; (void)args; (void)f;
	printf("HPDJ OpenBeken %s on Marvell 88PAAX01-BPK2 (HP DeskJet 2620) IP=%s\n",
	       USER_SW_VER, HAL_GetMyIPString());
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPDJ_Net(const void *c, const char *cmd, const char *args, int f)
{
	(void)c; (void)cmd; (void)args; (void)f;
	unsigned char mac[6];
	HAL_GetMACWiFi(mac);
	printf("net: mac %02x:%02x:%02x:%02x:%02x:%02x ip %s mask %s gw %s dns %s (rx %u tx %u)\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
	       HAL_GetMyIPString(), HAL_GetMyMaskString(),
	       HAL_GetMyGatewayString(), HAL_GetMyDNSString(),
	       (unsigned)net_stats.rx, (unsigned)net_stats.tx);
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPDJ_Uptime(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_uptime, cmd, args); }

static commandResult_t Cmd_HPDJ_Sysinfo(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_sysinfo, cmd, args); }

static commandResult_t Cmd_HPDJ_Slots(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_slots, cmd, args); }

static commandResult_t Cmd_HPDJ_Verify(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_verify, cmd, args); }

static commandResult_t Cmd_HPDJ_Update(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_update, cmd, args); }

static commandResult_t Cmd_HPDJ_Flash(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_flash, cmd, args); }

static commandResult_t Cmd_HPDJ_Usb(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_usb, cmd, args); }

static commandResult_t Cmd_HPDJ_Wifi(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_wifi, cmd, args); }

static commandResult_t Cmd_HPDJ_Ui(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_ui, cmd, args); }

static commandResult_t Cmd_HPDJ_Lcd(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_lcd, cmd, args); }

static commandResult_t Cmd_HPDJ_Buttons(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_buttons, cmd, args); }

static commandResult_t Cmd_HPDJ_Sensors(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_sensors, cmd, args); }

static commandResult_t Cmd_HPDJ_Motor(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_motor, cmd, args); }

static commandResult_t Cmd_HPDJ_Asic(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_asic, cmd, args); }

static commandResult_t Cmd_HPDJ_Adc(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_adc, cmd, args); }

static commandResult_t Cmd_HPDJ_Eeprom(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_eeprom, cmd, args); }

static commandResult_t Cmd_HPDJ_I2c(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_i2c, cmd, args); }

static commandResult_t Cmd_HPDJ_Scan(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_scan, cmd, args); }

static commandResult_t Cmd_HPDJ_Ramrun(const void *c, const char *cmd, const char *args, int f)
{ (void)c; (void)f; return run_hpdj_cmd(cmd_ramrun, cmd, args); }

static commandResult_t Cmd_HPDJ_Reboot(const void *c, const char *cmd, const char *args, int f)
{
	(void)c; (void)cmd; (void)args; (void)f;
	HAL_RebootModule();
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPDJ_Bl(const void *c, const char *cmd, const char *args, int f)
{
	(void)c; (void)cmd; (void)args; (void)f;
	const image_header_t *hdr = (const image_header_t *)flash_mmap(FLASH_BL_OFFSET);
	dcache_invalidate_range(FLASH_BL_OFFSET, FLASH_BL_OFFSET + FLASH_BL_SIZE);
	if (image_validate(hdr, IMAGE_TYPE_BL, BL_LOAD_ADDR, BL_CODE_MAX, 1) != IMG_OK) {
		printf("bootloader image in flash is invalid\n");
		return CMD_RES_ERROR;
	}
	printf("re-entering bootloader v%u.%u.%u shell\n",
	       VERSION_MAJOR(hdr->version), VERSION_MINOR(hdr->version), VERSION_PATCH(hdr->version));
	uart_flush();
	usbcon_stop();
	uint32_t dst = RW_ALIAS(hdr->load_address);
	memcpy((void *)dst, flash_mmap(FLASH_BL_OFFSET + IMAGE_HEADER_SIZE), hdr->payload_size);
	cache_sync_code(dst, dst + hdr->payload_size, hdr->load_address, hdr->load_address + hdr->payload_size);
	if (crc32_compute((const void *)(uintptr_t)hdr->load_address, hdr->payload_size) != hdr->payload_crc32)
		return CMD_RES_ERROR;
	jump_to(hdr->entry_address, BL_STAY_MAGIC);
	return CMD_RES_OK;
}

static void poll_console_rx(void)
{
	usbcon_poll();
	for (;;) {
		int c = uart_getc_nonblock();
		if (c < 0)
			break;
		if (c == '\r' || c == '\n') {
			uart_puts("\n");
			if (g_cli_len > 0) {
				g_cli_line[g_cli_len] = '\0';
				g_cli_len = 0;
				CMD_ExecuteCommand(g_cli_line, 0);
			}
			uart_puts("app> ");
		} else if (c == 0x08 || c == 0x7F) {
			if (g_cli_len > 0) {
				g_cli_len--;
				uart_puts("\b \b");
			}
		} else if (c >= 0x20 && c < 0x7F) {
			if (g_cli_len < (int)sizeof(g_cli_line) - 1) {
				g_cli_line[g_cli_len++] = (char)c;
				uart_putc((char)c);
			}
		}
	}
}

extern void mqtt_poll(void);
extern int g_openAP;
extern void Main_ConnectToWiFiNow(void);

static void background_poll(void)
{
	static int in_bg = 0;
	if (in_bg)
		return;
	in_bg = 1;
	watchdog_kick();
	timer_poll();
	usbcon_poll();
	net_poll();
	HAL_WiFi_Poll_88PAAX01();
	tcp_poll();
	mqtt_poll();
	motor_api_poll();
	in_bg = 0;
}

void main(uint32_t arg)
{
	uart_init();
	timer_init();
	flash_init();

	if (arg == BOOTINFO_ADDR && BOOTINFO->magic == BOOTINFO_MAGIC) {
		if (BOOTINFO->active_slot == BOOTINFO_SLOT_RAM) {
			slot_info_t infos[FLASH_SLOT_COUNT];
			for (int i = 0; i < FLASH_SLOT_COUNT; i++)
				slot_inspect(i, &infos[i], 0);
			g_current_slot = slot_select_best(infos);
		} else {
			g_current_slot = (int)BOOTINFO->active_slot;
		}
		if (BOOTINFO->ticks_per_ms)
			timer_set_rate(BOOTINFO->ticks_per_ms);
	}

	flash_autodetect_mode();
	GPIO_OUTPUT(GPIO_PIN_POWER_LED) = 0; /* active-low Power LED on */
	GPIO_DIR(GPIO_PIN_POWER_LED) = 1;
	boot_attempts_mark_alive();

	printf("\n========================================================\n");
	printf(" OpenBeken (%s) on Marvell 88PAAX01-BPK2 (AXON ARM946E-S)\n", USER_SW_VER);
	printf(" Board: HP DeskJet 2620 All-in-One (slot %c)\n",
	       g_current_slot >= 0 ? slot_letter(g_current_slot) : 'R');
	printf("========================================================\n");

	heap_init();
	rtos_init();
	rtos_register_poll(background_poll);
	pax_flash_init();

	static const uint8_t default_mac[6] = { 0x02, 0x88, 0x50, 0x26, 0x20, 0x01 };
	static const uint8_t zero_ip[4]     = { 0, 0, 0, 0 };
	net_init(default_mac, zero_ip);

	HAL_WiFi_Init_88PAAX01();

#if USB_BOOT_ON
	usbcon_start();
#endif

	Main_Init();
	CMD_ExecuteCommand("loglevel 2", 0);
	pax_setup_default_channels();
	DRV_StartDriver("HP2620Panel");
	DRV_StartDriver("HP2620Motors");

	if (DEFAULT_WIFI_SSID[0] && (!CFG_GetWiFiSSID() || !CFG_GetWiFiSSID()[0])) {
		CFG_SetWiFiSSID(DEFAULT_WIFI_SSID);
		CFG_SetWiFiPass(DEFAULT_WIFI_PASS);
		g_openAP = 0;
		Main_ConnectToWiFiNow();
	}

	/* Register HPDJ hardware & recovery commands in OpenBeken CLI */
	CMD_RegisterCommand("version",  Cmd_HPDJ_Version, NULL);
	CMD_RegisterCommand("net",      Cmd_HPDJ_Net,     NULL);
	CMD_RegisterCommand("uptime",   Cmd_HPDJ_Uptime,  NULL);
	CMD_RegisterCommand("sysinfo",  Cmd_HPDJ_Sysinfo, NULL);
	CMD_RegisterCommand("slots",    Cmd_HPDJ_Slots,   NULL);
	CMD_RegisterCommand("verify",   Cmd_HPDJ_Verify,  NULL);
	CMD_RegisterCommand("update",   Cmd_HPDJ_Update,  NULL);
	CMD_RegisterCommand("flash",    Cmd_HPDJ_Flash,   NULL);
	CMD_RegisterCommand("usb",      Cmd_HPDJ_Usb,     NULL);
	CMD_RegisterCommand("wifi",     Cmd_HPDJ_Wifi,    NULL);
	CMD_RegisterCommand("ui",       Cmd_HPDJ_Ui,      NULL);
	CMD_RegisterCommand("lcd",      Cmd_HPDJ_Lcd,     NULL);
	CMD_RegisterCommand("buttons",  Cmd_HPDJ_Buttons, NULL);
	CMD_RegisterCommand("sensors",  Cmd_HPDJ_Sensors, NULL);
	CMD_RegisterCommand("motor",    Cmd_HPDJ_Motor,   NULL);
	CMD_RegisterCommand("asic",     Cmd_HPDJ_Asic,    NULL);
	CMD_RegisterCommand("adc",      Cmd_HPDJ_Adc,     NULL);
	CMD_RegisterCommand("eeprom",   Cmd_HPDJ_Eeprom,  NULL);
	CMD_RegisterCommand("i2c",      Cmd_HPDJ_I2c,     NULL);
	CMD_RegisterCommand("scan",     Cmd_HPDJ_Scan,    NULL);
	CMD_RegisterCommand("ramrun",   Cmd_HPDJ_Ramrun,  NULL);
	CMD_RegisterCommand("reboot",   Cmd_HPDJ_Reboot,  NULL);
	CMD_RegisterCommand("bl",       Cmd_HPDJ_Bl,      NULL);

	g_last_second_ms = timer_uptime_ms();
	uart_puts("app> ");

	for (;;) {
		poll_console_rx();
		background_poll();

		uint32_t now = timer_uptime_ms();
		if ((uint32_t)(now - g_last_second_ms) >= 1000u) {
			g_last_second_ms = now;
			Main_OnEverySecond();
		}

		rtos_yield();
	}
}
