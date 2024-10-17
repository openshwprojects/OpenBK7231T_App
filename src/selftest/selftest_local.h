#ifndef __SELFTEST_LOCAL_H__
#define __SELFTEST_LOCAL_H__

#ifdef WINDOWS

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../sim/sim_import.h"

void SelfTest_Failed(const char *file, const char *function, int line, const char *exp);

#define SELFTEST_ASSERT(expr) \
	if (!(expr)) \
	SelfTest_Failed(__FILE__, __FUNCTION__, __LINE__, #expr)

#define SELFTEST_ASSERT_FLOATCOMPARE(exp, res) SELFTEST_ASSERT(Float_Equals(exp, res));
#define SELFTEST_ASSERT_INTCOMPARE(exp, res) SELFTEST_ASSERT((exp==res));
#define SELFTEST_ASSERT_FLOATCOMPAREEPSILON(exp, res, eps) SELFTEST_ASSERT(Float_EqualsEpsilon(exp, res, eps));
#define SELFTEST_ASSERT_EXPRESSION(exp, res) SELFTEST_ASSERT(Float_Equals(CMD_EvaluateExpression(exp,0), res));
// currently, channels are integers
#define SELFTEST_ASSERT_CHANNEL(channelIndex, res) SELFTEST_ASSERT(CHANNEL_Get(channelIndex) == res);
#define SELFTEST_ASSERT_CHANNELEPSILON(channelIndex, res, marg) SELFTEST_ASSERT(Float_EqualsEpsilon(CHANNEL_Get(channelIndex), res, marg));
#define SELFTEST_ASSERT_CHANNELTYPE(channelIndex, res) SELFTEST_ASSERT(CHANNEL_GetType(channelIndex)==res);
#define SELFTEST_ASSERT_PIN_BOOLEAN(pinIndex, res) SELFTEST_ASSERT((SIM_GetSimulatedPinValue(pinIndex)== res));
#define SELFTEST_ASSERT_ARGUMENT(argumentIndex, res) SELFTEST_ASSERT(!strcmp(Tokenizer_GetArg(argumentIndex), res));
#define SELFTEST_ASSERT_ARGUMENT_INTEGER(argumentIndex, res) SELFTEST_ASSERT((Tokenizer_GetArgInteger(argumentIndex)== res));
#define SELFTEST_ASSERT_ARGUMENTS_COUNT(wantedCount) SELFTEST_ASSERT((Tokenizer_GetArgsCount()==wantedCount));
#define SELFTEST_ASSERT_JSON_VALUE_STRING(obj, varName, res) SELFTEST_ASSERT(!strcmp(Test_GetJSONValue_String(varName,obj), res));
#define SELFTEST_ASSERT_JSON_VALUE_STRING_NOT_PRESENT(obj, varName) SELFTEST_ASSERT((*Test_GetJSONValue_String(varName,obj))==0);
#define SELFTEST_ASSERT_JSON_VALUE_EXISTS(obj, varName) SELFTEST_ASSERT(Test_GetJSONValue_Generic(varName,obj));
#define SELFTEST_ASSERT_JSON_VALUE_INTEGER(obj, varName, res) SELFTEST_ASSERT((Test_GetJSONValue_Integer(varName,obj) == res));
#define SELFTEST_ASSERT_JSON_VALUE_INTEGER_NESTED2(par1, par2, varName, res) SELFTEST_ASSERT((Test_GetJSONValue_Integer_Nested2(par1, par2,varName) == res));
#define SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2(par1, par2, varName, res) SELFTEST_ASSERT((Float_Equals(Test_GetJSONValue_Float_Nested2(par1, par2,varName),res)));
#define SELFTEST_ASSERT_JSON_VALUE_STRING_NESTED2(par1, par2, varName, res) SELFTEST_ASSERT((!strcmp(Test_GetJSONValue_String_Nested2(par1, par2,varName),res)));
#define SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(index, key, valInt) SELFTEST_ASSERT((Test_GetJSONValue_IntFromArray(index,key)==valInt));
#define SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(index, key, valInt) SELFTEST_ASSERT((!strcmp(Test_GetJSONValue_StrFromArray(index,key),valInt)));
#define SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(bHas) SELFTEST_ASSERT((bHas) == SIM_HasHTTPDimmer());
#define SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(bHas) SELFTEST_ASSERT((bHas) == SIM_HasHTTPTemperature());
#define SELFTEST_ASSERT_HTTP_HAS_LED_RGB(bHas) SELFTEST_ASSERT((bHas) == SIM_HasHTTPRGB());
#define SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(bHas) SELFTEST_ASSERT((bHas) == SIM_HasHTTP_LED_Toggler(true));
#define SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(bHas) SELFTEST_ASSERT((bHas) == SIM_HasHTTP_LED_Toggler(false));


#define SELFTEST_ASSERT_STRING(current,expected) SELFTEST_ASSERT((strcmp(expected,current) == 0));
#define SELFTEST_ASSERT_INTEGER(current,expected) SELFTEST_ASSERT((expected==current));
#define SELFTEST_ASSERT_HTML_REPLY(expected) SELFTEST_ASSERT((strcmp(Test_GetLastHTMLReply(),expected) == 0));
#define SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR(topic, value, bRetain) SELFTEST_ASSERT(SIM_CheckMQTTHistoryForString(topic,value,bRetain));
#define SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT(topic, value, bRetain) SELFTEST_ASSERT(SIM_CheckMQTTHistoryForFloat(topic,value,bRetain));
#define SELFTEST_ASSERT_FLAG(flag, value) SELFTEST_ASSERT(CFG_HasFlag(flag)==value);
#define SELFTEST_ASSERT_HAS_MQTT_JSON_SENT(topic, bPrefixMode) SELFTEST_ASSERT(!SIM_BeginParsingMQTTJSON(topic, bPrefixMode));
#define SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY(topic, bPrefixMode, object1, object2, key, value) SELFTEST_ASSERT(SIM_HasMQTTHistoryStringWithJSONPayload(topic, bPrefixMode, object1, object2, key, value));
#define SELFTEST_ASSERT_HAS_SENT_UART_STRING(str) SELFTEST_ASSERT(SIM_UART_ExpectAndConsumeHexStr(str));
#define SELFTEST_ASSERT_HAS_UART_EMPTY() SELFTEST_ASSERT(SIM_UART_GetDataSize()==0);

//#define FLOAT_EQUALS (a,b) (fabs(a-b)<0.001f)
inline float myFabs(float f) {
	if (f < 0)
		return -f;
	return f;
}
inline bool Float_Equals(float a, float b) {
	float res = myFabs(a - b);
	return res < 0.001f;
}
inline bool Float_EqualsEpsilon(float a, float b, float epsilon) {
	float res = myFabs(a - b);
	return res < epsilon;
}

#define VA_BUFFER_SIZE 4096
#define VA_COUNT 4

inline const char *va(const char *fmt, ...) {
	va_list argList;
	static int whi = 0;
	static char buffer[VA_COUNT][VA_BUFFER_SIZE];

	whi++;
	whi %= VA_COUNT;
	char *p = buffer[whi];

	va_start(argList, fmt);
	vsnprintf(p, VA_BUFFER_SIZE, fmt, argList);
	va_end(argList);
	return p;
}

void Test_Battery();
void Test_Flash_Search();
void Test_JSON_Lib();
void Test_Commands_Startup();
void Test_TwoPWMsOneChannel();
void Test_ClockEvents();
void Test_Commands_Channels();
void Test_LEDDriver();
void Test_TuyaMCU_Basic();
void Test_TuyaMCU_Boolean();
void Test_TuyaMCU_DP22();
void Test_TuyaMCU_Mult();
void Test_TuyaMCU_RawAccess();
void Test_Command_If();
void Test_Command_If_Else();
void Test_LFS();
void Test_Tokenizer();
void Test_Commands_Alias();
void Test_ExpandConstant();
void Test_Scripting();
void Test_RepeatingEvents();
void Test_HTTP_Client();
void Test_DeviceGroups();
void Test_NTP();
void Test_NTP_SunsetSunrise();
void Test_MQTT();
void Test_Tasmota();
void Test_Backlog();
void Test_EnergyMeter();
void Test_DHT();
void Test_Flags();
void Test_MultiplePinsOnChannel();
void Test_HassDiscovery();
void Test_HassDiscovery_Base();
void Test_HassDiscovery_Ext();
void Test_Demo_ExclusiveRelays();
void Test_MapRanges();
void Test_Demo_MapFanSpeedToRelays();
void Test_Demo_FanCyclingRelays();
void Test_Role_ToggleAll();
void Test_Demo_SignAndValue();
void Test_Demo_SimpleShuttersScript();
void Test_Commands_Generic();
void Test_ChangeHandlers_MQTT();
void Test_ChangeHandlers();
void Test_ChangeHandlers_EnsureThatChannelVariableIsExpandedAtHandlerRunTime();
void Test_Commands_Calendar();
void Test_CFG_Via_HTTP();
void Test_Demo_ButtonScrollingChannelValues();
void Test_Demo_ButtonToggleGroup();
void Test_Role_ToggleAll_2();
void Test_WaitFor();
void Test_IF_Inside_Backlog();
void Test_MQTT_Get_LED_EnableAll();
void Test_MQTT_Get_Relay();
void Test_TuyaMCU_BatteryPowered();
void Test_ChargeLimitDriver();
void Test_WS2812B();
void Test_DoorSensor();
void Test_Enums();
void Test_Expressions_RunTests_Basic();
void Test_Expressions_RunTests_Braces();
void Test_ButtonEvents();
void Test_Http();

void Test_GetJSONValue_Setup(const char *text);
void Test_FakeHTTPClientPacket_GET(const char *tg);
void Test_FakeHTTPClientPacket_POST(const char *tg, const char *data);
void Test_FakeHTTPClientPacket_POST_withJSONReply(const char *tg, const char *data);
void Test_FakeHTTPClientPacket_JSON(const char *tg, ...);
const char *Test_GetLastHTMLReply();

bool SIM_HasHTTPTemperature();
bool SIM_HasHTTPRGB();
bool SIM_HasHTTP_LED_Toggler(bool bIsNowOn);
bool SIM_HasHTTP_Active_RGB();
bool SIM_HasHTTPDimmer();

// TODO: move elsewhere?
void Sim_RunMiliseconds(int ms, bool bApplyRealtimeWait);
void Sim_RunSeconds(float f, bool bApplyRealtimeWait);
void Sim_RunFrames(int n, bool bApplyRealtimeWait);

int Test_GetJSONValue_Integer_Nested2(const char *par1, const char *par2, const char *keyword);
float Test_GetJSONValue_Float_Nested2(const char *par1, const char *par2, const char *keyword);
int Test_GetJSONValue_Integer(const char *keyword, const char *obj);
const char *Test_GetJSONValue_String(const char *keyword, const char *obj);
const char *Test_GetJSONValue_String_Nested(const char *par1, const char *keyword);
const char *Test_GetJSONValue_String_Nested2(const char *par1, const char *par2, const char *keyword);
int Test_GetJSONValue_IntFromArray(int index, const char *obj);
const char *Test_GetJSONValue_StrFromArray(int index, const char *obj);

void SIM_SendFakeMQTT(const char *text, const char *arguments);
void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments);
void SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic(const char *command, const char *arguments);
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments);
void SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(int channelIndex, const char *arguments);
void SIM_ClearMQTTHistory();
bool SIM_CheckMQTTHistoryForString(const char *topic, const char *value, bool bRetain);
bool SIM_HasMQTTHistoryStringWithJSONPayload(const char *topic, bool bPrefixMode, const char *object1, const char *object2, const char *key, const char *value);
bool SIM_CheckMQTTHistoryForFloat(const char *topic, float value, bool bRetain);
const char *SIM_GetMQTTHistoryString(const char *topic, bool bPrefixMode);
bool SIM_BeginParsingMQTTJSON(const char *topic, bool bPrefixMode);

void SIM_SimulateUserClickOnPin(int pin);

// simulated UART
void SIM_UART_InitReceiveRingBuffer(int size);
int SIM_UART_GetDataSize();
byte SIM_UART_GetByte(int index);
void SIM_UART_ConsumeBytes(int idx);
void SIM_AppendUARTByte(byte rc);
bool SIM_UART_ExpectAndConsumeHByte(byte b);
bool SIM_UART_ExpectAndConsumeHexStr(const char *hexString);
void SIM_ClearUART();


#endif
#endif // __SELFTEST_LOCAL_H__
