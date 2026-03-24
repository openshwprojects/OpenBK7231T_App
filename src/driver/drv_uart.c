#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"
#include "../hal/hal_uart.h"

//#define UART_ALWAYSFIRSTBYTES 
#define UART_DEFAULT_BUFIZE 512
#ifdef UART_2_UARTS_CONCURRENT
#define UART_BUF_CNT 2
#else
#define UART_BUF_CNT 1
#endif
//index of buffer
// if we have 2 buffers, port UART_PORT_INDEX_0 uses buf 0 and UART_PORT_INDEX_1 uses buf 1
// if we have 1 buffer, both ports using buf 0  (as before)
#define UART_BUF_INDEX_0 0  
#ifdef UART_2_UARTS_CONCURRENT
#define UART_BUF_INDEX_1 1
#endif

int UART_GetSelectedPortIndex() {
  return (CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART)) ? UART_PORT_INDEX_1 : UART_PORT_INDEX_0;
}

int UART_GetBufIndexFromPort(int aport) {
#if UART_BUF_CNT==2
  return (aport == UART_PORT_INDEX_1) ? UART_BUF_INDEX_1 : UART_BUF_INDEX_0;
#else
  return UART_BUF_INDEX_0;
#endif
}

typedef struct {
  byte* g_recvBuf;
  int g_recvBufSize;
  int g_recvBufIn;
  int g_recvBufOut;
// used to detect uart reinit
  int g_uart_init_counter;
// used to detect uart manual mode
  int g_uart_manualInitCounter;
} uartbuf_t;

static uartbuf_t uartbuf[UART_BUF_CNT] = { {0,0,0,0,0,-1}
  #if UART_BUF_CNT == 2
    , { 0,0,0,0,0,-1 } 
  #endif
  };

uartbuf_t * UART_GetBufFromPort(int aport) {
  return &uartbuf[UART_GetBufIndexFromPort(aport)];
}

int get_g_uart_init_counter() {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(UART_GetSelectedPortIndex());
  return fuartbuf->g_uart_init_counter;
}

void UART_InitReceiveRingBufferEx(int auartindex, int size){
  uartbuf_t* fuartbuf=UART_GetBufFromPort(auartindex);
  //XJIKKA 20241122 - Note that the actual usable buffer size must be g_recvBufSize-1, 
    //otherwise there would be no difference between an empty and a full buffer.
	  if(fuartbuf->g_recvBuf!=0)
        free(fuartbuf->g_recvBuf);
	  fuartbuf->g_recvBuf = (byte*)malloc(size);
	  memset(fuartbuf->g_recvBuf,0,size);
    fuartbuf->g_recvBufSize = size;
    fuartbuf->g_recvBufIn = 0;
    fuartbuf->g_recvBufOut = 0;
}

void UART_InitReceiveRingBuffer(int size) {
  int fuartindex = UART_GetSelectedPortIndex();
  UART_InitReceiveRingBufferEx(fuartindex, size);
}

int UART_GetReceiveRingBufferSizeEx(int auartindex) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  return fuartbuf->g_recvBufSize;
}

int UART_GetReceiveRingBufferSize() {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_GetReceiveRingBufferSizeEx(fuartindex);
}

int UART_GetDataSizeEx(int auartindex) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  return (fuartbuf->g_recvBufIn >= fuartbuf->g_recvBufOut
                ? fuartbuf->g_recvBufIn - fuartbuf->g_recvBufOut
                : fuartbuf->g_recvBufIn + (fuartbuf->g_recvBufSize - fuartbuf->g_recvBufOut)); //XJIKKA 20241122 fixed buffer size calculation on ring bufferroverflow
}

int UART_GetDataSize() {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_GetDataSizeEx(fuartindex);
}

byte UART_GetByteEx(int auartindex, int idx) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  return fuartbuf->g_recvBuf[(fuartbuf->g_recvBufOut + idx) % fuartbuf->g_recvBufSize];
}

byte UART_GetByte(int idx) {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_GetByteEx(fuartindex, idx);
}

void UART_ConsumeBytesEx(int auartindex, int idx) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  fuartbuf->g_recvBufOut += idx;
  fuartbuf->g_recvBufOut %= fuartbuf->g_recvBufSize;
}

void UART_ConsumeBytes(int idx) {
  int fuartindex = UART_GetSelectedPortIndex();
  UART_ConsumeBytesEx(fuartindex, idx);
}

void UART_AppendByteToReceiveRingBufferEx(int auartindex, int rc) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  if (fuartbuf->g_recvBufSize <= 0) {
      //if someone (uartFakeHex) send data without init, and if flag 26 changes(UART)
      addLogAdv(LOG_ERROR, LOG_FEATURE_DRV, "UART %i not initialized\n",auartindex);
      //return;
      UART_InitReceiveRingBufferEx(auartindex,UART_DEFAULT_BUFIZE);
    }
