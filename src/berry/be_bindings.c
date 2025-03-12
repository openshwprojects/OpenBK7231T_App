#include "be_bindings.h"
#include "../new_pins.h"

int be_ChannelSet(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		CHANNEL_Set(ch, v, 0);
	}
	be_return_nil(vm); /* return calculation result */
}

