#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"


#if PLATFORM_BK7231T | PLATFORM_BK7231N
#include "../../beken378/func/user_driver/BkDriverUart.h"
#endif

#if PLATFORM_BL602
#include <bl_uart.h>
#include <bl_irq.h>
#endif
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	// from uart_bk.c
	extern void bk_send_byte(UINT8 uport, UINT8 data);
#elif WINDOWS

#elif PLATFORM_BL602 
//int g_fd;
uint8_t g_id = 1;
#else
#endif

static byte *g_recvBuf = 0;
static int g_recvBufSize = 0;
static int g_recvBufIn = 0;
static int g_recvBufOut = 0;

void UART_InitReceiveRingBuffer(int size){
	if(g_recvBuf!=0)
		free(g_recvBuf);
	g_recvBuf = (byte*)malloc(size);
	memset(g_recvBuf,0,size);
	g_recvBufSize = size;
	g_recvBufIn = 0;
}
int UART_GetDataSize()
{
    int remain_buf_size = 0;

    if(g_recvBufIn >= g_recvBufOut) {
        remain_buf_size = g_recvBufIn - g_recvBufOut;
    }else {
        remain_buf_size = g_recvBufIn + g_recvBufSize - g_recvBufOut;
    }

    return remain_buf_size;
}
byte UART_GetNextByte(int index) {
	int realIndex = g_recvBufOut + index;
	if(realIndex > g_recvBufSize)
		realIndex -= g_recvBufSize;

	return g_recvBuf[realIndex];
}
void UART_ConsumeBytes(int idx) {
	g_recvBufOut += idx;
	if(g_recvBufOut > g_recvBufSize)
		g_recvBufOut -= g_recvBufSize;
}

void UART_AppendByteToCircularBuffer(int rc) {
    if(UART_GetDataSize() < (g_recvBufSize-1))
    {
        g_recvBuf[g_recvBufIn++] = rc;
        if(g_recvBufIn >= g_recvBufSize){
            g_recvBufIn = 0;
        }
   }
}
#if PLATFORM_BK7231T | PLATFORM_BK7231N
void test_ty_read_uart_data_to_buffer(int port, void* param)
{
    int rc = 0;

    while((rc = uart_read_byte(port)) != -1)
    {
		UART_AppendByteToCircularBuffer(rc);
    }

}
#endif

#ifdef PLATFORM_BL602
//void UART_RunQuickTick() {
//}
void MY_UART1_IRQHandler(void)
{
	int length;
	byte buffer[16];
	//length = aos_read(g_fd, buffer, 1);
	//if (length > 0) {
	//	UART_AppendByteToCircularBuffer(buffer[0]);
	//}
	int res = bl_uart_data_recv(g_id);
	if (res >= 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "UART received: %i\n", res);
		UART_AppendByteToCircularBuffer(res);
	}
}
#endif
void UART_SendByte(byte b) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	// BK_UART_1 is defined to 0
	bk_send_byte(BK_UART_1, b);
#elif WINDOWS
	// STUB - for testing
    addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"%02X", b);
#elif PLATFORM_BL602
	//aos_write(g_fd, &b, 1);
	bl_uart_data_send(g_id, b);
#else


#endif
}

commandResult_t CMD_UART_Send_Hex(const void *context, const char *cmd, const char *args, int cmdFlags) {
	//const char *args = CMD_GetArg(1);
	if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_Send_Hex: requires 1 argument (hex string, like FFAABB00CCDD\n");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	while (*args) {
		byte b;
		b = hexbyte(args);

		UART_SendByte(b);

		args += 2;
	}
	return CMD_RES_OK;
}


// uartSendASCII test123
commandResult_t CMD_UART_Send_ASCII(const void *context, const char *cmd, const char *args, int cmdFlags) {
	//const char *args = CMD_GetArg(1);
	if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_Send_ASCII: requires 1 argument (hex string, like hellp world\n");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	while (*args) {

		UART_SendByte(*args);

		args++;
	}
	return CMD_RES_OK;
}
bool b_uart_commands_added = false;
void UART_AddCommands() {
	//cmddetail:{"name":"uartSendHex","args":"[HexString]",
	//cmddetail:"descr":"Sends raw data by UART, can be used to send TuyaMCU data, but you must write whole packet with checksum yourself",
	//cmddetail:"fn":"CMD_UART_Send_Hex","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("uartSendHex", NULL, CMD_UART_Send_Hex, NULL, NULL);
	//cmddetail:{"name":"uartSendASCII","args":"[AsciiString]",
	//cmddetail:"descr":"Sends given string by UART.",
	//cmddetail:"fn":"CMD_UART_Send_ASCII","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("uartSendASCII", NULL, CMD_UART_Send_ASCII, NULL, NULL);
}
void UART_InitUART(int baud) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	bk_uart_config_t config;

    config.baud_rate = baud;
    config.data_width = 0x03;
    config.parity = 0;    //0:no parity,1:odd,2:even
    config.stop_bits = 0;   //0:1bit,1:2bit
    config.flow_control = 0;   //FLOW_CTRL_DISABLED
    config.flags = 0;

	// BK_UART_1 is defined to 0
    bk_uart_initialize(BK_UART_1, &config, NULL);
	// BK_UART_1 is defined to 0
    bk_uart_set_rx_callback(BK_UART_1, test_ty_read_uart_data_to_buffer, NULL);
#elif PLATFORM_BL602
	uint8_t tx_pin = 16;
	uint8_t rx_pin = 7;
	bl_uart_init(g_id, tx_pin, rx_pin, 0, 0, baud);
	//g_fd = aos_open(name, 0);
	bl_uart_int_rx_enable(1);
	bl_irq_register(UART1_IRQn, MY_UART1_IRQHandler);
	bl_irq_enable(UART1_IRQn);
#else


#endif

	if (b_uart_commands_added == false) {
		b_uart_commands_added = true;
		UART_AddCommands();
	}
}







