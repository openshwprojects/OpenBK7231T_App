#ifdef WINDOWS

#include "selftest_local.h"

void Test_WaitFor_MQTTState() {
	int i;
	char buffer[64];

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor MQTTState 1\n"
		"setChannel 1 123\n"
		"setChannel 2 234\n");

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("startScript testScript.txt", 0);

	for (i = 0; i < 10; i++) {
		 SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_MQTT_STATE,1);

	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);

}

void Test_WaitFor_NoPingTime() {
	int i;
	char buffer[64];

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime 12\n"
		"setChannel 1 123\n"
		"setChannel 2 234\n");

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("startScript testScript.txt", 0);

	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	int fakeVal = 0;
	while(fakeVal < 12) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);
		fakeVal++;
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);
	fakeVal++;
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);

	while (fakeVal < 24) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);
		fakeVal++;
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_NoPingTime2() {
	int i;
	char buffer[64];

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime 56\n"
		"setChannel 1 123\n"
		"setChannel 2 234\n");

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("startScript testScript.txt", 0);

	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 55);

	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 57);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 50);
	SELFTEST_ASSERT_CHANNEL(2, 75);

	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 56);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor() {
	Test_WaitFor_MQTTState();
	Test_WaitFor_NoPingTime();
	Test_WaitFor_NoPingTime2();
}

#endif
