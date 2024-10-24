#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "wm_config.h"
#include "wm_type_def.h"
#include "wm_regs.h"
#include "wm_include.h"
#include "utils.h"
#include "wm_watchdog.h"
#include "wm_internal_flash.h"
#include "litepoint.h"
#include "wm_ram_config.h"
#include "wm_adc.h"
#include "wm_gpio_afsel.h"

#define FACTORY_ATCMD_MAX_ARG      10
#define FACTORY_ATCMD_NAME_MAX_LEN 10

#define FACTORY_ATCMD_OP_NULL       0
#define FACTORY_ATCMD_OP_EQ         1    /* = */
#define FACTORY_ATCMD_OP_EP         2    /* =! , update flash*/
#define FACTORY_ATCMD_OP_QU         3    /* =? */

/* error code */
#define FACTORY_ATCMD_ERR_OK              0
#define FACTORY_ATCMD_ERR_INV_FMT         1
#define FACTORY_ATCMD_ERR_UNSUPP          2
#define FACTORY_ATCMD_ERR_OPS             3
#define FACTORY_ATCMD_ERR_INV_PARAMS      4
#define FACTORY_ATCMD_ERR_NOT_ALLOW       5
#define FACTORY_ATCMD_ERR_MEM             6
#define FACTORY_ATCMD_ERR_FLASH           7
#define FACTORY_ATCMD_ERR_BUSY            8
#define FACTORY_ATCMD_ERR_SLEEP           9
#define FACTORY_ATCMD_ERR_JOIN            10
#define FACTORY_ATCMD_ERR_NO_SKT          11
#define FACTORY_ATCMD_ERR_INV_SKT         12
#define FACTORY_ATCMD_ERR_SKT_CONN        13
#define FACTORY_ATCMD_ERR_UNDEFINE        64
#define FACTORY_ATCMD_ERR_SCANNING		14

typedef unsigned int u32;
typedef unsigned char u8;

struct factory_atcmd_token_t
{
    char   name[FACTORY_ATCMD_NAME_MAX_LEN];
    u32   op;
    char  *arg[FACTORY_ATCMD_MAX_ARG];
    u32   arg_found;
};

typedef int (* atcmd_proc)(struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len);

struct factory_atcmd_t
{
    char   *name;
    int (* proc_func)(struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len);
};
extern int tls_tx_gainindex_map2_gainvalue(u8 *dst_txgain, u8 *srcgain_index);
extern int tls_tx_gainvalue_map2_gainindex(u8 *dst_txgain_index, u8 *srcgain);
extern u8* ieee80211_get_tx_gain(void);
extern void tls_rf_freqerr_adjust(int freqerr);
extern u32 rf_spi_read(u32 reg);
extern u32 rf_spi_write(u32 reg);

extern const char FirmWareVer[];
extern const char HwVer[];


/*ATCMD resource*/
#define FACTORY_ATCMD_BUF_SIZE     (256)
#define FACTORY_ATCMD_RESPONSE_SIZE (512)
static char *factory_cmdrsp_buf = NULL;
#define FACTORY_ATCMD_STACK_SIZE  (2048)
static u32 *factory_atcmd_stack = NULL;

static int hexstr_to_uinit(char *buf, u32 *d)
{
    int i;
    int len = strlen(buf);
    int c;
    *d = 0;

    if (len > 8)
        return -1;
    for (i = 0; i < len; i++)
    {
        c = hex_to_digit(buf[i]);
        if (c < 0)
            return -1;
        *d = (u8)c | (*d << 4);
    }
    return 0;
}

static int factory_atcmd_ok_resp(char *buf)
{
    int len;
    len = sprintf(buf, "+OK");
    return len;
}

static int factory_atcmd_err_resp(char *buf, int err_code)
{
    int len;
    len = sprintf(buf, "+ERR=%d", -err_code);
    return len;
}


