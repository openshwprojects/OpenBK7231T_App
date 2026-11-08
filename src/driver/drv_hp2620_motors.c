/*
 * drv_hp2620_motors.c - OpenBeken driver for the HP DeskJet 2620 MCASIC DC motors,
 *                       quadrature optical encoders, and optical/door sensors.
 *
 * Exposes:
 *   - Motor A (0x31): Paper Feed DC motor (FWD / REV)
 *   - Motor B (0x33): Printhead Carriage DC motor + quadrature encoder (Home, Range, Goto)
 *   - Motor C (0x35): Flatbed Scanner DC motor + quadrature encoder (Home, Range, Goto)
 *   - Live MCASIC & optical sensor telemetry inside OpenBeken's /index state
 *   - Console commands: HPMotor_Init, HPMotor_Stop, HPMotor_Pulse, HPMotor_Home, HPMotor_Goto
 */
#include "drv_hp2620_motors.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#ifdef PLATFORM_88PAAX01

#include "hw.h"
#include "timer.h"
#include "periph.h"
#include "periph_cmds.h"
#include "tcp.h"

enum pending_motor_op {
	MOP_NONE = 0,
	MOP_HOME,
	MOP_RANGE,
	MOP_GOTO,
};

static int               g_motors_running = 0;
static char              g_motor_msg[96]  = "ready";
static enum pending_motor_op g_pending_op = MOP_NONE;
static int               g_pending_m      = 1;
static int               g_pending_mode   = 1;
static uint32_t          g_pending_pwm    = 350u;
static int32_t           g_pending_target = 0;
static int32_t           g_last_span[3]   = { 0, 0, 0 };

static int parse_motor_idx(const char *s)
{
	if (!s || !s[0])
		return -1;
	if (s[0] == 'a' || s[0] == 'A' || s[0] == '0')
		return 0;
	if (s[0] == 'b' || s[0] == 'B' || s[0] == '1')
		return 1;
	if (s[0] == 'c' || s[0] == 'C' || s[0] == '2')
		return 2;
	return -1;
}

