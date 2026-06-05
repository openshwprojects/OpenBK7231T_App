
// Trying to narrow down Boozeman crash.
// Is the code with this define enabled crashing/freezing BK after few minutes for anybody?

#include "../new_common.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
#include "../littlefs/our_lfs.h"
#include "../new_cfg.h"	// we will use CFG_Set_log2lfs();
// uptime counter - seconds since boot to know how long to logg to LFS
extern int g_secondsElapsed;
#endif

#if PLATFORM_BEKEN
// to get uart.h
#include "command_line.h"
int UART_PORT = UART2_PORT;
int UART_PORT_INDEX = 1;
#endif
#if PLATFORM_BEKEN_NEW
#include "uart.h"
#include "arm_arch.h"
#undef PLATFORM_BEKEN
#elif PLATFORM_LN882H
#include "serial_hw.h"
#include "ln_kv_api.h"
#define KV_LOG_PORT "logport"
#elif PLATFORM_GD32VW553
#include "gd32vw55x.h"
#include "gd32vw55x_it.h"
#include "rom_export.h"
#include "uart_config.h"
#endif

extern uint8_t g_StartupDelayOver;


int g_loglevel = LOG_INFO; // default to info
unsigned int logfeatures = (
	(1 << 0) |
	(1 << 1) |
	(1 << 2) |
	(1 << 3) |
	(1 << 4) |
	(1 << 5) |
	(1 << 6) |
	(1 << 7) |
	(1 << 8) |
	(0 << 9) | // disable LFS by default
	(1 << 10) |
	(1 << 11) |
	(1 << 12) |
	(1 << 13) |
	(1 << 14) |
	(1 << 15) |
	(1 << 16) |
	(1 << 17) |
	(1 << 18) |
	(1 << 19) |
	(1 << 20) |
	(1 << 21) |
	(1 << 22) |
	(1 << 23) |
	(1 << 24) |
	(1 << 25)
	);
static int log_delay = 0;

// must match header definitions in logging.h
char* loglevelnames[] = {
	"NONE:",
	"Error:",
	"Warn:",
	"Info:",
	"Debug:",
	"ExtraDebug:",
	"All:"
};

// must match header definitions in logging.h
char* logfeaturenames[] = {
	"HTTP:",//            = 0,
	"MQTT:",//            = 1,
	"CFG:",//             = 2,
	"HTTP_CLIENT:",//     = 3,
	"OTA:",//             = 4,
	"PINS:",//            = 5,
	"MAIN:",//            = 6,
	"GEN:", //              = 7
	"API:", // = 8
	"LFS:", // = 9
	"CMD:", // = 10
	"NTP:", // = 11
	"TuyaMCU:",// = 12
	"I2C:",// = 13
	"EnergyMeter:",// = 14
	"EVENT:",// = 15
	"DGR:",// = 16
	"DDP:",// = 17
	"RAW:", // = 18 raw, without any prefix
	"HASS:", // = 19
	"IR:", // = 20
	"SENSOR:", // = 21
    "DRV:", // = 22,
	"BERRY:",// = 23,
	"ERROR",// = 24,
	"ERROR",// = 25,
	"ERROR",// = 26,
	"ERROR",// = 27,
    "ERROR",// = 28,
};

#define LOGGING_BUFFER_SIZE		1024

volatile int direct_serial_log = DEFAULT_DIRECT_SERIAL_LOG;

static int g_extraSocketToSendLOG = 0;
static char g_loggingBuffer[LOGGING_BUFFER_SIZE];

#define MAX_TCP_LOG_PORTS 2
int tcp_log_ports[MAX_TCP_LOG_PORTS] = {-1};


void LOG_SetRawSocketCallback(int newFD)
{
	g_extraSocketToSendLOG = newFD;
}

static int http_getlog(http_request_t* request);
static int http_getlograw(http_request_t* request);

static void log_server_thread(beken_thread_arg_t arg);
static void log_client_thread(beken_thread_arg_t arg);
static void log_serial_thread(beken_thread_arg_t arg);

static void startSerialLog();
static void startLogServer();

#define LOGSIZE 4096
#define LOGPORT 9000

int logTcpPort = LOGPORT;

