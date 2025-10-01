#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#ifndef __DRV_HLW8112_H__
#define __DRV_HLW8112_H__

#define HLW8112_SPI_RAWACCESS 0			//WIP 
#include "../httpserver/new_http.h"
#include "../hal/hal_flashVars.h"
#include <stdint.h>

//driver funcs
void HLW8112SPI_Init(void);
void HLW8112SPI_Stop(void);
void HLW8112_RunEverySecond(void);
void HLW8112_OnHassDiscovery(const char *topic);
void HLW8112_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

//for external use fill force write to flash if driver is running
void HLW8112_Save_Statistics();

#pragma region util macros

#define CHECK_AND_RETURN(value) \
	if (value != 0) { \
		return value; \
	}

#define READ_REG(REG,SIZE) HLW8112_ReadRegister##SIZE(HLW8112_REG_##REG, (uint##SIZE##_t *) &device.EX_REGiSTERS._##REG);

#define REG_EDIT(REG,SIZE,RO,C) appendRegEdit(request, #REG , HLW8112_REG_##REG, RO, device.EX_REGiSTERS._##REG, SIZE, C)

#pragma endregion


// constants
#define HLW8112_SPI_BAUD_RATE 			1000000 // 1000000  // 900000
#define DEFAULT_INTERNAL_CLK 			3579545UL
#define HLW8112_INVALID_REGVALUE  		1 << 23
#define HLW8112_SAVE_COUNTER 			3600 // 1 * 60 * 60;	// maybe once in a hour

#define DEFAULT_RES_KU 					1.0f
#define DEFAULT_RES_KIA 				0.2f
#define DEFAULT_RES_KIB 				0.2f




#pragma region resistors

// Register addresses
#define HLW8112_REG_SYSCON 				0x00 // System Control 	2 bytes	 0x0a04
#define HLW8112_REG_SYSCON_ADC3ON 		11
#define HLW8112_REG_SYSCON_ADC2ON 		10
#define HLW8112_REG_SYSCON_ADC1ON 		9
#define HLW8112_REG_SYSCON_PGAIB 		6
#define HLW8112_REG_SYSCON_PGAU 		3
#define HLW8112_REG_SYSCON_PGAIA 		0

#define HLW8112_REG_EMUCON 				0x01 // Energy Measure Control 2 bytes 0x0000
#define HLW8112_REG_EMUCON_TEMP_STEP 	14
#define HLW8112_REG_EMUCON_TEMP_EN 		13
#define HLW8112_REG_EMUCON_COMP_OFF 	12
#define HLW8112_REG_EMUCON_PMODE 		10
#define HLW8112_REG_EMUCON_DC_MODE 		9
#define HLW8112_REG_EMUCON_ZXD1 		8
#define HLW8112_REG_EMUCON_ZXD0 		7
#define HLW8112_REG_EMUCON_HPFIBOFF 	6
#define HLW8112_REG_EMUCON_HPFIAOFF 	5
#define HLW8112_REG_EMUCON_HPFUOFF 		4
#define HLW8112_REG_EMUCON_PBRUN 		1
#define HLW8112_REG_EMUCON_PARUN 		0

#define HLW8112_REG_HFCONST 			0x02 // Pulse Frequency 2bytes	0x1000
#define HLW8112_REG_PSTARTA 			0x03 // Active Start Power A  2 bytes 0x0060
#define HLW8112_REG_PSTARTB 			0x04 // Active Start Power B
#define HLW8112_REG_PAGAIN 				0x05 // Power Gain Calibration A 2 bytes 0x0000
#define HLW8112_REG_PBGAIN 				0x06 // Power Gain Calibration B
#define HLW8112_REG_PHASEA 				0x07 // Phase Calibration A 1 bytes 0x00
#define HLW8112_REG_PHASEB 				0x08 // Phase Calibration B
#define HLW8112_REG_PAOS 				0x0A // Active Power Offset Calibration A 2 bytes 0x0000
#define HLW8112_REG_PBOS 				0x0B // Active Power Offset Calibration B  2 bytes 0x0000
#define HLW8112_REG_RMSIAOS 			0x0E // RMS Offset Compensation A  2 bytes 0x0000
#define HLW8112_REG_RMSIBOS 			0x0F // RMS Offset Compensation B  2 bytes 0x0000
#define HLW8112_REG_IBGAIN 				0x10 // Channel B Current Gain  2 bytes 0x0000
#define HLW8112_REG_PSGAIN 				0x11 // Apparent Power Gain Calibration 2 bytes 0x0000
#define HLW8112_REG_PSOS 				0x12 // Apparent Power Offset Compensation 2 bytes 0x0000

