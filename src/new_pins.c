


#include "new_common.h"
#include "new_pins.h"
#include "quicktick.h"
#include "new_cfg.h"
#include "httpserver/new_http.h"
#include "logging/logging.h"
#include "mqtt/new_mqtt.h"
// Commands register, execution API and cmd tokenizer
#include "cmnds/cmd_public.h"
#include "i2c/drv_i2c_public.h"
#include "driver/drv_tuyaMCU.h"
#include "driver/drv_public.h"
#include "hal/hal_flashVars.h"
#include "hal/hal_pins.h"
#include "hal/hal_adc.h"

#ifdef PLATFORM_BEKEN
#include <gpio_pub.h>
#include "driver/drv_ir.h"
#elif PLATFORM_ESPIDF
#include "esp_sleep.h"
#endif


// According to your need to modify the constants.
#define PIN_TMR_DURATION      QUICK_TMR_DURATION // Delay (in ms) between button scan iterations
#define BTN_DEBOUNCE_MS         75  
// NOTE: Original settings was PIN_TIMER_DURATION 5 and BTN_DEBOUNCE_MS 15
// Now that QUICK_TIMER_DURATION is 25, we can increase debounce to 75 or so...

// loaded from config, they are now configurable
int BTN_SHORT_MS;
int BTN_LONG_MS;
int BTN_HOLD_REPEAT_MS;
byte *g_defaultWakeEdge = 0;
int g_initialPinStates = 0;

void PIN_DeepSleep_MakeSureEdgesAreAlloced() {
	int i;
	if (g_defaultWakeEdge == 0) {
		g_defaultWakeEdge = (byte*)malloc(PLATFORM_GPIO_MAX);
		for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
			g_defaultWakeEdge[i] = 2;//default
		}
	}
}
void PIN_DeepSleep_SetWakeUpEdge(int pin, byte edgeCode) {
	PIN_DeepSleep_MakeSureEdgesAreAlloced();
	g_defaultWakeEdge[pin] = edgeCode;
}
void PIN_DeepSleep_SetAllWakeUpEdges(byte edgeCode) {
	int i;
	PIN_DeepSleep_MakeSureEdgesAreAlloced();
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		g_defaultWakeEdge[i] = edgeCode;
	}
}
typedef enum {
	BTN_PRESS_DOWN = 0,
	BTN_PRESS_UP,
	BTN_PRESS_REPEAT,
	BTN_SINGLE_CLICK,
	BTN_DOUBLE_CLICK,
	BTN_LONG_RRESS_START,
	BTN_LONG_PRESS_HOLD,
	BTN_TRIPLE_CLICK,
	BTN_QUADRUPLE_CLICK,
	BTN_5X_CLICK,
	BTN_number_of_event,
	BTN_NONE_PRESS
}BTN_PRESS_EVT;

typedef struct pinButton_ {
	uint16_t ticks;
	uint16_t holdRepeatTicks;
	uint8_t  repeat : 4;
	uint8_t  event : 4;
	uint8_t  state : 3;
	uint8_t  active_level : 1;
	uint8_t  button_level : 1;

	uint8_t  debounce_cnt; // make a full byte, so we can count ms

	uint8_t(*hal_button_Level)(void* self);
}pinButton_s;

// overall pins enable.
// if zero, all hardware action is disabled.
char g_enable_pins = 0;

// it was nice to have it as bits but now that we support PWM...
//int g_channelStates;
int g_channelValues[CHANNEL_MAX] = { 0 };
float g_channelValuesFloats[CHANNEL_MAX] = { 0 };

pinButton_s g_buttons[PLATFORM_GPIO_MAX];

void (*g_doubleClickCallback)(int pinIndex) = 0;

static short g_times[PLATFORM_GPIO_MAX];
static short g_times2[PLATFORM_GPIO_MAX];
static byte g_lastValidState[PLATFORM_GPIO_MAX];


// a bitfield indicating which GPI are inputs.
// could be used to control edge triggered interrupts...
/*  @param  gpio_index_map:The gpio bitmap which set 1 enable wakeup deep sleep.
 *              gpio_index_map is hex and every bits is map to gpio0-gpio31.
 *          gpio_edge_map:The gpio edge bitmap for wakeup gpios,
 *              gpio_edge_map is hex and every bits is map to gpio0-gpio31.
 *              0:rising,1:falling.
 */
 // these map directly to void bk_enter_deep_sleep(uint32_t gpio_index_map,uint32_t gpio_edge_map);
uint32_t g_gpio_index_map[2] = { 0, 0 };
uint32_t g_gpio_edge_map[2] = { 0, 0 }; // note: 0->rising, 1->falling


void setGPIActive(int index, int active, int falling) {
	if (active) {
		if (index >= 32)
			g_gpio_index_map[1] |= (1 << (index - 32));
		else
			g_gpio_index_map[0] |= (1 << index);
	}
	else {
		if (index >= 32)
			g_gpio_index_map[1] &= ~(1 << (index - 32));
		else
			g_gpio_index_map[0] &= ~(1 << index);
	}
	if (falling) {
		if (index >= 32)
			g_gpio_edge_map[1] |= (1 << (index - 32));
		else
			g_gpio_edge_map[0] |= (1 << index);
	}
	else {
		if (index >= 32)
			g_gpio_edge_map[1] &= ~(1 << (index - 32));
		else
			g_gpio_edge_map[0] &= ~(1 << index);
	}
}
void PINS_BeginDeepSleepWithPinWakeUp(unsigned int wakeUpTime) {
	int i;
	int value;
	int falling;

	// door input always uses opposite level for wakeup
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep
			|| g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_NoPup
			|| g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_pd
			|| g_cfg.pins.roles[i] == IOR_DigitalInput
			|| g_cfg.pins.roles[i] == IOR_DigitalInput_n
			|| g_cfg.pins.roles[i] == IOR_DigitalInput_NoPup
			|| g_cfg.pins.roles[i] == IOR_DigitalInput_NoPup_n) {
			//value = CHANNEL_Get(g_cfg.pins.channels[i]);

			// added per request
			// https://www.elektroda.pl/rtvforum/viewtopic.php?p=20543190#20543190
			// forcing a certain edge for both states helps on some door sensors, somehow
			// 0 means always wake up on rising edge, 1 means on falling, 2 means if state is high, use falling edge, if low, use rising
			if (g_defaultWakeEdge == NULL || g_defaultWakeEdge[i] == 2) {
				value = HAL_PIN_ReadDigitalInput(i);
				if (value) {
					// on falling edge wake up
					falling = 1;
				}
				else {
					// on rising edge wake up
					falling = 0;
				}
			}
			else {
				falling = g_defaultWakeEdge[i];
			}
			setGPIActive(i, 1, falling);
		}
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Index map: %i, edge: %i", g_gpio_index_map[0], g_gpio_edge_map[0]);
#ifdef PLATFORM_BEKEN
	// NOTE: this function:
	// void bk_enter_deep_sleep(UINT32 gpio_index_map,UINT32 gpio_edge_map)
	// On BK7231T, will overwrite HAL pin settings, and depending on edge map,
	// will set a internal pullup or internall pulldown
#ifdef PLATFORM_BK7231T
	extern void deep_sleep_wakeup_with_gpio(UINT32 gpio_index_map, UINT32 gpio_edge_map);
	deep_sleep_wakeup_with_gpio(g_gpio_index_map[0], g_gpio_edge_map[0]);
#else
	extern void bk_enter_deep_sleep(UINT32 g_gpio_index_map, UINT32 g_gpio_edge_map);
	extern void deep_sleep_wakeup(const UINT32* g_gpio_index_map,
		const UINT32* g_gpio_edge_map, const UINT32* sleep_time);
	if (wakeUpTime) {
		deep_sleep_wakeup(&g_gpio_index_map[0],
			&g_gpio_edge_map[0],
			&wakeUpTime);
	}
	else {
		bk_enter_deep_sleep(g_gpio_index_map[0], g_gpio_edge_map[0]);
	}
#endif
#elif PLATFORM_ESPIDF
//	if(wakeUpTime)
//	{
//		esp_sleep_enable_timer_wakeup(timeMS * 1000000);
//	}
//#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
//	esp_deep_sleep_start();
//#else
//	addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "%s, doesn't support gpio deep sleep, entering light sleep.", PLATFORM_MCU_NAME);
//	delay_ms(10);
//	esp_sleep_enable_gpio_wakeup();
//	esp_light_sleep_start();
//#endif
#endif
}


#ifdef PLATFORM_BEKEN
#ifdef BEKEN_PIN_GPI_INTERRUPTS
// TODO: EXAMPLE of edge based interrupt handling
// NOT YET ENABLED

// this will be from hal_bk....
// causes button read.
extern void BUTTON_TriggerRead();


// NOTE: ISR!!!!
// triggers one-shot timer to fire in 1ms
// from hal_main_bk7231.c
// THIS IS AN ISR.
void PIN_IntHandler(unsigned char index) {
	BUTTON_TriggerRead();
}

// this will be from hal_bk....
// ensures that we will get called in 50ms or less.
extern void BUTTON_TriggerRead_quick();

// called in PIN_ticks to carry on polling for 20 polls after last button is active
void PIN_TriggerPoll() {
	BUTTON_TriggerRead_quick();
}
#endif
#endif


