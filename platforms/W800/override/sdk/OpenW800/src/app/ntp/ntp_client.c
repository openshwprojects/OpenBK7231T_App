/**
 * @file    ntp_client.c
 *
 * @brief   ntp client
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#include <stdio.h>
#include <lwip/netdb.h>
#include <string.h>
#include <time.h>               /* for time() and ctime() */
#include "wm_config.h"
#include "wm_type_def.h"
#include "wm_sockets.h"
#include "wm_debug.h"
#include "wm_osal.h"
#include "wm_params.h"

#if TLS_CONFIG_NTP

#define NTP_DEBUG 0

#if NTP_DEBUG
#define ntp_debug printf
#else
#define ntp_debug(...)
#endif

#define UTC_NTP 2208988800U     /* 1970 - 1900 ;年 换算成秒 */
#define BUF_LEN	48

#define NTP_MAX_TRY_TIMES   4
#define NTP_SERVER_MAX_NUM	3
#define NTP_SERVER_DOMAIN_LEN	32
#define NTP_SERVER_IP_LEN 16
u8 serverno = NTP_SERVER_MAX_NUM;
char serverip[NTP_SERVER_MAX_NUM][NTP_SERVER_DOMAIN_LEN];

u32 sntp_serverip[NTP_SERVER_MAX_NUM];
/* get Timestamp for NTP in LOCAL ENDIAN */
void get_time64(u32 * ts)
{
	ts[0] = UTC_NTP;
    ts[1] = 0;
}

int open_connect(u8 * buf)
{
	int s[NTP_SERVER_MAX_NUM] = {-1,-1,-1};
	int max_socketnum = 0;
	int total_socketnum = 0;
	int last_total_socketnum = 0;
	int ip_num = 0;

    struct sockaddr_in pin;
    u32 tts[2];                 /* Transmit Timestamp */
    int ret = 0;
	int retval = WM_FAILED;
	int i;
    int servernum = 0;
    fd_set readfd;
    struct timeval timeout;
    socklen_t addrlen = sizeof(struct sockaddr);
	struct hostent *host = NULL;
	int ip_avial_num = 0;
	
	for(i = 0; i < NTP_SERVER_MAX_NUM; i++)
	{
		tls_param_get(TLS_PARAM_ID_SNTP_SERVER1 + i, serverip[i], 1);
		s[i] = -1;
	}	

	for(i = 0; i < NTP_SERVER_MAX_NUM; i++)
	{
	    host = gethostbyname(serverip[i]);
	    if(NULL == host)
	    {
	        continue;
	    }
		memcpy((u8 *)&sntp_serverip[ip_avial_num++], host->h_addr, host->h_length);
	}	
	if(ip_avial_num == 0)
	{
		ntp_debug("NTP Server DNS Failed ");
		return	retval;
	}

	for (i = 0; i < NTP_MAX_TRY_TIMES; i++){
		/*initialize */
		max_socketnum = 0;
		FD_ZERO(&readfd);
		for (servernum = 0; servernum < NTP_SERVER_MAX_NUM; servernum++)
		{
			if (s[servernum] >= 0)
			{
				closesocket(s[servernum]);
			}
			s[servernum] = -1;
		}

		if (WM_SUCCESS == retval)
		{
			break;
		}

		/*send NTP request to multiple servers*/
		for(servernum = 0;servernum < ip_avial_num;servernum ++ )
		{
			s[servernum] = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
			//printf("s[%d]:%d\n", servernum, s[servernum]);
			if(s[servernum] < 0)
			{
				ntp_debug("sock err\n");
				continue;
			}

			ip_num = (servernum + last_total_socketnum)%ip_avial_num;

			memset(&pin, 0, sizeof(struct sockaddr));
			pin.sin_family=AF_INET;                 //AF_INET表示使用IPv4
			pin.sin_addr.s_addr=sntp_serverip[ip_num];  //IPADDR_udp
			pin.sin_port=htons(123);

			buf[0] = 0x23;
			get_time64(tts);
			(*(u32 *)&buf[40]) = htonl(tts[0]);
			(*(u32 *)&buf[44])= htonl(tts[1]);

			ret = sendto(s[servernum], buf, BUF_LEN, 0, (struct sockaddr *)&pin, addrlen);
			if(ret < 0)
			{
				ntp_debug("\nsend err\n");
				closesocket(s[servernum]);
				s[servernum] = -1;
				continue;
			}

			FD_SET(s[servernum],&readfd);
			max_socketnum = (max_socketnum >= s[servernum])?max_socketnum:s[servernum];
			total_socketnum++;
		}

		last_total_socketnum = total_socketnum;

		/*select timeout 5 second*/
		timeout.tv_sec = 1;
		timeout.tv_usec =  0;
		ret = select(max_socketnum + 1 , &readfd , NULL,NULL ,&timeout) ;
		if(ret <= 0)
		{
			ntp_debug("Failed to select or timeout ");
		}
		else
		{
			for(servernum = 0;servernum < ip_avial_num ;servernum ++ )
			{
				if ((s[servernum] >= 0) && (FD_ISSET(s[servernum],&readfd)))
				{
					/*receive data from ntp server*/
					memset(buf,0,BUF_LEN);
					ret = recvfrom(s[servernum],buf,BUF_LEN,0,(struct sockaddr *)&pin,&addrlen);
					if(ret <=  0)
					{
						ntp_debug("Fail to recvfrom ");
						continue;
					}
					else
					{	
						retval = WM_SUCCESS;
						break;
					}
				}
			}
		}

        for(servernum = 0;servernum < ip_avial_num ;servernum ++ )
		{
			if ( s[servernum] >= 0 )
			{
                closesocket(s[servernum]);
			}
		}
	}
	return retval;
}

