//#include "../hal_adc.h"
#include "hal_pinmap_ln882h.h"

#if PLATFORM_LN882H

#include "hal/hal_adc.h"

void HAL_ADC_Init(int pinNumber)
{
	adc_init_t_def adc_init          = { 0 };

	adc_init.adc_ch                  = ADC_CH0;
	adc_init.adc_conv_mode           = ADC_CONV_MODE_CONTINUE;
	adc_init.adc_presc               = 0xFF;
	if(pinNumber != 0xFF)
	{
		adc_init.adc_ov_smp_ratio    = ADC_OVER_SAMPLING_RATIO_X8;
		adc_init.adc_ov_smp_ratio_en = ADC_OVER_SAMPLING_EN_STATUS_BIT0;
	}
	switch(pinNumber)
	{
		case 0:
			adc_init.adc_ch = ADC_CH2;
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_ANALOG);
			break;
		case 1:
			adc_init.adc_ch = ADC_CH3;
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_1, GPIO_MODE_ANALOG);
			break;
		case 4:
			adc_init.adc_ch = ADC_CH4;
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_ANALOG);
			break;
		case 19:
			adc_init.adc_ch = ADC_CH5;
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_3, GPIO_MODE_ANALOG);
			break;
		case 20:
			adc_init.adc_ch = ADC_CH6;
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_4, GPIO_MODE_ANALOG);
			break;
		case 21:
			adc_init.adc_ch = ADC_CH7;
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_5, GPIO_MODE_ANALOG);
			break;
		case 0xFF: break;
		default: return;
	}
	hal_adc_init(ADC_BASE, &adc_init);

	hal_adc_en(ADC_BASE, HAL_ENABLE);
}

int HAL_ADC_Read(int pinNumber)
{
	adc_ch_t ch;
	switch(pinNumber)
	{
		case 0:    ch = ADC_CH2; break;
		case 1:    ch = ADC_CH3; break;
		case 4:    ch = ADC_CH4; break;
		case 19:   ch = ADC_CH5; break;
		case 20:   ch = ADC_CH6; break;
		case 21:   ch = ADC_CH7; break;
		case 0xFF: ch = ADC_CH0; break;
		default: return 0;
	}
	uint16_t read_adc = 0;

	hal_adc_start_conv(ADC_BASE);

	while(hal_adc_get_conv_status(ADC_BASE, ch) == 0);

	read_adc = hal_adc_get_data(ADC_BASE, ch);

	hal_adc_clr_conv_status(ADC_BASE, ch);

	return read_adc;
}

void HAL_ADC_Deinit(int pinNumber)
{
	switch(pinNumber)
	{
		case 0:
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_DIGITAL);
			break;
		case 1:
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_1, GPIO_MODE_DIGITAL);
			break;
		case 4:
			hal_gpio_pin_mode_set(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_DIGITAL);
			break;
		case 19:
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_3, GPIO_MODE_DIGITAL);
			break;
		case 20:
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_4, GPIO_MODE_DIGITAL);
			break;
		case 21:
			hal_gpio_pin_mode_set(GPIOB_BASE, GPIO_PIN_5, GPIO_MODE_DIGITAL);
			break;
		default: return;
	}
}

#endif // PLATFORM_LN882H

#if PLATFORM_LN8825

#include "hal/hal_adc.h"

void OBK_HAL_ADC_Init(int pinNumber)
{
	// adc is already initialized for temp sensor
	//ADC_InitTypeDef adc_init_struct;
	//adc_init_struct.ADC_Autoff = FENABLE;
	//adc_init_struct.ADC_ContinuousConvMode = FDISABLE;
	//adc_init_struct.ADC_DataAlign = ADC_DataAlign_Right;
	//adc_init_struct.ADC_WaitMode = FDISABLE;
	//HAL_ADC_Init(ADC, &adc_init_struct);
	//HAL_ADC_PrescCfg(ADC, 42);
	HAL_SYSCON_GPIO_Digital_Analog_Select(pinNumber, GPIO_ANALOG_MOD);
}

int HAL_ADC_Read(int pinNumber)
{
	adc_chan_t ch;
	switch(pinNumber)
	{
		case 0:  ch = EXTL_ADC_CHAN_0; break;
		case 1:  ch = EXTL_ADC_CHAN_1; break;
		case 4:  ch = EXTL_ADC_CHAN_2; break;
		case 19: ch = EXTL_ADC_CHAN_3; break;
		case 20: ch = EXTL_ADC_CHAN_4; break;
		case 21: ch = EXTL_ADC_CHAN_5; break;
		default: return 0;
	}
	uint16_t read_adc = 0;

	HAL_ADC_SeqChanSelect_Cfg(ADC, ch);
	HAL_ADC_Cmd(ADC, FENABLE);
	HAL_ADC_SoftwareStartConvCmd(ADC);
	for(volatile uint32_t t = 0; t < 40 * 3; t++)
	{
		__NOP();
	}

	read_adc = HAL_ADC_GetConversionValue(ADC, ch);
	HAL_ADC_StopConvCmd(ADC);
	HAL_ADC_Cmd(ADC, FDISABLE);

	return read_adc;
}

void HAL_ADC_Deinit(int pinNumber)
{
	HAL_SYSCON_GPIO_Digital_Analog_Select(pinNumber, GPIO_DIGITAL_MOD);
}

#endif