static struct tag_logMemory {
	char log[LOGSIZE];
	int head;
	int tailserial;
	int tailtcp;
	int tailhttp;
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
	int taillfs;
#endif
	SemaphoreHandle_t mutex;
} logMemory;


static int initialised = 0;
static int tcpLogStarted = 0;

commandResult_t log_command(const void* context, const char* cmd, const char* args, int cmdFlags);

#if PLATFORM_BEKEN || PLATFORM_LN882H || PLATFORM_GD32VW553

commandResult_t log_port(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	int idx;

	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if(Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	idx = Tokenizer_GetArgInteger(0);
#if PLATFORM_BEKEN
	switch (idx)
	{
		case 1:
			UART_PORT = UART1_PORT;
			UART_PORT_INDEX = 0;
			break;
		case 2:
			UART_PORT = UART2_PORT;
			UART_PORT_INDEX = 1;
			break;
	}
#elif PLATFORM_LN882H
	serial_port_id_t port;
	switch(idx)
	{
		case 0:
			port = SER_PORT_UART0;
			break;
		case 1:
			port = SER_PORT_UART1;
			break;
		//case 2:
		//	port = SER_PORT_UART2;
		//	break;
		default:
			return CMD_RES_BAD_ARGUMENT;
	}

	ln_kv_set(KV_LOG_PORT, &port, sizeof(serial_port_id_t));
	log_deinit();
	log_init();
#elif PLATFORM_GD32VW553
	uint32_t usart_periph = LOG_UART;
	int baud = Tokenizer_GetArgIntegerDefault(1, 1500000);
	switch(idx)
	{
		case -1:
			usart_periph = LOG_UART;
			break;
		case 0:
			usart_periph = USART0;
			break;
		case 1:
			usart_periph = UART1;
			break;
		case 2:
			usart_periph = UART2;
			break;
	}
	uart_config(usart_periph, baud > 0 ? baud : 1500000, false, false, false);
	log_uart_change(usart_periph);
#endif

	return CMD_RES_OK;
}
#endif

// Here is how you can get log print on UART1:
/*
// Enable "[UART] Enable UART command line"
// this also can be done in flags, enable command line on UART1 at 115200 baud
SetFlag 31 1
// UART1 is RXD1/TXD1 which is used for programming and for TuyaMCU/BL0942,
// but now we will set that UART1 is used for log
logPort 1 
*/

#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
// forward declaration - we'll need function here
static void LOG_LFS_StartThread(const char *prefix);

static commandResult_t CMD_logStartup2lfs(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	uint8_t secs = (uint8_t)Tokenizer_GetArgIntegerDefault(0,10);
	if (secs > LOG2LFS_MAX_SECONDS) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "log2lfs: Seconds out of range. Set seconds to maximum value %i",LOG2LFS_MAX_SECONDS);
		secs = LOG2LFS_MAX_SECONDS;
	}
	uint8_t repeats = (uint8_t)Tokenizer_GetArgIntegerDefault(1,1);
	if (repeats > LOG2LFS_MAX_REPEATS) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "log2lfs: Repeats out of range. Set repeats to maximum value %i",LOG2LFS_MAX_REPEATS);
		repeats = LOG2LFS_MAX_REPEATS;
	}
	CFG_Set_log2lfs(LOG2LFS_ENCODE(secs,repeats));
	CFG_Save_IfThereArePendingChanges();
	return CMD_RES_OK;
}

