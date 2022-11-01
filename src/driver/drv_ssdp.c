////////////////////////////////////////////////////////
// 
// OpenBeken SSDP support
// listens on multicast 239.255.255.250:1900
// responds to ANY M-SEARCH with a valid response.
// add an HTTP service routine for /ssdp.xml
// which returns the SSDP Description
//
// Result - start this driver and your device is visible in Windows 
// 'Other Devices' in network.
// double click a device to go to the device webpage
//
// copyright Simon Hailes & OpenBeken
/////////////////////////////////////////////////////////

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../devicegroups/deviceGroups_public.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../obk_config.h"
#include "../httpserver/new_http.h"
#include "common_math.h"

extern int DRV_SSDP_Active;

static const char* ssdp_group = "239.255.255.250";
static int ssdp_port = 1900;

static int g_ssdp_socket_receive = -1;

static char g_ssdp_uuid[40] = "e427ce1a-3e80-43d0-ad6f-89ec42e46363";

static void DRV_SSDP_Send_Notify();
static int ssdp_timercount = 0;

const char *HAL_GetMyIPString();

// allocated at first use, freed if stopped
static char *advert_message = NULL;
static char *udp_msgbuf = NULL;
#define UDP_MSGBUF_LEN 500
static char *notify_message = NULL;
static char *http_message = NULL;
int http_message_len = 0;


///////////////////////////////
// private functions, only used by the public functions...

static void DRV_SSDP_CreateSocket_Receive() {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int broadcast = 1;
	int iResult = 1;

    // create what looks like an ordinary UDP socket
    //
    g_ssdp_socket_receive = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_ssdp_socket_receive < 0) {
		g_ssdp_socket_receive = -1;
		addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_CreateSocket_Receive: failed to do socket\n");
        return ;
    }

	if(broadcast)
	{

		iResult = setsockopt(g_ssdp_socket_receive, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_CreateSocket_Receive: failed to do setsockopt SO_BROADCAST\n");
			close(g_ssdp_socket_receive);
			g_ssdp_socket_receive = -1;
			return ;
		}
	}
	else{
		// allow multiple sockets to use the same PORT number
		//
		if (
			setsockopt(
				g_ssdp_socket_receive, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
			) < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_CreateSocket_Receive: failed to do setsockopt SO_REUSEADDR\n");
			close(g_ssdp_socket_receive);
			g_ssdp_socket_receive = -1;
		  return ;
		}
	}

    // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(ssdp_port);

    // bind to receive address
    //
    if (bind(g_ssdp_socket_receive, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_DGR_CreateSocket_Receive: failed to do bind\n");
		close(g_ssdp_socket_receive);
		g_ssdp_socket_receive = -1;
        return ;
    }

/*	if(broadcast)
	{

	}
	else*/
	{

	  // use setsockopt() to request that the kernel join a multicast group
		//
		mreq.imr_multiaddr.s_addr = inet_addr(ssdp_group);
		//mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	    mreq.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(MY_CAPTURE_IP);
    	///mreq.imr_interface.s_addr = inet_addr("192.168.0.122");
	    iResult = setsockopt(
			g_ssdp_socket_receive, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
		);
		if (
			iResult < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_CreateSocket_Receive: failed to do setsockopt IP_ADD_MEMBERSHIP %i\n",iResult);
			close(g_ssdp_socket_receive);
			g_ssdp_socket_receive = -1;
			return ;
		}
	}

	lwip_fcntl(g_ssdp_socket_receive, F_SETFL,O_NONBLOCK);

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_CreateSocket_Receive: Socket created, waiting for packets\n");
}



static const char message_template[] = 
"HTTP/1.1 200 OK\r\n" \
"CACHE-CONTROL: max-age=1800\r\n" \
"EXT:\r\n" \
"LOCATION: http://%s:80/ssdp.xml\r\n" \
"OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n" \
"01-NLS: %s\r\n" \
"ST: upnp:rootdevice\r\n" \
"SERVER: OpenBk\r\n" \
"USN: %s::upnp:rootdevice\r\n" \
"BOOTID.UPNP.ORG: 0\r\n" \
"CONFIGID.UPNP.ORG: 1\r\n" \
"\r\n\r\n" \
/* like e427ce1a-3e80-43d0-ad6f-89ec42e46363*/ \
/*"last-seen: 1477147409.432466\r\n"*/ \
/*"DATE: Sat, 22 Oct 2016 14:44:26 GMT\r\n"*/ \
;


