
#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
/*
An ability to evaluate a conditional string.
For use in conjuction with command aliases.

Example usage 1:
	alias mybri backlog led_dimmer 100; led_enableAll
	alias mydrk backlog led_dimmer 10; led_enableAll
	if MQTTOn then mybri else mydrk

	if $CH6<5 then mybri else mydrk

Example usage 2:
	if MQTTOn then "backlog led_dimmer 100; led_enableAll" else "backlog led_dimmer 10; led_enableAll"

	if $CH6<5 then "backlog led_dimmer 100; led_enableAll" else "backlog led_dimmer 10; led_enableAll"
*/

typedef struct sOperator_s {
	const char *txt;
	byte len;
} sOperator_t;

typedef enum {
	OP_EQUAL,
	OP_EQUAL_OR_GREATER,
	OP_EQUAL_OR_LESS,
	OP_NOT_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_AND,
	OP_OR,
} opCode_t;

static sOperator_t g_operators[] = {
	{ "==", 2 },
	{ ">=", 2 },
	{ "<=", 2 },
	{ "!=", 2 },
	{ ">", 1 },
	{ "<", 1 },
	{ "&&", 2 },
	{ "||", 2 },
};
static int g_numOperators = sizeof(g_operators)/sizeof(g_operators[0]);

const char *CMD_FindOperator(const char *s, const char *stop, byte *oCode) {
	int o = 0;

	while(s[0] && s[1] && (s < stop || stop == 0)) {
		for(o = 0; o < g_numOperators; o++) {
			if(!strncmp(s,g_operators[o].txt,g_operators[o].len)) {
				*oCode = o;
				return s;
			}
		}
		s++;
	}
	return 0;
}
bool strCompareBound(const char *s, const char *templ, const char *stopper, int bAllowWildCard) {
	while(true) {
		// template ended and reached stopper
		if(s == stopper && *templ == 0) {
			return true;
		}
		// template ended and reached end of string
		if(*s == 0 && *templ == 0) {
			return true;
		}
		// reached end of string but template still has smth
		if(*s == 0)
		{
			return false;
		}
		// are the chars the same?
		if(bAllowWildCard && *templ == '*') {

		} else {
            char c1 = tolower((unsigned char)*s);
            char c2 = tolower((unsigned char)*templ);
			if(c1 != c2) {
				return false;
			}
		}
		s++;
		templ++;
	}
	return false;
}
int CMD_EvaluateCondition(const char *s, const char *stop) {
	byte opCode;
	const char *op;
	int a, b, c;

	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateCondition: will run '%s'",s);

	op = CMD_FindOperator(s, stop, &opCode);
	if(op) {
		const char *p2;
	
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateCondition: operator %i",opCode);

		// first token block begins at 's' and ends at 'op'
		// second token block begins at 'p2' and ends at NULL
		p2 = op + g_operators[opCode].len;

		a = CMD_EvaluateCondition(s, op);
		b = CMD_EvaluateCondition(p2, 0);

		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateCondition: a = %i, b = %i", a, b);
		switch(opCode)
		{
		case OP_EQUAL:
			c = a == b;
			break;
		case OP_EQUAL_OR_GREATER:
			c = a >= b;
			break;
		case OP_EQUAL_OR_LESS:
			c = a <= b;
			break;
		case OP_NOT_EQUAL:
			c = a != b;
			break;
		case OP_GREATER:
			c = a > b;
			break;
		case OP_LESS:
			c = a < b;
			break;
		case OP_AND:
			c = a && b;
			break;
		case OP_OR:
			c = a || b;
			break;
		default:
			c = 0;
			break;
		}
		return c;
	}
	if(s[0] == '!') {
		return !CMD_EvaluateCondition(s+1,0);
	}
	if(strCompareBound(s,"MQTTOn", stop, false)) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateCondition: MQTTOn");
		return Main_HasMQTTConnected();
	}
	if(strCompareBound(s,"$CH*", stop, 1) || strCompareBound(s,"$CH**", stop, 1)) {
		c = atoi(s+3);
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateCondition: channel value of idx %i",c);
		return CHANNEL_Get(c);
	}
	return atoi(s);
}

// if MQTTOnline then "qq" else "qq"
int CMD_If(const void *context, const char *cmd, const char *args, int cmdFlags){
	const char *cmdA;
	const char *cmdB;
	const char *condition;
	//char buffer[256];
	int value;

	if(args==0||*args==0) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: command require at least 3 args");
		return 1;
	}
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if(Tokenizer_GetArgsCount() < 3) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: command require at least 3 args, you gave %i",Tokenizer_GetArgsCount());
		return 1;
	}
	condition = Tokenizer_GetArg(0);
	if(stricmp(Tokenizer_GetArg(1),"then")) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: second argument always must be 'then', but it's '%s'",Tokenizer_GetArg(1));
		return 1;
	}
	if(Tokenizer_GetArgsCount() >= 5) {
		cmdA = Tokenizer_GetArg(2);
		if(stricmp(Tokenizer_GetArg(3),"else")) {
			ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: fourth argument always must be 'else', but it's '%s'",Tokenizer_GetArg(3));
			return 1;
		}
		cmdB = Tokenizer_GetArg(4);
	} else {
		cmdA = Tokenizer_GetArgFrom(2);
		cmdB = 0;
	}

	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: cmdA is '%s'",cmdA);
	if(cmdB) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: cmdB is '%s'",cmdB);
	}
	ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: condition is '%s'",condition);

	value = CMD_EvaluateCondition(condition, 0);

	// This buffer is here because we may need to exec commands recursively
	// and the Tokenizer_ etc is global?
	//if(value)
	//	strcpy_safe(buffer,cmdA);
	//else
	//	strcpy_safe(buffer,cmdB);
	//CMD_ExecuteCommand(buffer,0);
	if(value)
		CMD_ExecuteCommand(cmdA,0);
	else {
		if(cmdB) {
			CMD_ExecuteCommand(cmdB,0);
		}
	}

	return 1;
}

