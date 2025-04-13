
void HAL_RebootModule();
void HAL_Delay_us(int delay);
#if PLATFORM_BEKEN
void BEKEN_test_Tickscount();
#endif
void HAL_Configure_WDT();
void HAL_Run_WDT();
