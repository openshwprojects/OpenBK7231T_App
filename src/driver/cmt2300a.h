#pragma once

#include "drv_cmt2300a.h"

void CMT2300A_SoftReset(void);

BYTE CMT2300A_GetChipStatus(void);
BOOL CMT2300A_WaitState(BYTE target, int timeout_ms);

BOOL CMT2300A_GoSleep(void);
BOOL CMT2300A_GoStby(void);
BOOL CMT2300A_GoRFS(void);
BOOL CMT2300A_GoTFS(void);
BOOL CMT2300A_GoRx(void);
BOOL CMT2300A_GoTx(void);

void CMT2300A_Init(void);

BOOL CMT2300A_ConfigRegBank(
	BYTE base_addr,
	const BYTE bank[],
	BYTE len);
void CMT2300A_ConfigGpio(void);
void CMT2300A_ConfigInterrupt(void);
BYTE CMT2300A_ClearInterruptFlags(void);
void CMT2300A_EnableInterrupt(BYTE mask);
void CMT2300A_EnableLfosc(BOOL enable);
void CMT2300A_EnableFifoMerge(BOOL enable);
void CMT2300A_SetFifoThreshold(BYTE th);
