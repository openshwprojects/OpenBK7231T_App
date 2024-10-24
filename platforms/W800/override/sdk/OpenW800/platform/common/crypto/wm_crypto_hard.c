#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core_804.h"
#include "wm_irq.h"
#include "wm_regs.h"
#include "wm_debug.h"
#include "wm_crypto_hard.h"
#include "wm_internal_flash.h"
#include "wm_pmu.h"

//#define TEST_ALL_CRYPTO
#undef	DIGIT_BIT
#define DIGIT_BIT			28//32

#define SOFT_RESET_RC4    	25
#define SOFT_RESET_AES    	26
#define SOFT_RESET_DES    	27

#define RNG_SWITCH        	28
#define RNG_LOAD_SEED    	29
#define RNG_START         	30

#define TRNG_EN             0
#define TRNG_SEL            1
#define TRNG_DIG_BYPASS     2
#define TRNG_CP             3
#define TRNG_INT_MASK       6
#define USE_TRNG            1

#define DES_KEY_LEN     8
#define DES3_KEY_LEN	24
#define DES3_IV_LEN     8

#define SHA1_HASH_SIZE      20
#define MD5_HASH_SIZE 16 

#define STORE32H(x, y) { \
(y)[0] = (unsigned char)(((x)>>24)&255); \
(y)[1] = (unsigned char)(((x)>>16)&255); \
(y)[2] = (unsigned char)(((x)>>8)&255); \
(y)[3] = (unsigned char)((x)&255); \
}
#define STORE32L(x, y) { \
unsigned long __t = (x); memcpy(y, &__t, 4); \
}

//#define CRYPTO_LOG printf
#define CRYPTO_LOG(...)
//extern volatile uint32_t sys_count;
#define sys_count tls_os_get_time()

extern 	void delay_cnt(int count);

struct wm_crypto_ctx  g_crypto_ctx = {0,0
#ifndef CONFIG_KERNEL_NONE
	,NULL
#endif
	};

#if 1
typedef s32 psPool_t;
#include "libtommath.h"
extern int wpa_mp_init (mp_int * a);
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


void RSA_F_IRQHandler(void)
{
    RSACON = 0x00;
    g_crypto_ctx.rsa_complete = 1;
}
void CRYPTION_IRQHandler(void)
{
    tls_reg_write32(HR_CRYPTO_SEC_STS, 0x10000);
    g_crypto_ctx.gpsec_complete = 1;
}

#if 1
static int16 pstm_get_bit (hstm_int *a, int16 idx)
{
    int16     r;
    int16 n = idx / DIGIT_BIT;
    int16 m = idx % DIGIT_BIT;

    if (a->used <= 0)
    {
        return 0;
    }

    r = (a->dp[n] >> m) & 0x01;
    return r;
}
#endif
u32 Reflect(u32 ref, u8 ch)
{
    int i;
    u32 value = 0;
    for( i = 1; i < ( ch + 1 ); i++ )
    {
        if( ref & 1 )
            value |= 1 << ( ch - i );
        ref >>= 1;
    }
    return value;
}
#ifndef CONFIG_KERNEL_NONE
void tls_crypto_sem_lock(void)
{
    if (g_crypto_ctx.gpsec_lock == NULL)
    {
        return;
    }
    tls_os_sem_acquire(g_crypto_ctx.gpsec_lock, 0);
}
void tls_crypto_sem_unlock(void)
{
    if (g_crypto_ctx.gpsec_lock == NULL)
    {
        return;
    }
    tls_os_sem_release(g_crypto_ctx.gpsec_lock);
}
#else
#define         tls_crypto_sem_lock    
#define         tls_crypto_sem_unlock    
#endif
void tls_crypto_set_key(void *key, int keylen)
{
    uint32_t *key32 = (uint32_t *)key;
    int i = 0;
    for(i = 0; i < keylen / 4 && i < 6; i++)
    {
        M32(HR_CRYPTO_KEY0 + (4 * i)) = key32[i];
    }
    if(keylen == 32)
    {
        M32(HR_CRYPTO_KEY6) = key32[6];
        M32(HR_CRYPTO_KEY7) = key32[7];
    }
}
void tls_crypto_set_iv(void *iv, int ivlen)
{
    uint32_t *IV32 = (uint32_t *)iv;

    if(ivlen >= 8)
    {
        M32(HR_CRYPTO_IV0) = IV32[0];
        M32(HR_CRYPTO_IV0 + 4) = IV32[1];
    }
    if(ivlen == 16)
    {
        M32(HR_CRYPTO_IV1) = IV32[2];
        M32(HR_CRYPTO_IV1 + 4) = IV32[3];
    }
}

/**
 * @brief          	This function is used to stop random produce.
 *
 * @param[in]      	None
 *
 * @retval         	0     		success
 * @retval         	other 	failed
 *
 * @note           	None
 */
int tls_crypto_random_stop(void)
{
    unsigned int sec_cfg;
#if USE_TRNG
#else
	unsigned int val;
#endif
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);	
#if USE_TRNG
	sec_cfg = 0x40;
	tls_reg_write32(HR_CRYPTO_TRNG_CR, sec_cfg);
	g_crypto_ctx.gpsec_complete = 0;
#else
    val = tls_reg_read32(HR_CRYPTO_SEC_CFG);
    sec_cfg = val & ~(1 << RNG_START);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
#endif
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
    return ERR_CRY_OK;
}

