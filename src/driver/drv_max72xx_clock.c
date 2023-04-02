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
#include "drv_ntp.h"
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
	CLOCK_TIME
};
void Clock_Send(int type) {
	char time[64];
	struct tm *ltm;
	char *p;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = localtime((time_t*)&g_ntpTime);

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
	else {
		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_mday);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon+1);
		//p = my_strcat(p, ".");
		//p = add_padded(p, ltm->tm_year);
		strcat(p, "   ");
	}

	CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
}
void Clock_SendTime() {
	Clock_Send(CLOCK_TIME);
}
void Clock_SendDate() {
	Clock_Send(CLOCK_DATE);
}
static int cycle = 0;
void Run_NoAnimation() {
	cycle++;
	if (cycle < 10) {
		Clock_SendDate();
	}
	else {
		Clock_SendTime();
	}
	cycle %= 20;
}
void Run_Animated() {
	char time[64];
	struct tm *ltm;
	char *p;

	// NOTE: on windows, you need _USE_32BIT_TIME_T 
	ltm = localtime((time_t*)&g_ntpTime);

	if (ltm == 0) {
		return;
	}
	//scroll_cycle = 0;
	if (ltm->tm_sec == 0) {
		time[0] = 0;
		p = time;

		p = add_padded(p, ltm->tm_hour);
		p = my_strcat(p, ":");
		p = add_padded(p, ltm->tm_min);
		strcat(p, " ");

		p = my_strcat(p, " ");
		p = add_padded(p, ltm->tm_year);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mon + 1);
		p = my_strcat(p, ".");
		p = add_padded(p, ltm->tm_mday);
		strcat(p, "");

		CMD_ExecuteCommandArgs("MAX72XX_Print", time, 0);
	}
	else {
		CMD_ExecuteCommandArgs("MAX72XX_Scroll", "1", 0);
	}
}
void DRV_MAX72XX_Clock_OnEverySecond() {
	Run_NoAnimation();
}
void DRV_MAX72XX_Clock_RunFrame() {

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
void DRV_MAX72XX_Clock_Init() {


}





