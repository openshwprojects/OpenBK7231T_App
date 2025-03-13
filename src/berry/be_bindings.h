#pragma once
#include "../new_common.h"
#include "berry.h"

int be_SetStartValue(bvm *vm);
int be_ChannelSet(bvm *vm);
int be_ChannelGet(bvm *vm);
int be_SetChannelType(bvm *vm);
int be_SetChannelLabel(bvm *vm);

int be_rtosDelayMs(bvm* vm);
int be_delayUs(bvm* vm);
int be_initI2c(bvm* vm);
int be_deinitI2c(bvm* vm);
int be_startI2c(bvm* vm);
int be_stopI2c(bvm* vm);
int be_writeByteI2c(bvm* vm);
int be_readByteI2c(bvm* vm);
int be_readBytesI2c(bvm* vm);