static int factory_atcmd_mac_proc(
    struct factory_atcmd_token_t *tok,
    char *res_resp, u32 *res_len)
{
	u8 mac[6];
	int ret = 0;
    if (!tok->arg_found &&
            ((tok->op == FACTORY_ATCMD_OP_NULL) || (tok->op == FACTORY_ATCMD_OP_QU)))
    {
    	ret = tls_get_mac_addr(&mac[0]);
		if (ret == 0)
		{
        	*res_len = sprintf(res_resp, "+OK=%02x%02x%02x%02x%02x%02x",
            	               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}
		else
		{
        	*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_FLASH);
		}
    }
    else
    {
    	if (tok->arg_found && ((tok->op == FACTORY_ATCMD_OP_EQ) || (tok->op == FACTORY_ATCMD_OP_EP)))
    	{
    		ret = strtohexarray(mac, ETH_ALEN, (char *)tok->arg[0]);
			if (ret < 0)
			{
				*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
				return 0;
			}
		
    		ret = tls_set_mac_addr(mac);
			if (ret == 0)
			{
				*res_len = factory_atcmd_ok_resp(res_resp);
			}
			else
			{
				*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_FLASH);
			}			
    	}
		else
		{
        	*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
		}
    }
    return 0;
}

static int factory_atcmd_reset_proc(
    struct factory_atcmd_token_t *tok,
    char *res_resp, u32 *res_len)
{
	tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
	tls_sys_reset();
	return 0;
}


/******************************************************************
* Description:	Read register or memory

* Format:		AT+&REGR=<address>,[num]<CR>
			+OK=<value1>,[value2]...<CR><LF><CR><LF>

* Argument:	address: num:

* Author: 	kevin 2014-03-19
******************************************************************/
static int factory_atcmd_regr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret;
    u32 Addr, Num, Value;
    u8 buff[16];

    if(2 != tok->arg_found)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[1], &Num);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }
    Value = tls_reg_read32(Addr);
    *res_len = sprintf(res_resp, "+OK=%08x", Value);
    memset(buff, 0, sizeof(buff));
    while(--Num)
    {
        Addr += 4;
        Value = tls_reg_read32(Addr);
        *res_len += sprintf((char *)buff, ",%08x", Value);
        strcat(res_resp, (char *)buff);
    }
    return 0;
}

/******************************************************************
* Description:	Write register or memory

* Format:		AT+&REGW=<address>,<value1>,[value2]...<CR>
			+OK=<CR><LF><CR><LF>

* Argument:	address: value:

* Author: 	kevin 2014-03-19
******************************************************************/
static int factory_atcmd_regw_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret;
    u32 Addr, Value, i;

    if((tok->arg_found < 2) || (tok->arg_found > 9))
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }

    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }

    for(i = 0; i < tok->arg_found - 1; i++)
    {
        ret = hexstr_to_uinit(tok->arg[i + 1], &Value);
        if(ret)
        {
            *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
            return 0;
        }
        else
        {
            tls_reg_write32(Addr, Value);
        }
        Addr += 4;
    }
    *res_len = factory_atcmd_ok_resp(res_resp);
    return 0;
}

/******************************************************************
* Description:	Read RF register

* Format:		AT+&RFR=<address>,[num]<CR>
			+OK=<value1>,[value2]...<CR><LF><CR><LF>

* Argument:	address: size:

* Author: 	kevin 2014-03-19
******************************************************************/
static int factory_atcmd_rfr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Num;
    u8 buff[16];
    u16 databuf[8], *pdatabuf;

    if(2 != tok->arg_found)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[1], &Num);
    if(ret || (Num < 1) || (Num > 8) || (Addr + Num) > 25)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }

    for(i = 0; i < Num; i++)
    {
        databuf[i] = (u16)rf_spi_read(Addr);
        Addr += 1;
    }
    *res_len = sprintf(res_resp, "+OK=%04x", databuf[0]);
    pdatabuf = &databuf[1];
    while(--Num)
    {
        *res_len += sprintf((char *)buff, ",%04x", *pdatabuf++);
        strcat(res_resp, (char *)buff);
    }
    return 0;
}

/******************************************************************
* Description:	Write RF registers

* Format:		AT+&RFW=<address>,<value1>,[value2]...<CR>
			+OK<CR><LF><CR><LF>

* Argument:	address: value:

* Author: 	kevin 2014-03-19
******************************************************************/
static int factory_atcmd_rfw_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Num, Value;
    u16 databuf[8];

    if((tok->arg_found < 2) || (tok->arg_found > 9))
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }

    Num = 0;
    for(i = 0; i < tok->arg_found - 1; i++)
    {
        ret = hexstr_to_uinit(tok->arg[i + 1], &Value);
        if(ret)
        {
            *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
            return 0;
        }
        else
        {
            databuf[Num++] = Value;
        }
    }
    if((Num < 1) || (Num > 8) || (Addr + Num) > 25)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }
    Addr = Addr * 2;
    for(i = 0; i < Num; i++)
    {
        rf_spi_write((Addr << 16) | databuf[i]);
        Addr += 2;
    }
    *res_len = factory_atcmd_ok_resp(res_resp);
    return 0;
}


