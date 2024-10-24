#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "core_804.h"
#include "wm_irq.h"
#include "wm_regs.h"
#include "wm_debug.h"
#include "wm_pmu.h"
#include "wm_crypto_hard.h"
#include "wm_crypto_hard_mbed.h"
#include "wm_internal_flash.h"
#include "libtommath.h"


#define ciL    (sizeof(mbedtls_mpi_uint))         /* chars in limb  */
#define biL    (ciL << 3)               /* bits  in limb  */
#define biH    (ciL << 2)               /* half limb size */

extern int mbedtls_mpi_write_binary_nr( const mbedtls_mpi *X,unsigned char *buf, size_t buflen );
extern struct wm_crypto_ctx  g_crypto_ctx;
static void rsaMonMulSetLen(const u32 len)
{
    RSAN = len;
}
static void rsaMonMulWriteMc(const u32 mc)
{
    u32 val = 0;
    RSAMC = mc;
    val = RSAMC;
    if(val == mc)
    {
        val = 1;
        return;
    }
}
static void rsaMonMulWriteA(const u32 *const in)
{
    memcpy((u32 *)&RSAXBUF, in, RSAN * sizeof(u32));
}
static void rsaMonMulWriteB(const u32 *const in)
{
    memcpy((u32 *)&RSAYBUF, in, RSAN * sizeof(u32));
}
static void rsaMonMulWriteM(const u32 *const in)
{
    memcpy((u32 *)&RSAMBUF, in, RSAN * sizeof(u32));
}
static void rsaMonMulReadA(u32 *const in)
{
    memcpy(in, (u32 *)&RSAXBUF, RSAN * sizeof(u32));
}
static void rsaMonMulReadB(u32 *const in)
{
    memcpy(in, (u32 *)&RSAYBUF, RSAN * sizeof(u32));
}
static void rsaMonMulReadD(u32 *const in)
{
    memcpy(in, (u32 *)&RSADBUF, RSAN * sizeof(u32));
}
static int rsaMulModRead(unsigned char w, mbedtls_mpi *a)
{
    u32 in[64];
    int err = 0;
    memset(in, 0, 64 * sizeof(u32));
    switch(w)
    {
    case 'A':
        rsaMonMulReadA(in);
        break;
    case 'B':
        rsaMonMulReadB(in);
        break;
    case 'D':
        rsaMonMulReadD(in);
        break;
    }
    mp_reverse((unsigned char *)in, RSAN * sizeof(u32));
    if ((err = mbedtls_mpi_read_binary(a, (unsigned char *)in, RSAN * sizeof(u32))) != 0)
    {
        mbedtls_mpi_free(a);
        return err;
    }
    return 0;
}

#if 0
static void rsaMulModDump(unsigned char w)
{
	int addr = 0;
    switch(w)
    {
    case 'A':
		addr = 0;
        break;
    case 'B':
        addr = 0x100;
        break;
    case 'D':
        addr = 0x300;
        break;
    }
	printf("%c", w);
	dumpUint32(" Val:",((volatile u32*) (RSA_BASE_ADDRESS + addr )), RSAN);
}
#endif
static void rsaMulModWrite(unsigned char w, mbedtls_mpi *a)
{
    u32 in[64];
    memset(in, 0, 64 * sizeof(u32));
    mbedtls_mpi_write_binary_nr(a, (unsigned char *)in, a->n * ciL);
	//printf("rsaMulModWrite %c\n", w);
	//dumpUint32("a", a->p, a->n);
	//dumpUint32("in", in, a->n);
    switch(w)
    {
    case 'A':
        rsaMonMulWriteA(in);
        break;
    case 'B':
        rsaMonMulWriteB(in);
        break;
    case 'M':
        rsaMonMulWriteM(in);
        break;
    }
}
static void rsaMonMulAA(void)
{
    g_crypto_ctx.rsa_complete = 0;
    RSACON = 0x2c;

    while (!g_crypto_ctx.rsa_complete)
    {

    }
    g_crypto_ctx.rsa_complete = 0;
}
static void rsaMonMulDD(void)
{
    g_crypto_ctx.rsa_complete = 0;
    RSACON = 0x20;

    while (!g_crypto_ctx.rsa_complete)
    {

    }
    g_crypto_ctx.rsa_complete = 0;
}
static void rsaMonMulAB(void)
{
    g_crypto_ctx.rsa_complete = 0;
    RSACON = 0x24;

    while (!g_crypto_ctx.rsa_complete)
    {

    }
    g_crypto_ctx.rsa_complete = 0;
}
static void rsaMonMulBD(void)
{
    g_crypto_ctx.rsa_complete = 0;
    RSACON = 0x28;

    while (!g_crypto_ctx.rsa_complete)
    {

    }
    g_crypto_ctx.rsa_complete = 0;
}
/******************************************************************************
compute mc, s.t. mc * in = 0xffffffff
******************************************************************************/
static void rsaCalMc(u32 *mc, const u32 in)
{
    u32 y = 1;
    u32 i = 31;
    u32 left = 1;
    u32 right = 0;
    for(i = 31; i != 0; i--)
    {
        left <<= 1;										/* 2^(i-1) */
        right = (in * y) & left;                        /* (n*y) mod 2^i */
        if( right )
        {
            y += left;
        }
    }
    *mc =  ~y + 1;
}

