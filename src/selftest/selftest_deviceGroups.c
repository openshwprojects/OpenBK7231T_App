#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_local.h"
#include "../devicegroups/deviceGroups_public.h"
#include "../bitmessage/bitmessage_public.h"

void DGR_AddToSendQueue(byte *data, int len);
void DGR_FlushSendQueue(void);
int DGR_GetPendingPacketCountForTest(void);
int DGR_GetPendingPacketByteForTest(int packetIndex, int byteIndex);
int DGR_GetMemberCountForTest(void);
int DGR_IsTimeReachedForTest(uint32_t now, uint32_t deadline);
int DGR_IsSequenceNewerForTest(uint16_t sequence, uint16_t previous);
uint32_t DGR_TicksToMillisecondsForTest(uint32_t ticks);
void SIM_SendFakeDGRPowerPacketToSelf(const char *groupName, int seq, int powerBits, int powerCount);

static int sim_fakeSeq = 1;
static int dgrTestFade;
static int dgrTestSpeed;
static int dgrTestScheme;
static int dgrTestLow;
static int dgrTestHigh;

static void DGR_TestFade(byte value) { dgrTestFade = value; }
static void DGR_TestSpeed(byte value) { dgrTestSpeed = value; }
static void DGR_TestScheme(byte value) { dgrTestScheme = value; }
static void DGR_TestLow(byte value) { dgrTestLow = value; }
static void DGR_TestHigh(byte value) { dgrTestHigh = value; }

static void Test_DeviceGroups_BoundedStrings(void) {
	bitMessage_t msg;
	byte valid[] = { 'g', 'r', 'p', 0 };
	byte tooLong[] = { '1', '2', '3', '4', '5', 0 };
	byte unterminated[] = { 'b', 'a', 'd' };
	char output[5];

	MSG_BeginReading(&msg, valid, sizeof(valid));
	SELFTEST_ASSERT(MSG_ReadString(&msg, output, sizeof(output)) == 3);
	SELFTEST_ASSERT(strcmp(output, "grp") == 0);

	MSG_BeginReading(&msg, tooLong, sizeof(tooLong));
	SELFTEST_ASSERT(MSG_ReadString(&msg, output, sizeof(output)) == -1);
	SELFTEST_ASSERT(output[0] == 0);

	MSG_BeginReading(&msg, unterminated, sizeof(unterminated));
	SELFTEST_ASSERT(MSG_ReadString(&msg, output, sizeof(output)) == -1);
	SELFTEST_ASSERT(output[0] == 0);
}

static void Test_DeviceGroups_TruncatedItems(void) {
	byte buffer[64];
	bitMessage_t msg;
	dgrDevice_t device;
	struct sockaddr_in source;
	int len;

	memset(&device, 0, sizeof(device));
	memset(&source, 0, sizeof(source));
	strcpy_safe(device.gr.groupName, "parse-test", sizeof(device.gr.groupName));

	MSG_BeginWriting(&msg, buffer, sizeof(buffer));
	MSG_WriteBytes(&msg, "TASMOTA_DGR", 11);
	MSG_WriteString(&msg, "parse-test");
	MSG_WriteU16(&msg, 1);
	MSG_WriteU16(&msg, 0);
	MSG_WriteByte(&msg, 128); /* DGR_ITEM_POWER without its four-byte value */
	len = msg.position;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == -2);

	MSG_BeginWriting(&msg, buffer, sizeof(buffer));
	MSG_WriteBytes(&msg, "TASMOTA_DGR", 11);
	MSG_WriteString(&msg, "parse-test");
	MSG_WriteU16(&msg, 2);
	MSG_WriteU16(&msg, 0);
	MSG_WriteByte(&msg, 193); /* DGR_ITEM_COMMAND */
	MSG_WriteByte(&msg, 10);  /* Declared data is longer than the packet. */
	MSG_WriteByte(&msg, 'x');
	len = msg.position;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == -2);

	MSG_BeginWriting(&msg, buffer, sizeof(buffer));
	MSG_WriteBytes(&msg, "TASMOTA_DGR", 11);
	MSG_WriteString(&msg, "parse-test");
	MSG_WriteU16(&msg, 3);
	MSG_WriteU16(&msg, 0);
	MSG_WriteByte(&msg, 128); /* DGR_ITEM_POWER */
	MSG_Write3Bytes(&msg, 0);
	MSG_WriteByte(&msg, 25);  /* Power payload contains only 24 state bits. */
	MSG_WriteByte(&msg, 0);
	len = msg.position;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == -2);
}

