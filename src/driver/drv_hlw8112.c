// HLW8112

// workaround for code folding region pragma remove it when compiler are updated
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas" 

#include "drv_hlw8112.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_HLW8112SPI

#include <math.h>
#include <stdint.h>
#include <inttypes.h>


#include "../cmnds/cmd_public.h"
#include "../hal/hal_flashVars.h"
#include "../hal/hal_ota.h"
#include "../hal/hal_pins.h"
#include "../httpserver/hass.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "../new_cfg.h"
#include "../new_pins.h"

#include "drv_public.h"
#include "drv_spi.h"


static const uint32_t RMS_I_RESOLUTION  = 8388608 ; // 1 << 23;
static const uint32_t RMS_U_RESOLUTION  = 4194304 ; //1 << 22;
static const uint32_t PF_RESOLUTION  = 8388607 ; //
static const uint32_t PWR_RESOLUTION  = 2147483648; // 1 << 31;
static const uint32_t E_RESOLUTION  = 536870912; // 1 << 29;

static HLW8112_Device_Conf_t device;   // coeff reg k1 k2 pga etc
static HLW8112_Data_t last_data;		// last read reg values
static HLW8112_UpdateData_t last_update_data;	// last scaled values for ext systems 


// energy stats saved/restored in/out flash
static ENERGY_DATA energy_acc_a = {
	.Import =0 , .Export = 0
}; 
static ENERGY_DATA energy_acc_b= {
	.Import =0 , .Export = 0
};;

static HLW8112_UpdateData_t last_update_data = {
	.v_rms = 0, .freq = 0, .pf = 0 , .ap = 0, 
	.ia_rms = 0, .pa =0, .ea= &energy_acc_a,
	.ib_rms = 0, .pb =0, .eb= &energy_acc_b
};	// last scaled values for ext systems 

static int stat_save_count_down = HLW8112_SAVE_COUNTER;
int GPIO_HLW_SCSN = 9;

#pragma region HLW8112 utils

#if HLW8112_SPI_RAWACCESS

void HLW8112_Print_Array(uint8_t *data, int size) {
	for (int i = 0; i <= size; i++) {
		ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_Print_Array i = %d :  v = %02hhX ", i, data[i]);
	}
}
#else
void HLW8112_Print_Array(uint8_t *data, int size) {}
#endif

static int32_t HLW8112_24BitTo32Bit(uint32_t value) {
	int32_t result = 0;
	value &= 0x00FFFFFF;
	if (value & 0x00800000)
		result = 0xFF000000;
	result |= value;
	return result;
}

#pragma endregion


#pragma region HLW8112 register ops

#pragma region HLW8112 register lowlevel ops

void HLW8112_SPI_Txn_Begin(void) {
	HAL_PIN_SetOutputValue(GPIO_HLW_SCSN, 0); 
}

void HLW8112_SPI_Txn_End(void) {
	 HAL_PIN_SetOutputValue(GPIO_HLW_SCSN, 1);
}

int HLW8112_SPI_ReadBytes(uint8_t *buffer, uint32_t size) {
	int Result = SPI_ReadBytes(buffer, size);
	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_SPI_Read  result %x ", Result);
	return Result;
}

int HLW8112_SPI_WriteBytes(uint8_t *data, uint32_t size) {
  	int Result = SPI_WriteBytes(data, size);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_SPI_Write  result %x ", Result);
  	return Result;
}

int HLW8112_SPI_Transact(uint8_t *txBuffer, uint32_t txSize, uint8_t *rxBuffer, uint32_t rxSize) {
  	HLW8112_SPI_Txn_Begin();
  	int Result = SPI_Transmit(txBuffer, txSize, rxBuffer, rxSize);
  	HLW8112_SPI_Txn_End();
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_SPI_Transact  result %d", Result);
  	return Result;
}
#pragma endregion

#pragma region read

int HLW8112_ReadRegister(uint8_t reg, uint8_t size, uint32_t *valueResult) {
  	uint8_t tx[1] = {0xFF};
  	uint8_t rx[5] = {0};
  	tx[0] = reg & 0x7F;
  	
	int result = HLW8112_SPI_Transact(tx, 1, rx, 5);
  	if (result < 0) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "HLW8112_ReadRegister non zero result %d", result);
    	return result;
  	}
  	HLW8112_Print_Array(rx, 5);
  
	uint32_t value = 0x0;
  	if (size == 4) {
    	value = ((uint32_t)rx[0] << 24) | ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | ((uint32_t)rx[3]);
  	} else if (size == 3) {
    	value = ((uint32_t)rx[0] << 16) | ((uint32_t)rx[1] << 8) | ((uint32_t)rx[2]);
  	} else if (size == 2) {
    	value = ((uint32_t)rx[0] << 8) | ((uint32_t)rx[1]);
  	} else {
    	value = ((uint32_t)rx[0]);
  	}
  	*valueResult = value;
  	return result;
}


int HLW8112_ReadRegister8(uint8_t reg, uint8_t *valueResult) {
  	uint32_t tmpValue;
  	int result = HLW8112_ReadRegister(reg, 1, &tmpValue);
  	*valueResult = (uint8_t)tmpValue;
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_ReadRegister8 reg= %02X  :  v = %02X", reg, *valueResult);
  	return result;
}

int HLW8112_ReadRegister16(uint8_t reg, uint16_t *valueResult) {
  	uint32_t tmpValue;
  	int result = HLW8112_ReadRegister(reg, 2, &tmpValue);
  	*valueResult = (uint16_t)tmpValue;
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_ReadRegister16 reg= %02X  :  v = %04X", reg, *valueResult);
  	return result;
}

int HLW8112_ReadRegister24(uint8_t reg, uint32_t *valueResult) {
	int result = HLW8112_ReadRegister(reg, 3, valueResult);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_ReadRegister24 reg= %02X  :  v = %06X", reg, *valueResult);
  	return result;
}

int HLW8112_ReadRegister32(uint8_t reg, uint32_t *valueResult) {
  	int result = HLW8112_ReadRegister(reg, 4, valueResult);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_ReadRegister32 reg= %02X  :  v = %08X", reg, *valueResult);
  	return result;
}
#pragma endregion

#pragma region write

// needs to take care of spi txn by callee
int HLW8112_WriteRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  	uint8_t tx[3] = {0};
  	if (reg == HLW8112_REG_COMMAND) {
    	tx[0] = reg;
  	} else {
    	tx[0] = reg | 0x80;
  	}
	//checks
  	if (size > 2) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "HLW8112_WriteRegister can only write 8 or 16 bit reg = %02X", reg);
    	return -2;
  	}

	// buffer prepare
  	for (uint8_t i = 0; i < size; i++) {
    	tx[1 + i] = data[i];
  	}
  	
  	int result = HLW8112_SPI_WriteBytes(tx, size + 1);
  	//TODO: verify written bytes register
  	return result;
}

