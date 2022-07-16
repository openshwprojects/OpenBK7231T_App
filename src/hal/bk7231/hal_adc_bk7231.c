#include "../hal_adc.h"

#include <include.h>
#include <arm_arch.h>
#include <saradc_pub.h>
#include <drv_model_pub.h>
#include <rtos_pub.h>
#include <rtos_error.h>
#include <sys_ctrl_pub.h>
#include "../../logging/logging.h"
#include "../../new_common.h"

#include <saradc_pub.h>
#include <drv_model_pub.h>


static int adcToGpio[] = {
	-1,		// ADC0 - VBAT
	4, //GPIO4,	// ADC1
	5, //GPIO5,	// ADC2
	23,//GPIO23, // ADC3
	2,//GPIO2,	// ADC4
	3,//GPIO3,	// ADC5
	12,//GPIO12, // ADC6
	13,//GPIO13, // ADC7
};
static int c_adcToGpio = sizeof(adcToGpio)/sizeof(adcToGpio[0]);


static uint8_t gpioToAdc(int gpio) {
	uint8_t i;
	for ( i = 0; i < sizeof(adcToGpio); i++) {
		if (adcToGpio[i] == gpio)
			return i;
	}
	return -1;
}

void HAL_ADC_Init(int pinNumber) {

}
//int HAL_ADC_Read(int pinNumber) {
//	UINT32 status;
//	saradc_desc_t adc;
//	DD_HANDLE handle;
//	
//	saradc_config_param_init(&adc);
//	adc.channel		   = gpioToAdc(pinNumber);
//	adc.mode		   = ADC_CONFIG_MODE_CONTINUE;
//	adc.pData		   = adcData;
//	adc.data_buff_size = 1;
//	handle			   = ddev_open(SARADC_DEV_NAME, &status, (uint32_t)&adc);
//	if (status)
//		return 0;
//	// wait for data
//	while (!adc.has_data || adc.current_sample_data_cnt < 1) {
//		delay(1);
//	}
//	ddev_control(handle, SARADC_CMD_RUN_OR_STOP_ADC, (void *)false);
//	ddev_close(handle);
//	return adcData[0];
//}

#define ADC_TEMP_BUFFER_SIZE 5
//#define ADC_TEMP_SENSER_CHANNEL 3

// for single temp dectect
static saradc_desc_t tmp_single_desc;
static UINT16 tmp_single_buff[ADC_TEMP_BUFFER_SIZE];
static volatile DD_HANDLE tmp_single_hdl = DD_HANDLE_UNVALID;
static beken_semaphore_t tmp_single_semaphore = NULL;


static void temp_single_get_disable(void)
{
    UINT32 status = DRV_SUCCESS;
    
    status = ddev_close(tmp_single_hdl);
    if(DRV_FAILURE == status )
    {
        //TMP_DETECT_PRT("saradc disable failed\r\n");
        //return;
    }
    saradc_ensure_close();
    tmp_single_hdl = DD_HANDLE_UNVALID;
    
    status = BLK_BIT_TEMPRATURE_SENSOR;
    sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_BLK_DISABLE, &status);
    
}
static void temp_single_detect_handler(void)
{
    if(tmp_single_desc.current_sample_data_cnt >= tmp_single_desc.data_buff_size) 
    {
        turnon_PA_in_temp_dect();
        temp_single_get_disable();

        
        rtos_set_semaphore(&tmp_single_semaphore);
    }
}
static void temp_single_get_desc_init(int ch)
{
    os_memset(&tmp_single_buff[0], 0, sizeof(UINT16)*ADC_TEMP_BUFFER_SIZE);

    saradc_config_param_init(&tmp_single_desc);
    tmp_single_desc.channel = ch;
    tmp_single_desc.pData = &tmp_single_buff[0];
    tmp_single_desc.data_buff_size = ADC_TEMP_BUFFER_SIZE;
    tmp_single_desc.p_Int_Handler = temp_single_detect_handler;
}

static UINT32 temp_single_get_enable(int ch)
{
    UINT32 status;
    GLOBAL_INT_DECLARATION();

    temp_single_get_desc_init(ch);

  //  status = BLK_BIT_TEMPRATURE_SENSOR;
   // sddev_control(SCTRL_DEV_NAME, CMD_SCTRL_BLK_ENABLE, &status);

#if (CFG_SOC_NAME == SOC_BK7231)
    turnoff_PA_in_temp_dect();
#endif // (CFG_SOC_NAME == SOC_BK7231)
    GLOBAL_INT_DISABLE();
    tmp_single_hdl = ddev_open(SARADC_DEV_NAME, &status, (UINT32)&tmp_single_desc);
    if(DD_HANDLE_UNVALID == tmp_single_hdl)
    {
        GLOBAL_INT_RESTORE();
       // TMP_DETECT_FATAL("Can't open saradc, have you register this device?\r\n");
        return SARADC_FAILURE;
    }   
    GLOBAL_INT_RESTORE();
    
    return SARADC_SUCCESS;
}

int HAL_ADC_Read(int pinNumber)
{
    UINT32 ret;
    int result;
	int channel;

	channel = gpioToAdc(pinNumber);

	if(channel == -1) {
		return -1;
	}

    if(tmp_single_semaphore == NULL) {
        result = rtos_init_semaphore(&tmp_single_semaphore, 1);
        ASSERT(kNoErr == result);
    }
    
	if(temp_single_get_enable(channel)==SARADC_FAILURE) {

		return -2;
	}
    
    ret = 1000; // 1s
    result = rtos_get_semaphore(&tmp_single_semaphore, ret);
    if(result == kNoErr) {
        return tmp_single_desc.pData[0];
    } 
    return -3;
}

