/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef __IPERF_H
#define __IPERF_H

#define TLS_CONFIG_WIFI_PERF_TEST			CFG_ON
#define TLS_IPERF_AUTO_TEST 				CFG_OFF

#define TLS_MSG_WIFI_PERF_TEST_START 		1


#include <stdint.h>

#include "iperf_queue.h"
#include "wm_sockets.h"

//typedef u64 iperf_size_t;
typedef s64 iperf_size_t;
//typedef int iperf_size_t;

struct tht_param_1{
	char role;//'s', 'c'
	
	char     *server_hostname; //192.168.1.100
	char	    *protocol;//TCP, UDP
	char	    *report_interval;//-i 10
	char     *duration;//-t 10
	char     *bandwidth; //-b 10k UDP
	char     *socket_size; //-l 1024 TCP
	
};

struct tht_param{
	char 	role;//'s', 'c'
	char     server_hostname[32]; //192.168.1.100
	int 		protocol;//TCP, UDP
	int 		report_interval;//-i 10
	int 		duration;//-t 10
	u64 	 	rate; //-b 10k UDP
	int 		block_size; //for TCP -l 1024 	

	int 		port;
	int 		localport;
};
/*
//AT+THT=Ss,TCP, -i=1
//AT+THT=Ss,UDP,-i=1

AT+THT=Ss,-i=1
AT+THT=Ss

AT+THT=Cc,192.168.1.100, UDP, -b=10k,-t=10,-i=1
-b=0: full speed test

AT+THT=Cc,192.168.1.100, TCP, -l=1024,-t=10,-i=1
*/
struct iperf_interval_results
{
    iperf_size_t bytes_transferred; /* bytes transfered in this interval */
    struct timeval interval_start_time;
    struct timeval interval_end_time;
    //float     interval_duration;
    int interval_duration;
#if defined(linux) || defined(__FreeBSD__)
    struct tcp_info tcpInfo;	/* getsockopt(TCP_INFO) results here for
                                 * Linux and FreeBSD stored here */
#else
    char *tcpInfo;	/* just a placeholder */
#endif
    struct iperf_interval_results *next;
    void     *custom_data;
};

struct iperf_stream_result
{
    iperf_size_t bytes_received;
    iperf_size_t bytes_sent;
    iperf_size_t bytes_received_this_interval;
    iperf_size_t bytes_sent_this_interval;
	int interval_list_flag;
    struct timeval start_time;
    struct timeval end_time;
    struct iperf_interval_results *interval_results;      // head of list
    struct iperf_interval_results *last_interval_results; // end of list
    void     *data;
};

#define COOKIE_SIZE 37              /* size of an ascii uuid */
struct iperf_settings
{
    int       domain;               /* AF_INET or AF_INET6 */
    int       socket_bufsize;       /* window size for TCP */
    int       blksize;              /* size of read/writes (-l) */
    u64  rate;                 /* target data rate, UDP only */
    int       mss;                  /* for TCP MSS */
    int       ttl;                  /* IP TTL option */
    int       tos;                  /* type of service bit */
    //iperf_size_t bytes;             /* number of bytes to send */
    u64 bytes;             /* number of bytes to send */
    char      unit_format;          /* -f */
};

struct iperf_stream
{
    /* configurable members */
    int       local_port;
    int       remote_port;
    int       socket;
    int       id;
	/* XXX: is settings just a pointer to the same struct in iperf_test? if not, 
		should it be? */
    struct iperf_settings *settings;	/* pointer to structure settings */

    /* non configurable members */
    struct iperf_stream_result *result;	/* structure pointer to result */
    struct timer *send_timer;
    char      *buffer;		/* data to send */

    /*
     * for udp measurements - This can be a structure outside stream, and
     * stream can have a pointer to this
     */
    int       packet_count;
//    double    jitter;
    float	  jitter;
    double    prev_transit;
    int       outoforder_packets;
    int       cnt_error;
    u64  target;

    struct sockaddr local_addr;
    struct sockaddr remote_addr;

    int       (*rcv) (struct iperf_stream * stream);
    int       (*snd) (struct iperf_stream * stream);

//    struct iperf_stream *next;
    SLIST_ENTRY(iperf_stream) streams;

    void     *data;
};

struct iperf_test;

