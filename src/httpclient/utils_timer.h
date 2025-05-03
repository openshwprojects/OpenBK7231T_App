/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef _UTILS_TIMER_H_
#define _UTILS_TIMER_H_

//#include "iot_import.h"
#include "../new_common.h"

#if PLATFORM_BEKEN || WINDOWS

typedef struct {
    uint32_t time;
} iotx_time_t;


void iotx_time_start(iotx_time_t *timer);

uint32_t utils_time_spend(iotx_time_t *start);

uint32_t iotx_time_left(iotx_time_t *end);

uint32_t utils_time_is_expired(iotx_time_t *timer);

void iotx_time_init(iotx_time_t *timer);

void utils_time_countdown_ms(iotx_time_t *timer, uint32_t millisecond);

uint32_t utils_time_get_ms(void);

uint64_t utils_time_left(uint64_t t_end, uint64_t t_now);

#endif

#endif /* _IOTX_COMMON_TIMER_H_ */
