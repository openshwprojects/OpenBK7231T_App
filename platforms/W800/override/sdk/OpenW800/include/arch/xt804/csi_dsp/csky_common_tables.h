/******************************************************************************
 * @file     csky_common_tables.h
 * @brief    This file has extern declaration for common tables like
 *           Bitreverse, reciprocal etc which are used across different functions.
 * @version  V1.0
 * @date     20. Dec 2016
 ******************************************************************************/
/* ---------------------------------------------------------------------------
 * Copyright (C) 2016 CSKY Limited. All rights reserved.
 *
 * Redistribution and use of this software in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   * Neither the name of CSKY Ltd. nor the names of CSKY's contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission of CSKY Ltd.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------------- */

#ifndef _CSKY_COMMON_TABLES_H
#define _CSKY_COMMON_TABLES_H

#include "csky_math.h"

extern const uint16_t cskyBitRevTable[1024];
extern const q15_t cskyRecipTableQ15[64];
extern const q31_t cskyRecipTableQ31[64];
extern const float32_t twiddleCoef_16[32];
extern const float32_t twiddleCoef_32[64];
extern const float32_t twiddleCoef_64[128];
extern const float32_t twiddleCoef_128[256];
extern const float32_t twiddleCoef_256[512];
extern const float32_t twiddleCoef_512[1024];
extern const float32_t twiddleCoef_1024[2048];
extern const float32_t twiddleCoef_2048[4096];
extern const float32_t twiddleCoef_4096[8192];
extern const q31_t twiddleCoef_16_q31[24];
extern const q31_t twiddleCoef_32_q31[48];
extern const q31_t twiddleCoef_64_q31[96];
extern const q31_t twiddleCoef_128_q31[192];
extern const q31_t twiddleCoef_256_q31[384];
extern const q31_t twiddleCoef_512_q31[768];
extern const q31_t twiddleCoef_1024_q31[1536];
extern const q31_t twiddleCoef_2048_q31[3072];
extern const q31_t twiddleCoef_4096_q31[6144];
extern const q15_t twiddleCoef_16_q15[24];
extern const q15_t twiddleCoef_32_q15[48];
extern const q15_t twiddleCoef_64_q15[96];
extern const q15_t twiddleCoef_128_q15[192];
extern const q15_t twiddleCoef_256_q15[384];
extern const q15_t twiddleCoef_512_q15[768];
extern const q15_t twiddleCoef_1024_q15[1536];
extern const q15_t twiddleCoef_2048_q15[3072];
extern const q15_t twiddleCoef_4096_q15[6144];
extern const float32_t twiddleCoef_rfft_32[32];
extern const float32_t twiddleCoef_rfft_64[64];
extern const float32_t twiddleCoef_rfft_128[128];
extern const float32_t twiddleCoef_rfft_256[256];
extern const float32_t twiddleCoef_rfft_512[512];
extern const float32_t twiddleCoef_rfft_1024[1024];
extern const float32_t twiddleCoef_rfft_2048[2048];
extern const float32_t twiddleCoef_rfft_4096[4096];
extern const float32_t twiddleCoef_rfft_8192[8192];


extern const q15_t realCoefAQ15_8192[8192];
extern const q31_t realCoefAQ31_8192[8192];

/*Tables for RFFT.*/
extern const q15_t ALIGN4 realCoefAQ15_32[32];
extern const q15_t ALIGN4 realCoefAQ15_64[64];
extern const q15_t ALIGN4 realCoefAQ15_128[128];
extern const q15_t ALIGN4 realCoefAQ15_256[256];
extern const q15_t ALIGN4 realCoefAQ15_512[512];
extern const q15_t ALIGN4 realCoefAQ15_1024[1024];
extern const q15_t ALIGN4 realCoefAQ15_2048[2048];
extern const q15_t ALIGN4 realCoefAQ15_4096[4096];

extern const q31_t realCoefAQ31_32[32];
extern const q31_t realCoefAQ31_64[64];
extern const q31_t realCoefAQ31_128[128];
extern const q31_t realCoefAQ31_256[256];
extern const q31_t realCoefAQ31_512[512];
extern const q31_t realCoefAQ31_1024[1024];
extern const q31_t realCoefAQ31_2048[2048];
extern const q31_t realCoefAQ31_4096[4096];


