#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../obk_config.h"

#if ENABLE_OBK_BERRY

#include "../driver/drv_openWeatherMap.h"
#include "../driver/drv_ir.h"
#include "../driver/drv_ntp.h"
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
#include "../libraries/obktime/obktime.h"	// for time functions
#include "../driver/drv_deviceclock.h"
bvm *g_vm = NULL;

typedef struct berryInstance_s
{
	int uniqueID;
	int totalDelayMS;
	int currentDelayMS;
	int delayRepeats;
	int closureId;
	eventWait_t wait;
	bool bFire;

	struct berryInstance_s* next;
} berryInstance_t;

berryInstance_t *g_berryThreads = 0;

berryInstance_t *Berry_RegisterThread() {
	berryInstance_t *r;

	r = g_berryThreads;

	while (r) {
		if (r->uniqueID == 0) {
			break;
		}
		r = r->next;
	}
	if (r == 0) {
		r = malloc(sizeof(berryInstance_t));
		memset(r, 0, sizeof(berryInstance_t));
		r->next = g_berryThreads;
		g_berryThreads = r;
	}
	r->uniqueID = 0;
	r->currentDelayMS = 0;
	return r;
}

void CMD_Berry_ProcessWaitersForEvent(byte eventCode, int argument) {
	berryInstance_t *t;


	t = g_berryThreads;

	while (t) {
		if (CheckEventCondition(&t->wait, eventCode, argument)) {
			t->bFire = true;
		}
		t = t->next;
	}
	// TODO: better
	CMD_Berry_RunEventHandlers_IntInt(eventCode, argument, 0);
}
void CMD_Berry_RunEventHandlers_IntInt(byte eventCode, int argument, int argument2) {
	berryInstance_t *t;

	t = g_berryThreads;

	while (t) {
		if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'a') {
			berryRunClosureIntInt(g_vm, t->closureId, argument, argument2);
		} else if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'm'
			&& t->wait.waitingForArgument == argument) {
			berryRunClosureInt(g_vm, t->closureId, argument2);
		}
		t = t->next;
	}
}

int CMD_Berry_RunEventHandlers_StrPtr(byte eventCode, const char *argument, void* argument2) {
	berryInstance_t *t;

	t = g_berryThreads;

	int calls = 0;
	while (t) {
		if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'a') {
			berryRunClosureStr(g_vm, t->closureId, argument, argument2);
			calls++;
		} else if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'm'
			&& !stricmp(t->wait.waitingForArgumentStr,argument)) {
			berryRunClosurePtr(g_vm, t->closureId, argument2);
			calls++;
		}
		t = t->next;
	}
	return calls;
}

void CMD_Berry_RunEventHandlers_IntBytes(byte eventCode, int argument, const byte *data, int size) {
	berryInstance_t *t;

	t = g_berryThreads;

	while (t) {
		if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'a') {
			berryRunClosureIntBytes(g_vm, t->closureId, argument, data, size);
		}
		else if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'm'
			&& t->wait.waitingForArgument == argument) {
			berryRunClosureBytes(g_vm, t->closureId, data, size);
		}
		t = t->next;
	}
}
int CMD_Berry_RunEventHandlers_Str(byte eventCode, const char *argument, const char *argument2) {
	berryInstance_t *t;

	t = g_berryThreads;

	int c_run = 0;
	while (t) {
		if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'a') {
			berryRunClosureStr(g_vm, t->closureId, argument, argument2);
			c_run++;
		}
		else if (t->wait.waitingForEvent == eventCode
			&& t->wait.waitingForRelation == 'm'
			&& !stricmp(t->wait.waitingForArgumentStr,argument)) {
			berryRunClosureStr(g_vm, t->closureId, argument2, "");
			c_run++;
		}
		t = t->next;
	}
	return c_run;
}
int be_addClosure(bvm *vm, const char *eventName, int relation, int reqArg, const char *reqArgStr, int argumentIndex) {
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
	be_pushvalue(vm, argumentIndex);
	// call suspend_closure with the second arg
	be_call(vm, 1);
	// it should return an ID of the suspended closure, to be used to wake up later
	if (be_isint(vm, -2)) {
		int closure_id = be_toint(vm, -2);
		berryInstance_t *th;
		th = Berry_RegisterThread();
		if (th == 0) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "be_AddChangeHandler: failed to alloc thread");
			be_return_nil(vm);
		}
		int thread_id = 5000 + closure_id; // TODO: alloc IDs?
		th->uniqueID = thread_id;
		th->currentDelayMS = 0;
		th->wait.waitingForEvent = eventCode;
		th->wait.waitingForArgument = reqArg;
		if (reqArgStr) {
			strcpy_safe(th->wait.waitingForArgumentStr, reqArgStr, sizeof(th->wait.waitingForArgumentStr));
		}
		else {
			th->wait.waitingForArgumentStr[0] = 0;
		}
		th->wait.waitingForRelation = relation;
		th->closureId = closure_id;

		// remove the 2 values we pushed on the stack
		be_pop(vm, 2);

		// Return the thread ID to Berry
		be_pushint(vm, thread_id);
		be_return(vm);
	}
	// remove the 2 values we pushed on the stack
	be_pop(vm, 2);
	be_return_nil(vm);
}
int be_poststr(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isstring(vm, 2) && be_iscomptr(vm, 1)) {
		const char *s = be_tostring(vm, 2);
		http_request_t* request = be_tocomptr(vm, 1);

		poststr(request, s);
	}
	be_return_nil(vm);
}
http_request_t *g_currentRequest;
int be_echo(bvm *vm) {
	int top = be_top(vm);

	if (top == 1) {
		const char *s = be_tostring(vm, 1);

		poststr(g_currentRequest, s);
	}
	be_return_nil(vm);
}
void Berry_SaveRequest(http_request_t *r) {
	g_currentRequest = r;
}

