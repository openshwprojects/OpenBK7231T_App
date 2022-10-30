
// I know this is not the best way to do this, and we can easily support config-strings like Tasmota
// but for now let's use that
#include "new_common.h"
#include "new_pins.h"
#include "new_cfg.h"

void Setup_Device_Empty() {
	CFG_ClearPins();

	CFG_Save_SetupTimer();

}

void Setup_Device_WB2L_FCMila_Smart_Spotlight_Gu10() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	PIN_SetPinChannelForPinIndex(6, 0);

	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 3);

	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 4);

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 2);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();

}

void Setup_Device_WiFi_DIY_Switch_WB2S_ZN268131() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(7, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_ToggleChannelOnToggle);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();

}

// https://www.elektroda.pl/rtvforum/topic3881416.html
void Setup_Device_BL602_MagicHome_IR_RGB_LedStrip() {
	CFG_ClearPins();

	// red
	PIN_SetPinRoleForPinIndex(4, IOR_PWM);
	PIN_SetPinChannelForPinIndex(4, 0);

	// green
	PIN_SetPinRoleForPinIndex(3, IOR_PWM);
	PIN_SetPinChannelForPinIndex(3, 1);

	// blue
	PIN_SetPinRoleForPinIndex(21, IOR_PWM);
	PIN_SetPinChannelForPinIndex(21, 2);

	// dummy unused channel 4 with place on pcb for transistor
	//PIN_SetPinRoleForPinIndex(20, IOR_Relay);
	//PIN_SetPinChannelForPinIndex(20, 3);

	// IR recv
	//PIN_SetPinRoleForPinIndex(12, IOR_IR_RECV);
	//PIN_SetPinChannelForPinIndex(12, 0);

	CFG_Save_SetupTimer();
}

// MagicHome CCT : ZJ-BWCE-CCT v1.1
void Setup_Device_BL602_MagicHome_CCT_LedStrip()
{
	CFG_ClearPins();

	// White CCT
	PIN_SetPinRoleForPinIndex(20, IOR_PWM);
	PIN_SetPinChannelForPinIndex(20, 1);

	CFG_Save_SetupTimer();
}	

// DeviceName: Sonoff MiniR3
// ShortDeviceName: minir3
// https://sonoff.tech/product/minir3
void Setup_Device_Sonoff_MiniR3() {
	CFG_ClearPins();
	// P1 WIFI LED inverted
	PIN_SetPinRoleForPinIndex(1, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(1, 1);
	// P5 Relay Button
	PIN_SetPinRoleForPinIndex(5, IOR_Button);
	PIN_SetPinChannelForPinIndex(5, 1);
	// P22 Relay
	PIN_SetPinRoleForPinIndex(22, IOR_Relay);
	PIN_SetPinChannelForPinIndex(22, 1);

	CFG_Save_SetupTimer();
}

// https://www.elektroda.pl/rtvforum/topic3804553.html
// SmartSwitch Tuya WL-SW01_16 16A
void Setup_Device_TuyaWL_SW01_16A() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}
// https://www.elektroda.pl/rtvforum/topic3822484.html
//  WiFi Tuya SmartLife 4CH 10A
void Setup_Device_TuyaSmartLife4CH10A() {
	CFG_ClearPins();

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

	CFG_Save_SetupTimer();
}
// Tuya "12W" smart light bulb
// "Tuya Wifi Smart Life Light Bulb Lamp E27 LED RGBCW Dimmable For Alexa/Google 18W
// See this topic: https://www.elektroda.pl/rtvforum/viewtopic.php?t=3880540&highlight=
void Setup_Device_BK7231N_TuyaLightBulb_RGBCW_5PWMs() {
	CFG_ClearPins();

	// RGBCW, in that order
	// Raw PWMS (no I2C)

	// P26 - red
	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 1);

	// P8 - green
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 2);

	// P7 - blue
	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 3);

	// P9 - cold white
	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 4);

	// P6 - warm white
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	PIN_SetPinChannelForPinIndex(6, 5);

	CFG_Save_SetupTimer();
}
// https://www.elektroda.pl/rtvforum/viewtopic.php?p=19743751#19743751
void Setup_Device_IntelligentLife_NF101A() {
	CFG_ClearPins();

	// TODO: LED

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(6, IOR_Button);
	PIN_SetPinChannelForPinIndex(6, 1);


	CFG_Save_SetupTimer();
}
// https://www.elektroda.pl/rtvforum/topic3798114.html
void Setup_Device_TuyaLEDDimmerSingleChannel() {
	CFG_ClearPins();

	// pin 8 has PWM
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 1);

	// button is on RXD2, which is a debug uart..
	PIN_SetPinRoleForPinIndex(1, IOR_Button);
	PIN_SetPinChannelForPinIndex(1, 1);


	CFG_Save_SetupTimer();
}


