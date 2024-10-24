/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "iperf_net.h"
#include "iperf.h"
#include "iperf_api.h"
#include "iperf_udp.h"
#include "iperf_tcp.h"
#include "iperf_error.h"
#include "iperf_timer.h"

#include "iperf_units.h"
#include "tcp_window_size.h"
#include "iperf_util.h"
#include "iperf_locale.h"

#include "wm_socket.h"
#include "lwip/sockets.h"

#include "iperf_error.h"


/******************************************************************************/
void
iperf_set_test_role(struct iperf_test *ipt, char role)
{

	if((role != 'c') && (role != 's')){
		i_errno = IESERVCLIENT;
		usage_long();
		return ;
	}
	ipt->role = role;

}
#define UDP_RATE_1M (1024 * 1024) /* 1 Mbps */

int
iperf_init_test_wm(struct iperf_test *test, struct tht_param*tht)
{
	int blksize = 0;
	
	iperf_set_test_role(test, tht->role);
	
	set_protocol(test, tht->protocol);

#if TLS_IPERF_AUTO_TEST
	test->server_port = tht->port;
	test->local_port = tht->localport;
#endif

	if(test->role == 's')
		test->settings->rate = test->protocol->id == Pudp ? UDP_RATE_1M : 0;

	if(test->role == 'c'){
		/*server ip addr*/
		test->server_hostname = (char *) tls_mem_alloc(strlen(tht->server_hostname)+1);
		strncpy(test->server_hostname, tht->server_hostname, strlen(tht->server_hostname)+1);

		/*server port, setting as default*/
#if !TLS_IPERF_AUTO_TEST
		test->server_port = PORT;
#endif

		if(tht->protocol == Pudp)
			test->settings->rate = tht->rate; //UDP_RATE_1M; 

	}
#if 0//WIFI_PERF_TEST_SERVER /* as server*/
	//server or client
	iperf_set_test_role(test, 's');

	
	#if 0
	//udp
	set_protocol(test, Pudp);

	
	//bandwidth
	test->settings->rate = test->protocol->id == Pudp ? UDP_RATE_1M : 0;
	#else 
	set_protocol(test, Ptcp);
	#endif
	
	//test time
	test->duration = MAX_TIME;
	
#endif
#if 0	/* as client*/
	char* server_hostname = "192.168.1.101";
	iperf_set_test_role(test, 'c');

	/*server ip addr*/
	test->server_hostname = os_strdup(server_hostname);

	/*server port, setting as default*/
	test->server_port = PORT;

	test->duration = 10;//DURATION;
#if 0/* UDP */
	/*set UDP test; iperf defalt is TCP*/
	set_protocol(test, Pudp);
	//test->settings->blksize = DEFAULT_UDP_BLKSIZE;
	test->settings->rate = 10*1024*1024; //UDP_RATE_1M; 
#else
	/*set TCP test */
	set_protocol(test, Ptcp);
#endif
#endif

	/*test time for one time*/
	test->duration = tht->duration;

	/*setting for report time*/
	test->stats_interval = test->reporter_interval = tht->report_interval;

	/*set blksize for both UDP & TCP*/
	blksize = tht->block_size;
	if (blksize == 0) {
		if (test->protocol->id == Pudp)
			blksize = DEFAULT_UDP_BLKSIZE;
		else
			blksize = DEFAULT_TCP_BLKSIZE;
	}
	if (blksize <= 0 || blksize > MAX_BLOCKSIZE) {
		//i_errno = IEBLOCKSIZE;
		return -1;
	}
	test->settings->blksize = blksize;
	return 0;
}

/*************************** Print usage functions ****************************/

void
usage(void)
{
    printf(usage_short);
}


void
usage_long(void)
{
    printf(usage_long1);
    printf(usage_long2);
}


void warning(char *str)
{
    printf("warning: %s\n", str);
}


/********************** Get/set test protocol structure ***********************/

struct protocol *
get_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id)
            break;
    }

    if (prot == NULL)
        i_errno = IEPROTOCOL;

    return (prot);
}

int
set_protocol(struct iperf_test *test, int prot_id)
{
    struct protocol *prot = NULL;

    SLIST_FOREACH(prot, &test->protocols, protocols) {
        if (prot->id == prot_id) {
            test->protocol = prot;
            return (0);
        }
    }

    i_errno = IEPROTOCOL;

    return (-1);
}


/************************** Iperf callback functions **************************/

void
iperf_on_new_stream(struct iperf_stream *sp)
{
    connect_msg(sp);
}

void
iperf_on_test_start(struct iperf_test *test)
{
    //if (test->verbose) {
        if (test->settings->bytes) {
            printf(test_start_bytes, test->protocol->name, test->num_streams,
                test->settings->blksize, test->settings->bytes);
        } else {
            printf(test_start_time, test->protocol->name, test->num_streams,
                test->settings->blksize, test->duration);
        }
    //}
}

/** const char *
* inet_ntop4(src, dst, size)
*	format an IPv4 address, more or less like inet_ntoa()
* return:
*	`dst' (as a const)
* notes:
*	(1) uses no statics
*	(2) takes a u_char* not an in_addr as input
*/
static const char *
iperf_inet_ntop4(const u8 *src, char *dst, socklen_t size)
{
	static const char fmt[] = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];
	sprintf(tmp, fmt, src[0], src[1], src[2], src[3]);
	if ((socklen_t)strlen(tmp) > size) {
		errno = ENOSPC;
		return (NULL);
	}
	strcpy(dst, tmp);
	return (dst);
}

/** char *
 * inet_ntop(af, src, dst, size)
 *  convert a network format address to presentation format.
 * return:
 *  pointer to presentation format address (`dst'), or NULL (see errno).
 */
static const char *
iperf_inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    switch (af) {
    case AF_INET:
        return (iperf_inet_ntop4(src, dst, size));
    default:
        errno = EAFNOSUPPORT;
        return (NULL);
    }
    /** NOTREACHED */
}
 

void
iperf_on_connect(struct iperf_test *test)
{
    char ipr[48];
    struct sockaddr temp;
    socklen_t len;
    int domain;

    if (test->role == 'c') {
        printf("Connecting to host %s, port %d\n", test->server_hostname,
            test->server_port);
    } else {
        domain = test->settings->domain;
        len = sizeof(temp);
        getpeername(test->ctrl_sck, (struct sockaddr *) &temp, &len);
        if (domain == AF_INET) {
            iperf_inet_ntop(domain, &((struct sockaddr_in *) &temp)->sin_addr, ipr, sizeof(ipr));
            printf("Accepted connection from %s, port %d\n", ipr, ntohs(((struct sockaddr_in *) &temp)->sin_port));
        }
    }
	
    //if (test->verbose) {
        printf("      Cookie: %s\n", test->cookie);
        #if 0
		if (test->protocol->id == SOCK_STREAM) {
            if (test->settings->mss) {
                printf("      TCP MSS: %d\n", test->settings->mss);
            } else {
                len = sizeof(opt);
                getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len);
                printf("      TCP MSS: %d (default)\n", opt);
            }
        }
		#endif
    //}
	
}

void
iperf_on_test_finish(struct iperf_test *test)
{
    //if (test->verbose) {
        printf("Test Complete. Summary Results:\n");
    //}
}


