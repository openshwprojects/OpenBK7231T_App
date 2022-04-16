


#include "new_common.h"
#include "new_pins.h"
#include "httpserver/new_http.h"
#include "logging/logging.h"
// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"
#include "i2c/drv_i2c_public.h"
#include "hal/hal_pins.h"


//According to your need to modify the constants.
#define BTN_TICKS_INTERVAL    5	//ms
#define BTN_DEBOUNCE_TICKS    3	//MAX 8
#define BTN_SHORT_TICKS       (300 /BTN_TICKS_INTERVAL)
#define BTN_LONG_TICKS        (1000 /BTN_TICKS_INTERVAL)

typedef enum {
	BTN_PRESS_DOWN = 0,
	BTN_PRESS_UP,
	BTN_PRESS_REPEAT,
	BTN_SINGLE_CLICK,
	BTN_DOUBLE_CLICK,
	BTN_LONG_RRESS_START,
	BTN_LONG_PRESS_HOLD,
	BTN_number_of_event,
	BTN_NONE_PRESS
}BTN_PRESS_EVT;


typedef void (*new_btn_callback)(void*);

typedef struct pinButton_ {
	uint16_t ticks;
	uint8_t  repeat : 4;
	uint8_t  event : 4;
	uint8_t  state : 3;
	uint8_t  debounce_cnt : 3; 
	uint8_t  active_level : 1;
	uint8_t  button_level : 1;
	
	uint8_t  (*hal_button_Level)(void *self);
	new_btn_callback  cb[BTN_number_of_event];
}pinButton_s;

// overall pins enable.
// if zero, all hardware action is disabled.
char g_enable_pins = 0;

// it was nice to have it as bits but now that we support PWM...
//int g_channelStates;
int g_channelValues[GPIO_MAX] = { 0 };
// channel content types, mostly not required to set
int g_channelTypes[GPIO_MAX] = { 0 };

pinButton_s g_buttons[GPIO_MAX];

void (*g_channelChangeCallback)(int idx, int iVal) = 0;
void (*g_doubleClickCallback)(int pinIndex) = 0;




void PIN_SetupPins() {
	int i;
	for(i = 0; i < GPIO_MAX; i++) {
		PIN_SetPinRoleForPinIndex(i,g_cfg.pins.roles[i]);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"PIN_SetupPins pins have been set up.\r\n");
}

int PIN_GetPinRoleForPinIndex(int index) {
	return g_cfg.pins.roles[index];
}
int PIN_GetPinChannelForPinIndex(int index) {
	return g_cfg.pins.channels[index];
}
int PIN_GetPinChannel2ForPinIndex(int index) {
	return g_cfg.pins.channels2[index];
}
void RAW_SetPinValue(int index, int iVal){
	if (g_enable_pins) {
		HAL_PIN_SetOutputValue(index, iVal);
	}
}
void Button_OnShortClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"%i key_short_press\r\n", index);
	if(g_cfg.pins.roles[index] == IOR_Button_ToggleAll || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n)
	{
		CHANNEL_DoSpecialToggleAll();
		return;
	}
	// first click toggles FIRST CHANNEL linked to this button
	CHANNEL_Toggle(g_cfg.pins.channels[index]);
}
void Button_OnDoubleClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"%i key_double_press\r\n", index);
	if(g_cfg.pins.roles[index] == IOR_Button_ToggleAll || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n)
	{
		CHANNEL_DoSpecialToggleAll();
		return;
	}
	// double click toggles SECOND CHANNEL linked to this button
	CHANNEL_Toggle(g_cfg.pins.channels2[index]);

	if(g_doubleClickCallback!=0) {
		g_doubleClickCallback(index);
	}
}
void Button_OnLongPressHold(int index) {
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"%i key_long_press_hold\r\n", index);
}

