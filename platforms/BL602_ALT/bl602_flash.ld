/****************************************************************************************
* @file map.txt
*
* @brief This file is the map file (gnuarm or armgcc).
*
* Copyright (C) BouffaloLab 2018
*
****************************************************************************************
*/

/* configure the CPU type */
OUTPUT_ARCH( "riscv" )

/* configure the entry point */
ENTRY(__start)

StackSize    = 0x0800; /*  2KB */
HeapMinSize  = 0x1000; /*  4KB */

__EM_SIZE = DEFINED(ble_controller_init) ? 8K : 0K;
__RFTLV_SIZE_OFFSET = 1K;
__RFTLV_SIZE_HOLE = 2K;
__RFTLV_HEAD1_H = (0x46524C42); /* BLRF */
__RFTLV_HEAD1_L = (0x41524150); /* PAPA */

MEMORY
{
    fw_header_memory  (rx)  : ORIGIN = 0x23000000 - 0x1000, LENGTH = 4K
    xip_memory  (rx)  : ORIGIN = 0x23000000, LENGTH = 1024K
    itcm_memory (rx)  : ORIGIN = 0x22010000, LENGTH = 16K
    dtcm_memory (rx)  : ORIGIN = 0x4200C000, LENGTH = 48K
    ram_memory  (!rx) : ORIGIN = 0x42018000, LENGTH = 176K + 32K
    ram_wifi    (wxa) : ORIGIN = 0x42042000 - __EM_SIZE, LENGTH = (8K + 104K - 64K - 8K) /*leave 8K left for BLE*/
}

