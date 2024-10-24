/**
 * @file    wm_socket_client_demo.c
 *
 * @brief   socket demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"

#if DEMO_STD_SOCKET_CLIENT
#define     DEMO_SOCK_C_TASK_SIZE      		512
#define     DEMO_SOCK_BUF_SIZE            	512

#define		DEMO_SOCK_MSG_UART_RX			1
#define		DEMO_SOCK_MSG_UART_OFF			2



static OS_STK   sock_c_task_stk[DEMO_SOCK_C_TASK_SIZE];
static OS_STK   sock_c_rcv_task_stk[DEMO_SOCK_C_TASK_SIZE];


/**
 * @typedef struct demo_sock_c
 */
typedef struct demo_sock_c
{
    tls_os_queue_t *sock_c_q;
    char *sock_rx;
    char *sock_tx;
    bool socket_ok;
    int socket_num;
    int remote_port;
    u32 remote_ip;
    u32 rcv_data_len;
    int snd_skt_no;
    int snd_data_len;
    int uart_trans;
    int uart_txlen;

} ST_Demo_Sock_C;

typedef struct demo_sockc_uart
{
    u8 *rxbuf;
    u16 rxlen;
} ST_Demo_Sockc_uart;


ST_Demo_Sock_C *demo_sock_c = NULL;

tls_os_queue_t *demo_sockc_q = NULL;

ST_Demo_Sockc_uart demo_sockc_uart = {0};


static void demo_sock_c_task(void *sdata);


int create_client_socket_demo(void)
{
    struct sockaddr_in pin;

    memset(&pin, 0, sizeof(struct sockaddr));
    pin.sin_family = AF_INET;
    demo_sock_c->socket_num = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    pin.sin_addr.s_addr = demo_sock_c->remote_ip;
    pin.sin_port = htons(demo_sock_c->remote_port);

    printf("\nsocket num=%d\n", demo_sock_c->socket_num);
    if (connect(demo_sock_c->socket_num, (struct sockaddr *)&pin, sizeof(struct sockaddr)) != 0)
    {
        printf("connect failed! socket num=%d\n", demo_sock_c->socket_num);
        closesocket(demo_sock_c->socket_num);
        tls_os_queue_send(demo_sock_c->sock_c_q, (void *)DEMO_MSG_SOCKET_ERR, 0);
        return WM_FAILED;
    }
    else
    {
        demo_sock_c->socket_ok = TRUE;
    }

    return WM_SUCCESS;
}

int socket_client_demo(void)
{
    struct tls_ethif *ethif;

    ethif = tls_netif_get_ethif();
    printf("\nip=%d.%d.%d.%d\n",  ip4_addr1(ip_2_ip4(&ethif->ip_addr)), ip4_addr2(ip_2_ip4(&ethif->ip_addr)),
           ip4_addr3(ip_2_ip4(&ethif->ip_addr)), ip4_addr4(ip_2_ip4(&ethif->ip_addr)));


    return create_client_socket_demo();
}

static void sock_client_recv_task(void *sdata)
{
    ST_Demo_Sock_C *sock_c = (ST_Demo_Sock_C *)sdata;
    int ret = 0;
    fd_set fdsr;
    struct timeval tv;

    for(;;)
    {
        if (sock_c->socket_ok)
        {
        	FD_ZERO(&fdsr);
			FD_SET(sock_c->socket_num, &fdsr);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			ret = select(sock_c->socket_num+1, &fdsr, NULL, NULL, &tv);
			if (0 == ret)
			{
				//			  printf("select ret=%d\n",ret);
				continue;
			}
			else if (-1 == ret)
			{
				printf("select error\n");
				continue;
			}

			if (FD_ISSET(sock_c->socket_num, &fdsr))
			{
				FD_CLR(sock_c->socket_num, &fdsr);

	            ret = 0;
	            ret = recv(sock_c->socket_num, sock_c->sock_rx, DEMO_SOCK_BUF_SIZE, 0);

	            if(ret > 0)
	            {
	                sock_c->rcv_data_len += ret;

	                if(sock_c->uart_trans)
	                {
	                    tls_uart_write(TLS_UART_1, sock_c->sock_rx, ret);
	                }
	                else
	                {
	                    printf("rcv: %d\n", sock_c->rcv_data_len);
	                }
	            }
	            else
	            {
	                sock_c->socket_ok = FALSE;
	                closesocket(sock_c->socket_num);
	                printf("closesocket: %d\n", sock_c->socket_num);
	                sock_c->socket_num = 0;
	                tls_os_queue_send(sock_c->sock_c_q, (void *)DEMO_MSG_SOCKET_ERR, 0);
	            }
	            continue;
			}
        }
        tls_os_time_delay(1);
    }
}