uint8_t HLW8112_WriteRegisterValue(uint8_t reg, uint16_t value) {
  	uint8_t data[2] = {0};
  	data[0] = (value >> 8) & 0xFF;
  	data[1] = value & 0xFF;
  	uint8_t result = HLW8112_WriteRegister(reg, data, 2);
  	return result;
}

uint8_t HLW8112_WriteRegisterValue8(uint8_t reg, uint8_t value) {
  	uint8_t data[1] = {value};
  	uint8_t result = HLW8112_WriteRegister(reg, data, 1);
  	return result;
}

uint8_t HLW8112_SPI_WriteControl(uint8_t control) {
  	return HLW8112_WriteRegisterValue8(HLW8112_REG_COMMAND, control);
}

int writeEnable() {
  	return HLW8112_SPI_WriteControl(HLW8112_COMMAND_WRITE_EN);
}

int writeDisable() {
  	return HLW8112_SPI_WriteControl(HLW8112_COMMAND_WRITE_PROTECT);
}

uint8_t HLW8112_WriteRegister16(uint8_t reg, uint16_t value) {
  	HLW8112_SPI_Txn_Begin();
  	writeEnable();
  	
	uint8_t result = HLW8112_WriteRegisterValue(reg, value);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_WriteRegisterValue result %d", result);
  	
	writeDisable();
  	HLW8112_SPI_Txn_End();
  
	//TODO verify reg this is big no for clearing regs will need to switch last read reg
	uint16_t readValue;
  	int rresult = HLW8112_ReadRegister16(reg, &readValue);
  	if (rresult < 0) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Write Verify read result %d", rresult);
		return -2;
  	}

  	if (value != readValue) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Write Verify failed reg=%02X value=%04X rv=%04X", reg, value, readValue);
		return -3;
	}

  	return result;
}
uint8_t HLW8112_WriteRegister8(uint8_t reg, uint8_t value) {
  	HLW8112_SPI_Txn_Begin();
  	writeEnable();
  	
	uint8_t result = HLW8112_WriteRegisterValue8(reg, value);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_WriteRegisterValue8 result %d", result);
  	
	writeDisable();
  	HLW8112_SPI_Txn_End();
  
	//TODO verify reg this is big no for clearing regs will need to switch last read reg
	uint8_t readValue;
  	int rresult = HLW8112_ReadRegister8(reg, &readValue);
  	if (rresult < 0) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Write Verify read result %d", rresult);
		return -2;
  	}

  	if (value != readValue) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Write Verify failed reg=%02X value=%04X rv=%04X", reg, value, readValue);
		return -3;
	}

  	return result;
}
#pragma endregion

#pragma endregion


#pragma region flash ops
void HLW8112_Restore_Stats(void) {
	energy_acc_a.Export = 0;
	energy_acc_a.Import = 0;
	energy_acc_b.Export = 0;
	energy_acc_a.Import = 0;
	HAL_FlashVars_GetEnergy(&energy_acc_a, ENERGY_CHANNEL_A);
	HAL_FlashVars_GetEnergy(&energy_acc_b, ENERGY_CHANNEL_B);
}

void HLW8112_save_stats(HLW8112_SaveFlags_t save) {
	ADDLOG_DEBUG( LOG_FEATURE_ENERGYMETER , "HLW8112_SaveFlash_t  value = %08X", save);
	if(save > 0 ){
		//TODO write proper flash write debounce
		int pg = OTA_GetProgress();  
		// sometime this messup ota when ota is also writing to flash
		if (pg != -1)
      	{
        	ADDLOG_WARN( LOG_FEATURE_ENERGYMETER , "OTA in progress skip write to flash pg=%d", pg);
			return;
      	}
		stat_save_count_down --;
		if(stat_save_count_down <= 0 || save & HLW8112_SAVE_FORCE) {
			stat_save_count_down = HLW8112_SAVE_COUNTER;
			ENERGY_DATA* data[2] = {&energy_acc_a, &energy_acc_b};
			HAL_FlashVars_SaveEnergy(data, 2);
		}
	}
}
void HLW8112_Set_EnergyStat(HLW8112_Channel_t channel, float import, float export) {
	ENERGY_DATA* data = &energy_acc_a;
	uint16_t counter_reg = HLW8112_REG_PFCntPA;
	if (channel == HLW8112_CHANNEL_B){
		data = &energy_acc_b;
		counter_reg = HLW8112_REG_PFCntPB;
		CHANNEL_Set(HLW8112_Channel_export_B, export * 1000, 0);
		CHANNEL_Set(HLW8112_Channel_import_B, import * 1000, 0);
	}else {
		CHANNEL_Set(HLW8112_Channel_export_A, export * 1000, 0);
		CHANNEL_Set(HLW8112_Channel_import_A, import * 1000, 0);
	}

	HLW8112_WriteRegister16(counter_reg,0);
	data->Import = import;
	data->Export = export;
	HLW8112_save_stats(HLW8112_SAVE_FORCE);
}
#pragma endregion


#pragma region commands

static commandResult_t HLW8112_SetClock(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	uint32_t value = Tokenizer_GetArgInteger(0);
	device.CLKI = value;
	//CHANNEL_Set(HLW8112_Channel_Clk, value, 0 );
	CFG_SetPowerMeasurementCalibrationInteger(CFG_OBK_CLK,value);
	HLW8112_compute_scale_factor();
	return CMD_RES_OK;
}
static commandResult_t HLW8112_SetResistorGain(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	
	device.ResistorCoeff.KU = Tokenizer_GetArgFloat(0);
	device.ResistorCoeff.KIA = Tokenizer_GetArgFloat(1);
	device.ResistorCoeff.KIB = Tokenizer_GetArgFloat(2);
	
	//CHANNEL_Set(HLW8112_Channel_ResCof_Voltage, device.ResistorCoeff.KU, 0 );
	//CHANNEL_Set(HLW8112_Channel_ResCof_A, device.ResistorCoeff.KIA, 0 );
	//CHANNEL_Set(HLW8112_Channel_ResCof_B, device.ResistorCoeff.KIB, 0 );
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KU, device.ResistorCoeff.KU );
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIA, device.ResistorCoeff.KIA );
	CFG_SetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIB, device.ResistorCoeff.KIB );
	HLW8112_compute_scale_factor();
	return CMD_RES_OK;
}
static commandResult_t HLW8112_SetEnergyStat(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 4)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	HLW8112_Set_EnergyStat(HLW8112_CHANNEL_A,Tokenizer_GetArgFloat(0),Tokenizer_GetArgFloat(1));
	HLW8112_Set_EnergyStat(HLW8112_CHANNEL_B,Tokenizer_GetArgFloat(2),Tokenizer_GetArgFloat(3));
	return CMD_RES_OK;
}