SECTIONS
{
    PROVIDE(__metal_chicken_bit = 0);

    .fw_header :
    {
        KEEP(*(.fw_header))
    } > fw_header_memory

    .init :
    {
        KEEP (*(SORT_NONE(.init)))
        KEEP (*(SORT_NONE(.vector)))

	. = ORIGIN(xip_memory) + 512;
        __multi_bins__ = ABSOLUTE(.);
        LONG(0x464C4553) LONG(0x00000000) /*SELF*/      LONG(4K)         LONG(4K)
        LONG(0x52415744) LONG(0x49464346) /*DWARFCFI*/  LONG(0x00000000) LONG(0x00000000)
        LONG(0x464D4F52) LONG(0x00000053) /*ROMFS*/     LONG(0x00000000) LONG(0x00000000)
        LONG(0x54524543) LONG(0x00000053) /*CERTS*/     LONG(0x00000000) LONG(0x00000000)
        LONG(0xFFFFFFFF) LONG(0xFFFFFFFF)               LONG(0xFFFFFFFF) LONG(0xFFFFFFFF)

    } > xip_memory

    /* value for rftlv */

    .rftlv.tool :
    {
        . = ORIGIN(xip_memory) + __RFTLV_SIZE_OFFSET;
        PROVIDE( _ld_symbol_rftlv_address = . );
        LONG(__RFTLV_HEAD1_H);
        LONG(__RFTLV_HEAD1_L);
        . = ORIGIN(xip_memory) + __RFTLV_SIZE_OFFSET + __RFTLV_SIZE_HOLE;
    } > xip_memory

    .text :
    {
        . = ALIGN(4);
        __text_code_start__ = .;

        *(.text)
        *(.text.*)

        /* section information for components's version */
        . = ALIGN(4);
        _version_info_section_start = .;
        KEEP(*(.version.*))
        _version_info_section_end = .;

        /* section information for shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;

        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;

        /* static fw attribute entry */
        . = ALIGN(4);
        _bl_static_fw_cfg_entry_start = .;
        KEEP(*(.wifi.cfg.entry))
        _bl_static_fw_cfg_entry_end = .;

        /*put .rodata**/
        *(EXCLUDE_FILE( *bl602_glb*.o* \
                        *bl602_pds*.o* \
                        *bl602_common*.o* \
                        *bl602_sf_cfg*.o* \
                        *bl602_sf_cfg_ext*.o* \
                        *bl602_sf_ctrl*.o* \
                        *bl602_sflash*.o* \
                        *bl602_sflash_ext*.o* \
                        *bl602_xip_sflash*.o* \
                        *bl602_xip_sflash_ext*.o* \
                        *bl602_ef_ctrl*.o*) .rodata*)
        *(.rodata)
        *(.rodata.*)

        *(.srodata)
        *(.srodata.*)

        KEEP(*(.bleromro))

        /* _dump_sections for bflb_coredump.c*/
        . = ALIGN(4);
        PROVIDE( _dump_sections = ABSOLUTE(.));
        LONG(ORIGIN(itcm_memory));
        LONG(LENGTH(itcm_memory));
        LONG(ORIGIN(dtcm_memory));
        LONG(LENGTH(dtcm_memory));
        LONG(ORIGIN(ram_memory));
        LONG(LENGTH(ram_memory));
        LONG(0xffffffff);
        LONG(0xffffffff);
        /* _dump_sections for bl_coredump.c*/

        . = ALIGN(4);
        __text_code_end__ = .;

    } > xip_memory

    .sysinit.array :
    {
        . = ALIGN(4);
        __sysinit_array_start = .;
        KEEP(*(SORT_BY_NAME(.sysinit.*)))
        __sysinit_array_stop = .;
    } > xip_memory

    .build_id :
    {
        . = ALIGN(4);
        elf_build_id = ABSOLUTE(.) + 16;
        *(.note.gnu.build-id)
    } > xip_memory

    . = ALIGN(4);
    __itcm_load_addr = .;

    .itcm_region : AT (__itcm_load_addr)
    {
        . = ALIGN(4);
        __tcm_code_start__ = .;

        *(.tcm_code.*)
        *(.tcm_const.*)
        *(.sclock_rlt_code.*)
        *(.sclock_rlt_const.*)

        *bl602_glb*.o*(.rodata*)
        *bl602_pds*.o*(.rodata*)
        *bl602_common*.o*(.rodata*)
        *bl602_sf_cfg*.o*(.rodata*)
        *bl602_sf_cfg_ext*.o*(.rodata*)
        *bl602_sf_ctrl*.o*(.rodata*)
        *bl602_sflash*.o*(.rodata*)
        *bl602_sflash_ext*.o*(.rodata*)
        *bl602_xip_sflash*.o*(.rodata*)
        *bl602_xip_sflash_ext*.o*(.rodata*)
        *bl602_ef_ctrl*.o*(.rodata*)

        . = ALIGN(4);
        __tcm_code_end__ = .;
    } > itcm_memory

    __dtcm_load_addr = __itcm_load_addr + SIZEOF(.itcm_region);

    .dtcm_region : AT (__dtcm_load_addr)
    {
        . = ALIGN(4);
        __tcm_data_start__ = .;

        *(.tcm_data)

        . = ALIGN(4);
        __tcm_data_end__ = .;
    } > dtcm_memory

    /*************************************************************************/
    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (NOLOAD):
    {
        . = ALIGN(0x4);
        . = . + StackSize;
        . = ALIGN(0x4);
    } > dtcm_memory

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(dtcm_memory) + LENGTH(dtcm_memory);
    PROVIDE( __freertos_irq_stack_top = __StackTop);
    PROVIDE( _sp_main = __StackTop);
    PROVIDE( _sp_base = __StackTop - StackSize);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __tcm_data_end__, "region RAM overflowed with stack")
    /*************************************************************************/

    __ram_load_addr = __dtcm_load_addr + SIZEOF(.dtcm_region);

    /* Data section */
    RAM_DATA : AT (__ram_load_addr)
    {
        . = ALIGN(4);
        __ram_data_start__ = .;

        PROVIDE( __global_pointer$ = . + 0x800 );

        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        *(.sdata2)
        *(.sdata2.*)

        *(.nocache_ram)
        . = ALIGN(8);
        *(._k_queue.static.*)
        *(._k_sem.static.*)
        *(._k_mutex.static.*)
        _bt_gatt_service_static_list_start = .;
        KEEP(*(SORT_BY_NAME("._bt_gatt_service_static.static.*")))
        _bt_gatt_service_static_list_end = .;
        _bt_l2cap_fixed_chan_list_start = .;
        KEEP(*(SORT_BY_NAME("._bt_l2cap_fixed_chan.static.*")))
        _bt_l2cap_fixed_chan_list_end = .;

        . = ALIGN(4);
        __bflb_log_tags_start__ = .;
        *(.bflb_log_tags_array)
        . = ALIGN(4);
        __bflb_log_tags_end__ = .;
        __ram_data_end__ = .;
    } > ram_memory

    /*put wifibss in the first place*/
    .wifibss         (NOLOAD) :
    {
      PROVIDE( __wifi_bss_start = ADDR(.wifibss) );
      PROVIDE( __wifi_bss_end = ADDR(.wifibss) + SIZEOF(.wifibss) );
      _sshram = . ;
      *(SHAREDRAMIPC)
      *(SHAREDRAM)
      _eshram = . ;
      *ipc_shared.o(COMMON)
      *sdu_shared.o(COMMON)
      *hal_desc.o(COMMON)
      *txl_buffer_shared.o(COMMON)
      *txl_frame_shared.o(COMMON)
      *scan_shared.o(COMMON)
      *scanu_shared.o(COMMON)
      *mfp_bip.o(COMMON)
      *me_mic.o(COMMON)
      *bl_sta_mgmt_others.o(COMMON)
      *bl_pmk_mgmt.o(COMMON)
      *bl_pmk_mgmt_internal.o(COMMON)
      *libwifi_drv.a:bl_utils.o(COMMON)
      *libwifi_drv.a:bl_utils.o(.bss*)
      *(.wifi_ram*)
      . = ALIGN(16);
    } > ram_wifi

    PROVIDE( _heap_wifi_start = . );
    PROVIDE( _heap_wifi_size = ORIGIN(ram_wifi) + LENGTH(ram_wifi) - _heap_wifi_start );

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;

        *(.bss*)
        *(.sbss*)
        *(COMMON)

        . = ALIGN(4);
        __bss_end__ = .;
    } > ram_memory

    .noinit_data (NOLOAD) :
    {
        . = ALIGN(4);
        __noinit_data_start__ = .;

        *(.noinit_data*)
        *(.nocache_noinit_ram)

        . = ALIGN(4);
        __noinit_data_end__ = .;
    } > ram_memory

    .heap (NOLOAD):
    {
        . = ALIGN(4);
        __HeapBase = .;

        /*__end__ = .;*/
        /*end = __end__;*/
        KEEP(*(.heap*))

        . = ALIGN(4);
        __HeapLimit = .;
    } > ram_memory

    __HeapLimit = ORIGIN(ram_memory) + LENGTH(ram_memory);
    ASSERT(__HeapLimit - __HeapBase >= HeapMinSize, "heap region overflow")

    /*CFG FW used in code*/
    PROVIDE( _ld_bl_static_cfg_entry_start = _bl_static_fw_cfg_entry_start );
    PROVIDE( _ld_bl_static_cfg_entry_end   = _bl_static_fw_cfg_entry_end );

    /* EM size */
    PROVIDE( __LD_CONFIG_EM_SEL = __EM_SIZE );
}

