#ifndef TLS_SOCKET_FWUP_H
#define TLS_SOCKET_FWUP_H

#include "wm_socket.h"

#define SOCKET_FWUP_PORT 65533

#define SOCKET_FWUP_START  1
#define SOCKET_FWUP_END      2
#define SOCKET_FWUP_DATA    3

struct socket_fwup_pack{
	u8 operation;
	u16 datalen;
	u8 * data;
};

s8  socket_fwup_recv(u8 skt_num, struct pbuf *p, s8 err);
void  socket_fwup_err(u8 skt_num, s8 err);
s8 socket_fwup_poll(u8 skt_num);
s8 socket_fwup_accept(u8 skt_num, s8 err);

#endif
