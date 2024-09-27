#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_local.h"
#include "../devicegroups/deviceGroups_public.h"

static int sim_fakeSeq = 1;

void SIM_SendFakeDGRPowerPacketToSelf(const char *groupName, int seq, int powerBits, int powerCount) {
	byte buffer[256];
	int len;

	len = DGR_Quick_FormatPowerState(buffer, sizeof(buffer), groupName, seq, 0, powerBits, powerCount);

	DGR_SpoofNextDGRPacketSource("192.168.0.123");
	DGR_ProcessIncomingPacket((char*)buffer, len);
}

void SIM_SendFakeDGRBrightnessPacketToSelf(const char *groupName, int seq, byte brightness) {
	byte buffer[256];
	int len;

	len = DGR_Quick_FormatBrightness(buffer, sizeof(buffer), groupName, seq, 0, brightness);

	DGR_SpoofNextDGRPacketSource("192.168.0.123");
	DGR_ProcessIncomingPacket((char*)buffer, len);
}

void SIM_SendFakeDGRBrightnessPacketToSelf_Next(const char *groupName, byte brightness) {

	sim_fakeSeq++;

	SIM_SendFakeDGRBrightnessPacketToSelf(groupName, sim_fakeSeq,  brightness);
}

void SIM_SendFakeDGRPowerPacketToSelf_Next(const char *groupName, int powerBits, int powerCount) {
	
	sim_fakeSeq++;

	SIM_SendFakeDGRPowerPacketToSelf(groupName, sim_fakeSeq, powerBits, powerCount);
}

void Test_DeviceGroups_TwoRelays() {
	const char *testName = "win_dblR3l4yTst";
	// reset whole device
	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CFG_DeviceGroups_SetName(testName);
	CFG_DeviceGroups_SetRecvFlags(0);
	CFG_DeviceGroups_SetSendFlags(0);
	CMD_ExecuteCommand("startDriver DGR", 0);

	// wrong dgr name
	SIM_SendFakeDGRPowerPacketToSelf_Next("test_smOth3rGrps", 0b10, 2);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	// not enabled in flags
	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b10, 2);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CFG_DeviceGroups_SetRecvFlags(DGR_SHARE_POWER);
	CFG_DeviceGroups_SetSendFlags(0);

	// should work
	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b10, 2);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	// should work
	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b11, 2);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	// should work
	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b01, 2);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 0);
}

void Test_DeviceGroups_RGB() {
	const char *testName = "win_RGByTst";
	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	CFG_DeviceGroups_SetName(testName);
	CFG_DeviceGroups_SetRecvFlags(0);
	CFG_DeviceGroups_SetSendFlags(0);
	CMD_ExecuteCommand("startDriver DGR", 0);


	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0000", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeDGRPowerPacketToSelf_Next("test_smOth3rGrps", 0b0, 1);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b0, 1);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	CFG_DeviceGroups_SetRecvFlags(DGR_SHARE_POWER);
	CFG_DeviceGroups_SetSendFlags(0);

	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b0, 1);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeDGRPowerPacketToSelf_Next(testName, 0b1, 1);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeDGRBrightnessPacketToSelf_Next(testName, 127);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	CFG_DeviceGroups_SetRecvFlags(DGR_SHARE_POWER | DGR_SHARE_LIGHT_BRI);
	CFG_DeviceGroups_SetSendFlags(0);

	SIM_SendFakeDGRBrightnessPacketToSelf_Next(testName, 127);

	printf("R %i G %i B %i\n", CHANNEL_Get(1), CHANNEL_Get(2), CHANNEL_Get(3));

	SELFTEST_ASSERT_CHANNEL(1, 20);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeDGRBrightnessPacketToSelf_Next(testName, 255);

	printf("R %i G %i B %i\n", CHANNEL_Get(1), CHANNEL_Get(2), CHANNEL_Get(3));

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

}
void Test_DeviceGroups() {

	Test_DeviceGroups_TwoRelays();
	Test_DeviceGroups_RGB();

}


#endif