static commandResult_t HLW8112_ClearEnergy(const void *context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	char* channel = Tokenizer_GetArg(0);
	if (!strcmp("channel_a" , channel)) {
		HLW8112_Set_EnergyStat(HLW8112_CHANNEL_A,0,0);
	} else if (!strcmp("channel_b" , channel)){
		HLW8112_Set_EnergyStat(HLW8112_CHANNEL_B,0,0);
	}
	return CMD_RES_OK;
}

#if HLW8112_SPI_RAWACCESS
static commandResult_t HLW8112_write_reg(const void *context, const char *cmd, const char *args, int cmdFlags) {

  	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
    	return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  	}
  	int reg = Tokenizer_GetArgInteger(0);
  	if (reg < 0 || reg > 255) {
    	return CMD_RES_BAD_ARGUMENT;
  	}
  	int val = Tokenizer_GetArgInteger(1);
  	if (val > 65535) {
    	return CMD_RES_BAD_ARGUMENT;
  	}
  	int result = HLW8112_WriteRegister16((uint8_t)reg, (uint16_t)val);
  	ADDLOG_INFO(LOG_FEATURE_CMD, "HLW8112_write_reg result %d", result);

  	int cr = HLW8112_CheckCoeffs();
  	if (cr > 0) {
    	HLW8112_UpdateCoeff();
  	}
  return CMD_RES_OK;
}
static commandResult_t HLW8112_read_reg(const void *context, const char *cmd, const char *args, int cmdFlags) {

  	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
    	return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  	}
  	int reg = Tokenizer_GetArgInteger(0);
  	if (reg < 0 || reg > 255) {
    	return CMD_RES_BAD_ARGUMENT;
  	}
  	int width = Tokenizer_GetArgInteger(1);
  	if (width > 32) {
    	return CMD_RES_BAD_ARGUMENT;
  	}
  	uint32_t val;
  	int result;
  	if (width == 8) {
    	result = HLW8112_ReadRegister8((uint8_t)reg, (uint8_t *)&val);
  	}
	else if (width == 16) {
    	result = HLW8112_ReadRegister16((uint8_t)reg, (uint16_t *)&val);
  	}
	else if (width == 24) {
    	result = HLW8112_ReadRegister24((uint8_t)reg, &val);
  	}
  	else if (width == 32) {
    	result = HLW8112_ReadRegister32((uint8_t)reg, &val);
  	}
	else {
    	return CMD_RES_BAD_ARGUMENT;
  	}
  	ADDLOG_INFO( LOG_FEATURE_CMD , "HLW8112_read_reg result %d reg = %02X value = %08X", result, reg , val);

  	return CMD_RES_OK;
}
static commandResult_t HLW8112_print_coeff(const void *context, const char *cmd, const char *args, int cmdFlags) {

  	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 0)) {
    	return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  	}
  	HLW8112_UpdateCoeff();
	return CMD_RES_OK;
}


static commandResult_t HLW8112_c(const void *context, const char *cmd, const char *args, int cmdFlags) {

  	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
    	return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  	}
  	energy_acc_a.Export =  Tokenizer_GetArgFloat(0);
  	HLW8112_save_stats(HLW8112_SAVE_A_EXP);
	return CMD_RES_OK;
}

static commandResult_t HLW8112_a(const void *context, const char *cmd, const char *args, int cmdFlags) {

  	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 0)) {
    	return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  	}
  	HLW8112_Restore_Stats();
  	ADDLOG_INFO( LOG_FEATURE_CMD , "HLW8112_a HLW8112_Restore_Stats exp = %.4f imp = %.4f", energy_acc_a.Export, energy_acc_a.Import);
  	return CMD_RES_OK;
}
#endif

void HLW8112_addCommads(void){
    CMD_RegisterCommand("HLW8112_SetClock", HLW8112_SetClock, NULL);
    CMD_RegisterCommand("HLW8112_SetResistorGain", HLW8112_SetResistorGain, NULL);
    CMD_RegisterCommand("HLW8112_SetEnergyStat", HLW8112_SetEnergyStat, NULL);
    CMD_RegisterCommand("clear_energy", HLW8112_ClearEnergy, NULL);
#if HLW8112_SPI_RAWACCESS
	CMD_RegisterCommand("HLW8112_write_reg", HLW8112_write_reg, NULL);
	CMD_RegisterCommand("HLW8112_read_reg", HLW8112_read_reg, NULL);
	CMD_RegisterCommand("HLW8112_print_coeff", HLW8112_print_coeff, NULL);
	CMD_RegisterCommand("HLW8112_c", HLW8112_c, NULL);
	CMD_RegisterCommand("HLW8112_a", HLW8112_a, NULL);
#endif
}

#pragma endregion



#pragma region HLW8112 init

void HLW8112_Shutdown_Pins() {

}
void HLW8112_Init_Channels() {
	CHANNEL_SetType(HLW8112_Channel_Voltage, ChType_Voltage_div100);
	CHANNEL_SetType(HLW8112_Channel_Frequency, ChType_Frequency_div100);
	CHANNEL_SetType(HLW8112_Channel_PowerFactor, ChType_PowerFactor_div1000);
	CHANNEL_SetType(HLW8112_Channel_current_B, ChType_Current_div1000);
	CHANNEL_SetType(HLW8112_Channel_current_A, ChType_Current_div1000);
	CHANNEL_SetType(HLW8112_Channel_power_B, ChType_Power_div100);
	CHANNEL_SetType(HLW8112_Channel_power_A, ChType_Power_div100);
	CHANNEL_SetType(HLW8112_Channel_apparent_power_A, ChType_Power_div100);
	CHANNEL_SetType(HLW8112_Channel_export_B, ChType_EnergyExport_kWh_div1000);
	CHANNEL_SetType(HLW8112_Channel_export_A, ChType_EnergyExport_kWh_div1000);
	CHANNEL_SetType(HLW8112_Channel_import_B, ChType_EnergyImport_kWh_div1000);
	CHANNEL_SetType(HLW8112_Channel_import_A, ChType_EnergyImport_kWh_div1000);
	//CHANNEL_SetType(HLW8112_Channel_ResCof_Voltage, ChType_Custom);
	//CHANNEL_SetType(HLW8112_Channel_ResCof_A, ChType_Custom);
	//CHANNEL_SetType(HLW8112_Channel_ResCof_B, ChType_Custom);
	//CHANNEL_SetType(HLW8112_Channel_Clk, ChType_Custom);

	CHANNEL_SetLabel(HLW8112_Channel_PowerFactor , "Power Factor", 1);
	CHANNEL_SetLabel(HLW8112_Channel_current_B , "Current B", 1);
	CHANNEL_SetLabel(HLW8112_Channel_current_A , "Current A", 1);
	CHANNEL_SetLabel(HLW8112_Channel_power_B , "Power B", 1);
	CHANNEL_SetLabel(HLW8112_Channel_power_A , "Power A", 1);
	CHANNEL_SetLabel(HLW8112_Channel_apparent_power_A , "Apparent Power", 1);
	CHANNEL_SetLabel(HLW8112_Channel_export_B , "Energy Export B", 1);
	CHANNEL_SetLabel(HLW8112_Channel_export_A , "Energy Export A", 1);
	CHANNEL_SetLabel(HLW8112_Channel_import_A , "Energy Import A", 1);
	CHANNEL_SetLabel(HLW8112_Channel_import_B , "Energy Import B", 1);
	//CHANNEL_SetLabel(HLW8112_Channel_ResCof_Voltage, "Voltage Resistor Ratio",1 );
	//CHANNEL_SetLabel(HLW8112_Channel_ResCof_A, "Channel A Resistor Ratio", 1);
	//CHANNEL_SetLabel(HLW8112_Channel_ResCof_B, "Channel b Resistor Ratio",1 );
	//CHANNEL_SetLabel(HLW8112_Channel_ResCof_B, "CLK",1 );

}

