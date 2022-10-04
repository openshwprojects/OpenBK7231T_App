
// Thx to the work of https://github.com/arendst (Tasmota) for making the initial version of the driver

// Layout: Bits B[7:8]=10 (address selection identification bits), B[5:6] sleep mode if set to 00, B[0:4] Address selection
#define BP5758D_ADDR_SLEEP   0x80  //10 00 0110: Sleep mode bits set (OUT1-5 enable setup selected, ignored by chip)
#define BP5758D_ADDR_SETUP   0x90  //10 01 0000: OUT1-5 enable/disable setup - used during init
#define BP5758D_ADDR_OUT1_CR 0x91  //10 01 0001: OUT1 current range
#define BP5758D_ADDR_OUT2_CR 0x92  //10 01 0010: OUT2 current range
#define BP5758D_ADDR_OUT3_CR 0x93  //10 01 0011: OUT3 current range
#define BP5758D_ADDR_OUT4_CR 0x94  //10 01 0100: OUT4 current range
#define BP5758D_ADDR_OUT5_CR 0x95  //10 01 0101: OUT5 current range
#define BP5758D_ADDR_OUT1_GL 0x96  //10 01 0110: OUT1 gray-scale level
#define BP5758D_ADDR_OUT2_GL 0x98  //10 01 1000: OUT2 gray-scale level
#define BP5758D_ADDR_OUT3_GL 0x9A  //10 01 1010: OUT3 gray-scale level
#define BP5758D_ADDR_OUT4_GL 0x9C  //10 01 1100: OUT4 gray-scale level
#define BP5758D_ADDR_OUT5_GL 0x9E  //10 01 1110: OUT5 gray-scale level

// Output enabled (OUT1-5, represented by lower 5 bits)
#define BP5758D_ENABLE_OUTPUTS_ALL 0x1F
#define BP5758D_DISABLE_OUTPUTS_ALL 0x00

// Current values: Bit 6 to 0 represent 30mA, 32mA, 16mA, 8mA, 4mA, 2mA, 1mA respectively

#define BP5758D_2MA 0x02 // 0 0000010 // added by me	// decimal 2
#define BP5758D_5MA 0x05 // 0 0000101 // added by me	// decimal 5
#define BP5758D_8MA 0x08 // 0 0001000 // added by me	// decimal 8
#define BP5758D_10MA 0x0A // 0 0001010					// decimal 10
#define BP5758D_14MA 0x0E // 0 0001110					// decimal 14
#define BP5758D_15MA 0x0F // 0 0001111					// decimal 15
#define BP5758D_65MA 0x63 // 0 1100011					// decimal 99 // wtf?
#define BP5758D_90MA 0x7C // 0 1111100					// decimal 124 // wtf?


