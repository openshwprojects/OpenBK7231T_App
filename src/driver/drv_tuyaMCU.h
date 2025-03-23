

void TuyaMCU_Init();
void TuyaMCU_RunFrame();
void TuyaMCU_Send(byte *data, int size);
void TuyaMCU_OnChannelChanged(int channel,int iVal);
void TuyaMCU_Send_RawBuffer(byte *data, int len);
bool TuyaMCU_IsChannelUsedByTuyaMCU(int channelIndex);
void TuyaMCU_ForcePublishChannelValues();

