#include "../obkdef.h"
#include "../obkhelper.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../littlefs/our_lfs.h"
#include "../cJSON/cJSON.h"
#include "../jsmn/jsmn_stream.h"

#include "drv_idisplay.h"
#include "drv_microui.h"
#include "drv_microui_core.h"
#include "drv_microui_renderer.h"

#include "drv_local.h"

typedef struct mu_BeginWindowProcessData {
	mu_Rect     rect;
	uint16_t    opt;
	char       *frm_str; /* window title */
} mu_BeginWindowProcessData_t;

typedef struct mu_LabelProcessData {
	uint8_t 	size;
	uint16_t    align;
	mu_Color    color;
	char       *frm_str; /* caption */
	/* channel list for caption text */
	int         channel;
} mu_LabelProcessData_t;

typedef struct mu_HeaderProcessData {
	uint16_t 	opt;
	char       *frm_str; /* title */
} mu_HeaderProcessData_t;

typedef struct mu_LayoutProcessData {
	uint16_t    layout_type:8;
	uint16_t    res:7;
	uint16_t    relative:1;
	union {
		mu_Rect     rect;
		struct {
			uint16_t 	width;
			uint16_t 	res1[3];
		};
		struct {
			uint16_t 	height;
			uint16_t 	res2[3];
		};
		struct {
			uint16_t 	row_count;
			uint16_t 	row_height;
			uint16_t 	res3[2];
		};
	};
} mu_LayoutProcessData_t;

typedef struct mu_ButtonProcessData {	
	uint16_t 	opt;
	uint16_t 	icon;
	mu_Color    color;
	char       *command;  /* command */
	char       *title;    /* title */
	/* channel list for title text */
	//int       channel;
} mu_ButtonProcessData_t;

typedef struct mu_CheckboxProcessData {
	int         state;
	char       *label;    /* label */
	/* channel list for title text */
	int         channel;
} mu_CheckboxProcessData_t;

typedef struct mu_IconProcessData {
	uint16_t 	icon_on;
	uint16_t 	icon_off;
	int         channel;
} mu_IconProcessData_t;

typedef struct mu_ProcessItem {
	int8_t 		(*processfunc)(mu_Context *, void *);
	void  		 *func_data;				
	obk_stree_t    node;
} mu_ProcessItem_t;

typedef struct mu_String_LI {	
	char  		 *frm_str;
	obk_slist_t    item;
} mu_String_LI_t;

typedef struct microui {
	obk_gui_service_t 	gui;	
	mu_Context          ctx;
	obk_stree_t         mu_process_tree;
	obk_slist_t         mu_strings_list;
	obk_slist_t         mu_commands_list;
	//obk_slist_t          mu_scripts_list;
	uint16_t            display_w;
	uint16_t            display_h;
	uint8_t             display_frame_support;
	uint16_t            display_frame_count;
	uint16_t            display_frame_w;
	uint16_t            display_frame_h;
	uint16_t            display_bightness_max;
	uint16_t            screen_saver_timer_max;
	uint16_t            screen_saver_use;
	uint16_t            screen_saver_timer;
	uint16_t            screen_saver_state;
	
	uint16_t            update_frame;
	
} microui_t;

static mu_Style obk_gui_style = {
  /* font | font_size | size | padding | spacing | indent */
  NULL, 1, { 100, 14 }, 5, 4, 24,
  /* title_height | scrollbar_size | thumb_size */
  26, 12, 8,
  {
    { 255, 255, 255, 255 }, /* MU_COLOR_TEXT */
    { 25,  25,  25,  255 }, /* MU_COLOR_BORDER */
    { 50,  50,  50,  255 }, /* MU_COLOR_WINDOWBG */
    { 25,  25,  25,  255 }, /* MU_COLOR_TITLEBG */
    { 240, 240, 240, 255 }, /* MU_COLOR_TITLETEXT */
    { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
    { 0,   0,   255, 255 }, /* MU_COLOR_BUTTON */
    { 0,   0,   95,  255 }, /* MU_COLOR_BUTTONHOVER */
    { 0,   0,   115, 255 }, /* MU_COLOR_BUTTONFOCUS */
    { 30,  30,  30,  255 }, /* MU_COLOR_BASE */
    { 35,  35,  35,  255 }, /* MU_COLOR_BASEHOVER */
    { 40,  40,  40,  255 }, /* MU_COLOR_BASEFOCUS */
    { 43,  43,  43,  255 }, /* MU_COLOR_SCROLLBASE */
    { 30,  30,  30,  255 }  /* MU_COLOR_SCROLLTHUMB */
  }
};

//static xTaskHandle g_main_thread = NULL;
static microui_t MicroUI;
static int8_t MicroUI_Inited = 0;

int8_t MicroUI_LoadJSON(void);
int8_t MicroUI_LoadItems(cJSON *mu_json_array, obk_stree_t **node, int8_t addchild);
uint16_t MicroUI_ParseOption(const char *opt_str);
char *MicroUI_AddString(obk_slist_t *list, char *str);

static int16_t text_width(mu_Font font, uint8_t font_size, const char *text, int8_t len);
static int16_t text_height(mu_Font font, uint8_t font_size);



static commandResult_t MicroUI_OnKeyPress(const void *context, const char *cmd, const char *args, int cmdFlags) {
	if (!MicroUI_Inited) return CMD_RES_OK;	

	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	//int arg_cnt = Tokenizer_GetArgsCount();		
	/*
	for (int i=0; i<arg_cnt; i++) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: OnKeyPress[%d]=%s", i, Tokenizer_GetArg(i));
	}
	*/
	/*TODO mutex?*/
	/*
	MU_KEY_SHIFT
	MU_KEY_CTRL
	MU_KEY_ALT
	MU_KEY_BACKSPACE
	MU_KEY_RETURN
	MU_KEY_ENTER
	MU_KEY_TAB
	*/
	const char *key_name = Tokenizer_GetArg(0);
	if (!key_name) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	
	if (MicroUI.screen_saver_use) {
		/*screen saver is on*/
		if (MicroUI.screen_saver_state) {
			MicroUI.screen_saver_timer = MicroUI.screen_saver_timer_max;
			MicroUI.screen_saver_state = 0;
			r_setBrightness(MicroUI.gui.renderer, MicroUI.display_bightness_max);
			return CMD_RES_OK;
		}
		MicroUI.screen_saver_timer = MicroUI.screen_saver_timer_max;
	}
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: OnKeyPress %s", key_name);
	/* handle events */ 
	if (strcmp(key_name, "TAB") == 0) {		
		mu_input_keydown(&MicroUI.ctx, MU_KEY_TAB);
	} else 
	if (strcmp(key_name, "ENTER") == 0) {		
		mu_input_keydown(&MicroUI.ctx, MU_KEY_ENTER);
	} else 
	if (strcmp(key_name, "RETURN") == 0) {		
		mu_input_keydown(&MicroUI.ctx, MU_KEY_RETURN);
	} else 
	if (strcmp(key_name, "BACKSPACE") == 0) {		
		mu_input_keydown(&MicroUI.ctx, MU_KEY_BACKSPACE);
	} 
	MicroUI.update_frame = 1;
	return CMD_RES_OK;

}


// startDriver MicroUI [IDISPLAY] [SETTINGS]
// IDISPLAY - display render interface
// SETTINGS - file on littlefs with GUI description, format TODO, or default microui.json ? xml
void MicroUI_Init() {
	
	memset((uint8_t*)&MicroUI, 0, sizeof(microui_t));
	
	int arg_cnt = Tokenizer_GetArgsCount();	
	if(arg_cnt < 2) {
    	 ADDLOG_INFO(LOG_FEATURE_CMD, "\"startdriver MicroUI\" needs at least one argument <IDISPLAY>");
    	 return;
    }
	const char *idisplay_name = Tokenizer_GetArg(1);
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI Init Start. Display interface on %s", idisplay_name);
	/* attach to IDisplay idisplay_name */
	obk_display_attach_gui(&MicroUI.gui, idisplay_name, NULL);
	
	/* TODO:  Load settings from microui.json*/
	obk_stree_init(&MicroUI.mu_process_tree);
	obk_slist_init(&MicroUI.mu_strings_list);
	obk_slist_init(&MicroUI.mu_commands_list);
	MicroUI_LoadJSON();
	
	/* init microui */
	if (mu_init(&MicroUI.ctx)) {
    	 ADDLOG_INFO(LOG_FEATURE_CMD, "MicroUI Init failed.");
    	 return;		
	}
	
	/* get display params */
	r_get_display_info(MicroUI.gui.renderer, &MicroUI.display_w, &MicroUI.display_h);
	/*  */
	MicroUI.display_frame_support = r_frame_support(MicroUI.gui.renderer);
	if (MicroUI.display_frame_support && MicroUI.display_w && MicroUI.display_h ) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: display supports frames");
		/* get free memory */
		uint32_t heap_size_for_frame = obk_get_free_heap_size()/10;
		uint32_t frame_size_px = heap_size_for_frame/sizeof(uint16_t); /* pixels count depends from color size. TODO: color size from display */
		MicroUI.display_frame_w = MicroUI.display_w;
		MicroUI.display_frame_h = MicroUI.display_h;
		MicroUI.display_frame_count = 1;
		while (frame_size_px < (MicroUI.display_frame_w*MicroUI.display_frame_h)) {
			MicroUI.display_frame_h = MicroUI.display_h / MicroUI.display_frame_count;
			if ((MicroUI.display_frame_h == 0) || (MicroUI.display_frame_count > 30)) {
				addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: can't determine frames. Heap is too small.");
				MicroUI.display_frame_support = 0;
				break;
			}
			MicroUI.display_frame_count++;
		}
	}
	
	//cmddetail:{"name":"OnKeyPress","args":"[ButtonName]",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"MicroUI_OnKeyPress","file":"drivers/drv_microui.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("OnKeyPress", MicroUI_OnKeyPress, NULL);

	
	MicroUI.ctx.style = &obk_gui_style;
	MicroUI.ctx.text_width = text_width;
	MicroUI.ctx.text_height = text_height;
	
	MicroUI.display_bightness_max = 100;
	
	MicroUI.screen_saver_use = 1;		
	MicroUI.screen_saver_state = 0;
	MicroUI.screen_saver_timer_max = 60;
	MicroUI.screen_saver_timer = MicroUI.screen_saver_timer_max;		
	
	r_setBrightness(MicroUI.gui.renderer, MicroUI.display_bightness_max);
	
	MicroUI.update_frame = 1;
	
	MicroUI_Inited = 1;	
}

