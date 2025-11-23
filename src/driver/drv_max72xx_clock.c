#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_deviceclock.h"
#include <time.h>

char *my_strcat(char *p, const char *s) {
	strcat(p, s);
	return p + strlen(s);
}
char *add_padded(char *o, int i) {
	if (i < 10) {
		*o = '0';
		o++;
	}
	sprintf(o, "%i", i);
	return o + strlen(o);
}
enum {
	CLOCK_DATE,
	CLOCK_TIME,
	CLOCK_HUMIDITY,
	CLOCK_TEMPERATURE,
};
void Clock_Send(int type) {
	char time[64];
	struct tm *ltm;
	float val;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)Clock_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&ntpTime);

	if (ltm == 0) {
		return;
	}
	time[0] = 0;
	p = time;
	if (type == CLOCK_TIME) {
		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_hour);
		p = my_strcat(p, ":");
		p = add_padded(p, ltm->tm_min);
		strcat(p, "   ");
	}
	else if (type == CLOCK_DATE) {
		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_mday);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon + 1);
		//p = my_strcat(p, ".");
		//p = add_padded(p, ltm->tm_year);
		strcat(p, "   ");
	}
	else if (type == CLOCK_HUMIDITY) {
		if (false==CHANNEL_GetGenericHumidity(&val)) {
			// failed - exit early, do not change string
			return;
		}
		sprintf(time, "H: %i%%   ", (int)val);
	} 
	else if (type == CLOCK_TEMPERATURE) {
		if (false == CHANNEL_GetGenericTemperature(&val)) {
			// failed - exit early, do not change string
			return;
		}
		sprintf(time, "T: %iC    ", (int)val);
	}

	CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
}
void Clock_SendTime() {
	Clock_Send(CLOCK_TIME);
}
void Clock_SendDate() {
	Clock_Send(CLOCK_DATE);
}
void Clock_SendHumidity() {
	Clock_Send(CLOCK_HUMIDITY);
}
void Clock_SendTemperature() {
	Clock_Send(CLOCK_TEMPERATURE);
}
static unsigned int cycle = 0;

void Run_NoAnimation() {
	cycle+=4;
	if (cycle < 10) {
		Clock_SendDate();
	}
	else if(cycle < 20) {
		Clock_SendTime();
	}
	else if (cycle < 30) {
		Clock_SendHumidity();
	}
	else {
		Clock_SendTemperature();
	}
	CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
	cycle %= 40;
}
static int g_del = 0;
void Run_Animated() {
	cycle++;
	if (cycle < 4) {
		return;
	}
	cycle = 0;
	char time[64];
	struct tm *ltm;
	char *p;
	time_t ntpTime;

	ntpTime=(time_t)Clock_GetCurrentTime();
	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = gmtime(&ntpTime);

	if (ltm == 0) {
		return;
	}
	int scroll = MAX72XXSingle_GetScrollCount();
	//scroll_cycle = 0;
	if (scroll == 0 && g_del == 0) {
		time[0] = 0;
		p = time;
		p = my_strcat(p, "  ");

		p = add_padded(p, ltm->tm_hour);
		p = my_strcat(p, ":");
		p = add_padded(p, ltm->tm_min);
		strcat(p, " ");

		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_year+1900);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon + 1);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mday);
		strcat(p, "   ");

		CMD_ExecuteCommandArgs("MAX72XX_Clear", NULL, 0);
		CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
		CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
		g_del = 10;
	}
	if (g_del > 0) {
		g_del--;
		return;
	}
	CMD_ExecuteCommandArgs("MAX72XX_Scroll", "-1", 0);
	CMD_ExecuteCommandArgs("MAX72XX_refresh", "", 0);
}
bool g_animated = false;
void DRV_MAX72XX_Clock_OnEverySecond() {
	if (g_animated == false) {
		Run_NoAnimation();
	}
}
void DRV_MAX72XX_Clock_RunFrame() {
	if (g_animated) {
		Run_Animated();
	}
}
/*
Config for my clock with IR

startDriver MAX72XX
MAX72XX_Setup 0 1 26
startDriver NTP
startDriver MAX72XX_Clock
ntp_timeZoneOfs 2

alias my_ledStrip_off SendGet http://192.168.0.200/cm?cmnd=POWER%20OFF
alias my_ledStrip_plus SendGet http://192.168.0.200/cm?cmnd=backlog%20POWER%20ON;add_dimmer%2010
alias my_ledStrip_minus SendGet http://192.168.0.200/cm?cmnd=backlog%20POWER%20ON;add_dimmer%20-10
alias main_switch_off SendGet http://192.168.0.159/cm?cmnd=POWER%20OFF
alias main_switch_on SendGet http://192.168.0.159/cm?cmnd=POWER%20ON

alias my_send_style_cold SendGet http://192.168.0.210/cm?cmnd=backlog%20CT%20153;%20POWER%20ON
alias my_send_style_warm SendGet http://192.168.0.210/cm?cmnd=backlog%20CT%20500%20POWER%20ON
alias my_send_style_red SendGet http://192.168.0.210/cm?cmnd=backlog%20color%20FF0000%20POWER%20ON
alias my_send_style_green SendGet http://192.168.0.210/cm?cmnd=backlog%20color%2000FF00%20POWER%20ON
alias my_send_style_blue SendGet http://192.168.0.210/cm?cmnd=backlog%20color%200000FF%20POWER%20ON
alias my_send_style_plus SendGet http://192.168.0.210/cm?cmnd=backlog%20POWER%20ON;add_dimmer%2010
alias my_send_style_minus SendGet http://192.168.0.210/cm?cmnd=backlog%20POWER%20ON;add_dimmer%20-10

// 1
addEventHandler2 IR_Samsung 0x707 0x4 my_ledStrip_off
// 2
addEventHandler2 IR_Samsung 0x707 0x5 my_ledStrip_plus
// 3
addEventHandler2 IR_Samsung 0x707 0x6 my_ledStrip_minus
// 4
addEventHandler2 IR_Samsung 0x707 0x8 main_switch_off
// 5
addEventHandler2 IR_Samsung 0x707 0x9 main_switch_on
// 6
addEventHandler2 IR_Samsung 0x707 0xA startScript autoexec.bat next_light_style
// 7
addEventHandler2 IR_Samsung 0x707 0xC my_send_style_plus
// 8
addEventHandler2 IR_Samsung 0x707 0xD my_send_style_minus

// stop autoexec execution here
return

// this is function for scrolling light styles
next_light_style:

// add to ch 10, wrap within range [0,5]
addChannel 10 1 0 5 1

// start main switch
//main_switch_on

if $CH10==0 then my_send_style_cold
if $CH10==1 then my_send_style_warm
if $CH10==2 then my_send_style_red
if $CH10==3 then my_send_style_green
if $CH10==4 then my_send_style_blue






*/
static commandResult_t DRV_MAX72XX_Clock_Animate(const void *context, const char *cmd, const char *args, int flags) {


	Tokenizer_TokenizeString(args, 0);

	g_animated = Tokenizer_GetArgInteger(0);


	return CMD_RES_OK;
}
void DRV_MAX72XX_Clock_Init() {

	CMD_RegisterCommand("MAX72XXClock_Animate", DRV_MAX72XX_Clock_Animate, NULL);
}





