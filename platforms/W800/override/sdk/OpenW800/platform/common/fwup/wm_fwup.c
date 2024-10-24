/***************************************************************************** 
* 
* File Name : wm_fwup.c
* 
* Description: firmware update Module 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-16
*****************************************************************************/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wm_mem.h"
#include "list.h"
#include "wm_debug.h"
#include "wm_internal_flash.h"
#include "wm_crypto_hard.h"

#include "utils.h"
#include "wm_fwup.h"
#include "wm_watchdog.h"
#include "wm_wifi.h"
#include "wm_flash_map.h"
#include "wm_wl_task.h"
#include "wm_params.h"
#include "wm_param.h"
#include "wm_ram_config.h"

#define FWUP_MSG_QUEUE_SIZE      (4)

#define FWUP_TASK_STK_SIZE      (256)

#define FWUP_MSG_START_ENGINEER      (1)

static struct tls_fwup *fwup = NULL;
static tls_os_queue_t *fwup_msg_queue = NULL;

static u32 *fwup_task_stk = NULL;
static u8 oneshotback = 0;

extern int tls_fls_fast_write_init(void);
extern int tls_fls_fast_write(u32 addr, u8 *buf, u32 length);
extern void tls_fls_fast_write_destroy(void);


static void fwup_update_autoflag(void)
{
    u8 auto_reconnect = 0xff;

    tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_GET, &auto_reconnect);
    if(auto_reconnect == WIFI_AUTO_CNT_TMP_OFF)
    {
    	auto_reconnect = WIFI_AUTO_CNT_ON;
    	tls_wifi_auto_connect_flag(WIFI_AUTO_CNT_FLAG_SET, &auto_reconnect);
    }
    return;
}

