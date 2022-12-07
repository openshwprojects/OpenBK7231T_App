#ifdef WINDOWS

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"

void SelfTest_Failed(const char *file, const char *function, int line, const char *exp);

#define SELFTEST_ASSERT(expr) \
	if (!(expr)) \
	SelfTest_Failed(__FILE__, __FUNCTION__, __LINE__, #expr)

#define SELFTEST_ASSERT_EXPRESSION(exp, res) SELFTEST_ASSERT(Float_Equals(CMD_EvaluateExpression(exp,0), res));
#define SELFTEST_ASSERT_CHANNEL(channelIndex, res) SELFTEST_ASSERT(Float_Equals(CHANNEL_Get(channelIndex), res));
#define SELFTEST_ASSERT_ARGUMENT(argumentIndex, res) SELFTEST_ASSERT(!strcmp(Tokenizer_GetArg(argumentIndex), res));
#define SELFTEST_ASSERT_ARGUMENT_INTEGER(argumentIndex, res) SELFTEST_ASSERT((Tokenizer_GetArgInteger(argumentIndex)== res));
#define SELFTEST_ASSERT_ARGUMENTS_COUNT(wantedCount) SELFTEST_ASSERT((Tokenizer_GetArgsCount()==wantedCount));
#define SELFTEST_ASSERT_JSON_VALUE_STRING(obj, varName, res) SELFTEST_ASSERT(!strcmp(Test_GetJSONValue_String(varName,obj), res));
#define SELFTEST_ASSERT_JSON_VALUE_INTEGER(obj, varName, res) SELFTEST_ASSERT((Test_GetJSONValue_Integer(varName,obj) == res));
#define SELFTEST_ASSERT_STRING(current,expected) SELFTEST_ASSERT((strcmp(expected,current) == 0));
#define SELFTEST_ASSERT_INTEGER(current,expected) SELFTEST_ASSERT((expected==current));
#define SELFTEST_ASSERT_HTML_REPLY(expected) SELFTEST_ASSERT((strcmp(Test_GetLastHTMLReply(),expected) == 0));

//#define FLOAT_EQUALS (a,b) (abs(a-b)<0.001f)
inline bool Float_Equals(float a, float b) {
	float res = abs(a - b);
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

void Test_FakeHTTPClientPacket_GET(const char *tg);
void Test_FakeHTTPClientPacket_POST(const char *tg, const char *data);
const char *Test_GetLastHTMLReply();

// TODO: move elsewhere?
void Sim_RunMiliseconds(int ms, bool bApplyRealtimeWait);
void Sim_RunSeconds(float f, bool bApplyRealtimeWait);
void Sim_RunFrames(int n, bool bApplyRealtimeWait);


void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments);
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments);

#endif
