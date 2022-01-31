/*
 * @Author: yj 
 * @email: shiliu.yang@tuya.com
 * @LastEditors: yj 
 * @file name: tuya_device.c
 * @Description: template demo for SDK WiFi & BLE for BK7231T
 * @Copyright: HANGZHOU TUYA INFORMATION TECHNOLOGY CO.,LTD
 * @Company: http://www.tuya.com
 * @Date: 2021-01-23 16:33:00
 * @LastEditTime: 2021-01-27 17:00:00
 */

#define _TUYA_DEVICE_GLOBAL

/* Includes ------------------------------------------------------------------*/
#include "uni_log.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_hal_system.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_com_defs.h"
#include "gw_intf.h"
#include "gpio_test.h"
#include "tuya_gpio.h"
#include "tuya_key.h"
#include "tuya_led.h"
#include "wlan_ui_pub.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

/* Private includes ----------------------------------------------------------*/
#include "tuya_device.h"
#include "new_http.h"
#include "new_pins.h"

#include "../../beken378/func/key/multi_button.h"
#include "../../beken378/app/config/param_config.h"
#include "lwip/apps/mqtt.h"

static int g_secondsElapsed = 0;



#define HTTP_SERVER_PORT            80 /*set up a tcp server,port at 20000*/

int my_fd = -1; 

int g_my_reconnect_mqtt_after_time = -1;


#define tcp_server_log(M, ...) os_printf("TCP", M, ##__VA_ARGS__)

int unw_recv(const int fd, void *buf, u32 nbytes)
{
    fd_set readfds, errfds;
    int ret = 0;   

    if( fd < 0 ) 
    {        
        return -1;//UNW_FAIL;
    } 

    FD_ZERO( &readfds );
    FD_ZERO( &errfds ); 
    FD_SET( fd, &readfds );
    FD_SET( fd, &errfds );

    ret = select( fd+1, &readfds, NULL, &errfds, NULL);
    os_printf("select ret:%d, %d, %d\r\n", ret, FD_ISSET( fd, &readfds ), FD_ISSET( fd, &errfds ));

    if(ret > 0 && FD_ISSET( fd, &readfds ))
        return recv(fd,buf,nbytes,0); 
    else
        return -1;//UNW_FAIL;
    
}

void tcp_client_thread( beken_thread_arg_t arg )
{
    OSStatus err = kNoErr;
    int fd = (int) arg;
    int len = 0;
    fd_set readfds, errfds, readfds2; 
    char *buf = NULL;
    char *reply = NULL;
	int replyBufferSize = 10000;
	int res;
	//char reply[8192];

    my_fd = fd;

    reply = (char*) os_malloc( replyBufferSize );
    buf = (char*) os_malloc( 1024 );
    ASSERT(buf);
    

    
    while ( 1 )
    {
           
        {
            len = recv( fd, buf, 1024, 0 );

            if ( len <= 0 )
            {
                os_printf( "TCP Client is disconnected, fd: %d", fd );
                goto exit;
            }
  
      PR_NOTICE( "TCP received string %s\n",buf );
		  
		HTTP_ProcessPacket(buf, reply, replyBufferSize);

		///	strcpy(buf,"[WB2S example TCP reply!]");
			len = strlen(reply);
      PR_NOTICE( "TCP sending reply len %i\n",len );
            len = send( fd, reply, len, 0 );

          rtos_delay_milliseconds(10);

          //  rtos_delay_milliseconds(100);
			// it gives me problems, ERR_CONNECTION_RESET
		  
			// do not wait for another request, just close
			// TODO: what if a retransmit or smth happens and takes more than 100ms?
			// let's just assume it will be ok?
			// Without break, Chrome has X loading ico nforever here
			//break;
		  // break; here breaks stuff
        }
    }


exit:
    if ( err != kNoErr ) 
		tcp_server_log( "TCP client thread exit with err: %d", err );
	
    if ( buf != NULL ) 
		os_free( buf );
    if ( reply != NULL ) 
		os_free( reply );
	
    close( fd );
    rtos_delete_thread( NULL );
}