void HLW8112_Init_Pins() {

	//TODO INT1/INT2
  	int tmp;
  	tmp = PIN_FindPinIndexForRole(IOR_HLW8112_SCSN, -1);
  	if (tmp != -1) {
    	GPIO_HLW_SCSN = tmp;
  	} else {
    	GPIO_HLW_SCSN = PIN_FindPinIndexForRole(IOR_HLW8112_SCSN, GPIO_HLW_SCSN);
  	}
  
  	HAL_PIN_Setup_Output(GPIO_HLW_SCSN);
  	HAL_PIN_SetOutputValue(GPIO_HLW_SCSN, 1);

}

#pragma region register init and ops


int HLW8112_UpdateCoeffFromRegister(uint16_t reg, uint16_t* sink, char* name) {
	uint16_t regValue;
	int cmdResult;
	cmdResult = HLW8112_ReadRegister16(reg, &regValue);
	ADDLOG_INFO(LOG_FEATURE_ENERGYMETER, "HLW8112_UpdateCoeff %s %i value = %04X",name, cmdResult, regValue);
	if (cmdResult < 0)
		return cmdResult;
	*sink = regValue;
	return 0;
}

void HLW8112_compute_scale_factor() {
	double k2 = device.ResistorCoeff.KU;
	double k1a = device.ResistorCoeff.KIA;
	double k1b = device.ResistorCoeff.KIB;

	double v = (double)device.DeviceRegisterCoeff.RmsUC *10 / (k2 * RMS_U_RESOLUTION);  // mV
	double frq = (double) device.CLKI * 100 / 8;
	double pf =  (double) 1000.0 / ( (double) PF_RESOLUTION );


	double ia = (double) device.DeviceRegisterCoeff.RmsIAC / (k1a * RMS_I_RESOLUTION); // ma
	double ib = (double) device.DeviceRegisterCoeff.RmsIBC / (k1b * RMS_I_RESOLUTION);

	double pa = (double) device.DeviceRegisterCoeff.PowerPAC * 1000 / (k1a * k2 * PWR_RESOLUTION);
	double pb = (double) device.DeviceRegisterCoeff.PowerPBC * 1000 / (k1b * k2 * PWR_RESOLUTION);
	double apa = (double) device.DeviceRegisterCoeff.PowerSC * 1000 / (k1a * k2 * PWR_RESOLUTION);
	double apb = (double) device.DeviceRegisterCoeff.PowerSC * 1000 / (k1b * k2 * PWR_RESOLUTION);

	double ea = (double) device.DeviceRegisterCoeff.EnergyAC * 10000000 / (k1a * k2 * E_RESOLUTION);
	double eb = (double) device.DeviceRegisterCoeff.EnergyAC * 10000000 / (k1b * k2 * E_RESOLUTION);

	device.ScaleFactor.v_rms = v;
	device.ScaleFactor.freq = frq;
	device.ScaleFactor.pf = pf;
	device.ScaleFactor.a.i = ia;
	device.ScaleFactor.a.p = pa;
	device.ScaleFactor.a.ap = apa;
	device.ScaleFactor.a.e = ea;
	device.ScaleFactor.b.i = ib;
	device.ScaleFactor.b.p = pb;
	device.ScaleFactor.b.ap = apb;
	device.ScaleFactor.b.e = eb;
	
}

int HLW8112_UpdateCoeff() {
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_HFCONST, &device.HFconst, "HLW8112_REG_HFCONST"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_RMSIAC, &device.DeviceRegisterCoeff.RmsIAC, "HLW8112_REG_RMSIAC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_RMSIBC, &device.DeviceRegisterCoeff.RmsIBC, "HLW8112_REG_RMSIBC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_RMSUC, &device.DeviceRegisterCoeff.RmsUC, "HLW8112_REG_RMSUC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_POWER_PAC, &device.DeviceRegisterCoeff.PowerPAC, "HLW8112_REG_POWER_PAC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_POWER_PBC, &device.DeviceRegisterCoeff.PowerPBC, "HLW8112_REG_POWER_PBC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_POWER_SC, &device.DeviceRegisterCoeff.PowerSC, "HLW8112_REG_POWER_SC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_ENERGY_AC, &device.DeviceRegisterCoeff.EnergyAC, "HLW8112_REG_ENERGY_AC"));
	CHECK_AND_RETURN(HLW8112_UpdateCoeffFromRegister(HLW8112_REG_ENERGY_BC, &device.DeviceRegisterCoeff.EnergyBC, "HLW8112_REG_ENERGY_BC"));
	HLW8112_compute_scale_factor();
	return 0;
}

