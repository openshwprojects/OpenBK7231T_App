/*
 * CMT2300A Treiber (BK7231N, Bit-Bang-SPI), Direct Mode / OOK-Empfang und -Senden.
 *
 * Konsolenbefehle (falls CMT_ENABLE_COMMANDS = 1):
 *   cmtinfo            Status, wichtige Register, RSSI, GPIO-Pegel
 *   cmtdump            alle Register 0x00..0x6F
 *   cmtreg <a> [<v>]   Register a (hex) lesen bzw. mit v (hex) schreiben
 *   cmtfreq [MHz]      Frequenz anzeigen bzw. setzen (jeder abstimmbare Wert, siehe cmtvco)
 *   cmtvco [min max]   VCO-Bereich in MHz anzeigen/setzen (Standard 1680 2040) und die
 *                      daraus abstimmbaren Baender listen. Senden auf jeder abstimmbaren Frequenz
 *                      (Sperre auf ISM-Baender per CMT_TX_BAND_LOCK = 1 einschaltbar).
 *   cmtscan [von bis [kHz [s]]]  Frequenz des Signals suchen (RSSI-Sweep), Sender druecken
 *   cmtspy [MHz [s]]   Protokoll-Spy: Pulse aufzeichnen, Pakete/PWM/PPM auswerten
 *   cmtstop            Scan/Spy abbrechen
 *   cmtcap [ms] [clk]  DOUT aufzeichnen und dekodieren (Standard 1500 ms, blockiert!)
 *                      clk=1: DOUT nur bei steigender DCLK-Flanke abtasten (wie Tuya)
 *   cmtsend <Data> [Wdh]                         Code senden (24 Bit PWM wie RF-Bridge)
 *   cmtsend <Data> <Sync> <Low> <High> [Wdh] [Bits]
 *   cmtsend {"Sync":8970,"Low":300,"High":850,"Data":"6B59A2"}   (auch RfReceived-JSON,
 *                      optional "Repeat" und "Bits"). Ohne Zeiten: komplette Form des zuletzt
 *                      empfangenen Codes, falls Data gleich ist, sonst 8970/300/850.
 *                      Erweitert (z.B. Klingel): "T0H","T0L","T1H","T1L" = High/Pause fuer
 *                      Bit 0/1, "Merged":1 = kein Sync-Puls, Luecke ersetzt die letzte Pause.
 *   cmtcarrier [ms]    Testtraeger senden (Standard 1000 ms, max. 3000), misst DCLK im TX
 *   cmtsensor <Kanal> <Temp> <Feuchte> [Id] [Batterie] [Intervall_s]
 *   cmtsensor {"Channel":1,"Temperature":21.5,"Humidity":45,"Id":"5A","Battery":1,"Interval":60}
 *                      Funksensor nachbilden (Nexus-Format wie der Empfang, 36 Bit PPM).
 *                      Sendet sofort und danach alle <Intervall> s von selbst (0 = nur einmal).
 *   cmtsensor <Kanal> off   virtuellen Sensor abschalten
 *   cmtspec [von bis [kHz [MQTT_s [schnell]]]]   Spektrum dauernd messen (Standard
 *                      433.0 434.8 MHz, 25 kHz, MQTT alle 5 s, schnell=0). Anzeige auf der
 *                      Hauptseite, MQTT <Geraet>/RfSpectrum/get. Empfang ruht so lange.
 *   cmtspec off | reset | clear   anhalten | Max-Hold/Mittel loeschen | Daten freigeben
 */
#include "../new_common.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_hwtimer.h"
#include "../new_pins.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/new_http.h"

void PIN_set_wifi_led(int value);
#include "cmt2300a.h"
#include "cmt2300a_defs.h"
#include "cmt2300a_params.h"

/* 1 = Konsolenbefehle registrieren. Bei Compilerfehlern in diesem Block auf 0 setzen
 *     und stattdessen CMT_AUTOCAP_MS nutzen. */
#define CMT_ENABLE_COMMANDS   1
/* >0 = nach dem Start einmal so viele ms aufzeichnen (blockiert so lange) */
#define CMT_AUTOCAP_MS        0
/* 1 = erkannte Codes per MQTT melden (Funktionsname unten gegen dein Projekt pruefen) */
#define CMT_ENABLE_MQTT       1
/* Sensor-Einzelwerte zusaetzlich auch ohne "/get" im Topic melden (benoetigt OBK_PUBLISH_FLAG_FORCE_REMOVE_GET
 * aus new_mqtt.h; fehlt das Makro in deinem Projekt, wird nur die /get-Variante gesendet) */
#define CMT_MQTT_NO_GET_COPY  1
/* WiFi-LED kurz ausschalten, wenn ein Code gemeldet wird (danach wieder an) */
#define CMT_LED_BLINK_MS      80
#define CMT_LED_FLASH()       cmt_led_flash()
/* 1 = Senden nur in 433,05..434,79 / 863..870 MHz erlauben, 0 = ueberall (Hinweis im Log) */
#define CMT_TX_BAND_LOCK      0

#if CMT_ENABLE_COMMANDS
#include "../cmnds/cmd_public.h"
#endif

/* ---------------- Pins ---------------- */
#define CMT_SCLK    14
#define CMT_SDIO    16
#define CMT_CSB      6
#define CMT_FCSB    26
#define CMT_GPIO1   20   /* DCLK */
#define CMT_GPIO2   22   /* DOUT (RX) / DIN (TX) */

#define CMT_READ_DOUT()  HAL_PIN_ReadDigitalInput(CMT_GPIO2)

/* ---------------- Bit-Bang-SPI ---------------- */
static void cmt_delay(void)
{
	volatile int i;
	for(i = 0; i < 10; i++);
}

static void ty_set_csb_h(void)   { HAL_PIN_SetOutputValue(CMT_CSB, 1); }
static void ty_set_csb_l(void)   { HAL_PIN_SetOutputValue(CMT_CSB, 0); }
static void ty_set_fcsb_h(void)  { HAL_PIN_SetOutputValue(CMT_FCSB, 1); }
static void ty_set_fcsb_l(void)  { HAL_PIN_SetOutputValue(CMT_FCSB, 0); }
static void ty_set_sclk_h(void)  { HAL_PIN_SetOutputValue(CMT_SCLK, 1); }
static void ty_set_sclk_l(void)  { HAL_PIN_SetOutputValue(CMT_SCLK, 0); }
static void ty_set_sdio_h(void)  { HAL_PIN_SetOutputValue(CMT_SDIO, 1); }
static void ty_set_sdio_l(void)  { HAL_PIN_SetOutputValue(CMT_SDIO, 0); }
static void ty_set_sdio_out(void){ HAL_PIN_Setup_Output(CMT_SDIO); }
static void ty_set_sdio_in(void) { HAL_PIN_Setup_Input(CMT_SDIO); }
static int  ty_read_sdio_pin(void){ return HAL_PIN_ReadDigitalInput(CMT_SDIO); }
static int  ty_read_gpio1_pin(void){ return HAL_PIN_ReadDigitalInput(CMT_GPIO1); }
static int  ty_read_gpio2_pin(void){ return HAL_PIN_ReadDigitalInput(CMT_GPIO2); }

static void cmt_spi3_send(BYTE data8)
{
	BYTE i;

	for(i = 0; i < 8; i++)
	{
		ty_set_sclk_l();
		if(data8 & 0x80) ty_set_sdio_h(); else ty_set_sdio_l();
		cmt_delay();
		data8 <<= 1;
		ty_set_sclk_h();
		cmt_delay();
	}
}

static BYTE cmt_spi3_recv(void)
{
	BYTE i;
	BYTE data8 = 0xFF;

	for(i = 0; i < 8; i++)
	{
		ty_set_sclk_l();
		cmt_delay();
		data8 <<= 1;
		ty_set_sclk_h();
		if(ty_read_sdio_pin())
			data8 |= 1;
		cmt_delay();
	}
	return data8;
}

void CMT2300A_DelayMs(int ms)
{
	rtos_delay_milliseconds(ms);
}

void CMT2300A_DelayUs(int us)
{
	volatile int i;

	while(us--)
	{
		for(i = 0; i < 20; i++)
		{
		}
	}
}

static void cmt_spi3_write(BYTE addr, BYTE dat)
{
	ty_set_sdio_out();
	ty_set_sclk_l();
	ty_set_csb_l();
	cmt_delay();
	cmt_delay();

	cmt_spi3_send(addr & 0x7F);
	cmt_spi3_send(dat);

	ty_set_sclk_l();
	cmt_delay();
	cmt_delay();
	ty_set_csb_h();
	ty_set_sdio_in();
}

static void cmt_spi3_read(BYTE addr, BYTE *dat)
{
	ty_set_sdio_out();
	ty_set_sclk_l();
	ty_set_csb_l();
	cmt_delay();
	cmt_delay();

	cmt_spi3_send(addr | 0x80);
	ty_set_sdio_in();
	*dat = cmt_spi3_recv();

	ty_set_sclk_l();
	cmt_delay();
	cmt_delay();
	ty_set_csb_h();
}

void cmt_spi3_write_fifo(const BYTE *buf, WORD len)
{
	WORD i;

	ty_set_sclk_l();
	ty_set_fcsb_l();
	for(i = 0; i < len; i++)
		cmt_spi3_send(buf[i]);
	ty_set_fcsb_h();
}

void cmt_spi3_read_fifo(BYTE *buf, WORD len)
{
	WORD i;

	ty_set_sdio_in();
	ty_set_sclk_l();
	ty_set_fcsb_l();
	for(i = 0; i < len; i++)
		buf[i] = cmt_spi3_recv();
	ty_set_fcsb_h();
}

BYTE CMT2300A_ReadReg(BYTE addr)
{
	BYTE dat = 0x55;
	cmt_spi3_read(addr, &dat);
	return dat;
}

void CMT2300A_WriteReg(BYTE addr, BYTE dat)
{
	cmt_spi3_write(addr, dat);
}

static int CMT2300A_IsExist(void)
{
	BYTE back, dat;

	back = CMT2300A_ReadReg(CMT2300A_CUS_PKT17);
	CMT2300A_WriteReg(CMT2300A_CUS_PKT17, 0xAA);
	dat = CMT2300A_ReadReg(CMT2300A_CUS_PKT17);
	CMT2300A_WriteReg(CMT2300A_CUS_PKT17, back);

	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT REG48 before=%02X after=%02X", back, dat);
	return dat == 0xAA;
}

/* ====================================================================
 *  OOK-Decoder (EV1527/PT2262-aehnlich): Sync-Luecke, dann 24 Bit,
 *  jedes Bit = High-Puls + Low-Puls. Bit = 1, wenn High-Puls lang.
 *  Zeiten in us. Passt zu RF-Bridge: Sync ~8970, Low ~300, High ~850.
 *  Auch Klingel SFK 36 A1: Gap ~20 ms, High 450/1450, Low 1600/580,
 *  23 volle Paare + 24. High verschmolzen mit der Luecke (= Bit 0).
 * ==================================================================== */
#define CMT_FRAME_BITS       24
#define CMT_SYNC_MIN_US      5000
#define CMT_SYNC_MAX_US      40000    /* Klingel SFK 36 A1: ~20 ms */
#define CMT_PULSE_MIN_US     150
#define CMT_PULSE_MAX_US     2000    /* Klingel: High 1450 / Low 1600 us */
#define CMT_BIT_THRESH_US    500      /* nur Fallback; normal gilt High > Low */
#define CMT_BIT_MARGIN_US    100      /* High/Low naeher beieinander = mehrdeutig */
#define CMT_MAX_FRAMES       32
#define CMT_RAW_MAX          100

typedef struct
{
	uint32_t data;
	int syncUs;
	int shortUs;
	int longUs;
	int low0Us;     /* mittlere Pause nach kurzem High (Bit 0) */
	int low1Us;     /* mittlere Pause nach langem High (Bit 1) */
	int merged;     /* 1 = letztes Bit ohne eigene Pause, Luecke folgt direkt (Klingel) */
} cmt_frame_t;

typedef struct
{
	int inFrame;
	int nbits;
	uint32_t data;
	int pendHigh;
	int syncUs;
	int sumShort, nShort, sumLong, nLong;
	int sumL0, nL0, sumL1, nL1;
	int merged;
} cmt_dec_t;

static cmt_dec_t s_dec;
static cmt_frame_t s_frames[CMT_MAX_FRAMES];
static int s_frameCount;
static int s_frameTotal;

static void cmt_dec_reset(void)
{
	memset(&s_dec, 0, sizeof(s_dec));
}

static void cmt_dec_start(int syncUs)
{
	cmt_dec_reset();
	s_dec.inFrame = 1;
	s_dec.syncUs = syncUs;
}

static void cmt_dec_complete(void)
{
	/* Plausibilitaet: kurze und lange Pulse muessen klar verschieden sein */
	if(s_dec.nShort && s_dec.nLong
		&& (uint32_t)(s_dec.sumLong / s_dec.nLong) * 10u < (uint32_t)(s_dec.sumShort / s_dec.nShort) * 17u)
	{
		s_dec.inFrame = 0;
		return;
	}
	s_frameTotal++;
	if(s_frameCount < CMT_MAX_FRAMES)
	{
		cmt_frame_t *f = &s_frames[s_frameCount++];
		f->data = s_dec.data;
		f->syncUs = s_dec.syncUs;
		f->shortUs = s_dec.nShort ? s_dec.sumShort / s_dec.nShort : 0;
		f->longUs = s_dec.nLong ? s_dec.sumLong / s_dec.nLong : 0;
		f->low0Us = s_dec.nL0 ? s_dec.sumL0 / s_dec.nL0 : 0;
		f->low1Us = s_dec.nL1 ? s_dec.sumL1 / s_dec.nL1 : 0;
		f->merged = s_dec.merged;
	}
	s_dec.inFrame = 0;
}

/* lowUs <= 0: Low-Puls unbekannt (verschmolzen mit Sync-Luecke) -> Schwelle.
 * Rueckgabe 0 = mehrdeutig, Frame verwerfen. */
static int cmt_dec_addbit(int highUs, int lowUs)
{
	int bit;

	if(lowUs > 0)
	{
		int d = highUs - lowUs;
		if(d < 0) d = -d;
		if(d < CMT_BIT_MARGIN_US)
			return 0;
		bit = (highUs > lowUs) ? 1 : 0;
	}
	else
	{
		/* Pause unbekannt: Schwelle aus diesem Rahmen (Mitte zwischen kurzen und langen Highs).
		 * Feste Schwelle nur, wenn bisher nur eine Sorte vorkam. */
		int thr = CMT_BIT_THRESH_US;

		if(s_dec.nShort && s_dec.nLong)
			thr = (s_dec.sumShort / s_dec.nShort + s_dec.sumLong / s_dec.nLong) / 2;
		else if(s_dec.nShort)
			thr = (s_dec.sumShort / s_dec.nShort) * 17 / 10;
		else if(s_dec.nLong)
			thr = (s_dec.sumLong / s_dec.nLong) * 10 / 17;
		bit = (highUs >= thr) ? 1 : 0;
	}

	if(bit) { s_dec.sumLong += highUs;  s_dec.nLong++; }
	else    { s_dec.sumShort += highUs; s_dec.nShort++; }
	if(lowUs > 0)
	{
		if(bit) { s_dec.sumL1 += lowUs; s_dec.nL1++; }
		else    { s_dec.sumL0 += lowUs; s_dec.nL0++; }
	}
	s_dec.data = (s_dec.data << 1) | (uint32_t)bit;
	s_dec.nbits++;
	s_dec.pendHigh = 0;
	return 1;
}

/* Wird fuer jeden abgeschlossenen Pegel (level) mit seiner Dauer aufgerufen */
static void cmt_dec_feed(int level, int us)
{
	/* letztes Bit, dessen Low-Puls mit der Sync-Luecke verschmilzt */
	if(s_dec.inFrame && level == 0 && s_dec.pendHigh
		&& s_dec.nbits == CMT_FRAME_BITS - 1
		&& us >= CMT_SYNC_MIN_US && us <= CMT_SYNC_MAX_US)
	{
		s_dec.merged = 1;
		if(cmt_dec_addbit(s_dec.pendHigh, 0))
			cmt_dec_complete();
		cmt_dec_start(us);
		return;
	}

	if(level == 0 && us >= CMT_SYNC_MIN_US && us <= CMT_SYNC_MAX_US)
	{
		cmt_dec_start(us);
		return;
	}
	if(!s_dec.inFrame)
		return;
	if(us < CMT_PULSE_MIN_US || us > CMT_PULSE_MAX_US)
	{
		s_dec.inFrame = 0;
		return;
	}
	if(level == 1)
	{
		s_dec.pendHigh = us;
		return;
	}
	if(s_dec.pendHigh == 0)
	{
		s_dec.inFrame = 0;
		return;
	}
	if(!cmt_dec_addbit(s_dec.pendHigh, us))
	{
		s_dec.inFrame = 0;
		return;
	}
	if(s_dec.nbits == CMT_FRAME_BITS)
		cmt_dec_complete();
}

