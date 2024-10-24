/***************************************************************************** 
* 
* File Name : wm_http_fwup.h 
* 
* Description: Http firmware update header file.
* 
* Copyright (c) 2014 Winner Microelectronics Co., Ltd. 
* All rights reserved. 
* 
* Author : wanghf 
* 
* Date : 2014-6-5 
*****************************************************************************/ 

#ifndef WM_HTTP_FWUP_H
#define WM_HTTP_FWUP_H

#include "wm_type_def.h"
#include "wm_http_client.h"

/*************************************************************************** 
* Function: http_fwup 
* Description: Download the firmware from internet by http and upgrade it. 
* 
* Input: ClientParams: The parameters of connecting http server. 
* 
* Output: None 
* 
* Return: 0-success, other- failed 
* 
* Date : 2014-6-5 
****************************************************************************/ 
int   http_fwup(HTTPParameters ClientParams);

#endif //WM_HTTP_FWUP_H