void PIN_SetupPins() {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		PIN_SetPinRoleForPinIndex(i, g_cfg.pins.roles[i]);
	}

#ifdef PLATFORM_BEKEN
#ifdef BEKEN_PIN_GPI_INTERRUPTS
	// TODO: EXAMPLE of edge based interrupt handling
	// NOT YET ENABLED
	for (int i = 0; i < 32; i++) {
		if (g_gpio_index_map[0] & (1 << i)) {
			uint32_t mode = GPIO_INT_LEVEL_RISING;
			if (g_gpio_edge_map[0] & (1 << i)) {
				mode = GPIO_INT_LEVEL_FALLING;
			}
			if (g_cfg.pins.roles[i] == IOR_IRRecv) {
				// not yet implemented
				//gpio_int_enable(i, mode, IR_GPI_IntHandler);
			}
			else {
				gpio_int_enable(i, mode, PIN_IntHandler);
			}
		}
		else {
			gpio_int_disable(i);
		}
	}
#endif
#endif
#ifdef ENABLE_DRIVER_DHT
	// TODO: better place to call?
	DHT_OnPinsConfigChanged();
#endif
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "PIN_SetupPins pins have been set up.\r\n");
}

int PIN_GetPinRoleForPinIndex(int index) {
	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_GetPinRoleForPinIndex: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return 0;
	}
	return g_cfg.pins.roles[index];
}
int PIN_GetPinChannelForPinIndex(int index) {
	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_GetPinChannelForPinIndex: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return 0;
	}
	return g_cfg.pins.channels[index];
}
int PIN_CountPinsWithRoleOrRole(int role, int role2) {
	int i;
	int r = 0;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == role)
			r++;
		else if (g_cfg.pins.roles[i] == role2)
			r++;
	}
	return r;
}
int PIN_CountPinsWithRole(int role) {
	int i;
	int r = 0;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == role)
			r++;
	}
	return r;
}
int PIN_FindPinIndexForRole(int role, int defaultIndexToReturnIfNotFound) {
	int i;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == role)
			return i;
	}
	return defaultIndexToReturnIfNotFound;
}
int PIN_GetPinChannel2ForPinIndex(int index) {
	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_GetPinChannel2ForPinIndex: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return 0;
	}
	return g_cfg.pins.channels2[index];
}
// return number of channels used for a role
// taken from code in http_fnc.c
int PIN_IOR_NofChan(int test){
	// For button, is relay index to toggle on double click
	if (test == IOR_Button || test == IOR_Button_n || IS_PIN_DHT_ROLE(test) || IS_PIN_TEMP_HUM_SENSOR_ROLE(test) || IS_PIN_AIR_SENSOR_ROLE(test)){
			return 2;
	}
	// Some roles don't need any channels
	if (test == IOR_SGP_CLK || test == IOR_SHT3X_CLK || test == IOR_CHT83XX_CLK || test == IOR_Button_ToggleAll || test == IOR_Button_ToggleAll_n
			|| test == IOR_BL0937_CF || test == IOR_BL0937_CF1 || test == IOR_BL0937_SEL
			|| test == IOR_LED_WIFI || test == IOR_LED_WIFI_n || test == IOR_LED_WIFI_n
			|| (test >= IOR_IRRecv && test <= IOR_DHT11)
			|| (test >= IOR_SM2135_DAT && test <= IOR_BP1658CJ_CLK)) {
			return 0;
	}
	// all others have 1 channel
	return 1;
}

void RAW_SetPinValue(int index, int iVal) {
	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "RAW_SetPinValue: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return;
	}
	if (g_enable_pins) {
		HAL_PIN_SetOutputValue(index, iVal);
	}
}
void Button_OnPressRelease(int index) {
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was released
	EventHandlers_FireEvent(CMD_EVENT_PIN_ONRELEASE, index);
}
void Button_OnInitialPressDown(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i Button_OnInitialPressDown\r\n", index);

	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}

	EventHandlers_FireEvent(CMD_EVENT_PIN_ONPRESS, index);

	// so-called SetOption13 - instant reaction to touch instead of waiting for release
	if (CFG_HasFlag(OBK_FLAG_BTN_INSTANTTOUCH)) {
		if (g_cfg.pins.roles[index] == IOR_Button_ToggleAll || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n)
		{
			CHANNEL_DoSpecialToggleAll();
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextColor || g_cfg.pins.roles[index] == IOR_Button_NextColor_n)
		{
			LED_NextColor();
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextDimmer || g_cfg.pins.roles[index] == IOR_Button_NextDimmer_n)
		{

			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextTemperature || g_cfg.pins.roles[index] == IOR_Button_NextTemperature_n)
		{

			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_ScriptOnly || g_cfg.pins.roles[index] == IOR_Button_ScriptOnly_n)
		{

			return;
		}
		// is it a device with RGB/CW/single color/etc LED driver?
		if (LED_IsLEDRunning()) {
			LED_ToggleEnabled();
		}
		else {
			// Relays
			CHANNEL_Toggle(g_cfg.pins.channels[index]);
		}
	}
}
void Button_OnShortClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i key_short_press\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was clicked
	EventHandlers_FireEvent(CMD_EVENT_PIN_ONCLICK, index);
	// so-called SetOption13 - instant reaction to touch instead of waiting for release
	// first click toggles FIRST CHANNEL linked to this button
	if (CFG_HasFlag(OBK_FLAG_BTN_INSTANTTOUCH) == false) {
		if (g_cfg.pins.roles[index] == IOR_Button_ToggleAll || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n)
		{
			CHANNEL_DoSpecialToggleAll();
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextColor || g_cfg.pins.roles[index] == IOR_Button_NextColor_n)
		{
			LED_NextColor();
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextDimmer || g_cfg.pins.roles[index] == IOR_Button_NextDimmer_n)
		{
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_NextTemperature || g_cfg.pins.roles[index] == IOR_Button_NextTemperature_n)
		{
			return;
		}
		if (g_cfg.pins.roles[index] == IOR_Button_ScriptOnly || g_cfg.pins.roles[index] == IOR_Button_ScriptOnly_n)
		{
			return;
		}
		// is it a device with RGB/CW/single color/etc LED driver?
		if (LED_IsLEDRunning()) {
			LED_ToggleEnabled();
		}
		else {
			// Relays
			CHANNEL_Toggle(g_cfg.pins.channels[index]);
		}
	}
}
void Button_OnDoubleClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i key_double_press\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	if (g_cfg.pins.roles[index] == IOR_Button_ToggleAll || g_cfg.pins.roles[index] == IOR_Button_ToggleAll_n)
	{
		CHANNEL_DoSpecialToggleAll();
		return;
	}
	// fire event - button on pin <index> was dbclicked
	EventHandlers_FireEvent(CMD_EVENT_PIN_ONDBLCLICK, index);

	if (g_cfg.pins.roles[index] == IOR_Button || g_cfg.pins.roles[index] == IOR_Button_n)
	{
		// double click toggles SECOND CHANNEL linked to this button
		CHANNEL_Toggle(g_cfg.pins.channels2[index]);
	}
	if (g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs || g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs_n) {
		LED_NextColor();
		// make it easier for users, enable LED by default
		LED_SetEnableAll(true);
	}
	if (g_doubleClickCallback != 0) {
		g_doubleClickCallback(index);
	}
}
void Button_OnTripleClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i key_triple_press\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was 3clicked
	EventHandlers_FireEvent(CMD_EVENT_PIN_ON3CLICK, index);
	if (g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs || g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs_n) {
		LED_NextTemperature();
		// make it easier for users, enable LED by default
		LED_SetEnableAll(true);
	}
}
void Button_OnQuadrupleClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i key_quadruple_press\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was 4clicked
	EventHandlers_FireEvent(CMD_EVENT_PIN_ON4CLICK, index);
}
void Button_On5xClick(int index)
{
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i key_5x_press\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was 4clicked
	EventHandlers_FireEvent(CMD_EVENT_PIN_ON5CLICK, index);
}
void Button_OnLongPressHold(int index) {
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i Button_OnLongPressHold\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was held
	EventHandlers_FireEvent(CMD_EVENT_PIN_ONHOLD, index);

	if (g_cfg.pins.roles[index] == IOR_Button_NextDimmer || g_cfg.pins.roles[index] == IOR_Button_NextDimmer_n) {
		LED_NextDimmerHold();
	}
	if (g_cfg.pins.roles[index] == IOR_Button_NextTemperature || g_cfg.pins.roles[index] == IOR_Button_NextTemperature_n) {
		LED_NextTemperatureHold();
	}
	if (g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs || g_cfg.pins.roles[index] == IOR_SmartButtonForLEDs_n) {
		LED_NextDimmerHold();
		// make it easier for users, enable LED by default
		LED_SetEnableAll(true);
	}
}
void Button_OnLongPressHoldStart(int index) {
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "%i Button_OnLongPressHoldStart\r\n", index);
	if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
		return;
	}
	// fire event - button on pin <index> was held
	EventHandlers_FireEvent(CMD_EVENT_PIN_ONHOLDSTART, index);
}