/**
 * @brief          	This function initializes random digit seed and BIT number.
 *
 * @param[in]   	seed 		The random digit seed.
 * @param[in]   	rng_switch 	The random digit bit number.   (0: 16bit    1:32bit)
 *
 * @retval  		0  			success
 * @retval  		other   		failed
 *
 * @note             	None
 */
int tls_crypto_random_init(u32 seed, CRYPTO_RNG_SWITCH rng_switch)
{
    unsigned int sec_cfg;
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);	
#if USE_TRNG
	sec_cfg = (1 << TRNG_INT_MASK) | (4 << TRNG_CP) | (1 << TRNG_SEL) | (1 << TRNG_EN);
	sec_cfg &= ~(1 << TRNG_INT_MASK);
	g_crypto_ctx.gpsec_complete = 0;
	tls_reg_write32(HR_CRYPTO_TRNG_CR, sec_cfg);
#else
    tls_reg_write32(HR_CRYPTO_KEY0, seed);
    sec_cfg = (rng_switch << RNG_SWITCH) | (1 << RNG_LOAD_SEED) | (1 << RNG_START);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
#endif
    return ERR_CRY_OK;
}

/**
 * @brief          	This function is used to get random digit content.
 *
 * @param[in]   	out 			Pointer to the output of random digit.
 * @param[in]   	len 			The random digit bit number will output.
 *
 * @retval  		0  			success
 * @retval  		other   		failed
 *
 * @note             	None
 */
int tls_crypto_random_bytes(unsigned char *out, u32 len)
{
    unsigned int val;
    uint32 inLen = len;
    int randomBytes = 2;
#if USE_TRNG
	delay_cnt(1000);
	randomBytes = 4;
#else
    val = tls_reg_read32(HR_CRYPTO_SEC_CFG);
    randomBytes = val & (1 << RNG_SWITCH) ? 4 : 2;
#endif
    while(inLen > 0)
    {
#if USE_TRNG
		while (!g_crypto_ctx.gpsec_complete)
		{

		}
		g_crypto_ctx.gpsec_complete = 0;
#endif
        val = tls_reg_read32(HR_CRYPTO_RNG_RESULT);
        if(inLen >= randomBytes)
        {
            memcpy(out, (char *)&val, randomBytes);
            out += randomBytes;
            inLen -= randomBytes;
        }
        else
        {
            memcpy(out, (char *)&val, inLen);
            inLen = 0;
        }
    }
    //tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);	
    return ERR_CRY_OK;
}

/**
 * @brief          	This function is used to generate true random number.
 *
 * @param[in]   	out 			Pointer to the output of random number.
 * @param[in]   	len 			The random number length.
 *
 * @retval  		0  			success
 * @retval  		other   		failed
 *
 * @note           	None
 */
int tls_crypto_trng(unsigned char *out, u32 len)
{
    unsigned int sec_cfg, val;
    uint32 inLen = len;	
    int randomBytes = 4;

	tls_crypto_sem_lock();	
	tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);	
	sec_cfg = (1 << TRNG_INT_MASK) | (4 << TRNG_CP) | (1 << TRNG_SEL) | (1 << TRNG_EN);
	tls_reg_write32(HR_CRYPTO_TRNG_CR, sec_cfg);
	sec_cfg &= ~(1 << TRNG_INT_MASK);
	tls_reg_write32(HR_CRYPTO_TRNG_CR, sec_cfg);		
	delay_cnt(1000);
	while(inLen > 0)
	{
		g_crypto_ctx.gpsec_complete = 0;
		while (!g_crypto_ctx.gpsec_complete)
		{

		}
		g_crypto_ctx.gpsec_complete = 0;
		val = tls_reg_read32(HR_CRYPTO_RNG_RESULT);
		if(inLen >= randomBytes)
		{
			memcpy(out, (char *)&val, randomBytes);
			out += randomBytes;
			inLen -= randomBytes;
		}
		else
		{
			memcpy(out, (char *)&val, inLen);
			inLen = 0;
		}
	}

	tls_reg_write32(HR_CRYPTO_TRNG_CR, 0x40);
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();	
    return ERR_CRY_OK;
}


int tls_crypto_random_bytes_range(unsigned char *out, u32 len, u32 range)
{
	unsigned int val, i;

	val = tls_reg_read32(HR_CRYPTO_SEC_CFG);
	for(i = 0; i< len; i++) {
		val =  tls_reg_read32(HR_CRYPTO_RNG_RESULT);
		out[i] = val % range;
		// printf("rand val:%d, val:%d\r\n", val, out[i]);
	}
	return ERR_CRY_OK;
}


/**
 * @brief          	This function initializes a RC4 encryption algorithm,
 *				i.e. fills the psCipherContext_t structure pointed to by ctx with necessary data.
 *
 * @param[in]   	ctx 		Pointer to the Cipher Context.
 * @param[in]   	key 		Pointer to the key.
 * @param[in]   	keylen 	the length of key.
 *
 * @retval  		0  		success
 * @retval  		other   	failed

 *
 * @note             	The first parameter ctx must be a structure which is allocated externally.
 *      			And all of Context parameters in the initializing methods should be allocated externally too.
 */
int tls_crypto_rc4_init(psCipherContext_t *ctx, const unsigned char *key, u32 keylen)
{
    if(keylen != 16 && keylen != 32)
    {
        return ERR_FAILURE;
    }
    memcpy(ctx->arc4.state, key, keylen);
    ctx->arc4.byteCount = keylen;
    return ERR_CRY_OK;
}


