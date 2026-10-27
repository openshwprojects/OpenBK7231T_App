#include "../obk_config.h"
#if ENABLE_DRIVER_NEO6M
#include <stdint.h>
#include <stdbool.h>

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../libraries/obktime/obktime.h"
#include "drv_deviceclock.h"

/* --------------------------------------------------------------------------
 * Configuration / state
 * ---------------------------------------------------------------------- */

static unsigned short NEO6M_baudRate = 9600;

#define NEO6M_UART_RECEIVE_BUFFER_SIZE 512

// Fake lat/long for testing (overrides GPS values when set)
static char fakelat[12]  = {0};
static char fakelong[12] = {0};

static bool setclock2gps = false;
#if ENABLE_TIME_SUNRISE_SUNSET
static bool setlatlong2gps = false;
#endif

static uint8_t failedTries = 0;

// All parsed GPS fields in one place
typedef struct {
    int   H, M, S, SS;     // time
    int   DD, MM, YY;      // date
    float lat, lon;
    char  ns, ew;
    bool  locked;
} NEO6MState_t;

static NEO6MState_t s_gps;

// NMEA field indices for $GPRMC "sentence"
enum {
    NMEA_TIME,
    NMEA_LOCK,
    NMEA_LAT,
    NMEA_LAT_DIR,
    NMEA_LONG,
    NMEA_LONG_DIR,
    NMEA_SPEED,
    NMEA_COURSE,
    NMEA_DATE,
    NMEA_MAGVAR,
    NMEA_MAGVAR_DIR,
    NMEA_MODE,
    // parse stops at '*', so NMEA_STAR / NMEA_CHECKSUM not needed
    //NMEA_STAR,
    //NMEA_CHECKSUM,
    NMEA_WORDS
};

// some UART helpers

static void uart_write_bytes(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        UART_SendByte(buf[i]);
    }
}

static void uart_write_str(const char *s)
{
    uart_write_bytes((const uint8_t *)s, strlen(s));
}

// Send a NULL-terminated list of NMEA command strings
static void uart_send_nmea_list(const char * const *cmds)
{
    for (; *cmds; cmds++) {
        uart_write_str(*cmds);
    }
}

// NMEA / UBX command senders
/*
 * Disable unused NMEA sentences, keep only RMC.
 *
 * Full reference:
 *   $PUBX,40,GGA,0,0,0,0*5A  Disable GGA
 *   $PUBX,40,GLL,0,0,0,0*5C  Disable GLL
 *   $PUBX,40,GSA,0,0,0,0*4E  Disable GSA
 *   $PUBX,40,GSV,0,0,0,0*59  Disable GSV
 *   $PUBX,40,RMC,0,0,0,0*47  Disable RMC  (we keep it enabled: *46)
 *   $PUBX,40,VTG,0,0,0,0*5E  Disable VTG
 *   $PUBX,40,ZDA,0,0,0,0*44  Disable ZDA
 */
static void UART_WriteDisableNMEA(void)
{
    static const char * const cmds[] = {
        "$PUBX,40,GGA,0,0,0,0*5A\r\n",
        "$PUBX,40,GLL,0,0,0,0*5C\r\n",
        "$PUBX,40,GSA,0,0,0,0*4E\r\n",
        "$PUBX,40,GSV,0,0,0,0*59\r\n",
        "$PUBX,40,RMC,0,1,0,0*46\r\n",   /* keep RMC enabled */
        "$PUBX,40,VTG,0,0,0,0*5E\r\n",
        "$PUBX,40,ZDA,0,0,0,0*44\r\n",
        NULL
    };
    uart_send_nmea_list(cmds);
}

// Re-enable RMC (sent twice for reliability), used as recovery after too many failures
static void UART_WriteEnableRMC(void)
{
    static const char * const cmds[] = {
        "$PUBX,40,RMC,0,1,0,0*46\r\n",
        "$PUBX,40,RMC,0,1,0,0*46\r\n",
        NULL
    };
    uart_send_nmea_list(cmds);
}

/* UBX CFG-CFG: save current configuration to flash/BBR */
static void UART_Write_SAVE(void)
{
    static const uint8_t ubx_cfg_save[] = {
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x03, 0x1D, 0xAB
    };
    uart_write_bytes(ubx_cfg_save, sizeof(ubx_cfg_save));
}

// Poll for RMC data (currently unused — leave her for polling-mode experiments)
/*
static void UART_WritePollReq(void)
{
    static const char poll[] = "$EIGPQ,RMC*3A\r\n$EIGPQ,RMC*3A\r\n";
    uart_write_str(poll);
}
*/

// NMEA degrees+minutes -> decimal degrees
// Input:  NMEA value like 5213.00212  (ddmm.mmmmm)
// Output: decimal degrees like 52.216702
static float nmea_to_decimal(float nmea_val)
{
    int whole = (int)(nmea_val / 100);
    return (float)whole + (nmea_val - 100.0f * whole) / 60.0f;
}

