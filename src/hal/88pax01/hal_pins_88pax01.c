#include "../../new_common.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_pins.h"
#include "hal_pinmap_88pax01.h"
#include "hw.h"

pax_pin_t g_pins[PLATFORM_GPIO_MAX] = {
	{ "P0 (WiFi LED_n)",      0 },
	{ "P1 (Tray IR S1)",      1 },
	{ "P2 (Tray IR S2)",      2 },
	{ "P3 (Tray IR S3)",      3 },
	{ "P4",                   4 },
	{ "P5",                   5 },
	{ "P6",                   6 },
	{ "P7",                   7 },
	{ "P8 (Door Sensor)",     8 },
	{ "P9",                   9 },
	{ "P10",                 10 },
	{ "P11 (Sensor 11)",     11 },
	{ "P12 (WiFi Power)",    12 },
	{ "P13",                 13 },
	{ "P14",                 14 },
	{ "P15 (Sensor 15)",     15 },
	{ "P16",                 16 },
	{ "P17",                 17 },
	{ "P18",                 18 },
	{ "P19",                 19 },
	{ "P20",                 20 },
	{ "P21",                 21 },
	{ "P22",                 22 },
	{ "P23",                 23 },
	{ "P24",                 24 },
	{ "P25",                 25 },
	{ "P26 (Panel Strobe)",  26 },
	{ "P27",                 27 },
	{ "P28 (Power LED_n)",   28 },
	{ "P29",                 29 },
	{ "P30",                 30 },
	{ "P31",                 31 },
};

const int g_numPins = PLATFORM_GPIO_MAX;

static int pin_valid(int index)
{
	return index >= 0 && index < PLATFORM_GPIO_MAX;
}

const char *HAL_PIN_GetPinNameAlias(int index)
{
	if (!pin_valid(index))
		return "error";
	return g_pins[index].name;
}

int HAL_PIN_CanThisPinBePWM(int index)
{
	(void)index;
	return 0;
}

void pax_setup_default_channels(void)
{
	/* Clear any stale virtual-pin roles from earlier config and set raw GPIO defaults */
	if (PIN_GetPinRoleForPinIndex(28) != IOR_Relay_n ||
	    PIN_GetPinRoleForPinIndex(0)  != IOR_Relay_n ||
	    PIN_GetPinRoleForPinIndex(9)  != IOR_None) {
		CFG_ClearPins();
		/* Raw SoC LEDs & sensors */
		PIN_SetPinRoleForPinIndex(28, IOR_Relay_n);        PIN_SetPinChannelForPinIndex(28, 1); /* P28 Power LED_n */
		PIN_SetPinRoleForPinIndex(0,  IOR_Relay_n);        PIN_SetPinChannelForPinIndex(0,  2); /* P0  WiFi LED_n */
		PIN_SetPinRoleForPinIndex(8,  IOR_DigitalInput_n); PIN_SetPinChannelForPinIndex(8,  3); /* P8  Door Sensor */
		PIN_SetPinRoleForPinIndex(1,  IOR_DigitalInput);   PIN_SetPinChannelForPinIndex(1,  4); /* P1  Tray IR S1 */
		PIN_SetPinRoleForPinIndex(2,  IOR_DigitalInput);   PIN_SetPinChannelForPinIndex(2,  5); /* P2  Tray IR S2 */
		PIN_SetPinRoleForPinIndex(3,  IOR_DigitalInput);   PIN_SetPinChannelForPinIndex(3,  6); /* P3  Tray IR S3 */
		PIN_SetupPins();
		CFG_Save_IfThereArePendingChanges();
	}

	CHANNEL_SetLabel(1, "P28 Power LED", 1);
	CHANNEL_SetLabel(2, "P0 WiFi LED", 1);
	CHANNEL_SetLabel(3, "P8 Door Sensor", 1);
	CHANNEL_SetLabel(4, "P1 Tray IR S1", 1);
	CHANNEL_SetLabel(5, "P2 Tray IR S2", 1);
	CHANNEL_SetLabel(6, "P3 Tray IR S3", 1);

	CHANNEL_Set(1, 1, 0);
	CHANNEL_Set(2, 1, 0);
}

void HAL_PIN_SetOutputValue(int index, int iVal)
{
	if (!pin_valid(index))
		return;
	int g = g_pins[index].gpio;
	if (g == 12)
		return; /* Keep P12 (Broadcom BCM43143 WL_REG_ON) powered */
	GPIO_OUTPUT(g) = (uint16_t)(iVal ? 1 : 0);
	GPIO_DIR(g) = 1;
}

int HAL_PIN_ReadDigitalInput(int index)
{
	if (!pin_valid(index))
		return 0;
	int g = g_pins[index].gpio;
	return (int)(GPIO_INPUT(g) & 1U);
}

void HAL_PIN_Setup_Input_Pullup(int index)
{
	if (!pin_valid(index))
		return;
	int g = g_pins[index].gpio;
	if (g == 12)
		return; /* Keep P12 (WL_REG_ON) driven high */
	GPIO_DIR(g) = 0;
}

void HAL_PIN_Setup_Input_Pulldown(int index)
{
	HAL_PIN_Setup_Input_Pullup(index);
}

void HAL_PIN_Setup_Input(int index)
{
	HAL_PIN_Setup_Input_Pullup(index);
}

void HAL_PIN_Setup_Output(int index)
{
	if (!pin_valid(index))
		return;
	int g = g_pins[index].gpio;
	if (g == 12)
		return;
	GPIO_DIR(g) = 1;
}

void HAL_PIN_PWM_Stop(int index)
{
	(void)index;
}

void HAL_PIN_PWM_Start(int index, int freq)
{
	(void)index;
	(void)freq;
}

void HAL_PIN_PWM_Update(int index, float value)
{
	(void)index;
	(void)value;
}

unsigned int HAL_GetGPIOPin(int index)
{
	if (!pin_valid(index))
		return (unsigned int)index;
	return (unsigned int)g_pins[index].gpio;
}
