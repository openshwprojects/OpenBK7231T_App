#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"

#if ENABLE_OBK_BERRY

#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"
#include "../driver/drv_uart.h"
#include "../driver/drv_public.h"
#include "../driver/drv_local.h"
#include "../hal/hal_adc.h"
#include "../hal/hal_flashVars.h"

#include "berry.h"
#include "be_repl.h"
#include "be_vm.h"
#include "../berry/be_run.h"
#include "../berry/be_bindings.h"

bvm* g_vm = NULL;

int be_AddChangeHandler(bvm* vm)
{
	int top = be_top(vm);

	if(top == 4 && be_isstring(vm, 1) && be_isstring(vm, 2) && be_isint(vm, 3) && be_isfunction(vm, 4))
	{
		const char* eventName = be_tostring(vm, 1);
		const char* relationStr = be_tostring(vm, 2);
		int reqArg = be_toint(vm, 3);

		// XXX: Copy paste
		int relation = 0;
		if(*relationStr == '<')
		{
			relation = '<';
		}
		else if(*relationStr == '>')
		{
			relation = '>';
		}
		else if(*relationStr == '!')
		{
			relation = '!';
		}
		else
		{
			relation = 0;
		}
		int eventCode = EVENT_ParseEventName(eventName);
		if(eventCode == CMD_EVENT_NONE)
		{
			ADDLOG_INFO(LOG_FEATURE_EVENT, "be_AddChangeHandler: %s is not a valid event", eventName);
			be_return_nil(vm);
		}

		// try to push suspend_closure function on the stack
		if(!be_getglobal(vm, "suspend_closure"))
		{
			// prelude not loaded??
			be_return_nil(vm);
		}
		// push the 4th argument (closure) on the stack
		be_pushvalue(vm, 4);
		// call suspend_closure with the second arg
		be_call(vm, 1);
		// it should return an ID of the suspended closure, to be used to wake up later
		if(be_isint(vm, -2))
		{
			int closure_id = be_toint(vm, -2);
			scriptInstance_t* th;
			th = SVM_RegisterThread();
			if(th == 0)
			{
				ADDLOG_INFO(LOG_FEATURE_CMD, "be_AddChangeHandler: failed to alloc thread");
				be_return_nil(vm);
			}
			th->uniqueID = 5000 + closure_id; // TODO: alloc IDs?
			th->curFile = NULL;
			th->curLine = ""; // NB. needs to be != NULL to get scheduled
			th->currentDelayMS = 0;
			th->waitingForEvent = eventCode;
			th->waitingForArgument = reqArg;
			th->waitingForRelation = relation;
			th->isBerry = true;
			th->closureId = closure_id;
		}
		// remove the 2 values we pushed on the stack
		be_pop(vm, 2);
	}
	be_return_nil(vm);
}

int be_ScriptDelayMs(bvm* vm)
{
	int top = be_top(vm);
	if(top == 2 && be_isint(vm, 1) && be_isfunction(vm, 2))
	{
		int delay_ms = be_toint(vm, 1);
		// try to push suspend_closure function on the stack
		if(!be_getglobal(vm, "suspend_closure"))
		{
			// prelude not loaded??
			be_return_nil(vm);
		}
		// push the second argument (closure) on the stack
		be_pushvalue(vm, 2);
		// call suspend_closure with the second arg
		be_call(vm, 1);
		// it should return an ID of the suspended closure, to be used to wake up later
		if(be_isint(vm, -2))
		{
			int closure_id = be_toint(vm, -2);
			scriptInstance_t* th;
			th = SVM_RegisterThread();
			if(th == 0)
			{
				ADDLOG_INFO(LOG_FEATURE_CMD, "be_delayMs: failed to alloc thread");
				be_return_nil(vm);
			}
			th->uniqueID = 5000 + closure_id; // TODO: alloc IDs?
			th->curFile = NULL;
			th->curLine = ""; // NB. needs to be != NULL to get scheduled
			th->currentDelayMS = delay_ms;
			th->isBerry = true;
			th->closureId = closure_id;

		}
		// remove the 2 values we pushed on the stack
		be_pop(vm, 2);
	}
	be_return_nil(vm);
}

static int BasicInit()
{
	if(!g_vm)
	{
		// Lazy init for now, to avoid resource consumption and boot loops
		ADDLOG_INFO(LOG_FEATURE_BERRY, "[berry init]");
		g_vm = be_vm_new(); /* create a virtual machine instance */
		be_regfunc(g_vm, "channelSet", be_ChannelSet);
		be_regfunc(g_vm, "scriptDelayMs", be_ScriptDelayMs);
		be_regfunc(g_vm, "channelGet", be_ChannelGet);
		be_regfunc(g_vm, "setStartValue", be_SetStartValue);
		be_regfunc(g_vm, "setChannelType", be_SetChannelType);
		be_regfunc(g_vm, "setChannelLabel", be_SetChannelLabel);
		be_regfunc(g_vm, "addChangeHandler", be_AddChangeHandler);

		be_regfunc(g_vm, "rtosDelayMs", be_rtosDelayMs);
		be_regfunc(g_vm, "delayUs", be_delayUs);
		be_regfunc(g_vm, "initI2c", be_initI2c);
		be_regfunc(g_vm, "deinitI2c", be_deinitI2c);
		be_regfunc(g_vm, "startI2c", be_startI2c);
		be_regfunc(g_vm, "stopI2c", be_stopI2c);
		be_regfunc(g_vm, "writeByteI2c", be_writeByteI2c);
		be_regfunc(g_vm, "readByteI2c", be_readByteI2c);
		be_regfunc(g_vm, "readBytesI2c", be_readBytesI2c);
		if(!berryRun(g_vm, berryPrelude))
		{
			return 0;
		}
	}
	return 1;
}

static commandResult_t CMD_Berry(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	if(BasicInit())
	{
		return berryRun(g_vm, args) ? CMD_RES_OK : CMD_RES_ERROR;
	}
	return CMD_RES_ERROR;
}

void CMD_InitBerry() 
{
	//cmddetail:{"name":"berry","args":"[Berry code]",
	//cmddetail:"descr":"Execute Berry code",
	//cmddetail:"fn":"CMD_Berry","file":"cmnds/cmd_berry.c","requires":"",
	//cmddetail:"examples":"berry 1+2"}
	CMD_RegisterCommand("berry", CMD_Berry, NULL);
}

#endif
