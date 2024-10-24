/**
 * @file    wm_socket_server_demo.c
 *
 * @brief   socket demo function
 *
 * @Author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#include "wm_include.h"
#include <string.h>
#include "wm_demo.h"


#if DEMO_STD_SOCKET_SERVER
#define     DEMO_SOCK_S_TASK_SIZE      		512
#define     DEMO_SOCK_BUF_SIZE            	1024
#define     BACKLOG     7       // how many pending connections queue will hold


#define		DEMO_SOCK_MSG_UART_RX			1
#define		DEMO_SOCK_MSG_UART_OFF			2


static OS_STK   sock_s_task_stk[DEMO_SOCK_S_TASK_SIZE];
static OS_STK   sock_s_rcv_task_stk[DEMO_SOCK_S_TASK_SIZE];
static OS_STK   sock_s_snd_task_stk[DEMO_SOCK_S_TASK_SIZE];


/**
 * @typedef struct demo_sock_s
 */
typedef struct demo_sock_s
{
    //    OS_STK *sock_s_task_stk;
    tls_os_queue_t *sock_s_q;
    //    OS_STK *sock_rcv_task_stk;
    //    OS_STK *sock_snd_task_stk;
    char *sock_rx;
    char *sock_tx;
    bool socket_ok;
    int server_fd;
    int my_port;
    int conn_num;
    int client_fd_queue[BACKLOG];
    u32 rcv_data_len[BACKLOG];
    int if_snd[BACKLOG];
    int all_snd;
    int uart_trans;
    int uart_txlen;
    int snd_data_len[BACKLOG];
} ST_Demo_Sock_S;

typedef struct demo_socks_uart
{
    u8 *rxbuf;
    u16 rxlen;
} ST_Demo_Socks_uart;


ST_Demo_Sock_S *demo_sock_s = NULL;

tls_os_queue_t *demo_socks_q = NULL;

ST_Demo_Socks_uart demo_socks_uart = {0};



static void demo_sock_s_task(void *sdata);
static void sock_server_recv_task(void *sdata);
static void sock_server_snd_task(void *sdata);

static void sock_server_recv_task(void *sdata)
{
    ST_Demo_Sock_S *sock_s = (ST_Demo_Sock_S *) sdata;
    int ret = 0;
    fd_set fdsr;
    //fd_set fdexpt;
    int maxsock = 0;
#if 1
    struct timeval tv;
#endif
    int i;

    for (;;)
    {
        tls_os_time_delay(1);
        if (sock_s->conn_num < 1)
        {
            // printf("no socket \n");
            continue;
        }
        /** initialize file descriptor set */
        FD_ZERO(&fdsr);
        //     FD_ZERO(&fdexpt);
#if 1       /**not use timeout*/
        /** timeout setting */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
#endif
        maxsock = 0;
        /** add active connection to fd set */
        for (i = 0; i < BACKLOG; i++)
        {
            if (sock_s->client_fd_queue[i] != -1)
            {
                FD_SET(sock_s->client_fd_queue[i], &fdsr);
                //   FD_SET(sock_s->client_fd_queue[i], &fdexpt);
                if (sock_s->client_fd_queue[i] > maxsock)
                {
                    maxsock = sock_s->client_fd_queue[i];
                }
            }
        }

        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        //ret = select(maxsock + 1, &fdsr, NULL, &fdexpt, NULL);
        //    printf("recv select ret=%d\n",ret);
        if (0 == ret)
        {
            //            printf("select ret=%d\n",ret);
            continue;
        }
        else if (-1 == ret)
        {
            printf("select error\n");
            continue;
        }

        for (i = 0; i < BACKLOG; i++)
        {
            if (-1 == sock_s->client_fd_queue[i])
            {
                continue;
            }
            if (FD_ISSET(sock_s->client_fd_queue[i], &fdsr))
            {
                memset(sock_s->sock_rx, 0, DEMO_SOCK_BUF_SIZE);
                ret = recv(sock_s->client_fd_queue[i], sock_s->sock_rx, DEMO_SOCK_BUF_SIZE, 0);
                if (ret <= 0)
                {
                    // client close
                    printf("client[%d] close\n", i);
                    closesocket(sock_s->client_fd_queue[i]);
                    FD_CLR(sock_s->client_fd_queue[i], &fdsr);
                    sock_s->conn_num --;
                    if (sock_s->conn_num <= 0)
                    {
                        sock_s->socket_ok = 0;
                    }
                    sock_s->client_fd_queue[i] = -1;
                }
                else
                {
                    // receive data
                    sock_s->rcv_data_len[i] += ret;
                    if(sock_s->uart_trans
                            && sock_s->if_snd[i])
                    {
                        tls_uart_write(TLS_UART_1, sock_s->sock_rx, ret);
                    }
                    else
                    {
                        printf("\nsock[%d] rcv len==%d\n", sock_s->client_fd_queue[i], sock_s->rcv_data_len[i]);
                    }
                }
            }

            //    if (FD_ISSET(sock_s->client_fd_queue[i], &fdexpt))
            //    {
            //        printf("expt error\n");
            //    }
        }

    }
}

