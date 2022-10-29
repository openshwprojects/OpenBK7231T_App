
#include "../new_common.h"
#include "cmd_public.h"
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

bool isWhiteSpace(char ch) {
	if(ch == ' ')
		return true;
	if(ch == '\t')
		return true;
	if(ch == '\n')
		return true;
	if(ch == '\r')
		return true;
	return false;
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

	if(s[0] == '$' && s[1] == 'C' && s[2] == 'H') {
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
	int channelIndex;
	const char *s;
	s = g_args[i];
	if(s[0] == '0' && s[1] == 'x') {
		int ret;
		sscanf(s, "%x", &ret);
		return ret;
	}
	if(s[0] == '$') {
		// constant
		if(s[1] == 'C' && s[2] == 'H') {
			channelIndex = atoi(s+3);
			return CHANNEL_Get(channelIndex);
		}
	}
	return atoi(s);
}
void Tokenizer_TokenizeString(const char *s, int bAllowQuotedStrings) {
	char *p;

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

	strcpy(g_buffer,s);
	p = g_buffer;
	g_args[g_numArgs] = p;
	g_argsFrom[g_numArgs] = (s+(p-g_buffer));
	g_numArgs++;
	while(*p != 0) {
		if(isWhiteSpace(*p)) {
			*p = 0;
			if(p[1] != 0 && isWhiteSpace(p[1])==false) {
				// we need to rewrite this function
				if(bAllowQuotedStrings && p[1] == '"') { 
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
		if(bAllowQuotedStrings && *p == '"') {
quote:
			*p = 0;
			g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
			p++;
			g_args[g_numArgs] = p;
			g_numArgs++;
			while(*p != 0) {
				if(*p == '"') {
					*p = 0;
					p++;
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
