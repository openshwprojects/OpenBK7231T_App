#ifdef PLATFORM_RDA5981

#include "../../new_common.h"
#include "../../logging/logging.h"
#include "../../new_cfg.h"
#include "../../new_pins.h"
#include "../hal_pins.h"
#include "hal_pinmap_rda5981.h"

rdaPinMapping_t g_pins[] = {
	{ "IO0 (B0)",		GPIO_PIN0,		NULL, NULL },
	{ "IO1 (B1/RX1)",	GPIO_PIN1,		NULL, NULL },
	{ "IO2 (B2/TX1)",	GPIO_PIN2,		NULL, NULL },
	{ "IO3 (B3)",		GPIO_PIN3,		NULL, NULL },
	{ "IO4 (B4)",		GPIO_PIN4,		NULL, NULL },
	{ "IO5 (B5)",		GPIO_PIN5,		NULL, NULL },
	{ "IO6 (B6)",		GPIO_PIN6,		NULL, NULL },
	{ "IO7 (B7)",		GPIO_PIN7,		NULL, NULL },
	{ "IO8 (B8)",		GPIO_PIN8,		NULL, NULL },
	{ "IO9 (B9)",		GPIO_PIN9,		NULL, NULL },
	{ "IO10 (A8)",		GPIO_PIN10,		NULL, NULL },
	{ "IO11 (A9)",		GPIO_PIN11,		NULL, NULL },
	{ "IO12 (C0)",		GPIO_PIN12,		NULL, NULL },
	{ "IO13 (C1)",		GPIO_PIN13,		NULL, NULL },
	{ "IO14 (C2)",		GPIO_PIN14,		NULL, NULL },
	{ "IO15 (C3)",		GPIO_PIN15,		NULL, NULL },
	{ "IO16 (C4)",		GPIO_PIN16,		NULL, NULL },
	{ "IO17 (C5)",		GPIO_PIN17,		NULL, NULL },
	{ "IO18 (C6)",		GPIO_PIN18,		NULL, NULL },
	{ "IO19 (C7)",		GPIO_PIN19,		NULL, NULL },
	{ "IO20 (C8)",		GPIO_PIN20,		NULL, NULL },
	{ "IO21 (C9)",		GPIO_PIN21,		NULL, NULL },
	{ "IO22 (D0)",		GPIO_PIN22,		NULL, NULL },
	{ "IO23 (D1)",		GPIO_PIN23,		NULL, NULL },
	{ "IO24 (D2/RX1)",	GPIO_PIN24,		NULL, NULL },
	{ "IO25 (D3/TX1)",	GPIO_PIN25,		NULL, NULL },
	{ "IO26 (A0/RX0)",	GPIO_PIN26,		NULL, NULL },
	{ "IO27 (A1/TX0)",	GPIO_PIN27,		NULL, NULL },
	//{ "IO14 (A2)",		GPIO_PIN14A,	NULL, NULL },
	//{ "IO15 (A3)",		GPIO_PIN15A,	NULL, NULL },
	//{ "IO16 (A4)",		GPIO_PIN16A,	NULL, NULL },
	//{ "IO17 (A5)",		GPIO_PIN17A,	NULL, NULL },
	//{ "IO18 (A6)",		GPIO_PIN18A,	NULL, NULL },
	//{ "IO19 (A7)",		GPIO_PIN19A,	NULL, NULL },
	{ "VBAT",			ADC_PIN2,		NULL, NULL },
};

int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]) - 1;

int HAL_PIN_CanThisPinBePWM(int index)
{
	if(index >= g_numPins)
		return 0;
	rdaPinMapping_t* pin = g_pins + index;
	switch(pin->pin)
	{
		case PA_0:
		case PA_1:
		case PB_0:
		case PB_1:
		case PB_2:
		case PB_3:
		case PB_8:
		case PD_0:
		case PD_1:
		case PD_2:
		case PD_3:
			return 1;
		default: return 0;
	}
}

int PIN_GetPWMIndexForPinIndex(int index)
{
	rdaPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins)
		return -1;
	if(pin->pwm != NULL) return pin->pwm->channel;
	else return HAL_PIN_CanThisPinBePWM(index);
}

