
#include "private-libwebsockets.h"
#include <wm_include.h>
#include <wm_sockets.h>
#include "random.h"

static int poll ( struct lws_pollfd * fds, unsigned int nfds, int timeout)
{
	int ret, i, max_fd=0;
	fd_set readset, writeset;
	struct timeval tv;
	struct lws_pollfd *fd = fds;

	tv.tv_sec = timeout/1000;
	tv.tv_usec = timeout%1000 * 1000;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);

	for(i=1;i<nfds+1;i++)
	{
		if(fd->events & POLLIN)
		{
			FD_SET(fd->fd, &readset);
		}

		if(fd->events & POLLOUT){
			FD_SET(fd->fd, &writeset);
		}
		if(fd->fd > max_fd)
			max_fd=fd->fd;
		fd = fd+1;
	}
	ret=select(max_fd+1, &readset, &writeset, 0, &tv);
	fd = fds;
	if(ret == 0) //time out
		;
	else if(ret < 0){
		ret = -1;
	}
	else{
		for(i=1;i<nfds+1;i++)
		{
			if(FD_ISSET(fd->fd, &readset))
				fd->revents |= POLLIN;
			if(FD_ISSET(fd->fd, &writeset))
				fd->revents |= POLLOUT;
			fd = fd + 1;
		}
	}

	return ret;
}

unsigned long long time_in_microseconds(void)
{
	//struct timeval tv;
	//gettimeofday(&tv, NULL);
	return (tls_os_get_time()*1000/HZ) *1000; //(tv.tv_sec * 1000000) + tv.tv_usec;
}

LWS_VISIBLE int lws_send_pipe_choked(struct lws *wsi)
{
	struct lws_pollfd fds;

	/* treat the fact we got a truncated send pending as if we're choked */
	if (wsi->trunc_len)
		return 1;

	fds.fd = wsi->sock;
	fds.events = POLLOUT;
	fds.revents = 0;

	if (poll(&fds, 1, 0) != 1)
		return 1;

	if ((fds.revents & POLLOUT) == 0)
		return 1;

	/* okay to send another packet without blocking */

	return 0;
}

LWS_VISIBLE int
lws_poll_listen_fd(struct lws_pollfd *fd)
{
	return poll(fd, 1, 0);
}

int
lws_plat_service_tsi(struct lws_context *context, int timeout_ms, int tsi)
{
	struct lws_context_per_thread *pt = &context->pt[tsi];
	int n = -1, m, c;
//	char buf;

	/* stay dead once we are dead */

	if (!context || !context->vhost_list)
		return 1;

	if (timeout_ms < 0)
		goto faked_service;

	lws_libev_run(context, tsi);
	lws_libuv_run(context, tsi);

	if (!context->service_tid_detected) {
		struct lws _lws;

		memset(&_lws, 0, sizeof(_lws));
		_lws.context = context;

		context->service_tid_detected =
			context->vhost_list->protocols[0].callback(
			&_lws, LWS_CALLBACK_GET_THREAD_ID, NULL, NULL, 0);
	}
	context->service_tid = context->service_tid_detected;

	/*
	 * is there anybody with pending stuff that needs service forcing?
	 */
	if (!lws_service_adjust_timeout(context, 1, tsi)) {
		/* -1 timeout means just do forced service */
		lws_plat_service_tsi(context, -1, pt->tid);
		/* still somebody left who wants forced service? */
		if (!lws_service_adjust_timeout(context, 1, pt->tid))
			/* yes... come back again quickly */
			timeout_ms = 0;
	}

	n = poll(pt->fds, pt->fds_count, timeout_ms);

#if 0//def LWS_OPENSSL_SUPPORT
	if (!pt->rx_draining_ext_list &&
	    !lws_ssl_anybody_has_buffered_read_tsi(context, tsi) && !n) {
#else
	if (!pt->rx_draining_ext_list && !n) /* poll timeout */ {
#endif
		lws_service_fd_tsi(context, NULL, tsi);
		return 0;
	}

faked_service:
	m = lws_service_flag_pending(context, tsi);
	if (m)
		c = -1; /* unknown limit */
	else
		if (n < 0) {
			if (LWS_ERRNO != LWS_EINTR)
				return -1;
			return 0;
		} else
			c = n;

	/* any socket with events to service? */
	for (n = 0; n < pt->fds_count && c; n++) {
		if (!pt->fds[n].revents)
			continue;

		c--;
#if 0
		if (pt->fds[n].fd == pt->dummy_pipe_fds[0]) {
			if (read(pt->fds[n].fd, &buf, 1) != 1)
				lwsl_err("Cannot read from dummy pipe. fd %d\n", pt->fds[n].fd);
			continue;
		}
#endif
		m = lws_service_fd_tsi(context, &pt->fds[n], tsi);
		if (m < 0)
			return -1;
		/* if something closed, retry this slot */
		if (m)
			n--;
	}

	return 0;
}
	
LWS_VISIBLE int
lws_plat_service(struct lws_context *context, int timeout_ms)
{
	return lws_plat_service_tsi(context, timeout_ms, 0);
}

LWS_VISIBLE int
lws_plat_set_socket_options(struct lws_vhost *vhost, int fd)
{
	int optval = 1;
	socklen_t optlen = sizeof(optval);

#if defined(__APPLE__) || \
    defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__NetBSD__) || \
    defined(__OpenBSD__)
	struct protoent *tcp_proto;
#endif

	if (vhost->ka_time) {
		/* enable keepalive on this socket */
		optval = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
			       (const void *)&optval, optlen) < 0)
			return 1;

