#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../obk_config.h"

#if ENABLE_OBK_BERRY

#include "../driver/drv_ir.h"
#include "../driver/drv_local.h"
#include "../driver/drv_public.h"
#include "../driver/drv_uart.h"
#include "../hal/hal_adc.h"
#include "../hal/hal_flashVars.h"
#include "cmd_local.h"
#include <ctype.h>

#include "../berry/be_bindings.h"
#include "../berry/be_run.h"
#include "be_repl.h"
#include "be_vm.h"
#include "berry.h"

bvm *g_vm = NULL;

int be_AddChangeHandler(bvm *vm) {
	int top = be_top(vm);

	if (top == 4 && be_isstring(vm, 1) && be_isstring(vm, 2) && be_isint(vm, 3) && be_isfunction(vm, 4)) {
		const char *eventName = be_tostring(vm, 1);
		const char *relationStr = be_tostring(vm, 2);
		int reqArg = be_toint(vm, 3);

		int relation = parseRelationChar(relationStr);

		int eventCode = EVENT_ParseEventName(eventName);
		if (eventCode == CMD_EVENT_NONE) {
			ADDLOG_INFO(LOG_FEATURE_EVENT, "be_AddChangeHandler: %s is not a valid event", eventName);
			be_return_nil(vm);
		}

		// try to push suspend_closure function on the stack
		if (!be_getglobal(vm, "suspend_closure")) {
			// prelude not loaded??
			be_return_nil(vm);
		}
		// push the 4th argument (closure) on the stack
		be_pushvalue(vm, 4);
		// call suspend_closure with the second arg
		be_call(vm, 1);
		// it should return an ID of the suspended closure, to be used to wake up later
		if (be_isint(vm, -2)) {
			int closure_id = be_toint(vm, -2);
			scriptInstance_t *th;
			th = SVM_RegisterThread();
			if (th == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "be_AddChangeHandler: failed to alloc thread");
				be_return_nil(vm);
			}
			int thread_id = 5000 + closure_id; // TODO: alloc IDs?
			th->uniqueID = thread_id;
			th->curFile = NULL;
			th->curLine = ""; // NB. needs to be != NULL to get scheduled
			th->currentDelayMS = 0;
			th->waitingForEvent = eventCode;
			th->waitingForArgument = reqArg;
			th->waitingForRelation = relation;
			th->isBerry = true;
			th->closureId = closure_id;

			// remove the 2 values we pushed on the stack
			be_pop(vm, 2);

			// Return the thread ID to Berry
			be_pushint(vm, thread_id);
			be_return(vm);
		}
		// remove the 2 values we pushed on the stack
		be_pop(vm, 2);
	}
	be_return_nil(vm);
}

int be_ScriptDelayMs(bvm *vm) {
	int top = be_top(vm);
	if (top == 2 && be_isint(vm, 1) && be_isfunction(vm, 2)) {
		int delay_ms = be_toint(vm, 1);
		// try to push suspend_closure function on the stack
		if (!be_getglobal(vm, "suspend_closure")) {
			// prelude not loaded??
			be_return_nil(vm);
		}
		// push the second argument (closure) on the stack
		be_pushvalue(vm, 2);
		// call suspend_closure with the second arg
		be_call(vm, 1);
		// it should return an ID of the suspended closure, to be used to wake up later
		if (be_isint(vm, -2)) {
			int closure_id = be_toint(vm, -2);
			scriptInstance_t *th;
			th = SVM_RegisterThread();
			if (th == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "be_delayMs: failed to alloc thread");
				be_return_nil(vm);
			}
			int thread_id = 5000 + closure_id; // TODO: alloc IDs?
			th->uniqueID = thread_id;
			th->curFile = NULL;
			th->curLine = ""; // NB. needs to be != NULL to get scheduled
			th->currentDelayMS = delay_ms;
			th->isBerry = true;
			th->closureId = closure_id;

			// remove the 2 values we pushed on the stack
			be_pop(vm, 2);

			// Return the thread ID to Berry
			be_pushint(vm, thread_id);
			be_return(vm);
		}
		// remove the 2 values we pushed on the stack
		be_pop(vm, 2);
	}
	be_return_nil(vm);
}

