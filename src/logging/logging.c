
// Trying to narrow down Boozeman crash.
// Is the code with this define enabled crashing/freezing BK after few minutes for anybody?

#include "../new_common.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"

static char tmp[1024];
int loglevel = 4; // default to info
unsigned int logfeatures = (
    (1 << 0) |
    (1 << 1) |
    (1 << 2) |
    (1 << 3) |
    (1 << 4) |
    (1 << 5) |
    (1 << 6) |
    (1 << 7) |
    (1 << 8) |
    (0 << 9) | // disable LFS by default
    (1 << 10) |
    (1 << 11) |
    (1 << 12) |
    (1 << 13) |
    (1 << 14) |
    (1 << 15) |
    (1 << 16) |
    (1 << 17) |
    (1 << 18) |
    (1 << 19) |
    (1 << 20) |
    (1 << 21) |
    (1 << 22) |
    (1 << 23) |
    (1 << 24)
);
static int log_delay = 0;

// must match header definitions in logging.h
char *loglevelnames[] = {
    "NONE:",
    "Error:",
    "Warn:",
    "Info:",
    "Debug:",
    "ExtraDebug:",
    "All:"
};

// must match header definitions in logging.h
char *logfeaturenames[] = {
    "HTTP:",//            = 0,
    "MQTT:",//            = 1,
    "CFG:",//             = 2,
    "HTTP_CLIENT:",//     = 3,
    "OTA:",//             = 4,
    "PINS:",//            = 5,
    "MAIN:",//            = 6,
    "GEN:", //              = 7
    "API:", // = 8
    "LFS:", // = 9
    "CMD:", // = 10
    "NTP:", // = 11
	"TuyaMCU:",// = 12
	"I2C:",// = 13
	"EnergyMeter:",// = 14
	"EVENT:",// = 15
	"DGR:",// = 16
};


int direct_serial_log = DEFAULT_DIRECT_SERIAL_LOG;

#ifdef WINDOWS


void addLogAdv(int level, int feature, char *fmt, ...){
    va_list argList;
    char *t = tmp;
    if (!((1<<feature) & logfeatures)){
        return;
    }
    if (level > loglevel){
        return;
    }
    strcpy(t, loglevelnames[level]);
    t += strlen(t);
    if (feature < sizeof(logfeaturenames)/sizeof(*logfeaturenames)){
        strcpy(t, logfeaturenames[feature]);
        t += strlen(t);
    }
    va_start(argList, fmt);
    vsprintf(t, fmt, argList);
    va_end(argList);

    printf(tmp);
    printf("\r\n");
}

#else // from WINDOWS

static SemaphoreHandle_t g_mutex = 0;

#ifdef DEBUG_USE_SIMPLE_LOGGER

void addLogAdv(int level, int feature, char *fmt, ...){
    va_list argList;
    BaseType_t taken;
    char *t = tmp;
    if (!((1<<feature) & logfeatures)){
        return;
    }
    if (level > loglevel){
        return;
    }
	if(g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex( );
	}
	// TODO: semaphore

    taken = xSemaphoreTake( g_mutex, 100 );
    if (taken == pdTRUE) {

        strcpy(t, loglevelnames[level]);
        t += strlen(t);
        if (feature < sizeof(logfeaturenames)/sizeof(*logfeaturenames)){
            strcpy(t, logfeaturenames[feature]);
            t += strlen(t);
        }
		va_start(argList, fmt);
		vsprintf(t, fmt, argList);
		va_end(argList);

		bk_printf(tmp);
		bk_printf("\r\n");

        xSemaphoreGive( g_mutex );
        if (log_delay){
            rtos_delay_milliseconds(log_delay);
        }
    }
}
#else

static int http_getlog(http_request_t *request);
static int http_getlograw(http_request_t *request);

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

static void initLog( void ) {
    bk_printf("Entering init log...\r\n");
    logMemory.head = logMemory.tailserial = logMemory.tailtcp = logMemory.tailhttp = 0;
    logMemory.mutex = xSemaphoreCreateMutex( );
    initialised = 1;
    startSerialLog();
    startLogServer();
    HTTP_RegisterCallback( "/logs", HTTP_GET, http_getlog);
    HTTP_RegisterCallback( "/lograw", HTTP_GET, http_getlograw);
    bk_printf("Init log done!\r\n");

    CMD_RegisterCommand("loglevel", "", log_command, "set log level <0..6>", NULL);
    CMD_RegisterCommand("logfeature", "", log_command, "set log feature filter, <0..10> <0|1>", NULL);
    CMD_RegisterCommand("logtype", "", log_command, "logtype direct|all - direct logs only to serial immediately", NULL);
    CMD_RegisterCommand("logdelay", "", log_command, "logdelay 0..n - impose ms delay after every log", NULL);

    bk_printf("Commands registered!\r\n");
}


