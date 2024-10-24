/*****************************************************************************
*
* File Name : wm_mqtt_demo.c
*
* Description: mqtt demo function
*
* Copyright (c) 2015 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : LiLimin
*
* Date : 2019-3-24
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
#include "libemqtt.h"

#if DEMO_MQTT

#define MQTT_DEMO_TASK_PRIO             39
#define MQTT_DEMO_TASK_SIZE             512
#define MQTT_DEMO_QUEUE_SIZE            4

#define MQTT_DEMO_RECV_BUF_LEN_MAX      1024

#define MQTT_DEMO_CMD_START             0x1
#define MQTT_DEMO_CMD_HEART             0x2
#define MQTT_DEMO_CMD_LOOP              0x3

#define MQTT_DEMO_READ_TIMEOUT        (-1000)

#define MQTT_DEMO_READ_TIME_SEC         1
#define MQTT_DEMO_READ_TIME_US          0

#define MQTT_DEMO_CLIENT_ID            "wm_mqtt_client"
#define MQTT_DEMO_TX_PUB_TOPIC         "winnermicro/mqtt_tx_demo"
#define MQTT_DEMO_RX_PUB_TOPIC         "winnermicro/mqtt_rx_demo"

#define MQTT_DEMO_SERVER_ADDR          "mqtt.yichen.link"
#define MQTT_DEMO_SERVER_PORT           3883

static bool mqtt_demo_inited = FALSE;
static OS_STK mqtt_demo_task_stk[MQTT_DEMO_TASK_SIZE];
static tls_os_queue_t *mqtt_demo_task_queue = NULL;
static tls_os_timer_t *mqtt_demo_heartbeat_timer = NULL;

static int mqtt_demo_socket_id;
static int mqtt_demo_mqtt_keepalive = 300;
static mqtt_broker_handle_t mqtt_demo_mqtt_broker;

static uint8_t mqtt_demo_packet_buffer[MQTT_DEMO_RECV_BUF_LEN_MAX];

extern struct netif *tls_get_netif(void);
extern int wm_printf(const char *fmt, ...);

static void mqtt_demo_net_status(u8 status)
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
        tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_START, 0);
        break;
    default:
        break;
    }
}

static void mqtt_demo_heart_timer(void *ptmr, void *parg)
{
    tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_HEART, 0);
}

static int mqtt_demo_close_socket(mqtt_broker_handle_t *broker)
{
    int fd = broker->socketid;
    return closesocket(fd);
}

static int mqtt_demo_send_packet(int socket_info, const void *buf, unsigned int count)
{
    int fd = socket_info;
    return send(fd, buf, count, 0);
}

static int mqtt_demo_read_packet(int sec, int us)
{
    int ret = 0;

    if ((sec >= 0) || (us >= 0))
    {
        fd_set readfds;
        struct timeval tmv;

        // Initialize the file descriptor set
        FD_ZERO (&readfds);
        FD_SET (mqtt_demo_socket_id, &readfds);

        // Initialize the timeout data structure
        tmv.tv_sec = sec;
        tmv.tv_usec = us;

        // select returns 0 if timeout, 1 if input available, -1 if error
        ret = select(mqtt_demo_socket_id + 1, &readfds, NULL, NULL, &tmv);
        if(ret < 0)
            return -2;
        else if(ret == 0)
            return MQTT_DEMO_READ_TIMEOUT;

    }

    int total_bytes = 0, bytes_rcvd, packet_length;
    memset(mqtt_demo_packet_buffer, 0, sizeof(mqtt_demo_packet_buffer));

    if((bytes_rcvd = recv(mqtt_demo_socket_id, (mqtt_demo_packet_buffer + total_bytes), MQTT_DEMO_RECV_BUF_LEN_MAX, 0)) <= 0)
    {
        //printf("%d, %d\r\n", bytes_rcvd, mqtt_demo_socket_id);
        return -1;
    }
    //printf("recv [len=%d] : %s\n", bytes_rcvd, mqtt_demo_packet_buffer);
    total_bytes += bytes_rcvd; // Keep tally of total bytes
    if (total_bytes < 2)
        return -1;

    // now we have the full fixed header in mqtt_demo_packet_buffer
    // parse it for remaining length and number of bytes
    uint16_t rem_len = mqtt_parse_rem_len(mqtt_demo_packet_buffer);
    uint8_t rem_len_bytes = mqtt_num_rem_len_bytes(mqtt_demo_packet_buffer);

    //packet_length = mqtt_demo_packet_buffer[1] + 2; // Remaining length + fixed header length
    // total packet length = remaining length + byte 1 of fixed header + remaning length part of fixed header
    packet_length = rem_len + rem_len_bytes + 1;

    while(total_bytes < packet_length) // Reading the packet
    {
        if((bytes_rcvd = recv(mqtt_demo_socket_id, (mqtt_demo_packet_buffer + total_bytes), MQTT_DEMO_RECV_BUF_LEN_MAX, 0)) <= 0)
            return -1;
        total_bytes += bytes_rcvd; // Keep tally of total bytes
    }

    return packet_length;
}

static int mqtt_demo_init_socket(mqtt_broker_handle_t *broker, const char *hostname, short port, int keepalive)
{
    int flag = 1;
    struct hostent *hp;

    // Create the socket
    if((mqtt_demo_socket_id = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // Disable Nagle Algorithm
    if (setsockopt(mqtt_demo_socket_id, IPPROTO_TCP, 0x01, (char *)&flag, sizeof(flag)) < 0)
    {
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -2;
    }

    // query host ip start
    hp = gethostbyname(hostname);
    if (hp == NULL )
    {
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -2;
    }

    struct sockaddr_in socket_address;
    memset(&socket_address, 0, sizeof(struct sockaddr_in));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    memcpy(&(socket_address.sin_addr), hp->h_addr, hp->h_length);

    // Connect the socket
    if((connect(mqtt_demo_socket_id, (struct sockaddr *)&socket_address, sizeof(socket_address))) < 0)
    {
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -1;
    }

    // MQTT stuffs
    mqtt_set_alive(broker, mqtt_demo_mqtt_keepalive);
    broker->socketid = mqtt_demo_socket_id;
    broker->mqttsend = mqtt_demo_send_packet;
    //wm_printf("socket id = %d\n", mqtt_demo_socket_id);
    return 0;
}

static int mqtt_demo_init(void)
{
    int packet_length, ret = 0;
    uint16_t msg_id, msg_id_rcv;

    wm_printf("step1: init mqtt lib.\r\n");
    mqtt_init(&mqtt_demo_mqtt_broker, MQTT_DEMO_CLIENT_ID);

    wm_printf("step2: establishing TCP connection.\r\n");
    ret = mqtt_demo_init_socket(&mqtt_demo_mqtt_broker, MQTT_DEMO_SERVER_ADDR, MQTT_DEMO_SERVER_PORT, mqtt_demo_mqtt_keepalive);
    if(ret)
    {
        wm_printf("init_socket ret=%d\n", ret);
        return -4;
    }

    wm_printf("step3: establishing mqtt connection.\r\n");
    ret = mqtt_connect(&mqtt_demo_mqtt_broker);
    if(ret)
    {
        wm_printf("mqtt_connect ret=%d\n", ret);
        return -5;
    }

    packet_length = mqtt_demo_read_packet(MQTT_DEMO_READ_TIME_SEC, MQTT_DEMO_READ_TIME_US);
    if(packet_length < 0)
    {
        wm_printf("Error(%d) on read packet!\n", packet_length);
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -1;
    }

    if(MQTTParseMessageType(mqtt_demo_packet_buffer) != MQTT_MSG_CONNACK)
    {
        wm_printf("CONNACK expected!\n");
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -2;
    }

    if(mqtt_demo_packet_buffer[3] != 0x00)
    {
        wm_printf("CONNACK failed!\n");
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -2;
    }


    wm_printf("step4: subscribe mqtt\r\n");
    mqtt_subscribe(&mqtt_demo_mqtt_broker, MQTT_DEMO_TX_PUB_TOPIC, &msg_id);

    packet_length = mqtt_demo_read_packet(MQTT_DEMO_READ_TIME_SEC, MQTT_DEMO_READ_TIME_US);
    if(packet_length < 0)
    {
        wm_printf("Error(%d) on read packet!\n", packet_length);
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -1;
    }

    if(MQTTParseMessageType(mqtt_demo_packet_buffer) != MQTT_MSG_SUBACK)
    {
        wm_printf("SUBACK expected!\n");
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -2;
    }

    msg_id_rcv = mqtt_parse_msg_id(mqtt_demo_packet_buffer);
    if(msg_id != msg_id_rcv)
    {
        wm_printf("%d message id was expected, but %d message id was found!\n", msg_id, msg_id_rcv);
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
        return -3;
    }

    wm_printf("step5: start the Heart-beat preservation timer\r\n");
    ret = tls_os_timer_create(&mqtt_demo_heartbeat_timer,
                              mqtt_demo_heart_timer,
                              NULL, (10 * HZ), TRUE, NULL);
    if (TLS_OS_SUCCESS == ret)
        tls_os_timer_start(mqtt_demo_heartbeat_timer);

    /* step6: push mqtt subscription (when a subscription message is received) */

    return 0;
}