static void Test_DeviceGroups_QueueSaturation(void) {
	byte packet[] = { 0, 2, 3 };
	int i;

	DGR_FlushSendQueue();
	for (i = 0; i < 8; i++) {
		packet[0] = i;
		DGR_AddToSendQueue(packet, sizeof(packet));
	}
	SELFTEST_ASSERT(DGR_GetPendingPacketCountForTest() == 8);
	for (i = 0; i < 8; i++) {
		SELFTEST_ASSERT(DGR_GetPendingPacketByteForTest(i, 0) == i);
	}
	DGR_AddToSendQueue(packet, sizeof(packet));
	SELFTEST_ASSERT(DGR_GetPendingPacketCountForTest() == 8);
	DGR_FlushSendQueue();
	SELFTEST_ASSERT(DGR_GetPendingPacketCountForTest() == 0);
	DGR_AddToSendQueue(packet, sizeof(packet));
	SELFTEST_ASSERT(DGR_GetPendingPacketCountForTest() == 1);
	DGR_FlushSendQueue();
}

static void Test_DeviceGroups_FullStatusFormat(void) {
	byte buffer[128];
	byte rgbcw[5] = { 1, 2, 3, 4, 5 };
	bitMessage_t msg;
	char groupName[32];
	int len;

	len = DGR_Quick_FormatFullStatus(buffer, sizeof(buffer), "status-test", 42,
		5, 3, DGR_SHARE_POWER | DGR_SHARE_LIGHT_BRI | DGR_SHARE_LIGHT_SCHEME | DGR_SHARE_LIGHT_COLOR,
		0, 127, 2, rgbcw);
	SELFTEST_ASSERT(len > 0);
	MSG_BeginReading(&msg, buffer, len);
	SELFTEST_ASSERT(MSG_CheckAndSkip(&msg, "TASMOTA_DGR", 11) == 11);
	SELFTEST_ASSERT(MSG_ReadString(&msg, groupName, sizeof(groupName)) == 11);
	SELFTEST_ASSERT(strcmp(groupName, "status-test") == 0);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 42);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 4);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 129);
	SELFTEST_ASSERT(MSG_Read3Bytes(&msg) == 0);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 0);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 128);
	SELFTEST_ASSERT(MSG_Read3Bytes(&msg) == 5);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 3);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 5);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 127);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 6);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 2);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 224);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 6);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 1);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 2);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 3);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 4);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 5);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) > 0);
	SELFTEST_ASSERT(MSG_ReadByte(&msg) == 0);
}

static void Test_DeviceGroups_HeaderOnlyMessages(void) {
	byte buffer[64];
	bitMessage_t msg;
	char groupName[32];
	int len;

	len = DGR_Quick_FormatACK(buffer, sizeof(buffer), "header-test", 7);
	MSG_BeginReading(&msg, buffer, len);
	SELFTEST_ASSERT(MSG_CheckAndSkip(&msg, "TASMOTA_DGR", 11) == 11);
	SELFTEST_ASSERT(MSG_ReadString(&msg, groupName, sizeof(groupName)) == 11);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 7);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 8);
	SELFTEST_ASSERT(MSG_EOF(&msg));

	len = DGR_Quick_FormatAnnouncement(buffer, sizeof(buffer), "header-test", 7);
	MSG_BeginReading(&msg, buffer, len);
	SELFTEST_ASSERT(MSG_CheckAndSkip(&msg, "TASMOTA_DGR", 11) == 11);
	SELFTEST_ASSERT(MSG_ReadString(&msg, groupName, sizeof(groupName)) == 11);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 7);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 64);
	SELFTEST_ASSERT(MSG_EOF(&msg));

	len = DGR_Quick_FormatStatusRequestWithFlags(buffer, sizeof(buffer), "header-test", 7,
		1 | 2);
	MSG_BeginReading(&msg, buffer, len);
	SELFTEST_ASSERT(MSG_CheckAndSkip(&msg, "TASMOTA_DGR", 11) == 11);
	SELFTEST_ASSERT(MSG_ReadString(&msg, groupName, sizeof(groupName)) == 11);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 7);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 3);
	SELFTEST_ASSERT(MSG_EOF(&msg));
}