int tls_crypto_mbedtls_exptmod( mbedtls_mpi *X, const mbedtls_mpi *A, const mbedtls_mpi *E, const mbedtls_mpi *N )
{
    int i = 0;
    u32 k = 0, mc = 0, dp0;
    volatile u8 monmulFlag = 0;
    mbedtls_mpi R, X1, Y;
//	mbedtls_mpi T;
	int ret = 0;
	size_t max_len;

    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_RSA);

#ifndef CONFIG_KERNEL_NONE
    tls_fls_sem_lock();
#endif

	max_len = (mbedtls_mpi_bitlen(N) + biL - 1) / biL;

    mbedtls_mpi_init(&X1);
    mbedtls_mpi_init(&Y);
    mbedtls_mpi_init(&R);
	MBEDTLS_MPI_CHK( mbedtls_mpi_shrink((mbedtls_mpi *)N, max_len ) );
	
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &R, 1 ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_shift_l( &R, N->n * biL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &R, &R, N ) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_shrink( &R, N->n ) );
	//dumpUint32("R", R.p, R.n);

	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &X1, A, &R ) );//X = A * R
	//dumpUint32("X = A * R", X1.p, X1.n);
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &X1, &X1, N ) ); //X = A * R mod N
	MBEDTLS_MPI_CHK( mbedtls_mpi_shrink( &X1, N->n ) );
	//dumpUint32("X = A * R mod N", X1.p, X1.n);
	MBEDTLS_MPI_CHK( mbedtls_mpi_copy( &Y, &R ) );
    
    dp0 = (u32)N->p[0];
    rsaCalMc(&mc, dp0);
    rsaMonMulSetLen((const u32)N->n);
    rsaMonMulWriteMc(mc);
    rsaMulModWrite('M', (mbedtls_mpi *)N);
    rsaMulModWrite('B', &X1);
    rsaMulModWrite('A', &Y);
	
    k = mbedtls_mpi_bitlen(E);
	//printf("mbedtls e bit len %d\n", k);
    for(i = k - 1; i >= 0; i--)
    {
        //montMulMod(&Y, &Y, n, &Y);
        //if(pstm_get_bit(e, i))
        //	montMulMod(&Y, &X, n, &Y);
        if(monmulFlag == 0)
        {
            rsaMonMulAA();
            monmulFlag = 1;
			//rsaMulModDump('D');
        }
        else
        {
            rsaMonMulDD();
            monmulFlag = 0;
			//rsaMulModDump('A');
        }

        if(mbedtls_mpi_get_bit(E, i))
        {
            if(monmulFlag == 0)
            {
                rsaMonMulAB();
                monmulFlag = 1;
				//rsaMulModDump('D');
            }
            else
            {
                rsaMonMulBD();
                monmulFlag = 0;
				//rsaMulModDump('A');
            }
        }
    }
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &R, 1 ) );
    rsaMulModWrite('B', &R);
    //montMulMod(&Y, &R, n, res);
    if(monmulFlag == 0)
    {
        rsaMonMulAB();
        rsaMulModRead('D', X);
    }
    else
    {
        rsaMonMulBD();
        rsaMulModRead('A', X);
    }
	MBEDTLS_MPI_CHK( mbedtls_mpi_shrink( X, N->n ) );
