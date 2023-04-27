// Here is how you can get log print on UART1, instead of default UART2 on Beken

// Enable "[UART] Enable UART command line"
// this also can be done in flags, enable command line on UART1 at 115200 baud
SetFlag 31 1
// UART1 is RXD1/TXD1 which is used for programming and for TuyaMCU/BL0942,
// but now we will set that UART1 is used for log
logPort 1 
