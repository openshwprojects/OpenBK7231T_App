#include "../new_common.h"
#include "cmd_local.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../driver/drv_public.h"
#include "../driver/drv_battery.h"
#include "../driver/drv_ntp.h"
#include <ctype.h> // isspace

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

#define ADDLOG_IF_MATHEXP_DBG(x, ...)



typedef struct sOperator_s {
	const char *txt;
	byte len;
	byte prio;
} sOperator_t;

typedef enum {
	OP_GREATER,
	OP_LESS,
	OP_EQUAL,
	OP_EQUAL_OR_GREATER,
	OP_EQUAL_OR_LESS,
	OP_NOT_EQUAL,
	OP_AND,
	OP_OR,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MODULO,
} opCode_t;

static sOperator_t g_operators[] = {
	{ ">", 1, 10 },
	{ "<", 1, 10 },
	{ "==", 2, 10 },
	{ ">=", 2, 10 },
	{ "<=", 2, 10 },
	{ "!=", 2, 10 },
	{ "&&", 2, 12 },
	{ "||", 2, 12 },
	{ "+", 1, 6 },
	{ "-", 1, 6 },
	{ "*", 1, 4 },
	{ "/", 1, 4 },
	{ "%", 1, 4 },
};
static int g_numOperators = sizeof(g_operators) / sizeof(g_operators[0]);

const char *CMD_FindOperator(const char *s, const char *stop, byte *oCode) {
	byte bestPriority;
	const char *retVal;
	int o = 0;
	const char *signSkip = 0;

	if (*s == 0)
		return 0;
	// special case for number like -12 at the start of the string
	if ((s[0] == '-') && (isdigit((int)s[1]))) {
		s++;// "-" is a part of the digit, not an operator
	}
	if (*s == 0)
		return 0;

	retVal = 0;
	bestPriority = 0;
	int level = 0;
	while (s[0] && s[1] && (s < stop || stop == 0)) {
		if (*s == '(') {
			level++;
		} else if (*s == ')') {
			level--;
		} else if (s != signSkip) {
			for (o = 0; o < g_numOperators; o++) {
				if (!strncmp(s, g_operators[o].txt, g_operators[o].len)) {
					int prio = g_operators[o].prio;
					prio -= level * 1000;
					if (prio >= bestPriority) {
						bestPriority = prio;
						retVal = s;
						*oCode = o;
						signSkip = s + g_operators[o].len;
						if (*signSkip != '-') {
							signSkip = 0;
						}
					}
				}
			}
		} else {
			signSkip = 0;
		}
		s++;
	}
	return retVal;
}
const char *strCompareBound(const char *s, const char *templ, const char *stopper, int bAllowWildCard) {
	const char *s_start;

	s_start = s;

	while (true) {
		if (stopper == 0) {
			// allow early end
			// template ended and reached stopper
			if (*templ == 0) {
				return s;
			}
		}
		else {
			// template ended and reached stopper
			if (s == stopper && *templ == 0) {
				return s;
			}
		}
		// template ended and reached end of string
		if (*s == 0 && *templ == 0) {
			return s;
		}
		// reached end of string but template still has smth
		if (*s == 0)
		{
			return 0;
		}
		// are the chars the same?
		if (bAllowWildCard && *templ == '*') {
			if (isdigit((int)(*s))) {

			}
			else {
				return 0;
			}
		}
		else {
			char c1 = tolower((unsigned char)*s);
			char c2 = tolower((unsigned char)*templ);
			if (c1 != c2) {
				return 0;
			}
		}
		s++;
		templ++;
	}
	return 0;
}
char *g_expDebugBuffer = 0;
#define EXPRESSION_DEBUG_BUFFER_SIZE 128
typedef struct {
	const char *constantName;
	float(*getValue)(const char *s);
} constant_t;

float getMQTTOn(const char *s) {
	return Main_HasMQTTConnected();
}

float getChannelValue(const char *s) {
	int idx = atoi(s + 3);
	return CHANNEL_Get(idx);
}
float getFlagValue(const char *s) {
	int idx = atoi(s + 5);
	return CFG_HasFlag(idx);
}

float getLedDimmer(const char *s) {
	return LED_GetDimmer();
}

float getLedEnableAll(const char *s) {
	return LED_GetEnableAll();
}

float getLedHue(const char *s) {
	return LED_GetHue();
}

