/**
 * @file    wm_io.c
 *
 * @brief   IO Driver Module
 *
 * @author  lilm
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */

#include "wm_regs.h"
#include "wm_osal.h"
#include "wm_io.h"
#include "wm_dbg.h"
#include "tls_common.h"

static void io_cfg_option1(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

    tls_reg_write32(HR_GPIO_AF_SEL + offset, tls_reg_read32(HR_GPIO_AF_SEL + offset) | BIT(pin));  /* gpio function */
    tls_reg_write32(HR_GPIO_AF_S1  + offset, tls_reg_read32(HR_GPIO_AF_S1  + offset) & (~BIT(pin)));
    tls_reg_write32(HR_GPIO_AF_S0  + offset, tls_reg_read32(HR_GPIO_AF_S0  + offset) & (~BIT(pin)));
}

static void io_cfg_option2(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

    tls_reg_write32(HR_GPIO_AF_SEL + offset, tls_reg_read32(HR_GPIO_AF_SEL + offset) | BIT(pin));  /* gpio function */
    tls_reg_write32(HR_GPIO_AF_S1  + offset, tls_reg_read32(HR_GPIO_AF_S1  + offset) & (~BIT(pin)));
    tls_reg_write32(HR_GPIO_AF_S0  + offset, tls_reg_read32(HR_GPIO_AF_S0  + offset) | BIT(pin));
}

static void io_cfg_option3(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

    tls_reg_write32(HR_GPIO_AF_SEL + offset, tls_reg_read32(HR_GPIO_AF_SEL + offset) | BIT(pin));  /* gpio function */
    tls_reg_write32(HR_GPIO_AF_S1  + offset, tls_reg_read32(HR_GPIO_AF_S1  + offset) | BIT(pin));
    tls_reg_write32(HR_GPIO_AF_S0  + offset, tls_reg_read32(HR_GPIO_AF_S0  + offset) & (~BIT(pin)));
}

static void io_cfg_option4(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

    tls_reg_write32(HR_GPIO_AF_SEL + offset, tls_reg_read32(HR_GPIO_AF_SEL + offset) | BIT(pin));  /* gpio function */
    tls_reg_write32(HR_GPIO_AF_S1  + offset, tls_reg_read32(HR_GPIO_AF_S1  + offset) | BIT(pin));
    tls_reg_write32(HR_GPIO_AF_S0  + offset, tls_reg_read32(HR_GPIO_AF_S0  + offset) | BIT(pin));
}

static void io_cfg_option5(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

    tls_reg_write32(HR_GPIO_AF_SEL + offset, tls_reg_read32(HR_GPIO_AF_SEL + offset) & (~BIT(pin)));  /* disable gpio function */
}

static u32 io_pa_option67 = 0;
static u32 io_pb_option67 = 0;
static void io_cfg_option6(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
		io_pb_option67  |= BIT(pin);
    }
    else
    {
        pin    = name;
        offset = 0;
		io_pa_option67  |= BIT(pin);
    }

    tls_reg_write32(HR_GPIO_AF_SEL  + offset, tls_reg_read32(HR_GPIO_AF_SEL  + offset) & (~BIT(pin)));  /* disable gpio function */
    tls_reg_write32(HR_GPIO_DIR     + offset, tls_reg_read32(HR_GPIO_DIR     + offset) & (~BIT(pin)));
	tls_reg_write32(HR_GPIO_PULLUP_EN + offset, tls_reg_read32(HR_GPIO_PULLUP_EN + offset) | (BIT(pin)));
	tls_reg_write32(HR_GPIO_PULLDOWN_EN + offset, tls_reg_read32(HR_GPIO_PULLDOWN_EN + offset) & (~BIT(pin)));	
}