// GPS sentence parser
//
// Data format: "$GPRMC,142953.00,A,5213.00212,N,02101.98018,E,0.105,,200724,,,A*71"
// Fields:       $GPRMC,HHMMSS[.SSS],A,BBBB.BBBB,b,LLLLL.LLLL,l,GG.G,RR.R,DDMMYY,M.M,m,F*PP

static void parseGPS(char *data)
{
    if (!data) {
        ADDLOG_INFO(LOG_FEATURE_DRV, "parseGPS: no data!");
        return;
    }

    char *p = strstr(data, "$GPRMC,");
    if (!p) {
        ADDLOG_INFO(LOG_FEATURE_DRV, "parseGPS: no $GPRMC found -- data=%s", data);
        failedTries++;
        return;
    }

    // Pre-seed checksum with XOR of "GPRMC," (= 103) so we can skip parsing that prefix.
    // strstr guarantees it is present in the string.
    int NMEACS = 'G'^'P'^'R'^'M'^'C'^',';
    int word = 0, letter = 0;
    char Nvalue[NMEA_WORDS][12] = {0};

    s_gps.locked = false;
    p += 7;     // advance past "$GPRMC,"

    while (p[0] != '*' && p[0] != '\0') {
        NMEACS ^= p[0];
        if (p[0] != ',') {
            if (letter < 11 && word < NMEA_WORDS) {
                Nvalue[word][letter++] = p[0];
            } else {
                ADDLOG_INFO(LOG_FEATURE_DRV, "parseGPS: field overflow! letter=%i word=%i data=%s", letter, word, data);
                return;
            }
        } else {
            Nvalue[word][letter] = '\0';
            word++;
            letter = 0;
        }
        p++;
    }

    // Verify checksum
    if (p[0] == '*') {
        p++;    // skip '*'
        char *endptr;
        int readcs = (int)strtol(p, &endptr, 16);
        if (p == endptr) {
            ADDLOG_WARN(LOG_FEATURE_DRV, "parseGPS: missing or unparseable checksum!");
        } else if (NMEACS != readcs) {
            ADDLOG_WARN(LOG_FEATURE_DRV, "parseGPS: checksum mismatch: calculated %02X, got %02X", NMEACS, readcs);
        }
    } else {
        ADDLOG_WARN(LOG_FEATURE_DRV, "parseGPS: sentence truncated, no '*' found");
    }

    // Parse time and date fields
    bool timeread = (sscanf(Nvalue[NMEA_TIME], "%2d%2d%2d.%d", &s_gps.H, &s_gps.M, &s_gps.S, &s_gps.SS) >= 3);
    bool dateread = (sscanf(Nvalue[NMEA_DATE], "%2d%2d%2d",    &s_gps.DD, &s_gps.MM, &s_gps.YY) == 3);
    s_gps.YY += 2000;

    s_gps.locked = (Nvalue[NMEA_LOCK][0] == 'A');
    if (!s_gps.locked) {
        ADDLOG_WARN(LOG_FEATURE_DRV, "parseGPS: no GPS lock! %s",
            timeread && dateread ? "Date/time might be valid." : "");
    }

    s_gps.ns = Nvalue[NMEA_LAT_DIR][0] ? Nvalue[NMEA_LAT_DIR][0] : '?';
    s_gps.ew = Nvalue[NMEA_LONG_DIR][0] ? Nvalue[NMEA_LONG_DIR][0] : '?';

    // Apply fake lat/long overrides if set (for testing)
    ADDLOG_DEBUG(LOG_FEATURE_DRV, "NEO6M: fakelat=%s Nvalue[NMEA_LAT]=%s", fakelat, Nvalue[NMEA_LAT]);
    if (*fakelat && strlen(fakelat) <= strlen(Nvalue[NMEA_LAT]))
        memcpy(Nvalue[NMEA_LAT], fakelat, strlen(fakelat));	// intentionally no '\0' copy -- overwrites digits only

    ADDLOG_DEBUG(LOG_FEATURE_DRV, "NEO6M: fakelong=%s Nvalue[NMEA_LONG]=%s", fakelong, Nvalue[NMEA_LONG]);
    if (*fakelong && strlen(fakelong) <= strlen(Nvalue[NMEA_LONG]))
        memcpy(Nvalue[NMEA_LONG], fakelong, strlen(fakelong));	// intentionally no '\0' copy -- overwrites digits only

    s_gps.lat = nmea_to_decimal(atof(Nvalue[NMEA_LAT]));
    s_gps.lon = nmea_to_decimal(atof(Nvalue[NMEA_LONG]));

    uint32_t epoch_time = dateToEpoch(s_gps.YY, s_gps.MM - 1, s_gps.DD, s_gps.H, s_gps.M, s_gps.S);

    // Apply GPS data to system clock / location if configured
    static char tempstr[50];
    tempstr[0] = '\0';

    if (setclock2gps && s_gps.locked && timeread && dateread) {
        TIME_setDeviceTime(epoch_time);
        strcat(tempstr, "(clock ");
    }
#if ENABLE_TIME_SUNRISE_SUNSET
    if (setlatlong2gps && s_gps.locked) {
        TIME_setLatitude(s_gps.lat);
        TIME_setLongitude(s_gps.lon);
        strcat(tempstr, tempstr[0] ? "and lat/long " : "(lat/long ");
    }
#endif
    if (tempstr[0]) strcat(tempstr, "set to GPS data)");

    ADDLOG_INFO(LOG_FEATURE_DRV,
        "Read GPS DATA:%02i.%02i.%i - %02i:%02i:%02i.%02i (epoch=%u) LAT=%f%c - LONG=%f%c  %s",
        s_gps.DD, s_gps.MM, s_gps.YY,
        s_gps.H,  s_gps.M,  s_gps.S, s_gps.SS,
        epoch_time, s_gps.lat, s_gps.ns, s_gps.lon, s_gps.ew,
        tempstr);
}

