#include <stdio.h>
#include <string.h>
#include "wm_sasc.h"

extern void dumpBuffer(char *name, char* buffer, int len);

#define TEST_DEBUG_EN           1
#if TEST_DEBUG_EN
#define TEST_DEBUG(fmt, ...)    printf("%s: "fmt, __func__, ##__VA_ARGS__)
#else
#define TEST_DEBUG(fmt, ...)
#endif

static void switch_to_ree(void)
{
	csi_vic_disable_irq(SYS_TICK_IRQn);
	__asm__ volatile(
		"mtcr r14, cr<6, 3> \n"
		"mfcr     a3, psr\n"
		"bclri    a3, 30 \n" //Clear PSR.T
		"mtcr     a3, psr \n"
		"psrset   ee, ie \n" //Set EE & IE
		"mfcr     a3, psr\n"
		"bseti    a3, 9 \n" //Set MM
		"mtcr     a3, psr \n"
	);
	__asm__ volatile("mtcr %0, vbr" : : "r"(& (__Vectors)));
	TEST_DEBUG("switched to ree\n");
}

static SASC_Type* get_block_from_addr(uint32_t base_addr)
{
	if(base_addr >= 0x8000000 && base_addr < 0x10000000)
	{
		return SASC_FLASH;
	}
	else if(base_addr < 0x20028000 && base_addr >= 0x20000000)
	{
		return SASC_B1;
	}
	else if(base_addr < 0x20048000 && base_addr >= 0x20028000)
	{
		return SASC_B2;
	}
	return NULL;
}


static void wm_sasc_config_region(SASC_Type *block, uint32_t idx, uint32_t base_addr, sasc_region_size_e size,
		sasc_region_attr_t *attr, uint32_t enable)
{
	base_addr &= 0x03FFFFFC;
	block->REGION[idx] = size | (base_addr << 6);
	//printf("region %d: 0x%x\n", idx, size | (base_addr << 6));
	block->CAR = _R2_Pos(attr->car, idx) | (block->CAR & ~(_R2_Msk(idx)));
	switch(attr->car)
	{
		case SASC_UN_SE_USER:
			block->AP0 = _R2_Pos(attr->ap, idx) | (block->AP0 & ~(_R2_Msk(idx)));
			block->CD0 = _R2_Pos(attr->cd, idx) | (block->CD0 & ~(_R2_Msk(idx)));
			break;
		case SASC_UN_SE_SUPER:
			block->AP1 = _R2_Pos(attr->ap, idx) | (block->AP1 & ~(_R2_Msk(idx)));
			block->CD1 = _R2_Pos(attr->cd, idx) | (block->CD1 & ~(_R2_Msk(idx)));
			break;
		case SASC_SE_USER:
			block->AP2 = _R2_Pos(attr->ap, idx) | (block->AP2 & ~(_R2_Msk(idx)));
			block->CD2 = _R2_Pos(attr->cd, idx) | (block->CD2 & ~(_R2_Msk(idx)));
			break;
		default:
			break;
	}
	block->CR = _R1_Pos(enable, idx) | (block->CR & ~(_R1_Msk(idx)));
}

void wm_sasc_enable_region(SASC_Type *block, uint32_t idx)
{
	block->CR = _R1_Pos(1, idx) | (block->CR & ~(_R1_Msk(idx)));
}

void wm_sasc_disable_region(SASC_Type *block, uint32_t idx)
{
	block->CR = (block->CR & ~(_R1_Msk(idx)));
}

void set_region_protect(uint32_t base_addr, uint32_t idx, sasc_region_size_e size, sasc_region_attr_t *attr)
{
	SASC_Type* block = get_block_from_addr(base_addr);

	TEST_DEBUG("base_addr %x idx %d size 0x%x ap %d cd %d car %d\n", base_addr, idx, size, attr->ap, attr->cd, attr->car);
	if(base_addr >= 0x8000000 && base_addr < 0x10000000)
	{
		base_addr &= 0x1FFFFFF;
	}
	else if(base_addr < 0x20028000 && base_addr >= 0x20000000)
	{
		base_addr &= 0x3FFFF;
	}
	else if(base_addr < 0x20048000 && base_addr >= 0x20028000)
	{
		base_addr &= 0x7FFFF;
	}
	wm_sasc_config_region(block, idx, base_addr, size, attr, 1);
}

/**
  *******************************************************
  *               TEST CODE IS BELOW
  *******************************************************
  */