volatile u8 test_flag = 0;
void close_tcp_client(void)
{
    os_printf("close_tcp_client:%d, %p\r\n", my_fd, rtos_get_current_thread());
    test_flag = 1;
    close( my_fd );
    my_fd = -1;
}

/* TCP server listener thread */
void tcp_server_thread( beken_thread_arg_t arg )
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
                tcp_server_log( "TCP Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
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
		tcp_server_log( "Server listerner thread exit with err: %d", err );
	
    close( tcp_listen_fd );
    rtos_delete_thread( NULL );
}

void connect_to_wifi(const char *oob_ssid,const char *connect_key)
{
#if 1
	network_InitTypeDef_adv_st	wNetConfigAdv;

	os_memset( &wNetConfigAdv, 0x0, sizeof(network_InitTypeDef_adv_st) );
	
	os_strcpy((char*)wNetConfigAdv.ap_info.ssid, oob_ssid);
	hwaddr_aton("48:ee:0c:48:93:12", wNetConfigAdv.ap_info.bssid);
	wNetConfigAdv.ap_info.security = SECURITY_TYPE_WPA2_MIXED;
	wNetConfigAdv.ap_info.channel = 5;
	
	os_strcpy((char*)wNetConfigAdv.key, connect_key);
	wNetConfigAdv.key_len = os_strlen(connect_key);
	wNetConfigAdv.dhcp_mode = DHCP_CLIENT;
	wNetConfigAdv.wifi_retry_interval = 100;

	bk_wlan_start_sta_adv(&wNetConfigAdv);
#else
    network_InitTypeDef_st network_cfg;
	
    os_memset(&network_cfg, 0x0, sizeof(network_InitTypeDef_st));

    os_strcpy((char *)network_cfg.wifi_ssid, oob_ssid);
    os_strcpy((char *)network_cfg.wifi_key, connect_key);

    network_cfg.wifi_mode = STATION;
    network_cfg.dhcp_mode = DHCP_CLIENT;
    network_cfg.wifi_retry_interval = 100;

    bk_printf("ssid:%s key:%s\r\n", network_cfg.wifi_ssid, network_cfg.wifi_key);
			
    bk_wlan_start(&network_cfg);
#endif
}



void demo_start_tcp()
{
    OSStatus err = kNoErr;

    err = rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY, 
									"TCP_server", 
									(beken_thread_function_t)tcp_server_thread,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
       os_printf("create \"TCP_server\" thread failed!\r\n");
    }
}


beken_timer_t led_timer;

#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(IPADDR_LOOPBACK))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
static mqtt_client_t* mqtt_client;

static struct mqtt_connect_client_info_t mqtt_client_info =
{
  "test",
  // do not fil those settings, they are overriden when read from memory
  "user", /* user */
  "pass", /* pass */
  100,  /* keep alive */
  NULL, /* will_topic */
  NULL, /* will_msg */
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};


/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
    PR_NOTICE("Publish result: %d\n", result);
  }
}
static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
void mqtt_example_init(void);
void example_publish(mqtt_client_t *client, int channel, int iVal)
{
	char pub_topic[32];
	char pub_payload[128];
//  const char *pub_payload= "{\"temperature\": \"45.5\"}";
  err_t err;
  int myValue;
  u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */
	const char *baseName;
  
  if(client==0)
	  return;
  if(mqtt_client_is_connected(client)==0) {
		 g_my_reconnect_mqtt_after_time = 5;
		return;
  }

  myValue = CHANNEL_Check(channel);
   sprintf(pub_payload,"%i",myValue);
   
    PR_NOTICE("calling pub: \n");

	baseName = CFG_GetShortDeviceName();

	//sprintf(pub_topic,"wb2s/%i/get",channel);
	sprintf(pub_topic,"%s/%i/get",baseName,channel);
  err = mqtt_publish(client, pub_topic, pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, 0);
  if(err != ERR_OK) {
    PR_NOTICE("Publish err: %d\n", err);
	 if(err == ERR_CONN) {
		 
		// g_my_reconnect_mqtt_after_time = 5;

       // mqtt_example_init();


	 }
  }
}

