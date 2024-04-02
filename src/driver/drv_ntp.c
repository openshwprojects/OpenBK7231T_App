// NTP client
// Based on my previous work here:
// https://www.elektroda.pl/rtvforum/topic3712112.html

#include <time.h>

#include "../new_common.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../ota/ota.h"

#include "drv_ntp.h"
#include "drv_deviceclock.h"


#define LOG_FEATURE LOG_FEATURE_NTP

typedef struct
{

  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

#define MAKE_WORD(hi, lo) hi << 8 | lo

// NTP time since 1900 to unix time (since 1970)
// Number of seconds to ad
#define NTP_OFFSET 2208988800L

static int g_ntp_socket = 0;
static struct sockaddr_in g_address;
static int adrLen;
// in seconds, before next retry
static int g_ntp_delay = 0;
static bool g_synced = false;

// current time - this may be 32 or 64 bit, depending on platform
//time_t g_deviceTime;

commandResult_t NTP_SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	int a, b;
	const char *arg;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	arg = Tokenizer_GetArg(0);
	if (strchr(arg, ':')) {
		int useSign = 1;
		if (*arg == '-') {
			arg++;
			useSign = -1;
		}
		sscanf(arg, "%i:%i", &a, &b);
		setDeviceTimeOffset(useSign * (a * 60 * 60 + b * 60));
	}
	else {
		setDeviceTimeOffset(Tokenizer_GetArgInteger(0) * 60 * 60);
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP offset set");
	return CMD_RES_OK;
}


//Set custom NTP server
commandResult_t NTP_SetServer(const void *context, const char *cmd, const char *args, int cmdFlags) {
    const char *newValue;

    Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
    newValue = Tokenizer_GetArg(0);
    CFG_SetNTPServer(newValue);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP server set to %s", newValue);
    return CMD_RES_OK;
}

//Display settings used by the NTP driver
commandResult_t NTP_Info(const void *context, const char *cmd, const char *args, int cmdFlags) {
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "Server=%s, Time offset=%d", CFG_GetNTPServer(), GetTimesZoneOfsSeconds());
    return CMD_RES_OK;
}
#if WINDOWS
bool b_ntp_simulatedTime = false;
void NTP_SetSimulatedTime(unsigned int timeNow) {
	setDeviceTime(timeNow);
	g_synced = true;
	b_ntp_simulatedTime = true;
}
#endif
void NTP_Init() {

#if WINDOWS
	b_ntp_simulatedTime = false;
#endif
	//cmddetail:{"name":"ntp_timeZoneOfs","args":"[Value]",
	//cmddetail:"descr":"Sets the time zone offset in hours. Also supports HH:MM syntax if you want to specify value in minutes. For negative values, use -HH:MM syntax, for example -5:30 will shift time by 5 hours and 30 minutes negative.",
	//cmddetail:"fn":"NTP_SetTimeZoneOfs","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_timeZoneOfs",NTP_SetTimeZoneOfs, NULL);
	//cmddetail:{"name":"ntp_setServer","args":"[ServerIP]",
	//cmddetail:"descr":"Sets the NTP server",
	//cmddetail:"fn":"NTP_SetServer","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_setServer", NTP_SetServer, NULL);
	//cmddetail:{"name":"ntp_info","args":"",
	//cmddetail:"descr":"Display NTP related settings",
	//cmddetail:"fn":"NTP_Info","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_info", NTP_Info, NULL);

    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP driver initialized with server=%s, offset=%d", CFG_GetNTPServer(), GetTimesZoneOfsSeconds());
    g_synced = false;
}

void NTP_StopDriver() {
// can/should we "de-Register" commands??
/*
stopping an staring driver will show:

Error:CMD:command with name ntp_timeZoneOfs already exists!
Error:CMD:command with name ntp_setServer already exists!
Error:CMD:command with name ntp_info already exists! 

*/
// commands registered during start:
/*
	//cmddetail:{"name":"ntp_timeZoneOfs","args":"[Value]",
	//cmddetail:"descr":"Sets the time zone offset in hours. Also supports HH:MM syntax if you want to specify value in minutes. For negative values, use -HH:MM syntax, for example -5:30 will shift time by 5 hours and 30 minutes negative.",
	//cmddetail:"fn":"NTP_SetTimeZoneOfs","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_timeZoneOfs",NTP_SetTimeZoneOfs, NULL);
	//cmddetail:{"name":"ntp_setServer","args":"[ServerIP]",
	//cmddetail:"descr":"Sets the NTP server",
	//cmddetail:"fn":"NTP_SetServer","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_setServer", NTP_SetServer, NULL);
#if ENABLE_SUNRISE_SUNSET
	//cmddetail:{"name":"ntp_setLatLong","args":"[Latlong]",
	//cmddetail:"descr":"Sets the NTP latitude and longitude",
	//cmddetail:"fn":"SetLatlong","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":"SetLatlong -34.911498 138.809488"}
    CMD_RegisterCommand("ntp_setLatLong", SetLatlong, NULL);
#endif
	//cmddetail:{"name":"ntp_info","args":"",
	//cmddetail:"descr":"Display NTP related settings",
	//cmddetail:"fn":"NTP_Info","file":"driver/drv_ntp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("ntp_info", NTP_Info, NULL);
*/
// at least take care, that time is recognized as invalid
// since stoping the driver will stop calling "NTP_OnEverySecond()"
// so no further  "g_deviceTime++;" from that moment ...
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP driver stoped");
    g_synced = false;
}