void MicroUI_OnEverySecond() {
	if (MicroUI.screen_saver_use) {
		/*screen saver is on*/
		if (MicroUI.screen_saver_timer > 0)
			MicroUI.screen_saver_timer--;
		if (!MicroUI.screen_saver_timer) {
			if (MicroUI.screen_saver_state) {
				MicroUI.screen_saver_state = 2;
				r_setBrightness(MicroUI.gui.renderer, 0);
			}
		} else 
		if (MicroUI.screen_saver_timer <= (MicroUI.screen_saver_timer_max>>1)) {
			MicroUI.screen_saver_state = 1;
			r_setBrightness(MicroUI.gui.renderer, MicroUI.display_bightness_max>>1);
		}
	}
}

void MicroUI_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState)
{
	if (!MicroUI_Inited) return;
}

// stopDriver 
void MicroUI_Stop() {
	if (!MicroUI_Inited) return;
	mu_deinit(&MicroUI.ctx);
	MicroUI_Inited = 0;
}

static int16_t text_width(mu_Font font, uint8_t font_size, const char *text, int8_t len) {
  if (len == -1) { len = strlen(text); }
  return r_get_text_width(MicroUI.gui.renderer, font, font_size, text, len);
}

static int16_t text_height(mu_Font font, uint8_t font_size) {
  return r_get_text_height(MicroUI.gui.renderer, font, font_size);
} 

void process_frame(mu_Context *ctx, obk_stree_t *tree);

void process_frame(mu_Context *ctx, obk_stree_t *tree) {
	if (!ctx || !tree) return;
	mu_ProcessItem_t *processItem;
	obk_stree_t *tn;
	obk_stree_for_each(tn, tree) {
		if (tn) {
			processItem = obk_stree_entry(tn, mu_ProcessItem_t, node);
			if (processItem->processfunc) {
				int8_t result = processItem->processfunc(ctx, processItem->func_data);
				if (!result && processItem->node.child) {
					obk_stree_t root_child_node;
					root_child_node.next = processItem->node.child;
					root_child_node.child = NULL;
					process_frame(ctx, &root_child_node);
				}
			}
		}
	}
}

void render_display(mu_Rect *frame_rect) {	
	mu_Command *cmd = NULL;	
	while (mu_next_command(&MicroUI.ctx, &cmd)) {	  
		switch (cmd->type) {
		case MU_COMMAND_TEXT: {
			mu_Vec2 pos;
			copy_pos(&pos, &cmd->text.pos);
			mu_Color color;
			copy_color(&color, &cmd->text.color);			
			r_draw_text(MicroUI.gui.renderer, cmd->text.str, &pos, &color, cmd->text.size); 
			break;
		}
		case MU_COMMAND_RECT: {
			mu_Rect rect;
			copy_rect(&rect, &cmd->rect.rect);
			mu_Color color;
			copy_color(&color, &cmd->rect.color);
			if (!mu_is_intersect_rect(&rect, frame_rect)) 
				r_draw_rect(MicroUI.gui.renderer, &rect, &color); 
			break;
		}
		case MU_COMMAND_ICON: {
			mu_Rect rect;
			copy_rect(&rect, &cmd->icon.rect);
			mu_Color color;
			copy_color(&color, &cmd->icon.color);
			uint16_t icon_id;
			memmove((uint8_t*)&icon_id, (uint8_t*)&cmd->icon.id, sizeof(cmd->icon.id));
			if (!mu_is_intersect_rect(&rect, frame_rect)) 
				r_draw_icon(MicroUI.gui.renderer, icon_id, &rect, &color); 
			break;
		}
		case MU_COMMAND_CLIP: {
			mu_Rect rect;
			copy_rect(&rect, &cmd->clip.rect);			
			r_setClipRect(MicroUI.gui.renderer, &rect); 
			break;
		}
		default: break;
		}
	}	
}

void MicroUI_OnChannelChanged(int ch, int value)
{
	if (!MicroUI_Inited) return;
	MicroUI.update_frame = 1;
}

void MicroUI_QuickFrame()
{
	if (!MicroUI_Inited) return;
	/* exit, if no udpate requared */
	if (!MicroUI.update_frame) return;	
	MicroUI.update_frame = 0;	
	/* process frame */
	process_frame(&MicroUI.ctx, &MicroUI.mu_process_tree);
	/* render */
	if (MicroUI.display_frame_support) {
		mu_Rect frame_rect = {.x=0, .y=0, .w=MicroUI.display_frame_w, .h=MicroUI.display_frame_h};
		for (uint8_t fr_i = MicroUI.display_frame_count; fr_i != 0 ; fr_i--) {
			r_begin_frame(MicroUI.gui.renderer, &frame_rect, &obk_gui_style.colors[MU_COLOR_WINDOWBG]);
			
			render_display(&frame_rect);
						
			if (r_end_frame(MicroUI.gui.renderer, NULL)) {
				addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Recommended to reduce frame size.");
				break; /* exit for(...)*/
			}			
			
			frame_rect.y += MicroUI.display_frame_h;
			if (!fr_i)  frame_rect.h = MicroUI.display_frame_h - frame_rect.y; /* remainder */
		}
	} else {
		mu_Rect frame_rect = {.x=0, .y=0, .w=MicroUI.display_w, .h=MicroUI.display_h};
		render_display(&frame_rect);
	}
    	
}