int HLW8112_CheckCoeffs() {
  	uint16_t checksum = 0xFFFF 
		+ device.DeviceRegisterCoeff.RmsIAC 
		+ device.DeviceRegisterCoeff.RmsIBC 
		+ device.DeviceRegisterCoeff.RmsUC 
		+ device.DeviceRegisterCoeff.PowerPAC 
		+ device.DeviceRegisterCoeff.PowerPBC 
		+ device.DeviceRegisterCoeff.PowerSC 
		+ device.DeviceRegisterCoeff.EnergyAC 
		+ device.DeviceRegisterCoeff.EnergyBC;
  	checksum = ~checksum;
  
	uint16_t regValue;
  	int cmdResult;
  	cmdResult = HLW8112_ReadRegister16(HLW8112_REG_COF_CHECKSUM, &regValue);
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_CheckCoeffs HLW8112_REG_EMUStatus_Chksum %i", cmdResult);
  	if (cmdResult < 0) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "HLW8112_CheckCoeffs HLW8112_REG_EMUStatus_Chksum no good %i", cmdResult);
    	return -1;
  	}

	if (checksum != regValue) {
		ADDLOG_WARN( LOG_FEATURE_ENERGYMETER, "HLW8112_CheckCoeffs Chksum mismatch  computed = %04X  read =  %04X", checksum, regValue);
		return 1;
	}
	return 0;
}

int HLW8112_SetMainChannel(HLW8112_Channel_t channel) {

	switch (channel) {
	case HLW8112_CHANNEL_A: {
		device.MainChannel = channel;
		return HLW8112_SPI_WriteControl(HLW8112_COMMAND_SELECT_CH_A);
	}
	case HLW8112_CHANNEL_B: {
		device.MainChannel = channel;
		return HLW8112_SPI_WriteControl(HLW8112_COMMAND_SELECT_CH_B);
	}
	}
	return -1;
}


int HLW8112_InitReg() {

	//HLW8112_SPI_WriteControl(HLW8112_COMMAND_RESET);

  	// pga values to config ?? 
	uint16_t PGA = HLW8112_PGA_16;
  	uint16_t PGB = HLW8112_PGA_16;
  	uint16_t PGU = HLW8112_PGA_1;	
	
	#pragma region init registers
	{
    	uint16_t syscon = 0;
    	syscon |= (1 << HLW8112_REG_SYSCON_ADC3ON);
    	syscon |= (1 << HLW8112_REG_SYSCON_ADC2ON);
    	syscon |= (1 << HLW8112_REG_SYSCON_ADC1ON);

    	syscon |= (PGA << HLW8112_REG_SYSCON_PGAIA);
    	syscon |= (PGB << HLW8112_REG_SYSCON_PGAIB);
    	syscon |= (PGU << HLW8112_REG_SYSCON_PGAU);

    	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_InitReg syscon %04X", syscon);
    	HLW8112_WriteRegister16(HLW8112_REG_SYSCON, syscon);

    	uint16_t emucon = 0;
    	emucon |= (1 << HLW8112_REG_EMUCON_PBRUN);
    	emucon |= (1 << HLW8112_REG_EMUCON_PARUN);
    	emucon |= (1 << HLW8112_REG_EMUCON_COMP_OFF);

    	emucon |= (HLW8112_ACTIVE_POW_CALC_METHOD_POS_NEG_ALGEBRAIC << HLW8112_REG_EMUCON_PMODE);

    	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_InitReg emucon %04X",  emucon);
    	HLW8112_WriteRegister16(HLW8112_REG_EMUCON, emucon);

    	uint16_t emucon2 = 0;

    	emucon2 |= (0 << HLW8112_REG_EMUCON2_EPB_CB);
    	emucon2 |= (0 << HLW8112_REG_EMUCON2_EPB_CA);
    	emucon2 |= (1 << HLW8112_REG_EMUCON2_CHS_IB);
    	emucon2 |= (1 << HLW8112_REG_EMUCON2_PFACTOREN);
    	emucon2 |= (1 << HLW8112_REG_EMUCON2_WAVEEN);
    	emucon2 |= (1 << HLW8112_REG_EMUCON2_ZXEN);
    	emucon2 |= (1 << HLW8112_REG_EMUCON2_VREFSEL);

    	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_InitReg emucon2 %04X", emucon2);
    	HLW8112_WriteRegister16(HLW8112_REG_EMUCON2, emucon2);
  	}
	#pragma endregion

  	// device info
	device.ResistorCoeff.KU = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KU, DEFAULT_RES_KU);
	device.ResistorCoeff.KIA = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIA, DEFAULT_RES_KIA);
	device.ResistorCoeff.KIB = CFG_GetPowerMeasurementCalibrationFloat(CFG_OBK_RES_KIB, DEFAULT_RES_KIB);

	device.PGA.U = PGU;
	device.PGA.IA = PGA;
	device.PGA.IB = PGB;

  	//internal clock

  	device.CLKI =  CFG_GetPowerMeasurementCalibrationInteger(CFG_OBK_CLK, DEFAULT_INTERNAL_CLK);

	device.ScaleFactor.v_rms = 1.0;
	device.ScaleFactor.freq = 1.0;
	device.ScaleFactor.a.i = 1.0;
	device.ScaleFactor.a.p = 1.0;
	device.ScaleFactor.a.e = 1.0;
	device.ScaleFactor.a.ap = 1.0;
	device.ScaleFactor.b.i = 1.0;
	device.ScaleFactor.b.p = 1.0;
	device.ScaleFactor.b.e = 1.0;
	device.ScaleFactor.b.ap = 1.0;

  	int cmdResult;

  	//TODO: to config
  	cmdResult = HLW8112_SetMainChannel(HLW8112_CHANNEL_A);
  	if (cmdResult < 0) {
    	return cmdResult;
  	}

  	HLW8112_UpdateCoeff();
	HLW8112_compute_scale_factor();

  	cmdResult = HLW8112_CheckCoeffs();
  	if (cmdResult > 0) {
    	ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "HLW8112_InitReg Chksum mismatch");
    	return -2;
  	}
  	return 0;
}

#pragma endregion
#pragma endregion

void HLW8112SPI_Stop(void) {
	HLW8112_Save_Statistics();
	SPI_Deinit();
	SPI_DriverDeinit();
}

void HLW8112_Init(void) {
	HLW8112_Restore_Stats();
	HLW8112_Init_Channels();
  	HLW8112_addCommads();
  	HLW8112_Init_Pins();

}

void HLW8112SPI_Init(void) {
	HLW8112_Init();
  	SPI_DriverInit();

	spi_config_t cfg;
	cfg.role = SPI_ROLE_MASTER;
	cfg.bit_width = SPI_BIT_WIDTH_8BITS;
	cfg.polarity = SPI_POLARITY_HIGH;
	cfg.phase = SPI_PHASE_1ST_EDGE;
	cfg.wire_mode = SPI_3WIRE_MODE;
	cfg.baud_rate = HLW8112_SPI_BAUD_RATE;
	cfg.bit_order = SPI_MSB_FIRST;
	OBK_SPI_Init(&cfg);

  	int result = HLW8112_InitReg();
  	ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER, "HLW8112_InitReg result %i", result);
 
}
#pragma endregion

