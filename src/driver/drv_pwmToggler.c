#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../quicktick.h"
#include <math.h>

/*
PWM toggler provides you an abstraction layer over PWM channels 
and allows you to enable/disable them without losing the set PWM value.

PWM toggler was created to support lamp with RGB LED and PWM laser and PWM motor.

Configuration:
- set channels 1, 2 and 3 for RGB 
- enable "force RGB mode" in General/Flags
- set channels 4 for motor and 5 for laser
- on web panel, create autoexec.bat with following script:

	// enable toggler driver
	startDriver PWMToggler
	// toggler slot 0 will be channel 4
	toggler_channel0 4
	// toggler slot 0 display name is laser
	toggler_name0 Laser

	// toggler slot 1 will be channel 5
	toggler_channel1 5
	// toggler slot 1 display name is motor
	toggler_name1 Motor

- for HA, configure RGB as usual in Yaml
- also add yaml entries for laser and motor:

	 - unique_id: "OpenBK7231T_760BF030_galaxy_laser"
		name: "GenioLaser"
		command_topic: "cmnd/obk760BF030/toggler_enable0"
		state_topic: "obk760BF030/toggler_enable0/get"
		payload_on: 1
		payload_off: 0
		brightness_command_topic: "cmnd/obk760BF030/toggler_set0"
		brightness_scale: 100
		brightness_state_topic: "obk760BF030/toggler_set0/get"
		brightness_value_template: "{{value}}"
		qos: 1


// Script to make PWM toggler follow the LED state

// this will make disabling LED also disable both togglers (laser and motor)
addEventHandler LEDState 0 backlog toggler_enable0 0; toggler_enable1 0; 
// this will make enabling LED also enable both togglers (laser and motor)
// Comment out if you don't want it!
addEventHandler LEDState 1 backlog toggler_enable0 1; toggler_enable1 1; 

To set startup value, use:
toggler_set0 50
toggler_enable0 1


Test code for smooth transitions:

// NOTE: Enable "show raw pwm controllers" in flags
startDriver PWMToggler


// assumes we have PWM on channel 1
toggler_channel0 1 20
toggler_name0 First

// assumes we have PWM on channel 2
toggler_channel1 2 20
toggler_name1 Second



*/

#define MAX_ONOFF_SLOTS 6

static char *g_names[MAX_ONOFF_SLOTS] = { 0 };
static int g_channels[MAX_ONOFF_SLOTS];
static float g_values[MAX_ONOFF_SLOTS];
static bool g_enabled[MAX_ONOFF_SLOTS];
static float g_speeds[MAX_ONOFF_SLOTS];
static float g_current[MAX_ONOFF_SLOTS];

int parsePowerArgument(const char *s);

void publish_enableState(int index) {
	char topic[32];
	snprintf(topic, sizeof(topic), "toggler_enable%i", index);

	MQTT_PublishMain_StringInt(topic, g_enabled[index], 0);
}
void publish_value(int index) {
	char topic[32];
	snprintf(topic, sizeof(topic), "toggler_set%i", index);

	MQTT_PublishMain_StringInt(topic, g_values[index], 0);
}
void setTargetChannel(int index) {

}
void apply(int index) {
	if (index < 0)
		return;
	if (index >= MAX_ONOFF_SLOTS)
		return;
	if (g_speeds[index] == 0) {
		g_current[index] = g_values[index];
		if (g_enabled[index]) {
			CHANNEL_Set(g_channels[index], g_values[index], 0);
		}
		else {
			CHANNEL_Set(g_channels[index], 0, 0);
		}
	}
	publish_enableState(index);
	publish_value(index);
}
void Toggler_Set(int index, int value) {
	if (index < 0)
		return;
	if (index >= MAX_ONOFF_SLOTS)
		return;
	g_values[index] = value;
	apply(index);
}
void Toggler_Toggle(int index) {
	if (index < 0)
		return;
	if (index >= MAX_ONOFF_SLOTS)
		return;
	g_enabled[index] = !g_enabled[index];
	apply(index);
}
commandResult_t Toggler_NameX(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *indexStr;
	int index;

	if (args == 0 || *args == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	indexStr = cmd + strlen("toggler_name");
	index = atoi(indexStr);

	if (index < 0 || index >= MAX_ONOFF_SLOTS) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Given index is out of range");
		return CMD_RES_BAD_ARGUMENT;
	}
	if (g_names[index])
		free(g_names[index]);
	g_names[index] = strdup(args);

	return CMD_RES_OK;
}
commandResult_t Toggler_EnableX(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *indexStr;
	int index;
	bool bEnabled;

	if (args == 0 || *args == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	indexStr = cmd + strlen("toggler_enable");
	index = atoi(indexStr);
	if (index < 0 || index >= MAX_ONOFF_SLOTS) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Given index is out of range");
		return CMD_RES_BAD_ARGUMENT;
	}

	bEnabled = parsePowerArgument(args);
	g_enabled[index] = bEnabled;

	apply(index);


	return CMD_RES_OK;
}

