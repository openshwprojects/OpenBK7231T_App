#ifdef WINDOWS

#include "new_common.h"

const char *dataToSimulate[] =
{
	"55AA0307000801020004000000041C",
	"55AA030700130600000F0000000C0000001001C3000077091CAD",
	"55AA0307000509050001001D",
	"55AA03070005100100010121",
	"55AA030700221100001E00000000000000000000006400010E0000AA00000000000A00000000000081",
	"55AA0307001512000011000500640005001E003C0000000000000009",
	"55AA03070008650200040000000480",
	"55AA0307000866020004000000007D",
	"55AA03070008670200040000000C8A",
	"55AA0307000869020004000013881B",
	"55AA030700086D0200040000000387",
	"55AA030700086E0200040000001095",
	"55AA030700086F020004000001BE45",
};
int g_totalStrings = sizeof(dataToSimulate) / sizeof(dataToSimulate[0]);

int delay_between_packets = 500;
int curString = 0;
const char *curP = 0;
int current_delay_to_wait_ms = 1000;

void NewTuyaMCUSimulator_RunQuickTick(int deltaMS) {
	if (current_delay_to_wait_ms > 0) {
		current_delay_to_wait_ms -= deltaMS;
		return;
	}
	if (curP == 0) {
		curP = dataToSimulate[curString];
	}

	byte b = hexbyte(curP);
	UART_AppendByteToCircularBuffer(b);
	curP += 2;
	if (*curP == 0) {
		curString++;
		if (curString >= g_totalStrings)
			curString = 0;
		curP = 0;
		current_delay_to_wait_ms = delay_between_packets;
	}

}


#endif