#pragma region scalers

void HLW8112_ScaleVoltage(uint32_t regValue, int32_t* value){
	if (regValue == 0) {
		*value = 0;
	}
	else if( regValue & HLW8112_INVALID_REGVALUE) {
		*value = 0;
	}
	else {
		double scale = device.ScaleFactor.v_rms;
		int32_t rv = HLW8112_24BitTo32Bit(regValue);
		double v = rv*scale;

		*value = (int32_t)v;
	}
	
}

void HLW8112_ScaleFrequency(uint32_t regValue, int32_t* value){
	if (regValue == 0) {
		*value = 0;
	} else {
		*value = device.ScaleFactor.freq / regValue;
	}
}

void HLW8112_ScaleCurrent(HLW8112_Channel_t channel, uint32_t regValue, int32_t* value){
if (regValue == 0) {
		*value = 0;
	}
	else if( regValue & HLW8112_INVALID_REGVALUE) {
		*value = 0;
	}
	 else {
		double scale = channel == HLW8112_CHANNEL_B ? device.ScaleFactor.b.i : device.ScaleFactor.a.i;
		int32_t rv = HLW8112_24BitTo32Bit(regValue);
		double v = rv*scale;
  
		*value = (int32_t)v;
	}
}

void HLW8112_ScalePower(HLW8112_Channel_t channel, uint32_t regValue, int32_t* value){
	if (regValue == 0) {
		*value = 0;
	}
	else {
		int32_t rv = (int32_t)regValue;
		double scale = channel == HLW8112_CHANNEL_B ? device.ScaleFactor.b.p : device.ScaleFactor.a.p;
		double v = rv*scale;
		*value = (int32_t)v;
	}

}

void HLW8112_ScaleEnergy(HLW8112_Channel_t channel, uint32_t regValue, int32_t* value){
	if (regValue == 0) {
		*value = 0;
	} else {
		int32_t rv = HLW8112_24BitTo32Bit(regValue);
		double scale = channel == HLW8112_CHANNEL_B ? device.ScaleFactor.b.e : device.ScaleFactor.a.e;
		double v = rv*scale;
		*value = (int32_t)v;
	}
}

void HLW8112_ScaleAparentPower(HLW8112_Channel_t channel, uint32_t regValue, int32_t* value){
	
	if (regValue == 0) {
		*value = 0;
	}
	else {
		int32_t rv = (int32_t)regValue;
		double scale = channel == HLW8112_CHANNEL_B ? device.ScaleFactor.b.ap : device.ScaleFactor.a.ap;
		double v = rv*scale;
		*value = (int32_t)v;
	}
}


void HLW8112_ScalePowerFactor(uint32_t regValue, int16_t* value){
		if (regValue == 0) {
		*value = 0;
	} else {
		int32_t rv = HLW8112_24BitTo32Bit(regValue);
		double scale =  device.ScaleFactor.pf;
		double v = rv*scale;
		*value = (int32_t)v;
	}
}

static void HLW8112_ScaleAndUpdate(HLW8112_Data_t* data) {

	int16_t power_factor;
	int32_t voltage, frequency, current_a, current_b, power_a, power_b, apparent_power ,energy_a, energy_b;

	HLW8112_ScaleVoltage(data->v_rms, &voltage);
	HLW8112_ScaleFrequency(data->freq, &frequency);

  	HLW8112_ScaleCurrent(HLW8112_CHANNEL_A, data->ia_rms, &current_a);
  	HLW8112_ScaleCurrent(HLW8112_CHANNEL_B, data->ib_rms, &current_b);

  	HLW8112_ScalePower(HLW8112_CHANNEL_A, data->pa, &power_a);
  	HLW8112_ScalePower(HLW8112_CHANNEL_B, data->pb, &power_b);


  	HLW8112_ScaleEnergy(HLW8112_CHANNEL_A, data->ea, &energy_a);
  	HLW8112_ScaleEnergy(HLW8112_CHANNEL_B, data->eb, &energy_b);


  	HLW8112_ScalePowerFactor(data->pf, &power_factor);

  	HLW8112_ScaleAparentPower(HLW8112_CHANNEL_A, data->ap, &apparent_power);

	//ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, 
	//	"Values X v=%d f=%d pf=%d ca=%d pa=%d ap=%d ea=%d cb=%d pb=%d eb=%d",
	//	voltage, frequency, power_factor, current_a, power_a, apparent_power, energy_a, current_b, power_b, energy_b);

	last_update_data.v_rms = voltage;
	last_update_data.freq = frequency;
	last_update_data.pf = power_factor;
	last_update_data.ap = apparent_power;
	last_update_data.ia_rms = current_a;
	last_update_data.ib_rms = current_b;
	last_update_data.pa = power_a;
	last_update_data.pb = power_b;

	HLW8112_SaveFlags_t save =  HLW8112_SAVE_NONE;
	if (energy_a !=0  ) {
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "EA val %08X", data->ea);
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "EA scaled val %08X", energy_a);
		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Import before %f", energy_acc_a.Import);
		if (power_a > 0) {
			energy_acc_a.Import +=  (double)energy_a / 10000000.0;
			save |= HLW8112_SAVE_A_IMP;
		}else {
			energy_acc_a.Export +=  (double)energy_a / 10000000.0;
			save |= HLW8112_SAVE_A_EXP;
		}

		ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "Import after %f", energy_acc_a.Import);
		}
	if (energy_b !=0.0f ) {
		if (power_b > 0) {
			energy_acc_b.Import +=  energy_b / 10000000.0;
			save |= HLW8112_SAVE_B_IMP;
		}else {
			energy_acc_b.Export +=  energy_b/ 10000000.0;
			save |= HLW8112_SAVE_B_EXP;
		}
	}
	if (save != HLW8112_SAVE_NONE) {
		HLW8112_save_stats(save);
	}
    // BL_ProcessUpdate(voltage, current_a, power_a, frequency, energy_a);

	// update
	
	CHANNEL_Set(HLW8112_Channel_Voltage, last_update_data.v_rms / 10.0, 0);
	CHANNEL_Set(HLW8112_Channel_Frequency,last_update_data.freq , 0);
	CHANNEL_Set(HLW8112_Channel_PowerFactor, last_update_data.pf, 0);
	CHANNEL_Set(HLW8112_Channel_current_B, last_update_data.ib_rms, 0);
	CHANNEL_Set(HLW8112_Channel_current_A,last_update_data.ia_rms , 0);
	CHANNEL_Set(HLW8112_Channel_power_B, last_update_data.pb / 10.0, 0);
	CHANNEL_Set(HLW8112_Channel_power_A, last_update_data.pa / 10.0, 0);
	CHANNEL_Set(HLW8112_Channel_apparent_power_A, last_update_data.ap / 10.0, 0);
	CHANNEL_Set(HLW8112_Channel_export_B, last_update_data.eb->Export * 1000, 0);
	CHANNEL_Set(HLW8112_Channel_export_A, last_update_data.ea->Export * 1000, 0);
	CHANNEL_Set(HLW8112_Channel_import_B, last_update_data.eb->Import * 1000, 0);
	CHANNEL_Set(HLW8112_Channel_import_A, last_update_data.ea->Import * 1000, 0);
}

