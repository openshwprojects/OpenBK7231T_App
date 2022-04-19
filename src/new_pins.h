#ifndef __NEW_PINS_H__
#define __NEW_PINS_H__
#include "new_common.h"

#define BIT_SET(PIN,N) (PIN |=  (1<<N))
#define BIT_CLEAR(PIN,N) (PIN &= ~(1<<N))
#define BIT_TGL(PIN,N) (PIN ^=  (1<<N))
#define BIT_CHECK(PIN,N) !!((PIN & (1<<N)))

enum IORole {
	IOR_None,
	IOR_Relay,
	IOR_Relay_n,
	IOR_Button,
	IOR_Button_n,
	IOR_LED,
	IOR_LED_n,
	IOR_PWM,
	IOR_LED_WIFI,
	IOR_LED_WIFI_n,
	IOR_Button_ToggleAll,
	IOR_Button_ToggleAll_n,
	// this copies directly value on pin (0 or 1) to channel
	IOR_DigitalInput,
	// as above, but inverted
	IOR_DigitalInput_n,
	IOR_ToggleChannelOnToggle,
	IOR_Total_Options,
};

enum ChannelType {
	ChType_Default,
	ChType_Error,
	ChType_Temperature,
	ChType_Humidity,
	// most likely will not be used
	// but it's humidity value times 10
	// TuyaMCU sends 225 instead of 22.5%
	ChType_Humidity_div10,
	ChType_Temperature_div10,
	ChType_Toggle,
};


#if PLATFORM_BL602
#define PLATFORM_GPIO_MAX 24
#else
#define PLATFORM_GPIO_MAX 27
#endif

#define CHANNEL_MAX 64

typedef struct pinsState_s {
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[32];
	byte channels[32];
	// extra channels array - this is needed for 
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[32];
	// This single field above, is indexed by CHANNEL INDEX 
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

//
// Main config structure (less than 2KB)
//
// This config structure is supposed  to be saved only when user 
// changes the device configuration, so really not often.
// We should not worry about flash memory wear in this case.
// The saved-every-reboot values are stored elsewhere
// (i.e. saved channel states, reboot counter?)
typedef struct mainConfig_s {
	byte ident0;
	byte ident1;
	byte ident2;
	byte crc;
	int version;
	// unused
	int genericFlags;
	// unused
	int genericFlags2;
	unsigned short changeCounter;
	unsigned short otaCounter;
	// target wifi credentials
	char wifi_ssid[64];
	char wifi_pass[64];
	// MQTT information for Home Assistant
	char mqtt_host[256];
	char mqtt_brokerName[64];
	char mqtt_userName[64];
	char mqtt_pass[128];
	int mqtt_port;
	// addon JavaScript panel is hosted on external server
	char webappRoot[64];
	// TODO?
	byte mac[6];
	// TODO?
	char shortDeviceName[32];
	char longDeviceName[64];
	pinsState_t pins;
	byte unusedSectorA[256];
	byte unusedSectorB[128];
	byte unusedSectorC[128];
	char initCommandLine[512];
} mainConfig_t;

extern mainConfig_t g_cfg;

extern char g_enable_pins;


void PIN_Init(void);
void PIN_SetupPins();
void CFG_ClearPins();
int PIN_GetPinRoleForPinIndex(int index);
int PIN_GetPinChannelForPinIndex(int index);
int PIN_GetPinChannel2ForPinIndex(int index);
const char *PIN_GetPinNameAlias(int index);
void PIN_SetPinRoleForPinIndex(int index, int role);
void PIN_SetPinChannelForPinIndex(int index, int ch);
void PIN_SetPinChannel2ForPinIndex(int index, int ch);
void CHANNEL_Toggle(int ch);
void CHANNEL_DoSpecialToggleAll();
bool CHANNEL_Check(int ch);
void CHANNEL_SetChangeCallback(void (*cb)(int idx, int iVal));
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex));
void CHANNEL_Set(int ch, int iVal, int bForce);
int CHANNEL_Get(int ch);
int CHANNEL_GetRoleForOutputChannel(int ch);
// See: enum ChannelType
void CHANNEL_SetType(int ch, int type);
int CHANNEL_GetType(int ch);
void CHANNEL_SetAll(int iVal, bool bForce);
void CHANNEL_SetStateOnly(int iVal);
int CHANNEL_HasChannelPinWithRole(int ch, int iorType);


void PIN_set_wifi_led(int value);
int PIN_GetPWMIndexForPinIndex(int pin);

// from new_builtin.c
void Setup_Device_Empty();
void Setup_Device_TuyaWL_SW01_16A();
void Setup_Device_TuyaSmartLife4CH10A();
void Setup_Device_BK7231N_TuyaLightBulb_RGBCW_5PWMs();
void Setup_Device_IntelligentLife_NF101A();
void Setup_Device_TuyaLEDDimmerSingleChannel();
void Setup_Device_CalexLEDDimmerFiveChannel();
void Setup_Device_CalexPowerStrip_900018_1v1_0UK();
void Setup_Device_ArlecCCTDownlight();
void Setup_Device_NedisWIFIPO120FWT_16A();
void Setup_Device_NedisWIFIP130FWT_10A();
void Setup_Device_EmaxHome_EDU8774();
void Setup_Device_TuyaSmartPFW02G();
void Setup_Device_BK7231N_CB2S_QiachipSmartSwitch();
void Setup_Device_BK7231T_WB2S_QiachipSmartSwitch();
void Setup_Device_BK7231T_Raw_PrimeWiFiSmartOutletsOutdoor_CCWFIO232PK();
void Setup_Device_AvatarASL04();
void Setup_Device_TuyaSmartWIFISwith_4Gang_CB3S();
void Setup_Device_BL602_MagicHome_IR_RGB_LedStrip();
void Setup_Device_WiFi_DIY_Switch_WB2S_ZN268131();
void Setup_Device_BK7231N_CB2S_LSPA9_BL0942();

#endif