float getLedRed(const char *s) {
	return LED_GetRed255();
}

float getLedGreen(const char *s) {
	return LED_GetGreen255();
}

float getLedBlue(const char *s) {
	return LED_GetBlue255();
}

float getLedSaturation(const char *s) {
	return LED_GetSaturation();
}

float getLedTemperature(const char *s) {
	return LED_GetTemperature();
}

float getActiveRepeatingEvents(const char *s) {
	return RepeatingEvents_GetActiveCount();
}

#ifdef ENABLE_DRIVER_BL0937

float getVoltage(const char *s) {
	return DRV_GetReading(OBK_VOLTAGE);
}
#ifdef ENABLE_DRIVER_BATTERY
float getBatteryVoltage(const char *s) {
	return Battery_lastreading(OBK_BATT_VOLTAGE);
}
float getBatteryLevel(const char *s) {
	return Battery_lastreading(OBK_BATT_LEVEL);
}
#endif

float getCurrent(const char *s) {
	return DRV_GetReading(OBK_CURRENT);
}

float getPower(const char *s) {
	return DRV_GetReading(OBK_POWER);
}
float getEnergy(const char *s) {
	return DRV_GetReading(OBK_CONSUMPTION_TOTAL);
}
float getYesterday(const char *s) {
	return DRV_GetReading(OBK_CONSUMPTION_YESTERDAY);
}
float getToday(const char *s) {
	return DRV_GetReading(OBK_CONSUMPTION_TODAY);
}



float getNTPOn(const char *s) {
	return NTP_IsTimeSynced();
}

#endif

float getRand(const char *s) {
	return rand();
}
// return float in [0,1] range
float getRand01(const char *s) {
	return ((float)rand()) / (float)(RAND_MAX);
}
float getFailedBoots(const char *s) {
	return g_bootFailures;
}
float getUpTime(const char *s) {
	return g_secondsElapsed;
}
float getWeekDay(const char *s) {
	return NTP_GetWeekDay();
}
float getMinute(const char *s) {
	return NTP_GetMinute();
}
float getHour(const char *s) {
	return NTP_GetHour();
}
float getSecond(const char *s) {
	return NTP_GetSecond();
}
float getYear(const char *s) {
	return NTP_GetYear();
}
float getMonth(const char *s) {
	return NTP_GetMonth();
}
float getMDay(const char *s) {
	return NTP_GetMDay();
}

#if ENABLE_NTP_SUNRISE_SUNSET

float getSunrise(const char *s) {
	return NTP_GetSunrise();
}
float getSunset(const char *s) {
	return NTP_GetSunset();
}

#endif


float getRebootReason(const char *s) {
	return g_rebootReason;
}


float getInternalTemperature(const char *s) {
	return g_wifi_temperature;
}


