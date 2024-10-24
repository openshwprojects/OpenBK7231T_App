/*****************************************************************************
*
* File Name : wm_https_demo.c
*
* Description: ssl client demo function
*
* Copyright (c) 2015 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : LiLimin
*
* Date : 2015-3-24
*****************************************************************************/
#include <string.h>
#include "wm_include.h"
#include "wm_netif.h"
#include "wm_demo.h"
#include "tls_common.h"
#include "lwip/netif.h"
#include "wm_sockets.h"
#include "lwip/inet.h"

#include "wm_sockets2.0.3.h"
#include "HTTPClientWrapper.h"

#if DEMO_HTTPS

#define HTTPS_DEMO_TASK_PRIO             38
#define HTTPS_DEMO_TASK_SIZE             2048 //1024
#define HTTPS_DEMO_QUEUE_SIZE            4

#define HTTPS_RECV_BUF_LEN_MAX           1024

#define HTTPS_DEMO_CMD_START             0x1

#define HTTPS_DEMO_SERVER               "www.tencent.com"
#define HTTPS_DEMO_PORT                  443

static bool https_demo_inited = FALSE;
static OS_STK https_demo_task_stk[HTTPS_DEMO_TASK_SIZE];
static tls_os_queue_t *https_demo_task_queue = NULL;

static const char *https_request = "GET /legal/html/zh-cn/index.html HTTP/1.0\r\n"
                                   "Host: "HTTPS_DEMO_SERVER"\r\n"
                                   "User-Agent: W80X\r\n"
                                   "\r\n";

extern struct netif *tls_get_netif(void);
extern int wm_printf(const char *fmt, ...);

static void https_demo_net_status(u8 status)
{
    struct netif *netif = tls_get_netif();

    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        wm_printf("sta join net failed\n");
        break;
    case NETIF_WIFI_DISCONNECTED:
        wm_printf("sta net disconnected\n");
        break;
    case NETIF_IP_NET_UP:
        wm_printf("sta ip: %v\n", netif->ip_addr.addr);
        tls_os_queue_send(https_demo_task_queue, (void *)HTTPS_DEMO_CMD_START, 0);
        break;
    default:
        break;
    }
}

static void https_demo_task(void *p)
{
    int ret;
    void *msg;
    tls_ssl_t *ssl_p;
    int fd;
    struct hostent *hp;
    struct sockaddr_in server;
    char *recvbuf;
    int total = 0;
    struct tls_ethif *ether_if = tls_netif_get_ethif();

    if (ether_if->status)
    {
        wm_printf("sta ip: %v\n", ether_if->ip_addr.addr);
        tls_os_queue_send(https_demo_task_queue, (void *)HTTPS_DEMO_CMD_START, 0);
    }

    for( ; ; )
    {
        ret = tls_os_queue_receive(https_demo_task_queue, (void **)&msg, 0, 0);
        if (!ret)
        {
            switch((u32)msg)
            {
            case HTTPS_DEMO_CMD_START:
                do
                {
                    hp = gethostbyname(HTTPS_DEMO_SERVER);
                    if (hp == NULL )
                    {
                        wm_printf("get address error\r\n");
                        break;
                    }

                    fd = socket(AF_INET, SOCK_STREAM, 0);
                    if (fd < 0)
                    {
                        wm_printf("create socket error\r\n");
                        break;
                    }

                    memset(&server, 0, sizeof(struct sockaddr_in));
                    memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
                    server.sin_family = AF_INET;
                    server.sin_port = htons(HTTPS_DEMO_PORT);

                    wm_printf("step 1: ssl connect to...\r\n");
                    ret = HTTPWrapperSSLConnect(&ssl_p, fd, (const struct sockaddr *)&server, sizeof(server), HTTPS_DEMO_SERVER);
                    if (ret < 0)
                    {
                        wm_printf("https connect error\r\n");
                        close(fd);
                        break;
                    }

                    recvbuf = tls_mem_alloc(HTTPS_RECV_BUF_LEN_MAX + 1);
                    if (!recvbuf)
                    {
                        wm_printf("https malloc error\r\n");
                        HTTPWrapperSSLClose(ssl_p, fd);
                        close(fd);
                        break;
                    }

                    wm_printf("step 2: send https request [%s]\r\n", https_request);
                    ret = HTTPWrapperSSLSend(ssl_p, fd, (char *)https_request, strlen(https_request), 0);
                    if (ret < 0)
                    {
                        wm_printf("https send error\r\n");
                        tls_mem_free(recvbuf);
                        HTTPWrapperSSLClose(ssl_p, fd);
                        close(fd);
                        break;
                    }

                    do
                    {
                        ret = HTTPWrapperSSLRecv(ssl_p, fd, recvbuf, HTTPS_RECV_BUF_LEN_MAX, 0);
                        if (ret <= 0)
                        {
                            if (SOCKET_SSL_MORE_DATA == ret)
                            {
                                recvbuf[HTTPS_RECV_BUF_LEN_MAX] = '\0';
                                wm_printf("step 3: recvd https resp %d bytes [%s]\r\n", HTTPS_RECV_BUF_LEN_MAX, recvbuf);
                                total += HTTPS_RECV_BUF_LEN_MAX;
                                ret = 1;
                            }
                            else
                            {
                                wm_printf("https recv closed\r\n");
                                break;
                            }
                        }
                        else
                        {
                            recvbuf[ret] = '\0';
                            wm_printf("step 3: recvd https resp %d bytes [%s]\r\n", ret, recvbuf);
                            total += ret;
                        }
                    }
                    while (ret > 0);

                    wm_printf("step 3: recvd https resp total %d bytes.\r\n", total);

                    tls_mem_free(recvbuf);
                    HTTPWrapperSSLClose(ssl_p, fd);
                    close(fd);

                    wm_printf("\r\nhttps demo end.\r\n");
                }
                while (0);

                break;
            default:
                break;
            }
        }
    }
}


//https request demo
//This example should make STA connected to AP firstly, then access web page https://www.tencent.com/legal/html/zh-cn/index.html
int https_demo(void)
{
    if (!https_demo_inited)
    {
        tls_os_task_create(NULL, NULL, https_demo_task,
                           NULL, (void *)https_demo_task_stk, /* task's stack start address */
                           HTTPS_DEMO_TASK_SIZE * sizeof(u32),/* task's stack size, unit:byte */
                           HTTPS_DEMO_TASK_PRIO, 0);

        tls_os_queue_create(&https_demo_task_queue, HTTPS_DEMO_QUEUE_SIZE);

        tls_netif_add_status_event(https_demo_net_status);

        https_demo_inited = TRUE;
    }

    return WM_SUCCESS;
}

#endif

