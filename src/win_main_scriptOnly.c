#ifdef WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "new_common.h"
#include "cmnds/cmd_public.h"
#include "cmnds/cmd_local.h"

int CHANNEL_Get(int ch) {
	return ch;
}

int Main_HasMQTTConnected() {
	return 1;
}

byte *LFS_ReadFile(const char *fname) {
	FILE *f;
	int len;
	byte *ret;

	f = fopen(fname,"rb");
	if(f == 0)
		return 0;
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);

	ret = malloc(len + 1);
	fread(ret,1,len,f);
	ret[len] = 0;

	return ret;
}
void CMD_StartTCPCommandLine() {

}
bool CFG_HasFlag(int flag) {
	return false;
}
int LED_GetEnableAll() {
	return 0;
}
int LED_GetDimmer() {
	return 0;
}
void CFG_Save_IfThereArePendingChanges() {

}
void CFG_SetDefaultConfig() {

}
void RESET_ScheduleModuleReset(int delSeconds){ 

}


void addLogAdv(int level, int feature, const char* fmt, ...);
int __cdecl main(void)
{
	bool energyCounterStatsJSONEnable = false;
	long energyCounterMinutesIndex = 123;
    struct tm *ltm;
	time_t ConsumptionResetTime;
	int i;
	unsigned int max_scheme_len = 12345;
	unsigned int some_uint_32 = 5667;

	ConsumptionResetTime = time(0);
          
	
	ltm = gmtime(&ConsumptionResetTime);

	addLogAdv(1,1,"Scheme str is too small (%u >= %u)", max_scheme_len, some_uint_32);
	addLogAdv(1,1,"Simon test %f!",1.0f/3.0f);
	addLogAdv(1,1,"Test %i, %f!",2010,3.14f);
    addLogAdv(1,1, "History Index: %ld<br>JSON Stats: %s <br>", energyCounterMinutesIndex,
                    (energyCounterStatsJSONEnable == true) ? "enabled" : "disabled");

	    //ADDLOGF_INFO("mqtt req %i/%i, free mem %i\n", mqtt_cur,mqtt_max,mqtt_mem);
	  addLogAdv(1,1, "%sTime %i, idle %i/s, free %d, MQTT %i(%i), bWifi %i, secondsWithNoPing %i, socks %i/%i\n",
		  "[TEST]", 123, 555,95000,1, 6, 
            1, -1, 3, 64);
    printf("Consumption Reset Time: %04d-%02d-%02d %02d:%02d:%02d",
                       ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    addLogAdv(1,1, "Consumption Reset Time: %04d-%02d-%02d %02d:%02d:%02d",
                       ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

            addLogAdv(1,1, "Today: %1.1f Wh DailyStats: [", 3.145f);
            for(i = 1; i < 16; i++)
            {
                if (i==1)
                    addLogAdv(1,1, "%1.1f", 3.145f);
                else
                    addLogAdv(1,1,  ",%1.1f", 3.145f);
            }
	SVM_StartScript("testScripts/testGoto.txt",0,0);
	while(1) {
		SVM_RunThreads(5);
	}
	system("pause");
    return 0;
}

#endif