const constant_t g_constants[] = {
	//cnstdetail:{"name":"MQTTOn",
	//cnstdetail:"title":"MQTTOn",
	//cnstdetail:"descr":"Legacy variable, without $ prefix. Returns 1 if MQTT is connected, otherwise 0.",
	//cnstdetail:"requires":""}
	{"MQTTOn", &getMQTTOn},
	//cnstdetail:{"name":"$MQTTOn",
	//cnstdetail:"title":"$MQTTOn",
	//cnstdetail:"descr":"Returns 1 if MQTT is connected, otherwise 0.",
	//cnstdetail:"requires":""}
	{"$MQTTOn", &getMQTTOn},
	//cnstdetail:{"name":"$CH***",
	//cnstdetail:"title":"$CH***",
	//cnstdetail:"descr":"Provides channel access, so you can do math expressions on channel values. $CH1 is channel 1, $CH20 is channel 20, $CH140 is channel 140, etc",
	//cnstdetail:"requires":""}
	{"$CH***", &getChannelValue},
	//cnstdetail:{"name":"$CH**",
	//cnstdetail:"title":"$CH**",
	//cnstdetail:"descr":"Provides channel access, as above.",
	//cnstdetail:"requires":""}
	{"$CH**", &getChannelValue},
	//cnstdetail:{"name":"$CH*",
	//cnstdetail:"title":"$CH*",
	//cnstdetail:"descr":"Provides channel access, as above.",
	//cnstdetail:"requires":""}
	{"$CH*", &getChannelValue},
	//cnstdetail:{"name":"$FLAG**",
	//cnstdetail:"title":"$FLAG**",
	//cnstdetail:"descr":"Provides flag access, as above.",
	//cnstdetail:"requires":""}
	{"$FLAG**", &getFlagValue},
	//cnstdetail:{"name":"$FLAG*",
	//cnstdetail:"title":"$FLAG*",
	//cnstdetail:"descr":"Provides flag access, as above.",
	//cnstdetail:"requires":""}
	{"$FLAG*", &getFlagValue},
	//cnstdetail:{"name":"$led_dimmer",
	//cnstdetail:"title":"$led_dimmer",
	//cnstdetail:"descr":"Current value of LED dimmer, 0-100 range",
	//cnstdetail:"requires":""}
	{"$led_dimmer", &getLedDimmer},
	//cnstdetail:{"name":"$led_enableAll",
	//cnstdetail:"title":"$led_enableAll",
	//cnstdetail:"descr":"Returns 1 if LED is enabled, otherwise 0.",
	//cnstdetail:"requires":""}
	{"$led_enableAll", &getLedEnableAll},
	//cnstdetail:{"name":"$led_hue",
	//cnstdetail:"title":"$led_hue",
	//cnstdetail:"descr":"Current LED Hue value",
	//cnstdetail:"requires":""}
	{"$led_hue", &getLedHue},
	//cnstdetail:{"name":"$led_red",
	//cnstdetail:"title":"$led_red",
	//cnstdetail:"descr":"Current LED red value",
	//cnstdetail:"requires":""}
	{"$led_red", &getLedRed},
	//cnstdetail:{"name":"$led_green",
	//cnstdetail:"title":"$led_green",
	//cnstdetail:"descr":"Current LED green value",
	//cnstdetail:"requires":""}
	{"$led_green", &getLedGreen},
	//cnstdetail:{"name":"$led_blue",
	//cnstdetail:"title":"$led_blue",
	//cnstdetail:"descr":"Current LED blue value",
	//cnstdetail:"requires":""}
	{"$led_blue", &getLedBlue},
	//cnstdetail:{"name":"$led_saturation",
	//cnstdetail:"title":"$led_saturation",
	//cnstdetail:"descr":"Current LED saturation value",
	//cnstdetail:"requires":""}
	{"$led_saturation", &getLedSaturation},
	//cnstdetail:{"name":"$led_temperature",
	//cnstdetail:"title":"$led_temperature",
	//cnstdetail:"descr":"Current LED temperature value",
	//cnstdetail:"requires":""}
	{"$led_temperature", &getLedTemperature},
	//cnstdetail:{"name":"$activeRepeatingEvents",
	//cnstdetail:"title":"$activeRepeatingEvents",
	//cnstdetail:"descr":"Current number of active repeating events",
	//cnstdetail:"requires":""}
	{"$activeRepeatingEvents", &getActiveRepeatingEvents},
#ifdef ENABLE_DRIVER_BL0937
	//cnstdetail:{"name":"$voltage",
	//cnstdetail:"title":"$voltage",
	//cnstdetail:"descr":"Current value of voltage from energy metering chip. You can use those variables to make, for example, a change handler that fires when voltage is above 245, etc.",
	//cnstdetail:"requires":""}
	{"$voltage", &getVoltage},
	//cnstdetail:{"name":"$current",
	//cnstdetail:"title":"$current",
	//cnstdetail:"descr":"Current value of current from energy metering chip",
	//cnstdetail:"requires":""}
	{"$current", &getCurrent},
	//cnstdetail:{"name":"$power",
	//cnstdetail:"title":"$power",
	//cnstdetail:"descr":"Current value of power from energy metering chip",
	//cnstdetail:"requires":""}
	{"$power", &getPower},
	//cnstdetail:{"name":"$energy",
	//cnstdetail:"title":"$energy",
	//cnstdetail:"descr":"Current value of energy counter from energy metering chip",
	//cnstdetail:"requires":""}
	{"$energy", &getEnergy},
	//cnstdetail:{"name":"$day",
	//cnstdetail:"title":"$day",
	//cnstdetail:"descr":"Current weekday from NTP",
	//cnstdetail:"requires":""}
#endif	//ENABLE_DRIVER_BL0937
#ifdef ENABLE_NTP
	{"$day", &getWeekDay},
	//cnstdetail:{"name":"$hour",
	//cnstdetail:"title":"$hour",
	//cnstdetail:"descr":"Current hour from NTP",
	//cnstdetail:"requires":""}
	{"$hour", &getHour},
	//cnstdetail:{"name":"$minute",
	//cnstdetail:"title":"$minute",
	//cnstdetail:"descr":"Current minute from NTP",
	//cnstdetail:"requires":""}
	{ "$minute", &getMinute },
	//cnstdetail:{"name":"$second",
	//cnstdetail:"title":"$second",
	//cnstdetail:"descr":"Current second from NTP",
	//cnstdetail:"requires":""}
	{ "$second", &getSecond },
	////cnstdetail:{"name":"$mday",
	////cnstdetail:"title":"$mday",
	////cnstdetail:"descr":"Current mday from NTP",
	////cnstdetail:"requires":""}
	{ "$mday", &getMDay },
	////cnstdetail:{"name":"$month",
	////cnstdetail:"title":"$month",
	////cnstdetail:"descr":"Current month from NTP",
	////cnstdetail:"requires":""}
	{ "$month", &getMonth },
	////cnstdetail:{"name":"$year",
	////cnstdetail:"title":"$year",
	////cnstdetail:"descr":"Current Year from NTP",
	////cnstdetail:"requires":""}
	{ "$year", &getYear },
	////cnstdetail:{"name":"$yesterday",
	////cnstdetail:"title":"$yesterday",
	////cnstdetail:"descr":"",
	////cnstdetail:"requires":""}
	{ "$yesterday", &getYesterday },
	////cnstdetail:{"name":"$today",
	////cnstdetail:"title":"$today",
	////cnstdetail:"descr":"",
	////cnstdetail:"requires":""}
	{ "$today", &getToday },
#if ENABLE_NTP_SUNRISE_SUNSET
	////cnstdetail:{"name":"$sunrise",
	////cnstdetail:"title":"$sunrise",
	////cnstdetail:"descr":"Next sunrise as a TimerSeconds from midnight",
	////cnstdetail:"requires":""}
	{ "$sunrise", &getSunrise },
	////cnstdetail:{"name":"$sunset",
	////cnstdetail:"title":"$sunset",
	////cnstdetail:"descr":"Next sunset as a TimerSeconds from midnight",
	////cnstdetail:"requires":""}
	{ "$sunset", &getSunset },
#endif
	//cnstdetail:{"name":"$NTPOn",
	//cnstdetail:"title":"$NTPOn",
	//cnstdetail:"descr":"Returns 1 if NTP is on and already synced (so device has correct time), otherwise 0.",
	//cnstdetail:"requires":""}
	{ "$NTPOn", &getNTPOn },
#endif	//ENABLE_NTP
#ifdef ENABLE_DRIVER_BATTERY
	//cnstdetail:{"name":"$batteryVoltage",
	//cnstdetail:"title":"$batteryVoltage",
	//cnstdetail:"descr":"Battery driver voltage",
	//cnstdetail:"requires":""}
	{ "$batteryVoltage", &getBatteryVoltage },
	//cnstdetail:{"name":"$batteryLevel",
	//cnstdetail:"title":"$batteryLevel",
	//cnstdetail:"descr":"Battery driver level",
	//cnstdetail:"requires":""}
	{ "$batteryLevel", &getBatteryLevel },
#endif	// ENABLE_DRIVER_BATTERY
	//cnstdetail:{"name":"$uptime",
	//cnstdetail:"title":"$uptime",
	//cnstdetail:"descr":"Time since reboot in seconds",
	//cnstdetail:"requires":""}
	{ "$uptime", &getUpTime },
	//cnstdetail:{"name":"$failedBoots",
	//cnstdetail:"title":"$failedBoots",
	//cnstdetail:"descr":"Get number of failed boots (too quick reboots). Remember that you can change the uptime required to mark boot as 'okay' in general/flags menu",
	//cnstdetail:"requires":""}
	{ "$failedBoots", &getFailedBoots },
	//cnstdetail:{"name":"$rand01",
	//cnstdetail:"title":"$rand01",
	//cnstdetail:"descr":"Random float between [0,1]",
	//cnstdetail:"requires":""}
	{ "$rand01", &getRand01 },
	//cnstdetail:{"name":"$rand",
	//cnstdetail:"title":"$rand",
	//cnstdetail:"descr":"Random unsigned value",
	//cnstdetail:"requires":""}
	{ "$rand", &getRand },
#ifdef PLATFORM_BEKEN
	//cnstdetail:{"name":"$rebootReason",
	//cnstdetail:"title":"$rebootReason",
	//cnstdetail:"descr":"Reboot reason",
	//cnstdetail:"requires":""}
	{ "$rebootReason", &getRebootReason },
#endif
	//cnstdetail:{"name":"$intTemp",
	//cnstdetail:"title":"$intTemp",
	//cnstdetail:"descr":"Internal temperature (of WiFi module sensor)",
	//cnstdetail:"requires":""}
	{ "$intTemp", &getInternalTemperature },
};

