#include <string.h>
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "wm_demo.h"


#if DEMO_ENCRYPT 

void RNG_hard_demo(void)
{
	u8 *out = NULL;
	u16 i;

	out = tls_mem_alloc(1000);
	
	tls_crypto_random_init(0xFDA3C97A, CRYPTO_RNG_SWITCH_16);
	tls_crypto_random_bytes(out, 10);

	printf("RNG out:\n");
	for(i=0; i<10; i++)
	{
		printf("%X ", out[i]);
	}
	printf("\n");

	tls_crypto_random_bytes(out, 20);

	printf("RNG out:\n");
	for(i=0; i<20; i++)
	{
		printf("%X ", out[i]);
	}
	printf("\n");
	
	tls_crypto_random_stop();

	tls_mem_free(out);
}

int rc4_hard_demo(void)
{
	psCipherContext_t ctx;
	u8 *in = NULL;
	u8 *out1 = NULL;
	u8 *out2 = NULL;
	u8 *key = NULL;
	int i;

	tls_crypto_init();
	in = tls_mem_alloc(1000);
	out1 = tls_mem_alloc(1000);
	out2 = tls_mem_alloc(1000);
	key = tls_mem_alloc(16);

	if((in==NULL) || (out1==NULL) || (out2==NULL) || (key==NULL))
	{
		printf("malloc err\n");
		goto OUT;
	}

	for(i=0; i<1000; i++)
	{
		in[i] = rand();
	}

	for(i=0; i<16; i++)
	{
		key[i] = rand();
	}

	memset(out1, 0, 1000);
	memset(out2, 0, 1000);

	if(ERR_OK != tls_crypto_rc4_init(&ctx, key, 16))
	{
		printf("rc4 init err\n");
		goto OUT;
	}
	tls_crypto_rc4(&ctx, in, out1, 1000);
	tls_crypto_rc4(&ctx, out1, out2, 1000);

	if(memcmp(in, out2, 1000))
	{
		printf("rc4 test fail\n");
	}
	else
	{
		printf("rc4 test success\n");
	}

OUT:
	if(in != NULL)
		tls_mem_free(in);
	if(out1 != NULL)
		tls_mem_free(out1);
	if(out2 != NULL)
		tls_mem_free(out2);
	if(key != NULL)
		tls_mem_free(key);
	
	return 0;
}


int aes_hard_demo(void)
{
	psCipherContext_t ctx;
	u8 *in = NULL;
	u8 *out1 = NULL;
	u8 *out2 = NULL;
	u8 *key = NULL;
	u8 *iv = NULL;
	int i;

	tls_crypto_init();

	in = tls_mem_alloc(1024);		//AES 必须是16的整数倍，否则会出错
	out1 = tls_mem_alloc(1024);
	out2 = tls_mem_alloc(1024);
	key = tls_mem_alloc(16);
	iv = tls_mem_alloc(16);

	if((in==NULL) || (out1==NULL) || (out2==NULL) || (key==NULL) || (iv==NULL))
	{
		printf("malloc err\n");
		goto OUT;
	}

	for(i=0; i<1024; i++)
	{
		in[i] = rand();
	}

	for(i=0; i<16; i++)
	{
		key[i] = rand();
		iv[i] = rand();
	}

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_aes_init(&ctx, iv, key, 16, CRYPTO_MODE_ECB);
	tls_crypto_aes_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_aes_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("aes ecb test fail\n");
	}
	else
	{
		printf("aes ecb test success\n");
	}

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_aes_init(&ctx, iv, key, 16, CRYPTO_MODE_CBC);
	tls_crypto_aes_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_aes_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("aes cbc test fail\n");
	}
	else
	{
		printf("aes cbc test success\n");
	}	

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_aes_init(&ctx, iv, key, 16, CRYPTO_MODE_CTR);
	tls_crypto_aes_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_aes_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("aes ctr test fail\n");
	}
	else
	{
		printf("aes ctr test success\n");
	}		

OUT:
	if(in != NULL)
		tls_mem_free(in);
	if(out1 != NULL)
		tls_mem_free(out1);
	if(out2 != NULL)
		tls_mem_free(out2);
	if(key != NULL)
		tls_mem_free(key);
	if(iv != NULL)
		tls_mem_free(iv);
	
	return 0;
	
}