static void Test_DeviceGroups_CommandFormatAndExtendedItems(void) {
	byte buffer[128];
	bitMessage_t msg;
	dgrDevice_t device;
	struct sockaddr_in source;
	uint32_t noStatusShare = 0;
	char groupName[32];
	int len;

	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "command-test", 55, 0,
		"3=1 4=9 6=2 8=20 9=80 192=hello\\ world 224=#010203040506");
	SELFTEST_ASSERT(len > 0);
	MSG_BeginReading(&msg, buffer, len);
	SELFTEST_ASSERT(MSG_CheckAndSkip(&msg, "TASMOTA_DGR", 11) == 11);
	SELFTEST_ASSERT(MSG_ReadString(&msg, groupName, sizeof(groupName)) == 12);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 55);
	SELFTEST_ASSERT(MSG_ReadU16(&msg) == 0);

	memset(&device, 0, sizeof(device));
	memset(&source, 0, sizeof(source));
	strcpy_safe(device.gr.groupName, "command-test", sizeof(device.gr.groupName));
	device.gr.devGroupShare_In = 0xFFFFFFFF;
	device.gr.noStatusShare = &noStatusShare;
	device.cbs.processLightFade = DGR_TestFade;
	device.cbs.processLightSpeed = DGR_TestSpeed;
	device.cbs.processLightScheme = DGR_TestScheme;
	device.cbs.processBrightnessPresetLow = DGR_TestLow;
	device.cbs.processBrightnessPresetHigh = DGR_TestHigh;
	dgrTestFade = dgrTestSpeed = dgrTestScheme = dgrTestLow = dgrTestHigh = 0;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == 0);
	SELFTEST_ASSERT(dgrTestFade == 1);
	SELFTEST_ASSERT(dgrTestSpeed == 9);
	SELFTEST_ASSERT(dgrTestScheme == 2);
	SELFTEST_ASSERT(dgrTestLow == 20);
	SELFTEST_ASSERT(dgrTestHigh == 80);
}

static void Test_DeviceGroups_NoShareItemFlag(void) {
	byte buffer[128];
	dgrDevice_t device;
	struct sockaddr_in source;
	uint32_t noStatusShare = 0;
	int len;
	memset(&device, 0, sizeof(device));
	memset(&source, 0, sizeof(source));
	strcpy_safe(device.gr.groupName, "noshare-test", sizeof(device.gr.groupName));
	device.gr.devGroupShare_In = DGR_SHARE_LIGHT_BRI;
	device.gr.noStatusShare = &noStatusShare;
	device.cbs.processLightBrightness = DGR_TestFade;
	dgrTestFade = 0;
	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "noshare-test", 1, 0, "5=N77");
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == 0);
	SELFTEST_ASSERT(dgrTestFade == 0);
	SELFTEST_ASSERT(noStatusShare & DGR_SHARE_LIGHT_BRI);
	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "noshare-test", 2, 0, "5=88");
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == 0);
	SELFTEST_ASSERT(dgrTestFade == 88);
	SELFTEST_ASSERT((noStatusShare & DGR_SHARE_LIGHT_BRI) == 0);
}

static void Test_DeviceGroups_CommandStateIsPerGroup(void) {
	byte buffer[128];
	dgrDevice_t device;
	struct sockaddr_in source;
	uint32_t noStatusShare = 0;
	int len;

	memset(&device, 0, sizeof(device));
	memset(&source, 0, sizeof(source));
	strcpy_safe(device.gr.groupName, "state-test", sizeof(device.gr.groupName));
	device.gr.devGroupShare_In = DGR_SHARE_LIGHT_BRI;
	device.gr.noStatusShare = &noStatusShare;
	device.cbs.processLightBrightness = DGR_TestFade;

	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "state-test", 1, 0, "5=10");
	SELFTEST_ASSERT(len > 0);
	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "state-test", 1, 1, "5=20");
	SELFTEST_ASSERT(len > 0);

	device.gr.stateIndex = 0;
	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "state-test", 2, 0, "5=@+1");
	dgrTestFade = 0;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == 0);
	SELFTEST_ASSERT(dgrTestFade == 11);

	device.gr.stateIndex = 1;
	len = DGR_Quick_FormatCommand(buffer, sizeof(buffer), "state-test", 2, 1, "5=@+1");
	dgrTestFade = 0;
	SELFTEST_ASSERT(DGR_Parse(buffer, len, &device, (struct sockaddr *)&source) == 0);
	SELFTEST_ASSERT(dgrTestFade == 21);
}

