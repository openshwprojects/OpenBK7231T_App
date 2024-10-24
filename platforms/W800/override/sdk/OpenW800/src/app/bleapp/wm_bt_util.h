#ifndef __WM_BT_UTIL_H__
#define __WM_BT_UTIL_H__
#include <stdint.h>
#include "wm_bt_def.h"
#include "host/ble_hs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TLS_TRACE_TYPE_ERROR            0x00000000
#define TLS_TRACE_TYPE_WARNING          0x00000001
#define TLS_TRACE_TYPE_API              0x00000002
#define TLS_TRACE_TYPE_EVENT            0x00000003
#define TLS_TRACE_TYPE_DEBUG            0x00000004

typedef struct {
    uint32_t total;
    uint32_t available;
    uint8_t *base;
    uint8_t *head;
    uint8_t *tail;
    struct ble_npl_mutex buffer_mutex;
} ringbuffer_t;

typedef void (app_async_func_t)(void *arg);

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

extern tls_bt_log_level_t tls_appl_trace_level;


/* define log for application */
#if 1
#define TLS_BT_APPL_TRACE_ERROR(...)                 {if (tls_appl_trace_level >= TLS_BT_LOG_ERROR) tls_bt_log(TLS_TRACE_TYPE_ERROR, ##__VA_ARGS__);}
#define TLS_BT_APPL_TRACE_WARNING(...)               {if (tls_appl_trace_level >= TLS_BT_LOG_WARNING) tls_bt_log(TLS_TRACE_TYPE_WARNING, ##__VA_ARGS__);}
#define TLS_BT_APPL_TRACE_API(...)                   {if (tls_appl_trace_level >= TLS_BT_LOG_API) tls_bt_log( TLS_TRACE_TYPE_API, ##__VA_ARGS__);}
#define TLS_BT_APPL_TRACE_EVENT(...)                 {if (tls_appl_trace_level >= TLS_BT_LOG_EVENT) tls_bt_log(TLS_TRACE_TYPE_EVENT, ##__VA_ARGS__);}
#define TLS_BT_APPL_TRACE_DEBUG(...)                 {if (tls_appl_trace_level >= TLS_BT_LOG_DEBUG) tls_bt_log(TLS_TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#define TLS_BT_APPL_TRACE_VERBOSE(...)               {if (tls_appl_trace_level >= TLS_BT_LOG_VERBOSE) tls_bt_log(TLS_TRACE_TYPE_DEBUG, ##__VA_ARGS__);}
#else
#define TLS_BT_APPL_TRACE_ERROR(...)
#define TLS_BT_APPL_TRACE_WARNING(...)
#define TLS_BT_APPL_TRACE_API(...)
#define TLS_BT_APPL_TRACE_EVENT(...)
#define TLS_BT_APPL_TRACE_DEBUG(...)
#define TLS_BT_APPL_TRACE_VERBOSE(...)

#endif

void tls_bt_log(uint32_t trace_set_mask, const char *fmt_str, ...);
const char *tls_bt_gap_evt_2_str(uint32_t event);
const char *tls_bt_rc_2_str(uint32_t event);

extern int tls_bt_util_init(void);
extern int tls_bt_util_deinit(void);
extern int tls_bt_async_proc_func(app_async_func_t *app_cb, void *app_arg, int ticks);
extern ringbuffer_t *bt_ringbuffer_init(const size_t size);
extern void bt_ringbuffer_free(ringbuffer_t *rb);
extern uint32_t bt_ringbuffer_available(const ringbuffer_t *rb);
extern uint32_t bt_ringbuffer_size(const ringbuffer_t *rb);
extern uint32_t bt_ringbuffer_insert(ringbuffer_t *rb, const uint8_t *p, uint32_t length);
extern uint32_t bt_ringbuffer_delete(ringbuffer_t *rb, uint32_t length);
extern uint32_t bt_ringbuffer_peek(const ringbuffer_t *rb, int offset, uint8_t *p, uint32_t length);
extern uint32_t bt_ringbuffer_pop(ringbuffer_t *rb, uint8_t *p, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
