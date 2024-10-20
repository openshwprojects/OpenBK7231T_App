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

	// check for 'all'

	CMD_ExecuteCommand("SM16703P_SetPixel all 123 231 132", 0);
	SELFTEST_ASSERT_PIXEL(0, 123, 231, 132);
	SELFTEST_ASSERT_PIXEL(1, 123, 231, 132);
	SELFTEST_ASSERT_PIXEL(2, 123, 231, 132);


	// fake DDP packet
	{
		byte ddpPacket[128];

		// data starts at offset 10
		// pixel 0
		ddpPacket[10] = 0xFF;
		ddpPacket[11] = 0xFF;
		ddpPacket[12] = 0xFF;
		// pixel 1
		ddpPacket[13] = 0xFF;
		ddpPacket[14] = 0x0;
		ddpPacket[15] = 0x0;
		// pixel 2
		ddpPacket[16] = 0xFF;
		ddpPacket[17] = 0x0;
		ddpPacket[18] = 0xFF;

		DDP_Parse(ddpPacket, sizeof(ddpPacket));

		SELFTEST_ASSERT_PIXEL(0, 0xFF, 0xFF, 0xFF);
		SELFTEST_ASSERT_PIXEL(1, 0xFF, 0, 0);
		SELFTEST_ASSERT_PIXEL(2, 0xFF, 0, 0xFF);
	}

	CMD_ExecuteCommand("SM16703P_Init 6", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 0 255 0 0", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 0 255 0", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 2 0 0 255", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 3 255 0 0", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 4 0 255 0", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 5 0 0 255", 0);
	SELFTEST_ASSERT_PIXEL(0, 255, 0, 0);
	SELFTEST_ASSERT_PIXEL(1, 0, 255, 0);
	SELFTEST_ASSERT_PIXEL(2, 0, 0, 255);
	SELFTEST_ASSERT_PIXEL(3, 255, 0, 0);
	SELFTEST_ASSERT_PIXEL(4, 0, 255, 0);
	SELFTEST_ASSERT_PIXEL(5, 0, 0, 255);

	CMD_ExecuteCommand("startDriver PixelAnim", 0);
	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0000", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 255, 0, 0);
	}
	CMD_ExecuteCommand("led_basecolor_rgb FFFF00", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 255, 255, 0);
	}
	CMD_ExecuteCommand("led_enableAll 0", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 0, 0, 0);
	}
	CMD_ExecuteCommand("led_enableAll 1", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 255, 255, 0);
	}
	CMD_ExecuteCommand("led_basecolor_rgb FF00FF", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 255, 0, 255);
	}
	CMD_ExecuteCommand("led_dimmer 50", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 55, 0, 55);
	}
	CMD_ExecuteCommand("led_dimmer 100", 0);
	Sim_RunFrames(5, false);
	CMD_ExecuteCommand("Anim 1", 0);
	Sim_RunFrames(5, false);
	CMD_ExecuteCommand("led_enableAll 0", 0);
	Sim_RunFrames(5, false);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 0, 0, 0);
	}
}


#endif
