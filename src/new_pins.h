#ifndef __NEW_PINS_H__
#define __NEW_PINS_H__
#include "new_common.h"


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
	// this copies directly value on pin (0 or 1) to channel
	IOR_DigitalInput_NoPup,
	// as above, but inverted
	IOR_DigitalInput_NoPup_n,
	// energy sensor
	IOR_BL0937_SEL,
	IOR_BL0937_CF,
	IOR_BL0937_CF1,
	IOR_ADC,
	IOR_SM2135_DAT,
	IOR_SM2135_CLK,
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
	ChType_Dimmer,
	ChType_LowMidHigh,
	ChType_TextField,
	ChType_ReadOnly,
	ChType_OffLowMidHigh,
};


#if PLATFORM_BL602
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_XR809
#define PLATFORM_GPIO_MAX 13
#else
#define PLATFORM_GPIO_MAX 29
#endif

#define CHANNEL_MAX 64

#define SPECIAL_CHANNEL_BRIGHTNESS 129
#define SPECIAL_CHANNEL_LEDPOWER 130
#define SPECIAL_CHANNEL_BASECOLOR 131
#define SPECIAL_CHANNEL_TEMPERATURE 132

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


// bit indexes (not values), so 0 1 2 3 4
#define OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER	0	
#define OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR		1
#define OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE	2
#define OBK_FLAG_LED_RAWCHANNELSMODE				3
#define OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER		4
#define OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER		5

#define OBK_TOTAL_FLAGS 6

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
	short startChannelValues[CHANNEL_MAX];
	int dgr_sendFlags;
	int dgr_recvFlags;
	char dgr_name[16];
	byte unusedSectorA[104];
	byte unusedSectorB[128];
	byte unusedSectorC[55];
	byte timeRequiredToMarkBootSuccessfull;
	int ping_interval;
	int ping_seconds;
	char ping_host[64];
	char initCommandLine[512];
} mainConfig_t;

extern mainConfig_t g_cfg;

extern char g_enable_pins;

#define CHANNEL_SET_FLAG_FORCE		1
#define CHANNEL_SET_FLAG_SKIP_MQTT	2
#define CHANNEL_SET_FLAG_SILENT		4

void PIN_AddCommands(void);
void PIN_SetupPins();
void PIN_StartButtonScanThread(void);
void PIN_OnReboot();
void CFG_ClearPins();
int PIN_CountPinsWithRole(int role);
int PIN_GetPinRoleForPinIndex(int index);
int PIN_GetPinChannelForPinIndex(int index);
int PIN_GetPinChannel2ForPinIndex(int index);
int PIN_FindPinIndexForRole(int role, int defaultIndexToReturnIfNotFound);
const char *PIN_GetPinNameAlias(int index);
void PIN_SetPinRoleForPinIndex(int index, int role);
void PIN_SetPinChannelForPinIndex(int index, int ch);
void PIN_SetPinChannel2ForPinIndex(int index, int ch);
void CHANNEL_Toggle(int ch);
void CHANNEL_DoSpecialToggleAll();
bool CHANNEL_Check(int ch);
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex));
// CHANNEL_SET_FLAG_*
void CHANNEL_Set(int ch, int iVal, int iFlags);
void CHANNEL_Add(int ch, int iVal);
void CHANNEL_AddClamped(int ch, int iVal, int min, int max);
int CHANNEL_Get(int ch);
int CHANNEL_GetRoleForOutputChannel(int ch);
// See: enum ChannelType
void CHANNEL_SetType(int ch, int type);
int CHANNEL_GetType(int ch);
// CHANNEL_SET_FLAG_*
void CHANNEL_SetAll(int iVal, int iFlags);
void CHANNEL_SetStateOnly(int iVal);
int CHANNEL_HasChannelPinWithRole(int ch, int iorType);
bool CHANNEL_IsInUse(int ch);
void Channel_SaveInFlashIfNeeded(int ch);
bool CHANNEL_HasChannelSomeOutputPin(int ch);

int PIN_GetPWMIndexForPinIndex(int pin);

int PIN_ParsePinRoleName(const char *name);

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
void Setup_Device_ArlecRGBCCTDownlight();
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
void Setup_Device_LSC_Smart_Connect_Plug_CB2S();
void Setup_Device_DS_102_1Gang_WB3S();
void Setup_Device_DS_102_2Gang_WB3S();
void Setup_Device_DS_102_3Gang_WB3S();
void Setup_Device_BK7231T_Gosund_Switch_SW5_A_V2_1();
void Setup_Device_13A_Socket_CB2S();
void Setup_Device_Deta_Smart_Double_Power_Point_6922HA_Series2();

#endif

