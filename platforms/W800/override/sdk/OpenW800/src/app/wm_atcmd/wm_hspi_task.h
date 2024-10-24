/***************************************************************************** 
* 
* File Name : wm_hspi_task.h 
* 
* Description: High speed spi slave Module include file
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-9
*****************************************************************************/ 
#include "list.h"
#include "wm_type_def.h"
#include "wm_osal.h"


#ifndef TLS_HSPI_H
#define TLS_HSPI_H

struct tls_hspi {
	struct tls_slave_hspi	*tls_slave_hspi;
	u8 iffwup;        //ÊÇ·ñ¹Ì¼þÉý¼¶
//	tls_os_queue_t 			*rx_msg_queue;
//	tls_os_sem_t            *tx_msg_sem;
}; 

#endif /* end of WM_HSPI_TASK_H */

