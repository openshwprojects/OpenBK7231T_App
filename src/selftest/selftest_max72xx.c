#ifdef WINDOWS

#include "selftest_local.h"

void Test_MAX72XX() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 16", 0);
	CMD_ExecuteCommand("MAX72XX_Print 1232132131212", 0);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 4", 0);
	CMD_ExecuteCommand("MAX72XX_Print 1232132131212", 0);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 16", 0);
	CMD_ExecuteCommand("MAX72XX_Clear", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 0);
	CMD_ExecuteCommand("MAX72XX_Print 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 10);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 1);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 10);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 2);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 10);
	CMD_ExecuteCommand("MAX72XX_Print 11", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2*10);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 3);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2*10);
	CMD_ExecuteCommand("MAX72XX_Scroll -1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2*10);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 2);
	CMD_ExecuteCommand("MAX72XX_Scroll -3", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 127);
	CMD_ExecuteCommand("MAX72XX_Scroll -10", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 117);
	CMD_ExecuteCommand("MAX72XX_Scroll -20", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 97);
	CMD_ExecuteCommand("MAX72XX_Scroll 20", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 117);
	CMD_ExecuteCommand("MAX72XX_Scroll 10", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 127);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 0);

	for (int i = 0; i < 128; i++) {
		CMD_ExecuteCommand("MAX72XX_Scroll -1", 0);
		SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	}
	for (int i = 0; i < 128; i++) {
		CMD_ExecuteCommand("MAX72XX_Scroll 2", 0);
		SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2 * 10);
	}
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 64", 0);
	CMD_ExecuteCommand("MAX72XX_Print 1232132131212", 0);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("startDriver MAX72XX_Clock", 0);
	Sim_RunSeconds(1, false); 
	CMD_ExecuteCommand("MAX72XXClock_Animate 1", 0);
	Sim_RunSeconds(1, false);
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);



	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 64", 0);
	CMD_ExecuteCommand("MAX72XX_Clear", 0);
	CMD_ExecuteCommand("MAX72XX_Print 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 1 * 10);
	for (int i = 1; i < 64; i+=23) {
		char buff[512];
		char buff2[512];
		int j;
		for ( j = 0; j < i; j++) {
			buff[j] = '1';
		}
		buff[j] = 0;
		CMD_ExecuteCommand("MAX72XX_Clear", 0);
		strcpy(buff2, "MAX72XX_Print ");
		strcat(buff2, buff);
		CMD_ExecuteCommand(buff2, 0);
		for ( j = 0; j < 3; j++) {
			CMD_ExecuteCommand("MAX72XX_Scroll 8", 0);
			int act = MAX72XXSingle_CountPixels(true);
			SELFTEST_ASSERT(act == i * 10);
		}
	}
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);

	CMD_ExecuteCommand("startDriver MAX72XX", 0);
	CMD_ExecuteCommand("MAX72XX_Setup 10 8 9 16", 0);
	CMD_ExecuteCommand("MAX72XX_Clear", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 0);
	CMD_ExecuteCommand("MAX72XX_SetPixel 1 1 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_GetScrollCount() == 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 1);
	CMD_ExecuteCommand("MAX72XX_Scroll 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 1);
	CMD_ExecuteCommand("MAX72XX_SetPixel 1 1 1", 0);
	SELFTEST_ASSERT(MAX72XXSingle_CountPixels(true) == 2);
	CMD_ExecuteCommand("stopDriver MAX72XX", 0);
	
}


#endif