static void sock_server_snd_task(void *sdata)
{
    ST_Demo_Sock_S *sock_s = (ST_Demo_Sock_S *) sdata;
    void *msg;
    int i;
    int len = DEMO_SOCK_BUF_SIZE;
    int ret;
    fd_set fdsr;
    int maxsock = 0;
    u8 if_send = 0;
    void *uart_msg;
    u16 offset = 0;
    u16 readlen = 0;

    for (;;)
    {
        tls_os_queue_receive(sock_s->sock_s_q, (void **) &msg, 0, 0);
        //   printf("msg=%d\n",msg);
        switch ((u32) msg)
        {
        case DEMO_MSG_UART_RECEIVE_DATA:
            while(1)
            {
                tls_os_time_delay(1);       /**adjust send speed*/
                FD_ZERO(&fdsr);
                /** add active connection to fd set */
                maxsock = 0;
                if_send = 0;
                for (i = 0; i < BACKLOG; i++)
                {
                    if (sock_s->client_fd_queue[i] != -1 && (sock_s->snd_data_len[i] != 0 || sock_s->uart_trans) &&
                            (1 == sock_s->if_snd[i] || 1 == sock_s->all_snd))
                    {
                        if_send = 1;
                        FD_SET(sock_s->client_fd_queue[i], &fdsr);

                        if (sock_s->client_fd_queue[i] > maxsock)
                        {
                            maxsock = sock_s->client_fd_queue[i];
                        }
                    }
                }
                //printf("maxsock==%d\n",maxsock);
                if (0 == if_send)
                {
                    //      printf("do not send\n");
                    break;
                }

                ret = select(maxsock + 1, NULL, &fdsr, NULL, NULL);
                if (ret <= 0)
                {
                    printf("send select ret=%d\n", ret);
                    continue;
                }

                if(sock_s->uart_trans)
                {
                    tls_os_queue_receive(demo_socks_q, (void **) &uart_msg, 0, 0);

                    if((u32)uart_msg != DEMO_SOCK_MSG_UART_RX)
                    {
                        continue;
                    }

                    readlen = (demo_socks_uart.rxlen > DEMO_SOCK_BUF_SIZE) ?
                              DEMO_SOCK_BUF_SIZE : demo_socks_uart.rxlen;
                    demo_socks_uart.rxbuf = tls_mem_alloc(readlen);
                    if(demo_socks_uart.rxbuf == NULL)
                    {
                        printf("demo_socks_uart->rxbuf malloc err\n");
                        continue;
                    }
                    ret = tls_uart_read(TLS_UART_1, demo_socks_uart.rxbuf, readlen);
                    if(ret < 0)
                    {
                        tls_mem_free(demo_socks_uart.rxbuf);
                        continue;
                    }
                    demo_socks_uart.rxlen -= ret;
                    readlen = ret;
                }

                for (i = 0; i < BACKLOG; i ++)
                {
                    if (-1 == sock_s->client_fd_queue[i] || (0 == sock_s->if_snd[i] &&
                            0 == sock_s->all_snd))
                    {
                        continue;
                    }
                    if (FD_ISSET(sock_s->client_fd_queue[i], &fdsr))
                    {

                        if(sock_s->uart_trans)
                        {
                            //								printf("send data len:%d\n", uart_msg->rxlen);
                            offset = 0;
                            do
                            {
                                ret = send(sock_s->client_fd_queue[i], demo_socks_uart.rxbuf + offset, readlen, 0);
                                offset += ret;
                                readlen -= ret;
                            }
                            while(readlen);
                            tls_mem_free(demo_socks_uart.rxbuf);
                        }
                        else
                        {
                            if (-1 == sock_s->snd_data_len[i])
                            {
                                len = DEMO_SOCK_BUF_SIZE;
                            }
                            else if(sock_s->snd_data_len[i] != 0)
                            {
                                len = (sock_s->snd_data_len[i] > DEMO_SOCK_BUF_SIZE) ?
                                      DEMO_SOCK_BUF_SIZE : sock_s->snd_data_len[i];
                            }

                            //			printf("send skno=%d\n",sock_s->client_fd_queue[i]);
                            memset(sock_s->sock_tx, sock_s->client_fd_queue[i] + 0x30, len);
                            ret = send(sock_s->client_fd_queue[i], sock_s->sock_tx, len, 0);
                            //			 printf("ret=%d\n",ret);
                            if (ret != -1)
                            {
                                if (sock_s->snd_data_len[i] != -1)
                                {
                                    sock_s->snd_data_len[i] -= ret;
                                }
                            }

                        }
                    }
                }
            }
            break;

        case DEMO_MSG_WJOIN_FAILD:
            sock_s->server_fd = -1;
            sock_s->socket_ok = FALSE;
            break;

        default:
            break;
        }
    }
}
int sck_s_send_data_demo(int skt_no, int len, int uart_trans)
{
    int i = 0;

    printf("\nsktno=%d, len=%d, uart_trans=%d\n", skt_no, len, uart_trans);
    if (NULL == demo_sock_s)
    {
        return WM_FAILED;
    }

    if (0 == skt_no || -1 == skt_no)
    {
        demo_sock_s->all_snd = 1;
        for (i = 0; i < BACKLOG; i ++)
        {
            demo_sock_s->snd_data_len[i] = len;
        }
    }
    else
    {
        demo_sock_s->all_snd = 0;
        for (i = 0; i < BACKLOG; i ++)
        {
            if(demo_sock_s->client_fd_queue[i] != -1)
            {
                if (skt_no == demo_sock_s->client_fd_queue[i])
                {
                    demo_sock_s->if_snd[i] = 1;
                    demo_sock_s->snd_data_len[i] = len;
                }
                else
                {
                    demo_sock_s->if_snd[i] = 0;
                }
            }
        }
    }

    if(demo_sock_s->uart_trans && (0 == uart_trans))
    {
        demo_sock_s->uart_trans = uart_trans;
        tls_os_queue_send(demo_socks_q, (void *) DEMO_SOCK_MSG_UART_OFF, 0);
    }

    demo_sock_s->uart_trans = uart_trans;

    tls_os_queue_send(demo_sock_s->sock_s_q,
                      (void *) DEMO_MSG_UART_RECEIVE_DATA, 0);

    return WM_SUCCESS;
}