void initLog2LFS(void){
	// since logging will be called beforeflash config is initialized, we can get
	// information, if we should log startup to LFS, not at init
	// so we need a separate function to set this up if logStartup2lfs is configureded
	// if so, g_log2lfs will hold the numbers of seconds to log to LFS
	if (g_log2lfs >0 && g_secondsElapsed < g_log2lfs) {
		LOG_LFS_StartThread("startupLog");
	}
}
#endif
static void initLog(void)
{
	bk_printf("Entering initLog()...\r\n");
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
	logMemory.head = logMemory.tailserial = logMemory.tailtcp = logMemory.tailhttp = logMemory.taillfs = 0;
#else
	logMemory.head = logMemory.tailserial = logMemory.tailtcp = logMemory.tailhttp = 0;
#endif
	logMemory.mutex = xSemaphoreCreateMutex();
	initialised = 1;
	startSerialLog();
	HTTP_RegisterCallback("/logs", HTTP_GET, http_getlog, 1);
	HTTP_RegisterCallback("/lograw", HTTP_GET, http_getlograw, 1);

	//cmddetail:{"name":"loglevel","args":"[Value]",
	//cmddetail:"descr":"Correct values are 0 to 7. Default is 3. Higher value includes more logs. Log levels are: ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4, EXTRADEBUG = 5. WARNING: you also must separately select logging level filter on web panel in order for more logs to show up there",
	//cmddetail:"fn":"log_command","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("loglevel", log_command, NULL);
	//cmddetail:{"name":"logfeature","args":"[Index][1or0]",
	//cmddetail:"descr":"set log feature filter, as an index and a 1 or 0",
	//cmddetail:"fn":"log_command","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("logfeature", log_command, NULL);
	//cmddetail:{"name":"logtype","args":"[TypeStr]",
	//cmddetail:"descr":"logtype direct|thread|none - type of serial logging - thread (in a thread; default), direct (logged directly to serial), none (no UART logging)",
	//cmddetail:"fn":"log_command","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("logtype", log_command, NULL);
	//cmddetail:{"name":"logdelay","args":"[Value]",
	//cmddetail:"descr":"Value is a number of ms. This will add an artificial delay in each log call. Useful for debugging. This way you can see step by step what happens.",
	//cmddetail:"fn":"log_command","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("logdelay", log_command, NULL);
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
	//cmddetail:{"name":"logStartup2lfs","args":"[Seconds - default: 10s] [repetitions - default 1]",
	//cmddetail:"descr":"Enable startup logging to LittleFS. Value is the number of seconds from boot during which log lines are appended to startupLog_N.txt on LFS. Set to 0 to disable.\nIf you want to log severals startups, use optional repeats argument",
	//cmddetail:"fn":"log_command","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":"logStartup2lfs 15"}
	CMD_RegisterCommand("logStartup2lfs", CMD_logStartup2lfs, NULL);
#endif
#if PLATFORM_BEKEN || PLATFORM_LN882H || PLATFORM_GD32VW553
	//cmddetail:{"name":"logport","args":"[Index]",
	//cmddetail:"descr":"Allows you to change log output port. On Beken, the UART1 is used for flashing and for TuyaMCU/BL0942, while UART2 is for log. Sometimes it might be easier for you to have log on UART1, so now you can just use this command like backlog uartInit 115200; logport 1 to enable logging on UART1..",
	//cmddetail:"fn":"log_port","file":"logging/logging.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("logport", log_port, NULL);
#endif

	bk_printf("Commands registered!\r\n");
	bk_printf("initLog() done!\r\n");
}

static void inittcplog(){
	startLogServer();
	tcpLogStarted = 1;
}
http_request_t *g_log_alsoPrintToHTTP = 0;
bool b_guard_recursivePrint = false;

void LOG_DeInit() {
	initialised = 0;
}
// all log printfs made by command will be sent also to request
void LOG_SetCommandHTTPRedirectReply(http_request_t* request) {
	g_log_alsoPrintToHTTP = request;
}



#ifdef PLATFORM_BEKEN
// run serial via timer thread.
	OSStatus OBK_rtos_callback_in_timer_thread( PendedFunction_t xFunctionToPend, void *pvParameter1, uint32_t ulParameter2, uint32_t delay_ms);
	void RunSerialLog();
	static void send_to_tcp();

	// called from timer thread
	volatile char log_timer_pended = 0;
	void log_timer_cb(void *a, uint32_t cmd){
		// we can pend another during this call...
		// so clear pended first
		log_timer_pended = 0;
		RunSerialLog();
		send_to_tcp();
	}

	void trigger_log_send(){
		// pend the function on timer thread.
		// as this is called form ISR, delay is ignored.
		// only allow one to pend at a time.
		if (!log_timer_pended){
			log_timer_pended = 1;
			OBK_rtos_callback_in_timer_thread( log_timer_cb, NULL, 0, 0);
		}
	}

	static int getSerial2();

	void RunSerialLog() {
		// send what we can.  if some remain, re-trigger send
		if (getSerial2()){
			trigger_log_send();
		}
	}
#endif