/* ====================================================================
 *  Aufzeichnung von DOUT (Polling, blockiert fuer window_ms)
 * ==================================================================== */
static uint8_t  s_rawLvl[CMT_RAW_MAX];
static uint16_t s_rawUs[CMT_RAW_MAX];
static int s_chipOk;

/* Schleifendurchlaeufe pro ms, mit derselben Schleifenstruktur wie die Aufzeichnung */
static uint32_t cmt_calibrate(void)
{
	uint32_t n = 0, t0, ms, per_ms;
	int last = CMT_READ_DOUT(), v;

	t0 = rtos_get_time();
	for(;;)
	{
		v = CMT_READ_DOUT();
		if(v != last)
			last = v;
		n++;
		if((n & 31) == 0 && (rtos_get_time() - t0) >= 50)
			break;
	}
	ms = rtos_get_time() - t0;
	if(ms == 0) ms = 1;
	per_ms = n / ms;
	return per_ms ? per_ms : 1;
}

/* DCLK (GPIO1) Frequenz in Hz messen: steigende Flanken in 100 ms zaehlen */
static uint32_t cmt_measure_dclk_hz(void)
{
	uint32_t n = 0, t0, ms, edges = 0;
	int last = ty_read_gpio1_pin(), v;

	t0 = rtos_get_time();
	for(;;)
	{
		v = ty_read_gpio1_pin();
		if(v && !last)
			edges++;
		last = v;
		n++;
		if((n & 31) == 0 && (rtos_get_time() - t0) >= 100)
			break;
	}
	ms = rtos_get_time() - t0;
	if(ms == 0) ms = 1;
	return edges * 1000u / ms;
}

static void cmt_log_raw(int rawN)
{
	char buf[160];
	int i, p = 0;

	for(i = 0; i < rawN; i++)
	{
		p += snprintf(buf + p, sizeof(buf) - p, "%d:%d ", s_rawLvl[i], s_rawUs[i]);
		if((i % 10) == 9 || i == rawN - 1)
		{
			ADDLOG_INFO(LOG_FEATURE_MAIN, "raw[%d..] %s", i - (i % 10), buf);
			p = 0;
		}
	}
}

/* Laufzeiten werden waehrend der Aufnahme nur als Zaehler (Schleifendurchlaeufe bzw.
 * DCLK-Takte) gespeichert und erst danach mit der ECHTEN Durchlaufrate des ganzen
 * Fensters in us umgerechnet. So haengt das Ergebnis nicht davon ab, wie schwer die
 * Schleife ist. */
#define CMT_MAX_RUNS   3000
static uint16_t *s_runs;          /* nur waehrend cmtcap belegt */
static int s_runN;

/* clocked = 1: DOUT wird nur bei steigender DCLK-Flanke (GPIO1) abgetastet, wie im
 * Tuya-Treiber (IRQ auf DCLK). Laufzeiten werden dann in DCLK-Takten gezaehlt. */
static void cmt_capture_run(int window_ms, int clocked)
{
	uint32_t it_ms, dclk_hz, cnt = 0, n = 0, t0, elapsed, per_ms_real;
	int last, firstLevel, v, rawN = 0, rawOn = 0, i, lastClk, clk, doSample, level;
	uint32_t candCnt = 0, glitchN;

	if(window_ms < 100) window_ms = 100;
	if(window_ms > 20000) window_ms = 20000;

	CMT2300A_GoRx();
	it_ms = cmt_calibrate();               /* nur fuer die Glitch-Schwelle */
	dclk_hz = cmt_measure_dclk_hz();
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT DCLK(GPIO1) = %u Hz", (unsigned)dclk_hz);
	if(clocked && dclk_hz < 100)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMTCAP: DCLK laeuft nicht, wechsle auf freie Abtastung");
		clocked = 0;
	}

	s_frameCount = 0;
	s_frameTotal = 0;
	s_runN = 0;
	cmt_dec_reset();

	/* Glitch-Filter: neuer Pegel muss ca. 60 us stabil sein (getaktet: sofort) */
	glitchN = clocked ? 1 : (it_ms * 60 / 1000);
	if(glitchN < 2 && !clocked) glitchN = 2;

	last = CMT_READ_DOUT();
	firstLevel = last;
	lastClk = ty_read_gpio1_pin();
	t0 = rtos_get_time();
	for(;;)
	{
		doSample = 1;
		if(clocked)
		{
			clk = ty_read_gpio1_pin();
			doSample = (clk && !lastClk);
			lastClk = clk;
		}
		if(doSample)
		{
			v = CMT_READ_DOUT();
			cnt++;
			if(candCnt)
			{
				if(v == last)
					candCnt = 0;              /* Glitch, wird verschluckt */
				else
					candCnt++;
			}
			else if(v != last)
				candCnt = 1;

			if(candCnt >= glitchN)             /* neuer Pegel stabil: Flanke buchen */
			{
				uint32_t runIt = cnt - candCnt;
				s_runs[s_runN++] = (uint16_t)(runIt > 65535 ? 65535 : runIt);
				cnt = candCnt;
				candCnt = 0;
				last = v;
				if(s_runN >= CMT_MAX_RUNS)
					break;                     /* Puffer voll (Rauschen) */
			}
		}
		n++;
		if((n & 31) == 0 && (int)(rtos_get_time() - t0) >= window_ms)
			break;
	}
	elapsed = rtos_get_time() - t0;
	if(elapsed == 0) elapsed = 1;
	/* echte Rate: freie Abtastung = Durchlaeufe pro ms ueber das ganze Fenster */
	per_ms_real = n / elapsed;
	if(per_ms_real == 0) per_ms_real = 1;

	/* ---- Auswertung ---- */
	level = firstLevel;
	for(i = 0; i < s_runN; i++)
	{
		int us = clocked ? (int)((uint64_t)s_runs[i] * 1000000u / dclk_hz)
		                 : (int)((uint64_t)s_runs[i] * 1000u / per_ms_real);

		cmt_dec_feed(level, us);
		if(!rawOn && level == 0 && us >= CMT_SYNC_MIN_US)
			rawOn = 1;
		if(rawOn && rawN < CMT_RAW_MAX)
		{
			s_rawLvl[rawN] = (uint8_t)level;
			s_rawUs[rawN] = (uint16_t)(us > 65535 ? 65535 : us);
			rawN++;
		}
		level ^= 1;
	}

	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTCAP %s window=%ums it/ms(real)=%u runs=%d%s frames=%d",
		clocked ? "clocked" : "free", (unsigned)elapsed, (unsigned)per_ms_real, s_runN,
		(s_runN >= CMT_MAX_RUNS) ? " (PUFFER VOLL)" : "", s_frameTotal);
	if(s_runN == 0)
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTCAP: DOUT blieb konstant auf %d", last);
	if(rawN)
		cmt_log_raw(rawN);
	if(s_frameCount > 0)
	{
		int ones[CMT_FRAME_BITS], b, same = 0;
		uint32_t cons = 0;

		memset(ones, 0, sizeof(ones));
		for(i = 0; i < s_frameCount; i++)
			for(b = 0; b < CMT_FRAME_BITS; b++)
				if((s_frames[i].data >> b) & 1)
					ones[b]++;
		for(b = 0; b < CMT_FRAME_BITS; b++)
			if(ones[b] * 2 > s_frameCount)
				cons |= (1u << b);
		for(i = 0; i < s_frameCount; i++)
			if((s_frames[i].data & 0xFFFFFF) == cons)
				same++;
		ADDLOG_INFO(LOG_FEATURE_MAIN,
			"CMT KONSENS Data=%06X (bitweise Mehrheit aus %d Frames, %d davon identisch)",
			(unsigned)cons, s_frameCount, same);
	}
	for(i = 0; i < s_frameCount; i++)
	{
		cmt_frame_t *f = &s_frames[i];
		ADDLOG_INFO(LOG_FEATURE_MAIN,
			"RfReceived? Sync=%d Low=%d High=%d Data=%06X (invertiert %06X)",
			f->syncUs, f->shortUs, f->longUs,
			(unsigned)(f->data & 0xFFFFFF), (unsigned)(~f->data & 0xFFFFFF));
	}
}

static void cmt_capture(int window_ms, int clocked)
{
	s_runs = (uint16_t *)malloc(CMT_MAX_RUNS * sizeof(uint16_t));
	if(!s_runs)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMTCAP: kein Speicher (%d Bytes)", (int)(CMT_MAX_RUNS * sizeof(uint16_t)));
		return;
	}
	cmt_capture_run(window_ms, clocked);
	free(s_runs);
	s_runs = NULL;
}

/* ====================================================================
 *  Dauerbetrieb: HW-Timer-ISR tastet DOUT ab (blockiert nichts).
 *  ISR: nur Pegel abtasten, Laufzeit in Ticks in den Ringpuffer.
 *  runQuickTick: Ringpuffer leeren, dekodieren, Frames entprellen, melden.
 *  Beim Senden benutzt dieselbe ISR (derselbe Timer) die Ticks zum Ausgeben
 *  der Pulsfolge auf GPIO2 (= DIN des Chips) -> kein zweiter Timer, kein Konflikt.
 * ==================================================================== */
#define CMT_RX_PERIOD_US   50.0f
#define CMT_RING           512
#define CMT_REPORT_GAP_MS  500     /* gleicher Code frueher nicht nochmal melden */
#define CMT_BURST_GAP_MS   1000    /* laengere Pause = neuer Tastendruck */

static volatile uint16_t s_ring[CMT_RING];
static volatile uint16_t s_head, s_tail;
static volatile uint32_t s_ringDrops;
static volatile int      isr_level;
static volatile uint32_t isr_run;
static volatile uint8_t  isr_candN;
static float  s_realPeriodUs = CMT_RX_PERIOD_US;
static int    s_periodInt = 50;
static int8_t s_timerChan = -1;
static int    s_rxRunning;

static uint32_t s_statFrames, s_statReports;
static uint32_t s_lastData = 0xFFFFFFFF, s_lastFrameMs, s_lastReportMs;
static int      s_sameCnt;
static uint32_t s_lastCode;     /* zuletzt gemeldeter Code */
static int      s_lastSync, s_lastLow, s_lastHigh;
static int      s_lastL0, s_lastL1, s_lastMerged;

/* ---- Senden: Daten fuer die ISR ----
 * s_txIsr: 0 = ISR tastet ab (RX), 1 = ISR tut nichts (Umschaltung), 2 = ISR sendet.
 * s_txDur[]: Pegeldauern in Timer-Ticks, Pegel abwechselnd, beginnend mit High. */
#define CMT_TX_MAX_ENTRIES 1400
static uint16_t *s_txDur;          /* nur waehrend einer Aussendung belegt */
static int       s_txCap;
static volatile int      s_txIsr;
static volatile int      s_txN, s_txPos;
static volatile uint16_t s_txCnt;
static volatile uint8_t  s_txLevel, s_txStarted, s_txDone;
static volatile uint32_t s_txTicks;

static void cmt_tx_isr(void)
{
	if(s_txDone)
		return;
	s_txTicks++;
	if(!s_txStarted)
	{
		/* erster Tick: High ausgeben, ab hier wird gezaehlt (kein angeschnittener erster Puls) */
		s_txStarted = 1;
		s_txLevel = 1;
		s_txCnt = 0;
		HAL_PIN_SetOutputValue(CMT_GPIO2, 1);
		return;
	}
	if(++s_txCnt < s_txDur[s_txPos])
		return;
	s_txCnt = 0;
	s_txPos++;
	if(s_txPos >= s_txN)
	{
		HAL_PIN_SetOutputValue(CMT_GPIO2, 0);
		s_txLevel = 0;
		s_txDone = 1;
		return;
	}
	s_txLevel ^= 1;
	HAL_PIN_SetOutputValue(CMT_GPIO2, s_txLevel);
}

/* Laeuft im Timer-Interrupt: so kurz wie moeglich */
static void cmt_rx_isr(void *arg)
{
	int v;

	if(s_txIsr)
	{
		if(s_txIsr == 2)
			cmt_tx_isr();
		return;
	}

	v = CMT_READ_DOUT() ? 1 : 0;

	if(isr_run < 0x7FFF)
		isr_run++;
	if(v != isr_level)
	{
		if(++isr_candN >= 2)                 /* 2 Abtastungen stabil: Flanke gilt */
		{
			uint16_t e = (uint16_t)(((uint32_t)isr_level << 15) | ((isr_run - isr_candN) & 0x7FFF));
			uint16_t nx = (uint16_t)((s_head + 1) % CMT_RING);

			if(nx != s_tail)
			{
				s_ring[s_head] = e;
				s_head = nx;
			}
			else
				s_ringDrops++;
			isr_run = isr_candN;
			isr_level = v;
			isr_candN = 0;
		}
	}
	else
		isr_candN = 0;                       /* Glitch verschluckt */
}

static void cmt_rx_start(void)
{
	if(s_rxRunning)
		return;
	s_head = s_tail = 0;
	s_ringDrops = 0;
	isr_run = 0;
	isr_candN = 0;
	isr_level = CMT_READ_DOUT() ? 1 : 0;
	if(s_timerChan < 0)
	{
		s_timerChan = HAL_RequestHWTimer(CMT_RX_PERIOD_US, &s_realPeriodUs, cmt_rx_isr, NULL);
		s_periodInt = (int)(s_realPeriodUs + 0.5f);
		if(s_periodInt < 1) s_periodInt = 1;
	}
	if(s_timerChan < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: kein HW-Timer verfuegbar (von IR/RCSwitch belegt?)");
		return;
	}
	HAL_HWTimerStart(s_timerChan);
	s_rxRunning = 1;
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT RX-Timer laeuft: Periode %d us (Kanal %d)", s_periodInt, (int)s_timerChan);
}

static void cmt_rx_pause(void)
{
	if(s_rxRunning && s_timerChan >= 0)
		HAL_HWTimerStop(s_timerChan);
}

static void cmt_rx_resume(void)
{
	if(s_rxRunning && s_timerChan >= 0)
	{
		s_head = s_tail = 0;
		isr_run = 0;
		isr_candN = 0;
		isr_level = CMT_READ_DOUT() ? 1 : 0;
		HAL_HWTimerStart(s_timerChan);
	}
}

static uint32_t s_ledOffUntil;

static void cmt_led_flash(void)
{
	PIN_set_wifi_led(0);
	s_ledOffUntil = rtos_get_time() + CMT_LED_BLINK_MS;
	if(s_ledOffUntil == 0) s_ledOffUntil = 1;
}

static void cmt_publish(const cmt_frame_t *f)
{
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT RfReceived Sync=%d Low=%d High=%d Data=%06X  [T0=%d/%d T1=%d/%d %s]",
		f->syncUs, f->shortUs, f->longUs, (unsigned)(f->data & 0xFFFFFF),
		f->shortUs, f->low0Us, f->longUs, f->low1Us, f->merged ? "ohne Sync-Puls" : "mit Sync-Puls");
#if CMT_ENABLE_MQTT
	{
		char val[96];

		/* <Geraet>/RfReceived/get = JSON wie bei der RF-Bridge, <Geraet>/RfCode/get = nur der Code */
		snprintf(val, sizeof(val), "{\"RfReceived\":{\"Sync\":%d,\"Low\":%d,\"High\":%d,\"Data\":\"%06X\"}}",
			f->syncUs, f->shortUs, f->longUs, (unsigned)(f->data & 0xFFFFFF));
		MQTT_PublishMain_StringString("RfReceived", val, 0);
		snprintf(val, sizeof(val), "%06X", (unsigned)(f->data & 0xFFFFFF));
		MQTT_PublishMain_StringString("RfCode", val, 0);
	}
#endif
	CMT_LED_FLASH();
}

