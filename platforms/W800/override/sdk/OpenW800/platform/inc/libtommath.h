#ifndef __LIBTOMMATH_H__
#define __LIBTOMMATH_H__

typedef signed short int16;

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define BN_MP_INVMOD_C
#define BN_S_MP_EXPTMOD_C /* Note: #undef in tommath_superclass.h; this would
			   * require BN_MP_EXPTMOD_FAST_C instead */
#define BN_S_MP_MUL_DIGS_C
#define BN_MP_INVMOD_SLOW_C
#define BN_S_MP_SQR_C
#define BN_S_MP_MUL_HIGH_DIGS_C /* Note: #undef in tommath_superclass.h; this
				 * would require other than mp_reduce */

#ifdef LTM_FAST

/* Use faster div at the cost of about 1 kB */
#define BN_MP_MUL_D_C

/* Include faster exptmod (Montgomery) at the cost of about 2.5 kB in code */
#define BN_MP_EXPTMOD_FAST_C
#define BN_MP_MONTGOMERY_SETUP_C
#define BN_FAST_MP_MONTGOMERY_REDUCE_C
#define BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
#define BN_MP_MUL_2_C

/* Include faster sqr at the cost of about 0.5 kB in code */
#define BN_FAST_S_MP_SQR_C

#else /* LTM_FAST */

#define BN_MP_DIV_SMALL
#define BN_MP_INIT_MULTI_C
#define BN_MP_CLEAR_MULTI_C
#define BN_MP_ABS_C
#endif /* LTM_FAST */

/* Current uses do not require support for negative exponent in exptmod, so we
 * can save about 1.5 kB in leaving out invmod. */
#define LTM_NO_NEG_EXP

/* from tommath.h */

#ifndef MIN
   #define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
   #define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#define  OPT_CAST(x)

typedef unsigned int	 mp_digit;
//typedef u64 mp_word;
typedef unsigned long long mp_word;

#define XMALLOC  tls_mem_alloc
#define XFREE    tls_mem_free
#define XREALLOC os_realloc


#define MP_MASK          ((((mp_digit)1)<<((mp_digit)DIGIT_BIT))-((mp_digit)1))

#define MP_LT        -1   /* less than */
#define MP_EQ         0   /* equal to */
#define MP_GT         1   /* greater than */

#define MP_ZPOS       0   /* positive integer */
#define MP_NEG        1   /* negative */

#define MP_OKAY       0   /* ok result */
#define MP_MEM        -2  /* out of mem */
#define MP_VAL        -3  /* invalid input */

#define MP_YES        1   /* yes response */
#define MP_NO         0   /* no response */

typedef int           mp_err;

/* define this to use lower memory usage routines (exptmods mostly) */
#define MP_LOW_MEM

/* default precision */
#ifndef MP_PREC
   #ifndef MP_LOW_MEM
      #define MP_PREC                 32     /* default digits of precision */
   #else
      #define MP_PREC                 8      /* default digits of precision */
   #endif   
#endif

/* size of comba arrays, should be at least 2 * 2**(BITS_PER_WORD - BITS_PER_DIGIT*2) */
#define MP_WARRAY               (1 << (sizeof(mp_word) * CHAR_BIT - 2 * DIGIT_BIT + 1))

/* the infamous mp_int structure */
typedef struct  {
    int16 used, alloc, sign;
    mp_digit *dp;
} mp_int;


/* ---> Basic Manipulations <--- */
#define mp_iszero(a) (((a)->used == 0) ? MP_YES : MP_NO)
#define mp_iseven(a) (((a)->used > 0 && (((a)->dp[0] & 1) == 0)) ? MP_YES : MP_NO)
#define mp_isodd(a)  (((a)->used > 0 && (((a)->dp[0] & 1) == 1)) ? MP_YES : MP_NO)

 void mp_reverse (unsigned char *s, int len);
#ifdef BN_MP_INIT_MULTI_C
 int mp_init_multi(mp_int *mp, ...);
#endif
#ifdef BN_MP_CLEAR_MULTI_C
 void mp_clear_multi(mp_int *mp, ...);
