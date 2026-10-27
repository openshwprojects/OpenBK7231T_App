#include "../obk_config.h"

#if ENABLE_DRIVER_BKCHARGE

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

#define BKCHARGE_ADDR_BASE   (0x008001C0)
#define BKCHARGE_REG0X2      (BKCHARGE_ADDR_BASE + 2*4)

#define BKCharge_EnabledGet()   ((REG_READ(SCTRL_ANALOG_CTRL7) & CHG_ENABLE) ? 1 : 0)
#define BKCharge_CurrentGet()   ((REG_READ(SCTRL_ANALOG_CTRL7) >> CHG_CURRENT_CONTROL_POS) & CHG_CURRENT_CONTROL_MASK)
#define BKCharge_VoutGet()      ((REG_READ(SCTRL_ANALOG_CTRL6) >> CHG_VOUT_SELECT_POS) & CHG_VOUT_SELECT_MASK)

// Order follows CHARGE_INDEX in charge_pub.h; state bit n is reg0x2 bit n+8
static const char *g_bkchg_names[8] = {
	"recharge", "vcal", "trickle", "full",
	"ical", "cv", "cc", "usb"
};

static UINT8 g_bkchg_state = 0;
static UINT8 g_bkchg_prev = 0;
static bool g_bkchg_havePrev = false;
static int g_bkchg_channel = -1;

static UINT8 BKCharge_ReadState(void)
{
	return (UINT8)((REG_READ(BKCHARGE_REG0X2) >> 8) & 0xFF);
}

static int BKCharge_IsCharging(UINT8 st)
{
	if (!BKCharge_EnabledGet())
		return 0;
	return (st & ((1 << CHARGE_2_TRICK) | (1 << CHARGE_5_CV) | (1 << CHARGE_6_CC))) ? 1 : 0;
}

static void BKCharge_Publish(UINT8 st)
{
	int charging = BKCharge_IsCharging(st);

#if ENABLE_MQTT
	MQTT_PublishMain_StringInt("charging", charging, 0);
	MQTT_PublishMain_StringInt("charge_full", (st & (1 << CHARGE_3_TERMINAL)) ? 1 : 0, 0);
	MQTT_PublishMain_StringInt("usb_power", (st & (1 << CHARGE_7_USB_READY)) ? 1 : 0, 0);
#endif
	if (g_bkchg_channel >= 0)
		CHANNEL_Set(g_bkchg_channel, charging, 0);
}

static const char *BKCharge_PhaseName(UINT8 st)
{
	if (!BKCharge_EnabledGet())
		return "off";
	if (st & (1 << CHARGE_6_CC))
		return "CC";
	if (st & (1 << CHARGE_5_CV))
		return "CV";
	if (st & (1 << CHARGE_2_TRICK))
		return "trickle";
	return "idle";
}

static commandResult_t BKCharge_Cmd_State(const void *context, const char *cmd,
	const char *args, int cmdFlags)
{
	UINT8 st = BKCharge_ReadState();
	int i;

	ADDLOG_INFO(LOG_FEATURE_CMD, "Charger: %s, %s, Ilim 0x%02X, Vout 0x%X, raw 0x%02X",
		BKCharge_PhaseName(st), BKCharge_EnabledGet() ? "enabled" : "disabled",
		(unsigned)BKCharge_CurrentGet(), (unsigned)BKCharge_VoutGet(), st);
	if (!BKCharge_EnabledGet()) {
		ADDLOG_INFO(LOG_FEATURE_CMD,
			"Charger: off, the status bits below do not track the cell while it is off");
	}
	for (i = 0; i < 8; i++) {
		if (st & (1 << i))
			ADDLOG_INFO(LOG_FEATURE_CMD, "Charger: %s", g_bkchg_names[i]);
	}
	return CMD_RES_OK;
}

static commandResult_t BKCharge_Cmd_Enable(const void *context, const char *cmd,
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
	BKCharge_Publish(BKCharge_ReadState());
	ADDLOG_INFO(LOG_FEATURE_CMD, "ChargeEnable: now %i", BKCharge_EnabledGet());
	return CMD_RES_OK;
}

static commandResult_t BKCharge_Cmd_Channel(const void *context, const char *cmd,
	const char *args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	g_bkchg_channel = Tokenizer_GetArgInteger(0);
	BKCharge_Publish(BKCharge_ReadState());
	ADDLOG_INFO(LOG_FEATURE_CMD, "ChargeChannel: publishing charging state to channel %i",
	            g_bkchg_channel);
	return CMD_RES_OK;
}

void BKCharge_Init(void)
{
	g_bkchg_state = BKCharge_ReadState();

	//cmddetail:{"name":"ChargeChannel","args":"[ChannelIndex]",
	//cmddetail:"descr":"Publishes the charging state to the given channel, 1 while the charger is in trickle, CC or CV.",
	//cmddetail:"fn":"BKCharge_Cmd_Channel","file":"driver/drv_bkCharge.c","requires":"",
	//cmddetail:"examples":"ChargeChannel 5"}
	CMD_RegisterCommand("ChargeChannel", BKCharge_Cmd_Channel, NULL);

	//cmddetail:{"name":"ChargeEnable","args":"[0or1]",
	//cmddetail:"descr":"Enables or disables the on-chip charger. The SDK configures and calibrates it but leaves it off, so this is what actually starts charging.",
	//cmddetail:"fn":"BKCharge_Cmd_Enable","file":"driver/drv_bkCharge.c","requires":"",
	//cmddetail:"examples":"ChargeEnable 1"}
	CMD_RegisterCommand("ChargeEnable", BKCharge_Cmd_Enable, NULL);

	//cmddetail:{"name":"ChargeState","args":"",
	//cmddetail:"descr":"Logs the charger phase, whether it is enabled, the current limit and the raw status bits.",
	//cmddetail:"fn":"BKCharge_Cmd_State","file":"driver/drv_bkCharge.c","requires":"",
	//cmddetail:"examples":"ChargeState"}
	CMD_RegisterCommand("ChargeState", BKCharge_Cmd_State, NULL);

	ADDLOG_INFO(LOG_FEATURE_DRV, "CHG: charger state 0x%02X", g_bkchg_state);
}

void BKCharge_OnEverySecond(void)
{
	g_bkchg_state = BKCharge_ReadState();
	if (!g_bkchg_havePrev || g_bkchg_state != g_bkchg_prev) {
		ADDLOG_INFO(LOG_FEATURE_DRV, "CHG: state 0x%02X -> 0x%02X",
		            g_bkchg_prev, g_bkchg_state);
		g_bkchg_prev = g_bkchg_state;
		g_bkchg_havePrev = true;
		BKCharge_Publish(g_bkchg_state);
	}
}

void BKCharge_StopDriver(void)
{
}

void BKCharge_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	UINT8 st;

	if (bPreState)
		return;
	st = BKCharge_ReadState();
	if (!BKCharge_EnabledGet()) {
		hprintf255(request, "<h5>Charger: off, Ilim 0x%02X (raw 0x%02X)</h5>",
		           (unsigned)BKCharge_CurrentGet(), st);
		return;
	}
	hprintf255(request, "<h5>Charger: %s, Ilim 0x%02X, USB %s (raw 0x%02X)</h5>",
	           BKCharge_PhaseName(st),
	           (unsigned)BKCharge_CurrentGet(),
	           (st & (1 << CHARGE_7_USB_READY)) ? "yes" : "no", st);
}

#endif