static int mqtt_demo_loop(void)
{
    int packet_length = 0;
    int counter = 0;

    counter++;
    packet_length = mqtt_demo_read_packet(0, 1);
    if(packet_length > 0)
    {
        //wm_printf("recvd Packet Header: 0x%x...\n", mqtt_demo_packet_buffer[0]);

        if (MQTTParseMessageType(mqtt_demo_packet_buffer) == MQTT_MSG_PUBLISH)
        {
            uint8_t topic[100], *msg;
            uint16_t len;
            len = mqtt_parse_pub_topic(mqtt_demo_packet_buffer, topic);
            topic[len] = '\0'; // for printf
            len = mqtt_parse_publish_msg(mqtt_demo_packet_buffer, &msg);
            msg[len] = '\0'; // for printf
            wm_printf("recvd: %s >>> %s\n", topic, msg);

            mqtt_publish(&mqtt_demo_mqtt_broker, (const char *)MQTT_DEMO_RX_PUB_TOPIC, (const char *)msg, len, 0);
            wm_printf("pushed: %s <<< %s\n", MQTT_DEMO_RX_PUB_TOPIC, msg);
        }

        tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_LOOP, 0);
    }
    else if (packet_length == MQTT_DEMO_READ_TIMEOUT)
    {
        tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_LOOP, 0);
    }
    else if(packet_length == -1)
    {
        wm_printf("mqtt error:(%d), stop mqtt demo!\n", packet_length);
        tls_os_timer_stop(mqtt_demo_heartbeat_timer);
        mqtt_demo_close_socket(&mqtt_demo_mqtt_broker);
    }

    return 0;
}

