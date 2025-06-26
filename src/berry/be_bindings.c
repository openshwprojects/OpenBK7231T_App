#include "be_bindings.h"
#include "../driver/drv_local.h"
#include "../hal/hal_generic.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "be_object.h"
#include "be_run.h"

int be_SetStartValue(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		CFG_SetChannelStartupValue(ch, v);
	}
	be_return_nil(vm);
}

int be_runCmd(bvm *vm) {
	int top = be_top(vm);
	int ret;
	if (top == 1) {
		const char *cmd = be_tostring(vm, 1);
		ret = CMD_ExecuteCommand(cmd, 0);
	}
	else {
		ret = 0;
	}
	be_return_nil(vm);
}
int be_ChannelSet(bvm *vm) {
	int top = be_top(vm);

	if (top == 2) {
		if (be_isint(vm, 1) && be_isint(vm, 2)) {
			int ch = be_toint(vm, 1);
			int v = be_toint(vm, 2);
			CHANNEL_Set(ch, v, 0);
		} else if (be_isint(vm, 1) && be_isreal(vm, 2)) {
			int ch = be_toint(vm, 1);
			int v = be_toreal(vm, 2);
			CHANNEL_Set(ch, v, 0);
		}
	}
	be_return_nil(vm); /* return calculation result */
}

int be_ChannelAdd(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		CHANNEL_Add(ch, v);
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

int be_CancelThread(bvm *vm) {
	int top = be_top(vm);
	if (top == 1 && be_isint(vm, 1)) {
		int thread_id = be_toint(vm, 1);
		Berry_StopScripts(thread_id);
		be_pushbool(vm, true);
		be_return(vm);
	}
	be_pushbool(vm, false);
	be_return(vm);
}
