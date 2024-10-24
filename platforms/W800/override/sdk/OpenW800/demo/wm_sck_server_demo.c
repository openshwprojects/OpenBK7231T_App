#include "wm_include.h"
#include <string.h>
#include "wm_demo.h"
#include "lwip/errno.h"


#define     DEMO_SOCK_BUF_SIZE     	512
static const char *sock_tx = "message from server";

int new_fd = -1;
int server_fd = -1;

static int create_socket_server(int port);

static void s_con_net_status_changed_event(u8 status )
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
			printf("network disconnection\n");
			closesocket(new_fd);
			closesocket(server_fd);
			new_fd = -1;
			break;
		default:
			break;
	}
}


static int s_connect_wifi(char *ssid, char *pwd)
{
	if (!ssid)
	{
		return WM_FAILED;
	}
	printf("\nssid:%s\n", ssid);
	printf("password=%s\n",pwd);
	tls_netif_add_status_event(s_con_net_status_changed_event);
	
	tls_wifi_connect((u8 *)ssid, strlen(ssid), (u8 *)pwd, strlen(pwd));
	return 0;
}       

static int create_socket_server(int port)
{
   
	char sock_rx[DEMO_SOCK_BUF_SIZE] = {0};
    struct sockaddr_in server_addr; // server address information
    struct sockaddr_in client_addr; // connector's address information
    socklen_t sin_size;
    int ret;	
	while(1)
	{
    	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
        	printf("create socket fail,errno :%d\n",errno);
        	break;
    	}
    	
    	server_addr.sin_family = AF_INET;   
    	server_addr.sin_port = htons(port);
    	server_addr.sin_addr.s_addr = ((u32) 0x00000000UL);   
    	memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    	if (bind(server_fd, (struct sockaddr *) &server_addr,
         	sizeof(server_addr)) != 0)
    	{
        	printf("bind fail, errno:%d\n",errno);
			break;
   	 	}
		
    	if (listen(server_fd, 1) != 0)
    	{
        	printf("listen fail,errno:%d\n",errno);
        	break;
    	}

    	printf("listen port %d\n", port);
    	sin_size = sizeof(client_addr);
		new_fd = accept(server_fd, (struct sockaddr *) &client_addr,
                       &sin_size);
        printf("accept newfd = %d\n",new_fd);
			
        if (new_fd < 0)
        {
              printf("accept fail,errno:%d\n",errno);
              break;
        }
		
    	while (1)
    	{
        	memset(sock_rx, 0, DEMO_SOCK_BUF_SIZE);
            ret = recv(new_fd, sock_rx, sizeof(sock_rx)-1, 0);
			if(ret == 0)
            {
            	printf("connection disconnect\n");
				break;
            }
			else if(ret < 0)
			{
				printf("receive fail errno:%d\n",errno);
				break;
			}
            else
            {     
            	 sock_rx[ret] = 0;
				 printf("\nReceive %d bytes from %s\n", ret, inet_ntoa(client_addr.sin_addr.s_addr));
				 printf("%s\n",sock_rx);
				 ret = send(new_fd, sock_tx, strlen(sock_tx), 0);
				 if (ret < 0)
				 {
				 	printf("Error occured during sending,errno:%d\n",errno);
					break;
				 }
            }
   	 	}
		if(new_fd != -1)
		{
			printf("shutting down socket and restaring...\n");
			shutdown(new_fd,0);
			closesocket(new_fd);
		}
	}
	return 0;
}



void demo_socket_server(char *ssid,char *pwd, int port)
{
	struct tls_ethif * ethif;
	s_connect_wifi(ssid, pwd);
	while(1)
	{
		tls_os_time_delay(1);
		ethif = tls_netif_get_ethif();
		if(ethif->status)
			break;
	}
	create_socket_server(port);
}