static int g_totalConstants = sizeof(g_constants) / sizeof(g_constants[0]);

// tries to expand a given string into a constant
// So, for $CH1 it will set out to given channel value
// For $led_dimmer it will set out to current led_dimmer value
// Etc etc
// Returns true if constant matches
// Returns false if no constants found
const char *CMD_ExpandConstant(const char *s, const char *stop, float *out) {
#if ENABLE_EXPAND_CONSTANT
	const constant_t *var;
	int i;
	var = g_constants;
	for (i = 0; i < g_totalConstants; i++, var++) {
		bool bAllowWildCard = strstr(var->constantName, "*") != 0;
		const char *ret = strCompareBound(s, var->constantName, stop, bAllowWildCard);
		if (ret) {
			*out = var->getValue(s);
			ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_ExpandConstant: %s", var->name);
			return ret;
		}
	}
#endif
	return false;
}

byte CMD_ParseOrExpandHexByte(const char **p) {
	int val;
	float fv = 0; // silence warning
	while (isWhiteSpace(*(*p))) {
		(*p)++;
	}
	if (*(*p) == '$') {
		const char *stop = (*p) + 1;
		while (*stop && *stop != '$') {
			stop++;
		}
		CMD_ExpandConstant(*p, stop, &fv);
		val = fv;

		*p = stop;
		if (**p != 0)
			(*p)++;
	}
	else {
		val = hexbyte(*p);
		if (**p) {
			(*p)++;
			if (**p) {
				(*p)++;
			}
		}
	}
	while (isWhiteSpace(*(*p))) {
		(*p)++;
	}
	return val;
}
#if WINDOWS

