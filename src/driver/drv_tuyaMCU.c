#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_tuyaMCU.h"
#include "drv_uart.h"
#include <time.h>
#include "drv_ntp.h"


#define TUYA_CMD_HEARTBEAT     0x00
#define TUYA_CMD_QUERY_PRODUCT 0x01
#define TUYA_CMD_MCU_CONF      0x02
#define TUYA_CMD_WIFI_STATE    0x03
#define TUYA_CMD_WIFI_RESET    0x04
#define TUYA_CMD_WIFI_SELECT   0x05
#define TUYA_CMD_SET_DP        0x06
#define TUYA_CMD_STATE         0x07
#define TUYA_CMD_QUERY_STATE   0x08
#define TUYA_CMD_SET_TIME      0x1C

void TuyaMCU_RunFrame();





//=============================================================================
//dp data point type
//=============================================================================
#define         DP_TYPE_RAW                     0x00        //RAW type
#define         DP_TYPE_BOOL                    0x01        //bool type
#define         DP_TYPE_VALUE                   0x02        //value type
#define         DP_TYPE_STRING                  0x03        //string type
#define         DP_TYPE_ENUM                    0x04        //enum type
#define         DP_TYPE_BITMAP                  0x05        //fault type


const char *TuyaMCU_GetDataTypeString(int dpId){
	if(DP_TYPE_RAW == dpId)
		return "DP_TYPE_RAW";
	if(DP_TYPE_BOOL == dpId)
		return "DP_TYPE_BOOL";
	if(DP_TYPE_VALUE == dpId)
		return "DP_TYPE_VALUE";
	if(DP_TYPE_STRING == dpId)
		return "DP_TYPE_STRING";
	if(DP_TYPE_ENUM == dpId)
		return "DP_TYPE_ENUM";
	if(DP_TYPE_BITMAP == dpId)
		return "DP_TYPE_BITMAP";
	return "DP_ERROR";
}
// from http_fns.  should move to a utils file.
extern unsigned char hexbyte( const char* hex );




const char *TuyaMCU_GetCommandTypeLabel(int t) {
	if(t == TUYA_CMD_HEARTBEAT)
		return "Hearbeat";
	if(t == TUYA_CMD_QUERY_PRODUCT)
		return "QueryProductInformation";
	if(t == TUYA_CMD_MCU_CONF)
		return "MCUconf";
	if(t == TUYA_CMD_WIFI_STATE)
		return "WiFiState";
	if(t == TUYA_CMD_WIFI_RESET)
		return "WiFiReset";
	if(t == TUYA_CMD_WIFI_SELECT)
		return "WiFiSelect";
	if(t == TUYA_CMD_SET_DP)
		return "SetDP";
	if(t == TUYA_CMD_STATE)
		return "State";
	if(t == TUYA_CMD_QUERY_STATE)
		return "QueryState";
	if(t == TUYA_CMD_SET_TIME)
		return "SetTime";
	return "Unknown";
}
typedef struct rtcc_s {
	uint8_t       second;
	uint8_t       minute;
	uint8_t       hour;
	uint8_t       day_of_week;               // sunday is day 1
	uint8_t       day_of_month;
	uint8_t       month;
	char          name_of_month[4];
	uint16_t      day_of_year;
	uint16_t      year;
	uint32_t      days;
	uint32_t      valid;
} rtcc_t;

typedef struct tuyaMCUMapping_s {
	// internal Tuya variable index
	int fnId;
	// target channel
	int channel;
	// TODO
	//int mode;
	// list
	struct tuyaMCUMapping_s *next;
} tuyaMCUMapping_t;

tuyaMCUMapping_t *g_tuyaMappings = 0;

