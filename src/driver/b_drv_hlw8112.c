/**
 *!
 * \file        b_drv_hlw811x.c
 * \version     v0.0.1
 * \date        2024/08/14
 * \author      hmchen(chenhaimeng@189.cn)
 *******************************************************************************
 * @attention
 *
 * Copyright (c) 2020 Bean
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************
 */

/*Includes ----------------------------------------------*/
#include "drivers/inc/b_drv_hlw811x.h"
#include "drivers/inc/HLW811x_regs.h"
#include <string.h>

#include "utils/inc/b_util_log.h"

/**
 * \addtogroup B_DRIVER
 * \{
 */

/**
 * \addtogroup HLW8112
 * \{
 */

/**
 * \defgroup HLW8112_Private_TypesDefinitions
 * \{
 */

/**
 * \}
 */

/**
 * \defgroup HLW8112_Private_Defines
 * \{
 */

#define DRIVER_NAME HLW811X

/** Device Identification (Who am I) **/

/**
 * \}
 */

/**
 * \defgroup HLW8112_Private_Macros
 * \{
 */

/**
 * \}
 */

/**
 * \defgroup HLW8112_Private_Variables
 * \{
 */
bDRIVER_HALIF_TABLE(bHLW811X_HalIf_t, DRIVER_NAME);

static bHlw811xPrivate_t bHlw811xRunInfo[bDRIVER_HALIF_NUM(bHLW811X_HalIf_t, DRIVER_NAME)];


/* Private Constants ------------------------------------------------------------*/
/**
 * @brief  Commands
 */
#define HLW811X_COMMAND_ADDRESS       HLW811X_REG_ADDR_Command
#define HLW811X_COMMAND_WRITE_ENABLE  0xE5
#define HLW811X_COMMAND_WRITE_CLOSE   0xDC
#define HLW811X_COMMAND_CHANNELA      0x5A
#define HLW811X_COMMAND_CHANNELB      0xA5
#define HLW811X_COMMAND_RESET         0x96

/**
 * @brief  CoefReg coefficients
 */
#define HLW811X_VREF                  1.25f

/**
 * @brief  PGA coefficients
 */
static const uint8_t HLW811X_PGA_TABLE[5] = {0x00, 0x01, 0x02, 0x03, 0x04};


/**
 ==================================================================================
                       ##### Private Functions #####
 ==================================================================================
 */
static inline int8_t
HLW811x_WriteReg(bDriverInterface_t *pdrv,
				 uint8_t Address, uint8_t Size, uint8_t *Data)
{
	bDRIVER_GET_HALIF(_if, bHLW811X_HalIf_t, pdrv);
	uint8_t Buffer[3] = {0};
	int8_t Result = 0;

#if (HLW811X_CONFIG_SPI_WRITE_RETRY > 0)
	uint8_t Retry = 0;
	uint8_t BufferCheckTX[3] = {HLW811X_REG_ADDR_Wdata, 0, 0};
	uint8_t BufferCheckRX[3] = {0};
#endif

	if (Address == HLW811X_COMMAND_ADDRESS) // Special address for commands
		Buffer[0] = Address;
	else
		Buffer[0] = Address | 0x80;

	if (Size > 2)
		return -1;

	for (uint8_t i = 0; i < Size; i++)
		Buffer[i + 1] = Data[i];


#if (HLW811X_CONFIG_SPI_WRITE_RETRY > 0)

	do
	{
		// Write data
		if (_if->is_spi)
		{
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
			bHalSpiSend(&_if->_if._spi, Buffer, Size + 1);
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
		}
		else
		{
			//uart
		}

		// Command address is not supposed to be checked
		if (Address == HLW811X_COMMAND_ADDRESS)
			break;

		// Read WDATA register to check the written data
		if (_if->is_spi)
		{
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
			bHalSpiSend(&_if->_if._spi, BufferCheckTX, 1);
			bHalSpiReceive(&_if->_if._spi, BufferCheckRX, 2);
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
		}
		else
		{
			//uart
		}

		// Compare the written data with the WDATA register
		Result = 0;

		for (uint8_t i = 0; i < Size; i++)
		{
			if (Data[i] != BufferCheckRX[i])
			{
				Result = -1;
				break;
			}
		}
	}
	while (Result < 0 && Retry++ < HLW811X_CONFIG_SPI_WRITE_RETRY);

#else

	if (_if->is_spi)
	{
		bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
		bHalSpiSend(&_if->_if._spi, Buffer, Size + 1);
		bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
	}
	else
	{
		//uart
	}

#endif

	return Result;

}
static inline int8_t
HLW811x_ReadReg(bDriverInterface_t *pdrv,
				uint8_t Address, uint8_t Size, uint8_t *Data)
{
	bDRIVER_GET_HALIF(_if, bHLW811X_HalIf_t, pdrv);
	uint8_t BufferTx[5] = {0xFF};
	uint8_t BufferRx[5] = {0};
	int8_t Result = 0;

#if (HLW811X_CONFIG_SPI_READ_RETRY > 0)
	uint8_t Retry = 0;
	uint8_t BufferCheckTX[5] = {HLW811X_REG_ADDR_Rdata, 0, 0};
	uint8_t BufferCheckRX[5] = {0};
#endif

	if (Address == HLW811X_COMMAND_ADDRESS) // Special address for commands
		return -1;
	else
		BufferTx[0] = Address & 0x7F;

	if (Size > 4)
		return -1;

#if (HLW811X_CONFIG_SPI_READ_RETRY > 0)

	do
	{
		// Read data
		if (_if->is_spi)
		{
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
			bHalSpiSend(&_if->_if._spi, BufferTx, 1);
			bHalSpiReceive(&_if->_if._spi, BufferRx, Size);
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
		}
		else
		{
			//uart
		}

		// Read RDATA register to check the read data
		if (_if->is_spi)
		{
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
			bHalSpiSend(&_if->_if._spi, BufferCheckTX, 1);
			bHalSpiReceive(&_if->_if._spi, BufferCheckRX, 4);
			bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
		}
		else
		{
			//uart
		}

		// Compare the read data with the RDATA register
		Result = 0;

		for (uint8_t i = 0; i < Size ; i++)
		{
			if (BufferRx[i] != BufferCheckRX[i])
			{
				Result = -1;
				break;
			}
		}
	}
	while (Result < 0 && Retry++ < HLW811X_CONFIG_SPI_READ_RETRY);

#else

	if (_if->is_spi)
	{
		bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 0);
		bHalSpiSend(&_if->_if._spi, BufferTx, 1);
		bHalSpiReceive(&_if->_if._spi, BufferRx, Size);
		bHalGpioWritePin(_if->_if._spi.cs.port, _if->_if._spi.cs.pin, 1);
	}
	else
	{
		//uart
	}

