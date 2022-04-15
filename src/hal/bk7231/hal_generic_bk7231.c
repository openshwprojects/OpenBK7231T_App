

// from wlan_ui.c
void bk_reboot(void);

void HAL_RebootModule() {
	bk_reboot();
}