void SIM_GenerateChannelStatesDesc(char *o, int outLen) {
	int role;
	const  char *roleStr;
	char buffer[64];
	for (int i = 0; i < CHANNEL_MAX; i++) {
		bool bFound = false;
		for (int j = 0; j < PLATFORM_GPIO_MAX; j++) {
			if (g_cfg.pins.roles[j] == IOR_None)
				continue;
			if (g_cfg.pins.channels[j] != i)
				continue;
			if (bFound == false) {
				bFound = true;
				snprintf(buffer, sizeof(buffer), "Ch %i - value %i - ", i, CHANNEL_Get(i));
				strcat_safe(o, buffer, outLen);
			}
			else {
				strcat_safe(o, ", ", outLen);
			}
			role = g_cfg.pins.roles[j];
			roleStr = PIN_RoleToString(role);
			strcat_safe(o, roleStr, outLen);
		}
		if (bFound) {
			strcat_safe(o, "\n", outLen);
		}
	}
}

const char *CMD_ExpandConstantString(const char *s, const char *stop, char *out, int outLen) {
	int idx;
	const char *ret;
	char tmp[32];

	ret = strCompareBound(s, "$autoexec.bat", stop, false);
	if (ret) {
		byte* data = LFS_ReadFile("autoexec.bat");
		if (data == 0) {
#if 1
			strcpy_safe(out, "No autoexec.bat for this sample", outLen);
			return ret;
#else
			return false;
#endif
		}
		strcpy_safe(out, (char*)data, outLen);
		free(data);
		return ret;
	}
	ret = strCompareBound(s, "$readfile(", stop, false);
	if (ret) {
		const char *opening = ret;
		const char *closingBrace = ret;
		while (1) {
			if (*closingBrace == 0)
				return false; // fail
			if (*closingBrace == ')') {
				break;
			}
			closingBrace++;
		}
		ret = closingBrace + 1;
		idx = closingBrace - opening;
		if (idx >= sizeof(tmp) - 1)
			idx = sizeof(tmp) - 2;
		strncpy(tmp, opening, idx);
		tmp[idx] = 0;
		byte* data = LFS_ReadFile(tmp);
		if (data == 0)
			return false;
		strcpy_safe(out, (char*)data, outLen);
		free(data);
		return ret;
	}
	ret = strCompareBound(s, "$pinstates", stop, false);
	if (ret) {
		SIM_GeneratePinStatesDesc(out, outLen);
		return ret;
	}
	ret = strCompareBound(s, "$channelstates", stop, false);
	if (ret) {
		SIM_GenerateChannelStatesDesc(out, outLen);
		return ret;
	}
	ret = strCompareBound(s, "$repeatingevents", stop, false);
	if (ret) {
		SIM_GenerateRepeatingEventsDesc(out, outLen);
		return ret;
	}
	ret = strCompareBound(s, "$simPowerState", stop, false);
	if (ret) {
		SIM_GeneratePowerStateDesc(out, outLen);
		return ret;
	}
	return false;
}
#endif