#endif
	uint32_t check_sum = 0;
	for (uint8_t i = 0; i < Size; i++)
	{
		check_sum += BufferRx[i];
		Data[i] = BufferRx[i];
	}
	if(check_sum == (0xff * Size))
	{
		Result = -1;	
	}
	return Result;
}

static int8_t
HLW811x_WriteReg16(bDriverInterface_t *pdrv,
				   uint8_t Address, uint16_t Data)
{
	uint8_t Buffer[2] = {0};

	Buffer[0] = (Data >> 8) & 0xFF;
	Buffer[1] = Data & 0xFF;

	return HLW811x_WriteReg(pdrv, Address, 2, Buffer);
}

static int8_t
HLW811x_ReadReg16(bDriverInterface_t *pdrv,
				  uint8_t Address, uint16_t *Data)
{
	uint8_t Buffer[2] = {0};
	int8_t Result = 0;

	Result = HLW811x_ReadReg(pdrv, Address, 2, Buffer);

	if (Result < 0)
		return Result;

	*Data = ((uint16_t)Buffer[0] << 8) | ((uint16_t)Buffer[1]);
	return Result;
}

static int8_t
HLW811x_ReadReg24(bDriverInterface_t *pdrv,
				  uint8_t Address, uint32_t *Data)
{
	uint8_t Buffer[3] = {0};
	int8_t Result = 0;

	Result = HLW811x_ReadReg(pdrv, Address, 3, Buffer);

	if (Result < 0)
		return Result;

	*Data = ((uint32_t)Buffer[0] << 16) | ((uint32_t)Buffer[1] << 8) |
			((uint32_t)Buffer[2]);
	return Result;
}

static int8_t
HLW811x_ReadReg32(bDriverInterface_t *pdrv,
				  uint8_t Address, uint32_t *Data)
{
	uint8_t Buffer[4] = {0};
	int8_t Result = 0;

	Result = HLW811x_ReadReg(pdrv, Address, 4, Buffer);

	if (Result < 0)
		return Result;

	*Data = ((uint32_t)Buffer[0] << 24) | ((uint32_t)Buffer[1] << 16) |
			((uint32_t)Buffer[2] << 8)  | ((uint32_t)Buffer[3]);
	return Result;
}

static int8_t
HLW811x_Command(bDriverInterface_t *pdrv, uint8_t Command)
{
	uint8_t Buffer = Command;
	return HLW811x_WriteReg(pdrv, HLW811X_COMMAND_ADDRESS, 1, &Buffer);
}

static inline int8_t
HLW811x_CommandEnableWriteOperation(bDriverInterface_t *pdrv)
{
	return HLW811x_Command(pdrv, HLW811X_COMMAND_WRITE_ENABLE);
}

static inline int8_t
HLW811x_CommandCloseWriteOperation(bDriverInterface_t *pdrv)
{
	return HLW811x_Command(pdrv, HLW811X_COMMAND_WRITE_CLOSE);
}

static inline int8_t
HLW811x_CommandReset(bDriverInterface_t *pdrv)
{
	int8_t Result = 0;
	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_Command(pdrv, HLW811X_COMMAND_RESET);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}

static int32_t
HLW811x_24BitTo32Bit(uint32_t Data)
{
	int32_t Res = 0;

	Data &= 0x00FFFFFF;

	// convert three bytes of 2s complement into 24 bit signed integer
	if (Data & 0x00800000)
		Res = 0xFF000000;

	Res |= Data;

	// Res is now a 32-bit signed integer
	return Res;
}

/**
 ==================================================================================
                            ##### Public Functions #####
 ==================================================================================
 */