tuyaMCUMapping_t *TuyaMCU_FindDefForID(int fnId) {
	tuyaMCUMapping_t *cur;

	cur = g_tuyaMappings;
	while(cur) {
		if(cur->fnId == fnId)
			return cur;
		cur = cur->next;
	}
	return 0;
}
void TuyaMCU_MapIDToChannel(int fnId, int channel) {
	tuyaMCUMapping_t *cur;

	cur = TuyaMCU_FindDefForID(fnId);

	if(cur == 0) {
		cur = (tuyaMCUMapping_t*)malloc(sizeof(tuyaMCUMapping_t));
		cur->fnId = fnId;
		cur->next = g_tuyaMappings;
		g_tuyaMappings = cur;
	}

	cur->channel = channel;
}


// header version command lenght data checksum
// 55AA		00		00		0000   xx	00

#define MIN_TUYAMCU_PACKET_SIZE (2+1+1+2+1)
int UART_TryToGetNextTuyaPacket(byte *out, int maxSize) {
	int cs;
	int len, i;
	int c_garbage_consumed = 0;
	byte a, b, version, command, lena, lenb;
	
	cs = UART_GetDataSize();

	if(cs < MIN_TUYAMCU_PACKET_SIZE) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
		a = UART_GetNextByte(0);
		b = UART_GetNextByte(1);
		if(a != 0x55 || b != 0xAA) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"Consumed %i unwanted non-header byte in Tuya MCU buffer\n", c_garbage_consumed);
	}
	if(cs < MIN_TUYAMCU_PACKET_SIZE) {
		return 0;
	}
	a = UART_GetNextByte(0);
	b = UART_GetNextByte(1);
	if(a != 0x55 || b != 0xAA) {
		return 0;
	}
	version = UART_GetNextByte(2);
	command = UART_GetNextByte(3);
	lena = UART_GetNextByte(4); // hi
	lenb = UART_GetNextByte(5); // lo
	len = lenb | lena >> 8;
	// now check if we have received whole packet
	len += 2 + 1 + 1 + 2 + 1; // header 2 bytes, version, command, lenght, chekcusm
	if(cs >= len) {
		for(i = 0; i < len; i++) {
			out[i] = UART_GetNextByte(i);
		}
		// consume whole packet (but don't touch next one, if any)
		UART_ConsumeBytes(len);
		return len;
	}
	return 0;
}




// append header, len, everything, checksum
void TuyaMCU_SendCommandWithData(byte cmdType, byte *data, int payload_len) {
	int i;

	byte check_sum = (0xFF + cmdType + (payload_len >> 8) + (payload_len & 0xFF));
	UART_InitUART(9600);
	UART_SendByte(0x55);
	UART_SendByte(0xAA);
	UART_SendByte(0x00);         // version 00
	UART_SendByte(cmdType);         // version 00
	UART_SendByte(payload_len >> 8);      // following data length (Hi)
	UART_SendByte(payload_len & 0xFF);    // following data length (Lo)
	for(i = 0; i < payload_len; i++) {
		byte b = data[i];
		check_sum += b;
		UART_SendByte(b);
	}
	UART_SendByte(check_sum);
}

void TuyaMCU_Send_SetTime(struct tm *pTime) {
	byte payload_buffer[8];
	byte tuya_day_of_week;

	if (pTime->tm_wday == 1) {
		tuya_day_of_week = 7;
	} else {
		tuya_day_of_week = pTime->tm_wday-1;
	}

	payload_buffer[0] = 0x01;
	payload_buffer[1] = pTime->tm_year % 100;
	payload_buffer[2] = pTime->tm_mon;
	payload_buffer[3] = pTime->tm_mday;
	payload_buffer[4] = pTime->tm_hour;
	payload_buffer[5] = pTime->tm_min;
	payload_buffer[6] = pTime->tm_sec;
	payload_buffer[7] = tuya_day_of_week; //1 for Monday in TUYA Doc

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_TIME, payload_buffer, 8);
}
struct tm * TuyaMCU_Get_NTP_Time() {
	int g_time;
	struct tm * ptm;

