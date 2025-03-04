#ifdef PLATFORM_ESPIDF

#include "../hal_adc.h"
#include "../../logging/logging.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal_generic_espidf.h"

extern espPinMapping_t g_pins[];
extern int g_numPins;

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;
static bool g_adc1_conf = false, g_adc2_conf = false;

void adc_err(int pin)
{
	ADDLOG_ERROR(LOG_FEATURE_PINS, "Selected pin (%i) is neither ADC1 or ADC2", pin);
}

void HAL_ADC_Init(int pinNumber)
{
	if(pinNumber >= g_numPins)
		return;
	espPinMapping_t* pin = g_pins + pinNumber;

	adc_channel_t channel;
	adc_unit_t unit_id;
	esp_err_t err = adc_oneshot_io_to_channel(pin->pin, &unit_id, &channel);
	if(err != ESP_OK)
	{
		adc_err(pin->pin);
		return;
	}
	else
	{
		adc_oneshot_unit_init_cfg_t init_config =
		{
			.unit_id = unit_id,
			.ulp_mode = ADC_ULP_MODE_DISABLE,
		};
		if(unit_id == ADC_UNIT_1 && !g_adc1_conf)
		{
			adc_oneshot_new_unit(&init_config, &adc1_handle);
			ADDLOG_DEBUG(LOG_FEATURE_PINS, "Init ADC1");
			g_adc1_conf = true;
		}
		else if(unit_id == ADC_UNIT_2 && !g_adc2_conf)
		{
			adc_oneshot_new_unit(&init_config, &adc2_handle);
			ADDLOG_DEBUG(LOG_FEATURE_PINS, "Init ADC2");
			g_adc2_conf = true;
		}
	}

	adc_oneshot_chan_cfg_t config =
	{
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};

	if(unit_id == ADC_UNIT_1)
	{
		adc_oneshot_config_channel(adc1_handle, channel, &config);
	}
	else if(unit_id == ADC_UNIT_2)
	{
		adc_oneshot_config_channel(adc2_handle, channel, &config);
	}
	else
	{
		adc_err(pin->pin);
		return;
	}
	ADDLOG_DEBUG(LOG_FEATURE_PINS, "Init ADC%d Channel[%d]", unit_id + 1, channel);
}

int HAL_ADC_Read(int pinNumber)
{
	if(pinNumber >= g_numPins)
		return 0;
	espPinMapping_t* pin = g_pins + pinNumber;
	int raw = 0;
	adc_channel_t channel;
	adc_unit_t unit_id;
	esp_err_t err = adc_oneshot_io_to_channel(pin->pin, &unit_id, &channel);
	if(err != ESP_OK)
	{
		adc_err(pin->pin);
	}
	else
	{
		if(unit_id == ADC_UNIT_1)
		{
			adc_oneshot_read(adc1_handle, channel, &raw);
		}
		else if(unit_id == ADC_UNIT_2)
		{
			adc_oneshot_read(adc2_handle, channel, &raw);
		}
		else
		{
			adc_err(pin->pin);
		}
	}
	return raw;
}

#endif // PLATFORM_ESPIDF
