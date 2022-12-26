#ifdef WINDOWS

#include "selftest_local.h".

void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments) {

	const char *myName = CFG_GetMQTTClientId();
	char buffer[4096];
	sprintf(buffer, "cmnd/%s/%s", myName, command);
	MQTT_Post_Received_Str(buffer, arguments);
	Sim_RunFrames(1, false);

}
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments) {
	const char *myName = CFG_GetMQTTClientId();
	char buffer[4096];
	sprintf(buffer, "%s/%i/set", myName, channelIndex);
	MQTT_Post_Received_Str(buffer, arguments);
	Sim_RunFrames(1, false);
}

#endif
