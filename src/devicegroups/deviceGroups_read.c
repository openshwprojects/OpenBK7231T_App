#include "deviceGroups_public.h"
#include "deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "../logging/logging.h"
#include "lwip/inet.h"



int DGR_Parse(const byte *data, int len, dgrDevice_t *dev, struct sockaddr *addr) {
	bitMessage_t msg;
	char groupName[32];
	uint16_t sequence;
	int flags, type;
	int bGotEOL = 0;
	int relayFlags,i;
	byte vals;
	byte relaysCnt;

	MSG_BeginReading(&msg,data,len);

	if(MSG_CheckAndSkip(&msg,TASMOTA_DEVICEGROUPS_HEADER,strlen(TASMOTA_DEVICEGROUPS_HEADER))==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: data chunk with len %i had bad header",len);
		return 1;
	}
	if(MSG_ReadString(&msg,groupName,sizeof(groupName)) <= 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: data chunk with len %i failed to read group name",len);
		return 1;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: grp name %s len %d", groupName, strlen(groupName));

	if(dev != 0) {
		// right now, only single group support
		if(strcmp(dev->gr.groupName,groupName)) {
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR,"DGR ignoring message from group %s - device is in %s",groupName,dev->gr.groupName);
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
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR,"DGR ignoring message from duplicate or older sequence %i",sequence);
		return 1;
	}


	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_Parse: [%s] seq 0x%04X, flags 0x%02X",inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),sequence, flags);

	while(MSG_EOF(&msg)==0) {
		type = MSG_ReadByte(&msg);
		addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"Next section - %i",type);
		if(type == DGR_ITEM_EOL) {
			bGotEOL = 1;
		} else if(type < DGR_ITEM_MAX_8BIT) {
			vals = MSG_ReadByte(&msg);
			if(type == DGR_ITEM_BRI_POWER_ON) {
				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DGR_ITEM_BRI_POWER_ON: %i",vals);
				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processBrightnessPowerOn(vals);
					}
				}
			} else if(type == DGR_ITEM_LIGHT_BRI) {
				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DGR_ITEM_LIGHT_BRI: %i",vals);
				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processLightBrightness(vals);
					}
				}
			}
			else if (type == DGR_ITEM_LIGHT_FIXED_COLOR) {
				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DGR_ITEM_LIGHT_FIXED_COLOR: %i", vals);
				// FORWARD TO PROCESSING BY API
				if (dev) {
					if (DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processLightFixedColor(vals);
					}
				}
			} else {
				
			}
		} else if(type < DGR_ITEM_MAX_16BIT) {
			MSG_SkipBytes(&msg,2);
		} else if(type < DGR_ITEM_MAX_32BIT) {
			if(type == DGR_ITEM_POWER) {

				relayFlags = MSG_Read3Bytes(&msg);
				relaysCnt = MSG_ReadByte(&msg);

				// FORWARD TO PROCESSING BY API
				if(dev) {
					if(DGR_IsItemInMask(type, dev->gr.devGroupShare_In)) {
						dev->cbs.processPower(relayFlags,relaysCnt);
					}
				}

				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"Power event - values %i, numChannels %i, chans=",relayFlags,relaysCnt);
				for(i = 0; i < relaysCnt; i++) {
					if(BIT_CHECK(relayFlags,i)) {
						addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"[ON]");
					} else {
						addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"[OFF]");
					}
				}
				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"\n");
			} else {
				MSG_SkipBytes(&msg,4);
			}
		} else if(type < DGR_ITEM_MAX_STRING) {
			byte sLen = MSG_ReadByte(&msg);
			// DevGroupSend 193=Tst
			// Gives sLen 4
			if(type == DGR_ITEM_COMMAND) {
				const char *cmd = MSG_GetStringPointerAtCurrentPosition(&msg);
				addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DGR_ITEM_COMMAND: %s",cmd);
			}
			MSG_SkipBytes(&msg,sLen);
		} else if(type == DGR_ITEM_LIGHT_CHANNELS) {
			byte sLen = MSG_ReadByte(&msg);
			// array of channels.
			// process as many as we find
			// note: from TAS h801, I get 6?!!

			// NOTE2: If byte is zero -> no change????
			// e.g. from h801, I get 00 00 00 xx yy

			// NOTE3: last byte (dat[5], 6th byte) in array seems to be an 8 bit sequence for color cmds?

			if(sLen > 0) {
				int count = 0;
				byte dat[6] = {0,0,0,0,0, 0};

				while (sLen && count < 6){
					dat[count] = MSG_ReadByte(&msg);
					count++;
					sLen--;
				}

				if (count == 3){
					// should we do something different for 3?
					// no, other byts woill be zero, so process will work.
				}
				if ((count == 5) || (count == 6)){

				}
				dev->cbs.processRGBCW(dat);
			}
			// skip any remaining from array.
			MSG_SkipBytes(&msg,sLen);
			
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