#if PLATFORM_ESPIDF || PLATFORM_ESP8266

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "hal_pinmap_espidf.h"
#include "driver/ledc.h"
#include "../hal_pins.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	// SPI flash 11-17
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20 (RX)", GPIO_NUM_20, false },
	{ "IO21 (TX)", GPIO_NUM_21, false },
};

#elif CONFIG_IDF_TARGET_ESP32C2

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
};

#elif CONFIG_IDF_TARGET_ESP32C6

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
	{ "IO22", GPIO_NUM_22, false },
	{ "IO23", GPIO_NUM_23, false },
	{ "IO24", GPIO_NUM_24, false },
	{ "IO25", GPIO_NUM_25, false },
	{ "IO26", GPIO_NUM_26, false },
	{ "IO27", GPIO_NUM_27, false },
	{ "IO28", GPIO_NUM_28, false },
	{ "IO29", GPIO_NUM_29, false },
	{ "IO30", GPIO_NUM_30, false },
};

#elif CONFIG_IDF_TARGET_ESP32S2

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "IO26", GPIO_NUM_26, false },
	{ "IO27", GPIO_NUM_27, false },
	{ "IO28", GPIO_NUM_28, false },
	{ "IO29", GPIO_NUM_29, false },
	{ "IO30", GPIO_NUM_30, false },
	{ "IO31", GPIO_NUM_31, false },
	{ "IO32", GPIO_NUM_32, false },
	{ "IO33", GPIO_NUM_33, false },
	{ "IO34", GPIO_NUM_34, false },
	{ "IO35", GPIO_NUM_35, false },
	{ "IO36", GPIO_NUM_36, false },
	{ "IO37", GPIO_NUM_37, false },
	{ "IO38", GPIO_NUM_38, false },
	{ "IO39", GPIO_NUM_39, false },
	{ "IO40", GPIO_NUM_40, false },
	{ "IO41", GPIO_NUM_41, false },
	{ "IO42", GPIO_NUM_42, false },
	{ "IO43", GPIO_NUM_43, false },
	{ "IO44", GPIO_NUM_44, false },
	{ "IO45", GPIO_NUM_45, false },
	{ "IO46", GPIO_NUM_46, false },
};

#elif CONFIG_IDF_TARGET_ESP32S3

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "NC", GPIO_NUM_NC, true },
	{ "IO26", GPIO_NUM_26, false },
	{ "IO27", GPIO_NUM_27, false },
	{ "IO28", GPIO_NUM_28, false },
	{ "IO29", GPIO_NUM_29, false },
	{ "IO30", GPIO_NUM_30, false },
	{ "IO31", GPIO_NUM_31, false },
	{ "IO32", GPIO_NUM_32, false },
	{ "IO33", GPIO_NUM_33, false },
	{ "IO34", GPIO_NUM_34, false },
	{ "IO35", GPIO_NUM_35, false },
	{ "IO36", GPIO_NUM_36, false },
	{ "IO37", GPIO_NUM_37, false },
	{ "IO38", GPIO_NUM_38, false },
	{ "IO39", GPIO_NUM_39, false },
	{ "IO40", GPIO_NUM_40, false },
	{ "IO41", GPIO_NUM_41, false },
	{ "IO42", GPIO_NUM_42, false },
	{ "IO43", GPIO_NUM_43, false },
	{ "IO44", GPIO_NUM_44, false },
	{ "IO45", GPIO_NUM_45, false },
	{ "IO46", GPIO_NUM_46, false },
	{ "IO47", GPIO_NUM_47, false },
	{ "IO48", GPIO_NUM_48, false },
};

#elif CONFIG_IDF_TARGET_ESP32

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
	{ "IO22", GPIO_NUM_22, false },
	{ "IO23", GPIO_NUM_23, false },
	{ "NC", GPIO_NUM_NC, true },
	{ "IO25", GPIO_NUM_25, false },
	{ "IO26", GPIO_NUM_26, false },
	{ "IO27", GPIO_NUM_27, false },
	{ "IO28", GPIO_NUM_28, false },
	{ "IO29", GPIO_NUM_29, false },
	{ "IO30", GPIO_NUM_30, false },
	{ "IO31", GPIO_NUM_31, false },
	{ "IO32", GPIO_NUM_32, false },
	{ "IO33", GPIO_NUM_33, false },
	{ "IO34", GPIO_NUM_34, false },
	{ "IO35", GPIO_NUM_35, false },
	{ "IO36", GPIO_NUM_36, false },
	{ "IO37", GPIO_NUM_37, false },
	{ "IO38", GPIO_NUM_38, false },
	{ "IO39", GPIO_NUM_39, false },
};

#elif CONFIG_IDF_TARGET_ESP32C5

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
	{ "IO22", GPIO_NUM_22, false },
	{ "IO23", GPIO_NUM_23, false },
	{ "IO24", GPIO_NUM_24, false },
	{ "IO25", GPIO_NUM_25, false },
	{ "IO26", GPIO_NUM_26, false },
	{ "IO27", GPIO_NUM_27, false },
	{ "IO28", GPIO_NUM_28, false },
};

#elif CONFIG_IDF_TARGET_ESP32C61

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO6", GPIO_NUM_6, false },
	{ "IO7", GPIO_NUM_7, false },
	{ "IO8", GPIO_NUM_8, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO11", GPIO_NUM_11, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_16, false },
	{ "IO17", GPIO_NUM_17, false },
	{ "IO18", GPIO_NUM_18, false },
	{ "IO19", GPIO_NUM_19, false },
	{ "IO20", GPIO_NUM_20, false },
	{ "IO21", GPIO_NUM_21, false },
};