int get_reply(u8 * buf, u32 * time)
{
    u32 *pt;
    u32 t3[2];                  /* t3 = Transmit Timestamp @ Server */
    u32 t4[2];                  /* t4 = Receive Timestamp @ Client */

    get_time64(t4);
//  printf("%x  %x\n",t4[0],t4[1]);
    pt = (u32 *) & buf[40];
    t3[0] = htonl(*pt);
    pt = (u32 *) & buf[44];
    t3[1] = htonl(*pt);

    t3[0] -= UTC_NTP;
    t3[0] += 8 * 3600;             // 加8小时
// printf("server Time : %s\n", ctime(&t3[0]));
    *time = t3[0];

    return WM_SUCCESS;
}

u32 tls_ntp_client(void)
{
    int ret = 0;
    u32 time = 0;
    u8 buf[BUF_LEN] = { 0 };

    ret = open_connect(buf);
    if (WM_SUCCESS == ret)
    {
        get_reply(buf, &time);
    }
    return time;
}

int tls_ntp_set_server(char *ipaddr, int server_no)
{
	char *buf;

	if(strlen(ipaddr) > NTP_SERVER_DOMAIN_LEN)
		return WM_FAILED;
	
    if (server_no > NTP_SERVER_MAX_NUM)
    {
        return WM_FAILED;
    }
	
	buf = tls_mem_alloc(NTP_SERVER_DOMAIN_LEN);
	if(buf == NULL)
		return WM_FAILED;
	
	for(int i = 0; i < NTP_SERVER_MAX_NUM; i++)
	{
		tls_param_get(TLS_PARAM_ID_SNTP_SERVER1 + i, buf, 1);
		if((strlen(buf) == strlen(ipaddr)) && !memcmp(buf, ipaddr, strlen(ipaddr)))
			goto out;
	}
	
	tls_param_set(TLS_PARAM_ID_SNTP_SERVER1 + server_no, ipaddr, 1);
out:
	tls_mem_free(buf);
    return WM_SUCCESS;
}

int tls_ntp_query_sntpcfg()
{
	char temp[128];
	int i;
	for(i = 0; i < NTP_SERVER_MAX_NUM; i++)
	{
		tls_param_get(TLS_PARAM_ID_SNTP_SERVER1 + i, serverip[i], 1);
	}
	sprintf(temp, "\"%s\",\"%s\",\"%s\"\r\n", serverip[0],serverip[1],serverip[2]);
	printf("%s",temp);

	return WM_SUCCESS;
}

#endif
