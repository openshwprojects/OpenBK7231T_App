#ifdef WINDOWS

#include "../../new_common.h"


int xSemaphoreTake(int semaphore, int blockTime) {
	return 1;
}
int xSemaphoreCreateMutex() {
	return 0;
}
int xSemaphoreGive(int semaphore) {
	return 0;
}
int rtos_delay_milliseconds(int sec) {
	Sleep(sec);
	return 0;
}
int delay_ms(int sec) {
	Sleep(sec);
	return 0;
}

int xTaskGetTickCount() {
	return 9999;
}

int xPortGetFreeHeapSize() {
	return 100 * 1000;
}

int LWIP_GetMaxSockets() {
	return 9999;
}

int LWIP_GetActiveSockets() {
	return 1;
}
int lwip_close(int socket) {
	return close(socket);
}
int lwip_close_force(int socket) {
	return close(socket);
}
int hal_machw_time() {
	return 1;
}
int hal_machw_time_past(int tt) {
	return 1;
}
int rtos_create_thread() {
	return 0;
}
void rtos_delete_thread(int i) {
	
}
int lwip_fcntl(int s, int cmd, int val) {
	int argp;

	argp = 1;
	if (ioctlsocket(s,
		FIONBIO,
		&argp) == SOCKET_ERROR)
	{
		printf("ioctlsocket() error %d\n", WSAGetLastError());
		return 1;
	}

	return 0;
}

#endif