const char* CMD_ExpandConstantToString(const char* constant, char* out, char* stop)
{
	int outLen;
	float value = 0;
	int valueInt;
	const char* after;
	float delta;

	outLen = (stop - out) - 1;

	after = CMD_ExpandConstant(constant, 0, &value);
#if WINDOWS
	if(after == 0)
	{
		after = CMD_ExpandConstantString(constant, 0, out, outLen);
		return after;
	}
#endif
	if(after == 0)
		return 0;

	valueInt = (int)value;
	delta = valueInt - value;
	if(delta < 0)
		delta = -delta;
	if(delta < 0.001f)
	{
		snprintf(out, outLen, "%i", valueInt);
	}
	else
	{
		snprintf(out, outLen, "%f", value);
	}
	return after;
}

void CMD_ExpandConstantsWithinString(const char *in, char *out, int outLen) {
	char *outStop;
	const char *tmp;
	// just let us be on the safe side, someone else might forget about that -1
	outStop = out + outLen - 1;

	while (*in) {
		if (out >= outStop) {
			break;
		}
		if (*in == '$') {
			*out = 0;
			tmp = CMD_ExpandConstantToString(in, out, outStop);
			while (*out)
				out++;
			if (tmp != 0) {
				in = tmp;
			}
			else {
				*out = *in;
				out++;
				in++;
			}
		}
		else {
			*out = *in;
			out++;
			in++;
		}
	}
	*out = 0;
}
// like a strdup, but will expand constants.
// Please remember to free the returned string
char *CMD_ExpandingStrdup(const char *in) {
	const char *p;
	char *ret;
	int varCount;
	int realLen;

	realLen = 0;
	varCount = 0;
	// I am not sure which approach should I take
	// It could be easily done with external buffer, but it would have to be on stack or a global one...
	// Maybe let's just assume that variables cannot grow string way too big
	p = in;
	while (*p) {
		if (*p == '$') {
			varCount++;
		}
		realLen++;
		p++;
	}

	// not all var names are short, some are long...
	// but $CH1 is short and could expand to something longer like, idk, 123456?
	// just to be on safe side....
	realLen += varCount * 5;

	// space for NULL and also space to be sure
	realLen += 2;

	ret = (char*)malloc(realLen);
	CMD_ExpandConstantsWithinString(in, ret, realLen);
	return ret;
}
const char *CMD_FindMatchingBrace(const char *s) {
	if (*s != '(')
		return s;
	int level = 1;
	s++;
	while (*s) {
		if (*s == '(')
			level++;
		else if (*s == ')') {
			level--;
			if (level == 0)
				return s;
		}
		s++;
	}
	return s;

}
float CMD_EvaluateExpression(const char *s, const char *stop) {
	byte opCode;
	const char *op;
	float a, b, c;
	int idx;

	if (s == 0)
		return 0;
	if (*s == 0)
		return 0;

	// cull whitespaces at the end of expression
	if (stop == 0) {
		stop = s + strlen(s);
	}
	while (stop > s && isspace(((int)stop[-1]))) {
		stop--;
	}
	// cull whitespaces at the start
	while (isspace(((int)*s))) {
		s++;
		if (s >= stop) {
			return 0;
		}
	}
	while (*s == '(' && stop[-1] == ')' && CMD_FindMatchingBrace(s) == (stop-1)) {
		s++;
		stop--;
	}
	if (g_expDebugBuffer == 0) {
		g_expDebugBuffer = malloc(EXPRESSION_DEBUG_BUFFER_SIZE);
	}
	if (1) {
		idx = stop - s;
		memcpy(g_expDebugBuffer, s, idx);
		g_expDebugBuffer[idx] = 0;
		ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_EvaluateExpression: will run '%s'", g_expDebugBuffer);
	}
	op = CMD_FindOperator(s, stop, &opCode);
	if (op) {
		const char *p2;

		ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_EvaluateExpression: operator %i", opCode);

		// first token block begins at 's' and ends at 'op'
		// second token block begins at 'p2' and ends at NULL
		p2 = op + g_operators[opCode].len;

		a = CMD_EvaluateExpression(s, op);
		b = CMD_EvaluateExpression(p2, stop);

		// Why, again, %f crashes?
		//ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_EvaluateExpression: a = %f, b = %f", a, b);
		// It crashes even on sprintf.
		//sprintf(g_expDebugBuffer,"CMD_EvaluateExpression: a = %f, b = %f", a, b);
		//ADDLOG_INFO(LOG_FEATURE_EVENT, g_expDebugBuffer);

		switch (opCode)
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
			c = ((int)a) && ((int)b);
			break;
		case OP_OR:
			c = ((int)a) || ((int)b);
			break;
		case OP_ADD:
			c = a + b;
			break;
		case OP_SUB:
			c = a - b;
			break;
		case OP_MUL:
			c = a * b;
			break;
		case OP_DIV:
			c = a / b;
			break;
		case OP_MODULO:
			c = ((int)a) % ((int)b);
			break;
		default:
			c = 0;
			break;
		}
		return c;
	}
	if (s[0] == '!') {
		return !CMD_EvaluateExpression(s + 1, stop);
	}
	if (CMD_ExpandConstant(s, stop, &c)) {
		return c;
	}

	if (1) {
		idx = stop - s;
		memcpy(g_expDebugBuffer, s, idx);
		g_expDebugBuffer[idx] = 0;
	}
	ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_EvaluateExpression: will call atof for %s", g_expDebugBuffer);
	return atof(g_expDebugBuffer);
}