static int factory_atcmd_flsr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Num;
    u32 Value;

    if(2 != tok->arg_found)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[1], &Num);
    if(ret || (Num < 1) || (Num > 4))
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }

	*res_len = sprintf(res_resp, "+OK=");
    for(i = 0; i < Num - 1; i++)
    {
		tls_fls_read(Addr, (u8 *)&Value,4);
		*res_len += sprintf(res_resp + *res_len, "%04x,", Value);
        Addr += 4;
    }
	tls_fls_read(Addr, (u8 *)&Value,4);
	*res_len += sprintf(res_resp + *res_len, "%04x", Value);

    return 0;
}


static int factory_atcmd_flsw_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    int ret, i;
    u32 Addr, Value;

    if((tok->arg_found < 2) || (tok->arg_found > 9))
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
        return 0;
    }
    ret = hexstr_to_uinit(tok->arg[0], &Addr);
    if(ret)
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
        return 0;
    }

    for(i = 0; i < tok->arg_found - 1; i++)
    {
        ret = hexstr_to_uinit(tok->arg[i + 1], &Value);
        if(ret)
        {
            *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
            return 0;
        }
        else
        {
			tls_fls_write(Addr, (u8 *)&Value, 4);
	        Addr += 4;
        }
    }

    *res_len = factory_atcmd_ok_resp(res_resp);
    return 0;
}

static int factory_atcmd_txg_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    u8* tx_gain = ieee80211_get_tx_gain();
	int i = 0;
	int len = 0;
	int ret = 0;
    if(tok->arg_found)
	{
	    if (strtohexarray(tx_gain, TX_GAIN_LEN, tok->arg[0]) < 0)
    	{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
    		return 0;
    	}

		ret = tls_set_tx_gain(tx_gain);
		if (ret == 0)
		{
			*res_len = factory_atcmd_ok_resp(res_resp);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
		}
    }
	else{
		*res_len = 0;
		len = 0;
		len = sprintf(res_resp, "+OK=");
		for (i = 0; i < TX_GAIN_LEN; i++)
		{
			len += sprintf(res_resp + len, "%02x", tx_gain[i]);
		}
		len += sprintf(res_resp + len, "\r\n");
		*res_len = len;
    }
    return 0;
}

static int factory_atcmd_txgi_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    u8 tx_gain[TX_GAIN_LEN];
	u8* param_tx_gain = ieee80211_get_tx_gain();
	int i = 0;
	int len = 0;
	u8 tx_gain_index[TX_GAIN_LEN/3];
	int ret = 0;

	if(tok->arg_found)
	{
	    if (strtohexarray(tx_gain_index, TX_GAIN_LEN/3, tok->arg[0]) < 0)
	    {
    		*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
    		return 0;
	    }

	    tls_tx_gainindex_map2_gainvalue(tx_gain, tx_gain_index);
		for (i = 0; i < TX_GAIN_LEN/3; i++)
		{
			if (tx_gain_index[i] == 0xFF)
			{
				tx_gain[i] = 0xFF;
				tx_gain[i+TX_GAIN_LEN/3] = 0xFF;
				tx_gain[i+TX_GAIN_LEN*2/3] = 0xFF;
			}
			else
			{
				param_tx_gain[i] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN/3] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN*2/3] = tx_gain[i];
			}
		}
	    ret = tls_set_tx_gain(tx_gain);
	    if (ret == 0)
	    {
    		*res_len = factory_atcmd_ok_resp(res_resp);	
	    }
	    else
	    {
    		*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
	    }
    }
	else{
		/*如实反映flash参数区的实际存储情况*/
		ret = tls_get_tx_gain(tx_gain);
		if (ret == 0)
		{
			memset(tx_gain_index, 0xFF, TX_GAIN_LEN/3);
			tls_tx_gainvalue_map2_gainindex(tx_gain_index, tx_gain);
			*res_len = 0;
			len = 0;
			len = sprintf(res_resp, "+OK=");
			for (i = 0; i < TX_GAIN_LEN/3; i++)
			{
				len += sprintf(res_resp + len, "%02x", tx_gain_index[i]);
			}
			len += sprintf(res_resp + len, "\r\n");
			*res_len = len;
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_FLASH);
		}
    }
    return 0;
}

