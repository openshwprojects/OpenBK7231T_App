#include "../hal_adc.h"
#include "../../new_common.h"


static int adcToGpio[] = {
	-1,		// ADC0 - VBAT
	4, //GPIO4,	// ADC1
	5, //GPIO5,	// ADC2
	23,//GPIO23, // ADC3
	2,//GPIO2,	// ADC4
	3,//GPIO3,	// ADC5
	12,//GPIO12, // ADC6
	13,//GPIO13, // ADC7
};
static int c_adcToGpio = sizeof(adcToGpio)/sizeof(adcToGpio[0]);

static uint16_t adcData[1];

static uint8_t gpioToAdc(int gpio) {
	uint8_t i;
	for ( i = 0; i < sizeof(adcToGpio); i++) {
		if (adcToGpio[i] == gpio)
			return i;
	}
	return -1;
}

void HAL_ADC_Init(int pinNumber) {

}




int HAL_ADC_Read(int pinNumber)
{

    return -3;
}

