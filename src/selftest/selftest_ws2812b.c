#ifdef WINDOWS

#include "selftest_local.h"


bool Strip_VerifyPixel(uint32_t pixel, byte r, byte g, byte b);
bool Strip_VerifyPixel4(uint32_t pixel, byte r, byte g, byte b, byte a);
bool Strip_VerifyPixel5(uint32_t pixel, byte r, byte g, byte b, byte c, byte w);

#define SELFTEST_ASSERT_PIXEL(index, r, g, b) SELFTEST_ASSERT(Strip_VerifyPixel(index, r, g, b));
#define SELFTEST_ASSERT_PIXEL4(index, r, g, b, w) SELFTEST_ASSERT(Strip_VerifyPixel4(index, r, g, b, w));
#define SELFTEST_ASSERT_PIXEL5(index, r, g, b, c, w) SELFTEST_ASSERT(Strip_VerifyPixel5(index, r, g, b, c, w));


void Strip_setMultiplePixel(uint32_t pixel, uint8_t *data, bool push);

void Test_DMX_RGB() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(4096);
	SIM_ClearUART();

	SELFTEST_ASSERT_HAS_UART_EMPTY();

	SELFTEST_ASSERT_PAGE_NOT_CONTAINS("index", "LED RGB Color");
	SELFTEST_ASSERT_PAGE_NOT_CONTAINS("index", "LED Temperature Slider");

	CMD_ExecuteCommand("startDriver DMX", 0);
	CMD_ExecuteCommand("Strip_Init 3", 0);

	SELFTEST_ASSERT_PAGE_CONTAINS("index", "LED RGB Color");
	SELFTEST_ASSERT_PAGE_NOT_CONTAINS("index", "LED Temperature Slider");

	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128", 0);
	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);

	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("SM16703P_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  FF0080  FF0080  FF0080");
	// 512 channels, but checked already 10
	for (int i = 0; i < 100; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00");
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("SM16703P_SetPixel 0 128 128 128", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 255 255 255", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 2 15 15 15", 0);
	SELFTEST_ASSERT_PIXEL(0, 128, 128, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 255, 255);
	SELFTEST_ASSERT_PIXEL(2, 15, 15, 15);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("SM16703P_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  808080  FFFFFF  0F0F0F");
	// 512 channels, but checked already 10
	for (int i = 0; i < 100; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00");
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("startDriver DDP", 0);
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

	// nothing is sent by OBK at that point
}
void Test_DMX_RGBC() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(4096);
	SIM_ClearUART();

	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("startDriver DMX", 0);
	CMD_ExecuteCommand("SM16703P_Init 3 RGBC", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128 255", 0);
	SELFTEST_ASSERT_PIXEL4(0, 255, 0, 128, 255);
	SELFTEST_ASSERT_PIXEL4(1, 255, 0, 128, 255);
	SELFTEST_ASSERT_PIXEL4(2, 255, 0, 128, 255);

	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("SM16703P_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  FF0080FF  FF0080FF  FF0080FF");
	// 512 channels, but checked already 12
	for (int i = 0; i < 100; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("SM16703P_SetPixel 0 128 128 128 128", 0);
	CMD_ExecuteCommand("Strip_SetPixel 1 255 255 255 255", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 2 15 15 15 15", 0);
	SELFTEST_ASSERT_PIXEL4(0, 128, 128, 128, 128);
	SELFTEST_ASSERT_PIXEL4(1, 255, 255, 255, 255);
	SELFTEST_ASSERT_PIXEL4(2, 15, 15, 15, 15);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("Strip_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  80808080  FFFFFFFF  0F0F0F0F");
	// 512 channels, but checked already 12
	for (int i = 0; i < 100; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// nothing is sent by OBK at that point
}
void Test_DMX_RGBW() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(4096);
	SIM_ClearUART();

	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("startDriver DMX", 0);
	CMD_ExecuteCommand("SM16703P_Init 3 RGBW", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128 255", 0);
	SELFTEST_ASSERT_PIXEL4(0, 255, 0, 128, 255);
	SELFTEST_ASSERT_PIXEL4(1, 255, 0, 128, 255);
	SELFTEST_ASSERT_PIXEL4(2, 255, 0, 128, 255);

	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("SM16703P_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  FF0080FF  FF0080FF  FF0080FF");
	// 512 channels, but checked already 12
	for (int i = 0; i < 100; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("SM16703P_SetPixel 0 128 128 128 128", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 255 255 255 255", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 2 15 15 15 15", 0);
	SELFTEST_ASSERT_PIXEL4(0, 128, 128, 128, 128);
	SELFTEST_ASSERT_PIXEL4(1, 255, 255, 255, 255);
	SELFTEST_ASSERT_PIXEL4(2, 15, 15, 15, 15);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	//CMD_ExecuteCommand("SM16703P_Start", 0);
	//SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	//SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  80808080  FFFFFFFF  0F0F0F0F");
	//// 512 channels, but checked already 12
	//for (int i = 0; i < 100; i++) {
	//	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	//}
	//SELFTEST_ASSERT_HAS_UART_EMPTY();

	// nothing is sent by OBK at that point
}
void Test_DMX_RGBCW() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(4096);
	SIM_ClearUART();

	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("startDriver DMX", 0);
	CMD_ExecuteCommand("SM16703P_Init 3 RGBCW", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128 255 128", 0);
	SELFTEST_ASSERT_PIXEL5(0, 255, 0, 128, 255, 128);
	SELFTEST_ASSERT_PIXEL5(1, 255, 0, 128, 255, 128);
	SELFTEST_ASSERT_PIXEL5(2, 255, 0, 128, 255, 128);

	SELFTEST_ASSERT_HAS_UART_EMPTY();
	CMD_ExecuteCommand("SM16703P_Start", 0);
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  FF0080FF80  FF0080FF80  FF0080FF80");
	// 512 channels, but checked already 12+3
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00");
	for (int i = 0; i < 99; i++) {
		SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	}
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	CMD_ExecuteCommand("SM16703P_SetPixel 0 128 128 128 128 11", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 1 255 255 255 255 22", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel 2 15 15 15 15 33", 0);
	SELFTEST_ASSERT_PIXEL5(0, 128, 128, 128, 128, 11);
	SELFTEST_ASSERT_PIXEL5(1, 255, 255, 255, 255, 22);
	SELFTEST_ASSERT_PIXEL5(2, 15, 15, 15, 15, 33);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	//CMD_ExecuteCommand("SM16703P_Start", 0);
	//SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	//SELFTEST_ASSERT_HAS_SENT_UART_STRING("00  80808080  FFFFFFFF  0F0F0F0F");
	//// 512 channels, but checked already 12
	//for (int i = 0; i < 100; i++) {
	//	SELFTEST_ASSERT_HAS_SENT_UART_STRING("00 00 00 00 00");
	//}
	//SELFTEST_ASSERT_HAS_UART_EMPTY();

	// nothing is sent by OBK at that point
}
extern int stat_ddpPacketsReceived;
void SIM_WaitForDDPPacket() {
	int prev = stat_ddpPacketsReceived;

	for (int i = 0; i < 100; i++) {
		Sim_RunFrames(1, true);
		if (stat_ddpPacketsReceived > prev) {
			break; // got it
		}
	}
	
}
void Test_WS2812B_and_PWM_CW() {
	// reset whole device
	SIM_ClearOBK(0);

	CFG_SetFlag(OBK_FLAG_LED_USE_OLD_LINEAR_MODE, 1);

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 3);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 4);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("SM16703P_Init 3", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0080", 0);
	CMD_ExecuteCommand("led_enableAll 1", 0);

	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);

	CMD_ExecuteCommand("led_dimmer 50", 0);
	SELFTEST_ASSERT_PIXEL(0, 127, 0, 64);
	SELFTEST_ASSERT_PIXEL(1, 127, 0, 64);
	SELFTEST_ASSERT_PIXEL(2, 127, 0, 64);

	// set 100% Warm
	CMD_ExecuteCommand("led_temperature 500", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);

	SELFTEST_ASSERT_CHANNEL(3, 0); // Cool
	SELFTEST_ASSERT_CHANNEL(4, 100); // Warm
	SELFTEST_ASSERT_PIXEL(0, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(1, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(2, 0, 0, 0);

	// 50% Warm
	CMD_ExecuteCommand("led_dimmer 50", 0);
	SELFTEST_ASSERT_CHANNEL(3, 0); // Cool
	SELFTEST_ASSERT_CHANNEL(4, 50); // Warm
	SELFTEST_ASSERT_PIXEL(0, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(1, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(2, 0, 0, 0);

	// set 100% Cool
	CMD_ExecuteCommand("led_temperature 154", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	SELFTEST_ASSERT_CHANNEL(3, 100); // Cool
	SELFTEST_ASSERT_CHANNEL(4, 0); // Warm
	SELFTEST_ASSERT_PIXEL(0, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(1, 0, 0, 0);
	SELFTEST_ASSERT_PIXEL(2, 0, 0, 0);
}
void Test_WS2812B_and_PWM_White() {
	// reset whole device
	SIM_ClearOBK(0);

	CFG_SetFlag(OBK_FLAG_LED_USE_OLD_LINEAR_MODE, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 4);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("SM16703P_Init 3", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0080", 0);
	CMD_ExecuteCommand("led_enableAll 1", 0);

	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);

	CMD_ExecuteCommand("led_dimmer 50", 0);
	SELFTEST_ASSERT_PIXEL(0, 127, 0, 64);
	SELFTEST_ASSERT_PIXEL(1, 127, 0, 64);
	SELFTEST_ASSERT_PIXEL(2, 127, 0, 64);

	// set 100% Warm
	CMD_ExecuteCommand("led_temperature 500", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);

}
void Test_WS2812B() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("SM16703P_Init 3", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128", 0);
	SELFTEST_ASSERT_PIXEL(0, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(1, 255, 0, 128);
	SELFTEST_ASSERT_PIXEL(2, 255, 0, 128);

	// test color order
	CMD_ExecuteCommand("SM16703P_Init 3 BGR", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 0", 0);
	SELFTEST_ASSERT_PIXEL(0, 0, 0, 255);
	SELFTEST_ASSERT_PIXEL(1, 0, 0, 255);
	SELFTEST_ASSERT_PIXEL(2, 0, 0, 255);

	// cannot crash
	CMD_ExecuteCommand("SM16703P_SetPixel 1234 255 0 0", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel -123 255 0 0", 0);

	// fake 3 pixels data
	{ // RGB
		CMD_ExecuteCommand("SM16703P_Init 3 RGB", 0);
		uint8_t dat[9] = { 255, 0, 0,
			0, 255, 0,
			0, 0, 255 };
		Strip_setMultiplePixel(3, dat, false);
		SELFTEST_ASSERT_PIXEL(0, 255, 0, 0);
		SELFTEST_ASSERT_PIXEL(1, 0, 255, 0);
		SELFTEST_ASSERT_PIXEL(2, 0, 0, 255);
	}
	{ // BGR
		CMD_ExecuteCommand("SM16703P_Init 3 BGR", 0);
		uint8_t dat[9] = { 255, 0, 0,
			0, 255, 0,
			0, 0, 255 };
		Strip_setMultiplePixel(3, dat, false);
		SELFTEST_ASSERT_PIXEL(0, 0, 0, 255);
		SELFTEST_ASSERT_PIXEL(1, 0, 255, 0);
		SELFTEST_ASSERT_PIXEL(2, 255, 0, 0);
	}
	{ // BGR
		CMD_ExecuteCommand("SM16703P_Init 3 BGR", 0);
		uint8_t dat[9] = { 255, 0, 0,
			0, 255, 0, 
			0, 0, 255 };
		Strip_setMultiplePixel(3, dat, false);
		SELFTEST_ASSERT_PIXEL(0, 0, 0, 255);
		SELFTEST_ASSERT_PIXEL(1, 0, 255, 0);
		SELFTEST_ASSERT_PIXEL(2, 255, 0, 0);
	} 
	{ // GRB
		CMD_ExecuteCommand("SM16703P_Init 3 GRB", 0);
		uint8_t dat[9] = { 255, 0, 0,
			0, 255, 0, 
			0, 0, 255 };
		Strip_setMultiplePixel(3, dat, false);
		SELFTEST_ASSERT_PIXEL(0, 0, 255, 0);
		SELFTEST_ASSERT_PIXEL(1, 255, 0, 0);
		SELFTEST_ASSERT_PIXEL(2, 0, 0, 255);
	}
	{ // BRG
		CMD_ExecuteCommand("SM16703P_Init 3 BRG", 0);
		uint8_t dat[9] = { 255, 0, 0, 
			0, 255, 0, 
			0, 0, 255 };
		Strip_setMultiplePixel(3, dat, false);
		SELFTEST_ASSERT_PIXEL(0, 0, 255, 0);
		SELFTEST_ASSERT_PIXEL(1, 0, 0, 255); 
		SELFTEST_ASSERT_PIXEL(2, 255, 0, 0); 
	}

	CMD_ExecuteCommand("SM16703P_Init 3 RGB", 0);
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
	CMD_ExecuteCommand("startDriver DDP", 0);
	CMD_ExecuteCommand("startDriver DDPSend", 0);
	CMD_ExecuteCommand("DDP_Send 127.0.0.1 3 0 FF00AB", 0);
	SIM_WaitForDDPPacket();
	// this requires udp to work so it can pass...
	if (1) {
		SELFTEST_ASSERT_PIXEL(0, 0xFF, 0x00, 0xAB);
	}
	CMD_ExecuteCommand("DDP_Send 127.0.0.1 3 0 ABCDEF", 0);
	SIM_WaitForDDPPacket();
	if (1) {
		SELFTEST_ASSERT_PIXEL(0, 0xAB, 0xCD, 0xEF);
	}
	CMD_ExecuteCommand("DDP_Send 127.0.0.1 3 0 ABCDEFAABBCC", 0);
	SIM_WaitForDDPPacket();
	if (1) {
		SELFTEST_ASSERT_PIXEL(0, 0xAB, 0xCD, 0xEF);
		SELFTEST_ASSERT_PIXEL(1, 0xAA, 0xBB, 0xCC);
	}

	
	// fake DDP RGBW packet
	{
		byte ddpPacket[128];

		ddpPacket[2] = 0x1A;

		// data starts at offset 10
		// pixel 0
		ddpPacket[10] = 0xFF;
		ddpPacket[11] = 0xFF;
		ddpPacket[12] = 0xFF;
		ddpPacket[13] = 0xFF;
		// pixel 1
		ddpPacket[14] = 0xFF;
		ddpPacket[15] = 0x0;
		ddpPacket[16] = 0x0;
		ddpPacket[17] = 0xFF;
		// pixel 2
		ddpPacket[18] = 0xFF;
		ddpPacket[19] = 0x0;
		ddpPacket[20] = 0xFF;
		ddpPacket[21] = 0xFF;

		DDP_Parse(ddpPacket, sizeof(ddpPacket));

		SELFTEST_ASSERT_PIXEL(0, 0xFF, 0xFF, 0xFF);
		///SELFTEST_ASSERT_PIXEL(1, 0xFF, 0, 0);
		//SELFTEST_ASSERT_PIXEL(2, 0xFF, 0, 0xFF);
	}

	CMD_ExecuteCommand("SM16703P_Init 6 RGB", 0);
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

	CMD_ExecuteCommand("SM16703P_SetPixel all 123 231 132", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 123, 231, 132);
	}

	CMD_ExecuteCommand("SM16703P_SetPixel all 128 128 128", 0);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 128, 128, 128);
	}
	Strip_scaleAllPixels(128);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 64, 64, 64);
	}
	Strip_scaleAllPixels(128);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 32, 32, 32);
	}
	Strip_scaleAllPixels(64);
	for (int i = 0; i < 6; i++) {
		SELFTEST_ASSERT_PIXEL(i, 8, 8, 8);
	}

	CMD_ExecuteCommand("SM16703P_SetRaw 1 0 FF000000FF000000FF", 0);
	SELFTEST_ASSERT_PIXEL(0, 0xff, 0x00, 0x00);
	SELFTEST_ASSERT_PIXEL(1, 0x00, 0xff, 0x00);
	SELFTEST_ASSERT_PIXEL(2, 0x00, 0x00, 0xff);
	CMD_ExecuteCommand("SM16703P_SetRaw 1 0 AABBCC00BA00000023", 0);
	SELFTEST_ASSERT_PIXEL(0, 0xAA, 0xBB, 0xCC);
	SELFTEST_ASSERT_PIXEL(1, 0x00, 0xBA, 0x00);
	SELFTEST_ASSERT_PIXEL(2, 0x00, 0x00, 0x23);

	
#if ENABLE_LED_BASIC
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
#endif
}

void Test_WS2812B_misc() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver SM16703P", 0);
	CMD_ExecuteCommand("setChannel 6 60", 0);
	CMD_ExecuteCommand("SM16703P_Init $CH6", 0);
	CMD_ExecuteCommand("SM16703P_SetPixel all 255 0 128", 0);
	for(int i = 0; i < 60; i++) {
		SELFTEST_ASSERT_PIXEL(i, 255, 0, 128);
	}


}

void Test_LEDstrips() {
	Test_WS2812B_misc();
	Test_DMX_RGB();
	Test_DMX_RGBC();
	Test_DMX_RGBW();
	Test_DMX_RGBCW();
	Test_WS2812B();
	Test_WS2812B_and_PWM_CW();
	Test_WS2812B_and_PWM_White();
}

#endif