static void factory_atcmd_lpinit(void)
{
    tls_litepoint_start();
	tls_set_tx_litepoint_period(0);
}

static int factory_atcmd_lpchl_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	u32 channel = 0;
	u32 bandwidth = 0;
	int ret = 0;

	if (tok->arg_found)
	{
		ret = string_to_uint(tok->arg[0], (u32*)&channel);
		if (ret < 0 || (channel < 1) || (channel > 14))
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		
		if (tok->arg_found == 2)
		{
			string_to_uint(tok->arg[1], (u32*)&bandwidth);
		}

	    factory_atcmd_lpinit();
	    tls_set_test_channel(channel, bandwidth);
		*res_len = factory_atcmd_ok_resp(res_resp);
		return 0;
	}
	else
	{
		*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_FMT);
		return 0;
	}

    return 0;
}

static int factory_atcmd_lptstr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	u32 tempcomp = 0;
	u32 packetcount = 0;
	u32 psdulen = 0;
	u32 txgain = 0;
	u32 datarate = 0;
	u32 gimode = 0;
	u32 greenfield =0 ;
	u32 rifs = 0;
	int ret = 0;

	if (tok->arg_found >= 5)
	{
		ret = hexstr_to_unit(tok->arg[0], (u32*)&tempcomp);
		if (ret)
			goto ERR;
		
		ret = hexstr_to_unit(tok->arg[1], (u32*)&packetcount);
		if (ret)
			goto ERR;

		ret = hexstr_to_unit(tok->arg[2], (u32*)&psdulen);
		if (ret)
			goto ERR;
		ret = hexstr_to_unit(tok->arg[3], (u32*)&txgain);
		if (ret)
			goto ERR;

		ret = hexstr_to_unit(tok->arg[4], (u32*)&datarate);
		if (ret)
			goto ERR;

		switch (tok->arg_found)
		{
			case 8:
				ret = hexstr_to_unit(tok->arg[7], (u32*)&gimode);
				if (ret)
					goto ERR;				
			case 7:
				ret = hexstr_to_unit(tok->arg[6], (u32*)&greenfield);
				if (ret)
					goto ERR;				
			case 6:
				ret = hexstr_to_unit(tok->arg[5], (u32*)&rifs);	
				if (ret)
					goto ERR;
			break;
			default:
			break;
		}

		factory_atcmd_lpinit();
		tls_tx_litepoint_test_start(tempcomp,packetcount, psdulen, txgain, datarate, gimode, greenfield, rifs);

		*res_len = factory_atcmd_ok_resp(res_resp);
		return 0;
	}

ERR:
	*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
	return 0;
}

static int factory_atcmd_lptperiod_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	int ret = 0;
	u32 period = 0;
	if (tok->arg_found)
	{
		ret = string_to_uint(tok->arg[0], (u32*)&period);
		if (ret)
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		factory_atcmd_lpinit();
    	tls_set_tx_litepoint_period(period);
		*res_len = factory_atcmd_ok_resp(res_resp);
	}
    return 0;
}


static int factory_atcmd_lptstp_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	factory_atcmd_lpinit();
	tls_txrx_litepoint_test_stop();
	tls_lp_notify_lp_tx_data();
	*res_len = factory_atcmd_ok_resp(res_resp);
	return 0;
}

static int factory_atcmd_lptstt_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{

	factory_atcmd_lpinit();
	*res_len = sprintf(res_resp, "+OK=%x\r\n", tls_tx_litepoint_test_get_totalsnd());
	return 0;
}

static int factory_atcmd_lprstr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	u32 channel = 0;
	u32 bandwidth = 0;
	int ret = 0;

	if (tok->arg_found)
	{
		ret = hexstr_to_unit(tok->arg[0], (u32*)&channel);
		if (ret < 0 || (channel < 1) || (channel > 14))
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		
		if (tok->arg_found == 2)
		{
			hexstr_to_unit(tok->arg[1], (u32*)&bandwidth);
		}

	    factory_atcmd_lpinit();
		tls_rx_litepoint_test_start(channel, bandwidth);		
		*res_len = factory_atcmd_ok_resp(res_resp);
		return 0;
	}
	else
	{
		*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_FMT);
		return 0;
	}

    return 0;
}