static s16 demo_socks_rx(u16 len)
{
    demo_socks_uart.rxlen += len;
    tls_os_queue_send(demo_socks_q, (void *) DEMO_SOCK_MSG_UART_RX, 0);

    return 0;
}


int socket_server_demo(int port)
{
    int i;
    if (demo_sock_s)
    {
        printf("run\n");
        return WM_FAILED;
    }
    if (-1 == port)
    {
        port = 1020;
    }
    printf("server demo port=%d\n", port);
    if (NULL == demo_sock_s)
    {
        demo_sock_s = tls_mem_alloc(sizeof(ST_Demo_Sock_S));
        if (NULL == demo_sock_s)
        {
            goto _error;
        }
        memset(demo_sock_s, 0, sizeof(ST_Demo_Sock_S));
        for (i = 0; i < BACKLOG; i++)
        {
            demo_sock_s->client_fd_queue[i] = -1;
        }

        demo_sock_s->sock_rx = tls_mem_alloc(DEMO_SOCK_BUF_SIZE + 1);
        if (NULL == demo_sock_s->sock_rx)
        {
            goto _error3;
        }

        demo_sock_s->sock_tx = tls_mem_alloc(DEMO_SOCK_BUF_SIZE + 1);
        if (NULL == demo_sock_s->sock_tx)
        {
            goto _error4;
        }

        tls_os_queue_create(&demo_sock_s->sock_s_q, DEMO_QUEUE_SIZE);

        tls_os_queue_create(&demo_socks_q, DEMO_QUEUE_SIZE);

        tls_uart_set_baud_rate(TLS_UART_1, 115200);
        tls_uart_rx_callback_register(TLS_UART_1,(s16(*)(u16, void*))demo_socks_rx, NULL);


        //deal with user's socket message
        tls_os_task_create(NULL, NULL,
                           demo_sock_s_task,
                           (void *) demo_sock_s,
                           (void *) sock_s_task_stk, /* task's stack start address */
                           DEMO_SOCK_S_TASK_SIZE*sizeof(u32),    /* task's stack size, unit:byte */
                           DEMO_SOCKET_S_TASK_PRIO,
                           0);

        //deal with socket's rx data
        tls_os_task_create(NULL, "recv",
                           sock_server_recv_task,
                           (void *) demo_sock_s,
                           (void *) sock_s_rcv_task_stk,    /* task's stack start address */
                           DEMO_SOCK_S_TASK_SIZE*sizeof(u32),           /* task's stack size, unit:byte */
                           DEMO_SOCKET_S_RECEIVE_TASK_PRIO,
                           0);
        // deal with socket's tx data
        tls_os_task_create(NULL, "send",
                           sock_server_snd_task,
                           (void *) demo_sock_s,
                           (void *) sock_s_snd_task_stk,    /* task's stack start address */
                           DEMO_SOCK_S_TASK_SIZE*sizeof(u32),           /* task's stack size, unit:byte */
                           DEMO_SOCKET_S_SEND_TASK_PRIO,
                           0);
    }

    demo_sock_s->my_port = port;

    return WM_SUCCESS;


    tls_mem_free(demo_sock_s->sock_tx);
_error4:
    tls_mem_free(demo_sock_s->sock_rx);
_error3:
    tls_mem_free(demo_sock_s);
    demo_sock_s = NULL;
_error:
    printf("mem err\n");
    return WM_FAILED;
}


