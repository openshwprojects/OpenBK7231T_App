

// NOTE: this offset was taken from BkDriverFlash.c
// search for BK_PARTITION_OTA

#if PLATFORM_BK7231T
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#elif PLATFORM_BK7231N
#define START_ADR_OF_BK_PARTITION_OTA 0x12A000
#else
// TODO
#define START_ADR_OF_BK_PARTITION_OTA 0x132000
#endif

// initialise OTA flash starting at startaddr 
int init_ota(unsigned int startaddr);

// add any length of data to OTA
void add_otadata(unsigned char *data, int len);

// finalise OTA flash (write last sector if incomplete)
void close_ota();

void otarequest(const char *urlin);