/*****************************************************************************
*
* File Name : wm_param.c
*
* Description: param manager Module
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : dave
*
* Date : 2014-6-12
*****************************************************************************/
#include <string.h>
#include "wm_debug.h"
#include "wm_efuse.h"
#include "wm_flash.h"
#include "wm_internal_flash.h"
#include "wm_params.h"
#include "wm_param.h"
#include "wm_mem.h"
#include "utils.h"
#include "wm_flash_map.h"
#include "wm_crypto_hard.h"

#define USE_TWO_RAM_FOR_PARAMETER  0
static struct tls_param_flash flash_param;
#if USE_TWO_RAM_FOR_PARAMETER
static struct tls_sys_param sram_param;
#endif

struct tls_sys_param *user_default_param = NULL;
struct tls_sys_param * tls_param_user_param_init(void);

static tls_os_sem_t *sys_param_lock = NULL;
static const u8 factory_default_hardware[8] = {'H', 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 updp_mode;//upadate default parameters mode, 0:not updating or up finish; 1:updating

static int param_flash_verify(u32 data_addr, u8 *data_buffer, u32 len)
{
	int err;
	u8 *buffer;

	buffer = tls_mem_alloc(len);
	if (buffer == NULL) {return 0;}

	do {
		err = tls_fls_read(data_addr, buffer, len);
		if (err != TLS_FLS_STATUS_OK) {
			err = 0;
			break;
		}

		if (memcmp(buffer, data_buffer, len) == 0) {
			err = 1;
		} else {
			err = 0;
		}
	} while (0);

	tls_mem_free(buffer);

	return err;
}

static int param_to_flash(int id, int modify_count, int partition_num)
{
	int err;
#if USE_TWO_RAM_FOR_PARAMETER	
	struct tls_sys_param *src;
	struct tls_sys_param *dest;
#endif	

	if ((id < TLS_PARAM_ID_ALL) || (id >= TLS_PARAM_ID_MAX)) {return TLS_PARAM_STATUS_EINVALID;}

	err = TLS_PARAM_STATUS_OK;

#if USE_TWO_RAM_FOR_PARAMETER
	src = &sram_param;
	dest = &flash_param.parameters;

	switch (id) {
		case TLS_PARAM_ID_ALL:
			MEMCPY(dest, src, sizeof(struct tls_sys_param));
			break;

		case TLS_PARAM_ID_SSID:
			MEMCPY(&dest->ssid, &src->ssid, sizeof(struct tls_param_ssid));
			break;

		case TLS_PARAM_ID_ENCRY:
			dest->encry = src->encry;
			break;

		case TLS_PARAM_ID_KEY:
			MEMCPY(&dest->key, &src->key, sizeof(struct tls_param_key));
			break;

		case TLS_PARAM_ID_IP:
			MEMCPY(&dest->ipcfg, &src->ipcfg, sizeof(struct tls_param_ip));
			break;

		case TLS_PARAM_ID_AUTOMODE:
			dest->auto_mode = src->auto_mode;
			break;

		case TLS_PARAM_ID_DEFSOCKET:
			MEMCPY(&dest->remote_socket_cfg, &src->remote_socket_cfg, sizeof(struct tls_param_socket));
			break;

		case TLS_PARAM_ID_BSSID:
			MEMCPY(&dest->bssid, &src->bssid, sizeof(struct tls_param_bssid));
			break;

		case TLS_PARAM_ID_CHANNEL:
			dest->channel = src->channel;
			break;

		case TLS_PARAM_ID_CHANNEL_EN:
			dest->channel_enable = src->channel_enable;
			break;

		case TLS_PARAM_ID_COUNTRY_REGION:
			dest->wireless_region = src->wireless_region;
			break;

		case TLS_PARAM_ID_WPROTOCOL:
			dest->wireless_protocol = src->wireless_protocol;
			break;

		case TLS_PARAM_ID_ADHOC_AUTOCREATE:
			dest->auto_create_adhoc = src->auto_create_adhoc;
			break;

		case TLS_PARAM_ID_ROAMING:
			dest->auto_roam = src->auto_roam;
			break;

		case TLS_PARAM_ID_AUTO_RETRY_CNT:
			dest->auto_retrycnt = src->auto_retrycnt;
			break;

		case TLS_PARAM_ID_WBGR:
			MEMCPY(&dest->wbgr, &src->wbgr, sizeof(struct tls_param_bgr));
			break;

		case TLS_PARAM_ID_USRINTF:
			dest->user_port_mode = src->user_port_mode;
			break;
		case TLS_PARAM_ID_ESCAPE_CHAR:
			dest->EscapeChar = src->EscapeChar;
			break;

		case TLS_PARAM_ID_ESCAPE_PERIOD:
			dest->EscapePeriod = src->EscapePeriod;
			break;

		case TLS_PARAM_ID_AUTO_TRIGGER_LENGTH:
			dest->transparent_trigger_length = src->transparent_trigger_length;
			break;

		case TLS_PARAM_ID_AUTO_TRIGGER_PERIOD:
			dest->transparent_trigger_period = src->transparent_trigger_period;
            break;

		case TLS_PARAM_ID_DEBUG_MODE:
			dest->debug_mode = src->debug_mode;
			break;

		case TLS_PARAM_ID_HARDVERSION:
			MEMCPY(&dest->hardware_version, &src->hardware_version, sizeof(struct tls_param_hardware_version));
			break;

		case TLS_PARAM_ID_BRDSSID:
			dest->ssid_broadcast_enable = src->ssid_broadcast_enable;
			break;

		case TLS_PARAM_ID_DNSNAME:
			MEMCPY(dest->local_dnsname, src->local_dnsname, 32);
			break;

		case TLS_PARAM_ID_DEVNAME:
			MEMCPY(dest->local_device_name, src->local_device_name, 32);
			break;

		case TLS_PARAM_ID_PSM:
			dest->auto_powersave = src->auto_powersave;
			break;

		case TLS_PARAM_ID_ORAY_CLIENT:
			MEMCPY(&dest->oray_client_setting, &src->oray_client_setting, sizeof(struct tls_param_oray_client));
			break;

		case TLS_PARAM_ID_UPNP:
			dest->upnp_enable = src->upnp_enable;
			break;

		case TLS_PARAM_ID_UART:
			MEMCPY(&dest->uart_cfg, &src->uart_cfg, sizeof(struct tls_param_uart));
			break;
#if TLS_CONFIG_WPS
		case TLS_PARAM_ID_WPS:
			MEMCPY(&dest->wps, &src->wps, sizeof(struct tls_param_wps));
			break;
#endif
		case TLS_PARAM_ID_CHANNEL_LIST:
			dest->channellist = src->channellist;
			break;
		case TLS_PARAM_ID_ONESHOT_CFG:
			dest->oneshotflag = src->oneshotflag;
			break;
		case TLS_PARAM_ID_SHA1:
			MEMCPY(&dest->psk, &src->psk, sizeof(struct tls_param_sha1));
			break;
		case TLS_PARAM_ID_ORIGIN_KEY:
			MEMCPY(&dest->original_key, &src->original_key, sizeof(struct tls_param_original_key));
			break;
		case TLS_PARAM_ID_ORIGIN_SSID:
			MEMCPY(&dest->original_ssid, &src->original_ssid, sizeof(struct tls_param_ssid));
			break;
		case TLS_PARAM_ID_AUTO_RECONNECT:
			dest->auto_reconnect = src->auto_reconnect;
			break;

		case TLS_PARAM_ID_IO_MODE:
			dest->IoMode = src->IoMode;
			break;

		case TLS_PARAM_ID_CMD_MODE:
			dest->CmdMode = src->CmdMode;
			break;

	    case TLS_PARAM_ID_PASSWORD:
			MEMCPY(dest->PassWord, src->PassWord, sizeof(src->PassWord));
			break;

		case TLS_PARAM_ID_WEBS_CONFIG:
			dest->WebsCfg = src->WebsCfg;
			break;
		case TLS_PARAM_ID_QUICK_CONNECT:
            MEMCPY(&dest->quick_connect, &src->quick_connect, sizeof(struct tls_param_quick_connect));
            break;
        case TLS_PARAM_ID_KEY_CHANGE:
            dest->key_changed = src->key_changed;
            break;
        case TLS_PARAM_ID_SSID_CHANGE:
            dest->ssid_changed = src->ssid_changed;
            break;
#if TLS_CONFIG_AP
        case TLS_PARAM_ID_SOFTAP_SSID:
            MEMCPY(&dest->apsta_ssid, &src->apsta_ssid, sizeof(struct tls_param_ssid));
            break;
        case TLS_PARAM_ID_SOFTAP_PSK:
            MEMCPY(&dest->apsta_psk, &src->apsta_psk, sizeof(struct tls_param_sha1));
            break;
        case TLS_PARAM_ID_SOFTAP_ENCRY:
            dest->encry4softap = src->encry4softap;
            break;
        case TLS_PARAM_ID_SOFTAP_KEY:
            MEMCPY(&dest->key4softap, &src->key4softap, sizeof(struct tls_param_key));
            break;
        case TLS_PARAM_ID_SOFTAP_IP:
            MEMCPY(&dest->ipcfg4softap, &src->ipcfg4softap, sizeof(struct tls_param_ip));
            break;
        case TLS_PARAM_ID_SOFTAP_CHANNEL:
            dest->channel4softap = src->channel4softap;
            break;
        case TLS_PARAM_ID_SOFTAP_WBGR:
			MEMCPY(&dest->wbgr4softap, &src->wbgr4softap, sizeof(struct tls_param_bgr));
            break;
#endif
		case TLS_PARAM_ID_SNTP_SERVER1:
			strncpy(dest->sntp_service1, src->sntp_service1, strlen(src->sntp_service1)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER2:
			strncpy(dest->sntp_service2, src->sntp_service2, strlen(src->sntp_service2)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER3:
			strncpy(dest->sntp_service3, src->sntp_service3, strlen(src->sntp_service3)+1);
			break;
        case TLS_PARAM_ID_TEM_OFFSET:
			MEMCPY(&dest->params_tem, &src->params_tem, sizeof(struct tls_param_tem_offset));
            break;

		
		case TLS_PARAM_ID_BT_ADAPTER:
			MEMCPY(&dest->adapter_t, &src->adapter_t, sizeof(bt_adapter_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_1:
			MEMCPY(&dest->remote_device1, &src->remote_device1, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_2:
			MEMCPY(&dest->remote_device2, &src->remote_device2, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_3:
			MEMCPY(&dest->remote_device3, &src->remote_device3, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_4:
			MEMCPY(&dest->remote_device4, &src->remote_device4, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_5:
			MEMCPY(&dest->remote_device5, &src->remote_device5, sizeof(bt_remote_device_t));
			break;
#if 0

		case TLS_PARAM_ID_BT_REMOTE_DEVICE_6:
			MEMCPY(&dest->remote_device6, &src->remote_device6, sizeof(bt_remote_device_t));
			break;
			
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_7:
			MEMCPY(&dest->remote_device7, &src->remote_device7, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_8:
			MEMCPY(&dest->remote_device8, &src->remote_device8, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_9:
			MEMCPY(&dest->remote_device9, &src->remote_device9, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_10:
			MEMCPY(&dest->remote_device10, &src->remote_device10, sizeof(bt_remote_device_t));
			break;
#endif
		default:
			err = TLS_PARAM_STATUS_EINVALIDID;
			goto exit;
	}
#endif
	flash_param.magic = TLS_PARAM_MAGIC;
	flash_param.length = sizeof(flash_param);

	if (modify_count < 0){
		flash_param.modify_count ++;
		TLS_DBGPRT_INFO("update the \"modify count(%d)\".\n", flash_param.modify_count);
	} else {
		flash_param.modify_count  = modify_count;
		TLS_DBGPRT_INFO("initialize the \"modify count(%d)\".\n", flash_param.modify_count);
	}

	if (partition_num < 0) {
		flash_param.partition_num = (flash_param.partition_num + 1) & 0x01;
		TLS_DBGPRT_INFO("switch the parameter patition number(%d).\n", flash_param.partition_num);
	} else {
		flash_param.partition_num = partition_num;
		TLS_DBGPRT_INFO("initialize the parameter patition number(%d).\n", flash_param.partition_num);
	}
	flash_param.resv_1 = flash_param.resv_2 = 0;
	flash_param.crc32 = get_crc32((u8 *)&flash_param, sizeof(flash_param) - 4);

	TLS_DBGPRT_INFO("update the parameters to parameter patition(%d) in spi flash.\n", flash_param.partition_num);

	err = tls_fls_write((flash_param.partition_num == 0) ? TLS_FLASH_PARAM1_ADDR : TLS_FLASH_PARAM2_ADDR,
		(u8 *)&flash_param, sizeof(flash_param));
	if (err != TLS_FLS_STATUS_OK) {
		TLS_DBGPRT_ERR("write to spi flash fail(%d)!\n", err);
		err = TLS_PARAM_STATUS_EIO;
		goto exit;
	}
	if (param_flash_verify((flash_param.partition_num == 0) ? TLS_FLASH_PARAM1_ADDR : TLS_FLASH_PARAM2_ADDR,
		(u8 *)&flash_param, sizeof(flash_param)) == 1) {err = TLS_PARAM_STATUS_OK;}
	else {
		TLS_DBGPRT_ERR("verify the parameters in spi flash fail(%d)!\n", err);
		err = TLS_PARAM_STATUS_EIO;
	}
exit:

	return err;
}

/**********************************************************************************************************
* Description: 	This function is used to initial system param.
*
* Arguments  : 	sys_param		is the system param addr
*

* Returns    :		TLS_PARAM_STATUS_OK		init success
*				TLS_PARAM_STATUS_EMEM		memory error
*				TLS_PARAM_STATUS_EIO		io error
*				TLS_PARAM_STATUS_EPERM
**********************************************************************************************************/
int tls_param_init(void)
{
	bool is_damage[TLS_PARAM_PARTITION_NUM];
	u8 damaged;
	int err;
	signed short i;
	u16 tryrestore = 0;
	u32 crckey = 0xFFFFFFFF;
	psCrcContext_t ctx;
	u32 crcvalue = 0;
	
	struct tls_param_flash *flash;

	if(flash_param.magic == TLS_PARAM_MAGIC)
	{
		TLS_DBGPRT_ERR("parameter management module has been initialized!\n");
		return TLS_PARAM_STATUS_EPERM;
	}

	err = tls_os_sem_create(&sys_param_lock, 1);
	if (err != TLS_OS_SUCCESS)
	{
		TLS_DBGPRT_ERR("create semaphore @sys_param_lock fail!\n");
		return TLS_PARAM_STATUS_EMEM;
	}

	tls_os_sem_acquire(sys_param_lock, 0);
	err = TLS_PARAM_STATUS_OK;
	i = 0;
	damaged= 0;
	is_damage[0] = is_damage[1] = FALSE;
	flash = NULL;
	memset(&flash_param, 0, sizeof(flash_param));
#if USE_TWO_RAM_FOR_PARAMETER	
	memset(&sram_param, 0, sizeof(sram_param));
#endif
	tryrestore = 0;
	do
	{
		flash = tls_mem_alloc(sizeof(*flash));
		if (flash == NULL)
		{
			TLS_DBGPRT_ERR("allocate \"struct tls_param_flash\" fail!\n");
			err = TLS_PARAM_STATUS_EMEM;
			break;
		}
		memset(flash, 0, sizeof(*flash));

		for (i = 0; i < TLS_PARAM_PARTITION_NUM; i++)
		{
			TLS_DBGPRT_INFO("read parameter partition - %d.\n", i);
			tls_fls_read((i == 0) ? TLS_FLASH_PARAM1_ADDR : TLS_FLASH_PARAM2_ADDR, (u8 *)flash, sizeof(*flash));
			TLS_DBGPRT_INFO("patition %d magic - 0x%x, crc -0x%x .\n", i, flash->magic, flash->crc32);

			if (flash->magic == TLS_PARAM_MAGIC)
			{
				crckey = 0xFFFFFFFF;
				tls_crypto_crc_init(&ctx, crckey, CRYPTO_CRC_TYPE_32, 3);
				tls_crypto_crc_update(&ctx, (u8 *)flash, flash->length - 4);
				tls_crypto_crc_final(&ctx, &crcvalue);
			}

			if (flash->magic != TLS_PARAM_MAGIC)
			{
				TLS_DBGPRT_WARNING("parameter partition - %d has been damaged.\n", i);
				is_damage[i] = TRUE;
				damaged++;
				continue;
			}
			else if ((~crcvalue) != *(u32*)((u8*)flash + flash->length - 4))
			{
				is_damage[i] = TRUE;
				damaged++;
				continue;
			}
			else
			{
				/* Load the latest parameters */
				TLS_DBGPRT_INFO("read parameter partition modify count - %d.\n", flash->modify_count);
				TLS_DBGPRT_INFO("current parameter partition modify count - %d.\n", flash_param.modify_count);
				if ((flash_param.magic == 0) || (flash_param.modify_count < flash->modify_count))
				{
					TLS_DBGPRT_INFO("update the parameter in sram using partition - %d,%d,%d.\n", i, flash->length,sizeof(*flash));
					if (flash->length != sizeof(*flash)){
						MEMCPY(&flash_param, flash, (flash->length-4));
#if USE_TWO_RAM_FOR_PARAMETER						
						MEMCPY(&sram_param, &flash_param.parameters, sizeof(sram_param));
#endif
					}else{
						MEMCPY(&flash_param, flash, sizeof(*flash));
#if USE_TWO_RAM_FOR_PARAMETER						
						MEMCPY(&sram_param, &flash_param.parameters, sizeof(sram_param));
#endif
					}
				}
				memset(flash, 0, sizeof(*flash));
			}

			/* try to erase one sector at the same block to restore parameter area*/
			if ((tryrestore == 0)&&(damaged >= TLS_PARAM_PARTITION_NUM))
			{
				damaged= 0;
				is_damage[0] = is_damage[1] = FALSE;
				memset(&flash_param, 0, sizeof(flash_param));
#if USE_TWO_RAM_FOR_PARAMETER				
				memset(&sram_param, 0, sizeof(sram_param));
#endif
				tls_fls_erase(TLS_FLASH_PARAM_RESTORE_ADDR / INSIDE_FLS_SECTOR_SIZE);
				tryrestore = 1;
				i = -1;
			}
		}

		if (damaged >= TLS_PARAM_PARTITION_NUM)
		{
			TLS_DBGPRT_INFO("all the parameter partitions has been demaged and load the default parameters.\n");
			tls_param_load_factory_default();

			TLS_DBGPRT_INFO("write the default parameters to all the partitions.\n");
			err = param_to_flash(TLS_PARAM_ID_ALL, 1, 0);
			if (err != TLS_PARAM_STATUS_OK)
			{
				TLS_DBGPRT_ERR("write the default parameters to the partitions - 0 fail!\n");
				err = TLS_PARAM_STATUS_EIO;
				break;
			}

			err = param_to_flash(TLS_PARAM_ID_ALL, 1, 1);
			if (err != TLS_PARAM_STATUS_OK)
			{
				TLS_DBGPRT_ERR("write the default parameters to the partitions - 1 fail!\n");
				err = TLS_PARAM_STATUS_EIO;
				break;
			}

			//MEMCPY(&flash_param, flash, sizeof(*flash));
		}
		else
		{
			/* restore damaged partitions */
			for (i = 0; i < TLS_PARAM_PARTITION_NUM; i++)
			{
				if (is_damage[i])
				{
					TLS_DBGPRT_INFO(" restore damaged partitions - %d.\n", i);
					err = param_to_flash(TLS_PARAM_ID_ALL, -1, i);
					if (err != TLS_PARAM_STATUS_OK)
					{
						TLS_DBGPRT_ERR("write the default parameters to the partitions - %d fail!\n", i);
						err = TLS_PARAM_STATUS_EIO;
						break;
					}
				}
			}

			if (err == TLS_PARAM_STATUS_OK)
			{
				break;
			}
		}
	}while (0);

	tls_os_sem_release(sys_param_lock);

	if (flash)
	{
		tls_mem_free(flash);
	}

#if 0
 	TLS_DBGPRT_INFO("sys_param = 0x%x, *sys_param = 0x%x\n", sys_param, *sys_param);
	if (sys_param)
	{
		*sys_param = &sram_param;
	}
	TLS_DBGPRT_INFO("sys_param = 0x%x, *sys_param = 0x%x\n", sys_param, *sys_param);
#endif
	return err;
}

#define TLS_USER_MAGIC	0x574D3031	// WM01
int tls_param_load_user(struct tls_sys_param *param)
{
	u32 magic, crc32, offset;

	offset = TLS_FLASH_PARAM_DEFAULT;
	tls_fls_read(offset, (u8 *)&magic, 4);
	if(TLS_USER_MAGIC != magic)
	{
		TLS_DBGPRT_INFO("no user default param = %x!!!\r\n", magic);
		return TLS_PARAM_STATUS_EINVALID;
	}
	offset += 4;
	memset(param, 0, sizeof(*param));
	tls_fls_read(offset, (u8 *)param, sizeof(struct tls_sys_param));
	offset += sizeof(struct tls_sys_param);
	tls_fls_read(offset, (u8 *)&crc32, 4);
	if(crc32 != get_crc32((u8 *)param, sizeof(struct tls_sys_param)))
	{
		TLS_DBGPRT_INFO("user default param crc err =%x!!!\r\n", crc32);
		return TLS_PARAM_STATUS_EINVALID;
	}
	return TLS_PARAM_STATUS_OK;
}

/**********************************************************************************************************
* Description: 	This function is used to load the system default parameters.
*
* Arguments  : 	param		is the param point
*

* Returns    :
*
* Notes	:		This function read user defined parameters first, if wrong, all the parameters restore factory settings.
**********************************************************************************************************/
void tls_param_load_factory_default(void)
{
#if USE_TWO_RAM_FOR_PARAMETER
	struct tls_sys_param *param = &sram_param;
#else
	struct tls_sys_param *param = &flash_param.parameters;
#endif

	if (param == NULL) {return;}

	TLS_DBGPRT_INFO("load the default parameters.\n");

	memset(param, 0, sizeof(*param));
	MEMCPY(&param->hardware_version, factory_default_hardware, 8);

	param->wireless_region = TLS_PARAM_REGION_1_BG_BAND;
	param->channel = 1;
	param->channellist = 0x3FFF;
#if TLS_CONFIG_11N
    param->wbgr.bg = TLS_PARAM_PHY_11BGN_MIXED;
    param->wbgr.max_rate = TLS_PARAM_TX_RATEIDX_MCS13;
#else
	param->wbgr.bg = TLS_PARAM_PHY_11BG_MIXED;
    param->wbgr.max_rate = TLS_PARAM_TX_RATEIDX_36M;
#endif
	param->ssid_broadcast_enable = TLS_PARAM_SSIDBRD_ENABLE;
	param->encry = TLS_PARAM_ENCRY_OPEN;
#if 0 //def CONFIG_AP
	param->wireless_protocol = TLS_PARAM_IEEE80211_SOFTAP;
	tls_efuse_read(TLS_EFUSE_MACADDR_OFFSET, mac_addr, 6);
	ssid_len = sprintf((char *)&param->ssid.ssid, "cuckoo_softap_%02x%02x%02x", mac_addr[3], mac_addr[4], mac_addr[5]);
	param->ssid.ssid_len = ssid_len;
#else
	param->wireless_protocol = TLS_PARAM_IEEE80211_INFRA;

#endif

	param->auto_retrycnt = 255;
	param->auto_roam = TLS_PARAM_ROAM_DISABLE;
	param->auto_powersave = TLS_PARAM_PSM_DISABLE;
	param->auto_reconnect = 0;
	//param->wps.wps_enable = TLS_PARAM_WPS_DISABLE;

	param->auto_mode = TLS_PARAM_MANUAL_MODE;
	param->transparent_trigger_length = 512;

	param->uart_cfg.baudrate = TLS_PARAM_UART_BAUDRATE_B115200;
	param->uart_cfg.stop_bits = TLS_PARAM_UART_STOPBITS_1BITS;
	param->uart_cfg.parity = TLS_PARAM_UART_PARITY_NONE;

	param->user_port_mode = TLS_PARAM_USR_INTF_LUART;

	param->ipcfg.dhcp_enable = TLS_PARAM_DHCP_ENABLE;
		param->ipcfg.ip[0] = 192;
		param->ipcfg.ip[1] = 168;
		param->ipcfg.ip[2] = 1;
		param->ipcfg.ip[3] = 1;
		param->ipcfg.netmask[0] = 255;
		param->ipcfg.netmask[1] = 255;
		param->ipcfg.netmask[2] = 255;
		param->ipcfg.netmask[3] = 0;
		param->ipcfg.gateway[0] = 192;
		param->ipcfg.gateway[1] = 168;
		param->ipcfg.gateway[2] = 1;
		param->ipcfg.gateway[3] = 1;
		param->ipcfg.dns1[0] = 192;
		param->ipcfg.dns1[1] = 168;
		param->ipcfg.dns1[2] = 1;
		param->ipcfg.dns1[3] = 1;
		param->ipcfg.dns2[0] = 192;
		param->ipcfg.dns2[1] = 168;
		param->ipcfg.dns2[2] = 1;
		param->ipcfg.dns2[3] = 1;

	strcpy((char *)param->local_dnsname, "local.winnermicro");
	strcpy((char *)param->local_device_name, "w800");

	param->remote_socket_cfg.protocol = TLS_PARAM_SOCKET_TCP;
	param->remote_socket_cfg.client_or_server = TLS_PARAM_SOCKET_SERVER;
	param->remote_socket_cfg.port_num = TLS_PARAM_SOCKET_DEFAULT_PORT;
	memset(param->remote_socket_cfg.host, 0, 32);

	param->EscapeChar = 0x2b;
	param->EscapePeriod = 200;

	param->WebsCfg.AutoRun = 1;
	param->WebsCfg.PortNum = 80;

	param->debug_mode = 0;
	memset(param->PassWord, '0', 6);

    param->channel4softap = 11;
    param->encry4softap = TLS_PARAM_ENCRY_OPEN;
    param->ipcfg4softap.dhcp_enable = TLS_PARAM_DHCP_ENABLE;
	param->ipcfg4softap.ip[0] = 192;
	param->ipcfg4softap.ip[1] = 168;
	param->ipcfg4softap.ip[2] = 0;
	param->ipcfg4softap.ip[3] = 1;
	param->ipcfg4softap.netmask[0] = 255;
	param->ipcfg4softap.netmask[1] = 255;
	param->ipcfg4softap.netmask[2] = 255;
	param->ipcfg4softap.netmask[3] = 0;
	param->ipcfg4softap.gateway[0] = 192;
	param->ipcfg4softap.gateway[1] = 168;
	param->ipcfg4softap.gateway[2] = 0;
	param->ipcfg4softap.gateway[3] = 1;
	param->ipcfg4softap.dns1[0] = 192;
	param->ipcfg4softap.dns1[1] = 168;
	param->ipcfg4softap.dns1[2] = 0;
	param->ipcfg4softap.dns1[3] = 1;
	param->ipcfg4softap.dns2[0] = 0;
	param->ipcfg4softap.dns2[1] = 0;
	param->ipcfg4softap.dns2[2] = 0;
	param->ipcfg4softap.dns2[3] = 0;
#if TLS_CONFIG_SOFTAP_11N
    param->wbgr4softap.bg = TLS_PARAM_PHY_11BGN_MIXED;
    param->wbgr4softap.max_rate = TLS_PARAM_TX_RATEIDX_36M;
#else
    param->wbgr4softap.bg = TLS_PARAM_PHY_11BG_MIXED;
    param->wbgr4softap.max_rate = TLS_PARAM_TX_RATEIDX_36M;
#endif

	strcpy(param->sntp_service1, "cn.ntp.org.cn");
	strcpy(param->sntp_service2, "ntp.sjtu.edu.cn");
	strcpy(param->sntp_service3, "us.pool.ntp.org");
}

/**********************************************************************************************************
* Description: 	This function is used to set system parameter.
*
* Arguments  : 	id		param id,from TLS_PARAM_ID_SSID to (TLS_PARAM_ID_MAX - 1)
*				argc		store parameters
*				to_flash	whether the parameter is written to flash,1 write

* Returns    :		TLS_PARAM_STATUS_OK		set success
*				TLS_PARAM_STATUS_EINVALID	invalid param
**********************************************************************************************************/
int tls_param_set(int id, void *argv, bool to_flash)
{
	int err = 0;
#if USE_TWO_RAM_FOR_PARAMETER	
	struct tls_sys_param *param = &sram_param;
#else
	struct tls_sys_param *param = &flash_param.parameters;
#endif
	if ((id < TLS_PARAM_ID_ALL) || (id >= TLS_PARAM_ID_MAX) || (argv == NULL)) {return TLS_PARAM_STATUS_EINVALID;}

	if(updp_mode)
	{
		param = tls_param_user_param_init();
		if (NULL == param)
		{
			return TLS_PARAM_STATUS_EMEM;
		}
	}
	tls_os_sem_acquire(sys_param_lock, 0);

	err = TLS_PARAM_STATUS_OK;
	switch (id) {
		case TLS_PARAM_ID_ALL:
			MEMCPY(param, argv, sizeof(struct tls_sys_param));
			break;

		case TLS_PARAM_ID_SSID:
			MEMCPY(&param->ssid, argv, sizeof(struct tls_param_ssid));
			break;

		case TLS_PARAM_ID_ENCRY:
			param->encry = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_KEY:
			MEMCPY(&param->key, argv, sizeof(struct tls_param_key));
			break;

		case TLS_PARAM_ID_IP:
			MEMCPY(&param->ipcfg, argv, sizeof(struct tls_param_ip));
			break;

		case TLS_PARAM_ID_AUTOMODE:
			param->auto_mode = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_DEFSOCKET:
			MEMCPY(&param->remote_socket_cfg, argv, sizeof(struct tls_param_socket));
			break;

		case TLS_PARAM_ID_BSSID:
			MEMCPY(&param->bssid, argv, sizeof(struct tls_param_bssid));
			break;

		case TLS_PARAM_ID_CHANNEL:
			param->channel = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_CHANNEL_LIST:
			param->channellist = *((u16*)argv);
			break;

		case TLS_PARAM_ID_CHANNEL_EN:
			param->channel_enable = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_COUNTRY_REGION:
			param->wireless_region = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_WPROTOCOL:
			param->wireless_protocol = *((u8 *)argv);;
			break;

		case TLS_PARAM_ID_ADHOC_AUTOCREATE:
			param->auto_create_adhoc = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_ROAMING:
			param->auto_roam = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_AUTO_RETRY_CNT:
			param->auto_retrycnt = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_WBGR:
			MEMCPY(&param->wbgr, argv, sizeof(struct tls_param_bgr));
			break;

		case TLS_PARAM_ID_USRINTF:
			param->user_port_mode = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_AUTO_TRIGGER_LENGTH:
			param->transparent_trigger_length = *((u16 *)argv);
			break;

		case TLS_PARAM_ID_AUTO_TRIGGER_PERIOD:
			param->transparent_trigger_period = *((u16 *)argv);
			break;

		case TLS_PARAM_ID_ESCAPE_CHAR:
			param->EscapeChar = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_ESCAPE_PERIOD:
			param->EscapePeriod = *((u16 *)argv);
			break;

		case TLS_PARAM_ID_IO_MODE:
			param->IoMode = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_CMD_MODE:
			param->CmdMode = *((u8 *)argv);
			break;

	    case TLS_PARAM_ID_PASSWORD:
			MEMCPY(param->PassWord, (u8 *)argv, sizeof(param->PassWord));
			break;
		case TLS_PARAM_ID_WEBS_CONFIG:
			param->WebsCfg = *((struct tls_webs_cfg *)argv);
			break;
		case TLS_PARAM_ID_DEBUG_MODE:
			param->debug_mode = *((u32 *)argv);;
			break;

		case TLS_PARAM_ID_HARDVERSION:
			MEMCPY(&param->hardware_version, argv, sizeof(struct tls_param_hardware_version));
			break;

		case TLS_PARAM_ID_BRDSSID:
			param->ssid_broadcast_enable = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_DNSNAME:
			strcpy((char *)param->local_dnsname, (char *)argv);
			break;

		case TLS_PARAM_ID_DEVNAME:
			strcpy((char *)param->local_device_name, argv);
			break;

		case TLS_PARAM_ID_PSM:
			param->auto_powersave = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_ORAY_CLIENT:
			MEMCPY(&param->oray_client_setting, argv, sizeof(struct tls_param_oray_client));
			break;

		case TLS_PARAM_ID_UPNP:
			param->upnp_enable = *((u8 *)argv);
			break;

		case TLS_PARAM_ID_UART:
			MEMCPY(&param->uart_cfg, argv, sizeof(struct tls_param_uart));
			break;
#if TLS_CONFIG_WPS
		case TLS_PARAM_ID_WPS:
			MEMCPY(&param->wps, argv, sizeof(struct tls_param_wps));
			break;
#endif
		case TLS_PARAM_ID_ONESHOT_CFG:
			param->oneshotflag = *((u8 *)argv);
			break;
		case TLS_PARAM_ID_SHA1:
			MEMCPY(&param->psk, (u8 *)argv, sizeof(struct tls_param_sha1));

			break;
		case TLS_PARAM_ID_ORIGIN_KEY:
			MEMCPY( &param->original_key, (u8*)argv, sizeof(struct tls_param_original_key));
			break;
		case TLS_PARAM_ID_ORIGIN_SSID:
			MEMCPY( &param->original_ssid, (u8*)argv, sizeof(struct tls_param_ssid));
			break;
		case TLS_PARAM_ID_AUTO_RECONNECT:
			param->auto_reconnect = *((u8 *)argv);
			break;
        case TLS_PARAM_ID_QUICK_CONNECT:
            MEMCPY(&param->quick_connect, argv, sizeof(struct tls_param_quick_connect));
            break;
        case TLS_PARAM_ID_KEY_CHANGE:
            param->key_changed = *((u8 *)argv);
            break;
        case TLS_PARAM_ID_SSID_CHANGE:
            param->ssid_changed = *((u8 *)argv);
            break;
#if TLS_CONFIG_AP
        case TLS_PARAM_ID_SOFTAP_SSID:
            MEMCPY(&param->apsta_ssid, argv, sizeof(struct tls_param_ssid));
            break;
        case TLS_PARAM_ID_SOFTAP_PSK:
            MEMCPY(&param->apsta_psk, argv, sizeof(struct tls_param_sha1));
            break;
        case TLS_PARAM_ID_SOFTAP_ENCRY:
            param->encry4softap = *((u8 *)argv);
            break;
        case TLS_PARAM_ID_SOFTAP_KEY:
            MEMCPY(&param->key4softap, argv, sizeof(struct tls_param_key));
            break;
        case TLS_PARAM_ID_SOFTAP_IP:
            MEMCPY(&param->ipcfg4softap, argv, sizeof(struct tls_param_ip));
            break;
        case TLS_PARAM_ID_SOFTAP_CHANNEL:
            param->channel4softap = *((u8 *)argv);
            break;
        case TLS_PARAM_ID_SOFTAP_WBGR:
			MEMCPY(&param->wbgr4softap, argv, sizeof(struct tls_param_bgr));
            break;
#endif
		case TLS_PARAM_ID_SNTP_SERVER1:
			strncpy(param->sntp_service1, (const char *)argv, strlen(argv)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER2:
			strncpy(param->sntp_service2, (const char *)argv, strlen(argv)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER3:
			strncpy(param->sntp_service3, (const char *)argv, strlen(argv)+1);
			break;
        case TLS_PARAM_ID_TEM_OFFSET:
            MEMCPY(&param->params_tem, argv, sizeof(struct tls_param_tem_offset));
			break;


		case TLS_PARAM_ID_BT_ADAPTER:
			MEMCPY(&param->adapter_t, argv,  sizeof(bt_adapter_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_1:
			MEMCPY(&param->remote_device1, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_2:
			MEMCPY(&param->remote_device2, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_3:
			MEMCPY(&param->remote_device3, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_4:
			MEMCPY(&param->remote_device4, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_5:
			MEMCPY(&param->remote_device5, argv, sizeof(bt_remote_device_t));
			break;
#if 0

		case TLS_PARAM_ID_BT_REMOTE_DEVICE_6:
			MEMCPY(&param->remote_device6, argv, sizeof(bt_remote_device_t));
			break;
		
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_7:
			MEMCPY(&param->remote_device7, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_8:
			MEMCPY(&param->remote_device8, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_9:
			MEMCPY(&param->remote_device9, argv, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_10:
			MEMCPY(&param->remote_device10, argv, sizeof(bt_remote_device_t));
			break;
#endif

		default:
			TLS_DBGPRT_WARNING("invalid parameter id - %d!\n", id);
			err = TLS_PARAM_STATUS_EINVALIDID;
			goto exit;
	}

	if (to_flash && !updp_mode) {
		err = param_to_flash(id, -1, -1);
		TLS_DBGPRT_INFO("write the parameter to spi flash - %d.\n", err);
	}
exit:
	tls_os_sem_release(sys_param_lock);

	return err;
}
/**********************************************************************************************************
* Description: 	This function is used to get bt param address offset in system param area.
*
* Arguments  : 	id		param id,from TLS_PARAM_ID_BT_REMOTE_DEVICE_1 to TLS_PARAM_ID_BT_REMOTE_DEVICE_5
*				from_flash	whether the parameter is readed from flash,1 read from flash(invalid for now), 0 read from ram

* Returns    :		address offset(>0) in system param area	success
*				-1	invalid param
**********************************************************************************************************/

int tls_param_get_bt_param_address(int id, int from_flash)
{
    int addr_offset = 0;
	struct tls_sys_param *src = NULL;

	if ((id < TLS_PARAM_ID_ALL) || (id >= TLS_PARAM_ID_MAX)) {return TLS_PARAM_STATUS_EINVALID;}  

    if(from_flash)
    {
        /*!!unsupport for now!!*/
        return -1;
    }

    src = &flash_param.parameters;
    tls_os_sem_acquire(sys_param_lock, 0);
    switch (id) {

	case TLS_PARAM_ID_BT_REMOTE_DEVICE_1:
        addr_offset = (int)&src->remote_device1;
		break;
	case TLS_PARAM_ID_BT_REMOTE_DEVICE_2:
		addr_offset = (int)&src->remote_device2;
		break;
	case TLS_PARAM_ID_BT_REMOTE_DEVICE_3:
		addr_offset = (int)&src->remote_device3;
		break;
	case TLS_PARAM_ID_BT_REMOTE_DEVICE_4:
		addr_offset = (int)&src->remote_device4;
		break;
	case TLS_PARAM_ID_BT_REMOTE_DEVICE_5:
		addr_offset = (int)&src->remote_device5;
		break;
    }
    
    tls_os_sem_release(sys_param_lock);

    return addr_offset;
}

/**********************************************************************************************************
* Description: 	This function is used to get system parameter.
*
* Arguments  : 	id		param id,from TLS_PARAM_ID_SSID to (TLS_PARAM_ID_MAX - 1)
*				argc		store parameters
*				from_flash	whether the parameter is readed from flash,1 read from flash

* Returns    :		TLS_PARAM_STATUS_OK	success
*				TLS_PARAM_STATUS_EINVALID	invalid param
**********************************************************************************************************/
int tls_param_get(int id, void *argv, bool from_flash)
{
	int err;
	struct tls_sys_param *src = NULL;

	if ((id < TLS_PARAM_ID_ALL) || (id >= TLS_PARAM_ID_MAX)) {return TLS_PARAM_STATUS_EINVALID;}

	err = TLS_PARAM_STATUS_OK;
#if USE_TWO_RAM_FOR_PARAMETER	
	if (from_flash) {src = &flash_param.parameters;}
	else {src = &sram_param;}
#else
	struct tls_param_flash *curflashparm = NULL;
	if (from_flash)
	{
		curflashparm = tls_mem_alloc(sizeof(struct tls_param_flash));
		if (curflashparm)
		{
			if (TLS_FLS_STATUS_OK == tls_fls_read((flash_param.partition_num == 0) ? TLS_FLASH_PARAM1_ADDR : TLS_FLASH_PARAM2_ADDR, (u8 *)curflashparm, sizeof(struct tls_param_flash)))
			{
				src = &curflashparm->parameters;
			}
		}
	}

	if (NULL == src)
	{
		src = &flash_param.parameters;
	}
#endif

	tls_os_sem_acquire(sys_param_lock, 0);

	switch (id) {
		case TLS_PARAM_ID_ALL:
			MEMCPY(argv, src, sizeof(struct tls_sys_param));
			break;

		case TLS_PARAM_ID_SSID:
			MEMCPY(argv, &src->ssid, sizeof(struct tls_param_ssid));
			break;

		case TLS_PARAM_ID_ENCRY:
			*((u8 *)argv) = src->encry;
			break;

		case TLS_PARAM_ID_KEY:
			MEMCPY(argv, &src->key, sizeof(struct tls_param_key));
			break;

		case TLS_PARAM_ID_IP:
			MEMCPY(argv, &src->ipcfg, sizeof(struct tls_param_ip));
			break;

		case TLS_PARAM_ID_AUTOMODE:
			*((u8 *)argv) = src->auto_mode;
			break;

		case TLS_PARAM_ID_DEFSOCKET:
			MEMCPY(argv, &src->remote_socket_cfg, sizeof(struct tls_param_socket));
			break;

		case TLS_PARAM_ID_BSSID:
			MEMCPY(argv, &src->bssid, sizeof(struct tls_param_bssid));
			break;

		case TLS_PARAM_ID_CHANNEL:
			*((u8 *)argv) = src->channel;
			break;
		case TLS_PARAM_ID_CHANNEL_LIST:
			*((u16*)argv) = src->channellist;
			break;

		case TLS_PARAM_ID_CHANNEL_EN:
			*((u8 *)argv) = src->channel_enable;
			break;

		case TLS_PARAM_ID_COUNTRY_REGION:
			*((u8 *)argv) = src->wireless_region;
			break;

		case TLS_PARAM_ID_WPROTOCOL:
			*((u8 *)argv) = src->wireless_protocol;
			break;

		case TLS_PARAM_ID_ADHOC_AUTOCREATE:
			*((u8 *)argv) = src->auto_create_adhoc;
			break;

		case TLS_PARAM_ID_ROAMING:
			*((u8 *)argv) = src->auto_roam;
			break;

		case TLS_PARAM_ID_AUTO_RETRY_CNT:
			*((u8 *)argv) = src->auto_retrycnt;
			break;

		case TLS_PARAM_ID_WBGR:
			MEMCPY(argv, &src->wbgr, sizeof(struct tls_param_bgr));
			break;

		case TLS_PARAM_ID_USRINTF:
			*((u8 *)argv) = src->user_port_mode;
			break;

		case TLS_PARAM_ID_AUTO_TRIGGER_LENGTH:
			*((u16 *)argv) = src->transparent_trigger_length;
			break;
		case TLS_PARAM_ID_AUTO_TRIGGER_PERIOD:
			*((u16 *)argv) = src->transparent_trigger_period;
			break;

		case TLS_PARAM_ID_ESCAPE_CHAR:
			*((u8 *)argv) = src->EscapeChar;
			break;

		case TLS_PARAM_ID_ESCAPE_PERIOD:
			*((u16 *)argv) = src->EscapePeriod;
			break;
		case TLS_PARAM_ID_IO_MODE:
			*((u8 *)argv) = src->IoMode;
			break;
		case TLS_PARAM_ID_CMD_MODE:
			*((u8 *)argv) = src->CmdMode;
			break;

		case TLS_PARAM_ID_PASSWORD:
			MEMCPY((u8 *)argv, src->PassWord, sizeof(src->PassWord));
			break;
		case TLS_PARAM_ID_WEBS_CONFIG:
			*((struct tls_webs_cfg *)argv) = src->WebsCfg;
			break;


		case TLS_PARAM_ID_DEBUG_MODE:
			*((u32 *)argv) = src->debug_mode;
			break;

		case TLS_PARAM_ID_HARDVERSION:
			MEMCPY(argv, &src->hardware_version, sizeof(struct tls_param_hardware_version));
			break;

		case TLS_PARAM_ID_BRDSSID:
			*((u8 *)argv) = src->ssid_broadcast_enable;
			break;

		case TLS_PARAM_ID_DNSNAME:
			strcpy((char *)argv, (char *)src->local_dnsname);
			break;

		case TLS_PARAM_ID_DEVNAME:
			strcpy((char *)argv, (char *)src->local_device_name);
			break;

		case TLS_PARAM_ID_PSM:
			*((u8 *)argv) = src->auto_powersave;
			break;

		case TLS_PARAM_ID_ORAY_CLIENT:
			MEMCPY(argv, &src->oray_client_setting, sizeof(struct tls_param_oray_client));
			break;

		case TLS_PARAM_ID_UPNP:
			*((u8 *)argv) = src->upnp_enable;
			break;

		case TLS_PARAM_ID_UART:
			MEMCPY(argv, &src->uart_cfg, sizeof(struct tls_param_uart));
			break;
#if TLS_CONFIG_WPS
		case TLS_PARAM_ID_WPS:
			MEMCPY(argv, &src->wps, sizeof(struct tls_param_wps));
			break;
#endif
		case TLS_PARAM_ID_ONESHOT_CFG:
			*((u8 *)argv) = src->oneshotflag;
			break;
		case TLS_PARAM_ID_SHA1:
			MEMCPY(argv, &src->psk, sizeof(struct tls_param_sha1));

			break;
		case TLS_PARAM_ID_ORIGIN_KEY:
			MEMCPY(argv, &src->original_key, sizeof(struct tls_param_original_key));
			break;
		case TLS_PARAM_ID_ORIGIN_SSID:
			MEMCPY(argv, &src->original_ssid, sizeof(struct tls_param_ssid));
			break;
		case TLS_PARAM_ID_AUTO_RECONNECT:
			*((u8 *)argv) = src->auto_reconnect;
			break;
        case TLS_PARAM_ID_QUICK_CONNECT:
            MEMCPY(argv, &src->quick_connect, sizeof(struct tls_param_quick_connect));
            break;
        case TLS_PARAM_ID_KEY_CHANGE:
            *((u8 *)argv) = src->key_changed;
            break;
        case TLS_PARAM_ID_SSID_CHANGE:
            *((u8 *)argv) = src->ssid_changed;
            break;
#if TLS_CONFIG_AP
        case TLS_PARAM_ID_SOFTAP_SSID:
            MEMCPY(argv, &src->apsta_ssid, sizeof(struct tls_param_ssid));
            break;
        case TLS_PARAM_ID_SOFTAP_PSK:
            MEMCPY(argv, &src->apsta_psk, sizeof(struct tls_param_sha1));
            break;
        case TLS_PARAM_ID_SOFTAP_ENCRY:
            *((u8 *)argv) = src->encry4softap;
			break;
        case TLS_PARAM_ID_SOFTAP_KEY:
            MEMCPY(argv, &src->key4softap, sizeof(struct tls_param_key));
            break;
        case TLS_PARAM_ID_SOFTAP_IP:
            MEMCPY(argv, &src->ipcfg4softap, sizeof(struct tls_param_ip));
            break;
        case TLS_PARAM_ID_SOFTAP_CHANNEL:
            *((u8 *)argv) = src->channel4softap;
            break;
        case TLS_PARAM_ID_SOFTAP_WBGR:
			MEMCPY(argv, &src->wbgr4softap, sizeof(struct tls_param_bgr));
            break;
#endif

		case TLS_PARAM_ID_SNTP_SERVER1:
			strncpy(argv, src->sntp_service1, strlen(src->sntp_service1)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER2:
			strncpy(argv, src->sntp_service2, strlen(src->sntp_service2)+1);
			break;
		case TLS_PARAM_ID_SNTP_SERVER3:
			strncpy(argv, src->sntp_service3, strlen(src->sntp_service3)+1);
			break;
        case TLS_PARAM_ID_TEM_OFFSET:
            MEMCPY(argv, &src->params_tem, sizeof(struct tls_param_tem_offset));
			break;
			
		case TLS_PARAM_ID_BT_ADAPTER:
			MEMCPY(argv, &src->adapter_t,   sizeof(bt_adapter_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_1:
			MEMCPY(argv,&src->remote_device1,  sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_2:
			MEMCPY(argv,&src->remote_device2, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_3:
			MEMCPY(argv,&src->remote_device3, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_4:
			MEMCPY(argv,&src->remote_device4, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_5:
			MEMCPY(argv,&src->remote_device5, sizeof(bt_remote_device_t));
			break;
#if 0

		case TLS_PARAM_ID_BT_REMOTE_DEVICE_6:
			MEMCPY(argv,&src->remote_device6, sizeof(bt_remote_device_t));
			break;
			
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_7:
			MEMCPY(argv,&src->remote_device7, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_8:
			MEMCPY(argv,&src->remote_device8, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_9:
			MEMCPY(argv,&src->remote_device9, sizeof(bt_remote_device_t));
			break;
		case TLS_PARAM_ID_BT_REMOTE_DEVICE_10:
			MEMCPY(argv,&src->remote_device10, sizeof(bt_remote_device_t));
			break;
#endif

		default:
			TLS_DBGPRT_WARNING("invalid parameter id - %d!\n", id);
			err = TLS_PARAM_STATUS_EINVALIDID;
			break;
	}
#if USE_TWO_RAM_FOR_PARAMETER
#else
	if (curflashparm)
	{
		tls_mem_free(curflashparm);
	}
#endif

	tls_os_sem_release(sys_param_lock);

	return err;
}

/**********************************************************************************************************
* Description: 	This function is used to write parameter to flash.
*
* Arguments  : 	id		param id,from TLS_PARAM_ID_ALL to (TLS_PARAM_ID_MAX - 1)
*
* Returns    :		* Returns    :		TLS_PARAM_STATUS_OK	success
*				TLS_PARAM_STATUS_EINVALID	invalid param
*				TLS_PARAM_STATUS_EIO		error
**********************************************************************************************************/
int tls_param_to_flash(int id)
{
	int err = 0;

	tls_os_sem_acquire(sys_param_lock, 0);
#if USE_TWO_RAM_FOR_PARAMETER
	if (TLS_PARAM_ID_ALL == id)
	{
		if (0 == memcmp(&sram_param, &flash_param.parameters,sizeof(sram_param)))
		{
			tls_os_sem_release(sys_param_lock);
			return TLS_FLS_STATUS_OK;
		}
	}
	err = param_to_flash(id, -1, -1);	
#else
	if (TLS_PARAM_ID_ALL == id)
	{
		struct tls_param_flash *sram_param;
		sram_param = tls_mem_alloc(sizeof(struct tls_param_flash));
		if (sram_param)
		{
			if (TLS_FLS_STATUS_OK != tls_fls_read((flash_param.partition_num == 0) ? TLS_FLASH_PARAM1_ADDR : TLS_FLASH_PARAM2_ADDR, (u8 *)sram_param, sizeof(struct tls_param_flash)))
			{
				/*write anyway!!!*/
			}
			else
			{	/*if not same, write to flash*/
				if (0 == memcmp(&sram_param->parameters, &flash_param.parameters,sizeof(struct tls_sys_param)))
				{
					tls_mem_free(sram_param);
					tls_os_sem_release(sys_param_lock);
					return TLS_FLS_STATUS_OK;
				}
			}
			tls_mem_free(sram_param);
		}
	}

	err = param_to_flash(id, -1, -1);
#endif
	tls_os_sem_release(sys_param_lock);

	return err;
}


/**********************************************************************************************************
* Description: 	This function is used to load default parametes to memory.
*
* Arguments  :
*
* Returns    :
*
* Notes		:	This function read user defined parameters first, if wrong, all the parameters restore factory settings.
**********************************************************************************************************/
int tls_param_to_default(void)
{
	int err = 0;
	tls_param_load_factory_default();
	err = param_to_flash(TLS_PARAM_ID_ALL, 1, 0);
	if(err)
		return err;
	err = param_to_flash(TLS_PARAM_ID_ALL, 1, 1);
	flash_param.magic = 0;

	return err;
}

struct tls_sys_param * tls_param_user_param_init(void)
{
	if (NULL == user_default_param)
	{
		user_default_param = tls_mem_alloc(sizeof(*user_default_param));
		if (user_default_param)
		memset(user_default_param, 0, sizeof(*user_default_param));
	}

	return user_default_param;
}

/**********************************************************************************************************
* Description: 	This function is used to modify user default parameters,then write to flash.
*
* Arguments  :
*
* Returns    :
**********************************************************************************************************/
int tls_param_save_user(struct tls_user_param *user_param)
{
	struct tls_sys_param *param = NULL;

	param = tls_param_user_param_init();
	if (NULL == param)
	{
		return TLS_PARAM_STATUS_EMEM;
	}

	param->wireless_protocol = user_param->wireless_protocol;
	param->auto_mode = user_param->auto_mode;
	param->uart_cfg.baudrate = user_param->baudrate;
	param->user_port_mode = user_param->user_port_mode;
	param->ipcfg.dhcp_enable = user_param->dhcp_enable;
	param->auto_powersave = user_param->auto_powersave;

	MEMCPY(param->ipcfg.ip, user_param->ip, 4);
	MEMCPY(param->ipcfg.netmask, user_param->netmask, 4);
	MEMCPY(param->ipcfg.gateway, user_param->gateway, 4);
	MEMCPY(param->ipcfg.dns1, user_param->dns, 4);
	MEMCPY(param->ipcfg.dns2, user_param->dns, 4);
	param->remote_socket_cfg.protocol = user_param->socket_protocol;
	param->remote_socket_cfg.client_or_server = user_param->socket_client_or_server;
	param->remote_socket_cfg.port_num = user_param->socket_port_num;
	MEMCPY(param->remote_socket_cfg.host, user_param->socket_host, 32);
	MEMCPY(param->PassWord, user_param->PassWord, 6);

	MEMCPY(&param->hardware_version, factory_default_hardware, 8);
	param->wireless_region = TLS_PARAM_REGION_1_BG_BAND;
	param->channel = 1;
	param->channellist = 0x3FFF;
	param->wbgr.bg = TLS_PARAM_PHY_11BG_MIXED;
	param->wbgr.max_rate = TLS_PARAM_TX_RATEIDX_36M;
	param->ssid_broadcast_enable = TLS_PARAM_SSIDBRD_ENABLE;
	param->encry = TLS_PARAM_ENCRY_OPEN;

	param->auto_retrycnt = 255;
	param->auto_roam = TLS_PARAM_ROAM_DISABLE;
	//param->wps.wps_enable = TLS_PARAM_WPS_DISABLE;

	param->transparent_trigger_length = 512;

	param->uart_cfg.stop_bits = TLS_PARAM_UART_STOPBITS_1BITS;
	param->uart_cfg.parity = TLS_PARAM_UART_PARITY_NONE;

	strcpy((char *)param->local_dnsname, "local.winnermicro");
	strcpy((char *)param->local_device_name, "w800");

	param->EscapeChar = 0x2b;
	param->EscapePeriod = 200;

	param->debug_mode = 0;

	tls_param_save_user_default();

	return TLS_PARAM_STATUS_OK;
}

/**********************************************************************************************************
* Description: 	This function is used to save user parameters to the flash.
*
* Arguments  :
*
* Returns    :
**********************************************************************************************************/
int tls_param_save_user_default(void)
{
	u32 magic, crc32, offset;
	if (NULL == user_default_param)
	{
		return TLS_PARAM_STATUS_EMEM;
	}

	offset = TLS_FLASH_PARAM_DEFAULT;
	magic = TLS_USER_MAGIC;
	TLS_DBGPRT_INFO("=====>\n");
	tls_fls_write(offset, (u8 *)&magic, 4);
	offset += 4;
	tls_fls_write(offset, (u8 *)user_default_param, sizeof(struct tls_sys_param));
	offset += sizeof(struct tls_sys_param);
	crc32 = get_crc32((u8 *)user_default_param, sizeof(struct tls_sys_param));
	tls_fls_write(offset, (u8 *)&crc32, 4);

	return TLS_PARAM_STATUS_OK;
}

u8 tls_param_get_updp_mode(void)
{
    return updp_mode;
}

void tls_param_set_updp_mode(u8 mode)
{
    updp_mode = mode;
}