int tls_fwup_img_header_check(IMAGE_HEADER_PARAM_ST *img_param)
{
	psCrcContext_t	crcContext;
	unsigned int crcvalue = 0;
	unsigned int crccallen = 0;
	int i = 0;
	IMAGE_HEADER_PARAM_ST run_param;

	if (img_param->magic_no != SIGNATURE_WORD)
	{
		return FALSE;	
	}
	/*temperary forbid to update secboot*/
	if (img_param->img_attr.b.img_type != IMG_TYPE_FLASHBIN0)
	{
		return FALSE;
	}

	/*check run image and upgrade image have the same image info*/
	tls_fls_read(img_param->img_header_addr, (u8 *)&run_param, sizeof(run_param));
	if ((run_param.img_attr.b.signature && (img_param->img_attr.b.signature == 0))
		|| ((run_param.img_attr.b.signature == 0) && img_param->img_attr.b.signature))
	{
		return FALSE;
	}

	if ((run_param.img_attr.b.code_encrypt && (img_param->img_attr.b.code_encrypt == 0))
		|| ((run_param.img_attr.b.code_encrypt == 0) && img_param->img_attr.b.code_encrypt))
	{
		return FALSE;
	}	

    if (img_param->img_addr % IMAGE_START_ADDR_MSK) //vbr register must be 1024 align.
    {
    	return FALSE;
    }

    if ((img_param->img_addr|FLASH_BASE_ADDR) >= USER_ADDR_START)
    {
    	return FALSE;
    }

    if ((img_param->img_addr|FLASH_BASE_ADDR) + img_param->img_len >= USER_ADDR_START)
    {
    	return FALSE;
    }

	if(((img_param->upgrade_img_addr|FLASH_BASE_ADDR) < CODE_UPD_START_ADDR)
		|| ((img_param->upgrade_img_addr|FLASH_BASE_ADDR) + img_param->img_len >= CODE_RUN_START_ADDR))
	{
		return FALSE;
	}

	crccallen = (sizeof(IMAGE_HEADER_PARAM_ST)-4);

	tls_crypto_crc_init(&crcContext, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, 3);
	for (i = 0; i <  crccallen/4; i++)
	{
		MEMCPY((unsigned char *)&crcvalue, (unsigned char *)img_param +i*4, 4); 
		tls_crypto_crc_update(&crcContext, (unsigned char *)&crcvalue, 4);
	}
	crcvalue = 0;
	tls_crypto_crc_final(&crcContext, &crcvalue);
	if (img_param->hd_checksum == crcvalue)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void fwup_scheduler(void *data)
{
	u8 *buffer = NULL;
	int err;
	u32 msg;
	u32 len;	
	u32 image_checksum = 0;
	//u32 org_checksum = 0;
	struct tls_fwup_request *request;
	struct tls_fwup_request *temp;
	IMAGE_HEADER_PARAM_ST booter;

	while (1) 
	{
		err = tls_os_queue_receive(fwup_msg_queue, (void **)&msg, 0, 0);
   		tls_watchdog_clr();
		if(err != TLS_OS_SUCCESS) 
		{
			continue;
		}
		switch(msg) 
		{
			case FWUP_MSG_START_ENGINEER:
				if(dl_list_empty(&fwup->wait_list) == 0) 
				{
					fwup->current_state |= TLS_FWUP_STATE_BUSY;
				}
				dl_list_for_each_safe(request, temp, &fwup->wait_list, struct tls_fwup_request, list) 
				{
					request->status = TLS_FWUP_REQ_STATUS_BUSY;
					if(fwup->current_state & TLS_FWUP_STATE_ERROR) 
					{
						TLS_DBGPRT_WARNING("some error happened during firmware update, so discard all the request in the waiting queue!\n");
						if(fwup->current_state & TLS_FWUP_STATE_ERROR_IO) 
						{
							request->status = TLS_FWUP_REQ_STATUS_FIO;
						}
						else if(fwup->current_state & TLS_FWUP_STATE_ERROR_SIGNATURE) 
						{
							request->status = TLS_FWUP_REQ_STATUS_FSIGNATURE;
						}
						else if(fwup->current_state & TLS_FWUP_STATE_ERROR_MEM) 
						{	
							request->status = TLS_FWUP_REQ_STATUS_FMEM;
						}
						else if(fwup->current_state & TLS_FWUP_STATE_ERROR_CRC) 
						{
							request->status = TLS_FWUP_REQ_STATUS_FCRC;
						}
						goto request_finish;
					} 
					else if(fwup->current_state & TLS_FWUP_STATE_COMPLETE) 
					{
						TLS_DBGPRT_WARNING("the firmware updating conpletes, so discard the request in the waiting queue!\n");
						request->status = TLS_FWUP_REQ_STATUS_FCOMPLETE;
						goto request_finish;
					}

					if(fwup->current_image_src <= TLS_FWUP_IMAGE_SRC_WEB)
					{
					    buffer = request->data;
						if(fwup->received_len < sizeof(IMAGE_HEADER_PARAM_ST))
						{
							len = sizeof(IMAGE_HEADER_PARAM_ST) - fwup->received_len;
							if(request->data_len < len)
							{
								len = request->data_len;
							}

							MEMCPY((u8 *)&booter + fwup->received_len, buffer, len);
							request->data_len  -= len;
							buffer += len;
							fwup->received_len += len;
							if(fwup->received_len == sizeof(IMAGE_HEADER_PARAM_ST))
							{
								if (!tls_fwup_img_header_check(&booter))
								{
									request->status = TLS_FWUP_REQ_STATUS_FIO;
									fwup->current_state |= TLS_FWUP_STATE_ERROR_IO;
									goto request_finish;
								}
							
								fwup->program_base = booter.upgrade_img_addr | FLASH_BASE_ADDR;
								fwup->program_offset = 0;
								fwup->total_len = booter.img_len + sizeof(IMAGE_HEADER_PARAM_ST);

								if (booter.img_attr.b.signature)
								{
									fwup->total_len += 128;
								}
								/*write booter header to flash*/
								err = tls_fls_fast_write(fwup->program_base + fwup->program_offset, (u8 *)&booter, sizeof(IMAGE_HEADER_PARAM_ST));
								if(err != TLS_FLS_STATUS_OK) 
								{
									TLS_DBGPRT_ERR("failed to program flash!\n");
									request->status = TLS_FWUP_REQ_STATUS_FIO;
									fwup->current_state |= TLS_FWUP_STATE_ERROR_IO;
									goto request_finish;
								}
								/*initialize updated_len*/
								fwup->updated_len = sizeof(IMAGE_HEADER_PARAM_ST);
								fwup->program_offset = sizeof(IMAGE_HEADER_PARAM_ST);
							}
						}
						fwup->received_len += request->data_len;
					}
					if (request->data_len > 0) 
					{
					//	TLS_DBGPRT_INFO("write the firmware image to the flash. %x\n\r", fwup->program_base + fwup->program_offset);
                        err = tls_fls_fast_write(fwup->program_base + fwup->program_offset, buffer, request->data_len);
						if(err != TLS_FLS_STATUS_OK) 
						{
							TLS_DBGPRT_ERR("failed to program flash!\n");
							request->status = TLS_FWUP_REQ_STATUS_FIO;
							fwup->current_state |= TLS_FWUP_STATE_ERROR_IO;
							goto request_finish;
						}	

						fwup->program_offset += request->data_len;
						fwup->updated_len += request->data_len;

						//TLS_DBGPRT_INFO("updated: %d bytes\n" , fwup->updated_len);
						if(fwup->updated_len >= (fwup->total_len)) 
						{

							u8 *buffer_t;
							u32 len, left, offset;	

							psCrcContext_t	crcContext;

							tls_fls_fast_write_destroy();
							buffer_t = tls_mem_alloc(1024);
							if (buffer_t == NULL) 
							{
								TLS_DBGPRT_ERR("unable to verify because of no memory\n");
								request->status = TLS_FWUP_REQ_STATUS_FMEM;
								fwup->current_state |= TLS_FWUP_STATE_ERROR_MEM;
								goto request_finish;
							} 
							else 
							{							
								offset = sizeof(IMAGE_HEADER_PARAM_ST);
								if (booter.img_attr.b.signature)
								{
									left = fwup->total_len - 128 - sizeof(IMAGE_HEADER_PARAM_ST);
								}
								else
								{
									left = fwup->total_len - sizeof(IMAGE_HEADER_PARAM_ST);
								}

								tls_crypto_crc_init(&crcContext, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, 3);
								while (left > 0) 
								{
									len = left > 1024 ? 1024 : left;
									err = tls_fls_read(fwup->program_base + offset, buffer_t, len);
									if (err != TLS_FLS_STATUS_OK) 
									{
										request->status = TLS_FWUP_REQ_STATUS_FIO;
										fwup->current_state |= TLS_FWUP_STATE_ERROR_IO;
										goto request_finish;
									}
									tls_crypto_crc_update(&crcContext, buffer_t, len);
									offset += len;
									left -= len;
								}
								tls_crypto_crc_final(&crcContext, &image_checksum);								
								tls_mem_free(buffer_t);
							}

							if (booter.org_checksum != image_checksum)			
							{
                                TLS_DBGPRT_ERR("verify incorrect[0x%02x, but 0x%02x]\n", booter.org_checksum, image_checksum);
								request->status = TLS_FWUP_REQ_STATUS_FCRC;
								fwup->current_state |= TLS_FWUP_STATE_ERROR_CRC;
								goto request_finish;
							}
							else
							{
								/*Write OTA flag to flash used by boot loader*/
								tls_fls_write(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&booter.org_checksum, sizeof(booter.org_checksum));
							}

							TLS_DBGPRT_INFO("update the firmware successfully!\n");
							fwup->current_state |= TLS_FWUP_STATE_COMPLETE;
							if (oneshotback != 0){
								tls_wifi_set_oneshot_flag(oneshotback);	// »Ö¸´Ò»¼üÅäÖÃ
							}
							
						}
					}
					request->status = TLS_FWUP_REQ_STATUS_SUCCESS;

request_finish:
					tls_os_sem_acquire(fwup->list_lock, 0);
					dl_list_del(&request->list);
					tls_os_sem_release(fwup->list_lock);
					if(dl_list_empty(&fwup->wait_list) == 1) 
					{
						fwup->current_state &= ~TLS_FWUP_STATE_BUSY;
					}

					/*when all data has received  or error happened, reset system*/
					if(((fwup->updated_len >= sizeof(IMAGE_HEADER_PARAM_ST)) && (fwup->updated_len >= (fwup->total_len)))
						|| (request->status != TLS_FWUP_REQ_STATUS_SUCCESS))
					{
					    fwup_update_autoflag();
						tls_sys_set_reboot_reason(REBOOT_REASON_ACTIVE);
					    tls_sys_reset();
					}
					
					if(request->complete) 
					{
						request->complete(request, request->arg);
					}
				}
				break;

			default:
				break;
		}
	}
}

void fwup_request_complete(struct tls_fwup_request *request, void *arg)
{
	tls_os_sem_t *sem;

	if((request == NULL) || (arg == NULL)) 
	{
		return;
	}
	sem = (tls_os_sem_t *)arg;
	tls_os_sem_release(sem);
}

u32 tls_fwup_enter(enum tls_fwup_image_src image_src)
{
	u32 session_id = 0;
	u32 cpu_sr;
	bool enable = FALSE;

	tls_fwup_init();

	if (fwup == NULL) 
	{
		TLS_DBGPRT_INFO("fwup is null!\n");
		return 0;
	}
	if (fwup->busy == TRUE) 
	{
		TLS_DBGPRT_INFO("fwup is busy!\n");
		return 0;
	}

	cpu_sr = tls_os_set_critical();
	
	do 
	{
		session_id = rand();
	}while(session_id == 0);
	
	fwup->current_state = 0;
	fwup->current_image_src = image_src;

	fwup->received_len = 0;
	fwup->total_len = 0;
	fwup->updated_len = 0;
	fwup->program_base = 0;
	fwup->program_offset = 0;
	fwup->received_number = -1;
		
	fwup->current_session_id = session_id;
	fwup->busy = TRUE;
	oneshotback = tls_wifi_get_oneshot_flag();
	if (oneshotback != 0){
		tls_wifi_set_oneshot_flag(0);	// ÍË³öÒ»¼üÅäÖÃ
	}
	tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE);	
	if (TRUE == enable)
	{
		tls_wifi_set_psflag(FALSE, 0);
	}
	tls_fls_fast_write_init();
	tls_os_release_critical(cpu_sr);
	return session_id;
}