cleanup:
    mbedtls_mpi_free(&X1);
    mbedtls_mpi_free(&Y);
    mbedtls_mpi_free(&R);
#ifndef CONFIG_KERNEL_NONE
    tls_fls_sem_unlock();
#endif
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_RSA);

    return ret;
}

#if 0
#if 1
typedef s32 psPool_t;
#include "libtommath.h"
#define pstm_set(a, b) mp_set((mp_int *)a, b)
#define pstm_init(pool, a) wpa_mp_init((mp_int *)a)
#define pstm_count_bits(a) mp_count_bits((mp_int *)a)
#define pstm_init_for_read_unsigned_bin(pool, a, len) mp_init_for_read_unsigned_bin((mp_int *)a, len)
#define pstm_read_unsigned_bin(a, b, c) mp_read_unsigned_bin((mp_int *)a, b, c)
#define pstm_copy(a, b) mp_copy((mp_int *)a, (mp_int *)b)
#define pstm_clear(a) mp_clear((mp_int *)a)
#define pstm_clamp(a) mp_clamp((mp_int *)a)
#define pstm_mulmod(pool, a, b, c, d) mp_mulmod((mp_int *)a, (mp_int *)b, (mp_int *)c, (mp_int *)d)
#define pstm_exptmod(pool, G, X, P, Y) mp_exptmod((mp_int *)G, (mp_int *)X, (mp_int *)P, (mp_int *)Y)
#define pstm_reverse mp_reverse
#define pstm_cmp mp_cmp
#define pstm_to_unsigned_bin_nr(pool, a, b) mp_to_unsigned_bin_nr((mp_int *)a, (unsigned char *)b)

#define pstm_2expt(a, b) mp_2expt((mp_int *)a, b)
#define pstm_mod(pool, a, b, c) mp_mod((mp_int *)a, (mp_int *)b, (mp_int *)c)

#endif

uint8_t modulus[] = {
    0xdf, 0x83, 0xe4, 0x76, 0x2d, 0x00, 0x61, 0xf6, 0xd0, 0x8d, 0x4a, 0x04, 0x66, 0xb1, 0xd5, 0x55,
    0xef, 0x71, 0xb5, 0xa5, 0x4e, 0x69, 0x44, 0xd3, 0x4f, 0xb8, 0x3d, 0xec, 0xb1, 0x1d, 0x5f, 0x82,
    0x6a, 0x48, 0x21, 0x00, 0x7f, 0xd7, 0xd5, 0xf6, 0x82, 0x35, 0xc2, 0xa6, 0x67, 0xa3, 0x53, 0x2d,
    0x3a, 0x83, 0x9a, 0xba, 0x60, 0xc2, 0x11, 0x22, 0xc2, 0x35, 0x83, 0xe9, 0x10, 0xa1, 0xb4, 0xa6,
    0x74, 0x57, 0x99, 0xd3, 0xa8, 0x6a, 0x21, 0x83, 0x76, 0xc1, 0x67, 0xde, 0xd8, 0xec, 0xdf, 0xf7,
    0xc0, 0x1b, 0xf6, 0xfa, 0x14, 0xa4, 0x0a, 0xec, 0xd1, 0xee, 0xc0, 0x76, 0x4c, 0xcd, 0x4a, 0x0a,
    0x5c, 0x96, 0xf2, 0xc9, 0xa4, 0x67, 0x03, 0x97, 0x2e, 0x17, 0xcd, 0xa9, 0x27, 0x9d, 0xa6, 0x35,
    0x5f, 0x7d, 0xb1, 0x6b, 0x68, 0x0e, 0x99, 0xc7, 0xdd, 0x5d, 0x6f, 0x15, 0xce, 0x8e, 0x85, 0x33
};
static const uint8_t publicExponent[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01
};
static const uint8_t privateExponent[] = {
    0xc6, 0x15, 0x3d, 0x02, 0xfe, 0x1e, 0xb8, 0xb2, 0xe3, 0x60, 0x53, 0x98, 0x52, 0xea, 0x87, 0x06,
    0x01, 0x8d, 0xe4, 0x4c, 0xfb, 0x90, 0x8f, 0x4e, 0x35, 0xf8, 0x31, 0xe8, 0xf1, 0x8d, 0xf6, 0x76,
    0xbd, 0x79, 0xee, 0xc5, 0x62, 0x87, 0x05, 0x37, 0xd1, 0x6d, 0x93, 0x73, 0xa5, 0xa5, 0x38, 0xb1,
    0x7c, 0x89, 0xe5, 0x36, 0x07, 0x49, 0xf5, 0xa5, 0xb8, 0x37, 0x75, 0x0f, 0xb7, 0x8d, 0x97, 0x69,
    0xc4, 0xd4, 0x8a, 0xb7, 0xfe, 0x74, 0x48, 0x45, 0x58, 0x47, 0x29, 0xa3, 0x0b, 0xa7, 0xdc, 0x55,
    0x98, 0x18, 0x8c, 0xd4, 0x52, 0xf5, 0xc9, 0xe8, 0x40, 0xce, 0x97, 0x46, 0x14, 0x1f, 0x62, 0x94,
    0xc3, 0x21, 0x1e, 0x5d, 0x49, 0x59, 0x31, 0xeb, 0xc4, 0x95, 0xf9, 0x33, 0x70, 0xa7, 0x90, 0xc3,
    0x9e, 0x98, 0x58, 0xa4, 0x00, 0xa4, 0x0f, 0xf3, 0x51, 0x80, 0xc6, 0x14, 0xfb, 0xd5, 0x5b, 0x01
};