bool BTN_ShouldInvert(int index) {
	if(g_cfg.pins.roles[index] == IOR_Button_n || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n|| g_cfg.pins.roles[index] == IOR_DigitalInput_n) {
		return true;
	}
	return false;
}
static uint8_t PIN_ReadDigitalInputValue_WithInversionIncluded(int index) {
	uint8_t iVal = HAL_PIN_ReadDigitalInput(index);

	// support inverted button
	if(BTN_ShouldInvert(index)) {
		return !iVal;
	}
	return iVal;
}
static uint8_t button_generic_get_gpio_value(void *param)
{
	int index;
	index = ((pinButton_s*)param) - g_buttons;

	return PIN_ReadDigitalInputValue_WithInversionIncluded(index);
}
#define PIN_UART1_RXD 10
#define PIN_UART1_TXD 11
#define PIN_UART2_RXD 1
#define PIN_UART2_TXD 0

void NEW_button_init(pinButton_s* handle, uint8_t(*pin_level)(void *self), uint8_t active_level)
{
	memset(handle, sizeof(pinButton_s), 0);
	
	handle->event = (uint8_t)BTN_NONE_PRESS;
	handle->hal_button_Level = pin_level;
	handle->button_level = handle->hal_button_Level(handle);
	handle->active_level = active_level;
}
void CHANNEL_SetType(int ch, int type) {

	g_channelTypes[ch] = type;
}
int CHANNEL_GetType(int ch) {
	return g_channelTypes[ch];
}
bool CHANNEL_IsInUse(int ch) {
	int i;
	for(i = 0; i < GPIO_MAX; i++){
		if(g_cfg.pins.channels[i] == ch) {
			switch(g_cfg.pins.roles[i])
			{
			case IOR_LED:
			case IOR_LED_n:
			case IOR_Relay:
			case IOR_Relay_n:
			case IOR_PWM:
				return true;
				break;

			default:
				break;
			}
		}
	}
	return false;
}
void CHANNEL_SetAll(int iVal, bool bForce) {
	int i;


	for(i = 0; i < GPIO_MAX; i++) {	
		switch(g_cfg.pins.roles[i])
		{
		case IOR_Button:
		case IOR_Button_n:
		case IOR_Button_ToggleAll:
		case IOR_Button_ToggleAll_n:
			{

			}
			break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_Relay:
		case IOR_Relay_n:
			CHANNEL_Set(g_cfg.pins.channels[i],iVal,bForce);
			break;
		case IOR_PWM:
			CHANNEL_Set(g_cfg.pins.channels[i],iVal,bForce);
			break;

		default:
			break;
		}
	}
}
void CHANNEL_SetStateOnly(int iVal) {
	int i;

	if(iVal == 0) {
		CHANNEL_SetAll(0, true);
		return;
	}

	// is it already on on some channel?
	for(i = 0; i < GPIO_MAX; i++) {
		if(CHANNEL_IsInUse(i)) {
			if(CHANNEL_Get(i) > 0) {
				return;
			}
		}
	}
	CHANNEL_SetAll(iVal, true);


}
void CHANNEL_DoSpecialToggleAll() {
	int anyEnabled, i;

	anyEnabled = 0;

	for(i = 0; i < CHANNEL_MAX; i++) {	
		if(CHANNEL_IsInUse(i)==false)
			continue;
		if(g_channelValues[i] > 0) {
			anyEnabled++;
		}	
	}
	if(anyEnabled)
		CHANNEL_SetAll(0, true);
	else
		CHANNEL_SetAll(255, true);

}
void PIN_SetPinRoleForPinIndex(int index, int role) {
#if 0
	if(index == PIN_UART1_RXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if(index == PIN_UART1_TXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if(index == PIN_UART2_RXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if(index == PIN_UART2_TXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
#endif
	if (g_enable_pins) {
		switch(g_cfg.pins.roles[index])
		{
		case IOR_Button:
		case IOR_Button_n:
		case IOR_Button_ToggleAll:
		case IOR_Button_ToggleAll_n:
			{
				//pinButton_s *bt = &g_buttons[index];
				// TODO: disable button
			}
			break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_Relay:
		case IOR_Relay_n:
			// TODO: disable?
			break;
			// Disable PWM for previous pin role
		case IOR_PWM:
			{
				HAL_PIN_PWM_Stop(index);
			}
			break;

		default:
			break;
		}
	}
	// set new role
	g_cfg.pins.roles[index] = role;

	if (g_enable_pins) {
		// init new role
		switch(role)
		{
		case IOR_Button:
		case IOR_Button_n:
		case IOR_Button_ToggleAll:
		case IOR_Button_ToggleAll_n:
			{
				pinButton_s *bt = &g_buttons[index];

				// digital input
				HAL_PIN_Setup_Input_Pullup(index);

				// init button after initializing pin role
				NEW_button_init(bt, button_generic_get_gpio_value, 0);
			}
			break;
		case IOR_DigitalInput:
		case IOR_DigitalInput_n:
			{
				// digital input
				HAL_PIN_Setup_Input_Pullup(index);
			}
			break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_Relay:
		case IOR_Relay_n:
		{
			HAL_PIN_Setup_Output(index);
		}
			break;
		case IOR_PWM:
			{
				int channelIndex;
				int channelValue;

				channelIndex = PIN_GetPinChannelForPinIndex(index);
				channelValue = g_channelValues[channelIndex];
				HAL_PIN_PWM_Start(index);
				HAL_PIN_PWM_Update(index,channelValue);
			}
			break;

		default:
			break;
		}
	}
}

void PIN_SetPinChannelForPinIndex(int index, int ch) {
	g_cfg.pins.channels[index] = ch;
}
void PIN_SetPinChannel2ForPinIndex(int index, int ch) {
	g_cfg.pins.channels2[index] = ch;
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

#ifndef OBK_DISABLE_ALL_DRIVERS
	I2C_OnChannelChanged(ch,iVal);
#endif

	for(i = 0; i < GPIO_MAX; i++) {
		if(g_cfg.pins.channels[i] == ch) {
			if(g_cfg.pins.roles[i] == IOR_Relay || g_cfg.pins.roles[i] == IOR_LED) {
				RAW_SetPinValue(i,bOn);
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,bOn);
				}
			}
			if(g_cfg.pins.roles[i] == IOR_Relay_n || g_cfg.pins.roles[i] == IOR_LED_n) {
				RAW_SetPinValue(i,!bOn);
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,bOn);
				}
			}
			if(g_cfg.pins.roles[i] == IOR_DigitalInput) {
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,iVal);
				}
			}
			if(g_cfg.pins.roles[i] == IOR_PWM) {
				if(g_channelChangeCallback != 0) {
					g_channelChangeCallback(ch,iVal);
				}
				HAL_PIN_PWM_Update(i,iVal);
			}
		}
	}

}
int CHANNEL_Get(int ch) {
	if(ch < 0 || ch >= GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,"CHANNEL_Get: Channel index %i is out of range <0,%i)\n\r",ch,GPIO_MAX);
		return 0;
	}
	return g_channelValues[ch];
}

