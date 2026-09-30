#ifdef WINDOWS

#include "selftest_local.h"

#if ENABLE_TUYAMCU_OTA

#include "../littlefs/our_lfs.h"
#include "../driver/drv_tuyaMCU_ota.h"

// Firmware update of the TuyaMCU (tuyaMcu_otaStart) against a scripted MCU

#define OTA_TEST_IMAGE_SIZE 600

static byte g_otaImage[OTA_TEST_IMAGE_SIZE];

static void Test_OTA_WriteImage(const char *name, int size) {
	lfs_file_t file;
	int i;

	for (i = 0; i < size; i++) {
		// includes 0x00, 0x55 and 0xAA bytes
		g_otaImage[i] = (byte)(i * 37 + 5);
	}
	memset(&file, 0, sizeof(file));
	SELFTEST_ASSERT(lfs_file_open(&lfs, &file, name, LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC) >= 0);
	SELFTEST_ASSERT(lfs_file_write(&lfs, &file, g_otaImage, size) == size);
	lfs_file_close(&lfs, &file);
}

// Take frames sent by OBK off the simulated UART until one with the given
// command is found; other frames (heartbeats, queries) are discarded.
// Returns the payload length, or -1 if no such frame was sent.
static int Test_OTA_PopFrame(byte cmd, byte *payload, int maxLen) {
	while (SIM_UART_GetDataSize() > 0) {
		int avail = SIM_UART_GetDataSize();
		int len, total, i;
		byte sum;

		if (SIM_UART_GetByte(0) != 0x55 || avail < 2 || SIM_UART_GetByte(1) != 0xAA) {
			SIM_UART_ConsumeBytes(1);
			continue;
		}
		if (avail < 7) {
			return -1;
		}
		len = (SIM_UART_GetByte(4) << 8) | SIM_UART_GetByte(5);
		total = len + 7;
		if (avail < total) {
			return -1;
		}
		sum = 0;
		for (i = 0; i < total - 1; i++) {
			sum += SIM_UART_GetByte(i);
		}
		SELFTEST_ASSERT(sum == SIM_UART_GetByte(total - 1));
		if (SIM_UART_GetByte(3) == cmd) {
			SELFTEST_ASSERT(len <= maxLen);
			for (i = 0; i < len; i++) {
				payload[i] = SIM_UART_GetByte(6 + i);
			}
			SIM_UART_ConsumeBytes(total);
			return len;
		}
		SIM_UART_ConsumeBytes(total);
	}
	return -1;
}

// Feed a version-3 frame from the "MCU" into the TuyaMCU driver
static void Test_OTA_Reply(byte cmd, const byte *payload, int len) {
	char buffer[512];
	byte frame[64];
	int n = 0, i;
	byte sum = 0;

	frame[n++] = 0x55;
	frame[n++] = 0xAA;
	frame[n++] = 0x03;
	frame[n++] = cmd;
	frame[n++] = (byte)(len >> 8);
	frame[n++] = (byte)len;
	for (i = 0; i < len; i++) {
		frame[n++] = payload[i];
	}
	for (i = 0; i < n; i++) {
		sum += frame[i];
	}
	frame[n++] = sum;

	strcpy(buffer, "uartFakeHex ");
	for (i = 0; i < n; i++) {
		sprintf(buffer + strlen(buffer), "%02X ", frame[i]);
	}
	CMD_ExecuteCommand(buffer, 0);
	Sim_RunFrames(5, false);
}

static uint32_t Test_OTA_BE32(const byte *p) {
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

// Expect a data packet for [offset, offset + expectedLen) with the image bytes
static void Test_OTA_ExpectPacket(int offset, int expectedLen) {
	byte payload[4 + 1024];
	int len;

	Sim_RunFrames(5, false);
	len = Test_OTA_PopFrame(0x0B, payload, sizeof(payload));
	SELFTEST_ASSERT(len == 4 + expectedLen);
	SELFTEST_ASSERT(Test_OTA_BE32(payload) == (uint32_t)offset);
	SELFTEST_ASSERT(memcmp(payload + 4, g_otaImage + offset, expectedLen) == 0);
}

static void Test_OTA_Ack() {
	Test_OTA_Reply(0x0B, NULL, 0);
}

static void Test_OTA_Setup() {
	SIM_ClearOBK(0);
	SIM_UART_InitReceiveRingBuffer(4096);
	CMD_ExecuteCommand("lfs_format", 0);
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);
	Test_OTA_WriteImage("mcu.bin", OTA_TEST_IMAGE_SIZE);
	SIM_UART_ConsumeBytes(SIM_UART_GetDataSize());
}

// Start the update and answer 0x0A with the given packet size code
static void Test_OTA_Begin(byte sizeCode) {
	byte payload[16];

	CMD_ExecuteCommand("tuyaMcu_otaStart mcu.bin", 0);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "starting");
	SELFTEST_ASSERT(Test_OTA_PopFrame(0x0A, payload, sizeof(payload)) == 4);
	SELFTEST_ASSERT(Test_OTA_BE32(payload) == OTA_TEST_IMAGE_SIZE);
	Test_OTA_Reply(0x0A, &sizeCode, 1);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "sending");
}

