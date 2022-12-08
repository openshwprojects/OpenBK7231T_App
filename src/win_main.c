#ifdef WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "new_common.h"
#include "driver\drv_public.h"
#include "httpserver\new_http.h"
#include "new_pins.h"
#include <timeapi.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "Winmm.lib")

int accum_time = 0;
int win_frameNum = 0;
#define DEFAULT_FRAME_TIME 5


void strcat_safe_test(){
	char tmpA[16];
	char tmpB[16];
	char buff[128];
	char timeStrA[128];
	char timeStrB[128];
	char timeStrC[128];
	char timeStrD[128];
	char timeStrE[128];
	char timeStrF[128];
	char timeStrG[128];
	char timeStrH[128];
	int res0, res1, res2, res3, res4, res5;
	tmpA[0] = 0;
	res0 = strcat_safe(tmpA,"Test1",sizeof(tmpA));
	res1 = strcat_safe(tmpA," ",sizeof(tmpA));
	res2 = strcat_safe(tmpA,"is now processing",sizeof(tmpA));
	res3 = strcat_safe(tmpA," very long string",sizeof(tmpA));
	res4 = strcat_safe(tmpA," and it",sizeof(tmpA));
	res5 = strcat_safe(tmpA," and it",sizeof(tmpA));
	tmpB[0] = 0;
	res0 = strcat_safe(tmpB,"Test1",sizeof(tmpB));
	res1 = strcat_safe(tmpB," ",sizeof(tmpB));
	res2 = strcat_safe(tmpB,"is now processing",sizeof(tmpB));
	res3 = strcat_safe(tmpB," very long string",sizeof(tmpB));
	res4 = strcat_safe(tmpB," and it",sizeof(tmpB));
	res5 = strcat_safe(tmpB," and it",sizeof(tmpB));

	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));
	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));


}

void Sim_RunFrame(int frameTime) {
	//printf("Sim_RunFrame: frametime %i\n", frameTime);
	win_frameNum++;
	accum_time += frameTime;
	PIN_ticks(0);
	HTTPServer_RunQuickTick();
	if (accum_time > 1000) {
		accum_time -= 1000;
		Main_OnEverySecond();
	}
}
void Sim_RunMiliseconds(int ms, bool bApplyRealtimeWait) {
	while (ms > 0) {
		if (bApplyRealtimeWait) {
			Sleep(DEFAULT_FRAME_TIME);
		}
		Sim_RunFrame(DEFAULT_FRAME_TIME);
		ms -= DEFAULT_FRAME_TIME;
	}
}
void Sim_RunSeconds(float f, bool bApplyRealtimeWait) {
	int ms = f * 1000;
	Sim_RunMiliseconds(ms, bApplyRealtimeWait);
}
void Sim_RunFrames(int n, bool bApplyRealtimeWait) {
	int i;

	for (i = 0; i < n; i++) {
		if (bApplyRealtimeWait) {
			Sleep(DEFAULT_FRAME_TIME);
		}
		Sim_RunFrame(DEFAULT_FRAME_TIME);
	}
}

void Win_DoUnitTests() {
	//CFG_ClearPins();


	Test_HTTP_Client();
	Test_ExpandConstant();
	Test_ChangeHandlers();
	Test_RepeatingEvents();
	Test_ButtonEvents();
	Test_Commands_Alias();
	Test_Expressions_RunTests_Basic();
	Test_LEDDriver();
	Test_LFS();
	Test_Scripting();
	Test_Commands_Channels();
	Test_Command_If();
	Test_Command_If_Else(); 
	Test_Tokenizer();
	Test_Http();

	// this is slowest
	Test_TuyaMCU_Basic();
}
bool bObkStarted = false;
void SIM_Hack_ClearSimulatedPinRoles();

void SIM_ClearOBK() {
	if (bObkStarted) {
		DRV_ShutdownAllDrivers();
		SIM_Hack_ClearSimulatedPinRoles();
		CMD_ExecuteCommand("clearAll", 0);
	}
}
void SIM_DoFreshOBKBoot() {
	bObkStarted = true;
	Main_Init();
}
long start_time = 0;
bool bStartTimeSet = false;
long SIM_GetTime() {
	long cur = timeGetTime();
	if (bStartTimeSet == false) {
		start_time = cur;
		bStartTimeSet = true;
	}
	return cur - start_time;
}
#include "sim/sim_public.h"
int __cdecl main(int argc, char **argv)
{
	bool bWantsUnitTests = 1;
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }


	if (bWantsUnitTests) {
		SIM_DoFreshOBKBoot();
		// let things warm up a little
		Sim_RunFrames(50, false);
		// run tests
		Win_DoUnitTests();
		Sim_RunFrames(50, false);
	}

	SIM_CreateWindow(argc, argv);
	if (false) {
		while (1) {
			Sleep(DEFAULT_FRAME_TIME);
			Sim_RunFrame(DEFAULT_FRAME_TIME);
			SIM_RunWindow();
		}
	}
	else {
		long prev_time = SIM_GetTime();
		while (1) {
			long cur_time = SIM_GetTime();
			long delta = cur_time - prev_time;
			if (delta <= 0)
				continue;
			Sim_RunFrame(delta);
			SIM_RunWindow();
			prev_time = cur_time;
		}
	}
	return 0;
}

// initialise OTA flash starting at startaddr
int init_ota(unsigned int startaddr) {
	return 0;
}

// add any length of data to OTA
void add_otadata(unsigned char *data, int len) {
	return;
}

// finalise OTA flash (write last sector if incomplete)
void close_ota() {
	return;
}

void otarequest(const char *urlin) {
	return;
}

int ota_progress();
int ota_total_bytes();
#endif

