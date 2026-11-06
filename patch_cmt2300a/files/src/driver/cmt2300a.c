#include "cmt2300a.h"
#include "cmt2300a_defs.h"

/* Von drv_cmt2300a.c bereitgestellt (Bit-Bang-SPI) */
extern void cmt_spi3_write_fifo(const BYTE *buf, WORD len);
extern void cmt_spi3_read_fifo(BYTE *buf, WORD len);

void CMT2300A_SoftReset(void)
{
	CMT2300A_WriteReg(0x7F, 0xFF);
}

BYTE CMT2300A_GetChipStatus(void)
{
	return CMT2300A_ReadReg(CMT2300A_CUS_MODE_STA) & CMT2300A_MASK_CHIP_MODE_STA;
}

BOOL CMT2300A_WaitState(BYTE target, int timeout_ms)
{
	while(timeout_ms-- > 0)
	{
		if(CMT2300A_GetChipStatus() == target)
			return TRUE;
		CMT2300A_DelayMs(1);
	}
	return (CMT2300A_GetChipStatus() == target) ? TRUE : FALSE;
}

/* Kommando schreiben und warten, bis der Chip den Zielzustand erreicht hat */
static BOOL CMT2300A_SwitchState(BYTE cmd, BYTE target)
{
	CMT2300A_WriteReg(CMT2300A_CUS_MODE_CTL, cmd);
	return CMT2300A_WaitState(target, 50);
}

BOOL CMT2300A_GoSleep(void) { return CMT2300A_SwitchState(CMT2300A_GO_SLEEP, CMT2300A_STA_SLEEP); }
BOOL CMT2300A_GoStby(void)  { return CMT2300A_SwitchState(CMT2300A_GO_STBY,  CMT2300A_STA_STBY);  }
BOOL CMT2300A_GoRFS(void)   { return CMT2300A_SwitchState(CMT2300A_GO_RFS,   CMT2300A_STA_RFS);   }
BOOL CMT2300A_GoTFS(void)   { return CMT2300A_SwitchState(CMT2300A_GO_TFS,   CMT2300A_STA_TFS);   }
BOOL CMT2300A_GoRx(void)    { return CMT2300A_SwitchState(CMT2300A_GO_RX,    CMT2300A_STA_RX);    }
BOOL CMT2300A_GoTx(void)    { return CMT2300A_SwitchState(CMT2300A_GO_TX,    CMT2300A_STA_TX);    }

void CMT2300A_Init(void)
{
	BYTE tmp;

	CMT2300A_SoftReset();
	CMT2300A_DelayMs(20);
	CMT2300A_GoStby();

	tmp = CMT2300A_ReadReg(CMT2300A_CUS_MODE_STA);
	tmp |= CMT2300A_MASK_CFG_RETAIN;
	tmp &= ~CMT2300A_MASK_RSTN_IN_EN;
	CMT2300A_WriteReg(CMT2300A_CUS_MODE_STA, tmp);

	tmp = CMT2300A_ReadReg(CMT2300A_CUS_EN_CTL);
	tmp |= CMT2300A_MASK_LOCKING_EN;
	CMT2300A_WriteReg(CMT2300A_CUS_EN_CTL, tmp);
}

BOOL CMT2300A_ConfigRegBank(BYTE base_addr, const BYTE bank[], BYTE len)
{
	BYTE i;

	for(i = 0; i < len; i++)
		CMT2300A_WriteReg(base_addr + i, bank[i]);
	return TRUE;
}

/* ---- FIFO (nur fuer spaeteren Packet Mode / TX, im Direct Mode ungenutzt) ---- */

void CMT2300A_EnableReadFifo(void)
{
	BYTE reg = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
	reg |= CMT2300A_MASK_SPI_FIFO_RD_WR_SEL;
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, reg);
}

void CMT2300A_EnableWriteFifo(void)
{
	BYTE reg = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
	reg &= ~CMT2300A_MASK_SPI_FIFO_RD_WR_SEL;
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, reg);
}

void CMT2300A_ClearRxFifo(void)
{
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, CMT2300A_MASK_FIFO_CLR_RX);
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, 0);
}

void CMT2300A_ClearTxFifo(void)
{
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, CMT2300A_MASK_FIFO_CLR_TX);
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, 0);
}

void CMT2300A_WriteFifo(const BYTE *buf, WORD len)
{
	cmt_spi3_write_fifo(buf, len);
}

void CMT2300A_ReadFifo(BYTE *buf, WORD len)
{
	cmt_spi3_read_fifo(buf, len);
}

/* GPIO1 = DCLK, GPIO2 = DOUT, GPIO3 = INT2  (IO_SEL = 0x2B) */
void CMT2300A_ConfigGpio(void)
{
	BYTE reg = 0;

	reg |= CMT2300A_GPIO1_SEL_DCLK;
	reg |= CMT2300A_GPIO2_SEL_DOUT;
	reg |= CMT2300A_GPIO3_SEL_INT2;
	CMT2300A_WriteReg(CMT2300A_CUS_IO_SEL, reg);
}

void CMT2300A_ConfigInterrupt(void)
{
	CMT2300A_WriteReg(CMT2300A_CUS_INT1_CTL, CMT2300A_INT_SEL_RX_FIFO_TH);
	CMT2300A_WriteReg(CMT2300A_CUS_INT2_CTL, CMT2300A_INT_SEL_PKT_OK);
}

BYTE CMT2300A_ClearInterruptFlags(void)
{
	BYTE flags = CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1);

	CMT2300A_WriteReg(CMT2300A_CUS_INT_CLR1, 0xFF);
	CMT2300A_WriteReg(CMT2300A_CUS_INT_CLR2, 0xFF);
	return flags;
}

/* ---- Konfig-Helfer aus dem Hersteller-Ablauf (radio.c: RF_Config) ---- */

void CMT2300A_EnableInterrupt(BYTE mask)
{
	CMT2300A_WriteReg(CMT2300A_CUS_INT_EN, mask);
}

void CMT2300A_EnableLfosc(BOOL enable)
{
	BYTE tmp = CMT2300A_ReadReg(CMT2300A_CUS_SYS2);
	BYTE m = CMT2300A_MASK_LFOSC_RECAL_EN | CMT2300A_MASK_LFOSC_CAL1_EN | CMT2300A_MASK_LFOSC_CAL2_EN;

	if(enable) tmp |= m; else tmp &= ~m;
	CMT2300A_WriteReg(CMT2300A_CUS_SYS2, tmp);
}

void CMT2300A_EnableFifoMerge(BOOL enable)
{
	BYTE tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);

	if(enable) tmp |= CMT2300A_MASK_FIFO_MERGE_EN; else tmp &= ~CMT2300A_MASK_FIFO_MERGE_EN;
	CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

void CMT2300A_SetFifoThreshold(BYTE th)
{
	BYTE tmp = CMT2300A_ReadReg(CMT2300A_CUS_PKT29);

	tmp &= ~CMT2300A_MASK_FIFO_TH;
	tmp |= (th & CMT2300A_MASK_FIFO_TH);
	CMT2300A_WriteReg(CMT2300A_CUS_PKT29, tmp);
}