#pragma endregion


void HLW8112_RunEverySecond(void) {
	HLW8112_Data_t data;

	HLW8112_ReadRegister24(HLW8112_REG_RMSU, &data.v_rms);
	HLW8112_ReadRegister16(HLW8112_REG_UFREQ, &data.freq);

	HLW8112_ReadRegister24(HLW8112_REG_RMSIA, &data.ia_rms);
	HLW8112_ReadRegister32(HLW8112_REG_POWER_PA, &data.pa);
	HLW8112_ReadRegister24(HLW8112_REG_ENERGY_PA, &data.ea);

	HLW8112_ReadRegister24(HLW8112_REG_RMSIB, &data.ib_rms);

	HLW8112_ReadRegister32(HLW8112_REG_POWER_PB, &data.pb);
	HLW8112_ReadRegister24(HLW8112_REG_ENERGY_PB, &data.eb);

	HLW8112_ReadRegister24(HLW8112_REG_POWER_FACTOR, &data.pf);
	HLW8112_ReadRegister32(HLW8112_REG_POWER_S, &data.ap);
	
	HLW8112_ReadRegister8(HLW8112_REG_SYSSTATUS, &data.sysstat);
	HLW8112_ReadRegister24(HLW8112_REG_EMUSTATUS, &data.emustat);
	HLW8112_ReadRegister16(HLW8112_REG_IF, &data.int_f);



	READ_REG(SYSCON,16);
	READ_REG(EMUCON,16);
	READ_REG(HFCONST,16);
	READ_REG(PSTARTA,16);
	READ_REG(PSTARTB,16);
	READ_REG(PAGAIN,16);
	READ_REG(PBGAIN,16);
	READ_REG(PHASEA,8);
	READ_REG(PHASEB,8);
	READ_REG(PAOS,16);
	READ_REG(PBOS,16);
	READ_REG(RMSIAOS,16);
	READ_REG(RMSIBOS,16);
	READ_REG(IBGAIN,16);
	READ_REG(PSGAIN,16);
	READ_REG(PSOS,16);
	READ_REG(EMUCON2,16);
	
	last_data = data;

    HLW8112_ScaleAndUpdate(&data);
}


#pragma region http ui

void appendBitFlag(char *name, uint32_t regValue, uint8_t bitNum, http_request_t *request) {
	hprintf255(request, "<div class='reg-flag'><span>%s</span><span>%d</span></div>", name, ((regValue & ( 1 << bitNum)) > 0 ? 1 : 0) );
  
}

void appendTableRow(http_request_t *request, char *name,char* unit, int32_t value, int precision, float factor) {
	hprintf255(request,
        "<tr><td><b>%s</b></td><td style='text-align: right;'>%.*f</td><td>%s</td></tr>",
		name,   precision, value / factor, unit);
}



void appendChannelTableRow(http_request_t *request, char *name,char* unit, float value_a, float value_b, int precision, float factor) {
	hprintf255(request,
        "<tr><td><b>%s</b></td><td style='text-align: right;'>%.*f</td><td>%s</td><td style='text-align: right;'>%.*f</td><td>%s</td></tr>",
		name, precision, value_a/ factor, unit,precision, value_b/ factor, unit);
}

void appendRegEdit(http_request_t *request, char *name,uint16_t reg, bool readonly, 
	int32_t currentVal, uint16_t width, bool comp) {
	
	poststr(request,"<form >");
    hprintf255(request,"<label> %s </label>",name);
    hprintf255(request,"<input type='text' name='reg_value' value='%d'>",currentVal);
    hprintf255(request,"<input type='hidden' name='reg' value='%d'/> ",reg);
    hprintf255(request,"<input type='hidden' name='reg_width' value='%d'/> ",width);
    hprintf255(request,"<input type='hidden' name='compliment' value='%d'> ",comp);
    poststr(request,"<input type='hidden' name='reg_edit' value='1'> ");
    poststr(request,"<input type='submit' value='Save' style='width: 80px;'>");
    poststr(request,"</form>");
}

void HLW8112_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState) {
		if (http_getArgInteger(request->url, "clear_energy")) {
			char channel[2];
			if (http_getArg(request->url, "channel", channel, sizeof(channel))) {
				if (!strcmp("a" , channel)) {
					HLW8112_Set_EnergyStat(HLW8112_CHANNEL_A,0,0);
				} else {
					HLW8112_Set_EnergyStat(HLW8112_CHANNEL_B,0,0);
				}
			}
		}
		else if (http_getArgInteger(request->url, "reg_edit")) {
			//TODO fix this 
			u16_t reg = http_getArgInteger(request->url, "reg");
			uint32_t val = http_getArgInteger(request->url, "reg_value");
			uint16_t width = http_getArgInteger(request->url, "reg_width");
			bool comp = http_getArgInteger(request->url, "compliment") ==1;
			switch (width){
				case 8: {
					HLW8112_WriteRegister8(reg, val);
					break;
				}
				case 16: {
					HLW8112_WriteRegister16(reg, val);
					break;
				}
			}
		}
		//?action=reg_edit&reg_value=&reg=100&reg_width=24&compliment=1
		return;
	}

	poststr(request, "<hr><table style='width:100%'>");
	appendTableRow(request, "Voltage", "V", last_update_data.v_rms, 2,1000.0f );
	appendTableRow(request, "Frequency", "Hz", last_update_data.freq, 2, 100.0f );
	appendTableRow(request, "Active Power", "W", last_update_data.pa, 3,1000.0f );
	appendTableRow(request, "Apparent Power", "VA", last_update_data.ap, 3, 1000.0f );
	appendTableRow(request, "Power Factor", "", last_update_data.pf, 2, 1000.0f );
	poststr(request, "</table>");

	poststr(request, "<hr><table style='width:100%'>");
	poststr(request, "<tr><th></th><th>Channel A</th><th></th><th>Channel B</th><th></th></tr>");
	appendChannelTableRow(request, "Current", "mA", last_update_data.ia_rms, last_update_data.ib_rms, 0,1 );
	appendChannelTableRow(request, "Active Power", "W", last_update_data.pa, last_update_data.pb, 3,1000 );
	appendChannelTableRow(request, "Import Energy", "KWh", last_update_data.ea->Import, last_update_data.eb->Import, 4 , 1 );
	appendChannelTableRow(request, "Export Energy", "KWh", last_update_data.ea->Export, last_update_data.eb->Export, 4 , 1 );

	poststr(request,
          "<tr><td><b>Actions</b></td><td style='text-align: right;'> \
		  <button style='background-color:red;' onclick='location.href=\"?clear_energy=1&channel=a\"'>Clear</button> \
		  </td><td></td><td style='text-align: right;'>\
		  <button style='background-color:red;' onclick='location.href=\"?clear_energy=1&channel=b\"'>Clear </button> \
		  </td><td></td></tr>");

	poststr(request, "</table>");