/**
 * @brief  Initializer function
 * @param  Handler: Pointer to handler
 * @param  Device: Device type
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_Init(bDriverInterface_t *pdrv, HLW811x_Device_t Device)
{
	if (!pdrv)
		return HLW811X_INVALID_PARAM;

	return HLW811X_OK;
}

/**
 * @brief  Deinitialize function
 * @param  Handler: Pointer to handler
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_DeInit(bDriverInterface_t *pdrv)
{
	if (!pdrv)
		return HLW811X_INVALID_PARAM;

	return HLW811X_OK;
}

/**
 * @brief  Low level read register function
 * @param  Handler: Pointer to handler
 * @param  RegAddr: Register address
 * @param  Data: Pointer to buffer to store data
 * @param  Len: Register data length in bytes
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_ReadRegLL(bDriverInterface_t *pdrv,
				  uint8_t RegAddr,
				  uint8_t *Data,
				  uint8_t Len)
{
	if (!pdrv || !Data)
		return HLW811X_INVALID_PARAM;

	if (Len == 0)
		return HLW811X_INVALID_PARAM;

	return ((HLW811x_ReadReg(pdrv, RegAddr, Len, Data) >= 0) ? HLW811X_OK : HLW811X_FAIL);
}

/**
 * @brief  Low level write register function
 * @param  Handler: Pointer to handler
 * @param  RegAddr: Register address
 * @param  Data: Pointer to data to write
 * @param  Len: Register data length in bytes
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_WriteRegLL(bDriverInterface_t *pdrv,
				   uint8_t RegAddr,
				   uint8_t *Data,
				   uint8_t Len)
{
	int8_t Result = 0;

	if (!pdrv || !Data)
		return HLW811X_INVALID_PARAM;

	if (Len == 0)
		return HLW811X_INVALID_PARAM;

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg(pdrv, RegAddr, Len, Data);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the ratio of the resistors for current channel A
 * @note   This ration mentioned in the datasheet as K1
 * @param  Handler: Pointer to handler
 * @param  KIA: Ratio of the resistors for current channel A
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 */
HLW811x_Result_t
HLW811x_SetResRatioIA(bDriverInterface_t *pdrv, float KIA)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);
	_priv->ResCoef.KIA = KIA;
	return HLW811X_OK;
}


/**
 * @brief  Set the ratio of the resistors for current channel B
 * @note   This ration mentioned in the datasheet as K1
 * @param  Handler: Pointer to handler
 * @param  KIA: Ratio of the resistors for current channel B
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 */
HLW811x_Result_t
HLW811x_SetResRatioIB(bDriverInterface_t *pdrv, float KIB)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);
	_priv->ResCoef.KIB = KIB;
	return HLW811X_OK;
}


/**
 * @brief  Set the ratio of the resistors for voltage channel
 * @note   This ration mentioned in the datasheet as K2
 * @param  Handler: Pointer to handler
 * @param  KU: Ratio of the resistors for voltage channel
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 */
HLW811x_Result_t
HLW811x_SetResRatioU(bDriverInterface_t *pdrv, float KU)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);
	_priv->ResCoef.KU = KU;
	return HLW811X_OK;
}


/**
 * @brief  Set the oscillator frequency
 * @param  Handler: Pointer to handler
 * @param  Freq: Frequency in Hz
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 */
HLW811x_Result_t
HLW811x_SetCLKFreq(bDriverInterface_t *pdrv, uint32_t Freq)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);
	_priv->CLKI = Freq;
	return HLW811X_OK;
}


