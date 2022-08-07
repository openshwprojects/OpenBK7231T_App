#ifdef PLATFORM_XR809

#include "../hal_adc.h"

//HAL_ADC_Init is already definied in OpenXR809\src\driver\chip\hal_adc.c with a different signature.
//void HAL_ADC_Init(int pinNumber) {
//}

int HAL_ADC_Read(int pinNumber) {
	// TODO
	return 123;
}


#endif // PLATFORM_XR809