#ifdef UART_ALWAYSFIRSTBYTES
    //20250119 old style, if g_recvBufSize-1 is reached, received byte was ignored
    if (UART_GetDataSizeEx(auartindex) < (fuartbuf->g_recvBufSize - 1)) {
#else
    //20250119 new style, we have always last g_recvBufSize-1 bytes
    //if g_recvBufSize-1 is reached, first byte is overwritten
#endif
        fuartbuf->g_recvBuf[fuartbuf->g_recvBufIn++] = rc;
        fuartbuf->g_recvBufIn %= fuartbuf->g_recvBufSize;
#ifdef UART_ALWAYSFIRSTBYTES
    }
#endif
    //XJIKKA 20241122 if the same pointer is reached (in and out), we must also advance 
    //the outbuffer pointer, otherwise UART_GetDataSize will return g_recvBufSize.
    //This way now we always have the last (g_recvBufSize - 1) bytes
    if (fuartbuf->g_recvBufIn == fuartbuf->g_recvBufOut) {
      fuartbuf->g_recvBufOut++;
      fuartbuf->g_recvBufOut %= fuartbuf->g_recvBufSize;
    }
}

void UART_AppendByteToReceiveRingBuffer(int rc) {
  int fuartindex = UART_GetSelectedPortIndex();
  UART_AppendByteToReceiveRingBufferEx(fuartindex, rc);
}

void UART_SendByteEx(int auartindex, byte b) {
#ifdef UART_2_UARTS_CONCURRENT
  HAL_UART_SendByteEx(auartindex, b);
#else
  HAL_UART_SendByte(b);
#endif
}

void UART_SendByte(byte b) {
  int fuartindex = UART_GetSelectedPortIndex();
  UART_SendByteEx(fuartindex, b);
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
        if (*args == ' ' || *args == '\t') {
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

void UART_ResetForSimulator() {
  for (int i = 0; i < UART_BUF_CNT; i++) {
    uartbuf[i].g_uart_init_counter = 0;
  }
}

int UART_InitUARTEx(int auartindex, int baud, int parity, bool hwflowc)
{
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  fuartbuf->g_uart_init_counter++;
#ifdef UART_2_UARTS_CONCURRENT
    HAL_UART_InitEx(auartindex, baud, parity, hwflowc, -1, -1);
#else
    HAL_UART_Init(baud, parity, hwflowc, -1, -1);
#endif
  return fuartbuf->g_uart_init_counter;
}

int UART_InitUART(int baud, int parity, bool hwflowc) {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_InitUARTEx(fuartindex, baud, parity, hwflowc);
}

void UART_LogBufState(int auartindex) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  ADDLOG_WARN(LOG_INFO,
    "Uart ix %d inbuf %i inptr %i outptr %i \n",
    auartindex, UART_GetDataSizeEx(auartindex), fuartbuf->g_recvBufIn, fuartbuf->g_recvBufOut
  );
}
void UART_DebugTool_Run(int auartindex) {
	byte b;
	char tmp[128];
	char *p = tmp;
	int bytes = 0;

	while (UART_GetDataSizeEx(auartindex) > 0) {
		b = UART_GetByteEx(auartindex, 0);

		int needed = (bytes ? 3 : 2);  // 2 for first byte, 3 (space + 2 hex) after
		if ((p - tmp) + needed >= sizeof(tmp) - 1) break;

		if (bytes) *p++ = ' ';
		sprintf(p, "%02X", b);
		p += 2;
		UART_ConsumeBytesEx(auartindex, 1);
		bytes++;
	}

	*p = 0;
	if (bytes) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
			"UART %i received %i bytes: %s\n", auartindex, bytes, tmp);
	}
}

void UART_RunEverySecond() {
  for (int i = 0; i < UART_BUF_CNT; i++) {
    if (uartbuf[i].g_uart_manualInitCounter == uartbuf[i].g_uart_init_counter) {
      UART_DebugTool_Run(i);
    }
  }
}
// uartInit 115200
// uartSendASCII Hello123
commandResult_t CMD_UART_Init(const void *context, const char *cmd, const char *args, int cmdFlags) {
    int baud;

    Tokenizer_TokenizeString(args, 0);
    // following check must be done after 'Tokenizer_TokenizeString',
    // so we know arguments count in Tokenizer. 'cmd' argument is
    // only for warning display
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    int fuartindex = UART_GetSelectedPortIndex();

    baud = Tokenizer_GetArgInteger(0);

    UART_InitReceiveRingBufferEx(fuartindex, UART_DEFAULT_BUFIZE);

    UART_InitUARTEx(fuartindex, baud, 0, false);
    uartbuf_t* fuartbuf = UART_GetBufFromPort(fuartindex);
    fuartbuf->g_uart_manualInitCounter = fuartbuf->g_uart_init_counter;

    return CMD_RES_OK;
}

void UART_AddCommands() {
	//cmddetail:{"name":"uartSendHex","args":"[HexString]",
	//cmddetail:"descr":"Sends raw data by UART, can be used to send TuyaMCU data, but you must write whole packet with checksum yourself",
	//cmddetail:"fn":"CMD_UART_Send_Hex","file":"driver/drv_uart.c","requires":"",
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