void Setup_Device_CalexLEDDimmerFiveChannel() {

	// pins are:
	// red - PWM2 = P7
	// green - PWM3 = P8
	// blue - PWM1 = P6
	// warm white - PWM5 = P26
	// cold white - PWM4 = P24

	CFG_ClearPins();

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

	// IR Input
	//PIN_SetPinChannelForPinIndex(25, 6);
	//PIN_SetPinRoleForPinIndex(25, IO_IRIN);


	CFG_Save_SetupTimer();
}

void Setup_Device_CalexPowerStrip_900018_1v1_0UK() {

	// pins are:
	// red - PWM2 = P7
	// green - PWM3 = P8
	// blue - PWM1 = P6
	// warm white - PWM5 = P26
	// cold white - PWM4 = P24

	CFG_ClearPins();

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

	CFG_Save_SetupTimer();
}

// https://www.bunnings.com.au/arlec-grid-connect-smart-9w-cct-led-downlight_p0168694
void Setup_Device_ArlecCCTDownlight() {

	// WB3L
	// pins are:
	// cold white - PWM1 = P6
	// warm white - PWM2 = P24

	CFG_ClearPins();

	// cold white
	PIN_SetPinChannelForPinIndex(6, 4);
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	// warm white
	PIN_SetPinChannelForPinIndex(24, 5);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);

	CFG_Save_SetupTimer();
}

// https://www.bunnings.com.au/arlec-grid-connect-smart-9w-rgb-cct-led-downlight_p0168695
void Setup_Device_ArlecRGBCCTDownlight() {

	// WB3L
	// pins are:
	// red = PWM1 = P8
	// green = PWM2 = P7
	// blue = PWM3 = P9
	// cold white - PWM4 = P6
	// warm white - PWM5 = P24

	CFG_ClearPins();
	// red
	PIN_SetPinChannelForPinIndex(8, 1);
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	// green
	PIN_SetPinChannelForPinIndex(7, 2);
	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	// blue
	PIN_SetPinChannelForPinIndex(9, 3);
	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	// cold white
	PIN_SetPinChannelForPinIndex(6, 4);
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	// warm white
	PIN_SetPinChannelForPinIndex(24, 5);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);

	CFG_Save_SetupTimer();
}

// https://www.elektroda.com/rtvforum/viewtopic.php?p=20123466#20123466
// CasaLife CCT Downlight SMART-AL2017-TGTS
// Sold by ALDI Australia
void Setup_Device_CasaLifeCCTDownlight() {

	// WB2L
	// pins are:
	// color temperature - PWM1 = P7
	// brightness - PWM2 = P8

	CFG_ClearPins();

	// color temperature
	PIN_SetPinChannelForPinIndex(7, 0);
	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	// brightness
	PIN_SetPinChannelForPinIndex(8, 1);
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	// Raw PWM values already control temperature/brighness so enable "Flag 3 -[LED][Debug] Show raw PWM controller on WWW index instead of new LED RGB/CW/etc picker"
	CFG_SetFlag(OBK_FLAG_LED_RAWCHANNELSMODE, true);

	CFG_Save_SetupTimer();
}

