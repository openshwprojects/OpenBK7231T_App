#pragma once
#ifndef __DRV_HLW8112_H__
#define __DRV_HLW8112_H__

#include "../httpserver/new_http.h"
#include "../obk_config.h"

typedef enum _hlw8112_reg_e
{
    // Calibration Paramters and metering control registers
    HLW8112_REG_SYSCON          = 0x00U, // Byte: 2 (RW), System Control Register
    HLW8112_REG_EMUCON          = 0x01U, // Byte: 2 (RW), Energy Measure Control Register
    HLW8112_REG_HFCONST         = 0x02U, // Byte: 2 (RW), Pulse Frequency Register
    HLW8112_REG_PSTART          = 0x03U, // Byte: 2 (RW), Active Start Power Channel A
    HLW8112_REG_PBSTART         = 0x04U, // Byte: 2 (RW), Active Start Power Channel B
    HLW8112_REG_PAGAIN          = 0x05U, // Byte: 2 (RW), Channel A Power Gain Calibration
    HLW8112_REG_PBGAIN          = 0x06U, // Byte: 2 (RW), Channel B Power Gain Calibration
    HLW8112_REG_PHASEA          = 0x07U, // Byte: 1 (RW), Channel A Phase Calibration
    HLW8112_REG_PHASEB          = 0x08U, // Byte: 1 (RW), Channel B Phase Calibration
    HLW8112_REG_PAOS            = 0x0AU, // Byte: 2 (RW), Channel A Power Offset Calibration
    HLW8112_REG_PBOS            = 0x0BU, // Byte: 2 (RW), Channel B Power Offset Calibration
    HLW8112_REG_RMSIAOS         = 0x0EU, // Byte: 2 (RW), Current Channel A RMS Offset Compensation
    HLW8112_REG_RMSIBOS         = 0x0FU, // Byte: 2 (RW), Current Channel B RMS Offset Compensation
    HLW8112_REG_IBGAIN          = 0x10U, // Byte: 2 (RW), Current Channel B Gain Settings
    HLW8112_REG_PSGAIN          = 0x11U, // Byte: 2 (RW), Apparent Power Gain Calibration
    HLW8112_REG_PSOS            = 0x12U, // Byte: 2 (RW), Apparent Power Offset Compensation
    HLW8112_REG_EMUCON2         = 0x13U, // Byte: 2 (RW), Meter Control Register 2
    HLW8112_REG_DCIA            = 0x14U, // Byte: 2 (RW), IA Channel DC Offset Correction
    HLW8112_REG_DCIB            = 0x15U, // Byte: 2 (RW), IB Channel DC Offset Correction
    HLW8112_REG_DCIC            = 0x16U, // Byte: 2 (RW), U Channel DC Offset Correction
    HLW8112_REG_SAGCYC          = 0x17U, // Byte: 2 (RW), Voltage Sag Period Setting
    HLW8112_REG_SAGLVL          = 0x18U, // Byte: 2 (RW), Voltage Sag Threshold Setting
    HLW8112_REG_OVLVL           = 0x19U, // Byte: 2 (RW), Voltage Overvoltage Threshold
    HLW8112_REG_OIALVL          = 0x1AU, // Byte: 2 (RW), Current Channel A Overcurrent Threshold
    HLW8112_REG_OIBLVL          = 0x1BU, // Byte: 2 (RW), Current Channel B Overcurrent Threshold
    HLW8112_REG_OPLVL           = 0x1CU, // Byte: 2 (RW), Active Power Overload Threshold
    HLW8112_REG_INT             = 0x1DU, // Byte: 2 (RW), INT1/INT2 Interrupt Default Output

    // Meter Parameters and Status Registers
    HLW8112_REG_PFCNTPA         = 0x20U, // Byte: 2 (RW), Channel A Fast Active Pulse Counter
    HLW8112_REG_PFCNTPB         = 0x21U, // Byte: 2 (RW), Channel B Fast Active Pulse Counter
    HLW8112_REG_ANGLE           = 0x22U, // Byte: 2 (RO), Phase Angle Register
    HLW8112_REG_UFREQ           = 0x23U, // Byte: 2 (RO), Voltage Frequency Register
    HLW8112_REG_RMSIA           = 0x24U, // Byte: 3 (RO), Current RMS Channel A
    HLW8112_REG_RMSIB           = 0x25U, // Byte: 3 (RO), Current RMS Channel B
    HLW8112_REG_RMSU            = 0x26U, // Byte: 3 (RO), Voltage RMS
    HLW8112_REG_POWERFACTOR     = 0x27U, // Byte: 3 (RO), Power Factor Register
    HLW8112_REG_ENERGY_PA       = 0x28U, // Byte: 3 (RO), Channel A Active Energy
    HLW8112_REG_ENERGY_PB       = 0x29U, // Byte: 3 (RO), Channel B Active Energy
    HLW8112_REG_POWERPA         = 0x2CU, // Byte: 4 (RO), Active Power Channel A
    HLW8112_REG_POWERPB         = 0x2DU, // Byte: 4 (RO), Active Power Channel B
    HLW8112_REG_POWERS          = 0x2EU, // Byte: 4 (RO), Apparent Power
    HLW8112_REG_EMUSTATUS       = 0x2FU, // Byte: 3 (RO), Measurement Status Register
    HLW8112_REG_PEAKIA          = 0x30U, // Byte: 3 (RO), Peak of Channel A
    HLW8112_REG_PEAKIB          = 0x31U, // Byte: 3 (RO), Peak of Channel B
    HLW8112_REG_PEAKU           = 0x32U, // Byte: 3 (RO), Voltage Peak
    HLW8112_REG_INSTANIA        = 0x33U, // Byte: 3 (RO), Instantaneous Current A
    HLW8112_REG_INSTANIB        = 0x34U, // Byte: 3 (RO), Instantaneous Current B
    HLW8112_REG_INSTANU         = 0x35U, // Byte: 3 (RO), Instantaneous Voltage
    HLW8112_REG_WAVEIA          = 0x36U, // Byte: 3 (RO), Waveform Current A
    HLW8112_REG_WAVEIB          = 0x37U, // Byte: 3 (RO), Waveform Current B
    HLW8112_REG_WAVEU           = 0x38U, // Byte: 3 (RO), Waveform Voltage
    HLW8112_REG_INSTANP         = 0x3CU, // Byte: 4 (RO), Instantaneous Active Power
    HLW8112_REG_INSTANS         = 0x3DU, // Byte: 4 (RO), Instantaneous Apparent Power

    // Interrupt Registers
    HLW8112_REG_IE              = 0x40U, // Byte: 2 (RW), Interrupt Enable Register
    HLW8112_REG_IF              = 0x41U, // Byte: 2 (RO), Interrupt Flag Register
    HLW8112_REG_RIF             = 0x42U, // Byte: 2 (RO), Reset Interrupt Status Register

    // System Status Registers
    HLW8112_REG_SYSSTATUS       = 0x43U, // Byte: 1 (RO), ystem Status Register
    HLW8112_REG_RDATA           = 0x44U, // Byte: 4 (RO), ast SPI Read Data
    HLW8112_REG_WDATA           = 0x45U, // Byte: 2 (RO), ast SPI Write Data
    HLW8112_REG_COEFF_CHKSUM    = 0x6FU, // Byte: 2 (RO), Coefficient Checksum
    HLW8112_REG_RMSIAC          = 0x70U, // Byte: 2 (RO), RMS Coefficient A
    HLW8112_REG_RMSIBC          = 0x71U, // Byte: 2 (RO), RMS Coefficient B
    HLW8112_REG_RMSUC           = 0x72U, // Byte: 2 (RO), RMS Voltage Coefficient
    HLW8112_REG_POWERPAC        = 0x73U, // Byte: 2 (RO), Power Coefficient A
    HLW8112_REG_POWERPBC        = 0x74U, // Byte: 2 (RO), Power Coefficient B
    HLW8112_REG_POWERSC         = 0x75U, // Byte: 2 (RO), Apparent Power Coefficient
    HLW8112_REG_ENERGYAC        = 0x76U, // Byte: 2 (RO), Energy Coefficient A
    HLW8112_REG_ENERGYBC        = 0x77U, // Byte: 2 (RO), Energy Coefficient B

    // Command Register
    HLW8112_REG_CMD	            = 0xEAU, // Write-only so always 0x6A
} hlw8112_reg_t;

typedef enum _hlw8112_cmd_e
{
    HLW8112_CMD_WRITE_ENABLE    = 0xE5U, // Enable write 
    HLW8112_CMD_WRITE_DISABLE   = 0xDCU, // Disable writes
    HLW8112_CMD_SETCHAN_A       = 0x5AU, // Set Channel A for power calculations
    HLW8112_CMD_SETCHAN_B       = 0xA5U, // Set Channel B for power calculations
    HLW8112_CMD_RESET_CHIP      = 0x96U, // Reset Chip
} hlw8112_cmd_t;

void HLW8112_SPI_Init(void);
void HLW8112_SPI_RunEverySecond(void);
void HLW8112_AppendInformationToHTTPIndexPage(http_request_t *request);
#if ENABLE_BL_TWIN
void HLW8112_AddCommands(void);
#endif


#endif