#include "include.h"
#include "sys_rtos.h"
#include "role_launch.h"
#include "mem_pub.h"
#include "str_pub.h"
#include "rtos_pub.h"
#include "rtos_error.h"
#include "wlan_ui_pub.h"
#include "ieee802_11_defs.h"

#if RL_SUPPORT_FAST_CONNECT
#include "drv_model_pub.h"
#include "flash_pub.h"
#include "param_config.h"
#endif

#if (FAST_CONNECT_INFO_ENC_METHOD != ENC_METHOD_NULL)
#include "soft_encrypt.h"
#endif

#if CFG_ROLE_LAUNCH
RL_T g_role_launch = {{0}};
RL_SOCKET_T g_rl_socket = {0};
RL_SOCKET_CACHE_T g_sta_cache = {0};

extern u8* wpas_get_sta_psk(void);

#if RL_SUPPORT_FAST_CONNECT
extern void user_connected_callback(FUNCPTR fn);
#endif

#if (FAST_CONNECT_INFO_ENC_METHOD == ENC_METHOD_XOR)
void bssid_info_enc(RL_BSSID_INFO_PTR bssid_info)
{
	RL_BSSID_INFO_T tmp_out;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	xor_enc(bssid_info, &tmp_out, sizeof(tmp_out));
	os_memcpy(bssid_info, &tmp_out, sizeof(tmp_out));
	GLOBAL_INT_RESTORE();
}

void bssid_info_dec(RL_BSSID_INFO_PTR bssid_info)
{
	RL_BSSID_INFO_T tmp_out;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	xor_dec(bssid_info, &tmp_out, sizeof(tmp_out));
	os_memcpy(bssid_info, &tmp_out, sizeof(tmp_out));
	GLOBAL_INT_RESTORE();
}
#elif (FAST_CONNECT_INFO_ENC_METHOD == ENC_METHOD_AES)
void bssid_info_enc(RL_BSSID_INFO_PTR bssid_info)
{
	RL_BSSID_INFO_T tmp_out;
    weeny_aes_context ctx;
    uint8_t iv[BK_AES_IV_LEN + 1];
    uint8_t private_key[BK_AES_KEY_LEN + 1];
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
    memcpy(iv, BK_TINY_AES_IV, strlen(BK_TINY_AES_IV));
    iv[sizeof(iv) - 1] = '\0';
    memcpy(private_key, BK_TINY_AES_KEY, strlen(BK_TINY_AES_KEY));
    private_key[sizeof(private_key) - 1] = '\0';

    memset(&tmp_out, 0x0, sizeof(tmp_out));
    weeny_aes_setkey_enc(&ctx, (uint8_t *) private_key, 256);
    weeny_aes_crypt_cbc(&ctx, AES_ENCRYPT, sizeof(tmp_out), iv, (unsigned char *)bssid_info, (unsigned char *)&tmp_out);
	os_memcpy(bssid_info, &tmp_out, sizeof(tmp_out));
	GLOBAL_INT_RESTORE();
}

void bssid_info_dec(RL_BSSID_INFO_PTR bssid_info)
{
	RL_BSSID_INFO_T tmp_out;
	weeny_aes_context ctx;
	uint8_t iv[BK_AES_IV_LEN + 1];
	uint8_t private_key[BK_AES_KEY_LEN + 1];
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	memcpy(iv, BK_TINY_AES_IV, strlen(BK_TINY_AES_IV));
	iv[sizeof(iv) - 1] = '\0';
	memcpy(private_key, BK_TINY_AES_KEY, strlen(BK_TINY_AES_KEY));
	private_key[sizeof(private_key) - 1] = '\0';

	memset(&tmp_out, 0x0, sizeof(tmp_out));
	weeny_aes_setkey_dec(&ctx, (uint8_t *) private_key, 256);
	weeny_aes_crypt_cbc(&ctx, AES_DECRYPT, sizeof(tmp_out), iv, (unsigned char *)bssid_info, (unsigned char *)&tmp_out);
	os_memcpy(bssid_info, &tmp_out, sizeof(tmp_out));
	GLOBAL_INT_RESTORE();
}
#endif

#if RL_SUPPORT_FAST_CONNECT
#define BSSID_INFO_ADDR 0x1E8000 /*reserve 4k for bssid info*/

