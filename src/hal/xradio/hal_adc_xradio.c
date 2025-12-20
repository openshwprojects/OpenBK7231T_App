#if PLATFORM_XRADIO

//#include "../hal_adc.h"
#include "../../logging/logging.h"
#include "hal_pinmap_xradio.h"

//HAL_ADC_Init is already definied in SDK.
void OBK_HAL_ADC_Init(int index)
{
	if(index >= g_numPins + 1)
		return;
	if(g_pins[index].adc < 0) return; // do not init adc if pin is invalid
	ADC_InitParam adc_init_param;
	adc_init_param.delay = 10;
	adc_init_param.freq = 500000;
	adc_init_param.mode = ADC_CONTI_CONV;
#undef HAL_ADC_Init
	HAL_Status status = HAL_ADC_Init(&adc_init_param);
	if(status != HAL_OK && status != HAL_BUSY)
	{
		ADDLOG_ERROR(LOG_FEATURE_PINS, "HAL_ADC_Init error");
		return;
	}
}

void HAL_ADC_Deinit(int index)
{
	if(index >= g_numPins + 1)
		return;
	if(g_pins[index].adc < 0) return;
	HAL_Status status = HAL_ADC_DeInit();
	if(status != HAL_OK)
	{
		ADDLOG_ERROR(LOG_FEATURE_PINS, "HAL_ADC_DeInit error");
		return;
	}
}

int HAL_ADC_Read(int index) 
{
	if(index >= g_numPins + 1)
		return 0;
	if(g_pins[index].adc < 0) return 0;
	uint32_t ad_value = 0;
	HAL_ADC_Conv_Polling(g_pins[index].adc, &ad_value, 100);
	return ad_value;
}

#endif // PLATFORM_XR809
