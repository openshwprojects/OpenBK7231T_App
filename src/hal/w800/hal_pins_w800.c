#if defined(PLATFORM_W800) || defined (PLATFORM_W600)

#include "../../new_common.h"
#include "../../logging/logging.h"

#include "wm_include.h"

typedef struct wmPin_s {
	const char* name;
	unsigned short code;
	short pwm_channel;
} wmPin_t;

//NOTE: pwm_channel is defined based on demo/wm_pwm_demo.cs. Both W600 nd W800 support 5 PWM channels.

#if PLATFORM_W800

static wmPin_t g_pins[] = {
    {"PA0",WM_IO_PA_00, -1},
	{"PA1",WM_IO_PA_01, -1},
    {"PA2",WM_IO_PA_02, -1},
    {"PA3",WM_IO_PA_03, -1},
	{"PA4",WM_IO_PA_04, -1},
    {"PA5",WM_IO_PA_05, -1},
    {"PA6",WM_IO_PA_06, -1},
	{"PA7",WM_IO_PA_07, 4},
    {"PA8",WM_IO_PA_08, -1},
    {"PA9",WM_IO_PA_09, -1},

    {"PA10",WM_IO_PA_10, -1},
    {"PA11",WM_IO_PA_11, -1},
    {"PA12",WM_IO_PA_12, -1},
    {"PA13",WM_IO_PA_13, -1},
    {"PA14",WM_IO_PA_14, -1},
    {"PA15",WM_IO_PA_15, -1},    
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
    {"PB12",WM_IO_PB_12, -1},
    {"PB13",WM_IO_PB_13, -1},

    {"PB14",WM_IO_PB_14, -1},
    {"PB15",WM_IO_PB_15, -1},
    {"PB16",WM_IO_PB_16, -1},
    {"PB17",WM_IO_PB_17, -1},
    {"PB18",WM_IO_PB_18, -1},
	{"PB19",WM_IO_PB_19, -1},
	{"PB20",WM_IO_PB_20, -1},
    {"PB21",WM_IO_PB_21, -1},
    {"PB22",WM_IO_PB_22, -1},
    {"PB23",WM_IO_PB_23, -1},

    {"PB24",WM_IO_PB_24, -1},
    {"PB25",WM_IO_PB_25, -1},
    {"PB26",WM_IO_PB_26, -1},
    {"PB27",WM_IO_PB_27, -1}
};

#else

//W600 pinouts is based on W600-User Manual which lists pins for TW-02 and TW-03 modules.
static wmPin_t g_pins[] = {
    {"PA0",WM_IO_PA_00, -1},
	{"PA1",WM_IO_PA_01, 1},
	{"PA4",WM_IO_PA_04, 4},
	{"PA5",WM_IO_PA_05, 0},
	{"PB6",WM_IO_PB_06, 3},
	{"PB7",WM_IO_PB_07, -1},
	{"PB8",WM_IO_PB_08, 4},
	{"PB9",WM_IO_PB_09, -1},
	{"PB10",WM_IO_PB_10, -1},
	{"PB11",WM_IO_PB_11, -1},
    
	{"PB12",WM_IO_PB_12, -1},
	{"PB13",WM_IO_PB_13, 1},
	{"PB14",WM_IO_PB_14, 4},
	{"PB15",WM_IO_PB_15, 3},
	{"PB16",WM_IO_PB_16, 2},
	{"PB17",WM_IO_PB_17, 1},
	{"PB18",WM_IO_PB_18, 0}
};

#endif

static int g_numPins = sizeof(g_pins) / sizeof(g_pins[0]);

static int IsPinIndexOk(int index) {
	if (index < 0)
		return 0;
	if (index >= g_numPins)
		return 0;
	return 1;
}
static int PIN_GetPWMIndexForPinIndex(int index) {
	return g_pins[index].pwm_channel;
}
const char* HAL_PIN_GetPinNameAlias(int index) {
	if (IsPinIndexOk(index) == 0)
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index) {
	if (IsPinIndexOk(index) == 0)
		return 0;

	return PIN_GetPWMIndexForPinIndex(index) == -1 ? 0 : 1;
}
void HAL_PIN_SetOutputValue(int index, int iVal) {
	int realCode;
	if (IsPinIndexOk(index) == 0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_write(realCode, iVal);			/*Ð´¸ß*/
}

int HAL_PIN_ReadDigitalInput(int index) {
	int realCode;
	if (IsPinIndexOk(index) == 0)
		return 0;
	realCode = g_pins[index].code;

	return tls_gpio_read(realCode);
}
void HAL_PIN_Setup_Input_Pulldown(int index) {

}
void HAL_PIN_Setup_Input_Pullup(int index) {
	int realCode;
	if (IsPinIndexOk(index) == 0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
}
void HAL_PIN_Setup_Input(int index) {
	int realCode;
	if (IsPinIndexOk(index) == 0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_FLOATING);
}
void HAL_PIN_Setup_Output(int index) {
	int realCode;
	if (IsPinIndexOk(index) == 0)
		return;
	realCode = g_pins[index].code;

	tls_gpio_cfg(realCode, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
}

static int pwm_demo_multiplex_config(u8 channel, int obkPin)
{
	int realCode;
	realCode = g_pins[obkPin].code;
	switch (channel)
	{
	case 0:
		wm_pwm1_config(realCode);
		break;
	case 1:
		wm_pwm2_config(realCode);
		break;
	case 2:
		wm_pwm3_config(realCode);
		break;
	case 3:
		wm_pwm4_config(realCode);
		break;
	case 4:
		wm_pwm5_config(realCode);
		break;
	default:
		break;
	}

	return 0;
}

void HAL_PIN_PWM_Stop(int index) {
	int channel;
	if (IsPinIndexOk(index) == 0)
		return;
	channel = PIN_GetPWMIndexForPinIndex(index);
	if (channel == -1)
		return;

	tls_pwm_stop(channel);
}

void HAL_PIN_PWM_Start(int index) {
	int ret;
	int channel;
	if (IsPinIndexOk(index) == 0)
		return;
	channel = PIN_GetPWMIndexForPinIndex(index);
	if (channel == -1)
		return;

	pwm_demo_multiplex_config(channel, index);
	ret = tls_pwm_init(channel, 1000, 0, 0);
	if (ret != WM_SUCCESS)
		return;
	tls_pwm_start(channel);
}
//value is in 0 100 range
void HAL_PIN_PWM_Update(int index, float value) {
	int channel;
	if (IsPinIndexOk(index) == 0)
		return;
	channel = PIN_GetPWMIndexForPinIndex(index);
	if (channel == -1)
		return;

	if (value == 0) {
		tls_pwm_stop(channel);
	} else {
		tls_pwm_start(channel);
		tls_pwm_duty_set(channel, value * 2.55f);
	}
}

unsigned int HAL_GetGPIOPin(int index) {
	return g_pins[index].code;
}
#endif
