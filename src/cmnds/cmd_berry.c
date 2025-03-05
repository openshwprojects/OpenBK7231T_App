#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"
#include "../driver/drv_uart.h"
#include "../driver/drv_public.h"
#include "../hal/hal_adc.h"
#include "../hal/hal_flashVars.h"

#include "berry.h"
#include "be_repl.h"
#include "be_vm.h"


static void be_ChannelSet(bvm *vm) {
	int top = be_top(vm);
	ADDLOG_INFO(LOG_FEATURE_CMD, "be_ChannelSet top = %d", top);
	/* check the arguments are all integers */

	if (top == 2 && be_isint(vm, 1) && be_isint(vm, 2)) {
		int ch = be_toint(vm, 1);
		int v = be_toint(vm, 2);
		ADDLOG_INFO(LOG_FEATURE_CMD, "be_ChannelSet ch = %d, v = %d", ch, v);
		CHANNEL_Set(ch, v, 0);
	}
	be_pushnil(vm); /* push the nil to the stack */
	be_return(vm); /* return calculation result */
}

static commandResult_t CMD_Berry(const void* context, const char* cmd, const char* args, int cmdFlags) {

	ADDLOG_INFO(LOG_FEATURE_CMD, "[berry start]");
	bvm *vm = be_vm_new(); /* create a virtual machine instance */
	be_regfunc(vm, "channelSet", be_ChannelSet);
	int ret_code1 = be_loadstring(vm, args);
	if (ret_code1 != 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "be_loadstring fail");
		goto err;
	}

	int ret_code2 = be_pcall(vm, 0);
	if (ret_code1 != 0) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "be_pcall fail");
		goto err;
	}

	int top = be_top(vm);
	ADDLOG_INFO(LOG_FEATURE_CMD, "top = %d", top);
	for (int i = 0; i < top; ++i) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = isnumber = %d", i, be_isnumber(vm, i));
		ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = isnil = %d", i, be_isnil(vm, i));
		ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = isbool = %d", i, be_isbool(vm, i));
		ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = isint = %d", i, be_isint(vm, i));
		ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = isreal = %d", i, be_isreal(vm, i));
		if (be_isint(vm, i)) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "stack[%d] = %d", i, be_toint(vm, i));
		}
	}

err:
	be_vm_delete(vm); /* free all objects and vm */

	ADDLOG_INFO(LOG_FEATURE_CMD, "[berry end]");
	return CMD_RES_OK;
}


void CMD_InitBerry() {

	CMD_RegisterCommand("berry", CMD_Berry, NULL);
	//cmddetail:{"name":"berry","args":"[Berry code]",
	//cmddetail:"descr":"Execute Berry code",
	//cmddetail:"fn":"CMD_Berry","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":"berry 1+2"}
}