/**
 * @brief          	This function encrypts a variable length data stream according to RC4.
 *				The RC4 algorithm it generates a "keystream" which is simply XORed with the plaintext to produce the ciphertext stream.
 *				Decryption is exactly the same as encryption. This function also decrypts a variable length data stream according to RC4.
 *
 * @param[in]   	ctx 		Pointer to the Cipher Context.
 * @param[in]   	in 		Pointer to the input plaintext data stream(or the encrypted text data stream) of variable length.
 * @param[in]   	out 		Pointer to the resulting ciphertext data stream.
 * @param[in]		len 		Length of the plaintext data stream in octets.
 *
 * @retval  		0  		success
 * @retval  		other   	failed
 *
 * @note             	None
 */
int tls_crypto_rc4(psCipherContext_t *ctx, unsigned char *in, unsigned char *out, u32 len)
{
    unsigned int sec_cfg;
    unsigned char *key = ctx->arc4.state;
    u32 keylen = ctx->arc4.byteCount;
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_crypto_set_key(key, keylen);
    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)in);
    tls_reg_write32(HR_CRYPTO_DEST_ADDR, (unsigned int)out);
    sec_cfg = (CRYPTO_METHOD_RC4 << 16) | (1 << SOFT_RESET_RC4) | (len & 0xFFFF);
    if(keylen == 32)
    {
        sec_cfg |= (1 << 31);
    }
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    CRYPTO_LOG("[%d]:rc4[%d] start\n", sys_count, len);
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    CRYPTO_LOG("[%d]:rc4 end status: %x\n", sys_count, tls_reg_read32(HR_CRYPTO_SEC_STS));
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
    return ERR_CRY_OK;
}


/**
 * @brief          	This function initializes a AES encryption algorithm,  i.e. fills the psCipherContext_t structure pointed to by ctx with necessary data.
 *
 * @param[in]   	ctx 		Pointer to the Cipher Context.
 * @param[in]   	IV 		Pointer to the Initialization Vector
 * @param[in]   	key 		Pointer to the key.
 * @param[in]		keylen 	the length of key.
 * @param[in]   	cbc 		the encryption mode, AES supports ECB/CBC/CTR modes.
 *
 * @retval  		0  		success
 * @retval  		other   	failed
 *
 * @note             	None
 */
int tls_crypto_aes_init(psCipherContext_t *ctx, const unsigned char *IV, const unsigned char *key, u32 keylen, CRYPTO_MODE cbc)
{
    int x = 0;
    if (keylen != 16)
        return ERR_FAILURE;

    memcpy(ctx->aes.key.skey, key, keylen);
    ctx->aes.key.type = cbc;
    ctx->aes.key.rounds = 16;
	if(IV)
	{
	    for (x = 0; x < ctx->aes.key.rounds; x++)
	    {
	        ctx->aes.IV[x] = IV[x];
	    }
	}
    return ERR_CRY_OK;
}