struct protocol {
    int       id;
    char      *name;
    int       (*accept)(struct iperf_test *);
    int       (*listen)(struct iperf_test *);
    int       (*connect)(struct iperf_test *);
    int       (*send)(struct iperf_stream *);
    int       (*recv)(struct iperf_stream *);
    int       (*init)(struct iperf_test *);
    SLIST_ENTRY(protocol) protocols;
};

struct iperf_test
{
    char      role;                             /* c' lient or 's' erver */
    struct protocol *protocol;
    char      state;
    char     *server_hostname;                  /* -c option */
    char     *bind_address;                     /* -B option */
    int       server_port;
    int       duration;                         /* total duration of test (-t flag) */

	int 	  local_port;

    int       ctrl_sck;
    int       listener;
    int       prot_listener;

    /* boolean variables for Options */
    int       daemon;                           /* -D option */
    int	      debug;                            /* -d option - debug mode */
    int       no_delay;                         /* -N option */
    int       output_format;                    /* -O option */
    int       reverse;                          /* -R option */
    int       tcp_info;                         /* -T option - display getsockopt(TCP_INFO) results. */
    int       v6domain;                         /* -6 option */
    int	      verbose;                          /* -V option - verbose mode */

    /* Select related parameters */
    int       max_fd;
    fd_set    read_set;                         /* set of read sockets */
    fd_set    write_set;                        /* set of write sockets */

    /* Interval related members */ 
    //double    stats_interval;
    int    stats_interval;
    //double    reporter_interval;
    int    reporter_interval;
	
    void      (*stats_callback) (struct iperf_test *);
    void      (*reporter_callback) (struct iperf_test *);
    struct timer *timer;
    struct timer *stats_timer;
    struct timer *reporter_timer;

    double cpu_util;                            /* cpu utilization of the test */
    double remote_cpu_util;                     /* cpu utilization for the remote host/client */

    int       num_streams;                      /* total streams in the test (-P) */

    iperf_size_t bytes_sent;
    char      cookie[COOKIE_SIZE];
//    struct iperf_stream *streams;               /* pointer to list of struct stream */
    SLIST_HEAD(slisthead, iperf_stream) streams;
    struct iperf_settings *settings;

    SLIST_HEAD(plisthead, protocol) protocols;

    /* callback functions */
    void      (*on_new_stream)(struct iperf_stream *);
    void      (*on_test_start)(struct iperf_test *);
    void      (*on_connect)(struct iperf_test *);
    void      (*on_test_finish)(struct iperf_test *);
};

enum
{
    /* default settings */
    Ptcp = SOCK_STREAM,
    Pudp = SOCK_DGRAM,
    PORT = 5201,  /* default port to listen on (don't use the same port as iperf2) */
    uS_TO_NS = 1000,
    SEC_TO_US = 1000000,
    RATE = 1024 * 1024, /* 1 Mbps */
    DURATION = 5, /* seconds */
    DEFAULT_UDP_BLKSIZE = 1450, /* 1 packet per ethernet frame, IPV6 too */
    DEFAULT_TCP_BLKSIZE =  2 * 1024,  //128 * 1024,  /* default read/write block size */

    /* other useful constants */
    TEST_START = 1,
    TEST_RUNNING = 2,
    RESULT_REQUEST = 3,
    TEST_END = 4,
    STREAM_BEGIN = 5,
    STREAM_RUNNING = 6,
    STREAM_END = 7,
    ALL_STREAMS_END = 8,
    PARAM_EXCHANGE = 9,
    CREATE_STREAMS = 10,
    SERVER_TERMINATE = 11,
    CLIENT_TERMINATE = 12,
    EXCHANGE_RESULTS = 13,
    DISPLAY_RESULTS = 14,
    IPERF_START = 15,
    IPERF_DONE = 16,
    ACCESS_DENIED = -1,
    SERVER_ERROR = -2,
};

#define SEC_TO_NS 1000000000	/* too big for enum on some platforms */
#define MAX_RESULT_STRING 4096

/* constants for command line arg sanity checks
*/
#define MB 1024 * 1024
#define MAX_TCP_BUFFER 128 * MB
#define MAX_BLOCKSIZE MB
#define MAX_INTERVAL 60
#define MAX_TIME 3600
#define MAX_MSS 9 * 1024
#define MAX_STREAMS 128

#endif

