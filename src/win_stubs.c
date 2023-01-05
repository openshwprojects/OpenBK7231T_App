// stubs so I can compile HTTP server on Windows for testing and faster development
#include "new_common.h"

#if WINDOWS

void CMD_StartTCPCommandLine() {

}

void Main_SetupPingWatchDog(const char *target/*, int delayBetweenPings_Seconds*/) {

}
int PingWatchDog_GetTotalLost() {
	return 0;
}
int PingWatchDog_GetTotalReceived() {
	return 0;
}


// placeholder - TODO
char myIP[] = "127.0.0.1";
char *getMyIp() {
	return myIP;
}
void __asm__(const char *s) {

}

#endif