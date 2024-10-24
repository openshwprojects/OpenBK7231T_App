/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

/* iperf_server_api.c: Functions to be used by an iperf server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "iperf.h"
#include "iperf_server_api.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "iperf_error.h"
#include "iperf_util.h"
#include "iperf_timer.h"
#include "iperf_net.h"
#include "iperf_units.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "iperf_locale.h"


#include "lwip/arch.h"

#include "wm_sockets.h"
#include "lwip/sockets.h"
#include "wm_debug.h"
extern int exit_last_test;


int
iperf_server_listen(struct iperf_test *test)
{
    //char ubuf[UNIT_LEN];
    //int x;

    if((test->listener = netannounce(test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
        i_errno = IELISTEN;
        return (-1);
    }

    printf("-----------------------------------------------------------\n");
    printf("Server listening on %d\n", test->server_port);

    // This needs to be changed to reflect if client has different window size
    // make sure we got what we asked for
    /* XXX: This needs to be moved to the stream listener
    if ((x = get_tcp_windowsize(test->listener, SO_RCVBUF)) < 0) {
        // Needs to set some sort of error number/message
        perror("SO_RCVBUF");
        return -1;
    }
    */

    // XXX: This code needs to be moved to after parameter exhange
    /*
    if (test->protocol->id == Ptcp) {
        if (test->settings->socket_bufsize > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) x, 'A');
            printf("TCP window size: %s\n", ubuf);
        } else {
            printf("Using TCP Autotuning\n");
        }
    }
    */
    printf("-----------------------------------------------------------\n");

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = (test->listener > test->max_fd) ? test->listener : test->max_fd;
	IPF_DBG(" maxfd: %d\n", test->max_fd);

    return 0;
}

int
iperf_accept(struct iperf_test *test)
{
    int s;
    int rbuf = ACCESS_DENIED;
    char cookie[COOKIE_SIZE];
    socklen_t len;
    struct sockaddr_in addr;

	IPF_DBG("ctrl_sck = %d", test->ctrl_sck);

    len = sizeof(addr);
    if ((s = accept(test->listener, (struct sockaddr *) &addr, &len)) < 0) {
        i_errno = IEACCEPT;
        return (-1);
    }

    if (test->ctrl_sck == -1) {
        /* Server free, accept new client */
        if (Nread(s, test->cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return (-1);
        }

        FD_SET(s, &test->read_set);
        FD_SET(s, &test->write_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
		
		IPF_DBG("maxfd: %d\n", test->max_fd);
        test->ctrl_sck = s;

		IPF_DBG("iperf_set_send_state\n");
        test->state = PARAM_EXCHANGE;
        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }

		IPF_DBG("iperf_exchange_parameters\n");
        if (iperf_exchange_parameters(test) < 0) {
            return (-1);
        }

		
        if (test->on_connect) {
            test->on_connect(test);
        }
    } else {
        // XXX: Do we even need to receive cookie if we're just going to deny anyways?
        if (Nread(s, cookie, COOKIE_SIZE, Ptcp) < 0) {
            i_errno = IERECVCOOKIE;
            return (-1);
        }
        if (Nwrite(s, &rbuf, sizeof(int), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }
        close(s);
    }

    return (0);
}


/**************************************************************************/
int
iperf_handle_message_server(struct iperf_test *test)
{
    int rval;
    struct iperf_stream *sp;

    // XXX: Need to rethink how this behaves to fit API
    IPF_DBG("test->state: %d\n", test->state);
    if ((rval = Nread(test->ctrl_sck, &test->state, sizeof(char), Ptcp)) <= 0) {
        if (rval == 0) {
            printf( "The client has unexpectedly closed the connection.\n");
            i_errno = IECTRLCLOSE;
            test->state = IPERF_DONE;
            return (0);
        } else {
            i_errno = IERECVMESSAGE;
            return (-1);
        }
    }

    switch(test->state) {
        case TEST_START:
            break;
        case TEST_END:
            cpu_util(&test->cpu_util);
            test->stats_callback(test);
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = EXCHANGE_RESULTS;
            if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            if (iperf_exchange_results(test) < 0)
                return (-1);
            test->state = DISPLAY_RESULTS;
            if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            if (test->on_test_finish)
                test->on_test_finish(test);
            test->reporter_callback(test);
            break;
        case IPERF_DONE:
            break;
        case CLIENT_TERMINATE:
            i_errno = IECLIENTTERM;

            // XXX: Remove this line below!
            printf( "The client has terminated.\n");
            SLIST_FOREACH(sp, &test->streams, streams) {
                FD_CLR(sp->socket, &test->read_set);
                FD_CLR(sp->socket, &test->write_set);
                close(sp->socket);
            }
            test->state = IPERF_DONE;
            break;
        default:
            i_errno = IEMESSAGE;
            return (-1);
    }

    return (0);
}

/* XXX: This function is not used anymore */
void
iperf_test_reset(struct iperf_test *test)
{
    struct iperf_stream *sp;

    close(test->ctrl_sck);

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);
        iperf_free_stream(sp);
    }
    free_timer(test->timer);
    free_timer(test->stats_timer);
    free_timer(test->reporter_timer);
    test->timer = NULL;
    test->stats_timer = NULL;
    test->reporter_timer = NULL;

    SLIST_INIT(&test->streams);

    test->role = 's';
    set_protocol(test, Ptcp);
    test->duration = DURATION;
    test->state = 0;
    test->server_hostname = NULL;

    test->ctrl_sck = -1;
    test->prot_listener = -1;

    test->bytes_sent = 0;

    test->reverse = 0;
    test->no_delay = 0;

    FD_ZERO(&test->read_set);
    FD_ZERO(&test->write_set);
    FD_SET(test->listener, &test->read_set);
    test->max_fd = test->listener;
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = RATE;   /* UDP only */
    test->settings->mss = 0;
    memset(test->cookie, 0, COOKIE_SIZE); 
}

