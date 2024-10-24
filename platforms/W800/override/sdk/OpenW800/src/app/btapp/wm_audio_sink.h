#ifndef __WM_AUDIO_SINK_H__
#define __WM_AUDIO_SINK_H__
#include "wm_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

extern tls_bt_status_t tls_bt_enable_a2dp_sink();
extern tls_bt_status_t tls_bt_disable_a2dp_sink();

#ifdef __cplusplus
}
#endif

#endif
