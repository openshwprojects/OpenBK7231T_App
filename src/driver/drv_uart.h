#pragma once

void UART_InitReceiveRingBuffer(int size);
int UART_GetDataSize();
byte UART_GetByte(int idx);
void UART_ConsumeBytes(int idx);
void UART_AppendByteToReceiveRingBuffer(int rc);
void UART_SendByte(byte b);
void UART_InitUART(int baud, int parity);
void UART_AddCommands();
void UART_RunEverySecond();

// used to detect uart reinit/takeover by driver
extern int g_uart_init_counter;
