/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */


#ifndef __IPERF_UNITS_H
#define __IPERF_UNITS_H

enum {
    UNIT_LEN = 11
};

//double unit_atof( const char *s );
u64 unit_atof( const char *s );

iperf_size_t unit_atoi( const char *s );

void unit_snprintf( char *s, int inLen, double inNum, char inFormat );
//void unit_snprintf( char *s, int inLen, float inNum, char inFormat );

#endif