void rl_read_bssid_info(RL_BSSID_INFO_PTR bssid_info)
{
	UINT32 status, addr,ret;
	DD_HANDLE flash_hdl;
	
	flash_hdl = ddev_open(FLASH_DEV_NAME,&status,0);
	if(DD_HANDLE_UNVALID == flash_hdl)
	{
		os_null_printf("DD_HANDLE_UNVALID\r\n");
		return;
	}
	
	addr = BSSID_INFO_ADDR;
	ret = ddev_read(flash_hdl, (char *)bssid_info, sizeof(RL_BSSID_INFO_T), addr);
	if(DRV_FAILURE == ret)
	{
		os_null_printf("DRV_FAILURE\r\n");
	}

#if (FAST_CONNECT_INFO_ENC_METHOD != ENC_METHOD_NULL)
	bssid_info_dec(bssid_info);
#endif
	
	ddev_close(flash_hdl);
}

static void rl_write_bssid_info(void)
{
	int i;
	uint8_t *psk;
	UINT32 status;
	uint32_t addr;
	RL_BSSID_INFO_T bssid_info, pre_bssid;
	LinkStatusTypeDef link_status;
	uint8_t protect_flag, protect_param, temp[4];
	DD_HANDLE flash_hdl;
	uint32_t ssid_len;

	os_memset(&link_status, 0, sizeof(link_status));
	bk_wlan_get_link_status(&link_status);
	os_memset(&bssid_info, 0, sizeof(bssid_info));

	ssid_len = os_strlen((const char*)link_status.ssid);
	if(ssid_len > SSID_MAX_LEN)
	{
		ssid_len = SSID_MAX_LEN;
	}
	os_strncpy((char*)bssid_info.ssid, (char*)link_status.ssid, ssid_len);
	os_memcpy(bssid_info.bssid, link_status.bssid, 6);
	bssid_info.security = link_status.security;
	bssid_info.channel = link_status.channel;

	psk = wpas_get_sta_psk();
	os_memset(temp, 0, sizeof(temp));
	for(i = 0; i < 32; i++)
	{
		sprintf((char*)temp, "%02x", psk[i]);
		strcat((char*)bssid_info.psk, (char*)temp);
	}
	if (g_sta_param_ptr)
	{
		os_strcpy((char*)bssid_info.pwd, (char *)g_sta_param_ptr->orig_key);
	}

	/*obtain the previous bssid-info*/
	rl_read_bssid_info(&pre_bssid);

	/*if different, save the latest information about fast connect*/
	if(0 == os_memcmp(&pre_bssid, &bssid_info, sizeof(bssid_info)))
	{
		bk_printf("same_bssid_info\r\n");
		goto wr_exit;
	}

	flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
	ddev_control(flash_hdl, CMD_FLASH_GET_PROTECT, &protect_flag);
	protect_param = FLASH_PROTECT_NONE;
	ddev_control(flash_hdl, CMD_FLASH_SET_PROTECT, (void *)&protect_param);
	addr = BSSID_INFO_ADDR;
	ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&addr);
	
#if (FAST_CONNECT_INFO_ENC_METHOD != ENC_METHOD_NULL)
	bssid_info_enc(&bssid_info);
#endif
	
	ddev_write(flash_hdl, (char*)&bssid_info, sizeof(bssid_info), addr);
	ddev_control(flash_hdl, CMD_FLASH_SET_PROTECT, (void *)&protect_flag);
	ddev_close(flash_hdl);

wr_exit:
	return;
}

static void rl_sta_fast_connect(RL_BSSID_INFO_PTR bssid_info)
{
	network_InitTypeDef_adv_st inNetworkInitParaAdv;

	os_memset(&inNetworkInitParaAdv, 0, sizeof(inNetworkInitParaAdv));
	
	os_strcpy((char*)inNetworkInitParaAdv.ap_info.ssid, (char*)bssid_info->ssid);
	os_memcpy(inNetworkInitParaAdv.ap_info.bssid, bssid_info->bssid, 6);
	inNetworkInitParaAdv.ap_info.security = bssid_info->security;
	inNetworkInitParaAdv.ap_info.channel = bssid_info->channel;
	
	if(bssid_info->security < SECURITY_TYPE_WPA_TKIP)
	{
		os_strcpy((char*)inNetworkInitParaAdv.key, (char*)bssid_info->pwd);
		inNetworkInitParaAdv.key_len = os_strlen((char*)bssid_info->pwd);
	}
	else
	{
		os_strcpy((char*)inNetworkInitParaAdv.key, (char*)bssid_info->psk);
		inNetworkInitParaAdv.key_len = os_strlen((char*)bssid_info->psk);
	}
	inNetworkInitParaAdv.dhcp_mode = DHCP_CLIENT;

	bk_wlan_start_sta_adv(&inNetworkInitParaAdv);
}