// adds a log to the log memory
// if head collides with either tail, move the tails on.
void addLogAdv(int level, int feature, const char* fmt, ...)
{
	char* tmp;
	char* t;
	int len;
	va_list argList;
	BaseType_t taken;
	int i;

	if (fmt == 0)
	{
		return;
	}
	if (!((1 << feature) & logfeatures)) {
		return;
	}
	if (level > g_loglevel) {
		return;
	}

	// if not initialised, direct output
	if (!initialised) {
		initLog();
	}
	if (g_StartupDelayOver && !tcpLogStarted){
		inittcplog();
	}


	taken = xSemaphoreTake(logMemory.mutex, 100);
	tmp = g_loggingBuffer;
	memset(tmp, 0, LOGGING_BUFFER_SIZE);
	t = tmp;

	if (feature == LOG_FEATURE_RAW)
	{
		// raw means no prefixes
	}
	else {
		strncpy(t, loglevelnames[level], (LOGGING_BUFFER_SIZE - (3 + t - tmp)));
		t += strlen(t);
		if (feature < sizeof(logfeaturenames) / sizeof(*logfeaturenames))
		{
			strncpy(t, logfeaturenames[feature], (LOGGING_BUFFER_SIZE - (3 + t - tmp)));
			t += strlen(t);
		}
	}

	va_start(argList, fmt);
	//vsnprintf3(t, (LOGGING_BUFFER_SIZE - (3 + t - tmp)), fmt, argList);
	//vsnprintf2(t, (LOGGING_BUFFER_SIZE - (3 + t - tmp)), fmt, argList);
	vsnprintf(t, (LOGGING_BUFFER_SIZE - (3 + t - tmp)), fmt, argList);
	va_end(argList);
	if (tmp[strlen(tmp) - 1] == '\n') tmp[strlen(tmp) - 1] = '\0';
	if (tmp[strlen(tmp) - 1] == '\r') tmp[strlen(tmp) - 1] = '\0';

	len = strlen(tmp); // save 3 bytes at end for /r/n/0
	tmp[len++] = '\r';
	tmp[len++] = '\n';
	tmp[len] = '\0';
#if WINDOWS
	printf("%s", tmp);
#endif
	// This is used by HTTP console
	if (g_log_alsoPrintToHTTP) {
		// guard here is used for the rare case when poststr attempts to do an addLogAdv as well
		if (b_guard_recursivePrint == false) {
			b_guard_recursivePrint = true;
			poststr(g_log_alsoPrintToHTTP, tmp);
			poststr(g_log_alsoPrintToHTTP, "<br>");
			b_guard_recursivePrint = false;
		}
	}
	if (g_extraSocketToSendLOG)
	{
		send(g_extraSocketToSendLOG, tmp, strlen(tmp), 0);
	}

	if (direct_serial_log == LOGTYPE_DIRECT) {
		bk_printf("%s", tmp);
		if (taken == pdTRUE) {
			xSemaphoreGive(logMemory.mutex);
		}
		/* no need to delay becasue bk_printf currently delays
		if (log_delay){
			if (log_delay < 0){
				int cps = (115200/8);
				timems = (1000*len)/cps;
			}
			rtos_delay_milliseconds(log_delay);
		}
		*/
		return;
	}

	for ( i = 0; i < len; i++)
	{
		logMemory.log[logMemory.head] = tmp[i];
		logMemory.head = (logMemory.head + 1) % LOGSIZE;
		if (logMemory.tailserial == logMemory.head)
		{
			logMemory.tailserial = (logMemory.tailserial + 1) % LOGSIZE;
		}
		if (logMemory.tailtcp == logMemory.head)
		{
			logMemory.tailtcp = (logMemory.tailtcp + 1) % LOGSIZE;
		}
		if (logMemory.tailhttp == logMemory.head)
		{
			logMemory.tailhttp = (logMemory.tailhttp + 1) % LOGSIZE;
		}
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS
		if (logMemory.taillfs == logMemory.head)
		{
			logMemory.taillfs = (logMemory.taillfs + 1) % LOGSIZE;
		}
#endif
	}

	if (taken == pdTRUE) {
		xSemaphoreGive(logMemory.mutex);
	}
#ifdef PLATFORM_BEKEN
	trigger_log_send();
#endif	
	if (log_delay != 0) 
    {
		int timems = log_delay;
		// is log_delay set -ve, then calculate delay
		// required for the number of characters to TX
		// plus 2ms to be sure.
		if (log_delay < 0) 
        {
			int cps = (115200 / 8);
			timems = (((1000 / portTICK_RATE_MS) * len) / cps) + 2;
            if (timems < 2)
                timems = 2;
		}
		rtos_delay_milliseconds(timems);
	}
}


