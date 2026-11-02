// drv_veml7700.h
// VEML7700 High Accuracy Ambient Light Sensor driver
// Reference: Vishay VEML7700 Datasheet Rev. 1.8, 28-Nov-2024 (Doc 84286)
#ifndef DRV_VEML7700_H
#define DRV_VEML7700_H

// I2C address: 7-bit = 0x10, 8-bit wire = 0x20 write / 0x21 read
#define VEML7700_I2C_ADDR       (0x10 << 1)   // = 0x20

// Command codes (register addresses, datasheet p.7)
#define VEML7700_REG_ALS_CONF   0x00   // Configuration R/W
#define VEML7700_REG_ALS_WH     0x01   // High threshold window R/W
#define VEML7700_REG_ALS_WL     0x02   // Low threshold window  R/W
#define VEML7700_REG_PSM        0x03   // Power saving mode     R/W
#define VEML7700_REG_ALS        0x04   // ALS output data       R
#define VEML7700_REG_WHITE      0x05   // WHITE channel output  R
#define VEML7700_REG_ALS_INT    0x06   // Interrupt status      R
#define VEML7700_REG_ID         0x07   // Device ID             R

// ALS_CONF bit fields (register 0x00, datasheet Table 1)
#define VEML7700_CONF_SD        (1u << 0)   // Shutdown: 0=power on, 1=shutdown (POR default)
#define VEML7700_CONF_INT_EN    (1u << 1)   // Interrupt enable
// ALS_GAIN [12:11]
#define VEML7700_GAIN_x1        (0u << 11)
#define VEML7700_GAIN_x2        (1u << 11)
#define VEML7700_GAIN_x1_8      (2u << 11)
#define VEML7700_GAIN_x1_4      (3u << 11)
// ALS_IT [9:6]: integration time
#define VEML7700_IT_25MS        (0x0Cu << 6)
#define VEML7700_IT_50MS        (0x08u << 6)
#define VEML7700_IT_100MS       (0x00u << 6)  // default
#define VEML7700_IT_200MS       (0x01u << 6)
#define VEML7700_IT_400MS       (0x02u << 6)
#define VEML7700_IT_800MS       (0x03u << 6)

// Device ID (register 0x07, datasheet Table 8)
//   Low byte  = 0x81 (fixed device ID code)
//   High byte = 0xC4 for slave address 0x20
#define VEML7700_DEVICE_ID_LO   0x81

// Device state – one instance per sensor (matches shtxx_dev_t pattern)
typedef struct {
    softI2C_t i2c;
    int       channel_lux;    // output channel for lux*10 (-1 = unused)
    int       channel_white;  // output channel for raw WHITE count (-1 = unused)
    uint16_t  conf;           // shadow of ALS_CONF register
    uint16_t  rawALS;         // last raw ALS reading
    uint16_t  rawWhite;       // last raw WHITE reading
    float     lux;            // last calculated lux value
    bool      isWorking;
    uint8_t   secondsBetween;
    uint8_t   secondsUntilNext;
} veml7700_dev_t;

void VEML7700_Init(void);
void VEML7700_StopDriver(void);
void VEML7700_OnEverySecond(void);
void VEML7700_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#endif // DRV_VEML7700_H
