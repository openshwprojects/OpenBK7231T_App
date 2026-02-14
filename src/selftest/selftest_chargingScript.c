#ifdef WINDOWS

#include "selftest_local.h"

#if ENABLE_BL_SHARED

// autoexec.bat script:
// - 1 second loop
// - if power < 5W, increment counter (channel 10)
// - if power >= 5W, reset counter to 0
// - if counter >= 30, turn off relay (channel 1) and reset counter
static const char *test_chargingScript =
	"setChannelType 10 ReadOnly\r\n"
	"setChannelLabel 10 \"Countdown\"\r\n"
	"setChannel 15 123\r\n"
    "setChannel 10 0\r\n"
    "again:\r\n"
    "    delay_s 1\r\n"
    "    if $power<5 then addChannel 10 1\r\n"
    "    if $power>=5 then setChannel 10 0\r\n"
    "    if $CH10>=30 then backlog setChannel 1 0; setChannel 10 0\r\n"
    "    goto again\r\n";

void Test_ChargingScript() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("chargeDev", "bekens");

	// Setup relay on pin 9, channel 1
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	// Start simulated power measurement driver
	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.5 100 50 0", 0);

	// Upload autoexec.bat to LittleFS
	Test_FakeHTTPClientPacket_POST("api/lfs/testCharge.bat", test_chargingScript);
	// Verify it was stored
	Test_FakeHTTPClientPacket_GET("api/lfs/testCharge.bat");
	SELFTEST_ASSERT_HTML_REPLY(test_chargingScript);

	SVM_FreeAllFiles();
	// Start the script
	CMD_ExecuteCommand("startScript testCharge.bat", 0);

	Sim_RunSeconds(1.0f, false);
	// Turn relay ON
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(15, 123);

	// =========================================================
	// TEST 1: High power (100W) - counter should stay at 0,
	//         relay should remain ON
	// =========================================================
	CMD_ExecuteCommand("SetupTestPower 230 0.5 100 50 0", 0);
	for (int i = 0; i < 35; i++) {
	Sim_RunSeconds(1.0f, false);
	// Counter should be 0 because power >= 5W
	SELFTEST_ASSERT_CHANNEL(10, 0);
	// Relay should still be ON
	SELFTEST_ASSERT_CHANNEL(1, 1);
	}

	// =========================================================
	// TEST 2: Low power (1W) - counter should increment,
	//         relay should stay ON for first 29 seconds
	// =========================================================
	CMD_ExecuteCommand("SetupTestPower 230 0.004 1 50 0", 0);
	for (int i = 0; i < 29; i++) {
		Sim_RunSeconds(1.0f, false);
	}
	// Counter should be around 29
	int counter = CHANNEL_Get(10);
	printf("After 29s low power, counter = %i\n", counter);
	SELFTEST_ASSERT(counter > 27 && counter <= 29);
	// Relay should still be ON
	SELFTEST_ASSERT_CHANNEL(1, 1);

	// =========================================================
	// TEST 3: Continue low power - after total 30+ seconds,
	//         relay should turn OFF
	// =========================================================
	for (int i = 0; i < 5; i++) {
		Sim_RunSeconds(1.0f, false);
	}
	// Counter should have been reset by the script after hitting 30
	// Relay should be OFF
	printf("After 34s total low power, relay = %i, counter = %i\n",
	CHANNEL_Get(1), CHANNEL_Get(10));
	SELFTEST_ASSERT_CHANNEL(1, 0);

	// =========================================================
	// TEST 4: Recovery - high power again, turn relay back ON,
	//         counter should reset to 0
	// =========================================================
	CMD_ExecuteCommand("SetupTestPower 230 0.5 100 50 0", 0);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	for (int i = 0; i < 10; i++) {
		Sim_RunSeconds(1.0f, false);
	}
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);

	// =========================================================
	// TEST 5: Second low-power cycle - relay should turn OFF
	//         again after 30+ seconds
	// =========================================================
	CMD_ExecuteCommand("SetupTestPower 230 0.004 1 50 0", 0);
	for (int i = 0; i < 35; i++) {
		Sim_RunSeconds(1.0f, false);
	}
	printf("After second low power cycle, relay = %i, counter = %i\n",
	CHANNEL_Get(1), CHANNEL_Get(10));
	SELFTEST_ASSERT_CHANNEL(1, 0);

	SIM_ClearMQTTHistory();
}

#endif
#endif
