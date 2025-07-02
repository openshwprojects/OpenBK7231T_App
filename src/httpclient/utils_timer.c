/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */


#include "../new_common.h"
#include "../quicktick.h"

#if ENABLE_SEND_POSTANDGET
 //#include "lite-log.h"
#include "utils_timer.h"

#if PLATFORM_BEKEN || WINDOWS
#include "hal_machw.h"
#elif PLATFORM_ECR6600
int hal_machw_time() {
	return os_time_get() * 1000;
}
#else
int hal_machw_time() {
	return g_timeMs;
}
#endif

void iotx_time_start(iotx_time_t *timer)
{
    if (!timer) {
        return;
    }

    timer->time = utils_time_get_ms();
}

uint32_t utils_time_spend(iotx_time_t *start)
{
    uint32_t now, res;

    if (!start) {
        return 0;
    }

    now = utils_time_get_ms();
    res = now - start->time;
    return res;
}

uint32_t iotx_time_left(iotx_time_t *end)
{
    uint32_t now, res;

    if (!end) {
        return 0;
    }

    if (utils_time_is_expired(end)) {
        return 0;
    }

    now = utils_time_get_ms();
    res = end->time - now;
    return res;
}

uint32_t utils_time_is_expired(iotx_time_t *timer)
{
    uint32_t cur_time;

    if (!timer) {
        return 1;
    }

    cur_time = utils_time_get_ms();
    /*
     *  WARNING: Do NOT change the following code until you know exactly what it do!
     *
     *  check whether it reach destination time or not.
     */
    if ((cur_time - timer->time) < (UINT32_MAX / 2)) {
        return 1;
    } else {
        return 0;
    }
}

void iotx_time_init(iotx_time_t *timer)
{
    if (!timer) {
        return;
    }

    timer->time = 0;
}

void utils_time_countdown_ms(iotx_time_t *timer, uint32_t millisecond)
{
    if (!timer) {
        return;
    }

    timer->time = utils_time_get_ms() + millisecond;
}

uint32_t utils_time_get_ms(void)
{

    return hal_machw_time()/1000;
}

uint64_t utils_time_left(uint64_t t_end, uint64_t t_now)
{
	int t = hal_machw_time();
	return (t < (int)(t_end * 1000)) ? 1 : 0;
}


#endif
