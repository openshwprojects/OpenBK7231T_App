#if PLATFORM_XR809 || PLATFORM_XR872

void HAL_WDG_Reboot();

void HAL_RebootModule() {


	HAL_WDG_Reboot();
}

#endif // PLATFORM_XR809