int8_t MicroUI_ProcessBegin(mu_Context *ctx, void *funcdata) {	
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessBegin");
	if (!ctx) return -1;
	mu_begin(ctx);
	return 0;
}

int8_t MicroUI_ProcessEnd(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessEnd");
	if (!ctx) return -1;
	mu_end(ctx);
	return 0;
}

int8_t MicroUI_ProcessBeginWindow(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessBeginWindow");
	if (!ctx && !funcdata) return -1;
	mu_BeginWindowProcessData_t *mu_BeginWindowData = (mu_BeginWindowProcessData_t *)funcdata;	
	const char *win_title = "";
	if (mu_BeginWindowData->frm_str) win_title = mu_BeginWindowData->frm_str;
	if (mu_begin_window_ex(ctx, win_title, &mu_BeginWindowData->rect, mu_BeginWindowData->opt)) return 0;
	return -1;
}

int8_t MicroUI_ProcessHeader(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessHeader");
	if (!ctx && !funcdata) return -1;	
	mu_HeaderProcessData_t *mu_HeaderData = (mu_HeaderProcessData_t *)funcdata;	
	const char *title = "";
	if (mu_HeaderData->frm_str) title = mu_HeaderData->frm_str;
	if (mu_header_ex(ctx, title, mu_HeaderData->opt)) return 0;
	return -1;
}

int8_t MicroUI_ProcessEndWindow(mu_Context *ctx, void *funcdata) {	
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessEndWindow");
	if (!ctx) return -1;
	mu_end_window(ctx);
	return 0;
}

int8_t MicroUI_ProcessLabel(mu_Context *ctx, void *funcdata) {	
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessLabel");
	if (!ctx || !funcdata) return -1;
	mu_LabelProcessData_t *mu_LabelData = (mu_LabelProcessData_t *)funcdata;
	if (!mu_LabelData->frm_str) return 0;
	char *result_str = mu_LabelData->frm_str;
	if (mu_LabelData->channel != -1) {		
		/* TODO: check channel type */
		result_str = os_malloc(strlen(mu_LabelData->frm_str)+16);
		sprintf(result_str, mu_LabelData->frm_str, CHANNEL_GetFinalValue(mu_LabelData->channel));	
	}
	mu_label_ex(ctx, result_str, &mu_LabelData->color, mu_LabelData->size, mu_LabelData->align);
	if (mu_LabelData->channel != -1) {	
		os_free(result_str);
	}
	return 0;
}

int8_t MicroUI_ProcessLayout(mu_Context *ctx, void *funcdata) {	
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessLayout");
	if (!ctx || !funcdata) return -1;
	mu_LayoutProcessData_t *mu_LayoutData = (mu_LayoutProcessData_t *)funcdata;
	switch (mu_LayoutData->layout_type) {
		/*layout set next*/
		case 0:	mu_layout_set_next(ctx, &mu_LayoutData->rect, mu_LayoutData->relative); break;
		/*layout height*/
		case 1:	mu_layout_height(ctx, mu_LayoutData->height); break;
		/*layout width*/
		case 2:	mu_layout_width(ctx, mu_LayoutData->width); break;		
		/*layout row*/
		case 3:	mu_layout_row(ctx, mu_LayoutData->row_count, (const int16_t*)((uint8_t*)mu_LayoutData+sizeof(mu_LayoutProcessData_t)), mu_LayoutData->row_height); break;
	}
	return 0;
}

int8_t MicroUI_ProcessButton(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessButton");
	if (!ctx || !funcdata) return -1;
	mu_ButtonProcessData_t *mu_ButtonData = (mu_ButtonProcessData_t *)funcdata;
	//const char *title = "";
	//if (mu_ButtonData->title) title = mu_ButtonData->title;
	if (mu_button_ex(ctx, mu_ButtonData->title, mu_ButtonData->icon, mu_ButtonData->opt)) {
		if (mu_ButtonData->command) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: Executing command %s", mu_ButtonData->command);
			CMD_ExecuteCommand(mu_ButtonData->command, COMMAND_FLAG_SOURCE_DRIVER);
		}; 
	} 
	return 0;
}

int8_t MicroUI_ProcessCheckbox(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessCheckbox");
	if (!ctx || !funcdata) return -1;
	mu_CheckboxProcessData_t *mu_CheckboxData = (mu_CheckboxProcessData_t *)funcdata;
	const char *label = "";
	if (mu_CheckboxData->label) label = mu_CheckboxData->label;
	if (mu_CheckboxData->channel != -1) {
		mu_CheckboxData->state = CHANNEL_Get(mu_CheckboxData->channel);
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: checkbox state=%d", mu_CheckboxData->state);
	}
	if (mu_checkbox(ctx, label, &mu_CheckboxData->state))	{
		if (mu_CheckboxData->channel != -1)
			CHANNEL_Set(mu_CheckboxData->channel, mu_CheckboxData->state, 0);
	}
	return 0;
}
int8_t MicroUI_ProcessIcon(mu_Context *ctx, void *funcdata) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: ProcessIcon");
	if (!ctx || !funcdata) return -1;
	mu_IconProcessData_t *mu_IconData = (mu_IconProcessData_t *)funcdata;
	int state = 0;
	if (mu_IconData->channel != -1) state = CHANNEL_Get(mu_IconData->channel);
	//mu_Id id = mu_get_id(ctx, &state, sizeof(state));
	mu_Rect r; mu_layout_next(ctx, &r);
	mu_Rect box; 
	box.x = r.x;
	box.y = r.y;
	box.w = box.h = r.w < r.h ? r.w : r.h;
	/* draw */
	//mu_draw_control_frame(ctx, id, &box, MU_COLOR_BASE, 0);
	if (state) {
		mu_draw_icon(ctx, mu_IconData->icon_on, &box, &ctx->style->colors[MU_COLOR_TEXT]);
	} else {
		mu_draw_icon(ctx, mu_IconData->icon_off, &box, &ctx->style->colors[MU_COLOR_TEXT]);
	}
	return 0;
}

char *MicroUI_AddString(obk_slist_t *list, char *str) {
	if (!str) return NULL;	
	int   len = strlen(str);
	if (!len) return NULL;		
	
	mu_String_LI_t *str_li;
	obk_slist_t    *slist_e;
	obk_slist_for_each(slist_e, list) {
		str_li = obk_slist_entry(slist_e, mu_String_LI_t, item);
		if (str_li->frm_str)
			if (strcmp(str_li->frm_str, str) == 0) return str_li->frm_str;
	}
	
	char *cstr  =            (char*)os_malloc(len+1);
	str_li      = (mu_String_LI_t *)os_malloc(sizeof(mu_String_LI_t));
	if (!cstr || !str_li) {
		if (cstr)   os_free(cstr);
		if (str_li) os_free(str_li);
		return NULL;
	}
	memmove(cstr, str, len); 
	cstr[len] = 0;	
	
	str_li->frm_str = cstr;
	obk_slist_init(&str_li->item);
	obk_slist_append(list, &str_li->item);
	return str_li->frm_str;
}

