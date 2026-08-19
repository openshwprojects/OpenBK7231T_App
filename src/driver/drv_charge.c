#include "../obk_config.h"

#if ENABLE_DRIVER_CHARGE

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "drv_local.h"

#include "include.h"
#include "arm_arch.h"
#include "charge_pub.h"
#include "sys_ctrl.h"

#define CHG_ADDR_BASE   (0x008001C0)
#define CHG_REG0X2      (CHG_ADDR_BASE + 2*4)

#define CHG_EnabledGet()   ((REG_READ(SCTRL_ANALOG_CTRL7) & CHG_ENABLE) ? 1 : 0)
#define CHG_CurrentGet()   ((REG_READ(SCTRL_ANALOG_CTRL7) >> CHG_CURRENT_CONTROL_POS) & CHG_CURRENT_CONTROL_MASK)
#define CHG_VoutGet()      ((REG_READ(SCTRL_ANALOG_CTRL6) >> CHG_VOUT_SELECT_POS) & CHG_VOUT_SELECT_MASK)

// Order follows CHARGE_INDEX in charge_pub.h; state bit n is reg0x2 bit n+8
static const char *g_chg_names[8] = {
	"recharge", "vcal", "trickle", "full",
	"ical", "cv", "cc", "usb"
};

static UINT8 g_chg_state = 0;
static UINT8 g_chg_prev = 0;
static int g_chg_channel = -1;

static UINT8 CHG_ReadState(void)
{
	return (UINT8)((REG_READ(CHG_REG0X2) >> 8) & 0xFF);
}

static int CHG_IsCharging(UINT8 st)
{
	if (!CHG_EnabledGet())
		return 0;
	return (st & ((1 << CHARGE_2_TRICK) | (1 << CHARGE_5_CV) | (1 << CHARGE_6_CC))) ? 1 : 0;
}

static void CHG_Publish(UINT8 st)
{
#if ENABLE_MQTT
	MQTT_PublishMain_StringInt("charging", CHG_IsCharging(st), 0);
	MQTT_PublishMain_StringInt("charge_full", (st & (1 << CHARGE_3_TERMINAL)) ? 1 : 0, 0);
	MQTT_PublishMain_StringInt("usb_power", (st & (1 << CHARGE_7_USB_READY)) ? 1 : 0, 0);
#endif
	if (g_chg_channel >= 0)
		CHANNEL_Set(g_chg_channel, CHG_IsCharging(st), 0);
}

static const char *CHG_PhaseName(UINT8 st)
{
	if (st & (1 << CHARGE_6_CC))
		return "CC";
	if (st & (1 << CHARGE_5_CV))
		return "CV";
	if (st & (1 << CHARGE_2_TRICK))
		return "trickle";
	return "idle";
}

static commandResult_t CHG_Cmd_Enable(const void *context, const char *cmd,
	const char *args, int cmdFlags)
{
	UINT32 v;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;

	v = REG_READ(SCTRL_ANALOG_CTRL7);
	if (Tokenizer_GetArgInteger(0))
		v |= CHG_ENABLE;
	else
		v &= ~CHG_ENABLE;
	REG_WRITE(SCTRL_ANALOG_CTRL7, v);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ChargeEnable: now %i", CHG_EnabledGet());
	return CMD_RES_OK;
}

static commandResult_t CHG_Cmd_Channel(const void *context, const char *cmd,
	const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_chg_channel = Tokenizer_GetArgInteger(0);
	ADDLOG_INFO(LOG_FEATURE_CMD, "ChargeChannel: publishing charging state to channel %i",
	            g_chg_channel);
	return CMD_RES_OK;
}

static int CHG_Http(http_request_t *request)
{
	UINT8 st = CHG_ReadState();
	char line[96];
	int i;

	poststr(request, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
	        "Connection: close\r\n\r\n");
	for (i = 0; i < 8; i++) {
		snprintf(line, sizeof(line), "%-9s %u\n", g_chg_names[i], (st >> i) & 1u);
		poststr(request, line);
	}
	snprintf(line, sizeof(line), "phase %s, charging %i\n", CHG_PhaseName(st), CHG_IsCharging(st));
	poststr(request, line);
	snprintf(line, sizeof(line), "enabled %i, current 0x%02X, vout_sel 0x%X\n",
	         CHG_EnabledGet(), (unsigned)CHG_CurrentGet(), (unsigned)CHG_VoutGet());
	poststr(request, line);
	snprintf(line, sizeof(line), "raw 0x%02X, analog6 0x%08X, analog7 0x%08X\n",
	         st, REG_READ(SCTRL_ANALOG_CTRL6), REG_READ(SCTRL_ANALOG_CTRL7));
	poststr(request, line);
	return 0;
}

void CHG_Init(void)
{
	g_chg_state = g_chg_prev = CHG_ReadState();
	HTTP_RegisterCallback("/charge", HTTP_GET, CHG_Http, 0);

	//cmddetail:{"name":"ChargeChannel","args":"[ChannelIndex]",
	//cmddetail:"descr":"Publishes the charging state to the given channel, 1 while the charger is in trickle, CC or CV.",
	//cmddetail:"fn":"CHG_Cmd_Channel","file":"driver/drv_charge.c","requires":"",
	//cmddetail:"examples":"ChargeChannel 5"}
	CMD_RegisterCommand("ChargeChannel", CHG_Cmd_Channel, NULL);

	//cmddetail:{"name":"ChargeEnable","args":"[0or1]",
	//cmddetail:"descr":"Enables or disables the on-chip charger. The SDK configures and calibrates it but leaves it off, so this is what actually starts charging.",
	//cmddetail:"fn":"CHG_Cmd_Enable","file":"driver/drv_charge.c","requires":"",
	//cmddetail:"examples":"ChargeEnable 1"}
	CMD_RegisterCommand("ChargeEnable", CHG_Cmd_Enable, NULL);

	ADDLOG_INFO(LOG_FEATURE_DRV, "CHG: charger state 0x%02X", g_chg_state);
}

void CHG_OnEverySecond(void)
{
	g_chg_state = CHG_ReadState();
	if (g_chg_state != g_chg_prev) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "CHG: state 0x%02X -> 0x%02X",
		            g_chg_prev, g_chg_state);
		g_chg_prev = g_chg_state;
		CHG_Publish(g_chg_state);
	}
}

void CHG_StopDriver(void)
{
}

void CHG_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	UINT8 st;

	if (bPreState)
		return;
	st = CHG_ReadState();
	hprintf255(request, "<h5>Charger: %s, %s, Ilim 0x%02X, USB %s (0x%02X)</h5>",
	           CHG_PhaseName(st),
	           CHG_EnabledGet() ? "enabled" : "disabled",
	           (unsigned)CHG_CurrentGet(),
	           (st & (1 << CHARGE_7_USB_READY)) ? "yes" : "no", st);
}

#endif
