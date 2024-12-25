#ifdef PLATFORM_TR6260

void HAL_RebootModule()
{
	wdt_reset_chip(0);
}

#endif // PLATFORM_TR6260