static int getData(char* buff, int buffsize, int* tail) {
	BaseType_t taken;
	int count;
	char* p;
	if (!initialised)
		return 0;
	taken = xSemaphoreTake(logMemory.mutex, 100);
	if (taken == 0)
	{
		return 0;
	}

	count = 0;
	p = buff;
	while (buffsize > 1) {
		if (*tail == logMemory.head) {
			break;
		}
		*p = logMemory.log[*tail];
		p++;
		(*tail) = ((*tail) + 1) % LOGSIZE;
		buffsize--;
		count++;
	}
	*p = 0;

	if (taken == pdTRUE) {
		xSemaphoreGive(logMemory.mutex);
	}
	return count;
}

#if PLATFORM_BEKEN

// for T & N, we can send bytes if TX fifo is not full,
// and not wait.
// so in our thread, send until full, and never spin waiting to send...
// H/W TX fifo seems to be 256 bytes!!!
static int getSerial2() {
	if (!initialised) return 0;
	int* tail = &logMemory.tailserial;
	char c;
	BaseType_t taken = xSemaphoreTake(logMemory.mutex, 100);
	char overflow = 0;

	// if we hit overflow
	if (logMemory.tailserial == (logMemory.head + 1) % LOGSIZE) {
		overflow = 1;
	}

	while ((*tail != logMemory.head) && !uart_is_tx_fifo_full(UART_PORT)) {
		c = logMemory.log[*tail];
		if (overflow) {
			c = '^'; // replace the first char with ^ if we overflowed....
			overflow = 0;
		}

		(*tail) = ((*tail) + 1) % LOGSIZE;

		if (direct_serial_log == LOGTYPE_THREAD) {
			UART_WRITE_BYTE(UART_PORT_INDEX, c);
		}
	}

	int remains = (*tail != logMemory.head);

	if (taken == pdTRUE) {
		xSemaphoreGive(logMemory.mutex);
	}
	return remains;
}

#else

static int getSerial(char* buff, int buffsize) {
	int len = getData(buff, buffsize, &logMemory.tailserial);
	//bk_printf("got serial: %d:%s\r\n", len, buff);
	return len;
}

#endif


static int getTcp(char* buff, int buffsize) {
	int len = getData(buff, buffsize, &logMemory.tailtcp);
	//bk_printf("got tcp: %d:%s\r\n", len,buff);
	return len;
}

static int getHttp(char* buff, int buffsize) {
	int len = getData(buff, buffsize, &logMemory.tailhttp);
	//printf("got tcp: %d:%s\r\n", len,buff);
	return len;
}
#if ENABLE_LITTLEFS && ENABLE_LOG2LFS

// Startup log filename, chosen when the thread first opens the file.
static char g_lfsLogName[48] = {0};
static const char *g_lfsLogPrefix = "startupLog"; // default name

// Continuous-drain thread: mirrors log_serial_thread / log_client_thread.
// Waits for LFS to mount, then drains logMemory.taillfs into the file in
// small chunks until the capture window closes. Writing in a thread means
// flash erase/program never blocks the main system or IRQ watchdog.
// lfs_file_t is in a local struct on the heap to keep the thread stack lean.

// how much to write in one chunk
#define LOG2LFS_WRITE_CHUNK             128

// after sucesfull logging, set new value for the next start:
// set to 0, if this was the only/last logging configured
// else set new value for next start (decrease repeat count by 1)
void log2lfs_set_next_startup(){
	uint8_t act = CFG_Get_log2lfs();
	// shall we repeat logging?
	// then set value for next startup
	if (LOG2LFS_REPEATS(act) > 1) {
		CFG_Set_log2lfs(LOG2LFS_DECREASE_REPEAT(act));
	} else {
	// not repeating? Set to 0 for next startup
		CFG_Set_log2lfs(0);
	}
	ADDLOG_INFO(LOG_FEATURE_RAW, "log2lfs: Remaining logs to LFS: %i",LOG2LFS_REPEATS(act)-1);
}

