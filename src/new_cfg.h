


extern int g_cfg_pendingChanges;

const char *CFG_GetDeviceName();
const char *CFG_GetShortDeviceName();
void CFG_CreateDeviceNameUnique();
int CFG_GetMQTTPort();
void CFG_SetMQTTPort(int p);
void CFG_SetOpenAccessPoint();
const char *CFG_GetWiFiSSID();
const char *CFG_GetWiFiPass();
void CFG_SetWiFiSSID(const char *s);
void CFG_SetWiFiPass(const char *s);
const char *CFG_GetMQTTHost();
const char *CFG_GetMQTTBrokerName();
const char *CFG_GetMQTTUserName();
const char *CFG_GetMQTTPass();
void CFG_SetMQTTHost(const char *s);
void CFG_SetMQTTBrokerName(const char *s);
void CFG_SetMQTTUserName(const char *s);
void CFG_SetMQTTPass(const char *s);
const char *CFG_GetWebappRoot();
int CFG_SetWebappRoot(const char *s);
void CFG_InitAndLoad();
void CFG_Save_IfThereArePendingChanges();
void CFG_IncrementOTACount();



