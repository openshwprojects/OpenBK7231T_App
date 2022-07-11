#ifdef PLATFORM_W800

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "wm_include.h"

typedef struct wmPin_s {
	const char *name;
	int code;
} wmPin_t;

static wmPin_t g_pins[] = {
	{"PA1",WM_IO_PA_01},
	{"PA4",WM_IO_PA_04},
	{"PA7",WM_IO_PA_07},
	{"PB0",WM_IO_PB_00},
	{"PB1",WM_IO_PB_01},
	{"PB2",WM_IO_PB_02},
	{"PB3",WM_IO_PB_03},
	{"PB4",WM_IO_PB_04},
	{"PB5",WM_IO_PB_05},
	{"PB6",WM_IO_PB_06},
	{"PB7",WM_IO_PB_07},
	{"PB8",WM_IO_PB_08},
	{"PB9",WM_IO_PB_09},
	{"PB10",WM_IO_PB_10},
	{"PB11",WM_IO_PB_11},
	{"PB19",WM_IO_PB_19},
	{"PB20",WM_IO_PB_20}
};
static int g_numPins = sizeof(g_pins)/sizeof(g_pins[0]);

static int PIN_GetPWMIndexForPinIndex(int pin) {
	if(pin == 6)
		return 0;
	if(pin == 7)
		return 1;
	if(pin == 8)
		return 2;
	if(pin == 9)
		return 3;
	if(pin == 24)
		return 4;
	if(pin == 26)
		return 5;
	return -1;
}

static int IsPinIndexOk(int index){
	if(index<0)
		return 0;
	if(index>=g_numPins)
		return 0;
	return 1;
}
const char *HAL_PIN_GetPinNameAlias(int index) {
	if(IsPinIndexOk(index)==0)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index) {

	return 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_write(realCode,iVal);			/*Ð´¸ß*/
}

int HAL_PIN_ReadDigitalInput(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return 0;
	realCode = g_pins[index].code;

	return tls_gpio_read(realCode);
}
void HAL_PIN_Setup_Input_Pullup(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
}
void HAL_PIN_Setup_Input(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_FLOATING);
}
void HAL_PIN_Setup_Output(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
}
void HAL_PIN_PWM_Stop(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

}

void HAL_PIN_PWM_Start(int index) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

}
void HAL_PIN_PWM_Update(int index, int value) {
	int realCode ;
	if(IsPinIndexOk(index)==0)
		return;
	realCode = g_pins[index].code;

}


#endif