// https://www.elektroda.pl/rtvforum/topic3804553.html
// SmartSwitch Nedis WIFIPO120FWT
void Setup_Device_NedisWIFIPO120FWT_16A() {

	// WB2S
	// Pins are:
	// Led - PWM0 - P6
	// BL0937-CF - PWM1 - P7
	// BL0937-CF1- PWM2 - P8
	// Button - RX1  - P10
	// BL0937-SEL - PWM4 - P24
	// Relay - PWM5 - P26


	CFG_ClearPins();
	// LEd
	PIN_SetPinRoleForPinIndex(6, IOR_LED);
	PIN_SetPinChannelForPinIndex(6, 1);
	// Button
	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(26, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// https://www.elektroda.pl/rtvforum/topic3804553.html
// SmartSwitch Nedis WIFIP130FWT
void Setup_Device_NedisWIFIP130FWT_10A() {

	// WB2S
	// Pins are:
	// Led - PWM0 - P6
	// Button - RX1  - P10
	// Relay - PWM5 - P26


	CFG_ClearPins();
	// Led
	PIN_SetPinRoleForPinIndex(6, IOR_LED);
	PIN_SetPinChannelForPinIndex(6, 1);
	// Button
	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// https://www.elektroda.com/rtvforum/topic3819498.html
//
void Setup_Device_TH06_LCD_RTCC_WB3S() {

}

// https://www.elektroda.pl/rtvforum/topic3804553.html
// SmartSwitch Emax Home EDU8774 16A
void Setup_Device_EmaxHome_EDU8774() {

	// WB2S
	// Pins are:
	// BL0937-CF - PWM0 - P6
	// BL0937-CF1 - PWM1 - P7
	// BL0937-SEL - PWM2 - P8
	// Button - RX1 - P10
	// Relay - PWM4 - P24
	// Led - PWM5 - P26


	CFG_ClearPins();
	// Button
	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(24, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(24, 1);
	// Led
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// LSPA9
// See teardown article here:
// https://www.elektroda.pl/rtvforum/viewtopic.php?t=3887748&highlight=
void Setup_Device_BK7231N_CB2S_LSPA9_BL0942() {



	CFG_ClearPins();
	// Button
	PIN_SetPinRoleForPinIndex(6, IOR_Button);
	PIN_SetPinChannelForPinIndex(6, 1);
	// LED
	PIN_SetPinRoleForPinIndex(8, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(8, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 1);
	// Led

	CFG_SetShortStartupCommand_AndExecuteNow("backlog startDriver BL0942; VREF 15987.125000; PREF -683.023987; IREF 272302.687500");

	CFG_Save_SetupTimer();
}

// QiachipSmartSwitch
// See teardown article here:
// https://www.elektroda.pl/rtvforum/viewtopic.php?t=3874289&highlight=
void Setup_Device_BK7231N_CB2S_QiachipSmartSwitch() {



	CFG_ClearPins();
	// Button
	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(8, IOR_Relay);
	PIN_SetPinChannelForPinIndex(8, 1);
	// Led

	CFG_Save_SetupTimer();
}

void Setup_Device_BK7231N_KS_602_TOUCH() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(17, IOR_Relay);
	PIN_SetPinChannelForPinIndex(17, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_SetFlag(OBK_FLAG_BTN_INSTANTTOUCH,true);
	CFG_Save_SetupTimer();
}

void Setup_Device_BK7231T_WB2S_QiachipSmartSwitch() {
	CFG_ClearPins();
	// Button
	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(6, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(6, 1);
	// Led
	PIN_SetPinRoleForPinIndex(10, IOR_LED);
	PIN_SetPinChannelForPinIndex(10, 1);

	CFG_Save_SetupTimer();
}



// Strigona donation
// Teardown article: https://www.elektroda.pl/rtvforum/viewtopic.php?p=19906670#19906670
// https://obrazki.elektroda.pl/6606464600_1642467157.jpg
// NOTE: It used to be ESP-based https://templates.blakadder.com/prime_CCWFIO232PK.html
void Setup_Device_BK7231T_Raw_PrimeWiFiSmartOutletsOutdoor_CCWFIO232PK() {



	CFG_ClearPins();
	// Relay
	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 2);
	// Led
	PIN_SetPinRoleForPinIndex(10, IOR_LED);
	PIN_SetPinChannelForPinIndex(10, 1);
	// Led
	PIN_SetPinRoleForPinIndex(26, IOR_LED);
	PIN_SetPinChannelForPinIndex(26, 2);

	// Single button
	PIN_SetPinRoleForPinIndex(24, IOR_Button);
	PIN_SetPinChannelForPinIndex(24, 1);


	CFG_Save_SetupTimer();
}


// https://www.tokmanni.fi/alypistorasia-home-connect-ip20-6419860720456
// Marked as Smart-PFW02-G
// Relay (with npn-transistor) at PWM4 P24
// Button PWM5 P26
// LED PWM1 P7
void Setup_Device_TuyaSmartPFW02G() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(24, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 1);

	PIN_SetPinRoleForPinIndex(7, IOR_LED);
	PIN_SetPinChannelForPinIndex(7, 1);


	CFG_Save_SetupTimer();
}


void Setup_Device_AvatarASL04() {

	// pins are:
	// red - PWM1 = P24
	// green - PWM2 = P6
	// blue - PWM3 = P8

	// buttons
	// music - P7
	// color - P9
	// on/off - P14

	// IR - P14

	// audio input ???? - most likely P23/ADC?

	CFG_ClearPins();

	// red
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	// green
	PIN_SetPinChannelForPinIndex(8, 2);
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	// blue
	PIN_SetPinChannelForPinIndex(6, 3);
	PIN_SetPinRoleForPinIndex(6, IOR_PWM);


	// just set to buttons 1/2/3 for the moment
	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(9, IOR_Button);
	PIN_SetPinChannelForPinIndex(9, 2);

	PIN_SetPinRoleForPinIndex(14, IOR_Button);
	PIN_SetPinChannelForPinIndex(14, 3);


	CFG_Save_SetupTimer();
}


void Setup_Device_TuyaSmartWIFISwith_4Gang_CB3S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(24, IOR_Button);
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(20, IOR_Button);
	PIN_SetPinChannelForPinIndex(20, 2);
	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 3);
	PIN_SetPinRoleForPinIndex(14, IOR_Button);
	PIN_SetPinChannelForPinIndex(14, 4);

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);
	PIN_SetPinRoleForPinIndex(8, IOR_Relay);
	PIN_SetPinChannelForPinIndex(8, 2);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 3);
	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 4);

	PIN_SetPinRoleForPinIndex(22, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(22, 1);

	CFG_Save_SetupTimer();
}


void Setup_Device_LSC_Smart_Connect_Plug_CB2S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_LED);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(7, IOR_Button);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}