/* Frame entprellen: erst ab dem 2. identischen Frame einer Serie melden */
/* ====================================================================
 *  Funk-Sensor (Wetterstation, Nexus-aehnlich): 36 Bit, PPM
 *  High ~500 us, Low ~1000 us = 0, ~2000 us = 1, Zeilenende: High + Luecke ~4 ms.
 *  Bits: ID(8) Batterie(1) Taste(1) Kanal-1(2) Temperatur(12, signed, 0,1 C) 1111(4) Feuchte(8)
 *  Es gibt keine Pruefsumme, daher werden zwei identische Zeilen in Folge verlangt.
 *  Meldung: MQTT <Geraet>/RfSensor/get = {"RfSensor":{"Id":"B9","Channel":1,"Temperature":25.6,"Humidity":39,"Battery":1}}
 * ==================================================================== */
#define CMT_SENS_BITS        36
#define CMT_SENS_HIGH_MIN    350
#define CMT_SENS_HIGH_MAX    750
#define CMT_SENS_LOW0_MAX    1400     /* 700..1400 = 0 */
#define CMT_SENS_LOW1_MIN    1500     /* 1500..2600 = 1 */
#define CMT_SENS_LOW1_MAX    2600
#define CMT_SENS_GAP_MIN     3000
#define CMT_SENS_GAP_MAX     6000
#define CMT_SENS_REPORT_MS   3000     /* gleiche Messung frueher nicht nochmal melden */

static struct
{
	int nbits, pendHigh;
	uint64_t data;
	uint64_t lastRow, lastReported;
	uint32_t lastRowMs, lastReportMs;
	uint32_t rows, reports;
} sens;

#if CMT_ENABLE_MQTT
/* Einzelwert melden: <Geraet>/<name>/get und (falls moeglich) zusaetzlich <Geraet>/<name> */
static void cmt_mqtt_value(const char *name, const char *val)
{
	MQTT_PublishMain_StringString(name, val, 0);
#if CMT_MQTT_NO_GET_COPY && defined(OBK_PUBLISH_FLAG_FORCE_REMOVE_GET)
	MQTT_PublishMain_StringString(name, val, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
#endif
}
#endif

static void cmt_sens_publish(uint64_t d)
{
	int id = (int)((d >> 28) & 0xFF);
	int bat = (int)((d >> 27) & 1);
	int ch = (int)((d >> 24) & 3) + 1;
	int t = (int)((d >> 12) & 0xFFF);
	int hum = (int)(d & 0xFF);
	int neg = 0;

	if(t & 0x800) t -= 0x1000;
	if(t < 0) { neg = 1; t = -t; }
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT RfSensor Id=%02X Channel=%d Temperature=%s%d.%d Humidity=%d Battery=%d",
		id, ch, neg ? "-" : "", t / 10, t % 10, hum, bat);
#if CMT_ENABLE_MQTT
	{
		char val[128];

		snprintf(val, sizeof(val),
			"{\"RfSensor\":{\"Id\":\"%02X\",\"Channel\":%d,\"Temperature\":%s%d.%d,\"Humidity\":%d,\"Battery\":%d}}",
			id, ch, neg ? "-" : "", t / 10, t % 10, hum, bat);
		cmt_mqtt_value("RfSensor", val);
		snprintf(val, sizeof(val), "%02X", id);
		cmt_mqtt_value("RfSensorId", val);
		snprintf(val, sizeof(val), "%d", ch);
		cmt_mqtt_value("RfSensorChannel", val);
		snprintf(val, sizeof(val), "%s%d.%d", neg ? "-" : "", t / 10, t % 10);
		cmt_mqtt_value("RfSensorTemperature", val);
		snprintf(val, sizeof(val), "%d", hum);
		cmt_mqtt_value("RfSensorHumidity", val);
		snprintf(val, sizeof(val), "%d", bat);
		cmt_mqtt_value("RfSensorBattery", val);
	}
#endif
	CMT_LED_FLASH();
}

static void cmt_sens_row(uint64_t d)
{
	uint32_t now = rtos_get_time();
	int t, hum;

	if(((d >> 8) & 0xF) != 0xF)               /* festes Nibble 1111 */
		return;
	hum = (int)(d & 0xFF);
	t = (int)((d >> 12) & 0xFFF);
	if(t & 0x800) t -= 0x1000;
	if(hum > 100 || t < -400 || t > 800)      /* unplausibel: Rauschen */
		return;
	sens.rows++;
	if(d == sens.lastRow && (now - sens.lastRowMs) < 500)
	{
		if(!(d == sens.lastReported && (now - sens.lastReportMs) < CMT_SENS_REPORT_MS))
		{
			sens.lastReported = d;
			sens.lastReportMs = now;
			sens.reports++;
			cmt_sens_publish(d);
		}
	}
	sens.lastRow = d;
	sens.lastRowMs = now;
}

/* Wird fuer jeden abgeschlossenen Pegel aufgerufen (parallel zum Fernbedienungs-Decoder) */
static void cmt_sens_feed(int level, int us)
{
	if(level == 1)
	{
		sens.pendHigh = (us >= CMT_SENS_HIGH_MIN && us <= CMT_SENS_HIGH_MAX) ? us : 0;
		return;
	}
	if(us >= CMT_SENS_GAP_MIN && us <= CMT_SENS_GAP_MAX)
	{
		if(sens.pendHigh && sens.nbits == CMT_SENS_BITS)
			cmt_sens_row(sens.data);
		sens.nbits = 0;
		sens.data = 0;
		sens.pendHigh = 0;
		return;
	}
	if(!sens.pendHigh)
	{
		sens.nbits = 0;
		return;
	}
	sens.pendHigh = 0;
	if(sens.nbits >= CMT_SENS_BITS)
	{
		sens.nbits = CMT_SENS_BITS + 1;       /* zu lang, bis zur naechsten Luecke ignorieren */
		return;
	}
	if(us >= 700 && us <= CMT_SENS_LOW0_MAX)
		sens.data = (sens.data << 1);
	else if(us >= CMT_SENS_LOW1_MIN && us <= CMT_SENS_LOW1_MAX)
		sens.data = (sens.data << 1) | 1u;
	else
	{
		sens.nbits = CMT_SENS_BITS + 1;       /* ungueltig: Zeile verwerfen */
		return;
	}
	sens.nbits++;
}

static void cmt_on_frame(const cmt_frame_t *f)
{
	uint32_t now = rtos_get_time();
	uint32_t d = f->data & 0xFFFFFF;

	s_statFrames++;
	if(d == s_lastData && (now - s_lastFrameMs) < CMT_BURST_GAP_MS)
		s_sameCnt++;
	else
	{
		s_sameCnt = 1;
		s_lastData = d;
	}
	s_lastFrameMs = now;

	if(s_sameCnt >= 2 && (s_lastCode != d || (now - s_lastReportMs) > CMT_REPORT_GAP_MS))
	{
		s_lastCode = d;
		s_lastReportMs = now;
		s_lastSync = f->syncUs; s_lastLow = f->shortUs; s_lastHigh = f->longUs;
		s_lastL0 = f->low0Us; s_lastL1 = f->low1Us; s_lastMerged = f->merged;
		s_statReports++;
		cmt_publish(f);
	}
}

#define CMT_MODE_RX    0
#define CMT_MODE_SCAN  1
#define CMT_MODE_SPY   2
#define CMT_MODE_SPEC  3
static int s_mode;
static void cmt_scan_tick(void);
static void cmt_spy_tick(void);
static void cmt_spec_tick(void);
static void cmt_spec_stop(const char *why);
static uint32_t cmt_cur_khz(int tx);

/* ====================================================================
 *  Senden (OOK, Direct Mode)
 *  Ablauf (alles im Quick-Tick, SPI also nur aus dem Hauptkontext):
 *   1. ISR anhalten (s_txIsr = 1), Chip -> STBY
 *   2. FIFO_CTL: TX_DIN_EN = 1, TX_DIN_SEL = GPIO2 (IO_SEL GPIO2 = DOUT/DIN bleibt)
 *   3. Chip -> TFS, BK-Pin 22 als Ausgang auf 0, Chip -> TX (Traeger aus, da DIN = 0)
 *   4. ISR gibt die vorberechnete Pulsfolge im 50-us-Raster aus (s_txIsr = 2)
 *   5. Ende: DIN = 0, Chip -> STBY, Pin wieder Eingang, FIFO_CTL zurueck,
 *      Chip -> RX, Ringpuffer/Decoder zuruecksetzen, ISR tastet wieder ab
 *  Umschaltungen passieren immer mit Pegel 0 auf beiden Seiten.
 *  Rahmen (wie rc-switch/RF-Bridge, Bit MSB zuerst):
 *   Sync [ 24 Bit, Sync ] x Wiederholungen
 *   Sync  = High <Low us>, Pause <Sync us>
 *   Bit 1 = High <High us>, Pause <Low us>;  Bit 0 = High <Low us>, Pause <High us>
 * ==================================================================== */
#define CMT_TX_DEF_REPEAT   8
#define CMT_TX_MAX_REPEAT   20
#define CMT_TX_DEF_SYNC     8970
#define CMT_TX_DEF_LOW      300
#define CMT_TX_DEF_HIGH     850
#define CMT_TXQ             4

typedef struct
{
	uint64_t data;
	int bits, syncUs, lowUs, highUs, repeat;
	int t0h, t0l, t1h, t1l; /* Bit 0 / Bit 1: High- und Pausenzeit (aus Low/High abgeleitet, wenn 0) */
	int merged;             /* 1 = kein Sync-Puls, Luecke ersetzt die Pause des letzten Bits */
	int noLead;             /* 1 = kein Sync am Anfang (PPM-Sensor: Daten, dann Puls + Luecke) */
	int sensorCh;           /* > 0: virtueller Sensor (nur fuer das Log) */
	int carrierMs;          /* > 0: nur Testtraeger */
} cmt_txreq_t;

static cmt_txreq_t s_txq[CMT_TXQ];
static volatile int s_txqHead, s_txqTail;
static int      s_txState;            /* 0 = frei, 1 = sendet */
static int      s_txPinOut;           /* 1 = Pin 22 ist gerade Ausgang */
static cmt_txreq_t s_txCur;
static uint32_t s_txT0, s_txExpMs;
static BYTE     s_txFifoSave;
static uint32_t s_txCount, s_txErr;
static float    s_txCumUs;
static uint32_t s_txCumTicks;
static int      s_txBuildN;
static uint32_t s_txKhzNow;           /* aktuelle TX-Frequenz (bei jedem Umstimmen gesetzt) */

/* ISM/SRD-Baender, fuer die die Hardware gedacht ist */
static int cmt_tx_ism(uint32_t khz)
{
	return (khz >= 433050 && khz <= 434790) || (khz >= 863000 && khz <= 870000);
}

static int cmt_tx_allowed(uint32_t khz)
{
#if CMT_TX_BAND_LOCK
	return cmt_tx_ism(khz);
#else
	(void)khz;
	return 1;
#endif
}

static int cmt_tx_busy(void)
{
	return s_txState != 0 || s_txqHead != s_txqTail;
}

static int cmt_wait_state(BYTE st, int ms)
{
	uint32_t t0 = rtos_get_time();

	do
	{
		if(CMT2300A_GetChipStatus() == st)
			return 1;
	} while((int)(rtos_get_time() - t0) < ms);
	return CMT2300A_GetChipStatus() == st;
}

/* Dauer anhaengen. Rundung ueber die aufsummierte Zeit, damit sich Fehler nicht addieren. */
static int cmt_tx_push(int us)
{
	uint32_t t, d;

	if(!s_txDur || s_txBuildN >= s_txCap)
		return 0;
	s_txCumUs += (float)us;
	t = (uint32_t)(s_txCumUs / s_realPeriodUs + 0.5f);
	d = (t > s_txCumTicks) ? t - s_txCumTicks : 1;
	if(d < 1) d = 1;
	if(d > 65535) d = 65535;
	s_txCumTicks += d;
	s_txDur[s_txBuildN++] = (uint16_t)d;
	return 1;
}

static int cmt_tx_build(const cmt_txreq_t *r)
{
	int rep, b;

	s_txBuildN = 0;
	s_txCumUs = 0.0f;
	s_txCumTicks = 0;
	if(r->carrierMs > 0)
	{
		cmt_tx_push(r->carrierMs * 1000);
		cmt_tx_push(5000);
		return s_txBuildN;
	}
	if(r->merged)
	{
		/* Klingel-Form: N x (High, Pause), letzte Pause = Luecke. Davor reicht die Funkstille. */
		for(rep = 0; rep < r->repeat; rep++)
		{
			for(b = r->bits - 1; b >= 0; b--)
			{
				int one = (int)((r->data >> b) & 1ULL);

				cmt_tx_push(one ? r->t1h : r->t0h);
				cmt_tx_push(b == 0 ? r->syncUs : (one ? r->t1l : r->t0l));
			}
		}
		return s_txBuildN;
	}
	/* RF-Bridge/rc-switch-Form: Sync, [Bits, Sync] x Wdh. Sync-Puls = kurzer High.
	 * noLead (Sensor): [Bits, Puls + Luecke] x Wdh, davor nur Funkstille. */
	if(!r->noLead)
	{
		cmt_tx_push(r->t0h);
		cmt_tx_push(r->syncUs);
	}
	for(rep = 0; rep < r->repeat; rep++)
	{
		for(b = r->bits - 1; b >= 0; b--)
		{
			int one = (int)((r->data >> b) & 1ULL);

			cmt_tx_push(one ? r->t1h : r->t0h);
			cmt_tx_push(one ? r->t1l : r->t0l);
		}
		cmt_tx_push(r->t0h);
		cmt_tx_push(r->syncUs);
	}
	return s_txBuildN;
}

static void cmt_tx_freebuf(void)
{
	if(s_txDur)
		free(s_txDur);
	s_txDur = NULL;
	s_txCap = 0;
}

/* Zurueck in den Empfang. Wird auch bei Fehlern im Aufbau benutzt. */
static void cmt_tx_finish(void)
{
	s_txIsr = 1;                                  /* ISR: nichts tun */
	cmt_tx_freebuf();
	if(s_txPinOut)
		HAL_PIN_SetOutputValue(CMT_GPIO2, 0);     /* Traeger aus */
	CMT2300A_GoStby();
	if(s_txPinOut)
	{
		HAL_PIN_Setup_Input(CMT_GPIO2);
		s_txPinOut = 0;
	}
	if(!cmt_wait_state(CMT2300A_STA_STBY, 20))
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: STBY nicht erreicht (state=%02X)", CMT2300A_GetChipStatus());
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, s_txFifoSave);
	CMT2300A_GoRx();
	if(!cmt_wait_state(CMT2300A_STA_RX, 20))
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: RX nicht wieder erreicht (state=%02X)", CMT2300A_GetChipStatus());

	/* Empfang sauber neu aufsetzen */
	cmt_dec_reset();
	sens.nbits = 0;
	sens.data = 0;
	sens.pendHigh = 0;
	s_head = s_tail = 0;
	isr_run = 0;
	isr_candN = 0;
	isr_level = CMT_READ_DOUT() ? 1 : 0;
	s_txState = 0;
	s_txIsr = 0;                                  /* ISR tastet wieder ab */
}

