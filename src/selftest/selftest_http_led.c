#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/new_http.h"


void Test_Http_LED_CW() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_temperature 153", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);


	// HTML page must contains dimmer and temperature control
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(false);
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);



	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_enableAll 0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_dimmer 61", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(false);
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(false);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(true);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_temperature 500", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);
}
void Test_Http_LED_RGB() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0000", 0);


	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(false);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(true);
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);



	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,0,0");


	CMD_ExecuteCommand("led_basecolor_rgb FFFF00", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,255,0");

	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	CMD_ExecuteCommand("led_dimmer 50", 0);

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "55,55,0");


	CMD_ExecuteCommand("led_basecolor_rgb 0000FF", 0);
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,55");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	CMD_ExecuteCommand("led_dimmer 100", 0);
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "OFF");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,0");

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "1");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "50");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 50);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,55");

	// dimmer back to 100
	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,255");

	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%200,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"FF00000000","HSBColor":"0,100,100","White":0,"CT":479,"Channel":[100,0,0,0,0]}

	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "0,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,0,0");
	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%2090,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"7FFF000000","HSBColor":"90,100,100","White":0,"CT":479,"Channel":[50,100,0,0,0]}

	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "90,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "127,255,0");
	// check Tasmota HSBColor
	/*
	NOTE:
	HTTP Request: http://192.168.0.153/cm?cmnd=HSBColor%20180,100,100
	Return value: {"POWER":"ON","Dimmer":100,"Color":"00FFFF0000","HSBColor":"180,100,100","White":0,"CT":479,"Channel":[0,100,100,0,0]}
	*/
	SIM_SendFakeMQTTAndRunSimFrame_CMND("HSBColor", "180,100,100");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	// Tasmota colors are scalled by Dimmer in this case. Confirmed.
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,255,255");


	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);


	
	CMD_ExecuteCommand("led_enableAll 0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	// color is now 0,0,0 because it's OFF?
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "0,0,0");

	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	// the red button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(false);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(true);
	/*
	CMD_ExecuteCommand("led_dimmer 61", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);


	CMD_ExecuteCommand("led_enableAll 1", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 153);

	CMD_ExecuteCommand("led_temperature 500", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "CT", 500);
	*/

}


void Test_Http_LED_RGBCW() {

	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 4);

	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 5);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_basecolor_rgb FF0000", 0);


	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(true);
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);



	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "Color", "255,0,0,0,0");


	CMD_ExecuteCommand("led_basecolor_rgb FFFF00", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);



	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);



	CMD_ExecuteCommand("led_enableAll 0", 0);


	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	// the red button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(false);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(true);


}


void Test_Http_LED_SingleChannel() {

	SIM_ClearOBK(0);
	// Setup single PWM Device
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);

	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(false);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(false);
	// the green button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(true);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(false);

	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");

	CMD_ExecuteCommand("led_enableAll 0", 0);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll", 0);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll==1", 0);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll==0", 1);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll!=1", 1);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll!=2", 1);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll<1", 1);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll<0", 0);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 100);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");

	// HTML page must contains dimmer, but no RGB and no temeprature controls
	Test_FakeHTTPClientPacket_GET("index");
	SELFTEST_ASSERT_HTTP_HAS_LED_DIMMER(true);
	SELFTEST_ASSERT_HTTP_HAS_LED_TEMPERATURE(false);
	SELFTEST_ASSERT_HTTP_HAS_LED_RGB(false);
	// the red button is on the page
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_ON(false);
	SELFTEST_ASSERT_HTTP_HAS_BUTTON_LEDS_OFF(true);


	CMD_ExecuteCommand("led_dimmer 61", 0);
	SELFTEST_ASSERT_EXPRESSION("$led_dimmer", 61);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "OFF");


	CMD_ExecuteCommand("led_enableAll 1", 0);
	SELFTEST_ASSERT_EXPRESSION("$led_enableAll", 1);
	// StatusSTS contains POWER and Dimmer
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER("StatusSTS", "Dimmer", 61);
	SELFTEST_ASSERT_JSON_VALUE_STRING("StatusSTS", "POWER", "ON");



}

void Test_Http_LED() {
	Test_Http_LED_SingleChannel();
	Test_Http_LED_CW();
	Test_Http_LED_RGB();
	Test_Http_LED_RGBCW();
}

#endif

