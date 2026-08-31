#include "../obk_config.h"

#if ENABLE_DRIVER_RTC

#include "../new_common.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_ntp.h"

#include "rtos_pub.h"

#define RTC_REBASE_THRESHOLD_S 5

static unsigned long long g_rtcBaseUs;
static unsigned int g_rtcEpochBase;
static int g_rtcHasBase;
static unsigned int g_rtcRebases;
static int g_rtcLastDelta;

unsigned long long RTC_GetUptimeUs(void)
{
	return (unsigned long long)rtos_get_time_us();
}

unsigned int RTC_GetUptimeSeconds(void)
{
	return (unsigned int)(RTC_GetUptimeUs() / 1000000ull);
}

int RTC_HasBase(void)
{
	return g_rtcHasBase;
}

unsigned int RTC_GetEpoch(void)
{
	if (!g_rtcHasBase)
		return 0;
	return g_rtcEpochBase + (unsigned int)((RTC_GetUptimeUs() - g_rtcBaseUs) / 1000000ull);
}

static void RTC_SetBase(unsigned int epoch)
{
	g_rtcBaseUs = RTC_GetUptimeUs();
	g_rtcEpochBase = epoch;
	g_rtcHasBase = 1;
}

void RTC_OnEverySecond(void)
{
	unsigned int ntp;

	if (!NTP_IsTimeSynced())
		return;

	ntp = NTP_GetCurrentTime();
	if (ntp == 0)
		return;

	if (!g_rtcHasBase) {
		RTC_SetBase(ntp);
		ADDLOG_INFO(LOG_FEATURE_DRV, "RTC: base set to %u", ntp);
		return;
	}

	g_rtcLastDelta = (int)(ntp - RTC_GetEpoch());
	if (g_rtcLastDelta >= RTC_REBASE_THRESHOLD_S || g_rtcLastDelta <= -RTC_REBASE_THRESHOLD_S) {
		RTC_SetBase(ntp);
		g_rtcRebases++;
		ADDLOG_WARN(LOG_FEATURE_DRV, "RTC: rebased on %u, drift was %i s", ntp, g_rtcLastDelta);
	}
}

void RTC_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	unsigned int up;

	if (bPreState)
		return;

	up = RTC_GetUptimeSeconds();
	if (g_rtcHasBase) {
		hprintf255(request, "<h5>RTC: hardware uptime %us, epoch %u, drift %is, rebases %u</h5>",
		           up, RTC_GetEpoch(), g_rtcLastDelta, g_rtcRebases);
	} else {
		hprintf255(request, "<h5>RTC: hardware uptime %us, no time base yet</h5>", up);
	}
}

static commandResult_t CMD_RTCTime(const void *context, const char *cmd,
                                   const char *args, int cmdFlags)
{
	unsigned long long us = RTC_GetUptimeUs();

	ADDLOG_INFO(LOG_FEATURE_CMD, "RTCTime: uptime %u.%06u s, epoch %u, base %s, drift %i s, rebases %u",
	            (unsigned int)(us / 1000000ull), (unsigned int)(us % 1000000ull),
	            RTC_GetEpoch(), g_rtcHasBase ? "set" : "none", g_rtcLastDelta, g_rtcRebases);
	return CMD_RES_OK;
}

void RTC_Init(void)
{
	g_rtcHasBase = 0;
	g_rtcRebases = 0;
	g_rtcLastDelta = 0;

	//cmddetail:{"name":"RTCTime","args":"",
	//cmddetail:"descr":"Prints the hardware RTC uptime, the derived wall clock, and how far the software clock has drifted from it.",
	//cmddetail:"fn":"CMD_RTCTime","file":"driver/drv_rtc.c","requires":"",
	//cmddetail:"examples":"RTCTime"}
	CMD_RegisterCommand("RTCTime", CMD_RTCTime, NULL);
}

#endif
