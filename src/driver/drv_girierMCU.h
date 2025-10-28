

void GirierMCU_Init();
void GirierMCU_RunFrame();
void GirierMCU_Shutdown();
void GirierMCU_Send(byte *data, int size);
void GirierMCU_OnChannelChanged(int channel,int iVal);
void GirierMCU_Send_RawBuffer(byte *data, int len);
bool GirierMCU_IsChannelUsedByGirierMCU(int channelIndex);
void GirierMCU_ForcePublishChannelValues();
