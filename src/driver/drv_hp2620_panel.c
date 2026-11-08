/*
 * drv_hp2620_panel.c - OpenBeken driver for the HP DeskJet 2620 front panel
 *                      (64-bit shift-register LCD + 3 orange LEDs + 7 buttons).
 *
 * Keeps /cfg_pins strictly for the 32 raw SoC GPIOs (P0..P31) while exposing:
 *   - Interactive SVG Front Panel & LCD clone inside OpenBeken's /index state
 *   - Shift-register LED & Ink-gauge channels (Ch 4..8) with physical button sync
 *   - Console commands: HPPanel_Char, HPPanel_Bars, HPPanel_Icon, HPPanel_Test
 */
#include "drv_hp2620_panel.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#ifdef PLATFORM_88PAAX01

#include "hw.h"
#include "timer.h"
#include "periph.h"
#include "wifi.h"
#include "tcp.h"
#include "../hal/hal_wifi.h"

/* Channels mapped to the front-panel shift register */
#define CH_WIFI_LED      1   /* Raw GPIO P0 (LED_n), synced with Wireless button */
#define CH_POWER_LED     2   /* Raw GPIO P28 (LED_n), synced with Power button */
#define CH_DOOR_SENSOR   3   /* Raw GPIO P8 (dInput_n), synced with '!' icon */
#define CH_INK_LED       4   /* Shift-reg bit 8 (Ink/Cancel orange LED), Cancel button */
#define CH_RESUME_LED    5   /* Shift-reg bit 9 (Resume orange LED), Resume button */
#define CH_INFO_LED      6   /* Shift-reg bit 11 (Info orange LED), Info button */
#define CH_COLOR_INK     7   /* Color ink 3-bar gauge + outline, Copy Color button */
#define CH_BLACK_INK     8   /* Black ink 3-bar gauge + outline, Copy Black button */

/* Shift-register button input bit numbers (active-low in raw frame) */
#define BTN_BIT_POWER       1
#define BTN_BIT_RESUME      2
#define BTN_BIT_WIRELESS    3
#define BTN_BIT_CANCEL      6
#define BTN_BIT_INFO        7
#define BTN_BIT_COPY_COLOR  8
#define BTN_BIT_COPY_BLACK  9

/* Shift-register indicator output bit numbers */
#define IND_BIT_INK_LED     8
#define IND_BIT_RESUME_LED  9
#define IND_BIT_INFO_LED    11
#define IND_BIT_WIFI_ICON   12
#define IND_BIT_PAPER_ICON  13
#define IND_BIT_EXCLAMATION 14
#define IND_BIT_LETTER_E    15
#define IND_BIT_WIFI_DIRECT 20
#define IND_BIT_INK_DROP    21
#define IND_BIT_DROP_PAPER  25
#define IND_BIT_WIRELESS_EX 32
#define PANEL_ALL_TEST_BITS 0x00000001FFFFFB00ULL

#define PANEL_POLL_INTERVAL_MS 350u
#define IP_SCROLL_STEP_MS      650u
#define IP_SCROLL_ON_MS        480u

static int      g_panel_running    = 0;
static int      g_panel_dirty      = 1;
static int      g_in_channel_sync  = 0;
static char     g_lcd_char         = '0';
static int      g_manual_wifi_bars = -1; /* -1 = auto from Wi-Fi RSSI / Ch 1 */
static int      g_color_bars       = 3;
static int      g_black_bars       = 3;
static uint64_t g_extra_icons      = 0;

static uint32_t g_cur_lo           = 0x0F00u;
static uint32_t g_cur_hi           = 0;
static uint32_t g_last_shift_ms    = 0;
static uint32_t g_prev_pressed     = 0;
static uint32_t g_test_until_ms    = 0;

/* IP address scroll state (triggered by Info button) */
static char     g_scroll_seq[24];
static unsigned g_scroll_len       = 0;
static uint32_t g_scroll_start_ms  = 0;

static int rssi_to_bars(int dbm)
{
	if (dbm == 0)
		return 4;
	if (dbm >= -55)
		return 5;
	if (dbm >= -62)
		return 4;
	if (dbm >= -70)
		return 3;
	if (dbm >= -78)
		return 2;
	return 1;
}

