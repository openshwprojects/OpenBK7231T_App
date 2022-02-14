



#include "new_pins.h"
#include "httpserver/new_http.h"
#include "logging/logging.h"

#if WINDOWS

#elif PLATFORM_XR809

// XR809 sysinfo is used to save configuration to flash
#include "common/framework/sysinfo.h"
#include "driver/chip/hal_gpio.h"

#define os_memcpy memcpy

#else
#include "flash_config/flash_config.h"
#include <gpio_pub.h>

#include "../../beken378/func/include/net_param_pub.h"
#include "../../beken378/func/user_driver/BkDriverPwm.h"
#undef PR_DEBUG
#undef PR_NOTICE
#define PR_DEBUG addLog
#define PR_NOTICE addLog

typedef struct item_pins_config
{
	INFO_ITEM_ST head;
	pinsState_t pins;
}ITEM_PINS_CONFIG,*ITEM_PINS_CONFIG_PTR;


#endif



/*
// from BkDriverPwm.h"
OSStatus bk_pwm_start(bk_pwm_t pwm);

OSStatus bk_pwm_update_param(bk_pwm_t pwm, uint32_t frequency, uint32_t duty_cycle);

OSStatus bk_pwm_stop(bk_pwm_t pwm);
*/
//char g_pwmIndexForPinIndexMapping []
//{
//	// pin 0
//
//
//};

// pwm mqtt
// https://community.home-assistant.io/t/shelly-dimmer-with-mqtt/149871
// https://community.home-assistant.io/t/mqtt-light-problems-with-dimmer-devices/120822
// https://community.home-assistant.io/t/mqtt-dimming/96447/2
// https://community.home-assistant.io/t/mqtt-dimmer-switch-doesnt-display-brightness-slider/15471/2

int PIN_GetPWMIndexForPinIndex(int pin) {
	if(pin == 6)
		return 0;
	if(pin == 7)
		return 1;
	if(pin == 8)
		return 2;
	if(pin == 9)
		return 3;
	if(pin == 24)
		return 4;
	if(pin == 26)
		return 5;
	return -1;
}

#if PLATFORM_XR809
typedef struct xr809pin_s {
	const char *name;
	int port;
	int pin;
} xr809pin_t;

// https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g

xr809pin_t g_xrPins[] = {
	{ "PA19", GPIO_PORT_A, GPIO_PIN_19 },
	{ "PB03", GPIO_PORT_B, GPIO_PIN_3 },
	{ "PA12", GPIO_PORT_A, GPIO_PIN_12 },
	{ "PA14", GPIO_PORT_A, GPIO_PIN_14 },
	{ "PA15", GPIO_PORT_A, GPIO_PIN_15 },
	{ "PA06", GPIO_PORT_A, GPIO_PIN_6 },
	{ "PA07", GPIO_PORT_A, GPIO_PIN_7 },
	{ "PA16", GPIO_PORT_A, GPIO_PIN_16 },
	{ "PA20", GPIO_PORT_A, GPIO_PIN_20 },
	{ "PA21", GPIO_PORT_A, GPIO_PIN_21 },
	{ "PA22", GPIO_PORT_A, GPIO_PIN_22 },
	{ "PB02", GPIO_PORT_B, GPIO_PIN_2 },
	{ "PA08", GPIO_PORT_A, GPIO_PIN_8 },
};
int g_numXRPins = sizeof(g_xrPins) / sizeof(g_xrPins[0]);

void PIN_XR809_GetPortPinForIndex(int index, int *xr_port, int *xr_pin) {
	if(index < 0 || index >= g_numXRPins) {
		*xr_port = 0;
		*xr_pin = 0;
		return;
	}
	*xr_port = g_xrPins[index].port;
	*xr_pin = g_xrPins[index].pin;
}

#endif
// it was nice to have it as bits but now that we support PWM...
//int g_channelStates;
unsigned char g_channelValues[GPIO_MAX] = { 0 };

#ifdef WINDOWS

