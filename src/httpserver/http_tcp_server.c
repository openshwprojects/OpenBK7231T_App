
#include "../new_common.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../logging/logging.h"
#include "new_http.h"

#define HTTP_SERVER_PORT            80

#define REPLY_BUFFER_SIZE			10000
#define INCOMING_BUFFER_SIZE		1024

xTaskHandle g_http_thread = NULL;

int sendfn(int fd, char * data, int len){
	if (fd){
		return send( fd, data, len, 0 );
	}
	return -1;
}

static void tcp_client_thread( int fd, char *buf, char *reply )
{
	OSStatus err = kNoErr;

	http_request_t request;
	os_memset(&request, 0, sizeof(request));

	request.fd = fd;
	request.received = buf;
	request.receivedLenmax = INCOMING_BUFFER_SIZE - 1;
	request.responseCode = HTTP_RESPONSE_OK;
	request.receivedLen = recv( fd, request.received, request.receivedLenmax, 0 );
	request.received[request.receivedLen] = 0;

	request.reply = reply;
	request.replylen = 0;
	reply[0] = '\0';

	request.replymaxlen = REPLY_BUFFER_SIZE - 1;

	if ( request.receivedLen <= 0 )
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client is disconnected, fd: %d", fd );
		return;
	}
	int lenret = HTTP_ProcessPacket(&request);
	if (lenret > 0){
		send( fd, reply, lenret, 0 );
	}
	rtos_delay_milliseconds(10);

}

/* TCP server listener thread */
static void tcp_server_thread( beken_thread_arg_t arg )
{
    (void)( arg );
    OSStatus err = kNoErr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sockaddr_t_size = sizeof(client_addr);
    char client_ip_str[16];
    int tcp_listen_fd = -1, client_fd = -1;
    fd_set readfds;
	char *buf = NULL;
	char *reply = NULL;

    tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
    server_addr.sin_port = htons( HTTP_SERVER_PORT );/* Server listen on port: 20000 */
    err = bind( tcp_listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );

    err = listen( tcp_listen_fd, 0 );

	reply = (char*) os_malloc( REPLY_BUFFER_SIZE );
	buf = (char*) os_malloc( INCOMING_BUFFER_SIZE );

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( tcp_listen_fd, &readfds );

        select( tcp_listen_fd + 1, &readfds, NULL, NULL, NULL);

        if ( FD_ISSET( tcp_listen_fd, &readfds ) )
        {
            client_fd = accept( tcp_listen_fd, (struct sockaddr *) &client_addr, &sockaddr_t_size );
            if ( client_fd >= 0 )
            {
                os_strcpy( client_ip_str, inet_ntoa( client_addr.sin_addr ) );
                //  ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );

				tcp_client_thread(client_fd,reply,buf);

				lwip_close( client_fd );
            }
        }
    }

    if ( err != kNoErr )
		  ADDLOG_ERROR(LOG_FEATURE_HTTP,  "Server listerner thread exit with err: %d", err );

    lwip_close( tcp_listen_fd );

    rtos_delete_thread( NULL );

}

void HTTPServer_Start()
{
    OSStatus err = kNoErr;

    err = rtos_create_thread( &g_http_thread, BEKEN_APPLICATION_PRIORITY,
									"TCP_server",
									(beken_thread_function_t)tcp_server_thread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
       ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"TCP_server\" thread failed with %i!\r\n",err);
    }
}


