#pragma once
#include "../new_common.h"
#include "../hal/hal_generic.h"
#include "../hal/hal_pins.h"

#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"


#define OWtimeA	6
#define OWtimeB	64
#define OWtimeC	60
#define OWtimeD	10
#define OWtimeE	9
#define OWtimeF	55
#define OWtimeG	0
#define OWtimeH	480
#define OWtimeI	70
#define OWtimeJ	410


// some functions used in OneWire / DS18X20 code
// especially timing to implement an own "usleep()"
void OWusleep(int r);
void OWusleepshort(int r);
void OWusleepmed(int r);
void OWusleeplong(int r);

int OWReset(int Pin);

//-----------------------------------------------------------------------------
// Send a 1-Wire write bit. Provide 10us recovery time.
//-----------------------------------------------------------------------------
void OWWriteBit(int Pin, int bit);

//-----------------------------------------------------------------------------
// Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
//-----------------------------------------------------------------------------
int OWReadBit(int Pin);

//-----------------------------------------------------------------------------
// Write 1-Wire data byte
//-----------------------------------------------------------------------------
void OWWriteByte(int Pin, int data);

//-----------------------------------------------------------------------------
// Read 1-Wire data byte and return it
//-----------------------------------------------------------------------------
int OWReadByte(int Pin);

//-----------------------------------------------------------------------------
// Write a 1-Wire data byte and return the sampled result.
//-----------------------------------------------------------------------------
int OWTouchByte(int Pin, int data);

// quicker CRC with lookup table
// based on code found here: https://community.st.com/t5/stm32-mcus-security/use-stm32-crc-to-compare-ds18b20-crc/m-p/333749/highlight/true#M4690
// Dallas 1-Wire CRC Test App -
//  x^8 + x^5 + x^4 + 1 0x8C (0x131)
uint8_t Crc8CQuick(uint8_t* Buffer, uint8_t Size);



// ############################################ DS1820 commons ############################################

//-----------------------------------------------------------------------------
// Poll if DS1820 temperature conversion is complete
//-----------------------------------------------------------------------------
int DS1820TConversionDone(int Pin);


void register_testus();
