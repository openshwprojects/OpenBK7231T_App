#include "../obk_config.h"

#if ENABLE_DRIVER_BKAUDIO

#include "../new_common.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "drv_local.h"

#include "include.h"
#include "drv_model_pub.h"
#include "audio_pub.h"

#define BKAUDIO_RING_LEN   (2048)
#define BKAUDIO_CHUNK_LEN  (512)
#define BKAUDIO_RATE       (8000)
#define BKAUDIO_KEEP       (32)

static UINT8 g_audRing[BKAUDIO_RING_LEN] __attribute__((aligned(4)));
static UINT8 g_audChunk[BKAUDIO_CHUNK_LEN] __attribute__((aligned(4)));
static short g_audKeep[BKAUDIO_KEEP];
static DD_HANDLE g_audHdl = DD_HANDLE_UNVALID;

static UINT32 g_audCbCount;
static UINT32 g_audTicks;
static UINT32 g_audReadFail;
static UINT32 g_audOverflow;

static long long g_accSq;
static UINT32 g_accN;
static int g_accPeak;
static int g_accMin;
static int g_accMax;

static int g_rateSeen;
static int g_rms;
static int g_peak;
static int g_minSeen;
static int g_maxSeen;
static int g_peakHold;

static void BKAudio_RxCallback(UINT32 fill_size)
{
	g_audCbCount++;
}

static int BKAudio_Isqrt(long long v)
{
	int r = 0, bit;

	for (bit = 1 << 15; bit; bit >>= 1) {
		int t = r | bit;
		if ((long long)t * t <= v)
			r = t;
	}
	return r;
}

void BKAudio_RunQuickTick(void)
{
	UINT32 fill, take, i;
	UINT32 got;
	int kept = 0;

	if (g_audHdl == DD_HANDLE_UNVALID)
		return;

	g_audTicks++;

	fill = ddev_control(g_audHdl, AUD_ADC_CMD_GET_FILL_BUF_SIZE, NULL);
	if (fill >= BKAUDIO_RING_LEN - 4)
		g_audOverflow++;

	while (fill >= 2) {
		take = (fill > BKAUDIO_CHUNK_LEN) ? BKAUDIO_CHUNK_LEN : (fill & ~1u);
		if (take == 0)
			break;
		got = ddev_read(g_audHdl, (char *)g_audChunk, take, 0);
		if (got == 0) {
			g_audReadFail++;
			break;
		}
		if (got > take)
			got = take;
		for (i = 0; i + 1 < got; i += 2) {
			int s = (int)(short)(g_audChunk[i] | (g_audChunk[i + 1] << 8));
			int a = (s < 0) ? -s : s;

			g_accSq += (long long)s * s;
			if (a > g_accPeak)
				g_accPeak = a;
			if (s < g_accMin)
				g_accMin = s;
			if (s > g_accMax)
				g_accMax = s;
			if (kept < BKAUDIO_KEEP)
				g_audKeep[kept++] = (short)s;
			g_accN++;
		}
		fill = ddev_control(g_audHdl, AUD_ADC_CMD_GET_FILL_BUF_SIZE, NULL);
	}
}

void BKAudio_OnEverySecond(void)
{
	if (g_accN > 0) {
		g_rms = BKAudio_Isqrt(g_accSq / g_accN);
		g_peak = g_accPeak;
		g_minSeen = g_accMin;
		g_maxSeen = g_accMax;
		g_rateSeen = (int)g_accN;
		if (g_accPeak > g_peakHold)
			g_peakHold = g_accPeak;
	} else {
		g_rateSeen = 0;
	}
	g_accSq = 0;
	g_accN = 0;
	g_accPeak = 0;
	g_accMin = 32767;
	g_accMax = -32768;
}

void BKAudio_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	if (bPreState)
		return;

	if (g_audHdl == DD_HANDLE_UNVALID) {
		hprintf255(request, "<h5>BKAudio: microphone not open</h5>");
		return;
	}
	hprintf255(request, "<h5>BKAudio: rms %i peak %i hold %i, min %i max %i</h5>",
	           g_rms, g_peak, g_peakHold, g_minSeen, g_maxSeen);
	hprintf255(request, "<h5>BKAudio: %i samples/s of %i, ticks %u, cb %u, readfail %u, ovf %u</h5>",
	           g_rateSeen, BKAUDIO_RATE, g_audTicks, g_audCbCount, g_audReadFail, g_audOverflow);
}

static commandResult_t CMD_BKAudioInfo(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioInfo: rms %i peak %i hold %i min %i max %i, %i sps, ticks %u cb %u fail %u ovf %u",
	            g_rms, g_peak, g_peakHold, g_minSeen, g_maxSeen, g_rateSeen,
	            g_audTicks, g_audCbCount, g_audReadFail, g_audOverflow);
	g_peakHold = 0;
	return CMD_RES_OK;
}

static commandResult_t CMD_BKAudioDump(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
	char line[200];
	int i, n = 0;

	line[0] = 0;
	for (i = 0; i < 16; i++)
		n += snprintf(line + n, sizeof(line) - n, "%i ", (int)g_audKeep[i]);
	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioDump: %s", line);
	return CMD_RES_OK;
}