void access_region(uint32_t base_addr, sasc_region_size_e size, sasc_region_attr_t *attr, int pos)
{
	char dest[4] = {0xa5, 0xa5, 0xa5, 0xa5};
	uint32_t s = 1 << (size - 3);
	TEST_DEBUG("base_addr %x size 0x%x\n", base_addr, s);

	switch(pos)
	{
	case 0:
		memcpy(dest, (char*)(base_addr + s), sizeof(dest));
		TEST_DEBUG("Behind addr %x size 0x%x can read\n", base_addr + s, sizeof(dest));
		TEST_DEBUG("start to read addr %x size 0x%x\n", base_addr + s - sizeof(dest), sizeof(dest));
		memcpy(dest, (char*)(base_addr + s - sizeof(dest)), sizeof(dest));
		TEST_DEBUG("addr %x size 0x%x can read\n", base_addr + s - sizeof(dest), sizeof(dest));
		break;
	case 1:
		memcpy(dest, (char*)(base_addr - sizeof(dest)), sizeof(dest));
		TEST_DEBUG("Front addr %x size 0x%x can read\n", base_addr - sizeof(dest), sizeof(dest));
		TEST_DEBUG("start to read addr %x size 0x%x\n", base_addr, sizeof(dest));
		memcpy(dest, (char*)base_addr, sizeof(dest));
		TEST_DEBUG("base_addr %x size 0x%x can read\n", base_addr, sizeof(dest));
		break;
	case 2:
		memcpy((char*)(base_addr - sizeof(dest)), dest, sizeof(dest));
		TEST_DEBUG("Front addr %x size 0x%x can write\n", base_addr - sizeof(dest), sizeof(dest));
		TEST_DEBUG("start to write addr %x size 0x%x\n", base_addr, sizeof(dest));
		memcpy((char*)base_addr, dest, sizeof(dest));
		TEST_DEBUG("base_addr %x size 0x%x can write\n", base_addr, sizeof(dest));
		break;
	default:
		memcpy((char*)(base_addr + s), dest, sizeof(dest));
		TEST_DEBUG("Behind addr %x size 0x%x can write\n", base_addr + s, sizeof(dest));
		TEST_DEBUG("start to write addr %x size 0x%x\n", base_addr + s - sizeof(dest), sizeof(dest));
		memcpy((char*)(base_addr + s - sizeof(dest)), dest, sizeof(dest));
		TEST_DEBUG("addr %x size 0x%x can write\n", base_addr + s - sizeof(dest), sizeof(dest));
		break;
	}
	dumpBuffer("dest", dest, sizeof(dest));
}

int sasc_region_security_test(int num)
{
	sasc_region_attr_t attr;
	attr.ap = SASC_AP_DENYALL;
	attr.cd = SASC_CD_DA_OF;
	attr.car = SASC_SE_SUPER;
	set_region_protect(0x08010000, 0, SASC_REGION_SIZE_64KB, &attr);
	set_region_protect(0x20010000, 1, SASC_REGION_SIZE_16KB, &attr);
	set_region_protect(0x20030000, 2, SASC_REGION_SIZE_32KB, &attr);

//	SASC_B2->REGION[0] = SASC_REGION_SIZE_32KB | (0x8000 << 6);

	switch_to_ree();

	switch(num)
	{
	case 0:
		access_region(0x20010000, SASC_REGION_SIZE_16KB, &attr, 0);
		break;
	case 1:
		access_region(0x20010000, SASC_REGION_SIZE_16KB, &attr, 1);
		break;
	case 2:
		access_region(0x20010000, SASC_REGION_SIZE_16KB, &attr, 2);
		break;
	case 3:
		access_region(0x20010000, SASC_REGION_SIZE_16KB, &attr, 3);
		break;
	case 4:
		access_region(0x20030000, SASC_REGION_SIZE_32KB, &attr, 0);
		break;
	case 5:
		access_region(0x20030000, SASC_REGION_SIZE_32KB, &attr, 1);
		break;
	case 6:
		access_region(0x20030000, SASC_REGION_SIZE_32KB, &attr, 2);
		break;
	case 7:
		access_region(0x20030000, SASC_REGION_SIZE_32KB, &attr, 3);
		break;
	case 8:
		access_region(0x08010000, SASC_REGION_SIZE_64KB, &attr, 0);
		break;
	case 9:
		access_region(0x08010000, SASC_REGION_SIZE_64KB, &attr, 1);
		break;
	}
	return -1;
}

