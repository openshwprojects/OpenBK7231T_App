#ifdef WINDOWS

#include "selftest_local.h"
void Test_WS2812B() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("SM16703P_Init 3", 0);

}


#endif
