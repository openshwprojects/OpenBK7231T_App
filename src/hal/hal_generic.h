#include <stdint.h>

void HAL_RebootModule();
void HAL_Delay_us(int delay);
// Return a wrapping microsecond count for short interval timing.
uint32_t HAL_GetMicroseconds(void);
void HAL_Configure_WDT();
void HAL_Run_WDT();
void HAL_RegisterPlatformSpecificCommands();