#elif PLATFORM_ESP8266

espPinMapping_t g_pins[] = {
	{ "IO0", GPIO_NUM_0, false },
	{ "IO1", GPIO_NUM_1, false },
	{ "IO2", GPIO_NUM_2, false },
	{ "IO3", GPIO_NUM_3, false },
	{ "IO4", GPIO_NUM_4, false },
	{ "IO5", GPIO_NUM_5, false },
	{ "IO9", GPIO_NUM_9, false },
	{ "IO10", GPIO_NUM_10, false },
	{ "IO12", GPIO_NUM_12, false },
	{ "IO13", GPIO_NUM_13, false },
	{ "IO14", GPIO_NUM_14, false },
	{ "IO15", GPIO_NUM_15, false },
	{ "IO16", GPIO_NUM_15, false },
};

#else

espPinMapping_t g_pins[] = { };

#endif

#if PLATFORM_ESP8266
#include "driver/pwm.h"
#define gpio_reset_pin(x) //ESP_ConfigurePin(x, GPIO_MODE_INPUT, false, false, GPIO_INTR_DISABLE)
#define LEDC_MAX_CH 8
#else
#define LEDC_MAX_CH 6
#endif

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return;
	gpio_set_level(pin->pin, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return 0;
	return gpio_get_level(pin->pin);
}

void ESP_ConfigurePin(gpio_num_t pin, gpio_mode_t mode, bool pup, bool pdown, gpio_int_type_t intr)
{
	gpio_config_t conf = {};
	conf.pin_bit_mask = 1ULL << (uint32_t)pin;
	conf.mode = mode;
	conf.pull_up_en = pup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
	conf.pull_down_en = pdown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
	conf.intr_type = intr;
	gpio_config(&conf);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return;
	if(!pin->isConfigured)
	{
		pin->isConfigured = true;
		ESP_ConfigurePin(pin->pin, GPIO_MODE_INPUT, true, false, GPIO_INTR_DISABLE);
		return;
	}
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLUP_ONLY);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return;
	if(!pin->isConfigured)
	{
		pin->isConfigured = true;
		ESP_ConfigurePin(pin->pin, GPIO_MODE_INPUT, false, true, GPIO_INTR_DISABLE);
		return;
	}
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLDOWN_ONLY);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return;
	if(!pin->isConfigured)
	{
		pin->isConfigured = true;
		ESP_ConfigurePin(pin->pin, GPIO_MODE_INPUT, false, false, GPIO_INTR_DISABLE);
		return;
	}
	gpio_set_direction(pin->pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(pin->pin, GPIO_FLOATING);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + index;
	if(pin->pin == GPIO_NUM_NC) return;
	if(!pin->isConfigured)
	{
		pin->isConfigured = true;
		ESP_ConfigurePin(pin->pin, GPIO_MODE_OUTPUT, true, false, GPIO_INTR_DISABLE);
		return;
	}
	gpio_set_direction(pin->pin, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(pin->pin, GPIO_PULLUP_ONLY);
	gpio_set_level(pin->pin, 0);
}

#if PLATFORM_ESPIDF || PLATFORM_ESP8266

static ledc_channel_config_t ledc_channel[LEDC_MAX_CH];
static float obk_ch_value[LEDC_MAX_CH];
static bool g_ledc_init = false;

void InitLEDC()
{
	if(!g_ledc_init)
	{
		ledc_timer_config_t ledc_timer =
		{
			.duty_resolution = LEDC_TIMER_13_BIT,
			.freq_hz = 1000,
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.timer_num = LEDC_TIMER_0,
#if PLATFORM_ESPIDF
			.clk_cfg = SOC_MOD_CLK_RC_FAST,
#endif
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

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	espPinMapping_t* pin = g_pins + index;
#if PLATFORM_ESP8266
	// it can be used, but will result in bootloop on reset
	if(pin->pin == GPIO_NUM_0) return 0;
#endif
	if(pin->pin != GPIO_NUM_NC) return 1;
	else return 0;
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
		pin->isConfigured = false;
		ledc_channel[ch].gpio_num = GPIO_NUM_NC;
	}
}

void HAL_PIN_PWM_Start(int index, int freq)
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
#if PLATFORM_ESP8266
		// will bootloop without delay
		delay_ms(100);
		pwm_deinit();
		ledc_fade_func_install(0);
#endif
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
#if PLATFORM_ESPIDF
		uint32_t propduty = value * 81.91;
#else
		uint32_t propduty = value * 81.96;
#endif
		if(value != obk_ch_value[ch]) 
		{ 
			obk_ch_value[ch] = value;
			ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, propduty);
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

#endif

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}


OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void ESP_Interrupt(void* context) {
	int obkPinNum = (int)context;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

bool b_esp_ready = false;
void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	if (b_esp_ready == false) {
		gpio_install_isr_service(0);
		b_esp_ready = true;
	}
	espPinMapping_t* esp_cf = g_pins + pinIndex;
	int esp_mode;
	if (mode == INTERRUPT_RISING) {
		esp_mode = GPIO_INTR_POSEDGE;
	}
	else {
		esp_mode = GPIO_INTR_NEGEDGE;
	}
	ESP_ConfigurePin(esp_cf->pin, GPIO_MODE_INPUT, true, false, esp_mode);
	gpio_isr_handler_add(esp_cf->pin, ESP_Interrupt, (void*)pinIndex);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}

	espPinMapping_t* esp_cf;
	esp_cf = g_pins + pinIndex;
	gpio_isr_handler_remove(esp_cf->pin);
	///gpio_uninstall_isr_service();
	g_handlers[pinIndex] = 0;
}

#endif // PLATFORM_ESPIDF
