/*
 * digitalWriteFast.h
 *
 * Optimized digital functions for AVR microcontrollers
 * by Watterott electronic (www.watterott.com)
 * based on https://code.google.com/p/digitalwritefast
 *
 * License: BSD 3-Clause License (https://opensource.org/licenses/BSD-3-Clause)
 */

/**
 * 
 * Emulation layer for the OpenBK7231T 
 * 
*/


#ifndef __digitalWriteFast_h_
#define __digitalWriteFast_h_ 1


#if PLATFORM_BEKEN
//TODO: check these?
typedef enum {
  LOW     = 0,
  HIGH    = 1,
  CHANGE  = 2,
  FALLING = 3,
  RISING  = 4,
} PinStatus;

typedef enum {
  INPUT           = 0x0,
  OUTPUT          = 0x1,
  INPUT_PULLUP    = 0x2,
  INPUT_PULLDOWN  = 0x3,
} PinMode;
#endif


void digitalToggleFast(unsigned char P);
unsigned char digitalReadFast(unsigned char P);
void digitalWriteFast(unsigned char P, unsigned char V);
void pinModeFast(unsigned char P, unsigned char V);

#endif //__digitalWriteFast_h_