bool BTN_ShouldInvert(int index) {
	int role;
	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "BTN_ShouldInvert: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return false;
	}
	role = g_cfg.pins.roles[index];
	if (role == IOR_Button_n || role == IOR_Button_ToggleAll_n ||
		role == IOR_DigitalInput_n || role == IOR_DigitalInput_NoPup_n
		|| role == IOR_Button_NextColor_n || role == IOR_Button_NextDimmer_n
		|| role == IOR_Button_NextTemperature_n || role == IOR_Button_ScriptOnly_n
		|| role == IOR_SmartButtonForLEDs_n) {
		return true;
	}
	if (CFG_HasFlag(OBK_FLAG_DOORSENSOR_INVERT_STATE)) {
		if (role == IOR_DoorSensorWithDeepSleep || role == IOR_DoorSensorWithDeepSleep_NoPup || role == IOR_DoorSensorWithDeepSleep_pd) {
			return true;
		}
	}
	return false;
}
static uint8_t PIN_ReadDigitalInputValue_WithInversionIncluded(int index) {
	uint8_t iVal = HAL_PIN_ReadDigitalInput(index);

	// support inverted button
	if (BTN_ShouldInvert(index)) {
		return (iVal == 0) ? 1 : 0;
	}
	return iVal;
}
static uint8_t button_generic_get_gpio_value(void* param)
{
	int index;
	index = ((pinButton_s*)param) - g_buttons;

	return PIN_ReadDigitalInputValue_WithInversionIncluded(index);
}
#define PIN_UART1_RXD 10
#define PIN_UART1_TXD 11
#define PIN_UART2_RXD 1
#define PIN_UART2_TXD 0

void NEW_button_init(pinButton_s* handle, uint8_t(*pin_level)(void* self), uint8_t active_level)
{
	memset(handle, 0, sizeof(pinButton_s));

	handle->event = (uint8_t)BTN_NONE_PRESS;
	handle->hal_button_Level = pin_level;
	handle->button_level = handle->hal_button_Level(handle);
	handle->active_level = active_level;
}
void CHANNEL_SetFirstChannelByType(int requiredType, int newVal) {
	int i;

	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_GetType(i) == requiredType) {
			CHANNEL_Set(i, newVal, 0);
			return;
		}
	}
}

void CHANNEL_SetAll(int iVal, int iFlags) {
	int i;


	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		switch (g_cfg.pins.roles[i])
		{
		case IOR_Button:
		case IOR_Button_n:
		case IOR_Button_ToggleAll:
		case IOR_Button_ToggleAll_n:
		case IOR_Button_NextColor:
		case IOR_Button_NextColor_n:
		case IOR_Button_NextDimmer:
		case IOR_Button_NextDimmer_n:
		case IOR_Button_NextTemperature:
		case IOR_Button_NextTemperature_n:
		case IOR_Button_ScriptOnly:
		case IOR_Button_ScriptOnly_n:
		case IOR_SmartButtonForLEDs:
		case IOR_SmartButtonForLEDs_n:
		{

		}
		break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_BAT_Relay:
		case IOR_BAT_Relay_n:
		case IOR_Relay:
		case IOR_Relay_n:
			CHANNEL_Set(g_cfg.pins.channels[i], iVal, iFlags);
			break;
		case IOR_PWM:
		case IOR_PWM_n:
			CHANNEL_Set(g_cfg.pins.channels[i], iVal, iFlags);
			break;
		case IOR_BridgeForward:
		case IOR_BridgeReverse:
			CHANNEL_Set(g_cfg.pins.channels[i], iVal, iFlags);
			break;

		default:
			break;
		}
	}
}
void CHANNEL_SetStateOnly(int iVal) {
	int i;

	if (iVal == 0) {
		CHANNEL_SetAll(0, true);
		return;
	}

	// is it already on on some channel?
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (CHANNEL_IsInUse(i)) {
			if (CHANNEL_Get(i) > 0) {
				return;
			}
		}
	}
	CHANNEL_SetAll(iVal, true);


}
void CHANNEL_DoSpecialToggleAll() {
	int anyEnabled, i;

	anyEnabled = 0;

	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_IsPowerRelayChannel(i) == false)
			continue;
		if (g_channelValues[i] > 0) {
			anyEnabled = true;
		}
	}
	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_IsPowerRelayChannel(i)) {
			CHANNEL_Set(i, !anyEnabled, 0);
		}
	}
}
void PIN_SetPinRoleForPinIndex(int index, int role) {
	bool bDHTChange = false;
	bool bSampleInitialState = false;

	if (index < 0 || index >= PLATFORM_GPIO_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_CFG, "PIN_SetPinRoleForPinIndex: Pin index %i out of range <0,%i).", index, PLATFORM_GPIO_MAX);
		return;
	}