int8_t MicroUI_LoadWindow(cJSON *mu_json_window, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_BeginWindow = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));
	mu_ProcessItem_t *mupi_EndWindow   = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));
	mu_BeginWindowProcessData_t *mu_BeginWindowData = (mu_BeginWindowProcessData_t*)os_malloc(sizeof(mu_BeginWindowProcessData_t));
	if (!mupi_BeginWindow || !mupi_EndWindow || !mu_BeginWindowData) {
		if (mupi_BeginWindow)   os_free(mupi_BeginWindow);
		if (mupi_EndWindow)     os_free(mupi_EndWindow);
		if (mu_BeginWindowData) os_free(mu_BeginWindowData);		
		return -1;
	}
	/*add*/
	mupi_BeginWindow->processfunc = MicroUI_ProcessBeginWindow;
	mupi_BeginWindow->func_data = mu_BeginWindowData;
	obk_stree_init(&mupi_BeginWindow->node);
	obk_stree_append(*node, &mupi_BeginWindow->node);
	*node = &mupi_BeginWindow->node;
	obk_stree_t *_node = &mupi_BeginWindow->node;
	/*title*/
	mu_BeginWindowData->frm_str = NULL;
	cJSON *mu_json_title = cJSON_GetObjectItem(mu_json_window, "title");
	if ((mu_json_title) && cJSON_IsString(mu_json_title)) {
		char *title_str = cJSON_GetStringValue(mu_json_title);
		mu_BeginWindowData->frm_str = MicroUI_AddString(&MicroUI.mu_strings_list, title_str);
	}	
	/*opt*/
	mu_BeginWindowData->opt = 0;
	cJSON *mu_json_opt = cJSON_GetObjectItem(mu_json_window, "opt");
	if (mu_json_opt && cJSON_IsArray(mu_json_opt)) {
		uint8_t opt_count = cJSON_GetArraySize(mu_json_opt);
		for (uint8_t opt_i=0; opt_i < opt_count; opt_i++) {
			cJSON *mu_json_opt_i = cJSON_GetArrayItem(mu_json_opt, opt_i);
			if (mu_json_opt_i && cJSON_IsString(mu_json_opt_i)) {				
				mu_BeginWindowData->opt |= MicroUI_ParseOption(cJSON_GetStringValue(mu_json_opt_i));
			}
		}
	}	
	/*window rect*/
	cJSON *mu_json_rect = cJSON_GetObjectItem(mu_json_window, "rect");
	if (mu_json_rect) {
		cJSON *mu_json_rect_items;
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "x");
		if (mu_json_rect_items) mu_BeginWindowData->rect.x = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "y");
		if (mu_json_rect_items) mu_BeginWindowData->rect.y = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "w");
		if (mu_json_rect_items) mu_BeginWindowData->rect.w = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "h");
		if (mu_json_rect_items) mu_BeginWindowData->rect.h = cJSON_GetNumberValue(mu_json_rect_items);
	}
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: window[%d,%d,%d,%d]-%d", mu_BeginWindowData->rect.x, mu_BeginWindowData->rect.y,
	//                                                                    mu_BeginWindowData->rect.w, mu_BeginWindowData->rect.h, mu_BeginWindowData->opt);
	cJSON *mu_json_win_items = cJSON_GetObjectItem(mu_json_window, "items");
	if (mu_json_win_items) {		
		int8_t result = MicroUI_LoadItems(mu_json_win_items, &_node, 1);
	}
	
	mupi_EndWindow->processfunc = MicroUI_ProcessEndWindow;
	mupi_EndWindow->func_data = NULL;
	obk_stree_init(&mupi_EndWindow->node);
	
	if (_node == &mupi_BeginWindow->node) {
		obk_stree_add_child(_node, &mupi_EndWindow->node);
	} else {
		obk_stree_append(_node, &mupi_EndWindow->node);
	}
	
	return 0;
}

int8_t MicroUI_LoadLabel(cJSON *mu_json_label, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Label = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_LabelProcessData_t *mu_LabelData = (mu_LabelProcessData_t*)os_malloc(sizeof(mu_LabelProcessData_t));
	if (!mupi_Label || !mu_LabelData) {
		if (mupi_Label)   os_free(mupi_Label);
		if (mu_LabelData) os_free(mu_LabelData);
		return -1;
	}
	
	mu_LabelData->frm_str = NULL;
	cJSON *mu_json_text = cJSON_GetObjectItem(mu_json_label, "text");
	if (mu_json_text && cJSON_IsString(mu_json_text)) {
		char *text_str = cJSON_GetStringValue(mu_json_text);
		mu_LabelData->frm_str = MicroUI_AddString(&MicroUI.mu_strings_list, text_str);
	}
	if (mu_LabelData->frm_str == NULL) {
		if (mupi_Label)   os_free(mupi_Label);
		if (mu_LabelData) os_free(mu_LabelData);
		return -1;
	}
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: label[%s]", mu_LabelData->frm_str);
																		
	mupi_Label->processfunc = MicroUI_ProcessLabel;
	mupi_Label->func_data = mu_LabelData;
	obk_stree_init(&mupi_Label->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Label->node);
	} else {
		obk_stree_append(*node, &mupi_Label->node);
	}
	*node = &mupi_Label->node;
	/* text align */
	mu_LabelData->align = 0;	
	cJSON *mu_json_align = cJSON_GetObjectItem(mu_json_label, "align");
	if (mu_json_align) {
		char *align_str = cJSON_GetStringValue(mu_json_align);
		if (align_str) {
				if (strcmp(align_str, "center") == 0) mu_LabelData->align = MU_OPT_ALIGNCENTER;
				else if (strcmp(align_str, "right") == 0) mu_LabelData->align = MU_OPT_ALIGNRIGHT;
		}
	}	
	/* text color */
	mu_LabelData->color.r = obk_gui_style.colors[MU_COLOR_TEXT].r;
	mu_LabelData->color.g = obk_gui_style.colors[MU_COLOR_TEXT].g;
	mu_LabelData->color.b = obk_gui_style.colors[MU_COLOR_TEXT].b;
	mu_LabelData->color.a = obk_gui_style.colors[MU_COLOR_TEXT].a;
	cJSON *mu_json_color = cJSON_GetObjectItem(mu_json_label, "color");
	if (mu_json_color) {
		cJSON *mu_json_color_items;
		mu_json_color_items = cJSON_GetObjectItem(mu_json_color, "r");
		if (mu_json_color_items) mu_LabelData->color.r = cJSON_GetNumberValue(mu_json_color_items);
		mu_json_color_items = cJSON_GetObjectItem(mu_json_color, "g");
		if (mu_json_color_items) mu_LabelData->color.g = cJSON_GetNumberValue(mu_json_color_items);
		mu_json_color_items = cJSON_GetObjectItem(mu_json_color, "b");
		if (mu_json_color_items) mu_LabelData->color.b = cJSON_GetNumberValue(mu_json_color_items);
		mu_json_color_items = cJSON_GetObjectItem(mu_json_color, "a");
		if (mu_json_color_items) mu_LabelData->color.a = cJSON_GetNumberValue(mu_json_color_items);
	}	
	/* text font name & size */
	cJSON *mu_json_size;
	mu_json_size = cJSON_GetObjectItem(mu_json_label, "size");
	if (mu_json_size) mu_LabelData->size = cJSON_GetNumberValue(mu_json_size);
		
	mu_LabelData->channel = -1;
	cJSON *mu_json_args;
	mu_json_args = cJSON_GetObjectItem(mu_json_label, "arguments");
	if (mu_json_args) {
		cJSON *mu_json_arg;
		if (cJSON_IsArray(mu_json_args)) {
			uint8_t arg_count = cJSON_GetArraySize(mu_json_args);
			for (uint8_t i=0; i<arg_count; i++) {
				mu_json_arg = cJSON_GetArrayItem(mu_json_args, i);
				if (cJSON_IsString(mu_json_arg)) {
					char *arg_str = cJSON_GetStringValue(mu_json_arg);
					if ((arg_str[0] == 'C') && (arg_str[1] == 'H')) {
						mu_LabelData->channel = atoi(arg_str+2);
					}
				} else
				if (cJSON_IsNumber(mu_json_arg)) {
					mu_LabelData->channel = cJSON_GetNumberValue(mu_json_arg);
				}				
				if (i>0) break; /* only first now */
			}
		} else {
			
		}
	}	
	return 0;
}

uint16_t MicroUI_ParseOption(const char *opt_str) {	
	if (!opt_str) return 0;	
	if (!strcmp(opt_str, "center")) 		return MU_OPT_ALIGNCENTER;
	if (!strcmp(opt_str, "right")) 			return MU_OPT_ALIGNRIGHT;
	if (!strcmp(opt_str, "nointeract"))  	return MU_OPT_NOINTERACT;
	if (!strcmp(opt_str, "noframe"))     	return MU_OPT_NOFRAME;
	if (!strcmp(opt_str, "notitle"))     	return MU_OPT_NOTITLE;
	if (!strcmp(opt_str, "noscroll"))     	return MU_OPT_NOSCROLL;
	if (!strcmp(opt_str, "noclose"))     	return MU_OPT_NOCLOSE;
	if (!strcmp(opt_str, "autosize"))     	return MU_OPT_AUTOSIZE;
	if (!strcmp(opt_str, "popup"))       	return MU_OPT_POPUP;
	if (!strcmp(opt_str, "closed"))     	return MU_OPT_CLOSED;
	if (!strcmp(opt_str, "expanded"))     	return MU_OPT_EXPANDED;
	return 0;
}


