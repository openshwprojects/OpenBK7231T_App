/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef _LINUX_BYTEORDER_GENERIC_H
#define _LINUX_BYTEORDER_GENERIC_H

/*
 * linux/byteorder_generic.h
 * Generic Byte-reordering support
 *
 * Francois-Rene Rideau <fare@tunes.org> 19970707
 *    gathered all the good ideas from all asm-foo/byteorder.h into one file,
 *    cleaned them up.
 *    I hope it is compliant with non-GCC compilers.
 *    I decided to put __BYTEORDER_HAS_U64__ in byteorder.h,
 *    because I wasn't sure it would be ok to put it in types.h
 *    Upgraded it to 2.1.43
 * Francois-Rene Rideau <fare@tunes.org> 19971012
 *    Upgraded it to 2.1.57
 *    to please Linus T., replaced huge #ifdef's between little/big endian
 *    by nestedly #include'd files.
 * Francois-Rene Rideau <fare@tunes.org> 19971205
 *    Made it to 2.1.71; now a facelift:
 *    Put files under include/linux/byteorder/
 *    Split swab from generic support.
 *
 * TODO:
 *   = Regular kernel maintainers could also replace all these manual
 *    byteswap macros that remain, disseminated among drivers,
 *    after some grep or the sources...
 *   = Linus might want to rename all these macros and files to fit his taste,
 *    to fit his personal naming scheme.
 *   = it seems that a few drivers would also appreciate
 *    nybble swapping support...
 *   = every architecture could add their byteswap macro in asm/byteorder.h
 *    see how some architectures already do (i386, alpha, ppc, etc)
 *   = cpu_to_beXX and beXX_to_cpu might some day need to be well
 *    distinguished throughout the kernel. This is not the case currently,
 *    since little endian, big endian, and pdp endian machines needn't it.
 *    But this might be the case for, say, a port of Linux to 20/21 bit
 *    architectures (and F21 Linux addict around?).
 */

/*
 * The following macros are to be defined by <asm/byteorder.h>:
 *
 * Conversion of long and short int between network and host format
 *  ntohl(__u32 x)
 *  ntohs(__u16 x)
 *  htonl(__u32 x)
 *  htons(__u16 x)
 * It seems that some programs (which? where? or perhaps a standard? POSIX?)
 * might like the above to be functions, not macros (why?).
 * if that's true, then detect them, and take measures.
 * Anyway, the measure is: define only ___ntohl as a macro instead,
 * and in a separate file, have
 * unsigned long inline ntohl(x){return ___ntohl(x);}
 *
 * The same for constant arguments
 *  __constant_ntohl(__u32 x)
 *  __constant_ntohs(__u16 x)
 *  __constant_htonl(__u32 x)
 *  __constant_htons(__u16 x)
 *
 * Conversion of XX-bit integers (16- 32- or 64-)
 * between native CPU format and little/big endian format
 * 64-bit stuff only defined for proper architectures
 *  cpu_to_[bl]eXX(__uXX x)
 *  [bl]eXX_to_cpu(__uXX x)
 *
 * The same, but takes a pointer to the value to convert
 *  cpu_to_[bl]eXXp(__uXX x)
 *  [bl]eXX_to_cpup(__uXX x)
 *
 * The same, but change in situ
 *  cpu_to_[bl]eXXs(__uXX x)
 *  [bl]eXX_to_cpus(__uXX x)
 *
 * See asm-foo/byteorder.h for examples of how to provide
 * architecture-optimized versions
 *
 */
#pragma once

#undef ntohl
#undef ntohs
#undef htonl
#undef htons


/** 16 bits byte swap */
#define swap_byte_16(x) \
    ((unsigned short)((((unsigned short)(x) & 0x00ffU) << 8) | \
                      (((unsigned short)(x) & 0xff00U) >> 8)))

/** 32 bits byte swap */
#define swap_byte_32(x) \
    ((unsigned int)((((unsigned int)(x) & 0x000000ffUL) << 24) | \
                    (((unsigned int)(x) & 0x0000ff00UL) <<  8) | \
                    (((unsigned int)(x) & 0x00ff0000UL) >>  8) | \
                    (((unsigned int)(x) & 0xff000000UL) >> 24)))

