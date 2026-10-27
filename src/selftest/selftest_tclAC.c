#ifdef WINDOWS

#include "selftest_local.h"

extern void TCL_UART_TryToGetNextPacket(void);
extern void TCL_UART_RunEverySecond(void);

static const char *valid_status_reply =
	"BB 01 00 04 2D 04 00 11 21 00 00 00 FF 00 00 00 00 1D CA FF 00 00 00 00 00 F0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 7F";

static const char *invalid_status_reply =
	"BB 01 00 04 2D 04 00 11 21 00 00 00 FF 00 00 00 00 1D CA FF 00 00 00 00 00 F0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";

void Test_Driver_TCL_AC_InvalidReplyDoesNotBlockPolling() {
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);
	SIM_ClearUART();
	SIM_UART_InitReceiveRingBuffer(256);

	CMD_ExecuteCommand("startDriver TCL", 0);

	CMD_ExecuteCommand(va("uartFakeHex %s", invalid_status_reply), 0);
	TCL_UART_TryToGetNextPacket();
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	TCL_UART_RunEverySecond();
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
}

void Test_Driver_TCL_AC() {
	Test_Driver_TCL_AC_InvalidReplyDoesNotBlockPolling();

	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);
	SIM_ClearUART();
	// Required to set up the receive ring buffer before feeding bytes with uartFakeHex
	SIM_UART_InitReceiveRingBuffer(256);

	CMD_ExecuteCommand("startDriver TCL", 0);

	// ACMode 1 = COOL mode: builds a 35-byte set command and flags it for sending
	CMD_ExecuteCommand("ACMode 1", 0);
	TCL_UART_RunEverySecond();
	// TCL set command is always 35 bytes — verify bytes were sent
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SIM_ClearUART();

	// FANMode 1 = FAN_1: rebuilds and queues another 35-byte set command
	CMD_ExecuteCommand("FANMode 1", 0);
	TCL_UART_RunEverySecond();
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
	SIM_ClearUART();

	// Feed a valid get-response packet into the driver UART buffer and parse it directly.
	// Then run the normal every-second path once to ensure the driver continues polling.
	CMD_ExecuteCommand(va("uartFakeHex %s", valid_status_reply), 0);
	TCL_UART_TryToGetNextPacket();
	// The fake response should be fully consumed by the parser.
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	TCL_UART_RunEverySecond();
	// After parsing, the regular poll path should still send the next request frame.
	SELFTEST_ASSERT_HAS_SOME_DATA_IN_UART();
}



#endif