#define HLW8112_REG_EMUCON2 			0x13 // Meter Control2 2 bytes 0x0001
#define HLW8112_REG_EMUCON2_SDOCMOS 	12
#define HLW8112_REG_EMUCON2_EPB_CB 		11
#define HLW8112_REG_EMUCON2_EPB_CA 		10
#define HLW8112_REG_EMUCON2_DUPSEL 		8
#define HLW8112_REG_EMUCON2_CHS_IB 		7
#define HLW8112_REG_EMUCON2_PFACTOREN 	6
#define HLW8112_REG_EMUCON2_WAVEEN 		5
#define HLW8112_REG_EMUCON2_SAGEN 		4
#define HLW8112_REG_EMUCON2_OVEREN 		3
#define HLW8112_REG_EMUCON2_ZXEN 		2
#define HLW8112_REG_EMUCON2_PEAKEN 		1
#define HLW8112_REG_EMUCON2_VREFSEL 	0

#define HLW8112_REG_DCIA 				0x14 // DC Offset Correction A  2 bytes 0x0000
#define HLW8112_REG_DCIB 				0x15 // DC Offset Correction B 
#define HLW8112_REG_DCIC 				0x16 // DC Offset Correction U 
#define HLW8112_REG_SAGCYC 				0x17 // Voltage Sag Period  2 bytes 0x0000
#define HLW8112_REG_SAGLVL 				0x18 // Voltage Sag Threshold  2 bytes 0x0000
#define HLW8112_REG_OVLVL 				0x19 // Overvoltage Threshold  2 bytes 0x0ffff
#define HLW8112_REG_OIALVL              0x1A // Overcurrent Threshold A 2 bytes 0x0ffff
#define HLW8112_REG_OIBLVL              0x1B // COvercurrent Threshold B 2 bytes 0x0ffff
#define HLW8112_REG_OPLVL 				0x1C // Active Power Overload Threshold  2 bytes 0x0ffff
#define HLW8112_REG_INT 				0x1D // INT1/INT2 Interrupt Setting 2 bytes 0x3210

// Meter Parameter and Status Registers
#define HLW8112_REG_PFCntPA 			0x20 // Fast Combination Active Pulse Count A 2 bytes 0x0000
#define HLW8112_REG_PFCntPB 			0x21 // Fast Combination Active Pulse Count B 2 bytes 0x0000


//readonly regss below
#define HLW8112_REG_ANGLE    			0x22 // Angle between Current and Voltage 2 bytes 0x0000
#define HLW8112_REG_UFREQ 				0x23 // Frequency 2 bytes 0x0000
#define HLW8112_REG_RMSIA 				0x24 // RMS Current A 3 bytes 0x000000
#define HLW8112_REG_RMSIB 				0x25 // RMS Current B 3 bytes 0x000000
#define HLW8112_REG_RMSU 				0x26 // RMS Voltage 3 bytes 0x000000
#define HLW8112_REG_POWER_FACTOR 		0x27 // Power Factor 3 bytes 0x7fffff
#define HLW8112_REG_ENERGY_PA   		0x28 // Active Power Energy A 3 bytes 0x000000 (reset on read )
#define HLW8112_REG_ENERGY_PB    		0x29 // Active Power Energy B
#define HLW8112_REG_POWER_PA 			0x2C // Active Power A 4 bytes 0x00000000
#define HLW8112_REG_POWER_PB 			0x2D // Active Power B
#define HLW8112_REG_POWER_S 			0x2E // Apparent Power

#define HLW8112_REG_EMUSTATUS 			0x2F // Measurement Status and Check 3 bytes 0x00b32f
#define HLW8112_REG_EMUSTATUS_CHA_SEL 	21
#define HLW8112_REG_EMUSTATUS_NOPLDB 	20
#define HLW8112_REG_EMUSTATUS_NOPLDA 	19
#define HLW8112_REG_EMUSTATUS_REVPB 	18
#define HLW8112_REG_EMUSTATUS_REVPA 	17
#define HLW8112_REG_EMUSTATUS_CHKSUMBSY 16
#define HLW8112_REG_EMUSTATUS_CHKSUM 	0