void CHANNEL_Set(int ch, int iVal, int bForce) {
	if(ch < 0 || ch >= GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,"CHANNEL_Set: Channel index %i is out of range <0,%i)\n\r",ch,GPIO_MAX);
		return;
	}
	if(bForce == 0) {
		int prevVal;

		prevVal = g_channelValues[ch];
		if(prevVal == iVal) {
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"No change in channel %i - ignoring\n\r",ch);
			return;
		}
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"CHANNEL_Set channel %i has changed to %i\n\r",ch,iVal);
	g_channelValues[ch] = iVal;

	Channel_OnChanged(ch);
}
void CHANNEL_Toggle(int ch) {
	if(ch < 0 || ch >= GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,"CHANNEL_Toggle: Channel index %i is out of range <0,%i)\n\r",ch,GPIO_MAX);
		return;
	}
	if(g_channelValues[ch] == 0)
		g_channelValues[ch] = 100;
	else
		g_channelValues[ch] = 0;

	Channel_OnChanged(ch);
}
bool CHANNEL_Check(int ch) {
	if(ch < 0 || ch >= GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,"CHANNEL_Check: Channel index %i is out of range <0,%i)\n\r",ch,GPIO_MAX);
		return 0;
	}
	if (g_channelValues[ch] > 0)
		return 1;
	return 0;
}