extern const float32_t realCoefA[8192];
extern const float32_t realCoefB[8192];


/*Tables for DCT4*/
extern const q15_t ALIGN4 WeightsQ15_128[128+2];
extern const q15_t ALIGN4 WeightsQ15_512[512+2];
extern const q15_t ALIGN4 WeightsQ15_2048[2048+2];
extern const q15_t ALIGN4 WeightsQ15_8192[8192+2];

extern const q15_t ALIGN4 cos_factorsQ15_128[128];
extern const q15_t ALIGN4 cos_factorsQ15_512[512];
extern const q15_t ALIGN4 cos_factorsQ15_2048[2048];
extern const q15_t ALIGN4 cos_factorsQ15_8192[8192];


extern const q31_t WeightsQ31_128[128+2];
extern const q31_t WeightsQ31_512[512+2];
extern const q31_t WeightsQ31_2048[2048+2];
extern const q31_t WeightsQ31_8192[8192+2];

extern const q31_t cos_factorsQ31_128[128];
extern const q31_t cos_factorsQ31_512[512];
extern const q31_t cos_factorsQ31_2048[2048];
extern const q31_t cos_factorsQ31_8192[8192];


extern const float32_t Weights_128[128+2];
extern const float32_t Weights_512[512+2];
extern const float32_t Weights_2048[2048+2];
extern const float32_t Weights_8192[8192+2];

extern const float32_t cos_factors_128[128];
extern const float32_t cos_factors_512[512];
extern const float32_t cos_factors_2048[2048];
extern const float32_t cos_factors_8192[8192];

/* floating-point bit reversal tables */
#define CSKYBITREVINDEXTABLE__16_TABLE_LENGTH ((uint16_t)20  )
#define CSKYBITREVINDEXTABLE__32_TABLE_LENGTH ((uint16_t)48  )
#define CSKYBITREVINDEXTABLE__64_TABLE_LENGTH ((uint16_t)56  )
#define CSKYBITREVINDEXTABLE_128_TABLE_LENGTH ((uint16_t)208 )
#define CSKYBITREVINDEXTABLE_256_TABLE_LENGTH ((uint16_t)440 )
#define CSKYBITREVINDEXTABLE_512_TABLE_LENGTH ((uint16_t)448 )
#define CSKYBITREVINDEXTABLE1024_TABLE_LENGTH ((uint16_t)1800)
#define CSKYBITREVINDEXTABLE2048_TABLE_LENGTH ((uint16_t)3808)
#define CSKYBITREVINDEXTABLE4096_TABLE_LENGTH ((uint16_t)4032)