static int cmt_tx_begin(const cmt_txreq_t *r)
{
	int n, need;

	if(!cmt_tx_allowed(s_txKhzNow))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Senden auf %u kHz gesperrt (nur 433050..434790 / 863000..870000 kHz)", (unsigned)s_txKhzNow);
		return 0;
	}
	if(!cmt_tx_ism(s_txKhzNow))
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX: sende auf %u kHz (ausserhalb der ISM-Baender)", (unsigned)s_txKhzNow);

	need = (r->carrierMs > 0) ? 2 : 2 + r->repeat * (2 * r->bits + 2);
	if(need > CMT_TX_MAX_ENTRIES)
		need = CMT_TX_MAX_ENTRIES;
	cmt_tx_freebuf();
	s_txDur = (uint16_t *)malloc((size_t)need * sizeof(uint16_t));
	if(!s_txDur)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: kein Speicher (%d Bytes)", (int)(need * sizeof(uint16_t)));
		return 0;
	}
	s_txCap = need;
	n = cmt_tx_build(r);
	if(n < 2)
	{
		cmt_tx_freebuf();
		return 0;
	}

	s_txIsr = 1;                                  /* RX-Abtastung aus, Timer laeuft weiter */
	CMT2300A_GoStby();
	if(!cmt_wait_state(CMT2300A_STA_STBY, 20))
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: STBY nicht erreicht (state=%02X)", CMT2300A_GetChipStatus());

	s_txFifoSave = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL,
		(BYTE)((s_txFifoSave & ~CMT2300A_MASK_TX_DIN_SEL) | CMT2300A_MASK_TX_DIN_EN | CMT2300A_TX_DIN_SEL_GPIO2));

	CMT2300A_GoTFS();
	if(!cmt_wait_state(CMT2300A_STA_TFS, 20))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: TFS nicht erreicht (state=%02X)", CMT2300A_GetChipStatus());
		cmt_tx_finish();
		return 0;
	}
	HAL_PIN_Setup_Output(CMT_GPIO2);
	HAL_PIN_SetOutputValue(CMT_GPIO2, 0);
	s_txPinOut = 1;

	CMT2300A_GoTx();
	if(!cmt_wait_state(CMT2300A_STA_TX, 20))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: TX nicht erreicht (state=%02X)", CMT2300A_GetChipStatus());
		cmt_tx_finish();
		return 0;
	}

	s_txN = n;
	s_txPos = 0;
	s_txCnt = 0;
	s_txStarted = 0;
	s_txDone = 0;
	s_txTicks = 0;
	s_txExpMs = (uint32_t)((float)s_txCumTicks * s_realPeriodUs / 1000.0f) + 1;
	s_txT0 = rtos_get_time();
	s_txState = 1;
	s_txIsr = 2;                                  /* ab jetzt gibt die ISR aus */
	return 1;
}

/* Quick-Tick-Teil. Rueckgabe 1 = Sendevorgang aktiv/gestartet, sonst nichts weiter tun */
static int cmt_tx_tick(void)
{
	if(s_txState)
	{
		uint32_t now = rtos_get_time();

		if(s_txDone)
		{
			uint32_t ms = now - s_txT0;
			uint32_t ticks = s_txTicks;

			cmt_tx_finish();
			s_txCount++;
			if(s_txCur.carrierMs > 0)
				ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX Traeger fertig: %u ms (erwartet %u ms), zurueck in RX",
					(unsigned)ms, (unsigned)s_txExpMs);
			else if(s_txCur.sensorCh > 0)
				ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX Sensor fertig: Kanal=%d Data=%09llX Wdh=%d, %u ms (erwartet %u ms), zurueck in RX",
					s_txCur.sensorCh, (unsigned long long)s_txCur.data, s_txCur.repeat, (unsigned)ms, (unsigned)s_txExpMs);
			else
				ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX fertig: Data=%0*llX Bits=%d Sync=%d T0=%d/%d T1=%d/%d %s Wdh=%d, %u ms (erwartet %u ms, %u Ticks), zurueck in RX",
					(s_txCur.bits + 3) / 4, (unsigned long long)s_txCur.data, s_txCur.bits, s_txCur.syncUs,
					s_txCur.t0h, s_txCur.t0l, s_txCur.t1h, s_txCur.t1l,
					s_txCur.merged ? "ohne Sync-Puls" : "mit Sync-Puls",
					s_txCur.repeat, (unsigned)ms, (unsigned)s_txExpMs, (unsigned)ticks);
			CMT_LED_FLASH();
		}
		else if((now - s_txT0) > s_txExpMs + 1000)
		{
			ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Zeitueberschreitung (Pos %d/%d, Ticks %u) - Timer-ISR laeuft nicht?",
				(int)s_txPos, (int)s_txN, (unsigned)s_txTicks);
			cmt_tx_finish();
			s_txErr++;
		}
		return 1;
	}
	if(s_mode == CMT_MODE_RX && s_rxRunning && s_txqTail != s_txqHead)
	{
		s_txCur = s_txq[s_txqTail];
		s_txqTail = (s_txqTail + 1) % CMT_TXQ;
		if(!cmt_tx_begin(&s_txCur))
		{
			s_txErr++;
			return 1;
		}
		if(s_txCur.carrierMs > 0)
		{
			/* Diagnose: laeuft DCLK im TX? (dann tastet der Chip DIN evtl. mit der Datenrate ab) */
			uint32_t hz;

			HAL_PIN_SetOutputValue(CMT_GPIO2, 1);
			hz = cmt_measure_dclk_hz();          /* blockiert 100 ms, ISR haelt den Pegel */
			ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX Traeger an: %d ms auf %u kHz, state=%02X, DCLK(GPIO1) im TX = %u Hz",
				s_txCur.carrierMs, (unsigned)cmt_cur_khz(1), CMT2300A_GetChipStatus(), (unsigned)hz);
		}
		return 1;
	}
	return 0;
}

static int cmt_tx_enqueue(const cmt_txreq_t *r)
{
	int nx = (s_txqHead + 1) % CMT_TXQ;

	if(nx == s_txqTail)
		return 0;
	s_txq[s_txqHead] = *r;
	s_txqHead = nx;
	return 1;
}

/* ====================================================================
 *  Virtuelle Funksensoren (Senden im Nexus-Format, wie der Empfang oben):
 *  36 Bit PPM, High 500 us, Pause 1000 us = 0 / 2000 us = 1,
 *  Zeilenende High 500 us + Luecke 4000 us, 10 Zeilen pro Aussendung.
 *  Bits: ID(8) Batterie(1) Taste(1)=0 Kanal-1(2) Temperatur(12, 0,1 C) 1111 Feuchte(8)
 * ==================================================================== */
#define CMT_VS_SLOTS        3
#define CMT_VS_REPEAT       10
#define CMT_VS_PULSE_US     500
#define CMT_VS_ZERO_US      1000
#define CMT_VS_ONE_US       2000
#define CMT_VS_GAP_US       4000
#define CMT_VS_DEF_ID       0x5A
#define CMT_VS_DEF_INTERVAL 60

static struct
{
	int active, id, tenth, hum, bat, interval;
	uint32_t lastMs;
} s_vs[CMT_VS_SLOTS];

static uint64_t cmt_vs_encode(int ch)
{
	const int i = ch - 1;

	return ((uint64_t)(s_vs[i].id & 0xFF) << 28)
	     | ((uint64_t)(s_vs[i].bat & 1) << 27)
	     | ((uint64_t)((ch - 1) & 3) << 24)
	     | ((uint64_t)(s_vs[i].tenth & 0xFFF) << 12)
	     | (0xFULL << 8)
	     | (uint64_t)(s_vs[i].hum & 0xFF);
}

static int cmt_vs_send(int ch)
{
	cmt_txreq_t r;

	memset(&r, 0, sizeof(r));
	r.data = cmt_vs_encode(ch);
	r.bits = 36;
	r.repeat = CMT_VS_REPEAT;
	r.syncUs = CMT_VS_GAP_US;
	r.lowUs = r.t0h = r.t1h = CMT_VS_PULSE_US;
	r.highUs = CMT_VS_ONE_US;
	r.t0l = CMT_VS_ZERO_US;
	r.t1l = CMT_VS_ONE_US;
	r.noLead = 1;
	r.sensorCh = ch;
	return cmt_tx_enqueue(&r);
}

/* faellige virtuelle Sensoren einreihen (Quick-Tick, nur im RX-Betrieb) */
static void cmt_vs_tick(void)
{
	uint32_t now = rtos_get_time();
	int i;

	for(i = 0; i < CMT_VS_SLOTS; i++)
	{
		if(!s_vs[i].active || s_vs[i].interval <= 0)
			continue;
		if((now - s_vs[i].lastMs) >= (uint32_t)s_vs[i].interval * 1000u)
		{
			if(cmt_vs_send(i + 1))
				s_vs[i].lastMs = now;
		}
	}
}

static void cmt_fmt_tenth(char *buf, int sz, int tenth)
{
	int a = tenth < 0 ? -tenth : tenth;

	snprintf(buf, sz, "%s%d.%d", tenth < 0 ? "-" : "", a / 10, a % 10);
}

/* Ein Element aus dem Ringpuffer holen (Hauptkontext). 1 = Element geholt */
static int cmt_ring_pop(uint16_t *e)
{
	if(s_tail == s_head)
		return 0;
	*e = s_ring[s_tail];
	s_tail = (uint16_t)((s_tail + 1) % CMT_RING);
	return 1;
}

void CMT2300A_RunQuickTick(void)
{
	int n = 0;

	if(s_ledOffUntil && (int32_t)(rtos_get_time() - s_ledOffUntil) >= 0)
	{
		PIN_set_wifi_led(1);
		s_ledOffUntil = 0;
	}
	if(!s_chipOk)
		return;
	if(s_mode == CMT_MODE_SCAN)
	{
		cmt_scan_tick();
		return;
	}
	if(s_mode == CMT_MODE_SPEC)
	{
		cmt_spec_tick();
		return;
	}
	if(!s_rxRunning)
		return;
	if(s_mode == CMT_MODE_SPY)
	{
		cmt_spy_tick();
		return;
	}
	if(!s_txState)
		cmt_vs_tick();
	if(cmt_tx_tick())
		return;
	while(s_tail != s_head && n < 128)
	{
		uint16_t e = s_ring[s_tail];

		s_tail = (uint16_t)((s_tail + 1) % CMT_RING);
		s_frameCount = 0;
		cmt_dec_feed((e >> 15) & 1, (int)(e & 0x7FFF) * s_periodInt);
		cmt_sens_feed((e >> 15) & 1, (int)(e & 0x7FFF) * s_periodInt);
		if(s_frameCount > 0)
		{
			cmt_on_frame(&s_frames[0]);
			s_frameCount = 0;
		}
		n++;
	}
}

void CMT2300A_Stop(void)
{
	if(s_mode == CMT_MODE_SPEC)
		cmt_spec_stop("Treiber gestoppt");
	s_mode = CMT_MODE_RX;
	s_txqHead = s_txqTail = 0;
	if(s_txState)
	{
		s_txIsr = 1;
		if(s_txPinOut)
		{
			HAL_PIN_SetOutputValue(CMT_GPIO2, 0);
			HAL_PIN_Setup_Input(CMT_GPIO2);
			s_txPinOut = 0;
		}
		if(s_chipOk)
		{
			CMT2300A_GoStby();
			CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, s_txFifoSave);
		}
		s_txState = 0;
		cmt_tx_freebuf();
	}
	if(s_rxRunning && s_timerChan >= 0)
		HAL_HWTimerStop(s_timerChan);
	s_rxRunning = 0;
	s_txIsr = 0;
	if(s_chipOk)
		CMT2300A_GoStby();
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT2300A gestoppt");
}

/* ====================================================================
 *  Frequenz:  f = (N + K/2^20) * 26 MHz / DIV
 *  DIV steckt in RF4 Bits 6:4 (DIVX_CODE): DIV = (code+1)*2.
 *  GEPRUEFT: Code 1 = DIV 4 ergibt mit deiner Bank genau 433,92 MHz (TX).
 *  ANNAHME (ungeprueft): alle Codes 0..7 = DIV 2..16, VCO-Bereich s_vcoMin/MaxKhz.
 *  Bisheriger Kommentar: Code 0 = DIV 2 fuer 840..1020 MHz. Das passt zur
 *  Rechnung (gleicher VCO-Bereich ~1,74 GHz wie bei 433 MHz), aber ich habe
 *  es nicht an Hardware verifiziert. VCO-Bank, PA und Anpassung bleiben wie
 *  in der Tuya-Bank (ausgelegt fuer 433 MHz).
 * ==================================================================== */
static uint64_t cmt_read_total(int tx)
{
	BYTE b = tx ? CMT2300A_CUS_RF5 : CMT2300A_CUS_RF1;
	uint64_t n = CMT2300A_ReadReg(b);
	uint64_t k = (uint64_t)CMT2300A_ReadReg((BYTE)(b + 1))
	           | ((uint64_t)CMT2300A_ReadReg((BYTE)(b + 2)) << 8)
	           | ((uint64_t)(CMT2300A_ReadReg((BYTE)(b + 3)) & 0x0F) << 16);
	return (n << 20) | k;
}

static void cmt_write_total(int tx, uint64_t t)
{
	BYTE b = tx ? CMT2300A_CUS_RF5 : CMT2300A_CUS_RF1;
	BYTE hi = CMT2300A_ReadReg((BYTE)(b + 3));

	CMT2300A_WriteReg(b,               (BYTE)((t >> 20) & 0xFF));
	CMT2300A_WriteReg((BYTE)(b + 1),   (BYTE)(t & 0xFF));
	CMT2300A_WriteReg((BYTE)(b + 2),   (BYTE)((t >> 8) & 0xFF));
	CMT2300A_WriteReg((BYTE)(b + 3),   (BYTE)((hi & 0xF0) | ((t >> 16) & 0x0F)));
}

static int cmt_cur_code(void) { return (CMT2300A_ReadReg(CMT2300A_CUS_RF4) >> 4) & 7; }
static uint32_t cmt_total_to_khz(uint64_t t, int code)
{
	return (uint32_t)(((t * 26000ULL) >> 20) / (uint32_t)((code + 1) * 2));
}
static uint32_t cmt_cur_khz(int tx) { return cmt_total_to_khz(cmt_read_total(tx), cmt_cur_code()); }

/* VCO-Bereich in kHz. 1680..2040 MHz deckt die bisher erprobten Baender ab
 * (420..510 und 840..1020 MHz); mit cmtvco erweiterbar, um zu testen, wo die PLL einrastet. */
static uint32_t s_vcoMinKhz = 1680000, s_vcoMaxKhz = 2040000;

/* Teiler-Code fuer eine Frequenz: VCO = f x DIV muss im Bereich liegen, DIV = (Code+1)*2.
 * Passen mehrere, gewinnt der VCO-Wert naechst der Bereichsmitte. -1 = nicht abstimmbar */
static int cmt_div_for(uint32_t khz)
{
	uint32_t center = (s_vcoMinKhz + s_vcoMaxKhz) / 2, bestD = 0xFFFFFFFFu, v, d;
	int c, best = -1;

	for(c = 0; c < 8; c++)
	{
		v = khz * (uint32_t)((c + 1) * 2);
		if(v < s_vcoMinKhz || v > s_vcoMaxKhz)
			continue;
		d = v > center ? v - center : center - v;
		if(d < bestD) { bestD = d; best = c; }
	}
	return best;
}

static uint64_t cmt_total_for(uint32_t khz, int code)
{
	return ((((uint64_t)khz * (uint32_t)((code + 1) * 2)) << 20) + 13000ULL) / 26000ULL;   /* gerundet */
}

static int cmt_calc(uint32_t khz, BYTE *code, uint64_t *total)
{
	int c = cmt_div_for(khz);

	if(c < 0)
		return 0;
	*code = (BYTE)c;
	*total = cmt_total_for(khz, c);
	return 1;
}

/* Register direkt setzen (Timer/Modus regelt der Aufrufer) */
static void cmt_apply_regs(uint64_t rx, uint64_t tx, BYTE code, int sleep)
{
	BYTE rf4 = CMT2300A_ReadReg(CMT2300A_CUS_RF4);

	s_txKhzNow = cmt_total_to_khz(tx, code);
	CMT2300A_GoStby();
	CMT2300A_WriteReg(CMT2300A_CUS_RF4, (BYTE)((rf4 & 0x8F) | ((code & 7) << 4)));
	cmt_write_total(0, rx);
	cmt_write_total(1, tx);
	if(sleep)
	{
		CMT2300A_GoSleep();      /* wie Hersteller: Konfiguration wirksam machen */
		CMT2300A_GoStby();
	}
	CMT2300A_GoRx();
}

static int cmt_tune_khz(uint32_t khz, int sleep)
{
	static int ofsValid;
	static int32_t rxOfsKhz;
	BYTE code, code2;
	uint64_t ntx, nrx;

	if(!cmt_calc(khz, &code, &ntx))
		return 0;
	if(!ofsValid)
	{
		rxOfsKhz = (int32_t)cmt_cur_khz(0) - (int32_t)cmt_cur_khz(1);   /* RX/TX-Abstand */
		ofsValid = 1;
	}
	/* RX mit demselben Teiler (der ZF-Abstand ist in kHz konstant) */
	nrx = cmt_total_for((uint32_t)((int32_t)khz + rxOfsKhz), code);
	(void)code2;
	cmt_apply_regs(nrx, ntx, code, sleep);
	return 1;
}

static int cmt_rssi(void) { return (int)CMT2300A_ReadReg(CMT2300A_CUS_RSSI_DBM); }