	g_time = NTP_GetCurrentTime();
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"MCU time to set: %i\n", g_time);
	ptm = gmtime(&g_time);
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"ptime ->gmtime => tm_hour: %i\n",ptm->tm_hour );
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"ptime ->gmtime => tm_min: %i\n", ptm->tm_min );

	return ptm;
}
int TuyaMCU_Send_Hex(const void *context, const char *cmd, const char *args) {
	//const char *args = CMD_GetArg(1);
	if(!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_Send_Hex: requires 1 argument (hex string, like FFAABB00CCDD\n");
		return -1;
	}
	while(*args) {
		byte b;
		b = hexbyte(args);
		
		UART_SendByte(b);

		args += 2;
	}
	return 1;
}

int TuyaMCU_LinkTuyaMCUOutputToChannel(const void *context, const char *cmd, const char *args) {
	int dpID;
	int channelID;

	// linkTuyaMCUOutputToChannel dpID channelID [varType]
	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_LinkTuyaMCUOutputToChannel: requires 2 arguments (dpID, channelIndex)\n");
		return -1;
	}
	dpID = Tokenizer_GetArgInteger(0);
	channelID = Tokenizer_GetArgInteger(1);

	TuyaMCU_MapIDToChannel(dpID,channelID);

	return 1;
}

int TuyaMCU_Send_SetTime_Current(const void *context, const char *cmd, const char *args) {

	TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time());

	return 1;
}
int TuyaMCU_Send_SetTime_Example(const void *context, const char *cmd, const char *args) {
	struct tm testTime;

	testTime.tm_year = 2012;
	testTime.tm_mon = 7;
	testTime.tm_mday = 15;
	testTime.tm_wday = 4;
	testTime.tm_hour = 6;
	testTime.tm_min = 54;
	testTime.tm_sec = 32;

	TuyaMCU_Send_SetTime(&testTime);
	return 1;
}

void TuyaMCU_Send(byte *data, int size) {
	int i;
    unsigned char check_sum;

	check_sum = 0;
	for(i = 0; i < size; i++) {
		byte b = data[i];
		check_sum += b;
		UART_SendByte(b);
	}
	UART_SendByte(check_sum);

	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"\nWe sent %i bytes to Tuya MCU\n",size+1);
}
void TuyaMCU_Init()
{
	UART_InitUART(9600);
	UART_InitReceiveRingBuffer(256);
	// uartSendHex 55AA0008000007
	CMD_RegisterCommand("tuyaMcu_testSendTime","",TuyaMCU_Send_SetTime_Example, "Sends a example date by TuyaMCU to clock/callendar MCU", NULL);
	CMD_RegisterCommand("tuyaMcu_sendCurTime","",TuyaMCU_Send_SetTime_Current, "Sends a current date by TuyaMCU to clock/callendar MCU", NULL);
	CMD_RegisterCommand("uartSendHex","",TuyaMCU_Send_Hex, "Sends raw data by TuyaMCU UART, you must write whole packet with checksum yourself", NULL);
	///CMD_RegisterCommand("tuyaMcu_sendSimple","",TuyaMCU_Send_Simple, "Appends a 0x55 0xAA header to a data, append a checksum at end and send");
	CMD_RegisterCommand("linkTuyaMCUOutputToChannel","",TuyaMCU_LinkTuyaMCUOutputToChannel, "Map value send from TuyaMCU (eg. humidity or temperature) to channel", NULL);

}
// ntp_timeZoneOfs 2
// addRepeatingEvent 10 uartSendHex 55AA0008000007
// setChannelType 1 temperature_div10
// setChannelType 2 humidity
// linkTuyaMCUOutputToChannel 1 1
// linkTuyaMCUOutputToChannel 2 2
// addRepeatingEvent 10 tuyaMcu_sendCurTime
//
// same as above but merged
// backlog ntp_timeZoneOfs 2; addRepeatingEvent 10 uartSendHex 55AA0008000007; setChannelType 1 temperature_div10; setChannelType 2 humidity; linkTuyaMCUOutputToChannel 1 1; linkTuyaMCUOutputToChannel 2 2; addRepeatingEvent 10 tuyaMcu_sendCurTime
//
void TuyaMCU_ApplyMapping(int fnID, int value) {
	tuyaMCUMapping_t *mapping;

	// find mapping (where to save received data)
	mapping = TuyaMCU_FindDefForID(fnID);

	if(mapping == 0){
		return;
	}
	CHANNEL_Set(mapping->channel,value,0);
}
void TuyaMCU_ParseStateMessage(const byte *data, int len) {
	int ofs;
	int sectorLen;
	int fnId;
	int dataType;

	ofs = 0;
	
	while(ofs + 4 < len) {
		sectorLen = data[ofs + 2] << 8 | data[ofs + 3];
		fnId = data[ofs];
		dataType = data[ofs+1];
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ParseStateMessage: processing command %i, dataType %i-%s and %i data bytes\n",
			fnId, dataType, TuyaMCU_GetDataTypeString(dataType),sectorLen);


		if(sectorLen == 1) {
			int iVal = (int)data[ofs+4];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ParseStateMessage: raw data 1 byte: %c\n",iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId,iVal);
		}
		if(sectorLen == 4) {  
			int iVal = data[ofs + 4] << 24 | data[ofs + 5] << 16 | data[ofs + 6] << 8 | data[ofs + 7];
			addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ParseStateMessage: raw data 4 int: %i\n",iVal);
			// apply to channels
			TuyaMCU_ApplyMapping(fnId,iVal);
		}

		// size of header (type, datatype, len 2 bytes) + data sector size
		ofs += (4+sectorLen);
	}

}