static void mqtt_demo_task(void *p)
{
    int ret;
    void *msg;
    struct tls_ethif *ether_if = tls_netif_get_ethif();

    if (ether_if->status)
    {
        wm_printf("sta ip: %v\n", ether_if->ip_addr.addr);
        tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_START, 0);
    }

    for ( ; ; )
    {
        ret = tls_os_queue_receive(mqtt_demo_task_queue, (void **)&msg, 0, 0);
        if (!ret)
        {
            switch((u32)msg)
            {
            case MQTT_DEMO_CMD_START:
                do
                {
                    ret = mqtt_demo_init();
                    if (ret)
                        break;
                    tls_os_queue_send(mqtt_demo_task_queue, (void *)MQTT_DEMO_CMD_LOOP, 0);
                }
                while (0);
                break;
            case MQTT_DEMO_CMD_HEART:
                wm_printf("send heart ping\r\n");
                mqtt_ping(&mqtt_demo_mqtt_broker);
                break;
            case MQTT_DEMO_CMD_LOOP:
                mqtt_demo_loop();
                break;
            default:
                break;
            }
        }
    }
}


//mqtt demo
//测试服务器:mqtt.yichen.link:3883
//服务器端用于发送的订阅主题为:winnermicro/mqtt_tx_demo
//服务器端用于接收的订阅主题为:winnermicro/mqtt_rx_demo
//工作流程: 接收到winnermicro/mqtt_tx_demo推送的消息后打印在屏幕上，并再次推送到winnermicro/mqtt_rx_demo
int mqtt_demo(void)
{
    if (!mqtt_demo_inited)
    {
        tls_os_task_create(NULL, NULL, mqtt_demo_task,
                           NULL, (void *)mqtt_demo_task_stk,  /* task's stack start address */
                           MQTT_DEMO_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
                           MQTT_DEMO_TASK_PRIO, 0);

        tls_os_queue_create(&mqtt_demo_task_queue, MQTT_DEMO_QUEUE_SIZE);

        tls_netif_add_status_event(mqtt_demo_net_status);

        mqtt_demo_inited = TRUE;
    }

    return WM_SUCCESS;
}

#endif

