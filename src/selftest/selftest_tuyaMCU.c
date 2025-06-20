
#ifdef WINDOWS

#include "selftest_local.h"

void Test_TuyaMCU_RawAccess() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("TuyaMCU", "bekens");

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	CFG_SetFlag(OBK_FLAG_TUYAMCU_STORE_RAW_DATA, 1);

	// This will map TuyaMCU dpID 2 of type Value to channel 15
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 100);

	SIM_ClearMQTTHistory();

	// This packet sets dpID 2 of type Value to 90
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
	// This will map TuyaMCU dpID 2 of type Value to channel 15 and mult %f
	sprintf(cmd,"linkTuyaMCUOutputToChannel 2 val 15 0 %f",mul);
	CMD_ExecuteCommand(cmd, 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dp", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/TuyaMCU/DP", false);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "id", 2);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "type", 0x02);
	SELFTEST_ASSERT_HAS_MQTT_ARRAY_ITEM_INT(0, "data", 100 * mul);

	SIM_ClearMQTTHistory();

	// This packet sets dpID 2 of type Value to 90
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
void Test_TuyaMCU_Boolean() {
	SIM_ClearOBK(0);
	SIM_UART_InitReceiveRingBuffer(2048);
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 bool 2", 0);

	CMD_ExecuteCommand("setChannel 2 1", 0);
	/*
	55 AA    00    06        00 05    0201000101    0F    
	HEADER    VER=00    SetDP        LEN    dpId=2 Bool V=1        CHK    
	*/
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA    00    06        00 05    0201000101    0F");
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	//CMD_ExecuteCommand("setChannel 2 0", 0);
	/*
	55 AA    00    06        00 05    0201000100    0E    
	HEADER    VER=00    SetDP        LEN    dpId=2 Bool V=0        CHK    
	*/
	//SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA    00    06        00 05    0201000100    0E");
	//SELFTEST_ASSERT_HAS_UART_EMPTY();

}
void Test_TuyaMCU_DP22() {
	SIM_ClearOBK(0);
	SIM_UART_InitReceiveRingBuffer(2048);
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 1 bool 1", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 bool 2", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 3 bool 3", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 4 bool 4", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 5 bool 5", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 6 bool 6", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 101 bool 11", 0);
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 102 bool 12", 0);
	CMD_ExecuteCommand("setChannel 1 123", 0);
	CMD_ExecuteCommand("setChannel 2 234", 0);
	CMD_ExecuteCommand("setChannel 3 456", 0);
	CMD_ExecuteCommand("setChannel 4 567", 0);
	CMD_ExecuteCommand("setChannel 5 653", 0);
	CMD_ExecuteCommand("setChannel 6 777", 0);
	CMD_ExecuteCommand("setChannel 7 777", 0); // not set
	CMD_ExecuteCommand("setChannel 11 777", 0);
	CMD_ExecuteCommand("setChannel 12 777", 0);

	CMD_ExecuteCommand("uartFakeHex 55AA03220028010100010002010001000301000100040100010005010001000601000100650100010066010001003C", 0);

	Sim_RunFrames(1000, false);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_CHANNEL(6, 0);
	SELFTEST_ASSERT_CHANNEL(7, 777);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
}
void Test_TuyaMCU_Basic() {
	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	// This will map TuyaMCU dpID 2 of type Value to channel 15
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 100...
	SELFTEST_ASSERT_CHANNEL(15, 100);

	// This packet sets dpID 2 of type Value to 90
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000005A73", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 90...
	SELFTEST_ASSERT_CHANNEL(15, 90);

	// This packet sets dpID 2 of type Value to 110
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000006E87", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 110...
	SELFTEST_ASSERT_CHANNEL(15, 110);

	// This packet sets dpID 2 of type Value to 120
	CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000007891", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to 120...
	SELFTEST_ASSERT_CHANNEL(15, 120);

	SIM_ClearUART();

	//
	// check sending from OBK to MCU
	//
	// OBK sends:   55 AA	00	06		00 05	1001000100	1C
	//HEADER	VER = 00	SetDP		LEN	dpId = 16 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 16 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	1001000100	1C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends:  55 AA	00	06		00 05	0101000101	0E
	// HEADER	VER = 00	Unk		LEN	dpId = 1 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000101	0E");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends:  55 AA	00	06		00 05	0101000100	0D
	// HEADER	VER = 00	Unk		LEN	dpId = 1 Bool V = 0	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 1 1 0", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	0101000100	0D");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	
	// OBK sends:  55 AA	00	06		00 05	6C01000101	79
	// HEADER	VER = 00	Unk		LEN	dpId = 108 Bool V = 1	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 108 bool 1", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6C01000101	79");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends:   55 AA	00	06		00 05	6D04000110	8C
	// HEADER	VER = 00	Unk		LEN	dpId = 109 Enum V = 16	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 109 4 16", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA	00	06		00 05	6D04000110	8C");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();

	// OBK sends:  55 AA	00	06		00 05	6D04000110	8C
	// HEADER	VER = 00	Unk		LEN	dpId = 109 Enum V = 16	CHK
	CMD_ExecuteCommand("tuyaMcu_sendState 109 enum 16", 0);
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

	// This packet sets dpID 104 of type Val
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
	// This command will calculate checksum as well - 0x3
	// This will send wifi state 0x04
	CMD_ExecuteCommand("tuyaMcu_sendCmd 0x03 04", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 03 00 01 04 07");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();




	// check tuyamcu inversion

	CMD_ExecuteCommand("setChannel 15 0", 0);
	// This will map TuyaMCU dpID 2 of type Value to channel 15 with inverse
	// [dpId][varType][channelID][bDPCache-Optional][mult-optional][bInverse]
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15 0 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to negated 100. so 0
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// and disable inversion
	CMD_ExecuteCommand("setChannel 15 0", 0);
	// This will map TuyaMCU dpID 2 of type Value to channel 15 with no inverse
	// [dpId][varType][channelID][bDPCache-Optional][mult-optional][bInverse]
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15 0 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to  100
	SELFTEST_ASSERT_CHANNEL(15, 100);

	// now try multiplier

	CMD_ExecuteCommand("setChannel 15 0", 0);
	// This will map TuyaMCU dpID 2 of type Value to channel 15 with no inverse
	// 10 is multiplier
	// [dpId][varType][channelID][bDPCache-Optional][mult-optional][bInverse]
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15 0 10 0", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to  100*10
	SELFTEST_ASSERT_CHANNEL(15, 100 * 10);



	SIM_ClearUART();
	// now try delta - value 5
	CMD_ExecuteCommand("setChannel 15 0", 0);
	// This will map TuyaMCU dpID 2 of type Value to channel 15 with no inverse
	// 10 is multiplier
	// 5 is delta
	// [dpId][varType][channelID][bDPCache-Optional][mult-optional][bInverse][delta]
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15 0 10 0 5", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to....
	SELFTEST_ASSERT_CHANNEL(15, (100+5) * 10);
	SIM_ClearUART();
	// now try delta - value -5
	CMD_ExecuteCommand("setChannel 15 0", 0);
	// This will map TuyaMCU dpID 2 of type Value to channel 15 with no inverse
	// 10 is multiplier
	// 5 is delta
	// [dpId][varType][channelID][bDPCache-Optional][mult-optional][bInverse][delta]
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 2 val 15 0 10 0 -5", 0);
	SELFTEST_ASSERT_CHANNEL(15, 0);
	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(1000, false);
	// Now, channel 15 should be set to....
	SELFTEST_ASSERT_CHANNEL(15, (100 + -5) * 10);
	SIM_ClearUART();


	CMD_ExecuteCommand("tuyaMcu_sendCmd 0x06 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 06 00 3A 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030 18");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	// check to see if we can execute a file from within LFS
	Test_FakeHTTPClientPacket_POST("api/lfs/tuyamcu1.txt", "tuyaMcu_sendCmd 0x06 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030");
	// exec will execute a script file in-place, all commands at once
	CMD_ExecuteCommand("exec tuyamcu1.txt", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 06 00 3A 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030 18");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("alias xyz123 tuyaMcu_sendCmd 0x06 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030",0);
	CMD_ExecuteCommand("xyz123", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 06 00 3A 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030 18");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	// twice
	CMD_ExecuteCommand("xyz123", 0);
	CMD_ExecuteCommand("xyz123", 0);
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 06 00 3A 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030 18");
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55 AA 00 06 00 3A 19030036303035413030303130303738303345383033453830303030303030303030303030313030303030303030303030303030303030303030 18");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();
	SIM_ClearMQTTHistory();
	// cause error
	//SELFTEST_ASSERT_CHANNEL(15, 666);



	// fake time request
	NTP_SetSimulatedTime(1732094000);
	// Simulate TuyaMCU sending 0x1C request to OBK
	CMD_ExecuteCommand("uartFakeHex 55 AA 03 1C 00 00 1E", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);
	// OBK will reply with 0x1C packet (TUYA_CMD_SET_TIME)
	SELFTEST_ASSERT_HAS_SENT_UART_STRING("55AA001C000801180B14090D140388");
	// skip optional heartbeat
	SIM_UART_ExpectAndConsumeHexStr("55AA00000000FF");
	// nothing is sent by OBK at that point
	SELFTEST_ASSERT_HAS_UART_EMPTY();


	
	SIM_ClearUART();


	// this will write to channel 40, 41, and 42
	// 232.6V 153mA 3.5W
	// 0.0153*232.6 = 3.55878W
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP 40", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA03070014060000080916000099000023010200040000000310", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);
	SELFTEST_ASSERT_CHANNEL(40, 2326);// voltage
	SELFTEST_ASSERT_CHANNEL(41, 153);// current
	SELFTEST_ASSERT_CHANNEL(42, 35);// power

	// this will write to channel 45, 46 and 47
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 8 RAW_TAC2121C_VCP 45", 0);
	CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 0C 08 00 00 08 09 01 00 01 2E 00 00 43 A1 ", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);
	SELFTEST_ASSERT_CHANNEL(45, 2305); // voltage
	SELFTEST_ASSERT_CHANNEL(46, 302);// current
	SELFTEST_ASSERT_CHANNEL(47, 67); // power

	SIM_ClearUART();
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	g_cfg.pins.channelTypes[3] = ChType_Voltage_div10;
	g_cfg.pins.channelTypes[7] = ChType_Current_div1000;
	g_cfg.pins.channelTypes[9] = ChType_Power;

	CMD_ExecuteCommand("setChannel 0 12345", 0);
	// this will write to auto-found channels
	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 8 RAW_TAC2121C_VCP", 0);
	CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 0C 08 00 00 08 09 01 00 01 2E 00 00 43 A1 ", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);

	SELFTEST_ASSERT_CHANNEL(0, 12345); // not changed
	SELFTEST_ASSERT_CHANNEL(3, 2305); // voltage
	SELFTEST_ASSERT_CHANNEL(7, 302);// current
	SELFTEST_ASSERT_CHANNEL(9, 67); // power


	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 115 RAW_VCPPfF", 0);
	CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 13 73 00 00 0F 09 0F 00 00 BF 00 00 78 00 00 25 01 10 C3 32 18 ", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);

	CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 113 RAW_VCPPfF", 0);
	CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 13 71 00 00 0F 09 29 00 01 B7 00 03 FC 00 00 00 03 E8 C3 32 65", 0);
	// above command will just put into buffer - need at least a frame to parse it
	Sim_RunFrames(100, false);

	SIM_ClearUART();
}
void Test_TuyaMCU_Calib() {
	{
		// reset whole device
		SIM_ClearOBK(0);

		SIM_UART_InitReceiveRingBuffer(2048);

		CMD_ExecuteCommand("startDriver TuyaMCU", 0);

		g_cfg.pins.channelTypes[3] = ChType_Voltage_div10;
		g_cfg.pins.channelTypes[7] = ChType_Current_div1000;
		g_cfg.pins.channelTypes[9] = ChType_Power;

		CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 113 RAW_VCPPfF", 0);
		CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 13 71 00 00 0F 09 29 00 01 B7 00 03 FC 00 00 00 03 E8 C3 32 65", 0);
		// above command will just put into buffer - need at least a frame to parse it
		Sim_RunFrames(100, false);

		SELFTEST_ASSERT_CHANNEL(3, 2345);
		SELFTEST_ASSERT_CHANNEL(7, 439);
		SELFTEST_ASSERT_CHANNEL(9, 1020);


		SIM_ClearUART();
	}
	{
		// reset whole device
		SIM_ClearOBK(0);

		SIM_UART_InitReceiveRingBuffer(2048);

		CMD_ExecuteCommand("startDriver TuyaMCU", 0);

		g_cfg.pins.channelTypes[3] = ChType_Voltage_div10;
		g_cfg.pins.channelTypes[7] = ChType_Current_div1000;
		g_cfg.pins.channelTypes[9] = ChType_Power;


		CMD_ExecuteCommand("linkTuyaMCUOutputToChannel 113 RAW_VCPPfF -1 0 1 0 22 33 44", 0);
		CMD_ExecuteCommand("uartFakeHex 55 AA 03 07 00 13 71 00 00 0F 09 29 00 01 B7 00 03 FC 00 00 00 03 E8 C3 32 65", 0);
		// above command will just put into buffer - need at least a frame to parse it
		Sim_RunFrames(100, false);

		SELFTEST_ASSERT_CHANNEL(3, 2345+22);
		SELFTEST_ASSERT_CHANNEL(7, 439+33);
		SELFTEST_ASSERT_CHANNEL(9, 1020+44);


		SIM_ClearUART();
	}

}

#endif