/**
 * @brief  Set current channel for special measurements
 * @note   The selected channel will be used for special measurements such as
 *         apparent power, power factor, phase angle, instantaneous
 *         apparent power and active power overload
 *
 * @param  Handler: Pointer to handler
 * @param  Channel: Current channel
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetSpecialMeasurementChannel(bDriverInterface_t *pdrv,
									 HLW811x_CurrentChannel_t Channel)
{
	int8_t Result = 0;
	uint8_t Command = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	switch (Channel)
	{
		case HLW811X_CURRENT_CHANNEL_A:
			_priv->CurrentChannel = HLW811X_CURRENT_CHANNEL_A;
			Command = HLW811X_COMMAND_CHANNELA;
			break;

		case HLW811X_CURRENT_CHANNEL_B:
			_priv->CurrentChannel = HLW811X_CURRENT_CHANNEL_B;
			Command = HLW811X_COMMAND_CHANNELB;
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_Command(pdrv, Command);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Enable/Disable the channels
 * @param  Handler: Pointer to handler
 * @param  U: Voltage channel
 * @param  IA: Current channel A
 * @param  IB: Current channel B
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetChannelOnOff(bDriverInterface_t *pdrv,
						HLW811x_EnDis_t U, HLW811x_EnDis_t IA, HLW811x_EnDis_t IB)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_SYSCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (U == HLW811X_ENDIS_ENABLE)
	{
		Reg |= (1 << HLW811X_REG_SYSCON_ADC3ON);
	}
	else if (U == HLW811X_ENDIS_DISABLE)
	{
		Reg &= ~(1 << HLW811X_REG_SYSCON_ADC3ON);
	}

	if (IA == HLW811X_ENDIS_ENABLE)
	{
		Reg |= (1 << HLW811X_REG_SYSCON_ADC1ON);
	}
	else if (IA == HLW811X_ENDIS_DISABLE)
	{
		Reg &= ~(1 << HLW811X_REG_SYSCON_ADC1ON);
	}

	if (IB == HLW811X_ENDIS_ENABLE)
	{
		Reg |= (1 << HLW811X_REG_SYSCON_ADC2ON);
	}
	else if (IB == HLW811X_ENDIS_DISABLE)
	{
		Reg &= ~(1 << HLW811X_REG_SYSCON_ADC2ON);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_SYSCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the PGA gain
 * @param  Handler: Pointer to handler
 * @param  U: PGA gain for voltage channel
 * @param  IA: PGA gain for current channel A
 * @param  IB: PGA gain for current channel B
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetPGA(bDriverInterface_t *pdrv,
			   HLW811x_PGA_t U, HLW811x_PGA_t IA, HLW811x_PGA_t IB)
{
	int8_t Result = 0;
	uint16_t Reg = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_SYSCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (U < HLW811X_PGA_NONE)
	{
		Reg &= ~(0x07 << HLW811X_REG_SYSCON_PGAU);
		Reg |= (HLW811X_PGA_TABLE[U] << HLW811X_REG_SYSCON_PGAU);
	}

	if (IA < HLW811X_PGA_NONE)
	{
		Reg &= ~(0x07 << HLW811X_REG_SYSCON_PGAIA);
		Reg |= (HLW811X_PGA_TABLE[IA] << HLW811X_REG_SYSCON_PGAIA);
	}

	if (IB < HLW811X_PGA_NONE)
	{
		Reg &= ~(0x07 << HLW811X_REG_SYSCON_PGAIB);
		Reg |= (HLW811X_PGA_TABLE[IB] << HLW811X_REG_SYSCON_PGAIB);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_SYSCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->PGA.U = U;
	_priv->PGA.IA = IA;
	_priv->PGA.IB = IB;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}
/**
 * @brief  Set the comparator on off
 * @param  Handler: Pointer to handler
 * @param  Method: Active power calculation method
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetComparatorOff(bDriverInterface_t *pdrv,
						 HLW811x_ComparatorOff_t OnOff)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	//  if (OnOff > 2)
	//    return HLW811X_INVALID_PARAM;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Reg &= ~(0x01 << HLW811X_REG_EMUCON_comp_off);
	Reg |= (OnOff << HLW811X_REG_EMUCON_comp_off);

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}

/**
 * @brief  Set the active power calculation method
 * @param  Handler: Pointer to handler
 * @param  Method: Active power calculation method
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetActivePowCalcMethod(bDriverInterface_t *pdrv,
							   HLW811x_ActivePowCalcMethod_t Method)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	if (Method > 3)
		return HLW811X_INVALID_PARAM;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Reg &= ~(0x03 << HLW811X_REG_EMUCON_Pmode);
	Reg |= (Method << HLW811X_REG_EMUCON_Pmode);

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the RMS calculation mode
 * @param  Handler: Pointer to handler
 * @param  Mode: RMS calculation mode
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetRMSCalcMode(bDriverInterface_t *pdrv, HLW811x_RMSCalcMode_t Mode)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (Mode == HLW811X_RMS_CALC_MODE_NORMAL)
		Reg &= ~(1 << HLW811X_REG_EMUCON_DC_MODE);
	else
		Reg |= (1 << HLW811X_REG_EMUCON_DC_MODE);


	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the zero crossing detection mode
 * @param  Handler: Pointer to handler
 * @param  Mode: Zero crossing detection mode
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetZeroCrossing(bDriverInterface_t *pdrv,
						HLW811x_ZeroCrossingMode_t Mode)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Mode)
	{
		case HLW811X_ZERO_CROSSING_MODE_POSITIVE:
			Reg &= ~(1 << HLW811X_REG_EMUCON_ZXD0);
			Reg &= ~(1 << HLW811X_REG_EMUCON_ZXD1);
			break;

		case HLW811X_ZERO_CROSSING_MODE_NEGATIVE:
			Reg |= (1 << HLW811X_REG_EMUCON_ZXD0);
			Reg &= ~(1 << HLW811X_REG_EMUCON_ZXD1);
			break;

		case HLW811X_ZERO_CROSSING_MODE_BOTH:
			Reg &= ~(1 << HLW811X_REG_EMUCON_ZXD0);
			Reg |= (1 << HLW811X_REG_EMUCON_ZXD1);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the high pass filter for digital signal
 * @param  Handler: Pointer to handler
 * @param  U: Voltage channel
 * @param  IA: Current channel A
 * @param  IB: Current channel B
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetDigitalHighPassFilter(bDriverInterface_t *pdrv,
								 HLW811x_EnDis_t U, HLW811x_EnDis_t IA, HLW811x_EnDis_t IB)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (U == HLW811X_ENDIS_ENABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON_HPFUOFF);
	}
	else if (U == HLW811X_ENDIS_DISABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON_HPFUOFF);
	}

	if (IA == HLW811X_ENDIS_ENABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON_HPFIAOFF);
	}
	else if (IA == HLW811X_ENDIS_DISABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON_HPFIAOFF);
	}

	if (IB == HLW811X_ENDIS_ENABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON_HPFIBOFF);
	}
	else if (IB == HLW811X_ENDIS_DISABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON_HPFIBOFF);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the PFB and PFA pulses
 * @param  Handler: Pointer to handler
 * @param  PFA: PFA pulse
 * @param  PFB: PFB pulse
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetPFPulse(bDriverInterface_t *pdrv,
				   HLW811x_EnDis_t PFA, HLW811x_EnDis_t PFB)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (PFA == HLW811X_ENDIS_ENABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON_PARUN);
	}
	else if (PFA == HLW811X_ENDIS_DISABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON_PARUN);
	}

	if (PFB == HLW811X_ENDIS_ENABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON_PBRUN);
	}
	else if (PFB == HLW811X_ENDIS_DISABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON_PBRUN);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set open drain for SDO pin
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable open drain for SDO pin
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetSDOPinOpenDrain(bDriverInterface_t *pdrv, HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_SDOCmos);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_SDOCmos);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the energy clearance
 * @param  Handler: Pointer to handler
 * @param  PA: Energy clearance for channel A
 * @param  PB: Energy clearance for channel B
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetEnergyClearance(bDriverInterface_t *pdrv,
						   HLW811x_EnDis_t PA, HLW811x_EnDis_t PB)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (PA == HLW811X_ENDIS_ENABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON2_EPB_CA);
	}
	else if (PA == HLW811X_ENDIS_DISABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON2_EPB_CA);
	}

	if (PB == HLW811X_ENDIS_ENABLE)
	{
		Reg &= ~(1 << HLW811X_REG_EMUCON2_EPB_CB);
	}
	else if (PB == HLW811X_ENDIS_DISABLE)
	{
		Reg |= (1 << HLW811X_REG_EMUCON2_EPB_CB);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the data update frequency
 * @param  Handler: Pointer to handler
 * @param  Freq: Data update frequency
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetDataUpdateFreq(bDriverInterface_t *pdrv,
						  HLW811x_DataUpdateFreq_t Freq)
{
	int8_t Result = 0;
	uint16_t Reg = 0;
	uint16_t Mask = 0;

	switch (Freq)
	{
		case HLW811X_DATA_UPDATE_FREQ_3_4HZ:
			Mask = 0;
			break;

		case HLW811X_DATA_UPDATE_FREQ_6_8HZ:
			Mask = (1 << HLW811X_REG_EMUCON2_DUPSEL);
			break;

		case HLW811X_DATA_UPDATE_FREQ_13_65HZ:
			Mask = (2 << HLW811X_REG_EMUCON2_DUPSEL);
			break;

		case HLW811X_DATA_UPDATE_FREQ_27_3HZ:
			Mask = (3 << HLW811X_REG_EMUCON2_DUPSEL);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Reg &= ~(3 << HLW811X_REG_EMUCON2_DUPSEL);
	Reg |= Mask;

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the power factor functionality
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable power factor functionality
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetPowerFactorFunctionality(bDriverInterface_t *pdrv,
									HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_PfactorEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_PfactorEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the waveform data
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable waveform data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetWaveformData(bDriverInterface_t *pdrv,
						HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_WaveEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_WaveEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the voltage sag detection
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable voltage sag detection
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetVoltageSagDetection(bDriverInterface_t *pdrv,
							   HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_SAGEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_SAGEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the over voltage, current and load detection
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable over voltage and current detection
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetOverVolCarDetection(bDriverInterface_t *pdrv,
							   HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_OverEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_OverEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set zero crossing detection
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable zero crossing detection
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetZeroCrossingDetection(bDriverInterface_t *pdrv,
								 HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_ZxEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_ZxEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set the peak detection
 * @param  Handler: Pointer to handler
 * @param  Enable: Enable/Disable peak detection
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetPeakDetection(bDriverInterface_t *pdrv,
						 HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_PeakEN);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_PeakEN);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}

HLW811x_Result_t
HLW811x_SetInternalVref(bDriverInterface_t *pdrv,
						HLW811x_EnDis_t Enable)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	switch (Enable)
	{
		case HLW811X_ENDIS_ENABLE:
			Reg |= (1 << HLW811X_REG_EMUCON2_VrefSel);
			break;

		case HLW811X_ENDIS_DISABLE:
			Reg &= ~(1 << HLW811X_REG_EMUCON2_VrefSel);
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_EMUCON2, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}

/**
 * @brief  Set INT1 and INT2 pins functionality
 * @param  Handler: Pointer to handler
 * @param  INT1: INT1 pin functionality
 * @param  INT2: INT2 pin functionality
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_SetIntOutFunc(bDriverInterface_t *pdrv,
					  HLW811x_IntOutFunc_t INT1, HLW811x_IntOutFunc_t INT2)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_INT, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (INT1 < HLW811X_INTOUT_FUNC_NO_CHANGE)
	{
		Reg &= ~(0x0F << HLW811X_REG_INT_P1sel);
		Reg |= ((INT1 & 0x0F) << HLW811X_REG_INT_P1sel);
	}

	if (INT2 < HLW811X_INTOUT_FUNC_NO_CHANGE)
	{
		Reg &= ~(0x0F << HLW811X_REG_INT_P2sel);
		Reg |= ((INT2 & 0x0F) << HLW811X_REG_INT_P2sel);
	}

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_INT, Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Set interrupt mask
 * @param  Handler: Pointer to handler
 * @param  Mask: Interrupt mask value
 * @note   Use the define HLW811X_REG_IE_XXX to set the mask value
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_SetIntMask(bDriverInterface_t *pdrv, uint16_t Mask)
{
	int8_t Result = 0;

	Result = HLW811x_CommandEnableWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_WriteReg16(pdrv, HLW811X_REG_ADDR_IE, Mask);

	if (Result < 0)
		return HLW811X_FAIL;

	Result = HLW811x_CommandCloseWriteOperation(pdrv);

	if (Result < 0)
		return HLW811X_FAIL;

	return HLW811X_OK;
}


/**
 * @brief  Get interrupt status flag
 * @note   Use the define HLW811X_REG_IF_XXX to check the flag
 * @param  Handler: Pointer to handler
 * @param  Mask: Pointer to interrupt mask value
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetIntFlag(bDriverInterface_t *pdrv, uint16_t *Mask)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_IF, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (Mask)
		*Mask = Reg;

	return HLW811X_OK;
}


/**
 * @brief  Get reset interrupt status flag
 * @note   Use the define HLW811X_REG_RIF_XXX to check the flag
 * @param  Handler: Pointer to handler
 * @param  Mask: Pointer to interrupt mask value
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetResIntFlag(bDriverInterface_t *pdrv, uint16_t *Mask)
{
	int8_t Result = 0;
	uint16_t Reg = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_RIF, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	if (Mask)
		*Mask = Reg;

	return HLW811X_OK;
}


/**
 * @brief  Get the RMS value of the voltage in V
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetRmsU(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	float ResCoef = 0;
	uint8_t PGA = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_RmsU, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;
//	b_log("HLW811x_GetRmsU reg: %d \r\n",Reg);
	RawValue = HLW811x_24BitTo32Bit(Reg);
	CoefReg = _priv->CoefReg.RmsUC;
	ResCoef = _priv->ResCoef.KU;
	PGA = (1 << _priv->PGA.U);
	*Data = (float)RawValue * (CoefReg / 4194304.0f / ResCoef / 100 / PGA);

	return HLW811X_OK;
}


/**
 * @brief  Get the RMS value of the current channel A in A
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetRmsIA(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	float ResCoef = 0;
	uint8_t PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_RmsIA, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValue = HLW811x_24BitTo32Bit(Reg);
	CoefReg = _priv->CoefReg.RmsIAC;
	ResCoef = _priv->ResCoef.KIA;
	PGA = 16 >> _priv->PGA.IA;
	DoubleBuffer = (double)RawValue * (CoefReg / 8388608.0 / ResCoef / 1000 * PGA);
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get the RMS value of the current channel B in A
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetRmsIB(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	float ResCoef = 0;
	uint8_t PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_RmsIB, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValue = HLW811x_24BitTo32Bit(Reg);
	CoefReg = _priv->CoefReg.RmsIBC;
	ResCoef = _priv->ResCoef.KIB;
	PGA = 16 >> _priv->PGA.IB;
	DoubleBuffer = (double)RawValue * (CoefReg / 8388608.0 / ResCoef / 10000 * PGA);
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get active power of channel A in W
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetPowerPA(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	double ResCoef = 0;
	double PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg32(pdrv, HLW811X_REG_ADDR_PowerPA, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValue = *((int32_t*)&Reg);
	CoefReg = _priv->CoefReg.PowerPAC;
	PGA = 16 >> (_priv->PGA.U + _priv->PGA.IA);
	ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIA;
	DoubleBuffer = (double)RawValue * (CoefReg / 2147483648.0 / ResCoef * PGA);
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get active power of channel B in W
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetPowerPB(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	double ResCoef = 0;
	double PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg32(pdrv, HLW811X_REG_ADDR_PowerPB, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValue = *((int32_t*)&Reg);
	CoefReg = _priv->CoefReg.PowerPBC;
	PGA = 16 >> (_priv->PGA.U + _priv->PGA.IB);
	ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIB;
	DoubleBuffer = (double)RawValue * (CoefReg / 2147483648.0 / ResCoef * PGA);
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get the apparent power of selected channel in W
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetPowerS(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t Reg = 0;
	int32_t RawValue = 0;
	uint16_t CoefReg = 0;
	double ResCoef = 0;
	double PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg32(pdrv, HLW811X_REG_ADDR_PowerS, &Reg);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValue = *((int32_t*)&Reg);
	CoefReg = _priv->CoefReg.PowerSC;

	switch (_priv->CurrentChannel)
	{

		case HLW811X_CURRENT_CHANNEL_A:
			PGA = 16 >> (_priv->PGA.U + _priv->PGA.IA);
			ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIA;
			break;

		case HLW811X_CURRENT_CHANNEL_B:
			PGA = 16 >> (_priv->PGA.U + _priv->PGA.IB);
			ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIB;
			break;

		default:
			return HLW811X_INVALID_PARAM;

	}

	DoubleBuffer = (double)RawValue * (CoefReg / 2147483648.0 / ResCoef * PGA);
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get Energy of channel A in KWH
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetEnergyA(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t RawValue = 0;
	uint16_t CoefReg = 0;
	float ResCoef = 0;
	uint16_t PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_Energy_PA, &RawValue);

	if (Result < 0)
		return HLW811X_FAIL;

	CoefReg = _priv->CoefReg.EnergyAC;
	PGA = (1 << _priv->PGA.U) * (1 << _priv->PGA.IA);
	ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIA;
	DoubleBuffer = (double)RawValue * (CoefReg / 536870912.0 / PGA / 4096 / ResCoef) * _priv->HFconst;
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get Energy of channel B in KWH
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetEnergyB(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t RawValue = 0;
	uint16_t CoefReg = 0;
	float ResCoef = 0;
	uint16_t PGA = 0;
	double DoubleBuffer = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_Energy_PB, &RawValue);

	if (Result < 0)
		return HLW811X_FAIL;

	CoefReg = _priv->CoefReg.EnergyBC;
	PGA = (1 << _priv->PGA.U) * (1 << _priv->PGA.IB);
	ResCoef = _priv->ResCoef.KU * _priv->ResCoef.KIB;
	DoubleBuffer = (double)RawValue * (CoefReg / 536870912.0 / PGA / 4096 / ResCoef) * _priv->HFconst;
	*Data = (float)DoubleBuffer;

	return HLW811X_OK;
}


/**
 * @brief  Get the frequency of the voltage channel in Hz
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetFreqU(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint16_t RawValue = 0;
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_Ufreq, &RawValue);

	if (Result < 0)
		return HLW811X_FAIL;

	*Data = _priv->CLKI / 8.0 / RawValue;

	return HLW811X_OK;
}


/**
 * @brief  Get the power factor of the selected channel
 * @note   The power factor is a value between -1 and 1
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_GetPowerFactor(bDriverInterface_t *pdrv, float *Data)
{
	int8_t Result = 0;
	uint32_t RawValue = 0;
	int32_t RawValueInt = 0;

	Result = HLW811x_ReadReg24(pdrv, HLW811X_REG_ADDR_PowerFactor, &RawValue);

	if (Result < 0)
		return HLW811X_FAIL;

	RawValueInt = HLW811x_24BitTo32Bit(RawValue);
	*Data = (float)((double)RawValueInt / 8388607.0);

	return HLW811X_OK;
}


/**
 * @brief  Get phase angle in degrees
 * @param  Handler: Pointer to handler
 * @param  Data: Pointer to store the data
 * @param  Freq: Frequency of the voltage channel in Hz
 * @note   The phase angle should be 50 or 60 Hz. The other values are invalid
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 *         - HLW811X_INVALID_PARAM: One of parameters is invalid.
 */