#elif PLATFORM_XR809

#else
BUTTON_S g_buttons[GPIO_MAX];

#endif
pinsState_t g_pins;

void (*g_channelChangeCallback)(int idx, int iVal) = 0;
void (*g_doubleClickCallback)(int pinIndex) = 0;


void PIN_SaveToFlash() {
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
	int res;
	inf = sysinfo_get();
	if(inf == 0) {
		printf("PIN_SaveToFlash: sysinfo_get returned 0!\n\r");
		return;
	}
	os_memcpy(&inf->pins, &g_pins, sizeof(inf->pins));
	
	if(sizeof(inf->pins) != sizeof(g_pins)) {
		printf("PIN_SaveToFlash: FATAL ERROR - pin structures size mismatch! %i vs %i\r\n",sizeof(inf->pins),sizeof(g_pins));
	}

	res = sysinfo_save_wrapper();
	if(res != 0) {
		printf("PIN_SaveToFlash: sysinfo_save error - %i!\n\r",res);
	}

#else
	ITEM_PINS_CONFIG pins;
	os_memcpy(&pins.pins, &g_pins, sizeof(pins.pins));
	CONFIG_INIT_ITEM(CONFIG_TYPE_PINS, &pins);
	config_save_item(&pins);
	// delete old if it exists
	config_delete_item(NEW_PINS_CONFIG);
#endif
}
void PIN_LoadFromFlash() {
	int i;
#if WINDOWS

#elif PLATFORM_XR809
	sysinfo_t *inf;
#else
	int res;
	ITEM_PINS_CONFIG pins;
#endif


	PR_NOTICE("PIN_LoadFromFlash called - going to load pins.\r\n");
	PR_NOTICE("UART log breaking after that means that you changed the role of TX pin to digital IO or smth else.\r\n");

#if WINDOWS

#elif PLATFORM_XR809
	inf = sysinfo_get();
	if(inf == 0) {
		printf("PIN_LoadFromFlash: sysinfo_get returned 0!\n\r");
		return;
	}
	os_memcpy(&g_pins, &inf->pins, sizeof(g_pins));
	if(sizeof(inf->pins) != sizeof(g_pins)) {
		printf("PIN_LoadFromFlash: FATAL ERROR - pin structures size mismatch! %i vs %i\r\n",sizeof(inf->pins),sizeof(g_pins));
	}
#else
	CONFIG_INIT_ITEM(CONFIG_TYPE_PINS, &pins);
	res = config_get_item(&pins);
	if (res){
			os_memcpy(&g_pins, &pins.pins, sizeof(g_pins));
	}
#endif
	for(i = 0; i < GPIO_MAX; i++) {
		PIN_SetPinRoleForPinIndex(i,g_pins.roles[i]);
	}
	PR_NOTICE("PIN_LoadFromFlash pins have been set up.\r\n");
}
void PIN_ClearPins() {
	memset(&g_pins,0,sizeof(g_pins));
}
int PIN_GetPinRoleForPinIndex(int index) {
	return g_pins.roles[index];
}
int PIN_GetPinChannelForPinIndex(int index) {
	return g_pins.channels[index];
}
void RAW_SetPinValue(int index, int iVal){
#if WINDOWS

#elif PLATFORM_XR809
	int xr_port; // eg GPIO_PORT_A
	int xr_pin; // eg. GPIO_PIN_20

	PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

	HAL_GPIO_WritePin(xr_port, xr_pin, iVal);
#else
    bk_gpio_output(index, iVal);
#endif
}
void button_generic_short_press(int index)
{
	CHANNEL_Toggle(g_pins.channels[index]);

	PR_NOTICE("%i key_short_press\r\n", index);
}
void button_generic_double_press(int index)
{
	CHANNEL_Toggle(g_pins.channels[index]);

	if(g_doubleClickCallback!=0) {
		g_doubleClickCallback(index);
	}
	PR_NOTICE("%i key_double_press\r\n", index);
}
void button_generic_long_press_hold(int index)
{
	PR_NOTICE("%i key_long_press_hold\r\n", index);
}