int8_t MicroUI_LoadHeader(cJSON *mu_json_header, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Header = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_HeaderProcessData_t *mu_HeaderData = (mu_HeaderProcessData_t*)os_malloc(sizeof(mu_HeaderProcessData_t));
	if (!mupi_Header || !mu_HeaderData) {
		if (mupi_Header)   os_free(mupi_Header);
		if (mu_HeaderData) os_free(mu_HeaderData);
		return -1;
	}
	mupi_Header->processfunc = MicroUI_ProcessHeader;
	mupi_Header->func_data = mu_HeaderData;
	obk_stree_init(&mupi_Header->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Header->node);
	} else {
		obk_stree_append(*node, &mupi_Header->node);
	}
	*node = &mupi_Header->node;
	/*title*/
	mu_HeaderData->frm_str = NULL;
	cJSON *mu_json_title = cJSON_GetObjectItem(mu_json_header, "title");
	if (mu_json_title && cJSON_IsString(mu_json_title)) {
		char *title_str = cJSON_GetStringValue(mu_json_title);
		mu_HeaderData->frm_str = MicroUI_AddString(&MicroUI.mu_strings_list, title_str);
	}
	/*opt*/
	mu_HeaderData->opt=0;	
	cJSON *mu_json_opt = cJSON_GetObjectItem(mu_json_header, "opt");
	if (mu_json_opt && cJSON_IsArray(mu_json_opt)) {
		uint8_t opt_count = cJSON_GetArraySize(mu_json_opt);
		for (uint8_t opt_i=0; opt_i < opt_count; opt_i++) {
			cJSON *mu_json_opt_i = cJSON_GetArrayItem(mu_json_opt, opt_i);
			if (mu_json_opt_i && cJSON_IsString(mu_json_opt_i)) {				
				mu_HeaderData->opt |= MicroUI_ParseOption(cJSON_GetStringValue(mu_json_opt_i));
			}
		}
	}
	/*item*/
	obk_stree_t *_node = *node;
	cJSON *mu_json_items = cJSON_GetObjectItem(mu_json_header, "items");
	if (mu_json_items) {		
		int8_t result = MicroUI_LoadItems(mu_json_items, &_node, 1);
	}
	return 0;
}
int8_t MicroUI_LoadLayout(cJSON *mu_json_layout, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Layout = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_LayoutProcessData_t *mu_LayoutData;
	if (!mupi_Layout) {
		if (mupi_Layout)   os_free(mupi_Layout);
		return -1;
	}
	
	cJSON *mu_json_rect = cJSON_GetObjectItem(mu_json_layout, "rect");
	if (mu_json_rect) {
		mu_LayoutData = (mu_LayoutProcessData_t*)os_malloc(sizeof(mu_LayoutProcessData_t));
		mu_LayoutData->layout_type = 0;
		cJSON *mu_json_rect_items;
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "x");
		if (mu_json_rect_items) mu_LayoutData->rect.x = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "y");
		if (mu_json_rect_items) mu_LayoutData->rect.y = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "w");
		if (mu_json_rect_items) mu_LayoutData->rect.w = cJSON_GetNumberValue(mu_json_rect_items);
		mu_json_rect_items = cJSON_GetObjectItem(mu_json_rect, "h");
		if (mu_json_rect_items) mu_LayoutData->rect.h = cJSON_GetNumberValue(mu_json_rect_items);
		cJSON *mu_json_relative = cJSON_GetObjectItem(mu_json_layout, "relative");
		if (mu_json_relative)
			mu_LayoutData->relative = 1;
	} else {
		cJSON *mu_json_width = cJSON_GetObjectItem(mu_json_layout, "width");
		if (mu_json_width) {
			mu_LayoutData = (mu_LayoutProcessData_t*)os_malloc(sizeof(mu_LayoutProcessData_t));
			mu_LayoutData->layout_type = 2;
			mu_LayoutData->width = cJSON_GetNumberValue(mu_json_width);
		} else {			
			cJSON *mu_json_row = cJSON_GetObjectItem(mu_json_layout, "row");
			if (mu_json_row && cJSON_IsArray(mu_json_row) && cJSON_GetArraySize(mu_json_row)) {
				int row_count = cJSON_GetArraySize(mu_json_row);					
				mu_LayoutData = (mu_LayoutProcessData_t*)os_malloc(sizeof(mu_LayoutProcessData_t)+row_count*sizeof(int16_t));
				mu_LayoutData->layout_type = 3;
				mu_LayoutData->row_height = obk_gui_style.size.y;
				mu_LayoutData->row_count = row_count;					
				int16_t *widths_p = (int16_t*)((uint8_t*)mu_LayoutData + sizeof(mu_LayoutProcessData_t));					
				for (int w_i = 0; w_i < mu_LayoutData->row_count; w_i++) {
					cJSON *mu_json_row_width = cJSON_GetArrayItem(mu_json_row, w_i);
					if (mu_json_row_width) {
						*widths_p = cJSON_GetNumberValue(mu_json_row_width);
						widths_p++;
					} else {
						mu_LayoutData->row_count--;
					}
				}
				cJSON *mu_json_row_h = cJSON_GetObjectItem(mu_json_layout, "height");
				if (mu_json_row_h) {
					mu_LayoutData->row_height = cJSON_GetNumberValue(mu_json_row_h);
				}
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: layout_row[%d][%d]", mu_LayoutData->row_count, mu_LayoutData->row_height);
			} else {
				cJSON *mu_json_height = cJSON_GetObjectItem(mu_json_layout, "height");
				if (mu_json_height) {
					mu_LayoutData = (mu_LayoutProcessData_t*)os_malloc(sizeof(mu_LayoutProcessData_t));
					mu_LayoutData->layout_type = 1;
					mu_LayoutData->height = cJSON_GetNumberValue(mu_json_height);
				} else {
					if (mupi_Layout)   os_free(mupi_Layout);					
					return -1;					
				}
			}
		}
	}	
	mupi_Layout->processfunc = MicroUI_ProcessLayout;
	mupi_Layout->func_data = mu_LayoutData;
	obk_stree_init(&mupi_Layout->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Layout->node);
	} else {
		obk_stree_append(*node, &mupi_Layout->node);
	}
	*node = &mupi_Layout->node;	
	return 0;
}

