
#ifdef WINDOWS

#include "selftest_local.h"

void Test_TuyaMCU_RawAccess() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("TuyaMCU", "bekens");

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	CFG_SetFlag(OBK_FLAG_TUYAMCU_STORE_RAW_DATA, 1);

	// This will map TuyaMCU fnID 2 of type Value to channel 15
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets fnID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 100);

	SIM_ClearMQTTHistory();

	// This packet sets fnID 2 of type Value to 90
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000005A73", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 90);

	SIM_ClearMQTTHistory();


	// This packet sets dpID 18 of type RAW
	// dpID 18
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 18 Raw", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA030700101200000C0101003F030100FA040100AA25", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 18);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(0, "data", "0101003F030100FA040100AA");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "data", 90);

	SIM_ClearMQTTHistory();

	// This packet sets dpID 104 of type RAW
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 104 Raw", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070008680200040000000180", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 104);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(0, "data", "00000001");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "id", 18);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(1, "data", "0101003F030100FA040100AA");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "data", 90);

	SIM_ClearMQTTHistory();

}
void Test_TuyaMCU_Mult_Internal(float mul) {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("TuyaMCU", "bekens");

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	CFG_SetFlag(OBK_FLAG_TUYAMCU_STORE_RAW_DATA, 1);

	char cmd[256];
	// This will map TuyaMCU fnID 2 of type Value to channel 15 and mult %f
	sprintf(cmd,"linkTuyaMCUOutputToChannel 2 val 15 0 %f",mul);
	CMD_ExecuteCommand(cmd, 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets fnID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 100 * mul);

	SIM_ClearMQTTHistory();

	// This packet sets fnID 2 of type Value to 90
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000005A73", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 90 * mul);

	SIM_ClearMQTTHistory();


	// This packet sets dpID 18 of type RAW
	// dpID 18
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 18 Raw", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA030700101200000C0101003F030100FA040100AA25", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 18);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(0, "data", "0101003F030100FA040100AA");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "data", 90 * mul);

	SIM_ClearMQTTHistory();

	// This packet sets dpID 104 of type RAW
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 104 Raw", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070008680200040000000180", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 104);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(0, "data", "00000001");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "id", 18);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(1, "type", 0x00);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_STR(1, "data", "0101003F030100FA040100AA");

	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(2, "data", 90 * mul);

	SIM_ClearMQTTHistory();

}
void Test_TuyaMCU_Mult() {
	Test_TuyaMCU_Mult_Internal(1.0f);
	Test_TuyaMCU_Mult_Internal(-1.0f);
	Test_TuyaMCU_Mult_Internal(-5.0f);
	Test_TuyaMCU_Mult_Internal(0.5f);
	Test_TuyaMCU_Mult_Internal(100.0f);
}
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
	//HEADER	VER = 00	SetDP		LEN	dpId = 16 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 16 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	1001000100	1C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends: 
	// 55 AA	00	06		00 05	0101000101	0E
	// HEADER	VER = 00	Unk		LEN	dpId = 1 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000101	0E");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends: 
	// 55 AA	00	06		00 05	0101000100	0D
	// HEADER	VER = 00	Unk		LEN	dpId = 1 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000100	0D");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	

	// OBK sends: 
	// 55 AA	00	06		00 05	6C01000101	79
	// HEADER	VER = 00	Unk		LEN	dpId = 108 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 108 1 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6C01000101	79");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();



	// OBK sends: 
	// 55 AA	00	06		00 05	6D04000110	8C
	// HEADER	VER = 00	Unk		LEN	dpId = 109 Enum V = 16	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 109 4 16", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6D04000110	8C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	
		

		


	// OBK sends: 
	// 55 AA	00	06		00 14	1100001001010050030100F5040100A008000032	64
	//HEADER	VER = 00	Unk		LEN	dpId = 17 Raw V = 01 01 00 50 03 01 00 F5 04 01 00 A0 08 00 00 32	CHK
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


	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	// This packet sets dpID 2 of type Value to 120
	// linkTuyaMCUOutputToChannel dpId varType channelID
	// Special Syntax! is used to link it to MQTT
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 MQTT", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000007891", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, expect a certain MQTT packet to be published....
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/tm/val/2", "120", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This packet sets dpID 17 of type RAW
	// dpID 17: Leak protection
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 17 MQTT", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070008110000040400001E48", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, expect a certain MQTT packet to be published....
	// NOTE: I just did hex to ascii on payload 
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/tm/raw/17", "0400001E", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This packet sets dpID 18 of type RAW
	// dpID 18
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 18 MQTT", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA030700101200000C0101003F030100FA040100AA25",0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, expect a certain MQTT packet to be published....
	// NOTE: I just did hex to ascii on payload 
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/tm/raw/18", "0101003F030100FA040100AA", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This packet sets dpID 104 of type RAW
	// dpID 104
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 104 MQTT", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070008680200040000000180",0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, expect a certain MQTT packet to be published....
	// NOTE: I just did hex to ascii on payload 
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/tm/val/104", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_ClearUART();

	//
	// uartSendHex tests
	//
	CMD_ExecuteCommand("uartSendHex FFAABB", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF AA	BB");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 12 0xCC", 0);
	CMD_ExecuteCommand("uartSendHex FF$CH12$BB", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF CC	BB");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 12 0xDD", 0);
	CMD_ExecuteCommand("uartSendHex FF$CH12$BB", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF DD	BB");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 13 0xEE", 0);
	CMD_ExecuteCommand("uartSendHex FF$CH12$$CH13$", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF DD	EE");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();


	CMD_ExecuteCommand("uartSendHex FF$CH12$00$CH13$00", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF DD 00 EE 00");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	// make sure that spaces won't break stuff
	CMD_ExecuteCommand("uartSendHex FF$CH12$00$CH13$00  ", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF DD 00 EE 00");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	// make sure that spaces won't break stuff
	CMD_ExecuteCommand("uartSendHex   FF  $CH12$  00  $CH13$   00  ", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("FF DD 00 EE 00");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	//
	// tuyaMcu_sendCmd tests
	//
	// This command will calculate checksum as well - 0x32
	CMD_ExecuteCommand("tuyaMcu_sendCmd 0x30 000000", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 30 00 03 00 00 00 32");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();
	// This command will calculate checksum as well - 0x38
	CMD_ExecuteCommand("tuyaMcu_sendCmd 0x30 000006", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 30 00 03 00 00 06 38");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	SIM_ClearUART();


	// cause error
	//SELFTEST_ASSERT_CHANNEL(15, 666);
}

#endif