int CHANNEL_GetRoleForOutputChannel(int ch){
	int i;
	for (i = 0; i < 32; i++){
		if (g_cfg.pins.channels[i] == ch){
			switch(g_cfg.pins.roles[i]){
				case IOR_Relay:
				case IOR_Relay_n:
				case IOR_LED:
				case IOR_LED_n:
				case IOR_PWM:
					return g_cfg.pins.roles[i];
					break;
				case IOR_Button:
				case IOR_Button_n:
				case IOR_LED_WIFI:
				case IOR_LED_WIFI_n:
					break;
			}
		}
	}
	return IOR_None;
}


#define EVENT_CB(ev)   if(handle->cb[ev])handle->cb[ev]((pinButton_s*)handle)

#define PIN_TMR_DURATION       5

void PIN_Input_Handler(int pinIndex)
{
	pinButton_s *handle;
	uint8_t read_gpio_level;
	
	handle = &g_buttons[pinIndex];
	read_gpio_level = button_generic_get_gpio_value(handle);

	//ticks counter working..
	if((handle->state) > 0)
		handle->ticks++;

	/*------------button debounce handle---------------*/
	if(read_gpio_level != handle->button_level) { //not equal to prev one
		//continue read 3 times same new level change
		if(++(handle->debounce_cnt) >= BTN_DEBOUNCE_TICKS) {
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
			handle->event = (uint8_t)BTN_PRESS_DOWN;
			EVENT_CB(BTN_PRESS_DOWN);
			handle->ticks = 0;
			handle->repeat = 1;
			handle->state = 1;
		} else {
			handle->event = (uint8_t)BTN_NONE_PRESS;
		}
		break;

	case 1:
		if(handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			handle->ticks = 0;
			handle->state = 2;

		} else if(handle->ticks > BTN_LONG_TICKS) {
			handle->event = (uint8_t)BTN_LONG_RRESS_START;
			EVENT_CB(BTN_LONG_RRESS_START);
			handle->state = 5;
		}
		break;

	case 2:
		if(handle->button_level == handle->active_level) { //press down again
			handle->event = (uint8_t)BTN_PRESS_DOWN;
			EVENT_CB(BTN_PRESS_DOWN);
			handle->repeat++;
			if(handle->repeat == 2) {
				EVENT_CB(BTN_DOUBLE_CLICK); // repeat hit
				Button_OnDoubleClick(pinIndex);
			} 
			EVENT_CB(BTN_PRESS_REPEAT); // repeat hit
			handle->ticks = 0;
			handle->state = 3;
		} else if(handle->ticks > BTN_SHORT_TICKS) { //released timeout
			if(handle->repeat == 1) {
				handle->event = (uint8_t)BTN_SINGLE_CLICK;
				EVENT_CB(BTN_SINGLE_CLICK);
				Button_OnShortClick(pinIndex);
			} else if(handle->repeat == 2) {
				handle->event = (uint8_t)BTN_DOUBLE_CLICK;
			}
			handle->state = 0;
		}
		break;

	case 3:
		if(handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			if(handle->ticks < BTN_SHORT_TICKS) {
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
			handle->event = (uint8_t)BTN_LONG_PRESS_HOLD;
			EVENT_CB(BTN_LONG_PRESS_HOLD);

		} else { //releasd
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			handle->state = 0; //reset
		}
		break;
	}
}


//  background ticks, timer repeat invoking interval 5ms.
void PIN_ticks(void *param)
{
	int i;
	int value;

	for(i = 0; i < GPIO_MAX; i++) {
		if(g_cfg.pins.roles[i] == IOR_Button || g_cfg.pins.roles[i] == IOR_Button_n
			|| g_cfg.pins.roles[i] == IOR_Button_ToggleAll || g_cfg.pins.roles[i] == IOR_Button_ToggleAll_n) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"Test hold %i\r\n",i);
			PIN_Input_Handler(i);
		}		
		else if(g_cfg.pins.roles[i] == IOR_DigitalInput || g_cfg.pins.roles[i] == IOR_DigitalInput_n) {
			// read pin digital value (and already invert it if needed)
			value = PIN_ReadDigitalInputValue_WithInversionIncluded(i);
			CHANNEL_Set(g_cfg.pins.channels[i], value,0);
		}
	}
}
int CHANNEL_ParseChannelType(const char *s) {
	if(!stricmp(s,"temperature"))
		return ChType_Temperature;
	if(!stricmp(s,"humidity") )
		return ChType_Humidity;
	if(!stricmp(s,"humidity_div10") )
		return ChType_Humidity_div10;
	if(!stricmp(s,"temperature_div10"))
		return ChType_Temperature_div10;
	if(!stricmp(s,"toggle"))
		return ChType_Toggle;
	if(!stricmp(s,"default") )
		return ChType_Default;
	return ChType_Error;
}
static int CMD_SetChannelType(const void *context, const char *cmd, const char *args){
	int channel;
	const char *type;
	int typeCode;

	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"This command requires 2 arguments");
		return 1;
	}
	channel = Tokenizer_GetArgInteger(0);
	type = Tokenizer_GetArg(1);

	typeCode = CHANNEL_ParseChannelType(type);
	if(typeCode == ChType_Error) {

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"Channel %i type not set because %s is not a known type", channel,type);
		return 1;
	}

	CHANNEL_SetType(channel,typeCode);

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"Channel %i type changed to %s", channel,type);
	return 0;
}