#if HLW8112_SPI_RAWACCESS
	poststr(request,"<style> \
                div form { \
                    display: flex; align-items: stretch; gap: 10px;justify-content: center; \
                }\
                div form label {\
                    flex: 1;text-align: right;align-self: center; max-width: 220px; min-width: 100px;\
                    min-width: 90px;\
                }\
				div form input {\
                    width: 100px;\
                }\
            </style>\
            <div style='display: flex; flex-wrap: wrap;'>");
	
	REG_EDIT(SYSCON, 16, false, false);
	REG_EDIT(EMUCON, 16, false, false);
	REG_EDIT(EMUCON2, 16, false, false);
	REG_EDIT(HFCONST, 16, false, false);
	REG_EDIT(PSTARTA, 16, false, false);
	REG_EDIT(PSTARTB, 16, false, false);
	REG_EDIT(PAGAIN, 16, false, false);
	REG_EDIT(PBGAIN, 16, false, false);
	REG_EDIT(PHASEA, 16, false, false);
	REG_EDIT(PHASEB, 16, false, false);
	REG_EDIT(PAOS, 16, false, false);
	REG_EDIT(PBOS, 16, false, false);
	REG_EDIT(RMSIAOS, 16, false, false);
	REG_EDIT(RMSIBOS, 16, false, false);
	REG_EDIT(IBGAIN, 16, false, false);
	REG_EDIT(PSOS, 16, false, false);

	poststr(request, "</div>");
	poststr(request, "<style>\
        div.reg-flag {\
            display: flex;\
            flex-direction: column;\
            align-items: center;\
        }\
        div.flags {\
            display: flex;\
            flex-wrap: wrap;\
        }\
        </style>");


	poststr(request, "<hr><h2>System Status</h4><div class='flags'>");
	appendBitFlag("RST", last_data.sysstat, HLW8112_REG_SYSSTATUS_RST, request);
	appendBitFlag("WREN", last_data.sysstat, HLW8112_REG_SYSSTATUS_WREN, request);
	appendBitFlag("clksel", last_data.sysstat, HLW8112_REG_SYSSTATUS_CLKSEL, request);
	poststr(request, "</div>");
	
	hprintf255(request, "<hr><h2>Emu Status</h4><div class='flags'>");
	// TODO: check sum
	appendBitFlag("ChksumBusy", last_data.emustat, HLW8112_REG_EMUSTATUS_CHKSUMBSY, request);
	appendBitFlag("REVPA", last_data.emustat, HLW8112_REG_EMUSTATUS_REVPA, request);
	appendBitFlag("REVPB", last_data.emustat, HLW8112_REG_EMUSTATUS_REVPB, request);
	appendBitFlag("NopldA", last_data.emustat, HLW8112_REG_EMUSTATUS_NOPLDA, request);
	appendBitFlag("NopldB", last_data.emustat, HLW8112_REG_EMUSTATUS_NOPLDB, request);
	appendBitFlag("Channel_sel", last_data.emustat, HLW8112_REG_EMUSTATUS_CHA_SEL, request);
	poststr(request, "</div>");

	hprintf255(request, "<hr><h2>Interrupt status</h4><div class='flags'>");

	appendBitFlag("DUPDIF", last_data.int_f, HLW8112_REG_IF_DUPDIF, request);
	appendBitFlag("PFBIF", last_data.int_f, HLW8112_REG_IF_PFAIF, request);
	appendBitFlag("PFBIF", last_data.int_f, HLW8112_REG_IF_PFBIF, request);
	appendBitFlag("PEAOIF", last_data.int_f, HLW8112_REG_IF_PEAOIF, request);
	appendBitFlag("PEBOIF", last_data.int_f, HLW8112_REG_IF_PEBOIF, request);
	appendBitFlag("INSTANIF", last_data.int_f, HLW8112_REG_IF_INSTANIF, request);
	appendBitFlag("OIAIF", last_data.int_f, HLW8112_REG_IF_OIAIF, request);
	appendBitFlag("OIBIF", last_data.int_f, HLW8112_REG_IF_OIBIF, request);
	appendBitFlag("OVIF", last_data.int_f, HLW8112_REG_IF_OVIF, request);
	appendBitFlag("OPIF", last_data.int_f, HLW8112_REG_IF_OPIF, request);
	appendBitFlag("SAGIF", last_data.int_f, HLW8112_REG_IF_SAGIF, request);
	appendBitFlag("ZX_IAIF", last_data.int_f, HLW8112_REG_IF_ZX_IAIF, request);
	appendBitFlag("ZX_IBIF", last_data.int_f, HLW8112_REG_IF_ZX_IBIF, request);
	appendBitFlag("ZX_UIF ", last_data.int_f, HLW8112_REG_IF_ZX_UIF, request);
	appendBitFlag("LeakageIF ", last_data.int_f, HLW8112_REG_IF_LEAKAGEIF, request);
	poststr(request, "</div>");
	#endif
}


#pragma endregion


void HLW8112_OnHassDiscovery(const char *topic) {
	//TODO clear energy buttons

    ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER, "HLW8112_OnHassDiscovery");
	HassDeviceInfo* dev_info = NULL;
	dev_info = hass_init_button_device_info("Clear Energy A", "clear_energy", "channel_a", HASS_CATEGORY_DIAGNOSTIC);
	MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
	dev_info = hass_init_button_device_info("Clear Energy B", "clear_energy", "channel_b", HASS_CATEGORY_DIAGNOSTIC);
	MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
	hass_free_device_info(dev_info);
}

void HLW8112_Save_Statistics() {
	if (DRV_IsRunning("HLW8112SPI")){
		HLW8112_save_stats(HLW8112_SAVE_FORCE);
	}
}
#endif // ENABLE_DRIVER_HLW8112SPI

#pragma GCC diagnostic pop