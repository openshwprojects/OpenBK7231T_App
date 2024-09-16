#ifdef PLATFORM_ESPIDF

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "hal_generic_espidf.h"

#define LEDC_MAX_CH 6

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
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
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
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
	{ "NC", GPIO_NUM_NC },
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
	{ "NC", GPIO_NUM_NC },
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

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

static ledc_channel_config_t ledc_channel[LEDC_MAX_CH];
static bool g_ledc_init = false;

void InitLEDC()
{
	if(!g_ledc_init)
	{
		ledc_timer_config_t ledc_timer =
		{
			.duty_resolution = LEDC_TIMER_13_BIT,
			.freq_hz = 9765,
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.timer_num = LEDC_TIMER_0,
			.clk_cfg = SOC_MOD_CLK_RC_FAST,
		};
		ledc_timer_config(&ledc_timer);
		for(int i = 0; i < LEDC_MAX_CH; i++)
		{
			ledc_channel[i].channel = i;
			ledc_channel[i].duty = 0;
			ledc_channel[i].gpio_num = GPIO_NUM_NC;
			ledc_channel[i].speed_mode = LEDC_LOW_SPEED_MODE;
			ledc_channel[i].hpoint = 0;
			ledc_channel[i].timer_sel = LEDC_TIMER_0;
			ledc_channel[i].intr_type = LEDC_INTR_DISABLE;
		}
		g_ledc_init = true;
	}
}

int GetAvailableLedcChannel()
{
	for(int i = 0; i < LEDC_MAX_CH; i++)
	{
		if(ledc_channel[i].gpio_num == GPIO_NUM_NC)
		{
			return ledc_channel[i].channel;
		}
	}
	return -1;
}

int GetLedcChannelForPin(gpio_num_t pin)
{
	for(int i = 0; i < LEDC_MAX_CH; i++)
	{
		if(ledc_channel[i].gpio_num == pin)
		{
			return ledc_channel[i].channel;
		}
	}
	return -1;
}

int PIN_GetPWMIndexForPinIndex(int index)
{
	if(index >= g_numPins)
		return -1;
	espPinMapping_t* pin = g_pins + index;
	int ch = GetLedcChannelForPin(pin->pin);
	if(ch >= 0)
	{
		return ch;
	}
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
	if(index >= g_numPins)
		return 0;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin != GPIO_NUM_NC) return 1;
	else return 0;
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
	gpio_set_pull_mode(pin->pin, GPIO_FLOATING);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	gpio_set_direction(pin->pin, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLUP_ONLY);
	gpio_set_level(pin->pin, 0);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	int ch = GetLedcChannelForPin(pin->pin);
	if(ch >= 0)
	{
		ledc_stop(LEDC_LOW_SPEED_MODE, ch, 0);
		gpio_reset_pin(pin->pin);
		ledc_channel[ch].gpio_num = GPIO_NUM_NC;
	}
}

void HAL_PIN_PWM_Start(int index)
{
	if(index >= g_numPins)
		return;
	InitLEDC();
	espPinMapping_t* pin = g_pins + index;
	int freecha = GetAvailableLedcChannel();
	if(freecha >= 0)
	{
		ledc_channel[freecha].gpio_num = pin->pin;
		ledc_channel_config(&ledc_channel[freecha]);
		ADDLOG_INFO(LOG_FEATURE_PINS, "init ledc ch %i pin %i\n", freecha, pin->pin);
	}
	else
	{
		ADDLOG_ERROR(LOG_FEATURE_PINS, "PWM_Start: no free ledc channels for pin %i", pin->pin);
	}
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	int ch = GetLedcChannelForPin(pin->pin);
	if(ch >= 0)
	{
		uint32_t curduty = ledc_get_duty(LEDC_LOW_SPEED_MODE, ch);
		uint32_t propduty = value * 81.91;
		if(propduty != curduty)
		{
			ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, value * 81.91);
			ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
			if(value == 100.0f)
			{
				ledc_stop(LEDC_LOW_SPEED_MODE, ch, 1);
			}
			else if(value <= 0.01f)
			{
				ledc_stop(LEDC_LOW_SPEED_MODE, ch, 0);
			}
		}
	}
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

#endif // PLATFORM_ESPIDF
