#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_bl0942.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

#include "BkDriverTimer.h"
#include "BkDriverGpio.h"
#include "sys_timer.h"
#include "gw_intf.h"

#define ELE_HW_TIME 1
#define HW_TIMER_ID 0



int GPIO_NRG_SEL = 24; // pwm4
int GPIO_HLW_CF = 7; 
int GPIO_NRG_CF1 = 8; 

bool g_sel = true;

volatile uint32_t g_vc_pulses = 0;
volatile uint32_t g_p_pulses = 0;

void HlwCf1Interrupt(unsigned char pinNum) {  // Service Voltage and Current
	g_vc_pulses++;
}
void HlwCfInterrupt(unsigned char pinNum) {  // Service Power
	g_p_pulses++;
}
void BL0937_Init() {
	
	HAL_PIN_Setup_Output(GPIO_NRG_SEL);
    HAL_PIN_SetOutputValue(GPIO_NRG_SEL, g_sel);

	HAL_PIN_Setup_Input_Pullup(GPIO_NRG_CF1);
	
    gpio_int_enable(GPIO_NRG_CF1,IRQ_TRIGGER_FALLING_EDGE,HlwCf1Interrupt);

	HAL_PIN_Setup_Input_Pullup(GPIO_HLW_CF);
    gpio_int_enable(GPIO_HLW_CF,IRQ_TRIGGER_FALLING_EDGE,HlwCfInterrupt);

}
uint32_t res_v = 0;
uint32_t res_c = 0;
uint32_t res_p = 0;
void BL0937_RunFrame() {

	if(g_sel) {
		res_v = g_vc_pulses;
		g_sel = false;
	} else {
		res_c = g_vc_pulses;
		g_sel = true;
	}
    HAL_PIN_SetOutputValue(GPIO_NRG_SEL, g_sel);
	g_vc_pulses = 0;

	res_p = g_p_pulses;
	g_p_pulses = 0;

	addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Voltage pulses %i, current %i, power %i\n", res_v, res_c, g_p_pulses);
}