uint8_t Digest_signature_pkcs1_padding_out[] = {
    0x07, 0x2d, 0x25, 0xde, 0xa5, 0xfd, 0x7c, 0xb0, 0x92, 0xb4, 0xee, 0x57, 0xe8, 0xd3, 0x79, 0x74,
    0x59, 0x25, 0x34, 0xef, 0xfd, 0x2b, 0xda, 0x8b, 0xa4, 0x40, 0x4e, 0xd8, 0x92, 0x6e, 0xee, 0x84,
    0x52, 0xb0, 0xe1, 0x0e, 0xa8, 0xa9, 0x68, 0x62, 0x1b, 0x51, 0xed, 0x50, 0x84, 0x98, 0x6a, 0x97,
    0x98, 0xe8, 0xcf, 0x3f, 0x85, 0xd3, 0x28, 0x26, 0xf3, 0x7a, 0x52, 0x4b, 0x04, 0x95, 0xe6, 0xfd,
    0xfa, 0x41, 0xf3, 0xac, 0x8a, 0x6d, 0x74, 0x91, 0x8c, 0x87, 0x52, 0x38, 0x08, 0x49, 0xf4, 0x60,
    0xcd, 0x4b, 0x1a, 0x9e, 0x52, 0x60, 0xf2, 0x73, 0x60, 0x31, 0x78, 0x37, 0xd9, 0x42, 0xc4, 0x61,
    0x43, 0xcf, 0x6d, 0x55, 0xee, 0x05, 0x19, 0xb7, 0xc3, 0x37, 0xa7, 0xa8, 0xa4, 0xbd, 0xf1, 0xac,
    0x8e, 0x39, 0x20, 0x59, 0xcd, 0xfc, 0x50, 0x16, 0x81, 0x2d, 0xeb, 0xba, 0x95, 0xe9, 0x38, 0xa5,
};

static const uint8_t Digest[] = {
	0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0xe4, 0x2c, 0x9f, 0x12, 0xf7, 0xd2, 0x67, 0x3a, 0x23, 0xea, 0x85, 0x61, 0xeb, 0xb2, 0xc2, 0x19,
    0xdc, 0xd9, 0xf1, 0xaa
};
static const uint8_t base[] = {
	0x79, 0x91, 0x2F, 0x5D, 0x2C, 0x58, 0xED, 0xBF, 0xF8, 0x35, 0x75, 0x9B, 0x06, 0xF5, 0x08, 0x66,
	0xDD, 0xA4, 0xA8, 0x8D, 0x39, 0xDB, 0xB0, 0x20, 0xDB, 0xAE, 0xFC, 0x17, 0x16, 0xC2, 0x07, 0x77,
	0x01, 0x45, 0xA7, 0xC3, 0xFE, 0xEA, 0x98, 0x62, 0x50, 0x18, 0xB3, 0x1F, 0x6D, 0xF6, 0x39, 0xFA,
	0x1F, 0x2F, 0xB4, 0xBD, 0x72, 0x1D, 0x09, 0x51, 0x3D, 0xA0, 0x2B, 0xEC, 0x89, 0xD9, 0x78, 0xBD,
	0xE4, 0x8A, 0x3D, 0x48, 0x36, 0xD2, 0x25, 0xF2, 0x24, 0xC2, 0x60, 0xC6, 0x88, 0x50, 0x47, 0xB8,
	0xD4, 0x3E, 0x82, 0x8C, 0x94, 0x4B, 0x53, 0x4B, 0x7C, 0xE9, 0x52, 0x3D, 0x96, 0xEF, 0x08, 0x3E,
	0xCA, 0xA7, 0x4A, 0xD8, 0x18, 0xFB, 0x97, 0xCE, 0x5F, 0x9A, 0x75, 0x79, 0x22, 0x62, 0x47, 0x79,
	0xFA, 0x8D, 0xD5, 0x42, 0x61, 0xB4, 0xFF, 0x5D, 0xF4, 0x89, 0x0C, 0x69, 0x3D, 0x3A, 0x3A, 0x2D
};