int g_incoming_channel_mqtt = 0;
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
	int iValue;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  //PR_NOTICE("MQTT client in mqtt_incoming_data_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_data_cb data is %s for ch %i\n",data,g_incoming_channel_mqtt);

  iValue = atoi(data);
  CHANNEL_Set(g_incoming_channel_mqtt,iValue,0);

 // PR_NOTICE(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id, (int)len, (int)flags));
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
	const char *p;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  //PR_NOTICE("MQTT client in mqtt_incoming_publish_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_publish_cb topic %s\n",topic);
// TODO: better 
//  g_incoming_channel_mqtt = topic[5] - '0';
  p = topic;
  while(*p != '/') {
	  if(*p == 0)
		  return;
		p++;
  }
  p++;
  g_incoming_channel_mqtt = *p - '0';

 // PR_NOTICE(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id, topic, (int)tot_len));
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  PR_NOTICE(("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err));
}
static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
     normal behaviour would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
  PR_NOTICE("Subscribe result: %i\n", result);
}
void example_do_connect(mqtt_client_t *client);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
	int i;
	char tmp[64];
	const char *baseName;
  err_t err = ERR_OK;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

//  PR_NOTICE(("MQTT client < removed name > connection cb: status %d\n",  (int)status));
 // PR_NOTICE(("MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status));

  if (status == MQTT_CONNECT_ACCEPTED) {
    PR_NOTICE("mqtt_connection_cb: Successfully connected\n");


  mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));

	 /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */

	baseName = CFG_GetShortDeviceName();
  // + is a MQTT wildcard
	sprintf(tmp,"%s/+/set",baseName);
    err = mqtt_sub_unsub(client,
          //  "wb2s/+/set", 1,
		  tmp, 1,
            mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
            1);
    if(err != ERR_OK) {
      PR_NOTICE("mqtt_subscribe return: %d\n", err);
    }


    //mqtt_sub_unsub(client,
    //        "topic_qos1", 1,
    //        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
    //        1);
    //mqtt_sub_unsub(client,
    //        "topic_qos0", 0,
    //        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
    //        1);
  } else {
    PR_NOTICE("mqtt_connection_cb: Disconnected, reason: %d\n", status);
    example_do_connect(client);

  }
}

void example_do_connect(mqtt_client_t *client)
{
  err_t err;
  const char *mqtt_userName, *mqtt_host, *mqtt_pass, *mqtt_clientID;
  int mqtt_port;


  mqtt_userName = CFG_GetMQTTUserName();
  mqtt_pass = CFG_GetMQTTPass();
  mqtt_clientID = CFG_GetMQTTBrokerName();
  mqtt_host = CFG_GetMQTTHost();

  // set pointer, there are no buffers to strcpy
  mqtt_client_info.client_id = mqtt_host;
  mqtt_client_info.client_pass = mqtt_pass;
  mqtt_client_info.client_user = mqtt_userName;

  // host name/ip
  	ipaddr_aton(mqtt_host,&mqtt_ip);
	mqtt_port = CFG_GetMQTTPort();

  /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting
     to establish a connection with the server.
     For now MQTT version 3.1.1 is always used */

  mqtt_client_connect(mqtt_client,
          &mqtt_ip, mqtt_port,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);


  /* For now just print the result code if something goes wrong */
  if(err != ERR_OK) {
    PR_NOTICE("mqtt_connect return %d\n", err);
  }
}


void mqtt_example_init(void)
{
  mqtt_client = mqtt_client_new();

	

	example_do_connect(mqtt_client);
}

static void app_my_channel_toggle_callback(int channel, int iVal)
{
    PR_NOTICE("Channel has changed! Publishing change %i with %i \n",channel,iVal);
	example_publish(mqtt_client,channel,iVal);
}
int loopsWithDisconnected = 0;
static void app_led_timer_handler(void *data)
{
	if(mqtt_client != 0 && mqtt_client_is_connected(mqtt_client) == 0) {
		PR_NOTICE("Timer discovetrs disconnected mqtt %i\n",loopsWithDisconnected);
		loopsWithDisconnected++;
		if(loopsWithDisconnected>10){ 
			example_do_connect(mqtt_client);
			loopsWithDisconnected = 0;
		}
	}

	g_secondsElapsed ++;

    PR_NOTICE("Timer is %i\n",g_secondsElapsed);
}

