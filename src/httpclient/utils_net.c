/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include "../new_common.h"
#include "../logging/logging.h"
#if PLATFORM_BEKEN || WINDOWS
#include "utils_net.h"
#include "utils_timer.h"
#include "errno.h"
#include "lwip/sockets.h"
#ifndef WINDOWS
#include "lwip/netdb.h"
#endif

uintptr_t HAL_TCP_Establish(const char *host, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = 0;
    char service[6];

    os_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //only IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    snprintf(service, sizeof(service), "%u", port);

    if ((rc = getaddrinfo(host, service, &hints, &addrInfoList)) != 0) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"getaddrinfo error");
        return 0;
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"socket type error");
            rc = 0;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"create socket error %i",fd);
            rc = 0;
            continue;
        }
		ADDLOG_INFO(LOG_FEATURE_HTTP_CLIENT, "HAL_TCP_Establish: created socket %i\n\r",(int)fd);

        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        lwip_close(fd);
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"connect error");
        rc = 0;
    }

    if (0 == rc){
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"fail to establish tcp");
    } else {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"success to establish tcp, fd=%d", rc);
    }
    freeaddrinfo(addrInfoList);

    return (uintptr_t)rc;
}


int32_t HAL_TCP_Destroy(uintptr_t fd)
{
    int rc;
	///int att;

    //Shutdown both send and receive operations.
    rc = shutdown((int) fd, 2);
    if (0 != rc) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"shutdown error %i",rc);
        return -1;
    }
#if 0
	for(att = 0; att < 10; att++) {
		rc = lwip_close((int) fd);
		if (0 != rc) {
			ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"closesocket(%i) error %i at attemtp %i",((int)fd),rc,att);
			delay_ms(500);
		} else {
			break;
		}
	}
#elif 1
    rc = lwip_close((int) fd);
    if (0 != rc) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"closesocket(%i) error %i",((int)fd),rc);
		// same as above but without SOCK_DEINIT_SYNC check
		// There is a bug in our htttp client and this is a temporary work around for that
		// Otherwise, it leaves sockets unfried and they adds up to 38 and block all networking
		lwip_close_force((int) fd);
        return -1;
    }
#else
    rc = lwip_close((int) fd);
    if (0 != rc) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"closesocket(%i) error %i",((int)fd),rc);
        return -1;
    }
#endif
    return 0;
}


int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
#if 0
	uint32_t len_sent;
	len_sent = send(fd, buf, len, 0);
#else
    int ret, err_code;
    uint32_t len_sent;
    uint64_t t_end, t_left;
    fd_set sets;

    t_end = utils_time_get_ms() + timeout_ms;
    len_sent = 0;
    err_code = 0;
    ret = 1; //send one time if timeout_ms is value 0

    do {
        t_left = utils_time_left(t_end, utils_time_get_ms());

        if (0 != t_left) {
            struct timeval timeout;

            FD_ZERO( &sets );
            FD_SET(fd, &sets);

            timeout.tv_sec = t_left / 1000;
            timeout.tv_usec = (t_left % 1000) * 1000;

            ret = select(fd + 1, NULL, &sets, NULL, &timeout);
            if (ret > 0) {
               if (0 == FD_ISSET(fd, &sets)) {
                    ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"Should NOT arrive");
                    //If timeout in next loop, it will not sent any data
                    ret = 0;
                    continue;
                }
            } else if (0 == ret) {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"select-write timeout %lu", fd);
                break;
            } else {
                if (EINTR == errno) {
                    ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"EINTR be caught");
                    continue;
                }

                err_code = -1;
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"select-write fail");
                break;
            }
        }

        if (ret > 0) {
            ret = send(fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"No data be sent");
            } else {
                if (EINTR == errno) {
                    ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"EINTR be caught");
                    continue;
                }

                err_code = -1;
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"send fail");
                break;
            }
        }
    } while((len_sent < len) && (utils_time_left(t_end, utils_time_get_ms()) > 0));
#endif
    return len_sent;
}


