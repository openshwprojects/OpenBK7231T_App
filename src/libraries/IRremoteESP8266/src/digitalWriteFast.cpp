extern "C" {
#include <gpio_pub.h>
}

#include "digitalWriteFast.h"


void digitalToggleFast(unsigned char P) {
	bk_gpio_output((GPIO_INDEX)P, !bk_gpio_input((GPIO_INDEX)P));
}

unsigned char digitalReadFast(unsigned char P) {
	return bk_gpio_input((GPIO_INDEX)P);
}

void digitalWriteFast(unsigned char P, unsigned char V) {
	//RAW_SetPinValue(P, V);
	//HAL_PIN_SetOutputValue(index, iVal);
	bk_gpio_output((GPIO_INDEX)P, V);
}

void pinModeFast(unsigned char P, unsigned char V) {
	if (V == INPUT_PULLUP) {
		bk_gpio_config_input_pup((GPIO_INDEX)P);
	}
	else if (V == INPUT_PULLDOWN) {
		bk_gpio_config_input_pdwn((GPIO_INDEX)P);
	}
	else if (V == INPUT) {
		bk_gpio_config_input((GPIO_INDEX)P);
	}
	else if (V == OUTPUT) {
		bk_gpio_config_output((GPIO_INDEX)P);
	}
}
