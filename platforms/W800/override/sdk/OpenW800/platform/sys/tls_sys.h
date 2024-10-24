
#ifndef TLS_SYS_H
#define TLS_SYS_H
#include "wm_type_def.h"
#include "wm_params.h"

#define SYS_MSG_NET_UP            1
#define SYS_MSG_NET_DOWN          2
#define SYS_MSG_CONNECT_FAILED    3
#define SYS_MSG_AUTO_MODE_RUN     4

#define SYS_MSG_NET2_UP           5
#define SYS_MSG_NET2_DOWN         6
#define SYS_MSG_NET2_FAIL         7

#define SYS_MSG_RMMS		   8

int tls_sys_init(void);
void tls_auto_reconnect(u8 delayflag);


#endif /* end of TLS_SYS_H */
