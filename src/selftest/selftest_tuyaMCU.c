
#ifdef WINDOWS

#include "selftest_local.h".

void Test_TuyaMCU_Basic() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	// This will map TuyaMCU fnID 2 of type Value to channel 15
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets fnID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 100...
	SELFTEST_ASSERT_CHANNEL(15, 100);

	// This packet sets fnID 2 of type Value to 90
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000005A73", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 90...
	SELFTEST_ASSERT_CHANNEL(15, 90);

	// This packet sets fnID 2 of type Value to 110
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000006E87", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 110...
	SELFTEST_ASSERT_CHANNEL(15, 110);

	// This packet sets fnID 2 of type Value to 120
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000007891", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 120...
	SELFTEST_ASSERT_CHANNEL(15, 120);

	SIM_ClearUART();

	//
	// check sending from OBK to MCU
	//
	// OBK sends: 
	// 55 AA	00	06		00 05	1001000100	1C
	//HEADER	VER = 00	SetDP		LEN	fnId = 16 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 16 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	1001000100	1C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends: 
	// 55 AA	00	06		00 05	0101000101	0E
	// HEADER	VER = 00	Unk		LEN	fnId = 1 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000101	0E");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends: 
	// 55 AA	00	06		00 05	0101000100	0D
	// HEADER	VER = 00	Unk		LEN	fnId = 1 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000100	0D");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	

	// OBK sends: 
	// 55 AA	00	06		00 05	6C01000101	79
	// HEADER	VER = 00	Unk		LEN	fnId = 108 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 108 1 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6C01000101	79");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();



	// OBK sends: 
	// 55 AA	00	06		00 05	6D04000110	8C
	// HEADER	VER = 00	Unk		LEN	fnId = 109 Enum V = 16	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 109 4 16", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6D04000110	8C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	
		

		


	// OBK sends: 
	// 55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64
	//HEADER	VER = 00	Unk		LEN	fnId = 17 Raw V = 01 01 00 50 03 01 00 F5 04 01 00 A0 08 00 00 32	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 17 0 01010050030100F5040100A008000032", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// check channel as argument in raw
	CMD_ExecuteCommand("setChannel 10 1",0);
	CMD_ExecuteCommand("tuyaMcu_sendState 17 0 $CH10$$CH10$0050030100F5040100A008000032", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// check channel as argument in raw
	CMD_ExecuteCommand("setChannel 10 1", 0);
	CMD_ExecuteCommand("setChannel 11 0", 0);
	CMD_ExecuteCommand("setChannel 2 0x50", 0);
	CMD_ExecuteCommand("setChannel 3 0x03", 0);
	CMD_ExecuteCommand("setChannel 4 0xF5", 0);
	CMD_ExecuteCommand("tuyaMcu_sendState 17 0 $CH10$$CH10$$CH11$$CH2$$CH3$0100$CH4$040100A008000032", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// check channel as argument in raw
	CMD_ExecuteCommand("setChannel 10 1", 0);
	CMD_ExecuteCommand("setChannel 11 0", 0);
	CMD_ExecuteCommand("setChannel 2 0x50", 0);
	CMD_ExecuteCommand("setChannel 3 0x03", 0);
	CMD_ExecuteCommand("setChannel 4 0xF5", 0);
	CMD_ExecuteCommand("tuyaMcu_sendState 17 0 $CH10$ $CH10$ $CH11$ $CH2$ $CH3$ 01 00 $CH4$ 04 01 00 A0 08 00 00 32", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// cause error
	//SELFTEST_ASSERT_CHANNEL(15, 666);
}

#endif
