/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#ifndef        __IPERF_API_H
#define        __IPERF_API_H

#include <setjmp.h>
#include "iperf.h"


/**
 * exchange_parameters - handles the param_Exchange part for client
 *
 */
int      iperf_exchange_parameters(struct iperf_test * test);

/**
 * add_to_interval_list -- adds new interval to the interval_list
 *
 */
void      add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results *temp);

/**
 * connect_msg -- displays connection message
 * denoting senfer/receiver details
 *
 */
void      connect_msg(struct iperf_stream * sp);

/**
 * iperf_stats_callback -- handles the statistic gathering
 *
 */
void     iperf_stats_callback(struct iperf_test * test);


/**
 * iperf_reporter_callback -- handles the report printing
 *
 */
void     iperf_reporter_callback(struct iperf_test * test);


/**
 * iperf_new_test -- return a new iperf_test with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_test *iperf_new_test(void);

int      iperf_defaults(struct iperf_test * testp);

/**
 * iperf_free_test -- free resources used by test, calls iperf_free_stream to
 * free streams
 *
 */
void      iperf_free_test(struct iperf_test * testp);


/**
 * iperf_new_stream -- return a net iperf_stream with default values
 *
 * returns NULL on failure
 *
 */
struct iperf_stream *iperf_new_stream(struct iperf_test *, int);

/**
 * iperf_add_stream -- add a stream to a test
 *
 */
void      iperf_add_stream(struct iperf_test * test, struct iperf_stream * stream);

/**
 * iperf_init_stream -- init resources associated with test
 *
 */
int       iperf_init_stream(struct iperf_stream *, struct iperf_test *);

/**
 * iperf_free_stream -- free resources associated with test
 *
 */
void      iperf_free_stream(struct iperf_stream * sp);

void get_tcpinfo(struct iperf_stream *, struct iperf_interval_results *);
void print_tcpinfo(struct iperf_test *);
void build_tcpinfo_message(struct iperf_interval_results *r, char *message);

void print_interval_results(struct iperf_test * test, struct iperf_stream *sp);
int iperf_send(struct iperf_test *);
int iperf_recv(struct iperf_test *);
void sig_handler(int);
void usage(void);
void usage_long(void);
void warning(char *);
int all_data_sent(struct iperf_test *);
int package_parameters(struct iperf_test *);
int parse_parameters(struct iperf_test *);
int iperf_exchange_results(struct iperf_test *);
int parse_results(struct iperf_test *, char *);
int iperf_init_test(struct iperf_test *);
int iperf_parse_arguments(struct iperf_test *, int, char **);
void iperf_reset_test(struct iperf_test *);
void cleanup_server(struct iperf_test *test);
void  cleanup_client(struct iperf_test *test);

struct protocol *get_protocol(struct iperf_test *, int);
int set_protocol(struct iperf_test *, int);

void iperf_on_new_stream(struct iperf_stream *);
void iperf_on_test_start(struct iperf_test *);
void iperf_on_connect(struct iperf_test *);
void iperf_on_test_finish(struct iperf_test *);
int iperf_init_test_wm(struct iperf_test *test, struct tht_param*tht);

//extern jmp_buf env;

#endif

