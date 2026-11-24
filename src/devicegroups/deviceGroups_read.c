#include "deviceGroups_public.h"
#include "deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "../logging/logging.h"
#include "lwip/inet.h"

static bool DGR_ShouldProcessItem(dgrDevice_t *dev, byte item, byte itemFlags) {
	unsigned int mask;
	unsigned int noStatusShare = 0;
	if (dev == 0) return false;
	mask = DGR_GetMaskForItem(item);
	if (dev->gr.noStatusShare) {
		noStatusShare = *dev->gr.noStatusShare;
		if (itemFlags & DGR_ITEM_FLAG_NO_SHARE) noStatusShare |= mask;
		else noStatusShare &= ~mask;
		*dev->gr.noStatusShare = noStatusShare;
	}
	return (!mask || (dev->gr.devGroupShare_In & mask))
		&& (dev->gr.local || !(noStatusShare & mask));
}

int DGR_Parse(const byte *data, int len, dgrDevice_t *dev, struct sockaddr *addr) {
	bitMessage_t msg;
	char groupName[32];
	uint16_t sequence;
	uint16_t flags;
	byte itemFlags = 0;

	if (data == 0 || len <= 0) return -2;
	MSG_BeginReading(&msg, data, len);
	if (MSG_CheckAndSkip(&msg, TASMOTA_DEVICEGROUPS_HEADER, strlen(TASMOTA_DEVICEGROUPS_HEADER)) == 0
		|| MSG_ReadString(&msg, groupName, sizeof(groupName)) <= 0
		|| msg.position + 4 > len) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_Parse: malformed header, len %i", len);
		return -2;
	}
	if (dev && strcmp(dev->gr.groupName, groupName)) return -1;
	sequence = MSG_ReadU16(&msg);
	flags = MSG_ReadU16(&msg);
	if (addr) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_Parse: [%s] group %s seq 0x%04X, flags 0x%02X",
			inet_ntoa(((struct sockaddr_in *)addr)->sin_addr), groupName, sequence, flags);
	}
	if (flags == DGR_FLAG_ANNOUNCEMENT) return 0;
	if (flags & DGR_FLAG_STATUS_REQUEST) {
		if (dev && dev->cbs.sendFullStatus) dev->cbs.sendFullStatus();
		return 0;
	}
	if (dev && dev->cbs.checkSequence && dev->cbs.checkSequence(sequence)) return 1;

	while (!MSG_EOF(&msg)) {
		byte item = MSG_ReadByte(&msg);
		uint32_t value = 0;
		bool process;
		if (item == DGR_ITEM_EOL) return 0;
		if (item <= DGR_ITEM_MAX_8BIT) {
			if (msg.position + 1 > msg.totalSize) return -2;
			value = MSG_ReadByte(&msg);
		} else if (item <= DGR_ITEM_MAX_16BIT) {
			if (msg.position + 2 > msg.totalSize) return -2;
			value = MSG_ReadU16(&msg);
		} else if (item <= DGR_ITEM_MAX_32BIT) {
			if (msg.position + 4 > msg.totalSize) return -2;
			value = MSG_Read3Bytes(&msg);
			value |= ((uint32_t)MSG_ReadByte(&msg)) << 24;
		} else {
			byte valueLen;
			const char *valuePtr;
			if (msg.position + 1 > msg.totalSize) return -2;
			valueLen = MSG_ReadByte(&msg);
			if (msg.position + valueLen > msg.totalSize) return -2;
			valuePtr = MSG_GetStringPointerAtCurrentPosition(&msg);
			process = DGR_ShouldProcessItem(dev, item, itemFlags);
			if (process && item == DGR_ITEM_EVENT && dev->cbs.processEvent) {
				dev->cbs.processEvent(valuePtr, valueLen);
			} else if (process && item == DGR_ITEM_COMMAND && dev->cbs.processCommand) {
				dev->cbs.processCommand(valuePtr, valueLen);
			} else if (process && item == DGR_ITEM_LIGHT_CHANNELS && dev->cbs.processRGBCW) {
				byte channels[6] = { 0 };
				byte copyLen = valueLen < sizeof(channels) ? valueLen : sizeof(channels);
				memcpy(channels, valuePtr, copyLen);
				dev->cbs.processRGBCW(channels);
			}
			MSG_SkipBytes(&msg, valueLen);
			itemFlags = 0;
			continue;
		}
		if (item == DGR_ITEM_FLAGS) {
			itemFlags = (byte)value;
			continue;
		}
		if (item == DGR_ITEM_NO_STATUS_SHARE) {
			if (dev && dev->gr.noStatusShare) *dev->gr.noStatusShare = value;
			itemFlags = 0;
			continue;
		}
		/* Validate values independently of the configured receive mask. */
		if (item == DGR_ITEM_POWER && (value >> 24) > 24) return -2;
		DGR_CommandSetValue(dev ? dev->gr.stateIndex : 0, item, value);
		process = DGR_ShouldProcessItem(dev, item, itemFlags);
		if (process) {
			switch (item) {
				case DGR_ITEM_LIGHT_FADE: if (dev->cbs.processLightFade) dev->cbs.processLightFade((byte)value); break;
				case DGR_ITEM_LIGHT_SPEED: if (dev->cbs.processLightSpeed) dev->cbs.processLightSpeed((byte)value); break;
				case DGR_ITEM_LIGHT_BRI: if (dev->cbs.processLightBrightness) dev->cbs.processLightBrightness((byte)value); break;
				case DGR_ITEM_LIGHT_SCHEME: if (dev->cbs.processLightScheme) dev->cbs.processLightScheme((byte)value); break;
				case DGR_ITEM_LIGHT_FIXED_COLOR: if (dev->cbs.processLightFixedColor) dev->cbs.processLightFixedColor((byte)value); break;
				case DGR_ITEM_BRI_PRESET_LOW: if (dev->cbs.processBrightnessPresetLow) dev->cbs.processBrightnessPresetLow((byte)value); break;
				case DGR_ITEM_BRI_PRESET_HIGH: if (dev->cbs.processBrightnessPresetHigh) dev->cbs.processBrightnessPresetHigh((byte)value); break;
				case DGR_ITEM_BRI_POWER_ON: if (dev->cbs.processBrightnessPowerOn) dev->cbs.processBrightnessPowerOn((byte)value); break;
				case DGR_ITEM_POWER:
					if (dev->cbs.processPower) dev->cbs.processPower(value & 0xFFFFFF, value >> 24);
					break;
			}
		}
		itemFlags = 0;
	}
	return 0;
}