#endif
 int mp_lshd(mp_int * a, int b);
 void mp_set(mp_int * a, mp_digit b);
 void mp_clamp(mp_int * a);
 void mp_exch(mp_int * a, mp_int * b);
 void mp_rshd(mp_int * a, int b);
 void mp_zero(mp_int * a);
 int mp_mod_2d(mp_int * a, int b, mp_int * c);
 int mp_div_2d(mp_int * a, int b, mp_int * c, mp_int * d);
 int mp_init_copy(mp_int * a, mp_int * b);
 int mp_mul_2d(mp_int * a, int b, mp_int * c);
#ifndef LTM_NO_NEG_EXP
 int mp_div_2(mp_int * a, mp_int * b);
 int mp_invmod(mp_int * a, mp_int * b, mp_int * c);
 int mp_invmod_slow(mp_int * a, mp_int * b, mp_int * c);
#endif /* LTM_NO_NEG_EXP */
 int mp_copy(mp_int * a, mp_int * b);
 int mp_count_bits(mp_int * a);
 int mp_div(mp_int * a, mp_int * b, mp_int * c, mp_int * d);
 int mp_mod(mp_int * a, mp_int * b, mp_int * c);
 int mp_grow(mp_int * a, int size);
 int mp_cmp_mag(mp_int * a, mp_int * b);
#ifdef BN_MP_ABS_C
 int mp_abs(mp_int * a, mp_int * b);
#endif
 int mp_sqr(mp_int * a, mp_int * b);
 int mp_reduce_2k_l(mp_int *a, mp_int *n, mp_int *d);
 int mp_reduce_2k_setup_l(mp_int *a, mp_int *d);
 int mp_2expt(mp_int * a, int b);
 int mp_reduce_setup(mp_int * a, mp_int * b);
 int mp_reduce(mp_int * x, mp_int * m, mp_int * mu);
 int mp_init_size(mp_int * a, int size);
#ifdef BN_MP_EXPTMOD_FAST_C
 int mp_exptmod_fast (mp_int * G, mp_int * X, mp_int * P, mp_int * Y, int redmode);
#endif /* BN_MP_EXPTMOD_FAST_C */
#ifdef BN_FAST_S_MP_SQR_C
 int fast_s_mp_sqr (mp_int * a, mp_int * b);
#endif /* BN_FAST_S_MP_SQR_C */
#ifdef BN_MP_MUL_D_C
 int mp_mul_d (mp_int * a, mp_digit b, mp_int * c);
#endif /* BN_MP_MUL_D_C */
#ifdef BN_MP_MUL_2_C
/* b = a*2 */
int mp_mul_2(mp_int * a, mp_int * b);
#endif
 int mp_init_for_read_unsigned_bin(mp_int *a, mp_digit len);
 void mp_clear (mp_int * a);
int mp_exptmod (mp_int * G, mp_int * X, mp_int * P, mp_int * Y);
int mp_init (mp_int * a);
int mp_read_unsigned_bin (mp_int * a, const unsigned char *b, int c);
int mp_to_unsigned_bin_nr (mp_int * a, unsigned char *b);
int mp_to_unsigned_bin (mp_int * a, unsigned char *b);
int mp_unsigned_bin_size (mp_int * a);
int mp_add (mp_int * a, mp_int * b, mp_int * c);
int mp_cmp (mp_int * a, mp_int * b);
int mp_sub (mp_int * a, mp_int * b, mp_int * c);
int mp_mulmod (mp_int * a, mp_int * b, mp_int * c, mp_int * d);
int mp_cmp_d(mp_int * a, mp_digit b);

#ifdef BN_MP_MONTGOMERY_SETUP_C
int mp_montgomery_setup (mp_int * n, mp_digit * rho);
#endif
#ifdef BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
int mp_montgomery_calc_normalization (mp_int * a, mp_int * b);
#endif
#ifdef BN_FAST_MP_MONTGOMERY_REDUCE_C
int fast_mp_montgomery_reduce (mp_int * x, mp_int * n, mp_digit rho);
#endif

#endif //__LIBTOMMATH_H__