static void start_ip_scroll(void)
{
	const char *ip = HAL_GetMyIPString();
	if (!ip || !ip[0] || !strcmp(ip, "0.0.0.0")) {
		strcpy(g_scroll_seq, "-");
	} else {
		unsigned i;
		strncpy(g_scroll_seq, ip, sizeof(g_scroll_seq) - 1);
		g_scroll_seq[sizeof(g_scroll_seq) - 1] = '\0';
		for (i = 0; g_scroll_seq[i]; i++) {
			if (g_scroll_seq[i] == '.')
				g_scroll_seq[i] = '-';
		}
	}
	g_scroll_len = (unsigned)strlen(g_scroll_seq);
	g_scroll_start_ms = timer_uptime_ms();
}

static void rebuild_frame_state(void)
{
	uint32_t now = timer_uptime_ms();
	if (g_test_until_ms && (int32_t)(g_test_until_ms - now) > 0) {
		panel_encode('8', 5, 3, 3, PANEL_ALL_TEST_BITS, &g_cur_lo, &g_cur_hi);
		return;
	}
	g_test_until_ms = 0;

	char ch = g_lcd_char;
	if (g_scroll_len > 0) {
		uint32_t elapsed = now - g_scroll_start_ms;
		uint32_t idx = elapsed / IP_SCROLL_STEP_MS;
		if (idx >= g_scroll_len) {
			g_scroll_len = 0;
		} else {
			ch = ((elapsed % IP_SCROLL_STEP_MS) < IP_SCROLL_ON_MS) ? g_scroll_seq[idx] : ' ';
		}
	}

	int wifi_bars = g_manual_wifi_bars;
	if (wifi_bars < 0) {
		if (CHANNEL_Check(CH_WIFI_LED))
			wifi_bars = rssi_to_bars(wifi_last_rssi());
		else
			wifi_bars = 0;
	}

	int col_bars = CHANNEL_Check(CH_COLOR_INK) ? g_color_bars : 0;
	int blk_bars = CHANNEL_Check(CH_BLACK_INK) ? g_black_bars : 0;

	uint64_t icons = g_extra_icons;
	if (CHANNEL_Check(CH_INK_LED))
		icons |= (1ULL << IND_BIT_INK_LED);
	if (CHANNEL_Check(CH_RESUME_LED))
		icons |= (1ULL << IND_BIT_RESUME_LED);
	if (CHANNEL_Check(CH_INFO_LED))
		icons |= (1ULL << IND_BIT_INFO_LED);

	/* Auto-light '!' icon when the cartridge access door (P8) is open */
	if (sensor_door_active())
		icons |= (1ULL << IND_BIT_EXCLAMATION);

	if (panel_encode(ch, wifi_bars, col_bars, blk_bars, icons, &g_cur_lo, &g_cur_hi) != 0)
		panel_encode(' ', wifi_bars, col_bars, blk_bars, icons, &g_cur_lo, &g_cur_hi);
}

static void handle_button_edge(uint32_t newly_pressed)
{
	if (!newly_pressed)
		return;

	g_in_channel_sync = 1;
	if (newly_pressed & (1u << BTN_BIT_WIRELESS))
		CHANNEL_Toggle(CH_WIFI_LED);
	if (newly_pressed & (1u << BTN_BIT_POWER))
		CHANNEL_Toggle(CH_POWER_LED);
	if (newly_pressed & (1u << BTN_BIT_CANCEL))
		CHANNEL_Toggle(CH_INK_LED);
	if (newly_pressed & (1u << BTN_BIT_RESUME))
		CHANNEL_Toggle(CH_RESUME_LED);
	if (newly_pressed & (1u << BTN_BIT_INFO)) {
		CHANNEL_Toggle(CH_INFO_LED);
		start_ip_scroll();
	}
	if (newly_pressed & (1u << BTN_BIT_COPY_COLOR)) {
		CHANNEL_Toggle(CH_COLOR_INK);
		g_lcd_char = 'C';
	}
	if (newly_pressed & (1u << BTN_BIT_COPY_BLACK)) {
		CHANNEL_Toggle(CH_BLACK_INK);
		g_lcd_char = 'b';
	}
	g_in_channel_sync = 0;
	rebuild_frame_state();
	g_panel_dirty = 1;
}