static commandResult_t CMD_BKAudioGain(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
	UINT32 gain;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	gain = (UINT32)Tokenizer_GetArgInteger(0);
	if (gain > AUD_ADC_MAX_VOLUME)
		return CMD_RES_BAD_ARGUMENT;
	if (g_audHdl == DD_HANDLE_UNVALID)
		return CMD_RES_ERROR;
	ddev_control(g_audHdl, AUD_ADC_CMD_SET_VOLUME, &gain);
	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioGain: %u", gain);
	return CMD_RES_OK;
}

void BKAudio_StopDriver(void)
{
	if (g_audHdl != DD_HANDLE_UNVALID) {
		ddev_control(g_audHdl, AUD_ADC_CMD_PAUSE, NULL);
		ddev_close(g_audHdl);
		g_audHdl = DD_HANDLE_UNVALID;
	}
}

static commandResult_t CMD_BKAudioOpen(const void *context, const char *cmd,
                                       const char *args, int cmdFlags)
{
	AUD_ADC_CFG_ST cfg;
	UINT32 status;
	int mode = 0;
	int rate = BKAUDIO_RATE;

	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() >= 1)
		mode = Tokenizer_GetArgInteger(0);
	if (Tokenizer_GetArgsCount() >= 2)
		rate = Tokenizer_GetArgInteger(1);

	if (g_audHdl != DD_HANDLE_UNVALID) {
		ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioOpen: already open");
		return CMD_RES_OK;
	}

	os_memset(&cfg, 0, sizeof(cfg));
	cfg.buf = g_audRing;
	cfg.buf_len = BKAUDIO_RING_LEN;
	cfg.inter_thre = AUD_ADC_DEF_WR_THRED;
	cfg.freq = (UINT16)rate;
	cfg.channels = 1;
	cfg.mode = (UINT8)mode;
	cfg.linein_detect_pin = 0;
	cfg.rx_cb = BKAudio_RxCallback;

	g_audCbCount = 0;
	g_audTicks = 0;
	g_audReadFail = 0;
	g_audOverflow = 0;
	g_peakHold = 0;
	g_accMin = 32767;
	g_accMax = -32768;

	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioOpen: mode 0x%02X rate %i", mode, rate);
	g_audHdl = ddev_open(AUD_ADC_DEV_NAME, &status, (UINT32)&cfg);
	if (g_audHdl == DD_HANDLE_UNVALID) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "BKAudioOpen: failed, status %u", status);
		return CMD_RES_ERROR;
	}
	ddev_control(g_audHdl, AUD_ADC_CMD_PLAY, NULL);
	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioOpen: capturing");
	return CMD_RES_OK;
}

static commandResult_t CMD_BKAudioClose(const void *context, const char *cmd,
                                        const char *args, int cmdFlags)
{
	BKAudio_StopDriver();
	ADDLOG_INFO(LOG_FEATURE_CMD, "BKAudioClose: closed");
	return CMD_RES_OK;
}

void BKAudio_Init(void)
{
	g_audHdl = DD_HANDLE_UNVALID;
	g_accMin = 32767;
	g_accMax = -32768;

	//cmddetail:{"name":"BKAudioInfo","args":"",
	//cmddetail:"descr":"Prints capture statistics and clears the held peak.",
	//cmddetail:"fn":"CMD_BKAudioInfo","file":"driver/drv_audio.c","requires":"",
	//cmddetail:"examples":"BKAudioInfo"}
	CMD_RegisterCommand("BKAudioInfo", CMD_BKAudioInfo, NULL);
	//cmddetail:{"name":"BKAudioDump","args":"",
	//cmddetail:"descr":"Prints the sixteen most recent raw samples.",
	//cmddetail:"fn":"CMD_BKAudioDump","file":"driver/drv_audio.c","requires":"",
	//cmddetail:"examples":"BKAudioDump"}
	CMD_RegisterCommand("BKAudioDump", CMD_BKAudioDump, NULL);
	//cmddetail:{"name":"BKAudioGain","args":"[Gain]",
	//cmddetail:"descr":"Sets the microphone input gain, 0 to 124.",
	//cmddetail:"fn":"CMD_BKAudioGain","file":"driver/drv_audio.c","requires":"",
	//cmddetail:"examples":"BKAudioGain 45"}
	CMD_RegisterCommand("BKAudioGain", CMD_BKAudioGain, NULL);
	//cmddetail:{"name":"BKAudioOpen","args":"[ModeBits][Rate]",
	//cmddetail:"descr":"Opens the microphone. Mode bit one selects DMA, bit two selects the line input. Rate defaults to 8000.",
	//cmddetail:"fn":"CMD_BKAudioOpen","file":"driver/drv_audio.c","requires":"",
	//cmddetail:"examples":"BKAudioOpen 0 8000"}
	CMD_RegisterCommand("BKAudioOpen", CMD_BKAudioOpen, NULL);
	//cmddetail:{"name":"BKAudioClose","args":"",
	//cmddetail:"descr":"Stops capture and closes the microphone.",
	//cmddetail:"fn":"CMD_BKAudioClose","file":"driver/drv_audio.c","requires":"",
	//cmddetail:"examples":"BKAudioClose"}
	CMD_RegisterCommand("BKAudioClose", CMD_BKAudioClose, NULL);
}

#endif