int initMpiParams(u32 len, mbedtls_mpi	*pa, mbedtls_mpi *pb, mbedtls_mpi	*pm, int isRand){

	u32 * a = NULL;
	u32 * b = NULL;
	u32 * m = NULL;
	int err = -1;

	a = tls_mem_alloc(64 * sizeof(u32));
	if(a == NULL)
		goto out;
	b = tls_mem_alloc(64 * sizeof(u32));
	if(b== NULL)
		goto out;
	m = tls_mem_alloc(64 * sizeof(u32));
	if(m == NULL)
		goto out;

	memcpy(a, base, sizeof(base));
	memcpy(b, privateExponent, sizeof(privateExponent));
	memcpy(m, modulus, sizeof(modulus));

	dumpBuffer("modulus", (unsigned char *)m, len * 4);
	dumpBuffer("exponent", (unsigned char *)b, len * 4);
	dumpBuffer("base", (unsigned char *)a, len * 4);
	mbedtls_mpi_init(pa);
	if ((err = mbedtls_mpi_read_binary(pa, (unsigned char *)a, len * sizeof(u32))) != PS_SUCCESS) {
		mbedtls_mpi_free(pa);
		goto out;
	}
	mbedtls_mpi_init(pb);
	if ((err = mbedtls_mpi_read_binary(pb, (unsigned char *)b, len * sizeof(u32))) != PS_SUCCESS) {
		mbedtls_mpi_free(pa);
		mbedtls_mpi_free(pb);
		goto out;
	}
	mbedtls_mpi_init(pm);
	if ((err = mbedtls_mpi_read_binary(pm, (unsigned char *)m, len * sizeof(u32))) != PS_SUCCESS) {
		mbedtls_mpi_free(pa);
		mbedtls_mpi_free(pb);
		mbedtls_mpi_free(pm);
		goto out;
	}

out:
	if(a)
		tls_mem_free(a);
	if(b)
		tls_mem_free(b);
	if(m)
		tls_mem_free(m);
	return err;
}

int initPstmParams(u32 len, hstm_int	*pa, hstm_int *pb, hstm_int	*pm, int isRand){

	u32 * a = NULL;
	u32 * b = NULL;
	u32 * m = NULL;
	int err = -1;

	a = tls_mem_alloc(64 * sizeof(u32));
	if(a == NULL)
		goto out;
	b = tls_mem_alloc(64 * sizeof(u32));
	if(b== NULL)
		goto out;
	m = tls_mem_alloc(64 * sizeof(u32));
	if(m == NULL)
		goto out;

	memcpy(a, base, sizeof(base));
	memcpy(b, privateExponent, sizeof(privateExponent));
	memcpy(m, modulus, sizeof(modulus));

//	pstm_reverse((unsigned char *)a, len * sizeof(u32));
//	pstm_reverse((unsigned char *)b, len * sizeof(u32));
//	pstm_reverse((unsigned char *)m, len * sizeof(u32));
	dumpBuffer("modulus", (unsigned char *)m, len * 4);
	dumpBuffer("exponent", (unsigned char *)b, len * 4);
	dumpBuffer("base", (unsigned char *)a, len * 4);
	if ((err = pstm_init_for_read_unsigned_bin(NULL, pa, len * sizeof(u32))) != PS_SUCCESS){
		goto out;
	}
	if ((err = pstm_read_unsigned_bin(pa, (unsigned char *)a, len * sizeof(u32))) != PS_SUCCESS) {
		pstm_clear(pa);
		goto out;
	}
	if ((err = pstm_init_for_read_unsigned_bin(NULL, pb, len * sizeof(u32))) != PS_SUCCESS){
		pstm_clear(pa);
		goto out;
	}
	if ((err = pstm_read_unsigned_bin(pb, (unsigned char *)b, len * sizeof(u32))) != PS_SUCCESS) {
		pstm_clear(pa);
		pstm_clear(pb);
		goto out;
	}
	if ((err = pstm_init_for_read_unsigned_bin(NULL, pm, len * sizeof(u32))) != PS_SUCCESS){
		pstm_clear(pa);
		pstm_clear(pb);
		goto out;
	}
	if ((err = pstm_read_unsigned_bin(pm, (unsigned char *)m, len * sizeof(u32))) != PS_SUCCESS) {
		pstm_clear(pa);
		pstm_clear(pb);
		pstm_clear(pm);
		goto out;
	}

out:
	if(a)
		tls_mem_free(a);
	if(b)
		tls_mem_free(b);
	if(m)
		tls_mem_free(m);
	return err;
}


