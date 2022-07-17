#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"


#define CMD_CLIENT_SLEEP_TIME_MS 50
#define CMD_CLIENT_DISCONNECT_AFTER_IDLE_MS (60 * 1000)

#define CMD_SERVER_PORT		100
#define MAX_COMMAND_LEN		64

static xTaskHandle g_cmd_thread = NULL;
static int g_bStarted = 0;

static void CMD_ClientThread(int fd)
{
	char buf[MAX_COMMAND_LEN];
	int len;
	int sleepTime;

	sleepTime = 0;
	//send(fd,"CMD:",5,0);

	while(1) {
		if(sleepTime >= CMD_CLIENT_DISCONNECT_AFTER_IDLE_MS) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "TCP Console dropping because of inactivity" );
			break;
		}
		rtos_delay_milliseconds(CMD_CLIENT_SLEEP_TIME_MS);
		sleepTime += CMD_CLIENT_SLEEP_TIME_MS;
		len = recv( fd, buf, sizeof(buf)-1, 0 );
		if(len < 0) {
			if(errno == EAGAIN) {
				continue;
			}
			break;
		}
		if(len == 0) {
			break;
		}
		if(len > 0) {
			sleepTime = 0;
			buf[len] = 0;
			ADDLOG_ERROR(LOG_FEATURE_CMD, "TCP Console command: %s (raw len %i, strlen %i)", buf, len, strlen(buf) );
			LOG_SetRawSocketCallback(fd);
			CMD_ExecuteCommand(buf,COMMAND_FLAG_SOURCE_TCP);
			LOG_SetRawSocketCallback(0);
			//send(fd,"OK",3,0);
		}
	}

	ADDLOG_ERROR(LOG_FEATURE_CMD, "TCP client endd" );
	rtos_delay_milliseconds(10);


}

/* TCP server listener thread */
static void CMD_ServerThread( beken_thread_arg_t arg )
{
    (void)( arg );
    OSStatus err = kNoErr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sockaddr_t_size = sizeof(client_addr);
    int tcp_listen_fd = -1, client_fd = -1;
    fd_set readfds;

    tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
    server_addr.sin_port = htons( CMD_SERVER_PORT );/* Server listen on port: 20000 */
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
#if 1
				// Put the socket in non-blocking mode:
				if(fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
				    ADDLOG_DEBUG(LOG_FEATURE_CMD,  "CMD Client failed to made non-blocking" );
				}
#endif
             //   os_strcpy( client_ip_str, inet_ntoa( client_addr.sin_addr ) );
           ///     ADDLOG_DEBUG(LOG_FEATURE_CMD,  "CMD Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
				CMD_ClientThread(client_fd);
				lwip_close( client_fd );;
            }
        }
    }

    if ( err != kNoErr )
		  ADDLOG_ERROR(LOG_FEATURE_CMD,  "Server listerner thread exit with err: %d", err );

    lwip_close( tcp_listen_fd );

    rtos_delete_thread( NULL );

}


void CMD_StartTCPCommandLine()
{
    OSStatus err = kNoErr;

	if(g_bStarted) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "CMD server is already running!\r\n");
		return;
	}
    err = rtos_create_thread( &g_cmd_thread, 6,
									"CMD_server",
									(beken_thread_function_t)CMD_ServerThread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "create \"CMD_server\" thread failed with %i!\r\n",err);
    }
	else
	{
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD TCP server started!\r\n");
		g_bStarted = 1;
	}
}


