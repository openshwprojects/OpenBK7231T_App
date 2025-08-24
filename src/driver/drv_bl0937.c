// HLW8012 aka BL0937
#include "drv_bl0937.h"
#include "../obk_config.h"



#include "../hal/hal_pins.h"
#include "../new_pins.h"

OBKInterruptHandler g_handlers[PLATFORM_GPIO_MAX];
OBKInterruptType g_modes[PLATFORM_GPIO_MAX];

#if PLATFORM_BEKEN

#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"

void Beken_Interrupt(unsigned char pinNum) {
	if (g_handlers[pinNum]) {
		g_handlers[pinNum](pinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;
	int bk_mode;
	if (mode == INTERRUPT_RISING) {
		bk_mode = IRQ_TRIGGER_RISING_EDGE;
	}
	else {
		bk_mode = IRQ_TRIGGER_FALLING_EDGE;
	}
	gpio_int_enable(pinIndex, bk_mode, Beken_Interrupt);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	gpio_int_disable(pinIndex);
	g_handlers[pinIndex] = 0;
}

#elif PLATFORM_W600 || PLATFORM_W800

void W600_Interrupt(void* context) {
	int obkPinNum = (int)(intptr_t)context;
	int w600Pin = HAL_GetGPIOPin(obkPinNum);
	tls_clr_gpio_irq_status(w600Pin);
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;
	int w600Pin = HAL_GetGPIOPin(pinIndex); 
	tls_gpio_isr_register(w600Pin, W600_Interrupt, (void*)(intptr_t)pinIndex);
	int w_mode;
	if (mode == INTERRUPT_RISING) {
		w_mode = WM_GPIO_IRQ_TRIG_RISING_EDGE;
	}
	else {
		w_mode = WM_GPIO_IRQ_TRIG_FALLING_EDGE;
	}
	tls_gpio_irq_enable(w600Pin, w_mode);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	int w600Pin = HAL_GetGPIOPin(pinIndex);
	tls_gpio_irq_disable(w600Pin);
	g_handlers[pinIndex] = 0;
}



#elif PLATFORM_BL602

#include "hal_gpio.h"
#include "bl_gpio.h"

void BL602_Interrupt(void* context) {
	int obkPinNum = (int)context;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
	bl_gpio_intmask(obkPinNum, 0);
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;
	int bl_mode;
	if (mode == INTERRUPT_RISING) {
		bl_mode = GPIO_INT_TRIG_POS_PULSE;
	}
	else {
		bl_mode = GPIO_INT_TRIG_NEG_PULSE;
	}
	hal_gpio_register_handler(BL602_Interrupt, pinIndex,
		GPIO_INT_CONTROL_ASYNC, bl_mode, (void*)pinIndex);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	g_handlers[pinIndex] = 0;
}

#elif PLATFORM_LN882H

#include "../../sdk/OpenLN882H/mcu/driver_ln882h/hal/hal_common.h"
#include "../../sdk/OpenLN882H/mcu/driver_ln882h/hal/hal_gpio.h"

uint32_t GetBaseForPin(int pinIndex)
{
	return pinIndex < 16 ? GPIOA_BASE : GPIOB_BASE;
}

int GetIRQForPin(int pinIndex)
{
	return pinIndex < 16 ? GPIOA_IRQn : GPIOB_IRQn;
}

uint16_t GetGPIOForPin(int pinIndex)
{
	return (uint16_t)1 << (uint16_t)(pinIndex % 16);
}
void Shared_Handler() {
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (g_handlers[i]) {
			uint32_t base = GetBaseForPin(i);
			uint16_t gpio_pin = GetGPIOForPin(i);
			if (hal_gpio_pin_get_it_flag(base, gpio_pin) == HAL_SET) {
				hal_gpio_pin_clr_it_flag(base, gpio_pin);
				g_handlers[i](i);
			}
		}
	}
}
void GPIOA_IRQHandler()
{
	Shared_Handler();
}

void GPIOB_IRQHandler()
{
	Shared_Handler();
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	int ln_mode;
	if (mode == INTERRUPT_RISING) {
		ln_mode = GPIO_INT_RISING;
	}
	else {
		ln_mode = GPIO_INT_FALLING;
	}
	hal_gpio_pin_it_cfg(GetBaseForPin(pinIndex), GetGPIOForPin(pinIndex), ln_mode);
	hal_gpio_pin_it_en(GetBaseForPin(pinIndex), GetGPIOForPin(pinIndex), HAL_ENABLE);
	NVIC_SetPriority(GetIRQForPin(pinIndex), 1);
	NVIC_EnableIRQ(GetIRQForPin(pinIndex));

}

void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	g_handlers[pinIndex] = 0;
}

