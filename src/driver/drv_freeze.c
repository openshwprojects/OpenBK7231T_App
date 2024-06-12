// freeze
#include "drv_local.h"

void Freeze_OnEverySecond() {
	while (1) {
		// freeze
	}
}
void Freeze_RunFrame() {
	//don't freeze, seems this does not allow watchdog reset to work on BL602
}
void Freeze_Init() {
	// don't freeze
}