// Just write to LFS until we finished writing ---
// or writing returns an error if we filled up the whole remaining LFS
static void log2lfs_thread(beken_thread_arg_t arg)
{
    char chunk[LOG2LFS_WRITE_CHUNK];
    lfs_file_t *lf = os_malloc(sizeof(lfs_file_t));
    int file_open = 0;
    int n;

    if (!lf) {
        ADDLOG_ERROR(LOG_FEATURE_RAW, "log2lfs: could not allocate lfs_file_t");
        goto done;
    }

    // Wait for LFS to become available before opening the file.
    while (!lfs_present()) {
// not needed if logging only on startup
//        if (g_secondsElapsed > log2lfs_end)
        if (g_secondsElapsed > g_log2lfs)
                    goto done; // logging window closed before LFS ever mounted
        rtos_delay_milliseconds(100);
    }

    int highest = -1;
    unsigned int cnt;
    // caution: we made cnt an unsigned int, so it won't accept "negative numbers" in sscanf (using %u instead of %d)
    // to compare them, make sure to cast cnt to int
    // else highest will be casted as unsigned, resulting in a huge number if it was negative!!!
    char t = '0';
    lfs_dir_t dir;
    struct lfs_info info;
    char scanfmt[32];
    // since we might have some "timestamp" files, dont use date as number - check after the number there's the '.' from ".txt"
    snprintf(scanfmt, sizeof(scanfmt), "%s_%%u%%ctxt", g_lfsLogPrefix);
    if (lfs_dir_open(&lfs, &dir, "/") >= 0) {
        while (lfs_dir_read(&lfs, &dir, &info) > 0) {
            t = '0';
            if (info.type == LFS_TYPE_REG &&
                sscanf(info.name, scanfmt, &cnt, &t) == 2 &&
                t == '.' &&
                (int)cnt > highest) {
                    highest = (int)cnt;
                }
        }
        lfs_dir_close(&lfs, &dir);
    }
    snprintf(g_lfsLogName, sizeof(g_lfsLogName), "%s_%d.txt", g_lfsLogPrefix, highest + 1);

    if (lfs_file_open(&lfs, lf, g_lfsLogName, LFS_O_RDWR | LFS_O_CREAT) < 0) {
        ADDLOG_ERROR(LOG_FEATURE_RAW, "log2lfs: could not open %s", g_lfsLogName);
        goto done;
    }
    file_open = 1;
    lfs_file_seek(&lfs, lf, 0, LFS_SEEK_END);
    ADDLOG_INFO(LOG_FEATURE_RAW, "log2lfs: writing to %s", g_lfsLogName);

    // Drain taillfs continuously until the capture window closes.
    while (g_secondsElapsed <= g_log2lfs) {
        while ((n = getData(chunk, sizeof(chunk), &logMemory.taillfs)) > 0) {
            if (lfs_file_write(&lfs, lf, chunk, n) < 0) {
                ADDLOG_ERROR(LOG_FEATURE_RAW, "log2lfs: write error, stopping");
                goto done;
            }
        }
        rtos_delay_milliseconds(200);
    }

    // Final drain: capture any lines logged in the last poll interval.
    while ((n = getData(chunk, sizeof(chunk), &logMemory.taillfs)) > 0)
        if (lfs_file_write(&lfs, lf, chunk, n) < 0)
            break;

done:
    if (file_open) {
        lfs_file_close(&lfs, lf);
        ADDLOG_INFO(LOG_FEATURE_RAW, "log2lfs: done, file closed");
    }
    os_free(lf);
    log2lfs_set_next_startup();
    g_log2lfs = 0;
    rtos_delete_thread(NULL);
}

static void LOG_LFS_StartThread(const char *prefix)
{
    if (rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
            "log2lfs",
            (beken_thread_function_t)log2lfs_thread,
            0x800,
            (beken_thread_arg_t)0) != kNoErr) {
        ADDLOG_ERROR(LOG_FEATURE_RAW, "log2lfs: could not create thread");
    } else {
        ADDLOG_DEBUG(LOG_FEATURE_RAW, "log2lfs: created thread");
    }
}

