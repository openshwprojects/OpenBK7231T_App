#define SGT_DELAY         4
#define SGT_INIT_DELAY         30

#define SGP30_PRODUCT_TYPE 0

/* command and constants for reading the serial ID */
#define SGP30_CMD_GET_SERIAL_ID 0x3682
#define SGP30_CMD_GET_SERIAL_ID_DURATION_US 500
#define SGP30_CMD_GET_SERIAL_ID_WORDS 3

/* command and constants for reading the featureset version */
#define SGP30_CMD_GET_FEATURESET 0x202f
#define SGP30_CMD_GET_FEATURESET_DURATION_US 10000
#define SGP30_CMD_GET_FEATURESET_WORDS 1

/* command and constants for on-chip self-test */
#define SGP30_CMD_MEASURE_TEST 0x2032
#define SGP30_CMD_MEASURE_TEST_DURATION_US 220000
#define SGP30_CMD_MEASURE_TEST_WORDS 1
#define SGP30_CMD_MEASURE_TEST_OK 0xd400

/* command and constants for IAQ init */
#define SGP30_CMD_IAQ_INIT 0x2003
#define SGP30_CMD_IAQ_INIT_DURATION_US 10000

/* command and constants for IAQ measure */
#define SGP30_CMD_IAQ_MEASURE 0x2008
#define SGP30_CMD_IAQ_MEASURE_DURATION_US 12000
#define SGP30_CMD_IAQ_MEASURE_WORDS 2

/* command and constants for getting IAQ baseline */
#define SGP30_CMD_GET_IAQ_BASELINE 0x2015
#define SGP30_CMD_GET_IAQ_BASELINE_DURATION_US 10000
#define SGP30_CMD_GET_IAQ_BASELINE_WORDS 2

/* command and constants for setting IAQ baseline */
#define SGP30_CMD_SET_IAQ_BASELINE 0x201e
#define SGP30_CMD_SET_IAQ_BASELINE_DURATION_US 10000

/* command and constants for raw measure */
#define SGP30_CMD_RAW_MEASURE 0x2050
#define SGP30_CMD_RAW_MEASURE_DURATION_US 25000
#define SGP30_CMD_RAW_MEASURE_WORDS 2

/* command and constants for setting absolute humidity */
#define SGP30_CMD_SET_ABSOLUTE_HUMIDITY 0x2061
#define SGP30_CMD_SET_ABSOLUTE_HUMIDITY_DURATION_US 10000

/* command and constants for getting TVOC inceptive baseline */
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE 0x20b3
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_DURATION_US 10000
#define SGP30_CMD_GET_TVOC_INCEPTIVE_BASELINE_WORDS 1

/* command and constants for setting TVOC baseline */
#define SGP30_CMD_SET_TVOC_BASELINE 0x2077
#define SGP30_CMD_SET_TVOC_BASELINE_DURATION_US 10000
