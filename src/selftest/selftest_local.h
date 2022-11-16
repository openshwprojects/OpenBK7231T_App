#ifdef WINDOWS

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"

void SelfTest_Failed(const char *file, int line, const char *exp);

#define SELFTEST_ASSERT(expr) \
	if (!(expr)) \
	SelfTest_Failed(__FILE__, __FUNCTION__, __LINE__, #expr)

#define SELFTEST_ASSERT_EXPRESSION(exp, res) SELFTEST_ASSERT(Float_Equals(CMD_EvaluateExpression(exp,0), res));
#define SELFTEST_ASSERT_CHANNEL(channelIndex, res) SELFTEST_ASSERT(Float_Equals(CHANNEL_Get(channelIndex), res));
#define SELFTEST_ASSERT_ARGUMENT(argumentIndex, res) SELFTEST_ASSERT(!strcmp(Tokenizer_GetArg(argumentIndex), res));
#define SELFTEST_ASSERT_ARGUMENTS_COUNT(wantedCount) SELFTEST_ASSERT((Tokenizer_GetArgsCount()==wantedCount));

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


#endif
