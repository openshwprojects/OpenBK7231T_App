#include "new_common.h"
#include "new_pins.h"
#include "new_cfg.h"
#include "new_cmd.h"


#if PLATFORM_BK7231T | PLATFORM_BK7231N
#include "../../beken378/func/user_driver/BkDriverUart.h"
#endif



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

#if PLATFORM_BK7231T | PLATFORM_BK7231N
void test_ty_read_uart_data_to_buffer(int port, void* param)
{
    int rc = 0;
    
    while((rc = uart_read_byte(port)) != -1)
    {
     //   if(__ty_uart_read_data_size(port) < (ty_uart[port].buf_len-1)) 
    //    {
     //       ty_uart[port].buf[ty_uart[port].in++] = rc;
      ///      if(ty_uart[port].in >= ty_uart[port].buf_len){
     ///           ty_uart[port].in = 0;
     ///       }
      //  }
    }

}
#endif

void TuyaMCU_Bridge_InitUART(int baud) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	bk_uart_config_t config;

    config.baud_rate = 9600;
    config.data_width = 0x03;
    config.parity = 0;    //0:no parity,1:odd,2:even
    config.stop_bits = 0;   //0:1bit,1:2bit
    config.flow_control = 0;   //FLOW_CTRL_DISABLED
    config.flags = 0;

    bk_uart_initialize(0, &config, NULL);
    bk_uart_set_rx_callback(0, test_ty_read_uart_data_to_buffer, NULL);
#else


#endif
}
void TuyaMCU_Bridge_SendUARTByte(byte b) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	bk_send_byte(0, b);
#elif WINDOWS
	// STUB - for testing
    printf("%02X", b);
#else


#endif
}

// append header, len, everything, checksum
void TuyaMCU_SendCommandWithData(byte cmdType, byte *data, int payload_len) {
	int i;

	byte check_sum = (0xFF + cmdType + (payload_len >> 8) + (payload_len & 0xFF));
	TuyaMCU_Bridge_InitUART(9600);
	TuyaMCU_Bridge_SendUARTByte(0x55);
	TuyaMCU_Bridge_SendUARTByte(0xAA);
	TuyaMCU_Bridge_SendUARTByte(0x00);         // version 00
	TuyaMCU_Bridge_SendUARTByte(cmdType);         // version 00
	TuyaMCU_Bridge_SendUARTByte(payload_len >> 8);      // following data length (Hi)
	TuyaMCU_Bridge_SendUARTByte(payload_len & 0xFF);    // following data length (Lo)
	for(i = 0; i < payload_len; i++) {
		byte b = data[i];
		check_sum += b;
		TuyaMCU_Bridge_SendUARTByte(b);
	}
	TuyaMCU_Bridge_SendUARTByte(check_sum);
}

void TuyaMCU_Send_SetTime(rtcc_t *pTime) {
	byte payload_buffer[8];
	byte tuya_day_of_week;

	if (pTime->day_of_week == 1) {
		tuya_day_of_week = 7;
	} else {
		tuya_day_of_week = pTime->day_of_week-1;
	}

	payload_buffer[0] = 0x01;
	payload_buffer[1] = pTime->year % 100;
	payload_buffer[2] = pTime->month;
	payload_buffer[3] = pTime->day_of_month;
	payload_buffer[4] = pTime->hour;
	payload_buffer[5] = pTime->minute;
	payload_buffer[6] = pTime->second;
	payload_buffer[7] = tuya_day_of_week; //1 for Monday in TUYA Doc

	TuyaMCU_SendCommandWithData(TUYA_CMD_SET_TIME, payload_buffer, 8);
}
void TuyaMCU_Send_SetTime_Example() {
	rtcc_t testTime;

	testTime.year = 2012;
	testTime.month = 7;
	testTime.day_of_month = 15;
	testTime.day_of_week = 4;
	testTime.hour = 6;
	testTime.minute = 54;
	testTime.second = 32;

	TuyaMCU_Send_SetTime(&testTime);
}
void TuyaMCU_Send(byte *data, int size) {
	int i;
    unsigned char check_sum;

	TuyaMCU_Bridge_InitUART(9600);

	check_sum = 0;
	for(i = 0; i < size; i++) {
		byte b = data[i];
		check_sum += b;
		TuyaMCU_Bridge_SendUARTByte(b);
	}
	TuyaMCU_Bridge_SendUARTByte(check_sum);

	printf("\nWe sent %i bytes to Tuya MCU\n",size+1);
}
void TuyaMCU_Init()
{
	CMD_RegisterCommand("tuyaMcu_testSendTime","",TuyaMCU_Send_SetTime_Example);

}