void rl_clear_bssid_info(void)
{
	DD_HANDLE flash_hdl;
	UINT32 status;
	uint32_t addr;
	uint8_t protect_flag, protect_param;

	/* clear fast connect flag when clear bssid info in flash */
	g_sta_param_ptr->fast_connect_set = 0;
	flash_hdl = ddev_open(FLASH_DEV_NAME, &status, 0);
	ddev_control(flash_hdl, CMD_FLASH_GET_PROTECT, &protect_flag);
	protect_param = FLASH_PROTECT_NONE;
	ddev_control(flash_hdl, CMD_FLASH_SET_PROTECT, (void *)&protect_param);
	addr = BSSID_INFO_ADDR;
	ddev_control(flash_hdl, CMD_FLASH_ERASE_SECTOR, (void *)&addr);
	ddev_control(flash_hdl, CMD_FLASH_SET_PROTECT, (void *)&protect_flag);
	ddev_close(flash_hdl);
}
#endif

uint32_t rl_launch_sta(void)
{
    uint32_t next_launch_flag = 0;
    RL_ENTITY_T *entity, *pre_entity;
    uint32_t ret = LAUNCH_STATUS_OVER;
    
    if(NULL == g_role_launch.jl_previous_sta)
    {
        if(g_role_launch.jl_following_sta)
        {
            pre_entity = g_role_launch.jl_following_sta;
            g_role_launch.jl_previous_sta = pre_entity;
            g_role_launch.jl_following_sta = NULL;

			rl_sta_request_start(&pre_entity->rlaunch);
			
            ret = LAUNCH_STATUS_CONT;
        }
    }
    else
    {
        if(g_role_launch.jl_following_sta)
        {
            entity = g_role_launch.jl_following_sta;
            if(LAUNCH_TYPE_ASAP == entity->launch_type)
            {
                rl_pre_sta_set_cancel();
            }
        }
        
        pre_entity = g_role_launch.jl_previous_sta;
        if(pre_entity->relaunch_limit 
                && (pre_entity->launch_count >= pre_entity->relaunch_limit))
        {
            rl_pre_sta_set_cancel();
        }

        next_launch_flag = rl_sta_may_next_launch();
        if(next_launch_flag)
        {
            rl_pre_sta_stop_launch();
            
            pre_entity = g_role_launch.jl_previous_sta;
            rl_free_entity(pre_entity);
                
            pre_entity = g_role_launch.jl_following_sta;
            g_role_launch.jl_previous_sta = pre_entity;
            g_role_launch.jl_following_sta = NULL;

            if(pre_entity)
            {   
				rl_sta_request_start(&pre_entity->rlaunch);
                
                ret = LAUNCH_STATUS_CONT;
            }
        }
        else
        {
            ret = LAUNCH_STATUS_CONT;
        }
    }
    
    return ret;
}