static void Test_DeviceGroups_MultipleGroupsAndTies(void) {
	SIM_ClearOBK(0);
	CFG_DeviceGroups_SetNameByIndex(0, "relay-one");
	CFG_DeviceGroups_SetNameByIndex(1, "relay-two");
	CFG_DeviceGroups_SetNameByIndex(2, "");
	CFG_DeviceGroups_SetNameByIndex(3, "");
	CFG_DeviceGroups_SetTie(0, 1);
	CFG_DeviceGroups_SetTie(1, 2);
	CFG_DeviceGroups_SetRecvFlags(DGR_SHARE_POWER);
	CFG_DeviceGroups_SetSendFlags(DGR_SHARE_POWER);
	SELFTEST_ASSERT(strcmp(CFG_DeviceGroups_GetNameByIndex(0), "relay-one") == 0);
	SELFTEST_ASSERT(strcmp(CFG_DeviceGroups_GetNameByIndex(1), "relay-two") == 0);
	SELFTEST_ASSERT(CFG_DeviceGroups_GetTie(0) == 1);
	SELFTEST_ASSERT(CFG_DeviceGroups_GetTie(1) == 2);
	SELFTEST_ASSERT(CFG_DeviceGroups_GetCount() == 2);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);
	CHANNEL_Set(1, 0, 0);
	CHANNEL_Set(2, 0, 0);
	CMD_ExecuteCommand("startDriver DGR", 0);
	SIM_SendFakeDGRPowerPacketToSelf("relay-two", 10, 1, 1);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	/* Per-group sequence tracking must accept the same sequence from the same IP. */
	SIM_SendFakeDGRPowerPacketToSelf("relay-one", 10, 1, 1);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
}

static void Test_DeviceGroups_ACKGroupFiltering(void) {
	byte buffer[64];
	int initialMembers;
	int len;

	SIM_ClearOBK(0);
	CFG_DeviceGroups_SetName("ack-test");
	CMD_ExecuteCommand("startDriver DGR", 0);
	DGR_SpoofNextDGRPacketSource("192.168.0.124");
	initialMembers = DGR_GetMemberCountForTest();
	len = DGR_Quick_FormatACK(buffer, sizeof(buffer), "another-group", 10);
	DGR_ProcessIncomingPacket((char*)buffer, len);
	SELFTEST_ASSERT(DGR_GetMemberCountForTest() == initialMembers);
	len = DGR_Quick_FormatACK(buffer, sizeof(buffer), "ACK-test", 10);
	DGR_ProcessIncomingPacket((char*)buffer, len);
	SELFTEST_ASSERT(DGR_GetMemberCountForTest() == initialMembers);
	len = DGR_Quick_FormatACK(buffer, sizeof(buffer), "ack-test", 10);
	DGR_ProcessIncomingPacket((char*)buffer, len);
	SELFTEST_ASSERT(DGR_GetMemberCountForTest() == initialMembers + 1);
}

static void Test_DeviceGroups_WrapSafeTimer(void) {
	SELFTEST_ASSERT(DGR_IsTimeReachedForTest(100, 100));
	SELFTEST_ASSERT(DGR_IsTimeReachedForTest(101, 100));
	SELFTEST_ASSERT(!DGR_IsTimeReachedForTest(99, 100));
	SELFTEST_ASSERT(DGR_IsTimeReachedForTest(5, 0xFFFFFFF0));
	SELFTEST_ASSERT(!DGR_IsTimeReachedForTest(0xFFFFFFF0, 5));
	SELFTEST_ASSERT(DGR_TicksToMillisecondsForTest(10) == 10 * portTICK_PERIOD_MS);
}

static void Test_DeviceGroups_WrapSafeSequence(void) {
	SELFTEST_ASSERT(DGR_IsSequenceNewerForTest(101, 100));
	SELFTEST_ASSERT(!DGR_IsSequenceNewerForTest(100, 100));
	SELFTEST_ASSERT(!DGR_IsSequenceNewerForTest(99, 100));
	SELFTEST_ASSERT(DGR_IsSequenceNewerForTest(1, 0xFFFF));
}

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
	Test_DeviceGroups_BoundedStrings();
	Test_DeviceGroups_TruncatedItems();
	Test_DeviceGroups_FullStatusFormat();
	Test_DeviceGroups_HeaderOnlyMessages();
	Test_DeviceGroups_CommandFormatAndExtendedItems();
	Test_DeviceGroups_NoShareItemFlag();
	Test_DeviceGroups_CommandStateIsPerGroup();
	Test_DeviceGroups_MultipleGroupsAndTies();
	Test_DeviceGroups_ACKGroupFiltering();
	Test_DeviceGroups_WrapSafeTimer();
	Test_DeviceGroups_WrapSafeSequence();

	Test_DeviceGroups_TwoRelays();
	Test_DeviceGroups_RGB();
	Test_DeviceGroups_QueueSaturation();

}


#endif
