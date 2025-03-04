#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../httpserver/new_http.h"
#include "drv_ntp.h"

/*
startDriver widget
// widget_create [LocationIndex][bAllowCache][FileName]
widget_create 0 0 demo1.html

*/
const char *demo =
"<button id='colorToggleButton' style='padding: 10px 20px;' onclick='changeColor(this)'>Click Me</button>"
"<script>"
"const colors = ['red', 'green', 'blue'];"
"let currentColorIndex = 0;"
"function changeColor(button) {"
"    currentColorIndex = (currentColorIndex + 1) % colors.length;"
"    button.style.backgroundColor = colors[currentColorIndex];"
"}"
"</script>";

typedef enum {
	WIDGET_STATIC,
	WIDGET_STATE,
} WidgetLocation;

typedef struct widget_s {
	char *fname;
	WidgetLocation location;
	char *cached;
	byte bAllowCache;
	struct widget_s *next;
} widget_t;

widget_t *g_widgets = 0;

void DRV_Widget_Display(http_request_t *request, widget_t *w) {
	// fast path - cached
	if (w->cached) {
		poststr(request, w->cached);
		return;
	}
	char *data = (char*)LFS_ReadFile(w->fname);
	if (data == 0) {
		return;
	}
	poststr(request, data);

	if (w->bAllowCache) {
		w->cached = data;
		return;
	}
	free(data);
}
void DRV_Widget_DisplayList(http_request_t* request, WidgetLocation loc) {
	widget_t *w = g_widgets;
	while (w) {
		if (w->location == loc) {
			DRV_Widget_Display(request, w);
		}
		w = w->next;
	}
}
void DRV_Widget_BeforeState(http_request_t* request) {
	DRV_Widget_DisplayList(request, WIDGET_STATIC);
}

void DRV_Widget_AddToHtmlPage(http_request_t *request) {
	DRV_Widget_DisplayList(request, WIDGET_STATE);
}
void Widget_Add(widget_t **first, widget_t *n) {
	if (*first == 0) {
		*first = n;
	}
	else {
		widget_t *tmp = *first;
		while (tmp->next) {
			tmp = tmp->next;
		}
		tmp->next = n;
	}
}
static commandResult_t CMD_Widget_Create(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if(Tokenizer_GetArgsCount()<3) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	widget_t *n = malloc(sizeof(widget_t));
	memset(n, 0, sizeof(widget_t));
	n->location = Tokenizer_GetArgInteger(0);
	n->bAllowCache = Tokenizer_GetArgInteger(1);
	const char *fname = Tokenizer_GetArg(2);
	n->fname = strdup(fname);
	Widget_Add(&g_widgets, n);
	return CMD_RES_OK;
}
static commandResult_t CMD_Widget_ClearAll(const void *context, const char *cmd, const char *args, int flags) {
	widget_t *current = g_widgets;
	while (current) {
		widget_t *next = current->next;
		free(current->fname);
		if (current->cached) {
			free(current->cached);
		}
		free(current);
		current = next;
	}
	g_widgets = NULL;
	return CMD_RES_OK;
}

void DRV_Widget_Init() {
	
	//cmddetail:{"name":"widget_clearAll","args":"",
	//cmddetail:"descr":"Removes all registered widgets",
	//cmddetail:"fn":"NULL);","file":"driver/drv_widget.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("widget_clearAll", CMD_Widget_ClearAll, NULL);

	//cmddetail:{"name":"widget_create","args":"[LocationIndex][bAllowCache][FileName]",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"NULL);","file":"driver/drv_widget.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("widget_create", CMD_Widget_Create, NULL);

}