commandResult_t Toggler_SetX(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *indexStr;
	int index;

	if (args == 0 || *args == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	indexStr = cmd + strlen("toggler_set");
	index = atoi(indexStr);
	if (index < 0 || index >= MAX_ONOFF_SLOTS) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Given index is out of range");
		return CMD_RES_BAD_ARGUMENT;
	}

	if (*args == '+') {
		args++;
		g_values[index] += atoi(args);
	}
	else if (*args == '-') {
		args++;
		g_values[index] -= atoi(args);
	}
	else {
		g_values[index] = atoi(args);
	}

	if (g_values[index] < 0)
		g_values[index] = 0;
	if (g_values[index] > 100)
		g_values[index] = 100;

	apply(index);


	return CMD_RES_OK;
}
commandResult_t Toggler_ChannelX(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *indexStr;
	int index;


	indexStr = cmd + strlen("toggler_channel");
	index = atoi(indexStr);
	if (index < 0 || index >= MAX_ONOFF_SLOTS) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Given index is out of range");
		return CMD_RES_BAD_ARGUMENT;
	}

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_channels[index] = Tokenizer_GetArgInteger(0);
	g_speeds[index] = Tokenizer_GetArgFloat(1);

	return CMD_RES_OK;
}
const char *Toggler_GetName(int i) {
	if (i < 0 || i >= MAX_ONOFF_SLOTS) {
		return "";
	}
	const char *name = g_names[i];
	if (name == 0)
		name = "Unnamed";
	return name;
}
void DRV_Toggler_ProcessChanges(http_request_t *request) {
	int j;
	int val;
	char tmpA[8];

	if (http_getArg(request->url, "togglerOn", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		const char *name = Toggler_GetName(j);
		hprintf255(request, "<h3>Toggled %s!</h3>", name);
		Toggler_Toggle(j);
	}
	if (http_getArg(request->url, "togglerValueID", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		const char *name = Toggler_GetName(j);
		http_getArg(request->url, "togglerValue", tmpA, sizeof(tmpA));
		val = atoi(tmpA);
		Toggler_Set(j, val);
		hprintf255(request, "<h3>Set value %i for %s!</h3>", val, name);
	}

}
void DRV_Toggler_AddToHtmlPage(http_request_t *request) {
	int i;
	const char *c;

	for (i = 0; i < MAX_ONOFF_SLOTS; i++) {
		const char *name = Toggler_GetName(i);
		int maxValue = 100;
		if (g_channels[i] == -1)
			continue;

		//hprintf255(request, "<tr> Toggler %s</tr>",name);

		hprintf255(request, "<tr>");
		if (g_enabled[i]) {
			c = "bgrn";
		}
		else {
			c = "bred";
		}
		poststr(request, "<td><form action=\"index\">");
		hprintf255(request, "<input type=\"hidden\" name=\"togglerOn\" value=\"%i\">", i);
		hprintf255(request, "<input class=\"%s\" type=\"submit\" value=\"Toggle %s\"/></form></td>", c, name);
		poststr(request, "</tr>");

		poststr(request, "<tr><td>");
		hprintf255(request, "<form action=\"index\" id=\"form%i\">", i);
		hprintf255(request, "<input type=\"range\" min=\"0\" max=\"%i\" name=\"togglerValue\" id=\"togglerValue\" value=\"%i\" onchange=\"this.form.submit()\">", 
			maxValue, (int)g_values[i]);
		hprintf255(request, "<input type=\"hidden\" name=\"togglerValueID\" value=\"%i\">", i);
		hprintf255(request, "<input type=\"submit\" class='disp-none' value=\"Set %s\"/></form>", name);
		poststr(request, "</td></tr>");
	}
}
void DRV_Toggler_QuickTick() {
	int i;

	for (i = 0; i < MAX_ONOFF_SLOTS; i++) {
		if (g_channels[i] == -1)
			continue;
		if (g_speeds[i] == 0)
			continue;
		float tgVal;
		if (g_enabled[i] == 0)
			tgVal = 0;
		else
			tgVal = g_values[i];
		// move g_current[i] towards g_values[i] but do not overshoot
		float delta = tgVal - g_current[i];
		float canMove = g_speeds[i] * g_deltaTimeMS * 0.001f;
		float prev = g_current[i];
		if (fabs(delta) <= canMove) {
			g_current[i] = tgVal;
		}
		else {
			g_current[i] += (delta > 0 ? canMove : -canMove);
		}
		if (prev != g_current[i]) {
			//printf("Lerping %i, cur %f, tg %f\n", i, g_current[i], tgVal);
			if (g_current[i]) {
				CHANNEL_Set(g_channels[i], g_current[i], 0);
			}
			else {
				CHANNEL_Set(g_channels[i], 0, 0);
			}
		}
	}
}
void DRV_Toggler_AppendInformationToHTTPIndexPage(http_request_t* request) {
	int i;
	hprintf255(request, "<h4>Toggler: ");
	int cnt = 0;
	for (i = 0; i < MAX_ONOFF_SLOTS; i++) {
		if (g_channels[i] == -1)
			continue;
		if (cnt != 0) {
			hprintf255(request, ", ");
		}
		const char *name = Toggler_GetName(i);
		const char *st;
		if (g_enabled[i])
			st = "ON";
		else
			st = "OFF";
		hprintf255(request, "slot %i-%s (target %i) has value %f, state %s", 
			i, name, g_channels[i], g_values[i],st);
		cnt++;
	}
	hprintf255(request, "</h4>");

}
void DRV_InitPWMToggler() {
	int i;

	for (i = 0; i < MAX_ONOFF_SLOTS; i++) {
		g_channels[i] = -1;
		g_names[i] = 0;
	}
	//cmddetail:{"name":"toggler_enable","args":"[1or0]",
	//cmddetail:"descr":"Sets the given output ON or OFF.  handles toggler_enable0, toggler_enable1, etc",
	//cmddetail:"fn":"Toggler_EnableX","file":"driver/drv_pwmToggler.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("toggler_enable", Toggler_EnableX, NULL);
	//cmddetail:{"name":"toggler_set","args":"[Value]",
	//cmddetail:"descr":"Sets the VALUE of given output. Handles toggler_set0, toggler_set1, etc. The last digit after command name is changed to slot index. It can also add to current value if you write value like +25 and subtract if you prefix it with - like -25",
	//cmddetail:"fn":"Toggler_SetX","file":"driver/drv_pwmToggler.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("toggler_set", Toggler_SetX, NULL);
	//cmddetail:{"name":"toggler_channel","args":"[ChannelIndex]",
	//cmddetail:"descr":"handles toggler_channel0, toggler_channel1. Sets channel linked to given toggler slot.",
	//cmddetail:"fn":"Toggler_ChannelX","file":"driver/drv_pwmToggler.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("toggler_channel", Toggler_ChannelX, NULL);
	//cmddetail:{"name":"toggler_name","args":"",
	//cmddetail:"descr":"Handles toggler_name0, toggler_name1, etc. Sets the name of a toggler for GUI.",
	//cmddetail:"fn":"Toggler_NameX","file":"driver/drv_pwmToggler.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("toggler_name", Toggler_NameX, NULL);
}