#endif // ENABLE_LITTLEFS && ENABLE_LOG2LFS

void startLogServer() {
#if WINDOWS

#else
	OSStatus err = kNoErr;

	err = rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
		"Log_server",
		(beken_thread_function_t)log_server_thread,
		0x800,
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		bk_printf("startLogServer: create \"TCP_server\" thread failed!\r\n");
	}
#endif
}

void startSerialLog() {
#if WINDOWS

#else

#ifndef PLATFORM_BEKEN
	OSStatus err = kNoErr;
	err = rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
		"log_serial",
		(beken_thread_function_t)log_serial_thread,
#if PLATFORM_BL_NEW
		0x1000,
#else
		0x800,
#endif
		(beken_thread_arg_t)0);
	if (err != kNoErr)
	{
		bk_printf("create \"log_serial\" thread failed!\r\n");
	}
#endif

#endif
}


/* TCP server listener thread */
void log_server_thread(beken_thread_arg_t arg)
{
	//(void)( arg );
	int tcp_select_fd;
	OSStatus err = kNoErr;
	struct sockaddr_in server_addr, client_addr;
	socklen_t sockaddr_t_size = sizeof(client_addr);
	char client_ip_str[16];
	int tcp_listen_fd = -1, client_fd = -1;
	fd_set readfds;

	tcp_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	tcp_select_fd = tcp_listen_fd + 1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;/* Accept conenction request on all network interface */
	server_addr.sin_port = htons(logTcpPort);/* Server listen on port: 20000 */
	err = bind(tcp_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	err = listen(tcp_listen_fd, 0);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(tcp_listen_fd, &readfds);

		select(tcp_select_fd, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(tcp_listen_fd, &readfds))
		{
			client_fd = accept(tcp_listen_fd, (struct sockaddr*)&client_addr, &sockaddr_t_size);
			if (client_fd >= 0)
			{
				strcpy(client_ip_str, inet_ntoa(client_addr.sin_addr));
#ifdef PLATFORM_BEKEN
				// Just note the new client port, if we have an available slot out of the two we record.
				int found_port_slot = 0;
				for (int i = 0; i < MAX_TCP_LOG_PORTS; i++) {
					if (tcp_log_ports[i] == -1){
						tcp_log_ports[i] = client_fd;
						found_port_slot = 1;
						break;
					}
				}
				if (!found_port_slot){
					close(client_fd);
					client_fd = -1;
				}
#else
				//addLog( "TCP Log Client %s:%d connected, fd: %d", client_ip_str, client_addr.sin_port, client_fd );
                if (kNoErr
                    != rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
                        "Logging TCP Client",
                        (beken_thread_function_t)log_client_thread,
                        0x800,
                        (beken_thread_arg_t)client_fd))
                {
					close(client_fd);
					client_fd = -1;
				}
#endif
			}
		}
	}

	if (err != kNoErr) {
		//addLog( "Server listener thread exit with err: %d", err );
	}

	close(tcp_listen_fd);
	rtos_delete_thread(NULL);
}

#define TCPLOGBUFSIZE 128
static char tcplogbuf[TCPLOGBUFSIZE];

#ifdef PLATFORM_BEKEN
static void send_to_tcp(){
	int i;
	for (i = 0; i < MAX_TCP_LOG_PORTS; i++){
		if (tcp_log_ports[i] >= 0){
			break;
		}
	}
	// no ports to send to
	if (i == MAX_TCP_LOG_PORTS){
		return;
	}
	int count;
	do {
		count = getTcp(tcplogbuf, TCPLOGBUFSIZE);
		if (count) {
			for (i = 0; i < MAX_TCP_LOG_PORTS; i++){
				if (tcp_log_ports[i] >= 0){
					int len = send(tcp_log_ports[i], tcplogbuf, count, 0);
					// if some error, close socket
					if (len != count) {
						// this is the only place this port can be closed.
						close(tcp_log_ports[i]);
						tcp_log_ports[i] = -1;
					}
				}
			}
		}
	} while(count);
}
#endif

// on beken, we trigger log send from timer thread
#ifndef PLATFORM_BEKEN