static void io_cfg_option7(enum tls_io_name name)
{
    u8  pin;
    u16 offset;

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
		io_pb_option67  &= BIT(pin);		
    }
    else
    {
        pin    = name;
        offset = 0;
		io_pa_option67  &= BIT(pin);
    }

    tls_reg_write32(HR_GPIO_AF_SEL  + offset, tls_reg_read32(HR_GPIO_AF_SEL  + offset) & (~BIT(pin)));  /* enable gpio function */
    tls_reg_write32(HR_GPIO_DIR     + offset, tls_reg_read32(HR_GPIO_DIR     + offset) & (~BIT(pin)));  /* set input */
	tls_reg_write32(HR_GPIO_PULLUP_EN + offset, tls_reg_read32(HR_GPIO_PULLUP_EN + offset) | (BIT(pin))); /* disable pull up */
	tls_reg_write32(HR_GPIO_PULLDOWN_EN + offset, tls_reg_read32(HR_GPIO_PULLDOWN_EN + offset) & (~BIT(pin)));	/*disable pull down*/		
}


/**
 * @brief          This function is used to config io function
 *
 * @param[in]      name      io name
 * @param[in]      option    io function option, value is WM_IO_OPT*_*, also is WM_IO_OPTION1-6
 *
 * @return         None
 *
 * @note           None
 */
void tls_io_cfg_set(enum tls_io_name name, u8 option)
{
    if (WM_IO_OPTION1 == option)
        io_cfg_option1(name);
    else if (WM_IO_OPTION2 == option)
        io_cfg_option2(name);
    else if (WM_IO_OPTION3 == option)
        io_cfg_option3(name);
    else if (WM_IO_OPTION4 == option)
        io_cfg_option4(name);
    else if (WM_IO_OPTION5 == option)
        io_cfg_option5(name);
    else if (WM_IO_OPTION6 == option)
        io_cfg_option6(name);
	else if (WM_IO_OPTION7 == option)
		io_cfg_option7(name);
    else
        TLS_DBGPRT_IO_ERR("invalid io option.\r\n");
}

/**
 * @brief          	This function is used to get io function config 
 *
 * @param[in]      	name      io name
 *
 * @retval         	WM_IO_OPTION1~6  Mapping io function
 *
 * @note           	None
 */
int tls_io_cfg_get(enum tls_io_name name)
{
    u8  pin;
    u16 offset;
	u32 afsel,afs1,afs0,dir,pullupen, pulldownen;
	

    if (name >= WM_IO_PB_00)
    {
        pin    = name - WM_IO_PB_00;
        offset = TLS_IO_AB_OFFSET;
    }
    else
    {
        pin    = name;
        offset = 0;
    }

	afsel = tls_reg_read32(HR_GPIO_AF_SEL + offset);
	afs1 = tls_reg_read32(HR_GPIO_AF_S1 + offset);
	afs0 = tls_reg_read32(HR_GPIO_AF_S0 + offset);
	dir = tls_reg_read32(HR_GPIO_DIR + offset);
	pullupen = tls_reg_read32(HR_GPIO_PULLUP_EN + offset);
	pulldownen = tls_reg_read32(HR_GPIO_PULLDOWN_EN + offset);

	if(afsel&BIT(pin))
	{
		if((0==(afs1&BIT(pin))) && (0==(afs0&BIT(pin))))
			return WM_IO_OPTION1;
		else if((0==(afs1&BIT(pin))) && (afs0&BIT(pin)))
			return WM_IO_OPTION2;
		else if((afs1&BIT(pin)) && (0==(afs0&BIT(pin))))
			return WM_IO_OPTION3;
		else if((afs1&BIT(pin)) && (afs0&BIT(pin)))
			return WM_IO_OPTION4;
	}
	else
	{
		if((!(dir&BIT(pin))) && (pullupen&BIT(pin)) && (!(pulldownen&BIT(pin))))
		{
			if (offset)
			{
				if (io_pb_option67 & BIT(pin))
				{
					return WM_IO_OPTION6;
				}
				else
				{
					return WM_IO_OPTION7;
				}
			}
			else
			{
				if (io_pa_option67 & BIT(pin))
				{
					return WM_IO_OPTION6;
				}
				else
				{
					return WM_IO_OPTION7;
				}
			}
		}
		else
			return WM_IO_OPTION5;
	}
	
	return 0;
}