/**
 * @brief			This function encrypts or decrypts a variable length data stream according to AES.
 *
 * @param[in]		ctx 		Pointer to the Cipher Context.
 * @param[in]		in 		Pointer to the input plaintext data stream(or the encrypted text data stream) of variable length.
 * @param[in]		out 		Pointer to the resulting ciphertext data stream.
 * @param[in]		len 		Length of the plaintext data stream in octets.
 * @param[in]		dec 		The cryption way which indicates encryption or decryption.
 *
 * @retval		0  		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_aes_encrypt_decrypt(psCipherContext_t *ctx, unsigned char *in, unsigned char *out, u32 len, CRYPTO_WAY dec)
{
    unsigned int sec_cfg;
    u32 keylen = 16;
    unsigned char *key = (unsigned char *)ctx->aes.key.skey;
    unsigned char *IV = ctx->aes.IV;
    CRYPTO_MODE cbc = (CRYPTO_MODE)(ctx->aes.key.type & 0xFF);
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_crypto_set_key(key, keylen);
    tls_crypto_set_iv(IV, 16);

    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)in);
    tls_reg_write32(HR_CRYPTO_DEST_ADDR, (unsigned int)out);
    sec_cfg = (CRYPTO_METHOD_AES << 16) | (1 << SOFT_RESET_AES) | (dec << 20) | (cbc << 21) | (len & 0xFFFF);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    CRYPTO_LOG("[%d]:aes[%d] %s %s start\n", sys_count, len, dec == CRYPTO_WAY_ENCRYPT ? "ENCRYPT" : "DECRYPT",
               cbc == CRYPTO_MODE_ECB ? "ECB" : (cbc == CRYPTO_MODE_CBC ? "CBC" : (cbc == CRYPTO_MODE_CTR ? "CTR" : "MAC")));
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    CRYPTO_LOG("[%d]:aes end %d\n", sys_count, tls_reg_read32(HR_CRYPTO_SEC_STS) & 0xFFFF);
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
    return ERR_CRY_OK;
}

/**
 * @brief			This function initializes a 3DES encryption algorithm,  i.e. fills the psCipherContext_t structure pointed to by ctx with necessary data.
 *
 * @param[in]		ctx 		Pointer to the Cipher Context.
 * @param[in]		IV 		Pointer to the Initialization Vector
 * @param[in]		key 		Pointer to the key.
 * @param[in]		keylen 	the length of key.
 * @param[in]		cbc 		the encryption mode, 3DES supports ECB/CBC modes.
 *
 * @retval		0  		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_3des_init(psCipherContext_t *ctx, const unsigned char *IV, const unsigned char *key, u32 keylen, CRYPTO_MODE cbc)
{
    unsigned int x;
    if (keylen != DES3_KEY_LEN)
        return ERR_FAILURE;

    memcpy(ctx->des3.key.ek[0], key, keylen);
    ctx->des3.key.ek[1][0] =  cbc;
    ctx->des3.blocklen = DES3_IV_LEN;
	if(IV)
	{
	    for (x = 0; x < ctx->des3.blocklen; x++)
	    {
	        ctx->des3.IV[x] = IV[x];
	    }
	}

    return ERR_CRY_OK;
}

/**
 * @brief			This function encrypts or decrypts a variable length data stream according to 3DES.
 *
 * @param[in]		ctx 		Pointer to the Cipher Context.
 * @param[in]		in 		Pointer to the input plaintext data stream(or the encrypted text data stream) of variable length.
 * @param[in]		out 		Pointer to the resulting ciphertext data stream.
 * @param[in]		len 		Length of the plaintext data stream in octets.
 * @param[in]		dec 		The cryption way which indicates encryption or decryption.
 *
 * @retval		0  		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_3des_encrypt_decrypt(psCipherContext_t *ctx, unsigned char *in, unsigned char *out, u32 len, CRYPTO_WAY dec)
{
    unsigned int sec_cfg;
    u32 keylen = DES3_KEY_LEN;
    unsigned char *key = (unsigned char *)(unsigned char *)ctx->des3.key.ek[0];
    unsigned char *IV = ctx->des3.IV;
    CRYPTO_MODE cbc = (CRYPTO_MODE)(ctx->des3.key.ek[1][0] & 0xFF);
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_crypto_set_key(key, keylen);
    tls_crypto_set_iv(IV, DES3_IV_LEN);
    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)in);
    tls_reg_write32(HR_CRYPTO_DEST_ADDR, (unsigned int)out);
    sec_cfg = (CRYPTO_METHOD_3DES << 16) | (1 << SOFT_RESET_DES) | (dec << 20) | (cbc << 21) | (len & 0xFFFF);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    CRYPTO_LOG("[%d]:3des[%d] %s %s start\n", sys_count, len, dec == CRYPTO_WAY_ENCRYPT ? "ENCRYPT" : "DECRYPT",
               cbc == CRYPTO_MODE_ECB ? "ECB" : "CBC");
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    CRYPTO_LOG("[%d]:3des end %d\n", sys_count, tls_reg_read32(HR_CRYPTO_SEC_STS) & 0xFFFF);
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
    return ERR_CRY_OK;
}


/**
 * @brief			This function initializes a DES encryption algorithm,  i.e. fills the psCipherContext_t structure pointed to by ctx with necessary data.
 *
 * @param[in]		ctx 		Pointer to the Cipher Context.
 * @param[in]		IV 		Pointer to the Initialization Vector
 * @param[in]		key 		Pointer to the key.
 * @param[in]		keylen 	the length of key.
 * @param[in]		cbc 		the encryption mode, DES supports ECB/CBC modes.
 *
 * @retval		0  		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_des_init(psCipherContext_t *ctx, const unsigned char *IV, const unsigned char *key, u32 keylen, CRYPTO_MODE cbc)
{
    unsigned int x;
    if (keylen != DES_KEY_LEN)
        return ERR_FAILURE;
    memcpy(ctx->des3.key.ek[0], key, keylen);
    ctx->des3.key.ek[1][0] =  cbc;
    ctx->des3.blocklen = DES3_IV_LEN;
	if(IV)
	{
	    for (x = 0; x < ctx->des3.blocklen; x++)
	    {
	        ctx->des3.IV[x] = IV[x];
	    }
	}
    return ERR_CRY_OK;
}


/**
 * @brief			This function encrypts or decrypts a variable length data stream according to DES.
 *
 * @param[in]		ctx 		Pointer to the Cipher Context.
 * @param[in]		in 		Pointer to the input plaintext data stream(or the encrypted text data stream) of variable length.
 * @param[in]		out 		Pointer to the resulting ciphertext data stream.
 * @param[in]		len 		Length of the plaintext data stream in octets.
 * @param[in]		dec 		The cryption way which indicates encryption or decryption.
 *
 * @retval		0  		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_des_encrypt_decrypt(psCipherContext_t *ctx, unsigned char *in, unsigned char *out, u32 len, CRYPTO_WAY dec)
{
    unsigned int sec_cfg;
    u32 keylen = DES_KEY_LEN;
    unsigned char *key = (unsigned char *)ctx->des3.key.ek[0];
    unsigned char *IV = ctx->des3.IV;
    CRYPTO_MODE cbc = (CRYPTO_MODE)(ctx->des3.key.ek[1][0] & 0xFF);
    //uint32_t *IV32 = (uint32_t *)IV;

	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_crypto_set_key(key, keylen);
    tls_crypto_set_iv(IV, DES3_IV_LEN);
    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)in);
    tls_reg_write32(HR_CRYPTO_DEST_ADDR, (unsigned int)out);
    sec_cfg = (CRYPTO_METHOD_DES << 16) | (1 << SOFT_RESET_DES) | (dec << 20) | (cbc << 21) | (len & 0xFFFF);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    CRYPTO_LOG("[%d]:des[%d] %s %s start\n", sys_count, len, dec == CRYPTO_WAY_ENCRYPT ? "ENCRYPT" : "DECRYPT",
               cbc == CRYPTO_MODE_ECB ? "ECB" : "CBC");
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    CRYPTO_LOG("[%d]:des end %d\n", sys_count, tls_reg_read32(HR_CRYPTO_SEC_STS) & 0xFFFF);
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();

    return ERR_CRY_OK;
}


/**
 * @brief			This function initializes a CRC algorithm,  i.e. fills the psCrcContext_t structure pointed to by ctx with necessary data.
 *
 * @param[in]		ctx 		Pointer to the CRC Context.
 * @param[in]		key 		The initialization key.
 * @param[in]		crc_type 	The CRC type, supports CRC8/CRC16 MODBUS/CRC16 CCITT/CRC32
 * @param[in]		mode 	Set input or outpu reflect.
 * @param[in]		dec 		The cryption way which indicates encryption or decryption.
 *				see OUTPUT_REFLECT
 * 				see INPUT_REFLECT
 *
 * @retval		0		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_crc_init(psCrcContext_t *ctx, u32 key, CRYPTO_CRC_TYPE crc_type, u8 mode)
{
    ctx->state = key;
    ctx->type = crc_type;
    ctx->mode = mode;
    return ERR_CRY_OK;
}

/**
 * @brief			This function updates the CRC value with a variable length bytes.
 *				This function may be called as many times as necessary, so the message may be processed in blocks.
 *
 * @param[in]		ctx 		Pointer to the CRC Context.
 * @param[in]		in 		Pointer to a variable length bytes
 * @param[in]		len 		The bytes 's length
 *
 * @retval		0		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_crc_update(psCrcContext_t *ctx, unsigned char *in, u32 len)
{
    unsigned int sec_cfg;
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    sec_cfg =  (CRYPTO_METHOD_CRC << 16) | (ctx->type << 21) | (ctx->mode << 23) | (len & 0xFFFF);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    if(ctx->mode & OUTPUT_REFLECT)
    {
        u8 ch_crc = 16;
        u32 state = 0;
        switch(ctx->type)
        {
        case CRYPTO_CRC_TYPE_8:
            ch_crc = 8;
            break;
        case CRYPTO_CRC_TYPE_16_MODBUS:
            ch_crc = 16;
            break;
        case CRYPTO_CRC_TYPE_16_CCITT:
            ch_crc = 16;
            break;
        case CRYPTO_CRC_TYPE_32:
            ch_crc = 32;
            break;
        default:
            break;
        }
        state = Reflect(ctx->state, ch_crc);
        tls_reg_write32(HR_CRYPTO_CRC_KEY, state);
    }
    else
        tls_reg_write32(HR_CRYPTO_CRC_KEY, ctx->state);

    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)in);
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    ctx->state = tls_reg_read32(HR_CRYPTO_CRC_RESULT);
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x4);//clear crc fifo
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
    return ERR_CRY_OK;
}


/**
 * @brief			This function ends a CRC operation and produces a CRC value.
 *
 * @param[in]		ctx 		Pointer to the CRC Context.
 * @param[in]		crc_val 	Pointer to the CRC value.
 *
 * @retval		0		success
 * @retval		other	failed
 *
 * @note			None
 */