#define HLW8112_REG_PEAKIA 				0x30 // Peak Current A 3 bytes 0x000000
#define HLW8112_REG_PEAKIB 				0x31 // Peak Current B 3 bytes 0x000000
#define HLW8112_REG_PEAKU 				0x32 // Peak Voltage
#define HLW8112_REG_INSTANTIA 			0x33 // Instantaneous Current A 3 bytes 0x000000
#define HLW8112_REG_INSTANTIB 			0x34 // Instantaneous Current B
#define HLW8112_REG_INSTANTU 			0x35 // Instantaneous Voltage
#define HLW8112_REG_WAVEIA 				0x36 // Waveform of Current A 3 bytes 0x000000
#define HLW8112_REG_WAVEIB 				0x37 // Waveform of Current B
#define HLW8112_REG_WAVEU 				0x38 // Waveform of Voltage U
#define HLW8112_REG_INSTANTP 			0x3C // Instantaneous Active Power  4 bytes 0x00000000
#define HLW8112_REG_INSTANTS 			0x3D // Instantaneous Apparent Power 4 bytes 0x00000000

// Interrupt Registers
#define HLW8112_REG_IE 					0x40 // Interrupt Enable writable 2 bytes 0x0000
#define HLW8112_REG_IE_LEAKAGEIE 		15
#define HLW8112_REG_IE_ZX_UIE 			14
#define HLW8112_REG_IE_ZX_IBIE 			13
#define HLW8112_REG_IE_ZX_IAIE 			12
#define HLW8112_REG_IE_SAGIE 			11
#define HLW8112_REG_IE_OPIE 			10
#define HLW8112_REG_IE_OVIE 			9
#define HLW8112_REG_IE_OIBIE 			8
#define HLW8112_REG_IE_OIAIE 			7
#define HLW8112_REG_IE_INSTANIE 		6
#define HLW8112_REG_IE_PEBOIE 			4
#define HLW8112_REG_IE_PEAOIE 			3
#define HLW8112_REG_IE_PFBIE 			2
#define HLW8112_REG_IE_PFAIE 			1
#define HLW8112_REG_IE_DUPDIE 			0

#define HLW8112_REG_INT_P2				4
#define HLW8112_REG_INT_P1				0

#define HLW8112_REG_IF 					0x41 // Interrupt Flag  2 bytes 0x0000
#define HLW8112_REG_RIF 				0x42 // Reset Interrupt Status  2 bytes 0x0000
#define HLW8112_REG_IF_LEAKAGEIF 		15
#define HLW8112_REG_IF_ZX_UIF 			14
#define HLW8112_REG_IF_ZX_IBIF 			13
#define HLW8112_REG_IF_ZX_IAIF 			12
#define HLW8112_REG_IF_SAGIF 			11
#define HLW8112_REG_IF_OPIF 			10
#define HLW8112_REG_IF_OVIF 			9
#define HLW8112_REG_IF_OIBIF 			8
#define HLW8112_REG_IF_OIAIF 			7
#define HLW8112_REG_IF_INSTANIF 		6
#define HLW8112_REG_IF_PEBOIF 			4
#define HLW8112_REG_IF_PEAOIF 			3
#define HLW8112_REG_IF_PFBIF 			2
#define HLW8112_REG_IF_PFAIF 			1
#define HLW8112_REG_IF_DUPDIF 			0


// System Status Registers
#define HLW8112_REG_SYSSTATUS 			0x43 // System Status 1 byte
#define HLW8112_REG_SYSSTATUS_CLKSEL 	6
#define HLW8112_REG_SYSSTATUS_WREN 		5
#define HLW8112_REG_SYSSTATUS_RST 		0

#define HLW8112_REG_RDATA 				0x44 // SPI Read Data 4 bytes
#define HLW8112_REG_WDATA 				0x45 // SPI Written Data 4bytes

// Calibration Coefficients Read only
#define HLW8112_REG_COF_CHECKSUM 		0x6F // Coeff checksum 2 bytes 0xffff
#define HLW8112_REG_RMSIAC				0x70 // RMS Current A 2 bytes 0xffff
#define HLW8112_REG_RMSIBC 				0x71 // RMS Current B 2 bytes 0xffff
#define HLW8112_REG_RMSUC 				0x72 // RMS Voltage 2 bytes 0xffff
#define HLW8112_REG_POWER_PAC 			0x73 // Active Power A
#define HLW8112_REG_POWER_PBC 			0x74 // Active Power B
#define HLW8112_REG_POWER_SC 			0x75 // Apparent Power 
#define HLW8112_REG_ENERGY_AC 			0x76 // Energy  A
#define HLW8112_REG_ENERGY_BC 			0x77 // Energy B