int des_hard_demo(void)
{
	psCipherContext_t ctx;
	u8 *in = NULL;
	u8 *out1 = NULL;
	u8 *out2 = NULL;
	u8 *key = NULL;
	u8 *iv = NULL;
	int i;

	tls_crypto_init();

	in = tls_mem_alloc(1024);		
	out1 = tls_mem_alloc(1024);
	out2 = tls_mem_alloc(1024);
	key = tls_mem_alloc(8);
	iv = tls_mem_alloc(8);

	if((in==NULL) || (out1==NULL) || (out2==NULL) || (key==NULL) || (iv==NULL))
	{
		printf("malloc err\n");
		goto OUT;
	}

	for(i=0; i<1024; i++)
	{
		in[i] = rand();
	}

	for(i=0; i<8; i++)
	{
		key[i] = rand();
		iv[i] = rand();
	}	

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_des_init(&ctx, iv, key, 8, CRYPTO_MODE_ECB);
	tls_crypto_des_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_des_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("des ecb test fail\n");
	}
	else
	{
		printf("des ecb test success\n");
	}	

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_des_init(&ctx, iv, key, 8, CRYPTO_MODE_CBC);
	tls_crypto_des_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_des_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("des cbc test fail\n");
	}
	else
	{
		printf("des cbc test success\n");
	}	

OUT:
	if(in != NULL)
		tls_mem_free(in);
	if(out1 != NULL)
		tls_mem_free(out1);
	if(out2 != NULL)
		tls_mem_free(out2);
	if(key != NULL)
		tls_mem_free(key);
	if(iv != NULL)
		tls_mem_free(iv);
	
	return 0;	

}


int des3_hard_demo(void)
{
	psCipherContext_t ctx;
	u8 *in = NULL;
	u8 *out1 = NULL;
	u8 *out2 = NULL;
	u8 *key = NULL;
	u8 *iv = NULL;
	int i;

	tls_crypto_init();

	in = tls_mem_alloc(1024);		
	out1 = tls_mem_alloc(1024);
	out2 = tls_mem_alloc(1024);
	key = tls_mem_alloc(24);
	iv = tls_mem_alloc(8);

	if((in==NULL) || (out1==NULL) || (out2==NULL) || (key==NULL) || (iv==NULL))
	{
		printf("malloc err\n");
		goto OUT;
	}

	for(i=0; i<1024; i++)
	{
		in[i] = rand();
	}

	for(i=0; i<24; i++)
	{
		key[i] = rand();
	}

	for(i=0; i<8; i++)
	{	
		iv[i] = rand();
	}	

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_3des_init(&ctx, iv, key, 24, CRYPTO_MODE_ECB);
	tls_crypto_3des_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_3des_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("3des ecb test fail\n");
	}
	else
	{
		printf("3des ecb test success\n");
	}

	memset(out1, 0, 1024);
	memset(out2, 0, 1024);
	tls_crypto_3des_init(&ctx, iv, key, 24, CRYPTO_MODE_CBC);
	tls_crypto_3des_encrypt_decrypt(&ctx, in, out1, 1024, CRYPTO_WAY_ENCRYPT);
	tls_crypto_3des_encrypt_decrypt(&ctx, out1, out2, 1024, CRYPTO_WAY_DECRYPT);
	if(memcmp(in, out2, 1024))
	{
		printf("3des cbc test fail\n");
	}
	else
	{
		printf("3des cbc test success\n");
	}	

OUT:
	if(in != NULL)
		tls_mem_free(in);
	if(out1 != NULL)
		tls_mem_free(out1);
	if(out2 != NULL)
		tls_mem_free(out2);
	if(key != NULL)
		tls_mem_free(key);
	if(iv != NULL)
		tls_mem_free(iv);
	
	return 0;

}