int tls_crypto_crc_final(psCrcContext_t *ctx, u32 *crc_val)
{
    *crc_val = ctx->state;
    return ERR_CRY_OK;
}

static void hd_sha1_compress(psDigestContext_t *md)
{
    unsigned int sec_cfg, val;
    int i = 0;
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)md->u.sha1.buf);

    sec_cfg = (CRYPTO_METHOD_SHA1 << 16) | (64 & 0xFFFF); // TODO
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST0, md->u.sha1.state[0]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST1, md->u.sha1.state[1]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST2, md->u.sha1.state[2]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST3, md->u.sha1.state[3]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST4, md->u.sha1.state[4]);
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    for (i = 0; i < 5; i++)
    {
        val = tls_reg_read32(HR_CRYPTO_SHA1_DIGEST0 + (4 * i));
        md->u.sha1.state[i] = val;
    }
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
}


/**
 * @brief			This function initializes Message-Diggest context for usage in SHA1 algorithm, starts a new SHA1 operation and writes a new Digest Context.
 *
 * @param[in]		md 		Pointer to the SHA1 Digest Context.
 *
 * @retval		0		success
 * @retval		other	failed
 *
 * @note			None
 */
void tls_crypto_sha1_init(psDigestContext_t *md)
{
    md->u.sha1.state[0] = 0x67452301UL;
    md->u.sha1.state[1] = 0xefcdab89UL;
    md->u.sha1.state[2] = 0x98badcfeUL;
    md->u.sha1.state[3] = 0x10325476UL;
    md->u.sha1.state[4] = 0xc3d2e1f0UL;
    md->u.sha1.curlen = 0;
#ifdef HAVE_NATIVE_INT64
    md->u.sha1.length = 0;
#else
    md->u.sha1.lengthHi = 0;
    md->u.sha1.lengthLo = 0;
#endif /* HAVE_NATIVE_INT64 */
}


/**
 * @brief			Process a message block using SHA1 algorithm.
 *				This function performs a SHA1 block update operation. It continues an SHA1 message-digest operation,
 *				by processing InputLen-byte length message block pointed to by buf, and by updating the SHA1 context pointed to by md.
 *				This function may be called as many times as necessary, so the message may be processed in blocks.
 *
 * @param[in]		md		Pointer to the SHA1 Digest Context.
 * @param[in]  	buf 		InputLen-byte length message block
 * @param[in]  	len 		The buf 's length
 *
 * @returnl		None
 *
 * @note			None
 */