#if 0
	if (index == PIN_UART1_RXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if (index == PIN_UART1_TXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if (index == PIN_UART2_RXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
	if (index == PIN_UART2_TXD) {
		// default role to None in order to fix broken config
		role = IOR_None;
	}
#endif
	if (g_enable_pins) {

		// remove from active inputs
		setGPIActive(index, 0, 0);

		switch (g_cfg.pins.roles[index])
		{
		case IOR_Button:
		case IOR_Button_n:
		case IOR_Button_ToggleAll:
		case IOR_Button_ToggleAll_n:
		case IOR_Button_NextColor:
		case IOR_Button_NextColor_n:
		case IOR_Button_NextDimmer:
		case IOR_Button_NextDimmer_n:
		case IOR_Button_NextTemperature:
		case IOR_Button_NextTemperature_n:
		case IOR_Button_ScriptOnly:
		case IOR_Button_ScriptOnly_n:
		case IOR_SmartButtonForLEDs:
		case IOR_SmartButtonForLEDs_n:
		{
			//pinButton_s *bt = &g_buttons[index];
			// TODO: disable button
		}
		break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_BAT_Relay:
		case IOR_BAT_Relay_n:
		case IOR_Relay:
		case IOR_Relay_n:
		case IOR_LED_WIFI:
		case IOR_LED_WIFI_n:
			// TODO: disable?
			break;
			// Disable PWM for previous pin role
		case IOR_PWM_n:
		case IOR_PWM:
		{
			HAL_PIN_PWM_Stop(index);
		}
		break;
		case IOR_BAT_ADC:
		case IOR_ADC:
			// TODO: disable?
			break;
		case IOR_BridgeForward:
		case IOR_BridgeReverse:
			break;

		default:
			break;
		}
	}
	// set new role
	if (g_cfg.pins.roles[index] != role) {
		if (g_enable_pins) {
			// if old role is DHT
			if (IS_PIN_DHT_ROLE(g_cfg.pins.roles[index])) {
				bDHTChange = true;
			}
			// or new role is DHT
			if (IS_PIN_DHT_ROLE(role)) {
				bDHTChange = true;
			}
		}
		g_cfg.pins.roles[index] = role;
		g_cfg_pendingChanges++;
	}

	if (g_enable_pins) {
		int falling = 0;

		// init new role
		switch (role)
		{
		case IOR_Button:
		case IOR_Button_ToggleAll:
		case IOR_Button_NextColor:
		case IOR_Button_NextDimmer:
		case IOR_Button_NextTemperature:
		case IOR_Button_ScriptOnly:
		case IOR_SmartButtonForLEDs:
			falling = 1;
		case IOR_Button_n:
		case IOR_Button_ToggleAll_n:
		case IOR_Button_NextColor_n:
		case IOR_Button_NextDimmer_n:
		case IOR_Button_NextTemperature_n:
		case IOR_Button_ScriptOnly_n:
		case IOR_SmartButtonForLEDs_n:
		{
			pinButton_s* bt = &g_buttons[index];

			// add to active inputs
			setGPIActive(index, 1, falling);

			// digital input
			HAL_PIN_Setup_Input_Pullup(index);

			// init button after initializing pin role
			NEW_button_init(bt, button_generic_get_gpio_value, 0);
			// this is input - sample initial state down below
			bSampleInitialState = true;
		}
		break;

		case IOR_IRRecv:
			falling = 1;
			// add to active inputs
			setGPIActive(index, 1, falling);
			break;

		case IOR_ToggleChannelOnToggle:
		{
			// add to active inputs
			falling = 1;
			setGPIActive(index, 1, falling);

			// digital input
			HAL_PIN_Setup_Input_Pullup(index);
			// otherwise we get a toggle on start			
#ifdef PLATFORM_BEKEN
			//20231217 XJIKKA
			//On the BK7231N Mini WiFi Smart Switch, the correct state of the ADC input pin
			//can be readed 1000us after the pin is initialized. Maybe there is a capacitor?
			//Without delay, g_lastValidState is after restart set to 0, so the light will toggle, if the switch on input pin is on (1).
			//To be sure, we will wait for 20000 us.
			usleep(20000);
#endif
			g_lastValidState[index] = PIN_ReadDigitalInputValue_WithInversionIncluded(index);
			// this is input - sample initial state down below
			bSampleInitialState = true;
		}
		break;
		case IOR_DigitalInput_n:
			falling = 1;
		case IOR_DoorSensorWithDeepSleep:
		case IOR_DigitalInput:
		{
			// add to active inputs
			setGPIActive(index, 1, falling);
			// digital input
			HAL_PIN_Setup_Input_Pullup(index);
			// this is input - sample initial state down below
			bSampleInitialState = true;
		}
		break;
		case IOR_DoorSensorWithDeepSleep_pd:
		{
			// add to active inputs
			setGPIActive(index, 1, falling);
			// digital input
			HAL_PIN_Setup_Input_Pulldown(index);
			// this is input - sample initial state down below
			bSampleInitialState = true;
		}
		break;
		case IOR_DigitalInput_NoPup_n:
			falling = 1;
		case IOR_DoorSensorWithDeepSleep_NoPup:
		case IOR_DigitalInput_NoPup:
		{
			// add to active inputs
			// TODO: We cannot set active here, as later code may enforce pullup/down????
			//setGPIActive(index, 1, falling);
			// digital input
			HAL_PIN_Setup_Input(index);
			// this is input - sample initial state down below
			bSampleInitialState = true;
		}
		break;
		case IOR_LED:
		case IOR_LED_n:
		case IOR_BAT_Relay:
		case IOR_BAT_Relay_n:
		case IOR_Relay:
		case IOR_Relay_n:
		{
			int channelIndex;
			int channelValue;

			channelIndex = PIN_GetPinChannelForPinIndex(index);
			channelValue = g_channelValues[channelIndex];

			HAL_PIN_Setup_Output(index);
			if (role == IOR_LED_n || role == IOR_Relay_n || role == IOR_BAT_Relay_n) {
				HAL_PIN_SetOutputValue(index, !channelValue);
			}
			else {
				HAL_PIN_SetOutputValue(index, channelValue);
			}
		}
		break;
		case IOR_BridgeForward:
		case IOR_BridgeReverse:
		{
			int channelIndex;
			int channelValue;

			channelIndex = PIN_GetPinChannelForPinIndex(index);
			channelValue = g_channelValues[channelIndex];

			HAL_PIN_Setup_Output(index);
			HAL_PIN_SetOutputValue(index, 0);
		}
		break;

		case IOR_AlwaysHigh:
		{
			HAL_PIN_Setup_Output(index);
			HAL_PIN_SetOutputValue(index, 1);
		}
		break;
		case IOR_AlwaysLow:
		{
			HAL_PIN_Setup_Output(index);
			HAL_PIN_SetOutputValue(index, 0);
		}
		break;
		case IOR_LED_WIFI:
		case IOR_LED_WIFI_n:
		{
			HAL_PIN_Setup_Output(index);
		}
		break;
		case IOR_BAT_ADC:
		case IOR_ADC_Button:
		case IOR_ADC:
			// init ADC for given pin
			HAL_ADC_Init(index);
			break;
		case IOR_PWM_n:
		case IOR_PWM:
		{
			int channelIndex;
			float channelValue;

			channelIndex = PIN_GetPinChannelForPinIndex(index);
			channelValue = g_channelValuesFloats[channelIndex];
			HAL_PIN_PWM_Start(index);

			if (role == IOR_PWM_n) {
				// inversed PWM
				HAL_PIN_PWM_Update(index, 100 - channelValue);
			}
			else {
				HAL_PIN_PWM_Update(index, channelValue);
			}
		}
		break;

		default:
			break;
		}
	}
	if (bSampleInitialState) {
		if (PIN_ReadDigitalInputValue_WithInversionIncluded(index)) {
			BIT_SET(g_initialPinStates, index);
		}
		else {
			BIT_CLEAR(g_initialPinStates, index);
		}
	}

	if (bDHTChange) {
#ifdef ENABLE_DRIVER_DHT
		// TODO: better place to call?
		DHT_OnPinsConfigChanged();
#endif
	}
}

void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex)) {
	g_doubleClickCallback = cb;
}
void Channel_SaveInFlashIfNeeded(int ch) {
	// save, if marked as save value in flash (-1)
	if (g_cfg.startChannelValues[ch] == -1) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Channel_SaveInFlashIfNeeded: Channel %i is being saved to flash, state %i", ch, g_channelValues[ch]);
		HAL_FlashVars_SaveChannel(ch, g_channelValues[ch]);
	}
	else {
		//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Channel_SaveInFlashIfNeeded: Channel %i is not saved to flash, state %i", ch, g_channelValues[ch]);
	}
}
static void Channel_OnChanged(int ch, int prevValue, int iFlags) {
	int i;
	int iVal;
	int bOn;

	//bOn = BIT_CHECK(g_channelStates,ch);
	iVal = g_channelValues[ch];
	g_channelValuesFloats[ch] = (float)iVal;
	bOn = iVal > 0;

#if ENABLE_I2C
	I2C_OnChannelChanged(ch, iVal);
#endif

#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_OnChannelChanged(ch, iVal);
#endif

#if ENABLE_DRIVER_TUYAMCU
	TuyaMCU_OnChannelChanged(ch, iVal);
#endif

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			if (g_cfg.pins.roles[i] == IOR_Relay || g_cfg.pins.roles[i] == IOR_BAT_Relay || g_cfg.pins.roles[i] == IOR_LED) {
				RAW_SetPinValue(i, bOn);
			}
			else if (g_cfg.pins.roles[i] == IOR_Relay_n || g_cfg.pins.roles[i] == IOR_LED_n || g_cfg.pins.roles[i] == IOR_BAT_Relay_n) {
				RAW_SetPinValue(i, !bOn);
			}
			else if (g_cfg.pins.roles[i] == IOR_PWM) {
				HAL_PIN_PWM_Update(i, iVal);
			}
			else if (g_cfg.pins.roles[i] == IOR_PWM_n) {
				HAL_PIN_PWM_Update(i, 100 - iVal);
			}
		}
	}
	if ((iFlags & CHANNEL_SET_FLAG_SKIP_MQTT) == 0) {
		if (CHANNEL_ShouldBePublished(ch)) {
			MQTT_ChannelPublish(ch, 0);
		}
	}
	// Simple event - it just says that there was a change
	EventHandlers_FireEvent(CMD_EVENT_CHANNEL_ONCHANGE, ch);
	// more advanced events - change FROM value TO value
	EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CHANNEL0 + ch, prevValue, iVal);
	//addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,"CHANNEL_OnChanged: Channel index %i startChannelValues %i\n\r",ch,g_cfg.startChannelValues[ch]);

	Channel_SaveInFlashIfNeeded(ch);
}
void CFG_ApplyChannelStartValues() {
	int i;
	for (i = 0; i < CHANNEL_MAX; i++) {
		int iValue;

		iValue = g_cfg.startChannelValues[i];
		if (iValue == -1) {
			g_channelValuesFloats[i] = g_channelValues[i] = HAL_FlashVars_GetChannelValue(i);
			//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CFG_ApplyChannelStartValues: Channel %i is being set to REMEMBERED state %i", i, g_channelValues[i]);
		}
		else {
			g_channelValuesFloats[i] = g_channelValues[i] = iValue;
			//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CFG_ApplyChannelStartValues: Channel %i is being set to constant state %i", i, g_channelValues[i]);
		}
	}
}
int ChannelType_GetDecimalPlaces(int type) {
	int pl;

	int div = ChannelType_GetDivider(type);
	int cur = 10;
	for (pl = 0; pl < 6; pl++) {
		if (div < cur)
			return pl + 1;
		cur *= 10;
	}
	return pl;
}
int ChannelType_GetDivider(int type) {
	switch (type)
	{
	case ChType_Humidity_div10:
	case ChType_Temperature_div10:
	case ChType_Voltage_div10:
	case ChType_Power_div10:
	case ChType_Frequency_div10:
	case ChType_ReadOnly_div10:
		return 10;
	case ChType_Frequency_div100:
	case ChType_Current_div100:
	case ChType_EnergyTotal_kWh_div100:
	case ChType_Voltage_div100:
	case ChType_PowerFactor_div100:
	case ChType_Pressure_div100:
	case ChType_Temperature_div100:
	case ChType_Power_div100:
	case ChType_ReadOnly_div100:
	case ChType_Ph:
		return 100;
	case ChType_PowerFactor_div1000:
	case ChType_EnergyTotal_kWh_div1000:
	case ChType_EnergyExport_kWh_div1000:
	case ChType_EnergyToday_kWh_div1000:
	case ChType_Current_div1000:
	case ChType_LeakageCurrent_div1000:
	case ChType_ReadOnly_div1000:
		return 1000;
	case ChType_Temperature_div2:
		return 2;
	}
	return 1;
}
const char *ChannelType_GetUnit(int type) {
	switch (type)
	{
	case ChType_BatteryLevelPercent:
	case ChType_Humidity:
	case ChType_Humidity_div10:
		return "%";
	case ChType_Temperature_div100:
	case ChType_Temperature_div10:
	case ChType_Temperature_div2:
	case ChType_Temperature:
		return "C";
	case ChType_Voltage_div100:
	case ChType_Voltage_div10:
		return "V";
	case ChType_Power:
	case ChType_Power_div10:
	case ChType_Power_div100:
		return "W";
	case ChType_Frequency_div10:
	case ChType_Frequency_div100:
		return "Hz";
	case ChType_LeakageCurrent_div1000:
	case ChType_Current_div1000:
	case ChType_Current_div100:
		return "A";
	case ChType_EnergyTotal_kWh_div1000:
	case ChType_EnergyExport_kWh_div1000:
	case ChType_EnergyToday_kWh_div1000:
	case ChType_EnergyTotal_kWh_div100:
		return "kWh";
	case ChType_PowerFactor_div1000:
	case ChType_PowerFactor_div100:
		return "";
	case ChType_Pressure_div100:
		return "hPa";
	case ChType_ReactivePower:
		return "vAr";
	case ChType_Illuminance:
		return "Lux";
	case ChType_Ph:
		return "Ph";
	case ChType_Orp:
		return "mV";
	case ChType_Tds:
		return "ppm";
	}
	return "";
}
const char *ChannelType_GetTitle(int type) {
	switch (type)
	{
	case ChType_BatteryLevelPercent:
		return "Battery";
	case ChType_Humidity:
	case ChType_Humidity_div10:
		return "Humidity";
	case ChType_Temperature_div100:
	case ChType_Temperature_div10:
	case ChType_Temperature_div2:
	case ChType_Temperature:
		return "Temperature";
	case ChType_Voltage_div100:
	case ChType_Voltage_div10:
		return "Voltage"; 
	case ChType_Power:
	case ChType_Power_div10:
	case ChType_Power_div100:
		return "Power";
	case ChType_Frequency_div10:
	case ChType_Frequency_div100:
		return "Frequency";
	case ChType_Current_div1000:
	case ChType_Current_div100:
		return "Current";
	case ChType_LeakageCurrent_div1000:
		return "Leakage"; 
	case ChType_EnergyTotal_kWh_div1000:
	case ChType_EnergyTotal_kWh_div100:
		return "EnergyTotal";
	case ChType_EnergyExport_kWh_div1000:
		return "EnergyExport";
	case ChType_EnergyToday_kWh_div1000:
		return "EnergyToday";
	case ChType_PowerFactor_div1000:
	case ChType_PowerFactor_div100:
		return "PowerFactor";
	case ChType_Pressure_div100:
		return "Pressure";
	case ChType_ReactivePower:
		return "ReactivePower";
	case ChType_Illuminance:
		return "Illuminance";
	case ChType_Ph:
		return "Ph Water Quality";
	case ChType_Orp:
		return "Orp Water Quality";
	case ChType_Tds:
		return "TDS Water Quality";
	case ChType_ReadOnly:
	case ChType_ReadOnly_div10:
	case ChType_ReadOnly_div100:
	case ChType_ReadOnly_div1000:
		return "ReadOnly:";
	}
	return "";
}
float CHANNEL_GetFinalValue(int channel) {
	float dVal;

	dVal = CHANNEL_Get(channel);
	dVal /= ChannelType_GetDivider(CHANNEL_GetType(channel));

	return dVal;
}
float CHANNEL_GetFloat(int ch) {
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Get: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return 0;
	}
	return g_channelValuesFloats[ch];
}
int CHANNEL_Get(int ch) {
	// special channels
	if (ch == SPECIAL_CHANNEL_LEDPOWER) {
		return LED_GetEnableAll();
	}
	if (ch == SPECIAL_CHANNEL_BRIGHTNESS) {
		return LED_GetDimmer();
	}
	if (ch == SPECIAL_CHANNEL_TEMPERATURE) {
		return LED_GetTemperature();
	}
	if (ch >= SPECIAL_CHANNEL_BASECOLOR_FIRST && ch <= SPECIAL_CHANNEL_BASECOLOR_LAST) {
		return 0; // TODO
	}
	if (ch >= SPECIAL_CHANNEL_FLASHVARS_FIRST && ch <= SPECIAL_CHANNEL_FLASHVARS_LAST) {
		return HAL_FlashVars_GetChannelValue(ch - SPECIAL_CHANNEL_FLASHVARS_FIRST);
	}
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Get: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return 0;
	}
	return g_channelValues[ch];
}
void CHANNEL_ClearAllChannels() {
	int i;

	for (i = 0; i < CHANNEL_MAX; i++) {
		CHANNEL_Set(i, 0, CHANNEL_SET_FLAG_SILENT);
	}
}