void Setup_Device_DS_102_1Gang_WB3S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(1, IOR_LED_WIFI);
	PIN_SetPinChannelForPinIndex(1, 1);

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_LED);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// see https://www.elektroda.com/rtvforum/viewtopic.php?p=20008969#20008969

void Setup_Device_BK7231T_Gosund_Switch_SW5_A_V2_1(){
	CFG_ClearPins();

	// green
	PIN_SetPinRoleForPinIndex(7, IOR_LED_WIFI);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(14, IOR_Relay);
	PIN_SetPinChannelForPinIndex(14, 1);

	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 1);

	// red
	PIN_SetPinRoleForPinIndex(16, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(16, 1);


	CFG_Save_SetupTimer();
}

void Setup_Device_DS_102_2Gang_WB3S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(1, IOR_LED_WIFI);
	PIN_SetPinChannelForPinIndex(1, 1);

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_LED);
	PIN_SetPinChannelForPinIndex(26, 1);

	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 2);

	PIN_SetPinRoleForPinIndex(11, IOR_LED);
	PIN_SetPinChannelForPinIndex(11, 2);

	CFG_Save_SetupTimer();
}


void Setup_Device_DS_102_3Gang_WB3S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(1, IOR_LED_WIFI);
	PIN_SetPinChannelForPinIndex(1, 1);

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);
	PIN_SetPinRoleForPinIndex(24, IOR_Button);
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(14, IOR_LED);
	PIN_SetPinChannelForPinIndex(14, 1);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 2);
	PIN_SetPinRoleForPinIndex(10, IOR_Button);
	PIN_SetPinChannelForPinIndex(10, 2);
	PIN_SetPinRoleForPinIndex(26, IOR_LED);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 3);
	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 3);
	PIN_SetPinRoleForPinIndex(11, IOR_LED);
	PIN_SetPinChannelForPinIndex(11, 3);


	CFG_Save_SetupTimer();
}

