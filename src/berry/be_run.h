#pragma once
#include "../new_common.h"
#include "berry.h"

extern const char berryPrelude[];
void be_dumpstack(bvm *vm);

bool berryRun(bvm* vm, const char* prog);
void berryResumeClosure(bvm* vm, int closureId);
void berryFreeAllClosures(bvm* vm);