int tls_fwup_exit(u32 session_id)
{
	u32 cpu_sr;
	bool enable = FALSE;
	//tls_os_task_t fwtask;
	//tls_os_status_t osstatus = 0;
	
	if ((fwup == NULL) || (fwup->busy == FALSE)) 
	{
		return TLS_FWUP_STATUS_EPERM;
	}
	if (session_id != fwup->current_session_id) 
	{
		return TLS_FWUP_STATUS_ESESSIONID;
	}
	if (fwup->current_state & TLS_FWUP_STATE_BUSY) 
	{
		return TLS_FWUP_STATUS_EBUSY;
	}

	cpu_sr = tls_os_set_critical();

	fwup->current_state = 0;

	fwup->received_len = 0;
	fwup->total_len = 0;
	fwup->updated_len = 0;
	fwup->program_base = 0;
	fwup->program_offset = 0;
	fwup->received_number = -1;

	fwup->current_session_id = 0;
	fwup->busy = FALSE;
	if (oneshotback != 0){
		tls_wifi_set_oneshot_flag(oneshotback); // »Ö¸´Ò»¼üÅäÖÃ
	}
	tls_param_get(TLS_PARAM_ID_PSM, &enable, TRUE);	
	tls_wifi_set_psflag(enable, 0);
	tls_os_release_critical(cpu_sr);
	return TLS_FWUP_STATUS_OK;
}