int
iperf_run_server(struct iperf_test *test)
{
    int result, s, streams_accepted;
    fd_set temp_read_set, temp_write_set;
    struct iperf_stream *sp;
    struct timeval tv;
    time_t sec, usec;

    // Open socket and listen
    if (iperf_server_listen(test) < 0) {
        return (-1);
    }

    // Begin calculating CPU utilization
    cpu_util(NULL);

    test->state = IPERF_START;
    streams_accepted = 0;

    while (test->state != IPERF_DONE) {
		IPF_DBG("test->state = %d\n", test->state);
        memcpy(&temp_read_set, &test->read_set, sizeof(fd_set));
        memcpy(&temp_write_set, &test->write_set, sizeof(fd_set));
        tv.tv_sec = 15;
        tv.tv_usec = 0;

        result = select(test->max_fd + 1, &temp_read_set, &temp_write_set, NULL, &tv);
        if ((result == 0) && exit_last_test)
        {
            break;
        }
        if (result < 0 && errno != EINTR) {
            i_errno = IESELECT;
            return (-1);
        } else if (result > 0) {
        	IPF_DBG("select result: %d\n", result);
            if (FD_ISSET(test->listener, &temp_read_set)) {
                if (test->state != CREATE_STREAMS) {
                    if (iperf_accept(test) < 0) {
						FD_CLR(test->listener, &temp_read_set);
                        cleanup_server(test);
                        return (-1);
                    }
                    FD_CLR(test->listener, &temp_read_set);
                }
            }
            if (FD_ISSET(test->ctrl_sck, &temp_read_set)) {
                if (iperf_handle_message_server(test) < 0){
					FD_CLR(test->ctrl_sck, &temp_read_set);  
                    cleanup_server(test);
                    return (-1);
                }
                FD_CLR(test->ctrl_sck, &temp_read_set);                
            }

            if (test->state == CREATE_STREAMS) {
                if (FD_ISSET(test->prot_listener, &temp_read_set)) {
    				IPF_DBG("CREATE_STREAMS  \n");
                    if ((s = test->protocol->accept(test)) < 0){
						FD_CLR(test->prot_listener, &temp_read_set);
						cleanup_server(test);
						return (-1);
                    }
					IPF_DBG("protocol->accept s= %d, test->max_fd=%d\n", s, test->max_fd);
                    if (!is_closed(s)) {
						IPF_DBG("!is_closed\n");
                        sp = iperf_new_stream(test, s);
                        if (!sp){
							FD_CLR(test->prot_listener, &temp_read_set);
							cleanup_server(test);
                            return (-1);
                        }
                        FD_SET(s, &test->read_set);
                        FD_SET(s, &test->write_set);
                        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
						IPF_DBG("protocol->accept s= %d, test->max_fd=%d\n", s, test->max_fd);

                        streams_accepted++;
                        if (test->on_new_stream)
                            test->on_new_stream(sp);
                    }
                    FD_CLR(test->prot_listener, &temp_read_set);
                }

				IPF_DBG("===>test->ctrl_sck = %d\n", test->ctrl_sck);
				IPF_DBG("===>test->listener = %d\n", test->listener);
				IPF_DBG("===>test->prot_listener = %d\n", test->prot_listener);

                if (streams_accepted == test->num_streams) {
					
                    if (test->protocol->id != Ptcp) {
						IPF_DBG("protocol->id != Ptcp\n");
                        FD_CLR(test->prot_listener, &test->read_set);
                        close(test->prot_listener);
						
                    } else { 
                        if (test->no_delay || test->settings->mss || test->settings->socket_bufsize) {
                            FD_CLR(test->listener, &test->read_set);
							IPF_DBG("close(test->listener) %d ", test->listener);
                            close(test->listener);
                            if ((s = netannounce(test->settings->domain, Ptcp, test->bind_address, test->server_port)) < 0) {
                                i_errno = IELISTEN;
								cleanup_server(test);
                                return (-1);
                            }
                            test->listener = s;
                            test->max_fd = (s > test->max_fd ? s : test->max_fd);
                            FD_SET(test->listener, &test->read_set);
							
							IPF_DBG(" maxfd: %d\n", test->max_fd);
                        }
                    }
                    test->prot_listener = -1;
                    test->state = TEST_START;
					IPF_DBG("TEST_START\n");
					
                    if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                        i_errno = IESENDMESSAGE;
						cleanup_server(test);
                        return (-1);
                    }
                    if (iperf_init_test(test) < 0){
						cleanup_server(test);
						return (-1);
                   	}
                    test->state = TEST_RUNNING;
					IPF_DBG("TEST_RUNNING");
                    if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
                        i_errno = IESENDMESSAGE;
						cleanup_server(test);
                        return (-1);
                    }
                }
            }

            if (test->state == TEST_RUNNING) {
                if (test->reverse) {
                    // Reverse mode. Server sends.
                    if (iperf_send(test) < 0){
						cleanup_server(test);
                        return (-1);
                    }
                } else {
                    // Regular mode. Server receives.
                    if (iperf_recv(test) < 0){
						cleanup_server(test);
                        return (-1);
                    }
                }

                /* Perform callbacks */
                if (timer_expired(test->stats_timer)) {
                    test->stats_callback(test);
                    sec = (time_t) test->stats_interval;
                    usec = (test->stats_interval - sec) * SEC_TO_US;
                    if (update_timer(test->stats_timer, sec, usec) < 0){
						cleanup_server(test);
                        return (-1);
                    }
                }
                if (timer_expired(test->reporter_timer)) {
                    test->reporter_callback(test);
                    sec = (time_t) test->reporter_interval;
                    usec = (test->reporter_interval - sec) * SEC_TO_US;
                    if (update_timer(test->reporter_timer, sec, usec) < 0){
						cleanup_server(test);
                        return (-1);
                    }
                }
            }
        }
    }

    /* Close open test sockets */
	cleanup_server(test);

    return (0);
}