int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
#if 0
	int err_code = 0;
	uint32_t len_recv;
	len_recv = recv(fd, buf, len, 0);
#else
    int ret, err_code,data_over;
    uint32_t len_recv;
    uint64_t t_end, t_left;
    fd_set sets;
    struct timeval timeout;

    t_end = utils_time_get_ms( ) + timeout_ms ;
    len_recv = 0;
    err_code = 0;

    data_over = 0;

    do {
        t_left = utils_time_left(t_end, utils_time_get_ms());
/*        if (0 == t_left && bk_http_ptr->do_data == 0) {
            break;
        }*/
        FD_ZERO( &sets );
        FD_SET(fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        ret = select(fd + 1, &sets, NULL, NULL, NULL);
        if ( FD_ISSET( fd, &sets ) )
        {
            if (ret > 0) {
                ret = recv(fd, buf, len, 0);
                if (ret > 0) {
                    if(ret < len)
                        {
                        data_over = 1;
                    }
//                    if(bk_http_ptr->do_data == 1)
//                        {
//                        http_data_process(buf,ret);
//                    }

                    len_recv += ret;
                } else if (0 == ret) {
                    ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"connection is closed");
                    err_code = -1;
                    break;
                } else {
                    if (EINTR == errno) {
                        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"EINTR be caught");
                        continue;
                    }
                    ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"send fail");
                    err_code = -2;
                    break;
                }
            } else if (0 == ret) {
                break;
            } else {
                if (EINTR == errno) {
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"EINTR be caught-------\r\n");
                //continue;
                }
                ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"select-recv fail");
                err_code = -2;
                break;
            }
       }
       else
       {
       }
    }while(/*(bk_http_ptr->do_data == 1 && len_recv < bk_http_ptr->http_total) || */((len_recv < len) && (0 == data_over)));
#endif
    //priority to return data bytes if any data be received from TCP connection.
    //It will get error code on next calling
    return (0 != len_recv) ? len_recv : err_code;
}

/*** TCP connection ***/
int read_tcp(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return HAL_TCP_Read(pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return HAL_TCP_Write(pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (0 == pNetwork->handle) {
        return -1;
    }

    HAL_TCP_Destroy(pNetwork->handle);
    pNetwork->handle = 0;
    return 0;
}


static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"network is null");
        return 1;
    }

    pNetwork->handle = HAL_TCP_Establish(pNetwork->pHostAddress, pNetwork->port);
    if (0 == pNetwork->handle) {
        return -1;
    }

    return 0;
}

/****** network interface ******/

int utils_net_read(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return read_tcp(pNetwork, buffer, len, timeout_ms);
    }

    return 0;
}


int utils_net_write(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return write_tcp(pNetwork, buffer, len, timeout_ms);
    }
    return 0;
}


int iotx_net_disconnect(utils_network_pt pNetwork)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return disconnect_tcp(pNetwork);
    }
    return 0;
}


int iotx_net_connect(utils_network_pt pNetwork)
{
    if (NULL == pNetwork->ca_crt) { //TCP connection
        return connect_tcp(pNetwork);
    }
    return 0;
}


int iotx_net_init(utils_network_pt pNetwork, const char *host, uint16_t port, const char *ca_crt)
{
    if (!pNetwork || !host) {
        ADDLOG_ERROR(LOG_FEATURE_HTTP_CLIENT,"parameter error! pNetwork=%p, host = %p", pNetwork, host);
        return -1;
    }
    pNetwork->pHostAddress = host;
    pNetwork->port = port;
    pNetwork->ca_crt = ca_crt;

    if (NULL == ca_crt) {
        pNetwork->ca_crt_len = 0;
    } else {
        pNetwork->ca_crt_len = os_strlen(ca_crt);
    }

    pNetwork->handle = 0;
    pNetwork->doRead = utils_net_read;
    pNetwork->doWrite = utils_net_write;
    pNetwork->doDisconnect = iotx_net_disconnect;
    pNetwork->doConnect = iotx_net_connect;

    return 0;
}

#endif
