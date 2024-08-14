#ifdef WINDOWS

#include "selftest_local.h"

void Test_Tasmota_MQTT_Switch() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	const char *my_full_device_name = "TestingDevMQTTSwitch";
	CFG_SetDeviceName(my_full_device_name);

	CFG_SetFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES, true);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	/*
	// If single power
	cmnd/tasmota_switch/Power ?     // an empty message/payload sends a status query
		? stat/tasmota_switch/RESULT ? {"POWER":"OFF"}  
		? stat/tasmota_switch/POWER ? OFF
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();

	/*
	// If multiple power
	cmnd/tasmota_switch/Power1 ?     // an empty message/payload sends a status query
		? stat/tasmota_switch/RESULT ? {"POWER1":"OFF"}  
		? stat/tasmota_switch/POWER1 ? OFF
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power1", "");

	/*
	cmnd/tasmota_switch/Power TOGGLE
		? // Power for relay 1 is toggled
		? stat/tasmota_switch/RESULT ? {"POWER":"ON"}
		? stat/tasmota_switch/POWER ? ON
	*/
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "Toggle");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");
	SIM_ClearMQTTHistory();

	/*
	// When send status, we get full status on stat
	// cmnd/tasmota_787019/STATUS

	Message 6 received on stat/tasmota_787019/STATUS at 10:36 AM:
	{
		"Status": {
			"Module": 0,
			"DeviceName": "TasmotaBathroom",
			"FriendlyName": [
				"TasmotaBathroom"
			],
			"Topic": "tasmota_787019",
			"ButtonTopic": "0",
			"Power": 0,
			"PowerOnState": 3,
			"LedState": 1,
			"LedMask": "FFFF",
			"SaveData": 1,
			"SaveState": 1,
			"SwitchTopic": "0",
			"SwitchMode": [
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0
			],
			"ButtonRetain": 0,
			"SwitchRetain": 0,
			"SensorRetain": 0,
			"PowerRetain": 0,
			"InfoRetain": 0,
			"StateRetain": 0
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING("Status", "DeviceName", CFG_GetShortDeviceName());
	SIM_ClearMQTTHistory();

	// Status 6 for MQTT
	/*
	Message 53 received on stat/tasmota_01FDF1/STATUS6 at 2:55 AM:
	{
		"StatusMQT": {
			"MqttHost": "192.168.0.113",
			"MqttPort": 1883,
			"MqttClientMask": "DVES_%06X",
			"MqttClient": "DVES_01FDF1",
			"MqttUser": "homeassistant",
			"MqttCount": 2,
			"MAX_PACKET_SIZE": 1200,
			"KEEPALIVE": 30,
			"SOCKET_TIMEOUT": 4
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "6");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS6", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusMQT", "MqttHost", CFG_GetMQTTHost());
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusMQT", "MqttPort", CFG_GetMQTTPort());
	SIM_ClearMQTTHistory();

	// Status 7 is TIM
	/*
	Message 60 received on stat/tasmota_01FDF1/STATUS7 at 3:07 AM:
	{
		"StatusTIM": {
			"UTC": "2023-01-24T02:07:13",
			"Local": "2023-01-24T03:07:13",
			"StartDST": "2023-03-26T02:00:00",
			"EndDST": "2023-10-29T03:00:00",
			"Timezone": "+01:00",
			"Sunrise": "08:30",
			"Sunset": "17:33"
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "7");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS7", false);
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "UTC");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "Local");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "Timezone");
	// TRIGGER error to debug
	//SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "QQQ");
	SIM_ClearMQTTHistory();

	// status 5 is NET
	/*
	Message 61 received on stat/tasmota_01FDF1/STATUS5 at 3:12 AM:
	{
		"StatusNET": {
			"Hostname": "Tasmota-BWSHP9",
			"IPAddress": "192.168.0.157",
			"Gateway": "192.168.0.1",
			"Subnetmask": "255.255.255.0",
			"DNSServer1": "192.168.0.1",
			"DNSServer2": "0.0.0.0",
			"Mac": "D8:F1:5B:01:FD:F1",
			"Webserver": 2,
			"HTTP_API": 1,
			"WifiConfig": 4,
			"WifiPower": 17
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "5");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS5", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusNET", "Hostname", CFG_GetShortDeviceName());
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusNET", "IPAddress", HAL_GetMyIPString());
	// TRIGGER error to debug
	//SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "QQQ");
	SIM_ClearMQTTHistory();


	// Status 4 is MEM
	/*

	Listening to
	stat/#

	Message 62 received on stat/tasmota_01FDF1/STATUS4 at 3:17 AM:
	{
		"StatusMEM": {
			"ProgramSize": 626,
			"Free": 376,
			"Heap": 27,
			"ProgramFlashSize": 1024,
			"FlashSize": 1024,
			"FlashChipId": "144068",
			"FlashFrequency": 40,
			"FlashMode": 3,
			"Features": [
				"00000809",
				"8FDAC787",
				"04368001",
				"000000CF",
				"010013C0",
				"C000F981",
				"00004004",
				"00001000",
				"04000020"
			],
			"Drivers": "1,2,3,4,5,6,7,8,9,10,12,16,18,19,20,21,22,24,26,27,29,30,35,37,45,56,62",
			"Sensors": "1,2,3,4,5,6"
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "4");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS4", false);
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusMEM", "ProgramSize");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusMEM", "FlashSize");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusMEM", "Heap");
	// TRIGGER error to debug
	//SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "QQQ");
	SIM_ClearMQTTHistory();

	/*
	Message 64 received on stat/tasmota_01FDF1/STATUS2 at 3:20 AM:
	{
		"StatusFWR": {
			"Version": "11.1.0(tasmota)",
			"BuildDateTime": "2022-04-13T06:40:42",
			"Boot": 7,
			"Core": "2_7_4_9",
			"SDK": "2.2.2-dev(38a443e)",
			"CpuFrequency": 80,
			"Hardware": "ESP8266EX",
			"CR": "496/699"
		}
	}

	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATUS", "2");
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/miscDevice/STATUS2", false);
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusFWR", "Version");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusFWR", "BuildDateTime");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusFWR", "Hardware");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusFWR", "Hardware", PLATFORM_MCU_NAME);
	// TRIGGER error to debug
	//SELFTEST_ASSERT_JSON_VALUE_EXISTS("StatusTIM", "QQQ");
	SIM_ClearMQTTHistory();

}
void Test_Tasmota_MQTT_Switch_Double() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("twoRelaysDevice", "bekens");

	const char *my_full_device_name = "TestingDevMQTTSwitch";
	CFG_SetDeviceName(my_full_device_name);

	CFG_SetFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES, true);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	/*
	// If single power
	cmnd/tasmota_switch/Power ?     // an empty message/payload sends a status query
		? stat/tasmota_switch/RESULT ? {"POWER":"OFF"}
		? stat/tasmota_switch/POWER ? OFF
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power1", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER1", "OFF");
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power2", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER2", "ON");
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power1", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER1", "ON");
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power2", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER2", "ON");
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power1", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER1", "OFF");
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power2", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER2", "OFF");
	SIM_ClearMQTTHistory();


	// command STATE
	// check for TELE state
	/*
	Message 33 received on stat/tasmota_787019/RESULT at 10:08 AM:
	{
		"Time": "2023-01-24T10:08:17",
		"Uptime": "0T00:10:43",
		"UptimeSec": 643,
		"Heap": 27,
		"SleepMode": "Dynamic",
		"Sleep": 50,
		"LoadAvg": 20,
		"MqttCount": 1,
		"POWER": "OFF",
		"Wifi": {
			"AP": 1,
			"SSId": "DLINK_FastNet",
			"BSSId": "28:87:BA:A0:F5:6D",
			"Channel": 2,
			"Mode": "11n",
			"RSSI": 64,
			"Signal": -68,
			"LinkCount": 1,
			"Downtime": "0T00:00:04"
		}
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("STATE", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/twoRelaysDevice/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Time");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Uptime");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "UptimeSec");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Sleep");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("Wifi", "SSId");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("Wifi", "RSSI");
	SIM_ClearMQTTHistory();

	// The same is sent to TELE periodically
	/*
	Message 6 received on tele/tasmota_787019/STATE at 9:57 AM:
	{
		"Time": "2023-01-24T09:57:44",
		"Uptime": "0T00:00:10",
		"UptimeSec": 10,
		"Heap": 27,
		"SleepMode": "Dynamic",
		"Sleep": 50,
		"LoadAvg": 25,
		"MqttCount": 1,
		"POWER": "OFF",
		"Wifi": {
			"AP": 1,
			"SSId": "DLINK_FastNet",
			"BSSId": "28:87:BA:A0:F5:6D",
			"Channel": 2,
			"Mode": "11n",
			"RSSI": 64,
			"Signal": -68,
			"LinkCount": 1,
			"Downtime": "0T00:00:04"
		}
	}
	*/
	SIM_ClearMQTTHistory();
	Sim_RunSeconds(130, false);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("tele/twoRelaysDevice/STATE", false);
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Time");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Uptime");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "UptimeSec");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS(0, "Sleep");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("Wifi", "SSId");
	SELFTEST_ASSERT_JSON_VALUE_EXISTS("Wifi", "RSSI");
	SIM_ClearMQTTHistory();



}
void Test_Tasmota_MQTT_RGBCW() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("rgbcwBulb", "bekens");

	CMD_ExecuteCommand("led_dimmer 50", 0);

	CFG_SetFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES, true);

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 3);

	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 3);

	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Power ?     // an empty message/payload sends a status query
		? stat/tasmota_rgbcw/RESULT ? {"POWER":"OFF"}
		? stat/tasmota_switch/POWER ? OFF
	*/
	CMD_ExecuteCommand("led_enableAll 0", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_enableAll 1", 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/CT 153    
	gives stat/tasmota_rgbcw/RESULT
	{
		"POWER": "ON",
		"Dimmer": 100,
		"Color": "000000FF00",
		"HSBColor": "180,100,0",
		"White": 100,
		"CT": 153,
		"Channel": [
			0,
			0,
			0,
			100,
			0
		]
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("CT", "153");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 153);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();

	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("CT", "500");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 500);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/CT
	gives stat/tasmota_rgbcw/RESULT
	{
		"CT": 153
	}
	*/
	CMD_ExecuteCommand("CT 153", 0);
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("CT", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 153);
	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("CT 444", 0);
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("CT", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 444);
	SIM_ClearMQTTHistory();

	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/HSBColor
	gives stat/tasmota_rgbcw/RESULT
	{
		"POWER": "ON",
		"Dimmer": 100,
		"Color": "000000FF00",
		"HSBColor": "180,100,0",
		"White": 100,
		"CT": 153,
		"Channel": [
			0,
			0,
			0,
			100,
			0
		]
	}
	*/
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/HSBColor 90,100,0
	gives stat/tasmota_rgbcw/RESULT
	{
		"POWER": "ON",
		"Dimmer": 100,
		"Color": "7FFF000000",
		"HSBColor": "90,100,100",
		"White": 0,
		"CT": 153,
		"Channel": [
			50,
			100,
			0,
			0,
			0
		]
	}
	*/
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Dimmer
	gives stat/tasmota_rgbcw/RESULT
	{
		"Dimmer": 20
	}
	*/
	CMD_ExecuteCommand("Dimmer 20", 0);
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dimmer", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 20);
	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("Dimmer 88", 0);
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dimmer", "");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 88);
	SIM_ClearMQTTHistory();

	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Dimmer 20
	gives stat/tasmota_rgbcw/RESULT
	{
		"POWER": "ON",
		"Dimmer": 20,
		"Color": "2B33240000",
		"HSBColor": "90,30,20",
		"White": 0,
		"CT": 153,
		"Channel": [
			17,
			20,
			14,
			0,
			0
		]
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dimmer", "20");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 444);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 20);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Dimmer 100
	gives stat/tasmota_rgbcw/RESULT
	{
		"POWER": "ON",
		"Dimmer": 100,
		"Color": "D8FFB20000",
		"HSBColor": "90,30,100",
		"White": 0,
		"CT": 153,
		"Channel": [
			85,
			100,
			70,
			0,
			0
		]
	}
	*/
	SIM_ClearMQTTHistory();
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Dimmer", "100");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("stat/rgbcwBulb/RESULT", false);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "CT", 444);
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(0, "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");
	SIM_ClearMQTTHistory();
}
void Test_Backlog() {

	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog setChannel 1 2; ", 0) == CMD_RES_OK);
	SELFTEST_ASSERT_CHANNEL(1, 2);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog setChannel 1 3", 0) == CMD_RES_OK);
	SELFTEST_ASSERT_CHANNEL(1, 3);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog    setChannel 1     4", 0) == CMD_RES_OK);
	SELFTEST_ASSERT_CHANNEL(1, 4);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog    setChannel 1     5 ;;;", 0) == CMD_RES_OK);
	SELFTEST_ASSERT_CHANNEL(1, 5);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog    thiisCooommandNotExists 1     5 ;;;", 0)
		== CMD_RES_UNKNOWN_COMMAND);
	SELFTEST_ASSERT_CHANNEL(1, 5);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog    setChannel 1;;;", 0) == CMD_RES_NOT_ENOUGH_ARGUMENTS);
	SELFTEST_ASSERT_CHANNEL(1, 5);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog    setChannel ;;;", 0) == CMD_RES_NOT_ENOUGH_ARGUMENTS);
	SELFTEST_ASSERT_CHANNEL(1, 5);
	SELFTEST_ASSERT(CMD_ExecuteCommand("backlog setChannel 1 22; setChannel 1 33", 0) == CMD_RES_OK);
	SELFTEST_ASSERT_CHANNEL(1, 33);
}
void Test_Tasmota() {
	Test_Tasmota_MQTT_Switch();
	Test_Tasmota_MQTT_Switch_Double();
	Test_Tasmota_MQTT_RGBCW();
}
#endif