static int factory_atcmd_lprstp_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
    factory_atcmd_lpinit();
	tls_txrx_litepoint_test_stop();		
	*res_len = factory_atcmd_ok_resp(res_resp);
	return 0;
}

static int factory_atcmd_lprstt_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	u32 cnt_total = 0, cnt_good = 0, cnt_bad = 0;

    factory_atcmd_lpinit();
	tls_rx_litepoint_test_result(&cnt_total, &cnt_good, &cnt_bad);
	*res_len = sprintf(res_resp, "+OK=%x,%x,%x", cnt_total, cnt_good, cnt_bad);

	return 0;
}

static int factory_atcmd_calfinish_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	int ret = 0;
	int val = 0;

	if (tok->arg_found)
	{
		ret = strtodec((int *)&val, tok->arg[0]);
		if (ret < 0 || ((val != 1) && (val != 0)))
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}

		ret = tls_rf_cal_finish_op((u8 *)&val, 1);
		if (ret == 0 )
		{
			*res_len = factory_atcmd_ok_resp(res_resp);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_FLASH);
		}
	}
	else
	{
		ret = tls_rf_cal_finish_op((u8 *)&val, 0);
		if (ret == 0)
		{
			if (1 != val)
			{
				val = 0;
			}
			*res_len = sprintf(res_resp, "+OK=%d\r\n", val);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_FLASH);
		}
	}

	return 0;
}

static int factory_atcmd_freq_err_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	int ret = 0;
	int val = 0;

	if (tok->arg_found)
	{
		ret = strtodec((int *)&val, tok->arg[0]);
		if (ret < 0 )
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		
		ret = tls_freq_err_op((u8 *)&val, 1);
		if (ret == 0)
		{
			tls_rf_freqerr_adjust(val);
			*res_len = factory_atcmd_ok_resp(res_resp);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_FLASH);
		}		
		return 0;
	}
	else
	{
		ret = tls_freq_err_op((u8 *)&val, 0);
		if (ret == 0)
		{
			*res_len = sprintf(res_resp, "+OK=%d\r\n", (val == -1)? 0:val);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_FLASH);
		}		

		return 0;
	}
}

/*dummy function*/
static int factory_atcmd_wbgr_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	*res_len = factory_atcmd_ok_resp(res_resp);
	return 0;
}

static int factory_atcmd_qver_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	*res_len = sprintf(res_resp, "+OK=%c%x.%02x.%02x.%02x%02x,%c%x.%02x.%02x@ %s %s",
                HwVer[0], HwVer[1], HwVer[2],HwVer[3], HwVer[4], HwVer[5], \
                FirmWareVer[0], FirmWareVer[1], FirmWareVer[2],FirmWareVer[3], \
                __DATE__, __TIME__);
	return 0;
}
static int factory_atcmd_test_mode_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	wm_adc_config(0);
	wm_adc_config(1);
	wm_adc_config(2);
	wm_adc_config(3);
	*res_len = factory_atcmd_ok_resp(res_resp);
	return 0;
}
static int factory_atcmd_adc_cal_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	int ret = 0;
	int val = 0;
	int i = 0; 
	int refvoltage[4] = {0};
	FT_ADC_CAL_ST adc_cal;

	if (tok->arg_found)
	{
		if (tok->arg_found < 5)
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		ret = strtodec(&val, tok->arg[0]);
		if (ret < 0 || val > 15 || val < 1)
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
		for(i = 0; i < 4; i++)
		{
			ret = strtodec(&refvoltage[i], tok->arg[i + 1]);
			if (ret < 0 )
			{
				*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
				return 0;
			}
		}
		
		ret = adc_multipoint_calibration(val, refvoltage);
		if (ret == 0)
		{
			*res_len = factory_atcmd_ok_resp(res_resp);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_FLASH);
		}		
		return 0;
	}
	else
	{
		ret = tls_get_adc_cal_param(&adc_cal);
		//dumpBuffer("&adc_cal", &adc_cal, sizeof(adc_cal));
		if (ret == 0)
		{
			*res_len = sprintf(res_resp, "+OK=%lf,%lf\r\n", adc_cal.a, adc_cal.b);
		}
		else
		{
			*res_len = factory_atcmd_err_resp(res_resp,FACTORY_ATCMD_ERR_FLASH);
		}		

		return 0;
	}
}