// UART packet handling
static void UART_GetNextPacket(void)
{
    char data[NEO6M_UART_RECEIVE_BUFFER_SIZE + 5] = {0};
    int cs = UART_GetDataSize();

    for (int i = 0; i < cs; i++) {
        data[i] = UART_GetByte(i);
    }
    data[cs] = '\0';

    UART_ConsumeBytes(cs);
    parseGPS(data);
}


// Called by 'startDriver NEO6M [options]'
// Options: setclock, setlatlong, fakelat=<val>, fakelong=<val>, savecfg, <baudrate>
void NEO6M_UART_Init(void)
{
    uint8_t     argc = Tokenizer_GetArgsCount() - 1;
    const char *arg;
    const char *fake;

    // Reset all config to defaults
    NEO6M_baudRate = 9600;
    setclock2gps   = false;
    fakelat[0]     = '\0';
    fakelong[0]    = '\0';
    bool savecfg   = false;
#if ENABLE_TIME_SUNRISE_SUNSET
    setlatlong2gps = false;
#endif

    for (int i = 1; i <= argc; i++) {
        arg = Tokenizer_GetArg(i);
        if (!arg) continue;

        ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: argument %i/%i is %s", i, argc, arg);

        if (!stricmp(arg, "setclock")) {
            setclock2gps = true;
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: will sync local clock to GPS UTC");

        } else if (!stricmp(arg, "setlatlong")) {
#if ENABLE_TIME_SUNRISE_SUNSET
            setlatlong2gps = true;
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: will sync lat/long from GPS");
#else
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: SUNRISE_SUNSET not enabled, ignoring 'setlatlong'");
#endif
        } else if ((fake = strstr(arg, "fakelat=")) != NULL) {
            strncpy(fakelat, fake + 8, sizeof(fakelat) - 1);
            fakelat[sizeof(fakelat) - 1] = '\0';
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: fakelat=%s", fakelat);

        } else if ((fake = strstr(arg, "fakelong=")) != NULL) {
            strncpy(fakelong, fake + 9, sizeof(fakelong) - 1);
            fakelong[sizeof(fakelong) - 1] = '\0';
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: fakelong=%s", fakelong);

        } else if (strstr(arg, "savecfg")) {
            savecfg = true;

        } else if (Tokenizer_IsArgInteger(i)) {
            NEO6M_baudRate = Tokenizer_GetArgInteger(i);
            ADDLOG_INFO(LOG_FEATURE_DRV, "NEO6M: baudrate set to %i", NEO6M_baudRate);
        }
    }

    UART_InitUART(NEO6M_baudRate, 0, 0);
    UART_InitReceiveRingBuffer(NEO6M_UART_RECEIVE_BUFFER_SIZE);
    UART_WriteDisableNMEA();
    if (savecfg) UART_Write_SAVE();
}

void NEO6M_UART_RunEverySecond(void)
{
    if (g_secondsElapsed % 5 == 0) {
        // Read and parse buffered data every 5 seconds
        UART_GetNextPacket();
    } else {
        // Flush buffer between read cycles so stale data does not accumulate
        UART_ConsumeBytes(UART_GetDataSize());
    }

    if (g_secondsElapsed % 5 == 4) {
        // One second before next read: if module seems silent, try re-enabling RMC
        if (failedTries >= 5) {
            UART_WriteEnableRMC();
            failedTries = 0;
        }
    }
}

void NEO6M_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState)
{
    if (bPreState)
        return;
    if (s_gps.locked)
        hprintf255(request,
            "<h5>GPS: %i-%02i-%02iT%02i:%02i:%02i Lat: %f%c Long: %f%c</h5>",
            s_gps.YY, s_gps.MM, s_gps.DD,
            s_gps.H,  s_gps.M,  s_gps.S,
            s_gps.lat, s_gps.ns, s_gps.lon, s_gps.ew);
}

#endif // ENABLE_DRIVER_NEO6M
