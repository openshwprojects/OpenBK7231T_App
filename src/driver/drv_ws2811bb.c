// Bit-banged single-wire driver for WS2811/WS2812 style strips on Beken BK7231N.
//
// The data line is driven by writing the GPIO config register directly,
// with interrupts disabled for the duration of one frame. It plugs into the
// shared LED layer (drv_leds_shared), so SM16703P_Init / SM16703P_SetPixel /
// SM16703P_Start and the DDP driver all work on top of it.
//
// Usage:
//   set the pin role UCS1912_DIN on the data pin (P0 on the Action HDMI sync box,
//   where P8 must additionally be held low with the pin role AlwaysLow)
//   backlog startDriver WS2811BB; SM16703P_Init 28 GRB
//   SM16703P_SetPixel all 255 0 0
//   SM16703P_Start
//
// Bit timing is expressed in loop iterations (about 4 CPU cycles each):
//   WS2811BB_Timing n0h n0l n1h n1l

#include "../new_cfg.h"
#include "../new_common.h"
#include "../new_pins.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "../logging/logging.h"

#if ENABLE_DRIVER_WS2811BB

#include <include.h>
#include <arm_arch.h>

#include "drv_leds_shared.h"
#include "drv_local.h"

#define WS2811BB_GPIO_CFG_BASE	0x00802800
#define WS2811BB_OUTPUT_BIT		(1 << 1)
#define WS2811BB_MAX_BYTES		1024
// the whole frame is sent with interrupts disabled, keep it short
#define WS2811BB_MAX_FRAME_BYTES	450

static int g_pin = -1;
static volatile uint32_t *g_reg = 0;
static uint32_t g_hi = 0;
static uint32_t g_lo = 0;
static byte g_buf[WS2811BB_MAX_BYTES];
static int g_bytes = 0;

// loop iterations for: '0' high, '0' low, '1' high, '1' low
static int g_n0h = 9;
static int g_n0l = 22;
static int g_n1h = 22;
static int g_n1l = 10;
// the data line goes through an inverting gate on the Action sync box:
// idle is high and pulses are low on the pin when g_invert is 1
static int g_invert = 1;

static inline __attribute__((always_inline)) void WS2811BB_Delay(int n) {
	if (n <= 0) {
		return;
	}
	// this file is built in Thumb mode, where "subs rd, rs, #imm" needs unified syntax
	__asm__ volatile(
		".syntax unified\n"
		"1:\n"
		"subs %0, %0, #1\n"
		"bne 1b\n"
		".syntax divided\n"
		: "+l"(n)
		:
		: "cc");
}

typedef struct ws2811bb_job_s {
	volatile uint32_t *reg;
	uint32_t pulse;		// register value while a pulse is active
	uint32_t idle;		// register value while idle / reset
	const byte *buf;
	int bytes;
	int n0h, n0l, n1h, n1l;
} ws2811bb_job_t;

// Time critical part. It lives in ITCM (see the linker script), which has constant access time,
// so instruction fetch time is constant (code running from flash gets random wait
// states that ruin the pulse widths). It must not call any other function.
static void __attribute__((noinline, section(".text.WS2811BB_SendRam"))) WS2811BB_SendRam(const ws2811bb_job_t *job) {
	int i;
	int mask;
	byte b;
	int n0h = job->n0h, n0l = job->n0l, n1h = job->n1h, n1l = job->n1l;
	volatile uint32_t *reg = job->reg;
	uint32_t hi = job->pulse;
	uint32_t lo = job->idle;
	const byte *buf = job->buf;
	int bytes = job->bytes;

	for (i = 0; i < bytes; i++) {
		b = buf[i];
		for (mask = 0x80; mask != 0; mask >>= 1) {
			if (b & mask) {
				*reg = hi;
				WS2811BB_Delay(n1h);
				*reg = lo;
				WS2811BB_Delay(n1l);
			} else {
				*reg = hi;
				WS2811BB_Delay(n0h);
				*reg = lo;
				WS2811BB_Delay(n0l);
			}
		}
	}
}