HLW811x_Result_t
HLW811x_GetPhaseAngle(bDriverInterface_t *pdrv, float *Data, uint32_t Freq)
{
	int8_t Result = 0;
	uint16_t RawValue = 0;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_Angle, &RawValue);

	if (Result < 0)
		return HLW811X_FAIL;

	if (Freq == 50)
		*Data = (float)RawValue * 0.0805f;
	else if (Freq == 60)
		*Data = (float)RawValue * 0.0965f;
	else
		return HLW811X_INVALID_PARAM;

	// while (*Data > 360.0f)
	//   *Data -= 360.0f;

	return HLW811X_OK;
}

/**
 * @brief  Begin function to initialize the device and set the default values
 * @note   This function must be called after initializing platform dependent and
 *         HLW811x_Init functions.
 *
 * @note   This function will read the coefficients from the device. It is
 *         mandatory to call this function before using other functions such as
 *         HLW811x_GetRmsU, HLW811x_GetRmsIx and etc.
 *
 * @param  Handler: Pointer to handler
 * @retval HLW811x_Result_t
 *         - HLW811X_OK: Operation was successful.
 *         - HLW811X_FAIL: Failed to send or receive data.
 */
HLW811x_Result_t
HLW811x_Coefficient_checksum(bDriverInterface_t *pdrv)
{
	int8_t Result = 0;
	uint16_t Reg16 = 0;
	uint16_t Checksum = 0;

	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_HFConst, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->HFconst = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_RmsIAC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.RmsIAC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_RmsIBC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.RmsIBC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_RmsUC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.RmsUC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_PowerPAC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.PowerPAC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_PowerPBC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.PowerPBC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_PowerSC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.PowerSC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EnergyAC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.EnergyAC = Reg16;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_EnergyBC, &Reg16);

	if (Result < 0)
		return HLW811X_FAIL;

	_priv->CoefReg.EnergyBC = Reg16;

	Checksum = 0xFFFF +
			   _priv->CoefReg.RmsIAC +
			   _priv->CoefReg.RmsIBC +
			   _priv->CoefReg.RmsUC +
			   _priv->CoefReg.PowerPAC +
			   _priv->CoefReg.PowerPBC +
			   _priv->CoefReg.PowerSC +
			   _priv->CoefReg.EnergyAC +
			   _priv->CoefReg.EnergyBC;
	Checksum = ~Checksum;

	Result = HLW811x_ReadReg16(pdrv, HLW811X_REG_ADDR_Coeff_chksum, &Reg16);
	if (Result < 0 || Checksum != Reg16)
		return HLW811X_FAIL;

	return HLW811X_OK;
}