int sck_c_send_data_demo(int len, int uart_trans)
{
    printf("\nlen=%d, uart_trans=%d\n", len, uart_trans);
    if (NULL == demo_sock_c)
    {
        return WM_FAILED;
    }
    if (!demo_sock_c->socket_ok)
    {
        printf("skt not created\n");
        return WM_FAILED;
    }

    demo_sock_c->snd_data_len = len;

    if(demo_sock_c->uart_trans && (0 == uart_trans))
    {
        demo_sock_c->uart_trans = uart_trans;
        tls_os_queue_send(demo_sockc_q, (void *) DEMO_SOCK_MSG_UART_OFF, 0);
    }

    demo_sock_c->uart_trans = uart_trans;

    tls_os_queue_send(demo_sock_c->sock_c_q, (void *)DEMO_MSG_UART_RECEIVE_DATA, 0);

    return WM_SUCCESS;
}

static s16 demo_sockc_rx(u16 len)
{
    demo_sockc_uart.rxlen += len;
    tls_os_queue_send(demo_sockc_q, (void *) DEMO_SOCK_MSG_UART_RX, 0);

    return 0;
}


int Socket_Client_Demo(int port, char *ip)
{
    if (demo_sock_c)
    {
        printf("run\n");
        return WM_FAILED;
    }
    if (-1 == port || NULL == ip)
    {
        printf("please input remote port and ip\n");
        return WM_FAILED;
    }
    printf("\nremote ip: %s, remote port: %d\n", ip, port);

    if (NULL == demo_sock_c)
    {
        demo_sock_c = tls_mem_alloc(sizeof(ST_Demo_Sock_C));
        if (NULL == demo_sock_c)
        {
            goto _error;
        }
        memset(demo_sock_c, 0, sizeof(ST_Demo_Sock_C));

        demo_sock_c->sock_rx = tls_mem_alloc(DEMO_SOCK_BUF_SIZE + 1);
        if (NULL == demo_sock_c->sock_rx)
        {
            goto _error3;
        }

        demo_sock_c->sock_tx = tls_mem_alloc(DEMO_SOCK_BUF_SIZE + 1);
        if (NULL == demo_sock_c->sock_tx)
        {
            goto _error4;
        }

        tls_os_queue_create(&(demo_sock_c->sock_c_q), DEMO_QUEUE_SIZE);

        tls_os_queue_create(&demo_sockc_q, DEMO_QUEUE_SIZE);

        tls_uart_set_baud_rate(TLS_UART_1, 115200);
        tls_uart_rx_callback_register(TLS_UART_1, (s16(*)(u16, void*))demo_sockc_rx,NULL);

        //deal with socket's message
        tls_os_task_create(NULL, NULL,
                           demo_sock_c_task,
                           (void *)demo_sock_c,
                           (void *)sock_c_task_stk, /* task's stack start address */
                           DEMO_SOCK_C_TASK_SIZE*sizeof(u32),   /* task's stack size, unit:byte */
                           DEMO_SOCKET_C_TASK_PRIO,
                           0);

        //deal with socket's rx data
        tls_os_task_create(NULL, NULL,
                           sock_client_recv_task,
                           (void *)demo_sock_c,
                           (void *)sock_c_rcv_task_stk, /* task's stack start address */
                           DEMO_SOCK_C_TASK_SIZE*sizeof(u32),       /* task's stack size, unit:byte */
                           DEMO_SOCKET_RECEIVE_TASK_PRIO,
                           0);
    }

    demo_sock_c->remote_port = port;
    demo_sock_c->remote_ip = ipaddr_addr(ip);

    return WM_SUCCESS;

_error4:
    tls_mem_free(demo_sock_c->sock_rx);
_error3:
    tls_mem_free(demo_sock_c);
    demo_sock_c = NULL;
_error:
    return WM_FAILED;
}


