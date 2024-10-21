


void HAL_ADC_Init(int pinNumber);
int HAL_ADC_Read(int pinNumber);
#if defined(PLATFORM_W800)  || defined(PLATFORM_W600)
float HAL_ADC_Temp(void);
#endif
