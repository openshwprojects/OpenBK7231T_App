#pragma once

//---------------------------------------------------
// Routines using UART port depending on config 
// flag OBK_FLAG_USE_SECONDARY_UART
// Note: using same buffer as routines below
//--------------------------------------------------
void UART_InitReceiveRingBuffer(int size);
int UART_GetReceiveRingBufferSize();
int UART_GetDataSize();
byte UART_GetByte(int idx);
void UART_ConsumeBytes(int idx);
void UART_AppendByteToReceiveRingBuffer(int rc);
void UART_SendByte(byte b);
int UART_InitUART(int baud, int parity, bool hwflowc);
void UART_AddCommands();
void UART_RunEverySecond();

// used to detect uart reinit/takeover by driver
int get_g_uart_init_counter();
// used to get selected port from config - OBK_FLAG_USE_SECONDARY_UART
int UART_GetSelectedPortIndex();

//---------------------------------------------------
// XJIKKA 20241123 new routines with uart index param 
// BEKEN platform only (yet)
// Independent of OBK_FLAG_USE_SECONDARY_UART 
//---------------------------------------------------
//index of UART port 
//	BEKEN - UART_PORT_INDEX_0 = BK_UART_1 RX1 TX1
//	BEKEN - UART_PORT_INDEX_1 = BK_UART_2 RX2 TX2
#define UART_PORT_INDEX_0 0
#define UART_PORT_INDEX_1 1
//
int UART_GetBufIndexFromPort(int aport);
void UART_InitReceiveRingBufferEx(int auartindex, int size);
void UART_AppendByteToReceiveRingBufferEx(int auartindex, int rc);
int UART_GetReceiveRingBufferSizeEx(int auartindex);
int UART_GetDataSizeEx(int auartindex);
byte UART_GetByteEx(int auartindex, int idx);
void UART_ConsumeBytesEx(int auartindex, int idx);
void UART_SendByteEx(int auartindex, byte b);
int UART_InitUARTEx(int auartindex, int baud, int parity, bool hwflowc);
void UART_LogBufState(int auartindex);

