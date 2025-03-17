#include "be_bindings.h"
#include "be_object.h"
#include "../driver/drv_local.h"
#include "../hal/hal_generic.h"
#include "../new_cfg.h"
#include "../new_pins.h"

int be_SetStartValue(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		CFG_SetChannelStartupValue(ch, v);
	}
	be_return_nil(vm);
}

int be_ChannelSet(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		CHANNEL_Set(ch, v, 0);
	}
	be_return_nil(vm); /* return calculation result */
}

int be_ChannelGet(bvm *vm) {
	int top = be_top(vm);

	if (top == 1 && be_isint(vm, 1)) {
		int ch = be_toint(vm, 1);
		int v = CHANNEL_Get(ch);
		be_pushint(vm, v);
		be_return(vm);
	} else {
		be_return_nil(vm);
	}
}

int be_SetChannelType(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isstring(vm, 2)) {
		int ch = be_toint(vm, 1);
		const char *typeStr = be_tostring(vm, 2);
		int type = CHANNEL_ParseChannelType(typeStr);
		CHANNEL_SetType(ch, type);
	}
	be_return_nil(vm);
}

int be_SetChannelLabel(bvm *vm) {
	int top = be_top(vm);

	if (top >= 2 && top <= 3 && be_isint(vm, 1) && be_isstring(vm, 2)) {
		int ch = be_toint(vm, 1);
		const char *label = be_tostring(vm, 2);
		bool bHideTogglePrefix = true;
		if (top == 3 && be_isbool(vm, 3)) {
			bHideTogglePrefix = be_tobool(vm, 3);
		}
		CHANNEL_SetLabel(ch, label, bHideTogglePrefix);
	}
	be_return_nil(vm);
}

int be_rtosDelayMs(bvm *vm) {
	int top = be_top(vm);
	if (top >= 1 && be_isint(vm, 1)) {
		int delay = be_toint(vm, 1);

		rtos_delay_milliseconds(delay);
	}
	be_return_nil(vm);
}

int be_delayUs(bvm *vm) {
	int top = be_top(vm);
	if (top >= 1 && be_isint(vm, 1)) {
		int delay = be_toint(vm, 1);

		HAL_Delay_us(delay);
	}
	be_return_nil(vm);
}

int be_initI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		softI2C_t *i2c = malloc(sizeof(softI2C_t));
		memset(i2c, 0, sizeof(softI2C_t));
		i2c->pin_clk = be_toint(vm, 1);
		i2c->pin_data = be_toint(vm, 2);
		be_pushcomptr(vm, i2c);
		be_newcomobj(vm, i2c, &be_commonobj_destroy_generic);
		be_return(vm);
	}
	be_return_nil(vm);
}

int be_startI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 2 && be_iscomptr(vm, 1) && be_isint(vm, 2)) {
		softI2C_t *i2c = be_tocomptr(vm, 1);
		int addr = be_toint(vm, 2);
		Soft_I2C_Start(i2c, addr);
	}
	be_return_nil(vm);
}

int be_stopI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 1 && be_iscomptr(vm, 1)) {
		softI2C_t *i2c = be_tocomptr(vm, 1);
		Soft_I2C_Stop(i2c);
	}
	be_return_nil(vm);
}

int be_writeByteI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 2 && be_iscomptr(vm, 1) && be_isint(vm, 2)) {
		softI2C_t *i2c = be_tocomptr(vm, 1);
		int data = be_toint(vm, 2);
		Soft_I2C_WriteByte(i2c, data);
	}
	be_return_nil(vm);
}

int be_readByteI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 2 && be_iscomptr(vm, 1) && be_isbool(vm, 2)) {
		softI2C_t *i2c = be_tocomptr(vm, 1);
		int nack = be_tobool(vm, 2);
		uint8_t data = Soft_I2C_ReadByte(i2c, nack);
		be_pushint(vm, data);
		be_return(vm);
	}
	be_return_nil(vm);
}

int be_readBytesI2c(bvm *vm) {
	int top = be_top(vm);
	if (top >= 2 && be_iscomptr(vm, 1) && be_isint(vm, 2)) {
		softI2C_t *i2c = be_tocomptr(vm, 1);
		int size = be_toint(vm, 2);
		uint8_t *data = malloc(size * sizeof(uint8_t));
		Soft_I2C_ReadBytes(i2c, data, size);
		be_newobject(vm, "list");
		for (int i = 0; i < size; i++) {
			be_pushint(vm, data[i]);
			be_data_push(vm, -2);
			be_pop(vm, 1);
		}
		be_pop(vm, 1);
		free(data);
		be_return(vm);
	}
	be_return_nil(vm);
}
