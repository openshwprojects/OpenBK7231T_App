#include "wm_include.h"
#include "libwebsockets.h"

#if DEMO_WEBSOCKETS

/*
 ****************************************************************************************
                                      Notice                                   
 Test steps(with LWS_USE_SSL disabled):
 1, Download websocket tool from https://download.csdn.net/download/zwl1584671413/10618985 ;
 2, Unzip it and excute "websocketd --port=8080 echo_client.bat" in your CLI window.
 3, Redefine LWS_SERVER_NAME.
 4, Run function lwsDemoTest() in your main entrance or demo console.

 Test steps(with LWS_USE_SSL enabled):
 1, Download websocket tool from https://download.csdn.net/download/zwl1584671413/10618985 ;
 2, Unzip it and excute 
    "websocketd --port=8080 --ssl --sslcert="certificate.pem" --sslkey="key.pem" echo_client.bat" 
    in your CLI window.
 3, Run function lwsDemoTest() in your main entrance or demo console.

 Need to see detail logs? Please mask NDEBUG and LWS_WITH_NO_LOGS from file lws_config.h.
 ****************************************************************************************
*/

#define   lws_debug printf
#define   SEND_MSG_MIN   128 // only {"msg_type":"keepalive"}
#define   DEMO_LWS_RECV_TASK_SIZE      2048
#define   DEMO_LWS_SEND_TASK_SIZE      256
#define   LWS_USE_SSL       (1 && TLS_CONFIG_HTTP_CLIENT_SECURE)
#define   LWS_SERVER_NAME   "192.168.1.100"
#define   LWS_SERVER_PORT   8080//4
#define   LWS_SERVER_PATH    "/"

enum lws_state{
    EXIT_SESSION,
    INIT_SESSION,
    SET_UP_SESSION,
    HANDLE_SESSION
};

typedef void (*Communication_ReceiveData)(const char *data, unsigned int dataLength);
static Communication_ReceiveData  receive_data_notify = NULL ;

static OS_STK DemoLwsRecvStk[DEMO_LWS_RECV_TASK_SIZE];
static OS_STK DemoLwsSendStk[DEMO_LWS_SEND_TASK_SIZE];
static struct lws* g_wsContext=NULL;
static u8 send_buffer[SEND_MSG_MIN +  LWS_SEND_BUFFER_PRE_PADDING + 4];
static u8 *data_2send=NULL;
static int data_length;
static u32 watchDog = 0;
enum lws_state now_state = INIT_SESSION;


static int demoLwsSend(const char *data, unsigned int length)
{
	if( g_wsContext==NULL){
		return 0;
	}

	data_length = length;
	if(length < SEND_MSG_MIN ){
        memset( send_buffer, 0, sizeof(send_buffer) );
		memcpy(send_buffer + LWS_SEND_BUFFER_PRE_PADDING , data, length );
	    data_2send = send_buffer + LWS_SEND_BUFFER_PRE_PADDING;
	}
    else{
	    lws_debug("length > SEND_MSG_MIN\r\n");
	}
	return length;
}

static int lwsCallbackNotify(struct lws *wsi,enum lws_callback_reasons reason,void *user, void *in, size_t len)
{
	switch (reason) 
    {
    	case LWS_CALLBACK_CLIENT_ESTABLISHED:
    		lws_debug("CLIENT_ESTABLISHED\n");
    		now_state = HANDLE_SESSION;
    		break;

    	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    		lws_debug("CLIENT_CONNECTION_ERROR\n");
    		now_state = EXIT_SESSION;
    		break;

    	case LWS_CALLBACK_CLOSED:
    		lws_debug("CLOSED\n");
    		now_state = EXIT_SESSION;
    		break;

    	case LWS_CALLBACK_CLIENT_RECEIVE:
    		((char *)in)[len] = '\0';
    		if( receive_data_notify!=NULL )
    			receive_data_notify( in, len );
    		break;

    	case LWS_CALLBACK_CLIENT_WRITEABLE:
            watchDog = 0;
    		if( data_2send !=NULL ){
                lws_debug("send %s\r\n", data_2send);
    			int n = lws_write( g_wsContext, data_2send, data_length, LWS_WRITE_TEXT );
                if( n <= 0 ) {
                    lws_debug("send %d\r\n", n);
                    now_state = EXIT_SESSION;
                }
                data_2send = NULL;
    		}
    		break;

    	default:
    		break;
	}

	return 0;
}

static int isNetworkOk(void)
{
	struct tls_ethif* etherIf= tls_netif_get_ethif();

	return etherIf->status;
}


static void dataReceived(const char *data, unsigned int dataLength)
{
	lws_debug("recv:%s\r\n\r\n", data );
}

static const struct lws_protocols protocols[] = {
	{
		"wss",
		lwsCallbackNotify,
		0,
		2048,
	},
	{ NULL, NULL, 0, 0 } /* end */
};