/* ====================================================================
 *  Frequenz-Scan (cooperativ, laeuft im Quick-Tick): faehrt Frequenzen ab
 *  und merkt sich je Frequenz den hoechsten RSSI. Waehrenddessen Sender
 *  druecken/halten.
 * ==================================================================== */
#define CMT_SCAN_MAXPTS 160
static struct
{
	uint32_t from, step, endMs;
	int n, idx, phase, sweeps;
	uint8_t mx[CMT_SCAN_MAXPTS];
	uint64_t oRx, oTx;
	BYTE oCode;
} sc;

static void cmt_save_orig(uint64_t *rx, uint64_t *tx, BYTE *code)
{
	*rx = cmt_read_total(0);
	*tx = cmt_read_total(1);
	*code = (BYTE)cmt_cur_code();
}

static void cmt_scan_finish(void)
{
	char buf[160];
	int i, p = 0, imax = 0, mn = 255, nsig = 0;
	uint64_t wsum = 0, fw = 0;

	cmt_apply_regs(sc.oRx, sc.oTx, sc.oCode, 1);
	cmt_rx_resume();
	s_mode = CMT_MODE_RX;

	for(i = 0; i < sc.n; i++)
	{
		if(sc.mx[i] > sc.mx[imax]) imax = i;
		if(sc.mx[i] && sc.mx[i] < mn) mn = sc.mx[i];
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN fertig: %d Durchlaeufe, %d Punkte, Schritt %u kHz", sc.sweeps, sc.n, (unsigned)sc.step);
	for(i = 0; i < sc.n; i++)
	{
		uint32_t f = sc.from + (uint32_t)i * sc.step;

		p += snprintf(buf + p, sizeof(buf) - p, "%u.%03u:%d ", (unsigned)(f / 1000), (unsigned)(f % 1000), sc.mx[i]);
		if((i % 8) == 7 || i == sc.n - 1)
		{
			ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN %s", buf);
			p = 0;
		}
	}
	if(sc.mx[imax] < mn + 12)
	{
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN: KEIN Signal gefunden (Maximum %d, Rauschen ~%d). Sender waehrend des Scans gedrueckt gehalten? Anderer Bereich/Modulation?",
			sc.mx[imax], mn);
		return;
	}
	for(i = 0; i < sc.n; i++)
	{
		if(sc.mx[i] + 6 >= sc.mx[imax])
		{
			uint32_t w = (uint32_t)(sc.mx[i] - (sc.mx[imax] - 6)) + 1;
			wsum += w;
			fw += (uint64_t)w * (sc.from + (uint32_t)i * sc.step);
			nsig++;
		}
	}
	{
		uint32_t fpk = sc.from + (uint32_t)imax * sc.step;
		uint32_t fc = wsum ? (uint32_t)(fw / wsum) : fpk;

		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN SIGNAL: Maximum %u.%03u MHz (RSSI %d, Rauschen ~%d), Schwerpunkt ca. %u.%03u MHz",
			(unsigned)(fpk / 1000), (unsigned)(fpk % 1000), sc.mx[imax], mn,
			(unsigned)(fc / 1000), (unsigned)(fc % 1000));
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN weiter mit: cmtspy %u.%02u", (unsigned)(fc / 1000), (unsigned)((fc % 1000) / 10));
	}
	/* weitere lokale Maxima */
	for(i = 1; i + 1 < sc.n; i++)
	{
		if(i != imax && sc.mx[i] >= mn + 12 && sc.mx[i] >= sc.mx[i - 1] && sc.mx[i] > sc.mx[i + 1])
		{
			uint32_t f = sc.from + (uint32_t)i * sc.step;
			ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN weiteres Maximum %u.%03u MHz (RSSI %d)", (unsigned)(f / 1000), (unsigned)(f % 1000), sc.mx[i]);
		}
	}
}

static void cmt_scan_tick(void)
{
	uint32_t now = rtos_get_time();
	int r;

	if(sc.phase == 0)
	{
		if((int32_t)(now - sc.endMs) >= 0)
		{
			cmt_scan_finish();
			return;
		}
		if(!cmt_tune_khz(sc.from + (uint32_t)sc.idx * sc.step, 1))
		{
			/* nicht abstimmbar: Punkt bleibt 0 */
			if(++sc.idx >= sc.n)
			{
				sc.idx = 0;
				sc.sweeps++;
			}
			return;
		}
		sc.phase = 1;
		return;
	}
	r = cmt_rssi();
	{
		int r2 = cmt_rssi();
		if(r2 > r) r = r2;
	}
	if(r > 255) r = 255;
	if(r > sc.mx[sc.idx]) sc.mx[sc.idx] = (uint8_t)r;
	if(++sc.idx >= sc.n)
	{
		sc.idx = 0;
		sc.sweeps++;
	}
	sc.phase = 0;
}

static int cmt_scan_start(uint32_t fromKhz, uint32_t toKhz, uint32_t stepKhz, int sec)
{
	BYTE c1, c2;
	uint64_t t;

	if(s_mode != CMT_MODE_RX || cmt_tx_busy())
		return -1;
	if(toKhz < fromKhz) { uint32_t x = fromKhz; fromKhz = toKhz; toKhz = x; }
	if(stepKhz < 25) stepKhz = 25;
	(void)c1; (void)c2; (void)t;
	if(fromKhz < 100000 || toKhz > 1100000)
		return -2;
	while(((toKhz - fromKhz) / stepKhz + 1) > CMT_SCAN_MAXPTS)
		stepKhz *= 2;
	if(sec < 2) sec = 2;
	if(sec > 120) sec = 120;

	memset(&sc, 0, sizeof(sc));
	sc.from = fromKhz;
	sc.step = stepKhz;
	sc.n = (int)((toKhz - fromKhz) / stepKhz) + 1;
	cmt_save_orig(&sc.oRx, &sc.oTx, &sc.oCode);
	sc.endMs = rtos_get_time() + (uint32_t)sec * 1000u;
	cmt_rx_pause();
	s_mode = CMT_MODE_SCAN;
	return sc.n;
}

/* ====================================================================
 *  Spektrum (cmtspec): faehrt einen Frequenzbereich dauernd ab und misst je
 *  Punkt den RSSI. dBm = Registerwert - 128 (wie die CMOSTEK-Bibliothek);
 *  Antenne und Anpassung sind fuer 433 MHz ausgelegt, die Werte sind also nur
 *  relativ belastbar. Pro Quick-Tick wird ein Block Punkte gemessen
 *  (hoechstens CMT_SPEC_BUDGET_MS). Ergebnis: letzter Durchlauf, Max-Hold
 *  seit Start, Mittelwert, Max seit der letzten MQTT-Meldung.
 * ==================================================================== */
#define CMT_SPEC_MAXPTS     400
#define CMT_SPEC_BUDGET_MS  8
#define CMT_SPEC_READS      8

typedef struct
{
	int running, n, idx, fast, pubSec;
	uint32_t from, step, sweeps, sweepMs, sweepT0, lastPub, pubSweeps;
	uint64_t oRx, oTx;
	BYTE oCode;
	uint8_t cur[CMT_SPEC_MAXPTS];
	uint8_t hold[CMT_SPEC_MAXPTS];
	uint8_t win[CMT_SPEC_MAXPTS];
	uint16_t avg[CMT_SPEC_MAXPTS];      /* Mittelwert x 16 */
} cmt_spec_t;

static cmt_spec_t *s_spec;              /* ca. 1 KB, erst bei cmtspec belegt */

static int cmt_spec_dbm(int raw) { return raw - 128; }

static int cmt_spec_point(uint32_t khz)
{
	int i, r, mx = 0;

	if(!cmt_tune_khz(khz, s_spec->fast ? 0 : 1))
		return 0;                        /* nicht abstimmbar -> Luecke */
	cmt_wait_state(CMT2300A_STA_RX, 5);
	(void)cmt_rssi();                    /* erster Wert nach dem Umstimmen verwerfen */
	for(i = 0; i < CMT_SPEC_READS; i++)
	{
		r = cmt_rssi();
		if(r > mx) mx = r;
	}
	return mx;
}

static void cmt_spec_publish(void)
{
	cmt_spec_t *s = s_spec;
	int i, pk = 0, noise = 255;
	uint32_t now = rtos_get_time();

	for(i = 0; i < s->n; i++)
	{
		if(s->win[i] > s->win[pk]) pk = i;
		if(s->avg[i] && s->avg[i] / 16 < noise) noise = s->avg[i] / 16;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSPEC Spitze %u.%03u MHz ca. %d dBm, Rauschen ca. %d dBm (%u Durchlaeufe, %u ms je Durchlauf)",
		(unsigned)((s->from + (uint32_t)pk * s->step) / 1000), (unsigned)((s->from + (uint32_t)pk * s->step) % 1000),
		cmt_spec_dbm(s->win[pk]), cmt_spec_dbm(noise), (unsigned)s->pubSweeps, (unsigned)s->sweepMs);
#if CMT_ENABLE_MQTT
	{
		int len = 220 + 2 * s->n, p;
		char *buf = (char *)malloc((size_t)len);

		if(buf)
		{
			p = snprintf(buf, len, "{\"RfSpectrum\":{\"From\":%u,\"Step\":%u,\"N\":%d,\"Sweeps\":%u,\"SweepMs\":%u,\"PeakKhz\":%u,\"PeakDbm\":%d,\"NoiseDbm\":%d,\"Max\":\"",
				(unsigned)s->from, (unsigned)s->step, s->n, (unsigned)s->pubSweeps, (unsigned)s->sweepMs,
				(unsigned)(s->from + (uint32_t)pk * s->step), cmt_spec_dbm(s->win[pk]), cmt_spec_dbm(noise));
			for(i = 0; i < s->n && p < len - 8; i++)
				p += snprintf(buf + p, len - p, "%02X", s->win[i]);
			snprintf(buf + p, len - p, "\"}}");
			MQTT_PublishMain_StringString("RfSpectrum", buf, 0);
			free(buf);
		}
	}
#endif
	memset(s->win, 0, sizeof(s->win));
	s->pubSweeps = 0;
	s->lastPub = now;
}

static void cmt_spec_tick(void)
{
	cmt_spec_t *s = s_spec;
	uint32_t t0 = rtos_get_time(), now;

	if(!s || !s->running)
	{
		s_mode = CMT_MODE_RX;
		return;
	}
	do
	{
		int r = cmt_spec_point(s->from + (uint32_t)s->idx * s->step);
		uint8_t v = (uint8_t)(r > 255 ? 255 : (r < 0 ? 0 : r));
		int i = s->idx;

		s->cur[i] = v;
		if(v > s->hold[i]) s->hold[i] = v;
		if(v > s->win[i]) s->win[i] = v;
		if(s->sweeps == 0 || v == 0)
			s->avg[i] = (uint16_t)(v * 16);
		else
			s->avg[i] = (uint16_t)((int)s->avg[i] + ((int)v * 16 - (int)s->avg[i]) / 8);
		if(++s->idx >= s->n)
		{
			s->idx = 0;
			s->sweeps++;
			s->pubSweeps++;
			now = rtos_get_time();
			s->sweepMs = now - s->sweepT0;
			s->sweepT0 = now;
		}
	} while((int)(rtos_get_time() - t0) < CMT_SPEC_BUDGET_MS);

	if(s->pubSec > 0 && s->pubSweeps > 0 && (rtos_get_time() - s->lastPub) >= (uint32_t)s->pubSec * 1000u)
		cmt_spec_publish();
}

static int cmt_spec_start(uint32_t fromKhz, uint32_t toKhz, uint32_t stepKhz, int pubSec, int fast)
{
	BYTE c1, c2;
	uint64_t t;

	if(s_mode != CMT_MODE_RX || cmt_tx_busy())
		return -1;
	if(toKhz < fromKhz) { uint32_t x = fromKhz; fromKhz = toKhz; toKhz = x; }
	if(stepKhz < 10) stepKhz = 10;
	(void)c1; (void)c2; (void)t;
	if(fromKhz < 100000 || toKhz > 1100000)
		return -2;
	if((toKhz - fromKhz) / stepKhz + 1 > CMT_SPEC_MAXPTS)
		stepKhz = (toKhz - fromKhz + CMT_SPEC_MAXPTS - 2) / (CMT_SPEC_MAXPTS - 1);
	if(!s_spec)
	{
		s_spec = (cmt_spec_t *)malloc(sizeof(cmt_spec_t));
		if(!s_spec)
			return -3;
	}
	memset(s_spec, 0, sizeof(cmt_spec_t));
	s_spec->from = fromKhz;
	s_spec->step = stepKhz;
	s_spec->n = (int)((toKhz - fromKhz) / stepKhz) + 1;
	if(s_spec->n < 2) s_spec->n = 2;
	s_spec->pubSec = pubSec < 0 ? 0 : pubSec;
	s_spec->fast = fast ? 1 : 0;
	cmt_save_orig(&s_spec->oRx, &s_spec->oTx, &s_spec->oCode);
	cmt_rx_pause();
	s_spec->sweepT0 = s_spec->lastPub = rtos_get_time();
	s_spec->running = 1;
	s_mode = CMT_MODE_SPEC;
	return s_spec->n;
}

static void cmt_spec_stop(const char *why)
{
	if(!s_spec || !s_spec->running)
		return;
	s_spec->running = 0;
	if(s_chipOk)
		cmt_apply_regs(s_spec->oRx, s_spec->oTx, s_spec->oCode, 1);
	cmt_rx_resume();
	s_mode = CMT_MODE_RX;
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSPEC %s nach %u Durchlaeufen, Empfang laeuft wieder", why, (unsigned)s_spec->sweeps);
}

/* SVG-Linie fuer die Hauptseite: which 0 = letzter Durchlauf, 1 = Max-Hold, 2 = Mittel.
 * Hoechstens 200 Bildpunkte (Maximum je Gruppe), nicht abstimmbare Punkte = Luecke. */
static int cmt_spec_val(int which, int i)
{
	return which == 0 ? s_spec->cur[i] : (which == 1 ? s_spec->hold[i] : s_spec->avg[i] / 16);
}

static void cmt_spec_svg_line(http_request_t *request, int which, const char *color)
{
	cmt_spec_t *s = s_spec;
	char buf[200];
	int bins = s->n > 200 ? 200 : s->n, b, p = 0, open = 0;

	for(b = 0; b < bins; b++)
	{
		int i0 = b * s->n / bins, i1 = (b + 1) * s->n / bins, i, v = 0, x, y;

		if(i1 <= i0) i1 = i0 + 1;
		for(i = i0; i < i1 && i < s->n; i++)
		{
			int w = cmt_spec_val(which, i);
			if(w > v) v = w;
		}
		if(v == 0)
		{
			if(open)
			{
				if(p) hprintf255(request, "%s", buf);
				hprintf255(request, "'/>");
				open = 0;
				p = 0;
			}
			continue;
		}
		if(!open)
		{
			hprintf255(request, "<polyline fill='none' stroke='%s' stroke-width='1.2' points='", color);
			open = 1;
		}
		x = bins > 1 ? b * 400 / (bins - 1) : 0;
		y = (-20 - cmt_spec_dbm(v)) * 120 / 110 + 5;
		if(y < 0) y = 0;
		if(y > 130) y = 130;
		p += snprintf(buf + p, sizeof(buf) - p, "%d,%d ", x, y);
		if(p > 170)
		{
			hprintf255(request, "%s", buf);
			p = 0;
		}
	}
	if(open)
	{
		if(p) hprintf255(request, "%s", buf);
		hprintf255(request, "'/>");
	}
}

/* ====================================================================
 *  Protokoll-Spy: wartet auf Signal (RSSI-Schwelle), zeichnet die Pulse
 *  auf, trennt Pakete an langen Luecken, clustert Pulsbreiten und decodiert
 *  PWM (EV1527/PT2262-aehnlich) bzw. PPM. Zeigt zusaetzlich Rohdaten.
 * ==================================================================== */
#define CMT_SPY_MAX 1000
/* nur waehrend cmtspy belegt (ein Block, ca. 8 KB) */
#define CMT_SPY_WORDS (3 * CMT_SPY_MAX + 2 * (CMT_SPY_MAX / 2 + 2))
static uint16_t *sp_mem;
static uint16_t *sp_e, *sp_us, *sp_hv, *sp_lv, *sp_tmp;

