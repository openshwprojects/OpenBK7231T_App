#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"
#include "../hal/hal_uart.h"

static byte *g_recvBuf = 0;
static int g_recvBufSize = 0;
static int g_recvBufIn = 0;
static int g_recvBufOut = 0;
// used to detect uart reinit
int g_uart_init_counter = 0;
// used to detect uart manual mode
int g_uart_manualInitCounter = -1;

void UART_InitReceiveRingBuffer(int size){
    //XJIKKA 20241122 - Note that the actual usable buffer size must be g_recvBufSize-1, 
    //otherwise there would be no difference between an empty and a full buffer.
	if(g_recvBuf!=0)
        free(g_recvBuf);
	g_recvBuf = (byte*)malloc(size);
	memset(g_recvBuf,0,size);
    g_recvBufSize = size;
    g_recvBufIn = 0;
    g_recvBufOut = 0;
}

int UART_GetDataSize() {
    return (g_recvBufIn >= g_recvBufOut
                ? g_recvBufIn - g_recvBufOut
                : g_recvBufIn + (g_recvBufSize - g_recvBufOut)); //XJIKKA 20241122 fixed buffer size calculation on ring bufferroverflow
}

byte UART_GetByte(int idx) {
    return g_recvBuf[(g_recvBufOut + idx) % g_recvBufSize];
}

void UART_ConsumeBytes(int idx) {
    g_recvBufOut += idx;
	g_recvBufOut %= g_recvBufSize;
}

void UART_AppendByteToReceiveRingBuffer(int rc) {
    if (UART_GetDataSize() < (g_recvBufSize - 1)) {
        g_recvBuf[g_recvBufIn++] = rc;
        g_recvBufIn %= g_recvBufSize;
    }
    //XJIKKA 20241122 if the same pointer is reached (in and out), we must also advance 
    //the outbuffer pointer, otherwise UART_GetDataSize will return g_recvBufSize.
    //This way now we always have the last (g_recvBufSize - 1) bytes
    if (g_recvBufIn == g_recvBufOut) {
      g_recvBufOut++;
      g_recvBufOut %= g_recvBufSize;
    }
}

void UART_SendByte(byte b) 
{
    HAL_UART_SendByte(b);
}

commandResult_t CMD_UART_Send_Hex(const void *context, const char *cmd, const char *args, int cmdFlags) {
    if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_Send_Hex: requires 1 argument (hex string, like FFAABB00CCDD\n");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    while (*args) {
		byte b = CMD_ParseOrExpandHexByte(&args);
        UART_SendByte(b);
    }
    return CMD_RES_OK;
}

// This is a tool for debugging.
// It simulates OpenBeken receiving packet from UART.
// For example, you can do:
/*
1. You can simulate TuyaMCU battery powered device packet:

uartFakeHex 55 AA 00 05 00 05 01 04 00 01 01 10 55
55 AA	00	05		00 05	0104000101	10
HEADER	VER=00	Unk		LEN	dpId=1 Enum V=1	CHK

backlog startDriver TuyaMCU; uartFakeHex 55 AA 00 05 00 05 01 04 00 01 01 10 55
*/

commandResult_t CMD_UART_FakeHex(const void *context, const char *cmd, const char *args, int cmdFlags) {
	//const char *args = CMD_GetArg(1);
	//byte rawData[128];
	//int curCnt;

	//curCnt = 0;
    if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_FakeHex: requires 1 argument (hex string, like FFAABB00CCDD\n");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    while (*args) {
        byte b;
        if (*args == ' ') {
            args++;
            continue;
        }
        b = hexbyte(args);

		//rawData[curCnt] = b;
		//curCnt++;
		//if(curCnt>=sizeof(rawData)) {
		//  addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"CMD_UART_FakeHex: sorry, given string is too long\n");
		//  return -1;
		//}

        UART_AppendByteToReceiveRingBuffer(b);

        args += 2;
    }
    return 1;
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

void UART_ResetForSimulator() 
{
	g_uart_init_counter = 0;
}

int UART_InitUART(int baud, int parity)
{
    g_uart_init_counter++;
    HAL_UART_Init(baud, parity);
    return g_uart_init_counter;
}

void UART_DebugTool_Run() {
    byte b;
    char tmp[128];
    char *p = tmp;
    int i;

    for (i = 0; i < sizeof(tmp) - 4; i++) {
		if (UART_GetDataSize()==0) {
            break;
        }
        b = UART_GetByte(0);
        if (i) {
            *p = ' ';
            p++;
        }
        sprintf(p, "%02X", b);
        p += 2;
        UART_ConsumeBytes(1);
    }
    *p = 0;
    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "UART received: %s\n", tmp);
}

void UART_RunEverySecond() {
    if (g_uart_manualInitCounter == g_uart_init_counter) {
        UART_DebugTool_Run();
    }
}

commandResult_t CMD_UART_Init(const void *context, const char *cmd, const char *args, int cmdFlags) {
    int baud;

    Tokenizer_TokenizeString(args, 0);
    // following check must be done after 'Tokenizer_TokenizeString',
    // so we know arguments count in Tokenizer. 'cmd' argument is
    // only for warning display
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    baud = Tokenizer_GetArgInteger(0);

    UART_InitReceiveRingBuffer(512);

    UART_InitUART(baud, 0);
    g_uart_manualInitCounter = g_uart_init_counter;

    return CMD_RES_OK;
}

void UART_AddCommands() {
	//cmddetail:{"name":"uartSendHex","args":"[HexString]",
	//cmddetail:"descr":"Sends raw data by UART, can be used to send TuyaMCU data, but you must write whole packet with checksum yourself",
	//cmddetail:"fn":"CMD_UART_Send_Hex","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartSendHex", CMD_UART_Send_Hex, NULL);
	//cmddetail:{"name":"uartSendASCII","args":"[AsciiString]",
	//cmddetail:"descr":"Sends given string by UART.",
	//cmddetail:"fn":"CMD_UART_Send_ASCII","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartSendASCII", CMD_UART_Send_ASCII, NULL);
	//cmddetail:{"name":"uartFakeHex","args":"[HexString]",
	//cmddetail:"descr":"Spoofs a fake hex packet so it looks like TuyaMCU send that to us. Used for testing.",
	//cmddetail:"fn":"CMD_UART_FakeHex","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartFakeHex", CMD_UART_FakeHex, NULL);
	//cmddetail:{"name":"uartInit","args":"[BaudRate]",
	//cmddetail:"descr":"Manually starts UART1 port. Keep in mind that you don't need to do it for TuyaMCU and BL0942, those drivers do it automatically.",
	//cmddetail:"fn":"CMD_UART_Init","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartInit", CMD_UART_Init, NULL);
}