const char *PIN_GetPinNameAlias(int index) {
#if PLATFORM_XR809
	if(index < 0 || index >= g_numXRPins) {
		return "bad_index";
	}
	return g_xrPins[index].name;
#else
	return "not_implemented_here";
#endif
}
unsigned char button_generic_get_gpio_value(void *param)
{
#if WINDOWS
	return 0;
#elif PLATFORM_XR809
	return 0;
#else
	int index = ((BUTTON_S*)param) - g_buttons;
	return bk_gpio_input(index);
#endif
}
#define PIN_UART1_RXD 10
#define PIN_UART1_TXD 11
#define PIN_UART2_RXD 1
#define PIN_UART2_TXD 0

void PIN_SetPinRoleForPinIndex(int index, int role) {
	//if(index == PIN_UART1_RXD)
	//	return;
	//if(index == PIN_UART1_TXD)
	//	return;
	//if(index == PIN_UART2_RXD)
	//	return;
	//if(index == PIN_UART2_TXD)
	//	return;
	switch(g_pins.roles[index])
	{
	case IOR_Button:
	case IOR_Button_n:
#if WINDOWS
	
#elif PLATFORM_XR809

#else
		{
			//BUTTON_S *bt = &g_buttons[index];
			// TODO: disable button
		}
#endif
		break;
	case IOR_LED:
	case IOR_LED_n:
	case IOR_Relay:
	case IOR_Relay_n:
#if WINDOWS
	
#elif PLATFORM_XR809

#else
		// TODO: disable?
#endif
		break;
		// Disable PWM for previous pin role
	case IOR_PWM:
		{
			int pwmIndex;
			//int channelIndex;

			pwmIndex = PIN_GetPWMIndexForPinIndex(index);
#if WINDOWS
	
#elif PLATFORM_XR809

#elif PLATFORM_BK7231N

#else
			bk_pwm_stop(pwmIndex);

#endif
		}
		break;

	default:
		break;
	}
	// set new role
	g_pins.roles[index] = role;
	// init new role
	switch(role)
	{
	case IOR_Button:
	case IOR_Button_n:
#if WINDOWS
	
#elif PLATFORM_XR809

#else
		{
			BUTTON_S *bt = &g_buttons[index];
			button_init(bt, button_generic_get_gpio_value, 0);
			bk_gpio_config_input_pup(index);
		/*	button_attach(bt, SINGLE_CLICK,     button_generic_short_press);
			button_attach(bt, DOUBLE_CLICK,     button_generic_double_press);
			button_attach(bt, LONG_PRESS_HOLD,  button_generic_long_press_hold);
			button_start(bt);*/
		}
#endif
		break;
	case IOR_LED:
	case IOR_LED_n:
	case IOR_Relay:
	case IOR_Relay_n:
	{
#if WINDOWS
	
#elif PLATFORM_XR809
		GPIO_InitParam param;
		int xr_port; // eg GPIO_PORT_A
		int xr_pin; // eg. GPIO_PIN_20

		PIN_XR809_GetPortPinForIndex(index, &xr_port, &xr_pin);

		/*set pin driver capability*/
		param.driving = GPIO_DRIVING_LEVEL_1;
		param.mode = GPIOx_Pn_F1_OUTPUT;
		param.pull = GPIO_PULL_NONE;
		HAL_GPIO_Init(xr_port, xr_pin, &param);
#else
		bk_gpio_config_output(index);
		bk_gpio_output(index, 0);
#endif
	}
		break;
	case IOR_PWM:
		{
			int pwmIndex;
			int channelIndex;
			float f;

			pwmIndex = PIN_GetPWMIndexForPinIndex(index);
			channelIndex = PIN_GetPinChannelForPinIndex(index);
#if WINDOWS
	
#elif PLATFORM_XR809

#elif PLATFORM_BK7231N

#else
			bk_pwm_start(pwmIndex);
			// they are using 1kHz PWM
			// See: https://www.elektroda.pl/rtvforum/topic3798114.html
		//	bk_pwm_update_param(pwmIndex, 1000, g_channelValues[channelIndex]);
			f = g_channelValues[channelIndex] * 0.01f;
			bk_pwm_update_param(pwmIndex, 1000, f * 1000.0f);

#endif
		}
		break;

	default:
		break;
	}
}
void PIN_SetPinChannelForPinIndex(int index, int ch) {
	g_pins.channels[index] = ch;
}
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex)) {
	g_doubleClickCallback = cb;
}
void CHANNEL_SetChangeCallback(void (*cb)(int idx, int iVal)) {
	g_channelChangeCallback = cb;
}
void Channel_OnChanged(int ch) {
	int i;
	int iVal;
	int bOn;


	//bOn = BIT_CHECK(g_channelStates,ch);
	iVal = g_channelValues[ch];
	bOn = iVal > 0;

	for(i = 0; i < GPIO_MAX; i++) {
		if(g_pins.channels[i] == ch) {
			if(g_pins.roles[i] == IOR_Relay || g_pins.roles[i] == IOR_LED) {
				RAW_SetPinValue(i,bOn);
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,bOn);
				}
			}
			if(g_pins.roles[i] == IOR_Relay_n || g_pins.roles[i] == IOR_LED_n) {
				RAW_SetPinValue(i,!bOn);
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,bOn);
				}
			}
			if(g_pins.roles[i] == IOR_PWM) {
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,iVal);
				}
				int pwmIndex = PIN_GetPWMIndexForPinIndex(i);

