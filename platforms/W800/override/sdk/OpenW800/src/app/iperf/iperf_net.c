/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "iperf_net.h"
#include "iperf_timer.h"
#include "iperf.h"

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "wm_debug.h"
#include "wm_wifi.h"
#include "wm_sockets.h"
#include "wm_osal.h"
#include "wm_netif.h"
/* netdial and netannouce code comes from libtask: http://swtch.com/libtask/
 * Copyright: http://swtch.com/libtask/COPYRIGHT
*/

/* make connection to server */
int
netdial(int domain, int proto, char *local, char *server, int port, int localport)
{
    int s;
    struct addrinfo hints, *res;

    s = socket(domain, proto, 0);
    if (s < 0) {
        return (-1);
    }
    if (local) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = domain;
        hints.ai_socktype = proto;

        // XXX: Check getaddrinfo for errors!
        if (getaddrinfo(local, NULL, &hints, &res) != 0)
            return (-1);
		
#if TLS_IPERF_AUTO_TEST
		struct sockaddr_in localAddr;
		memset(&localAddr, 0, sizeof(struct sockaddr_in));
		localAddr.sin_family = AF_INET;
		localAddr.sin_port = htons(10000+localport);	
		localAddr.sin_addr.s_addr = ipaddr_addr(local);;

		if (bind(s, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) < 0)
	     	return (-1);
#else
        if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0)
            return (-1);
#endif

        freeaddrinfo(res);
    }
	printf("local=%s\n", local);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = domain;
    hints.ai_socktype = proto;
	printf("server: %s\n", server);
    // XXX: Check getaddrinfo for errors!
    if (getaddrinfo(server, NULL, &hints, &res) != 0)
        return (-1);

    ((struct sockaddr_in *) res->ai_addr)->sin_port = htons(port);

    if (connect(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0 && errno != EINPROGRESS) {
        return (-1);
    }

    freeaddrinfo(res);

    return (s);
}

/***************************************************************/

int
netannounce(int domain, int proto, char *local, int port)
{
    int s, opt;
    struct addrinfo hints, *res;
    char portstr[6];
	struct tls_ethif *ethif;
	struct sockaddr_in * sin;

	snprintf(portstr, 6, "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = domain;
	hints.ai_socktype = proto;
   // hints.ai_flags = AI_PASSIVE;
	// XXX: Check getaddrinfo for errors!
	if (getaddrinfo(local, portstr, &hints, &res) != 0)
		return (-1); 
	
	ethif = tls_netif_get_ethif();
	sin = (struct sockaddr_in *) res->ai_addr ;
	MEMCPY((char *)&sin->sin_addr, (char *)ip_2_ip4(&ethif->ip_addr), 4); 


   // s = socket(domain, proto, 0);
    s = socket(res->ai_family, proto, 0);
    if (s < 0) {
        return (-1);
    }
    opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    if (bind(s, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
        close(s);
        return (-1);
    }

    freeaddrinfo(res);
    
    if (proto == SOCK_STREAM) {
        if (listen(s, 5) < 0) {
			close(s);
            return (-1);
        }
    }

    return (s);
}


/*******************************************************************/
/* reads 'count' byptes from a socket  */
/********************************************************************/

int
Nread(int fd, void *buf, int count, int prot)
{
    register int n;
    register int nleft = count;
	char * pbuf = (char *)buf;

    while (nleft > 0) {
        if ((n = read(fd, buf, nleft)) < 0) {
            if (errno == EINTR)
                n = 0;
            else
                return (-1);
        } else if (n == 0)
            break;

        nleft -= n;
        pbuf += n;
    }
    return (count - nleft);
}


/*
 *                      N W R I T E
 *
 * XXX: After updating this function to use read/write, the only difference between
 *      TCP and UDP is that udp handles ENOBUFS. Should we merge the two?
 */

int
Nwrite(int fd, void *buf, int count, int prot)
{
    register int n;
    register int nleft = count;
	char * pbuf = (char *)buf;

    if (prot == SOCK_DGRAM) { /* UDP mode */
        while (nleft > 0) {
            if ((n = write(fd, buf, nleft)) < 0) {
                if (errno == EINTR) {
                    n = 0;
                } else if (errno == ENOBUFS) {
                    /* wait if run out of buffers */
                    /* XXX: but how long to wait? Start shorter and increase delay each time?? */
                    //delay(18000);   // XXX: Fixme!

                   printf("ERROR: Run out of buffer");
				   tls_os_time_delay(2);//delay 20ms
                   // delay(18);
                    n = 0;
                } else {
                    return (-1);
                }
            }
            nleft -= n;
            pbuf += n;
        }
    } else {
        while (nleft > 0) {
            if ((n = write(fd, buf, nleft)) < 0) {
                if (errno == EINTR)
                    n = 0;
                else
                    return (-1);
            }
            nleft -= n;
            pbuf += n;
        }
    }
    return (count);
}

/*************************************************************************/

/**
 * getsock_tcp_mss - Returns the MSS size for TCP
 *
 */
#if 0
int
getsock_tcp_mss(int inSock)
{
    int             mss = 0;

    int             rc;
    socklen_t       len;

    //assert(inSock >= 0); /* print error and exit if this is not true */

    /* query for mss */
    len = sizeof(mss);
    rc = getsockopt(inSock, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, &len);

    return mss;
}
#endif


/*************************************************************/

/* sets TCP_NODELAY and TCP_MAXSEG if requested */
// XXX: This function is not being used.
#if 0
int
set_tcp_options(int sock, int no_delay, int mss)
{

    socklen_t       len;

    if (no_delay == 1) {
        int             no_delay = 1;

        len = sizeof(no_delay);
        int             rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
                             (char *)&no_delay, len);

        if (rc == -1) {
            printf("TCP_NODELAY");
            return -1;
        }
    }

    if (mss > 0) {
        int             rc;
        int             new_mss;

        len = sizeof(new_mss);

        assert(sock != -1);

        /* set */
        new_mss = mss;
        len = sizeof(new_mss);
        rc = setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, len);
        if (rc == -1) {
            printf("setsockopt");
            return -1;
        }
        /* verify results */
        rc = getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&new_mss, &len);
        if (new_mss != mss) {
            printf("setsockopt value mismatch");
            return -1;
        }
    }

    return 0;
}
#endif

/****************************************************************************/

// XXX: This function is not being used.
int
setnonblocking(int sock)
{
    int       opts = 0;

    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        printf("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

