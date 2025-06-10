#include "../new_common.h"
#include "../obk_config.h"

#if NEW_TCP_SERVER

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../logging/logging.h"
#include "new_http.h"

#ifndef MAX_SOCKETS_TCP
#define MAX_SOCKETS_TCP MEMP_NUM_TCP_PCB
#endif
#define MAX_SOCKETS_TCP 4

void HTTPServer_Start();

#define HTTP_SERVER_PORT		80
#define REPLY_BUFFER_SIZE		2048
//#define INCOMING_BUFFER_SIZE		2048
//#define MAX_INCOMING_BUFFER_SIZE	4096
#define INVALID_SOCK			-1
#define HTTP_CLIENT_STACK_SIZE		8192

// START change values just for testing
#define INCOMING_BUFFER_SIZE		512
#define MAX_INCOMING_BUFFER_SIZE	2048
// END change values just for testing



typedef struct
{
	int fd;
	xTaskHandle thread;
	bool isCompleted;
} tcp_thread_t;

static xTaskHandle g_http_thread = NULL;
static int listen_sock = INVALID_SOCK;
static tcp_thread_t sock[MAX_SOCKETS_TCP - 1] =
{
	[0 ... MAX_SOCKETS_TCP - 2] = { -1, NULL, false },
};

static void tcp_client_thread(tcp_thread_t* arg)
{
	int fd = arg->fd;
	char* buf = NULL;
	char* reply = NULL;
	int replyBufferSize = REPLY_BUFFER_SIZE;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	reply = (char*)os_malloc(replyBufferSize);
	buf = (char*)os_malloc(INCOMING_BUFFER_SIZE);
	GLOBAL_INT_RESTORE();

	if(buf == 0 || reply == 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client failed to malloc buffer");
		goto exit;
	}
	http_request_t request;
	os_memset(&request, 0, sizeof(request));

	request.fd = fd;
	request.received = buf;
	request.receivedLenmax = INCOMING_BUFFER_SIZE - 2;
	request.responseCode = HTTP_RESPONSE_OK;
	request.receivedLen = 0;
	while(1)
	{
		buf=request.received;		// even if we resized the buffer, make sure "buf" points to the actual one, buf could else be the old pointer ...

		int remaining = request.receivedLenmax - request.receivedLen;
		int received = recv(fd, request.received + request.receivedLen, remaining, 0);
		if(received <= 0)
		{
			break;
		}
		request.receivedLen += received;
		if(received < remaining)
		{
			break;
		}
		//     (our buffer already has maximum size) 		  ||	(growing buffer would exeed the maximum size)  
		if( (request.receivedLenmax >= MAX_INCOMING_BUFFER_SIZE) || (request.receivedLenmax + INCOMING_BUFFER_SIZE > MAX_INCOMING_BUFFER_SIZE))
		{
			// just some debug output for testing
			ADDLOG_DEBUG(LOG_FEATURE_HTTP, "NEW_TCP_server: Won't grow buffer any more, reached limit (request.receivedLenmax=%d; after resize would be=%d but MAX_INCOMING_BUFFER_SIZE=%d ).\n", 
				request.receivedLenmax, request.receivedLenmax + INCOMING_BUFFER_SIZE, MAX_INCOMING_BUFFER_SIZE);  
			break;
		}
		// grow by INCOMING_BUFFER_SIZE - we made sure, this is inside our limits ( <= MAX_INCOMING_BUFFER_SIZE) before 

		int oldsize = request.receivedLenmax;
		request.receivedLenmax += INCOMING_BUFFER_SIZE;
		ADDLOG_DEBUG(LOG_FEATURE_HTTP, "NEW_TCP_server: Growing buffer from %d to %d bytes ... \n", oldsize, request.receivedLenmax);
		GLOBAL_INT_DISABLE();
		request.received = (char*)realloc(request.received, request.receivedLenmax + 2);
		GLOBAL_INT_RESTORE();
		if(request.received == NULL)
		{
			// no memory
			request.received = buf;		// restore the saved pointer, so request.received points to the data before trying realloc
			goto exit;
/*
			// as an alternative, we could also be more tolerant and "proceed" with the data copied up to now 
			break;				// ... process the data in HTTP server 
*/
		}
		ADDLOG_EXTRADEBUG(LOG_FEATURE_HTTP, " ... sucessfully grown buffer!\n");
	}

	// moved up: if request.receivedLen is really < 0, setting "request.received[request.receivedLen] = 0;" will be illegal  
	if(request.receivedLen <= 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client is disconnected, fd: %d", fd);
		goto exit;
	}

	request.received[request.receivedLen] = 0;

	request.reply = reply;
	request.replylen = 0;
	reply[0] = '\0';

	request.replymaxlen = replyBufferSize - 1;

	//addLog( "TCP received string %s\n",buf );
	// returns length to be sent if any
	 // ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP will process packet of len %i\n", request.receivedLen );
	int lenret = HTTP_ProcessPacket(&request);
	if(lenret > 0)
	{
		ADDLOG_DEBUG(LOG_FEATURE_HTTP, "TCP sending reply len %i\n", lenret);
		send(fd, reply, lenret, 0);
	}
exit:
	GLOBAL_INT_DISABLE();
	if(buf != NULL)
		os_free(buf);
	if(reply != NULL)
		os_free(reply);
	GLOBAL_INT_RESTORE();

	lwip_close(fd);
	arg->isCompleted = true;
	rtos_suspend_thread(NULL);
}

