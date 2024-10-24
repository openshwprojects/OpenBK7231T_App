/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

/* iperf_util.c
 *
 * Iperf utility functions
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "config.h"
#include "wm_sockets.h"
#include "lwip/arch.h"
#include "iperf_timer.h"
/* XXX: this code is not portable: not all versions of linux install libuuidgen
 *      by default
 *
 *   if not installed, may need to do something like this:
 *     yum install libuuid-devel
 *     apt-get install uuid-dev
 */
#if 0
#if defined(HAVE_UUID_H)
#warning DOING SOMETHING
#include <uuid.h>
#elif defined(HAVE_UUID_UUID_H)
#include <uuid/uuid.h>
#else
#error No uuid header file specified
#endif

#endif

/* get_uuid
 *
 * Generate and return a UUID string
 *
 * Iperf uses this function to create test "cookies" which
 * server as unique test identifiers. These cookies are also
 * used for the authentication of stream connections.
 */
	
	/* make_cookie
	 *
	 * Generate and return a cookie string
	 *
	 * Iperf uses this function to create test "cookies" which
	 * server as unique test identifiers. These cookies are also
	 * used for the authentication of stream connections.
	 */
	
	void
	make_cookie(char *cookie)
	{
	 //   static int randomized = 0;
	//	  char hostname[500];
		struct timeval tv;
		char temp[100];
	
		//if ( ! randomized )
		//	  srandom((int) time(0) ^ getpid());
	
		/* Generate a string based on hostname, time, randomness, and filler. */
		//(void) gethostname(hostname, sizeof(hostname));
		iperf_gettimeofday(&tv, NULL);
//		(void) os_snprintf(temp, sizeof(temp), "%ld.%06ld.%s", (unsigned long int) tv.tv_sec, (unsigned long int) tv.tv_usec,  "1234567890123456789012345678901234567890");
		snprintf(temp, sizeof(temp), "%ld.%06ld.%s", (unsigned long int) tv.tv_sec, (unsigned long int) tv.tv_usec,  "1234567890123456789012345678901234567890");
		
		/* Now truncate it to 36 bytes and terminate. */
		memcpy(cookie, temp, 36);
		cookie[36] = '\0';
	}
#if 0
void
get_uuid(char *temp)
{
    char     *s;
    uuid_t    uu;

#if defined(HAVE_UUID_CREATE)
    uuid_create(&uu, NULL);
    uuid_to_string(&uu, &s, 0);
#elif defined(HAVE_UUID_GENERATE)
    s = (char *) malloc(37);
    uuid_generate(uu);
    uuid_unparse(uu, s);
#else
#error No uuid function specified
#endif
    memcpy(temp, s, 37);

    // XXX: Freeing s only works if you HAVE_UUID_GENERATE
    //      Otherwise use rpc_string_free (?)
    free(s);
}

#endif
/* is_closed
 *
 * Test if the file descriptor fd is closed.
 * 
 * Iperf uses this function to test whether a TCP stream socket
 * is closed, because accepting and denying an invalid connection
 * in iperf_tcp_accept is not considered an error.
 */

int
is_closed(int fd)
{
    struct timeval tv;
    fd_set readset;

    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (select(fd+1, &readset, NULL, NULL, &tv) < 0) {
        if (errno == EBADF)
            return (1);
    }
    return (0);
}