// https://obrazki.elektroda.pl/8563462300_1652196315.jpg
// https://obrazki.elektroda.pl/8998188300_1652196330.jpg
// https://obrazki.elektroda.pl/2281136200_1652196339.jpg
// https://obrazki.elektroda.pl/1568452900_1652196348.jpg
// https://obrazki.elektroda.pl/4600787700_1652196323.jpg
void Setup_Device_13A_Socket_CB2S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_LED);
	PIN_SetPinChannelForPinIndex(6, 0);
	PIN_SetPinRoleForPinIndex(7, IOR_LED);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 0);
	PIN_SetPinChannel2ForPinIndex(8, 1);

	PIN_SetPinRoleForPinIndex(24, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(24, 0);
	PIN_SetPinRoleForPinIndex(25, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(25, 1);

	CFG_Save_SetupTimer();
}

//Australian 2 Gang Wall Switch
//https://obrazki.elektroda.pl/2789424600_1656890909.jpg
void Setup_Device_Deta_Smart_Double_Power_Point_6922HA_Series2() {
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_Relay);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(9, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(14, IOR_Button);
	PIN_SetPinChannelForPinIndex(14, 1);

	PIN_SetPinRoleForPinIndex(24, IOR_Button);
	PIN_SetPinChannelForPinIndex(24, 2);

	PIN_SetPinRoleForPinIndex(26, IOR_Relay);
	PIN_SetPinChannelForPinIndex(26, 2);

	CFG_Save_SetupTimer();
}

//https://enbrightenme.com/enbrighten-indoor-plug-in-mini-wifi-smart-switch-2-pack-white
//https://www.elektroda.com/rtvforum/viewtopic.php?p=20133554#20133554
void Setup_Device_Enbrighten_WFD4103(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(7, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// Aubess Mini Smart Switch 16A
void Setup_Device_Aubess_Mini_Smart_Switch_16A() {

	// pins are:
	// led - led_n = P6
	// embedded button - btn = P8
	// switch - btn = P14
	// relay - rel = P15

	CFG_ClearPins();
	// Led
	PIN_SetPinRoleForPinIndex(6, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(6, 1);	
	// Embedded Button
	PIN_SetPinRoleForPinIndex(8, IOR_Button);
	PIN_SetPinChannelForPinIndex(8, 1);
	// Switch
	PIN_SetPinRoleForPinIndex(14, IOR_Button);
	PIN_SetPinChannelForPinIndex(14, 1);	
	// Relay
	PIN_SetPinRoleForPinIndex(15, IOR_Relay);
	PIN_SetPinChannelForPinIndex(15, 1);

	CFG_Save_SetupTimer();
}

// Aubess Mini Smart Switch 16A, alternative pin configuration
// https://www.aliexpress.com/item/1005002740558678.html
void Setup_Device_BK7231N_Aubess_A5C_30E1_02() {
	CFG_ClearPins();
	// LED
	PIN_SetPinRoleForPinIndex(7, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(7, 1);
	// Embedded Button
	PIN_SetPinRoleForPinIndex(6, IOR_Button);
	PIN_SetPinChannelForPinIndex(6, 1);
	// Relay
	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);
	// External toggle sensor
	PIN_SetPinRoleForPinIndex(26, IOR_ToggleChannelOnToggle);
	PIN_SetPinChannelForPinIndex(26, 1);

	CFG_Save_SetupTimer();
}

// Zemismart Light Switch (Neutral Optional) KS_811_3
// https://www.aliexpress.com/item/4000979779573.html
// Has a BK7231N direct on board, but easy access to UART on back with removal of 4 screws.
// There is a single and 2 switch version of this, which should be easy to template.
// Device Info, Pics and Flashing Notes: https://zorruno.com/2022/zemismart-ks-811-with-openbk7231n-openbeken/
void Setup_Device_Zemismart_Light_Switch_KS_811_3() {

	// GPIO:
	// Buttons 1,2,3 = P17,P26,P24
	// Relays/Red LEDs 1,2,3 = P14,P16,P15
	// All LEDs Blue/Off = P22 (Have used this as Wifi Status)

	CFG_ClearPins();
	// 3 Leds Blue (Wifi Status, All 3 flash together)
	PIN_SetPinRoleForPinIndex(22, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(22, 1);	
	// Buttons
	PIN_SetPinRoleForPinIndex(17, IOR_Button);
	PIN_SetPinChannelForPinIndex(17, 1);	
	PIN_SetPinRoleForPinIndex(26, IOR_Button);
	PIN_SetPinChannelForPinIndex(26, 2);	
	PIN_SetPinRoleForPinIndex(24, IOR_Button);
	PIN_SetPinChannelForPinIndex(24, 3);	
	// Relay, Individual LEDs Red (Relays and LEDs are on same GPIO Pins)
	PIN_SetPinRoleForPinIndex(14, IOR_Relay);
	PIN_SetPinChannelForPinIndex(14, 1);	
	PIN_SetPinRoleForPinIndex(16, IOR_Relay);
	PIN_SetPinChannelForPinIndex(16, 2);
	PIN_SetPinRoleForPinIndex(15, IOR_Relay);
	PIN_SetPinChannelForPinIndex(15, 3);

	CFG_Save_SetupTimer();
}

void Setup_Device_TeslaSmartPlus_TSL_SPL_1() {

    // CB2S
    // Pins are:
    // BL0937-CF1 - P6
    // BL0937-CF - P7
    // BL0937-SEL - P24
    // Button - RX1 - P10
    // Relay - P26
    // Led - P8


    CFG_ClearPins();
    // Button
    PIN_SetPinRoleForPinIndex(10, IOR_Button_n);
    PIN_SetPinChannelForPinIndex(10, 0);
    // Relay
    PIN_SetPinRoleForPinIndex(8, IOR_LED_WIFI_n);
    PIN_SetPinChannelForPinIndex(8, 0);
    // Led
    PIN_SetPinRoleForPinIndex(26, IOR_Relay);
    PIN_SetPinChannelForPinIndex(26, 0);
    // BL0937
    PIN_SetPinRoleForPinIndex(6, IOR_BL0937_CF1);
    PIN_SetPinChannelForPinIndex(6, 0);
    PIN_SetPinRoleForPinIndex(7, IOR_BL0937_CF);
    PIN_SetPinChannelForPinIndex(7, 0);
    PIN_SetPinRoleForPinIndex(24, IOR_BL0937_SEL);
    PIN_SetPinChannelForPinIndex(24, 0);

    CFG_SetFlag(2,1);
    CFG_SetFlag(10,1);

    CFG_Save_SetupTimer();
}

void Setup_Device_Calex_900011_1_WB2S(){
	CFG_ClearPins();

	PIN_SetPinRoleForPinIndex(6, IOR_Button);
	PIN_SetPinChannelForPinIndex(6, 1);

	PIN_SetPinRoleForPinIndex(7, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(7, 1);

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(8, IOR_LED_WIFI_n);
	PIN_SetPinChannelForPinIndex(8, 2);

	CFG_Save_SetupTimer();
}
void Setup_Device_Immax_NEO_LITE_NAS_WR07W()
{
    CFG_ClearPins();

    // Button
    PIN_SetPinRoleForPinIndex(9, IOR_Button_n);
    PIN_SetPinChannelForPinIndex(9, 0);
    // Relay
    PIN_SetPinRoleForPinIndex(6, IOR_LED_WIFI_n);
    PIN_SetPinChannelForPinIndex(6, 0);
    // Led
    PIN_SetPinRoleForPinIndex(26, IOR_Relay);
    PIN_SetPinChannelForPinIndex(26, 0);

    CFG_SetShortStartupCommand("backlog startDriver BL0942");
    CFG_SetFlag(2,1);
    CFG_SetFlag(10,1);

    CFG_Save_SetupTimer();
}

void Setup_Device_MOES_TouchSwitch_WS_EU1_RFW_N()
{
    CFG_ClearPins();
    CFG_SetShortStartupCommand("backlog startDriver TuyaMCU; setChannelType 1 toggle; linkTuyaMCUOutputToChannel 1 bool 1");
    CFG_SetFlag(2,1);
    CFG_SetFlag(10,1);
    CFG_Save_SetupTimer();
}