HLW811x_Result_t _bHlw811xDefaultCfg(bDriverInterface_t *pdrv)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);

	_priv->ResCoef.KU = 1.0f;
	_priv->ResCoef.KIA = 1.0f;
	_priv->ResCoef.KIB = 1.0f;

	_priv->PGA.U = HLW811X_PGA_1;
	_priv->PGA.IA = HLW811X_PGA_16;
	_priv->PGA.IB = HLW811X_PGA_1;

	_priv->CLKI = 3579545;

	if (HLW811x_SetSpecialMeasurementChannel(pdrv,
			HLW811X_CURRENT_CHANNEL_A) != HLW811X_OK)
		return HLW811X_FAIL;

	//HLW811X_REG_ADDR_SYSCON
	if (HLW811x_SetChannelOnOff(pdrv,
								HLW811X_ENDIS_ENABLE, HLW811X_ENDIS_ENABLE, HLW811X_ENDIS_DISABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetPGA(pdrv,
					   _priv->PGA.U, _priv->PGA.IA, _priv->PGA.IB) != HLW811X_OK)
		return HLW811X_FAIL;

	//HLW811X_REG_ADDR_EMUCON
	if (HLW811x_SetComparatorOff(pdrv,
								 HLW811X_COMPARATOC_POS_OFF) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetActivePowCalcMethod(pdrv,
									   HLW811X_ACTIVE_POW_CALC_METHOD_POS_NEG_ALGEBRAIC) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetRMSCalcMode(pdrv,
							   HLW811X_RMS_CALC_MODE_NORMAL) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetZeroCrossing(pdrv,
								HLW811X_ZERO_CROSSING_MODE_POSITIVE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetDigitalHighPassFilter(pdrv,
										 HLW811X_ENDIS_DISABLE, HLW811X_ENDIS_DISABLE, HLW811X_ENDIS_DISABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetPFPulse(pdrv,
						   HLW811X_ENDIS_DISABLE, HLW811X_ENDIS_DISABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	//HLW811X_REG_ADDR_EMUCON2
	if (HLW811x_SetEnergyClearance(pdrv,
								   HLW811X_ENDIS_DISABLE, HLW811X_ENDIS_DISABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetDataUpdateFreq(pdrv,
								  HLW811X_DATA_UPDATE_FREQ_27_3HZ) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetPowerFactorFunctionality(pdrv,
											HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetWaveformData(pdrv,
								HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetVoltageSagDetection(pdrv,
									   HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetOverVolCarDetection(pdrv,
									   HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetZeroCrossingDetection(pdrv,
										 HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetPeakDetection(pdrv,
								 HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	if (HLW811x_SetInternalVref(pdrv,
								HLW811X_ENDIS_ENABLE) != HLW811X_OK)
		return HLW811X_FAIL;

	return HLW811X_OK;
}
//read data unit
// mV / mA
static int _bHlw811xRead(bDriverInterface_t *pdrv, uint32_t off, uint8_t *pbuf, uint32_t len)
{
	bPowerMeter_hlw811x_t hlw811x;

	if (len < sizeof(bPowerMeter_hlw811x_t))
	{
		return -1;
	}

	if(HLW811x_GetRmsU(pdrv, &hlw811x.RmsU) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetRmsIA(pdrv, &hlw811x.RmsIA) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetRmsIB(pdrv, &hlw811x.RmsIB) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetPowerPA(pdrv, &hlw811x.PowerPA) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetPowerPB(pdrv, &hlw811x.PowerPB) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetPowerS(pdrv, &hlw811x.PowerSA) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetEnergyA(pdrv, &hlw811x.EnergyA) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetEnergyB(pdrv, &hlw811x.EnergyB) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetFreqU(pdrv, &hlw811x.FreqU) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetPowerFactor(pdrv, &hlw811x.PowerFactorA) != HLW811X_OK)
	{
		return -1;
	}

	if(HLW811x_GetPhaseAngle(pdrv, &hlw811x.PhaseAngleA, 50) != HLW811X_OK)
	{
		return -1;
	}

	memcpy(pbuf, &hlw811x, sizeof(bPowerMeter_hlw811x_t));
	return sizeof(bPowerMeter_hlw811x_t);
}

//--------------------------------------------------------------------------------------------------
static int _bHlw811xCtl(bDriverInterface_t *pdrv, uint8_t cmd, void *param)
{
	bDRIVER_GET_PRIVATE(_priv, bHlw811xPrivate_t, pdrv);
//	b_log("ctl:%d\r\n", cmd);

	switch (cmd)
	{
		case bCMD_HLW811X_REG_CALLBACK:
		{
			if (param == NULL)
			{
				return -1;
			}

			bHlw811xDrvCallback_t *pcb  = (bHlw811xDrvCallback_t *)param;
			_priv->cb.cb                    = pcb->cb;
			_priv->cb.user_data             = pcb->user_data;
		}
		break;

		case bCMD_HLW811X_SOFT_RST:
		{
			HLW811x_CommandReset(pdrv);
		}
		break;

		case bCMD_HLW811X_MODE_AC:
		{
			if (HLW811x_SetRMSCalcMode(pdrv,
									   HLW811X_RMS_CALC_MODE_NORMAL) != HLW811X_OK)
				return HLW811X_FAIL;
		}
		break;

		case bCMD_HLW811X_MODE_DC:
		{
			if (HLW811x_SetRMSCalcMode(pdrv,
									   HLW811X_RMS_CALC_MODE_DC) != HLW811X_OK)
				return HLW811X_FAIL;
		}
		break;
		
		case bCMD_HLW811X_SET_RESRATIO_IA:
		{
			if (param == NULL)
			{
				return -1;
			}
			float *pcb  = (float *)param;
			if (HLW811x_SetResRatioIA(pdrv,
									   *pcb) != HLW811X_OK)
				return HLW811X_FAIL;
		}
		break;
		
		case bCMD_HLW811X_SET_RESRATIO_IB:
		{
			if (param == NULL)
			{
				return -1;
			}
			float *pcb  = (float *)param;
			if (HLW811x_SetResRatioIB(pdrv,
									   *pcb) != HLW811X_OK)
				return HLW811X_FAIL;
		}
		break;
		
		case bCMD_HLW811X_SET_RESRATIO_U:
		{
			if (param == NULL)
			{
				return -1;
			}
			float *pcb  = (float *)param;
			if (HLW811x_SetResRatioU(pdrv,
									   *pcb) != HLW811X_OK)
				return HLW811X_FAIL;
		}
		break;
		
		default:

			break;
	}

	return 0;
}

static void set_nsi8241_en1(uint8_t dev_no)
{
	switch(dev_no)
	{
		case 0 :
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN12, 	0);
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN14, 	0);	
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN15, 	1);			
			break;
		case 1 :
		case 2 :
		case 4 :
		case 5 :
		case 6 :
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN12, 	1);
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN14, 	0);	
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN15, 	0);				
			break;
		case 3 :	
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN12, 	0);
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN14, 	1);	
					bHalGpioWritePin(B_HAL_GPIOC, B_HAL_PIN15, 	0);			
			break;
		default:
			break;
	}
	bHalDelayMs(10);

}

/**
 * \}
 */

/**
 * \addtogroup LIS3DH_Exported_Functions
 * \{
 */

int bHLW811X_Init(bDriverInterface_t *pdrv)
{
	bDRIVER_STRUCT_INIT(pdrv, DRIVER_NAME, bHLW811X_Init);
	pdrv->read        = _bHlw811xRead;
	pdrv->ctl         = _bHlw811xCtl;
	pdrv->_private._p = &bHlw811xRunInfo[pdrv->drv_no];
	memset(pdrv->_private._p, 0, sizeof(bHlw811xPrivate_t));
	
	set_nsi8241_en1(pdrv->drv_no);
	
	if(HLW811x_Coefficient_checksum(pdrv))
	{
		return -1;
	}

	if(_bHlw811xDefaultCfg(pdrv))
	{
		return -1;
	}

	return 0;
}

#ifdef BSECTION_NEED_PRAGMA
#pragma section driver_init
#endif
bDRIVER_REG_INIT(B_DRIVER_HLW8112, bHLW811X_Init);
#ifdef BSECTION_NEED_PRAGMA
#pragma section
#endif
/**
 * \}
 */

/**
 * \}
 */

/**
 * \}
 */

/************************ Copyright (c) 2020 Bean *****END OF FILE****/