static int factory_atcmd_adc_vol_proc( struct factory_atcmd_token_t *tok, char *res_resp, u32 *res_len)
{
	int ret = 0;
	int val = 0xF;
	int i = 0; 
	int voltage[4] = {0};

	if (tok->arg_found)
	{
		ret = strtodec(&val, tok->arg[0]);
		if (ret < 0 || val > 15 || val < 1)
		{
			*res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_INV_PARAMS);
			return 0;
		}
	}
	
	for(i = 0; i < 4; i++)
	{
		if (val & (1 << i))
		{
			wm_adc_config(i);
			voltage[i] = adc_get_inputVolt(i);
		}
	}
	*res_len = sprintf(res_resp, "+OK=%d,%d,%d,%d\r\n", voltage[0], voltage[1], voltage[2], voltage[3]);
	return 0;
}


static struct factory_atcmd_t  factory_atcmd_tbl[] =
{
    { "&REGR",   factory_atcmd_regr_proc },
    { "&REGW",   factory_atcmd_regw_proc },
    { "&RFR",    factory_atcmd_rfr_proc },
    { "&RFW",    factory_atcmd_rfw_proc },
   	{ "&FLSW",   factory_atcmd_flsw_proc},
   	{ "&FLSR",   factory_atcmd_flsr_proc},   	
   	{ "Z",       factory_atcmd_reset_proc}, 
    { "WBGR",    factory_atcmd_wbgr_proc},
    { "QVER",    factory_atcmd_qver_proc},
	{ "&TXG",    factory_atcmd_txg_proc},
	{ "&TXGI",   factory_atcmd_txgi_proc},
	{ "QMAC",    factory_atcmd_mac_proc},
	{ "&MAC",    factory_atcmd_mac_proc},
	{ "&LPCHL",  factory_atcmd_lpchl_proc},
	{ "&LPTSTR", factory_atcmd_lptstr_proc},
	{ "&LPTSTP", factory_atcmd_lptstp_proc},
    { "&LPTPD",  factory_atcmd_lptperiod_proc},
	{ "&LPTSTT", factory_atcmd_lptstt_proc},
	{ "&LPRSTR", factory_atcmd_lprstr_proc},
	{ "&LPRSTP", factory_atcmd_lprstp_proc},
	{ "&LPRSTT", factory_atcmd_lprstt_proc},
    { "&CALFIN", factory_atcmd_calfinish_proc},
    { "FREQ",    factory_atcmd_freq_err_proc},
    { "&TESTM",  factory_atcmd_test_mode_proc},
    { "&ADCCAL", factory_atcmd_adc_cal_proc},
    { "&ADCVOL", factory_atcmd_adc_vol_proc},
    { NULL,      NULL },
};

static int factory_atcmd_nop_proc(struct factory_atcmd_token_t *tok,
                          char *res_resp, u32 *res_len)
{
    if (!tok->arg_found && (tok->op == FACTORY_ATCMD_OP_NULL))
    {
        *res_len = factory_atcmd_ok_resp(res_resp);
    }
    else
    {
        *res_len = factory_atcmd_err_resp(res_resp, FACTORY_ATCMD_ERR_OPS);
    }

    return 0;
}