#if WINDOWS
	
#elif PLATFORM_XR809

#elif PLATFORM_BK7231N

#else
				// they are using 1kHz PWM
				// See: https://www.elektroda.pl/rtvforum/topic3798114.html
				bk_pwm_update_param(pwmIndex, 1000, iVal * 10.0f); // Duty cycle 0...100 * 10.0 = 0...1000
#endif
			}
			
		}
	}

}
int CHANNEL_Get(int ch) {
	return g_channelValues[ch];
}
void CHANNEL_Set(int ch, int iVal, int bForce) {
	if(bForce == 0) {
		int prevVal;

		//prevVal = BIT_CHECK(g_channelStates, ch);
		prevVal = g_channelValues[ch];
		if(prevVal == iVal) {
			PR_NOTICE("No change in channel %i - ignoring\n\r",ch);
			return;
		}
	}
	PR_NOTICE("CHANNEL_Set channel %i has changed to %i\n\r",ch,iVal);
	//if(iVal) {
	//	BIT_SET(g_channelStates,ch);
	//} else {
	//	BIT_CLEAR(g_channelStates,ch);
	//}
	g_channelValues[ch] = iVal;

	Channel_OnChanged(ch);
}
void CHANNEL_Toggle(int ch) {
	//BIT_TGL(g_channelStates,ch);
	if(g_channelValues[ch] == 0)
		g_channelValues[ch] = 100;
	else
		g_channelValues[ch] = 0;

	Channel_OnChanged(ch);
}
bool CHANNEL_Check(int ch) {
	//return BIT_CHECK(g_channelStates,ch);
	if (g_channelValues[ch] > 0)
		return 1;
	return 0;
}

#if WINDOWS

#elif PLATFORM_XR809

#else

#define EVENT_CB(ev)   if(handle->cb[ev])handle->cb[ev]((BUTTON_S*)handle)

#define PIN_TMR_DURATION       5
beken_timer_t g_pin_timer;

