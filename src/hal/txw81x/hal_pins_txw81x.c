#if PLATFORM_TXW81X

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "hal_pinmap_txw81x.h"

txwpin_t g_pins[] = {
	{ "PA00",	PA_0, GPIO_IRQ_EVENT_NONE },
	{ "PA01",	PA_1, GPIO_IRQ_EVENT_NONE },
	{ "PA02",	PA_2, GPIO_IRQ_EVENT_NONE },
	{ "PA03",	PA_3, GPIO_IRQ_EVENT_NONE },
	{ "PA04",	PA_4, GPIO_IRQ_EVENT_NONE },
	{ "PA05",	PA_5, GPIO_IRQ_EVENT_NONE },
	{ "PA06",	PA_6, GPIO_IRQ_EVENT_NONE },
	{ "PA07",	PA_7, GPIO_IRQ_EVENT_NONE },
	{ "PA08",	PA_8, GPIO_IRQ_EVENT_NONE },
	{ "PA09",	PA_9, GPIO_IRQ_EVENT_NONE },
	{ "PA10",	PA_10, GPIO_IRQ_EVENT_NONE },
	{ "PA11",	PA_11, GPIO_IRQ_EVENT_NONE },
	{ "PA12",	PA_12, GPIO_IRQ_EVENT_NONE },
	{ "PA13",	PA_13, GPIO_IRQ_EVENT_NONE },
	{ "PA14",	PA_14, GPIO_IRQ_EVENT_NONE },
	{ "PA15",	PA_15, GPIO_IRQ_EVENT_NONE },
	{ "PB00",	PB_0, GPIO_IRQ_EVENT_NONE },
	{ "PB01",	PB_1, GPIO_IRQ_EVENT_NONE },
	{ "PB02",	PB_2, GPIO_IRQ_EVENT_NONE },
	{ "PB03",	PB_3, GPIO_IRQ_EVENT_NONE },
	{ "PB04",	PB_4, GPIO_IRQ_EVENT_NONE },
	{ "PB05",	PB_5, GPIO_IRQ_EVENT_NONE },
	{ "PB06",	PB_6, GPIO_IRQ_EVENT_NONE },
	{ "PB07",	PB_7, GPIO_IRQ_EVENT_NONE },
	{ "PB08",	PB_8, GPIO_IRQ_EVENT_NONE },
	{ "PB09",	PB_9, GPIO_IRQ_EVENT_NONE },
	{ "PB10",	PB_10, GPIO_IRQ_EVENT_NONE },
	{ "PB11",	PB_11, GPIO_IRQ_EVENT_NONE },
	{ "PB12",	PB_12, GPIO_IRQ_EVENT_NONE },
	{ "PB13",	PB_13, GPIO_IRQ_EVENT_NONE },
	{ "PB14",	PB_14, GPIO_IRQ_EVENT_NONE },
	{ "PB15",	PB_15, GPIO_IRQ_EVENT_NONE },
	{ "PC00",	PC_0, GPIO_IRQ_EVENT_NONE },
	{ "PC01",	PC_1, GPIO_IRQ_EVENT_NONE },
	{ "PC02",	PC_2, GPIO_IRQ_EVENT_NONE },
	{ "PC03",	PC_3, GPIO_IRQ_EVENT_NONE },
	{ "PC04",	PC_4, GPIO_IRQ_EVENT_NONE },
	{ "PC05",	PC_5, GPIO_IRQ_EVENT_NONE },
	{ "PC06",	PC_6, GPIO_IRQ_EVENT_NONE },
	{ "PC07",	PC_7, GPIO_IRQ_EVENT_NONE },
	{ "PC08",	PC_8, GPIO_IRQ_EVENT_NONE },
	{ "PC09",	PC_9, GPIO_IRQ_EVENT_NONE },
	{ "PC10",	PC_10, GPIO_IRQ_EVENT_NONE },
	{ "PC11",	PC_11, GPIO_IRQ_EVENT_NONE },
	{ "PC12",	PC_12, GPIO_IRQ_EVENT_NONE },
	{ "PC13",	PC_13, GPIO_IRQ_EVENT_NONE },
	{ "PC14",	PC_14, GPIO_IRQ_EVENT_NONE },
	{ "PC15",	PC_15, GPIO_IRQ_EVENT_NONE },
	{ "PE00",	PE_0, GPIO_IRQ_EVENT_NONE },
	{ "PE01",	PE_1, GPIO_IRQ_EVENT_NONE },
	{ "PE02",	PE_2, GPIO_IRQ_EVENT_NONE },
	{ "PE03",	PE_3, GPIO_IRQ_EVENT_NONE },
	{ "PE04",	PE_4, GPIO_IRQ_EVENT_NONE },
	{ "PE05",	PE_5, GPIO_IRQ_EVENT_NONE },
	{ "PE06",	PE_6, GPIO_IRQ_EVENT_NONE },
	{ "PE07",	PE_7, GPIO_IRQ_EVENT_NONE },
	{ "PE08",	PE_8, GPIO_IRQ_EVENT_NONE },
	{ "PE09",	PE_9, GPIO_IRQ_EVENT_NONE },
	{ "PE10",	PE_10, GPIO_IRQ_EVENT_NONE },
	{ "PE11",	PE_11, GPIO_IRQ_EVENT_NONE },
	{ "PE12",	PE_12, GPIO_IRQ_EVENT_NONE },
	{ "PE13",	PE_13, GPIO_IRQ_EVENT_NONE },
	{ "PE14",	PE_14, GPIO_IRQ_EVENT_NONE },
	{ "PE15",	PE_15, GPIO_IRQ_EVENT_NONE },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins)
		return "error";
	return g_pins[index].name;
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if(index >= g_numPins)
		return;
	txwpin_t* pin = g_pins + index;
	gpio_set_val(pin->pin, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if(index >= g_numPins)
		return 0;
	txwpin_t* pin = g_pins + index;
	return gpio_get_val(pin->pin);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	txwpin_t* pin = g_pins + index;
	gpio_set_dir(pin->pin, GPIO_DIR_INPUT);
	gpio_set_mode(pin->pin, GPIO_PULL_UP, GPIO_PULL_LEVEL_4_7K);
	gpio_iomap_input(pin->pin, GPIO_IOMAP_INPUT);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	txwpin_t* pin = g_pins + index;
	gpio_set_dir(pin->pin, GPIO_DIR_INPUT);
	gpio_set_mode(pin->pin, GPIO_PULL_DOWN, GPIO_PULL_LEVEL_4_7K);
	gpio_iomap_input(pin->pin, GPIO_IOMAP_INPUT);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	txwpin_t* pin = g_pins + index;
	gpio_set_dir(pin->pin, GPIO_DIR_INPUT);
	gpio_set_mode(pin->pin, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
	gpio_iomap_input(pin->pin, GPIO_IOMAP_INPUT);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	txwpin_t* pin = g_pins + index;
	gpio_set_dir(pin->pin, GPIO_DIR_OUTPUT);
	gpio_set_mode(pin->pin, GPIO_PULL_UP, GPIO_PULL_LEVEL_4_7K);
	gpio_iomap_output(pin->pin, GPIO_IOMAP_OUTPUT);
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

void TXW_Interrupt(int32 data, enum gpio_irq_event event)
{
	if(g_handlers[data])
	{
		g_handlers[data](data);
	}
}

void HAL_AttachInterrupt(int index, OBKInterruptType mode, OBKInterruptHandler function)
{
	txwpin_t* pin = g_pins + index;
	if(index >= g_numPins || pin->irq_mode != GPIO_IRQ_EVENT_NONE)
		return;
	g_handlers[index] = function;

	enum gpio_irq_event txw_mode;
	switch(mode)
	{
		case INTERRUPT_RISING: txw_mode = GPIO_IRQ_EVENT_RISE; break;
		case INTERRUPT_FALLING: txw_mode = GPIO_IRQ_EVENT_FALL; break;
		case INTERRUPT_CHANGE: txw_mode = GPIO_IRQ_EVENT_ALL; break;
		default: txw_mode = GPIO_IRQ_EVENT_FALL; break;
	}
	gpio_request_pin_irq(pin->pin, TXW_Interrupt, pin->pin, txw_mode);

}
void HAL_DetachInterrupt(int index)
{
	if(g_handlers[index] == 0)
	{
		return; // already removed;
	}
	txwpin_t* pin = g_pins + index;
	gpio_release_pin_irq(pin->pin, pin->irq_mode);
	pin->irq_mode = GPIO_IRQ_EVENT_NONE;
	g_handlers[index] = 0;
}

#endif