static int parse_motor_dir(const char *s)
{
	if (!s || !s[0])
		return 0;
	if (!strcmp(s, "fwd") || !strcmp(s, "1") || !strcmp(s, "left"))
		return 1;
	if (!strcmp(s, "rev") || !strcmp(s, "2") || !strcmp(s, "right"))
		return 2;
	return 0;
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

static commandResult_t Cmd_HPMotor_Init(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)args; (void)cmdFlags;
	int rc = motor_api_init();
	snprintf(g_motor_msg, sizeof(g_motor_msg), "%s", rc == 0 ? "mcasic initialized" : "mcasic init failed");
	return (rc == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t Cmd_HPMotor_Stop(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)args; (void)cmdFlags;
	g_pending_op = MOP_NONE;
	motor_api_stop();
	snprintf(g_motor_msg, sizeof(g_motor_msg), "motors stopped");
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPMotor_Pulse(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	int m = parse_motor_idx(Tokenizer_GetArg(0));
	int mode = parse_motor_dir(Tokenizer_GetArg(1));
	uint32_t pwm = (Tokenizer_GetArgsCount() >= 3) ? (uint32_t)Tokenizer_GetArgInteger(2) : 350u;
	uint32_t ms  = (Tokenizer_GetArgsCount() >= 4) ? (uint32_t)Tokenizer_GetArgInteger(3) : 150u;
	if (m < 0 || !mode)
		return CMD_RES_BAD_ARGUMENT;
	int rc = motor_api_drive(m, mode, pwm, ms);
	snprintf(g_motor_msg, sizeof(g_motor_msg), "motor %c %s (%u pwm, %u ms): %s",
	         'A' + m, mode == 1 ? "FWD" : "REV", (unsigned)pwm, (unsigned)ms,
	         rc == 0 ? "driving" : (rc == -3 ? "stalled at end stop" : "err"));
	return (rc == 0) ? CMD_RES_OK : CMD_RES_ERROR;
}

static commandResult_t Cmd_HPMotor_Home(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	int m = parse_motor_idx(Tokenizer_GetArg(0));
	if (m < 1 || m > 2)
		return CMD_RES_BAD_ARGUMENT;
	int mode = (Tokenizer_GetArgsCount() >= 2) ? parse_motor_dir(Tokenizer_GetArg(1)) : (m == 1 ? 1 : 2);
	uint32_t pwm = (Tokenizer_GetArgsCount() >= 3) ? (uint32_t)Tokenizer_GetArgInteger(2) : 350u;
	g_pending_m    = m;
	g_pending_mode = mode ? mode : (m == 1 ? 1 : 2);
	g_pending_pwm  = pwm ? pwm : 350u;
	g_pending_op   = MOP_HOME;
	snprintf(g_motor_msg, sizeof(g_motor_msg), "homing motor %c...", 'A' + m);
	return CMD_RES_OK;
}

static commandResult_t Cmd_HPMotor_Goto(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	(void)context; (void)cmd; (void)cmdFlags;
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 2)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	int m = parse_motor_idx(Tokenizer_GetArg(0));
	if (m < 1 || m > 2)
		return CMD_RES_BAD_ARGUMENT;
	int32_t pos = Tokenizer_GetArgInteger(1);
	uint32_t pwm = (Tokenizer_GetArgsCount() >= 3) ? (uint32_t)Tokenizer_GetArgInteger(2) : 350u;
	g_pending_m      = m;
	g_pending_target = pos;
	g_pending_pwm    = pwm ? pwm : 350u;
	g_pending_op     = MOP_GOTO;
	snprintf(g_motor_msg, sizeof(g_motor_msg), "moving motor %c to %d...", 'A' + m, (int)pos);
	return CMD_RES_OK;
}

void HP2620Motors_Init(void)
{
	CMD_RegisterCommand("HPMotor_Init",  Cmd_HPMotor_Init,  NULL);
	CMD_RegisterCommand("HPMotor_Stop",  Cmd_HPMotor_Stop,  NULL);
	CMD_RegisterCommand("HPMotor_Pulse", Cmd_HPMotor_Pulse, NULL);
	CMD_RegisterCommand("HPMotor_Home",  Cmd_HPMotor_Home,  NULL);
	CMD_RegisterCommand("HPMotor_Goto",  Cmd_HPMotor_Goto,  NULL);

	g_motors_running = 1;
	snprintf(g_motor_msg, sizeof(g_motor_msg), "ready");
}

void HP2620Motors_Stop(void)
{
	g_pending_op = MOP_NONE;
	motor_api_stop();
	g_motors_running = 0;
}

void HP2620Motors_OnChannelChanged(int ch, int val)
{
	(void)ch;
	(void)val;
}

void HP2620Motors_OnEverySecond(void)
{
}

void HP2620Motors_RunQuickTick(void)
{
	if (!g_motors_running || g_pending_op == MOP_NONE)
		return;
	if (http_tx_busy())
		return;

	enum pending_motor_op op = g_pending_op;
	g_pending_op = MOP_NONE;

	int m = g_pending_m;
	int mode = g_pending_mode;
	uint32_t pwm = g_pending_pwm;

	if (op == MOP_HOME) {
		int rc = motor_api_home(m, mode, pwm);
		snprintf(g_motor_msg, sizeof(g_motor_msg),
		         rc == 0 ? "motor %c homed (pos 0)" : "motor %c home failed (%d)",
		         'A' + m, rc);
	} else if (op == MOP_RANGE) {
		int32_t span = 0;
		int rc = motor_api_range(m, mode, pwm, &span);
		if (rc == 0)
			g_last_span[m] = span;
		snprintf(g_motor_msg, sizeof(g_motor_msg),
		         rc == 0 ? "motor %c travel: %d counts" : "motor %c range failed (%d)",
		         'A' + m, (int)span);
	} else if (op == MOP_GOTO) {
		int32_t target = g_pending_target;
		int rc = motor_api_goto(m, target, pwm);
		snprintf(g_motor_msg, sizeof(g_motor_msg),
		         rc == 0 ? "motor %c at %d (target %d)" :
		         (rc == -3 ? "motor %c blocked at %d" : "motor %c goto %d rc=%d"),
		         'A' + m, (int)motor_api_position(m), (int)target);
	}
}

static void process_http_motor_args(http_request_t *request)
{
	char act[16], m_s[8], dir_s[8], pwm_s[16], ms_s[16], pos_s[16];
	if (!http_getArg(request->url, "hp_mact", act, sizeof(act)))
		return;

	int m = -1, mode = 0;
	uint32_t pwm = 350u, ms = 150u;
	if (http_getArg(request->url, "m", m_s, sizeof(m_s)))
		m = parse_motor_idx(m_s);
	if (http_getArg(request->url, "dir", dir_s, sizeof(dir_s)))
		mode = parse_motor_dir(dir_s);
	if (http_getArg(request->url, "mpwm", pwm_s, sizeof(pwm_s))) {
		int v = atoi(pwm_s);
		if (v >= 50 && v <= 600)
			pwm = (uint32_t)v;
	}
	if (http_getArg(request->url, "mms", ms_s, sizeof(ms_s))) {
		int v = atoi(ms_s);
		if (v >= 10 && v <= 500)
			ms = (uint32_t)v;
	}

	if (!strcmp(act, "init")) {
		int rc = motor_api_init();
		snprintf(g_motor_msg, sizeof(g_motor_msg), "%s",
		         rc == 0 ? "mcasic initialized" : "mcasic init failed");
	} else if (!strcmp(act, "stop")) {
		g_pending_op = MOP_NONE;
		motor_api_stop();
		snprintf(g_motor_msg, sizeof(g_motor_msg), "motors stopped");
	} else if (!strcmp(act, "move")) {
		int rc = motor_api_drive(m, mode, pwm, ms);
		snprintf(g_motor_msg, sizeof(g_motor_msg), "%s",
		         rc == 0 ? "driving" :
		         rc == -2 ? "hold limit reached" :
		         rc == -3 ? "stalled at end stop: drive the other way" : "drive failed");
	} else if (!strcmp(act, "home") && (m == 1 || m == 2)) {
		g_pending_m    = m;
		g_pending_mode = mode ? mode : (m == 1 ? 1 : 2);
		g_pending_pwm  = pwm;
		g_pending_op   = MOP_HOME;
		snprintf(g_motor_msg, sizeof(g_motor_msg), "homing motor %c...", 'A' + m);
	} else if (!strcmp(act, "range") && (m == 1 || m == 2)) {
		g_pending_m    = m;
		g_pending_mode = mode ? mode : (m == 1 ? 1 : 2);
		g_pending_pwm  = pwm;
		g_pending_op   = MOP_RANGE;
		snprintf(g_motor_msg, sizeof(g_motor_msg), "measuring motor %c travel...", 'A' + m);
	} else if (!strcmp(act, "goto") && (m == 1 || m == 2)) {
		int32_t target = 0;
		if (http_getArg(request->url, "pos", pos_s, sizeof(pos_s)))
			target = (int32_t)atoi(pos_s);
		g_pending_m      = m;
		g_pending_target = target;
		g_pending_pwm    = pwm;
		g_pending_op     = MOP_GOTO;
		snprintf(g_motor_msg, sizeof(g_motor_msg), "moving motor %c to %d...", 'A' + m, (int)target);
	}
}

static void hp_mprintf(http_request_t *request, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	poststr(request, buf);
}

void HP2620Motors_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	if (!g_motors_running)
		return;

	process_http_motor_args(request);

	if (bPreState) {
		/* Interactive Motor Control Deck (rendered once above <div id="state">) */
		poststr(request,
			"<style>"
			".hm-box{background:#1e293b;border:1px solid #334155;border-radius:8px;padding:8px;margin:6px 0;text-align:center}"
			".hm-sub{background:#0f172a;border:1px solid #334155;border-radius:6px;padding:6px;margin-top:5px}"
			".hm-row{display:flex;gap:5px;justify-content:center;flex-wrap:wrap;align-items:center}"
			".hm-b{width:auto;display:inline-block;padding:3px 9px;font-size:0.78em;line-height:1.5rem;border-radius:4px;margin:0}"
			".hm-inp{width:68px;padding:2px 4px;font-size:0.78em;background:#1e293b;color:#f8fafc;border:1px solid #475569;border-radius:4px}"
			"</style>"
			"<script>"
			"var hpHt=0;"
			"function hpM(q){"
			"var p=document.getElementById('hp_pwm'),m=document.getElementById('hp_ms');"
			"var u='index?state=1&'+q+(p?'&mpwm='+p.value:'')+(m?'&mms='+m.value:'');"
			"fetch(u).then(r=>r.text()).then(t=>{var s=document.getElementById('state');if(s)s.innerHTML=t;});}"
			"function hpHold(m,d){clearInterval(hpHt);var f=()=>hpM('hp_mact=move&m='+m+'&dir='+d+'&mms=280');f();hpHt=setInterval(f,140);}"
			"function hpStop(){if(hpHt){clearInterval(hpHt);hpHt=0;}hpM('hp_mact=stop');}"
			"</script>"
			"<div class=\"hm-box\">"
			"<div style=\"font-size:0.85em;margin-bottom:5px;color:#94a3b8\"><b>HP DeskJet 2620 Motors (MCASIC)</b></div>"
			"<div class=\"hm-row\" style=\"margin-bottom:6px\">"
			"<button type=\"button\" class=\"hm-b\" style=\"background:#059669\" onclick=\"hpM('hp_mact=init')\">INIT MCASIC</button>"
			"<button type=\"button\" class=\"hm-b\" style=\"background:#dc2626\" onclick=\"hpStop()\">STOP ALL</button>"
			"<label style=\"font-size:0.78em\">PWM <b id=\"hp_pv\">350</b>"
			"<input id=\"hp_pwm\" type=\"range\" min=\"100\" max=\"600\" step=\"10\" value=\"350\" style=\"width:90px;vertical-align:middle\" oninput=\"document.getElementById('hp_pv').textContent=this.value\"></label>"
			"<label style=\"font-size:0.78em\">Pulse <b id=\"hp_mv\">150</b>ms"
			"<input id=\"hp_ms\" type=\"range\" min=\"25\" max=\"500\" step=\"25\" value=\"150\" style=\"width:80px;vertical-align:middle\" oninput=\"document.getElementById('hp_mv').textContent=this.value\"></label>"
			"</div>");

		static const struct {
			const char *id;
			const char *title;
			const char *fwd_lbl;
			const char *rev_lbl;
			int has_enc;
		} mdefs[3] = {
			{ "a", "Motor A: Paper Feed (0x31)",         "FWD",        "REV",         0 },
			{ "b", "Motor B: Printhead Carriage (0x33)", "LEFT (FWD)", "RIGHT (REV)", 1 },
			{ "c", "Motor C: Flatbed Scanner (0x35)",    "FWD",        "REV",         1 },
		};

		for (int i = 0; i < 3; i++) {
			hp_mprintf(request,
				"<div class=\"hm-sub\">"
				"<div style=\"font-size:0.8em;color:#e2e8f0;margin-bottom:4px\"><b>%s</b></div>"
				"<div class=\"hm-row\">",
				mdefs[i].title);
			hp_mprintf(request,
				"<button type=\"button\" class=\"hm-b\" style=\"background:#2563eb\" "
				"onpointerdown=\"hpHold('%s','fwd')\" onpointerup=\"hpStop()\" onpointerleave=\"hpStop()\">HOLD %s</button>",
				mdefs[i].id, mdefs[i].fwd_lbl);
			hp_mprintf(request,
				"<button type=\"button\" class=\"hm-b\" style=\"background:#2563eb\" "
				"onpointerdown=\"hpHold('%s','rev')\" onpointerup=\"hpStop()\" onpointerleave=\"hpStop()\">HOLD %s</button>",
				mdefs[i].id, mdefs[i].rev_lbl);
			hp_mprintf(request,
				"<button type=\"button\" class=\"hm-b\" style=\"background:#475569\" "
				"onclick=\"hpM('hp_mact=move&m=%s&dir=fwd')\">Pulse %s</button>",
				mdefs[i].id, mdefs[i].fwd_lbl);
			hp_mprintf(request,
				"<button type=\"button\" class=\"hm-b\" style=\"background:#475569\" "
				"onclick=\"hpM('hp_mact=move&m=%s&dir=rev')\">Pulse %s</button>",
				mdefs[i].id, mdefs[i].rev_lbl);

			if (mdefs[i].has_enc) {
				hp_mprintf(request,
					"<button type=\"button\" class=\"hm-b\" style=\"background:#0d9488\" "
					"onclick=\"hpM('hp_mact=home&m=%s')\">Home</button>"
					"<button type=\"button\" class=\"hm-b\" style=\"background:#0d9488\" "
					"onclick=\"hpM('hp_mact=range&m=%s')\">Range</button>",
					mdefs[i].id, mdefs[i].id);
				hp_mprintf(request,
					"<input id=\"hp_go_%s\" class=\"hm-inp\" type=\"number\" placeholder=\"pos\">"
					"<button type=\"button\" class=\"hm-b\" style=\"background:#0284c7\" "
					"onclick=\"hpM('hp_mact=goto&m=%s&pos='+document.getElementById('hp_go_%s').value)\">Go</button>",
					mdefs[i].id, mdefs[i].id, mdefs[i].id);
			}
			poststr(request, "</div></div>");
		}
		poststr(request, "</div>");
		return;
	}

	/* Live Motor, Encoder & Sensor Telemetry inside <div id="state"> */
	struct motor_telemetry t;
	motor_api_telemetry(&t);

	poststr(request,
		"<div style=\"background:#0f172a;border:1px solid #334155;border-radius:8px;padding:8px;margin:6px 0;text-align:center;font-size:0.8em\">");

	hp_mprintf(request,
		"<div style=\"margin-bottom:4px\">"
		"<b>MCASIC:</b> <span style=\"color:%s\"><b>%s</b></span> (id <code>%04x</code> st <code>%04x</code> ctl <code>%04x/%04x/%04x</code>) | "
		"<b>Status:</b> <span style=\"color:#38bdf8\">%s</span>"
		"</div>",
		t.inited ? "#4ade80" : "#facc15",
		t.inited ? "INIT OK" : "UNINIT",
		(unsigned)t.mcasic_id, (unsigned)t.mcasic_status,
		(unsigned)t.motor_ctl[0], (unsigned)t.motor_ctl[1], (unsigned)t.motor_ctl[2],
		g_motor_msg);

	hp_mprintf(request,
		"<div style=\"margin-bottom:4px;color:#e2e8f0\">"
		"<b>Carriage (B):</b> pos <code>%d</code> (span <code>%d</code>) | "
		"<b>Scanner (C):</b> pos <code>%d</code> (span <code>%d</code>) | "
		"<b>Active:</b> <code>%s</code>",
		(int)t.pos[1], (int)g_last_span[1],
		(int)t.pos[2], (int)g_last_span[2],
		t.active_motor == 0 ? "Motor A" : (t.active_motor == 1 ? "Motor B" : (t.active_motor == 2 ? "Motor C" : "none")));

	if (t.stall_motor >= 0) {
		hp_mprintf(request,
			" | <span style=\"color:#f87171\"><b>Stall: Motor %c</b></span>",
			'A' + t.stall_motor);
	}
	poststr(request, "</div><div style=\"color:#94a3b8\"><b>Sensors:</b> ");

	int n_sens = sensor_api_count();
	for (int i = 0; i < n_sens; i++) {
		int act = sensor_api_active(i);
		hp_mprintf(request,
			"<span style=\"display:inline-block;margin:1px 4px;padding:1px 5px;border-radius:4px;background:%s;color:%s\">%s:%s</span>",
			act ? "#065f46" : "#1e293b",
			act ? "#4ade80" : "#64748b",
			sensor_api_name(i),
			act ? "ON" : "off");
	}
	poststr(request, "</div></div>");
}

#endif /* PLATFORM_88PAAX01 */
