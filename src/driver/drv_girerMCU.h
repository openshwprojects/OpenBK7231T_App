

void GirerMCU_Init();
void GirerMCU_RunFrame();
void GirerMCU_Shutdown();
void GirerMCU_Send(byte *data, int size);
void GirerMCU_OnChannelChanged(int channel,int iVal);
void GirerMCU_Send_RawBuffer(byte *data, int len);
bool GirerMCU_IsChannelUsedByGirerMCU(int channelIndex);
void GirerMCU_ForcePublishChannelValues();
