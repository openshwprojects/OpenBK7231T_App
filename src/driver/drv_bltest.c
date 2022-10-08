#include "../new_common.h"
#include "drv_local.h"

//Test driver

void BLTest_Init() {
}
void BLTest_RunFrame() {
	float final_v = 120;
	float final_c = 1;
	float final_p = 120;
	
	int r = rand()% 100;
	BL_ProcessUpdate(final_v + (r/1000), final_c + (r/10), final_p + r);
}
