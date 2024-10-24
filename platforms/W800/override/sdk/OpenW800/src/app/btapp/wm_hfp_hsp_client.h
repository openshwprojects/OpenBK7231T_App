#ifndef __WM_HFP_HSP_CLIENT_HH__
#define __WM_HFP_HSP_CLIENT_HH__
#include "wm_bt.h"

extern tls_bt_status_t tls_bt_enable_hfp_client();
extern tls_bt_status_t tls_bt_disable_hfp_client();

extern tls_bt_status_t tls_bt_dial_number(const char *number);

#endif
