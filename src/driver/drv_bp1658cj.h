
// Thx to the work of https://github.com/arendst (Tasmota) for making the initial version of the driver
// This implementation is heavily based on the BP5758D implementation by openshwprojects

// Sadly I couldn't find any datasheet of this ic, so I sniffed the i2c protocol with a logic analyser
// I've been testing the implementation for a week now and it seems to be working without any issues.

#define BP1658CJ_ADDR_OUT     0xB0
#define BP1658CJ_ADDR_SLEEP   0x80  // When this command + the subadress and 00 for every channel is send, the device goes to sleep. --> was send by original firmware

// Sub Address
#define BP1658CJ_SUBADDR      0x23  //assumed: this address has to be send for a color change (never saw any different subadress)
