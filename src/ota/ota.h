

// initialise OTA flash starting at startaddr 
int init_ota(unsigned int startaddr);

// add any length of data to OTA
void add_otadata(unsigned char *data, int len);

// finalise OTA flash (write last sector if incomplete)
int close_ota();