static void sock_s_net_status_changed_event(u8 status)
{
    switch (status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        tls_os_queue_send(demo_sock_s->sock_s_q,
                          (void *) DEMO_MSG_WJOIN_FAILD, 0);
        break;
    case NETIF_WIFI_JOIN_SUCCESS:
        tls_os_queue_send(demo_sock_s->sock_s_q,
                          (void *) DEMO_MSG_WJOIN_SUCCESS, 0);
        break;
    case NETIF_IP_NET_UP:
        tls_os_queue_send(demo_sock_s->sock_s_q,
                          (void *) DEMO_MSG_SOCKET_CREATE, 0);
        break;
    default:
        break;
    }
}

static void showclient(void)
{
    int i;

    printf("client count: %d\n", demo_sock_s->conn_num);
    for (i = 0; i < BACKLOG; i++)
    {
        printf("[%d]:%d  ", i, demo_sock_s->client_fd_queue[i]);
    }
    printf("\n");
}

static int socket_server(void)
{
    int new_fd;                 // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr; // server address information
    struct sockaddr_in client_addr; // connector's address information
    socklen_t sin_size;
    int yes = 1;
    int i;
    fd_set fdsr;
    int ret;

    if ((demo_sock_s->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket err\n");
        return WM_FAILED;
    }

    if (setsockopt(demo_sock_s->server_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1)
    {
        printf("set sockopt err\n");
        return WM_FAILED;
    }

    /** host byte order */
    server_addr.sin_family = AF_INET;
    /** short, network byte order */
    server_addr.sin_port = htons(demo_sock_s->my_port);
    /** automatically fill with my IP */
    server_addr.sin_addr.s_addr = ((u32) 0x00000000UL);
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(demo_sock_s->server_fd, (struct sockaddr *) &server_addr,
             sizeof(server_addr)) == -1)
    {
        printf("bind err\n");
        return 1;
    }

    if (listen(demo_sock_s->server_fd, BACKLOG) == -1)
    {
        printf("listen err\n");
        return 1;
    }

    printf("listen port %d\n", demo_sock_s->my_port);

    demo_sock_s->conn_num = 0;

    sin_size = sizeof(client_addr);

    while (1)
    {
        // initialize file descriptor set
        FD_ZERO(&fdsr);
        FD_SET(demo_sock_s->server_fd, &fdsr);

        ret = select(demo_sock_s->server_fd + 1, &fdsr, NULL, NULL, NULL);
        if(ret <= 0)
        {
            continue;
        }
        if (FD_ISSET(demo_sock_s->server_fd, &fdsr))
        {
            new_fd = accept(demo_sock_s->server_fd, (struct sockaddr *) &client_addr,
                            &sin_size);
            printf("accept newfd=%d\n", new_fd);
            if (new_fd <= 0)
            {
                printf("accept err\n");
                continue;
            }
            // add to fd queue
            if (demo_sock_s->conn_num < BACKLOG)
            {
                for (i = 0; i < BACKLOG; i++)
                {
                    if (demo_sock_s->client_fd_queue[i] == -1)
                    {
                        demo_sock_s->client_fd_queue[i] = new_fd;
                        demo_sock_s->conn_num++;
                        if (demo_sock_s->conn_num)
                        {
                            demo_sock_s->socket_ok = 1;
                            tls_os_queue_send(demo_sock_s->sock_s_q,
                                              (void *) DEMO_MSG_UART_RECEIVE_DATA, 0);
                        }
                        break;
                    }
                }
                printf("new connection client[%d] %s:%d\n",
                       demo_sock_s->conn_num, inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port));
            }
            else
            {
                printf("max connections arrive, exit\n");
                closesocket(new_fd);
            }

            showclient();
        }
    }
#if 0
    // close other connections
    for (i = 0; i < BACKLOG; i++)
    {
        if (demo_sock_s->client_fd_queue[i] != -1)
        {
            closesocket(demo_sock_s->client_fd_queue[i]);
        }
    }
    closesocket(demo_sock_s->server_fd);
    return 0;
#endif
}