// commands
#define HLW8112_REG_COMMAND 			0xEA // Special command operations
#define HLW8112_COMMAND_WRITE_EN 		0xE5 // Special command operations
#define HLW8112_COMMAND_WRITE_PROTECT 	0xDC // Special command operations
#define HLW8112_COMMAND_SELECT_CH_A 	0x5A // Special command operations
#define HLW8112_COMMAND_SELECT_CH_B 	0xA5 // Special command operations
#define HLW8112_COMMAND_RESET 			0x96 // Special command operations



typedef enum {
  	HLW8112_PGA_1 = 0,
  	HLW8112_PGA_2 = 1,
  	HLW8112_PGA_4 = 2,
  	HLW8112_PGA_8 = 3,
  	HLW8112_PGA_16 = 4,
} HLW8112_PGA_t;

typedef enum {
  	HLW8112_CHANNEL_A = 0,
  	HLW8112_CHANNEL_B = 1
} HLW8112_Channel_t; 

// does it work ???
typedef enum  {
  	HLW8112_ACTIVE_POW_CALC_METHOD_POS_NEG_ALGEBRAIC = 0,
  	HLW8112_ACTIVE_POW_CALC_METHOD_POS = 1,
  	HLW8112_ACTIVE_POW_CALC_METHOD_POS_NEG_ABSOLUTE = 2
} HLW8112_Power_CalcMethod_t;


/*
typedef enum {
  HLW8112_RMS_CALC_MODE_NORMAL = 0,
  HLW8112_RMS_CALC_MODE_DC = 1
} HLW8112_RMS_CalcMode_t;


typedef enum {
  HLW8112_ZX_MODE_POSITIVE = 0,
  HLW8112_ZX_MODE_NEGATIVE = 1,
  HLW8112_ZX_MODE_BOTH = 2
} HLW8112_ZX_Mode_t;


typedef enum {
  HLW8112_INTOUT_FUNC_PULSE_PFA = 0,
  HLW8112_INTOUT_FUNC_PULSE_PFB = 1,
  HLW8112_INTOUT_FUNC_LEAKAGE = 2,
  HLW8112_INTOUT_FUNC_IRQ = 3,
  HLW8112_INTOUT_FUNC_POWER_OVERLOAD = 4,
  HLW8112_INTOUT_FUNC_NEGATIVE_POWER_A = 5,
  HLW8112_INTOUT_FUNC_NEGATIVE_POWER_B = 6,
  HLW8112_INTOUT_FUNC_INSTAN_VALUE_UPDATE_INT = 7,
  HLW8112_INTOUT_FUNC_AVG_UPDATE_INT = 8,
  HLW8112_INTOUT_FUNC_VOLTAGE_ZERO_CROSSING = 9,
  HLW8112_INTOUT_FUNC_CURRENT_ZERO_CROSSING_A = 10,
  HLW8112_INTOUT_FUNC_CURRENT_ZERO_CROSSING_B = 11,
  HLW8112_INTOUT_FUNC_OVERVOLTAGE = 12,
  HLW8112_INTOUT_FUNC_UNDERVOLTAGE = 13,
  HLW8112_INTOUT_FUNC_OVERCURRENT_A = 14,
  HLW8112_INTOUT_FUNC_OVERCURRENT_B = 15,
  HLW8112_INTOUT_FUNC_NO_CHANGE = 16
} HLW8112_IntOutFunc_t;


typedef enum HLW8112_DataUpdateFreq_e {
  HLW8112_DATA_UPDATE_FREQ_3_4HZ = 0,
  HLW8112_DATA_UPDATE_FREQ_6_8HZ = 1,
  HLW8112_DATA_UPDATE_FREQ_13_65HZ = 2,
  HLW8112_DATA_UPDATE_FREQ_27_3HZ = 3
} HLW8112_DataUpdateFreq_t;


typedef enum HLW8112_EnDis_e {
  HLW8112_ENDIS_NOCHANGE = -1,
  HLW8112_ENDIS_DISABLE = 0,
  HLW8112_ENDIS_ENABLE = 1
} HLW8112_EnDis_t;
*/

typedef enum {
  	HLW8112_SAVE_NONE = 0,
  	HLW8112_SAVE_A_IMP = 1,
  	HLW8112_SAVE_A_EXP = 2,
  	HLW8112_SAVE_A = 3,
  	HLW8112_SAVE_B_IMP = 4,
  	HLW8112_SAVE_B_EXP = 8,
  	HLW8112_SAVE_B = 12,
  	HLW8112_SAVE_ALL = 15,
  	HLW8112_SAVE_FORCE = 16,
} HLW8112_SaveFlag_t;