void CHANNEL_Set_FloatPWM(int ch, float fVal, int iFlags) {
	int i;

	g_channelValues[ch] = (int)fVal;
	g_channelValuesFloats[ch] = fVal;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			if (g_cfg.pins.roles[i] == IOR_PWM) {
				HAL_PIN_PWM_Update(i, fVal);
			}
			else if (g_cfg.pins.roles[i] == IOR_PWM_n) {
				HAL_PIN_PWM_Update(i, 100.0f - fVal);
			}
		}
	}
}
void CHANNEL_Set(int ch, int iVal, int iFlags) {
	int prevValue;
	int bForce;
	int bSilent;
	bForce = iFlags & CHANNEL_SET_FLAG_FORCE;
	bSilent = iFlags & CHANNEL_SET_FLAG_SILENT;

	// special channels
	if (ch == SPECIAL_CHANNEL_LEDPOWER) {
		LED_SetEnableAll(iVal);
		return;
	}
	if (ch == SPECIAL_CHANNEL_BRIGHTNESS) {
		LED_SetDimmer(iVal);
		return;
	}
	if (ch == SPECIAL_CHANNEL_TEMPERATURE) {
		LED_SetTemperature(iVal, 1);
		return;
	}
	if (ch >= SPECIAL_CHANNEL_BASECOLOR_FIRST && ch <= SPECIAL_CHANNEL_BASECOLOR_LAST) {
		LED_SetBaseColorByIndex(ch - SPECIAL_CHANNEL_BASECOLOR_FIRST, iVal, 1);
		return;
	}
	if (ch >= SPECIAL_CHANNEL_FLASHVARS_FIRST && ch <= SPECIAL_CHANNEL_FLASHVARS_LAST) {
		HAL_FlashVars_SaveChannel(ch - SPECIAL_CHANNEL_FLASHVARS_FIRST, iVal);
		return;
	}
	if (ch < 0 || ch >= CHANNEL_MAX) {
		//if(bMustBeSilent==0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Set: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		//}
		return;
	}
	prevValue = g_channelValues[ch];
	if (bForce == 0) {
		if (prevValue == iVal) {
			if (bSilent == 0) {
				addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "No change in channel %i (still set to %i) - ignoring\n\r", ch, prevValue);
			}
			return;
		}
	}
	if (bSilent == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CHANNEL_Set channel %i has changed to %i (flags %i)\n\r", ch, iVal, iFlags);
	}
	g_channelValues[ch] = iVal;

	Channel_OnChanged(ch, prevValue, iFlags);
}
void CHANNEL_AddClamped(int ch, int iVal, int min, int max, int bWrapInsteadOfClamp) {
#if 0
	int prevValue;
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_AddClamped: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return;
	}
	prevValue = g_channelValues[ch];
	g_channelValues[ch] = g_channelValues[ch] + iVal;

	if (bWrapInsteadOfClamp) {
		if (g_channelValues[ch] > max)
			g_channelValues[ch] = min;
		if (g_channelValues[ch] < min)
			g_channelValues[ch] = max;
	}
	else {
		if (g_channelValues[ch] > max)
			g_channelValues[ch] = max;
		if (g_channelValues[ch] < min)
			g_channelValues[ch] = min;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CHANNEL_AddClamped channel %i has changed to %i\n\r", ch, g_channelValues[ch]);

	Channel_OnChanged(ch, prevValue, 0);
#else
	// we want to support special channel indexes, so it's better to use GET/SET interface
	// Special channel indexes are used to access things like dimmer, led colors, etc
	iVal = CHANNEL_Get(ch) + iVal;

	if (bWrapInsteadOfClamp) {
		if (iVal > max)
			iVal = min;
		if (iVal < min)
			iVal = max;
	}
	else {
		if (iVal > max)
			iVal = max;
		if (iVal < min)
			iVal = min;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CHANNEL_AddClamped channel %i has changed to %i\n\r", ch, iVal);

	CHANNEL_Set(ch, iVal, 0);
#endif
}
void CHANNEL_Add(int ch, int iVal) {
#if 0
	int prevValue;
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Add: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return;
	}
	prevValue = g_channelValues[ch];
	g_channelValues[ch] = g_channelValues[ch] + iVal;

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CHANNEL_Add channel %i has changed to %i\n\r", ch, g_channelValues[ch]);

	Channel_OnChanged(ch, prevValue, 0);
#else
	// we want to support special channel indexes, so it's better to use GET/SET interface
	// Special channel indexes are used to access things like dimmer, led colors, etc
	iVal = iVal + CHANNEL_Get(ch);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "CHANNEL_Add channel %i has changed to %i\n\r", ch, iVal);
	CHANNEL_Set(ch, iVal, 0);
#endif
}

int CHANNEL_FindMaxValueForChannel(int ch) {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		// is pin tied to this channel?
		if (g_cfg.pins.channels[i] == ch) {
			// is it PWM?
			if (g_cfg.pins.roles[i] == IOR_PWM) {
				return 100;
			}
			if (g_cfg.pins.roles[i] == IOR_PWM_n) {
				return 100;
			}
		}
	}
	if (g_cfg.pins.channelTypes[ch] == ChType_Dimmer)
		return 100;
	if (g_cfg.pins.channelTypes[ch] == ChType_Dimmer256)
		return 256;
	if (g_cfg.pins.channelTypes[ch] == ChType_Dimmer1000)
		return 1000;
	return 1;
}
// PWMs are toggled between 0 and 100 (0% and 100% PWM)
// Relays and everything else is toggled between 0 (off) and 1 (on)
void CHANNEL_Toggle(int ch) {
	int prev;

	// special channels
	if (ch == SPECIAL_CHANNEL_LEDPOWER) {
		LED_SetEnableAll(!LED_GetEnableAll());
		return;
	}
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Toggle: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return;
	}
	prev = g_channelValues[ch];
	if (g_channelValues[ch] == 0)
		g_channelValues[ch] = CHANNEL_FindMaxValueForChannel(ch);
	else
		g_channelValues[ch] = 0;

	Channel_OnChanged(ch, prev, 0);
}
int CHANNEL_HasChannelPinWithRoleOrRole(int ch, int iorType, int iorType2) {
	int i;

	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_HasChannelPinWithRole: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return 0;
	}
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			if (g_cfg.pins.roles[i] == iorType)
				return 1;
			else if (g_cfg.pins.roles[i] == iorType2)
				return 1;
		}
	}
	return 0;
}
int CHANNEL_HasChannelPinWithRole(int ch, int iorType) {
	int i;

	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_HasChannelPinWithRole: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return 0;
	}
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			if (g_cfg.pins.roles[i] == iorType)
				return 1;
		}
	}
	return 0;
}
bool CHANNEL_Check(int ch) {
	if (ch == SPECIAL_CHANNEL_LEDPOWER) {
		return LED_GetEnableAll();
	}
	if (ch < 0 || ch >= CHANNEL_MAX) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "CHANNEL_Check: Channel index %i is out of range <0,%i)\n\r", ch, CHANNEL_MAX);
		return 0;
	}
	if (g_channelValues[ch] > 0)
		return 1;
	return 0;
}

