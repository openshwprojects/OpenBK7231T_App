#ifdef WINDOWS

#include "selftest_local.h"

void Test_WaitFor_MQTTState() {
	int i;

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
void Test_WaitFor_OperatorMore() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime > 44\n"
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

	for (int fakeVal = 0; fakeVal < 45; fakeVal++) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes 45, and 45 is > than 44
	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 45);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_OperatorMore2() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime > 44\n"
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

	for (int fakeVal = 0; fakeVal < 45; fakeVal++) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes suddenly 202401, and 202401 is > than 44
	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 202401);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_OperatorLess() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime < 44\n"
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

	for (int fakeVal = 44; fakeVal < 124; fakeVal++) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes 43, and 43 is < than 44
	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 43);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_OperatorLess2() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime < 44\n"
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

	for (int fakeVal = 44; fakeVal < 124; fakeVal++) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, fakeVal);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes -1234, and -1234 is < than 44
	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, -1234);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_OperatorNotEqual() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor NoPingTime ! 44\n"
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

	for (int ii = 0; ii < 10; ii++) {
		CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 44);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes 45, and 45 is not 44
	CMD_Script_ProcessWaitersForEvent(CMD_EVENT_CHANGE_NOPINGTIME, 45);
	for (i = 0; i < 10; i++) {
		SVM_RunThreads(5);
	}
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
}
void Test_WaitFor_ChannelValue() {

	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/testScript.txt",
		"setChannel 1 50\n"
		"setChannel 2 75\n"
		"waitFor Channel5 55\n"
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

	for (int ii = 0; ii < 10; ii++) {
		CMD_ExecuteCommand("setChannel 5 45", 0);

		for (i = 0; i < 10; i++) {
			SVM_RunThreads(5);
		}
		SELFTEST_ASSERT_CHANNEL(1, 50);
		SELFTEST_ASSERT_CHANNEL(2, 75);
	}
	// now it becomes 55 and condition is met
	CMD_ExecuteCommand("setChannel 5 55", 0);
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
	Test_WaitFor_OperatorMore();
	Test_WaitFor_OperatorMore2();
	Test_WaitFor_OperatorLess();
	Test_WaitFor_OperatorLess2();
	Test_WaitFor_OperatorNotEqual();
	Test_WaitFor_ChannelValue();
}

#endif
