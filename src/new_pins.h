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
	IOR_Total_Options,
};

// NOTE: you must keep size of this structure in sync with
// structures in flash config code for both BK7231 and XR809
typedef struct pinsState_s {
	byte roles[32];
	byte channels[32];
} pinsState_t;


extern pinsState_t g_pins;


#define GPIO_MAX 27
#define CHANNEL_MAX 32

void PIN_Init(void);
void PIN_ClearPins();
int PIN_GetPinRoleForPinIndex(int index);
int PIN_GetPinChannelForPinIndex(int index);
const char *PIN_GetPinNameAlias(int index);
void PIN_SetPinRoleForPinIndex(int index, int role);
void PIN_SetPinChannelForPinIndex(int index, int ch);
void CHANNEL_Toggle(int ch);
bool CHANNEL_Check(int ch);
void CHANNEL_SetChangeCallback(void (*cb)(int idx, int iVal));
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex));
void CHANNEL_Set(int ch, int iVal, int bForce);
int CHANNEL_Get(int ch);
void PIN_SaveToFlash();
void PIN_LoadFromFlash();
void PIN_set_wifi_led(int value);

// from new_builtin.c
void Setup_Device_Empty();
void Setup_Device_TuyaWL_SW01_16A();
void Setup_Device_TuyaSmartLife4CH10A();
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

#endif

