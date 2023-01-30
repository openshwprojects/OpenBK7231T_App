


void UART_InitReceiveRingBuffer(int size);
int UART_GetDataSize();
byte UART_GetNextByte(int index);
void UART_ConsumeBytes(int idx);
void UART_AppendByteToCircularBuffer(int rc);
void UART_SendByte(byte b);
void UART_InitUART(int baud);

// used to detect uart reinit/takeover by driver
extern int g_uart_init_counter;




