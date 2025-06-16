#if PLATFORM_XR872

void HAL_WDG_Reboot();

void HAL_RebootModule() {


	HAL_WDG_Reboot();
}

#endif // PLATFORM_XR872