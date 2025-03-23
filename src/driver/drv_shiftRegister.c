#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

// GPIO index of Data
static byte g_data;
// GPIO index of Latch
static byte g_latch;
// GPIO index of Clk
static byte g_clk;
// First index of channel that is mapped to shift register.
static byte g_firstChannel;
// Current value on shift register
// This is a 32-bit integer, so up to 4 shift registers (32 bits) are supported 
static int g_currentValue;
// MSBFirst or LSBFirst
static byte g_order;
// how many 8 bit registers you have chained together
static byte g_totalRegisters;
// invert or not
static byte g_invert;

/*

// startDriver ShiftRegister [DataPin] [LatchPin] [ClkPin] [FirstChannel] [Order] [TotalRegisters] [Invert]
startDriver ShiftRegister 24 6 7 10 1 1 0
// If given argument is not present, default value is used
// First channel is a first channel that is mapped to first output of shift register.
// The total number of channels mapped is equal to TotalRegisters * 8, because every register has 8 pins.
// Order can be 0 or 1, MSBFirst or LSBFirst

// To make channel appear with Toggle on HTTP panel, please also set the type:
setChannelType 10 Toggle
setChannelType 11 Toggle
setChannelType 12 Toggle
setChannelType 13 Toggle
setChannelType 14 Toggle
setChannelType 15 Toggle
setChannelType 16 Toggle
setChannelType 17 Toggle


*/
void Shift_Init() {
	// NOTE: this is called by "startDriver ShiftRegister" command,
	// which means that Tokenizer has already tokenized the command,
	// and that argument 0 is "ShiftRegister" driver name
	g_data = Tokenizer_GetArgIntegerDefault(1, 24);
	g_latch = Tokenizer_GetArgIntegerDefault(2, 6);
	g_clk = Tokenizer_GetArgIntegerDefault(3, 7);
	g_firstChannel = Tokenizer_GetArgIntegerDefault(4, 10);
	g_order = Tokenizer_GetArgIntegerDefault(5, 1);
	g_totalRegisters = Tokenizer_GetArgIntegerDefault(6, 1);
	g_invert = Tokenizer_GetArgIntegerDefault(7, 0);

	HAL_PIN_Setup_Output(g_latch);
	HAL_PIN_Setup_Output(g_data);
	HAL_PIN_Setup_Output(g_clk);
}

void PORT_shiftOut(int dataPin, int clockPin, int bitOrder, int val, int totalRegisters);

void PORT_shiftOutLatch(int dataPin, int clockPin, int latchPin, int bitOrder, int val, int totalRegisters) {
	HAL_PIN_SetOutputValue(latchPin, 0);
	PORT_shiftOut(dataPin, clockPin, bitOrder, val, totalRegisters);
	HAL_PIN_SetOutputValue(latchPin, 1);
}

void Shift_OnEverySecond() {
#if 0
	static int testVal = 0;
	if (testVal) {
		testVal = 0;
	}
	else {
		testVal = 0xff;
	}
	PORT_shiftOutLatch(g_data, g_clk, g_latch, 0, testVal, 1);
#endif
}
void Shift_OnChannelChanged(int ch, int value) {
	int totalChannelsMapped = g_totalRegisters * 8;

	ch -= g_firstChannel;
	if (ch < 0) {
		return;
	}
	if (ch >= totalChannelsMapped) {
		return;
	}
	if (g_invert) {
		value = !value;
	}
	if (value) {
		BIT_SET(g_currentValue, ch);
	}
	else {
		BIT_CLEAR(g_currentValue, ch);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Will send value %i", g_currentValue);
	PORT_shiftOutLatch(g_data, g_clk, g_latch, g_order, g_currentValue, g_totalRegisters);
}