int tls_fwup_get_current_session_id(void)
{
	if (fwup){
		return fwup->current_session_id;
	}
	return 0;
}

int tls_fwup_set_update_numer(int number)
{
	if(1 == number - fwup->received_number)
	{
		fwup->received_number = number;
		return TLS_FWUP_STATUS_OK;
	}
	return TLS_FWUP_STATE_UNDEF;
}

int tls_fwup_get_current_update_numer(void)
{
	return fwup->received_number;
}

int tls_fwup_get_status(void)
{
	return fwup->busy;
}

int tls_fwup_set_crc_error(u32 session_id)
{
	if(fwup == NULL) 
	{
		return TLS_FWUP_STATUS_EPERM;
	}
	if(session_id != fwup->current_session_id) 
	{
		return TLS_FWUP_STATUS_ESESSIONID;
	}
	fwup->current_state |= TLS_FWUP_STATE_ERROR_CRC;

	return TLS_FWUP_STATUS_OK;
}

static int tls_fwup_request_async(u32 session_id, struct tls_fwup_request *request)
{
	u8 need_sched;
	
	if(fwup == NULL) 
	{
		return TLS_FWUP_STATUS_EPERM;
	}
	if(session_id != fwup->current_session_id) 
	{
		return TLS_FWUP_STATUS_ESESSIONID;
	}
	if((request == NULL) || (request->data == NULL) || (request->data_len == 0)) 
	{
		return TLS_FWUP_STATUS_EINVALID;
	}
	tls_os_sem_acquire(fwup->list_lock, 0);
	if(dl_list_empty(&fwup->wait_list)) 
	{
		need_sched = 1;
	}
	else
	{
		need_sched = 0;
	}
	request->status = TLS_FWUP_REQ_STATUS_IDLE;
	dl_list_add_tail(&fwup->wait_list, &request->list);
	tls_os_sem_release(fwup->list_lock);
	if(need_sched == 1) 
	{
		tls_os_queue_send(fwup_msg_queue, (void *)FWUP_MSG_START_ENGINEER, 0);
	}
	return TLS_FWUP_STATUS_OK;
}


