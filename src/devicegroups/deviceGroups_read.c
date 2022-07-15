#include "deviceGroups_public.h"
#include "deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "../logging/logging.h"
#include "lwip/inet.h"



int DGR_Parse(const byte *data, int len, dgrDevice_t *dev, struct sockaddr *addr) {
	bitMessage_t msg;
	char groupName[32];
	int sequence, flags, type;
	int bGotEOL = 0;
	int relayFlags,i;
	byte vals;
	byte relaysCnt;

	MSG_BeginReading(&msg,data,len);

	if(MSG_CheckAndSkip(&msg,TASMOTA_DEVICEGROUPS_HEADER,strlen(TASMOTA_DEVICEGROUPS_HEADER))==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: data chunk with len %i had bad header\n",len);
		return 1;
	}
	if(MSG_ReadString(&msg,groupName,sizeof(groupName)) <= 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: data chunk with len %i failed to read group name\n",len);
		return 1;
	}
	if(dev != 0) {
		// right now, only single group support
		if(strcmp(dev->gr.groupName,groupName)) {
			return -1;
		}
	}
	sequence = MSG_ReadU16(&msg);
	flags = MSG_ReadU16(&msg);

	// ack, not supported yet
	if(flags == 8) {
		return 1;
	}

	if(dev->cbs.checkSequence(sequence)) {
		return 1;
	}


	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: [%s] seq %i, flags %i\n",inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),sequence, flags);
	while(MSG_EOF(&msg)==0) {
		type = MSG_ReadByte(&msg);
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Next section - %i\n",type);
		if(type == DGR_ITEM_EOL) {
			bGotEOL = 1;
		} else if(type < DGR_ITEM_MAX_8BIT) {
			vals = MSG_ReadByte(&msg);
			if(type == DGR_ITEM_BRI_POWER_ON) {
				addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_ITEM_BRI_POWER_ON: %i\n",vals);
				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processBrightnessPowerOn(vals);
					}
				}
			} else if(type == DGR_ITEM_LIGHT_BRI) {
				addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_ITEM_LIGHT_BRI: %i\n",vals);
				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processLightBrightness(vals);
					}
				}
			} else {

			}
		} else if(type < DGR_ITEM_MAX_16BIT) {
			MSG_SkipBytes(&msg,2);
		} else if(type < DGR_ITEM_MAX_32BIT) {
			if(type == DGR_ITEM_POWER) {
				int total = 0;

				relayFlags = MSG_Read3Bytes(&msg);
				relaysCnt = MSG_ReadByte(&msg);

				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processPower(relayFlags,relaysCnt);
					}
				}

				addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Power event - values %i, numChannels %i, chans=",relayFlags,relaysCnt);
				for(i = 0; i < relaysCnt; i++) {
					if(BIT_CHECK(relayFlags,i)) {
						addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"[ON]");
					} else {
						addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"[OFF]");
					}
				}
				addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"\n");
			} else {
				MSG_SkipBytes(&msg,4);
			}
		} else if(type < DGR_ITEM_MAX_STRING) {
			byte sLen = MSG_ReadByte(&msg);
			// DevGroupSend 193=Tst
			// Gives sLen 4
			if(type == DGR_ITEM_COMMAND) {
				const char *cmd = MSG_GetStringPointerAtCurrentPosition(&msg);
				addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_ITEM_COMMAND: %s\n",cmd);
			}
			MSG_SkipBytes(&msg,sLen);
		} else if(type == DGR_ITEM_LIGHT_CHANNELS) {
			byte sLen = MSG_ReadByte(&msg);
			if(sLen == 5) {
				byte r, g, b, c, w;
				r = MSG_ReadByte(&msg);
				g = MSG_ReadByte(&msg);
				b = MSG_ReadByte(&msg);
				c = MSG_ReadByte(&msg);
				w = MSG_ReadByte(&msg);

				dev->cbs.processRGBCW(r,g,b,c,w);
			} else {
				MSG_SkipBytes(&msg,sLen);
			}
		} else {
			byte sLen = MSG_ReadByte(&msg);
			MSG_SkipBytes(&msg,sLen);
		}
		if(bGotEOL) {
			break;
		}
	}


	return 0;
}