uint32_t rl_launch_ap(void)
{
    uint32_t cancel_pre_flag = 0;
    uint32_t next_launch_flag = 0;
    RL_ENTITY_T *entity, *pre_entity;
    uint32_t ret = LAUNCH_STATUS_OVER;
    
    if(NULL == g_role_launch.jl_previous_ap)
    {
        if(g_role_launch.jl_following_ap)
        {
            pre_entity = g_role_launch.jl_following_ap;
            g_role_launch.jl_previous_ap = pre_entity;
            g_role_launch.jl_following_ap = NULL;

            rl_pre_ap_init();
            rl_ap_request_start(&pre_entity->rlaunch);
            
            ret = LAUNCH_STATUS_CONT;
        }
    }
    else
    {
        if(g_role_launch.jl_following_ap)
        {
            entity = g_role_launch.jl_following_ap;
            if(LAUNCH_TYPE_ASAP == entity->launch_type)
            {
                cancel_pre_flag = 1;
                rl_pre_ap_set_cancel();
            }
        }
        
        pre_entity = g_role_launch.jl_previous_ap;
        if(pre_entity->relaunch_limit 
                && (pre_entity->launch_count >= pre_entity->relaunch_limit))
        {
            cancel_pre_flag = 1;
            rl_pre_ap_set_cancel();
        }

        next_launch_flag = rl_ap_may_next_launch(cancel_pre_flag);
        if(next_launch_flag)
        {
            rl_pre_ap_stop_launch();
            
            pre_entity = g_role_launch.jl_previous_ap;
            rl_free_entity(pre_entity);
                
            pre_entity = g_role_launch.jl_following_ap;
            g_role_launch.jl_previous_ap = pre_entity;
            g_role_launch.jl_following_ap = NULL;

            if(pre_entity)
            {
                rl_pre_ap_init();
                rl_ap_request_start(&pre_entity->rlaunch);
                
                ret = LAUNCH_STATUS_CONT;
            }
        }
        else
        {
            ret = LAUNCH_STATUS_CONT;
        }
    }
    
    return ret;
}

uint32_t rl_relaunch_chance(void)
{
	ASSERT(rtos_is_oneshot_timer_init(&g_role_launch.rl_timer));
    
	rtos_oneshot_reload_timer(&g_role_launch.rl_timer);

	return 0;
}

uint32_t rl_sta_req_is_null(void)
{
	uint32_t ret = 0;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	if((0 == g_role_launch.jl_previous_sta)
        && (0 == g_role_launch.jl_following_sta))
	{
		ret = 1;
	}
	GLOBAL_INT_RESTORE();
	
	return ret;
}

uint32_t _sta_request_enter(LAUNCH_REQ *param, FUNC_1PARAM_PTR completion)
{
	uint32_t ret = 0;
    RL_ENTITY_T *entity = 0;
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    if((0 == g_role_launch.jl_previous_ap)
        && (0 == g_role_launch.jl_previous_sta))
    {
        entity = rl_alloc_entity(param, completion);

        g_role_launch.jl_previous_sta = entity;
        
        JL_PRT("rl_sta_start\r\n");
        rl_sta_request_start(param);

		ret = 1;
    }
    else if(0 == g_role_launch.jl_following_sta)
    {
        entity = rl_alloc_entity(param, completion);
        
        g_role_launch.jl_following_sta = entity;
        rl_start();
		ret = 2;
    }
	else
	{
		os_printf("cmd queue fill:%d\n", rl_pre_sta_get_status());
        rl_start();
	}

    if(entity
        && (PRE_ENTITY_IDLE == g_role_launch.pre_entity_type))
    {
        g_role_launch.pre_entity_type = PRE_ENTITY_STA;
    }
    GLOBAL_INT_RESTORE();

	return ret;
}

uint32_t _ap_request_enter(LAUNCH_REQ *param, FUNC_1PARAM_PTR completion)
{
	uint32_t ret = 0;
    RL_ENTITY_T *entity = 0;
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    if((0 == g_role_launch.jl_previous_ap)
        && (0 == g_role_launch.jl_previous_sta))
    {
        entity = rl_alloc_entity(param, completion);
        
        g_role_launch.jl_previous_ap = entity;
        
        JL_PRT("rl_ap_start\r\n");
        rl_ap_request_start(param);
		
		ret = 1;
    }
    else if(0 == g_role_launch.jl_following_ap)
    {
        entity = rl_alloc_entity(param, completion);
        
        g_role_launch.jl_following_ap = entity;
        rl_start();

		ret = 2;
    }
	else
	{
		os_printf("cmd queue fil2!\n");
        rl_start();
	}

    if(entity
        && (PRE_ENTITY_IDLE == g_role_launch.pre_entity_type))
    {
        g_role_launch.pre_entity_type = PRE_ENTITY_AP;
    }
    GLOBAL_INT_RESTORE();

	return ret;
}