bool CHANNEL_IsInUse(int ch) {
	int i;

	if (g_cfg.pins.channelTypes[ch] != ChType_Default) {
		return true;
	}

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] != IOR_None) {
			if (g_cfg.pins.channels[i] == ch) {
				return true;
			}
			if (g_cfg.pins.channels2[i] == ch) {
				return true;
			}
		}
	}
	return false;
}


bool CHANNEL_IsPowerRelayChannel(int ch) {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			int role = g_cfg.pins.roles[i];
			// NOTE: do not include Battery relay
			if (role == IOR_Relay || role == IOR_Relay_n) {
				return true;
			}
			// Also allow toggling Bridge channel
			// https://www.elektroda.com/rtvforum/viewtopic.php?p=20906463#20906463
			if (role == IOR_BridgeForward || role == IOR_BridgeReverse) {
				return true;
			}
		}
	}
	return false;
}
bool CHANNEL_ShouldBePublished(int ch) {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int role = g_cfg.pins.roles[i];

		if (g_cfg.pins.channels[i] == ch) {
			if (role == IOR_Relay || role == IOR_Relay_n
				|| role == IOR_LED || role == IOR_LED_n
				|| role == IOR_ADC || role == IOR_BAT_ADC
				|| role == IOR_CHT83XX_DAT || role == IOR_SHT3X_DAT || role == IOR_SGP_DAT
				|| role == IOR_DigitalInput || role == IOR_DigitalInput_n
				|| role == IOR_DoorSensorWithDeepSleep || role == IOR_DoorSensorWithDeepSleep_NoPup
				|| role == IOR_DoorSensorWithDeepSleep_pd
				|| IS_PIN_DHT_ROLE(role)
				|| role == IOR_DigitalInput_NoPup || role == IOR_DigitalInput_NoPup_n) {
				return true;
			}
		}
		else if (g_cfg.pins.channels2[i] == ch) {
			if (IS_PIN_DHT_ROLE(role)) {
				return true;
			}
			// SGP, CHT8305 and SHT3X uses secondary channel for humidity
			if (role == IOR_CHT83XX_DAT || role == IOR_SHT3X_DAT || role == IOR_SGP_DAT) {
				return true;
			}
		}
	}
	if (g_cfg.pins.channelTypes[ch] != ChType_Default) {
		return true;
	}
#ifdef ENABLE_DRIVER_TUYAMCU
	// publish if channel is used by TuyaMCU (no pin role set), for example door sensor state with power saving V0 protocol
	// Not enabled by default, you have to set OBK_FLAG_TUYAMCU_ALWAYSPUBLISHCHANNELS flag
	if (CFG_HasFlag(OBK_FLAG_TUYAMCU_ALWAYSPUBLISHCHANNELS) && TuyaMCU_IsChannelUsedByTuyaMCU(ch)) {
		return true;
	}
#endif
	if (CFG_HasFlag(OBK_FLAG_MQTT_PUBLISH_ALL_CHANNELS)) {
		return true;
	}
	return false;
}
int CHANNEL_GetRoleForOutputChannel(int ch) {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.channels[i] == ch) {
			switch (g_cfg.pins.roles[i]) {
			case IOR_BAT_Relay:
			case IOR_BAT_Relay_n:
			case IOR_Relay:
			case IOR_Relay_n:
			case IOR_LED:
			case IOR_LED_n:
			case IOR_PWM_n:
			case IOR_PWM:
				return g_cfg.pins.roles[i];
			case IOR_BridgeForward:
			case IOR_BridgeReverse:
				return g_cfg.pins.roles[i];
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


#define EVENT_CB(ev)   

#define PIN_TMR_LOOPS_PER_SECOND (1000/PIN_TMR_DURATION)
#define ADC_SAMPLING_TICK_COUNT PIN_TMR_LOOPS_PER_SECOND


void PIN_Input_Handler(int pinIndex, uint32_t ms_since_last)
{
	pinButton_s* handle;
	uint8_t read_gpio_level;

	handle = &g_buttons[pinIndex];
	if (handle->hal_button_Level != 0) {
		read_gpio_level = handle->hal_button_Level(handle);
	}
	else {
		read_gpio_level = handle->button_level;
	}

	//ticks counter working..
	if ((handle->state) > 0)
		handle->ticks += ms_since_last;

	/*------------button debounce handle---------------*/
	if (read_gpio_level != handle->button_level) { //not equal to prev one
		//continue read 3 times same new level change
		handle->debounce_cnt += ms_since_last;

		if (handle->debounce_cnt >= BTN_DEBOUNCE_MS) {
			handle->button_level = read_gpio_level;
			handle->debounce_cnt = 0;
		}
	}
	else { //leved not change ,counter reset.
		handle->debounce_cnt = 0;
	}

	/*-----------------State machine-------------------*/
	switch (handle->state) {
	case 0:
		if (handle->button_level == handle->active_level) {  //start press down
			handle->event = (uint8_t)BTN_PRESS_DOWN;
			EVENT_CB(BTN_PRESS_DOWN);
			Button_OnInitialPressDown(pinIndex);
			handle->ticks = 0;
			handle->repeat = 1;
			handle->state = 1;
		}
		else {
			handle->event = (uint8_t)BTN_NONE_PRESS;
		}
		break;

	case 1:
		if (handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			Button_OnPressRelease(pinIndex);
			handle->ticks = 0;
			handle->state = 2;

		}
		else if (handle->ticks > BTN_LONG_MS) {
			handle->event = (uint8_t)BTN_LONG_RRESS_START;
			Button_OnLongPressHoldStart(pinIndex);
			EVENT_CB(BTN_LONG_RRESS_START);
			handle->state = 5;
		}
		break;

	case 2:
		if (handle->button_level == handle->active_level) { //press down again
			handle->event = (uint8_t)BTN_PRESS_DOWN;
			EVENT_CB(BTN_PRESS_DOWN);
			handle->repeat++;
			//if(handle->repeat == 2) {
			//  EVENT_CB(BTN_DOUBLE_CLICK); // repeat hit
			//  Button_OnDoubleClick(pinIndex);
			//}
			EVENT_CB(BTN_PRESS_REPEAT); // repeat hit
			handle->ticks = 0;
			handle->state = 3;
		}
		else if (handle->ticks > BTN_SHORT_MS) { //released timeout
			if (handle->repeat == 1) {
				handle->event = (uint8_t)BTN_SINGLE_CLICK;
				EVENT_CB(BTN_SINGLE_CLICK);
				Button_OnShortClick(pinIndex);
			}
			else if (handle->repeat == 2) {
				handle->event = (uint8_t)BTN_DOUBLE_CLICK;
				Button_OnDoubleClick(pinIndex);
			}
			else if (handle->repeat == 3) {
				handle->event = (uint8_t)BTN_TRIPLE_CLICK;
				Button_OnTripleClick(pinIndex);
			}
			else if (handle->repeat == 4) {
				handle->event = (uint8_t)BTN_QUADRUPLE_CLICK;
				Button_OnQuadrupleClick(pinIndex);
			}
			else if (handle->repeat == 5) {
				handle->event = (uint8_t)BTN_5X_CLICK;
				Button_On5xClick(pinIndex);
			}
			handle->state = 0;
		}
		break;

	case 3:
		if (handle->button_level != handle->active_level) { //released press up
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			Button_OnPressRelease(pinIndex);
			if (handle->ticks < BTN_SHORT_MS) {
				handle->ticks = 0;
				handle->state = 2; //repeat press
			}
			else {
				handle->state = 0;
			}
		}
		break;

	case 5:
		if (handle->button_level == handle->active_level) {
			//continue hold trigger
			handle->event = (uint8_t)BTN_LONG_PRESS_HOLD;
			handle->holdRepeatTicks += ms_since_last;
			if (handle->holdRepeatTicks > BTN_HOLD_REPEAT_MS) {
				Button_OnLongPressHold(pinIndex);
				handle->holdRepeatTicks = 0;
			}
			EVENT_CB(BTN_LONG_PRESS_HOLD);
		}
		else { //releasd
			handle->event = (uint8_t)BTN_PRESS_UP;
			EVENT_CB(BTN_PRESS_UP);
			Button_OnPressRelease(pinIndex);
			handle->state = 0; //reset
		}
		break;
	}
}

void PIN_set_wifi_led(int value) {
	int i;
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_cfg.pins.roles[i] == IOR_LED_WIFI) {
			RAW_SetPinValue(i, value);
		}
		else if (g_cfg.pins.roles[i] == IOR_LED_WIFI_n) {
			// inversed
			RAW_SetPinValue(i, !value);
		}
	}
}