void tls_crypto_sha1_update(psDigestContext_t *md, const unsigned char *buf, u32 len)
{
    u32 n;
    while (len > 0)
    {
        n = min(len, (64 - md->u.sha1.curlen));
        memcpy(md->u.sha1.buf + md->u.sha1.curlen, buf, (size_t)n);
        md->u.sha1.curlen		+= n;
        buf					+= n;
        len					-= n;

        /* is 64 bytes full? */
        if (md->u.sha1.curlen == 64)
        {
            hd_sha1_compress(md);
#ifdef HAVE_NATIVE_INT64
            md->u.sha1.length += 512;
#else
            n = (md->u.sha1.lengthLo + 512) & 0xFFFFFFFFL;
            if (n < md->u.sha1.lengthLo)
            {
                md->u.sha1.lengthHi++;
            }
            md->u.sha1.lengthLo = n;
#endif /* HAVE_NATIVE_INT64 */
            md->u.sha1.curlen = 0;
        }
    }
}


/**
 * @brief			This function ends a SHA1 operation and produces a Message-Digest.
 *				This function finalizes SHA1 algorithm, i.e. ends an SHA1 Message-Digest operation,
 *				writing the Message-Digest in the 20-byte buffer pointed to by hash in according to the information stored in context.
 *
 * @param[in]		md		Pointer to the SHA1 Digest Context.
 * @param[in]		hash 	Pointer to the Message-Digest
 *
 * @retval  		20  		success, return the hash size.
 * @retval  		<0   	failed

 *
 * @note			None
 */
int tls_crypto_sha1_final(psDigestContext_t *md, unsigned char *hash)
{
    s32	i;
    u32 val;
#ifndef HAVE_NATIVE_INT64
    u32	n;
#endif
    if (md->u.sha1.curlen >= sizeof(md->u.sha1.buf) || hash == NULL)
    {
        return ERR_ARG_FAIL;
    }

    /*
    	increase the length of the message
     */
#ifdef HAVE_NATIVE_INT64
    md->u.sha1.length += md->u.sha1.curlen << 3;
#else
    n = (md->u.sha1.lengthLo + (md->u.sha1.curlen << 3)) & 0xFFFFFFFFL;
    if (n < md->u.sha1.lengthLo)
    {
        md->u.sha1.lengthHi++;
    }
    md->u.sha1.lengthHi += (md->u.sha1.curlen >> 29);
    md->u.sha1.lengthLo = n;
#endif /* HAVE_NATIVE_INT64 */

    /*
    	append the '1' bit
     */
    md->u.sha1.buf[md->u.sha1.curlen++] = (unsigned char)0x80;

    /*
    	if the length is currently above 56 bytes we append zeros then compress.
    	Then we can fall back to padding zeros and length encoding like normal.
     */
    if (md->u.sha1.curlen > 56)
    {
        while (md->u.sha1.curlen < 64)
        {
            md->u.sha1.buf[md->u.sha1.curlen++] = (unsigned char)0;
        }
        hd_sha1_compress(md);
        md->u.sha1.curlen = 0;
    }

    /*
    	pad upto 56 bytes of zeroes
     */
    while (md->u.sha1.curlen < 56)
    {
        md->u.sha1.buf[md->u.sha1.curlen++] = (unsigned char)0;
    }

    /*
    	store length
     */
#ifdef HAVE_NATIVE_INT64
    STORE64H(md->u.sha1.length, md->u.sha1.buf + 56);
#else
    STORE32H(md->u.sha1.lengthHi, md->u.sha1.buf + 56);
    STORE32H(md->u.sha1.lengthLo, md->u.sha1.buf + 60);
#endif /* HAVE_NATIVE_INT64 */
    hd_sha1_compress(md);

    /*
    	copy output
     */
    for (i = 0; i < 5; i++)
    {
        val = tls_reg_read32(HR_CRYPTO_SHA1_DIGEST0 + (4 * i));
        STORE32H(val, hash + (4 * i));
    }
    memset(md, 0x0, sizeof(psSha1_t));

    return SHA1_HASH_SIZE;
}

static void hd_md5_compress(psDigestContext_t *md)
{
    unsigned int sec_cfg, val, i;
	tls_crypto_sem_lock();
    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
    tls_reg_write32(HR_CRYPTO_SRC_ADDR, (unsigned int)md->u.md5.buf);
    sec_cfg = (CRYPTO_METHOD_MD5 << 16) |  (64 & 0xFFFF);
    tls_reg_write32(HR_CRYPTO_SEC_CFG, sec_cfg);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST0, md->u.md5.state[0]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST1, md->u.md5.state[1]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST2, md->u.md5.state[2]);
    tls_reg_write32(HR_CRYPTO_SHA1_DIGEST3, md->u.md5.state[3]);
    g_crypto_ctx.gpsec_complete = 0;
    tls_reg_write32(HR_CRYPTO_SEC_CTRL, 0x1);//start crypto
    while (!g_crypto_ctx.gpsec_complete)
    {

    }
    g_crypto_ctx.gpsec_complete = 0;
    for (i = 0; i < 4; i++)
    {
        val = tls_reg_read32(HR_CRYPTO_SHA1_DIGEST0 + (4 * i));
        md->u.md5.state[i] = val;
    }

    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_GPSEC);
	tls_crypto_sem_unlock();
}


