#if defined(PLATFORM_W800) || defined(PLATFORM_W600)

#include "../hal_adc.h"
#include "../../new_common.h"

#if defined(PLATFORM_W800)

//OpenW800\platform\drivers\gpio\wm_gpio_afsel.c
static int adcToGpio[] = {
	WM_IO_PA_01,
    WM_IO_PA_02,
    WM_IO_PA_03,
	WM_IO_PA_04
};
#else

//OpenW600\platform\drivers\wm_gpio_afsel.c
//More chip seem to support ADC but these 2 are ones usually accessible
static int adcToGpio[] = {
	WM_IO_PB_19,
	WM_IO_PB_20
};
#endif


static uint16_t adcData[1];

static uint8_t gpioToAdc(int gpio) {
	uint8_t i;
	for (i = 0; i < sizeof(adcToGpio); i++) {
		if (adcToGpio[i] == gpio)
			return i;
	}
	return -1;
}

void HAL_ADC_Init(int pinNumber) {
	wm_adc_config(gpioToAdc(pinNumber));
}

int HAL_ADC_Read(int pinNumber)
{
	return adc_get_inputVolt(gpioToAdc(pinNumber));
}

#endif
