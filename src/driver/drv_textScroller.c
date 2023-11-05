

#include "../new_common.h"
#include "../new_cfg.h"
#include "../quicktick.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "drv_local.h"

int g_maxScrollingText = 128;
int g_stepTimeMS = 500;
int g_stepCurMS = 0;
int g_curOfs;
int g_len;
char *g_buffer;

void TS_RunQuickTick() {
	g_stepCurMS += g_deltaTimeMS;
	if (g_stepCurMS > g_stepTimeMS) {
		g_stepCurMS = 0;

		int screenSize = 8;
#ifdef ENABLE_DRIVER_PT6523
		if (DRV_IsRunning("PT6523")) {
			screenSize = 8;
			PT6523_ClearString();
			if (g_curOfs < 0) {
				PT6523_DrawString(g_buffer,-g_curOfs);
			}
			else {
				PT6523_DrawString(g_buffer + g_curOfs,0);
			}
			PT6523_Refresh();
		}
#endif
#ifdef ENABLE_DRIVER_MAX72XX
		if (DRV_IsRunning("MAX72XX")) {
			// TODO 
			// TODO screenSize = qqq;
		}
#endif
#ifdef ENABLE_DRIVER_HT16K33
		if (DRV_IsRunning("HT16K33")) {
			// TODO 
			// TODO screenSize = qqq;
		}
#endif
		g_curOfs++;
		if (g_curOfs + 1 >= g_len) {
			g_curOfs = -screenSize;
		}
	}
}
void TS_PrintStringAt(const char *s, int ofs, int maxLen) {
	while (*s && maxLen) {
		g_buffer[ofs] = *s;
		ofs++;
		maxLen--;
		s++;
	}
	g_len = strlen(g_buffer);
}
void TS_SetText(const char *s) {
	memset(g_buffer, 0, g_maxScrollingText);
	strcpy(g_buffer, s);
	g_len = strlen(g_buffer);
}
static commandResult_t CMD_TS_Clear(const void *context, const char *cmd, const char *args, int flags) {
	TS_SetText("");


	return CMD_RES_OK;
}
// TS_Print [StartOfs] [MaxLenOr0] [StringText] [optionalBClampWithZeroesForClock]
// setChannel 10 12
// TS_Print 0 2 $CH10 1
// TS_Print 0 9999 "2023-10-29 15 54"
static commandResult_t CMD_TS_Print(const void *context, const char *cmd, const char *args, int flags) {
	int ofs;
	int maxLen;
	const char *s;
	int sLen;
	int iPadZeroes;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	if (Tokenizer_GetArgsCount() <= 2) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	ofs = Tokenizer_GetArgInteger(0);
	maxLen = Tokenizer_GetArgInteger(1);
	s = Tokenizer_GetArg(2);


	if (maxLen <= 0) {
		maxLen = 999;
	}
	if (Tokenizer_GetArgInteger(3)) {
		sLen = strlen(s);
		if (sLen < maxLen) {
			iPadZeroes = maxLen - sLen;
			TS_PrintStringAt("00000", ofs, iPadZeroes);
			ofs += iPadZeroes;
			maxLen -= iPadZeroes;
		}
	}
	TS_PrintStringAt(s, ofs, maxLen);

	return CMD_RES_OK;
}
void TS_Init() {
	g_buffer = malloc(g_maxScrollingText);
	TS_SetText("THIS IS A SCROLLING TEXT 123456789");

	//cmddetail:{"name":"TS_Clear","args":"",
	//cmddetail:"descr":"Clears the text scroller buffer",
	//cmddetail:"fn":"NULL);","file":"driver/drv_textScroller.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TS_Clear", CMD_TS_Clear, NULL);
	//cmddetail:{"name":"TS_Print","args":"[StartOfs] [MaxLenOr0] [StringText] [optionalBClampWithZeroesForClock]",
	//cmddetail:"descr":"Prints a text to the text scroller buffer",
	//cmddetail:"fn":"NULL);","file":"driver/drv_textScroller.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TS_Print", CMD_TS_Print, NULL);
}