typedef uint16_t HLW8112_SaveFlags_t;

typedef struct {
	uint32_t v_rms;  	// rms voltage
	uint16_t freq;		// frquency
	uint32_t pf;		// power factor
	uint32_t ap;		// Channel A aparent power

	uint32_t ia_rms;	// Channel A rms current 
	uint32_t pa;		// Channel A active power
	uint32_t ea;		// Channel A Energy

	uint32_t ib_rms;	// Channel B rms current 
	uint32_t pb;		// Channel B active power
	uint32_t eb;		// Channel B Energy

	uint8_t sysstat;	// System Status 0x43
	uint32_t emustat;	// Emu Status
	uint16_t int_f;		// Intrupt Status
	
} HLW8112_Data_t;



//TODO add proper units
typedef struct {
	int32_t v_rms;  	// rms voltage
	int32_t freq;		// frquency
	int16_t pf;		// power factor
	int32_t ap;		// Channel A aparent power

	int32_t ia_rms;	// Channel A rms current 
	int32_t pa;		// Channel A active power
	ENERGY_DATA* ea;		// Channel A Energy

	int32_t ib_rms;	// Channel B rms current 
	int32_t pb;		// Channel B active power
	ENERGY_DATA* eb;		// Channel B Energy
	
} HLW8112_UpdateData_t;


typedef struct {
	double i;
	double p;
	double e;
	double ap;
} HLW8112_Channel_Scale_t;

 typedef struct  {
	double v_rms;
	double freq;
	double pf;
	HLW8112_Channel_Scale_t a;
	HLW8112_Channel_Scale_t b;
} HLW8112_Scale_Factor_t;

typedef struct {
	struct {
		uint16_t RmsIAC;
		uint16_t RmsIBC;
		uint16_t RmsUC;
		uint16_t PowerPAC;
		uint16_t PowerPBC;
		uint16_t PowerSC;
		uint16_t EnergyAC;
		uint16_t EnergyBC;
	} DeviceRegisterCoeff;

	struct {
		float KU;
		float KIA;
		float KIB;
	} ResistorCoeff ;

	
	struct {
		HLW8112_PGA_t U;
		HLW8112_PGA_t IA;
		HLW8112_PGA_t IB;
	} PGA;

	uint16_t HFconst;
	uint32_t CLKI;
  	HLW8112_Channel_t MainChannel;
	HLW8112_Scale_Factor_t ScaleFactor ;

	struct 
	{
		uint32_t _SYSCON;
		uint32_t _EMUCON;
		uint32_t _HFCONST;
		uint32_t _PSTARTA;
		uint32_t _PSTARTB;
		uint32_t _PAGAIN;
		uint32_t _PBGAIN;
		uint32_t _PHASEA;
		uint32_t _PHASEB;
		uint32_t _PAOS;
		uint32_t _PBOS;
		uint32_t _RMSIAOS;
		uint32_t _RMSIBOS;
		uint32_t _IBGAIN;
		uint32_t _PSGAIN;
		uint32_t _PSOS;
		uint32_t _EMUCON2;
	} EX_REGiSTERS;
	

} HLW8112_Device_Conf_t;


typedef enum  {
  	HLW8112_Channel_Voltage = 0,
  	HLW8112_Channel_Frequency = 1,
  	HLW8112_Channel_PowerFactor = 2,
  	HLW8112_Channel_current_A = 3,
  	HLW8112_Channel_current_B = 4,
  	HLW8112_Channel_power_A = 5,
  	HLW8112_Channel_power_B = 6,
  	HLW8112_Channel_apparent_power_A = 7,
  	HLW8112_Channel_export_A = 8,
  	HLW8112_Channel_export_B = 9,
  	HLW8112_Channel_import_A = 10,
  	HLW8112_Channel_import_B = 11,
//	HLW8112_Channel_ResCof_Voltage = 12,
//	HLW8112_Channel_ResCof_A = 13,
//	HLW8112_Channel_ResCof_B = 14,
//	HLW8112_Channel_Clk = 15,
} HLW8112_Device_channels;


void HLW8112_compute_scale_factor();
void HLW8112_ScaleEnergy(HLW8112_Channel_t channel, uint32_t regValue, int32_t* value);
int HLW8112_CheckCoeffs();
int HLW8112_UpdateCoeff();

#endif // __DRV_HLW8112_H__

#pragma GCC diagnostic pop