int tls_fwup_request_sync(u32 session_id, u8 *data, u32 data_len)
{
	int err;
	tls_os_sem_t *sem;
	struct tls_fwup_request request;

	if(fwup == NULL) 
	{
		return TLS_FWUP_STATUS_EPERM;
	}
	if(session_id != fwup->current_session_id) 
	{
		return TLS_FWUP_STATUS_ESESSIONID;
	}
	if((data == NULL) || (data_len == 0)) 
	{
		return TLS_FWUP_STATUS_EINVALID;
	}

	err = tls_os_sem_create(&sem, 0);
	if(err != TLS_OS_SUCCESS) 
	{
		return TLS_FWUP_STATUS_EMEM;
	}
	request.data = data;
	request.data_len = data_len;
	request.complete = fwup_request_complete;
	request.arg = (void *)sem;

	err = tls_fwup_request_async(session_id, &request);
	if(err == TLS_FWUP_STATUS_OK) 
	{
		tls_os_sem_acquire(sem, 0);
	}
	tls_os_sem_delete(sem);

	switch(request.status) 
	{
		case TLS_FWUP_REQ_STATUS_SUCCESS:
			err = TLS_FWUP_STATUS_OK;
			break;

		case TLS_FWUP_REQ_STATUS_FIO:
			err = TLS_FWUP_STATUS_EIO;
			break;

		case TLS_FWUP_REQ_STATUS_FSIGNATURE:
			err = TLS_FWUP_STATUS_ESIGNATURE;
			break;

		case TLS_FWUP_REQ_STATUS_FMEM:
			err = TLS_FWUP_STATUS_EMEM;
			break;

		case TLS_FWUP_REQ_STATUS_FCRC:
			err = TLS_FWUP_STATUS_ECRC;
			break;

		case TLS_FWUP_REQ_STATUS_FCOMPLETE:
			err = TLS_FWUP_STATUS_EIO;
			break;

		default:
			err = TLS_FWUP_STATUS_EUNDEF;
			break;
	}
	return err;
}

u16 tls_fwup_current_state(u32 session_id)
{
	if(fwup == NULL) 
	{
		return TLS_FWUP_STATE_UNDEF;
	}
	if(session_id != fwup->current_session_id) 
	{
		return TLS_FWUP_STATE_UNDEF;
	}
	return fwup->current_state;
}

