#ifdef WINDOWS

#include "../../new_common.h"

#ifndef LINUX

#include <timeapi.h>

#define GETSOCKETERRNO() (WSAGetLastError())

#else

#include <time.h>

#define timeGetTime() time(NULL)
#define DWORD uint

#define SOCKET_ERROR SO_ERROR
#define Sleep sleep
#define ioctlsocket ioctl
#define closesocket close
#define GETSOCKETERRNO() (errno)

#endif

DWORD startTime = 0;

int xSemaphoreTake(int semaphore, int blockTime) {
	return 1;
}
int xSemaphoreCreateMutex() {
	return 0;
}
int xSemaphoreGive(int semaphore) {
	return 0;
}
extern int g_selfTestsMode;
int rtos_delay_milliseconds(int sec) {
	if (g_selfTestsMode)
		return;
	Sleep(sec);
	return 0;
}
int delay_ms(int sec) {
	if (g_selfTestsMode)
		return;
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
	return closesocket(socket);
}
int lwip_close_force(int socket) {
	return closesocket(socket);
}
int hal_machw_time() {
	DWORD timeNow = timeGetTime();
	if (startTime == 0)
		startTime = timeNow;
	DWORD ret = timeNow - startTime;
	return ret;
}
int hal_machw_time_past(int tt) {
	int t = hal_machw_time();
	if (t < tt)
		return 0;
	return 1;
}

#ifndef LINUX
int rtos_create_thread(int *out, int prio, const char *name, LPTHREAD_START_ROUTINE function, int stackSize, void *arg) {
	int handle;
	handle = CreateThread(NULL, 0, function, arg, 0, NULL);
	return 0;
}
#else
int rtos_create_thread(int *out, int prio, const char *name, void *function, int stackSize, void *arg) {
	pthread_t handle;
	pthread_create(&handle, NULL, function, (arg) != 0);
	return 0;
}
#endif
void rtos_delete_thread(int i) {
	
}
int lwip_fcntl(int s, int cmd, int val) {
	int argp;

	argp = 1;
	if (ioctlsocket(s,
		FIONBIO,
		&argp) == SOCKET_ERROR)
	{
		printf("ioctlsocket() error %d\n", GETSOCKETERRNO());
		return 1;
	}

	return 0;
}

#endif
