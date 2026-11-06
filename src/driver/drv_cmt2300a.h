#pragma once
#include "../new_common.h"
#include "../httpserver/new_http.h"
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void CMT2300A_SH4(void);
void CMT2300A_RunQuickTick(void);
void CMT2300A_Stop(void);
void CMT2300A_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

BYTE CMT2300A_ReadReg(BYTE addr);
void CMT2300A_WriteReg(BYTE addr, BYTE dat);

void CMT2300A_DelayMs(int ms);
void CMT2300A_DelayUs(int us);

void CMT2300A_ReadFifo(BYTE *buf, WORD len);
void CMT2300A_WriteFifo(const BYTE *buf, WORD len);

void CMT2300A_ClearRxFifo(void);
void CMT2300A_ClearTxFifo(void);

void CMT2300A_EnableReadFifo(void);
void CMT2300A_EnableWriteFifo(void);
