#include "drv_test_drivers.h"

#include <math.h>

#include "../cmnds/cmd_public.h"
#include "drv_bl_shared.h"

static float base_v = 120;
static float base_c = 1;
static float base_p = 120;
static bool bAllowRandom = true;

commandResult_t TestPower_Setup(const void* context, const char* cmd, const char* args, int cmdFlags) {
	

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	base_v = Tokenizer_GetArgFloat(0);
	base_c = Tokenizer_GetArgFloat(1);
	base_p = Tokenizer_GetArgFloat(2);
	bAllowRandom = Tokenizer_GetArgInteger(3);
	// SetupTestPower 230 0.23 60 

	return CMD_RES_OK;
}
//Test Power driver
void Test_Power_Init(void) {
    BL_Shared_Init();

	//cmddetail:{"name":"SetupTestPower","args":"[fakeVoltage] [FakeCurrent] [FakePower] [bAllowRandom]",
	//cmddetail:"descr":"Starts the fake power metering driver",
	//cmddetail:"fn":"TestPower_Setup","file":"driver/drv_test_drivers.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SetupTestPower", TestPower_Setup, NULL);
}
void Test_Power_RunEverySecond(void) {
    float final_v = base_v;
	float final_c = base_c;
	float final_p = base_p;

	if (bAllowRandom) {
		final_c += (rand() % 100) * 0.001f;
		final_v += (rand() % 100) * 0.1f;
		final_p += (rand() % 100) * 0.1f;
	}
	BL_ProcessUpdate(final_v, final_c, final_p, NAN, NAN);
}

//Test LED driver
void Test_LED_Driver_Init(void) {}
void Test_LED_Driver_RunEverySecond(void) {}
void Test_LED_Driver_OnChannelChanged(int ch, int value) {
}
