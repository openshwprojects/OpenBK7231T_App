/**************************************************************************
 * File Name                   : tls_cmdp_ri.h
 * Author                      :
 * Version                     :
 * Date                        :
 * Description                 :
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
 * All rights reserved.
 *
 ***************************************************************************/
#if 0
#ifndef TLS_CMDP_RI_H
#define TLS_CMDP_RI_H
#include "wm_type_def.h"
struct tls_ricmd_t{
	s16 cmdId;
	int (*proc_func)(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size);
};

int tls_ricmd_exec(char *buf, u32 length, char *cmdrsp_buf, u32 *cmdrsp_size);

#endif /* end of TLS_CMDP_RI_H */
#endif
