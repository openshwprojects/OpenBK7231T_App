
// Thx to the work of https://github.com/arendst (Tasmota) for making the initial version of the driver

#define BP1658CJ_ADDR_OUT     0xB0  //
#define BP1658CJ_ADDR_SLEEP   0x80  // When this command and 00 for every channel is send the device goes to sleep --> was send by original firmware

// Sub Addr
#define BP1658CJ_SUBADDR 0x23 //assumed: this address has to be send for a color change (never saw any different subadress)
