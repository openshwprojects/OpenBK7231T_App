#pragma once
#ifndef HLW8112_H
#define HLW8112_H

#include "../httpserver/new_http.h"

void HLW8112_ProcessUpdate(float voltage, float current, float power,
                      float frequency, float energyWh);
void HLW8112_AppendInformationToHTTPIndexPage(http_request_t *request);
void HLW8112_SPI_Init(void);
void HLW8112_SPI_RunEverySecond(void);

// HLW8112 I2C Address (Adjust as necessary)
#define HLW8112_ADDR 0x5A

#define HLW8112_REG_SPECIAL             0xEA    // Special command operations
#define HLW8112_COMMAND_WRITE_EN        0xE5    // Special command operations
#define HLW8112_COMMAND_WRITE_PROTECT   0xDC    // Special command operations
#define HLW8112_COMMAND_SELECT_CH_A     0x5A    // Special command operations
#define HLW8112_COMMAND_SELECT_CH_B     0xA5    // Special command operations
#define HLW8112_COMMAND_RESET           0x96    // Special command operations

// Register addresses
#define HLW8112_REG_SYSCON          0x00    // System Control Register
#define HLW8112_REG_EMUCON          0x01    // Meter Control Register
#define HLW8112_REG_HFCONST         0x02    // Pulse Frequency Register
#define HLW8112_REG_PSTARTA         0x03    // Active Start Power Setting for Channel A
#define HLW8112_REG_PSTARTB         0x04    // Active Start Power Setting for Channel B
#define HLW8112_REG_PAGAIN          0x05    // Channel A Power Gain Calibration Register
#define HLW8112_REG_PBGAIN          0x06    // Channel B Power Gain Calibration Register
#define HLW8112_REG_PHASEA          0x07    // Channel A Phase Calibration Register
#define HLW8112_REG_PHASEB          0x08    // Channel B Phase Calibration Register
#define HLW8112_REG_PAOS            0x0A    // Channel A Active Power Offset Calibration
#define HLW8112_REG_PBOS            0x0B    // Channel B Active Power Offset Calibration
#define HLW8112_REG_RMSIAOS         0x0E    // Current Channel A RMS Offset Compensation
#define HLW8112_REG_RMSIBOS         0x0F    // Current Channel B RMS Offset Compensation
#define HLW8112_REG_IBGAIN          0x10    // Current Channel B Gain Settings
#define HLW8112_REG_PSGAIN          0x11    // Apparent Power Gain Calibration
#define HLW8112_REG_PSOS            0x12    // Apparent Power Offset Compensation
#define HLW8112_REG_EMUCON2         0x13    // Meter Control Register 2
#define HLW8112_REG_DCIA            0x14    // IA Channel DC Offset Correction Register
#define HLW8112_REG_DCIB            0x15    // IB Channel DC Offset Correction Register
#define HLW8112_REG_DCIC            0x16    // U Channel DC Offset Correction Register
#define HLW8112_REG_SAGCYC          0x17    // Voltage Sag Period Setting
#define HLW8112_REG_SAGLVL          0x18    // Voltage Sag Threshold Setting
#define HLW8112_REG_OVLVL           0x19    // Voltage Overvoltage Threshold Setting
#define HLW8112_REG_OIALVL          0x1A    // Current Channel A Overcurrent Threshold Setting
#define HLW8112_REG_OIBLVL          0x1B    // Current Channel B Overcurrent Threshold Setting
#define HLW8112_REG_OPLVL           0x1C    // Threshold Setting of Active Power Overload
#define HLW8112_REG_INT             0x1D    // INT1/INT2 Interrupt Setting

// Meter Parameter and Status Registers
#define HLW8112_REG_PFCntPA         0x20    // Fast Combination Active Pulse Counting of Channel A
#define HLW8112_REG_PFCntPB         0x21    // Fast Combination Active Pulse Counting of Channel B
#define HLW8112_REG_ANGLE           0x22    // Angle between Current and Voltage (Channel A/B)
#define HLW8112_REG_UFREQ           0x23    // Voltage Frequency (L Line)
#define HLW8112_REG_RMSIA           0x24    // RMS Current for Channel A (3 bytes)
#define HLW8112_REG_RMSIB           0x25    // RMS Current for Channel B (3 bytes)
#define HLW8112_REG_RMSU            0x26    // RMS Voltage (3 bytes)
#define HLW8112_REG_POWER_FACTOR    0x27    // Power Factor Register (Channel A or B)
#define HLW8112_REG_ENERGY_PA       0x28    // Channel A Active Power (reset after reading)
#define HLW8112_REG_ENERGY_PB       0x29    // Channel B Active Power (reset after reading)
#define HLW8112_REG_POWER_PA        0x2C    // Active Power of Channel A (4 bytes)
#define HLW8112_REG_POWER_PB        0x2D    // Active Power of Channel B (4 bytes)
#define HLW8112_REG_POWER_S         0x2E    // Apparent Power of Channel A/B (4 bytes)
#define HLW8112_REG_EMU_STATUS      0x2F    // Measurement Status and Check Register
#define HLW8112_REG_PEAKIA          0x30    // Peak of Current Channel A
#define HLW8112_REG_PEAKIB          0x31    // Peak of Current Channel B
#define HLW8112_REG_PEAKU           0x32    // Peak Value of Voltage Channel U
#define HLW8112_REG_INSTANTIA       0x33    // Instantaneous Value of Current Channel A
#define HLW8112_REG_INSTANTIB       0x34    // Instantaneous Value of Current Channel B
#define HLW8112_REG_INSTANTU        0x35    // Instantaneous Value of Voltage Channel
#define HLW8112_REG_WAVEIA          0x36    // Waveform of Current Channel A
#define HLW8112_REG_WAVEIB          0x37    // Waveform of Current Channel B
#define HLW8112_REG_WAVEU           0x38    // Waveform of Voltage Channel U
#define HLW8112_REG_INSTANTP        0x3C    // Instantaneous Active Power (Channel A or B)
#define HLW8112_REG_INSTANTS        0x3D    // Instantaneous Apparent Power (Channel A or B)

// Interrupt Registers
#define HLW8112_REG_IE              0x40    // Interrupt Enable Register
#define HLW8112_REG_IF              0x41    // Interrupt Flag Register
#define HLW8112_REG_RIF             0x42    // Reset Interrupt Status Register

// System Status Registers
#define HLW8112_REG_SYS_STATUS      0x43    // System Status Register
#define HLW8112_REG_RDATA           0x44    // Data Read by SPI last time
#define HLW8112_REG_WDATA           0x45    // Data Written by the last SPI

// Calibration Coefficients
#define HLW8112_REG_RMSIAC          0x70    // Current Channel A RMS Conversion Coefficient
#define HLW8112_REG_RMSIBC          0x71    // Current Channel B RMS Conversion Coefficient
#define HLW8112_REG_RMSUC           0x72    // Voltage Channel RMS Conversion Coefficient
#define HLW8112_REG_POWER_PAC       0x73    // Active Power Conversion Coefficient for Channel A
#define HLW8112_REG_POWER_PBC       0x74    // Active Power Conversion Coefficient for Channel B
#define HLW8112_REG_POWER_SC        0x75    // Apparent Power Conversion Coefficient
#define HLW8112_REG_ENERGY_AC       0x76    // Energy Conversion Coefficient for Channel A
#define HLW8112_REG_ENERGY_BC       0x77    // Energy Conversion Coefficient for Channel B

#endif // HLW8112_H