void rl_enter_handler(void *left, void *right)
{	
	uint8_t *ptr;
	uint32_t ret;
	LAUNCH_REQ *ap_param;
	LAUNCH_REQ *sta_param;
	uint32_t hit_ap, hit_sta;
	FUNC_1PARAM_PTR ap_completion;
	FUNC_1PARAM_PTR sta_completion;	
	
    GLOBAL_INT_DECLARATION();
	
	JL_PRT("rl_enter_handler\r\n");
	ptr = (uint8_t *)os_malloc(sizeof(*ap_param) + sizeof(*sta_param));
	if(0 == ptr)
	{
		return;
	}
	ap_param = (LAUNCH_REQ *)ptr;
	sta_param = (LAUNCH_REQ *)&((LAUNCH_REQ *)ptr)[1];

    GLOBAL_INT_DISABLE();
	ap_completion = g_rl_socket.ap_completion;
	*ap_param = g_rl_socket.ap_param;
	hit_ap = g_rl_socket.ap_req_flag;
	
	sta_completion = g_rl_socket.sta_completion;
	*sta_param = g_rl_socket.sta_param;
	hit_sta = g_rl_socket.sta_req_flag;

	os_memset(&g_rl_socket, 0, sizeof(g_rl_socket));
    GLOBAL_INT_RESTORE();

	if(hit_sta)
	{
		JL_PRT("_sta_request_enter:0x%x :0x%x \r\n", g_role_launch.jl_previous_sta, g_role_launch.jl_following_sta);
		ret = _sta_request_enter(sta_param, sta_completion);
		if(ret)
		{
			g_sta_cache.sta_completion = sta_completion;
			g_sta_cache.sta_param = *sta_param;
			g_sta_cache.sta_req_flag = 1;
		}
	}
	
	if(hit_ap)
	{
		JL_PRT("_ap_request_enter\r\n");
		_ap_request_enter(ap_param, ap_completion);
	}

	os_free(ptr);
	ptr = 0;
}

extern void mhdr_set_station_status_when_reconnect_over(void);
uint32_t rl_sta_cache_request_enter(void)
{
	uint32_t ret = 0;
    GLOBAL_INT_DECLARATION();

	JL_PRT("sta_cache_request\r\n");
    GLOBAL_INT_DISABLE();
	
	do
	{
#if RL_SUPPORT_FAST_CONNECT
		if(g_sta_param_ptr->retry_cnt)
		{
			os_printf("g_sta_param_ptr->retry_cnt:%d\r\n", g_sta_param_ptr->retry_cnt);
			g_sta_param_ptr->retry_cnt--;
		}
		else
#endif
		{
			mhdr_set_station_status_when_reconnect_over();

			/* the opportunity of reconnecting is over*/
			break;
		}
			
		if(g_sta_cache.sta_req_flag)
		{
			rl_sta_request_enter(&g_sta_cache.sta_param, g_sta_cache.sta_completion);

			ret = 1;
		}

		os_memset(&g_sta_cache, 0, sizeof(g_sta_cache));
	}while(0);
	
    GLOBAL_INT_RESTORE();

	return ret;
}

void rl_launch_handler(void *left, void *right)
{
    uint32_t ap_ret = LAUNCH_STATUS_OVER;
    uint32_t sta_ret = LAUNCH_STATUS_OVER;
    
	JL_PRT("rl_launch_handler\r\n");
    switch(g_role_launch.pre_entity_type)
    {
        case PRE_ENTITY_AP:
            ap_ret = rl_launch_ap();
            if(LAUNCH_STATUS_OVER == ap_ret)
            {
                g_role_launch.pre_entity_type = PRE_ENTITY_STA;
                
                sta_ret = rl_launch_sta();
            }
            break;
            
        case PRE_ENTITY_STA:
            sta_ret = rl_launch_sta();
            if(LAUNCH_STATUS_OVER == sta_ret)
            {
                g_role_launch.pre_entity_type = PRE_ENTITY_AP;
                
                ap_ret = rl_launch_ap();
            }
            break;
            
        case PRE_ENTITY_IDLE:
			break;
			
        default:
            break;
    }

    if(!((LAUNCH_STATUS_OVER == sta_ret)
        && (LAUNCH_STATUS_OVER == ap_ret)))
    {
        rl_relaunch_chance();
    }
}
       
void rl_init(void)
{
    OSStatus err = kNoErr;

	JL_PRT("rl_init\r\n");
	err = rtos_init_oneshot_timer(&g_role_launch.rl_timer, 
									RL_LAUNCH_PERIOD, 
									(timer_2handler_t)rl_launch_handler, 
									NULL, 
									NULL);
	ASSERT(kNoErr == err); 
    
	err = rtos_init_oneshot_timer(&g_role_launch.enter_timer, 
									RL_ENTER_PERIOD, 
									(timer_2handler_t)rl_enter_handler, 
									NULL, 
									NULL);
	ASSERT(kNoErr == err); 
    g_role_launch.rl_timer_flag = RL_TIMER_INIT;

#if RL_SUPPORT_FAST_CONNECT
	user_connected_callback(rl_write_bssid_info);
#endif
}