static int showgpi(const void *context, const char *cmd, const char *args){
	int i;
	unsigned int value = 0;

	for (i = 0; i < 32; i++) {
		int val = 0;

		val = HAL_PIN_ReadDigitalInput(i);

		value |= ((val & 1)<<i);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"GPIs are 0x%x", value);
	return 1;
}

#if WINDOWS

#elif PLATFORM_BL602

#elif PLATFORM_XR809
OS_Timer_t timer;
#else
beken_timer_t g_pin_timer;
#endif
void PIN_Init(void)
{
#if WINDOWS

#elif PLATFORM_BL602

#elif PLATFORM_XR809

	OS_TimerSetInvalid(&timer);
	if (OS_TimerCreate(&timer, OS_TIMER_PERIODIC, PIN_ticks, NULL,
	                   PIN_TMR_DURATION) != OS_OK) {
		printf("PIN_Init timer create failed\n");
		return;
	}

	OS_TimerStart(&timer); /* start OS timer to feed watchdog */
#else
	OSStatus result;
	
    result = rtos_init_timer(&g_pin_timer,
                            PIN_TMR_DURATION,
                            PIN_ticks,
                            (void *)0);
    ASSERT(kNoErr == result);
	
    result = rtos_start_timer(&g_pin_timer);
    ASSERT(kNoErr == result);
#endif


	CMD_RegisterCommand("showgpi", NULL, showgpi, "log stat of all GPIs", NULL);
	CMD_RegisterCommand("setChannelType", NULL, CMD_SetChannelType, "qqqqqqqq", NULL);
}
void PIN_set_wifi_led(int value){
	int res = -1;
	int i;
	for ( i = 0; i < 32; i++){
		if ((g_cfg.pins.roles[i] == IOR_LED_WIFI) || (g_cfg.pins.roles[i] == IOR_LED_WIFI_n)){
			res = i;
			break;
		}
	}
	if (res >= 0){
		if (g_cfg.pins.roles[res] == IOR_LED_WIFI_n){
			value = !value;
		}
		RAW_SetPinValue(res, value & 1);
	}
}



