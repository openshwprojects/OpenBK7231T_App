#ifndef __HAL_HWTIMER_H__
#define __HAL_HWTIMER_H__

#include "stdint.h"

typedef void(*HWTimerCB)(void* arg);
// periodic only
// returns timer id or -1 if not available
int8_t HAL_RequestHWTimer(float requestPeriodUs, float* realPeriodUs, HWTimerCB callback, void* arg);
void HAL_HWTimerStart(int8_t timer);
void HAL_HWTimerStop(int8_t timer);
// timer must be disabled before deinit
void HAL_HWTimerDeinit(int8_t timer);

#endif