/******************************************************************************/
#if 0
int
iperf_parse_arguments(struct iperf_test *test, int argc, char **argv)
{
    static struct option longopts[] =
    {
        {"client", required_argument, NULL, 'c'},
        {"server", no_argument, NULL, 's'},
        {"time", required_argument, NULL, 't'},
        {"port", required_argument, NULL, 'p'},
        {"parallel", required_argument, NULL, 'P'},
        {"udp", no_argument, NULL, 'u'},
        {"bind", required_argument, NULL, 'B'},
        {"tcpInfo", no_argument, NULL, 'T'},
        {"bandwidth", required_argument, NULL, 'b'},
        {"length", required_argument, NULL, 'l'},
        {"window", required_argument, NULL, 'w'},
        {"interval", required_argument, NULL, 'i'},
        {"bytes", required_argument, NULL, 'n'},
        {"NoDelay", no_argument, NULL, 'N'},
        {"Set-mss", required_argument, NULL, 'M'},
        {"version", no_argument, NULL, 'v'},
        {"verbose", no_argument, NULL, 'V'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"daemon", no_argument, NULL, 'D'},
        {"format", required_argument, NULL, 'f'},
        {"reverse", no_argument, NULL, 'R'},
        {"version6", no_argument, NULL, '6'},

    /*  XXX: The following ifdef needs to be split up. linux-congestion is not necessarily supported
     *  by systems that support tos.
     */
#ifdef ADD_WHEN_SUPPORTED
        {"tos",        required_argument, NULL, 'S'},
        {"linux-congestion", required_argument, NULL, 'Z'},
#endif
        {NULL, 0, NULL, 0}
    };
    char ch;

    while ((ch = getopt_long(argc, argv, "c:p:st:uP:B:b:l:w:i:n:RS:NTvh6VdM:f:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'c':
                if (test->role == 's') {
                    i_errno = IESERVCLIENT;
                    return (-1);
                } else {
                    test->role = 'c';
                    test->server_hostname = (char *) malloc(strlen(optarg)+1);
                    strncpy(test->server_hostname, optarg, strlen(optarg)+1);
                }
                break;
            case 'p':
                test->server_port = atoi(optarg);
                break;
            case 's':
                if (test->role == 'c') {
                    i_errno = IESERVCLIENT;
                    return (-1);
                } else {
                    test->role = 's';
                }
                break;
            case 't':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->duration = atoi(optarg);
                if (test->duration > MAX_TIME) {
                    i_errno = IEDURATION;
                    return (-1);
                }
                break;
            case 'u':
                if (test->role == 's') {
                    warning("ignoring client only argument --udp (-u)");
                /* XXX: made a warning
                    i_errno = IECLIENTONLY;
                    return (-1);
                */
                }
                set_protocol(test, Pudp);
                test->settings->blksize = DEFAULT_UDP_BLKSIZE;
                break;
            case 'P':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->num_streams = atoi(optarg);
                if (test->num_streams > MAX_STREAMS) {
                    i_errno = IENUMSTREAMS;
                    return (-1);
                }
                break;
            case 'B':
                test->bind_address = (char *) malloc(strlen(optarg)+1);
                strncpy(test->bind_address, optarg, strlen(optarg)+1);
                break;
            case 'b':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->rate = unit_atof(optarg);
                break;
            case 'l':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->blksize = unit_atoi(optarg);
                if (test->settings->blksize > MAX_BLOCKSIZE) {
                    i_errno = IEBLOCKSIZE;
                    return (-1);
                }
                break;
            case 'w':
                // XXX: This is a socket buffer, not specific to TCP
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->socket_bufsize = unit_atof(optarg);
                if (test->settings->socket_bufsize > MAX_TCP_BUFFER) {
                    i_errno = IEBUFSIZE;
                    return (-1);
                }
                break;
            case 'i':
                /* XXX: could potentially want separate stat collection and reporting intervals,
                   but just set them to be the same for now */
                test->stats_interval = atof(optarg);
                test->reporter_interval = atof(optarg);
                if (test->stats_interval > MAX_INTERVAL) {
                    i_errno = IEINTERVAL;
                    return (-1);
                }
                break;
            case 'n':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->bytes = unit_atoi(optarg);
                break;
            case 'N':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->no_delay = 1;
                break;
            case 'M':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->settings->mss = atoi(optarg);
                if (test->settings->mss > MAX_MSS) {
                    i_errno = IEMSS;
                    return (-1);
                }
                break;
            case 'f':
                test->settings->unit_format = *optarg;
                break;
            case 'T':
#if !defined(linux) && !defined(__FreeBSD__)
                // XXX: Should check to make sure UDP mode isn't set!
                warning("TCP_INFO (-T) is not supported on your current platform");
#else
                test->tcp_info = 1;
#endif
                break;
            case '6':
                test->settings->domain = AF_INET6;
                break;
            case 'V':
                test->verbose = 1;
                break;
            case 'd':
                test->debug = 1;
                break;
            case 'R':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                test->reverse = 1;
                break;
            case 'S':
                if (test->role == 's') {
                    i_errno = IECLIENTONLY;
                    return (-1);
                }
                // XXX: Checking for errors in strtol is not portable. Leave as is?
                test->settings->tos = strtol(optarg, NULL, 0);
                break;
            case 'v':
                printf(version);
                exit(0);
            case 'h':
            default:
                usage_long();
                exit(1);
        }
    }
    /* For subsequent calls to getopt */
#ifdef __APPLE__
    optreset = 1;
#endif
    optind = 0;

    if ((test->role != 'c') && (test->role != 's')) {
        i_errno = IENOROLE;
        return (-1);
    }

    return (0);
}
#endif



void
cleanup_client(struct iperf_test *test)
{
	iperf_free_test(test);
}

void
cleanup_server(struct iperf_test *test)
{
	int  i = 0;
    /* Close open test sockets */
    close(test->ctrl_sck);
    close(test->listener);
	if (test->prot_listener >= 0)
	{
		if (test->ctrl_sck > test->listener)
		{
			for (i = test->ctrl_sck+1; i <= test->prot_listener; i++)
			{
				close(i);
			}
		}
		else
		{
			for (i = test->listener+1; i <= test->prot_listener; i++)
			{
				close(i);
			}
		}
	}
	test->prot_listener = -1;
	test->ctrl_sck = -1;
	test->listener = -1;

    /* Cancel any remaining timers. */
    if (test->stats_timer != NULL) {
		free_timer(test->stats_timer);
		test->stats_timer = NULL;
    }
    if (test->reporter_timer != NULL) {
		free_timer(test->reporter_timer);
		test->reporter_timer = NULL;
    }
}

int
all_data_sent(struct iperf_test * test)
{
    if (test->settings->bytes > 0) {
        if (test->bytes_sent >= (test->num_streams * test->settings->bytes)) {
            return (1);
        }
    }

    return (0);
}

int
iperf_send(struct iperf_test *test)
{
    int result;
    iperf_size_t bytes_sent;
    fd_set temp_write_set;
    struct timeval tv;
    struct iperf_stream *sp;

    memcpy(&temp_write_set, &test->write_set, sizeof(fd_set));
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    result = select(test->max_fd + 1, NULL, &temp_write_set, NULL, &tv);
    if (result < 0 && errno != EINTR) {
        i_errno = IESELECT;
        return (-1);
    }
    if (result > 0) {
        SLIST_FOREACH(sp, &test->streams, streams) {
            if (FD_ISSET(sp->socket, &temp_write_set)) {
                if ((bytes_sent = sp->snd(sp)) < 0) {
                    i_errno = IESTREAMWRITE;
                    return (-1);
                }
                test->bytes_sent += bytes_sent;
                FD_CLR(sp->socket, &temp_write_set);
            }
        }
    }

    return (0);
}

int
iperf_recv(struct iperf_test *test)
{
    int result;
    //iperf_size_t bytes_sent;
    int bytes_sent;
    fd_set temp_read_set;
    struct timeval tv;
    struct iperf_stream *sp;

	IPF_DBG("\n");
    memcpy(&temp_read_set, &test->read_set, sizeof(fd_set));
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    result = select(test->max_fd + 1, &temp_read_set, NULL, NULL, &tv);
    if (result < 0) {
        i_errno = IESELECT;
        return (-1);
    }
    if (result > 0) {
		IPF_DBG("===> result = %d\n",result);
        SLIST_FOREACH(sp, &test->streams, streams) {
            if (FD_ISSET(sp->socket, &temp_read_set)) {
                if ((bytes_sent = sp->rcv(sp)) < 0) {
					IPF_DBG(" IESTREAMREAD bytes_sent = %d\n",bytes_sent);
                    i_errno = IESTREAMREAD;
                    return (-1);
                }
				IPF_DBG(" bytes_sent = %d\n",bytes_sent);
                test->bytes_sent += bytes_sent;
                FD_CLR(sp->socket, &temp_read_set);
            }
        }
    }

    return (0);
}

int
iperf_init_test(struct iperf_test *test)
{
    struct iperf_stream *sp;
    time_t sec;
    time_t usec;

	IPF_DBG("\n");
    if (test->protocol->init) {
        if (test->protocol->init(test) < 0){
			IPF_DBG("init error\n");
			return (-1);
        	}
    }

    /* Set timers */
    if (test->settings->bytes == 0) {
        test->timer = new_timer(test->duration, 0);
        if (test->timer == NULL){
			IPF_DBG("new_timer error\n");
			return (-1);
        }
    } 

    if (test->stats_interval != 0) {
        sec = (time_t) test->stats_interval;
        usec = (test->stats_interval - sec) * SEC_TO_US;
        test->stats_timer = new_timer(sec, usec);
        if (test->stats_timer == NULL){
			IPF_DBG("new stats_timer timer error\n");

			return (-1);
        	}
    }
    if (test->reporter_interval != 0) {
        sec = (time_t) test->reporter_interval;
        usec = (test->reporter_interval - sec) * SEC_TO_US;
        test->reporter_timer = new_timer(sec, usec);
        if (test->reporter_timer == NULL){
			IPF_DBG("new reporter_timer timer error\n");
            return (-1);
        	}
    }

    /* Set start time */
    SLIST_FOREACH(sp, &test->streams, streams) {
        if (iperf_gettimeofday(&sp->result->start_time, NULL) < 0) {
            i_errno = IEINITTEST;
            return (-1);
        }
    }

    if (test->on_test_start)
        test->on_test_start(test);

    return (0);
}


/*********************************************************/
/*num：int型原数,
   str:需转换成的string，
   radix,原进制， */

char *itoa(int num,char *str,int radix){
    /* 索引表 */
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned int unum;/* 中间变量 */
    int i=0,j,k;
	if (radix == 0)
	{
		return str;
	}
    /* 确定unum的值 */
    if(radix==10&&num<0){/* 十进制负数 */
        unum=(unsigned)-num;
        str[i++]='-';
    }else
        unum=(unsigned)num;/* 其他情况 */
    /* 逆序 */
    do{
        str[i++]=index[unum%(unsigned)radix];
        unum/=radix;
    }while(unum);
    str[i]='\0';
    /* 转换 */
    if(str[0]=='-')
        k=1;/* 十进制负数 */
    else
        k=0;
    /* 将原来的“/2”改为“/2.0”，保证当num在16~255之间，radix等于16时，也能得到正确结果 */
    for(j=k;j<(i-1)/2.0+k;j++){
        num=str[j];
        str[j]=str[i-j-1+k];
        str[i-j-1+k]=num;
    }
    return str;
}

#define USE_MALLOC_FREE_BUF 1
#if !USE_MALLOC_FREE_BUF
char pstring[256];
char optbuf[128];
#endif
int
package_parameters(struct iperf_test *test)
{
#if USE_MALLOC_FREE_BUF
    char *pstring = NULL;
	char *optbuf = NULL;

	pstring = malloc(256);
	optbuf = malloc(128);
	if (pstring == NULL || optbuf == NULL)
	{
		if (pstring)
		{
			free(pstring);
			pstring = NULL;
		}
		if (optbuf)
		{
			free(optbuf);
			optbuf = NULL;
		}
		return -1;
	}
#endif	
    memset(pstring, 0, 256*sizeof(char));
	  //char * tmp;
    *pstring = ' ';

    if (test->protocol->id == Ptcp) {
        strncat(pstring, "-p ", strlen("-p "));
    } else if (test->protocol->id == Pudp) {
        strncat(pstring, "-u ", strlen("-u "));
    }

#if 0
	memcpy(optbuf, "-P ", sizeof("-P "));
	strncat(pstring, optbuf, sizeof(optbuf));
	printf("optbuf=%s, pstring=%s\n", optbuf, pstring);
			printf("send parameter\n");
			test->state = IPERF_DONE;
			return 0;


	//sprintf(optbuf+sizeof("-P "), "%d", test->num_streams);
	itoa(test->num_streams, optbuf, 10);
    strncat(pstring, optbuf, sizeof(optbuf));

	printf("optbuf=%s, pstring=%s\n", optbuf, pstring);
#endif	
//    snprintf(optbuf, sizeof(optbuf), "-P %d ", test->num_streams);
	snprintf(optbuf, 128, "-P %d ", test->num_streams);
    strncat(pstring, optbuf, strlen(optbuf));

    if (test->reverse)
        strncat(pstring, "-R ", strlen(optbuf));
    
    if (test->settings->socket_bufsize) {
        snprintf(optbuf, 128, "-w %d ", test->settings->socket_bufsize);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->settings->rate) {
        snprintf(optbuf, 128, "-b %llu ", test->settings->rate);
        //snprintf(optbuf, sizeof(optbuf), "-b %d ", test->settings->rate);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->settings->mss) {
//        snprintf(optbuf, sizeof(optbuf), "-m %d ", test->settings->mss);
		snprintf(optbuf, 128, "-m %d ", test->settings->mss);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->no_delay) {
        snprintf(optbuf, 128, "-N ");
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->settings->bytes) {
        snprintf(optbuf, 128, "-n %llu ", test->settings->bytes);
        //snprintf(optbuf, sizeof(optbuf), "-n %d ", test->settings->bytes);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->duration) {
 //       snprintf(optbuf, sizeof(optbuf), "-t %d ", test->duration);
		snprintf(optbuf, 128, "-t %d ", test->duration);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->settings->blksize) {
        snprintf(optbuf, 128, "-l %d ", test->settings->blksize);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    if (test->settings->tos) {
        snprintf(optbuf, 128, "-S %d ", test->settings->tos);
        strncat(pstring, optbuf, strlen(optbuf));
    }

    *pstring = (char) (strlen(pstring) - 1);
	

    if (Nwrite(test->ctrl_sck, pstring, strlen(pstring), Ptcp) < 0) {
        i_errno = IESENDPARAMS;
#if USE_MALLOC_FREE_BUF		
		if (pstring)
		{
			free(pstring);
			pstring = NULL;
		}
		if (optbuf)
		{
			free(optbuf);
			optbuf = NULL;
		}
#endif		
        return (-1);
    }
#if USE_MALLOC_FREE_BUF	
	if (pstring)
	{
		free(pstring);
		pstring = NULL;
	}
	if (optbuf)
	{
		free(optbuf);
		optbuf = NULL;
	}
#endif
    return 0;
}
 /****************************************************************************** 
* getopt() 
* 
* The getopt() function is a command line parser. It returns the next 
* option character in argv that matches an option character in opstring. 
* 
* The argv argument points to an array of argc+1 elements containing argc 
* pointers to character strings followed by a null pointer. 
* 
* The opstring argument points to a string of option characters; if an 
* option character is followed by a colon, the option is expected to have 
* an argument that may or may not be separated from it by white space. 
* The external variable optarg is set to point to the start of the option 
* argument on return from getopt(). 
* 
* The getopt() function places in optind the argv index of the next argument 
* to be processed. The system initializes the external variable optind to 
* 1 before the first call to getopt(). 
* 
* When all options have been processed (that is, up to the first nonoption 
* argument), getopt() returns EOF. The special option "--" may be used to 
* delimit the end of the options; EOF will be returned, and "--" will be 
* skipped. 
* 
* The getopt() function returns a question mark (?) when it encounters an 
* option character not included in opstring. This error message can be 
* disabled by setting opterr to zero. Otherwise, it returns the option 
* character that was detected. 
* 
* If the special option "--" is detected, or all options have been 
* processed, EOF is returned. 
* 
* Options are marked by either a minus sign (-) or a slash (/). 
* 
* No errors are defined. 
*****************************************************************************/ 



/* static (global) variables that are specified as exported by getopt() */ 
 #define EOF (-1)
char *optarg = NULL; /* pointer to the start of the option argument */ 
int optind = 1; /* number of the next argv[] to be evaluated */ 
int opterr = 1; /* non-zero if a question mark should be returned 
when a non-valid option character is detected */ 

/* handle possible future character set concerns by putting this in a macro */ 
#define _next_char(string) (char)(*(string+1)) 

int getopt(int argc, char *argv[], char *opstring) 
{ 
	static char *pIndexPosition = NULL; /* place inside current argv string */ 
	char *pArgString = NULL; /* where to start from next */ 
	char *pOptString; /* the string in our program */ 


	if (pIndexPosition != NULL) { 
	/* we last left off inside an argv string */ 
		if (*(++pIndexPosition)) { 
			/* there is more to come in the most recent argv */ 
			pArgString = pIndexPosition; 
		} 
	} 
	if (pArgString == NULL) { 
		/* we didn't leave off in the middle of an argv string */ 
		if (optind >= argc) { 
			/* more command-line arguments than the argument count */ 
			pIndexPosition = NULL; /* not in the middle of anything */ 
			return EOF; /* used up all command-line arguments */ 
		} 

		/*--------------------------------------------------------------------- 
		* If the next argv[] is not an option, there can be no more options. 
		*-------------------------------------------------------------------*/ 
		pArgString = argv[optind++]; /* set this to the next argument ptr */ 
		if (('/' != *pArgString) && /* doesn't start with a slash or a dash? */ 
		('-' != *pArgString)) { 
			--optind; /* point to current arg once we're done */ 
			optarg = NULL; /* no argument follows the option */ 
			pIndexPosition = NULL; /* not in the middle of anything */ 
			return EOF; /* used up all the command-line flags */ 
		} 

		/* check for special end-of-flags markers */ 
		if ((strcmp(pArgString, "-") == 0) || 
		(strcmp(pArgString, "--") == 0)) { 
			optarg = NULL; /* no argument follows the option */ 
			pIndexPosition = NULL; /* not in the middle of anything */ 
			return EOF; /* encountered the special flag */ 
		} 

		pArgString++; /* look past the / or - */ 
	} 

	if (':' == *pArgString) { /* is it a colon? */ 
		/*--------------------------------------------------------------------- 
		* Rare case: if opterr is non-zero, return a question mark; 
		* otherwise, just return the colon we're on. 
		*-------------------------------------------------------------------*/ 
		return (opterr ? (int)'?' : (int)':'); 
	} 
	else if ((pOptString = strchr(opstring, *pArgString)) == 0) { 
		/*--------------------------------------------------------------------- 
		* The letter on the command-line wasn't any good. 
		*-------------------------------------------------------------------*/ 
		optarg = NULL; /* no argument follows the option */ 
		pIndexPosition = NULL; /* not in the middle of anything */ 
		return (opterr ? (int)'?' : (int)*pArgString); 
	} 
	else { 
		/*--------------------------------------------------------------------- 
		* The letter on the command-line matches one we expect to see 
		*-------------------------------------------------------------------*/ 
		if (':' == _next_char(pOptString)) { /* is the next letter a colon? */ 
			/* It is a colon. Look for an argument string. */ 
			if ('\0' != _next_char(pArgString)) { /* argument in this argv? */ 
				optarg = &pArgString[1]; /* Yes, it is */ 
			} 
			else { 
			/*------------------------------------------------------------- 
			* The argument string must be in the next argv. 
			* But, what if there is none (bad input from the user)? 
			* In that case, return the letter, and optarg as NULL. 
			*-----------------------------------------------------------*/ 
				if (optind < argc) 
					optarg = argv[optind++]; 
				else { 
					optarg = NULL; 
					return (opterr ? (int)'?' : (int)*pArgString); 
				} 
			} 
			pIndexPosition = NULL; /* not in the middle of anything */ 
		} 
		else { 
			/* it's not a colon, so just return the letter */ 
			optarg = NULL; /* no argument follows the option */ 
			pIndexPosition = pArgString; /* point to the letter we're on */ 
		} 
			return (int)*pArgString; /* return the letter that matched */ 
	} 
}


int
parse_parameters(struct iperf_test *test)
{
    int n;
    char *param, **params;
    char len;
    int ch;

	IPF_DBG("\n");
    //char pstring[256];
    char *pstring = NULL;

	pstring = malloc(256);
	if (NULL == pstring)
	{
		i_errno = IERECVPARAMS;
		return -1;
	}

    memset(pstring, 0, 256 * sizeof(char));

    if (Nread(test->ctrl_sck, &len, sizeof(char), Ptcp) < 0) {
        i_errno = IERECVPARAMS;
        return (-1);
    }

    if (Nread(test->ctrl_sck, pstring, len, Ptcp) < 0) {
        i_errno = IERECVPARAMS;
        return (-1);
    }
	IPF_DBG("IERECVPARAMS\n");

    for (param = strtok(pstring, " "), n = 1, params = NULL; param; param = strtok(NULL, " ")) {
        if ((params = realloc(params, (n+1)*sizeof(char *))) == NULL) {
			free(pstring);
            i_errno = IERECVPARAMS;
            return (-1);
        }
        params[n] = param;
        n++;
    }
	params[n] = NULL;

    // XXX: Should we check for parameters exceeding maximum values here?
    while ((ch = getopt(n, params, "pt:n:m:uNP:Rw:l:b:S:")) != -1) {
        switch (ch) {
			IPF_DBG("ch = %c, optarg=%s\n", ch, optarg);
            case 'p':
                set_protocol(test, Ptcp);
                break;
            case 't':
                test->duration = atoi(optarg);
                break;
            case 'n':
                test->settings->bytes = atoll(optarg);
                break;
            case 'm':
                test->settings->mss = atoi(optarg);
                break;
            case 'u':
                set_protocol(test, Pudp);
                break;
            case 'N':
                test->no_delay = 1;
                break;
            case 'P':
                test->num_streams = atoi(optarg);
                break;
            case 'R':
                test->reverse = 1;
                break;
            case 'w':
                test->settings->socket_bufsize = atoi(optarg);
                break;
            case 'l':
                test->settings->blksize = atoi(optarg);
                break;
            case 'b':
                test->settings->rate = atoll(optarg);
                break;
            case 'S':
                test->settings->tos = atoi(optarg);
                break;
        }
    }
#ifdef __APPLE__
    optreset = 1;
#endif
    optind = 1;

    free(params);
	free(pstring);

    return (0);
}

/**
 * iperf_exchange_parameters - handles the param_Exchange part for client
 *
 */

int
iperf_exchange_parameters(struct iperf_test * test)
{
    int s, msg;
    int state;

	IPF_DBG("\n");
    if (test->role == 'c') {

        if (package_parameters(test) < 0)
            return (-1);

    } else {

        if (parse_parameters(test) < 0)
            return (-1);
		
		IPF_DBG("after parse_parameters\n");

        if ((s = test->protocol->listen(test)) < 0) {
			IPF_DBG("listen(test)) < 0\n");
            state = SERVER_ERROR;
            if (Nwrite(test->ctrl_sck, &state, sizeof(state), Ptcp) < 0) {
                i_errno = IESENDMESSAGE;
                return (-1);
            }
            msg = htonl(i_errno);
            if (Nwrite(test->ctrl_sck, &msg, sizeof(msg), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return (-1);
            }
            msg = htonl(errno);
            if (Nwrite(test->ctrl_sck, &msg, sizeof(msg), Ptcp) < 0) {
                i_errno = IECTRLWRITE;
                return (-1);
            }
            return (-1);
        }
        FD_SET(s, &test->read_set);
        test->max_fd = (s > test->max_fd) ? s : test->max_fd;
        test->prot_listener = s;
		IPF_DBG(" maxfd: %d\n", test->max_fd);
        // Send the control message to create streams and start the test
        test->state = CREATE_STREAMS;
        if (Nwrite(test->ctrl_sck, &test->state, sizeof(char), Ptcp) < 0) {
            i_errno = IESENDMESSAGE;
            return (-1);
        }

    }

    return (0);
}

/*************************************************************/

int
iperf_exchange_results(struct iperf_test *test)
{
    unsigned int size;
    char buf[128];
    char *results;
    struct iperf_stream *sp;
    iperf_size_t bytes_transferred;

    if (test->role == 'c') {
        /* Prepare results string and send to server */
        results = NULL;
        size = 0;

        snprintf(buf, 128, "-C %f\n", test->cpu_util);
        size += strlen(buf);
        if ((results = tls_mem_alloc(size+1)) == NULL) {
            i_errno = IEPACKAGERESULTS;
            return (-1);
        }
        *results = '\0';
        strncat(results, buf, size+1);

        SLIST_FOREACH(sp, &test->streams, streams) {
            bytes_transferred = (test->reverse ? sp->result->bytes_received : sp->result->bytes_sent);
            //snprintf(buf, 128, "%d:%llu,%lf,%d,%d\n", sp->id, bytes_transferred,sp->jitter,sp->cnt_error, sp->packet_count);
            snprintf(buf, 128, "%d:%lld,%f,%d,%d\n", sp->id, bytes_transferred,sp->jitter,sp->cnt_error, sp->packet_count);
            size += strlen(buf);
            if ((results = tls_mem_realloc(results, size+1)) == NULL) {
                i_errno = IEPACKAGERESULTS;
                return (-1);
            }
/*
            if (sp == SLIST_FIRST(&test->streams))
                *results = '\0';
*/
            strncat(results, buf, size+1);
        }
        size++;
        size = htonl(size);
        if (Nwrite(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        if (Nwrite(test->ctrl_sck, results, ntohl(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        tls_mem_free(results);

        /* Get server results string */
        if (Nread(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        size = ntohl(size);
        results = (char *) tls_mem_alloc(size * sizeof(char));
        if (results == NULL) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        if (Nread(test->ctrl_sck, results, size, Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }

        // XXX: The only error this sets is IESTREAMID, which may never be reached. Consider making void.
        if (parse_results(test, results) < 0)
            return (-1);

        tls_mem_free(results);

    } else {
        /* Get client results string */
        if (Nread(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        size = ntohl(size);
        results = (char *) tls_mem_alloc(size * sizeof(char));
        if (results == NULL) {
            i_errno = IERECVRESULTS;
            return (-1);
        }
        if (Nread(test->ctrl_sck, results, size, Ptcp) < 0) {
            i_errno = IERECVRESULTS;
            return (-1);
        }

        // XXX: Same issue as with client
        if (parse_results(test, results) < 0)
            return (-1);

        tls_mem_free(results);

        /* Prepare results string and send to client */
        results = NULL;
        size = 0;

//        snprintf(buf, 128, "-C %f\n", test->cpu_util);
		snprintf(buf, 128, "-C %f\n", test->cpu_util);
        size += strlen(buf);
        if ((results = tls_mem_alloc(size+1)) == NULL) {
            i_errno = IEPACKAGERESULTS;
            return (-1);
        }
        *results = '\0';
        strncat(results, buf, size+1);

        SLIST_FOREACH(sp, &test->streams, streams) {
            bytes_transferred = (test->reverse ? sp->result->bytes_sent : sp->result->bytes_received);
            //snprintf(buf, 128, "%d:%llu,%lf,%d,%d\n", sp->id, bytes_transferred, sp->jitter,sp->cnt_error, sp->packet_count);
            snprintf(buf, 128, "%d:%lld,%f,%d,%d\n", sp->id, bytes_transferred, sp->jitter,sp->cnt_error, sp->packet_count);
            size += strlen(buf);
            if ((results = tls_mem_realloc(results, size+1)) == NULL) {
                i_errno = IEPACKAGERESULTS;
                return (-1);
            }
/*
            if (sp == SLIST_FIRST(&test->streams))
                *results = '\0';
*/
            strncat(results, buf, size+1);
        }
        size++;
        size = htonl(size);
		//printf("buf=%s\n", results);
        if (Nwrite(test->ctrl_sck, &size, sizeof(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        if (Nwrite(test->ctrl_sck, results, ntohl(size), Ptcp) < 0) {
            i_errno = IESENDRESULTS;
            return (-1);
        }
        tls_mem_free(results);
    }

    return (0);
}

/*************************************************************/

int
parse_results(struct iperf_test *test, char *results)
{
    int sid, cerror, pcount;
    //double jitter;
    float jitter;
    char *strp;
    char *tok;
    iperf_size_t bytes_transferred;
//    int bytes_transferred;
//	double temp;
    struct iperf_stream *sp;

	IPF_DBG(": %s, %x\n", results, results);
    /* Isolate the first line */
    strp = strchr(results, '\n');
    *strp = '\0';
    strp++;
    
    for (tok = strtok(results, " "); tok; tok = strtok(NULL, " ")) {
        if (strcmp(tok, "-C") == 0) {
            test->remote_cpu_util = atof(strtok(NULL, " "));
        }
    }
	IPF_DBG("strp=%s\n", strp);

    for (/* strp */; *strp; strp = strchr(strp, '\n')+1) {
       // sscanf(strp, "%d:%llu,%lf,%d,%d\n", &sid, &bytes_transferred, &jitter, &cerror, &pcount);
		 sscanf(strp, "%d:%lld,%f,%d,%d\n", &sid, &bytes_transferred, &jitter, &cerror, &pcount);
		
		IPF_DBG("sid = %d, bytes_transferred=%d, jitter=%f, cerror=%d, pcount=%d\n ",sid,bytes_transferred,
			jitter, cerror, pcount);
		
        SLIST_FOREACH(sp, &test->streams, streams){
			IPF_DBG("sp->id = %d\n", sp->id);
			if (sp->id == sid) break;

		}
        if (sp == NULL) {
            i_errno = IESTREAMID;
            return (-1);
        }
        if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse)) {
            sp->jitter = jitter;
            sp->cnt_error = cerror;
            sp->packet_count = pcount;
            sp->result->bytes_received = bytes_transferred;
        } else
            sp->result->bytes_sent = bytes_transferred;
    }
//	printf("bytes_transferred:%llu\n", bytes_transferred);
	if (test->duration)
	{
		if(bytes_transferred > 1024 * 1024 * 8)
		{
			printf("[BandWidth:]%.4f Mbits/sec\r\n", (bytes_transferred * 8.0)/1024.0/1024.0/test->duration);
		}
		else
		{
			printf("[BandWidth:]%.4f Kbits/sec\r\n", (bytes_transferred*8)/1024.0/test->duration);
		}
	}
	else
	{
		printf("test->duration is ZERO!!!\r\n");
	}

    return (0);
}


/*************************************************************/
/**
 * add_to_interval_list -- adds new interval to the interval_list
 * XXX: Interval lists should use SLIST implementation fro queue
 */

void
add_to_interval_list(struct iperf_stream_result * rp, struct iperf_interval_results * new)
{
    struct iperf_interval_results *ip = NULL;

    ip = (struct iperf_interval_results *) tls_mem_alloc(sizeof(struct iperf_interval_results));
    memcpy(ip, new, sizeof(struct iperf_interval_results));
    ip->next = NULL;

    if (rp->interval_results == NULL) {    /* if 1st interval */
        rp->interval_results = ip;
        rp->last_interval_results = ip; /* pointer to last element in list */
		rp->interval_list_flag = 1;
    } else { /* add to end of list */
        rp->last_interval_results->next = ip;
        rp->last_interval_results = ip;
    }
}


/************************************************************/

/**
 * connect_msg -- displays connection message
 * denoting sender/receiver details
 *
 */

void
connect_msg(struct iperf_stream *sp)
{
    char ipl[48], ipr[48];
    int lport = 0, rport = 0; 
    int domain = sp->settings->domain;

    if (domain == AF_INET) {
        iperf_inet_ntop(domain, (void *) &((struct sockaddr_in *) &sp->local_addr)->sin_addr, ipl, sizeof(ipl));
        iperf_inet_ntop(domain, (void *) &((struct sockaddr_in *) &sp->remote_addr)->sin_addr, ipr, sizeof(ipr));
        lport = ntohs(((struct sockaddr_in *) &sp->local_addr)->sin_port);
        rport = ntohs(((struct sockaddr_in *) &sp->remote_addr)->sin_port);
    } 

    printf("[%3d] local %s port %d connected to %s port %d\n",
        sp->socket, ipl, lport, ipr, rport);
}


/**************************************************************************/

struct iperf_test *
iperf_new_test(void)
{
    struct iperf_test *test;

    test = (struct iperf_test *) tls_mem_alloc(sizeof(struct iperf_test));
    if (!test) {
        i_errno = IENEWTEST;
        return (NULL);
    }
    /* initialize everything to zero */
    memset(test, 0, sizeof(struct iperf_test));

    test->settings = (struct iperf_settings *) tls_mem_alloc(sizeof(struct iperf_settings));
    memset(test->settings, 0, sizeof(struct iperf_settings));

    return (test);
}

/**************************************************************************/
int
iperf_defaults(struct iperf_test * testp)
{
    testp->duration = DURATION;
    testp->server_port = PORT;
    testp->ctrl_sck = -1;
    testp->prot_listener = -1;

    testp->stats_callback = iperf_stats_callback;
    testp->reporter_callback = iperf_reporter_callback;

    testp->stats_interval = 0;
    testp->reporter_interval = 0;
    testp->num_streams = 1;

    testp->settings->domain = AF_INET;
    testp->settings->unit_format = 'a';
    testp->settings->socket_bufsize = 0;    /* use autotuning */
    testp->settings->blksize = DEFAULT_TCP_BLKSIZE;
    testp->settings->rate = RATE;    /* UDP only */
    testp->settings->mss = 0;
    testp->settings->bytes = 0;
    memset(testp->cookie, 0, COOKIE_SIZE);

    /* Set up protocol list */
    SLIST_INIT(&testp->streams);
    SLIST_INIT(&testp->protocols);

    struct protocol *tcp, *udp;
    tcp = (struct protocol *) tls_mem_alloc(sizeof(struct protocol));
    if (!tcp)
        return (-1);
    memset(tcp, 0, sizeof(struct protocol));
    udp = (struct protocol *) tls_mem_alloc(sizeof(struct protocol));
    if (!udp)
        return (-1);
    memset(udp, 0, sizeof(struct protocol));

    tcp->id = Ptcp;
    tcp->name = "TCP";
    tcp->accept = iperf_tcp_accept;
    tcp->listen = iperf_tcp_listen;
    tcp->connect = iperf_tcp_connect;
    tcp->send = iperf_tcp_send;
    tcp->recv = iperf_tcp_recv;
    tcp->init = NULL;
    SLIST_INSERT_HEAD(&testp->protocols, tcp, protocols);

    udp->id = Pudp;
    udp->name = "UDP";
    udp->accept = iperf_udp_accept;
    udp->listen = iperf_udp_listen;
    udp->connect = iperf_udp_connect;
    udp->send = iperf_udp_send;
    udp->recv = iperf_udp_recv;
    udp->init = iperf_udp_init;
    SLIST_INSERT_AFTER(tcp, udp, protocols);

    set_protocol(testp, Ptcp);

    testp->on_new_stream = iperf_on_new_stream;
    testp->on_test_start = iperf_on_test_start;
    testp->on_connect = iperf_on_connect;
    testp->on_test_finish = iperf_on_test_finish;

    return (0);
}


/**************************************************************************/
void
iperf_free_test(struct iperf_test * test)
{
    struct protocol *prot;
    struct iperf_stream *sp;

    /* Free streams */
    while (!SLIST_EMPTY(&test->streams)) {
        sp = SLIST_FIRST(&test->streams);
        SLIST_REMOVE_HEAD(&test->streams, streams);		
		close(sp->socket);
        iperf_free_stream(sp);
    }
    /* Close open test sockets */
    close(test->ctrl_sck);
    close(test->listener);
	if (test->server_hostname)
	{
    	tls_mem_free(test->server_hostname);
	}
	if (test->bind_address)
	{
	    tls_mem_free(test->bind_address);
	}
	if (test->settings)
	{
    	tls_mem_free(test->settings);
	}
    free_timer(test->timer);
    free_timer(test->stats_timer);
    free_timer(test->reporter_timer);

    /* Free protocol list */
    while (!SLIST_EMPTY(&test->protocols)) {
        prot = SLIST_FIRST(&test->protocols);
        SLIST_REMOVE_HEAD(&test->protocols, protocols);        
        tls_mem_free(prot);
    }

    /* XXX: Why are we setting these values to NULL? */
    // test->streams = NULL;
    test->stats_callback = NULL;
    test->reporter_callback = NULL;
	test->max_fd = 0;
    tls_mem_free(test);
}


void
iperf_reset_test(struct iperf_test *test)
{
    struct iperf_stream *sp;

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
    
    test->num_streams = 1;
    test->settings->socket_bufsize = 0;
    test->settings->blksize = DEFAULT_TCP_BLKSIZE;
    test->settings->rate = RATE;   /* UDP only */
    test->settings->mss = 0;
    memset(test->cookie, 0, COOKIE_SIZE);
	
	test->max_fd = 0;
}


/**************************************************************************/

/**
 * iperf_stats_callback -- handles the statistic gathering for both the client and server
 *
 * XXX: This function needs to be updated to reflect the new code
 */


void
iperf_stats_callback(struct iperf_test * test)
{
    struct iperf_stream *sp;
    struct iperf_stream_result *rp = NULL;
    struct iperf_interval_results *ip = NULL;
	struct iperf_interval_results temp;
	struct iperf_interval_results *ip_temp;
	
    SLIST_FOREACH(sp, &test->streams, streams) {
        rp = sp->result;

        if ((test->role == 'c' && !test->reverse) || (test->role == 's' && test->reverse))
            temp.bytes_transferred = rp->bytes_sent_this_interval;
        else
            temp.bytes_transferred = rp->bytes_received_this_interval;
     
        ip = sp->result->interval_results;
        /* result->end_time contains timestamp of previous interval */
        if ( ip != NULL ) /* not the 1st interval */
            memcpy(&temp.interval_start_time, &sp->result->end_time, sizeof(struct timeval));
        else /* or use timestamp from beginning */
            memcpy(&temp.interval_start_time, &sp->result->start_time, sizeof(struct timeval));
        /* now save time of end of this interval */
        iperf_gettimeofday(&sp->result->end_time, NULL);
        memcpy(&temp.interval_end_time, &sp->result->end_time, sizeof(struct timeval));
		//printf("interval_end_time: sec: %d, usec: %d\n", temp.interval_end_time.tv_sec, temp.interval_end_time.tv_usec);
        temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        //temp.interval_duration = timeval_diff(&temp.interval_start_time, &temp.interval_end_time);
        //printf("====>%d\n", temp.interval_duration);
        if (test->tcp_info)
            get_tcpinfo(sp, &temp);
		if (rp->interval_list_flag == 0)
		{
        add_to_interval_list(rp, &temp);
		}
		else
		{	
			temp.next = NULL;
			ip_temp = rp->interval_results;
			 memcpy(ip_temp, &temp, sizeof(struct iperf_interval_results));
		}
        rp->bytes_sent_this_interval = rp->bytes_received_this_interval = 0;

    }

}

static void
iperf_print_intermediate(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes = 0;
    int start_time, end_time;
    struct iperf_interval_results *ip = NULL;

    SLIST_FOREACH(sp, &test->streams, streams) {
        print_interval_results(test, sp);
        bytes += sp->result->last_interval_results->bytes_transferred; /* sum up all streams */
    }
    if (bytes <=0 ) { /* this can happen if timer goes off just when client exits */
        printf("error: bytes <= 0!\n");
        return;
    }
    /* next build string with sum of all streams */
    if (test->num_streams > 1) {
        sp = SLIST_FIRST(&test->streams); /* reset back to 1st stream */
        ip = sp->result->last_interval_results;    /* use 1st stream for timing info */

        unit_snprintf(ubuf, UNIT_LEN,  (bytes), 'A');
        unit_snprintf(nbuf, UNIT_LEN,  (double)(bytes / ip->interval_duration),
            test->settings->unit_format);

        start_time = timeval_diff(&sp->result->start_time,&ip->interval_start_time);
        end_time = timeval_diff(&sp->result->start_time,&ip->interval_end_time);
        printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
    }
    if (test->tcp_info) {
        print_tcpinfo(test);
    }
}

#if 0
static void
iperf_print_intermediate_1(struct iperf_test *test)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes = 0;
    double start_time, end_time;
    struct iperf_interval_results *ip = NULL;

    SLIST_FOREACH(sp, &test->streams, streams) {
        print_interval_results(test, sp);
        bytes += sp->result->last_interval_results->bytes_transferred; /* sum up all streams */
    }
    if (bytes <=0 ) { /* this can happen if timer goes off just when client exits */
        printf("error: bytes <= 0!\n");
        return;
    }
    /* next build string with sum of all streams */
    if (test->num_streams > 1) {
        sp = SLIST_FIRST(&test->streams); /* reset back to 1st stream */
        ip = sp->result->last_interval_results;    /* use 1st stream for timing info */

        unit_snprintf(ubuf, UNIT_LEN, (double) (bytes), 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) (bytes / ip->interval_duration),
            test->settings->unit_format);

        start_time = timeval_diff(&sp->result->start_time,&ip->interval_start_time);
        end_time = timeval_diff(&sp->result->start_time,&ip->interval_end_time);
        printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
    }
    if (test->tcp_info) {
        print_tcpinfo(test);
    }
}
#endif

static void
iperf_print_results (struct iperf_test *test)
{

    int total_packets = 0, lost_packets = 0;
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    //iperf_size_t bytes = 0;
	iperf_size_t bytes_sent = 0, bytes_received = 0;
    iperf_size_t total_sent = 0, total_received = 0;
    int start_time, end_time; 
	//	double avg_jitter;
	float avg_jitter = 0;
    //struct iperf_interval_results *ip = NULL;
    /* print final summary for all intervals */


#if 1
	if (test->protocol->id == Ptcp) 
	    printf(report_bw_header);
	else
		printf(report_bw_jitter_loss_header);
#endif 	
    start_time = 0;
    sp = SLIST_FIRST(&test->streams);
    end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
	end_time = end_time ? end_time : 1;
    SLIST_FOREACH(sp, &test->streams, streams) {
        bytes_sent = sp->result->bytes_sent;
        bytes_received = sp->result->bytes_received;
        total_sent += bytes_sent;
        total_received += bytes_received;

        if (test->protocol->id == Pudp) {
            total_packets += sp->packet_count;
            lost_packets += sp->cnt_error;
            avg_jitter += sp->jitter;
        }

        if (bytes_sent > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (bytes_sent), 'A');
            unit_snprintf(nbuf, UNIT_LEN, (bytes_sent / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Sent\n");
                printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            } else {
            #if 1
                printf(report_bw_jitter_loss_format, sp->socket, start_time,
                        end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, 
                        //sp->packet_count, (double) (100.0 * sp->cnt_error / sp->packet_count));
                        sp->packet_count, (100 * sp->cnt_error / (sp->packet_count?sp->packet_count:1)));
			#endif
                if (test->role == 'c') {
                    printf(report_datagrams, sp->socket, sp->packet_count);
                }
                if (sp->outoforder_packets > 0)
                    printf(report_sum_outoforder, start_time, end_time, sp->cnt_error);
            }
        }

        if (bytes_received > 0) {
            unit_snprintf(ubuf, UNIT_LEN,  bytes_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN,  (bytes_received / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Received\n");
                printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            }else{
                if ((test->protocol->id == Pudp) && ( test->role == 's' )){
                    printf("      Received\n");
                    printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
                }
            }
        }
    }

    if (test->num_streams > 1) {
        unit_snprintf(ubuf, UNIT_LEN, (double) total_sent, 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) total_sent / end_time, test->settings->unit_format);
        if (test->protocol->id == Ptcp) {
            printf("      Total sent\n");
            printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
            unit_snprintf(ubuf, UNIT_LEN,  (double)total_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN,  (double)(total_received / end_time), test->settings->unit_format);
            printf("      Total received\n");
            printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
        } else {

            avg_jitter /= test->num_streams;
            printf(report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf, avg_jitter,
                //lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
                lost_packets, total_packets,  (100 * lost_packets / (total_packets?total_packets:1)));
        }
    }

    if (test->tcp_info) {
        print_tcpinfo(test);
    }

    if (test->verbose) {
        printf("Host CPU Utilization:   %.1f%%\n", test->cpu_util);
        printf("Remote CPU Utilization: %.1f%%\n", test->remote_cpu_util);
    }
}

#if 0
static void
iperf_print_results_1 (struct iperf_test *test)
{

    int total_packets = 0, lost_packets = 0;
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    struct iperf_stream *sp = NULL;
    iperf_size_t bytes = 0, bytes_sent = 0, bytes_received = 0;
    iperf_size_t total_sent = 0, total_received = 0;
    double start_time, end_time, avg_jitter;
    struct iperf_interval_results *ip = NULL;
    /* print final summary for all intervals */

    printf(report_bw_header);

    start_time = 0.;
    sp = SLIST_FIRST(&test->streams);
    end_time = timeval_diff(&sp->result->start_time, &sp->result->end_time);
    SLIST_FOREACH(sp, &test->streams, streams) {
        bytes_sent = sp->result->bytes_sent;
        bytes_received = sp->result->bytes_received;
        total_sent += bytes_sent;
        total_received += bytes_received;

        if (test->protocol->id == Pudp) {
            total_packets += sp->packet_count;
            lost_packets += sp->cnt_error;
            avg_jitter += sp->jitter;
        }

        if (bytes_sent > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) (bytes_sent), 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (bytes_sent / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Sent\n");
                printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            } else {
                printf(report_bw_jitter_loss_format, sp->socket, start_time,
                        end_time, ubuf, nbuf, sp->jitter * 1000, sp->cnt_error, 
                        sp->packet_count, (double) (100.0 * sp->cnt_error / sp->packet_count));
                if (test->role == 'c') {
                    printf(report_datagrams, sp->socket, sp->packet_count);
                }
                if (sp->outoforder_packets > 0)
                    printf(report_sum_outoforder, start_time, end_time, sp->cnt_error);
            }
        }
        if (bytes_received > 0) {
            unit_snprintf(ubuf, UNIT_LEN, (double) bytes_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (bytes_received / end_time), test->settings->unit_format);
            if (test->protocol->id == Ptcp) {
                printf("      Received\n");
                printf(report_bw_format, sp->socket, start_time, end_time, ubuf, nbuf);
            }
        }
    }

    if (test->num_streams > 1) {
        unit_snprintf(ubuf, UNIT_LEN, (double) total_sent, 'A');
        unit_snprintf(nbuf, UNIT_LEN, (double) total_sent / end_time, test->settings->unit_format);
        if (test->protocol->id == Ptcp) {
            printf("      Total sent\n");
            printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
            unit_snprintf(ubuf, UNIT_LEN, (double) total_received, 'A');
            unit_snprintf(nbuf, UNIT_LEN, (double) (total_received / end_time), test->settings->unit_format);
            printf("      Total received\n");
            printf(report_sum_bw_format, start_time, end_time, ubuf, nbuf);
        } else {
            avg_jitter /= test->num_streams;
            printf(report_sum_bw_jitter_loss_format, start_time, end_time, ubuf, nbuf, avg_jitter,
                lost_packets, total_packets, (double) (100.0 * lost_packets / total_packets));
        }
    }

    if (test->tcp_info) {
        print_tcpinfo(test);
    }

    if (test->verbose) {
        printf("Host CPU Utilization:   %.1f%%\n", test->cpu_util);
        printf("Remote CPU Utilization: %.1f%%\n", test->remote_cpu_util);
    }
}
#endif

/**************************************************************************/

/**
 * iperf_reporter_callback -- handles the report printing
 *
 */

void
iperf_reporter_callback(struct iperf_test * test)
{
    //int total_packets = 0, lost_packets = 0;
    //char ubuf[UNIT_LEN];
    //char nbuf[UNIT_LEN];
    //struct iperf_stream *sp = NULL;
    //iperf_size_t bytes = 0, bytes_sent = 0, bytes_received = 0;
    //iperf_size_t total_sent = 0, total_received = 0;
    //struct iperf_interval_results *ip = NULL;

    switch (test->state) {
        case TEST_RUNNING:
        case STREAM_RUNNING:
            /* print interval results for each stream */
            iperf_print_intermediate(test);
            break;
        case DISPLAY_RESULTS:
//            iperf_print_intermediate(test);
            iperf_print_results(test);
            break;
    } 

}


/**************************************************************************/
void
print_interval_results(struct iperf_test * test, struct iperf_stream * sp)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    int st = 0., et = 0.;
    struct iperf_interval_results *ir = NULL;
//printf("===>print_interval_results: \n");
    ir = sp->result->last_interval_results; /* get last entry in linked list */
    if (ir == NULL) {
        printf("print_interval_results Error: interval_results = NULL \n");
        return;
    }
#if 0	
    if (sp == SLIST_FIRST(&test->streams)) {
        printf(report_bw_header);
    }
#endif	
//	printf("ir->bytes_transferred: %d \n",ir->bytes_transferred);
//		printf("ir->interval_duration: %d \n",ir->interval_duration);
//		printf("/=: %d \n",ir->bytes_transferred / ir->interval_duration);
		
    unit_snprintf(ubuf, UNIT_LEN,  (ir->bytes_transferred), 'A');
	if (ir->interval_duration)
	{
    	unit_snprintf(nbuf, UNIT_LEN, (ir->bytes_transferred / ir->interval_duration),
        	    test->settings->unit_format);
	}
	else
	{
    	unit_snprintf(nbuf, UNIT_LEN, (ir->bytes_transferred),
        	    test->settings->unit_format);	
	}
    
    st = timeval_diff(&sp->result->start_time,&ir->interval_start_time);
    et = timeval_diff(&sp->result->start_time,&ir->interval_end_time);

	if(st == 0)
	{
		printf(report_bw_header);
	}
    printf(report_bw_format, sp->socket, st, et, ubuf, nbuf);

/* doing aggregate TCP_INFO reporting for now...
    if (test->tcp_info)
        print_tcpinfo(ir);
*/

}

#if 0
/**************************************************************************/
void
print_interval_results_1(struct iperf_test * test, struct iperf_stream * sp)
{
    char ubuf[UNIT_LEN];
    char nbuf[UNIT_LEN];
    double st = 0., et = 0.;
    struct iperf_interval_results *ir = NULL;
printf("===>print_interval_results: ");
    ir = sp->result->last_interval_results; /* get last entry in linked list */
    if (ir == NULL) {
        printf("print_interval_results Error: interval_results = NULL \n");
        return;
    }
    if (sp == SLIST_FIRST(&test->streams)) {
        printf(report_bw_header);
    }

    unit_snprintf(ubuf, UNIT_LEN, (double) (ir->bytes_transferred), 'A');
    unit_snprintf(nbuf, UNIT_LEN, (double) (ir->bytes_transferred / ir->interval_duration),
            test->settings->unit_format);
    
    st = timeval_diff(&sp->result->start_time,&ir->interval_start_time);
    et = timeval_diff(&sp->result->start_time,&ir->interval_end_time);
    
    printf(report_bw_format, sp->socket, st, et, ubuf, nbuf);

/* doing aggregate TCP_INFO reporting for now...
    if (test->tcp_info)
        print_tcpinfo(ir);
*/

}
#endif

/**************************************************************************/
void
iperf_free_stream(struct iperf_stream * sp)
{
    struct iperf_interval_results *ip, *np;

    /* XXX: need to free interval list too! */
    tls_mem_free(sp->buffer);
    for (ip = sp->result->interval_results; ip; ip = np) {
        np = ip->next;
        tls_mem_free(ip);
    }

	if (sp->send_timer)
	{
    	free_timer(sp->send_timer);
	}
    tls_mem_free(sp->result);
    tls_mem_free(sp);
}

/**************************************************************************/
struct iperf_stream *
iperf_new_stream(struct iperf_test *test, int s)
{
    int i;
    struct iperf_stream *sp;

    sp = (struct iperf_stream *) tls_mem_alloc(sizeof(struct iperf_stream));
    if (!sp) {
        i_errno = IECREATESTREAM;
		IPF_DBG("malloc sp error\n");
        return (NULL);
    }

    memset(sp, 0, sizeof(struct iperf_stream));

    sp->buffer = (char *) tls_mem_alloc(test->settings->blksize);
    sp->result = (struct iperf_stream_result *) tls_mem_alloc(sizeof(struct iperf_stream_result));
    sp->settings = test->settings;

    if (!sp->buffer) {
        i_errno = IECREATESTREAM;
		tls_mem_free(sp);
		IPF_DBG("malloc sp buffer error, blksize = %d\n", test->settings->blksize);
        return (NULL);
    }
    if (!sp->result) {
        i_errno = IECREATESTREAM;
		tls_mem_free(sp->buffer);
		tls_mem_free(sp);
		IPF_DBG("malloc sp result error \n");
        return (NULL);
    }

    memset(sp->result, 0, sizeof(struct iperf_stream_result));
    
    /* Randomize the buffer */
	#if 0
    srandom(time(NULL));
	#endif
    for (i = 0; i < test->settings->blksize; ++i)
        sp->buffer[i] = i;//random();
	
	
    /* Set socket */
    sp->socket = s;

    sp->snd = test->protocol->send;
    sp->rcv = test->protocol->recv;

    /* Initialize stream */
    if (iperf_init_stream(sp, test) < 0){
		
		tls_mem_free(sp->buffer);
		tls_mem_free(sp->result);
		tls_mem_free(sp);
		return (NULL);
    }
    iperf_add_stream(test, sp);

    return (sp);
}

/**************************************************************************/
int
iperf_init_stream(struct iperf_stream *sp, struct iperf_test *test)
{
    socklen_t len;
    int opt;

	IPF_DBG("\n");

    len = sizeof(struct sockaddr);
    if (getsockname(sp->socket, (struct sockaddr *) &sp->local_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return (-1);
    }
    len = sizeof(struct sockaddr);
    if (getpeername(sp->socket, (struct sockaddr *) &sp->remote_addr, &len) < 0) {
        i_errno = IEINITSTREAM;
        return (-1);
    }
    /* Set IP TOS */
    if ((opt = test->settings->tos)!=0) {
#ifdef IPV6_TCLASS
        if (test->settings->domain == AF_INET6) {
            if (setsockopt(sp->socket, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETCOS;
                return (-1);
            }
        } else 
#endif
        {
            if (setsockopt(sp->socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)) < 0) {
                i_errno = IESETTOS;
                return (-1);
            }
        }
    }

    return (0);
}

/**************************************************************************/
void
iperf_add_stream(struct iperf_test * test, struct iperf_stream * sp)
{
    int i;
    struct iperf_stream *n, *prev;

    if (SLIST_EMPTY(&test->streams)) {
        SLIST_INSERT_HEAD(&test->streams, sp, streams);
        sp->id = 1;
    } else {
        // for (n = test->streams, i = 2; n->next; n = n->next, ++i);
        i = 2;
        SLIST_FOREACH(n, &test->streams, streams) {
            prev = n;
            ++i;
        }
        SLIST_INSERT_AFTER(prev, sp, streams);
        sp->id = i;
    }
}
#if 0
void
sig_handler(int sig)
{
   longjmp(env, 1); 
}
#endif
