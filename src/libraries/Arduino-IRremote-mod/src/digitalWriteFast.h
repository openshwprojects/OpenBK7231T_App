/*
 * digitalWriteFast.h
 *
 * Optimized digital functions for AVR microcontrollers
 * by Watterott electronic (www.watterott.com)
 * based on https://code.google.com/p/digitalwritefast
 *
 * License: BSD 3-Clause License (https://opensource.org/licenses/BSD-3-Clause)
 */

#ifndef __digitalWriteFast_h_
#define __digitalWriteFast_h_ 1

void digitalToggleFast(unsigned char P);
unsigned char digitalReadFast(unsigned char P);
void digitalWriteFast(unsigned char P, unsigned char V);
void pinModeFast(unsigned char P, unsigned char V);

#endif //__digitalWriteFast_h_


