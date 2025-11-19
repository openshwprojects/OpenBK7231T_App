

void GirierMCU_Init();
void GirierMCU_RunFrame();
void GirierMCU_Shutdown();
void GirierMCU_OnChannelChanged(int channel,int iVal);
bool GirierMCU_IsChannelUsedByGirierMCU(int channelIndex);
void GirierMCU_ForcePublishChannelValues();
