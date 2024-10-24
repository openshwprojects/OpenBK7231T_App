/*******************************************************************************
 *  L2CAP Socket Interface
 *******************************************************************************/

#ifndef BTIF_SOCK_L2CAP_H
#define BTIF_SOCK_L2CAP_H

#include "btif_uid.h"

#include <hardware/bluetooth.h>

#define L2CAP_MASK_FIXED_CHANNEL    0x10000
#define L2CAP_MASK_LE_COC_CHANNEL   0x20000

tls_bt_status_t btsock_l2cap_init(int handle, uid_set_t *set);
tls_bt_status_t btsock_l2cap_cleanup();
tls_bt_status_t btsock_l2cap_listen(const char *name, int channel,
                                    int *sock_fd, int flags, int app_uid);
tls_bt_status_t btsock_l2cap_connect(const tls_bt_addr_t *bd_addr,
                                     int channel, int *sock_fd, int flags, int app_uid);
void btsock_l2cap_signaled(int fd, int flags, uint32_t user_id);
void on_l2cap_psm_assigned(int id, int psm);

#endif

