#ifdef WINDOWS

#include "selftest_local.h"


void Test_TuyaMCU_BatteryPowered_Style1() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_UART_InitReceiveRingBuffer(1024);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);
	CMD_ExecuteCommand("startDriver tmSensor", 0);

	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 1 val 1", 0);
	CMD_ExecuteCommand("setChannelType 1 OpenClosed", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 3 val 3", 0);
	CMD_ExecuteCommand("setChannelType 3 Custom", 0);

	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Pressed: 1. Send query product information
	(simulated delays are disabled)
	Sent: 55 AA 00 01 00 00 00
	Sent: Ver=0, Cmd=QueryInfo, Len=0, CHECKSUM OK
	*/
	Sim_RunSeconds(3.0f, false);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 01 00 00 00");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Received: 55 AA 00 01 00 24 7B 22 70 22 3A 22 33 73 6C 70 69 72 6C 6E 6C 71 70 30 62 71 6F 31 22 2C 22 76 22 3A 22 31 2E 30 2E 30 22 7D C6
	Received: Ver=0, Cmd=QueryInfo, Len=36, JSON={"p":"3slpirlnlqp0bqo1","v":"1.0.0"}, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 01 00 24 7B 22 70 22 3A 22 33 73 6C 70 69 72 6C 6E 6C 71 70 30 62 71 6F 31 22 2C 22 76 22 3A 22 31 2E 30 2E 30 22 7D C6", 0);
	Sim_RunSeconds(2.0f, false);
	/*
	Pressed: 2. Send MCU conf (WiFiState 0x03)
	(simulated delays are disabled)
	Sent: 55 AA 00 02 00 01 03 05
	Sent: Ver=0, Cmd=QueryStatus, Len=1, CHECKSUM OK
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 02 00 01 03 05");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Received: 55 AA 00 02 00 00 01
	Received: Ver=0, Cmd=QueryStatus, Len=0, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 02 00 00 01", 0);
	Sim_RunSeconds(2.0f, false);
	Sim_RunSeconds(2.0f, false);

	/*
	Pressed: 3. Send MCU conf (WiFiState 0x04)
	(simulated delays are disabled)
	Sent: 55 AA 00 02 00 01 04 06
	Sent: Ver=0, Cmd=QueryStatus, Len=1, CHECKSUM OK
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 02 00 01 04 06");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	/*
	Received: 55 AA 00 02 00 00 01
	Received: Ver=0, Cmd=QueryStatus, Len=0, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 02 00 00 01", 0);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	Sim_RunSeconds(2.0f, false);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Received: 55 AA 00 05 00 05 03 04 00 01 02 13
	dpID 3 is battery state
	Received: Ver=0, Cmd=ReportRealtimeStatus, Len=5, [dpID=3, type=4, len=1, val=2], CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 05 00 05 03 04 00 01 02 13", 0);
	/*
	Received: 55 AA 00 05 00 05 01 01 00 01 00 0C
	dpID 1 is door state
	Received: Ver=0, Cmd=ReportRealtimeStatus, Len=5, [dpID=1, type=1, len=1, val=0], CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 05 00 05 01 01 00 01 00 0C", 0);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	Sim_RunSeconds(3.0f, false);
	// dpID 1 is door state
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// dpID 3 is battery state
	SELFTEST_ASSERT_CHANNEL(3, 2);
	Sim_RunSeconds(3.0f, false);
	/*
	Pressed: 3B. Send data accept confirmation 0x05 (will put device back to sleep)
	Waiting: 1,00
	Pressed: 3B. Send data accept confirmation 0x05 (will put device back to sleep)
	Waiting: 1,00
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 05 00 01 00 05");
	Sim_RunSeconds(3.0f, false);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 05 00 01 00 05");

	// cause error
	//SELFTEST_ASSERT_CHANNEL(15, 666);
}
void Test_TuyaMCU_BatteryPowered_Style2() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_UART_InitReceiveRingBuffer(1024);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);
	CMD_ExecuteCommand("startDriver tmSensor", 0);

	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 1 val 1", 0);
	CMD_ExecuteCommand("setChannelType 1 OpenClosed", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 3 val 3", 0);
	CMD_ExecuteCommand("setChannelType 3 Custom", 0);

	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Pressed: 1. Send query product information
	(simulated delays are disabled)
	Sent: 55 AA 00 01 00 00 00
	Sent: Ver=0, Cmd=QueryInfo, Len=0, CHECKSUM OK
	*/
	Sim_RunSeconds(3.0f, false);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 01 00 00 00");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Received: 55 AA 00 01 00 24 7B 22 70 22 3A 22 65 37 64 6E 79 38 7A 76 6D 69 79 68 71 65 72 77 22 2C 22 76 22 3A 22 31 2E 30 2E 30 22 7D 24
	Received: Ver=0, Cmd=QueryInfo, Len=36, JSON={"p":"e7dny8zvmiyhqerw","v":"1.0.0"}, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 01 00 24 7B 22 70 22 3A 22 65 37 64 6E 79 38 7A 76 6D 69 79 68 71 65 72 77 22 2C 22 76 22 3A 22 31 2E 30 2E 30 22 7D 24", 0);
	Sim_RunSeconds(2.0f, false);
	/*
	Pressed: 2. Send MCU conf (WiFiState 0x03)
	(simulated delays are disabled)
	Sent: 55 AA 00 02 00 01 03 05
	Sent: Ver=0, Cmd=QueryStatus, Len=1, CHECKSUM OK
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 02 00 01 03 05");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	/*
	Received: 55 AA 00 02 00 00 01
	Received: Ver=0, Cmd=QueryStatus, Len=0, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 02 00 00 01", 0);
	Sim_RunSeconds(2.0f, false);

	/*
	Pressed: 3. Send MCU conf (WiFiState 0x04)
	(simulated delays are disabled)
	Sent: 55 AA 00 02 00 01 04 06
	Sent: Ver=0, Cmd=QueryStatus, Len=1, CHECKSUM OK
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 02 00 01 04 06");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	/*
	Received: 55 AA 00 02 00 00 01
	Received: Ver=0, Cmd=QueryStatus, Len=0, CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 02 00 00 01", 0);
	Sim_RunSeconds(2.0f, false);

	Sim_RunSeconds(2.0f, false);
	/*
	Received: 55 AA 00 08 00 0C 00 02 02 02 02 02 02 01 01 00 01 01 23
	dpID 1 is door state
	Received: Ver=0, Cmd=QueryRecordStorage, Len=12, [dpID=1, type=1, len=1, val=1], CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 08 00 0C 00 02 02 02 02 02 02 01 01 00 01 01 23", 0);
	/*
	Received: 55 AA 00 08 00 0C 00 01 01 01 01 01 01 03 04 00 01 02 23
	dpID 3 is battery state
	Received: Ver=0, Cmd=QueryRecordStorage, Len=12, [dpID=3, type=4, len=1, val=2], CHECKSUM OK
	*/
	CMD_ExecuteCommand("uartFakeHex 55 AA 00 08 00 0C 00 01 01 01 01 01 01 03 04 00 01 02 23", 0);
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	Sim_RunSeconds(3.0f, false);
	// dpID 1 is door state
	SELFTEST_ASSERT_CHANNEL(1, 1);
	// dpID 3 is battery state
	SELFTEST_ASSERT_CHANNEL(3, 2);
	Sim_RunSeconds(3.0f, false);
	/*
	Pressed: 3B. Send data accept confirmation 0x05 (will put device back to sleep)
	Waiting: 1,00
	Pressed: 3B. Send data accept confirmation 0x05 (will put device back to sleep)
	Waiting: 1,00
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 08 00 01 00 08");
	Sim_RunSeconds(3.0f, false);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 08 00 01 00 08");

	// cause error
	//SELFTEST_ASSERT_CHANNEL(15, 666);
}
void Test_TuyaMCU_BatteryPowered() {
	Test_TuyaMCU_BatteryPowered_Style2();
	Test_TuyaMCU_BatteryPowered_Style1();
	Test_TuyaMCU_BatteryPowered_Style2();
	Test_TuyaMCU_BatteryPowered_Style1();
}

#endif