/**
 * @brief			This function initializes Message-Diggest context for usage in MD5 algorithm, starts a new MD5 operation and writes a new Digest Context.
 *				This function begins a MD5 Message-Diggest Algorithm, i.e. fills the psDigestContext_t structure pointed to by md with necessary data.
 *				MD5 is the algorithm which takes as input a message of arbitrary length and produces as output a 128-bit "fingerprint" or "message digest" of the input.
 *				It is conjectured that it is computationally infeasible to produce two messages having the same message digest,
 *				or to produce any message having a given prespecified target message digest.
 *
 * @param[in]		md		MD5 Digest Context.
 *
 * @return		None
 *
 * @note			None
 */
void tls_crypto_md5_init(psDigestContext_t *md)
{
    md->u.md5.state[0] = 0x67452301UL;
    md->u.md5.state[1] = 0xefcdab89UL;
    md->u.md5.state[2] = 0x98badcfeUL;
    md->u.md5.state[3] = 0x10325476UL;
    md->u.md5.curlen = 0;
#ifdef HAVE_NATIVE_INT64
    md->u.md5.length = 0;
#else
    md->u.md5.lengthHi = 0;
    md->u.md5.lengthLo = 0;
#endif /* HAVE_NATIVE_INT64 */
}


/**
 * @brief			Process a message block using MD5 algorithm.
 *				This function performs a MD5 block update operation. It continues an MD5 message-digest operation,
 *				by processing InputLen-byte length message block pointed to by buf, and by updating the MD5 context pointed to by md.
 *				This function may be called as many times as necessary, so the message may be processed in blocks.
 *
 * @param[in]		md		MD5 Digest Context.
 * @param[in]  	buf 		InputLen-byte length message block
 * @param[in]  	len 		The buf 's length
 *
 * @return		None
 *
 * @note			None
 */
void tls_crypto_md5_update(psDigestContext_t *md, const unsigned char *buf, u32 len)
{
    u32 n;

    while (len > 0)
    {
        n = min(len, (64 - md->u.md5.curlen));
        memcpy(md->u.md5.buf + md->u.md5.curlen, buf, (size_t)n);
        md->u.md5.curlen	+= n;
        buf				+= n;
        len				-= n;

        /*
        		is 64 bytes full?
         */
        if (md->u.md5.curlen == 64)
        {
            hd_md5_compress(md);
#ifdef HAVE_NATIVE_INT64
            md->u.md5.length += 512;
#else
            n = (md->u.md5.lengthLo + 512) & 0xFFFFFFFFL;
            if (n < md->u.md5.lengthLo)
            {
                md->u.md5.lengthHi++;
            }
            md->u.md5.lengthLo = n;
#endif /* HAVE_NATIVE_INT64 */
            md->u.md5.curlen = 0;
        }
    }
}

/**
 * @brief			This function ends a MD5 operation and produces a Message-Digest.
 *				This function finalizes MD5 algorithm, i.e. ends an MD5 Message-Digest operation,
 *				writing the Message-Digest in the 16-byte buffer pointed to by hash in according to the information stored in context.
 *
 * @param[in]		md		MD5 Digest Context.
 * @param[in]		hash 	the Message-Digest
 *
 * @retval  		16  		success, return the hash size.
 * @retval  		<0   	failed
 *
 * @note			None
 */
