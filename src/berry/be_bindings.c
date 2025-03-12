#include "be_bindings.h"
#include "../new_pins.h"
#include "../new_cfg.h"

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
	be_return_nil(vm);
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
		const char* typeStr = be_tostring(vm, 2);
		int type = CHANNEL_ParseChannelType(typeStr);
		CHANNEL_SetType(ch, type);
	}
	be_return_nil(vm);
}

int be_SetChannelLabel(bvm *vm) {
	int top = be_top(vm);

	if (top >= 2 && top <= 3 && be_isint(vm, 1) && be_isstring(vm, 2)) {
		int ch = be_toint(vm, 1);
		const char* label = be_tostring(vm, 2);
		bool bHideTogglePrefix = true;
		if (top == 3 && be_isbool(vm, 3)) {
			bHideTogglePrefix = be_tobool(vm, 3);
		}
		CHANNEL_SetLabel(ch, label, bHideTogglePrefix);
	}
	be_return_nil(vm);
}