// adds a log to the log memory
// if head collides with either tail, move the tails on.
void addLogAdv(int level, int feature, char *fmt, ...){
    char *t = tmp;
    if (!((1<<feature) & logfeatures)){
        return;
    }
    if (level > loglevel){
        return;
    }

    va_list argList;
    // if not initialised, direct output
    if (!initialised) {
        initLog();
    }
    BaseType_t taken = xSemaphoreTake( logMemory.mutex, 100 );

    strcpy(t, loglevelnames[level]);
    t += strlen(t);
    if (feature < sizeof(logfeaturenames)/sizeof(*logfeaturenames)){
        strcpy(t, logfeaturenames[feature]);
        t += strlen(t);
    }

    va_start(argList, fmt);
    vsprintf(t, fmt, argList);
    va_end(argList);

    int len = strlen(tmp);
    tmp[len++] = '\r';
    tmp[len++] = '\n';
    tmp[len] = '\0';
#if PLATFORM_XR809
    printf(tmp);
#endif
#if PLATFORM_W800
  //  printf(tmp);
#endif
//#if PLATFORM_BL602
//    printf(tmp);
//#endif

    if (direct_serial_log){
        bk_printf(tmp);
        if (taken == pdTRUE){
            xSemaphoreGive( logMemory.mutex );
        }
        if (log_delay){
            rtos_delay_milliseconds(log_delay);
        }
        return;
    }

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
    if (log_delay){
        rtos_delay_milliseconds(log_delay);
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
    //printf("got tcp: %d:%s\r\n", len,buff);
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
                //addLog( "TCP Log Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                if ( kNoErr
                     != rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY,
							                     "Logging TCP Client",
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

    if ( err != kNoErr ) {
		//addLog( "Server listener thread exit with err: %d", err );
    }

    close( tcp_listen_fd );
    rtos_delete_thread( NULL );
}

#define TCPLOGBUFSIZE 128
static char tcplogbuf[TCPLOGBUFSIZE];
static void log_client_thread( beken_thread_arg_t arg )
{
    int fd = (int) arg;
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

	//addLog( "TCP client thread exit with err: %d", len );

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


static int http_getlograw(http_request_t *request){
    http_setup(request, httpMimeTypeHTML);
    int len = 0;

    // get log in small chunks, posting on http
    do {
        char buf[128];
        len = getHttp(buf, 127);
        buf[len] = '\0';
        if (len){
            poststr(request, buf);
        }
    } while (len);
    poststr(request, NULL);
    return 0;
}

static int http_getlog(http_request_t *request){
    http_setup(request, httpMimeTypeHTML);
    poststr(request,htmlHeader);
    poststr(request,htmlReturnToMenu);

    poststr(request, "<pre>");
    char *post = "</pre>";

    http_getlograw(request);

    poststr(request, post);
    poststr(request, htmlEnd);
    poststr(request, NULL);

    return 0;
}


int log_command(const void *context, const char *cmd, const char *args, int cmdFlags){
    int result = 0;
    if (!cmd) return -1;
    if (!args) return -1;
    do{
        if (!stricmp(cmd, "loglevel")){
            int res, level;
            res = sscanf(args, "%d", &level);
            if (res == 1){
                if ((level >= 0) && (level <= 9)){
                    loglevel = level;
                    result = 1;
                    ADDLOG_DEBUG(LOG_FEATURE_CMD, "loglevel set %d", level);
                } else {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "loglevel %d out of range", level);
                    result = -1;
                }
            } else {
                ADDLOG_ERROR(LOG_FEATURE_CMD, "loglevel %s invalid?", args);
                result = -1;
            }
            break;
        }
        if (!stricmp(cmd, "logfeature")){
            int res, feat;
            int val = 1;
            res = sscanf(args, "%d %d", &feat, &val);
            if (res >= 1){
                if ((feat >= 0) && (feat < LOG_FEATURE_MAX)){
                    logfeatures &= ~(1 << feat);
                    if (val){
                        logfeatures |= (1 << feat);
                    }
                    ADDLOG_DEBUG(LOG_FEATURE_CMD, "logfeature set 0x%08X", logfeatures);
                    result = 1;
                } else {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "logfeature %d out of range", feat);
                    result = -1;
                }
            } else {
                ADDLOG_ERROR(LOG_FEATURE_CMD, "logfeature %s invalid?", args);
                result = -1;
            }
            break;
        }
        if (!stricmp(cmd, "logtype")){
            if (!strcmp(args, "direct")){
                direct_serial_log = 1;
            } else {
                direct_serial_log = 0;
            }
            break;
        }
        if (!stricmp(cmd, "logdelay")){
            int res, delay;
            res = sscanf(args, "%d", &delay);
            if (res == 1){
                log_delay = delay;
            } else {
                log_delay = 0;
            }
            break;
        }

    } while (0);

    return result;
}


#endif // else from simple logger
#endif // else from windows