// if MQTTOnline then "qq" else "qq"
commandResult_t CMD_If(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *cmdA;
	const char *cmdB;
	const char *condition;
	//char buffer[256];
	int value;
	int argsCount;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	condition = Tokenizer_GetArg(0);
	if (stricmp(Tokenizer_GetArg(1), "then")) {
		ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: second argument always must be 'then', but it's '%s'", Tokenizer_GetArg(1));
		return CMD_RES_BAD_ARGUMENT;
	}
	argsCount = Tokenizer_GetArgsCount();
	int elsePos = -1;
	for (int i = 2; i < argsCount; i++) {
		if (!stricmp(Tokenizer_GetArg(i), "else")) {
			elsePos = i;
		}
	}
	if (elsePos != -1) {
		cmdA = Tokenizer_GetArg(2);
		// TODO: better else
		if (stricmp(Tokenizer_GetArg(3), "else")) {
			ADDLOG_INFO(LOG_FEATURE_EVENT, "CMD_If: fourth argument always must be 'else', but it's '%s'", Tokenizer_GetArg(3));
			return CMD_RES_BAD_ARGUMENT;
		}
		cmdB = Tokenizer_GetArg(4);
	}
	else {
		cmdA = Tokenizer_GetArgFrom(2);
		cmdB = 0;
	}

#ifdef WINDOWS
	ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_If: cmdA is '%s'", cmdA);
	if (cmdB) {
		ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_If: cmdB is '%s'", cmdB);
	}
	ADDLOG_IF_MATHEXP_DBG(LOG_FEATURE_EVENT, "CMD_If: condition is '%s'", condition);
#endif

	value = CMD_EvaluateExpression(condition, 0);

	// This buffer is here because we may need to exec commands recursively
	// and the Tokenizer_ etc is global?
	//if(value)
	//	strcpy_safe(buffer,cmdA);
	//else
	//	strcpy_safe(buffer,cmdB);
	//CMD_ExecuteCommand(buffer,0);
	if (value)
		CMD_ExecuteCommand(cmdA, 0);
	else {
		if (cmdB) {
			CMD_ExecuteCommand(cmdB, 0);
		}
	}

	return CMD_RES_OK;
}