/** 64 bits byte swap */
#define swap_byte_64(x) \
    ((unsigned long long)((unsigned long long)(((unsigned long long)(x) & 0x00000000000000ffULL) << 56) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x000000000000ff00ULL) << 40) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x0000000000ff0000ULL) << 24) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x00000000ff000000ULL) <<  8) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x000000ff00000000ULL) >>  8) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x0000ff0000000000ULL) >> 24) | \
                          (unsigned long long)(((unsigned long long)(x) & 0x00ff000000000000ULL) >> 40) | \
                          (unsigned long long)(((unsigned long long)(x) & 0xff00000000000000ULL) >> 56) ))

#if BIG_ENDIAN
/** Convert ulong n/w to host */
#define ntohl(x) x
/** Convert host ulong to n/w */
#define htonl(x) x
/** Convert n/w to host */
#define ntohs(x)  x
/** Convert host to n/w */
#define htons(x)  x


/** Convert from 16 bit little endian format to CPU format */
#define le16_to_cpu(x) swap_byte_16(x)
/** Convert from 32 bit little endian format to CPU format */
#define le32_to_cpu(x) swap_byte_32(x)
/** Convert from 64 bit little endian format to CPU format */
#define le64_to_cpu(x) swap_byte_64(x)
/** Convert to 16 bit little endian format from CPU format */



#define cpu_to_le16(x) swap_byte_16(x)
/** Convert to 32 bit little endian format from CPU format */
#define cpu_to_le32(x) swap_byte_32(x)
/** Convert to 64 bit little endian format from CPU format */
#define cpu_to_le64(x) swap_byte_64(x)



#define cpu_to_be16(x) x
/** Convert to 32 bit big endian format from CPU format */
#define cpu_to_be32(x) x
/** Convert to 64 bit big endian format from CPU format */
#define cpu_to_be64(x) x

/** Convert from 16 bit big endian format to CPU format */
#define be16_to_cpu(x) swap_byte_16(x)
/** Convert from 32 bit big endian format to CPU format */
#define be32_to_cpu(x) swap_byte_32(x)
/** Convert from 64 bit big endian format to CPU format */
#define be64_to_cpu(x) swap_byte_64(x)
/** Convert to 16 bit big endian format from CPU format */
#else

/** Convert ulong n/w to host */
#define ntohl(x) swap_byte_32(x)
/** Convert host ulong to n/w */
#define htonl(x) swap_byte_32(x)
/** Convert n/w to host */
#define ntohs(x)  swap_byte_16(x)
/** Convert host to n/w */
#define htons(x)  swap_byte_16(x)


/** Convert from 16 bit little endian format to CPU format */
#define le16_to_cpu(x) x
/** Convert from 32 bit little endian format to CPU format */
#define le32_to_cpu(x) x
/** Convert from 64 bit little endian format to CPU format */
#define le64_to_cpu(x) x
/** Convert to 16 bit little endian format from CPU format */



#define cpu_to_le16(x) x
/** Convert to 32 bit little endian format from CPU format */
#define cpu_to_le32(x) x
/** Convert to 64 bit little endian format from CPU format */
#define cpu_to_le64(x) x



#define cpu_to_be16(x) swap_byte_16(x)
/** Convert to 32 bit big endian format from CPU format */
#define cpu_to_be32(x) swap_byte_32(x)
/** Convert to 64 bit big endian format from CPU format */
#define cpu_to_be64(x) swap_byte_64(x)

/** Convert from 16 bit big endian format to CPU format */
#define be16_to_cpu(x) swap_byte_16(x)
/** Convert from 32 bit big endian format to CPU format */
#define be32_to_cpu(x) swap_byte_32(x)
/** Convert from 64 bit big endian format to CPU format */
#define be64_to_cpu(x) swap_byte_64(x)
/** Convert to 16 bit big endian format from CPU format */

#endif



#endif /* _LINUX_BYTEORDER_GENERIC_H */
