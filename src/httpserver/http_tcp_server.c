#include "../new_common.h"
#include "ctype.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../logging/logging.h"
#include "new_http.h"
#include "str_pub.h"

static void tcp_server_thread( beken_thread_arg_t arg );
static void tcp_client_thread( beken_thread_arg_t arg );

#define HTTP_SERVER_PORT            80 /*set up a tcp server,port at 20000*/

void start_tcp_http()
{
    OSStatus err = kNoErr;

    err = rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
									"TCP_server", 
									(beken_thread_function_t)tcp_server_thread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
       ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"TCP_server\" thread failed!\r\n");
    }
}


int sendfn(int fd, char * data, int len){
  if (fd){
    return send( fd, data, len, 0 );
  }
  return -1;
}

static void tcp_client_thread( beken_thread_arg_t arg )
{
  OSStatus err = kNoErr;
  int fd = (int) arg;
  //fd_set readfds, errfds, readfds2; 
  char *buf = NULL;
  char *reply = NULL;
	int replyBufferSize = 10000;
	//int res;
	//char reply[8192];

  //my_fd = fd;

  reply = (char*) os_malloc( replyBufferSize );
  buf = (char*) os_malloc( 1026 );
  ASSERT(buf);
  
  http_request_t request;
  os_memset(&request, 0, sizeof(request));

  request.fd = fd;
  request.received = buf;
  request.receivedLen = recv( fd, request.received, 1024, 0 );
  request.received[request.receivedLen] = 0;

  request.reply = reply;
  request.replylen = 0;
  reply[0] = '\0';

  request.replymaxlen = replyBufferSize - 1;

  if ( request.receivedLen <= 0 )
  {
      ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client is disconnected, fd: %d", fd );
      goto exit;
  }

  //addLog( "TCP received string %s\n",buf );
  // returns length to be sent if any
  int lenret = HTTP_ProcessPacket(&request);
  if (lenret > 0){
    ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP sending reply len %i\n",lenret );
    send( fd, reply, lenret, 0 );
  }

  rtos_delay_milliseconds(10);

exit:
  if ( err != kNoErr ) 
    ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP client thread exit with err: %d", err );

  if ( buf != NULL ) 
    os_free( buf );
  if ( reply != NULL ) 
    os_free( reply );

  close( fd );
  rtos_delete_thread( NULL );
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

    tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
    server_addr.sin_port = htons( HTTP_SERVER_PORT );/* Server listen on port: 20000 */
    err = bind( tcp_listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );
    
    err = listen( tcp_listen_fd, 0 );
    
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
                ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                if ( kNoErr
                     != rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
							                     "TCP Clients",
                                                 (beken_thread_function_t)tcp_client_thread,
                                                 0x800, 
                                                 (beken_thread_arg_t)client_fd ) ) 
                {
                  close( client_fd );
                  client_fd = -1;
                }
            }
        }
    }
	
    if ( err != kNoErr ) 
		  ADDLOG_ERROR(LOG_FEATURE_HTTP,  "Server listerner thread exit with err: %d", err );
	
    close( tcp_listen_fd );
    rtos_delete_thread( NULL );
}
