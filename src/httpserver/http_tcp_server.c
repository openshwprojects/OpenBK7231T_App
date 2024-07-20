
#include "../new_common.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../logging/logging.h"
#include "new_http.h"

#define HTTP_SERVER_PORT            80
#define REPLY_BUFFER_SIZE			2048
#define INCOMING_BUFFER_SIZE		1024


// it was 0x800 - 2048 - until 23 10 2022
// The larger stack size for handling HTTP request is needed, for example, for commands
// See: https://github.com/openshwprojects/OpenBK7231T_App/issues/314
#if PLATFORM_BEKEN
#define HTTP_CLIENT_STACK_SIZE 8192
#elif PLATFORM_W600
#define HTTP_CLIENT_STACK_SIZE 8192
#else
#define HTTP_CLIENT_STACK_SIZE 8192
#endif

#define CREATE_THREAD_PER_EACH_HTTP_CLIENT

#ifdef CREATE_THREAD_PER_EACH_HTTP_CLIENT


#if PLATFORM_XR809

#define DISABLE_SEPARATE_THREAD_FOR_EACH_TCP_CLIENT 1

#endif

static void tcp_server_thread(beken_thread_arg_t arg);
static void tcp_client_thread(beken_thread_arg_t arg);


xTaskHandle g_http_thread = NULL;

void HTTPServer_Start()
{
	OSStatus err = kNoErr;

	err = rtos_create_thread(&g_http_thread, BEKEN_APPLICATION_PRIORITY,
		"TCP_server",
		(beken_thread_function_t)tcp_server_thread,
		0x800,
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"TCP_server\" thread failed with %i!\r\n", err);
	}
}


int sendfn(int fd, char* data, int len) {
	if (fd) {
		return send(fd, data, len, 0);
	}
	return -1;
}

static void tcp_client_thread(beken_thread_arg_t arg)
{
	OSStatus err = kNoErr;
	int fd = (int)arg;
	//fd_set readfds, errfds, readfds2;
	char* buf = NULL;
	char* reply = NULL;
	int replyBufferSize = REPLY_BUFFER_SIZE;
	//int res;
	//char reply[8192];

  //my_fd = fd;
	rtos_delay_milliseconds(20);

	reply = (char*)os_malloc(replyBufferSize);
	buf = (char*)os_malloc(INCOMING_BUFFER_SIZE);

	if (buf == 0 || reply == 0)
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
#if PLATFORM_BL602
	request.receivedLen = recv(fd, request.received, request.receivedLenmax, 0);
	request.received[request.receivedLen] = 0;
#else
	request.receivedLen = 0;
	while (1) {
		int remaining = request.receivedLenmax - request.receivedLen;
		int received = recv(fd, request.received + request.receivedLen, remaining, 0);
		if (received <= 0) {
			break;
		}
		request.receivedLen += received;
		if (received < remaining) {
			break;
		}
		// grow by 1024
		request.receivedLenmax += 1024;
		request.received = (char*)realloc(request.received, request.receivedLenmax+2);
		if (request.received == NULL) {
			// no memory
			return;
		}
	}
	request.received[request.receivedLen] = 0;
#endif

	request.reply = reply;
	request.replylen = 0;
	reply[0] = '\0';

	request.replymaxlen = replyBufferSize - 1;

	if (request.receivedLen <= 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client is disconnected, fd: %d", fd);
		goto exit;
	}

	//addLog( "TCP received string %s\n",buf );
	// returns length to be sent if any
	 // ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP will process packet of len %i\n", request.receivedLen );
	int lenret = HTTP_ProcessPacket(&request);
	if (lenret > 0) {
		ADDLOG_DEBUG(LOG_FEATURE_HTTP, "TCP sending reply len %i\n", lenret);
		send(fd, reply, lenret, 0);
	}

	//rtos_delay_milliseconds(10);

exit:
	if (err != kNoErr)
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP client thread exit with err: %d", err);

	if (buf != NULL)
		os_free(buf);
	if (reply != NULL)
		os_free(reply);

	lwip_close(fd);;
#if DISABLE_SEPARATE_THREAD_FOR_EACH_TCP_CLIENT

#else
	rtos_delete_thread(NULL);
#endif
}