void rl_uninit(void)
{    
    OSStatus err = kNoErr;

	JL_PRT("rl_uninit\r\n");

    if(RL_TIMER_START == g_role_launch.rl_timer_flag)
    {
        rl_stop();
    }
    
    err = rtos_deinit_oneshot_timer(&g_role_launch.rl_timer);
    ASSERT(kNoErr == err);  
    
    err = rtos_deinit_oneshot_timer(&g_role_launch.enter_timer);
    ASSERT(kNoErr == err);  
	
    g_role_launch.rl_timer_flag = RL_TIMER_UNINIT;   
}

void rl_start(void)
{    
    OSStatus err = kNoErr;
	
	JL_PRT("rl_timer-stop\r\n");
	err = rtos_stop_oneshot_timer(&g_role_launch.rl_timer);
	if(kNoErr != err)
	{
		JL_PRT("ERR:%d\r\n", err);
	}
	g_role_launch.rl_timer_flag = RL_TIMER_STOP;
	
	JL_PRT("rl_start_timer\r\n");
    err = rtos_start_oneshot_timer(&g_role_launch.rl_timer);
	if(kNoErr != err)
	{
		JL_PRT("err:%d\r\n", err);
	}
	
    ASSERT(kNoErr == err);

	g_role_launch.rl_timer_flag = RL_TIMER_START;
}

void rl_stop(void)
{    
    OSStatus err = kNoErr;
	
    err = rtos_stop_oneshot_timer(&g_role_launch.rl_timer);
    ASSERT(kNoErr == err);   
	
	JL_PRT("rl_stop\r\n");
    err = rtos_stop_oneshot_timer(&g_role_launch.enter_timer);
    ASSERT(kNoErr == err);   
    
    g_role_launch.rl_timer_flag = RL_TIMER_STOP;
}

RL_ENTITY_T *rl_alloc_entity(LAUNCH_REQ *param, FUNC_1PARAM_PTR completion)
{
    RL_ENTITY_T *entity;

    entity = (RL_ENTITY_T *)os_zalloc(sizeof(RL_ENTITY_T));
    ASSERT(entity);

    entity->completion_cb = completion;
    entity->launch_type = LAUNCH_TYPE_ASAP;
    os_memcpy(&entity->rlaunch, param, sizeof(LAUNCH_REQ));
    
    return entity;
}

void rl_free_entity(RL_ENTITY_T *d)
{
    os_free(d);
}

uint32_t rl_pre_sta_stop_launch(void)
{
	return 0;
}

uint32_t rl_pre_ap_stop_launch(void)
{
	return 0;
}

uint32_t rl_sta_may_next_launch(void)
{
    uint32_t yes = 0;

    if((RL_STATUS_STA_CSA_LAUNCHED_UNCERTAINTY != (rl_pre_sta_get_status() & RL_STATUS_OTHER_MASK)) 
            && ((rl_pre_sta_get_status() & RL_STATUS_CANCEL)
        || (RL_STATUS_STA_LAUNCHED == rl_pre_sta_get_status())
        || (RL_STATUS_STA_LAUNCH_FAILED == rl_pre_sta_get_status())))
    {
        yes = 1;
    }
	else if(g_role_launch.pre_sta_cancel
        && ((RL_STATUS_STA_DHCPING == rl_pre_sta_get_status())
        || (RL_STATUS_STA_CONNECTING == rl_pre_sta_get_status())
        || (RL_STATUS_STA_SCAN_VAIN == rl_pre_sta_get_status())))
	{
		yes = 2;
	}
    else 
	{
		if(RL_STATUS_STA_LAUNCH_FAILED == rl_pre_sta_get_status())
		{
			yes = 3;
		}
		
	}
	
    return yes;
}

