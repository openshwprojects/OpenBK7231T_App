#include "be_run.h"
#include "../logging/logging.h"
#include "be_debug.h"

const char berryPrelude[] =
	"_suspended_closures = {}\n"
	"_suspended_closures_idx = 0\n"
	"\n"
	"def suspend_closure(closure)\n"
	"  _suspended_closures_idx += 1\n"
	"  _suspended_closures[_suspended_closures_idx] = closure\n"
	"  return _suspended_closures_idx\n"
	"end\n"
	"\n"
	"def run_closure(idx,x)\n"
	"  closure = _suspended_closures[idx]\n"
	"  closure(x)\n"
	"end\n"
	"\n"
	"def remove_closure(idx)\n"
	"  _suspended_closures.remove(idx)\n"
	"end\n";

void be_error_pop_all(bvm *vm) {
	be_pop(vm, be_top(vm)); // clear Berry stack
}

// From Tasmota
void be_dumpstack(bvm *vm) {
	int32_t top = be_top(vm);
	ADDLOG_INFO(LOG_FEATURE_BERRY, "top=%d", top);
	be_tracestack(vm);
	for (uint32_t i = 1; i <= top; i++) {
		const char *tname = be_typename(vm, i);
		const char *cname = be_classname(vm, i);
		if (be_isstring(vm, i)) {
			cname = be_tostring(vm, i);
		}
		ADDLOG_INFO(LOG_FEATURE_BERRY, "stack[%d] = type='%s' (%s)", i, (tname != NULL) ? tname : "", (cname != NULL) ? cname : "");
	}
}

bool berryRun(bvm *vm, const char *prog) {
	bool success = true;
	ADDLOG_INFO(LOG_FEATURE_BERRY, "[berry start]");
	int ret_code1 = be_loadstring(vm, prog);
	if (ret_code1 != 0) {
		ADDLOG_INFO(LOG_FEATURE_BERRY, "be_loadstring fail, retcode %d: %s", ret_code1, prog);
		be_dumpstack(vm);
		be_error_pop_all(vm);
		success = false;
		goto err;
	}

	int ret_code2 = be_pcall(vm, 0);
	if (ret_code2 != 0) {
		ADDLOG_INFO(LOG_FEATURE_BERRY, "be_pcall fail, retcode %d", ret_code2);
		be_dumpstack(vm);
		be_error_pop_all(vm);
		success = false;
		goto err;
	}

	if (be_top(vm) > 1) {
		be_error_pop_all(vm);
	} else {
		be_pop(vm, 1);
	}

err:
	ADDLOG_INFO(LOG_FEATURE_BERRY, "[berry end]");
	return success;
}

void berryRunClosure(bvm *vm, int closureId) {
	if (!be_getglobal(vm, "run_closure")) {
		return;
	}
	be_pushint(vm, closureId);
	// call run_closure(closureId)
	be_call(vm, 1);
}
void berryRunClosureInt(bvm *vm, int closureId, int x) {
	if (!be_getglobal(vm, "run_closure")) {
		return;
	}
	be_pushint(vm, closureId);
	be_pushint(vm, x);
	// call run_closure(closureId)
	be_call(vm, 2);
}
void berryRemoveClosure(bvm *vm, int closureId) {
	if (!be_getglobal(vm, "remove_closure")) {
		return;
	}
	be_pushint(vm, closureId);
	// call remove_closure(closureId)
	be_call(vm, 1);
}

void berryFreeAllClosures(bvm *vm) {
	// free waiting closures
	// kind of hacky
	berryRun(vm, "_suspended_closures = {}");
}