static void DRV_SSDP_Send_Advert_To(struct sockaddr_in *addr) {
	int nbytes;
    const char *myip = HAL_GetMyIPString();

	if (g_ssdp_socket_receive <= 0) {
    	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_Send_Advert_To: no socket");
		return ;
	}

    if (!advert_message){
        advert_message = (char *)malloc(strlen(message_template) +  100);
    }

    sprintf(advert_message, message_template, 
        myip, 
        g_ssdp_uuid, 
        g_ssdp_uuid);

    // set up destination address
    //
    nbytes = sendto(
        g_ssdp_socket_receive,
        (const char*) advert_message,
        strlen(advert_message),
        0,
        (struct sockaddr*) addr,
        sizeof(struct sockaddr)
    );

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_Send_Advert_To: sent message");
}





static const char notify_template[] = 
"NOTIFY * HTTP/1.1\r\n" \
"HOST: 239.255.255.250:1900\r\n" \
"CACHE-CONTROL: max-age=1800\r\n" \
"LOCATION: http://%s:80/ssdp.xml'\r\n" \
"NTS: ssdp:alive\r\n" \
"NT: upnp:rootdevice\r\n" \
"USN: uuid:%s::upnp:rootdevice\r\n" \
"\r\n\r\n" \
;



static void DRV_SSDP_Send_Notify() {
	int nbytes;
    struct sockaddr_in multicastaddr;

    if (g_ssdp_socket_receive <= 0){
    	addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP,"DRV_SSDP_Send_Notify: no socket");
        return;
    }
    // set up destination address
    //
    memset(&multicastaddr, 0, sizeof(multicastaddr));
    multicastaddr.sin_family = AF_INET;
    multicastaddr.sin_addr.s_addr = inet_addr(ssdp_group);
    multicastaddr.sin_port = htons(ssdp_port);

    const char *myip = HAL_GetMyIPString();

    if (!notify_message){
        notify_message = (char *)malloc(strlen(notify_template) +  100);
    }

    sprintf(notify_message, notify_template, myip, g_ssdp_uuid);

    // set up destination address
    //
    nbytes = sendto(
        g_ssdp_socket_receive,
        (const char*) notify_message,
        strlen(notify_message),
        0,
        (struct sockaddr*) &multicastaddr,
        sizeof(multicastaddr)
    );
	
    if (nbytes <= 0){
	    addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"#### ERROR ##### DRV_SSDP_Send_Notify: sent message %d bytes", nbytes);
    } else {
	    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP,"DRV_SSDP_Send_Notify: sent message %d bytes", nbytes);
    }
}


static const char *http_reply = 
"<root>\r\n" \
"    <specVersion>\r\n" \
"        <major>1</major>\r\n" \
"        <minor>0</minor>\r\n" \
"    </specVersion>\r\n" \
"    <device>\r\n" \
"        <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>\r\n" \
"        <friendlyName>%s</friendlyName>\r\n" \
"        <manufacturer>OpenBeken</manufacturer>\r\n" \
"        <manufacturerURL>https://github.com/openshwprojects/OpenBK7231T_App</manufacturerURL>\r\n" \
"        <modelDescription>OpenBeken</modelDescription>\r\n" \
"        <modelName>%s</modelName>\r\n" \
"        <modelNumber>1</modelNumber>\r\n" \
"        <modelURL>https://github.com/openshwprojects/OpenBK7231T_App</modelURL>\r\n" \
"        <serialNumber>1</serialNumber>\r\n" \
"        <UDN>uuid:%s</UDN>\r\n" \
"        <serviceList>\r\n" \
"            <service>\r\n" \
"                <URLBase>http://%s:80</URLBase>\r\n" \
"   <serviceType>urn:schemas-upnp-org:service:openbk:1</serviceType>\r\n" \
"    <serviceId>urn:upnp-org:serviceId:1</serviceId>" \
"                <controlURL>/</controlURL>\r\n" \
"                <eventSubURL/>\r\n" \
"                <SCPDURL>/ssdp.xml</SCPDURL>\r\n" \
"            </service>\r\n" \
"        </serviceList>\r\n" \
"        <presentationURL>http://%s:80/</presentationURL>\r\n" \
"    </device>\r\n" \
"</root>\r\n"
;

