/**
 * @file    wm_flash_demo.c
 *
 * @brief   flash demo function
 *
 * @author  dave
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include <string.h>
#include "wm_include.h"
#include "wm_internal_flash.h"
#include "wm_demo.h"

#if DEMO_FLASH

#define TEST_FLASH_BUF_SIZE    4000

int flash_demo(void)
{
    u8 *write_buf = NULL;
    u8 *read_buf = NULL;
    u16 i;

    tls_fls_init();									//initialize flash driver

    write_buf = tls_mem_alloc(TEST_FLASH_BUF_SIZE);
    if (NULL == write_buf)
    {
        printf("\nmalloc write buf error\n");
        return WM_FAILED;
    }

    for (i = 0; i < TEST_FLASH_BUF_SIZE; i ++)
    {
        write_buf[i] = i + 1;
    }

    tls_fls_write(0x1F0303, write_buf, 1247);			/**verifying cross sector writing*/
    tls_fls_write(0x1F0303 + 1247, write_buf + 1247, 2571);
    tls_fls_write(0x1F0303 + 1247 + 2571, write_buf + 1247 + 2571, 182);

    read_buf = tls_mem_alloc(TEST_FLASH_BUF_SIZE);
    if (NULL == read_buf)
    {
        printf("\nmalloc read buf error\n");
        tls_mem_free(write_buf);
        return WM_FAILED;
    }
    memset(read_buf, 0, TEST_FLASH_BUF_SIZE);
    tls_fls_read(0x1F0303, read_buf, TEST_FLASH_BUF_SIZE);

    if (0 == memcmp(write_buf, read_buf, TEST_FLASH_BUF_SIZE))
    {
        printf("\nsuccess\n");
    }
    else
    {
        printf("\nfail\n");
    }

    tls_mem_free(write_buf);
    tls_mem_free(read_buf);

    return WM_SUCCESS;
}

#endif
