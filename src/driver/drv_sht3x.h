
// Add support for both SHT3X and SHT4X sensors
typedef enum {
    SHT3X_SENSOR,
    SHT4X_SENSOR
} sht_sensor_type_t;

// SHT4x I2C commands and parameters
#define SHT4X_I2C_ADDR           0x44  // I2C address for SHT4x
#define SHT4X_MEASURE_CMD        0xFD  // Measurement command for SHT4x
#define SHT4X_MEASURE_DURATION   10    // Measurement duration for SHT4x

#define SHT3X_DELAY
         4