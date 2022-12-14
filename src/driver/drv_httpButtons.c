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
	char label[32];
	char command[128];
	char color[16];
} httpButton_t;

httpButton_t **g_buttons = 0;
int g_buttonCount = 0;

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
	strcpy_safe(bt->label, lab, sizeof(bt->label));
}
void setCommand(int idx, const char *cmd) {
	httpButton_t *bt;
	bt = getOrAlloc(idx);
	if (bt == 0)
		return;
	strcpy_safe(bt->command, cmd, sizeof(bt->command));
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




void DRV_HTTPButtons_ProcessChanges(http_request_t *request) {
	int j;
	int val;
	char tmpA[8];
	httpButton_t *bt;

	if (http_getArg(request->url, "act", tmpA, sizeof(tmpA))) {
		j = atoi(tmpA);
		hprintf255(request, "<h3>Will do action %i!</h3>", j);
		bt = getSafe(j);
		if (bt) {
			CMD_ExecuteCommand(bt->command, 0);
		}
	}

}
void DRV_HTTPButtons_AddToHtmlPage(http_request_t *request) {
	int i;
	const char *c;

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
		
		poststr(request, "<td><form action=\"index\">");
		hprintf255(request, "<input type=\"hidden\" name=\"act\" value=\"%i\">", i);
		hprintf255(request, "<input class=\"%s\" ", c);
		if (bt->color[0]) {
			hprintf255(request, "style = \"background-color:%s;\" ",bt->color);
		}
		hprintf255(request, "type = \"submit\" value=\"%s\"/></form></td>",  bt->label);
		poststr(request, "</tr>");


	}
}

commandResult_t CMD_setButtonColor(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int buttonIndex;
	const char *cStr;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs 2 arguments");
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

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs 2 arguments");
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

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs 2 arguments");
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

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "This command needs 2 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	buttonIndex = Tokenizer_GetArgInteger(0);
	bOn = Tokenizer_GetArgInteger(1);

	setEnabled(buttonIndex, bOn);
	return CMD_RES_OK;
}
void DRV_InitHTTPButtons() {
	CMD_RegisterCommand("setButtonColor", "", CMD_setButtonColor, NULL, NULL);
	CMD_RegisterCommand("setButtonCommand", "", CMD_setButtonCommand, NULL, NULL);
	CMD_RegisterCommand("setButtonLabel", "", CMD_setButtonLabel, NULL, NULL);
	CMD_RegisterCommand("setButtonEnabled", "", CMD_setButtonEnabled, NULL, NULL);
}








