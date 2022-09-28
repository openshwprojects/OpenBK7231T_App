
// Thx to the work of https://github.com/arendst (Tasmota) for making the initial version of the driver

// Layout: Bits B[7:8]=10 (address selection identification bits), B[5:6] sleep mode if set to 00, B[0:4] Address selection
#define BP1658CJ_ADDR_OUT     0xB0  // Device Adress
#define BP1658CJ_ADDR_SLEEP   0x80

// Sub Addr
#define BP1658CJ_SUBADDR 0x23 //assumed: this address has to be send for a color change

// Output enabled (OUT1-5, represented by lower 5 bits)
#define BP1658CJ_ENABLE_OUTPUTS_ALL 0x1F

// Current values: Bit 6 to 0 represent 30mA, 32mA, 16mA, 8mA, 4mA, 2mA, 1mA respectively
#define BP1658CJ_10MA 0x0A // 0 0001010
#define BP1658CJ_14MA 0x0E // 0 0001110
#define BP1658CJ_15MA 0x0F // 0 0001111
#define BP1658CJ_65MA 0x63 // 0 1100011
#define BP1658CJ_90MA 0x7C // 0 1111100
