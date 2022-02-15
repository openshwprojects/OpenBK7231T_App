

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
void CFG_SaveWiFi();
void CFG_LoadWiFi();
void CFG_SaveMQTT();
void CFG_LoadMQTT();
const char *CFG_GetWebappRoot();
const char *CFG_LoadWebappRoot();
void CFG_SetWebappRoot(const char *s);
void CFG_InitAndLoad();
void WiFI_GetMacAddress(char *mac);
void WiFI_SetMacAddress(char *mac);

