#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"


#if PLATFORM_BK7231T | PLATFORM_BK7231N
#include "../../beken378/func/user_driver/BkDriverUart.h"
#endif

#if PLATFORM_BK7231T | PLATFORM_BK7231N
	// from uart_bk.c
	extern void bk_send_byte(UINT8 uport, UINT8 data);
#elif WINDOWS
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
void UART_SendByte(byte b) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	bk_send_byte(0, b);
#elif WINDOWS
	// STUB - for testing
    addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"%02X", b);
#else


#endif
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

    bk_uart_initialize(0, &config, NULL);
    bk_uart_set_rx_callback(0, test_ty_read_uart_data_to_buffer, NULL);
#else


#endif
}