void app_on_generic_dbl_click(int btnIndex)
{
	if(g_secondsElapsed < 5) {
		CFG_SetOpenAccessPoint();
		CFG_SaveWiFi();
	}
}

/* Private functions ---------------------------------------------------------*/
/**
 * @Function: wifi_state_led_reminder
 * @Description: WiFi led指示灯，根据当前 WiFi 状态，做出不同提示 
 * @Input: cur_stat：当前 WiFi 状态 
 * @Output: none
 * @Return: none
 * @Others: 
 */
STATIC VOID wifi_state_led_reminder(IN CONST GW_WIFI_NW_STAT_E cur_stat)
{
   
}

/**
 * @Function: wifi_key_process
 * @Description: 按键回调函数
 * @Input: port：触发引脚,type：按键触发类型,cnt:触发次数
 * @Output: none
 * @Return: none
 * @Others: 长按触发配网模式
 */
STATIC VOID wifi_key_process(TY_GPIO_PORT_E port,PUSH_KEY_TYPE_E type,INT_T cnt)
{


    return;
}

/**
 * @Function: wifi_config_init
 * @Description: 初始化 WiFi 相关设备，按键，led指示灯
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: 
 */
STATIC VOID wifi_config_init(VOID)
{
 

    return;
}

/**
 * @Function: hw_report_all_dp_status
 * @Description: 上报所有 dp 点
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: 
 */
VOID hw_report_all_dp_status(VOID)
{
    //report all dp status
}

/**
 * @Function:gpio_test 
 * @Description: gpio测试
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: none
 */
BOOL_T gpio_test(IN CONST CHAR_T *in, OUT CHAR_T *out)
{
    return gpio_test_all(in, out);
}

/**
 * @Function: mf_user_callback
 * @Description: 授权回调函数
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: 清空flash中存储的数据
 */
VOID mf_user_callback(VOID)
{
    hw_reset_flash_data();
    return;
}

/**
 * @Function: prod_test
 * @Description: 扫描到产测热点，进入回调函数，主要是按键、指示灯、继电器功能测试
 * @Input: flag:授权标识；rssi:信号强度
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID prod_test(BOOL_T flag, SCHAR_T rssi)
{
    if (flag == FALSE || rssi < -60) 
    {
        PR_ERR("Prod test failed... flag:%d, rssi:%d", flag, rssi);
        return;
    }
    PR_NOTICE("flag:%d rssi:%d", flag, rssi);

}

/**
 * @Function: app_init
 * @Description: 设备初始化，设置工作模式
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: 无
 */
// NOTE: this is externally called from tuya_mainc
VOID app_init(VOID)
{

}

/**
 * @Function: pre_device_init
 * @Description: 设备信息(SDK信息、版本号、固件标识等)打印、重启原因和打印等级设置
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID pre_device_init(VOID)
{
    PR_DEBUG("%s",tuya_iot_get_sdk_info());
    PR_DEBUG("%s:%s",APP_BIN_NAME,DEV_SW_VERSION);
    PR_NOTICE("firmware compiled at %s %s", __DATE__, __TIME__);
    PR_NOTICE("Hello Tuya World!");
    PR_NOTICE("system reset reason:[%s]",tuya_hal_system_get_rst_info());
    /* 打印等级设置 */
    SetLogManageAttr(TY_LOG_LEVEL_DEBUG);
}