// called through a pointer so the compiler uses a register call (RAM is far from flash)
static void (*volatile g_sendRam)(const ws2811bb_job_t *job) = WS2811BB_SendRam;

static void __attribute__((noinline)) WS2811BB_SendEx(int irqOff) {
	ws2811bb_job_t job;
	GLOBAL_INT_DECLARATION();

	if (g_reg == 0 || g_bytes <= 0 || g_bytes > WS2811BB_MAX_FRAME_BYTES) {
		return;
	}
	job.reg = g_reg;
	job.pulse = g_invert ? g_lo : g_hi;
	job.idle = g_invert ? g_hi : g_lo;
	job.buf = g_buf;
	job.bytes = g_bytes;
	job.n0h = g_n0h;
	job.n0l = g_n0l;
	job.n1h = g_n1h;
	job.n1l = g_n1l;

	if (irqOff) {
		GLOBAL_INT_DISABLE();
	}
	g_sendRam(&job);
	// line stays at the idle level afterwards, which is the reset / latch state
	if (irqOff) {
		GLOBAL_INT_RESTORE();
	}
}

static void WS2811BB_Send(void) {
	WS2811BB_SendEx(1);
}

static byte WS2811BB_GetByte(uint32_t idx) {
	if ((int)idx >= g_bytes) {
		return 0;
	}
	return g_buf[idx];
}

static void WS2811BB_SetByte(uint32_t idx, byte val) {
	if ((int)idx >= g_bytes) {
		return;
	}
	g_buf[idx] = val;
}

static void WS2811BB_Apply(void) {
	WS2811BB_Send();
}

static void WS2811BB_SetLEDCount(int pixel_count, int pixel_size) {
	int bytes = pixel_count * pixel_size;
	if (bytes > WS2811BB_MAX_FRAME_BYTES) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "WS2811BB: %i bytes is too long, limit is %i", bytes, WS2811BB_MAX_FRAME_BYTES);
		bytes = WS2811BB_MAX_FRAME_BYTES;
	}
	memset(g_buf, 0, sizeof(g_buf));
	g_bytes = bytes;
}

static commandResult_t WS2811BB_CMD_Timing(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 4) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_n0h = Tokenizer_GetArgIntegerRange(0, 0, 1000);
	g_n0l = Tokenizer_GetArgIntegerRange(1, 0, 1000);
	g_n1h = Tokenizer_GetArgIntegerRange(2, 0, 1000);
	g_n1l = Tokenizer_GetArgIntegerRange(3, 0, 1000);
	ADDLOG_INFO(LOG_FEATURE_CMD, "WS2811BB timing 0h=%i 0l=%i 1h=%i 1l=%i", g_n0h, g_n0l, g_n1h, g_n1l);
	return CMD_RES_OK;
}


// Sends 10 frames of 200 bytes of the given value, interrupts enabled so that the
// millisecond counter keeps running, and returns the average bit period in 0.1 ns.
static unsigned long WS2811BB_MeasureBitPeriod(byte fill) {
	unsigned long t0, t1;
	int i;
	memset(g_buf, fill, sizeof(g_buf));
	g_bytes = 200;
	t0 = (unsigned long)rtos_get_time();
	for (i = 0; i < 10; i++) {
		WS2811BB_SendEx(0);
	}
	t1 = (unsigned long)rtos_get_time();
	return (t1 - t0) * 625;	// ms * 1e6 / 16000 bits, in 0.1 ns
}

