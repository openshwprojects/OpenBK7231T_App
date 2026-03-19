#ifdef WINDOWS

#include "selftest_local.h"
#include "../cmnds/cmd_enums.h"
void Test_Enum_LowMidH() {
	// reset whole device
	const char *shortName = "myShortName";
	SIM_ClearOBK(shortName);
	const char *fullName = "Windows Enum";
	const char *mqttName = "obkEnumDemo";

	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CMD_ExecuteCommand("SetChannelType 13 LowMidHigh", 0);
	SELFTEST_ASSERT(g_cfg.pins.channelTypes[13] == ChType_LowMidHigh);

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"state_topic", "~/13/get",
		"uniq_id", "Windows_Enum_select_13",
		"uniq_id", "Windows_Enum_select_13",
		"uniq_id", "Windows_Enum_select_13");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"availability_topic", "~/connected",
		"payload_available", "online",
		"payload_not_available", "offline",
		"uniq_id", "Windows_Enum_select_13");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"command_topic", "~/13/set",
		"options", "[\"Low\",\"Mid\",\"High\"]",
		"value_template", "{{ {'0': 'Low', '1': 'Mid', '2': 'High'} [value] }}",
		"command_template", "{{ {'Low': '0', 'Mid': '1', 'High': '2'} [value] }}");

}
void Test_Enum_LowMidHighOff() {
	// reset whole device
	const char *shortName = "myShortName";
	SIM_ClearOBK(shortName);
	const char *fullName = "Windows Enum";
	const char *mqttName = "obkEnumDemo";

	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CMD_ExecuteCommand("SetChannelType 12 OffLowMidHigh", 0);
	SELFTEST_ASSERT(g_cfg.pins.channelTypes[12] == ChType_OffLowMidHigh);

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"state_topic", "~/12/get",
		"uniq_id", "Windows_Enum_select_12",
		"uniq_id", "Windows_Enum_select_12",
		"uniq_id", "Windows_Enum_select_12");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"availability_topic", "~/connected",
		"payload_available", "online",
		"payload_not_available", "offline",
		"uniq_id", "Windows_Enum_select_12");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"command_topic", "~/12/set",
		"options", "[\"Off\",\"Low\",\"Mid\",\"High\"]",
		"value_template", "{{ {'0': 'Off', '1': 'Low', '2': 'Mid', '3': 'High'} [value] }}",
		"command_template", "{{ {'Off': '0', 'Low': '1', 'Mid': '2', 'High': '3'} [value] }}");

}
void Test_Enum_3opts() {
	// reset whole device
	const char *shortName = "myShortName";
	SIM_ClearOBK(shortName);
	const char *fullName = "Windows Enum";
	const char *mqttName = "obkEnumDemo";

	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	//SELFTEST_ASSERT(g_enums == 0);

	CMD_ExecuteCommand("SetChannelEnum 14 \"1:One Switch\" 0:Zero 3:Three", 0);
	SELFTEST_ASSERT(g_enums);
	SELFTEST_ASSERT(g_enums[14]);
	SELFTEST_ASSERT(!g_enums[13]);
	SELFTEST_ASSERT(!g_enums[15]);
	SELFTEST_ASSERT(g_cfg.pins.channelTypes[14] != ChType_Enum);
	CMD_ExecuteCommand("SetChannelType 14 Enum", 0);
	SELFTEST_ASSERT(g_cfg.pins.channelTypes[14] == ChType_Enum);

	SELFTEST_ASSERT(g_enums[14]->numOptions == 3);
	SELFTEST_ASSERT(g_enums[14]->options[0].value == 1);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[0].label, "One Switch"));
	SELFTEST_ASSERT(g_enums[14]->options[1].value == 0);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[1].label, "Zero"));
	SELFTEST_ASSERT(g_enums[14]->options[2].value == 3);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[2].label, "Three"));

	SELFTEST_ASSERT(!strcmp(CMD_FindChannelEnumLabel(g_enums[14], 1), "One Switch"));
	SELFTEST_ASSERT(!strcmp(CMD_FindChannelEnumLabel(g_enums[14], 0), "Zero"));
	SELFTEST_ASSERT(!strcmp(CMD_FindChannelEnumLabel(g_enums[14], 3), "Three"));


	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"state_topic", "~/14/get",
		"unique_id", "14",
		"unique_id", "14",
		"uniq_id", "Windows_Enum_select_14");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"availability_topic", "~/connected",
		"payload_available", "online",
		"payload_not_available", "offline",
		"uniq_id", "Windows_Enum_select_14");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"command_topic", "~/14/set",
		"options", "[\"One Switch\",\"Zero\",\"Three\"]",
		"value_template", "{{ {1:'One Switch', 0:'Zero', 3:'Three', 99999:'Undefined'}[(value | int(99999))] | default(\"Undefined Enum [\"~value~\"]\") }}",
		"command_template", "{{ {'One Switch':'1', 'Zero':'0', 'Three':'3', 'Undefined':'99999'}[value] | default(99999) }}");

}
void Test_Enum_BadOk() {

	char tmp[1024];

	// reset whole device
	const char *shortName = "myShortName";
	SIM_ClearOBK(shortName);
	const char *fullName = "Windows Enum";
	const char *mqttName = "obkEnumDemo";

	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	SELFTEST_ASSERT(g_enums == 0);

	CMD_ExecuteCommand("SetChannelType 4 Enum", 0);
	CMD_ExecuteCommand("SetChannelEnum 4 1:Bad 0:Ok", 0);
	CMD_ExecuteCommand("SetChannelEnum 4 1:Ok 0:Bad", 0); // check options overwrite

	// null checks
	SELFTEST_ASSERT(g_enums);
	SELFTEST_ASSERT(!strcmp(CMD_FindChannelEnumLabel(g_enums[14], 1), "1"));
	CMD_GenEnumValueTemplate(g_enums[14], tmp, sizeof(tmp));
	// actual discovery template content assertions are in selftst_hass_discovery_ext.c
	SELFTEST_ASSERT(!strcmp(tmp, "{{ {99999:'Undefined'}[(value | int(99999))] | default(\"Undefined Enum [\"~value~\"]\") }}"));

	for (int i = 0; i < CHANNEL_MAX; i++) {
		if (i != 4) {
			SELFTEST_ASSERT(!g_enums[i]);
		}
	}
	SELFTEST_ASSERT(g_enums[4]);

	SELFTEST_ASSERT(g_enums[4]->numOptions == 2);
	SELFTEST_ASSERT(g_enums[4]->options[0].value == 1);
	SELFTEST_ASSERT(!strcmp(g_enums[4]->options[0].label, "Ok"));
	SELFTEST_ASSERT(g_enums[4]->options[1].value == 0);
	SELFTEST_ASSERT(!strcmp(g_enums[4]->options[1].label, "Bad"));

	CMD_FormatEnumTemplate(g_enums[4], tmp, sizeof(tmp), false);
	CMD_FormatEnumTemplate(g_enums[4], tmp, sizeof(tmp), true);





	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"state_topic", "~/4/get",
		"unique_id", "4",
		"unique_id", "4",
		"uniq_id", "Windows_Enum_select_4");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"availability_topic", "~/connected",
		"payload_available", "online",
		"payload_not_available", "offline",
		"uniq_id", "Windows_Enum_select_4");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY_4KEY("homeassistant", true, 0, 0,
		"command_topic", "~/4/set",
		"options", "[\"Ok\",\"Bad\"]",
		"value_template", "{{ {1:'Ok', 0:'Bad', 99999:'Undefined'}[(value | int(99999))] | default(\"Undefined Enum [\"~value~\"]\") }}",
		"command_template", "{{ {'Ok':'1', 'Bad':'0', 'Undefined':'99999'}[value] | default(99999) }}");


}
void Test_Enums() {

	Test_Enum_BadOk();
	Test_Enum_3opts();
	Test_Enum_LowMidH();
	Test_Enum_LowMidHighOff();
}



#endif
