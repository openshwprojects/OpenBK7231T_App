#ifdef PLATFORM_ESPIDF

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "driver/gpio.h"

typedef struct espPinMapping_s
{
	const char* name;
	gpio_num_t pin;
} espPinMapping_t;

#ifdef CONFIG_IDF_TARGET_ESP32C3

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	// SPI flash 11-17
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20 (RX)", GPIO_NUM_20 },
	{ "IO21 (TX)", GPIO_NUM_21 },
};

#elif CONFIG_IDF_TARGET_ESP32C2

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20", GPIO_NUM_20 },
};

#elif CONFIG_IDF_TARGET_ESP32C6

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20", GPIO_NUM_20 },
	{ "IO21", GPIO_NUM_21 },
	{ "IO22", GPIO_NUM_22 },
	{ "IO23", GPIO_NUM_23 },
	{ "IO24", GPIO_NUM_24 },
	{ "IO25", GPIO_NUM_25 },
	{ "IO26", GPIO_NUM_26 },
	{ "IO27", GPIO_NUM_27 },
	{ "IO28", GPIO_NUM_28 },
	{ "IO29", GPIO_NUM_29 },
	{ "IO30", GPIO_NUM_30 },
};

#elif CONFIG_IDF_TARGET_ESP32S2

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20", GPIO_NUM_20 },
	{ "IO21", GPIO_NUM_21 },
	{ "IO26", GPIO_NUM_26 },
	{ "IO27", GPIO_NUM_27 },
	{ "IO28", GPIO_NUM_28 },
	{ "IO29", GPIO_NUM_29 },
	{ "IO30", GPIO_NUM_30 },
	{ "IO31", GPIO_NUM_31 },
	{ "IO32", GPIO_NUM_32 },
	{ "IO33", GPIO_NUM_33 },
	{ "IO34", GPIO_NUM_34 },
	{ "IO35", GPIO_NUM_35 },
	{ "IO36", GPIO_NUM_36 },
	{ "IO37", GPIO_NUM_37 },
	{ "IO38", GPIO_NUM_38 },
	{ "IO39", GPIO_NUM_39 },
	{ "IO40", GPIO_NUM_40 },
	{ "IO41", GPIO_NUM_41 },
	{ "IO42", GPIO_NUM_42 },
	{ "IO43", GPIO_NUM_43 },
	{ "IO44", GPIO_NUM_44 },
	{ "IO45", GPIO_NUM_45 },
	{ "IO46", GPIO_NUM_46 },
};

#elif CONFIG_IDF_TARGET_ESP32S3

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20", GPIO_NUM_20 },
	{ "IO21", GPIO_NUM_21 },
	{ "IO26", GPIO_NUM_26 },
	{ "IO27", GPIO_NUM_27 },
	{ "IO28", GPIO_NUM_28 },
	{ "IO29", GPIO_NUM_29 },
	{ "IO30", GPIO_NUM_30 },
	{ "IO31", GPIO_NUM_31 },
	{ "IO32", GPIO_NUM_32 },
	{ "IO33", GPIO_NUM_33 },
	{ "IO34", GPIO_NUM_34 },
	{ "IO35", GPIO_NUM_35 },
	{ "IO36", GPIO_NUM_36 },
	{ "IO37", GPIO_NUM_37 },
	{ "IO38", GPIO_NUM_38 },
	{ "IO39", GPIO_NUM_39 },
	{ "IO40", GPIO_NUM_40 },
	{ "IO41", GPIO_NUM_41 },
	{ "IO42", GPIO_NUM_42 },
	{ "IO43", GPIO_NUM_43 },
	{ "IO44", GPIO_NUM_44 },
	{ "IO45", GPIO_NUM_45 },
	{ "IO46", GPIO_NUM_46 },
	{ "IO47", GPIO_NUM_47 },
	{ "IO48", GPIO_NUM_48 },
};

#elif CONFIG_IDF_TARGET_ESP32

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0 },
	{ "IO1", GPIO_NUM_1 },
	{ "IO2", GPIO_NUM_2 },
	{ "IO3", GPIO_NUM_3 },
	{ "IO4", GPIO_NUM_4 },
	{ "IO5", GPIO_NUM_5 },
	{ "IO6", GPIO_NUM_6 },
	{ "IO7", GPIO_NUM_7 },
	{ "IO8", GPIO_NUM_8 },
	{ "IO9", GPIO_NUM_9 },
	{ "IO10", GPIO_NUM_10 },
	{ "IO11", GPIO_NUM_11 },
	{ "IO12", GPIO_NUM_12 },
	{ "IO13", GPIO_NUM_13 },
	{ "IO14", GPIO_NUM_14 },
	{ "IO15", GPIO_NUM_15 },
	{ "IO16", GPIO_NUM_16 },
	{ "IO17", GPIO_NUM_17 },
	{ "IO18", GPIO_NUM_18 },
	{ "IO19", GPIO_NUM_19 },
	{ "IO20", GPIO_NUM_20 },
	{ "IO21", GPIO_NUM_21 },
	{ "IO22", GPIO_NUM_22 },
	{ "IO23", GPIO_NUM_23 },
	{ "IO25", GPIO_NUM_25 },
	{ "IO26", GPIO_NUM_26 },
	{ "IO27", GPIO_NUM_27 },
	{ "IO28", GPIO_NUM_28 },
	{ "IO29", GPIO_NUM_29 },
	{ "IO30", GPIO_NUM_30 },
	{ "IO31", GPIO_NUM_31 },
	{ "IO32", GPIO_NUM_32 },
	{ "IO33", GPIO_NUM_33 },
	{ "IO34", GPIO_NUM_34 },
	{ "IO35", GPIO_NUM_35 },
	{ "IO36", GPIO_NUM_36 },
	{ "IO37", GPIO_NUM_37 },
	{ "IO38", GPIO_NUM_38 },
	{ "IO39", GPIO_NUM_39 },
};

#else

espPinMapping_t g_pins[] = { };

#endif

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

int PIN_GetPWMIndexForPinIndex(int pin)
{
	return -1;
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	return 0;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_level(pin->pin, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	espPinMapping_t* pin = g_pins + index;
	return gpio_get_level(pin->pin);
}


void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLUP_ONLY);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLDOWN_ONLY);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_direction(pin->pin, GPIO_MODE_OUTPUT);
}

void HAL_PIN_PWM_Stop(int index)
{

}

void HAL_PIN_PWM_Start(int index)
{

}

void HAL_PIN_PWM_Update(int index, float value)
{

}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_ESPIDF