#elif PLATFORM_REALTEK

#include "gpio_irq_api.h"
#include "../hal/realtek/hal_pinmap_realtek.h"


void Realtek_Interrupt(uint32_t obkPinNum, gpio_irq_event event)
{
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	rtlPinMapping_t *rtl_cf = g_pins + pinIndex;
#if PLATFORM_RTL87X0C
	if (rtl_cf->gpio != NULL)
	{
		hal_pinmux_unregister(rtl_cf->pin, PID_GPIO);
		os_free(rtl_cf->gpio);
		rtl_cf->gpio = NULL;
	}
#endif
	rtl_cf->irq = os_malloc(sizeof(gpio_irq_t));
	memset(rtl_cf->irq, 0, sizeof(gpio_irq_t));

	int rtl_mode;
	if (mode == INTERRUPT_RISING) {
		rtl_mode = IRQ_RISE;
	}
	else {
		rtl_mode = IRQ_FALL;
	}
	gpio_irq_init(rtl_cf->irq, rtl_cf->pin, Realtek_Interrupt, pinIndex);
	gpio_irq_set(rtl_cf->irq, rtl_mode, 1);
	gpio_irq_enable(rtl_cf->irq);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	rtlPinMapping_t *rtl_cf = g_pins + pinIndex;
	gpio_irq_free(rtl_cf->irq);
	os_free(rtl_cf->irq);
	rtl_cf->irq = NULL;
	g_handlers[pinIndex] = 0;
}


#elif PLATFORM_ECR6600

#include "gpio.h"

void ECR6600_Interrupt(unsigned char obkPinNum) {
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	T_GPIO_ISR_CALLBACK cf1isr;
	cf1isr.gpio_callback = (&ECR6600_Interrupt);
	cf1isr.gpio_data = pinIndex;

	int ecr_mode;
	if (mode == INTERRUPT_RISING) {
		ecr_mode = DRV_GPIO_ARG_INTR_MODE_P_EDGE;
	}
	else {
		ecr_mode = DRV_GPIO_ARG_INTR_MODE_N_EDGE;
	}
	drv_gpio_ioctrl(pinIndex, DRV_GPIO_CTRL_INTR_MODE, ecr_mode);
	drv_gpio_ioctrl(pinIndex, DRV_GPIO_CTRL_REGISTER_ISR, (int)&cf1isr);
	drv_gpio_ioctrl(pinIndex, DRV_GPIO_CTRL_INTR_ENABLE, 0);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}

	drv_gpio_ioctrl(pinIndex, DRV_GPIO_CTRL_INTR_DISABLE, 0);
	g_handlers[pinIndex] = 0;
}


#elif PLATFORM_XRADIO

#include "../hal/xradio/hal_pinmap_xradio.h"
extern void HAL_XR_ConfigurePin(GPIO_Port port, GPIO_Pin pin, GPIO_WorkMode mode, GPIO_PullType pull);


void XRadio_Interrupt(void* context) {
	int obkPinNum = (int)context;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	xrpin_t *xr_cf = g_pins + pinIndex;
	HAL_XR_ConfigurePin(xr_cf->port, xr_cf->pin, GPIOx_Pn_F6_EINT, GPIO_PULL_UP);
	GPIO_IrqParam cfparam;
	int xr_mode;
	if (mode == INTERRUPT_RISING) {
		xr_mode = GPIO_IRQ_EVT_RISING_EDGE;
	}
	else {
		xr_mode = GPIO_IRQ_EVT_FALLING_EDGE;
	}
	cfparam.event = xr_mode;
	cfparam.callback = XRadio_Interrupt;
	cfparam.arg = (void*)pinIndex;
	HAL_GPIO_EnableIRQ(xr_cf->port, xr_cf->pin, &cfparam);

}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}
	xrpin_t* xr_cf;
	xr_cf = g_pins + pinIndex;
	HAL_GPIO_DeInit(xr_cf->port, xr_cf->pin);
	HAL_GPIO_DisableIRQ(xr_cf->port, xr_cf->pin);
	g_handlers[pinIndex] = 0;
}

