

void TuyaMCU_Init();
void TuyaMCU_RunEverySecond();
void TuyaMCU_RunFrame();
void TuyaMCU_Shutdown();

void TuyaMCU_OnChannelChanged(int channel,int iVal);
bool TuyaMCU_IsChannelUsedByTuyaMCU(int channelIndex);

int http_obk_json_dps(int id, void* request, jsonCb_t printer);
