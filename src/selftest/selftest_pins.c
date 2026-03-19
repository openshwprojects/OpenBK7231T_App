#ifdef WINDOWS

#include "selftest_local.h"

void Test_Pins() {
	// reset whole device
	SIM_ClearOBK(0);

/*
we are testing against this alias definition

const char *HAL_PIN_GetPinNameAlias(int index) {
	// some of pins have special roles
	if (index == 23)
		return "ADC3";
	if (index == 26)
		return "PWM5";
	if (index == 24)
		return "PWM4";
	if (index == 6)
		return "PWM0";
	if (index == 7)
		return "PWM1";
	if (index == 0)
		return "TXD2";
	if (index == 1)
		return "RXD2";
	if (index == 9)
		return "PWM3";
	if (index == 8)
		return "PWM2";
	if (index == 10)
		return "RXD1";
	if (index == 11)
		return "TXD1";
	return "N/A";
}

*/
//	printf("################################################################# Start Selftest PIN_FindIndexFromString() #################################################################\r\n");
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("TXD1"), 11);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("ADC3"), 23);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("TXD2"), 0);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("PWM0"), 6);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("PWM4"), 24);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("1"), 1);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("14"), 14);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("12A"), -1);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("A12"), -1);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("RXD"), -1);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("PWM"), -1);
	SELFTEST_ASSERT_INTCOMPARE(PIN_FindIndexFromString("TDX2"),-1);
//	printf("################################################################## End Selftest PIN_FindIndexFromString() ##################################################################\r\n");
}

#endif