int8_t MicroUI_LoadButton(cJSON *mu_json_button, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Button = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_ButtonProcessData_t *mu_ButtonData = (mu_ButtonProcessData_t*)os_malloc(sizeof(mu_ButtonProcessData_t));
	if (!mupi_Button || !mu_ButtonData) {
		if (mupi_Button)   os_free(mupi_Button);
		if (mu_ButtonData) os_free(mu_ButtonData);
		return -1;
	}
	/*title*/
	mu_ButtonData->title = NULL;
	cJSON *mu_json_title = cJSON_GetObjectItem(mu_json_button, "title");
	if (mu_json_title && cJSON_IsString(mu_json_title)) {
		char *title_str = cJSON_GetStringValue(mu_json_title);
		mu_ButtonData->title = MicroUI_AddString(&MicroUI.mu_strings_list, title_str);
	}
	/*opt*/
	mu_ButtonData->opt=0;	
	cJSON *mu_json_opt = cJSON_GetObjectItem(mu_json_button, "opt");
	if (mu_json_opt && cJSON_IsArray(mu_json_opt)) {
		uint8_t opt_count = cJSON_GetArraySize(mu_json_opt);
		for (uint8_t opt_i=0; opt_i < opt_count; opt_i++) {
			cJSON *mu_json_opt_i = cJSON_GetArrayItem(mu_json_opt, opt_i);
			if (mu_json_opt_i && cJSON_IsString(mu_json_opt_i)) {				
				mu_ButtonData->opt |= MicroUI_ParseOption(cJSON_GetStringValue(mu_json_opt_i));
			}
		}
	}
	/*command string*/
	mu_ButtonData->command = NULL;
	cJSON *mu_json_command = cJSON_GetObjectItem(mu_json_button, "command");
	if (mu_json_command && cJSON_IsString(mu_json_command)) {
		char *command_str = cJSON_GetStringValue(mu_json_command);
		mu_ButtonData->command = MicroUI_AddString(&MicroUI.mu_commands_list, command_str);
	}
	/*icon*/
	mu_ButtonData->icon = 0;	
	cJSON *mu_json_icon = cJSON_GetObjectItem(mu_json_button, "icon");
	if (mu_json_icon && cJSON_IsNumber(mu_json_icon)) {
		mu_ButtonData->icon = cJSON_GetNumberValue(mu_json_icon);
	}
	mupi_Button->processfunc = MicroUI_ProcessButton;
	mupi_Button->func_data = mu_ButtonData;
	obk_stree_init(&mupi_Button->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Button->node);
	} else {
		obk_stree_append(*node, &mupi_Button->node);
	}
	*node = &mupi_Button->node;	
	return 0;
}
int8_t MicroUI_LoadCheckBox(cJSON *mu_json_checkbox, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Checkbox = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_CheckboxProcessData_t *mu_CheckboxData = (mu_CheckboxProcessData_t*)os_malloc(sizeof(mu_CheckboxProcessData_t));
	if (!mupi_Checkbox || !mu_CheckboxData) {
		if (mupi_Checkbox)   os_free(mupi_Checkbox);
		if (mu_CheckboxData) os_free(mu_CheckboxData);
		return -1;
	}
	/*label*/
	mu_CheckboxData->label = NULL;
	cJSON *mu_json_label = cJSON_GetObjectItem(mu_json_checkbox, "label");
	if (mu_json_label && cJSON_IsString(mu_json_label)) {
		char *label_str = cJSON_GetStringValue(mu_json_label);
		mu_CheckboxData->label = MicroUI_AddString(&MicroUI.mu_strings_list, label_str);
	}
	/*state from channel*/
	mu_CheckboxData->channel = -1;
	cJSON *mu_json_state;
	mu_json_state = cJSON_GetObjectItem(mu_json_checkbox, "state");
	if (mu_json_state && cJSON_IsString(mu_json_state)) {
		char *state_str = cJSON_GetStringValue(mu_json_state);
		if ((state_str[0] == 'C') && (state_str[1] == 'H')) {
			mu_CheckboxData->channel = atoi(state_str+2);
		}
	}	
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: checkbox[%s].%d", mu_CheckboxData->label ? mu_CheckboxData->label:"", mu_CheckboxData->channel);
	
	mupi_Checkbox->processfunc = MicroUI_ProcessCheckbox;
	mupi_Checkbox->func_data = mu_CheckboxData;
	obk_stree_init(&mupi_Checkbox->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Checkbox->node);
	} else {
		obk_stree_append(*node, &mupi_Checkbox->node);
	}
	*node = &mupi_Checkbox->node;	
	return 0;
}

int8_t MicroUI_LoadIcon(cJSON *mu_json_icon, obk_stree_t **node, int8_t addchild) {
	mu_ProcessItem_t *mupi_Icon = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));	
	mu_IconProcessData_t *mu_IconData = (mu_IconProcessData_t*)os_malloc(sizeof(mu_IconProcessData_t));
	if (!mupi_Icon || !mu_IconData) {
		if (mupi_Icon)   os_free(mupi_Icon);
		if (mu_IconData) os_free(mu_IconData);
		return -1;
	}
	/*state from channel*/
	mu_IconData->channel = -1;
	cJSON *mu_json_state;
	mu_json_state = cJSON_GetObjectItem(mu_json_icon, "state");
	if (mu_json_state && cJSON_IsString(mu_json_state)) {
		char *state_str = cJSON_GetStringValue(mu_json_state);
		if ((state_str[0] == 'C') && (state_str[1] == 'H')) {
			mu_IconData->channel = atoi(state_str+2);
		}
	}
	/*state "on" settings*/
	mu_IconData->icon_on  = -1;
	cJSON *mu_json_stateOn = cJSON_GetObjectItem(mu_json_icon, "on");
	if (mu_json_stateOn) {
		cJSON *mu_json_stateOn_icon = cJSON_GetObjectItem(mu_json_stateOn, "icon");
		if (mu_json_stateOn_icon && cJSON_IsNumber(mu_json_stateOn_icon)) {
			mu_IconData->icon_on = cJSON_GetNumberValue(mu_json_stateOn_icon);
		}
		/*todo: color*/
	}
	
	/*state "off" settings*/
	mu_IconData->icon_off = -1;
	cJSON *mu_json_stateOff = cJSON_GetObjectItem(mu_json_icon, "off");
	if (mu_json_stateOff) {
		cJSON *mu_json_stateOff_icon = cJSON_GetObjectItem(mu_json_stateOff, "icon");
		if (mu_json_stateOff_icon && cJSON_IsNumber(mu_json_stateOff_icon)) {
			mu_IconData->icon_off = cJSON_GetNumberValue(mu_json_stateOff_icon);
		}
		/*todo: color*/
	}	
	
	mupi_Icon->processfunc = MicroUI_ProcessIcon;
	mupi_Icon->func_data = mu_IconData;
	obk_stree_init(&mupi_Icon->node);
	if (addchild) {
		obk_stree_add_child(*node, &mupi_Icon->node);
	} else {
		obk_stree_append(*node, &mupi_Icon->node);
	}
	*node = &mupi_Icon->node;	
	return 0;
}	
int8_t MicroUI_LoadItems(cJSON *mu_json_array, obk_stree_t **node, int8_t addchild) {
	if (cJSON_IsArray(mu_json_array) == false) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_LoadItems: gui=not array[%d]", __LINE__);
		//rtos_delay_milliseconds(200);
		return -1;
	}
	int8_t _addchild = addchild;
	cJSON *mu_gui, *mu_array_i;
	int8_t gui_count = cJSON_GetArraySize(mu_json_array);
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: %s[%d]",mu_json_array->string,gui_count);
	if (gui_count > 0) {
		for (int8_t i = 0; i < gui_count; i++) {
			mu_array_i = cJSON_GetArrayItem(mu_json_array, i);
			if (!mu_array_i) continue;
			mu_gui = cJSON_GetObjectItem(mu_array_i, "window");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadWindow");
				if (MicroUI_LoadWindow(mu_gui, node, _addchild)) {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide window");
					continue;
				}
				_addchild = 0;
				continue;
			}
			mu_gui = cJSON_GetObjectItem(mu_array_i, "label");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadLabel");
				if (MicroUI_LoadLabel(mu_gui, node, _addchild)) {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide label");
					continue;
				}
				_addchild = 0;
				continue;
			}
			mu_gui = cJSON_GetObjectItem(mu_array_i, "layout");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadLayout");
				if (MicroUI_LoadLayout(mu_gui, node, _addchild)) {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide layout");
					continue;
				}
				_addchild = 0;
				continue;
			}	
			mu_gui = cJSON_GetObjectItem(mu_array_i, "header");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadHeader");
				if (MicroUI_LoadHeader(mu_gui, node, _addchild))  {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide header");
					continue;
				}
				_addchild = 0;
				continue;
			}
			mu_gui = cJSON_GetObjectItem(mu_array_i, "button");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadButton");
				if (MicroUI_LoadButton(mu_gui, node, _addchild))  {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide button");
					continue;
				}
				_addchild = 0;
				continue;
			}
			mu_gui = cJSON_GetObjectItem(mu_array_i, "checkbox");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadCheckbox");
				if (MicroUI_LoadCheckBox(mu_gui, node, _addchild))  {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide checkbox");
					continue;
				}
				_addchild = 0;
				continue;
			}
			mu_gui = cJSON_GetObjectItem(mu_array_i, "icon");
			if (mu_gui) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LoadIcon");
				if (MicroUI_LoadIcon(mu_gui, node, _addchild))  {
					addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "MicroUI: Invalide icon");
					continue;
				}
				_addchild = 0;
				continue;
			}			
		}
	}
	return 0;
}

