
#include "../new_common.h"
#include "deviceGroups_public.h"

#define TASMOTA_DEVICEGROUPS_HEADER "TASMOTA_DGR"

#define DGR_ITEM_EOL				0
#define DGR_ITEM_STATUS				1
#define DGR_ITEM_FLAGS				2
#define DGR_ITEM_LIGHT_FADE			3
#define DGR_ITEM_LIGHT_SPEED		4
#define DGR_ITEM_LIGHT_BRI			5
#define DGR_ITEM_LIGHT_SCHEME		6
#define DGR_ITEM_LIGHT_FIXED_COLOR	7
#define DGR_ITEM_BRI_PRESET_LOW		8
#define DGR_ITEM_BRI_PRESET_HIGH	9
#define DGR_ITEM_BRI_POWER_ON		10
#define DGR_ITEM_LAST_8BIT			11
#define DGR_ITEM_MAX_8BIT			63

#define DGR_ITEM_LAST_16BIT			64
#define DGR_ITEM_MAX_16BIT			127

#define DGR_ITEM_POWER				128
#define DGR_ITEM_NO_STATUS_SHARE	129
#define DGR_ITEM_LAST_32BIT			130
#define DGR_ITEM_MAX_32BIT			191

#define DGR_ITEM_EVENT				192
#define DGR_ITEM_COMMAND			193
#define DGR_ITEM_LAST_STRING		194
#define DGR_ITEM_MAX_STRING			223

#define DGR_ITEM_LIGHT_CHANNELS		224
#define DGR_ITEM_LAST_ARRAY			225
#define DGR_ITEM_MAX_ARRAY			255



#define DGR_RELAY_NONE	0
#define DGR_RELAY_1		1
#define DGR_RELAY_2		2
#define DGR_RELAY_3		4
#define DGR_RELAY_4		8
#define DGR_RELAY_5		16
#define DGR_RELAY_6		32
#define DGR_RELAY_7		64
#define DGR_RELAY_8		128
#define DGR_RELAY_9		256
#define DGR_RELAY_10	512
#define DGR_RELAY_11	1024
#define DGR_RELAY_12	2048
#define DGR_RELAY_13	4096
#define DGR_RELAY_14	8192
#define DGR_RELAY_15	16384
#define DGR_RELAY_16	32768
#define DGR_RELAY_17	65536
#define DGR_RELAY_18	131072
#define DGR_RELAY_19	262144
#define DGR_RELAY_20	524288
#define DGR_RELAY_21	1048576
#define DGR_RELAY_22	2097152
#define DGR_RELAY_23	4194304
#define DGR_RELAY_24	8388608





unsigned int DGR_GetMaskForItem(byte item);
int DGR_IsItemInMask(byte item, unsigned int mask);



