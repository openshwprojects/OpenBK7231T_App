
// WXDM2 dimmer demo
// WXDM2 is using custom UART protocol with no checksum


again:
addChannel 10 1 0 255 1
delay_ms 100
uartSendHex 0A FF 55 02 00 $CH10$ 00 00 0A
goto again