static void Test_OTA_ExpectEndMarker() {
	byte payload[16];

	Sim_RunFrames(5, false);
	SELFTEST_ASSERT(Test_OTA_PopFrame(0x0B, payload, sizeof(payload)) == 4);
	SELFTEST_ASSERT(Test_OTA_BE32(payload) == OTA_TEST_IMAGE_SIZE);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "waiting for MCU restart");
}

void Test_TuyaMCU_OTA() {
	byte one = 1, zero = 0;
	byte payload[16];

	// Complete update with 256-byte packets: 256 + 256 + 88
	Test_OTA_Setup();
	Test_OTA_Begin(0);
	Test_OTA_ExpectPacket(0, 256);
	Test_OTA_Ack();
	Test_OTA_ExpectPacket(256, 256);
	Test_OTA_Ack();
	Test_OTA_ExpectPacket(512, 88);
	Test_OTA_Ack();
	Test_OTA_ExpectEndMarker();
	// A heartbeat reply of 1 is not a restart
	Test_OTA_Reply(0x00, &one, 1);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "waiting for MCU restart");
	// The first heartbeat after the MCU restarted is answered with 0
	Test_OTA_Reply(0x00, &zero, 1);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "done");
	// Progress on the main page, as polled by external tools
	Test_FakeHTTPClientPacket_GET("index?state=1");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("MCU update: done, 600/600 bytes (100%)");

	// MCU asks for 512-byte packets: 512 + 88
	Test_OTA_Setup();
	Test_OTA_Begin(1);
	Test_OTA_ExpectPacket(0, 512);
	Test_OTA_Ack();
	Test_OTA_ExpectPacket(512, 88);
	Test_OTA_Ack();
	Test_OTA_ExpectEndMarker();
	Test_OTA_Reply(0x00, &zero, 1);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "done");

	// A lost ack: the same packet is sent again
	Test_OTA_Setup();
	Test_OTA_Begin(0);
	Test_OTA_ExpectPacket(0, 256);
	Sim_RunSeconds(3.5f, false);
	Test_OTA_ExpectPacket(0, 256);
	Test_OTA_Ack();
	Test_OTA_ExpectPacket(256, 256);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "sending");

	// The MCU stops answering: failed after three retries
	Test_OTA_Setup();
	Test_OTA_Begin(0);
	Test_OTA_ExpectPacket(0, 256);
	Sim_RunSeconds(15.0f, false);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "failed");
	SELFTEST_ASSERT(!TuyaMCU_OTA_IsActive());

	// No answer to the upgrade start
	Test_OTA_Setup();
	CMD_ExecuteCommand("tuyaMcu_otaStart mcu.bin", 0);
	Sim_RunSeconds(11.0f, false);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "failed");

	// Abort in the middle; a late ack is no longer taken as OTA traffic
	Test_OTA_Setup();
	Test_OTA_Begin(0);
	Test_OTA_ExpectPacket(0, 256);
	CMD_ExecuteCommand("tuyaMcu_otaAbort", 0);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "failed");
	Test_OTA_Ack();
	Sim_RunFrames(5, false);
	SELFTEST_ASSERT(Test_OTA_PopFrame(0x0B, payload, sizeof(payload)) == -1);

	// All data sent but the MCU never restarts
	Test_OTA_Setup();
	Test_OTA_Begin(1);
	Test_OTA_ExpectPacket(0, 512);
	Test_OTA_Ack();
	Test_OTA_ExpectPacket(512, 88);
	Test_OTA_Ack();
	Test_OTA_ExpectEndMarker();
	Sim_RunSeconds(21.0f, false);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "failed");

	// Unsupported packet size code
	Test_OTA_Setup();
	CMD_ExecuteCommand("tuyaMcu_otaStart mcu.bin", 0);
	SELFTEST_ASSERT(Test_OTA_PopFrame(0x0A, payload, sizeof(payload)) == 4);
	payload[0] = 7;
	Test_OTA_Reply(0x0A, payload, 1);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "failed");

	// Only one update at a time
	Test_OTA_Setup();
	Test_OTA_Begin(0);
	SELFTEST_ASSERT(CMD_ExecuteCommand("tuyaMcu_otaStart mcu.bin", 0) != CMD_RES_OK);
	SELFTEST_ASSERT_STRING(TuyaMCU_OTA_GetStateName(), "sending");
	CMD_ExecuteCommand("tuyaMcu_otaAbort", 0);

	// Missing file
	Test_OTA_Setup();
	SELFTEST_ASSERT(CMD_ExecuteCommand("tuyaMcu_otaStart nosuchfile.bin", 0) != CMD_RES_OK);
	SELFTEST_ASSERT(!TuyaMCU_OTA_IsActive());
}

#else

void Test_TuyaMCU_OTA() {
}

#endif // ENABLE_TUYAMCU_OTA

#endif // WINDOWS
