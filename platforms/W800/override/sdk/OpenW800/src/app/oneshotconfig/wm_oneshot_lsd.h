
#ifndef WM_ONESHOT_LSD_H
#define WM_ONESHOT_LSD_H


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wm_type_def.h"

#include "wm_wifi.h"
#include "tls_common.h"
#include "wm_ieee80211_gcc.h"



#define LSD_ONESHOT_DEBUG 	0

typedef enum
{

	LSD_ONESHOT_CONTINUE = 0,

	LSD_ONESHOT_CHAN_TEMP_LOCKED = 1,

	LSD_ONESHOT_CHAN_LOCKED = 2,

	LSD_ONESHOT_COMPLETE = 3,

	LSD_ONESHOT_ERR = 4

} lsd_oneshot_status_t;

struct lsd_param_t{
	u8 ssid[33];
	u8 pwd[65];
	u8 bssid[6];
	u8 user_data[128];
	u8 ssid_len;
	u8 pwd_len;
	u8 user_len;
	u8 total_len;
};

extern struct lsd_param_t *lsd_param;

typedef int (*lsd_printf_fn) (const char* format, ...);

extern lsd_printf_fn lsd_printf;

int tls_lsd_recv(u8 *buf, u16 data_len);
void tls_lsd_init(u8 *scanBss);
void tls_lsd_deinit(void);



#endif


