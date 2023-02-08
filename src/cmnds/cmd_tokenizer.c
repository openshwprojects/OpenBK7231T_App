
#include "../new_common.h"
#include "cmd_public.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"

#define MAX_CMD_LEN 512
#define MAX_ARGS 32

static char g_buffer[MAX_CMD_LEN];
static const char *g_args[MAX_ARGS];
static char g_argsExpanded[MAX_ARGS][8];
static const char *g_argsFrom[MAX_ARGS];
static int g_numArgs = 0;
static int tok_flags = 0;

#define g_bAllowQuotes (tok_flags&TOKENIZER_ALLOW_QUOTES)
#define g_bAllowExpand (!(tok_flags&TOKENIZER_DONT_EXPAND))

bool isWhiteSpace(char ch) {
	if(ch == ' ')
		return true;
	if(ch == '\t')
		return true;
	if(ch == '\n')
		return true;
	if(ch == '\r')
		return true;
	// fix for https://www.elektroda.com/rtvforum/viewtopic.php?p=20420012#20420012
	if (ch == 0xA0)
		return true;
	return false;
}
bool Tokenizer_CheckArgsCountAndPrintWarning(const char *cmdString, int reqCount) {
	if (g_numArgs >= reqCount)
		return false;
	ADDLOG_ERROR(LOG_FEATURE_CMD, "Cant run '%s', expected at least %i args (given %i)", cmdString, reqCount, g_numArgs);
	return true;
}
int Tokenizer_GetArgsCount() {
	return g_numArgs;
}
bool Tokenizer_IsArgInteger(int i) {
	if(i >= g_numArgs)
		return false;
	return strIsInteger(g_args[i]);
}
const char *Tokenizer_GetArg(int i) {
	const char *s;

	if(i >= g_numArgs)
		return 0;

	s = g_args[i];

	if(g_bAllowExpand && s[0] == '$' && s[1] == 'C' && s[2] == 'H') {
		int channelIndex;
		int value;

		channelIndex = atoi(s+3);
		value = CHANNEL_Get(channelIndex);
		
		sprintf(g_argsExpanded[i],"%i",value);

		return g_argsExpanded[i];
	}

	return g_args[i];
}
const char *Tokenizer_GetArgFrom(int i) {
	return g_argsFrom[i];
}
int Tokenizer_GetArgIntegerRange(int i, int rangeMin, int rangeMax) {
	int ret = Tokenizer_GetArgInteger(i);
	if(ret < rangeMin) {
		ret = rangeMin;
		ADDLOG_ERROR(LOG_FEATURE_CMD, "Argument %i (val=%i) was out of range [%i,%i], clamped",i,ret,rangeMax,rangeMin);
	}
	if(ret > rangeMax) {
		ret = rangeMax;
		ADDLOG_ERROR(LOG_FEATURE_CMD, "Argument %i (val=%i) was out of range [%i,%i], clamped",i,ret,rangeMax,rangeMin);
	}
	return ret;
}
int Tokenizer_GetArgInteger(int i) {
	const char *s;
	int ret;

	s = g_args[i];
	if (s == 0)
		return 0;
	if(s[0] == '0' && s[1] == 'x') {
		sscanf(s, "%x", &ret);
		return ret;
	}
#if (!PLATFORM_BEKEN && !WINDOWS)
	if(g_bAllowExpand && s[0] == '$') {
		// constant
		int channelIndex;
		if(s[1] == 'C' && s[2] == 'H') {
			channelIndex = atoi(s+3);
			return CHANNEL_Get(channelIndex);
		}
	}
#else
	// TODO: make sure it's ok
	// It is supposed to handle expressions like:
	// - 5*10
	// - $CH5+$CH11
	// - $CH8*10
	if(g_bAllowExpand) {
		ret = CMD_EvaluateExpression(s,0);
		return ret;
	}
#endif
	return atoi(s);
}
float Tokenizer_GetArgFloat(int i) {
#if !PLATFORM_BEKEN
	int channelIndex;
#endif
	const char *s;
	s = g_args[i];
#if (!PLATFORM_BEKEN && !WINDOWS)
	if(g_bAllowExpand && s[0] == '$') {
		// constant
		if(s[1] == 'C' && s[2] == 'H') {
			channelIndex = atoi(s+3);
			return CHANNEL_Get(channelIndex);
		}
	}
#else
	// TODO: make sure it's ok
	// It is supposed to handle expressions like:
	// - 5*10
	// - $CH5+$CH11
	// - $CH8*10
	if(g_bAllowExpand) {
		return CMD_EvaluateExpression(s,0);
	}
#endif
	return atof(s);
}
void Tokenizer_TokenizeString(const char *s, int flags) {
	char *p;

	tok_flags = flags;
	g_numArgs = 0;

	if(s == 0) {
		return;
	}

	while(isWhiteSpace(*s)) {
		s++;
	}

	if(*s == 0) {
		return;
	}

	// not really needed, but nice for testing
	memset(g_args, 0, sizeof(g_args));
	memset(g_argsFrom, 0, sizeof(g_argsFrom));

	if (flags & TOKENIZER_ALTERNATE_EXPAND_AT_START) {
		CMD_ExpandConstantsWithinString(s, g_buffer, sizeof(g_buffer));
	} else {
		strcpy_safe(g_buffer, s, sizeof(g_buffer));
	}
	p = g_buffer;
	// we need to rewrite this function and check it well with unit tests
	if (*p == '"') {
		goto quote;
	}
	g_args[g_numArgs] = p;
	g_argsFrom[g_numArgs] = (s+(p-g_buffer));
	g_numArgs++;
	while(*p != 0) {
		if(isWhiteSpace(*p)) {
			*p = 0;
			if(p[1] != 0 && isWhiteSpace(p[1])==false) {
				// we need to rewrite this function and check it well with unit tests
				if(g_bAllowQuotes && p[1] == '"') { 
					p++;
					goto quote;
				}
				g_args[g_numArgs] = p+1;
				g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
				g_numArgs++;
			}
		}
		if(*p == ',') {
			*p = 0;
			g_args[g_numArgs] = p+1;
			g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
			g_numArgs++;
		}
		if(g_bAllowQuotes && *p == '"') {
quote:
			*p = 0;
			g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
			p++;
			g_args[g_numArgs] = p;
			g_numArgs++;
			while(*p != 0) {
				if(*p == '"') {
					*p = 0;
					break;
				}
				p++;
			}
		}
		if(g_numArgs>=MAX_ARGS) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Too many args, skipped all after 32nd.");
			break;
		}
		p++;
	}


}