uint32_t rl_ap_may_next_launch(uint32_t pre_cancel)
{
    uint32_t yes = 0;

    if(0 == pre_cancel)
    {
        yes = 1;
        goto exit_check;
    }
    
    if(((RL_STATUS_AP_TRANSMITTED_BCN + RL_STATUS_CANCEL) == rl_pre_ap_get_status())
        || (RL_STATUS_AP_LAUNCHED == (rl_pre_ap_get_status() & (~RL_STATUS_CANCEL))))
    {
        yes = 1;
    }
    
exit_check:    
    return yes;
}

uint32_t rl_pre_sta_get_status(void)
{
    return g_role_launch.pre_sta_status;
}

uint32_t rl_pre_ap_get_status(void)
{
    return g_role_launch.pre_ap_status;
}

uint32_t rl_pre_sta_set_status(uint32_t status)
{
    uint32_t cancel = 0;
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    if((RL_STATUS_STA_CHANNEL_SWITCHING == g_role_launch.pre_sta_status) 
            && (RL_STATUS_STA_LAUNCHED == status))
    {
        g_role_launch.pre_sta_status = RL_STATUS_STA_CSA_LAUNCHED_UNCERTAINTY;
    }
    else if((RL_STATUS_STA_CSA_LAUNCHED_UNCERTAINTY == g_role_launch.pre_sta_status) 
            && (RL_STATUS_STA_CHANNEL_SWITCHED== status))
    {
        g_role_launch.pre_sta_status = RL_STATUS_STA_LAUNCHED;
    }
    else
    {
        g_role_launch.pre_sta_status = status;
    }
    
    cancel = g_role_launch.pre_sta_cancel;
    if(cancel)
    {
        g_role_launch.pre_sta_status |= RL_STATUS_CANCEL;
    }
    GLOBAL_INT_RESTORE();
    
    return cancel;
}

uint32_t rl_pre_ap_disable_autobcn(void)
{
    if(g_role_launch.pre_ap_cancel)
    {
        mm_hw_ap_disable();
    }
    
    return g_role_launch.pre_ap_cancel;
}

uint32_t rl_pre_ap_set_status(uint32_t status)
{
    uint32_t cancel = 0;
    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    g_role_launch.pre_ap_status = status;
    cancel = g_role_launch.pre_ap_cancel;
    if(cancel)
    {
        g_role_launch.pre_ap_status += RL_STATUS_CANCEL;
    }

    GLOBAL_INT_RESTORE();
    
    return cancel;
}

uint32_t rl_pre_sta_get_cancel(void)
{
    return g_role_launch.pre_sta_cancel;
}

void rl_pre_sta_set_cancel(void)
{
    g_role_launch.pre_sta_cancel = 1;
}

void rl_pre_sta_clear_cancel(void)
{
    g_role_launch.pre_sta_cancel = 0;
}

uint32_t fl_get_pre_sta_cancel_status(void)
{
    return g_role_launch.pre_sta_cancel;
}

void rl_pre_ap_set_cancel(void)
{
    g_role_launch.pre_ap_cancel = 1;
}

void rl_pre_ap_clear_cancel(void)
{
    g_role_launch.pre_ap_cancel = 0;
}

uint32_t fl_get_pre_ap_cancel_status(void)
{
    return g_role_launch.pre_ap_cancel;
}

void rl_pre_ap_init(void)
{
    g_role_launch.pre_ap_cancel = 0;
    g_role_launch.pre_ap_status = 0;
}

void rl_pre_sta_init(void)
{
    g_role_launch.pre_sta_cancel = 0;
    g_role_launch.pre_sta_status = 0;
}