static int cmt_spy_alloc(void)
{
	sp_mem = (uint16_t *)malloc(CMT_SPY_WORDS * sizeof(uint16_t));
	if(!sp_mem)
		return 0;
	sp_e = sp_mem;
	sp_us = sp_e + CMT_SPY_MAX;
	sp_tmp = sp_us + CMT_SPY_MAX;
	sp_hv = sp_tmp + CMT_SPY_MAX;
	sp_lv = sp_hv + (CMT_SPY_MAX / 2 + 2);
	return 1;
}

static void cmt_spy_freebuf(void)
{
	if(sp_mem)
		free(sp_mem);
	sp_mem = sp_e = sp_us = sp_hv = sp_lv = sp_tmp = NULL;
}
static int sp_n;

static struct
{
	int state, thr, maxNoise, maxSeen, bursts, want, tuned;
	uint32_t t0, tEnd, tLast, tCap;
	uint64_t oRx, oTx;
	BYTE oCode;
} spy;

typedef struct { uint32_t sum; int n; } cmt_cl_t;

static void cmt_sort16(uint16_t *a, int n)
{
	int i, j;

	for(i = 1; i < n; i++)
	{
		uint16_t v = a[i];

		for(j = i - 1; j >= 0 && a[j] > v; j--)
			a[j + 1] = a[j];
		a[j + 1] = v;
	}
}

static int cmt_cluster(const uint16_t *v, int n, cmt_cl_t *out, int maxc)
{
	int i, k = 0;

	memcpy(sp_tmp, v, (size_t)n * sizeof(uint16_t));
	cmt_sort16(sp_tmp, n);
	for(i = 0; i < n; i++)
	{
		if(k == 0 || (k < maxc && sp_tmp[i] > (out[k - 1].sum / (uint32_t)out[k - 1].n) * 13 / 10 + 40))
		{
			out[k].sum = 0;
			out[k].n = 0;
			k++;
		}
		out[k - 1].sum += sp_tmp[i];
		out[k - 1].n++;
	}
	return k;
}

/* nur "echte" Cluster (nicht vereinzelte Ausreisser) in sig[] (Mittelwerte), Rueckgabe Anzahl */
static int cmt_sig(const cmt_cl_t *c, int k, int nTotal, int *sig, int maxs)
{
	int i, m = 0, minN = nTotal / 16;

	if(minN < 2) minN = 2;
	for(i = 0; i < k && m < maxs; i++)
		if(c[i].n >= minN)
			sig[m++] = (int)(c[i].sum / (uint32_t)c[i].n);
	return m;
}

static void cmt_fmt_cl(const cmt_cl_t *c, int k, char *buf, int sz)
{
	int i, p = 0;

	buf[0] = 0;
	for(i = 0; i < k && p < sz - 16; i++)
		p += snprintf(buf + p, sz - p, "%s%ux%d", i ? " " : "", (unsigned)(c[i].sum / (uint32_t)c[i].n), c[i].n);
}

static void cmt_spy_analyze(void)
{
	int n = sp_n, i, j, pkN = 0, pk, nl = 0, shown = 0;
	int pkS[8], pkE[8], gapUs[8];
	uint8_t first = (uint8_t)((sp_e[0] >> 15) & 1);
	uint32_t gapTh, med;
	char buf[200], b2[100], b3[100];

	if(n < 12)
	{
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY: nur %d Flanken, zu kurz zum Auswerten", n);
		return;
	}
	for(i = 0; i < n; i++)
	{
		uint32_t u = (uint32_t)(sp_e[i] & 0x7FFF) * (uint32_t)s_periodInt;
		sp_us[i] = (uint16_t)(u > 65535 ? 65535 : u);
	}
	/* Eintrag i hat Pegel first^(i&1); Auswertung beginnt mit einem High */
	for(i = 0; i < n; i++)
		if((first ^ (i & 1)) == 0)
			sp_lv[nl++] = sp_us[i];
	memcpy(sp_tmp, sp_lv, (size_t)nl * sizeof(uint16_t));
	cmt_sort16(sp_tmp, nl);
	med = nl ? sp_tmp[nl / 2] : 0;
	gapTh = med * 4;
	if(gapTh < 2500) gapTh = 2500;
	{
		/* besser: groesster haeufiger Low-Cluster, falls klar (>= 1,7x) ueber dem naechstkleineren
		 * (z.B. Sensor: Low 1000/2000 und Sync-Luecke 4000) */
		cmt_cl_t lc[6];
		int k = cmt_cluster(sp_lv, nl, lc, 6), a = -1, b = -1;

		for(i = k - 1; i >= 0 && b < 0; i--)
		{
			if(lc[i].n >= 2)
			{
				if(a < 0) a = i; else b = i;
			}
		}
		if(a >= 0 && b >= 0)
		{
			uint32_t ma = lc[a].sum / (uint32_t)lc[a].n, mb = lc[b].sum / (uint32_t)lc[b].n;

			if(ma * 10u >= mb * 17u)
				gapTh = (ma + mb) / 2;
		}
	}

	pkS[0] = first ? 0 : 1;
	for(i = pkS[0]; i < n && pkN < 8; i++)
	{
		if((first ^ (i & 1)) == 0 && sp_us[i] > gapTh)
		{
			pkE[pkN] = i;
			gapUs[pkN] = sp_us[i];
			pkN++;
			if(pkN < 8)
				pkS[pkN] = i + 1;
		}
	}
	if(pkN < 8 && n - pkS[pkN] >= 12)
	{
		pkE[pkN] = n;
		gapUs[pkN] = 0;
		pkN++;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY Burst: %d Flanken, Luecken-Schwelle %u us, %d Paket(e)", n, (unsigned)gapTh, pkN);

	/* Rohdaten */
	{
		int p = 0, cnt = 0;

		for(j = pkS[0]; j < n && cnt < 72; j++, cnt++)
		{
			p += snprintf(buf + p, sizeof(buf) - p, "%c%u ", (first ^ (j & 1)) ? '+' : '-', (unsigned)sp_us[j]);
			if((cnt % 12) == 11 || j == n - 1 || cnt == 71)
			{
				ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY RAW %s", buf);
				p = 0;
			}
		}
	}

	for(pk = 0; pk < pkN && shown < 5; pk++)
	{
		int a = pkS[pk], b = pkE[pk], nh = 0, nlw = 0, np, kh, kl, mod = 0, sh, sl;
		int sigH[3], sigL[3], nbits = 0;
		cmt_cl_t ch[6], cl[6];

		if(b - a < 12)
			continue;
		for(j = a; j < b; j++)
		{
			if(first ^ (j & 1)) sp_hv[nh++] = sp_us[j];
			else                sp_lv[nlw++] = sp_us[j];
		}
		np = nlw;                    /* Paare (High,Low); ein einzelner Rest-High = Sync-Puls */
		if(np < 4)
			continue;
		kh = cmt_cluster(sp_hv, np, ch, 6);
		kl = cmt_cluster(sp_lv, np, cl, 6);
		sh = cmt_sig(ch, kh, np, sigH, 3);
		sl = cmt_sig(cl, kl, np, sigL, 3);

		if(sh == 2 && sl >= 1 && sl <= 2)
		{
			uint32_t mean = 0;
			int ok = 1;

			for(j = 0; j < np; j++) mean += (uint32_t)sp_hv[j] + sp_lv[j];
			mean /= (uint32_t)np;
			for(j = 0; j < np; j++)
			{
				uint32_t sm = (uint32_t)sp_hv[j] + sp_lv[j];
				if(sm * 4 < mean * 3 || sm * 4 > mean * 5) { ok = 0; break; }
			}
			if(ok) mod = 1;           /* PWM: gleiche Periode, High-Breite codiert */
		}
		if(!mod && sh == 1 && sl == 2)
			mod = 2;                  /* PPM: fester High, Low-Breite codiert */

		cmt_fmt_cl(ch, kh, b2, sizeof(b2));
		cmt_fmt_cl(cl, kl, b3, sizeof(b3));
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY Paket %d: %d Paare, Gap danach %d us, Highs[%s] Lows[%s]%s",
			pk + 1, np, gapUs[pk], b2, b3, (nh > nlw) ? ", +Sync-Puls" : "");

		if(mod)
		{
			int thr = (mod == 1) ? (sigH[0] + sigH[1]) / 2 : (sigL[0] + sigL[1]) / 2;
			uint64_t v = 0, mask;
			int width;

			for(j = 0; j < np && nbits < 64; j++)
			{
				int bit = (mod == 1) ? (sp_hv[j] > thr) : (sp_lv[j] > thr);

				v = (v << 1) | (uint64_t)bit;
				nbits++;
			}
			{
				char bs[84];
				int q = 0;

				for(j = 0; j < nbits; j++)
				{
					bs[q++] = (char)('0' + (int)((v >> (nbits - 1 - j)) & 1));
					if((j & 3) == 3 && j != nbits - 1) bs[q++] = ' ';
				}
				bs[q] = 0;
				ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY Bits: %s", bs);
			}
			mask = (nbits >= 64) ? ~0ULL : ((1ULL << nbits) - 1ULL);
			width = (nbits + 3) / 4;
			if(mod == 1)
				ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY PWM (1 = langer High): %d Bit  Data=%0*llX (invertiert %0*llX)  [Sync=%d Low=%d High=%d]",
					nbits, width, (unsigned long long)v, width, (unsigned long long)(~v & mask), gapUs[pk], sigH[0], sigH[1]);
			else
				ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY PPM (1 = langer Low): %d Bit  Data=%0*llX (invertiert %0*llX)  [Sync=%d High=%d LowKurz=%d LowLang=%d]",
					nbits, width, (unsigned long long)v, width, (unsigned long long)(~v & mask), gapUs[pk], sigH[0], sigL[0], sigL[1]);
		}
		else
			ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY: Modulation nicht automatisch erkannt (mehrstufig/Manchester/FSK?). Siehe RAW und Cluster.");
		shown++;
	}
}

static void cmt_spy_finish(const char *why)
{
	if(spy.maxSeen < spy.thr)
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY %s: KEIN Signal ueber Schwelle (Schwelle %d, hoechster RSSI %d, Rauschen %d). Frequenz mit cmtscan suchen?",
			why, spy.thr, spy.maxSeen, spy.maxNoise);
	else
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY %s: %d Burst(s) ausgewertet (Schwelle %d, max RSSI %d)", why, spy.bursts, spy.thr, spy.maxSeen);
	if(spy.tuned)
	{
		cmt_rx_pause();
		cmt_apply_regs(spy.oRx, spy.oTx, spy.oCode, 1);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY: Frequenz wieder auf %u kHz (RX)", (unsigned)cmt_cur_khz(0));
	}
	cmt_rx_resume();
	s_mode = CMT_MODE_RX;
	cmt_spy_freebuf();
}

static void cmt_spy_drain(int keepLast)
{
	uint16_t e;

	while(cmt_ring_pop(&e))
	{
		if(sp_n < CMT_SPY_MAX)
			sp_e[sp_n++] = e;
	}
	if(keepLast && sp_n > 200)
	{
		memmove(sp_e, sp_e + sp_n - 40, 40 * sizeof(uint16_t));
		sp_n = 40;
	}
}

static void cmt_spy_tick(void)
{
	uint32_t now = rtos_get_time();
	int r = cmt_rssi();

	if(r > spy.maxSeen && spy.state != 0) spy.maxSeen = r;
	switch(spy.state)
	{
	case 0:                                   /* Grundrauschen messen (300 ms) */
		cmt_spy_drain(1);
		if(r > spy.maxNoise) spy.maxNoise = r;
		if((int32_t)(now - spy.t0) >= 300)
		{
			spy.thr = spy.maxNoise + 15;
			spy.maxSeen = spy.maxNoise;
			spy.state = 1;
			ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY bereit: Rauschen %d, Schwelle %d - jetzt Sender druecken", spy.maxNoise, spy.thr);
		}
		break;
	case 1:                                   /* auf Signal warten */
		cmt_spy_drain(1);
		if(r > spy.thr)
		{
			spy.state = 2;
			spy.tLast = spy.tCap = now;
		}
		else if((int32_t)(now - spy.tEnd) >= 0)
			cmt_spy_finish("Timeout");
		break;
	case 2:                                   /* aufzeichnen */
		cmt_spy_drain(0);
		if(r > spy.thr) spy.tLast = now;
		if((now - spy.tLast) > 60 || (now - spy.tCap) > 1500 || sp_n >= CMT_SPY_MAX)
		{
			cmt_spy_analyze();
			spy.bursts++;
			sp_n = 0;
			spy.state = (r > spy.thr) ? 3 : 1;      /* Signal laeuft noch: Ruhe abwarten, damit der naechste Burst nicht mitten drin beginnt */
			spy.tLast = now;
			if(spy.bursts >= spy.want)
				cmt_spy_finish("Fertig");
		}
		break;
	case 3:                                   /* auf Funkstille (100 ms) warten */
		cmt_spy_drain(0);
		sp_n = 0;
		if(r > spy.thr) spy.tLast = now;
		if((now - spy.tLast) > 100)
			spy.state = 1;
		else if((int32_t)(now - spy.tEnd) >= 0)
			cmt_spy_finish("Timeout");
		break;
	}
}

static int cmt_spy_start(uint32_t khz, int sec)
{
	BYTE c;
	uint64_t t;

	if(s_mode != CMT_MODE_RX || !s_rxRunning || cmt_tx_busy())
		return -1;
	if(khz && !cmt_calc(khz, &c, &t))
		return -2;
	if(!cmt_spy_alloc())
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "SPY: kein Speicher (%d Bytes)", (int)(CMT_SPY_WORDS * sizeof(uint16_t)));
		return -1;
	}
	memset(&spy, 0, sizeof(spy));
	cmt_save_orig(&spy.oRx, &spy.oTx, &spy.oCode);
	if(khz)
	{
		cmt_rx_pause();
		cmt_tune_khz(khz, 1);
		spy.tuned = 1;
		cmt_rx_resume();
	}
	if(sec < 3) sec = 3;
	if(sec > 300) sec = 300;
	spy.want = 2;
	spy.t0 = rtos_get_time();
	spy.tEnd = spy.t0 + (uint32_t)sec * 1000u;
	sp_n = 0;
	s_mode = CMT_MODE_SPY;
	return 0;
}

static void cmt_stop_special(void)
{
	if(s_mode == CMT_MODE_SCAN)
		cmt_scan_finish();
	else if(s_mode == CMT_MODE_SPY)
		cmt_spy_finish("Abbruch");
	else if(s_mode == CMT_MODE_SPEC)
		cmt_spec_stop("Abbruch");
}

/* ====================================================================
 *  Diagnose
 * ==================================================================== */
static void cmt_print_info(void)
{
	BYTE pkt1 = CMT2300A_ReadReg(CMT2300A_CUS_PKT1);

	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT state=%02X MODE_STA=%02X IO_SEL=%02X INT1=%02X INT2=%02X",
		CMT2300A_GetChipStatus(), CMT2300A_ReadReg(CMT2300A_CUS_MODE_STA),
		CMT2300A_ReadReg(CMT2300A_CUS_IO_SEL),
		CMT2300A_ReadReg(CMT2300A_CUS_INT1_CTL), CMT2300A_ReadReg(CMT2300A_CUS_INT2_CTL));
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT PKT1=%02X data_mode=%s RF8=%02X SYS10=%02X FIFO_CTL=%02X",
		pkt1,
		((pkt1 & CMT2300A_MASK_DATA_MODE) == CMT2300A_DATA_MODE_DIRECT) ? "DIRECT" : "PACKET/OTHER",
		CMT2300A_ReadReg(CMT2300A_CUS_RF8), CMT2300A_ReadReg(CMT2300A_CUS_SYS10),
		CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL));
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Frequenz RX=%u kHz TX=%u kHz",
		(unsigned)cmt_cur_khz(0), (unsigned)cmt_cur_khz(1));
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT RX %s period=%dus frames=%u reports=%u ringDrops=%u last=%06X (Sync=%d T0=%d/%d T1=%d/%d %s)",
		s_rxRunning ? "an" : "aus", s_periodInt, (unsigned)s_statFrames, (unsigned)s_statReports,
		(unsigned)s_ringDrops, (unsigned)s_lastCode, s_lastSync, s_lastLow, s_lastL0, s_lastHigh, s_lastL1,
		s_lastMerged ? "ohne Sync-Puls" : "mit Sync-Puls");
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX %s gesendet=%u Fehler=%u Warteschlange=%d",
		s_txState ? "sendet" : "frei", (unsigned)s_txCount, (unsigned)s_txErr,
		(s_txqHead - s_txqTail + CMT_TXQ) % CMT_TXQ);
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Sensor Zeilen=%u Meldungen=%u", (unsigned)sens.rows, (unsigned)sens.reports);
	if(s_spec)
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Spektrum %s: %u..%u kHz, Schritt %u kHz, %d Punkte, %u Durchlaeufe, %u ms je Durchlauf, %s",
			s_spec->running ? "laeuft" : "angehalten", (unsigned)s_spec->from,
			(unsigned)(s_spec->from + (uint32_t)(s_spec->n - 1) * s_spec->step), (unsigned)s_spec->step,
			s_spec->n, (unsigned)s_spec->sweeps, (unsigned)s_spec->sweepMs, s_spec->fast ? "schnell" : "mit Sleep");
	{
		int i;
		char t[12];

		for(i = 0; i < CMT_VS_SLOTS; i++)
		{
			if(!s_vs[i].active)
				continue;
			cmt_fmt_tenth(t, sizeof(t), s_vs[i].tenth);
			ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT virt. Sensor Kanal=%d Id=%02X Temp=%s Feuchte=%d Batterie=%d Intervall=%d s, zuletzt vor %u s",
				i + 1, s_vs[i].id, t, s_vs[i].hum, s_vs[i].bat, s_vs[i].interval,
				(unsigned)((rtos_get_time() - s_vs[i].lastMs) / 1000u));
		}
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT RSSI_CODE=%02X RSSI_DBM=%d GPIO1=%d GPIO2=%d",
		CMT2300A_ReadReg(CMT2300A_CUS_RSSI_CODE),
		(int)CMT2300A_ReadReg(CMT2300A_CUS_RSSI_DBM),
		ty_read_gpio1_pin(), ty_read_gpio2_pin());
}

