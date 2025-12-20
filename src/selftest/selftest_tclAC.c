#ifdef WINDOWS

#include "selftest_local.h"


void Test_Driver_TCL_AC() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);
	SIM_ClearUART();

	CMD_ExecuteCommand("startDriver TCL", 0);

	CMD_ExecuteCommand("ACMode 1", 0);
	TCL_UART_RunEverySecond();
	SIM_ClearUART();

	CMD_ExecuteCommand("FANMode 1", 0);
	TCL_UART_RunEverySecond();
	SIM_ClearUART();


	CMD_ExecuteCommand("uartFakeHex BB 01 00 04 2D 04 00 00 00 00 00 00 FF 00 00 00 00 00 FF FF 00 00 00 00 00 00 F0 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 6A", 0);
	TCL_UART_RunEverySecond();
}



#endif


