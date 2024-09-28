#pragma once

#include "../httpserver/new_http.h"
#include "../cmnds/cmd_public.h"

void DRV_DGR_Init();
void DRV_DGR_RunQuickTick();
void DRV_DGR_RunEverySecond();
void DRV_DGR_Shutdown();
void DRV_DGR_OnChannelChanged(int ch, int value);
void DRV_DGR_AppendInformationToHTTPIndexPage(http_request_t* request);

void DRV_DDP_Init();
void DRV_DDP_RunFrame();
void DRV_DDP_Shutdown();
void DRV_DDP_AppendInformationToHTTPIndexPage(http_request_t* request);

void BMP280_Init();
void BMP280_OnEverySecond();
void BMP280_AppendInformationToHTTPIndexPage(http_request_t* request);

void DoorDeepSleep_Init();
void DoorDeepSleep_OnEverySecond();
void DoorDeepSleep_StopDriver();
void DoorDeepSleep_AppendInformationToHTTPIndexPage(http_request_t* request);
void DoorDeepSleep_OnChannelChanged(int ch, int value);

void DRV_MAX72XX_Clock_OnEverySecond();
void DRV_MAX72XX_Clock_RunFrame();
void DRV_MAX72XX_Clock_Init();

void DRV_ADCButton_Init();
void DRV_ADCButton_RunFrame();

void PT6523_Init();
void PT6523_RunFrame();
void PT6523_DrawString(char gk[], int startOfs);
void PT6523_ClearString();

void TS_RunQuickTick();
void TS_Init();

void SM2135_Init();

void SM2235_Init();

void BP5758D_Init();

void BP1658CJ_Init();

void KP18058_Init();

void SM15155E_Init();

void SM16703P_Init();
void SM16703P_setPixel(int pixel, int r, int g, int b);
void SM16703P_setPixelWithBrig(int pixel, int r, int g, int b);
void SM16703P_setAllPixels(int r, int g, int b);
void SM16703P_scaleAllPixels(int scale);
void SM16703P_Show();
extern uint32_t pixel_count;

void TM1637_Init();

void GN6932_Init();

void TM1638_Init();

void HT16K33_Init();

void HD2015_Init();

void DRV_IR2_Init();

void DRV_ADCSmoother_Init();
void DRV_ADCSmoother_RunFrame();

bool DRV_IsRunning(const char* name);

// this is exposed here only for debug tool with automatic testing
void DGR_ProcessIncomingPacket(char* msgbuf, int nbytes);
void DGR_SpoofNextDGRPacketSource(const char* ipStrs);

void TuyaMCU_Sensor_RunEverySecond();
void TuyaMCU_Sensor_Init();

void DRV_Test_Charts_AddToHtmlPage(http_request_t *request);

void DRV_Charts_AddToHtmlPage(http_request_t *request);
void DRV_Charts_Init();

void DRV_Toggler_ProcessChanges(http_request_t* request);
void DRV_Toggler_AddToHtmlPage(http_request_t* request);
void DRV_Toggler_AppendInformationToHTTPIndexPage(http_request_t* request);
void DRV_Toggler_QuickTick();
void DRV_InitPWMToggler();


void DRV_HTTPButtons_ProcessChanges(http_request_t* request);
void DRV_HTTPButtons_AddToHtmlPage(http_request_t* request);
void DRV_InitHTTPButtons();

void CHT83XX_Init();
void CHT83XX_OnEverySecond();
void CHT83XX_AppendInformationToHTTPIndexPage(http_request_t* request);

void SHT3X_Init();
void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request);
void SHT3X_OnEverySecond();
void SHT3X_StopDriver();

void AHT2X_Init();
void AHT2X_AppendInformationToHTTPIndexPage(http_request_t* request);
void AHT2X_OnEverySecond();
void AHT2X_StopDriver();

void BMPI2C_Init();
void BMPI2C_AppendInformationToHTTPIndexPage(http_request_t* request);
void BMPI2C_OnEverySecond();

void SGP_Init();
void SGP_AppendInformationToHTTPIndexPage(http_request_t* request);
void SGP_OnEverySecond();
void SGP_StopDriver();

void Batt_Init();
void Batt_OnEverySecond();
void Batt_AppendInformationToHTTPIndexPage(http_request_t* request);
void Batt_StopDriver();

void Shift_Init();
void Shift_OnEverySecond();
void Shift_OnChannelChanged(int ch, int value);

void TMGN_RunQuickTick();

void DRV_MAX72XX_Init();

void apply_smart_light();

void WEMO_Init();
void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request);

void HUE_Init();
void HUE_AppendInformationToHTTPIndexPage(http_request_t* request);

void MCP9808_Init();
void MCP9808_OnEverySecond();
void MCP9808_AppendInformationToHTTPIndexPage(http_request_t* request);

void ChargingLimit_Init();
void ChargingLimit_OnEverySecond();
void ChargingLimit_AppendInformationToHTTPIndexPage(http_request_t *request);

void RN8209_Init(void);
void RN8029_RunEverySecond(void);

void MAX6675_Init(void);
void MAX6675_RunEverySecond(void);

void PWMG_Init();

void Freeze_Init();
void Freeze_OnEverySecond();
void Freeze_RunFrame();

void PixelAnim_Init();
void PixelAnim_SetAnimQuickTick();
void PixelAnim_SetAnim(int j);

void Drawers_Init();
void Drawers_QuickTick();

void HGS02_Init(void);
void HGS02_RunEverySecond(void);

#define SM2135_DELAY         4

// Software I2C 
typedef struct softI2C_s {
	short pin_clk;
	short pin_data;
	// I really have to place it here for a GN6932 driver, which is an SPI version of TM1637
	short pin_stb;
	// I must somehow be able to tell which proto we have?
	//short protocolType;
	byte address8bit;
} softI2C_t;

void Soft_I2C_SetLow(uint8_t pin);
void Soft_I2C_SetHigh(uint8_t pin);
bool Soft_I2C_PreInit(softI2C_t* i2c);
bool Soft_I2C_WriteByte(softI2C_t* i2c, uint8_t value);
bool Soft_I2C_Start(softI2C_t* i2c, uint8_t addr);
void Soft_I2C_Start_Internal(softI2C_t *i2c);
void Soft_I2C_Stop(softI2C_t* i2c);
uint8_t Soft_I2C_ReadByte(softI2C_t* i2c, bool nack);
void Soft_I2C_ReadBytes(softI2C_t* i2c, uint8_t* buf, int numOfBytes);

// Shared LED driver
commandResult_t CMD_LEDDriver_Map(const void* context, const char* cmd, const char* args, int flags);
commandResult_t CMD_LEDDriver_WriteRGBCW(const void* context, const char* cmd, const char* args, int flags);
void LED_I2CDriver_WriteRGBCW(float* finalRGBCW);

/* Bridge driver *********************************************/
void Bridge_driver_Init();
void Bridge_driver_DeInit();
void Bridge_driver_QuickFrame();
void Bridge_driver_OnChannelChanged(int ch, int value);
/*************************************************************/