static void cmt_dump_regs(void)
{
	char buf[128];
	int a, p;

	for(a = 0; a <= 0x6F; a += 8)
	{
		p = snprintf(buf, sizeof(buf), "%02X:", a);
		for(int k = 0; k < 8 && (a + k) <= 0x6F; k++)
			p += snprintf(buf + p, sizeof(buf) - p, " %02X", CMT2300A_ReadReg((BYTE)(a + k)));
		ADDLOG_INFO(LOG_FEATURE_MAIN, "%s", buf);
	}
}

#if CMT_ENABLE_COMMANDS
static commandResult_t CMD_CmtInfo(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	if(!s_chipOk) return CMD_RES_ERROR;
	cmt_print_info();
	return CMD_RES_OK;
}

static commandResult_t CMD_CmtDump(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	if(!s_chipOk) return CMD_RES_ERROR;
	cmt_dump_regs();
	return CMD_RES_OK;
}

static commandResult_t CMD_CmtCap(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	int ms = 1500, clocked = 0;

	if(!s_chipOk || s_mode != CMT_MODE_RX || cmt_tx_busy()) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	if(Tokenizer_GetArgsCount() >= 1)
		ms = Tokenizer_GetArgInteger(0);
	if(Tokenizer_GetArgsCount() >= 2)
		clocked = Tokenizer_GetArgInteger(1);   /* 1 = DCLK-getaktet */
	{
		int was = s_rxRunning;
		if(was) cmt_rx_pause();
		cmt_capture(ms, clocked);
		if(was) cmt_rx_resume();
	}
	return CMD_RES_OK;
}

static commandResult_t CMD_CmtFreq(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	if(!s_chipOk) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	if(Tokenizer_GetArgsCount() >= 1)
	{
		uint32_t khz = (uint32_t)(atof(Tokenizer_GetArg(0)) * 1000.0 + 0.5);
		int was = s_rxRunning;

		if(s_mode != CMT_MODE_RX || cmt_tx_busy())
			return CMD_RES_ERROR;
		if(was) cmt_rx_pause();
		if(!cmt_tune_khz(khz, 1))
		{
			if(was) cmt_rx_resume();
			ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: Frequenz nicht abstimmbar (VCO-Bereich, siehe cmtvco)");
			return CMD_RES_BAD_ARGUMENT;
		}
		if(was) cmt_rx_resume();
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Frequenz RX=%u kHz TX=%u kHz", (unsigned)cmt_cur_khz(0), (unsigned)cmt_cur_khz(1));
	return CMD_RES_OK;
}

/* cmtscan [von_MHz] [bis_MHz] [Schritt_kHz] [Sekunden]   Standard: 433.0 434.8 100 15 */
static commandResult_t CMD_CmtScan(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	uint32_t from = 433000, to = 434800, step = 100;
	int sec = 15, n, argc;

	if(!s_chipOk) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	argc = Tokenizer_GetArgsCount();
	if(argc >= 1) from = (uint32_t)(atof(Tokenizer_GetArg(0)) * 1000.0 + 0.5);
	if(argc >= 2) to = (uint32_t)(atof(Tokenizer_GetArg(1)) * 1000.0 + 0.5);
	else if(argc == 1) to = from + 1800;
	if(argc >= 3) step = (uint32_t)Tokenizer_GetArgInteger(2);
	if(argc >= 4) sec = Tokenizer_GetArgInteger(3);
	n = cmt_scan_start(from, to, step, sec);
	if(n == -1) { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: beschaeftigt (cmtstop / Senden laeuft)"); return CMD_RES_ERROR; }
	if(n < 0)   { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: Bereich ungueltig (100..1100 MHz)"); return CMD_RES_BAD_ARGUMENT; }
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSCAN gestartet: %u..%u kHz, %d Punkte, %d s - Sender JETZT druecken/halten", (unsigned)from, (unsigned)to, n, sec);
	return CMD_RES_OK;
}

/* cmtspy [MHz] [Sekunden]   ohne MHz: aktuelle Frequenz */
static commandResult_t CMD_CmtSpy(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	uint32_t khz = 0;
	int sec = 30, r, argc;

	if(!s_chipOk) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	argc = Tokenizer_GetArgsCount();
	if(argc >= 1) khz = (uint32_t)(atof(Tokenizer_GetArg(0)) * 1000.0 + 0.5);
	if(argc >= 2) sec = Tokenizer_GetArgInteger(1);
	r = cmt_spy_start(khz, sec);
	if(r == -1) { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: beschaeftigt, Senden laeuft oder RX-Timer laeuft nicht (cmtstop)"); return CMD_RES_ERROR; }
	if(r < 0)   { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: Frequenz nicht abstimmbar (VCO-Bereich, siehe cmtvco)"); return CMD_RES_BAD_ARGUMENT; }
	ADDLOG_INFO(LOG_FEATURE_MAIN, "SPY gestartet auf %u kHz (RX), max %d s", (unsigned)cmt_cur_khz(0), sec);
	return CMD_RES_OK;
}

static commandResult_t CMD_CmtStop(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	cmt_stop_special();
	return CMD_RES_OK;
}

static commandResult_t CMD_CmtReg(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	BYTE addr;

	if(!s_chipOk) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	if(Tokenizer_GetArgsCount() < 1)
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	addr = (BYTE)strtol(Tokenizer_GetArg(0), NULL, 16);
	if(Tokenizer_GetArgsCount() >= 2)
	{
		BYTE val = (BYTE)strtol(Tokenizer_GetArg(1), NULL, 16);
		CMT2300A_WriteReg(addr, val);
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT reg %02X = %02X", addr, CMT2300A_ReadReg(addr));
	return CMD_RES_OK;
}

/* ---- Senden ---- */

/* Zeiger hinter "key": (Doppelpunkt, Leerzeichen, Anfuehrungszeichen uebersprungen) */
static const char *cmt_json_find(const char *s, const char *key)
{
	char pat[24];
	const char *p;

	snprintf(pat, sizeof(pat), "\"%s\"", key);
	p = strstr(s, pat);
	if(!p)
		return NULL;
	p += strlen(pat);
	while(*p == ' ' || *p == '\t' || *p == ':' || *p == '"')
		p++;
	return p;
}

static uint64_t cmt_parse_hex(const char *p)
{
	while(*p == ' ' || *p == '"' || *p == '#')
		p++;
	if(p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		p += 2;
	return (uint64_t)strtoull(p, NULL, 16);
}

static commandResult_t CMD_CmtSend(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	cmt_txreq_t r;
	const char *p;
	int maxRep;

	if(!s_chipOk) return CMD_RES_ERROR;
	if(!s_rxRunning)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: HW-Timer laeuft nicht, Senden nicht moeglich");
		return CMD_RES_ERROR;
	}
	memset(&r, 0, sizeof(r));
	r.bits = 24;
	r.repeat = CMT_TX_DEF_REPEAT;

	if(args && strchr(args, '{'))
	{
		/* JSON, z.B. {"Sync":8970,"Low":300,"High":850,"Data":"6B59A2"} oder das komplette RfReceived */
		p = cmt_json_find(args, "Data");
		if(!p)
		{
			ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: \"Data\" fehlt");
			return CMD_RES_BAD_ARGUMENT;
		}
		r.data = cmt_parse_hex(p);
		if((p = cmt_json_find(args, "Sync")))   r.syncUs = atoi(p);
		if((p = cmt_json_find(args, "Low")))    r.lowUs = atoi(p);
		if((p = cmt_json_find(args, "High")))   r.highUs = atoi(p);
		if((p = cmt_json_find(args, "Repeat"))) r.repeat = atoi(p);
		if((p = cmt_json_find(args, "Bits")))   r.bits = atoi(p);
		if((p = cmt_json_find(args, "T0H")))    r.t0h = atoi(p);
		if((p = cmt_json_find(args, "T0L")))    r.t0l = atoi(p);
		if((p = cmt_json_find(args, "T1H")))    r.t1h = atoi(p);
		if((p = cmt_json_find(args, "T1L")))    r.t1l = atoi(p);
		if((p = cmt_json_find(args, "Merged"))) r.merged = atoi(p) ? 1 : 0;
	}
	else
	{
		/* cmtsend <Data> [Wdh]  |  cmtsend <Data> <Sync> <Low> <High> [Wdh] [Bits] */
		int argc;

		Tokenizer_TokenizeString(args, 0);
		argc = Tokenizer_GetArgsCount();
		if(argc < 1)
			return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		if(argc == 3)
		{
			ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: cmtsend <Data> [Wdh] oder cmtsend <Data> <Sync> <Low> <High> [Wdh] [Bits]");
			return CMD_RES_BAD_ARGUMENT;
		}
		r.data = cmt_parse_hex(Tokenizer_GetArg(0));
		if(argc == 2)
			r.repeat = atoi(Tokenizer_GetArg(1));
		if(argc >= 4)
		{
			r.syncUs = atoi(Tokenizer_GetArg(1));
			r.lowUs = atoi(Tokenizer_GetArg(2));
			r.highUs = atoi(Tokenizer_GetArg(3));
		}
		if(argc >= 5) r.repeat = atoi(Tokenizer_GetArg(4));
		if(argc >= 6) r.bits = atoi(Tokenizer_GetArg(5));
	}

	if(r.bits < 1 || r.bits > 64)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Bits muss 1..64 sein");
		return CMD_RES_BAD_ARGUMENT;
	}
	if(r.bits < 64)
		r.data &= (1ULL << r.bits) - 1ULL;

	/* Gar keine Zeiten angegeben und Code = zuletzt empfangener: komplette Form uebernehmen
	 * (inkl. getrennter Pausen und "ohne Sync-Puls" bei der Klingel) */
	if(r.syncUs <= 0 && r.lowUs <= 0 && r.highUs <= 0 && r.t0h <= 0 && r.t1h <= 0
		&& s_lastSync > 0 && r.bits == 24 && r.data == (s_lastCode & 0xFFFFFF))
	{
		r.syncUs = s_lastSync;
		r.lowUs = s_lastLow;
		r.highUs = s_lastHigh;
		r.t0h = s_lastLow;
		r.t1h = s_lastHigh;
		r.t0l = s_lastL0;      /* 0 = unbekannt -> unten aus Low/High */
		r.t1l = s_lastL1;
		r.merged = s_lastMerged;
	}
	/* sonst fehlende Werte: Standard der Fernbedienung */
	if(r.syncUs <= 0) r.syncUs = CMT_TX_DEF_SYNC;
	if(r.lowUs <= 0)  r.lowUs  = (r.t0h > 0) ? r.t0h : CMT_TX_DEF_LOW;
	if(r.highUs <= 0) r.highUs = (r.t1h > 0) ? r.t1h : CMT_TX_DEF_HIGH;
	if(r.t0h <= 0) r.t0h = r.lowUs;
	if(r.t0l <= 0) r.t0l = r.highUs;
	if(r.t1h <= 0) r.t1h = r.highUs;
	if(r.t1l <= 0) r.t1l = r.lowUs;

	if(r.t0h < 100 || r.t0h > 5000 || r.t0l < 100 || r.t0l > 5000
		|| r.t1h < 100 || r.t1h > 5000 || r.t1l < 100 || r.t1l > 5000
		|| r.syncUs < 1000 || r.syncUs > 40000)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Zeiten unplausibel (Pulse/Pausen 100..5000 us, Sync 1000..40000 us)");
		return CMD_RES_BAD_ARGUMENT;
	}
	maxRep = (CMT_TX_MAX_ENTRIES - 2) / (2 * r.bits + 2);
	if(maxRep > CMT_TX_MAX_REPEAT) maxRep = CMT_TX_MAX_REPEAT;
	if(r.repeat < 1) r.repeat = 1;
	if(r.repeat > maxRep) r.repeat = maxRep;

	if(s_mode != CMT_MODE_RX)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Scan/Spy laeuft (cmtstop)");
		return CMD_RES_ERROR;
	}
	if(!cmt_tx_allowed(s_txKhzNow))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Senden auf %u kHz gesperrt (cmtfreq 433.92)", (unsigned)s_txKhzNow);
		return CMD_RES_ERROR;
	}
	if(!cmt_tx_enqueue(&r))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Warteschlange voll");
		return CMD_RES_ERROR;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX eingereiht: Data=%0*llX Bits=%d Sync=%d T0=%d/%d T1=%d/%d %s Wdh=%d",
		(r.bits + 3) / 4, (unsigned long long)r.data, r.bits, r.syncUs, r.t0h, r.t0l, r.t1h, r.t1l,
		r.merged ? "ohne Sync-Puls" : "mit Sync-Puls", r.repeat);
	return CMD_RES_OK;
}

