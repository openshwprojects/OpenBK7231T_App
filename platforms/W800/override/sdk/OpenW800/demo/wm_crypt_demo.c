/***************************************************************************** 
* 
* File Name : wm_crypt_demo.c 
* 
* Description: crypt demo function 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : wangmin
* 
* Date : 2014-6-2 
*****************************************************************************/ 
#include <string.h>
#include "wm_include.h"
#include "wm_crypto.h"
 
#if DEMO_ENCRYPT
int crypt_demo(char *buf)
{
	
	u8 iv[16];
	u8 dat[24];
	u8 key[16]="abcdefghijklmnop";
	u8 plain[16]="0123456789ABCDEF";
	int i;
	int len = 16;
	
	for(i=0;i<16;i++) iv[i]=i;
	
	memset(dat, 0, 16);
	MEMCPY(dat, plain, sizeof(plain));
	
/******start AES 128 En/Decrypt********/
	printf("---start AES128:\n");
	if (aes_128_cbc_encrypt(key, iv, dat, sizeof(plain))){
		printf("Encrypted Failed\n");
		return WM_FAILED;
	}
	else{
		printf("Encrypted OK:\n");
		for(i=0; i<len; i++) printf("%x", dat[i]);
		printf("\n");
	}
	
	if(aes_128_cbc_decrypt(key, iv, dat, sizeof(plain))){
		printf("Decrypt failed\n");
		return WM_FAILED;
	}else{
		printf("Decrypted OK:\n");
 		for(i=0; i<len; i++) printf("%x", dat[i]);
		printf("\n");
	}
	
/******start RC4********/
	printf("\n----start RC4:\n");
	rc4(key, sizeof(key), plain, sizeof(plain));
	printf("Encrypted OK:\n");
	for(i=0; i<sizeof(plain); i++) printf("%x", plain[i]);
	printf("\n");
	
	rc4(key, sizeof(key),plain, sizeof(plain));
	printf("Decrypted OK:\n");
	for(i=0; i<sizeof(plain); i++) printf("%x", plain[i]);
	printf("\n");


/******start HMAC MD5********/
	printf("\n---start HMAC MD5:\n");
	memset(dat, 0, sizeof(dat));
	if(hmac_md5(key, sizeof(key), plain, sizeof(plain), dat)){
		printf("Caculated failed\n");
		return WM_FAILED;
	}else{
		printf("HMAC MD5: \n");
		for(i=0; i<len; i++) printf("%x", dat[i]);
		printf("\n");
	}

/******start  MD5********/
	printf("\n----start MD5:\n");
	memset(dat, 0, sizeof(dat));
	md5(plain, sizeof(plain),dat);
	printf("MD5: \n");
	for(i=0; i<len; i++) printf("%x", dat[i]);
	printf("\n");

/******start  SHA1********/
	printf("\n---start SHA1:\n");
	len = 20;
	memset(dat, 0, sizeof(dat));
	sha1(plain, sizeof(plain),dat);
	printf("SHA1: \n");
	for(i=0; i<len; i++) printf("%x", dat[i]);
	printf("\n");
	return WM_SUCCESS;;
}
#endif