static int factory_atcmd_parse(struct factory_atcmd_token_t *tok, char *buf, u32 len)
{
    char *c, *end_line, *comma;
    int remain_len;
    char *buf_start = buf;

    /* at command "AT+", NULL OP */
    if (len == 0)
    {
        *tok->name = '\0';
        tok->arg_found = 0;
        return -1;
    }

    /* parse command name */
    c = strchr(buf, '=');
    if (!c)
    {
        /* format :  at+wprt */
        c = strchr(buf, '\n');
        if (!c)
            return -FACTORY_ATCMD_ERR_INV_FMT;
        if ((c - buf) > (FACTORY_ATCMD_NAME_MAX_LEN - 1))
            return -FACTORY_ATCMD_ERR_UNSUPP;
        memcpy(tok->name, buf, c - buf);
        *(tok->name + (c - buf)) = '\0';
        tok->op = FACTORY_ATCMD_OP_NULL;
        tok->arg_found = 0;
        return 0;
    }
    else
    {
        /* format : at+wprt=0
         *          at+skct=0,0,192.168.1.4,80 */
        if ((c - buf) > (FACTORY_ATCMD_NAME_MAX_LEN - 1))
            return -FACTORY_ATCMD_ERR_UNSUPP;
        memcpy(tok->name, buf, c - buf);
        *(tok->name + (c - buf)) = '\0';
        tok->op = FACTORY_ATCMD_OP_NULL;
        buf += (c - buf + 1);
        switch(*buf)
        {
        case '!':
            tok->op = FACTORY_ATCMD_OP_EP;
            buf++;
            break;
        case '?':
            tok->op = FACTORY_ATCMD_OP_QU;
            buf++;
            break;
        default:
            tok->op = FACTORY_ATCMD_OP_EQ;
            break;
        }
        tok->arg[0] = buf;
        tok->arg_found = 0;
        remain_len = len - (buf - buf_start);
        end_line = strchr(buf, '\n');
        if (!end_line)
            return -FACTORY_ATCMD_ERR_INV_FMT;
        while (remain_len > 0)
        {
            comma = strchr(buf, ',');
            if (end_line && !comma)
            {
                if (tok->arg_found >= (FACTORY_ATCMD_MAX_ARG - 1))
                    return -FACTORY_ATCMD_ERR_INV_PARAMS;
                if (end_line != buf)
                    tok->arg_found++;
                /* last parameter */
                *(u8 *)end_line = '\0';
                tok->arg[tok->arg_found] = end_line + 1;
                remain_len -= (end_line - buf);
                if (remain_len > 1)
                    return -FACTORY_ATCMD_ERR_NOT_ALLOW;
                else
                    return 0;
            }
            else
            {
                if (tok->arg_found >= (FACTORY_ATCMD_MAX_ARG - 1))
                    return -FACTORY_ATCMD_ERR_INV_PARAMS;
                tok->arg_found++;
                *(u8 *)comma = '\0';
                tok->arg[tok->arg_found] = comma + 1;
                remain_len -= (comma - buf + 1);
                buf = comma + 1;
            }
        }
        return 0;
    }
}

static int factory_atcmd_exec(
    struct factory_atcmd_token_t *tok,
    char *res_rsp, u32 *res_len)
{
    int err;
    struct factory_atcmd_t *atcmd, *match = NULL;
    int i = 0;
    int name_len = strlen(tok->name);

    for (i = 0; i < name_len; i++)
        tok->name[i] = toupper(tok->name[i]);

    if (strlen(tok->name) == 0)
    {
        err = factory_atcmd_nop_proc(tok, res_rsp, res_len);
        return err;
    }

    /* look for AT CMD handle table */
    atcmd = factory_atcmd_tbl;
    while (atcmd->name)
    {
        if (strcmp(atcmd->name, tok->name) == 0)
        {
            match = atcmd;
            break;
        }
        atcmd++;
    }

    /* at command handle */
    if (match)
    {
        //TLS_DBGPRT_INFO("command find: %s\n", atcmd->name);
        err = match->proc_func(tok, res_rsp, res_len);
    }
    else
    {
        /* at command not found */
        *res_len = sprintf(res_rsp, "+ERR=%d", -FACTORY_ATCMD_ERR_UNSUPP);
        err = -FACTORY_ATCMD_ERR_UNSUPP;
    }

    return err;
}


int factory_atcmd_enter(char *charbuf, unsigned char charlen)
{
    struct factory_atcmd_token_t atcmd_tok;
    int err;
    u32 cmdrsp_size;

    if ((charlen >= 2) && (charbuf[charlen - 2] == '\r' || charbuf[charlen - 2] == '\n'))
    {
        charbuf[charlen - 2] = '\n';
        charbuf[charlen - 1] = '\0';
        charlen = charlen - 1;
    }
    else if ((charlen >= 1) && (charbuf[charlen - 1] == '\r' || charbuf[charlen - 1] == '\n'))
    {
        charbuf[charlen - 1] = '\n';
        charbuf[charlen] = '\0';
        charlen = charlen;
    }
    else
    {
        charbuf[charlen] = '\n';
        charbuf[charlen + 1] = '\0';
        charlen = charlen + 1;
    }

    memset(&atcmd_tok, 0, sizeof(struct factory_atcmd_token_t));

    err = factory_atcmd_parse(&atcmd_tok, (char *)charbuf, charlen);
    if (err)
        return -1;

    err = factory_atcmd_exec(&atcmd_tok, factory_cmdrsp_buf, &cmdrsp_size);
    if (err && (err != -FACTORY_ATCMD_ERR_UNSUPP))
    {
        return -1;
    }

	if (cmdrsp_size < (FACTORY_ATCMD_RESPONSE_SIZE - 5))
	{
	    factory_cmdrsp_buf[cmdrsp_size] = '\r';
	    factory_cmdrsp_buf[cmdrsp_size + 1] = '\n';
	    factory_cmdrsp_buf[cmdrsp_size + 2] = '\r';
	    factory_cmdrsp_buf[cmdrsp_size + 3] = '\n';	
	    factory_cmdrsp_buf[cmdrsp_size + 4] = '\0';
		cmdrsp_size += 4;

		tls_uart_write(TLS_UART_0, factory_cmdrsp_buf, cmdrsp_size);
	}
	else
	{
		
	}

    return 0;
}