/**
 * @Function: status_changed_cb
 * @Description: network status changed callback
 * @Input: status: current status
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID status_changed_cb(IN CONST GW_STATUS_E status)
{

}

/**
 * @Function: upgrade_notify_cb
 * @Description: firmware download finish result callback
 * @Input: fw: firmware info
 * @Input: download_result: 0 means download succes. other means fail
 * @Input: pri_data: private data
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID upgrade_notify_cb(IN CONST FW_UG_S *fw, IN CONST INT_T download_result, IN PVOID_T pri_data)
{

}

/**
 * @Function: get_file_data_cb
 * @Description: firmware download content process callback
 * @Input: fw: firmware info
 * @Input: total_len: firmware total size
 * @Input: offset: offset of this download package
 * @Input: data && len: this download package
 * @Input: pri_data: private data
 * @Output: remain_len: the size left to process in next cb
 * @Return: OPRT_OK: success  Other: fail
 * @Others: none
 */
OPERATE_RET get_file_data_cb(IN CONST FW_UG_S *fw, IN CONST UINT_T total_len, IN CONST UINT_T offset, \
                                     IN CONST BYTE_T *data, IN CONST UINT_T len, OUT UINT_T *remain_len, IN PVOID_T pri_data)
{

    return OPRT_OK;
}

/**
 * @Function: gw_ug_inform_cb
 * @Description: gateway ota firmware available nofity callback
 * @Input: fw: firmware info
 * @Output: none
 * @Return: int:
 * @Others: 
 */
INT_T gw_ug_inform_cb(IN CONST FW_UG_S *fw)
{

    return 0;
}

/**
 * @Function: hw_reset_flash_data
 * @Description: hardware reset, erase user data from flash
 * @Input: none
 * @Output: none
 * @Return: none
 * @Others: 
 */
VOID hw_reset_flash_data(VOID)
{
    return;
}

/**
 * @Function: gw_reset_cb
 * @Description: gateway restart callback, app remove the device 
 * @Input: type:gateway reset type
 * @Output: none
 * @Return: none
 * @Others: reset factory clear flash data
 */
VOID gw_reset_cb(IN CONST GW_RESET_TYPE_E type)
{

}

/**
 * @Function: dev_obj_dp_cb
 * @Description: obj dp info cmd callback, tuya cloud dp(data point) received
 * @Input: dp:obj dp info
 * @Output: none
 * @Return: none
 * @Others: app send data by dpid  control device stat
 */
VOID dev_obj_dp_cb(IN CONST TY_RECV_OBJ_DP_S *dp)
{

}

/**
 * @Function: dev_raw_dp_cb
 * @Description: raw dp info cmd callback, tuya cloud dp(data point) received (hex data)
 * @Input: dp: raw dp info
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID dev_raw_dp_cb(IN CONST TY_RECV_RAW_DP_S *dp)
{

    return;
}

/**
 * @Function: dev_dp_query_cb
 * @Description: dp info query callback, cloud or app actively query device info
 * @Input: dp_qry: query info
 * @Output: none
 * @Return: none
 * @Others: none
 */
STATIC VOID dev_dp_query_cb(IN CONST TY_DP_QUERY_S *dp_qry) 
{

}

/**
 * @Function: wf_nw_status_cb
 * @Description: tuya-sdk network state check callback
 * @Input: stat: curr network status
 * @Output: none
 * @Return: none
 * @Others: none
 */
VOID wf_nw_status_cb(IN CONST GW_WIFI_NW_STAT_E stat)
{

}



