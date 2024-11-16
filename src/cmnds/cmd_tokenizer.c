
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
static char *g_args[MAX_ARGS];
static char g_argsExpanded[MAX_ARGS][40];
static const char *g_argsFrom[MAX_ARGS];
static int g_numArgs = 0;
static int tok_flags = 0;

#define g_bAllowQuotes (tok_flags&TOKENIZER_ALLOW_QUOTES)
#define g_bAllowExpand (!(tok_flags&TOKENIZER_DONT_EXPAND))

int str_to_ip(const char *s, byte *ip) {
#if PLATFORM_W600 || PLATFORM_LN882H
	//seems like sscanf in W600 does not support %hhu and uses it as %u, thus overwriting more memory, use temp array for it
	// same for LN882h: %hhu isn't recognised, so we have to use %u for IP_STRING_FORMAT, which will lead to problems in sscanf, too
	int tmp_ip[4];
	int res; 
	res = sscanf(s, IP_STRING_FORMAT, &tmp_ip[0], &tmp_ip[1], &tmp_ip[2], &tmp_ip[3]);
	ip[0]=tmp_ip[0];
	ip[1]=tmp_ip[1];
	ip[2]=tmp_ip[2];
	ip[3]=tmp_ip[3];
	return res;
#else
	return sscanf(s, IP_STRING_FORMAT, &ip[0], &ip[1], &ip[2], &ip[3]);
#endif
}
void convert_IP_to_string(char *o, unsigned char *ip) {
	sprintf(o, IP_STRING_FORMAT, ip[0], ip[1], ip[2], ip[3]);
}
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
	if (*g_args[i] == '$') {
		return true;
	}
	return strIsInteger(g_args[i]);
}
const char *Tokenizer_GetArgExpanding(int i) {
	const char *s;
	char tokLine[sizeof(g_argsExpanded[i])];
	char Templine[sizeof(g_argsExpanded[i])];
	char convert[10];

	if (i >= g_numArgs)
		return 0;

	s = g_args[i];

	//séparators for strtok to detect constants
	const char * separators = "${}";
	//pointer for strstr function
	char *ptrConst;

	//copy input string before manipulations
	strcpy_safe(g_argsExpanded[i], s, sizeof(g_argsExpanded[i]));
	strcpy_safe(tokLine, s, sizeof(tokLine));

	//start strtok
	char *strToken = strtok(tokLine, separators);
	while (strToken != NULL) {
		//build tconst with ${<token>} and try find it
		char tconst[20] = "${";
		strcat(tconst, strToken);
		strcat(tconst, "}");
		ptrConst = strstr(g_argsExpanded[i], tconst);
		if (ptrConst == NULL) {
			// we didn't find ${<token>} so we try with $<token>
			strcpy(tconst, "$");
			strcat(tconst, strToken);
			ptrConst = strstr(g_argsExpanded[i], tconst);
		}
		// if we found ${<token>} or $<token> it means we found a constant
		if (ptrConst != NULL) {
			//put 0 on the start of the constant to copy the left part of the input string
			ptrConst[0] = 0;
			strcpy_safe(Templine, g_argsExpanded[i], sizeof(Templine));
			//analyse the constant found to replace it with it's value/string and concat it with the left part of the input string
			if (!strcmp(tconst, "${IP}") || !strcmp(tconst, "$IP")) {
				strcat_safe(Templine, HAL_GetMyIPString(), sizeof(Templine));
			}
			else if (!strcmp(tconst, "${ShortName}") || !strcmp(tconst, "$ShortName")) {
				strcat_safe(Templine, CFG_GetShortDeviceName(), sizeof(Templine));
			}
			else if (!strcmp(tconst, "${Name}") || !strcmp(tconst, "$Name")) {
				strcat_safe(Templine, CFG_GetDeviceName(), sizeof(Templine));
			}
			else {
				float f;
				int iValue;
				CMD_ExpandConstant(tconst, 0, &f);
				iValue = f;
				sprintf(convert, "%i", iValue);
				strcat_safe(Templine, convert, sizeof(Templine));
			}
			//concat with the right part, after the constant
			strcat_safe(Templine, ptrConst + strlen(tconst), sizeof(Templine));
			//update the input string with the replaced constant
			strcpy_safe(g_argsExpanded[i], Templine, sizeof(g_argsExpanded[i]));
		}
		//look for next token
		strToken = strtok(NULL, separators);

	}

	return g_argsExpanded[i];

}
const char *Tokenizer_GetArg(int i) {
	const char *s;

	if (i >= g_numArgs)
		return 0;

	if (g_argsExpanded[i][0] != 0) {
		return g_argsExpanded[i];
	}

	s = g_args[i];

#if 0
	if (g_bAllowExpand && s[0] == '$' && s[1] == 'C' && s[2] == 'H') {
		int channelIndex;
		int value;

		channelIndex = atoi(s + 3);
		value = CHANNEL_Get(channelIndex);

		sprintf(g_argsExpanded[i], "%i", value);

		return g_argsExpanded[i];
	}
#else
	if (g_bAllowExpand && (tok_flags & TOKENIZER_ALTERNATE_EXPAND_AT_START)) {
		CMD_ExpandConstantsWithinString(s, g_argsExpanded[i], sizeof(g_argsExpanded[i]));
		return g_argsExpanded[i];
	}
	else if (g_bAllowExpand && s[0] == '$') {
		// quick hack for str expansion here, may do it in a better way later
		if (!strcmp(s + 1, "IP")) {
			strcpy_safe(g_argsExpanded[i], HAL_GetMyIPString(), sizeof(g_argsExpanded[i]));
		}
		else if (!strcmp(s + 1, "ShortName")) {
			strcpy_safe(g_argsExpanded[i], CFG_GetShortDeviceName(), sizeof(g_argsExpanded[i]));
		}
		else if (!strcmp(s + 1, "Name")) {
			strcpy_safe(g_argsExpanded[i], CFG_GetDeviceName(), sizeof(g_argsExpanded[i]));
		}
		else {
			float f;
			int iValue;
			CMD_ExpandConstant(s, 0, &f);
			iValue = f;
			sprintf(g_argsExpanded[i], "%i", iValue);
		}
		return g_argsExpanded[i];
	}

#endif

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
int Tokenizer_GetArgIntegerDefault(int i, int def) {
	int r;

	if (g_numArgs <= i) {
		return def;
	}
	r = Tokenizer_GetArgInteger(i);

	return r;
}
float Tokenizer_GetArgFloatDefault(int i, float def) {
	float r;

	if (g_numArgs <= i) {
		return def;
	}
	r = Tokenizer_GetArgFloat(i);

	return r;
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
#if !ENABLE_EXPAND_CONSTANT
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
#if !ENABLE_EXPAND_CONSTANT
	int channelIndex;
#endif
	const char *s;
	s = g_args[i];
#if !ENABLE_EXPAND_CONSTANT
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
void expandQuotes(char* str) {
	size_t len = strlen(str);
	size_t readIndex = 0;
	size_t writeIndex = 0;

	while (readIndex < len) {
		if (str[readIndex] == '\\' && str[readIndex + 1] == '\"') {
			str[writeIndex++] = '\"';
			readIndex++;
		}
		else {
			str[writeIndex++] = str[readIndex];
		}

		readIndex++;
	}

	str[writeIndex] = 0;
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
	memset(g_args, 0, sizeof(g_args)); // backing buffer is g_buffer, which is mutated where spaces on arg boundaries are set to null char
	memset(g_argsFrom, 0, sizeof(g_argsFrom)); // backing buffer is s, original unmutated string
	memset(g_argsExpanded, 0, sizeof(g_argsExpanded));

	if (flags & TOKENIZER_EXPAND_EARLY) {
		CMD_ExpandConstantsWithinString(s, g_buffer, sizeof(g_buffer) - 1);
	}
	else {
		strcpy_safe(g_buffer, s, sizeof(g_buffer));
	}

	if (flags & TOKENIZER_FORCE_SINGLE_ARGUMENT_MODE) {
		g_args[g_numArgs] = g_buffer;
		g_argsFrom[g_numArgs] = g_buffer;
		// some hack, but we fored to have only have one arg, so we can extend the string over array bondaries.
		// probably better: introducing an union containing g_argsExpanded[][] and one sole string in the same memory area ...
		CMD_ExpandConstantsWithinString(g_buffer,g_argsExpanded,sizeof(g_argsExpanded)-1);
		g_numArgs = 1;
		return;
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
		//if(*p == ',') {
		//	*p = 0;
		//	g_args[g_numArgs] = p+1;
		//	g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
		//	g_numArgs++;
		//}
		if(g_bAllowQuotes && *p == '"' && ((p <= g_buffer) || isWhiteSpace(p[-1]))) {
quote:
			*p = 0;
			g_argsFrom[g_numArgs] = (s+((p+1)-g_buffer));
			p++;
			g_args[g_numArgs] = p;
			g_numArgs++;
			while(*p != 0) {
				if (flags & TOKENIZER_ALLOW_ESCAPING_QUOTATIONS) {
					if (*p == '"' && p[-1] != '\\') {
						*p = 0;
						break;
					}
				}
				else {
					if (*p == '"') {
						*p = 0;
						break;
					}
				}
				p++;
			}
			if (flags & TOKENIZER_ALLOW_ESCAPING_QUOTATIONS) {
				expandQuotes(g_args[g_numArgs - 1]);
			}
		}
		if(g_numArgs>=MAX_ARGS) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "Too many args, skipped all after 32nd.");
			break;
		}
		p++;
	}


}