static void factory_atcmd_thread_handle(void * pArg)
{
    wm_printf("%s  Successfully\r\n",__FUNCTION__);
    uint8_t *buf = NULL;
    uint8_t *pch = NULL;
    int len = 0;

    buf = (uint8_t *)tls_mem_alloc(FACTORY_ATCMD_BUF_SIZE);
	if (buf == NULL)
	{
		while(1)
		{
			wm_printf("fcmd buffer is NULL\r\n");
			tls_os_time_delay(1000);
		}
	}
	
    while(1)
    {
        len += tls_uart_read(TLS_UART_0, buf + len, (FACTORY_ATCMD_BUF_SIZE - len));
		if ((len >= 3) && (buf[0] == '+') && (buf[1] == '+') && (buf[2] == '+'))
		{
			factory_cmdrsp_buf[0] = '+';
			factory_cmdrsp_buf[1] = 'O';
			factory_cmdrsp_buf[2] = 'K';
			factory_cmdrsp_buf[3] = '\r';
			factory_cmdrsp_buf[4] = '\n';
			factory_cmdrsp_buf[5] = '\r';
			factory_cmdrsp_buf[6] = '\n'; 
			factory_cmdrsp_buf[7] = '\0';
			tls_uart_write(TLS_UART_0, factory_cmdrsp_buf, 6);
			memset(buf, 0, FACTORY_ATCMD_BUF_SIZE);
			len = 0;
		}
		else if ((len >= 3)
			&& ((buf[0] == 'a' || buf[0] == 'A')
			&& (buf[1] == 't' || buf[1] == 'T') 
			&& (buf[2] == '+')))
		{	
            pch = (uint8_t *)strchr((char *)buf, '\r');	
			if (pch == NULL)
			{
				pch = (uint8_t *)strchr((char *)buf, '\n');
			}

			if (pch)
			{
				if (len > (pch - buf))
				{
					buf[pch-buf + 1] = '\0';
		            factory_atcmd_enter((char *)&buf[3], (pch - buf - 1));
				}
				memset(buf, 0, FACTORY_ATCMD_BUF_SIZE);
				len = 0;
			}
			else
			{
				if (len >= FACTORY_ATCMD_BUF_SIZE)
				{
					memset(buf, 0, FACTORY_ATCMD_BUF_SIZE);
					len = 0;
				}
			}
		}
		else
		{
			if (len >= 3)
			{
				memset(buf, 0, FACTORY_ATCMD_BUF_SIZE);
				len = 0;				
			}
		}
        tls_os_time_delay(50);
    }
}

void factory_atcmd_init(void)
{
	factory_atcmd_stack = tls_mem_alloc(FACTORY_ATCMD_STACK_SIZE);
	factory_cmdrsp_buf = tls_mem_alloc(FACTORY_ATCMD_RESPONSE_SIZE);
	if (factory_atcmd_stack && factory_cmdrsp_buf)
	{
		memset(factory_cmdrsp_buf, 0, FACTORY_ATCMD_RESPONSE_SIZE);
		memset(factory_atcmd_stack, 0, FACTORY_ATCMD_STACK_SIZE);

		tls_uart_port_init(TLS_UART_0, NULL, 0);

		tls_os_task_create(NULL,NULL,factory_atcmd_thread_handle,NULL,(u8 *)factory_atcmd_stack,FACTORY_ATCMD_STACK_SIZE,50,0);
	}
}