static void demo_sock_s_task(void *sdata)
{
    ST_Demo_Sock_S *sock_s = (ST_Demo_Sock_S *) sdata;
    void *msg;
    struct tls_ethif *ethif = tls_netif_get_ethif();

    printf("\nsock s task\n");

    if (ethif->status)          /*connected to ap and get IP*/
    {
        tls_os_queue_send(sock_s->sock_s_q, (void *) DEMO_MSG_SOCKET_CREATE, 0);
    }
    else
    {
        struct tls_param_ip ip_param;

        tls_param_get(TLS_PARAM_ID_IP, &ip_param, TRUE);
        ip_param.dhcp_enable = TRUE;
        tls_param_set(TLS_PARAM_ID_IP, &ip_param, TRUE);

        tls_wifi_set_oneshot_flag(1);   /*Enable oneshot configuration*/
        printf("\nwait one shot......\n");
    }
    tls_netif_add_status_event(sock_s_net_status_changed_event);

    for (;;)
    {
        tls_os_queue_receive(sock_s->sock_s_q, (void **) &msg, 0, 0);
        switch ((u32) msg)
        {
        case DEMO_MSG_WJOIN_SUCCESS:
            break;

        case DEMO_MSG_SOCKET_CREATE:
            ethif = tls_netif_get_ethif();
            printf("\nserver ip=%d.%d.%d.%d\n",  ip4_addr1(ip_2_ip4(&ethif->ip_addr)), ip4_addr2(ip_2_ip4(&ethif->ip_addr)),
                   ip4_addr3(ip_2_ip4(&ethif->ip_addr)), ip4_addr4(ip_2_ip4(&ethif->ip_addr)));
            socket_server();
            break;

        default:
            break;
        }
    }
}

#endif