#define JSON_BUF_SIZE 256

typedef struct {
	cJSON       *json;
	obk_slist_t  item;
} cJSON_Stack_Item_t;

typedef struct {
	cJSON       *json_object;
	cJSON       *json_item;
	obk_slist_t  cJSON_Stack;
} LoadJSON_t;

int8_t MicroUI_LoadJSON_ParsePrimitive(cJSON *json, const char *str) {
	if ((!json) || (!str)) return -1;
	char *after_end;
	double number = strtod((const char*)str, (char**)&after_end);
	if (str != after_end) {
		/*it is number*/
		json->type = cJSON_Number;
		cJSON_SetNumberValue(json, number);			
		return 0;
	} else /* not a number */
	if (strcmp(str, "true") == 0) {
		/*it is true*/
		json->type = cJSON_True;
		return 0;
	} else /* not a boolean true */
	if (strcmp(str, "false") == 0) {
		/*it is false*/
		json->type = cJSON_False;
		return 0;
	} else { /* not a boolean false */
		
	}
	return -1;
}

void MicroUI_LoadJSON_start_object(void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: Start Object");	
	if (!json_parser_data) return;
    //rtos_delay_milliseconds(200);	
	
	if (json_parser_data->json_object) {
		if (cJSON_IsArray(json_parser_data->json_object)) {
			json_parser_data->json_item = cJSON_CreateObject();
			cJSON_AddItemToArray(json_parser_data->json_object, json_parser_data->json_item);			
		} else {
			
		}
		cJSON_Stack_Item_t *json_si = os_malloc(sizeof(cJSON_Stack_Item_t));
		json_si->json = json_parser_data->json_object;
		obk_slist_push(&json_parser_data->cJSON_Stack, &json_si->item);
		json_parser_data->json_object = json_parser_data->json_item;
		json_parser_data->json_item = 0;
	} else {
		/*new object*/
		json_parser_data->json_object = cJSON_CreateObject();
	}
}

void MicroUI_LoadJSON_end_object(void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: End Object");
	if (!json_parser_data) return;
	//rtos_delay_milliseconds(200); 
	
	if (!obk_slist_isempty(&json_parser_data->cJSON_Stack)) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndObj: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
		//rtos_delay_milliseconds(200);
		cJSON_Stack_Item_t *json_si = obk_slist_entry(obk_slist_pop(&json_parser_data->cJSON_Stack), cJSON_Stack_Item_t, item);
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndObj: json_si=%d[%d]", json_si, __LINE__);
		//rtos_delay_milliseconds(200);
		if (json_si) {
			json_parser_data->json_object = json_si->json;
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndObj: %d[%d]", json_parser_data->json_object, __LINE__);
			//rtos_delay_milliseconds(200);
			os_free(json_si);
			json_parser_data->json_item = NULL;
		} else {
			//json_parser_data->json_object = NULL;
			json_parser_data->json_item = NULL;
		}
	}
}

void MicroUI_LoadJSON_start_array(void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: Start Array");
	if (!json_parser_data) return;
	//rtos_delay_milliseconds(200); 
		
	if (json_parser_data->json_item) {
		json_parser_data->json_item->type = cJSON_Array;
		cJSON_Stack_Item_t *json_si = os_malloc(sizeof(cJSON_Stack_Item_t));
		json_si->json = json_parser_data->json_object;
		obk_slist_push(&json_parser_data->cJSON_Stack, &json_si->item);
		json_parser_data->json_object = json_parser_data->json_item;
		json_parser_data->json_item = 0;
	} else {
		/* ??? */
	}
}

void MicroUI_LoadJSON_end_array(void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: End Array");
	if (!json_parser_data) return;
	//rtos_delay_milliseconds(200); 
	
	if (!obk_slist_isempty(&json_parser_data->cJSON_Stack)) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndArr: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
		//rtos_delay_milliseconds(200);
		cJSON_Stack_Item_t *json_si = obk_slist_entry(obk_slist_pop(&json_parser_data->cJSON_Stack), cJSON_Stack_Item_t, item);
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndArr: json_si=%d[%d]", json_si, __LINE__);
		//rtos_delay_milliseconds(200);
		if (json_si) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndArr: [%d]", __LINE__);
			//rtos_delay_milliseconds(200);
			json_parser_data->json_object = json_si->json;
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_EndArr: %d[%d]", json_parser_data->json_object, __LINE__);
			//rtos_delay_milliseconds(200);
			os_free(json_si);
			json_parser_data->json_item = NULL;
		} else {
			json_parser_data->json_object = NULL;
			json_parser_data->json_item = NULL;
		}
	}
}

void MicroUI_LoadJSON_key(const char *key, size_t key_len, void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: Key=%s", key);
	if (!json_parser_data) return;
	//rtos_delay_milliseconds(200); 	
	//return;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Key: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
	//rtos_delay_milliseconds(200);
	if (json_parser_data->json_object) {
		if (cJSON_IsObject(json_parser_data->json_object)) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Key: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
			//rtos_delay_milliseconds(200);
			json_parser_data->json_item = cJSON_CreateObject();
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Key: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
			//rtos_delay_milliseconds(200);
			cJSON_AddItemToObject(json_parser_data->json_object, key, json_parser_data->json_item);
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Key: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
			//rtos_delay_milliseconds(200);
		} else {
			if (cJSON_IsArray(json_parser_data->json_object)) {
				json_parser_data->json_item = cJSON_CreateObject();
				cJSON_AddItemToArray(json_parser_data->json_object, json_parser_data->json_item);
				if (MicroUI_LoadJSON_ParsePrimitive(json_parser_data->json_item, key)) {
					json_parser_data->json_item->type = cJSON_String;
					cJSON_SetValuestring(json_parser_data->json_item, key);
				}
			}
		}
	}
}

void MicroUI_LoadJSON_string(const char *value, size_t len, void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: String=%s", value);
	if (!json_parser_data) return;
	//rtos_delay_milliseconds(200); 
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_String: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
	//rtos_delay_milliseconds(200);
	if (json_parser_data->json_item) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_String: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
		//rtos_delay_milliseconds(200);
		json_parser_data->json_item->type = cJSON_String;
		if (len) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_String: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
			//rtos_delay_milliseconds(200);
			cJSON_SetValuestring(json_parser_data->json_item, value);
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_String: %d,%d[%d]", json_parser_data->json_object, json_parser_data->json_item, __LINE__);
			//rtos_delay_milliseconds(200);
		}
	}
}

void MicroUI_LoadJSON_primitive(const char *value, size_t len, void *user_arg) {
	LoadJSON_t *json_parser_data = (LoadJSON_t *)user_arg;
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON: Primitive=%s", value);
	//rtos_delay_milliseconds(200); 
	if ((!json_parser_data) || (!len)) return;
	
	if (json_parser_data->json_item) {
		char *after_end;
		double number = strtod((const char*)value, (char**)&after_end);
		if (value != after_end) {
			/*it is number*/
			json_parser_data->json_item->type = cJSON_Number;
			cJSON_SetNumberValue(json_parser_data->json_item, number);			
		} else /* not a number */
		if (strcmp(value, "true") == 0) {
			/*it is true*/
			json_parser_data->json_item->type = cJSON_True;
		} else /* not a boolean true */
		if (strcmp(value, "false") == 0) {
			/*it is false*/
			json_parser_data->json_item->type = cJSON_False;
		} else { /* not a boolean false */
			json_parser_data->json_item->type = cJSON_Invalid;
		}
	} else 
	if ((json_parser_data->json_object) && (cJSON_IsArray(json_parser_data->json_object))) {
		cJSON *json_array_item = cJSON_CreateObject();
		cJSON_AddItemToArray(json_parser_data->json_object, json_array_item);
		if (MicroUI_LoadJSON_ParsePrimitive(json_array_item, value)) {
			json_parser_data->json_item->type = cJSON_String;
			cJSON_SetValuestring(json_parser_data->json_item, value);
		}
	}
}

