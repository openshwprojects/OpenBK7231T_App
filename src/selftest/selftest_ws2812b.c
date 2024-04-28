#ifdef WINDOWS

#include "selftest_local.h"


bool SM16703P_VerifyPixel(uint32_t pixel, byte r, byte g, byte b);

#define SELFTEST_ASSERT_PIXEL(index, r, g, b) SELFTEST_ASSERT(SM16703P_VerifyPixel(index, r, g, b));

void Test_WS2812B() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("SM16703P_Init 3", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128", 0);
	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 22 33 44", 0);
	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 22, 33, 44);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);
	// channel as pixel
	CMD_ExecuteCommand("SetChannel 5 124", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 $CH5 33 44", 0);
	SELFTEST_ASSERT_PIXEL(1, 124, 33, 44);
	// math expression
	CMD_ExecuteCommand("SetChannel 5 124", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 2*$CH5 33 44", 0);
	SELFTEST_ASSERT_PIXEL(1, 2 * 124, 33, 44);
	// math expression e2
	CMD_ExecuteCommand("SetChannel 5 124", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 10+$CH5 2*33+10 44/4", 0);
	SELFTEST_ASSERT_PIXEL(1, 10+ 124, 2 * 33+10, 44/4);

}


#endif
