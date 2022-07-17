
///////////////////////////////////////////////////////////////////////////
// intent: To log to a RAM area, which is then sent to serial and/or TCP
//
//
// use ADDLOG_XXX(<feature>, fmt, vars....)
//
// to log DIRECT to serial for debugging,
// set direct_serial_log = 1
// or to use from boot, #define DEFAULT_DIRECT_SERIAL_LOG 1
///////////////////////////////////////////////////////////////////////////


#ifndef _OBK_LOGGING_H
#define _OBK_LOGGING_H

void addLogAdv(int level, int feature, char *fmt, ...);
void LOG_SetRawSocketCallback(int newFD);
int log_command(const void *context, const char *cmd, const char *args, int cmdFlags);

#define ADDLOG_ERROR(x, fmt, ...) addLogAdv(LOG_ERROR, x, fmt, ##__VA_ARGS__)
#define ADDLOG_WARN(x, fmt, ...)  addLogAdv(LOG_WARN, x, fmt, ##__VA_ARGS__)
#define ADDLOG_INFO(x, fmt, ...)  addLogAdv(LOG_INFO, x, fmt, ##__VA_ARGS__)
#define ADDLOG_DEBUG(x, fmt, ...) addLogAdv(LOG_DEBUG, x, fmt, ##__VA_ARGS__)
#define ADDLOG_EXTRADEBUG(x, fmt, ...) addLogAdv(LOG_EXTRADEBUG, x, fmt, ##__VA_ARGS__)

#define ADDLOGF_ERROR(fmt, ...) addLogAdv(LOG_ERROR, LOG_FEATURE, fmt, ##__VA_ARGS__)
#define ADDLOGF_WARN(fmt, ...)  addLogAdv(LOG_WARN, LOG_FEATURE, fmt, ##__VA_ARGS__)
#define ADDLOGF_INFO(fmt, ...)  addLogAdv(LOG_INFO, LOG_FEATURE, fmt, ##__VA_ARGS__)
#define ADDLOGF_DEBUG(fmt, ...) addLogAdv(LOG_DEBUG, LOG_FEATURE, fmt, ##__VA_ARGS__)
#define ADDLOGF_EXTRADEBUG(fmt, ...) addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE, fmt, ##__VA_ARGS__)


extern int loglevel;
extern char *loglevelnames[];

extern unsigned int logfeatures;
extern char *logfeaturenames[];

extern int direct_serial_log;

// set to 1 to use only direct serial logging at startup - eg for boot issues
#define DEFAULT_DIRECT_SERIAL_LOG 0
//#define DEFAULT_DIRECT_SERIAL_LOG 1

typedef enum {
    LOG_NONE = 0,
    LOG_ERROR = 1,
    LOG_WARN = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_EXTRADEBUG = 5,
    LOG_ALL = 6,
    LOG_MAX = 7
} log_levels;

typedef enum {
    LOG_FEATURE_HTTP            = 0,
    LOG_FEATURE_MQTT            = 1,
    LOG_FEATURE_CFG             = 2,
    LOG_FEATURE_HTTP_CLIENT     = 3,
    LOG_FEATURE_OTA             = 4,
    LOG_FEATURE_PINS            = 5,
    LOG_FEATURE_MAIN            = 6,
    LOG_FEATURE_GENERAL         = 7,
    LOG_FEATURE_API             = 8,
    LOG_FEATURE_LFS             = 9,
    LOG_FEATURE_CMD             = 10,
    LOG_FEATURE_NTP             = 11,
	LOG_FEATURE_TUYAMCU			= 12,
	LOG_FEATURE_I2C				= 13,
	LOG_FEATURE_ENERGYMETER		= 14,
	LOG_FEATURE_EVENT			= 15,
	LOG_FEATURE_DGR				= 16,
	// user to print without any prefixes
	LOG_FEATURE_RAW				= 17,
    // add in here - but also in names in logging.c
    LOG_FEATURE_MAX             = 18,
} log_features;

#endif
