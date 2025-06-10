#pragma once
#include "../new_common.h"
#include "berry.h"

extern const char berryPrelude[];
void be_dumpstack(bvm *vm);

bool berryRun(bvm *vm, const char *prog);
void berryRunClosure(bvm* vm, int closureId);
void berryRunClosureBytes(bvm *vm, int closureId, byte *data, int len);
void berryRunClosureIntBytes(bvm *vm, int closureId, int x, const byte *data, int len);
void berryRunClosureIntInt(bvm *vm, int closureId, int x, int y);
void berryRunClosureInt(bvm *vm, int closureId, int x);
void berryRunClosureStr(bvm *vm, int closureId, const char *x, const char *y);
void berryRunClosurePtr(bvm *vm, int closureId, void *x);
void berryRemoveClosure(bvm *vm, int closureId);
void berryResumeClosure(bvm *vm, int closureId);
void berryFreeAllClosures(bvm *vm);
void Berry_StopScripts(int id);
int Berry_GetStackSizeCurrent();