static jsmn_stream_callbacks_t cbs = {
	MicroUI_LoadJSON_start_array,
	MicroUI_LoadJSON_end_array,
	MicroUI_LoadJSON_start_object,
	MicroUI_LoadJSON_end_object,
	MicroUI_LoadJSON_key,
	MicroUI_LoadJSON_string,
	MicroUI_LoadJSON_primitive
};

int8_t MicroUI_LoadJSON(void){
	cJSON *mu_json = NULL;	
	//return -1;
#if ENABLE_LITTLEFS
	if (lfs_present()) {
		jsmn_stream_parser *parser;
		parser = (jsmn_stream_parser *)os_malloc(sizeof(jsmn_stream_parser));
		if (!parser) return -1;
		memset((uint8_t *)parser, 0, sizeof(jsmn_stream_parser));
		
		LoadJSON_t *parser_data;		
		parser_data = (LoadJSON_t *)os_malloc(sizeof(LoadJSON_t));
		if (!parser_data) {
			if (parser) os_free(parser);
			return -1;
		}
		memset((uint8_t *)parser_data, 0, sizeof(LoadJSON_t));
		
		uint8_t *file_buf;
		file_buf = (uint8_t *)os_malloc(JSON_BUF_SIZE);
		if (!file_buf) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_DRV, "MicroUI: Not enought memory to load json file.");
			if (parser)      os_free(parser);
			if (parser_data) os_free(parser_data);
			return -1;
		}
	
		const char *fname = "microui.json";
		lfs_file_t file;	
		int lfsres;
		
		memset(&file, 0, sizeof(lfs_file_t));
		lfsres = lfs_file_open(&lfs, &file, fname, LFS_O_RDONLY);

		if (lfsres >= 0) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: file %s opened", fname);
			//len = lfs_file_size(&lfs,&file);
			lfs_file_seek(&lfs,&file,0,LFS_SEEK_SET);
			
			jsmn_stream_init(parser, &cbs, parser_data);

			do {
				//int read_byte;
				lfsres = lfs_file_read(&lfs, &file, file_buf, JSON_BUF_SIZE);
				if (lfsres <= 0) break;
				for (int i=0; i<lfsres; i++)
					jsmn_stream_parse(parser, (char)file_buf[i]);
			} while (1);
			//rtos_delay_milliseconds(500);
			mu_json = parser_data->json_object;
			lfs_file_close(&lfs, &file);			
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: file %s loaded", fname);
		}
		if (file_buf)    os_free(file_buf);
		if (parser)      os_free(parser);
		if (parser_data) os_free(parser_data);
	} else {
#else
		addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: LITTLEFS not present", fname);
		return 0;
#endif
	}
	//if (mu_json) cJSON_Delete(mu_json);
	
	//return 0;
		
		if (mu_json) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Load: json=%d[%d]", mu_json, __LINE__);
			//rtos_delay_milliseconds(200);
			cJSON *mu_style_json = cJSON_GetObjectItem(mu_json, "style");
			if (mu_style_json) {
				cJSON *mu_style_e;
				/*TODO: font*/
				/*font_size*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "font_size");
				if (mu_style_e) {
					obk_gui_style.font_size = cJSON_GetNumberValue(mu_style_e);
				}
				/*layout size*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "size");
				if (mu_style_e) 
					if (cJSON_IsArray(mu_style_e)) 
						if (cJSON_GetArraySize(mu_style_e) == 2) {
							cJSON *mu_layout_size;
							mu_layout_size = cJSON_GetArrayItem(mu_style_e, 0);
							obk_gui_style.size.x = cJSON_GetNumberValue(mu_layout_size);
							mu_layout_size = cJSON_GetArrayItem(mu_style_e, 1);
							obk_gui_style.size.x = cJSON_GetNumberValue(mu_layout_size);
						}
				/*padding*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "padding");
				if (mu_style_e) {
					obk_gui_style.padding = cJSON_GetNumberValue(mu_style_e);
				}
				/*spacing*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "spacing");
				if (mu_style_e) {
					obk_gui_style.spacing = cJSON_GetNumberValue(mu_style_e);
				}
				/*indent*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "indent");
				if (mu_style_e) {
					obk_gui_style.indent = cJSON_GetNumberValue(mu_style_e);
				}
				/*title_height*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "title_height");
				if (mu_style_e) {
					obk_gui_style.title_height = cJSON_GetNumberValue(mu_style_e);
				}
				/*scrollbar_size*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "scrollbar_size");
				if (mu_style_e) {
					obk_gui_style.scrollbar_size = cJSON_GetNumberValue(mu_style_e);
				}
				/*thumb_size*/
				mu_style_e = cJSON_GetObjectItem(mu_style_json, "thumb_size");
				if (mu_style_e) {
					obk_gui_style.thumb_size = cJSON_GetNumberValue(mu_style_e);
				}
				/*colors*/
				const char *color_json_names[] = {"color_text", "color_border", "color_windowbg", "color_titlebg", "color_titletext", 
				                                  "color_panelbg", "color_button", "color_buttonhover", "color_buttonfocus", "color_base", "color_basehover",
												  "color_basefocus", "color_scrollbase", "color_scrollthumb"};
				for (uint8_t color_i = 0; color_i < MU_COLOR_MAX; color_i++) {
					mu_style_e = cJSON_GetObjectItem(mu_style_json, color_json_names[color_i]);
					if (mu_style_e) {
						cJSON *mu_style_color;
						mu_style_color = cJSON_GetObjectItem(mu_style_e, "r");
						if (mu_style_color)
							obk_gui_style.colors[color_i].r = cJSON_GetNumberValue(mu_style_color);
						mu_style_color = cJSON_GetObjectItem(mu_style_e, "g");
						if (mu_style_color)
							obk_gui_style.colors[color_i].g = cJSON_GetNumberValue(mu_style_color);
						mu_style_color = cJSON_GetObjectItem(mu_style_e, "b");
						if (mu_style_color)
							obk_gui_style.colors[color_i].b = cJSON_GetNumberValue(mu_style_color);
						mu_style_color = cJSON_GetObjectItem(mu_style_e, "a");
						if (mu_style_color)
							obk_gui_style.colors[color_i].a = cJSON_GetNumberValue(mu_style_color);				
					}
				}
			}
			/*gui*/
			obk_stree_init(&MicroUI.mu_process_tree);			
			cJSON *mu_gui_json = cJSON_GetObjectItem(mu_json, "gui");
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "JSON_Load: gui=%d[%d]", mu_gui_json, __LINE__);
			//rtos_delay_milliseconds(200);
			if (mu_gui_json) {
			
				mu_ProcessItem_t *mupi_Begin = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));
				mupi_Begin->func_data = NULL;
				mupi_Begin->processfunc = MicroUI_ProcessBegin;
				
				obk_stree_init(&mupi_Begin->node);
				obk_stree_append(&MicroUI.mu_process_tree, &mupi_Begin->node);
				
				obk_stree_t *_node = &mupi_Begin->node;
				MicroUI_LoadItems(mu_gui_json, &_node, 0);
				
				mu_ProcessItem_t *mupi_End = (mu_ProcessItem_t *)os_malloc(sizeof(mu_ProcessItem_t));
				mupi_End->func_data = NULL;
				mupi_End->processfunc = MicroUI_ProcessEnd;
				
				obk_stree_init(&mupi_End->node);
				obk_stree_append(&MicroUI.mu_process_tree, &mupi_End->node);
				
			}			
			
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: json success");			
			cJSON_Delete(mu_json);
			return 0;
		} else {
			addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI: json parser error");
			return -1;	
		}
}