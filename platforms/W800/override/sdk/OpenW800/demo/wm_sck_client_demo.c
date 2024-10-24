#include<stdio.h>
#include <string.h>
#include "wm_include.h"
#include "wm_demo.h"
#include "lwip/errno.h"


#define     DEMO_SOCK_BUF_SIZE            	512
static const char *sock_tx = "message from client";

int socket_num = -1;
static int socket_client_connect(int port, char *ip);

static void c_con_net_status_changed_event(u8 status )
{
	switch(status)
	{
		case NETIF_IP_NET_UP:
		{
			struct tls_ethif * tmpethif = tls_netif_get_ethif();
#if TLS_CONFIG_IPV4
			print_ipaddr(&tmpethif->ip_addr);
#if TLS_CONFIG_IPV6
			print_ipaddr(&tmpethif->ip6_addr[0]);
			print_ipaddr(&tmpethif->ip6_addr[1]);
			print_ipaddr(&tmpethif->ip6_addr[2]);
#endif
#else
			printf("net up ==> ip = %d.%d.%d.%d\n",ip4_addr1(&tmpethif->ip_addr.addr),ip4_addr2(&tmpethif->ip_addr.addr),
							 ip4_addr3(&tmpethif->ip_addr.addr),ip4_addr4(&tmpethif->ip_addr.addr));
					
#endif
		}
			break;
		
		case NETIF_WIFI_JOIN_FAILED:
			break;
		case NETIF_WIFI_JOIN_SUCCESS:
			break;
		case NETIF_WIFI_DISCONNECTED:
			//shutdown(socket_num,0);
			closesocket(socket_num);
			socket_num = -1;
			break;
		default:
			break;
	}
}


static int c_connect_wifi(char *ssid, char *pwd)
{
	if (!ssid)
	{
		return WM_FAILED;
	}
	printf("\nssid:%s\n", ssid);
	printf("password=%s\n",pwd);
	
	tls_netif_add_status_event(c_con_net_status_changed_event);
	
	tls_wifi_connect((u8 *)ssid, strlen(ssid), (u8 *)pwd, strlen(pwd));
	return 0;
}


static int socket_client_connect(int port, char *ip)
{
    int ret;
	char sock_rx[DEMO_SOCK_BUF_SIZE] = {0};
	struct sockaddr_in pin;
    printf("port:%d,ip:%s\n",port,ip);
	while(1)
	{
		memset(&pin, 0, sizeof(struct sockaddr));
    	pin.sin_family=AF_INET;
    	
    	pin.sin_addr.s_addr = ipaddr_addr(ip);
    	pin.sin_port=htons(port);
		if((socket_num = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
		{
			printf("creat socket fail, errno:%d\n",errno);
			return -1;
		}
		
		if (connect(socket_num, (struct sockaddr *)&pin, sizeof(struct sockaddr)) != 0)
    	{
        	printf("connect fail errno:%d\n",errno);
    	}
    	else
    	{
			printf("connect success\n");
    	}
		while(1)
		{
			tls_os_time_delay(1000);
			ret = send(socket_num, sock_tx, strlen(sock_tx), 0);
			if(ret < 0)
			{
				printf("Error occured during sending, errno:%d\n",errno);
				break;
			}
			
            ret = recv(socket_num, sock_rx, sizeof(sock_rx)-1, 0);	
			if(ret < 0)
			{
				printf("Receive failed, errno:%d\n",errno);
				break;
			}
			else
			{
				sock_rx[ret] = 0;
                printf("\nReceive %d bytes from %s\n",ret,ip);	
				printf("%s\n",sock_rx);
			}
			tls_os_time_delay(2);
		}
		if(socket_num != -1)
		{
			printf("shutting down socket and restaring...\n");
			shutdown(socket_num,0);
			closesocket(socket_num);
			socket_num = -1;
		}
	}
}    

int demo_socket_client(char *ssid, char *pwd,int port,char *ip)
{
	struct tls_ethif * ethif;
	c_connect_wifi(ssid,pwd);
	while(1)
	{
		tls_os_time_delay(1);
		ethif = tls_netif_get_ethif();
		if(ethif->status)
			break;
	}
	socket_client_connect(port,ip);
	return 0;
}