s32 tls_crypto_md5_final(psDigestContext_t *md, unsigned char *hash)
{
    s32 i;
    u32 val;
#ifndef HAVE_NATIVE_INT64
    u32	n;
#endif

    //	psAssert(md != NULL);
    if (hash == NULL)
    {
        CRYPTO_LOG("NULL hash storage passed to psMd5Final\n");
        return PS_ARG_FAIL;
    }

    /*
    	increase the length of the message
     */
#ifdef HAVE_NATIVE_INT64
    md->u.md5.length += md->u.md5.curlen << 3;
#else
    n = (md->u.md5.lengthLo + (md->u.md5.curlen << 3)) & 0xFFFFFFFFL;
    if (n < md->u.md5.lengthLo)
    {
        md->u.md5.lengthHi++;
    }
    md->u.md5.lengthHi += (md->u.md5.curlen >> 29);
    md->u.md5.lengthLo = n;
#endif /* HAVE_NATIVE_INT64 */

    /*
    	append the '1' bit
     */
    md->u.md5.buf[md->u.md5.curlen++] = (unsigned char)0x80;

    /*
    	if the length is currently above 56 bytes we append zeros then compress.
    	Then we can fall back to padding zeros and length encoding like normal.
     */
    if (md->u.md5.curlen > 56)
    {
        while (md->u.md5.curlen < 64)
        {
            md->u.md5.buf[md->u.md5.curlen++] = (unsigned char)0;
        }
        hd_md5_compress(md);
        md->u.md5.curlen = 0;
    }

    /*
    	pad upto 56 bytes of zeroes
     */
    while (md->u.md5.curlen < 56)
    {
        md->u.md5.buf[md->u.md5.curlen++] = (unsigned char)0;
    }
    /*
    	store length
     */
#ifdef HAVE_NATIVE_INT64
    STORE64L(md->u.md5.length, md->u.md5.buf + 56);
#else
    STORE32L(md->u.md5.lengthLo, md->u.md5.buf + 56);
    STORE32L(md->u.md5.lengthHi, md->u.md5.buf + 60);
#endif /* HAVE_NATIVE_INT64 */
    hd_md5_compress(md);

    /*
    	copy output
     */
    for (i = 0; i < 4; i++)
    {
        val = tls_reg_read32(HR_CRYPTO_SHA1_DIGEST0 + (4 * i));
        STORE32L(val, hash + (4 * i));
    }
    memset(md, 0x0, sizeof(psMd5_t));

    return MD5_HASH_SIZE;
}

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
static int rsaMulModRead(unsigned char w, hstm_int *a)
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
    pstm_reverse((unsigned char *)in, RSAN * sizeof(u32));
    /* this a should be initialized outside. */
    //if ((err = pstm_init_for_read_unsigned_bin(NULL, a, RSAN * sizeof(u32) + sizeof(hstm_int))) != ERR_CRY_OK){
    //	return err;
    //}
    if ((err = pstm_read_unsigned_bin(a, (unsigned char *)in, RSAN * sizeof(u32))) != ERR_CRY_OK)
    {
        pstm_clear(a);
        return err;
    }
    return 0;
}
#if 0
static void rsaMulModDump(unsigned char w)
{
	extern void dumpUint32(char *name, uint32_t* buffer, int len);
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
static void rsaMulModWrite(unsigned char w, hstm_int *a)
{
    u32 in[64];
    memset(in, 0, 64 * sizeof(u32));
    pstm_to_unsigned_bin_nr(NULL, a, (unsigned char *)in);
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


/**
 * @brief			This function implements the large module power multiplication algorithm.
 *				res = a**e (mod n)
 *
 * @param[in]		a 		Pointer to a bignumber.
 * @param[in]		e 		Pointer to a bignumber.
 * @param[in]  	n 		Pointer to a bignumber.
 * @param[out]  	res 		Pointer to the result bignumber.
 *
 * @retval  		0  		success
 * @retval  		other   	failed
 *
 * @note			None
 */
int tls_crypto_exptmod(hstm_int *a, hstm_int *e, hstm_int *n, hstm_int *res)
{
    int i = 0;
    u32 k = 0, mc = 0, dp0;
    volatile u8 monmulFlag = 0;
    hstm_int R, X, Y;

    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_RSA);

#ifndef CONFIG_KERNEL_NONE
    tls_fls_sem_lock();
#endif
    pstm_init(NULL, &X);
    pstm_init(NULL, &Y);
    pstm_init(NULL, &R);
    k = pstm_count_bits(n);//n->used * DIGIT_BIT;//pstm_count_bits(n);
    k = ((k / 32) + (k % 32 > 0 ? 1 : 0)) * 32;
#if 0
    pstm_set(&Y, k);
    pstm_set(&X, 2);
    pstm_exptmod(NULL, &X, &Y, n, &R); //R = 2^k % n
#else
    pstm_2expt(&X, (int16)k); //X = 2^k
    pstm_mod(NULL, &X, n, &R); //R = 2^k % n
#endif
    //pstm_set(&Y, 1);
    pstm_mulmod(NULL, a, &R, n, &X); //X = A * R
    pstm_copy(&R, &Y);
    if(n->used > 1)
    {
#if (DIGIT_BIT < 32)
        dp0 = 0xFFFFFFFF & ((n->dp[0]) | (u32)(n->dp[1] << DIGIT_BIT));
#else
        dp0 = (n->dp[0]);
#endif
    }
    else
        dp0 = n->dp[0];
    rsaCalMc(&mc, dp0);
    k = pstm_count_bits(n);
    rsaMonMulSetLen(k / 32 + (k % 32 == 0 ? 0 : 1));
    rsaMonMulWriteMc(mc);
    rsaMulModWrite('M', n);
    rsaMulModWrite('B', &X);
    rsaMulModWrite('A', &Y);
    k = pstm_count_bits(e);
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

        if(pstm_get_bit(e, i))
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
    pstm_set(&R, 1);
    rsaMulModWrite('B', &R);
    //montMulMod(&Y, &R, n, res);
    if(monmulFlag == 0)
    {
        rsaMonMulAB();
        rsaMulModRead('D', res);
    }
    else
    {
        rsaMonMulBD();
        rsaMulModRead('A', res);
    }
    pstm_clamp(res);
    pstm_clear(&X);
    pstm_clear(&Y);
    pstm_clear(&R);
#ifndef CONFIG_KERNEL_NONE
    tls_fls_sem_unlock();
#endif
    tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_RSA);

    return 0;
}


/**
 * @brief			This function initializes the encryption module.
 *
 * @param		None
 *
 * @return  		None
 *
 * @note			None
 */
int tls_crypto_init(void)
{
#ifndef CONFIG_KERNEL_NONE
	int err = 0;
	if(g_crypto_ctx.gpsec_lock != NULL)
	{
		return 0;
	}
	err = tls_os_sem_create(&g_crypto_ctx.gpsec_lock, 1);
    if (err != TLS_OS_SUCCESS)
    {
        TLS_DBGPRT_ERR("create semaphore @gpsec_lock fail!\n");
        return -1;
    }
#endif
    tls_irq_enable(RSA_IRQn);
    tls_irq_enable(CRYPTION_IRQn);
	return 0;
}


/**
 * @brief        	This function is used to generate true random number seed.
 *
 * @param[in]   	None
 *
 * @retval  		random number
 *
 * @note         	None
 */
unsigned int tls_random_seed_generation(void)
{
#if 0
	extern void delay_cnt(int count);
	unsigned int val;
	unsigned int seed;
	val = tls_reg_read32(HR_CRYPTO_TRNG_CR);
	tls_reg_write32(HR_CRYPTO_TRNG_CR, val|(1 << TRNG_SEL));
	delay_cnt(2000);
	seed = tls_reg_read32(HR_CRYPTO_RNG_RESULT);
	tls_reg_write32(HR_CRYPTO_TRNG_CR, val);
	return seed;
#else
	return csi_coret_get_value();
#endif	
}


