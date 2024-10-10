#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"

typedef struct {
	bool bEnabled;
	char *label;
	char *command;
	char color[16];
} httpButton_t;

static httpButton_t **g_buttons = 0;
static int g_buttonCount = 0;

httpButton_t *getOrAlloc(int idx) {
	int i;

	if (idx >= g_buttonCount) {
		g_buttons = (httpButton_t**)realloc(g_buttons, sizeof(httpButton_t*)*(idx + 1));
		for (i = g_buttonCount; i <= idx; i++) {
			g_buttons[i] = 0;
		}
		g_buttonCount = idx + 1;
	}
	if (g_buttons[idx] == 0) {
		g_buttons[idx] = (httpButton_t*)malloc(sizeof(httpButton_t));
		if (g_buttons[idx] == 0) {
			// no memory, allocation failed
			return 0;
		}
		memset(g_buttons[idx], 0, sizeof(httpButton_t));
	}
	return g_buttons[idx];
}
httpButton_t *getSafe(int idx) {
	if (idx >= g_buttonCount) {
		return 0;
	}
	return g_buttons[idx];
}
void setEnabled(int idx, bool b) {
	httpButton_t *bt;
	bt = getOrAlloc(idx);
	if (bt == 0)
		return;
	bt->bEnabled = b;
}
void setLabel(int idx, const char *lab) {
	httpButton_t *bt;
	bt = getOrAlloc(idx);
	if (bt == 0)
		return;
	if (bt->label)
		free(bt->label);
	bt->label = strdup(lab);
}
void setCommand(int idx, const char *cmd) {
	httpButton_t *bt;
	bt = getOrAlloc(idx);
	if (bt == 0)
		return;
	if (bt->command)
		free(bt->command);
	bt->command = strdup(cmd);
}
void setColor(int idx, const char *cs) {
	httpButton_t *bt;
	bt = getOrAlloc(idx);
	if (bt == 0)
		return;
	strcpy_safe(bt->color, cs, sizeof(bt->color));
}
/*
Usage:

startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Turn off after 10 seconds"
setButtonCommand 0 "addRepeatingEvent 10 1 Power Off"
//setButtonColor 0 "#FF0000"


setButtonEnabled 1 1
setButtonLabel 1 "Set Red 50% brightness"
setButtonCommand 1 "backlog led_basecolor_rgb FF0000; led_dimmer 50; led_enableAll 1"
//setButtonColor 1 "blue"

*/
// Usage for making LFS page link :
//startDriver httpButtons
//setButtonEnabled 0 1
//setButtonLabel 0 "Open Config"
//setButtonCommand 0 "*/api/lfs/cfg.html"
//setButtonColor 0 "#FF0000"








void DRV_HTTPButtons_ProcessChanges(http_request_t *request) {
	int j;
	char tmpA[8];
	httpButton_t *bt;

	if (http_getArg(request->url, "act", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		bt = getSafe(j);
		hprintf255(request, "<h3>Will do action %s!</h3>", bt->label);
		if (bt) {
			CMD_ExecuteCommand(bt->command, 0);
		}
	}

}
void DRV_HTTPButtons_AddToHtmlPage(http_request_t *request) {
	int i;
	const char *c;
	const char *action;
	const char *label;

	for (i = 0; i < g_buttonCount; i++) {
		httpButton_t *bt = g_buttons[i];

		if (bt == 0)
			continue;
		if (bt->bEnabled == false)
			continue;


		hprintf255(request, "<tr>");

		c = "";
		if (!bt->color[0]) {
			c = "bgrn";
		}

		action = bt->command;
		if (action == 0)
			action = "";
		label = bt->label;
		if (label == 0)
			label = "";
		poststr(request, "<td><form action=\"");
		if (action[0] == '*') {
			poststr(request, action+1);
			poststr(request, "\">");
		} else {
			poststr(request, "index");
			poststr(request, "\">");
			hprintf255(request, "<input type=\"hidden\" name=\"act\" value=\"%i\">", i);
		}
		
		hprintf255(request, "<input class=\"%s\" ", c);
		if (bt->color[0]) {
			hprintf255(request, "style = \"background-color:%s;\" ",bt->color);
		}
		hprintf255(request, "type = \"submit\" value=\"%s\"/></form></td>",  label);
		poststr(request, "</tr>");


	}
}

commandResult_t CMD_setButtonColor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int buttonIndex;
	const char *cStr;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	buttonIndex = Tokenizer_GetArgInteger(0);
	cStr = Tokenizer_GetArg(1);

	setColor(buttonIndex, cStr);


	return CMD_RES_OK;
}

commandResult_t CMD_setButtonCommand(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int buttonIndex;
	const char *cmdToSet;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	buttonIndex = Tokenizer_GetArgInteger(0);
	cmdToSet = Tokenizer_GetArg(1);

	setCommand(buttonIndex, cmdToSet);

	return CMD_RES_OK;
}
commandResult_t CMD_setButtonLabel(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int buttonIndex;
	const char *lab;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	buttonIndex = Tokenizer_GetArgInteger(0);
	lab = Tokenizer_GetArg(1);

	setLabel(buttonIndex, lab);

	return CMD_RES_OK;
}
commandResult_t CMD_setButtonEnabled(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int buttonIndex;
	int bOn;

	Tokenizer_TokenizeString(args,0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	buttonIndex = Tokenizer_GetArgInteger(0);
	bOn = Tokenizer_GetArgInteger(1);

	setEnabled(buttonIndex, bOn);
	return CMD_RES_OK;
}
void DRV_InitHTTPButtons() {
	//cmddetail:{"name":"setButtonColor","args":"[ButtonIndex][Color]",
	//cmddetail:"descr":"Sets the colour of custom scriptable HTTP page button",
	//cmddetail:"fn":"CMD_setButtonColor","file":"driver/drv_httpButtons.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonColor", CMD_setButtonColor, NULL);
	//cmddetail:{"name":"setButtonCommand","args":"[ButtonIndex][Command]",
	//cmddetail:"descr":"Sets the command of custom scriptable HTTP page button",
	//cmddetail:"fn":"CMD_setButtonCommand","file":"driver/drv_httpButtons.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonCommand", CMD_setButtonCommand, NULL);
	//cmddetail:{"name":"setButtonLabel","args":"[ButtonIndex][Label]",
	//cmddetail:"descr":"Sets the label of custom scriptable HTTP page button",
	//cmddetail:"fn":"CMD_setButtonLabel","file":"driver/drv_httpButtons.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonLabel", CMD_setButtonLabel, NULL);
	//cmddetail:{"name":"setButtonEnabled","args":"[ButtonIndex][1or0]",
	//cmddetail:"descr":"Sets the visibility of custom scriptable HTTP page button",
	//cmddetail:"fn":"CMD_setButtonEnabled","file":"driver/drv_httpButtons.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonEnabled", CMD_setButtonEnabled, NULL);
}








