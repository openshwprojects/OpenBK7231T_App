
#include "../httpserver/new_http.h"
#include "../cmnds/cmd_public.h"

void BL0942_Init();
void BL0942_RunFrame();

void BL0937_Init();
void BL0937_RunFrame();

void CSE7766_Init();
void CSE7766_RunFrame();

void Test_Power_Init();
void Test_Power_RunFrame();

void Test_LED_Driver_Init();
void Test_LED_Driver_RunFrame();
void Test_LED_Driver_OnChannelChanged(int ch, int value);

void DRV_DGR_Init();
void DRV_DGR_RunQuickTick();
void DRV_DGR_RunEverySecond();
void DRV_DGR_Shutdown();
void DRV_DGR_OnChannelChanged(int ch, int value);

void DRV_DDP_Init();
void DRV_DDP_RunFrame();
void DRV_DDP_Shutdown();

void SM2135_Init();

void SM2235_Init();

void BP5758D_Init();

void BP1658CJ_Init();

void SM16703P_Init();

void BL_Shared_Init();
void BL_ProcessUpdate(float voltage, float current, float power);
void BL09XX_AppendInformationToHTTPIndexPage(http_request_t* request);
bool DRV_IsRunning(const char* name);

// this is exposed here only for debug tool with automatic testing
void DGR_ProcessIncomingPacket(char* msgbuf, int nbytes);
void DGR_SpoofNextDGRPacketSource(const char* ipStrs);

void TuyaMCU_Sensor_RunFrame();
void TuyaMCU_Sensor_Init();


void DRV_Toggler_ProcessChanges(http_request_t* request);
void DRV_Toggler_AddToHtmlPage(http_request_t* request);
void DRV_Toggler_AppendInformationToHTTPIndexPage(http_request_t* request);
void DRV_InitPWMToggler();


void DRV_HTTPButtons_ProcessChanges(http_request_t* request);
void DRV_HTTPButtons_AddToHtmlPage(http_request_t* request);
void DRV_InitHTTPButtons();

void CHT8305_Init();
void CHT8305_OnEverySecond();
void CHT8305_AppendInformationToHTTPIndexPage(http_request_t* request);

void SHT3X_Init();
void SHT3X_AppendInformationToHTTPIndexPage(http_request_t* request);
void SHT3X_StopDriver();

void DRV_MAX72XX_Init();

void WEMO_Init();
void WEMO_AppendInformationToHTTPIndexPage(http_request_t* request);

#define SM2135_DELAY         4

extern int g_i2c_pin_clk;
extern int g_i2c_pin_data;
void Soft_I2C_SetLow(uint8_t pin);
void Soft_I2C_SetHigh(uint8_t pin);
bool Soft_I2C_PreInit(void);
bool Soft_I2C_WriteByte(uint8_t value);
bool Soft_I2C_Start(uint8_t addr);
void Soft_I2C_Stop(void);
uint8_t Soft_I2C_ReadByte(bool nack);
void Soft_I2C_ReadBytes(uint8_t* buf, int numOfBytes);
commandResult_t CMD_LEDDriver_Map(const void *context, const char *cmd, const char *args, int flags);
commandResult_t CMD_LEDDriver_WriteRGBCW(const void *context, const char *cmd, const char *args, int flags);
void LED_I2CDriver_WriteRGBCW(float* finalRGBCW);

/* Bridge driver *********************************************/
void Bridge_driver_Init();
void Bridge_driver_DeInit();
void Bridge_driver_QuickFrame();
void Bridge_driver_OnChannelChanged(int ch, int value);
OBK_Publish_Result Bridge_driver_ChannelPublish(int channel);
/*************************************************************/

