
#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "str_pub.h"

static int http_getlog(const char *payload, char *outbuf, int outBufSize);

static void log_server_thread( beken_thread_arg_t arg );
static void log_client_thread( beken_thread_arg_t arg );
static void log_serial_thread( beken_thread_arg_t arg );


static void startSerialLog();
static void startLogServer();

#define LOGSIZE 4096
#define LOGPORT 9000

int logTcpPort = LOGPORT;

static struct tag_logMemory {
    char log[LOGSIZE];
    int head;
    int tailserial;
    int tailtcp;
    int tailhttp;
    SemaphoreHandle_t mutex;
} logMemory;

static int initialised = 0; 
static char tmp[1024];

static void initLog() {
    bk_printf("init log\r\n");
    logMemory.head = logMemory.tailserial = logMemory.tailtcp = logMemory.tailhttp = 0; 
    logMemory.mutex = xSemaphoreCreateMutex( );
    initialised = 1;
    startSerialLog();
    startLogServer();
    HTTP_RegisterCallback( "/logs", http_getlog);
}

// adds a log to the log memory
// if head collides with either tail, move the tails on.
void addLog(char *fmt, ...){
    va_list argList;
    // if not initialised, direct output
    if (!initialised) {
        initLog();
    }
    BaseType_t taken = xSemaphoreTake( logMemory.mutex, 100 );

    va_start(argList, fmt);
    vsprintf(tmp, fmt, argList);
    va_end(argList);

//#define DIRECTLOG
#ifdef DIRECTLOG
    bk_printf(tmp);
    if (taken == pdTRUE){
        xSemaphoreGive( logMemory.mutex );
    }
    return;
#endif    

    int len = strlen(tmp);
    tmp[len++] = '\r';
    tmp[len++] = '\n';

    //bk_printf("addlog %d.%d.%d %d:%s\n", logMemory.head, logMemory.tailserial, logMemory.tailtcp, len,tmp);

    for (int i = 0; i < len; i++){
        logMemory.log[logMemory.head] = tmp[i];
        logMemory.head = (logMemory.head + 1) % LOGSIZE;
        if (logMemory.tailserial == logMemory.head){
            logMemory.tailserial = (logMemory.tailserial + 1) % LOGSIZE;
        }
        if (logMemory.tailtcp == logMemory.head){
            logMemory.tailtcp = (logMemory.tailtcp + 1) % LOGSIZE;
        }
        if (logMemory.tailhttp == logMemory.head){
            logMemory.tailhttp = (logMemory.tailhttp + 1) % LOGSIZE;
        }
    }

    if (taken == pdTRUE){
        xSemaphoreGive( logMemory.mutex );
    }
}

static int getData(char *buff, int buffsize, int *tail) {
    if (!initialised) return 0;
    BaseType_t taken = xSemaphoreTake( logMemory.mutex, 100 );

    int count = 0;
    char *p = buff;
    while (buffsize > 1){
        if (*tail == logMemory.head){
            break;
        }
        *p = logMemory.log[*tail];
        p++;
        (*tail) = ((*tail) + 1) % LOGSIZE;
        buffsize--;
        count++;
    }
    *p = 0;

    if (taken == pdTRUE){
        xSemaphoreGive( logMemory.mutex );
    }
    return count;
}

static int getSerial(char *buff, int buffsize){
    int len = getData(buff, buffsize, &logMemory.tailserial);
    //bk_printf("got serial: %d:%s\r\n", len,buff);
    return len;
}

static int getTcp(char *buff, int buffsize){
    int len = getData(buff, buffsize, &logMemory.tailtcp);
    //bk_printf("got tcp: %d:%s\r\n", len,buff);
    return len;
}

static int getHttp(char *buff, int buffsize){
    int len = getData(buff, buffsize, &logMemory.tailhttp);
    //bk_printf("got tcp: %d:%s\r\n", len,buff);
    return len;
}

void startLogServer(){
    OSStatus err = kNoErr;

    err = rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
									"TCP_server", 
									(beken_thread_function_t)log_server_thread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
       bk_printf("create \"TCP_server\" thread failed!\r\n");
    }
}

void startSerialLog(){
    OSStatus err = kNoErr;
    err = rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
									"log_serial", 
									(beken_thread_function_t)log_serial_thread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
       bk_printf("create \"log_serial\" thread failed!\r\n");
    }
}


/* TCP server listener thread */
void log_server_thread( beken_thread_arg_t arg )
{
    //(void)( arg );
    OSStatus err = kNoErr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sockaddr_t_size = sizeof(client_addr);
    char client_ip_str[16];
    int tcp_listen_fd = -1, client_fd = -1;
    fd_set readfds;

    tcp_listen_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    int tcp_select_fd = tcp_listen_fd + 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
    server_addr.sin_port = htons( logTcpPort );/* Server listen on port: 20000 */
    err = bind( tcp_listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr) );
    
    err = listen( tcp_listen_fd, 0 );
    
    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( tcp_listen_fd, &readfds );

        select( tcp_select_fd, &readfds, NULL, NULL, NULL);

        if ( FD_ISSET( tcp_listen_fd, &readfds ) )
        {
            client_fd = accept( tcp_listen_fd, (struct sockaddr *) &client_addr, &sockaddr_t_size );
            if ( client_fd >= 0 )
            {
                os_strcpy( client_ip_str, inet_ntoa( client_addr.sin_addr ) );
                addLog( "TCP Log Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                if ( kNoErr
                     != rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
							                     "TCP Clients",
                                                 (beken_thread_function_t)log_client_thread,
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
		addLog( "Server listerner thread exit with err: %d", err );
	
    close( tcp_listen_fd );
    rtos_delete_thread( NULL );
}

#define TCPLOGBUFSIZE 128
static char tcplogbuf[TCPLOGBUFSIZE];
static void log_client_thread( beken_thread_arg_t arg )
{
    int fd = (int) arg;
    int len = 0;
    while ( 1 ){
        int count = getTcp(tcplogbuf, TCPLOGBUFSIZE);
        if (count){
            int len = send( fd, tcplogbuf, count, 0 );
            // if some error, close socket
            if (len != count){
                break;
            }
        }
        rtos_delay_milliseconds(10);
    }
	
	addLog( "TCP client thread exit with err: %d", len );
	
    close( fd );
    rtos_delete_thread( NULL );
}


#define SERIALLOGBUFSIZE 128
static char seriallogbuf[SERIALLOGBUFSIZE];
static void log_serial_thread( beken_thread_arg_t arg )
{
    while ( 1 ){
        int count = getSerial(seriallogbuf, SERIALLOGBUFSIZE);
        if (count){
            bk_printf(seriallogbuf);
        }
        rtos_delay_milliseconds(10);
    }
}



static int http_getlog(const char *payload, char *outbuf, int outBufSize){
    http_setup(outbuf, httpMimeTypeHTML);
    strcat_safe(outbuf,htmlHeader,outBufSize);
    strcat_safe(outbuf,htmlReturnToMenu,outBufSize);

    strcat_safe(outbuf, "<pre>",outBufSize);
    char *post = "</pre>";

    char *b = outbuf;

    int len = strlen(b);
    b += len;
    outBufSize -= len;

    int trailsize = strlen(htmlEnd) + strlen(post);
    len = getHttp(b, outBufSize - trailsize - 1);
    b += len;
    outBufSize -= len;

    strcat_safe(outbuf, post, outBufSize);
    strcat_safe(outbuf,htmlEnd,outBufSize);

    return strlen(outbuf);
}