static inline char* get_clientaddr(struct sockaddr_storage* source_addr)
{
	static char address_str[128];
	char* res = NULL;
	// Convert ip address to string
	if(source_addr->ss_family == PF_INET)
	{
		res = inet_ntoa_r(((struct sockaddr_in*)source_addr)->sin_addr, address_str, sizeof(address_str) - 1);
	}
#if LWIP_IPV6
	else if(source_addr->ss_family == PF_INET6)
	{
		res = inet6_ntoa_r(((struct sockaddr_in6*)source_addr)->sin6_addr, address_str, sizeof(address_str) - 1);
	}
#endif
	if(!res)
	{
		address_str[0] = '\0';
	}
	return address_str;
}

void restart_tcp_server(void* arg)
{
	HTTPServer_Start();
	vTaskDelete(NULL);
}

static void tcp_server_thread(beken_thread_arg_t arg)
{
	OSStatus err = kNoErr;
	int reuse = 1;
	const size_t max_socks = MAX_SOCKETS_TCP - 1;

	struct sockaddr_in server_addr =
	{
		.sin_family = AF_INET,
		.sin_addr =
		{
			.s_addr = INADDR_ANY,
		},
		.sin_port = htons(HTTP_SERVER_PORT),
	};

	if(listen_sock != INVALID_SOCK) close(listen_sock);

	for(int i = 0; i < max_socks; ++i)
	{
		if(sock[i].fd != INVALID_SOCK)
		{
			close(sock[i].fd);
		}
		if(sock[i].thread != NULL)
		{
			rtos_delete_thread(&sock[i].thread);
		}
		sock[i].fd = INVALID_SOCK;
		sock[i].thread = NULL;
		sock[i].isCompleted = false;
	}

	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listen_sock < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Unable to create socket");
		goto error;
	}

	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	err = bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Socket unable to bind");
		goto error;
	}
	ADDLOG_EXTRADEBUG(LOG_FEATURE_HTTP, "Socket bound on 0.0.0.0:%i", HTTP_SERVER_PORT);

	err = listen(listen_sock, 0);
	if(err != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Error occurred during listen");
		goto error;
	}
	ADDLOG_INFO(LOG_FEATURE_HTTP, "TCP server listening");
	while(true)
	{
		struct sockaddr_storage source_addr;
		socklen_t addr_len = sizeof(source_addr);

		int new_idx = 0;
		for(int i = 0; i < max_socks; ++i)
		{
			if(sock[i].isCompleted)
			{
				if(sock[i].thread != NULL)
				{
					rtos_delete_thread(&sock[i].thread);
					sock[i].thread = NULL;
				}
				sock[i].isCompleted = false;
				sock[i].fd = INVALID_SOCK;
			}
		}
		for(new_idx = 0; new_idx < max_socks; ++new_idx)
		{
			if(sock[new_idx].fd == INVALID_SOCK)
			{
				break;
			}
		}
		if(new_idx < max_socks)
		{
			sock[new_idx].fd = accept(listen_sock, (struct sockaddr*)&source_addr, &addr_len);

#if LWIP_SO_RCVTIMEO && !PLATFORM_ECR6600 && !PLATFORM_TR6260
			struct timeval tv;
			tv.tv_sec = 30;
			tv.tv_usec = 0;
			setsockopt(sock[new_idx].fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif

			if(sock[new_idx].fd < 0)
			{
				switch(errno)
				{
					//case EAGAIN:
					case EWOULDBLOCK: break;
					case EBADF: goto error;
					default:
						ADDLOG_ERROR(LOG_FEATURE_HTTP, "[sock=%d]: Error when accepting connection, err: %i", sock[new_idx].fd, errno);
						break;
				}
			}
			else
			{
				//ADDLOG_EXTRADEBUG(LOG_FEATURE_HTTP, "[sock=%d]: Connection accepted from IP:%s", sock[new_idx].fd, get_clientaddr(&source_addr));

				rtos_delay_milliseconds(20);

				int stacksize = HTTP_CLIENT_STACK_SIZE;
				// reduce stacksize if memory is low - try at least 1024 - e.g. W800 might crash else 
				while (  (xPortGetFreeHeapSize() < 2 * HTTP_CLIENT_STACK_SIZE) && (xPortGetFreeHeapSize() < 3 * stacksize) && stacksize > 2048) stacksize/=2;
				if (stacksize < HTTP_CLIENT_STACK_SIZE) ADDLOG_DEBUG(LOG_FEATURE_HTTP, "New TCP Client thread with reduced stackzise %d (FreeHeap=%d)\n",stacksize,xPortGetFreeHeapSize());

				if(kNoErr != rtos_create_thread(&sock[new_idx].thread,
					BEKEN_APPLICATION_PRIORITY,
					"HTTP Client",
					(beken_thread_function_t)tcp_client_thread,
					stacksize,
					(beken_thread_arg_t)&sock[new_idx]))
				{
					ADDLOG_ERROR(LOG_FEATURE_HTTP, "[sock=%d]: TCP Client thread creation failed!", sock[new_idx].fd);
					lwip_close(sock[new_idx].fd);
					sock[new_idx].fd = INVALID_SOCK;
					goto error;
				}
			}
		}
		rtos_delay_milliseconds(10);
	}

error:
	ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Server Error");
	if(listen_sock != INVALID_SOCK)
	{
		close(listen_sock);
	}

	for(int i = 0; i < max_socks; ++i)
	{
		if(sock[i].thread != NULL)
		{
			rtos_delete_thread(&sock[i].thread);
			sock[i].thread = NULL;
		}
		if(sock[i].fd != INVALID_SOCK)
		{
			close(sock[i].fd);
			sock[i].fd = INVALID_SOCK;
		}
	}
	xTaskCreate(
		(TaskFunction_t)restart_tcp_server,
		"TCP Restart",
		2048 / sizeof(StackType_t),
		NULL,
		BEKEN_APPLICATION_PRIORITY,
		NULL);
}

void HTTPServer_Start()
{
	OSStatus err = kNoErr;

	if(g_http_thread != NULL)
	{
		rtos_delete_thread(&g_http_thread);
	}

	err = rtos_create_thread(&g_http_thread, BEKEN_APPLICATION_PRIORITY,
		"TCP_server",
		(beken_thread_function_t)tcp_server_thread,
		0x800,
		(beken_thread_arg_t)0);
	if(err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"TCP_server\" thread failed with %i!\r\n", err);
	}
}

#endif
