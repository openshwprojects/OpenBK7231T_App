
#include "../new_common.h"
#include "../httpserver/new_http.h"
#include "../new_pins.h"
#include "../new_cfg.h"

#define MAX_CMD_LEN 512
#define MAX_ARGS 32

static char g_buffer[MAX_CMD_LEN];
static const char *g_args[MAX_ARGS];
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
const char *Tokenizer_GetArg(int i) {
	return g_args[i];
}
const char *Tokenizer_GetArgFrom(int i) {
	return g_argsFrom[i];
}
int Tokenizer_GetArgInteger(int i) {
	const char *s;
	s = g_args[i];
	if(s[0] == '0' && s[1] == 'x') {
		int ret;
		sscanf(s, "%x", &ret);
		return ret;
	}
	return atoi(s);
}
void Tokenizer_TokenizeString(const char *s) {
	char *p;

	while(isWhiteSpace(*s)) {
		s++;
	}

	g_numArgs = 0;
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
			if((p[1] != 0)) {
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
		p++;
	}


}