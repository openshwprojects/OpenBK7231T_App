/****************************************************************************
**
**  Name        data_types.h
**
**  Function    this file contains common data type definitions used
**              throughout the WIDCOMM Bluetooth code
**
**  Date       Modification
**  -----------------------
**  3/12/99    Create
**  07/27/00   Added nettohs macro for Little Endian
**
**  Copyright (c) 1999-2004, WIDCOMM Inc., All Rights Reserved.
**  Proprietary and confidential.
**
*****************************************************************************/

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>
#include <stdbool.h>


#ifndef NULL
#define NULL     0
#endif

#ifndef FALSE
#define FALSE  0
#endif


typedef unsigned char   uint8_t;

typedef uint32_t          TIME_STAMP;

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

typedef unsigned char   UBYTE;

#ifdef __arm
#define PACKED  __packed
#define INLINE  __inline
#else
#define PACKED
#define INLINE
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN FALSE
#endif
#define UINT16_LOW_BYTE(x)      ((x) & 0xff)
#define UINT16_HI_BYTE(x)       ((x) >> 8)


#define BCM_STRCPY_S(x1,x2,x3)      do{strcpy((x1),(x3)); __asm volatile("nop");__asm volatile("nop");__asm volatile("nop");}while(0)
#define BCM_STRNCPY_S(x1,x2,x3,x4)  do{strncpy((x1),(x3),(x4));__asm volatile("nop");__asm volatile("nop");__asm volatile("nop");} while(0)



#endif
