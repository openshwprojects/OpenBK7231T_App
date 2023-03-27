
// My MAX72XX old library
// See: https://www.elektroda.pl/rtvforum/viewtopic.php?p=18040628#18040628

typedef struct max72XX_s {
	byte port_clk, port_cs, port_mosi;
	byte maxDevices;
	unsigned char spidata[16];
	byte led_status[8*8];
} max72XX_t;

void MAX72XX_refresh(max72XX_t *led);
void MAX72XX_shutdown(max72XX_t *led, int addr, bool b);
void MAX72XX_setScanLimit(max72XX_t *led, int addr, int limit);
void MAX72XX_clearDisplay(max72XX_t *led,int addr) ;
void MAX72XX_rotate90CW(max72XX_t *led);
void MAX72XX_setIntensity(max72XX_t *led,int addr, int intensity) ;
void MAX72XX_displayArray(max72XX_t* led, byte *p, int devs, int ofs)  ;
void MAX72XX_shift(max72XX_t *led,int d);
void MAX72XX_setLed(max72XX_t *led,int addr, int row, int column, bool state) ;
void MAX72XX_init(max72XX_t *led);
void MAX72XX_setupPins(max72XX_t *led, int csi, int clki, int mosii, int maxD);
max72XX_t *MAX72XX_alloc();