static void sock_c_net_status_changed_event(u8 status )
{
    switch(status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        tls_os_queue_send(demo_sock_c->sock_c_q, (void *)DEMO_MSG_WJOIN_FAILD, 0);
        break;
    case NETIF_WIFI_JOIN_SUCCESS:
        tls_os_queue_send(demo_sock_c->sock_c_q, (void *)DEMO_MSG_WJOIN_SUCCESS, 0);
        break;
    case NETIF_IP_NET_UP:
        tls_os_queue_send(demo_sock_c->sock_c_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
        break;
    default:
        break;
    }
}

static void demo_sock_c_task(void *sdata)
{
    ST_Demo_Sock_C *sock_c = (ST_Demo_Sock_C *)sdata;
    void *msg;
    struct tls_ethif *ethif = tls_netif_get_ethif();
    int len;
    int ret;
    void *uart_msg;
    u16 offset = 0;
    u16 readlen = 0;


    printf("\nsock c task\n");

    if(ethif->status)	/*connected to ap and get IP*/
    {
        tls_os_queue_send(sock_c->sock_c_q, (void *)DEMO_MSG_SOCKET_CREATE, 0);
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
    tls_netif_add_status_event(sock_c_net_status_changed_event);

    for (;;)
    {
        tls_os_queue_receive(sock_c->sock_c_q, (void **)&msg, 0, 0);
        //printf("\n msg =%d\n",msg);
        switch((u32)msg)
        {
        case DEMO_MSG_WJOIN_SUCCESS:
            break;

        case DEMO_MSG_SOCKET_CREATE:
            socket_client_demo();
            break;

        case DEMO_MSG_WJOIN_FAILD:
            if(sock_c->socket_num > 0)
            {
                sock_c->socket_num = 0;
                sock_c->socket_ok = FALSE;
            }
            break;

        case DEMO_MSG_SOCKET_RECEIVE_DATA:
            break;

        case DEMO_MSG_UART_RECEIVE_DATA:

            while(sock_c->uart_trans)
            {
                tls_os_queue_receive(demo_sockc_q, (void **) &uart_msg, 0, 0);

                if((u32)uart_msg != DEMO_SOCK_MSG_UART_RX)
                {
                    continue;
                }

                readlen = (demo_sockc_uart.rxlen > DEMO_SOCK_BUF_SIZE) ?
                          DEMO_SOCK_BUF_SIZE : demo_sockc_uart.rxlen;
				if (readlen == 0)
				{
					continue;
				}

                demo_sockc_uart.rxbuf = tls_mem_alloc(readlen);
                if(demo_sockc_uart.rxbuf == NULL)
                {
                    printf("demo_socks_uart->rxbuf malloc err\n");
                    continue;
                }
                ret = tls_uart_read(TLS_UART_1, demo_sockc_uart.rxbuf, readlen);
                if(ret < 0)
                {
                    tls_mem_free(demo_sockc_uart.rxbuf);
                    continue;
                }
                demo_sockc_uart.rxlen -= ret;
                readlen = ret;

                offset = 0;
                do
                {
                    ret = send(sock_c->socket_num, demo_sockc_uart.rxbuf + offset, readlen, 0);
					if (ret < 0)
					{
					}
					else
					{
	                    offset += ret;
	                    readlen -= ret;
					}
                }
                while(readlen);
                tls_mem_free(demo_sockc_uart.rxbuf);
            }

            if (-1 == sock_c->snd_data_len)
            {
                len = DEMO_SOCK_BUF_SIZE;
            }
            else if(sock_c->snd_data_len != 0)
            {
                len = (sock_c->snd_data_len > DEMO_SOCK_BUF_SIZE) ?
                      DEMO_SOCK_BUF_SIZE : sock_c->snd_data_len;
            }
            else
            {
                break;
            }

            memset(sock_c->sock_tx, 0x35, len);
            ret = send(sock_c->socket_num, sock_c->sock_tx, len, 0);
            if (ret != -1)
            {
                if (sock_c->snd_data_len != -1)
                {
                    sock_c->snd_data_len -= ret;
                }
            }
            if (sock_c->socket_ok && sock_c->snd_data_len != 0)
            {
                tls_os_time_delay(1);
                tls_os_queue_send(sock_c->sock_c_q, (void *)DEMO_MSG_UART_RECEIVE_DATA, 0);
            }
            break;

        case DEMO_MSG_SOCKET_ERR:
            tls_os_time_delay(200);
            printf("\nsocket err\n");
            create_client_socket_demo( );
            break;

        default:
            break;
        }
    }

}

#endif
