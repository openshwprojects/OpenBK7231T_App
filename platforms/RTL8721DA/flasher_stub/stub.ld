ENTRY(Reset_Handler)

#include "ameba_layout.ld"
#include "project_km4/platform_autoconf.h"

MEMORY
{
    EXTENDED_FLOADER_RAM (rwx) : ORIGIN = 0x3000A020, LENGTH = 0x30040000 - 0x3000A020
}

/* define common load memory address */
#ifdef  GENERATE_FLOADER_LD
	#define OUTSECTION ram_image1
	#define IMG1_LMA_REGION 	EXTENDED_FLOADER_RAM
	#define KM4_LOADER_LMA	 	EXTENDED_FLOADER_RAM
#else
	#define KM4_LOADER_LMA  	KM4_BOOTLOADER_RAM_S
	#ifdef CONFIG_IMG1_FLASH
		#define OUTSECTION xip_image1
		#define IMG1_LMA_REGION KM4_BOOT_XIP
	#else
		#define OUTSECTION ram_image1
		#define IMG1_LMA_REGION KM4_BOOTLOADER_RAM_S
	#endif
#endif

/* define trustzone load memory address */
#ifdef CONFIG_IMG3_SRAM
	#define LMA_TZ_REGION KM4_BD_RAM_TZ_S
#else
	#define LMA_TZ_REGION KM4_BD_PSRAM_TZ_S
#endif


SECTIONS
{
	/* image1: boot image start */
	/* image1 entry, this section should in RAM and fixed address for ROM */
	.ram_image1.entry :
	{
		__ram_image1_text_start__ = .;
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.image1.entry.data*)))
		KEEP(*(.image1.validate.rodata*))
		*(.image1.export.symb)
	} > KM4_LOADER_LMA

	/*  Add . to assign the start address of the section */
	/*  to prevent the change of the start address by ld doing section alignment */
	.ram_image1.text :
	{
		/* image1 text */
		/* If there is any code that must be placed in RAM, please use .sramdram.only.text section*/
		*(.sramdram.only.text*)
		. = ALIGN (32);
	} > KM4_LOADER_LMA

	.OUTSECTION.text :
	{
		/* image1 text */
		*(.text*)
		*(.non.dram.text*)
		*(.rodata*)
		. = ALIGN (32);
	} > IMG1_LMA_REGION

	.ram_image1.data :
	{
		__ram_image1_data_start__ = .;
		*(.data*)
		. = ALIGN(32);
		__ram_image1_data_end__ = .;

		__ram_image1_text_end__ = .;
	} > KM4_LOADER_LMA

	.ram_image1.bss :
	{
		__image1_bss_start__ = .;
		*(.bss*)
		__image1_bss_end__ = .;
	} > KM4_LOADER_LMA

	/* image2: normal image start */
/*
	.ram_image2.entry :
	{
		__ram_image2_text_start__ = .;
		__image2_entry_func__ = .;
		KEEP(*(SORT(.image2.entry.data*)))
	} > KM4_IMG2_ENTRY
*/
	/* TFM text*/
/*
	.ram_image3.text :
	{
		__ram_image3_start__ = .;
		__ram_image3_end__ = __ram_image3_start__ + LENGTH(LMA_TZ_REGION);
	} > LMA_TZ_REGION
*/
}

__rom_stdlib_text_start__ = ORIGIN(KM4_IROM_STDLIB_NS);
