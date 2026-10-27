#ifdef WINDOWS

#include "selftest_local.h"

void Test_DHT() {
	// reset whole device
	SIM_ClearOBK(0);

	PIN_SetPinRoleForPinIndex(9, IOR_DHT11);
	PIN_SetPinChannelForPinIndex(9, 1);
	PIN_SetPinChannel2ForPinIndex(9, 2);

	Sim_RunSeconds(5.0f, false);

	SELFTEST_ASSERT_CHANNEL(1, 190);
	SELFTEST_ASSERT_CHANNEL(2, 67);

	PIN_SetPinRoleForPinIndex(9, IOR_None);
	PIN_SetPinChannelForPinIndex(9, 1);
	PIN_SetPinChannel2ForPinIndex(9, 2);

	Sim_RunSeconds(5.0f, false);
	// After removing the role the driver should no longer update the channels.
	// They must remain at the last values sampled while the DHT11 role was active.
	SELFTEST_ASSERT_CHANNEL(1, 190);
	SELFTEST_ASSERT_CHANNEL(2, 67);
}


#endif
