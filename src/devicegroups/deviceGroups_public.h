#ifndef __DGR_PUBLIC_H__
#define __DGR_PUBLIC_H__

#include "../new_common.h"

#define DGR_SHARE_POWER				1
#define DGR_SHARE_LIGHT_BRI			2
#define DGR_SHARE_LIGHT_FADE		4
#define DGR_SHARE_LIGHT_SCHEME		8
#define DGR_SHARE_LIGHT_COLOR		16
#define DGR_SHARE_DIMMER_SETTINGS	32
#define DGR_SHARE_EVENT				64

typedef struct dgrCallbacks_s {
	void (*processPower)(int relayStates, byte relaysCount);
	// they are both sent together by Tasmota devices
	void (*processBrightnessPowerOn)(byte brightness);
	void (*processLightBrightness)(byte brightness);
	void (*processRGBCW)(byte r, byte g, byte b, byte c, byte w);
	int (*checkSequence)(int seq);
} dgrCallbacks_t;

typedef struct dgrGroupDef_s {
	char groupName[32];
	unsigned int devGroupShare_In;
	unsigned int devGroupShare_Out;
} dgrGroupDef_t;

typedef struct dgrDevice_s {
	dgrGroupDef_t gr;
	dgrCallbacks_t cbs;
} dgrDevice_t;

int DGR_Parse(const byte *data, int len, dgrDevice_t *dev, struct sockaddr *addr);

int DGR_Quick_FormatPowerState(byte *buffer, int maxSize, const char *groupName, int sequence, int flags, int channels, int numChannels);
int DGR_Quick_FormatBrightness(byte *buffer, int maxSize, const char *groupName, int sequence, int flags, byte brightness);
int DGR_Quick_FormatRGBCW(byte *buffer, int maxSize, const char *groupName, int sequence, int flags, byte r, byte g, byte b, byte c, byte w);



#endif