// non-beken
static void log_client_thread(beken_thread_arg_t arg)
{
	int fd = (int)arg;
	while (1) {
		int count = getTcp(tcplogbuf, TCPLOGBUFSIZE);
		if (count) {
			int len = send(fd, tcplogbuf, count, 0);
			// if some error, close socket
			if (len != count) {
				break;
			}
		}
		rtos_delay_milliseconds(10);
	}

	//addLog( "TCP client thread exit with err: %d", len );

	close(fd);
	rtos_delete_thread(NULL);
}



#define SERIALLOGBUFSIZE 128
static char seriallogbuf[SERIALLOGBUFSIZE];
static void log_serial_thread(beken_thread_arg_t arg)
{
	while (1) {
		int count = getSerial(seriallogbuf, SERIALLOGBUFSIZE);
		if (count) {
			if (direct_serial_log == LOGTYPE_THREAD) {
				bk_printf("%s", seriallogbuf);
			}
		}
		rtos_delay_milliseconds(10);
	}
}
#endif


static int http_getlograw(http_request_t* request) {
	int len = 0;
	http_setup(request, httpMimeTypeHTML);

	// get log in small chunks, posting on http
	do {
		char buf[128];
		len = getHttp(buf, sizeof(buf) - 1);
		buf[len] = '\0';
		if (len) {
			poststr(request, buf);
		}
	} while (len);
	poststr(request, NULL);
	return 0;
}

static int http_getlog(http_request_t* request) {
	char* post = "</pre>";
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Log");
	poststr(request, htmlFooterReturnToMainPage);

	poststr(request, "<pre>");

	http_getlograw(request);

	poststr(request, post);
	http_html_end(request);
	poststr(request, NULL);

	return 0;
}


commandResult_t log_command(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int result = 0;
	if (!cmd) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	if (!args) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	do {
		if (!stricmp(cmd, "loglevel")) {
			int res, level;
			res = sscanf(args, "%d", &level);
			if (res == 1) {
				if ((level >= 0) && (level <= 9)) {
					g_loglevel = level;
					result = CMD_RES_OK;
					ADDLOG_DEBUG(LOG_FEATURE_CMD, "loglevel set %d", level);
				}
				else {
					ADDLOG_ERROR(LOG_FEATURE_CMD, "loglevel %d out of range", level);
					result = CMD_RES_BAD_ARGUMENT;
				}
			}
			else {
				ADDLOG_ERROR(LOG_FEATURE_CMD, "loglevel '%s' invalid? current is %i", args, g_loglevel);
				result = CMD_RES_BAD_ARGUMENT;
			}
			break;
		}
		if (!stricmp(cmd, "logfeature")) {
			int res, feat;
			int val = 1;
			res = sscanf(args, "%d %d", &feat, &val);
			if (res >= 1) {
				if ((feat >= 0) && (feat < LOG_FEATURE_MAX)) {
					logfeatures &= ~(1 << feat);
					if (val) {
						logfeatures |= (1 << feat);
					}
					ADDLOG_DEBUG(LOG_FEATURE_CMD, "logfeature set 0x%08X", logfeatures);
					result = CMD_RES_OK;
				}
				else {
					ADDLOG_ERROR(LOG_FEATURE_CMD, "logfeature %d out of range", feat);
					result = CMD_RES_BAD_ARGUMENT;
				}
			}
			else {
				ADDLOG_ERROR(LOG_FEATURE_CMD, "logfeature %s invalid?", args);
				result = CMD_RES_BAD_ARGUMENT;
			}
			break;
		}
		if (!stricmp(cmd, "logtype")) {
			if (!stricmp(args, "none")) {
				direct_serial_log = LOGTYPE_NONE;
			}
			else if (!stricmp(args, "direct")) {
				direct_serial_log = LOGTYPE_DIRECT;
			}
			else {
				direct_serial_log = LOGTYPE_THREAD;
			}
			ADDLOG_ERROR(LOG_FEATURE_CMD, "logtype changed to %i", direct_serial_log);
			result = CMD_RES_OK;
			break;
		}
		if (!stricmp(cmd, "logdelay")) {
			int res, delay;
			res = sscanf(args, "%d", &delay);
			if (res == 1) {
				log_delay = delay;
			}
			else {
				log_delay = 0;
			}
			result = CMD_RES_OK;
			break;
		}
	} while (0);

	return result;
}