#if defined(__APPLE__) || \
    defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
    defined(__NetBSD__) || \
        defined(__CYGWIN__) || defined(__OpenBSD__) || defined (__sun)

		/*
		 * didn't find a way to set these per-socket, need to
		 * tune kernel systemwide values
		 */
#else
		/* set the keepalive conditions we want on it too */
		optval = vhost->ka_time;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,
			       (const void *)&optval, optlen) < 0)
			return 1;

		optval = vhost->ka_interval;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,
			       (const void *)&optval, optlen) < 0)
			return 1;

		optval = vhost->ka_probes;
		if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,
			       (const void *)&optval, optlen) < 0)
			return 1;
#endif
	}

	/* Disable Nagle */
	optval = 1;
#if defined (__sun)
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&optval, optlen) < 0)
		return 1;
#elif !defined(__APPLE__) && \
      !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) &&        \
      !defined(__NetBSD__) && \
      !defined(__OpenBSD__)
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&optval, optlen) < 0)
		return 1;
#else
	tcp_proto = getprotobyname("TCP");
	if (setsockopt(fd, tcp_proto->p_proto, TCP_NODELAY, &optval, optlen) < 0)
		return 1;
#endif

	/* We are nonblocking... */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		return 1;

	return 0;
}

LWS_VISIBLE void
lws_plat_drop_app_privileges(struct lws_context_creation_info *info)
{
}

LWS_VISIBLE int
lws_plat_context_early_init(void)
{
	return 0;
}

LWS_VISIBLE void
lws_plat_context_early_destroy(struct lws_context *context)
{
}

LWS_VISIBLE void
lws_plat_context_late_destroy(struct lws_context *context)
{	

#ifdef LWS_WITH_PLUGINS
	if (context->plugin_list)
		lws_plat_plugins_destroy(context);
#endif

	if (context->lws_lookup)
		lws_free(context->lws_lookup);
#if 0
	int m = context->count_threads;
	struct lws_context_per_thread *pt = &context->pt[0];
	while (m--) {
		close(pt->dummy_pipe_fds[0]);
		close(pt->dummy_pipe_fds[1]);
		pt++;
	}
#endif
	close(context->fd_random);
}

LWS_VISIBLE void
lws_plat_insert_socket_into_fds(struct lws_context *context, struct lws *wsi)
{
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	lws_libev_io(wsi, LWS_EV_START | LWS_EV_READ);
	lws_libuv_io(wsi, LWS_EV_START | LWS_EV_READ);

	pt->fds[pt->fds_count++].revents = 0;
}

LWS_VISIBLE void
lws_plat_delete_socket_from_fds(struct lws_context *context,
						struct lws *wsi, int m)
{
	struct lws_context_per_thread *pt = &context->pt[(int)wsi->tsi];

	lws_libev_io(wsi, LWS_EV_STOP | LWS_EV_READ | LWS_EV_WRITE);
	lws_libuv_io(wsi, LWS_EV_STOP | LWS_EV_READ | LWS_EV_WRITE);

	pt->fds_count--;
}


LWS_VISIBLE void
lws_plat_service_periodic(struct lws_context *context)
{
}

LWS_VISIBLE int
lws_plat_change_pollfd(struct lws_context *context,
		      struct lws *wsi, struct lws_pollfd *pfd)
{
	return 0;
}

LWS_VISIBLE void *
lws_malloc(u32 size)
{
	return tls_mem_alloc(size);
}

LWS_VISIBLE void 
lws_free(void *p)
{
	tls_mem_free(p);
}

#ifdef LWS_USE_IPV6
LWS_VISIBLE const char *
lws_plat_inet_ntop(int af, const void *src, char *dst, int cnt)
{
	return inet_ntop(af, src, dst, cnt);
}
#endif

LWS_VISIBLE int
lws_get_random(struct lws_context *context, void *buf, int len)
{
	random_get_bytes(buf, len);
	return len;
}
LWS_VISIBLE int
lws_plat_check_connection_error(struct lws *wsi)
{
	return 0;
}
void *lws_zalloc(size_t size)
{
	void *ptr = lws_malloc(size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}

LWS_VISIBLE int
lws_plat_init(struct lws_context *context,
	      struct lws_context_creation_info *info)
{
//	struct lws_context_per_thread *pt = &context->pt[0];
//	int n = context->count_threads, fd;

	/* master context has the global fd lookup array */
	context->lws_lookup = lws_zalloc(sizeof(struct lws *) *
					 context->max_fds);
	if (context->lws_lookup == NULL) {
		lwsl_err("OOM on lws_lookup array for %d connections\n",
			 context->max_fds);
		return 1;
	}
	return 0;
}
LWS_VISIBLE int
lws_interface_to_sa(int ipv6, const char *ifname, struct sockaddr_in *addr,
		    size_t addrlen)
{

	struct tls_ethif * ethif = tls_netif_get_ethif();
	if(!ipv6)
	{
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = ip_addr_get_ip4_u32(&ethif->ip_addr);
		return 0;
	}
	return -1;
}
LWS_VISIBLE const char *
lws_plat_inet_ntop(int af, const void *src, char *dst, int cnt)
{
	return inet_ntop(af, src, dst, cnt);
}
LWS_VISIBLE void
lws_cancel_service_pt(struct lws *wsi)
{
	(void)wsi;
}
LWS_VISIBLE int 
getnameinfo(const struct sockaddr * sa, socklen_t salen,
       char * node, socklen_t nodelen, char * service,
       socklen_t servicelen, int flags)
{
	return 0;
}
