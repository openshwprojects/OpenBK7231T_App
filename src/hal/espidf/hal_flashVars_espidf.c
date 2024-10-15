#ifdef PLATFORM_ESPIDF

#include "../../new_cfg.h"
#include "../../logging/logging.h"
#include "../../new_common.h"
#include "../hal_flashVars.h"
#include "nvs_flash.h"
#include "nvs.h"

void InitFlashIfNeeded();

void HAL_FlashVars_IncreaseBootCount()
{
	uint32_t bootc = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_get_u32(handle, "bootc", &bootc);
	nvs_set_u32(handle, "bootc", ++bootc);
	nvs_commit(handle);
	nvs_close(handle);
}

int HAL_FlashVars_GetChannelValue(int ch)
{
	char channel[4];
	sprintf(channel, "ch%i", ch);
	int32_t value = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_i32(handle, channel, &value);
	nvs_close(handle);
	return value;
}

void HAL_FlashVars_SaveChannel(int index, int value)
{
	char channel[4];
	sprintf(channel, "ch%i", index);
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_set_i16(handle, channel, value);
	nvs_commit(handle);
	nvs_close(handle);
}

void HAL_FlashVars_ReadLED(byte* mode, short* brightness, short* temperature, byte* rgb, byte* bEnableAll)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_u8(handle, "mode", mode);
	nvs_get_i16(handle, "brs", brightness);
	nvs_get_i16(handle, "temp", temperature);
	nvs_get_u8(handle, "r", &rgb[0]);
	nvs_get_u8(handle, "g", &rgb[1]);
	nvs_get_u8(handle, "b", &rgb[2]);
	nvs_get_u8(handle, "ena", bEnableAll);
	nvs_close(handle);
}


void HAL_FlashVars_SaveLED(byte mode, short brightness, short temperature, byte r, byte g, byte b, byte bEnableAll)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_set_u8(handle, "mode", mode);
	nvs_set_i16(handle, "brs", brightness);
	nvs_set_i16(handle, "temp", temperature);
	nvs_set_u8(handle, "r", r);
	nvs_set_u8(handle, "g", g);
	nvs_set_u8(handle, "b", b);
	nvs_set_u8(handle, "ena", bEnableAll);
	nvs_commit(handle);
	nvs_close(handle);
}

short HAL_FlashVars_ReadUsage()
{
	short usage = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_i16(handle, "tu", &usage);
	nvs_close(handle);
	return usage;
}

void HAL_FlashVars_SaveTotalUsage(short usage)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_set_i16(handle, "tu", usage);
	nvs_commit(handle);
	nvs_close(handle);
}

void HAL_FlashVars_SaveBootComplete()
{
	uint32_t bootc = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_get_u32(handle, "bootc", &bootc);
	nvs_set_u32(handle, "bootsc", bootc);
	nvs_commit(handle);
	nvs_close(handle);
}

// call to return the number of boots since a HAL_FlashVars_SaveBootComplete
int HAL_FlashVars_GetBootFailures()
{
	uint32_t bootc = 0, bootsc = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_u32(handle, "bootc", &bootc);
	nvs_get_u32(handle, "bootsc", &bootsc);
	nvs_close(handle);
	return bootc - bootsc;
}

int HAL_FlashVars_GetBootCount()
{
	uint32_t bootc = 0;
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	nvs_get_u32(handle, "bootc", &bootc);
	nvs_close(handle);
	return bootc;
}

int HAL_GetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READONLY, &handle);
	size_t size = sizeof(data);
	nvs_get_blob(handle, "emd", data, &size);
	nvs_close(handle);
	return 0;
}

int HAL_SetEnergyMeterStatus(ENERGY_METERING_DATA* data)
{
	InitFlashIfNeeded();
	nvs_handle_t handle = 0;
	nvs_open("config", NVS_READWRITE, &handle);
	nvs_set_blob(handle, "emd", data, sizeof(data));
	nvs_commit(handle);
	nvs_close(handle);
	return 0;
}

void HAL_FlashVars_SaveTotalConsumption(float total_consumption)
{

}

#endif // PLATFORM_ESPIDF
