#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"

#define MAX_SHIFT_REGISTERS 4

// Create a structure to hold the state for each chip
typedef struct {
    byte data;
    byte latch;
    byte clk;
    short firstChannel;
    int currentValue;
    byte order;
    byte totalRegisters;
    byte invert;
    byte inUse;
} shiftReg_t;

static shiftReg_t g_shiftRegs[MAX_SHIFT_REGISTERS];

/*

Multiple independent shift registers are now supported!

// startDriver ShiftRegister [DataPin] [LatchPin] [ClkPin] [FirstChannel] [Order] [TotalRegisters] [Invert]
// Each call automatically finds the next available slot (supports up to 4 independent chips)

Example with 2 separate 74HC595 chips:
startDriver ShiftRegister 2 3 4 10 1 1 0
startDriver ShiftRegister 19 20 21 18 1 1 0

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
	shiftReg_t *reg = 0;
	
	// Find an empty slot
	for(int i = 0; i < MAX_SHIFT_REGISTERS; i++) {
		if(!g_shiftRegs[i].inUse) {
			reg = &g_shiftRegs[i];
			break;
		}
	}
	
	if(!reg) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "ShiftRegister: Max instances reached");
		return;
	}

	reg->data = Tokenizer_GetArgIntegerDefault(1, 24);
	reg->latch = Tokenizer_GetArgIntegerDefault(2, 6);
	reg->clk = Tokenizer_GetArgIntegerDefault(3, 7);
	reg->firstChannel = Tokenizer_GetArgIntegerDefault(4, 10);
	reg->order = Tokenizer_GetArgIntegerDefault(5, 1);
	reg->totalRegisters = Tokenizer_GetArgIntegerDefault(6, 1);
	reg->invert = Tokenizer_GetArgIntegerDefault(7, 0);
	reg->inUse = 1;

	HAL_PIN_Setup_Output(reg->latch);
	HAL_PIN_Setup_Output(reg->data);
	HAL_PIN_Setup_Output(reg->clk);
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
	// Loop through all active shift registers
	for(int i = 0; i < MAX_SHIFT_REGISTERS; i++) {
		shiftReg_t *reg = &g_shiftRegs[i];
		if(!reg->inUse) continue;

		int totalChannelsMapped = reg->totalRegisters * 8;
		int localCh = ch - reg->firstChannel;

		// Check if the changed channel belongs to this specific chip
		if (localCh < 0 || localCh >= totalChannelsMapped) {
			continue; 
		}

		int valToSet = value;
		if (reg->invert) {
			valToSet = !valToSet;
		}

		if (valToSet) {
			BIT_SET(reg->currentValue, localCh);
		} else {
			BIT_CLEAR(reg->currentValue, localCh);
		}

		PORT_shiftOutLatch(reg->data, reg->clk, reg->latch, reg->order, reg->currentValue, reg->totalRegisters);
	}
}