static uint32_t g_time = 0;
static uint32_t g_last_time = 0;
static int activepoll_time = 0; // time to keep polling active until

//  background ticks, timer repeat invoking interval defined by PIN_TMR_DURATION.
void PIN_ticks(void* param)
{
	int i;
	int value;

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	g_time = rtos_get_time();
#else
	g_time += PIN_TMR_DURATION;
#endif
	uint32_t t_diff = g_time - g_last_time;
	// cope with wrap
	if (t_diff > 0x4000) {
		t_diff = ((g_time + 0x4000) - (g_last_time + 0x4000));
	}
	g_last_time = g_time;

	BTN_SHORT_MS = (g_cfg.buttonShortPress * 100);
	BTN_LONG_MS = (g_cfg.buttonLongPress * 100);
	BTN_HOLD_REPEAT_MS = (g_cfg.buttonHoldRepeat * 100);

	int debounceMS;
	if (CFG_HasFlag(OBK_FLAG_BTN_INSTANTTOUCH)) {
		debounceMS = 100;
	}
	else {
		debounceMS = 250;
	}

	int activepins = 0;
	uint32_t pinvalues[2] = { 0, 0 };

	for (i = 0; i < PLATFORM_GPIO_MAX; i++)
	{
		// note pins which are active - i.e. would not trigger an edge interrupt on change.
		// if we have any, then we must poll until none
		// TODO: this will only be used when GPI interrupt triggeringis used.
		// but it's useful info anyway...

		if (i >= 32)
		{
			if (g_gpio_index_map[1] & (1 << (i - 32)))
			{
				uint32_t level = 1;
				if (g_gpio_edge_map[1] & (1 << (i - 32))) {
					level = 0;
				}
				int rawval = HAL_PIN_ReadDigitalInput(i);
				if (rawval && level == 1) {
					activepins++;
					pinvalues[1] |= (1 << (i - 32));
				}
				if (!rawval && level == 0) {
					activepins++;
					pinvalues[1] |= (1 << (i - 32));
				}
			}
		}
		else {
			if (g_gpio_index_map[0] & (1 << i))
			{
				uint32_t level = 1;
				if (g_gpio_edge_map[0] & (1 << i)) {
					level = 0;
				}
				int rawval = HAL_PIN_ReadDigitalInput(i);
				if (rawval && level == 1) {
					activepins++;
					pinvalues[0] |= (1 << i);
				}
				if (!rawval && level == 0) {
					activepins++;
					pinvalues[0] |= (1 << i);
				}
			}
		}

		// activepins is count of pins which are 'active', i.e. match thier expected active level
		if (activepins) {
			activepoll_time = 1000; //20 x 50ms = 1s of polls after button release
		}

#if 1
		if (g_cfg.pins.roles[i] == IOR_PWM) {
			HAL_PIN_PWM_Update(i, g_channelValuesFloats[g_cfg.pins.channels[i]]);
		}
		else if (g_cfg.pins.roles[i] == IOR_PWM_n) {
			// invert PWM value
			HAL_PIN_PWM_Update(i, 100 - g_channelValuesFloats[g_cfg.pins.channels[i]]);
		}
		else
#endif
			if (g_cfg.pins.roles[i] == IOR_Button || g_cfg.pins.roles[i] == IOR_Button_n
				|| g_cfg.pins.roles[i] == IOR_Button_ToggleAll || g_cfg.pins.roles[i] == IOR_Button_ToggleAll_n
				|| g_cfg.pins.roles[i] == IOR_Button_NextColor || g_cfg.pins.roles[i] == IOR_Button_NextColor_n
				|| g_cfg.pins.roles[i] == IOR_Button_NextDimmer || g_cfg.pins.roles[i] == IOR_Button_NextDimmer_n
				|| g_cfg.pins.roles[i] == IOR_Button_NextTemperature || g_cfg.pins.roles[i] == IOR_Button_NextTemperature_n
				|| g_cfg.pins.roles[i] == IOR_Button_ScriptOnly || g_cfg.pins.roles[i] == IOR_Button_ScriptOnly_n
				|| g_cfg.pins.roles[i] == IOR_SmartButtonForLEDs || g_cfg.pins.roles[i] == IOR_SmartButtonForLEDs_n) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,"Test hold %i\r\n",i);
				PIN_Input_Handler(i, t_diff);
			}
			else if (g_cfg.pins.roles[i] == IOR_DigitalInput || g_cfg.pins.roles[i] == IOR_DigitalInput_n
				||
				g_cfg.pins.roles[i] == IOR_DigitalInput_NoPup || g_cfg.pins.roles[i] == IOR_DigitalInput_NoPup_n
				|| g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep || g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_NoPup
				|| g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_pd) {
				// read pin digital value (and already invert it if needed)
				value = PIN_ReadDigitalInputValue_WithInversionIncluded(i);

#if 0
				CHANNEL_Set(g_cfg.pins.channels[i], value, 0);
#else
				// debouncing
				if (value) {
					if (g_times[i] > debounceMS) {
						if (g_lastValidState[i] != value) {
							// became up
							g_lastValidState[i] = value;
							CHANNEL_Set(g_cfg.pins.channels[i], value, 0);
						}
					}
					else {
						g_times[i] += t_diff;
					}
					g_times2[i] = 0;
				}
				else {
					if (g_times2[i] > debounceMS) {
						if (g_lastValidState[i] != value) {
							// became down
							g_lastValidState[i] = value;
							CHANNEL_Set(g_cfg.pins.channels[i], value, 0);
						}
					}
					else {
						g_times2[i] += t_diff;
					}
					g_times[i] = 0;
				}

#endif
			}
			else if (g_cfg.pins.roles[i] == IOR_ToggleChannelOnToggle) {
				// we must detect a toggle, but with debouncing
				value = PIN_ReadDigitalInputValue_WithInversionIncluded(i);
				// debouncing
				if (g_times[i] <= 0) {
					if (g_lastValidState[i] != value) {
						// became up
						g_lastValidState[i] = value;


						if (CFG_HasFlag(OBK_FLAG_BUTTON_DISABLE_ALL)) {
							addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Child lock!");
						}
						else {
							CHANNEL_Toggle(g_cfg.pins.channels[i]);
							// fire event - IOR_ToggleChannelOnToggle has been toggle
							// Argument is a pin number (NOT channel)
							EventHandlers_FireEvent(CMD_EVENT_PIN_ONTOGGLE, i);
						}
						// lock for given time
						g_times[i] = debounceMS;
					}
				}
				else {
					g_times[i] -= t_diff;
				}
			}
	}

#ifdef PLATFORM_BEKEN
#ifdef BEKEN_PIN_GPI_INTERRUPTS
	// TODO: not implemented yet - this bit continues polling
	// for a while after a GPI is fired, so that we can see long press, etc.
	if (param) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Pin intr at %d (+%d) (%08lX)", g_time, t_diff, pinvalues[0]);
	}
#endif
#endif

	if (activepoll_time) {
		activepoll_time -= t_diff;
		if (activepoll_time <= 0) {
			activepoll_time = 0;
		}
	}
	if (activepoll_time) {
		// setup to poll in 50ms
#ifdef PLATFORM_BEKEN
#ifdef BEKEN_PIN_GPI_INTERRUPTS
		PIN_TriggerPoll();
		if (activepins) {
			addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Pins active at %d (%x)", g_time, pinvalues[0]);
		}
		else {
			addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Pins ->inactive at %d (%x)", g_time, pinvalues[0]);
		}
#endif
#endif

	}
	else {
#ifdef PLATFORM_BEKEN
#ifdef BEKEN_PIN_GPI_INTERRUPTS
		addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "Pins inactive at %d", g_time);
