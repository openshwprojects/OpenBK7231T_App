#if PLATFORM_GD32VW553

#include "../hal_adc.h"
#include "../../logging/logging.h"
#include "hal_pinmap_gd32vw553.h"
#include "gd32vw55x_adc.h"
#include "wrapper_os.h"

void HAL_ADC_Init(int index)
{
	if(index >= g_numPins)
		return;
	if((g_pins[index].gpio == GPIOA && g_pins[index].pin >= GPIO_PIN_0 && g_pins[index].pin <= GPIO_PIN_7)
		|| (g_pins[index].gpio == GPIOB && g_pins[index].pin == GPIO_PIN_0))
	{
		gpio_mode_set(g_pins[index].gpio, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, g_pins[index].pin);
	}
}

int HAL_ADC_Read(int index) 
{
	if(index >= g_numPins)
		return 0;
	if((g_pins[index].gpio == GPIOA && g_pins[index].pin >= GPIO_PIN_0 && g_pins[index].pin <= GPIO_PIN_7)
		|| (g_pins[index].gpio == GPIOB && g_pins[index].pin == GPIO_PIN_0))
	{
		uint8_t ch_id = g_pins[index].gpio == GPIOA ? index : 8;
		adc_flag_clear(ADC_FLAG_EOC);
		adc_external_trigger_config(ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
		adc_channel_length_config(ADC_ROUTINE_CHANNEL, 1);
		adc_routine_channel_config(0, ch_id, ADC_SAMPLETIME_55POINT5);
		adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);
		adc_special_function_config(ADC_SCAN_MODE, DISABLE);
		adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
		adc_channel_length_config(ADC_ROUTINE_CHANNEL, 1);
		adc_resolution_config(ADC_RESOLUTION_12B);

		adc_enable();
		adc_software_trigger_enable(ADC_ROUTINE_CHANNEL);
		sys_ms_sleep(1);
		while(SET != adc_flag_get(ADC_FLAG_EOC));

		uint16_t raw = adc_routine_data_read();

		adc_disable();
		return raw;
	}
	return 0;
}

#endif