#elif PLATFORM_ESP8266 || PLATFORM_ESPIDF

#include "../hal/espidf/hal_pinmap_espidf.h"


void ESP_Interrupt(void* context) {
	int obkPinNum = (int)context;
	if (g_handlers[obkPinNum]) {
		g_handlers[obkPinNum](obkPinNum);
	}
}

bool b_esp_ready = false;
void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {
	g_handlers[pinIndex] = function;

	if (b_esp_ready == false) {
		gpio_install_isr_service(0);
		b_esp_ready = true;
	}
	espPinMapping_t* esp_cf = g_pins + pinIndex;
	int esp_mode;
	if (mode == INTERRUPT_RISING) {
		esp_mode = GPIO_INTR_POSEDGE;
	}
	else {
		esp_mode = GPIO_INTR_NEGEDGE;
	}
	ESP_ConfigurePin(esp_cf->pin, GPIO_MODE_INPUT, true, false, esp_mode);
	gpio_isr_handler_add(esp_cf->pin, ESP_Interrupt, (void*)pinIndex);
}
void HAL_DetachInterrupt(int pinIndex) {
	if (g_handlers[pinIndex] == 0) {
		return; // already removed;
	}

	espPinMapping_t* esp_cf;
	esp_cf = g_pins + pinIndex;
	gpio_isr_handler_remove(esp_cf->pin);
	///gpio_uninstall_isr_service();
	g_handlers[pinIndex] = 0;
}


#else
void HAL_AttachInterrupt(int pinIndex, OBKInterruptType mode, OBKInterruptHandler function) {

}
void HAL_DetachInterrupt(int pinIndex) {

}


#endif



#if ENABLE_DRIVER_BL0937

//dummy
#include <math.h>

#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_uart.h"





#define DEFAULT_VOLTAGE_CAL 0.13253012048f
#define DEFAULT_CURRENT_CAL 0.0118577075f
#define DEFAULT_POWER_CAL 1.5f

// Those can be set by Web page pins configurator
// The below are default values for Mycket smart socket
int GPIO_HLW_SEL = 24; // pwm4
bool g_invertSEL = false;
int GPIO_HLW_CF = 7;
int GPIO_HLW_CF1 = 8;

bool g_sel = true;
uint32_t res_v = 0;
uint32_t res_c = 0;
uint32_t res_p = 0;
float BL0937_PMAX = 3680.0f;
float last_p = 0.0f;

volatile uint32_t g_vc_pulses = 0;
volatile uint32_t g_p_pulses = 0;
static portTickType pulseStamp;


void HlwCf1Interrupt(int pinNum)
{
	g_vc_pulses++;
}
void HlwCfInterrupt(int pinNum)
{
	g_p_pulses++;
}

commandResult_t BL0937_PowerMax(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	float maxPower;

	if(args == 0 || *args == 0)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "This command needs one argument");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	maxPower = atof(args);
	if((maxPower > 200.0) && (maxPower < 7200.0f))
	{
		BL0937_PMAX = maxPower;
		// UPDATE: now they are automatically saved
		CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);
		{
			char dbg[128];
			snprintf(dbg, sizeof(dbg), "PowerMax: set max to %f\n", BL0937_PMAX);
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
		}
	}
	return CMD_RES_OK;
}

void BL0937_Shutdown_Pins()
{
	HAL_DetachInterrupt(GPIO_HLW_CF);
	HAL_DetachInterrupt(GPIO_HLW_CF1);
}

void BL0937_Init_Pins()
{
	int tmp;

	// if not found, this will return the already set value
	tmp = PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, -1);
	if(tmp != -1)
	{
		g_invertSEL = true;
		GPIO_HLW_SEL = tmp;
	}
	else
	{
		g_invertSEL = false;
		GPIO_HLW_SEL = PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL);
	}
	GPIO_HLW_CF = PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF);
	GPIO_HLW_CF1 = PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1);

	BL0937_PMAX = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_POWER_MAX, BL0937_PMAX);

	HAL_PIN_Setup_Output(GPIO_HLW_SEL);
	HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF1);
	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF);

	HAL_AttachInterrupt(GPIO_HLW_CF, INTERRUPT_STUB, HlwCfInterrupt);
	HAL_AttachInterrupt(GPIO_HLW_CF1, INTERRUPT_STUB, HlwCf1Interrupt);

	g_vc_pulses = 0;
	g_p_pulses = 0;
	pulseStamp = xTaskGetTickCount();
}