#endif      
#endif      
	}

}
const char* g_channelTypeNames[] = {
	"Default",
	"Error",
	"Temperature",
	"Humidity",
	// most likely will not be used
	// but it's humidity value times 10
	// TuyaMCU sends 225 instead of 22.5%
	"Humidity_div10",
	"Temperature_div10",
	"Toggle",
	// 0-100 range
	"Dimmer",
	"LowMidHigh",
	"TextField",
	"ReadOnly",
	// off (0) and 3 speeds
	"OffLowMidHigh",
	// off (0) and 5 speeds
	"OffLowestLowMidHighHighest",
	// only 5 speeds
	"LowestLowMidHighHighest",
	// like dimmer, but 0-255
	"Dimmer256",
	// like dimmer, but 0-1000
	"Dimmer1000",
	// for TuyaMCU power metering
	//NOTE: not used for BL0937 etc
	"Frequency_div100",
	"Voltage_div10",
	"Power",
	"Current_div100",
	"ActivePower",
	"PowerFactor_div1000",
	"ReactivePower",
	"EnergyTotal_kWh_div1000",
	"EnergyExport_kWh_div1000",
	"EnergyToday_kWh_div1000",
	"Current_div1000",
	"EnergyTotal_kWh_div100",
	"OpenClosed",
	"OpenClosed_Inv",
	"BatteryLevelPercent",
	"OffDimBright",
	"LowMidHighHighest",
	"OffLowMidHighHighest",
	"Custom",
	"Power_div10",
	"ReadOnlyLowMidHigh",
	"SmokePercent",
	"Illuminance",
	"Toggle_Inv",
	"OffOnRemember",
	"Voltage_div100",
	"Temperature_div2",
	"TimerSeconds",
	"Frequency_div10",
	"PowerFactor_div100",
	"Pressure_div100",
	"Temperature_div100",
	"LeakageCurrent_div1000",
	"Power_div100",
	"Motion",
	"ReadOnly_div10",
	"ReadOnly_div100",
	"ReadOnly_div1000",
	"Ph",
	"Orp",
	"Tds",
	"Motion_n",
	"error",
	"error",
};
// setChannelType 3 LowMidHigh
int CHANNEL_ParseChannelType(const char* s) {
	int i;
	for (i = 0; i < ChType_Max; i++) {
		if (!stricmp(g_channelTypeNames[i], s)) {
			return i;
		}
	}
	return ChType_Error;
}
static commandResult_t CMD_setButtonHoldRepeat(const void* context, const char* cmd, const char* args, int cmdFlags) {


	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "This command requires 1 argument - timeRepeat - current %i",
			g_cfg.buttonHoldRepeat);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}


	CFG_SetButtonRepeatPressTime(Tokenizer_GetArgInteger(0));

	CFG_Save_IfThereArePendingChanges();

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Times set, %i. Config autosaved to flash.",
		g_cfg.buttonHoldRepeat
	);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "If something is wrong, you can restore default %i",
		CFG_DEFAULT_BTN_REPEAT);
	return CMD_RES_OK;
}
// SetButtonTimes [ValLongPress] [ValShortPress] [ValRepeat]
// Each value is times 100ms, so: SetButtonTimes 2 1 1 means 200ms long press, 100ms short and 100ms repeat
static commandResult_t CMD_SetButtonTimes(const void* context, const char* cmd, const char* args, int cmdFlags) {


	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 3) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "This command requires 3 arguments - timeLong, timeShort, timeRepeat - current %i %i %i",
			g_cfg.buttonLongPress, g_cfg.buttonShortPress, g_cfg.buttonHoldRepeat);
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	CFG_SetButtonLongPressTime(Tokenizer_GetArgInteger(0));

	CFG_SetButtonShortPressTime(Tokenizer_GetArgInteger(1));

	CFG_SetButtonRepeatPressTime(Tokenizer_GetArgInteger(2));

	CFG_Save_IfThereArePendingChanges();

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Times set, %i %i %i. Config autosaved to flash.",
		g_cfg.buttonLongPress, g_cfg.buttonShortPress, g_cfg.buttonHoldRepeat
	);
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "If something is wrong, you can restore defaults - %i %i %i",
		CFG_DEFAULT_BTN_LONG, CFG_DEFAULT_BTN_SHORT, CFG_DEFAULT_BTN_REPEAT);
	return 0;
}
static commandResult_t CMD_ShowChannelValues(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int i;

	for (i = 0; i < CHANNEL_MAX; i++) {
		if (g_channelValues[i] > 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Channel %i value is %i", i, g_channelValues[i]);
		}
	}

	return CMD_RES_OK;
}
static commandResult_t CMD_SetChannelType(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int channel;
	const char* type;
	int typeCode;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "This command requires 2 arguments");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	channel = Tokenizer_GetArgInteger(0);
	type = Tokenizer_GetArg(1);

	typeCode = CHANNEL_ParseChannelType(type);
	if (typeCode == ChType_Error) {

		addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Channel %i type not set because %s is not a known type", channel, type);
		return CMD_RES_BAD_ARGUMENT;
	}

	CHANNEL_SetType(channel, typeCode);

	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "Channel %i type changed to %s", channel, type);
	return CMD_RES_OK;
}

/// @brief Computes the Relay and PWM count.
/// @param relayCount Number of relay and LED channels.
/// @param pwmCount Number of PWM channels.
void PIN_get_Relay_PWM_Count(int* relayCount, int* pwmCount, int* dInputCount) {
	int i;
	int pwmBits;
	if (relayCount) {
		(*relayCount) = 0;
	}
	if (pwmCount) {
		(*pwmCount) = 0;
	}
	if (dInputCount) {
		(*dInputCount) = 0;
	}

	// if we have two PWMs on single channel, count it once
	pwmBits = 0;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int role = PIN_GetPinRoleForPinIndex(i);
		switch (role) {
		case IOR_Relay:
		case IOR_Relay_n:
		case IOR_LED:
		case IOR_LED_n:
			if (relayCount) {
				(*relayCount)++;
			}
			break;
		case IOR_PWM:
		case IOR_PWM_n:
			// if we have two PWMs on single channel, count it once
			BIT_SET(pwmBits, g_cfg.pins.channels[i]);
			//(*pwmCount)++;
			break;
		case IOR_DigitalInput:
		case IOR_DigitalInput_n:
		case IOR_DigitalInput_NoPup:
		case IOR_DigitalInput_NoPup_n:
		case IOR_DoorSensorWithDeepSleep:
		case IOR_DoorSensorWithDeepSleep_NoPup:
		case IOR_DoorSensorWithDeepSleep_pd:
			if (dInputCount) {
				(*dInputCount)++;
			}
			break;
		default:
			break;
		}
	}
	if (pwmCount) {
		// if we have two PWMs on single channel, count it once
		for (i = 0; i < 32; i++) {
			if (BIT_CHECK(pwmBits, i)) {
				(*pwmCount)++;
			}
		}
	}
}


int h_isChannelPWM(int tg_ch) {
	int i;
	int role;
	int ch;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		ch = PIN_GetPinChannelForPinIndex(i);
		if (tg_ch != ch)
			continue;
		role = PIN_GetPinRoleForPinIndex(i);
		if (role == IOR_PWM || role == IOR_PWM_n) {
			return true;
		}
	}
	return false;
}
int h_isChannelRelay(int tg_ch) {
	int i;
	int role;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int ch = PIN_GetPinChannelForPinIndex(i);
		if (tg_ch != ch)
			continue;
		role = PIN_GetPinRoleForPinIndex(i);
		if (role == IOR_Relay || role == IOR_Relay_n || role == IOR_LED || role == IOR_LED_n) {
			return true;
		}
		if ((role == IOR_BridgeForward) || (role == IOR_BridgeReverse))
		{
			return true;
		}
	}
	return false;
}
int h_isChannelDigitalInput(int tg_ch) {
	int i;
	int role;

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int ch = PIN_GetPinChannelForPinIndex(i);
		if (tg_ch != ch)
			continue;
		role = PIN_GetPinRoleForPinIndex(i);
		if (role == IOR_DigitalInput || role == IOR_DigitalInput_n || role == IOR_DigitalInput_NoPup || role == IOR_DigitalInput_NoPup_n) {
			return true;
		}
		if (role == IOR_DoorSensorWithDeepSleep || role == IOR_DoorSensorWithDeepSleep_NoPup || role == IOR_DoorSensorWithDeepSleep_pd) {
			return true;
		}
	}
	return false;
}
static commandResult_t showgpi(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	int i;
	unsigned int value[2] = { 0, 0 };

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int val = 0;

		val = HAL_PIN_ReadDigitalInput(i);

		if (i >= 32)
			value[1] |= ((val & 1) << (i - 32));
		else
			value[0] |= ((val & 1) << i);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "GPIs are 0x%08lX%08lX", value[1], value[0]);
	return CMD_RES_OK;
}

void PIN_AddCommands(void)
{
	//cmddetail:{"name":"showgpi","args":"NULL",
	//cmddetail:"descr":"log stat of all GPIs",
	//cmddetail:"fn":"showgpi","file":"new_pins.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("showgpi", showgpi, NULL);
	//cmddetail:{"name":"setChannelType","args":"[ChannelIndex][TypeString]",
	//cmddetail:"descr":"Sets a custom type for channel. Types are mostly used to determine how to display channel value on GUI",
	//cmddetail:"fn":"CMD_SetChannelType","file":"new_pins.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setChannelType", CMD_SetChannelType, NULL);
	//cmddetail:{"name":"showChannelValues","args":"",
	//cmddetail:"descr":"log channel values",
	//cmddetail:"fn":"CMD_ShowChannelValues","file":"new_pins.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("showChannelValues", CMD_ShowChannelValues, NULL);
	//cmddetail:{"name":"setButtonTimes","args":"[ValLongPress][ValShortPress][ValRepeat]",
	//cmddetail:"descr":"Each value is times 100ms, so: SetButtonTimes 2 1 1 means 200ms long press, 100ms short and 100ms repeat",
	//cmddetail:"fn":"CMD_SetButtonTimes","file":"new_pins.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonTimes", CMD_SetButtonTimes, NULL);
	//cmddetail:{"name":"setButtonHoldRepeat","args":"[Value]",
	//cmddetail:"descr":"Sets just the hold button repeat time, given value is times 100ms, so write 1 for 100ms, 2 for 200ms, etc",
	//cmddetail:"fn":"CMD_setButtonHoldRepeat","file":"new_pins.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("setButtonHoldRepeat", CMD_setButtonHoldRepeat, NULL);

}
