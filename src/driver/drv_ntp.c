// NTP client
// Based on my previous work here: 
// https://www.elektroda.pl/rtvforum/topic3712112.html

#include "../new_common.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"


#include "../logging/logging.h"

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
static int g_ntp_delay = 5;
// current time
static unsigned int g_time;
// time offset (time zone?)
static int g_timeOffsetHours;

int NTP_SetTimeZoneOfs(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args);
	if(Tokenizer_GetArgsCount() < 1) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Command requires one argument\n");
		return 0;
	}
	g_timeOffsetHours = Tokenizer_GetArgInteger(0) * 60 * 60;
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP offset set, wait for next ntp packet to apply changes\n");
	return 1;
}
void NTP_Init() {

	CMD_RegisterCommand("ntp_timeZoneOfs","",NTP_SetTimeZoneOfs, "Sets the time zone offset in hours", NULL);
}

unsigned int NTP_GetCurrentTime() {
	return g_time;
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
	g_ntp_delay = 10;
}
void NTP_SendRequest(bool bBlocking) {
	byte *ptr;
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

        g_address.sin_family = AF_INET;
        g_address.sin_addr.s_addr = inet_addr("217.147.223.78"); // this is address of host which I want to send the socket
        g_address.sin_port = htons(123);


    // Send the message to server:
    if(sendto(g_ntp_socket, &packet, sizeof(packet), 0,
         (struct sockaddr*)&g_address, adrLen) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: Unable to send message");
		NTP_Shutdown();
        return;
    }

	// https://github.com/tuya/tuya-iotos-embeded-sdk-wifi-ble-bk7231t/blob/5e28e1f9a1a9d88425f3fd4b658e895a8ee7b83b/platforms/bk7231t/tuya_os_adapter/src/system/tuya_hal_network.c
	//
	if(bBlocking == false) {
#if WINDOWS
#else
		if(fcntl(g_ntp_socket, F_SETFL, O_NONBLOCK)) {
			addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_SendRequest: failed to make socket non-blocking!\n");
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
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"NTP_CheckForReceive: Error while receiving server's msg\n");
        return;
    }
	highWord = MAKE_WORD(ptr[40], ptr[41]);
	lowWord = MAKE_WORD(ptr[42], ptr[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    secsSince1900 = highWord << 16 | lowWord;
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Seconds since Jan 1 1900 = %u",secsSince1900);

	g_time = secsSince1900 - NTP_OFFSET;
	g_time += g_timeOffsetHours;
	addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"Unix time = %u",g_time);
#if 0
	//ptm = localtime (&g_time);
	ptm = gmtime(&g_time);
	if(ptm == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime somehow returned 0\n");
	} else {
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_year: %i\n",ptm->tm_year);
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mon: %i\n",ptm->tm_mon);
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_mday: %i\n",ptm->tm_mday);
		addLogAdv(LOG_INFO, LOG_FEATURE_NTP,"gmtime => tm_hour: %i\n",ptm->tm_hour	);
	}
#endif
	NTP_Shutdown();

}

void NTP_SendRequest_BlockingMode() {
	NTP_Shutdown();
	NTP_SendRequest(true);
	NTP_CheckForReceive();

}


void NTP_OnEverySecond() {
	g_time++;

	if(Main_IsConnectedToWiFi()==0) {
		return;
	}
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
