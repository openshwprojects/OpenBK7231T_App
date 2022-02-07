
// I know this is not the best way to do this, and we can easily support config-strings like Tasmota
// but for now let's use that
#include "new_common.h"
#include "new_pins.h"

void Setup_Device_Empty() {
	PIN_ClearPins();

	PIN_SaveToFlash();

}

// https://www.elektroda.pl/rtvforum/topic3804553.html
// SmartSwitch Tuya WL-SW01_16 16A
void Setup_Device_TuyaWL_SW01_16A() {
	PIN_ClearPins();

	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 1);

	PIN_SaveToFlash();
}
// https://www.elektroda.pl/rtvforum/topic3822484.html
//  WiFi Tuya SmartLife 4CH 10A
void Setup_Device_TuyaSmartLife4CH10A() {
	PIN_ClearPins();

	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 1);
	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 2);
	PIN_SetPinRoleForPinIndex(9, IOR_Button);
	PIN_SetPinChannelForPinIndex(9, 3);
	PIN_SetPinRoleForPinIndex(1, IOR_Button);
	PIN_SetPinChannelForPinIndex(1, 4);

	PIN_SetPinRoleForPinIndex(14, IOR_Relay);
	PIN_SetPinChannelForPinIndex(14, 1);
	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 2);
	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 3);
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 4);

	PIN_SaveToFlash();
}
// https://www.elektroda.pl/rtvforum/viewtopic.php?p=19743751#19743751
void Setup_Device_IntelligentLife_NF101A() {
	PIN_ClearPins();

	// TODO: LED

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(6, IOR_Button);
	PIN_SetPinChannelForPinIndex(6, 1);


	PIN_SaveToFlash();
}	
// https://www.elektroda.pl/rtvforum/topic3798114.html
void Setup_Device_TuyaLEDDimmerSingleChannel() {
	PIN_ClearPins();

	// pin 8 has PWM 
	PIN_SetPinRoleForPinIndex(8, IOR_Relay);
	PIN_SetPinChannelForPinIndex(8, 1);

	// button is on RXD2, which is a debug uart..
	PIN_SetPinRoleForPinIndex(1, IOR_Button);
	PIN_SetPinChannelForPinIndex(1, 1);


	PIN_SaveToFlash();
}


void Setup_Device_CalexLEDDimmerFiveChannel() {

	// pins are:
	// red - PWM2 = P7
	// green - PWM3 = P8
	// blue - PWM1 = P6
	// warm white - PWM5 = P26
	// cold white - PWM4 = P24

	PIN_ClearPins();

	// red
	PIN_SetPinChannelForPinIndex(7, 1);
	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	// green
	PIN_SetPinChannelForPinIndex(8, 2);
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	// blue
	PIN_SetPinChannelForPinIndex(6, 3);
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	// cold white
	PIN_SetPinChannelForPinIndex(24, 4);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	// warm white
	PIN_SetPinChannelForPinIndex(26, 5);
	PIN_SetPinRoleForPinIndex(26, IOR_PWM);

	PIN_SaveToFlash();
}

void Setup_Device_CalexPowerStrip_900018_1v1_0UK() {

	// pins are:
	// red - PWM2 = P7
	// green - PWM3 = P8
	// blue - PWM1 = P6
	// warm white - PWM5 = P26
	// cold white - PWM4 = P24

	PIN_ClearPins();

	// relays - 4 sockets + 1 USB
	PIN_SetPinChannelForPinIndex(6, 5);
	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 2);
	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(8, 3);
	PIN_SetPinRoleForPinIndex(8, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 4);
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);

	// button
	PIN_SetPinChannelForPinIndex(14, 1);
	PIN_SetPinRoleForPinIndex(14, IOR_Button);

	// 2 x LEDs
	// wifi stat
	PIN_SetPinChannelForPinIndex(10, 1);
	PIN_SetPinRoleForPinIndex(10, IOR_LED);
	// power stat
	PIN_SetPinChannelForPinIndex(24, 2);
	PIN_SetPinRoleForPinIndex(24, IOR_LED);

	PIN_SaveToFlash();
}

// https://www.bunnings.com.au/arlec-grid-connect-smart-9w-cct-led-downlight_p0168694
void Setup_Device_ArlecCCTDownlight() {

	// WB3L
	// pins are:
	// cold white - PWM1 = P6
	// warm white - PWM2 = P24

	PIN_ClearPins();

	// cold white
	PIN_SetPinChannelForPinIndex(6, 1);
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	// warm white
	PIN_SetPinChannelForPinIndex(24, 2);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);

	PIN_SaveToFlash();
}