void rl_sta_request_start(LAUNCH_REQ *req)
{
    extern void demo_scan_app_init(void);
#if RL_SUPPORT_FAST_CONNECT
	RL_BSSID_INFO_T bssid_info;
	uint32_t ssid_len;
#endif

    ASSERT(req);
    
    switch(req->req_type)
    {
        case LAUNCH_REQ_STA:
            rl_pre_sta_init();
			#if RL_SUPPORT_FAST_CONNECT
			if (g_sta_param_ptr)
			{
				g_sta_param_ptr->orig_key_len = os_strlen(req->descr.wifi_key);
				if (g_sta_param_ptr->orig_key_len >= sizeof(g_sta_param_ptr->orig_key))
				{
					g_sta_param_ptr->orig_key_len = sizeof(g_sta_param_ptr->orig_key) - 1;
				}
				os_memcpy((uint8_t *)g_sta_param_ptr->orig_key, req->descr.wifi_key, g_sta_param_ptr->orig_key_len);
				g_sta_param_ptr->orig_key[g_sta_param_ptr->orig_key_len] = '\0';
			}
			rl_read_bssid_info(&bssid_info);

			uint32_t req_ssid_len = os_strlen((char*)req->descr.wifi_ssid);
			ssid_len = os_strlen((char*)bssid_info.ssid);

			ssid_len = (req_ssid_len > ssid_len)? req_ssid_len : ssid_len;
			if(ssid_len > SSID_MAX_LEN)
			{
				ssid_len = SSID_MAX_LEN;
			}
			if(os_memcmp(req->descr.wifi_ssid, bssid_info.ssid, ssid_len) == 0
				&& os_strcmp((char*)req->descr.wifi_key, (char*)bssid_info.pwd) == 0)
			{
				bk_printf("fast_connect\r\n");
                if(rl_pre_sta_get_status() == RL_STATUS_STA_SCANNING)
                {
                    os_printf("[wzl]It's scanning, terminate scan!\r\n");
                    extern  void scan_fast_terminate(void);
                    scan_fast_terminate();
                    while(rl_pre_sta_get_status() == RL_STATUS_STA_SCANNING)
                    {
                        rtos_delay_milliseconds(20);
                    }
                }
                
				rl_sta_fast_connect(&bssid_info);
			}
			else
			{
				bk_printf("normal_connect\r\n");
				bk_wlan_start_sta(&req->descr);
			}
			#else
			bk_printf("normal_connect\r\n");
            bk_wlan_start_sta(&req->descr);
			#endif
            break;
            
        case LAUNCH_REQ_PURE_STA_SCAN:
			rl_pre_sta_init();
            demo_scan_app_init();
            break;
            
        case LAUNCH_REQ_DELIF_STA:
            bk_wlan_stop(STATION);
            break;
            
        default:
            break;
    }
}

void rl_ap_request_start(LAUNCH_REQ *req)
{
    ASSERT(req);
    
    switch(req->req_type)
    {
        case LAUNCH_REQ_AP:
            rl_pre_ap_init();
            bk_wlan_start_ap(&req->descr);
            break;
            
        case LAUNCH_REQ_DELIF_AP:
            bk_wlan_stop(SOFT_AP);
            break;
            
        default:
            break;
    }
}

void rl_sta_request_enter(LAUNCH_REQ *param, FUNC_1PARAM_PTR completion)
{
    OSStatus err = kNoErr;
	
    GLOBAL_INT_DECLARATION();

	JL_PRT("rl_sta_request_enter\r\n");
    GLOBAL_INT_DISABLE();
	g_rl_socket.sta_completion = completion;
	g_rl_socket.sta_param = *param;
	g_rl_socket.sta_req_flag = 1;
	
	JL_PRT("enter_timer-Sstop\r\n");
	err = rtos_stop_oneshot_timer(&g_role_launch.enter_timer);
	if(kNoErr != err)
	{
		JL_PRT("enter_timer-Sstop:0x%x\r\n", err);
	}	
	
	JL_PRT("enter_timer-Sstart\r\n");
    err = rtos_start_oneshot_timer(&g_role_launch.enter_timer);
	if(kNoErr != err)
	{
		JL_PRT("enter_timer-Sstart:0x%x\r\n", err);
	}
	
    ASSERT(kNoErr == err);
    GLOBAL_INT_RESTORE();
}

void rl_ap_request_enter(LAUNCH_REQ *param, FUNC_1PARAM_PTR completion)
{
    OSStatus err = kNoErr;
	
    GLOBAL_INT_DECLARATION();

	JL_PRT("rl_ap_request_enter\r\n");

    GLOBAL_INT_DISABLE();
	g_rl_socket.ap_completion = completion;
	g_rl_socket.ap_param = *param;
	g_rl_socket.ap_req_flag = 1;
	
	JL_PRT("enter_timer-Astop\r\n");
	rtos_stop_oneshot_timer(&g_role_launch.enter_timer);
	
	JL_PRT("enter_timer-Astart\r\n");
    err = rtos_start_oneshot_timer(&g_role_launch.enter_timer);
    ASSERT(kNoErr == err);
    GLOBAL_INT_RESTORE();
}

#endif // CFG_ROLE_LAUNCH

// eof