int be_AddChangeHandler(bvm *vm) {
	int top = be_top(vm);

	if (top == 4 && be_isstring(vm, 1) && be_isstring(vm, 2) && be_isint(vm, 3) && be_isfunction(vm, 4)) {
		const char *eventName = be_tostring(vm, 1);
		const char *relationStr = be_tostring(vm, 2);
		int reqArg = be_toint(vm, 3);

		int relation = parseRelationChar(relationStr);
		be_addClosure(vm, eventName, relation, reqArg, 0, 4);
	}
	be_return_nil(vm);
}

int be_AddEventHandler(bvm *vm) {
	int top = be_top(vm);

	if (top == 2 && be_isstring(vm, 1) && be_isfunction(vm, 2)) {
		const char *eventName = be_tostring(vm, 1);
		be_addClosure(vm, eventName, 'a', 0, 0, 2);
	} else if (top == 3 && be_isstring(vm, 1) && be_isint(vm, 2) && be_isfunction(vm, 3)) {
		const char *eventName = be_tostring(vm, 1);
		int arg = be_toint(vm, 2);
		be_addClosure(vm, eventName, 'm', arg, 0, 3);
	} else if (top == 3 && be_isstring(vm, 1) && be_isstring(vm, 2) && be_isfunction(vm, 3)) {
		const char *eventName = be_tostring(vm, 1);
		const char *argStr = be_tostring(vm, 2);
		be_addClosure(vm, eventName, 'm', 0, argStr, 3);
	}
	be_return_nil(vm);
}


