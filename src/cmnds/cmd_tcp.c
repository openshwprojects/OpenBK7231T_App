#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"


#define CMD_SERVER_PORT		100
#define MAX_COMMAND_LEN		64

static xTaskHandle g_cmd_thread = NULL;
static int g_bStarted = 0;

static void CMD_ClientThread( beken_thread_arg_t arg )
{
	OSStatus err = kNoErr;
	int fd = (int) arg;
	char buf[MAX_COMMAND_LEN];
	int len;


	while(1) {
		len = recv( fd, buf, sizeof(buf)-1, 0 );
		if(len < 0) {
			break;
		}
		if(len > 0) {
			buf[len] = 0;
			ADDLOG_ERROR(LOG_FEATURE_CMD, "TCP Console command: %s (raw len %i, strlen %i)", buf, len, strlen(buf) );
			LOG_SetRawSocketCallback(fd);
			CMD_ExecuteCommand(buf,0);
			LOG_SetRawSocketCallback(0);
			//send(fd,"OK",3,0);
		}
	}

	rtos_delay_milliseconds(10);

	if ( err != kNoErr )
		ADDLOG_ERROR(LOG_FEATURE_CMD, "TCP client thread exit with err: %d", err );

	lwip_close( fd );;
#if DISABLE_SEPARATE_THREAD_FOR_EACH_CMD_CLIENT

#else
	rtos_delete_thread( NULL );
#endif
}

/* TCP server listener thread */
static void CMD_ServerThread( beken_thread_arg_t arg )
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
#if PLATFORM_XR809
#if DISABLE_SEPARATE_THREAD_FOR_EACH_CMD_CLIENT

#else
				OS_Thread_t clientThreadUnused ;
#endif
#endif
                os_strcpy( client_ip_str, inet_ntoa( client_addr.sin_addr ) );
                ADDLOG_DEBUG(LOG_FEATURE_CMD,  "CMD Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
#if DISABLE_SEPARATE_THREAD_FOR_EACH_CMD_CLIENT
				// Use main server thread (blocking all other clients)
				// right now, I am getting OS_ThreadCreate everytime on XR809 platform
				CMD_ClientThread((beken_thread_arg_t)client_fd);
#else
				// Create separate thread for client
                if ( kNoErr !=
#if PLATFORM_XR809
					OS_ThreadCreate(&clientThreadUnused,
							                     "CMD Client",
		                CMD_ClientThread,
		                client_fd,
		                OS_THREAD_PRIO_CONSOLE,
		                0x400)

#else
                     rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY,
							                     "CMD Client",
                                                 (beken_thread_function_t)CMD_ClientThread,
                                                 0x800,
                                                 (beken_thread_arg_t)client_fd )

#endif
												 )
                {
                  ADDLOG_DEBUG(LOG_FEATURE_CMD,  "CMD Client %s:%d thread creation failed! fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                  lwip_close( client_fd );
                  client_fd = -1;
                }
#endif
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
    err = rtos_create_thread( &g_cmd_thread, BEKEN_APPLICATION_PRIORITY,
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