static void lwsRecvTask( void* lparam )
{
    struct lws_client_connect_info connInfo;
    struct lws_context_creation_info info;    
    struct lws_context *lwscontext = NULL;
    u32 oldTick = 0, nowTick = 0;

	while (1) 
    {
        //lws_debug("now_state: %d\n", now_state);
        switch(now_state)
        {
            case INIT_SESSION:
            {
                if(1 == isNetworkOk())
                {
                    if( lwscontext == NULL )
                    {
                        memset(&info, 0, sizeof(info) );
                        info.port = CONTEXT_PORT_NO_LISTEN;
#if LWS_USE_SSL
                        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif
                        info.protocols = protocols;
                        info.options = 0;
                        info.max_http_header_data=512;
                        info.max_http_header_pool=4;
                        
                        lwscontext = lws_create_context(&info);
                        memset(&connInfo, 0 , sizeof(connInfo) );
                        connInfo.context = lwscontext;
                        connInfo.address = LWS_SERVER_NAME;
                        connInfo.port = LWS_SERVER_PORT;
                        connInfo.ssl_connection=LWS_USE_SSL;
                        connInfo.path = LWS_SERVER_PATH;
                        connInfo.host = LWS_SERVER_NAME;
                        connInfo.userdata=NULL;
                        connInfo.protocol = protocols[0].name;
                        connInfo.ietf_version_or_minus_one=13;
                        connInfo.origin = LWS_SERVER_NAME;
                        now_state = SET_UP_SESSION;
                    }
                    else
                    {
                        lws_debug("illegal context\r\n");
                        now_state = EXIT_SESSION;
                    }
                }
                else
                {
                    lws_debug("wifi offline\r\n");
                    tls_os_time_delay(HZ*5);
                }
            }
            break;
            
            case SET_UP_SESSION:
            {
                g_wsContext=lws_client_connect_via_info( &connInfo );
                if(g_wsContext==NULL ){
                    lws_debug("lws_client_connect_via_info fail\r\n\r\n");
                    now_state = EXIT_SESSION;
                    tls_os_time_delay(HZ*5);
                }
                else{
                    now_state = HANDLE_SESSION;
                }
            }
            break;
            
            case HANDLE_SESSION:
            {
                nowTick = tls_os_get_time();
                if(nowTick - oldTick > HZ) {
                    if(isNetworkOk()) {
                        lws_callback_on_writable(g_wsContext);
                        (watchDog > 5)?(now_state = EXIT_SESSION):(watchDog ++);
                        oldTick = nowTick;
                    }
                    else {
                        now_state = EXIT_SESSION;
                    }
                }
                lws_service(lwscontext, 250);
            }
            break;
            
            case EXIT_SESSION:
            {
                lws_debug("EXIT_SESSION\n");
                if( lwscontext != NULL ) {
                    lws_context_destroy(lwscontext);
                }
                lwscontext = NULL;
                g_wsContext = NULL;
                watchDog = 0;
                tls_os_time_delay(HZ*5);
                now_state = INIT_SESSION;
            }
            break;
            
            default:
            break;          
        }    
	}
}

static void lwsSendTask( void* lparam )
{
    char dbgStr[128] = {0};
    u16 len = 0,  cnt = 0;
    
    while(1)
    {
        sprintf(dbgStr, "{\"msg_type\":\"keepalive\"} %d", cnt++);
        len = strlen(dbgStr);
        dbgStr[len] = '\0';
		demoLwsSend( dbgStr, len );
		tls_os_time_delay(2*HZ);
	}
}

static int demoWebsocketTaskCreate( void *recvFunc)
{
    receive_data_notify = (Communication_ReceiveData)recvFunc;

    tls_os_task_create(NULL, NULL,
            lwsRecvTask,
                    (void *)NULL,
                    (void *)DemoLwsRecvStk, 
                    DEMO_LWS_RECV_TASK_SIZE * sizeof(u32),
                    DEMO_SSL_SERVER_TASK_PRIO,
                    0);

    tls_os_task_create(NULL, NULL,
            lwsSendTask,
                    (void *)NULL,
                    (void *)DemoLwsSendStk, 
                    DEMO_LWS_SEND_TASK_SIZE * sizeof(u32),
                    DEMO_SSL_SERVER_TASK_PRIO,
                    0);
    return 0;
}

static void setAutoConnectMode(void)
{
    u8 auto_reconnect = 0xff;

    tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
    if(auto_reconnect != WIFI_AUTO_CNT_ON)
    {
    	auto_reconnect = WIFI_AUTO_CNT_ON;
    	tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
    }
}

int lwsDemoTest(void)
{
    setAutoConnectMode();
	demoWebsocketTaskCreate(dataReceived);
    return 0;
}

#endif
