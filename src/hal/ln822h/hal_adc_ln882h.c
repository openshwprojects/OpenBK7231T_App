#ifdef PLATFORM_LN882H

#include "../hal_adc.h"
#include "hal/hal_adc.h"

void HAL_ADC_Init(int pinNumber) {
    adc_init_t_def adc_init;

    memset(&adc_init, 0, sizeof(adc_init_t_def));
    adc_init.adc_ch        = ADC_CH0;
    adc_init.adc_conv_mode = ADC_CONV_MODE_CONTINUE;
    adc_init.adc_presc     = 0xFF;                             
    hal_adc_init(ADC_BASE, &adc_init);
        
    hal_adc_en(ADC_BASE, HAL_ENABLE);
    
    hal_adc_start_conv(ADC_BASE);
}

int HAL_ADC_Read(int pinNumber) {
    adc_ch_t ch = ADC_CH0;
    uint16_t read_adc = 0;

    while(hal_adc_get_conv_status(ADC_BASE, ch) == 0); 
    
    read_adc = hal_adc_get_data(ADC_BASE, ch);
    
    hal_adc_clr_conv_status(ADC_BASE,ch);
    
    return read_adc;	
}


#endif // PLATFORM_LN882H

