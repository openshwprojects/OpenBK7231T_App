#pragma once
#include "../new_common.h"
#include "berry.h"

int be_SetStartValue(bvm *vm);
int be_ChannelSet(bvm *vm);
int be_ChannelGet(bvm *vm);
int be_ChannelAdd(bvm *vm);
int be_SetChannelType(bvm *vm);
int be_SetChannelLabel(bvm *vm);
int be_runCmd(bvm *vm);

int be_rtosDelayMs(bvm *vm);
int be_delayUs(bvm *vm);
int be_CancelThread(bvm *vm);