// Measures the real bit period (and, by difference, the real time of one delay
// iteration) with the sending code running from RAM. Results go to the log.
static commandResult_t WS2811BB_CMD_Calib(const void *context, const char *cmd, const char *args, int flags) {
	unsigned long p0, p1, p2;
	int saveBytes = g_bytes;
	int saveN0h = g_n0h;

	if (g_reg == 0) {
		return CMD_RES_ERROR;
	}
	p0 = WS2811BB_MeasureBitPeriod(0x00);
	g_n0h = saveN0h + 50;
	p1 = WS2811BB_MeasureBitPeriod(0x00);
	g_n0h = saveN0h;
	p2 = WS2811BB_MeasureBitPeriod(0xFF);
	ADDLOG_INFO(LOG_FEATURE_CMD, "WS2811BB calib (RAM): zero bit period %lu x0.1 ns, one bit period %lu x0.1 ns, timing %i/%i/%i/%i",
		p0, p2, g_n0h, g_n0l, g_n1h, g_n1l);
	ADDLOG_INFO(LOG_FEATURE_CMD, "WS2811BB calib (RAM): 50 more iterations on a 0 high add %lu x0.1 ns, so %lu x0.1 ns per iteration",
		p1 - p0, (p1 - p0) / 50);

	memset(g_buf, 0x00, sizeof(g_buf));
	g_bytes = saveBytes;
	if (g_reg) {
		*g_reg = g_invert ? g_hi : g_lo;
	}
	return CMD_RES_OK;
}

static commandResult_t WS2811BB_CMD_Invert(const void *context, const char *cmd, const char *args, int flags) {
	Tokenizer_TokenizeString(args, 0);
	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_invert = Tokenizer_GetArgIntegerRange(0, 0, 1);
	if (g_reg) {
		*g_reg = g_invert ? g_hi : g_lo;
	}
	ADDLOG_INFO(LOG_FEATURE_CMD, "WS2811BB invert=%i", g_invert);
	return CMD_RES_OK;
}

// startDriver WS2811BB
void WS2811BB_Init() {
	uint32_t cur;
	ledStrip_t api;

	g_pin = PIN_FindPinIndexForRole(IOR_UCS1912_DIN, -1);
	if (g_pin < 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "WS2811BB: no pin with role UCS1912_DIN");
		return;
	}
	HAL_PIN_Setup_Output(g_pin);
	g_reg = (volatile uint32_t *)(WS2811BB_GPIO_CFG_BASE + g_pin * 4);
	cur = *g_reg;
	g_hi = cur | WS2811BB_OUTPUT_BIT;
	g_lo = cur & ~((uint32_t)WS2811BB_OUTPUT_BIT);
	*g_reg = g_invert ? g_hi : g_lo;

	api.apply = WS2811BB_Apply;
	api.getByte = WS2811BB_GetByte;
	api.setByte = WS2811BB_SetByte;
	api.setLEDCount = WS2811BB_SetLEDCount;
	LEDS_InitShared(&api);

	//cmddetail:{"name":"WS2811BB_Timing","args":"[n0h][n0l][n1h][n1l]",
	//cmddetail:"descr":"Bit timing of the bit-banged WS2811 driver, in loop iterations (about 4 CPU cycles each): high and low time of a 0 bit, then of a 1 bit.",
	//cmddetail:"fn":"WS2811BB_CMD_Timing","file":"driver/drv_ws2811bb.c","requires":"",
	//cmddetail:"examples":"WS2811BB_Timing 6 24 20 10"}
	CMD_RegisterCommand("WS2811BB_Timing", WS2811BB_CMD_Timing, NULL);
	//cmddetail:{"name":"WS2811BB_Invert","args":"[0|1]",
	//cmddetail:"descr":"1 (default): idle level is high and pulses are low on the pin, for lines going through an inverting gate. 0: normal polarity.",
	//cmddetail:"fn":"WS2811BB_CMD_Invert","file":"driver/drv_ws2811bb.c","requires":"",
	//cmddetail:"examples":"WS2811BB_Invert 1"}
	CMD_RegisterCommand("WS2811BB_Invert", WS2811BB_CMD_Invert, NULL);
	//cmddetail:{"name":"WS2811BB_Calib","args":"",
	//cmddetail:"descr":"Measures real delay loop and bit sending speed and prints it to the log.",
	//cmddetail:"fn":"WS2811BB_CMD_Calib","file":"driver/drv_ws2811bb.c","requires":"",
	//cmddetail:"examples":"WS2811BB_Calib"}
	CMD_RegisterCommand("WS2811BB_Calib", WS2811BB_CMD_Calib, NULL);
}

void WS2811BB_Shutdown() {
	LEDS_ShutdownShared();
	g_bytes = 0;
}

#endif
