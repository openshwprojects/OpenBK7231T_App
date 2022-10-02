#if defined(PLATFORM_W800) || defined (PLATFORM_W600)

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "wm_include.h"

typedef struct wmPin_s {
	const char *name;
	unsigned short code;
	short pwm_channel;
} wmPin_t;

//NOTE: pwm_channel is defined based on demo/wm_pwm_demo.cs

#if PLATFORM_W800

static wmPin_t g_pins[] = {
	{"PA1",WM_IO_PA_01, -1},
	{"PA4",WM_IO_PA_04, -1},
	{"PA7",WM_IO_PA_07, 4},
	{"PB0",WM_IO_PB_00, 0},
	{"PB1",WM_IO_PB_01, 1},
	{"PB2",WM_IO_PB_02, 2},
	{"PB3",WM_IO_PB_03, 3},
	{"PB4",WM_IO_PB_04, -1},
	{"PB5",WM_IO_PB_05, -1},
	{"PB6",WM_IO_PB_06, -1},
	{"PB7",WM_IO_PB_07, -1},
	{"PB8",WM_IO_PB_08, -1},
	{"PB9",WM_IO_PB_09, -1},
	{"PB10",WM_IO_PB_10, -1},
	{"PB11",WM_IO_PB_11, -1},
	{"PB19",WM_IO_PB_19, -1},
	{"PB20",WM_IO_PB_20, -1}
};

#else

//W600 pinouts is based on W600-User Manual which lists pins for TW-02 and TW-03 modules.
static wmPin_t g_pins[] = {
	{"PA1",WM_IO_PA_01, -1},
	{"PA4",WM_IO_PA_04, -1},
	{"PA5",WM_IO_PA_05, -1},
	{"PB8",WM_IO_PB_08, -1},
	{"PB6",WM_IO_PB_06, -1},
	{"PB7",WM_IO_PB_07, -1},
	{"PB9",WM_IO_PB_09, -1},
	{"PB10",WM_IO_PB_10, -1},
	{"PB11",WM_IO_PB_11, -1},
	{"PB12",WM_IO_PB_12, -1},
	{"PB13",WM_IO_PB_13, -1},
	{"PB14",WM_IO_PB_14, 4},
	{"PB15",WM_IO_PB_15, 3},
	{"PB16",WM_IO_PB_16, 2},
	{"PB17",WM_IO_PB_17, 1},
	{"PB18",WM_IO_PB_18, 0}
};

#endif

static int g_numPins = sizeof(g_pins)/sizeof(g_pins[0]);

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
	if(IsPinIndexOk(index)==0)
		return 0;
	
	return g_pins[index].pwm_channel == -1 ? 0 : 1;
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