int be_setTimeoutInternal(bvm *vm, int repeats) {
	int top = be_top(vm);
	if (top == 2 && be_isint(vm, 2) && be_isfunction(vm, 1)) {
		int delay_ms = be_toint(vm, 2);
		// try to push suspend_closure function on the stack
		if (!be_getglobal(vm, "suspend_closure")) {
			// prelude not loaded??
			be_return_nil(vm);
		}
		// push the second argument (closure) on the stack
		be_pushvalue(vm, 1);
		// call suspend_closure with the second arg
		be_call(vm, 1);
		// it should return an ID of the suspended closure, to be used to wake up later
		if (be_isint(vm, -2)) {
			int closure_id = be_toint(vm, -2);
			berryInstance_t *th;
			th = Berry_RegisterThread();
			if (th == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "be_delayMs: failed to alloc thread");
				be_return_nil(vm);
			}
			int thread_id = 5000 + closure_id; // TODO: alloc IDs?
			th->uniqueID = thread_id;
			th->currentDelayMS = delay_ms;
			th->totalDelayMS = delay_ms;
			th->closureId = closure_id;
			th->delayRepeats = repeats;

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
int be_setTimeout(bvm *vm) {
	be_setTimeoutInternal(vm, 0);
	return 1;
}
#if ENABLE_DRIVER_OPENWEATHERMAP
int be_getOpenWeatherReply(bvm *vm) {
	const char *data = Weather_GetReply();
	be_pushstring(vm, data);
	be_return(vm);
	return 1;
}
#endif
int be_get(bvm *vm) {
	char tmpA[64];
	int top = be_top(vm);
	if (top == 1 && be_isstring(vm, 1)) {
		const char *name = be_tostring(vm, 1);
		if (name[0] == '$') {
			float ret;
			CMD_ExpandConstantFloat(name, 0, &ret);
			be_pushreal(vm, ret);
			be_return(vm);
		}
		else {
			if (http_getArg(g_currentRequest->url, name, tmpA, sizeof(tmpA))) {
				be_pushstring(vm, tmpA);
				be_return(vm);
			}
		}
	}
	be_return_nil(vm);
}

int be_setInterval(bvm *vm) {
	be_setTimeoutInternal(vm, -1);
	return 1;
}
int be_gmtime(bvm *vm) {
	struct tm *ltm;
	const char *fields;
	int i;

	int top = be_top(vm);
	if (top == 1 && be_isstring(vm, 1)) {
		fields = be_tostring(vm, 1);
	}
	else {
		fields = "YMDhmsw"; // default all fields
	}

/*
	ltm = gmtime(&g_ntpTime);

	be_newobject(vm, "list"); // create a new list on top of the stack

	for (i = 0; fields[i]; i++) {
		int value = 0;
		switch (fields[i]) {
		case 'Y': value = ltm->tm_year + 1900; break;
		case 'M': value = ltm->tm_mon + 1;     break;
		case 'D': value = ltm->tm_mday;        break;
		case 'h': value = ltm->tm_hour;        break;
		case 'm': value = ltm->tm_min;         break;
		case 's': value = ltm->tm_sec;         break;
		case 'w': value = ltm->tm_wday;        break;
		}

		be_pushint(vm, value);    // push value onto stack
		be_data_push(vm, -2);     // append value to list at -2
		be_pop(vm, 1);            // pop the temporary value
	}
*/

	TimeComponents tc;
	tc=calculateComponents(TIME_GetCurrentTime());

	be_newobject(vm, "list"); // create a new list on top of the stack

	for (i = 0; fields[i]; i++) {
		int value = 0;
		switch (fields[i]) {
		case 'Y': value = tc.year;   break;
		case 'M': value = tc.month;  break;
		case 'D': value = tc.day;    break;
		case 'h': value = tc.hour;   break;
		case 'm': value = tc.minute; break;
		case 's': value = tc.second; break;
		case 'w': value = tc.wday;   break;
		}

		be_pushint(vm, value);    // push value onto stack
		be_data_push(vm, -2);     // append value to list at -2
		be_pop(vm, 1);            // pop the temporary value
	}



	be_pop(vm, 1);
	be_return(vm); // return the list on top of the stack
}







static int BasicInit() {
	if (!g_vm) {
		// Lazy init for now, to avoid resource consumption and boot loops
		ADDLOG_INFO(LOG_FEATURE_BERRY, "[berry init]");
		g_vm = be_vm_new(); /* create a virtual machine instance */
		be_regfunc(g_vm, "setChannel", be_ChannelSet);
		be_regfunc(g_vm, "setTimeout", be_setTimeout);
		be_regfunc(g_vm, "getVar", be_get);
		be_regfunc(g_vm, "setInterval", be_setInterval);
		be_regfunc(g_vm, "getChannel", be_ChannelGet);
		be_regfunc(g_vm, "addChannel", be_ChannelAdd);
		be_regfunc(g_vm, "setStartValue", be_SetStartValue);
		be_regfunc(g_vm, "setChannelType", be_SetChannelType);
		be_regfunc(g_vm, "setChannelLabel", be_SetChannelLabel);
		be_regfunc(g_vm, "addChangeHandler", be_AddChangeHandler);
		be_regfunc(g_vm, "addEventHandler", be_AddEventHandler);
		be_regfunc(g_vm, "runCmd", be_runCmd);
		be_regfunc(g_vm, "poststr", be_poststr);
		be_regfunc(g_vm, "echo", be_echo);

		be_regfunc(g_vm, "gmtime", be_gmtime);

#if ENABLE_DRIVER_OPENWEATHERMAP
		be_regfunc(g_vm, "getOpenWeatherReply", be_getOpenWeatherReply);
#endif
		be_regfunc(g_vm, "rtosDelayMs", be_rtosDelayMs);
		be_regfunc(g_vm, "delayUs", be_delayUs);
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
void eval_berry_snippet(const char *s) {
	if (BasicInit()) {
		berryRun(g_vm, s);
	}
}

void berryThreadComplete(berryInstance_t *thread) {
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
	thread->closureId = -1;
	thread->uniqueID = 0;
	thread->currentDelayMS = 0;
	thread->wait.waitingForArgument = 0;
	thread->wait.waitingForEvent = 0;
	thread->wait.waitingForRelation = 0;
}

// Useful for testing things that affect global state:
//   * globals
//   * module init()
void stopBerrySVM() {
	if (g_vm) {
		// Find and stop any Berry script threads
		berryInstance_t *t = g_berryThreads;
		while (t) {
			berryThreadComplete(t);
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

void Berry_StopAllScripts() {
	berryInstance_t *t;

	t = g_berryThreads;
	while (t) {
		t->uniqueID = 0;
		t->currentDelayMS = 0;
		berryThreadComplete(t);
		t = t->next;
	}
}
void Berry_RunThread(berryInstance_t *t) {
	berryRunClosure(g_vm, t->closureId);
	berryRemoveClosure(g_vm, t->closureId);
	berryThreadComplete(t);
}
void Berry_StopScripts(int id) {
	berryInstance_t *t;

	t = g_berryThreads;
	while (t) {
		if (t->uniqueID == id) {
			t->uniqueID = 0;
			t->currentDelayMS = 0;
			berryThreadComplete(t);
		}
		t = t->next;
	}
}
int Berry_GetStackSizeTotal() {
	if (g_vm) {
		int size = g_vm->stacktop - g_vm->stack;    /* with debug enabled, stack increase may be negative */
		return size;
	}
	return 0;
}
int Berry_GetStackSizeCurrent() {
	if (g_vm) {
		int size = g_vm->top - g_vm->stack;
		return size;
	}
	return 0;
}

void Berry_RunThreads(int deltaMS) {
	int c_sleep, c_run;

	c_sleep = 0;
	c_run = 0;


	berryInstance_t *g_activeThread = g_berryThreads;
	while (g_activeThread) {
		if (g_activeThread->uniqueID > 0) {
			if (g_activeThread->wait.waitingForEvent) {
				if (g_activeThread->bFire) {
					berryRunClosure(g_vm, g_activeThread->closureId);
					g_activeThread->bFire = false;
				}
				// do nothing
			}
			else {
				if (g_activeThread->currentDelayMS > 0) {
					g_activeThread->currentDelayMS -= deltaMS;
					// the following block is needed to handle with long freezes on simulator
					if (g_activeThread->currentDelayMS <= 0) {
						if (g_activeThread->delayRepeats == -1) {
							berryRunClosure(g_vm, g_activeThread->closureId);
							g_activeThread->currentDelayMS = g_activeThread->totalDelayMS;
						}
						else if (g_activeThread->delayRepeats > 0) {
							g_activeThread->delayRepeats--;
							berryRunClosure(g_vm, g_activeThread->closureId);
							g_activeThread->currentDelayMS = g_activeThread->totalDelayMS;
						}
						else {
							// finish totally
							berryRunClosure(g_vm, g_activeThread->closureId);
							berryRemoveClosure(g_vm, g_activeThread->closureId);
							g_activeThread->closureId = 0;
							g_activeThread->uniqueID = 0;//free
						}
					}
				}
				else {
					Berry_RunThread(g_activeThread);
				}
			}
		}
		g_activeThread = g_activeThread->next;
	}

}
void CMD_InitBerry() {
	//cmddetail:{"name":"berry","args":"[Berry code]",
	//cmddetail:"descr":"Execute Berry code",
	//cmddetail:"fn":"CMD_Berry","file":"cmnds/cmd_berry.c","requires":"",
	//cmddetail:"examples":"berry 1+2"}
	CMD_RegisterCommand("berry", CMD_Berry, NULL);
	//cmddetail:{"name":"stopBerry","args":"[Berry code]",
	//cmddetail:"descr":"Stop Berry VM",
	//cmddetail:"fn":"CMD_StopBerryCommand","file":"cmnds/cmd_berry.c","requires":"",
	//cmddetail:"examples":"stopBerry"}
	CMD_RegisterCommand("stopBerry", CMD_StopBerryCommand, NULL);
}

#endif
