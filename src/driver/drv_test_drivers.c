#include "../new_common.h"
#include "drv_local.h"

//Test Power driver
void Test_Power_Init() {
	BL_Shared_Init();
}
void Test_Power_RunFrame() {
	float final_v = 120;
	float final_c = 1;
	float final_p = 120;

	int r = rand() % 100;
	BL_ProcessUpdate(final_v + (r / 1000), final_c + (r / 10), final_p + r);
}

//Test LED driver
void Test_LED_Driver_Init() {
}
void Test_LED_Driver_RunFrame() {
}
void Test_LED_Driver_OnChannelChanged(int ch, int value) {
}