int exptModTest(u32 len){
	
	hstm_int	pa;
	hstm_int	pb;
	hstm_int	pm;
	hstm_int	pres;
	hstm_int	mres;

	
	mbedtls_mpi	ppa;
	mbedtls_mpi	ppb;
	mbedtls_mpi	ppm;
	mbedtls_mpi	ppres;
	mbedtls_mpi	pmres;
	
	int err = -1;
	
	if((err = initMpiParams(len, &ppa, &ppb, &ppm, 1)))
	{
		return err;
	}
	if((err = initPstmParams(len, &pa, &pb, &pm, 1)))
	{
		return err;
	}
	dumpUint32("mbed ppa", ppa.p, ppa.n);
	dumpUint32("mbed ppb", ppb.p, ppb.n);
	dumpUint32("mbed ppm", ppm.p, ppm.n);
	pstm_init(NULL, &pres);
	pstm_init(NULL, &mres);
	mbedtls_mpi_init(&ppres);
	mbedtls_mpi_init(&pmres);

	tls_crypto_mbedtls_exptmod(&ppres, &ppa, &ppb, &ppm);
	dumpUint32("mbed ppres", ppres.p, ppres.n);
	mbedtls_mpi_exp_mod(&pmres, &ppa, &ppb, &ppm, NULL);
	dumpUint32("mbed pmres", pmres.p, pmres.n);

	tls_crypto_exptmod(&pa, &pb, &pm, &pres);
	printf("pres:\n");
	dumpUint32("pres", pres.dp, pres.used);
	//montExptMod(&pa, &pb, &pm, &pres);
	//rsaMontExptMod(&pa, &pb, &pm, &mres);
	pstm_exptmod(NULL, &pa, &pb, &pm, &mres);
	if(pstm_cmp(&mres, &pres) != 0)
	{
		#if 1
		int i = 0;
		printf("mres:\n");
		for(;i<mres.used;i++)
		{
			printf("%x ", mres.dp[i]);
		}
		printf("\n");
		printf("pres:\n");
		for(i=0;i<pres.used;i++)
		{
			printf("%x ", pres.dp[i]);
		}
		printf("\n");
		#endif
		err = -1;
		goto out;
	}
	else
	{
#if 1
		int i = 0;
		printf("mres:\n");
		for(;i<mres.used;i++)
		{
			printf("%x ", mres.dp[i]);
		}
		printf("\n");
		printf("pres:\n");
		for(i=0;i<pres.used;i++)
		{
			printf("%x ", pres.dp[i]);
		}
		printf("\n");
		#endif
	}
	err = 0;
out:
	pstm_clear(&pa);
	pstm_clear(&pb);
	pstm_clear(&pm);
	pstm_clear(&pres);
	pstm_clear(&mres);
	mbedtls_mpi_free(&ppa);
	mbedtls_mpi_free(&ppb);
	mbedtls_mpi_free(&ppm);
	mbedtls_mpi_free(&ppres);
	mbedtls_mpi_free(&pmres);
	printf("exptModTest err %d\n", err);
	return err;
}
#endif

