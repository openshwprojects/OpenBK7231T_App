/*****************************************************************************
*
* File Name : wm_mcast_demo.c
*
* Description: mcast demo function
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : wanghaifang
*
* Date : 2014-6-2
*****************************************************************************/

#include <string.h>
#include "wm_include.h"

#if DEMO_UDP_MULTI_CAST
#define    DEMO_MCAST_TASK_SIZE      256
tls_os_queue_t *demo_mcast_q = NULL;
static OS_STK DemoMCastTaskStk[DEMO_MCAST_TASK_SIZE];
u8 is_snd = 1;

extern ST_Demo_Sys gDemoSys;
static void demo_mcast_task(void *sdata);

#define MPORT 5100
static u8 MCASTIP[4] = {224, 1, 2, 1};
#define MCAST_BUF_SIZE	1024
int mcastsendrcv(bool snd)
{
    int s;
    int ret;
    int size;
    int ttl = 10;
    int loop = 0;
    int times = 0;
    int i = 65;
    char *buffer = NULL;
    socklen_t socklen;
    struct sockaddr_in localaddr, fromaddr, Multi_addr;
    struct ip_mreq mreq;

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s < 0)
    {
        printf("socket error");
        return -1;
    }
    printf("multi case socket num=%d\n", s);
    Multi_addr.sin_family = AF_INET;
    Multi_addr.sin_port = htons(MPORT);
    MEMCPY((char *)&Multi_addr.sin_addr, (char *)MCASTIP, 4);

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(MPORT);
    localaddr.sin_addr.s_addr = htonl(0x00000000UL);
    fromaddr.sin_family = AF_INET;
    fromaddr.sin_port = htons(MPORT);
    fromaddr.sin_addr.s_addr = htonl(0x00000000UL);
    ret = bind(s, (struct sockaddr *)&localaddr, sizeof(localaddr));
    if(ret < 0)
    {
        printf("bind error");
        return -1;
    }
    //Configure multicast TTL
    if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        printf("IP_MULTICAST_TTL");
        return -1;
    }
    //Setting local loop for multicast
    if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
    {
        printf("IP_MULTICAST_LOOP");
        return -1;
    }
    MEMCPY((char *)&mreq.imr_multiaddr.s_addr, (char *)MCASTIP, 4);
    mreq.imr_interface.s_addr = htonl(0x00000000UL);
    //join multicast group
    if(setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        printf("IP_ADD_MEMBERSHIP");
        return -1;
    }
    buffer = tls_mem_alloc(MCAST_BUF_SIZE);
    if(NULL == buffer)
        return -1;
    memset(buffer, 0, MCAST_BUF_SIZE);
    //loop rx or tx data
    if(snd)
    {
        for(times = 0; times < 20; times++)
        {
            memset(buffer, i, MCAST_BUF_SIZE);
            size = sendto(s, buffer, MCAST_BUF_SIZE, 0, (struct sockaddr *)&Multi_addr, sizeof(Multi_addr));
            printf("multi case sendto buf=%s, size=%d\n", buffer, size);
            tls_os_time_delay(50);
            i++;
            memset(buffer, 0, MCAST_BUF_SIZE);
        }
    }
    else
    {
        for(times = 0; times < 5; times++)
        {
            socklen = sizeof(fromaddr);
            recvfrom(s, buffer, MCAST_BUF_SIZE, 0, (struct sockaddr *)&fromaddr, &socklen);
            printf("multi case recvfrom buf=%s, socklen=%d\n", buffer, socklen);
            memset(buffer, 0, MCAST_BUF_SIZE);
        }
    }
    tls_mem_free(buffer);
    //leave multicast group
    ret = setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    if(ret < 0)
    {
        printf("IP_DROP_MEMBERSHIP");
        return -1;
    }
    closesocket(s);
    return 0;
}

int CreateMCastDemoTask(char *buf)
{
    if(strstr(buf, "recv"))
    {
        is_snd = 0;
    }
    else
        is_snd = 1;
    printf("is_snd=%d, para:%s\n", is_snd, buf);
    tls_os_queue_create(&demo_mcast_q, DEMO_QUEUE_SIZE);
    //Task to deal with socket message
    tls_os_task_create(NULL, NULL,
                       demo_mcast_task,
                       NULL,
                       (void *)DemoMCastTaskStk,           /* task's stack start address*/
                       DEMO_MCAST_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                       DEMO_MCAST_TASK_PRIO,
                       0);

    return WM_SUCCESS;
}


static void mcast_net_status_changed_event(u8 status )
{
    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        tls_os_queue_send(demo_mcast_q, (void *)DEMO_MSG_WJOIN_FAILD, 0);
        break;
    case NETIF_WIFI_JOIN_SUCCESS:
        tls_os_queue_send(demo_mcast_q, (void *)DEMO_MSG_WJOIN_SUCCESS, 0);
        break;
    case NETIF_IP_NET_UP:
        tls_os_queue_send(demo_mcast_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
        break;
    default:
        break;
    }
}

static void demo_mcast_task(void *sdata)
{
    //	ST_Demo_Sys *sys = (ST_Demo_Sys *)sdata;
    void *msg;
    struct tls_ethif *ethif = tls_netif_get_ethif();

    printf("\nmcast task\n");

    if(ethif->status)	/*connected to ap and get IP*/
    {
        tls_os_queue_send(demo_mcast_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
    }
    else
    {
        struct tls_param_ip ip_param;

        tls_param_get(TLS_PARAM_ID_IP, &ip_param, TRUE);
        ip_param.dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_IP, &ip_param, TRUE);
        tls_wifi_set_oneshot_flag(1);		/*Enable oneshot configuration*/
        printf("\nwait one shot......\n");
    }
    tls_netif_add_status_event(mcast_net_status_changed_event);

    for(;;)
    {
        tls_os_queue_receive(demo_mcast_q, (void **)&msg, 0, 0);
        //printf("\n msg =%d\n",msg);
        switch((u32)msg)
        {
        case DEMO_MSG_WJOIN_SUCCESS:
            break;

        case DEMO_MSG_SOCKET_CREATE:
            mcastsendrcv(is_snd);
            break;

        case DEMO_MSG_WJOIN_FAILD:
            break;

        case DEMO_MSG_SOCKET_ERR:
            printf("\nsocket err\n");
            break;

        default:
            break;
        }
    }

}

#endif