static int DRV_SSDP_Service_Http(http_request_t* request){
    const char *myip = HAL_GetMyIPString();
    const char *freindly_name = CFG_GetDeviceName();

    if (!http_message){
        http_message_len = 
            strlen(http_reply) + 
            strlen(freindly_name) +
            strlen(PLATFORM_MCU_NAME) +
            strlen(g_ssdp_uuid) + 
            strlen(myip) + 
            strlen(myip) + 
            40;
        http_message = (char *)malloc(http_message_len);
    }

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"###################### DRV_SSDP_Service_Http");

    snprintf(http_message, http_message_len, http_reply, 
        freindly_name,
        PLATFORM_MCU_NAME,
        g_ssdp_uuid, 
        myip, 
        myip);

    http_setup(request, "application/xml");
	poststr(request, http_message);
    poststr(request, NULL);
    return 0;
}
// end private functions
///////////////////////////////


///////////////////////////////////////////////
// public functions, only used in drv_main

void DRV_SSDP_Init()
{
    addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_Init");
    // like "e427ce1a-3e80-43d0-ad6f-89ec42e46363";
    sprintf(g_ssdp_uuid, "%08x-%04x-%04x-%04x-%04x%08x",
        (unsigned int)rand(), 
        (unsigned int)rand()&0xffff,
        (unsigned int)rand()&0xffff,
        (unsigned int)rand()&0xffff,
        (unsigned int)rand()&0xffff,
        (unsigned int)rand()
    );

	DRV_SSDP_CreateSocket_Receive();
    HTTP_RegisterCallback("/ssdp.xml", HTTP_GET, DRV_SSDP_Service_Http);
    //CMD_RegisterCommand("SSDPNotify", "", CMD_SSDP_Notify, "qqq", NULL);

    DRV_SSDP_Active = 1;
}

void DRV_SSDP_RunEverySecond() {
    ssdp_timercount++;
    if (ssdp_timercount >= 30){
        // multicast a notify
        DRV_SSDP_Send_Notify();
        ssdp_timercount = 0;
    }
}

void DRV_SSDP_RunQuickTick() {

	if (g_ssdp_socket_receive <= 0) {
		return ;
	}
    // now just enter a read-print loop
    //
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    if (!udp_msgbuf){
        udp_msgbuf = (char *)malloc(UDP_MSGBUF_LEN);
    }

    socklen_t addrlen = sizeof(addr);
    int nbytes = recvfrom(
        g_ssdp_socket_receive,
        udp_msgbuf,
        UDP_MSGBUF_LEN,
        0,
        (struct sockaddr *) &addr,
        &addrlen
    );
    if (nbytes <= 0) {
        //addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"nothing\n");
        return ;
    }
    // just so we can terminate for print
    if (nbytes >= UDP_MSGBUF_LEN){
        nbytes = UDP_MSGBUF_LEN-1;
    }
    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP,"Received %i bytes from %s",nbytes,inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
    udp_msgbuf[nbytes] = 0;
    addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP,"data: %s",udp_msgbuf);
    udp_msgbuf[nbytes] = '\0';

    /* we may get:
    M-SEARCH * HTTP/1.1
    HOST:239.255.255.250:1900
    ST:upnp:rootdevice
    MX:2
    MAN:"ssdp:discover"
    */

    // if search, then respond
    // we SHOULD be a little more specific!!!
    if (!strncmp(udp_msgbuf, "M-SEARCH", 8)){
        // reply with our advert to the sender
        addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_HTTP,"Is MSEARCH - responding");
        DRV_SSDP_Send_Advert_To(&addr);
    }
}


void DRV_SSDP_Shutdown(){
    addLogAdv(LOG_INFO, LOG_FEATURE_HTTP,"DRV_SSDP_Shutdown");
    DRV_SSDP_Active = 0;

	if(g_ssdp_socket_receive>=0) {
		close(g_ssdp_socket_receive);
		g_ssdp_socket_receive = -1;
	}

    if (advert_message){
        free(advert_message);
        advert_message = NULL;
    }
    if (udp_msgbuf){
        free(udp_msgbuf);
        udp_msgbuf = NULL;
    }
    if (notify_message) {
        free(notify_message);
        notify_message = NULL;
    }
    if (http_message) {
        free(http_message);
        http_message = NULL;
    }
}

// end public
///////////////////////////////////////////////
