#ifdef WINDOWS

#include "selftest_local.h".

void Test_Flags() {
	// reset whole device
	SIM_ClearOBK();
	// test flags
	// 2^6 = 64
	CMD_ExecuteCommand("flags 64", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = i == 6;
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// 2^31 = 2147483648
	CMD_ExecuteCommand("flags 2147483648", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = i == 31;
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// 2^32 = 4294967296
	CMD_ExecuteCommand("flags 4294967296", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = i == 32;
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// 2^33 = 4294967296
	CMD_ExecuteCommand("flags 8589934592", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = i == 33;
		SELFTEST_ASSERT_FLAG(i, bSet);
	}

	// 2^33+2^1+2^0 = 8589934595
	CMD_ExecuteCommand("flags 8589934595", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = (i == 33 || i == 0 || i == 1);
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// append flag 55
	CMD_ExecuteCommand("SetFlag 55 1", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = (i == 33 || i == 0 || i == 1 || i == 55);
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// append flag 63
	CMD_ExecuteCommand("SetFlag 63 1", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = (i == 33 || i == 0 || i == 1 || i == 55 || i == 63);
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// clear flag 63
	CMD_ExecuteCommand("SetFlag 55 0", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = (i == 33 || i == 0 || i == 1 || i == 63);
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
	// zero
	CMD_ExecuteCommand("flags 0", 0);
	for (int i = 0; i < 64; i++) {
		bool bSet = false;
		SELFTEST_ASSERT_FLAG(i, bSet);
	}
}


#endif
