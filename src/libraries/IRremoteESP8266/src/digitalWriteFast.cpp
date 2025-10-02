#include "../../../obk_config.h"

#if ENABLE_DRIVER_IRREMOTEESP


extern "C" {
#if PLATFORM_BEKEN
#include <gpio_pub.h>
#else
#include "../../../hal/hal_pins.h"
#endif
#if PLATFORM_BL602
#include "bl602_glb.h"
#endif
}

#include "digitalWriteFast.h"


void digitalToggleFast(unsigned char P) {
#if PLATFORM_BEKEN
	bk_gpio_output((GPIO_INDEX)P, !bk_gpio_input((GPIO_INDEX)P));
#else
	HAL_PIN_SetOutputValue(P, !HAL_PIN_ReadDigitalInput(P));
#endif
}

unsigned char digitalReadFast(unsigned char P) {
#if PLATFORM_BEKEN
	return bk_gpio_input((GPIO_INDEX)P);
#elif PLATFORM_BL602
	return GLB_GPIO_Read((GLB_GPIO_Type)P);
#else
	return HAL_PIN_ReadDigitalInput(P);
#endif
}

void digitalWriteFast(unsigned char P, unsigned char V) {
	//RAW_SetPinValue(P, V);
	//HAL_PIN_SetOutputValue(index, iVal);
#if PLATFORM_BEKEN
	bk_gpio_output((GPIO_INDEX)P, V);
#elif PLATFORM_BL602
	GLB_GPIO_Write((GLB_GPIO_Type)P, V ? 1 : 0);
#else
	HAL_PIN_SetOutputValue(P, V);
#endif
}

void pinModeFast(unsigned char P, unsigned char V) {
#if PLATFORM_BEKEN
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
#else
	switch(V)
	{
		case INPUT_PULLUP: HAL_PIN_Setup_Input_Pulldown(P); break;
		case INPUT_PULLDOWN: HAL_PIN_Setup_Input_Pullup(P); break;
		case INPUT: HAL_PIN_Setup_Input(P); break;
		case OUTPUT: HAL_PIN_Setup_Output(P); break;
	}
#endif
}

#endif