void PIN_Input_Handler(int pinIndex)
{
	BUTTON_S *handle;
	uint8_t read_gpio_level;
	
	read_gpio_level = bk_gpio_input(pinIndex);
	handle = &g_buttons[pinIndex];

	//ticks counter working..
	if((handle->state) > 0)
		handle->ticks++;

	/*------------button debounce handle---------------*/
	if(read_gpio_level != handle->button_level) { //not equal to prev one
		//continue read 3 times same new level change
		if(++(handle->debounce_cnt) >= DEBOUNCE_TICKS) {
			handle->button_level = read_gpio_level;
			handle->debounce_cnt = 0;
		}
	} else { //leved not change ,counter reset.
		handle->debounce_cnt = 0;
	}

	/*-----------------State machine-------------------*/
	switch (handle->state) {
	case 0:
		if(handle->button_level == handle->active_level) {	//start press down
			handle->event = (uint8_t)PRESS_DOWN;
			EVENT_CB(PRESS_DOWN);
			handle->ticks = 0;
			handle->repeat = 1;
			handle->state = 1;
		} else {
			handle->event = (uint8_t)NONE_PRESS;
		}
		break;

	case 1:
		if(handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)PRESS_UP;
			EVENT_CB(PRESS_UP);
			handle->ticks = 0;
			handle->state = 2;

		} else if(handle->ticks > LONG_TICKS) {
			handle->event = (uint8_t)LONG_RRESS_START;
			EVENT_CB(LONG_RRESS_START);
			handle->state = 5;
		}
		break;

	case 2:
		if(handle->button_level == handle->active_level) { //press down again
			handle->event = (uint8_t)PRESS_DOWN;
			EVENT_CB(PRESS_DOWN);
			handle->repeat++;
			if(handle->repeat == 2) {
				EVENT_CB(DOUBLE_CLICK); // repeat hit
				button_generic_double_press(pinIndex);
			} 
			EVENT_CB(PRESS_REPEAT); // repeat hit
			handle->ticks = 0;
			handle->state = 3;
		} else if(handle->ticks > SHORT_TICKS) { //released timeout
			if(handle->repeat == 1) {
				handle->event = (uint8_t)SINGLE_CLICK;
				EVENT_CB(SINGLE_CLICK);
				button_generic_short_press(pinIndex);
			} else if(handle->repeat == 2) {
				handle->event = (uint8_t)DOUBLE_CLICK;
			}
			handle->state = 0;
		}
		break;

	case 3:
		if(handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)PRESS_UP;
			EVENT_CB(PRESS_UP);
			if(handle->ticks < SHORT_TICKS) {
				handle->ticks = 0;
				handle->state = 2; //repeat press
			} else {
				handle->state = 0;
			}
		}
		break;

	case 5:
		if(handle->button_level == handle->active_level) {
			//continue hold trigger
			handle->event = (uint8_t)LONG_PRESS_HOLD;
			EVENT_CB(LONG_PRESS_HOLD);

		} else { //releasd
			handle->event = (uint8_t)PRESS_UP;
			EVENT_CB(PRESS_UP);
			handle->state = 0; //reset
		}
		break;
	}
}


/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void PIN_ticks(void *param)
{
	int i;
	for(i = 0; i < GPIO_MAX; i++) {
		if(g_pins.roles[i] == IOR_Button || g_pins.roles[i] == IOR_Button_n) {
			//PR_NOTICE("Test hold %i\r\n",i);
			PIN_Input_Handler(i);
		}
	}
}

void PIN_Init(void)
{
	OSStatus result;
	
    result = rtos_init_timer(&g_pin_timer,
                            PIN_TMR_DURATION,
                            PIN_ticks,
                            (void *)0);
    ASSERT(kNoErr == result);
	
    result = rtos_start_timer(&g_pin_timer);
    ASSERT(kNoErr == result);
}

void PIN_set_wifi_led(int value){
	int res = -1;
	for (int i = 0; i < 32; i++){
		if ((g_pins.roles[i] == IOR_LED_WIFI) || (g_pins.roles[i] == IOR_LED_WIFI_n)){
			res = i;
			break;
		}
	}
	if (res >= 0){
		if (g_pins.roles[res] == IOR_LED_WIFI_n){
			value = !value;
		}
		RAW_SetPinValue(res, value & 1);
	}
}

#endif