/* TCP server listener thread */
static void tcp_server_thread(beken_thread_arg_t arg)
{
	(void)(arg);
	OSStatus err = kNoErr;
	struct sockaddr_in server_addr, client_addr;
	socklen_t sockaddr_t_size = sizeof(client_addr);
	char client_ip_str[16];
	int tcp_listen_fd = -1, client_fd = -1;
	fd_set readfds;

	tcp_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
	server_addr.sin_port = htons(HTTP_SERVER_PORT);/* Server listen on port: 20000 */
	err = bind(tcp_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	err = listen(tcp_listen_fd, 0);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(tcp_listen_fd, &readfds);

		select(tcp_listen_fd + 1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(tcp_listen_fd, &readfds))
		{
			client_fd = accept(tcp_listen_fd, (struct sockaddr*)&client_addr, &sockaddr_t_size);
			if (client_fd >= 0)
			{
#if PLATFORM_XR809
#if DISABLE_SEPARATE_THREAD_FOR_EACH_TCP_CLIENT

#else
				OS_Thread_t clientThreadUnused;
#endif
#endif
				os_strcpy(client_ip_str, inet_ntoa(client_addr.sin_addr));
				//  ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
#if DISABLE_SEPARATE_THREAD_FOR_EACH_TCP_CLIENT
				// Use main server thread (blocking all other clients)
				// right now, I am getting OS_ThreadCreate everytime on XR809 platform
				tcp_client_thread((beken_thread_arg_t)client_fd);
#else
				// delay each accept by 20ms
				// this allows previous to finish if
				// in a loop of sends from the browser, e.g. OTA
				// and we MUST get some IDLE thread time, else
				// thread resources are not deleted.
				rtos_delay_milliseconds(20);
				// Create separate thread for client
				if (kNoErr !=
#if PLATFORM_XR809
					OS_ThreadCreate(&clientThreadUnused,
						"HTTP Client",
						tcp_client_thread,
						client_fd,
						OS_THREAD_PRIO_CONSOLE,
						0x400)

#else
					rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
						"HTTP Client",
						(beken_thread_function_t)tcp_client_thread,
						HTTP_CLIENT_STACK_SIZE,
						(beken_thread_arg_t)client_fd)

#endif
					)
				{
					ADDLOG_DEBUG(LOG_FEATURE_HTTP, "TCP Client %s:%d thread creation failed! fd: %d", client_ip_str, client_addr.sin_port, client_fd);
					lwip_close(client_fd);
					client_fd = -1;
				}
#endif
			}
		}
	}

	if (err != kNoErr)
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Server listerner thread exit with err: %d", err);

	lwip_close(tcp_listen_fd);

	rtos_delete_thread(NULL);

}

#else


xTaskHandle g_http_thread = NULL;

int sendfn(int fd, char* data, int len) {
	if (fd) {
		return send(fd, data, len, 0);
	}
	return -1;
}

static void tcp_client_thread(int fd, char* buf, char* reply)
{
	//OSStatus err = kNoErr;

	http_request_t request;
	os_memset(&request, 0, sizeof(request));

	request.fd = fd;
	request.received = buf;
	request.receivedLenmax = INCOMING_BUFFER_SIZE - 1;
	request.responseCode = HTTP_RESPONSE_OK;
	request.receivedLen = recv(fd, request.received, request.receivedLenmax, 0);
	request.received[request.receivedLen] = 0;

	request.reply = reply;
	request.replylen = 0;
	reply[0] = '\0';

	request.replymaxlen = REPLY_BUFFER_SIZE - 1;

	if (request.receivedLen <= 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "TCP Client is disconnected, fd: %d", fd);
		return;
	}
	int lenret = HTTP_ProcessPacket(&request);
	if (lenret > 0) {
		send(fd, reply, lenret, 0);
	}
	rtos_delay_milliseconds(10);

}

/* TCP server listener thread */
static void tcp_server_thread(beken_thread_arg_t arg)
{
	(void)(arg);
	OSStatus err = kNoErr;
	struct sockaddr_in server_addr, client_addr;
	socklen_t sockaddr_t_size = sizeof(client_addr);
	char client_ip_str[16];
	int tcp_listen_fd = -1, client_fd = -1;
	fd_set readfds;
	char* buf = NULL;
	char* reply = NULL;

	tcp_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
	server_addr.sin_port = htons(HTTP_SERVER_PORT);/* Server listen on port: 20000 */
	err = bind(tcp_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	err = listen(tcp_listen_fd, 0);

	reply = (char*)os_malloc(REPLY_BUFFER_SIZE);
	buf = (char*)os_malloc(INCOMING_BUFFER_SIZE);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(tcp_listen_fd, &readfds);

		select(tcp_listen_fd + 1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(tcp_listen_fd, &readfds))
		{
			client_fd = accept(tcp_listen_fd, (struct sockaddr*)&client_addr, &sockaddr_t_size);
			if (client_fd >= 0)
			{
				os_strcpy(client_ip_str, inet_ntoa(client_addr.sin_addr));
				//  ADDLOG_DEBUG(LOG_FEATURE_HTTP,  "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );

				tcp_client_thread(client_fd, reply, buf);

				lwip_close(client_fd);
			}
		}
	}

	if (err != kNoErr)
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Server listerner thread exit with err: %d", err);

	lwip_close(tcp_listen_fd);

	rtos_delete_thread(NULL);

}

void HTTPServer_Start()
{
	OSStatus err = kNoErr;

	err = rtos_create_thread(&g_http_thread, BEKEN_APPLICATION_PRIORITY,
		"TCP_server",
		(beken_thread_function_t)tcp_server_thread,
		0x800,
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "create \"TCP_server\" thread failed with %i!\r\n", err);
	}
}


#endif

