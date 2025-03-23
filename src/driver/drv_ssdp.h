
extern int DRV_SSDP_Active;

void DRV_SSDP_Init();
void DRV_SSDP_RunEverySecond();
void DRV_SSDP_RunQuickTick();
void DRV_SSDP_Shutdown();
void DRV_SSDP_SendReply(struct sockaddr_in *addr, const char *message);




