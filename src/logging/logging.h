
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

void addLog(char *fmt, ...);
void addLogAdv(int level, int feature, char *fmt, ...);

#define ADDLOG_ERROR(x, y, ...) addLogAdv(LOG_ERROR, x, y, ##__VA_ARGS__)
#define ADDLOG_WARN(x, y, ...)  addLogAdv(LOG_WARN, x, y, ##__VA_ARGS__)
#define ADDLOG_INFO(x, y, ...)  addLogAdv(LOG_INFO, x, y, ##__VA_ARGS__)
#define ADDLOG_DEBUG(x, y, ...) addLogAdv(LOG_DEBUG, x, y, ##__VA_ARGS__)

extern int loglevel;
extern char *loglevelnames[];

extern unsigned int logfeatures;
extern char *logfeaturenames[];

extern int direct_serial_log;

// set to 1 to use only direct serial logging at startup - eg for boot issues
#define DEFAULT_DIRECT_SERIAL_LOG 0
//#define DEFAULT_DIRECT_SERIAL_LOG 1

enum {
    LOG_NONE = 0,
    LOG_ERROR = 1,
    LOG_WARN = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_ALL = 5,
    LOG_MAX = 6
} log_levels;

enum {
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
    // add in here - but also in names in logging.c
    LOG_FEATURE_MAX             = 10,
} log_features;