int crc_hard_demo(void)
{
	u32 crckey = 0xFFFFFFFF;
	psCrcContext_t ctx;
	u8 *in = NULL;
	int i;
	u32 crcvalue = 0;

	tls_crypto_init();

	in = tls_mem_alloc(1024);
	if(in==NULL)
	{
		printf("malloc err\n");
		goto OUT;
	}

	for(i=0; i<1024; i++)
	{
		in[i] = rand();
	}	
	
	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_8, 0);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_8 normal value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_8, INPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_8 INPUT_REFLECT value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_8, OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_8 OUTPUT_REFLECT value:0x%08X\n", crcvalue);	

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_8, INPUT_REFLECT | OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_8 INPUT_REFLECT | OUTPUT_REFLECT value:0x%08X\n", crcvalue); 


	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_MODBUS, 0);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_MODBUS normal value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_MODBUS, INPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_MODBUS INPUT_REFLECT value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_MODBUS, OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_MODBUS OUTPUT_REFLECT value:0x%08X\n", crcvalue); 

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_MODBUS, INPUT_REFLECT | OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_MODBUS INPUT_REFLECT | OUTPUT_REFLECT value:0x%08X\n", crcvalue); 


	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_CCITT, 0);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_CCITT normal value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_CCITT, INPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_CCITT INPUT_REFLECT value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_CCITT, OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_CCITT OUTPUT_REFLECT value:0x%08X\n", crcvalue); 

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_16_CCITT, INPUT_REFLECT | OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_16_CCITT INPUT_REFLECT | OUTPUT_REFLECT value:0x%08X\n", crcvalue);


	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_32, 0);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_32 normal value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_32, INPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_32 INPUT_REFLECT value:0x%08X\n", crcvalue);

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_32, OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_32 OUTPUT_REFLECT value:0x%08X\n", crcvalue); 

	tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_32, INPUT_REFLECT | OUTPUT_REFLECT);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_update(&ctx, in, 1024);
	tls_crypto_crc_final(&ctx, &crcvalue);
	printf("CRYPTO_CRC_TYPE_32 INPUT_REFLECT | OUTPUT_REFLECT value:0x%08X\n", crcvalue);

OUT:
	if(in != NULL)
		tls_mem_free(in);
	
	return 0;
}

int md5_hard_demo(void)
{
	psDigestContext_t ctx;
	u8 md5hash[16];
	u8 *md5in1 = (u8 *)"abacdefghgjklmno1234";
	u8 *md5in2 = (u8 *)"1234567890123456abcd";
	u8 md5real[16]={0xD6, 0xEB, 0xD1, 0x1B, 0x0D, 0x42, 0x84, 0xE8, 
					0x52, 0x74, 0xA6, 0xA8, 0x2A, 0x3B, 0x2A, 0x12,};

	tls_crypto_init();	

	tls_crypto_md5_init(&ctx);
	tls_crypto_md5_update(&ctx, md5in1, 20);
	tls_crypto_md5_update(&ctx, md5in2, 20);
	tls_crypto_md5_final(&ctx, md5hash);

	if(memcmp(md5hash, md5real, 16))
	{
		printf("md5 test fail\n");
	}
	else
	{
		printf("md5 test success\n");
	}	

//	printf("md5 hash:");
//	for(int i=0; i<16; i++)
//	{
//		printf("0x%02X, ", md5hash[i]);
//	}
//	printf("\n");
	
	return 0;
}

int sha1_hard_demo(void)
{
	psDigestContext_t ctx;
	u8 sha1hash[20];
	u8 *sha1in1 = (u8 *)"abacdefghgjklmno1234";
	u8 *sha1in2 = (u8 *)"1234567890123456abcd";
	u8 sha1real[20]={0xA9, 0xED, 0x29, 0x56, 0x83, 0x6A, 0x59, 0x22, 0x27, 0x70, 
					0x87, 0x52, 0xDC, 0xB5, 0x1D, 0x2B, 0xBA, 0x93, 0x40, 0xA6,};	

	tls_crypto_init();	

	tls_crypto_sha1_init(&ctx);
	tls_crypto_sha1_update(&ctx, sha1in1, 20);
	tls_crypto_sha1_update(&ctx, sha1in2, 20);
	tls_crypto_sha1_final(&ctx, sha1hash);

	if(memcmp(sha1hash, sha1real, 16))
	{
		printf("sha1 test fail\n");
	}
	else
	{
		printf("sha1 test success\n");
	}	

//	printf("sha1 hash:");
//	for(int i=0; i<20; i++)
//	{
//		printf("0x%02X, ", sha1hash[i]);
//	}
//	printf("\n");	
	return 0;
}


int crypt_hard_demo(void)
{
	RNG_hard_demo();
	rc4_hard_demo();
	aes_hard_demo();
	des_hard_demo();
	des3_hard_demo();
	crc_hard_demo();
	md5_hard_demo();
	sha1_hard_demo();
	return 0;
}

#endif