/* cmtsensor <Kanal> <Temp> <Feuchte> [Id] [Batterie] [Intervall_s] | cmtsensor <Kanal> off | JSON */
static commandResult_t CMD_CmtSensor(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	int ch = 0, hum = -1, id = -1, bat = -1, interval = -1, haveTemp = 0, off = 0, tenth, i;
	double temp = 0.0;
	const char *p;
	char t[12];

	if(!s_chipOk) return CMD_RES_ERROR;
	if(args && strchr(args, '{'))
	{
		if((p = cmt_json_find(args, "Channel")))     ch = atoi(p);
		if((p = cmt_json_find(args, "Temperature"))) { temp = atof(p); haveTemp = 1; }
		if((p = cmt_json_find(args, "Humidity")))    hum = atoi(p);
		if((p = cmt_json_find(args, "Id")))          id = (int)cmt_parse_hex(p);
		if((p = cmt_json_find(args, "Battery")))     bat = atoi(p);
		if((p = cmt_json_find(args, "Interval")))    interval = atoi(p);
		if((p = cmt_json_find(args, "Off")))         off = atoi(p);
	}
	else
	{
		int argc;

		Tokenizer_TokenizeString(args, 0);
		argc = Tokenizer_GetArgsCount();
		if(argc < 2)
			return CMD_RES_NOT_ENOUGH_ARGUMENTS;
		ch = atoi(Tokenizer_GetArg(0));
		p = Tokenizer_GetArg(1);
		if(!strcmp(p, "off") || !strcmp(p, "aus") || !strcmp(p, "OFF"))
			off = 1;
		else
		{
			if(argc < 3)
				return CMD_RES_NOT_ENOUGH_ARGUMENTS;
			temp = atof(p);
			haveTemp = 1;
			hum = atoi(Tokenizer_GetArg(2));
			if(argc >= 4) id = (int)cmt_parse_hex(Tokenizer_GetArg(3));
			if(argc >= 5) bat = atoi(Tokenizer_GetArg(4));
			if(argc >= 6) interval = atoi(Tokenizer_GetArg(5));
		}
	}
	if(ch < 1 || ch > CMT_VS_SLOTS)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT Sensor: Kanal muss 1..%d sein", CMT_VS_SLOTS);
		return CMD_RES_BAD_ARGUMENT;
	}
	i = ch - 1;
	if(off)
	{
		s_vs[i].active = 0;
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Sensor Kanal %d abgeschaltet", ch);
		return CMD_RES_OK;
	}
	tenth = (int)(temp * 10.0 + (temp >= 0.0 ? 0.5 : -0.5));
	if(!haveTemp || tenth < -400 || tenth > 800 || hum < 0 || hum > 100)
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT Sensor: Temperatur -40..80 C und Feuchte 0..100 %% noetig");
		return CMD_RES_BAD_ARGUMENT;
	}
	if(!s_vs[i].active)
	{
		/* neuer Kanal: Standardwerte, solange nichts angegeben ist */
		if(id < 0) id = (s_vs[i].id > 0) ? s_vs[i].id : CMT_VS_DEF_ID;
		if(bat < 0) bat = 1;
		if(interval < 0) interval = CMT_VS_DEF_INTERVAL;
	}
	if(id >= 0) s_vs[i].id = id & 0xFF;
	if(bat >= 0) s_vs[i].bat = bat ? 1 : 0;
	if(interval >= 0) s_vs[i].interval = interval > 3600 ? 3600 : interval;
	s_vs[i].tenth = tenth;
	s_vs[i].hum = hum;
	s_vs[i].active = 1;

	cmt_fmt_tenth(t, sizeof(t), tenth);
	if(s_rxRunning && s_mode == CMT_MODE_RX && cmt_vs_send(ch))
	{
		s_vs[i].lastMs = rtos_get_time();
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Sensor Kanal %d: Id=%02X Temp=%s Feuchte=%d Batterie=%d Intervall=%d s - wird gesendet",
			ch, s_vs[i].id, t, hum, s_vs[i].bat, s_vs[i].interval);
	}
	else
	{
		/* gerade belegt (Scan/Spy/Warteschlange): beim naechsten Tick nachholen */
		s_vs[i].lastMs = rtos_get_time() - (uint32_t)s_vs[i].interval * 1000u;
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Sensor Kanal %d gespeichert (Id=%02X Temp=%s Feuchte=%d), Senden %s",
			ch, s_vs[i].id, t, hum, s_vs[i].interval > 0 ? "folgt" : "nicht moeglich (Sender belegt, Intervall 0)");
	}
	return CMD_RES_OK;
}

/* cmtvco [min_MHz max_MHz]  VCO-Bereich anzeigen/setzen, abstimmbare Baender listen */
static commandResult_t CMD_CmtVco(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	int c;

	Tokenizer_TokenizeString(args, 0);
	if(Tokenizer_GetArgsCount() >= 2)
	{
		uint32_t mn = (uint32_t)(atof(Tokenizer_GetArg(0)) * 1000.0 + 0.5);
		uint32_t mx = (uint32_t)(atof(Tokenizer_GetArg(1)) * 1000.0 + 0.5);

		if(mn < 1000000 || mx > 3000000 || mn >= mx)
		{
			ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT: VCO-Bereich muss zwischen 1000 und 3000 MHz liegen (min < max)");
			return CMD_RES_BAD_ARGUMENT;
		}
		s_vcoMinKhz = mn;
		s_vcoMaxKhz = mx;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT VCO %u..%u MHz (Teiler = (Code+1)*2, nur Code 1 an Hardware geprueft)",
		(unsigned)(s_vcoMinKhz / 1000), (unsigned)(s_vcoMaxKhz / 1000));
	for(c = 7; c >= 0; c--)
	{
		uint32_t div = (uint32_t)((c + 1) * 2);
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT Band Code %d (Teiler %2u): %4u.%03u .. %4u.%03u MHz", c, (unsigned)div,
			(unsigned)(s_vcoMinKhz / div / 1000), (unsigned)(s_vcoMinKhz / div % 1000),
			(unsigned)(s_vcoMaxKhz / div / 1000), (unsigned)(s_vcoMaxKhz / div % 1000));
	}
	return CMD_RES_OK;
}

/* cmtspec [von bis [kHz [MQTT_s [schnell]]]] | cmtspec off | reset | clear */
static commandResult_t CMD_CmtSpec(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	uint32_t from = 433000, to = 434800, step = 25;
	int pub = 5, fast = 0, argc, n;
	const char *a0;

	if(!s_chipOk) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	argc = Tokenizer_GetArgsCount();
	a0 = argc >= 1 ? Tokenizer_GetArg(0) : "";
	if(!strcmp(a0, "off") || !strcmp(a0, "aus"))
	{
		cmt_spec_stop("angehalten");
		return CMD_RES_OK;
	}
	if(!strcmp(a0, "reset"))
	{
		if(s_spec)
		{
			memset(s_spec->hold, 0, sizeof(s_spec->hold));
			memset(s_spec->win, 0, sizeof(s_spec->win));
			s_spec->sweeps = 0;
			s_spec->pubSweeps = 0;
		}
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSPEC Max-Hold und Mittel geloescht");
		return CMD_RES_OK;
	}
	if(!strcmp(a0, "clear"))
	{
		cmt_spec_stop("angehalten");
		if(s_spec)
			free(s_spec);
		s_spec = NULL;
		ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSPEC Daten freigegeben");
		return CMD_RES_OK;
	}
	if(argc >= 1) from = (uint32_t)(atof(Tokenizer_GetArg(0)) * 1000.0 + 0.5);
	if(argc >= 2) to = (uint32_t)(atof(Tokenizer_GetArg(1)) * 1000.0 + 0.5);
	else if(argc == 1) to = from + 1800;
	if(argc >= 3) step = (uint32_t)atoi(Tokenizer_GetArg(2));
	if(argc >= 4) pub = atoi(Tokenizer_GetArg(3));
	if(argc >= 5) fast = atoi(Tokenizer_GetArg(4));
	n = cmt_spec_start(from, to, step, pub, fast);
	if(n == -1) { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMTSPEC: beschaeftigt (cmtstop / Senden laeuft)"); return CMD_RES_ERROR; }
	if(n == -2) { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMTSPEC: Bereich ungueltig (100..1100 MHz)"); return CMD_RES_BAD_ARGUMENT; }
	if(n < 0)   { ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMTSPEC: kein Speicher (%d Bytes)", (int)sizeof(cmt_spec_t)); return CMD_RES_ERROR; }
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMTSPEC gestartet: %u..%u kHz, Schritt %u kHz, %d Punkte, MQTT alle %d s, %s - Empfang ruht (cmtspec off)",
		(unsigned)s_spec->from, (unsigned)(s_spec->from + (uint32_t)(n - 1) * s_spec->step), (unsigned)s_spec->step, n,
		s_spec->pubSec, s_spec->fast ? "schnell" : "mit Sleep");
	return CMD_RES_OK;
}

/* cmtcarrier [ms]  Testtraeger (fuer SDR/Leistungsmessung), Standard 1000 ms */
static commandResult_t CMD_CmtCarrier(const void *context, const char *cmd, const char *args, int cmdFlags)
{
	cmt_txreq_t r;
	int ms = 1000;

	if(!s_chipOk || !s_rxRunning || s_mode != CMT_MODE_RX) return CMD_RES_ERROR;
	Tokenizer_TokenizeString(args, 0);
	if(Tokenizer_GetArgsCount() >= 1)
		ms = Tokenizer_GetArgInteger(0);
	if(ms < 10) ms = 10;
	if(ms > 3000) ms = 3000;
	memset(&r, 0, sizeof(r));
	r.carrierMs = ms;
	if(!cmt_tx_allowed(s_txKhzNow))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Senden auf %u kHz gesperrt (cmtfreq 433.92)", (unsigned)s_txKhzNow);
		return CMD_RES_ERROR;
	}
	if(!cmt_tx_enqueue(&r))
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT TX: Warteschlange voll");
		return CMD_RES_ERROR;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT TX Traeger eingereiht: %d ms auf %u kHz", ms, (unsigned)cmt_cur_khz(1));
	return CMD_RES_OK;
}
#endif

/* ====================================================================
 *  Init (Einstiegspunkt, in drv_main.c eingetragen)
 * ==================================================================== */
void CMT2300A_SH4(void)
{
#if CMT_ENABLE_COMMANDS
	static int cmdsRegistered = 0;
#endif

	s_chipOk = 0;
	s_mode = CMT_MODE_RX;
	s_txIsr = 0;
	s_txState = 0;
	s_txPinOut = 0;
	s_txqHead = s_txqTail = 0;

	HAL_PIN_Setup_Output(CMT_CSB);
	HAL_PIN_Setup_Output(CMT_FCSB);
	HAL_PIN_Setup_Output(CMT_SCLK);
	HAL_PIN_Setup_Output(CMT_SDIO);
	/* WICHTIG: GPIO1/GPIO2 muessen als Eingang konfiguriert werden, sonst
	 * liest man konstant 0 (war im alten Code nie der Fall). */
	HAL_PIN_Setup_Input(CMT_GPIO1);
	HAL_PIN_Setup_Input(CMT_GPIO2);

	ty_set_csb_h();
	ty_set_fcsb_h();
	ty_set_sclk_l();
	ty_set_sdio_h();

	if(!CMT2300A_IsExist())
	{
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT2300A NOT FOUND");
		return;
	}
	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT2300A DETECTED");

	CMT2300A_Init();

	CMT2300A_ConfigRegBank(CMT2300A_CMT_BANK_ADDR,       g_cmt2300aCmtBank,      CMT2300A_CMT_BANK_SIZE);
	CMT2300A_ConfigRegBank(CMT2300A_SYSTEM_BANK_ADDR,    g_cmt2300aSystemBank,   CMT2300A_SYSTEM_BANK_SIZE);
	CMT2300A_ConfigRegBank(CMT2300A_FREQUENCY_BANK_ADDR, g_cmt2300aFrequencyBank, CMT2300A_FREQUENCY_BANK_SIZE);
	CMT2300A_ConfigRegBank(CMT2300A_DATA_RATE_BANK_ADDR, g_cmt2300aDataRateBank, CMT2300A_DATA_RATE_BANK_SIZE);
	CMT2300A_ConfigRegBank(CMT2300A_BASEBAND_BANK_ADDR,  g_cmt2300aBasebandBank, CMT2300A_BASEBAND_BANK_SIZE);
	CMT2300A_ConfigRegBank(CMT2300A_TX_BANK_ADDR,        g_cmt2300aTxBank,       CMT2300A_TX_BANK_SIZE);

	/* Ablauf wie in der Tuya-Referenz (radio.c: RF_Init / RF_Config) */
	/* xosc_aac_code[2:0] = 2 */
	CMT2300A_WriteReg(CMT2300A_CUS_CMT10,
		(BYTE)((CMT2300A_ReadReg(CMT2300A_CUS_CMT10) & ~0x07) | 0x02));

	CMT2300A_ConfigGpio();
	CMT2300A_ConfigInterrupt();
	CMT2300A_EnableInterrupt(CMT2300A_MASK_TX_DONE_EN | CMT2300A_MASK_PKT_DONE_EN);
	CMT2300A_EnableLfosc(FALSE);
	CMT2300A_EnableFifoMerge(TRUE);
	CMT2300A_SetFifoThreshold(32);

	/* Hersteller: "Go to sleep for configuration to take effect" */
	CMT2300A_GoSleep();
	CMT2300A_GoStby();

	CMT2300A_ConfigGpio();          /* GPIO1=DCLK, GPIO2=DOUT/DIN, GPIO3=INT2 */
	CMT2300A_ConfigInterrupt();
	CMT2300A_EnableReadFifo();
	CMT2300A_ClearInterruptFlags();
	CMT2300A_ClearRxFifo();

	s_chipOk = 1;
	s_txKhzNow = cmt_cur_khz(1);

	if(!CMT2300A_GoRx())
		ADDLOG_ERROR(LOG_FEATURE_MAIN, "CMT2300A: RX-Zustand nicht erreicht (state=%02X)", CMT2300A_GetChipStatus());

	cmt_rx_start();
	cmt_print_info();

#if CMT_ENABLE_COMMANDS
	if(!cmdsRegistered)
	{
		cmdsRegistered = 1;
		CMD_RegisterCommand("cmtinfo",    CMD_CmtInfo,    NULL);
		CMD_RegisterCommand("cmtdump",    CMD_CmtDump,    NULL);
		CMD_RegisterCommand("cmtreg",     CMD_CmtReg,     NULL);
		CMD_RegisterCommand("cmtcap",     CMD_CmtCap,     NULL);
		CMD_RegisterCommand("cmtfreq",    CMD_CmtFreq,    NULL);
		CMD_RegisterCommand("cmtscan",    CMD_CmtScan,    NULL);
		CMD_RegisterCommand("cmtspy",     CMD_CmtSpy,     NULL);
		CMD_RegisterCommand("cmtstop",    CMD_CmtStop,    NULL);
		CMD_RegisterCommand("cmtsend",    CMD_CmtSend,    NULL);
		CMD_RegisterCommand("cmtcarrier", CMD_CmtCarrier, NULL);
		CMD_RegisterCommand("cmtsensor",  CMD_CmtSensor,  NULL);
		CMD_RegisterCommand("cmtspec",    CMD_CmtSpec,    NULL);
		CMD_RegisterCommand("cmtvco",     CMD_CmtVco,     NULL);
	}
#endif

#if CMT_AUTOCAP_MS > 0
	cmt_capture(CMT_AUTOCAP_MS, 1);
#endif

	ADDLOG_INFO(LOG_FEATURE_MAIN, "CMT2300A started (RX, Direct Mode, Timer-ISR, TX bereit)");
}

/* ====================================================================
 *  Hauptseite: Spektrum als SVG (der Statusbereich wird laufend neu geladen)
 * ==================================================================== */
void CMT2300A_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
	cmt_spec_t *s = s_spec;
	uint32_t to, fpk;
	int i, pk = 0, d;

	if(bPreState || !s || s->sweeps == 0 || s->n < 2)
		return;
	for(i = 0; i < s->n; i++)
		if(s->hold[i] > s->hold[pk]) pk = i;
	to = s->from + (uint32_t)(s->n - 1) * s->step;
	fpk = s->from + (uint32_t)pk * s->step;
	hprintf255(request, "<h5>CMT2300A Spektrum %u.%03u&ndash;%u.%03u MHz, %u kHz Schritt, %u ms je Durchlauf%s</h5>",
		(unsigned)(s->from / 1000), (unsigned)(s->from % 1000), (unsigned)(to / 1000), (unsigned)(to % 1000),
		(unsigned)s->step, (unsigned)s->sweepMs, s->running ? "" : " (angehalten)");
	hprintf255(request, "<svg viewBox='0 0 400 142' style='width:100%%;max-width:640px;background:#111;display:block'>");
	for(d = 0; d <= 4; d++)
	{
		uint32_t f = s->from + (uint32_t)((uint64_t)(to - s->from) * (uint32_t)d / 4u);
		const char *anc = d == 0 ? "start" : (d == 4 ? "end" : "middle");

		if(to - s->from < 10000)
			hprintf255(request, "<text x='%d' y='140' fill='#888' font-size='7' text-anchor='%s'>%u.%03u</text>", d * 100, anc, (unsigned)(f / 1000), (unsigned)(f % 1000));
		else
			hprintf255(request, "<text x='%d' y='140' fill='#888' font-size='7' text-anchor='%s'>%u MHz</text>", d * 100, anc, (unsigned)((f + 500) / 1000));
	}
	for(d = -120; d <= -40; d += 20)
	{
		int y = (-20 - d) * 120 / 110 + 5;
		hprintf255(request, "<line x1='0' x2='400' y1='%d' y2='%d' stroke='#333' stroke-width='0.5'/><text x='2' y='%d' fill='#888' font-size='7'>%d dBm</text>", y, y, y - 1, d);
	}
	cmt_spec_svg_line(request, 2, "#4a9eff");
	cmt_spec_svg_line(request, 0, "#888888");
	cmt_spec_svg_line(request, 1, "#ff9f1a");
	hprintf255(request, "</svg><div style='font-size:12px'>orange = Max-Hold, blau = Mittel, grau = letzter Durchlauf &middot; Spitze %u.%03u MHz ca. %d dBm &middot; %u Durchl&auml;ufe</div>",
		(unsigned)(fpk / 1000), (unsigned)(fpk % 1000), cmt_spec_dbm(s->hold[pk]), (unsigned)s->sweeps);
}