extern const uint16_t cskyBitRevIndexTable16[CSKYBITREVINDEXTABLE__16_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable32[CSKYBITREVINDEXTABLE__32_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable64[CSKYBITREVINDEXTABLE__64_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable128[CSKYBITREVINDEXTABLE_128_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable256[CSKYBITREVINDEXTABLE_256_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable512[CSKYBITREVINDEXTABLE_512_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable1024[CSKYBITREVINDEXTABLE1024_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable2048[CSKYBITREVINDEXTABLE2048_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable4096[CSKYBITREVINDEXTABLE4096_TABLE_LENGTH];

/* fixed-point bit reversal tables */
#define CSKYBITREVINDEXTABLE_FIXED___16_TABLE_LENGTH ((uint16_t)12  )
#define CSKYBITREVINDEXTABLE_FIXED___32_TABLE_LENGTH ((uint16_t)24  )
#define CSKYBITREVINDEXTABLE_FIXED___64_TABLE_LENGTH ((uint16_t)56  )
#define CSKYBITREVINDEXTABLE_FIXED__128_TABLE_LENGTH ((uint16_t)112 )
#define CSKYBITREVINDEXTABLE_FIXED__256_TABLE_LENGTH ((uint16_t)240 )
#define CSKYBITREVINDEXTABLE_FIXED__512_TABLE_LENGTH ((uint16_t)480 )
#define CSKYBITREVINDEXTABLE_FIXED_1024_TABLE_LENGTH ((uint16_t)992 )
#define CSKYBITREVINDEXTABLE_FIXED_2048_TABLE_LENGTH ((uint16_t)1984)
#define CSKYBITREVINDEXTABLE_FIXED_4096_TABLE_LENGTH ((uint16_t)4032)

extern const uint16_t cskyBitRevIndexTable_fixed_16[CSKYBITREVINDEXTABLE_FIXED___16_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_32[CSKYBITREVINDEXTABLE_FIXED___32_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_64[CSKYBITREVINDEXTABLE_FIXED___64_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_128[CSKYBITREVINDEXTABLE_FIXED__128_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_256[CSKYBITREVINDEXTABLE_FIXED__256_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_512[CSKYBITREVINDEXTABLE_FIXED__512_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_1024[CSKYBITREVINDEXTABLE_FIXED_1024_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_2048[CSKYBITREVINDEXTABLE_FIXED_2048_TABLE_LENGTH];
extern const uint16_t cskyBitRevIndexTable_fixed_4096[CSKYBITREVINDEXTABLE_FIXED_4096_TABLE_LENGTH];

/* Tables for Fast Math Sine and Cosine */
extern const float32_t sinTable_f32[FAST_MATH_TABLE_SIZE + 1];
extern const q31_t sinTable_q31[FAST_MATH_TABLE_SIZE + 1];
extern const q15_t sinTable_q15[FAST_MATH_TABLE_SIZE + 1];

/*Table for Fast math pow*/
extern  const log2_cof1  ui;
extern  const log2_cof2  vj;
extern  const exp_cof1 coar;
extern  const exp_cof2 fine;

/*Table for Fast math pow2*/
extern  const float64_t exp2_accuratetable[512];
extern  const float32_t exp2_deltatable[512];

/*Table for Fast math pow2*/
extern  const mynumber Iu[182];
extern  const mynumber Iv[362];
extern  const mynumber Lu[182][2];
extern  const mynumber Lv[362][2];

/*constant for Fast math*/
  const static mynumber
  nZERO	  = {{0, 0x80000000}},	        /* -0.0          */
  INF     = {{0x00000000, 0x7ff00000}}, /* INF           */
  nINF    = {{0x00000000, 0xfff00000}}, /* -INF          */
  sqrt_2  = {{0x667f3bcc, 0x3ff6a09e}}, /* sqrt(2)       */
  ln2a    = {{0xfefa3800, 0x3fe62e42}}, /* ln(2) 43 bits */
  ln2b    = {{0x93c76730, 0x3d2ef357}}, /* ln(2)-ln2a    */
  bigu    = {{0xfffffd2c, 0x4297ffff}}, /* 1.5*2**42 -724*2**-10  */
  bigv    = {{0xfff8016a, 0x4207ffff}}, /* 1.5*2**33-1+362*2**-19 */
  t52     = {{0x00000000, 0x43300000}}, /* 2**52         */
  two52e  = {{0x000003ff, 0x43300000}}, /* 2**52'        */
  //nan     = {{0x00000000, 0x7ff80000}}, /* NAN           */
  t256    = {{0, 0x4ff00000}},          /* 2^256         */
  ln_two1 = {{0xFEFA3800, 0x3FE62E42}}, /* 0.69314718055989033 */
  ln_two2 = {{0x93C76730, 0x3D2EF357}}, /* 5.4979230187083712e-14*/
  log2e   = {{0x652B82FE, 0x3FF71547}}, /* 1.4426950408889634  */
  ep2     = {{0x000004DC, 0x3FE00000}}, /* 0.50000000000013811 */
  ep3     = {{0x55555A0F, 0x3FC55555}}, /* 0.16666666666670024 */
  three33 = {{0, 0x42180000}},          /* 25769803776      */
  three51 = {{0, 0x43380000}};          /* 6755399441055744 */

  const static float64_t
  p2 = -0.5, p3 =  3.3333333333333333333e-1, p4 = -0.25,
  q2 = -0.5, q3 = 3.3333333333331404e-01, q4 =  -2.4999999999996436e-01,
  q5 =  2.0000010500004459e-01, q6 =  -1.6666678916688004e-01,
  r3 =  3.33333333333333333372884096563030E-01,
  r4 = -2.50000000000000000213574153875908E-01,
  r5 =  1.99999999999683593814072199830603E-01,
  r6 = -1.66666666666065494878165510225378E-01,
  r7 =  1.42857517857114380606360005067609E-01,
  r8 = -1.25000449999974370683775964001702E-01,
  s3 =  0.333251953125000000e0,
 ss3 =  8.138020833333333333e-05,
  s4 = -2.500000000000000000e-01,
  s5 =  1.999999999999960937e-01,
  s6 = -1.666666666666592447e-01,
  s7 =  1.428571845238194705e-01;
//  s8 = -1.250000500000149097e-01;

  const static float64_t huge = 1.0e300, tiny = 1.0e-300;
  const static float64_t err_0 = 1.000014, err_1 = 0.000016, zero = 0.0;
  const static q31_t bigint = 0x40862002,
               badint = 0x40876000, smallint = 0x3C8fffff;
  const static q31_t hugeint = 0x7fffffff, infint = 0x7ff00000;

static const mynumber
  /* polynomial I */
  a2  = {{0x0001aa8f, 0xbfe00000} }, /* -0.500... */
  a3             = {{0x55588d2e, 0x3fd55555} }, /*  0.333... */
  /*polynomial II */
  b0    = {{0x55555555, 0x3fd55555} }, /*  0.333... */
  b1    = {{0xffffffbb, 0xbfcfffff} }, /* -0.249... */
  b2    = {{0x9999992f, 0x3fc99999} }, /*  0.199... */
  b3    = {{0x556503fd, 0xbfc55555} }, /* -0.166... */
  b4    = {{0x925b3d62, 0x3fc24924} }, /*  0.142... */
  b5    = {{0x160472fc, 0xbfbffffe} }, /* -0.124... */
  b6    = {{0x25db58ac, 0x3fbc71c5} }, /*  0.111... */
  b7    = {{0x11a2a61c, 0xbfb9a4ac} }, /* -0.100... */
  b8    = {{0x0df2b591, 0x3fb75077} }, /*  0.091... */
  /*polynomial III */
  c2    = {{0x00000000, 0xbfe00000} }, /* -1/2      */
  c3    = {{0x55555555, 0x3fd55555} }, /*  1/3      */
  c4    = {{0x00000000, 0xbfd00000} }, /* -1/4      */
  c5    = {{0x9999999a, 0x3fc99999} }, /*  1/5      */
  /*polynomial IV */
  d2    = {{0x00000000, 0xbfe00000} }, /* -1/2      */
  dd2   = {{0x00000000, 0x00000000} }, /* -1/2-d2   */
  d3    = {{0x55555555, 0x3fd55555} }, /*  1/3      */
  dd3   = {{0x55555555, 0x3c755555} }, /*  1/3-d3   */
  d4    = {{0x00000000, 0xbfd00000} }, /* -1/4      */
  dd4   = {{0x00000000, 0x00000000} }, /* -1/4-d4   */
  d5    = {{0x9999999a, 0x3fc99999} }, /*  1/5      */
  dd5   = {{0x9999999a, 0xbc699999} }, /*  1/5-d5   */
  d6    = {{0x55555555, 0xbfc55555} }, /* -1/6      */
  dd6   = {{0x55555555, 0xbc655555} }, /* -1/6-d6   */
  d7    = {{0x92492492, 0x3fc24924} }, /*  1/7      */
  dd7   = {{0x92492492, 0x3c624924} }, /*  1/7-d7   */
  d8    = {{0x00000000, 0xbfc00000} }, /* -1/8      */
  dd8   = {{0x00000000, 0x00000000} }, /* -1/8-d8   */
  d9    = {{0x1c71c71c, 0x3fbc71c7} }, /*  1/9      */
  dd9   = {{0x1c71c71c, 0x3c5c71c7} }, /*  1/9-d9   */
  d10   = {{0x9999999a, 0xbfb99999} }, /* -1/10     */
  dd10  = {{0x9999999a, 0x3c599999} }, /* -1/10-d10 */
  d11   = {{0x745d1746, 0x3fb745d1} }, /*  1/11     */
  d12   = {{0x55555555, 0xbfb55555} }, /* -1/12     */
  d13   = {{0x13b13b14, 0x3fb3b13b} }, /*  1/13     */
  d14   = {{0x92492492, 0xbfb24924} }, /* -1/14     */
  d15   = {{0x11111111, 0x3fb11111} }, /*  1/15     */
  d16   = {{0x00000000, 0xbfb00000} }, /* -1/16     */
  d17   = {{0x1e1e1e1e, 0x3fae1e1e} }, /*  1/17     */
  d18   = {{0x1c71c71c, 0xbfac71c7} }, /* -1/18     */
  d19   = {{0xbca1af28, 0x3faaf286} }, /*  1/19     */
  d20   = {{0x9999999a, 0xbfa99999} }, /* -1/20     */
  /*constants    */
  h1    = {{0x00000000, 0x3fd2e000} }, /* 151/2**9  */
  h2    = {{0x00000000, 0x3f669000} }, /* 361/2**17 */
  delu  = {{0x00000000, 0x3f700000} }, /* 1/2**8    */
  delv  = {{0x00000000, 0x3ef00000} }, /* 1/2**16   */
  e1    = {{0x00000000, 0x3bbcc868} }, /* 6.095e-21 */
  e2    = {{0x00000000, 0x3c1138ce} }, /* 2.334e-19 */
  e3    = {{0x00000000, 0x3aa1565d} }, /* 2.801e-26 */
  e4    = {{0x00000000, 0x39809d88} }, /* 1.024e-31 */
  e[4]  = {{{0x00000000, 0x37da223a} },/* 1.2e-39   */
          {{0x00000000, 0x35c851c4} }, /* 1.3e-49   */
          {{0x00000000, 0x2ab85e51} }, /* 6.8e-103  */
          {{0x00000000, 0x17383827} }},/* 8.1e-197  */
  two54 = {{0x00000000, 0x43500000} }, /* 2**54     */
  u03   = {{0xeb851eb8, 0x3f9eb851} }; /* 0.03      */

#define  SQRT_2    sqrt_2.x
#define  DEL_U     delu.x
#define  DEL_V     delv.x
#define  LN2A      ln2a.x
#define  LN2B      ln2b.x
#define  E1        e1.x
#define  E2        e2.x
#define  E3        e3.x
#define  E4        e4.x
#define  U03       u03.x
#define  HALF      0x1.0p-1     /* 1/2 */
#define  MHALF     -0x1.0p-1    /* -1/2 */

/*coeffient for log2 funtion*/
static const float64_t
  ln2     = 0.69314718055994530942,
  two54_d = 1.80143985094819840000e+16,   /* 43500000 00000000 */
  Lg1     = 6.666666666666735130e-01,     /* 3FE55555 55555593 */
  Lg2     = 3.999999999940941908e-01,     /* 3FD99999 9997FA04 */
  Lg3     = 2.857142874366239149e-01,     /* 3FD24924 94229359 */
  Lg4     = 2.222219843214978396e-01,     /* 3FCC71C5 1D8E78AF */
  Lg5     = 1.818357216161805012e-01,     /* 3FC74664 96CB03DE */
  Lg6     = 1.531383769920937332e-01,     /* 3FC39A09 D078C69F */
  Lg7     = 1.479819860511658591e-01;     /* 3FC2F112 DF3E5244 */

/*coeffient for log10 function*/
static const float64_t
  ivln10    = 4.34294481903251816668e-01,	/* 0x3FDBCB7B, 0x1526E50E */
  log10_2hi = 3.01029995663611771306e-01,	/* 0x3FD34413, 0x509F6000 */
  log10_2lo = 3.69423907715893078616e-13;	/* 0x3D59FEF3, 0x11F12B36 */

/*constant for log10 function*/
static const float64_t
  TWO1023  = 8.988465674311579539e+307,
  TWOM1000 = 9.3326361850321887899e-302;

#endif /*  CSKY_COMMON_TABLES_H */
