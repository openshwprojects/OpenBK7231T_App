#include "../new_common.h"
#include "../httpserver/http_fns.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../driver/drv_tuyaMCU.h"
#include "../driver/drv_public.h"
#include "../hal/hal_wifi.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_flashVars.h"
#include "../hal/hal_flashConfig.h"
#include "../logging/logging.h"
#include "../devicegroups/deviceGroups_public.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/hass.h"
#include "../cJSON/cJSON.h"
#include <time.h>
#include "../driver/drv_ntp.h"
#include "../driver/drv_local.h"

#include "domoticz.h"


// #if ENABLE_DOMOTICZ

int is_not_in_table(uint16_t number, const uint16_t* table, size_t size);
float Dz_energyMetering(energySensor_t sensor);

/**
 * @brief Parse a JSON string and extract "dtype" and "nvalue" fields
 * @param[in] json_str string to parse
 * @param[out] dtype pointer to a buffer to store the "dtype" field
 * @param[out] nvalue_ptr pointer to an integer to store the "nvalue" field
 * @return 0 if successful, -1 otherwise
 */
int parse_json(char* json_str, char* dtype, int* nvalue_ptr) {  
    cJSON* root = cJSON_Parse(json_str);
    if (root == NULL) {
        return -1;
    }

    cJSON* dtype_item = cJSON_GetObjectItem(root, "dtype");
    if (dtype_item != NULL && cJSON_IsString(dtype_item)) {
        strcpy(dtype, dtype_item->valuestring);
    }

    cJSON* nvalue_item = cJSON_GetObjectItem(root, "nvalue");
    if (nvalue_item != NULL && cJSON_IsNumber(nvalue_item)) {
        *nvalue_ptr = (int) cJSON_GetNumberValue(nvalue_item);
    }

    cJSON_Delete(root);
    return 0;
}

/**
 * @brief Publish a message to domoticz for a channel
 *
 * The message is in the following format:
 * {"idx":<idx>,"nvalue":<nvalue>, "svalue":"<svalue>","Battery":<Battery>,"RSSI":<RSSI>}
 * where:
 * - idx is the index configured in the domoticz section of the config
 * - nvalue is the numeric value of the channel
 * - svalue is the string value of the channel
 * - Battery is the battery level of the device (for battery powered devices)
 * - RSSI is the RSSI of the device (for wifi devices)
 *
 * @param ch the channel number
 * @param nvalue the numeric value of the channel
 * @param svalue the string value of the channel
 * @param Battery the battery level of the device (for battery powered devices)
 * @param RSSI the RSSI of the device (for wifi devices)
 * 
 */
void send_domoticz_message(uint16_t ch, int nvalue, char* svalue, int Battery, int RSSI) {
    char msg[150];
    char tmpA[30];
    int idx = CFG_GetDomoticzIndex(ch); // get domoticz index corresponding to OB channel
    // TODO : support different type of devices (Power Sensor, ...)
	sprintf(msg, "{\"idx\":%d,\"nvalue\":%d, \"svalue\":\"%s\",\"Battery\":%d,\"RSSI\":%d}",idx, nvalue, svalue, Battery, RSSI);
    Dz_energyMetering(OBK_VOLTAGE);
    MQTT_Publish("domoticz","in", msg, 0);

}


int Dz_GetListOfChannelsUsed(int* listOfChannels, int numUsed) {
    int ch;
    int j = 0;

    for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
        ch = PIN_GetPinChannelForPinIndex(i);
        if (ch > 0) {
            if (j < numUsed) {
                listOfChannels[j++] = ch;
            }
        }
    }
    return j;  // number of channels used
}

void Dz_PublishAll() {
    int numUsed = Dz_GetNumberOfChannels();
    int channelsUsed[numUsed];
    Dz_GetListOfChannelsUsed(channelsUsed, numUsed);

    for (int j = 0; j < numUsed; j++) {
        int ch = channelsUsed[j];
        send_domoticz_message(ch, (int) CHANNEL_GetFinalValue(ch), "update", 100, (-1)*HAL_GetWifiStrength());
    }
}

#if ENABLE_BL_SHARED
float Dz_energyMetering(energySensor_t sensor){
    addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Domoticz: Energy Metering");
    if (DRV_IsMeasuringPower())
    {	
		float reading;
		if (sensor == OBK_VOLTAGE){
			reading = DRV_GetReading(sensor);
			addLogAdv(LOG_INFO, LOG_DEBUG, "Domoticz: %s: %.2f", sensor, reading);
			return reading;
		}
		if (sensor == OBK_POWER){
			reading = DRV_GetReading(sensor);
			addLogAdv(LOG_INFO, LOG_DEBUG, "Domoticz: %s: %.2f", sensor, reading);
			return reading;
			// simplified version : 
			// no definition of reading, its here fore debugging purposes
			// return DRV_GetReading(OBK_VOLTAGE);
		}
		if (sensor == OBK_CONSUMPTION_TOTAL){
			reading = DRV_GetReading(sensor);
			if (isnan(reading)){
				return 0;
			}
			else{
				return reading;
			}
		}
    }
	return 0;
}

void Dz_PublishEnergy(){
	// TODO : Check if idx is null and then not send value if true
	char tmpA[20];
	char msg[150];
	float Voltage = Dz_energyMetering(OBK_VOLTAGE);
	sprintf(tmpA, "%.2f", Voltage);
	sprintf(msg, "{\"idx\":%d, \"svalue\":\"%s\"}",CFG_GetDomoticzVoltageIndex(), tmpA);
	MQTT_Publish("domoticz","in", msg, 0);
	// and power
	float Power = Dz_energyMetering(OBK_POWER);
	float Consumption = Dz_energyMetering(OBK_CONSUMPTION_TOTAL);
	sprintf(tmpA, "%.2f;%.3f", Power, Consumption);	// consumption in Wh in domoticz
	sprintf(msg, "{\"idx\":%d, \"svalue\":\"%s\"}",CFG_GetDomoticzPowerIndex(), tmpA);
	MQTT_Publish("domoticz","in", msg, 0);
}
#endif

/**
 * @brief Given a domoticz index, return the channel number associated with it
 *
 * Iterate over all channels and return the channel number associated with the given domoticz index.
 * If no channel is found, return 0.
 *
 * @param idx the index configured in the domoticz section of the config (g_cfg.domoticz_idx)
 * @return the channel number associated with the given domoticz index
 */
int Dz_GetChannelFromIndex(int idx){
    int i;
    int ch = 0;
    for (i=0; i<PLATFORM_GPIO_MAX; i++){
        ch = PIN_GetPinChannelForPinIndex(i);
        if (CFG_GetDomoticzIndex(ch) == idx){
            return ch;
        }
    }
    return 0;
}

/**
 * @brief Return the number of channels currently in use.
 *
 * Iterate over all GPIO pins and count the number of channels currently
 * assigned to any pin. This provides the number of channels that are actively
 * being used.
 *
 * @return The number of channels currently in use.
 */
int Dz_GetNumberOfChannels(){
    int i;
    int ch = 0;
    int maxChannels = 0;
    for (i=0; i<PLATFORM_GPIO_MAX; i++){
        ch = PIN_GetPinChannelForPinIndex(i);
        if (ch > 0){
            maxChannels++;
        }
    }
    return maxChannels;
}

int is_not_in_table(uint16_t number, const uint16_t* table, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (table[i] == number) {
            return 0;
        }
    }
    return 1;
}


// #endif // ENABLE_MQTT