const char* HAL_PIN_GetPinNameAlias(int index)
{
	if(index >= g_numPins + 1)
		return "error";
	return g_pins[index].name;
}

void RDA_GPIO_Init(rdaPinMapping_t* pin)
{
	if(pin->gpio != NULL || pin->irq != NULL)
	{
		return;
	}
	pin->gpio = os_malloc(sizeof(gpio_t));
	memset(pin->gpio, 0, sizeof(gpio_t));
	gpio_init(pin->gpio, pin->pin);
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	rdaPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins || pin->gpio == NULL)
		return;
	gpio_write(pin->gpio, iVal ? 1 : 0);
}

int HAL_PIN_ReadDigitalInput(int index)
{
	rdaPinMapping_t* pin = g_pins + index;
	if(index >= g_numPins || pin->gpio == NULL)
		return 0;
	return gpio_read(pin->gpio);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if(index >= g_numPins)
		return;
	rdaPinMapping_t* pin = g_pins + index;
	RDA_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullUp);
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	if(index >= g_numPins)
		return;
	rdaPinMapping_t* pin = g_pins + index;
	RDA_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullDown);
}

void HAL_PIN_Setup_Input(int index)
{
	if(index >= g_numPins)
		return;
	rdaPinMapping_t* pin = g_pins + index;
	RDA_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_INPUT);
	gpio_mode(pin->gpio, PullNone);
}

void HAL_PIN_Setup_Output(int index)
{
	if(index >= g_numPins)
		return;
	rdaPinMapping_t* pin = g_pins + index;
	RDA_GPIO_Init(pin);
	if(pin->gpio == NULL)
		return;
	gpio_dir(pin->gpio, PIN_OUTPUT);
	gpio_mode(pin->gpio, PullUp);
}

void HAL_PIN_PWM_Stop(int index)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rdaPinMapping_t* pin = g_pins + index;
	if(pin->pwm == NULL) return;
	//pwmout_stop(pin->pwm);
	pwmout_free(pin->pwm);
	os_free(pin->pwm);
	pin->pwm = NULL;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rdaPinMapping_t* pin = g_pins + index;
	if(pin->pwm != NULL)
	{
		pwmout_period_us(pin->pwm, 1000000 / freq);
		return;
	}
	if(pin->gpio != NULL)
	{
		//gpio_deinit(pin->gpio);
		os_free(pin->gpio);
		pin->gpio = NULL;
	}
	pin->pwm = os_malloc(sizeof(pwmout_t));
	memset(pin->pwm, 0, sizeof(pwmout_t));
	pwmout_init(pin->pwm, pin->pin);
	pwmout_period_us(pin->pwm, 1000000 / freq);
}

void HAL_PIN_PWM_Update(int index, float value)
{
	if(index >= g_numPins || !HAL_PIN_CanThisPinBePWM(index))
		return;
	rdaPinMapping_t* pin = g_pins + index;
	if(pin->pwm == NULL) return;
	pwmout_write(pin->pwm, value / 100);
}

unsigned int HAL_GetGPIOPin(int index)
{
	return index;
}

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#include "gpio_irq_api.h"

void RDA_Interrupt(uint32_t obkPinNum, gpio_irq_event event)
{
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	rdaPinMapping_t *rda_irq = g_pins + pinIndex;
	rda_irq->irq = os_malloc(sizeof(gpio_irq_t));
	memset(rda_irq->irq, 0, sizeof(gpio_irq_t));

	int rda_mode;
	if (mode == INTERRUPT_RISING) {
		rda_mode = IRQ_RISE;
	}
	else {
		rda_mode = IRQ_FALL;
	}
	gpio_irq_init(rda_irq->irq, rda_irq->pin, RDA_Interrupt, pinIndex);
	gpio_irq_set(rda_irq->irq, rda_mode, 1);
	gpio_irq_enable(rda_irq->irq);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	rdaPinMapping_t *rda_irq = g_pins + pinIndex;
	gpio_irq_free(rda_irq->irq);
	os_free(rda_irq->irq);
	rda_irq->irq = NULL;
	g_handlers[pinIndex] = 0;
}

#endif // PLATFORM_RDA5981
