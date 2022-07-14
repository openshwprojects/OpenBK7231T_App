

#define SM2135_ADDR_MC      0xC0  // Max current register
#define SM2135_ADDR_CH      0xC1  // RGB or CW channel select register
#define SM2135_ADDR_R       0xC2  // Red color
#define SM2135_ADDR_G       0xC3  // Green color
#define SM2135_ADDR_B       0xC4  // Blue color
#define SM2135_ADDR_C       0xC5  // Cold
#define SM2135_ADDR_W       0xC6  // Warm

#define SM2135_RGB          0x00  // RGB channel
#define SM2135_CW           0x80  // CW channel (Chip default)

#define SM2135_10MA         0x00
#define SM2135_15MA         0x01
#define SM2135_20MA         0x02  // RGB max current (Chip default)
#define SM2135_25MA         0x03
#define SM2135_30MA         0x04  // CW max current (Chip default)
#define SM2135_35MA         0x05
#define SM2135_40MA         0x06
#define SM2135_45MA         0x07  // Max value for RGB
#define SM2135_50MA         0x08
#define SM2135_55MA         0x09
#define SM2135_60MA         0x0A

#define SM2135_DELAY         4