void BL0937_Init(void)
{
	BL_Shared_Init();

	PwrCal_Init(PWR_CAL_MULTIPLY, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
		DEFAULT_POWER_CAL);

	//cmddetail:{"name":"PowerMax","args":"[MaxPowerInW]",
	//cmddetail:"descr":"Sets the maximum power limit for BL measurement used to filter incorrect values",
	//cmddetail:"fn":"NULL);","file":"driver/drv_bl0937.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("PowerMax", BL0937_PowerMax, NULL);

	BL0937_Init_Pins();
}

void BL0937_RunEverySecond(void)
{
	float final_v;
	float final_c;
	float final_p;
	bool bNeedRestart;
	portTickType ticksElapsed;
	portTickType xPassedTicks;

	bNeedRestart = false;
	if(g_invertSEL)
	{
		if(GPIO_HLW_SEL != PIN_FindPinIndexForRole(IOR_BL0937_SEL_n, GPIO_HLW_SEL))
		{
			bNeedRestart = true;
		}
	}
	else
	{
		if(GPIO_HLW_SEL != PIN_FindPinIndexForRole(IOR_BL0937_SEL, GPIO_HLW_SEL))
		{
			bNeedRestart = true;
		}
	}
	if(GPIO_HLW_CF != PIN_FindPinIndexForRole(IOR_BL0937_CF, GPIO_HLW_CF))
	{
		bNeedRestart = true;
	}
	if(GPIO_HLW_CF1 != PIN_FindPinIndexForRole(IOR_BL0937_CF1, GPIO_HLW_CF1))
	{
		bNeedRestart = true;
	}


#if PLATFORM_BEKEN
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
#else

#endif

#if 1
	if(bNeedRestart)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "BL0937 pins have changed, will reset the interrupts");

		BL0937_Shutdown_Pins();
		BL0937_Init_Pins();
#if PLATFORM_BEKEN
		GLOBAL_INT_RESTORE();
#else

#endif
		return;
	}


#endif
	if(g_sel)
	{
		if(g_invertSEL)
		{
			res_c = g_vc_pulses;
		}
		else
		{
			res_v = g_vc_pulses;
		}
		g_sel = false;
	}
	else
	{
		if(g_invertSEL)
		{
			res_v = g_vc_pulses;
		}
		else
		{
			res_c = g_vc_pulses;
		}
		g_sel = true;
	}
	HAL_PIN_SetOutputValue(GPIO_HLW_SEL, g_sel);
	g_vc_pulses = 0;

	res_p = g_p_pulses;
	g_p_pulses = 0;
#if PLATFORM_BEKEN
	GLOBAL_INT_RESTORE();
#else

#endif
	xPassedTicks = xTaskGetTickCount();
	ticksElapsed = (xPassedTicks - pulseStamp);
	pulseStamp = xPassedTicks;
	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, res_p);

	PwrCal_Scale(res_v, res_c, res_p, &final_v, &final_c, &final_p);

	final_v *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_v /= (float)ticksElapsed;

	final_c *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_c /= (float)ticksElapsed;

	final_p *= (1000.0f / (float)portTICK_PERIOD_MS);
	final_p /= (float)ticksElapsed;

	/* patch to limit max power reading, filter random reading errors */
	if(final_p > BL0937_PMAX)
	{
		/* MAX value breach, use last value */
		{
			char dbg[128];
			snprintf(dbg, sizeof(dbg), "Power reading: %f exceeded MAX limit: %f, Last: %f\n", final_p, BL0937_PMAX, last_p);
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
		}
		final_p = last_p;
	}
	else
	{
		/* Valid value save for next time */
		last_p = final_p;
	}
#if 0
	{
		char dbg[128];
		snprintf(dbg, sizeof(dbg), "Voltage %f, current %f, power %f\n", final_v, final_c, final_p);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, dbg);
	}
#endif
	BL_ProcessUpdate(final_v, final_c, final_p, NAN, NAN);
}

// close ENABLE_DRIVER_BL0937
#endif

