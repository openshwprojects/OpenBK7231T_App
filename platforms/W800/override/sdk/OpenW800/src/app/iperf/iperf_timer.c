/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

#include "iperf_timer.h"
#include "iperf_error.h"
#include "lwip/arch.h"
#include "wm_sockets.h"
#include "wm_osal.h"
#include "wm_mem.h"
#include "iperf.h"
int iperf_gettimeofday(struct timeval *tv, void *tz)
{
	int ret = 0;
	u32 current_tick; 

	current_tick = tls_os_get_time();
	tv->tv_sec = (current_tick) / HZ;
	tv->tv_usec = (SEC_TO_US/HZ) * (current_tick % HZ);
	return ret;
}


double
timeval_to_double(struct timeval * tv)
{
    double d;

    d = tv->tv_sec + tv->tv_usec / 1000000;

    return d;
}

int
timeval_diff(struct timeval * tv0, struct timeval * tv1)
{
    int time1, time2;
	
    time1 = tv0->tv_sec ;//+ (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec ;//+ (tv1->tv_usec / 1000000.0);


    //time1 = time1 - time2;
    time1 = time2 - time1;
#if 0
    int end = 0, current = 0;


    end += tv1->tv_sec * 1000000;
    end += tv1->tv_usec;

    current += tv0->tv_sec * 1000000;
    current += tv0->tv_usec;
int t1, t2;
	end = end - current; 
	printf("end = %d\n", end);
	t1 = end / 1000000;
			printf("time1 = %f\n", t1);
	t2 = (end%1000000)/1000000;
		printf("time2 = %f\n", t2);

	t1 = t1+t2;
			printf("time1 = %f\n", t1);
#endif
    if (time1 < 0)
        time1 = -time1;
    return (time1);
}


double
timeval_diff_1(struct timeval * tv0, struct timeval * tv1)
{
    double time1, time2;
	
    time1 = tv0->tv_sec + (tv0->tv_usec / 1000000.0);
    time2 = tv1->tv_sec + (tv1->tv_usec / 1000000.0);

    //time1 = time1 - time2;
    time1 = time2 - time1;

    if (time1 < 0)
        time1 = -time1;
    return (time1);
}

int
timer_expired_1(struct timer * tp)
{
    if (tp == NULL)
        return 0;

    struct timeval now;
    s64 end = 0, current = 0;

    iperf_gettimeofday(&now, NULL);

    end += tp->end.tv_sec * 1000000;
    end += tp->end.tv_usec;

    current += now.tv_sec * 1000000;
    current += now.tv_usec;

    return current > end;
}

int
timer_expired(struct timer * tp)
{
   if (tp == NULL)
        return 0;

   struct timeval now;
   // s64 end = 0, current = 0;
    long end = 0, current = 0;
   	long endus=0, currentus = 0;

    iperf_gettimeofday(&now, NULL);

    end += tp->end.tv_sec; 
    current += now.tv_sec;

	endus += tp->end.tv_usec;
	currentus += now.tv_usec;

	if((current>=end+1) || ((current==end)&&(currentus>=endus)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int
update_timer(struct timer * tp, time_t sec, suseconds_t usec)
{
    if (iperf_gettimeofday(&tp->begin, NULL) < 0) {
        i_errno = IEUPDATETIMER;
        return (-1);
    }

    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;
    return (0);
}

int
update_endtimer(struct timer * tp, suseconds_t usec)
{ 
	tp->end.tv_usec = usec;

    tp->expired = timer_expired;
    return (0);
}

struct timer *
new_timer(time_t sec, suseconds_t usec)
{
    struct timer *tp = NULL;
    tp = (struct timer *) tls_mem_calloc(1, sizeof(struct timer));
    if (tp == NULL) {
        i_errno = IENEWTIMER;
        return (NULL);
    }
    if (iperf_gettimeofday(&tp->begin, NULL) < 0) {
        i_errno = IENEWTIMER;
        return (NULL);
    }
    tp->end.tv_sec = tp->begin.tv_sec + (time_t) sec;
    tp->end.tv_usec = tp->begin.tv_usec + (time_t) usec;

    tp->expired = timer_expired;

    return tp;
}

void
free_timer(struct timer * tp)
{
    if (tp)
    tls_mem_free(tp);
}
#if 0
int
delay(s64 ns)
{
    struct timespec req, rem;

    req.tv_sec = 0;

    while (ns >= 1000000000L) {
        ns -= 1000000000L;
        req.tv_sec += 1;
    }

    req.tv_nsec = ns;

    while (nanosleep(&req, &rem) == -1)
        if (EINTR == errno)
            memcpy(&req, &rem, sizeof rem);
        else
            return -1;
    return 0;
}
#endif
#if 1	
//#ifdef DELAY_SELECT_METHOD
int
delay(int us)
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = us;
    (void) select(1, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv);
    return (1);
}
#endif

s64
timer_remaining(struct timer * tp)
{
    struct timeval now;
    long int  end_time = 0, current_time = 0, diff = 0;

    iperf_gettimeofday(&now, NULL);

    end_time += tp->end.tv_sec * 1000000;
    end_time += tp->end.tv_usec;

    current_time += now.tv_sec * 1000000;
    current_time += now.tv_usec;

    diff = end_time - current_time;
    if (diff > 0)
        return diff;
    else
        return 0;
}

void
cpu_util(double *pcpu)
{
    if (pcpu)
	*pcpu = 0.5;

#if 0
    static struct timeval last;
    static clock_t clast;
    struct timeval temp;
    clock_t ctemp;
    double timediff;

    if (pcpu == NULL) {
        iperf_gettimeofday(&last, NULL);
        clast = clock();
        return;
    }

    iperf_gettimeofday(&temp, NULL);
    ctemp = clock();

    timediff = ((temp.tv_sec * 1000000.0 + temp.tv_usec) -
            (last.tv_sec * 1000000.0 + last.tv_usec));

    *pcpu = ((ctemp - clast) / timediff) * 100;
	#endif
}