static int setup_wifi_open_access_point(void)
{
    //#define APP_DRONE_DEF_SSID          "WIFI_UPV_000000"
    #define APP_DRONE_DEF_NET_IP        "192.168.4.1"
    #define APP_DRONE_DEF_NET_MASK      "255.255.255.0"
    #define APP_DRONE_DEF_NET_GW        "192.168.4.1"
    #define APP_DRONE_DEF_CHANNEL       1    
    
    general_param_t general;
    ap_param_t ap_info;
    network_InitTypeDef_st wNetConfig;
    int len;
    u8 *mac;
    
    os_memset(&general, 0, sizeof(general_param_t));
    os_memset(&ap_info, 0, sizeof(ap_param_t)); 
    os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));  
    
        general.role = 1,
        general.dhcp_enable = 1,

        os_strcpy((char *)wNetConfig.local_ip_addr, APP_DRONE_DEF_NET_IP);
        os_strcpy((char *)wNetConfig.net_mask, APP_DRONE_DEF_NET_MASK);
        os_strcpy((char *)wNetConfig.dns_server_ip_addr, APP_DRONE_DEF_NET_GW);
 

        PR_NOTICE("no flash configuration, use default\r\n");
        mac = (u8*)&ap_info.bssid.array;
		// this is MAC for Access Point, it's different than Client one
		// see wifi_get_mac_address source
        wifi_get_mac_address(mac, CONFIG_ROLE_AP);
        ap_info.chann = APP_DRONE_DEF_CHANNEL;
        ap_info.cipher_suite = 0;
        //os_memcpy(ap_info.ssid.array, APP_DRONE_DEF_SSID, os_strlen(APP_DRONE_DEF_SSID));
        os_memcpy(ap_info.ssid.array, CFG_GetDeviceName(), os_strlen(CFG_GetDeviceName()));
		
        ap_info.key_len = 0;
        os_memset(&ap_info.key, 0, 65);   
  

    bk_wlan_ap_set_default_channel(ap_info.chann);

    len = os_strlen(ap_info.ssid.array);

    os_strcpy((char *)wNetConfig.wifi_ssid, ap_info.ssid.array);
    os_strcpy((char *)wNetConfig.wifi_key, ap_info.key);
    
    wNetConfig.wifi_mode = SOFT_AP;
    wNetConfig.dhcp_mode = DHCP_SERVER;
    wNetConfig.wifi_retry_interval = 100;
    
	if(0) {
		PR_NOTICE("set ip info: %s,%s,%s\r\n",
				wNetConfig.local_ip_addr,
				wNetConfig.net_mask,
				wNetConfig.dns_server_ip_addr);
	}
    
	if(0) {
	  PR_NOTICE("ssid:%s  key:%s\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
	}
	bk_wlan_start(&wNetConfig);

    return 0;    
}


/**
 * @Function: device_init
 * @Description: device initialization process 
 * @Input: none
 * @Output: none
 * @Return: OPRT_OK: success  Other: fail
 * @Others: none
 */




OPERATE_RET device_init(VOID)
{
    OSStatus err;
	int bForceOpenAP = 0;
	const char *wifi_ssid, *wifi_pass;

    OPERATE_RET op_ret = OPRT_OK;

	CFG_CreateDeviceNameUnique();

	CFG_LoadWiFi();
	CFG_LoadMQTT();
	PIN_LoadFromFlash();

	wifi_ssid = CFG_GetWiFiSSID();
	wifi_pass = CFG_GetWiFiPass();

	PR_NOTICE("Using SSID [%s]\r\n",wifi_ssid);
	PR_NOTICE("Using Pass [%s]\r\n",wifi_pass);

#if 0
	// you can use this if you bricked your module by setting wrong access point data
	wifi_ssid = "qqqqqqqqqq";
	wifi_pass = "Fqqqqqqqqqqqqqqqqqqqqqqqqqqq"
#endif
#ifdef SPECIAL_UNBRICK_ALWAYS_OPEN_AP
	// you can use this if you bricked your module by setting wrong access point data
	bForceOpenAP = 1;
#endif
	if(*wifi_ssid == 0 || *wifi_pass == 0 || bForceOpenAP) {
		setup_wifi_open_access_point();
	} else {
		connect_to_wifi(wifi_ssid,wifi_pass);
	}

		// NOT WORKING, I done it other way, see ethernetif.c
	//net_dhcp_hostname_set(g_shortDeviceName);

	//demo_start_upd();
	demo_start_tcp();
	
	PIN_Init();


	PIN_SetGenericDoubleClickCallback(app_on_generic_dbl_click);
	CHANNEL_SetChangeCallback(app_my_channel_toggle_callback);

	mqtt_example_init();

    err = rtos_init_timer(&led_timer,
                          1 * 1000,
                          app_led_timer_handler,
                          (void *)0);
    ASSERT(kNoErr == err);

    err = rtos_start_timer(&led_timer);
    ASSERT(kNoErr == err);

    return op_ret;
}
