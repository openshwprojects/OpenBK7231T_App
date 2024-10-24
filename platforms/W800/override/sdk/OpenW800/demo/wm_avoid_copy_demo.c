#include <string.h>
#include <assert.h>
#include "wm_include.h"
#include "wm_internal_flash.h"
#include "wm_crypto_hard.h"
#include "wm_demo.h"

#if DEMO_AVOID_COPY
extern void dumpBuffer(char *name, char* buffer, int len);
extern int config_flash_decrypt_param(uint32_t code_decrypt, uint32_t dbus_decrypt, uint32_t data_decrypt);

int avoid_copy_secret_firm(u32 addr)
{
	int ret = 0;
	char *buf = tls_mem_alloc(128);
	u8 uuid[18];
	if (buf == NULL)
	{
		return -1;
	}
	tls_fls_read_unique_id(uuid);
	dumpBuffer("uuid", (char *)uuid, 18);
	config_flash_decrypt_param(1, 1, 1);
	tls_fls_read(addr, (u8 *)buf, 32);
	dumpBuffer("decrypted data", buf, 32);
	if(memcmp(uuid, buf, 9) || memcmp(&uuid[9], &buf[16], 9))
	{
		ret = -1;
	}
	tls_mem_free(buf);
	return ret;
}

int avoid_copy_open_firm(u32 addr)
{
	const u8 aesKey[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66};
	psCipherContext_t ctx;
	int ret = 0;
	char *buf = tls_mem_alloc(128);
	u8 uuid[18];
	if (buf == NULL)
	{
		return -1;
	}
	tls_fls_read_unique_id(uuid);
	dumpBuffer("uuid", (char *)uuid, 18);
	tls_fls_read(addr, (u8 *)buf, 32);
	tls_crypto_aes_init(&ctx, NULL, aesKey, 16, CRYPTO_MODE_ECB);
	tls_crypto_aes_encrypt_decrypt(&ctx, (unsigned char *)buf, (unsigned char *)buf, 32, CRYPTO_WAY_DECRYPT);
	dumpBuffer("decrypted data", buf, 32);
	if(memcmp(uuid, buf, 9) || memcmp(&uuid[9], &buf[16], 9))
	{
		ret = -1;
	}
	tls_mem_free(buf);
	return ret;
}


int avoid_copy_entry(void)
{
	int type = 0;
	FLASH_ENCRYPT_CTRL_Type encrypt_ctrl;
	encrypt_ctrl.w = M32(HR_FLASH_ENCRYPT_CTRL);
	type = encrypt_ctrl.b.code_decrypt;
	if((type == 1 && avoid_copy_secret_firm(0x800F000)) || (type == 0 && avoid_copy_open_firm(0x800F000)))
	{
		printf("avoid copy %s firm start fail!\n", type == 1 ? "secret" : "open");
		assert(0);
	}
	if (type == 1) /*restore for secret*/
	M32(HR_FLASH_ENCRYPT_CTRL) = encrypt_ctrl.w;
	printf("avoid copy %s firm start success!\n", type == 1 ? "secret" : "open");
	return 0;
}

#endif