void TuyaMCU_ProcessIncoming(const byte *data, int len) {
	int checkLen;
	int i;
	byte checkCheckSum;
	byte cmd;
	if(data[0] != 0x55 || data[1] != 0xAA) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ProcessIncoming: discarding packet with bad ident and len %i\n",len);
		return;
	}
	checkLen = data[5] | data[4] >> 8;
	checkLen = checkLen + 2 + 1 + 1 + 2 + 1;
	if(checkLen != len) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ProcessIncoming: discarding packet bad expected len, expected %i and got len %i\n",checkLen,len);
		return;
	}
	checkCheckSum = 0;
	for(i = 0; i < len-1; i++) {
		checkCheckSum += data[i];
	}
	if(checkCheckSum != data[len-1]) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ProcessIncoming: discarding packet bad expected checksum, expected %i and got checksum %i\n",(int)data[len-1],(int)checkCheckSum);
		return;
	}
	cmd = data[3];
	addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ProcessIncoming: processing command %i (%s) with %i bytes\n",cmd,TuyaMCU_GetCommandTypeLabel(cmd),len);
	switch(cmd)
	{
	case TUYA_CMD_STATE:
		TuyaMCU_ParseStateMessage(data+6,len-6);
		break;
	case TUYA_CMD_SET_TIME:
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TuyaMCU_ProcessIncoming: received TUYA_CMD_SET_TIME, so sending back time");
		TuyaMCU_Send_SetTime(TuyaMCU_Get_NTP_Time());
		break;
	}
}
void TuyaMCU_RunFrame() {
	byte data[128];
	char buffer_for_log[256];
	char buffer2[4];
	int len, i;

	//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"UART ring buffer state: %i %i\n",g_recvBufIn,g_recvBufOut);

	len = UART_TryToGetNextTuyaPacket(data,sizeof(data));
	if(len > 0) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TUYAMCU received: ");
		buffer_for_log[0] = 0;
		for(i = 0; i < len; i++) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"%02X ",data[i]);
			sprintf(buffer2,"%02X ",data[i]);
			strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,buffer_for_log);
		//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"\n");
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"TUYAMCU received: %s\n", buffer_for_log);
		TuyaMCU_ProcessIncoming(data,len);
	}
}

