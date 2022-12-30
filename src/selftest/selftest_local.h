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
#define SELFTEST_ASSERT_EXPRESSION(exp, res) SELFTEST_ASSERT(Float_Equals(CMD_EvaluateExpression(exp,0), res));
#define SELFTEST_ASSERT_CHANNEL(channelIndex, res) SELFTEST_ASSERT(Float_Equals(CHANNEL_Get(channelIndex), res));
#define SELFTEST_ASSERT_ARGUMENT(argumentIndex, res) SELFTEST_ASSERT(!strcmp(Tokenizer_GetArg(argumentIndex), res));
#define SELFTEST_ASSERT_ARGUMENT_INTEGER(argumentIndex, res) SELFTEST_ASSERT((Tokenizer_GetArgInteger(argumentIndex)== res));
#define SELFTEST_ASSERT_ARGUMENTS_COUNT(wantedCount) SELFTEST_ASSERT((Tokenizer_GetArgsCount()==wantedCount));
#define SELFTEST_ASSERT_JSON_VALUE_STRING(obj, varName, res) SELFTEST_ASSERT(!strcmp(Test_GetJSONValue_String(varName,obj), res));
#define SELFTEST_ASSERT_JSON_VALUE_INTEGER(obj, varName, res) SELFTEST_ASSERT((Test_GetJSONValue_Integer(varName,obj) == res));
#define SELFTEST_ASSERT_JSON_VALUE_INTEGER_NESTED2(par1, par2, varName, res) SELFTEST_ASSERT((Test_GetJSONValue_Integer_Nested2(par1, par2,varName) == res));
#define SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2(par1, par2, varName, res) SELFTEST_ASSERT((Float_Equals(Test_GetJSONValue_Float_Nested2(par1, par2,varName),res)));
#define SELFTEST_ASSERT_STRING(current,expected) SELFTEST_ASSERT((strcmp(expected,current) == 0));
#define SELFTEST_ASSERT_INTEGER(current,expected) SELFTEST_ASSERT((expected==current));
#define SELFTEST_ASSERT_HTML_REPLY(expected) SELFTEST_ASSERT((strcmp(Test_GetLastHTMLReply(),expected) == 0));
#define SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR(topic, value, bRetain) SELFTEST_ASSERT(SIM_CheckMQTTHistoryForString(topic,value,bRetain));
#define SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT(topic, value, bRetain) SELFTEST_ASSERT(SIM_CheckMQTTHistoryForFloat(topic,value,bRetain));

//#define FLOAT_EQUALS (a,b) (fabs(a-b)<0.001f)
inline bool Float_Equals(float a, float b) {
	float res = fabs(a - b);
	return res < 0.001f;
}


void Test_Commands_Channels();
void Test_LEDDriver();
void Test_TuyaMCU_Basic();
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
void Test_MQTT();
void Test_Tasmota();
void Test_EnergyMeter();

void Test_FakeHTTPClientPacket_GET(const char *tg);
void Test_FakeHTTPClientPacket_POST(const char *tg, const char *data);
void Test_FakeHTTPClientPacket_JSON(const char *tg);
const char *Test_GetLastHTMLReply();

// TODO: move elsewhere?
void Sim_RunMiliseconds(int ms, bool bApplyRealtimeWait);
void Sim_RunSeconds(float f, bool bApplyRealtimeWait);
void Sim_RunFrames(int n, bool bApplyRealtimeWait);

int Test_GetJSONValue_Integer_Nested2(const char *par1, const char *par2, const char *keyword);
float Test_GetJSONValue_Float_Nested2(const char *par1, const char *par2, const char *keyword);
int Test_GetJSONValue_Integer(const char *keyword, const char *obj);
const char *Test_GetJSONValue_String(const char *keyword, const char *obj);

void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments);
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments);
void SIM_ClearMQTTHistory();
bool SIM_CheckMQTTHistoryForString(const char *topic, const char *value, bool bRetain);
bool SIM_CheckMQTTHistoryForFloat(const char *topic, float value, bool bRetain);


#endif