static commandResult_t Cmd_HPPanel_Char(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	if (!args || !args[0]) {
		g_lcd_char = ' ';
	} else {
		while (*args == ' ')
			args++;
		g_lcd_char = (*args == '_' || !*args) ? ' ' : *args;
	}
	g_scroll_len = 0;
	rebuild_frame_state();
	g_panel_dirty = 1;
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPPanel_Bars(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() >= 1) {
		int w = Tokenizer_GetArgInteger(0);
		g_manual_wifi_bars = (w < 0) ? -1 : (w > 5 ? 5 : w);
	}
	if (Tokenizer_GetArgsCount() >= 2) {
		int c = Tokenizer_GetArgInteger(1);
		g_color_bars = (c < 0) ? 0 : (c > 3 ? 3 : c);
		CHANNEL_Set(CH_COLOR_INK, g_color_bars > 0 ? 1 : 0, 0);
	}
	if (Tokenizer_GetArgsCount() >= 3) {
		int b = Tokenizer_GetArgInteger(2);
		g_black_bars = (b < 0) ? 0 : (b > 3 ? 3 : b);
		CHANNEL_Set(CH_BLACK_INK, g_black_bars > 0 ? 1 : 0, 0);
	}
	rebuild_frame_state();
	g_panel_dirty = 1;
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPPanel_Icon(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	const char *arg0 = Tokenizer_GetArg(0);
	int bit = panel_indicator_bit(arg0);
	if (bit < 0)
		bit = Tokenizer_GetArgInteger(0);
	if (bit < 8 || bit > 45)
		return CMD_RES_BAD_ARGUMENT;
	if (Tokenizer_GetArgsCount() >= 2) {
		int on = Tokenizer_GetArgInteger(1);
		if (on)
			g_extra_icons |= (1ULL << bit);
		else
			g_extra_icons &= ~(1ULL << bit);
	} else {
		g_extra_icons ^= (1ULL << bit);
	}
	rebuild_frame_state();
	g_panel_dirty = 1;
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPPanel_Test(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)args; (void)cmdFlags;
	g_test_until_ms = timer_uptime_ms() + 1500u;
	rebuild_frame_state();
	g_panel_dirty = 1;
	return CMD_RES_OK;
}

void HP2620Panel_Init(void)
{
	/* Configure labels & toggle types for both raw LEDs and shift-register channels */
	CHANNEL_SetLabel(CH_WIFI_LED,    "WiFi LED (P0)",    0);
	CHANNEL_SetLabel(CH_POWER_LED,   "Power LED (P28)",  0);
	CHANNEL_SetLabel(CH_DOOR_SENSOR, "Cartridge Door",   0);
	CHANNEL_SetType(CH_DOOR_SENSOR,  ChType_OpenClosed_Inv);

	CHANNEL_SetType(CH_INK_LED,      ChType_Toggle);
	CHANNEL_SetLabel(CH_INK_LED,     "Panel Ink LED",    0);

	CHANNEL_SetType(CH_RESUME_LED,   ChType_Toggle);
	CHANNEL_SetLabel(CH_RESUME_LED,  "Panel Resume LED", 0);

	CHANNEL_SetType(CH_INFO_LED,     ChType_Toggle);
	CHANNEL_SetLabel(CH_INFO_LED,    "Panel Info LED",   0);

	CHANNEL_SetType(CH_COLOR_INK,    ChType_Toggle);
	CHANNEL_SetLabel(CH_COLOR_INK,   "LCD Color Ink",    0);

	CHANNEL_SetType(CH_BLACK_INK,    ChType_Toggle);
	CHANNEL_SetLabel(CH_BLACK_INK,   "LCD Black Ink",    0);

	/* Default ink gauges on so the printer LCD shows full ink bars + Wi-Fi + '0' */
	g_in_channel_sync = 1;
	CHANNEL_Set(CH_COLOR_INK, 1, 0);
	CHANNEL_Set(CH_BLACK_INK, 1, 0);
	g_in_channel_sync = 0;

	CMD_RegisterCommand("HPPanel_Char", Cmd_HPPanel_Char, NULL);
	CMD_RegisterCommand("HPPanel_Bars", Cmd_HPPanel_Bars, NULL);
	CMD_RegisterCommand("HPPanel_Icon", Cmd_HPPanel_Icon, NULL);
	CMD_RegisterCommand("HPPanel_Test", Cmd_HPPanel_Test, NULL);

	g_panel_running = 1;
	g_panel_dirty   = 1;
	rebuild_frame_state();
}

void HP2620Panel_Stop(void)
{
	g_panel_running = 0;
}

void HP2620Panel_OnChannelChanged(int ch, int val)
{
	(void)val;
	if (!g_panel_running || g_in_channel_sync)
		return;
	if (ch >= CH_WIFI_LED && ch <= CH_BLACK_INK) {
		rebuild_frame_state();
		g_panel_dirty = 1;
	}
}

void HP2620Panel_OnEverySecond(void)
{
	if (!g_panel_running)
		return;
	rebuild_frame_state();
}

static int http_tx_busy(void)
{
	for (int i = 0; i < TCP_MAX_CONN; i++) {
		tcp_conn_t *c = tcp_conn_by_index(i);
		if (c && tcp_local_port(c) == 80 && tcp_send_space(c) < TCP_SNDBUF)
			return 1;
	}
	return 0;
}

void HP2620Panel_RunQuickTick(void)
{
	if (!g_panel_running)
		return;
	if (http_tx_busy())
		return;

	uint32_t now = timer_uptime_ms();
	uint32_t interval = (g_scroll_len > 0 || g_test_until_ms != 0) ? 180u : PANEL_POLL_INTERVAL_MS;
	if (!g_panel_dirty && (uint32_t)(now - g_last_shift_ms) < interval)
		return;

	g_panel_dirty = 0;
	rebuild_frame_state();

	int ok = 0;
	uint32_t raw = panel_shift(g_cur_lo, g_cur_hi, &ok);
	/* Record timestamp AFTER panel_shift() so we always leave >=350ms of pure CPU time */
	g_last_shift_ms = timer_uptime_ms();

	if (ok) {
		uint32_t pressed = (~raw) & PANEL_BUTTON_MASK;
		uint32_t newly = pressed & ~g_prev_pressed;
		g_prev_pressed = pressed;
		if (newly)
			handle_button_edge(newly);
	}
}

static void process_http_panel_args(http_request_t *request)
{
	char tmp[32];
	if (http_getArg(request->url, "hp_ch", tmp, sizeof(tmp))) {
		g_lcd_char = (tmp[0] == '_' || tmp[0] == '\0') ? ' ' : tmp[0];
		g_scroll_len = 0;
		rebuild_frame_state();
		g_panel_dirty = 1;
	}
	if (http_getArg(request->url, "hp_ico", tmp, sizeof(tmp))) {
		int bit = atoi(tmp);
		if (bit >= 8 && bit <= 45) {
			g_extra_icons ^= (1ULL << bit);
			rebuild_frame_state();
			g_panel_dirty = 1;
		}
	}
	if (http_getArg(request->url, "hp_btn", tmp, sizeof(tmp))) {
		if (!strcmp(tmp, "test")) {
			g_test_until_ms = timer_uptime_ms() + 1500u;
			rebuild_frame_state();
			g_panel_dirty = 1;
		} else {
			uint32_t mask = 0;
			for (int i = 0; i < 18; i++) {
				const char *nm = panel_button_name(i);
				if (nm && !strcmp(nm, tmp))
					mask = (1u << i);
			}
			if (mask)
				handle_button_edge(mask);
		}
	}
}

static int bit_on(uint64_t logical, int n)
{
	return (int)((logical >> n) & 1ULL);
}

static void hp_pprintf(http_request_t *request, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	poststr(request, buf);
}

static void svg_rect(http_request_t *request, int x, int y, int w, int h, int on)
{
	hp_pprintf(request,
		"<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"2\" fill=\"%s\"/>",
		x, y, w, h, on ? "#fbbf24" : "#334155");
}

static void svg_text(http_request_t *request, int x, int y, const char *txt, int on)
{
	hp_pprintf(request,
		"<text x=\"%d\" y=\"%d\" font-size=\"10\" fill=\"%s\">%s</text>",
		x, y, on ? "#fbbf24" : "#475569", txt);
}

static void svg_led(http_request_t *request, int cx, int cy, int on, const char *color, const char *label)
{
	hp_pprintf(request,
		"<circle cx=\"%d\" cy=\"%d\" r=\"6\" fill=\"%s\"/>"
		"<text x=\"%d\" y=\"%d\" font-size=\"10\" fill=\"%s\">%s</text>",
		cx, cy, on ? color : "#334155",
		cx + 10, cy + 4, on ? "#f8fafc" : "#64748b", label);
}

void HP2620Panel_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	if (!g_panel_running)
		return;

	process_http_panel_args(request);

	if (bPreState) {
		/* Interactive control bar rendered once above <div id="state"> */
		poststr(request,
			"<style>"
			".hp-b{width:auto;display:inline-block;padding:2px 7px;font-size:0.78em;line-height:1.45rem;border-radius:4px;margin:0}"
			"</style>"
			"<script>"
			"function hpCmd(q){"
			"fetch('index?state=1&'+q).then(r=>r.text()).then(t=>{"
			"var s=document.getElementById('state');if(s)s.innerHTML=t;"
			"});}"
			"</script>"
			"<div style=\"background:#1e293b;border:1px solid #334155;border-radius:8px;padding:8px;margin:6px 0;text-align:center\">"
			"<div style=\"font-size:0.85em;margin-bottom:4px;color:#94a3b8\"><b>HP DeskJet 2620 Panel Controls</b> (7-Seg / Buttons / Icons)</div>"
			"<div style=\"display:flex;flex-wrap:wrap;gap:3px;justify-content:center;margin-bottom:5px\">");

		static const char lcd_chars[] = "0123456789AbCdEFJPUn-_";
		for (unsigned i = 0; i < sizeof(lcd_chars) - 1; i++) {
			char c = lcd_chars[i];
			const char *lbl = (c == '_') ? "CLR" : NULL;
			if (lbl) {
				hp_pprintf(request,
					"<button type=\"button\" class=\"hp-b\" style=\"background:#475569\" onclick=\"hpCmd('hp_ch=_')\">CLR</button>");
			} else {
				hp_pprintf(request,
					"<button type=\"button\" class=\"hp-b\" style=\"background:#334155\" onclick=\"hpCmd('hp_ch=%c')\">%c</button>",
					c, c);
			}
		}
		poststr(request, "</div><div style=\"display:flex;flex-wrap:wrap;gap:4px;justify-content:center\">");
		static const struct { const char *id; const char *label; } btns[] = {
			{ "power",      "Power" },
			{ "wireless",   "Wireless" },
			{ "info",       "Info (IP)" },
			{ "cancel",     "Cancel" },
			{ "resume",     "Resume" },
			{ "copy_color", "Copy Color" },
			{ "copy_black", "Copy Black" },
			{ "test",       "LCD Test" },
		};
		for (unsigned i = 0; i < sizeof(btns) / sizeof(btns[0]); i++) {
			hp_pprintf(request,
				"<button type=\"button\" class=\"hp-b\" style=\"background:#0284c7\" onclick=\"hpCmd('hp_btn=%s')\">%s</button>",
				btns[i].id, btns[i].label);
		}
		static const struct { int bit; const char *label; } icos[] = {
			{ 13, "Paper" }, { 14, "!" }, { 15, "E" },
			{ 20, "Direct" }, { 21, "InkDrop" }, { 25, "DropPaper" }, { 32, "W!" }
		};
		for (unsigned i = 0; i < sizeof(icos) / sizeof(icos[0]); i++) {
			hp_pprintf(request,
				"<button type=\"button\" class=\"hp-b\" style=\"background:#475569\" onclick=\"hpCmd('hp_ico=%d')\">[%s]</button>",
				icos[i].bit, icos[i].label);
		}
		poststr(request, "</div></div>");
		return;
	}

	/* Live SVG Front Panel & LCD clone rendered inside <div id="state"> */
	rebuild_frame_state();
	uint64_t logical = (((uint64_t)g_cur_hi << 32) | (uint64_t)g_cur_lo) ^ 0x0F00ULL;

	poststr(request,
		"<div style=\"background:#0f172a;border:1px solid #334155;border-radius:8px;padding:8px;margin:6px 0;text-align:center\">"
		"<svg width=\"320\" height=\"96\" viewBox=\"0 0 320 96\" style=\"max-width:100%;height:auto\">");

	/* 7-segment digit (bits 39, 38, 37, 36, 33, 35, 34) */
	svg_rect(request, 16,  8, 26,  5, bit_on(logical, 39)); /* A */
	svg_rect(request, 42, 13,  5, 28, bit_on(logical, 38)); /* B */
	svg_rect(request, 42, 46,  5, 28, bit_on(logical, 37)); /* C */
	svg_rect(request, 16, 74, 26,  5, bit_on(logical, 36)); /* D */
	svg_rect(request, 11, 46,  5, 28, bit_on(logical, 33)); /* E */
	svg_rect(request, 11, 13,  5, 28, bit_on(logical, 35)); /* F */
	svg_rect(request, 16, 41, 26,  5, bit_on(logical, 34)); /* G */

	/* 5 Wi-Fi signal bars (bits 24, 28, 29, 30, 31) */
	static const int wifi_bits[5] = { 24, 28, 29, 30, 31 };
	for (int i = 0; i < 5; i++)
		svg_rect(request, 64 + i * 9, 70 - i * 12, 6, 9 + i * 12, bit_on(logical, wifi_bits[i]));

	/* Color (j=0) and Black (j=1) 3-bar ink gauges + outlines */
	static const int gauge_bits[2][3] = { { 16, 18, 17 }, { 27, 22, 26 } };
	static const int outline_bits[2]  = { 19, 23 };
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 3; i++)
			svg_rect(request, 120 + j * 22, 58 - i * 20, 14, 16, bit_on(logical, gauge_bits[j][i]));
		hp_pprintf(request,
			"<rect x=\"%d\" y=\"16\" width=\"18\" height=\"62\" fill=\"none\" stroke=\"%s\"/>",
			118 + j * 22, bit_on(logical, outline_bits[j]) ? "#fbbf24" : "#334155");
	}

	/* 8 LCD status icons */
	static const struct { const char *name; int bit; } lcd_icons[8] = {
		{ "Wi-Fi",  12 }, { "paper", 13 },
		{ "!",      14 }, { "E",     15 },
		{ "Direct", 20 }, { "ink",   21 },
		{ "drop",   25 }, { "w!",    32 },
	};
	for (int i = 0; i < 8; i++)
		svg_text(request, 172 + (i % 2) * 40, 18 + (i >> 1) * 16, lcd_icons[i].name, bit_on(logical, lcd_icons[i].bit));

	/* 3 Orange button LEDs (ink=8, resume=9, info=11) */
	svg_led(request, 262, 20, bit_on(logical, 8),  "#f97316", "ink");
	svg_led(request, 262, 42, bit_on(logical, 9),  "#f97316", "resume");
	svg_led(request, 262, 64, bit_on(logical, 11), "#f97316", "info");

	/* Raw GPIO Power LED (P28) and Wireless LED (P0) */
	int pwr_on  = (GPIO_DIR(GPIO_PIN_POWER_LED) & 1) && !(GPIO_OUTPUT(GPIO_PIN_POWER_LED) & 1);
	int wifi_on = (GPIO_DIR(GPIO_PIN_WIFI_LED) & 1)  && !(GPIO_OUTPUT(GPIO_PIN_WIFI_LED) & 1);
	svg_led(request, 16, 90, pwr_on,  "#22c55e", "power");
	svg_led(request, 72, 90, wifi_on, "#3b82f6", "wireless");

	poststr(request, "</svg>");

	/* Compact telemetry bar below the SVG */
	uint32_t pressed = (~g_panel_last_raw) & PANEL_BUTTON_MASK;
	const char *btn_str = "none";
	for (int b = 0; b < 18; b++) {
		if (pressed & (1u << b)) {
			const char *nm = panel_button_name(b);
			if (nm) {
				btn_str = nm;
				break;
			}
		}
	}
	hp_pprintf(request,
		"<div style=\"font-size:0.78em;color:#94a3b8;margin-top:2px\">"
		"LCD frame <code>%08x:%08x</code> (%u shifts) | Button: <b>%s</b> | Door: <b style=\"color:%s\">%s</b> | Wi-Fi: <b>%d dBm</b>"
		"</div></div>",
		(unsigned)g_cur_hi, (unsigned)g_cur_lo, (unsigned)g_panel_frames,
		btn_str,
		sensor_door_active() ? "#f87171" : "#4ade80",
		sensor_door_active() ? "OPEN" : "CLOSED",
		wifi_last_rssi());
}

#endif /* PLATFORM_88PAAX01 */