int tls_fwup_reset(u32 session_id)
{
	u32 cpu_sr;
	
	if ((fwup == NULL) || (fwup->busy == FALSE)) {return TLS_FWUP_STATUS_EPERM;}
	if (session_id != fwup->current_session_id) {return TLS_FWUP_STATUS_ESESSIONID;}
	if (fwup->current_state & TLS_FWUP_STATE_BUSY) {return TLS_FWUP_STATUS_EBUSY;}

	cpu_sr = tls_os_set_critical();

	fwup->current_state = 0;

	fwup->received_len = 0;
	fwup->total_len = 0;
	fwup->updated_len = 0;
	fwup->program_base = 0;
	fwup->program_offset = 0;
	
	tls_os_release_critical(cpu_sr);
	
	return TLS_FWUP_STATUS_OK;
}

int tls_fwup_clear_error(u32 session_id)
{
	u32 cpu_sr;
	
	if ((fwup == NULL) || (fwup->busy == FALSE)) {return TLS_FWUP_STATUS_EPERM;}
	if (session_id != fwup->current_session_id) {return TLS_FWUP_STATUS_ESESSIONID;}
	if (fwup->current_state & TLS_FWUP_STATE_BUSY) {return TLS_FWUP_STATUS_EBUSY;}

	cpu_sr = tls_os_set_critical();

	fwup->current_state &= ~TLS_FWUP_STATE_ERROR;
	
	tls_os_release_critical(cpu_sr);

	return TLS_FWUP_STATUS_OK;
}

int tls_fwup_init(void)
{
	int err;

	if(fwup != NULL) 
	{
		TLS_DBGPRT_ERR("firmware update module has been installed!\n");
		return TLS_FWUP_STATUS_EBUSY;
	}

	fwup = tls_mem_alloc(sizeof(*fwup));
	if(fwup == NULL) 
	{
		TLS_DBGPRT_ERR("allocate @fwup fail!\n");
		return TLS_FWUP_STATUS_EMEM;
	}
	memset(fwup, 0, sizeof(*fwup));
	
	err = tls_os_sem_create(&fwup->list_lock, 1);
	if(err != TLS_OS_SUCCESS) 
	{
		TLS_DBGPRT_ERR("create semaphore @fwup->list_lock fail!\n");
		tls_mem_free(fwup);
		return TLS_FWUP_STATUS_EMEM;
	}

	dl_list_init(&fwup->wait_list);
	fwup->busy = FALSE;

	err = tls_os_queue_create(&fwup_msg_queue, FWUP_MSG_QUEUE_SIZE);
	if (err != TLS_OS_SUCCESS) 
	{
		TLS_DBGPRT_ERR("create message queue @fwup_msg_queue fail!\n");
		tls_os_sem_delete(fwup->list_lock);
		tls_mem_free(fwup);
		return TLS_FWUP_STATUS_EMEM;
	}
	fwup_task_stk = (u32 *)tls_mem_alloc(FWUP_TASK_STK_SIZE * sizeof(u32));
	if (fwup_task_stk)
	{
		err = tls_os_task_create(NULL, "fwup",
							fwup_scheduler,
							(void *)fwup,
							(void *)fwup_task_stk,
							FWUP_TASK_STK_SIZE * sizeof(u32),
							TLS_FWUP_TASK_PRIO,
							0);
		if (err != TLS_OS_SUCCESS)
		{
			TLS_DBGPRT_ERR("create firmware update process task fail!\n");

			tls_os_queue_delete(fwup_msg_queue);
			fwup_msg_queue = NULL;
			tls_os_sem_delete(fwup->list_lock);
			fwup->list_lock = NULL;
			tls_mem_free(fwup);
			fwup = NULL;
			tls_mem_free(fwup_task_stk);
			fwup_task_stk = NULL;
			return TLS_FWUP_STATUS_EMEM;
		}
	}
	else
	{
		tls_os_queue_delete(fwup_msg_queue);
		fwup_msg_queue = NULL;
		tls_os_sem_delete(fwup->list_lock);
		fwup->list_lock = NULL;
		tls_mem_free(fwup);
		return TLS_FWUP_STATUS_EMEM;
	}

	return TLS_FWUP_STATUS_OK;
}

