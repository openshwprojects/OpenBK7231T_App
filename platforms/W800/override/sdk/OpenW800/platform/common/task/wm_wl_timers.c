#include <string.h>
#include "wm_mem.h"
#include "wm_wl_timers.h"
#include "wm_wl_task.h"

struct tls_timeo {
  struct tls_timeo *next;
  u32 time;
  tls_timeout_handler h;
  void *arg;
};

/** The one and only timeout list */
struct tls_timeo *next_timeout[TLS_TIMEO_ALL_COUONT];

/**
 * @brief          Wait (forever) for a message to arrive in an mbox.
 *                 While waiting, timeouts are processed
 *
 * @param[in]      timeo_assigned   timer NO. by assigned
 * @param[in]      mbox             the mbox to fetch the message from
 * @param[out]     **msg            the place to store the message
 *
 * @return         None
 *
 * @note           None
 */
void tls_timeouts_mbox_fetch_p(u8 timeo_assigned, tls_mbox_t mbox, void **msg)
{
  u32 time_needed;
  struct tls_timeo *tmptimeout;
  tls_timeout_handler handler;
  void *arg;

  struct tls_timeo **timeo = &next_timeout[timeo_assigned];

 again:
  if (!(*timeo)) {
    time_needed = tls_arch_mbox_fetch(mbox, msg, 0);
  } else {
    if ((*timeo)->time > 0) {
      time_needed = tls_arch_mbox_fetch(mbox, msg, (*timeo)->time);
    } else {
      time_needed = SYS_ARCH_TIMEOUT;
    }

    if (time_needed == SYS_ARCH_TIMEOUT) {
      /* If time == SYS_ARCH_TIMEOUT, a timeout occured before a message
         could be fetched. We should now call the timeout handler and
         deallocate the memory allocated for the timeout. */
      tmptimeout = *timeo;
      *timeo = tmptimeout->next;
      handler = tmptimeout->h;
      arg = tmptimeout->arg;
      tls_mem_free(tmptimeout);
      if (handler != NULL) {
        handler(arg);
      }

      /* We try again to fetch a message from the mbox. */
      goto again;
    } else {
      /* If time != SYS_ARCH_TIMEOUT, a message was received before the timeout
         occured. The time variable is set to the number of
         milliseconds we waited for the message. */
      if (time_needed < (*timeo)->time) {
        (*timeo)->time -= time_needed;
      } else {
        (*timeo)->time = 0;
      }
    }
  }
}

/**
 * @brief          create a one-shot timer (aka timeout)
 *
 * @param[in]      timeo_assigned   timer NO. by assigned
 * @param[in]      msecs            time in milliseconds after that the timer should expire
 * @param[in]      handler          callback function that would be called by the timeout
 * @param[in]      *arg             callback argument that would be passed to handler
 *
 * @return         None
 *
 * @note           while waiting for a message using sys_timeouts_mbox_fetch()
 */
void tls_timeout_p(u8 timeo_assigned, u32 msecs, tls_timeout_handler handler, void *arg)
{
	  struct tls_timeo *timeout, *t;
	  struct tls_timeo **timeo = &next_timeout[timeo_assigned];

	  timeout = (struct tls_timeo *)tls_mem_alloc(sizeof(struct tls_timeo));
	  if (timeout == NULL) {
	    return;
	  }
	  timeout->next = NULL;
	  timeout->h = handler;
	  timeout->arg = arg;
	  timeout->time = msecs;

	  if (*timeo == NULL) {
	    *timeo = timeout;
	    return;
	  }

	  if ((*timeo)->time > msecs) {
	    (*timeo)->time -= msecs;
	    timeout->next = *timeo;
	    *timeo = timeout;
	  } else {
	    for(t = *timeo; t != NULL; t = t->next) {
	      timeout->time -= t->time;
	      if (t->next == NULL || t->next->time > timeout->time) {
	        if (t->next != NULL) {
	          t->next->time -= timeout->time;
	        }
	        timeout->next = t->next;
	        t->next = timeout;
	        break;
	      }
	    }
  }
}

/**
 * @brief          Go through timeout list (for this task only) and remove the first
 *                 matching entry, even though the timeout has not triggered yet
 *
 * @param[in]      timeo_assigned   timer NO. by assigned
 * @param[in]      handler          callback function that would be called by the timeout
 * @param[in]      *arg             callback argument that would be passed to handler
 *
 * @return         None
 *
 * @note           None
 */
void tls_untimeout_p(u8 timeo_assigned, tls_timeout_handler handler, void *arg)
{
  struct tls_timeo *prev_t, *t;
  struct tls_timeo **timeo = &next_timeout[timeo_assigned];

  if (*timeo == NULL) {
    return;
  }

  for (t = *timeo, prev_t = NULL; t != NULL; prev_t = t, t = t->next) {
    if ((t->h == handler) && (t->arg == arg)) {
      /* We have a match */
      /* Unlink from previous in list */
      if (prev_t == NULL) {
        *timeo = t->next;
      } else {
        prev_t->next = t->next;
      }
      /* If not the last one, add time of this one back to next */
      if (t->next != NULL) {
        t->next->time += t->time;
      }
      tls_mem_free(t);
      return;
    }
  }
  return;
}

/**
 * @brief          timer initialized
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
s8 tls_wl_timer_init(void)
{
	memset(next_timeout, 0, sizeof(struct tls_timeo *) * TLS_TIMEO_ALL_COUONT);

	return 0;
}
