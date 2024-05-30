#include "deviceGroups_local.h"

unsigned int DGR_GetMaskForItem(byte item)
{
	unsigned int mask = 0;
	if (item == DGR_ITEM_LIGHT_BRI || item == DGR_ITEM_BRI_POWER_ON)
		mask = DGR_SHARE_LIGHT_BRI;
	else if (item == DGR_ITEM_POWER)
		mask = DGR_SHARE_POWER;
	else if (item == DGR_ITEM_LIGHT_SCHEME)
		mask = DGR_SHARE_LIGHT_SCHEME;
	else if (item == DGR_ITEM_LIGHT_FIXED_COLOR || item == DGR_ITEM_LIGHT_CHANNELS)
		mask = DGR_SHARE_LIGHT_COLOR;
	else if (item == DGR_ITEM_LIGHT_FADE || item == DGR_ITEM_LIGHT_SPEED)
		mask = DGR_SHARE_LIGHT_FADE;
	else  if (item == DGR_ITEM_BRI_PRESET_LOW || item == DGR_ITEM_BRI_PRESET_HIGH)
		mask = DGR_SHARE_DIMMER_SETTINGS;
	else if (item == DGR_ITEM_EVENT)
		mask = DGR_SHARE_EVENT;
	return mask;
}
int DGR_IsItemInMask(byte item, unsigned int mask) {
	unsigned int itemMask;

	itemMask = DGR_GetMaskForItem(item);

	if(mask & itemMask)
		return 1;
	return 0;
}