void NTP_Shutdown() {
    if(g_ntp_socket != 0) {
#if WINDOWS
        closesocket(g_ntp_socket);
#else
        lwip_close(g_ntp_socket);
#endif
    }
    g_ntp_socket = 0;
    // can attempt in next 10 seconds
    g_ntp_delay = 60;
}
void NTP_SendRequest(bool bBlocking) {
    byte *ptr;
	const char *adrString;
    //int i, recv_len;
    //char buf[64];
    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    adrLen = sizeof(g_address);
    memset( &packet, 0, sizeof( ntp_packet ) );
    ptr = (byte*)&packet;
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    ptr[0] = 0xE3;   // LI, Version, Mode
    ptr[1] = 0;     // Stratum, or type of clock
    ptr[2] = 6;     // Polling Interval
    ptr[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    ptr[12]  = 49;
    ptr[13]  = 0x4E;
    ptr[14]  = 49;
    ptr[15]  = 52;


    //create a UDP socket
    if ((g_ntp_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1)
    {
        g_ntp_socket = 0;
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: failed to create socket");
        return;
    }

    memset((char *) &g_address, 0, sizeof(g_address));

	adrString = CFG_GetNTPServer();
	if (adrString == 0 || adrString[0] == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "NTP_SendRequest: somehow ntp server in config was empty, setting non-empty");
		CFG_SetNTPServer(DEFAULT_NTP_SERVER);
		adrString = CFG_GetNTPServer();
	}

    g_address.sin_family = AF_INET;
    g_address.sin_addr.s_addr = inet_addr(adrString);
    g_address.sin_port = htons(123);


    // Send the message to server:
    if(sendto(g_ntp_socket, &packet, sizeof(packet), 0,
         (struct sockaddr*)&g_address, adrLen) < 0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: Unable to send message");
        NTP_Shutdown();
		// quick next frame attempt
		if (g_secondsElapsed < 60) {
			g_ntp_delay = 0;
		}
        return;
    }

    // https://github.com/tuya/tuya-iotos-embeded-sdk-wifi-ble-bk7231t/blob/5e28e1f9a1a9d88425f3fd4b658e895a8ee7b83b/platforms/bk7231t/tuya_os_adapter/src/system/tuya_hal_network.c
    //
    if(bBlocking == false) {
#if WINDOWS
#else
        if(fcntl(g_ntp_socket, F_SETFL, O_NONBLOCK)) {
            addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: failed to make socket non-blocking!");
        }
#endif
    }

    // can attempt in next 10 seconds
    g_ntp_delay = 10;
}
void NTP_CheckForReceive() {
    byte *ptr;
    int i, recv_len;
    //struct tm * ptm;
    unsigned short highWord;
    unsigned short lowWord;
    unsigned int secsSince1900;
    struct tm *ltm;
    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ptr = (byte*)&packet;

    // Receive the server's response:
    i = sizeof(packet);
#if 0
    recv_len = recvfrom(g_ntp_socket, ptr, i, 0,
         (struct sockaddr*)&g_address, &adrLen);
#else
    recv_len = recv(g_ntp_socket, ptr, i, 0);
#endif

    if(recv_len < 0){
			addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_CheckForReceive: Error while receiving server's msg");
        return;
    }
    highWord = MAKE_WORD(ptr[40], ptr[41]);
    lowWord = MAKE_WORD(ptr[42], ptr[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    secsSince1900 = highWord << 16 | lowWord;
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Seconds since Jan 1 1900 = %u",secsSince1900);

    setDeviceTime(secsSince1900 - NTP_OFFSET);

    addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Unix time  : %u",(unsigned int)g_deviceTime);
    ltm = gmtime(&g_deviceTime);
    addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Local Time : %04d/%02d/%02d %02d:%02d:%02d",
            ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

	if (g_synced == false) {
		EventHandlers_FireEvent(CMD_EVENT_NTP_STATE, 1);
	}
    g_synced = true;
#if 0
    //ptm = gmtime (&g_deviceTime);
    ptm = gmtime(&g_deviceTime);
    if(ptm == 0) {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime somehow returned 0\n");
    } else {
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_year: %i\n",ptm->tm_year);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mon: %i\n",ptm->tm_mon);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mday: %i\n",ptm->tm_mday);
        addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_hour: %i\n",ptm->tm_hour  );
    }
#endif
    NTP_Shutdown();

}

void NTP_SendRequest_BlockingMode() {
    NTP_Shutdown();
    NTP_SendRequest(true);
    NTP_CheckForReceive();

}

void NTP_OnEverySecond()
{
    if(Main_IsConnectedToWiFi()==0)
    {
        return;
    }
#if WINDOWS
	if (b_ntp_simulatedTime) {
		return;
	}
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
    if (ota_progress() != -1)
    {
        return;
    }
#endif
    if(g_ntp_socket == 0) {
        // if no socket, this is a reconnect delay
        if(g_ntp_delay > 0) {
            g_ntp_delay--;
            return;
        }
        NTP_SendRequest(false);
    } else {
        NTP_CheckForReceive();
        // if socket exists, this is a disconnect timeout
        if(g_ntp_delay > 0) {
            g_ntp_delay--;
            if(g_ntp_delay<=0) {
                // disconnect and force reconnect
                NTP_Shutdown();
            }
        }
    }
}

void AppendTimeInformationToHTTPIndexPage(http_request_t* request)
{
    struct tm *ltm;

    ltm = gmtime(&g_deviceTime);

    if (g_synced == true)
        hprintf255(request, "<h5>NTP (%s): Local Time: %04d/%02d/%02d %02d:%02d:%02d </h5>",
			CFG_GetNTPServer(),ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    else 
        hprintf255(request, "<h5>NTP: Syncing with %s....</h5>",CFG_GetNTPServer());
}

bool NTP_IsTimeSynced()
{
    return g_synced;
}

