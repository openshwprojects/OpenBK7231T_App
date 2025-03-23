#pragma once
#ifndef __DRV_BL0942_H__
#define __DRV_BL0942_H__

void BL0942_UART_Init(void);
void BL0942_UART_RunEverySecond(void);
void BL0942_SPI_Init(void);
void BL0942_SPI_RunEverySecond(void);
#if ENABLE_BL_TWIN
void BL0942_AddCommands(void);
#endif
#endif