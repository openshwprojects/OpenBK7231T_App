#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/sockets.h"

extern struct hostent* lwip_gethostbyname(const char *name);
int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	return lwip_accept(s, addr, addrlen);
}

int
bind(int s, const struct sockaddr *name, socklen_t namelen)
{
	return lwip_bind(s, name, namelen);
}

int
shutdown(int s, int how)
{
	return lwip_shutdown(s, how);
}

int
closesocket(int s)
{
	return lwip_close(s);
}

int
connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	return lwip_connect(s, name, namelen);
}

int
getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
	return lwip_getsockname(s, name, namelen);
}

int
getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
	return lwip_getpeername(s, name, namelen);
}

int
setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
	return lwip_setsockopt(s, level, optname, optval, optlen);
}

int
getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
	return lwip_getsockopt(s, level, optname, optval, optlen);
}

int
listen(int s, int backlog)
{
	return lwip_listen(s, backlog);
}

int
recv(int s, void *mem, size_t len, int flags)
{
	return lwip_recv(s, mem, len, flags);
}

int
recvfrom(int s, void *mem, size_t len, int flags,
        struct sockaddr *from, socklen_t *fromlen)
{
  	return lwip_recvfrom(s, mem, len, flags, from, fromlen);
}

int
send(int s, const void *data, size_t size, int flags)
{
	return lwip_send(s, data, size, flags);
}

int
sendto(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen)
{
 	return lwip_sendto(s, data, size, flags, to, tolen);
}

int
socket(int domain, int type, int protocol)
{
	return lwip_socket(domain, type, protocol);
}

int
select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
            struct timeval *timeout)
{
	return lwip_select(maxfdp1, readset, writeset, exceptset, timeout);
}

int
ioctlsocket(int s, long cmd, void *argp)
{
	return lwip_ioctl(s, cmd, argp);
}

int
fcntl(int s, int cmd, int val)
{
	return lwip_fcntl(s, cmd, val);
}
struct hostent* gethostbyname(const char *name){
	return lwip_gethostbyname(name);
}

#endif /* LWIP_SOCKET */
