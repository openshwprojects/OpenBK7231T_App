#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"
#include "../hal/hal_uart.h"

#ifdef PLATFORM_ESPIDF
#include "freertos/portmacro.h"
static portMUX_TYPE uartRingMux = portMUX_INITIALIZER_UNLOCKED;
#define UART_ENTER_CRITICAL()   portENTER_CRITICAL(&uartRingMux)
#define UART_EXIT_CRITICAL()    portEXIT_CRITICAL(&uartRingMux)
#else
#define UART_ENTER_CRITICAL()
#define UART_EXIT_CRITICAL()
#endif

#define UART_DEFAULT_BUFIZE 512
#ifdef UART_2_UARTS_CONCURRENT
#define UART_BUF_CNT 2
#else
#define UART_BUF_CNT 1
#endif

#define UART_BUF_INDEX_0 0  
#ifdef UART_2_UARTS_CONCURRENT
#define UART_BUF_INDEX_1 1
#endif

typedef struct {
  byte* g_recvBuf;
  int g_recvBufSize;
  int g_recvBufIn;
  int g_recvBufOut;
  int g_uart_init_counter;
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
  UART_ENTER_CRITICAL();
  int size = (fuartbuf->g_recvBufIn >= fuartbuf->g_recvBufOut)
                ? fuartbuf->g_recvBufIn - fuartbuf->g_recvBufOut
                : fuartbuf->g_recvBufIn + (fuartbuf->g_recvBufSize - fuartbuf->g_recvBufOut);
  UART_EXIT_CRITICAL();
  return size;
}

int UART_GetDataSize() {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_GetDataSizeEx(fuartindex);
}

byte UART_GetByteEx(int auartindex, int idx) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  UART_ENTER_CRITICAL();
  byte b = fuartbuf->g_recvBuf[(fuartbuf->g_recvBufOut + idx) % fuartbuf->g_recvBufSize];
  UART_EXIT_CRITICAL();
  return b;
}

byte UART_GetByte(int idx) {
  int fuartindex = UART_GetSelectedPortIndex();
  return UART_GetByteEx(fuartindex, idx);
}

void UART_ConsumeBytesEx(int auartindex, int idx) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  UART_ENTER_CRITICAL();
  fuartbuf->g_recvBufOut += idx;
  fuartbuf->g_recvBufOut %= fuartbuf->g_recvBufSize;
  UART_EXIT_CRITICAL();
}

void UART_ConsumeBytes(int idx) {
  int fuartindex = UART_GetSelectedPortIndex();
  UART_ConsumeBytesEx(fuartindex, idx);
}

void UART_AppendByteToReceiveRingBufferEx(int auartindex, int rc) {
  uartbuf_t* fuartbuf = UART_GetBufFromPort(auartindex);
  UART_ENTER_CRITICAL();

  if (fuartbuf->g_recvBufSize <= 0) {
      UART_InitReceiveRingBufferEx(auartindex,UART_DEFAULT_BUFIZE);
    }

  fuartbuf->g_recvBuf[fuartbuf->g_recvBufIn++] = rc;
  fuartbuf->g_recvBufIn %= fuartbuf->g_recvBufSize;

  if (fuartbuf->g_recvBufIn == fuartbuf->g_recvBufOut) {
    fuartbuf->g_recvBufOut++;
    fuartbuf->g_recvBufOut %= fuartbuf->g_recvBufSize;
  }

  UART_EXIT_CRITICAL();
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

commandResult_t CMD_UART_FakeHex(const void *context, const char *cmd, const char *args, int cmdFlags) {
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
        UART_AppendByteToReceiveRingBuffer(b);
        args += 2;
    }
    return CMD_RES_OK;
}

commandResult_t CMD_UART_Send_ASCII(const void *context, const char *cmd, const char *args, int cmdFlags) {
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

		int needed = (bytes ? 3 : 2);
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

commandResult_t CMD_UART_Init(const void *context, const char *cmd, const char *args, int cmdFlags) {
    int baud;

    Tokenizer_TokenizeString(args, 0);
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
    CMD_RegisterCommand("uartSendHex", CMD_UART_Send_Hex, NULL);
    CMD_RegisterCommand("uartSendASCII", CMD_UART_Send_ASCII, NULL);
    CMD_RegisterCommand("uartFakeHex", CMD_UART_FakeHex, NULL);
    CMD_RegisterCommand("uartInit", CMD_UART_Init, NULL);
}

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