static int BasicInit() {
	if (!g_vm) {
		// Lazy init for now, to avoid resource consumption and boot loops
		ADDLOG_INFO(LOG_FEATURE_BERRY, "[berry init]");
		g_vm = be_vm_new(); /* create a virtual machine instance */
		be_regfunc(g_vm, "channelSet", be_ChannelSet);
		be_regfunc(g_vm, "scriptDelayMs", be_ScriptDelayMs);
		be_regfunc(g_vm, "channelGet", be_ChannelGet);
		be_regfunc(g_vm, "channelAdd", be_ChannelAdd);
		be_regfunc(g_vm, "setStartValue", be_SetStartValue);
		be_regfunc(g_vm, "setChannelType", be_SetChannelType);
		be_regfunc(g_vm, "setChannelLabel", be_SetChannelLabel);
		be_regfunc(g_vm, "addChangeHandler", be_AddChangeHandler);

		be_regfunc(g_vm, "rtosDelayMs", be_rtosDelayMs);
		be_regfunc(g_vm, "delayUs", be_delayUs);
		be_regfunc(g_vm, "initI2c", be_initI2c);
		be_regfunc(g_vm, "startI2c", be_startI2c);
		be_regfunc(g_vm, "stopI2c", be_stopI2c);
		be_regfunc(g_vm, "writeByteI2c", be_writeByteI2c);
		be_regfunc(g_vm, "readByteI2c", be_readByteI2c);
		be_regfunc(g_vm, "readBytesI2c", be_readBytesI2c);
		be_regfunc(g_vm, "cancel", be_CancelThread);
		if (!berryRun(g_vm, berryPrelude)) {
			return 0;
		}
	}
	return 1;
}

static commandResult_t CMD_Berry(const void *context, const char *cmd, const char *args, int cmdFlags) {
	if (BasicInit()) {
		return berryRun(g_vm, args) ? CMD_RES_OK : CMD_RES_ERROR;
	}
	return CMD_RES_ERROR;
}

void berryThreadComplete(scriptInstance_t *thread) {
	// Free the associated closure if it exists
	if (thread->closureId > 0 && g_vm) {
		// Get the _suspended_closures table from global scope
		if (be_getglobal(g_vm, "_suspended_closures")) {
			// Push the closure ID as the key
			be_pushint(g_vm, thread->closureId);

			// Push nil as the value
			be_pushnil(g_vm);

			// Set _suspended_closures[closureId] = nil
			be_setindex(g_vm, -3);

			// Pop the _suspended_closures table from the stack
			be_pop(g_vm, 1);
		}
	}

	// Reset all Berry-specific flags and data
	thread->isBerry = false;
	thread->closureId = -1;
	thread->curLine = 0;
	thread->curFile = 0;
	thread->uniqueID = 0;
	thread->currentDelayMS = 0;
}

// Useful for testing things that affect global state:
//   * globals
//   * module init()
void stopBerrySVM() {
	if (g_vm) {
		// Find and stop any Berry script threads
		scriptInstance_t *t = g_scriptThreads;
		while (t) {
			if (t->isBerry) {
				berryThreadComplete(t);
			}
			t = t->next;
		}
	}
}

void CMD_StopBerry() {
	if (g_vm) {
		stopBerrySVM();
		be_vm_delete(g_vm);
		g_vm = NULL;
	}
}
static commandResult_t CMD_StopBerryCommand(const void *context, const char *cmd, const char *args, int cmdFlags) {
	CMD_StopBerry();
	return CMD_RES_OK;
}

void CMD_InitBerry() {
	// cmddetail:{"name":"berry","args":"[Berry code]",
	// cmddetail:"descr":"Execute Berry code",
	// cmddetail:"fn":"CMD_Berry","file":"cmnds/cmd_berry.c","requires":"",
	// cmddetail:"examples":"berry 1+2"}
	CMD_RegisterCommand("berry", CMD_Berry, NULL);
	// cmddetail:{"name":"stopBerry","args":"[Berry code]",
	// cmddetail:"descr":"Stop Berry VM",
	// cmddetail:"fn":"CMD_StopBerry","file":"cmnds/cmd_berry.c","requires":"",
	// cmddetail:"examples":"stopBerry"}
	CMD_RegisterCommand("stopBerry", CMD_StopBerryCommand, NULL);
}

#endif
