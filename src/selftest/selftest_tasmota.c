#ifdef WINDOWS

#include "selftest_local.h".

void Test_Tasmota_MQTT_Switch() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("miscDevice");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	/*
	// If single power
	cmnd/tasmota_switch/Power ?     // an empty message/payload sends a status query
		? stat/tasmota_switch/RESULT ? {"POWER":"OFF"}  
		? stat/tasmota_switch/POWER ? OFF
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "");
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
	SIM_SendFakeMQTTAndRunSimFrame_CMND("Power", "Toggle");


}

void Test_Tasmota_MQTT_RGBCW() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("rgbcwBulb");

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


	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/CT 153    
	gives
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
	cmnd/tasmota_rgbcw/CT
	gives
	{
		"CT": 153
	}
	*/

	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/HSBColor
	gives
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
	gives
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
	gives
	{
		"Dimmer": 20
	}
	*/

	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Dimmer 20
	gives
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
	/*
	// Powers as for single relay, but also
	cmnd/tasmota_rgbcw/Dimmer 100
	gives
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
}
void Test_Tasmota() {
	Test_Tasmota_MQTT_Switch();
	Test_Tasmota_MQTT_RGBCW();
}
#endif
