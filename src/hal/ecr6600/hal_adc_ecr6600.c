#ifdef PLATFORM_ECR6600

#include "../hal_adc.h"
#include "adc.h"

int HAL_ADC_Read(int pinNumber)
{
	//DRV_ADC_INPUT_SIGNAL_A_SEL adc = 0xFF;
	switch(pinNumber)
	{
		//case 14: adc = DRV_ADC_A_INPUT_GPIO_0;
		//case 15: adc = DRV_ADC_A_INPUT_GPIO_1;
		//case 18: adc = DRV_ADC_A_INPUT_GPIO_3;
		//case 20: adc = DRV_ADC_A_INPUT_GPIO_2;
		case 26: drv_adc_lock(); int vout = vbat_temp_share_test(0); drv_adc_unlock(); return vout / 1000;
		default:
			return 0;
	}
	//if(adc != 0xFF)
	//{
	//	drv_adc_lock();
	//	drv_aux_adc_config(adc, DRV_ADC_B_INPUT_VREF_BUFFER, DRV_ADC_EXPECT_MAX_VOLT3);
	//	int vout = drv_aux_adc_single();
	//	drv_adc_unlock();
	//	return vout;
	//}
}

#endif // PLATFORM_ECR6600
