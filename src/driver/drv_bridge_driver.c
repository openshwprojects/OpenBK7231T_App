#include "../new_common.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_local.h"
#include "drv_public.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "../mqtt/new_mqtt.h"

/*
 * Bridge control structure */
typedef struct BRIDGE_CONTROL {
    int GPIO_HLW_FWD;                 // Forward direction control pin 
    int GPIO_HLW_REV;                 // reverse direction control pin
    int channel;                      // assigned channel (General Config)
    bool current_state;               // Current state
    bool new_state;                   // New state
    int pulseCnt;                     // Pulse Counter
    int pulseLen;                     // Pulse length
} BRIDGE_CONTROL;

/* Local variables */
static BRIDGE_CONTROL *br_ctrl = NULL;
static int ch_count = 0;

/**************************************************************************************/
// MosFet Bridge driver
void Bridge_driver_Init() 
{
    int i;
    int ch = 0;

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Bridge Driver Init.\n");	
   
    ch_count = PIN_CountPinsWithRole(IOR_BridgeForward);
    if (ch_count != PIN_CountPinsWithRole(IOR_BridgeReverse))
    {
        addLogAdv(LOG_WARN, LOG_FEATURE_DRV, "Bridge Driver Pins mismatched \n");
    }

    if (ch_count>0)
    {
        /* Brignde channel detected */
        addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Detected %i bridge channels\n", ch_count);
        if (br_ctrl != NULL)
            os_free(br_ctrl);
        /* Allocate memeory */
        br_ctrl = (BRIDGE_CONTROL *)os_malloc(sizeof(BRIDGE_CONTROL)*ch_count);
        /* Reset settings */
        for(ch=0;ch<ch_count;ch++)
        {
            br_ctrl[ch].GPIO_HLW_FWD = -1;
            br_ctrl[ch].GPIO_HLW_REV = -1;
            br_ctrl[ch].channel = -1;
            br_ctrl[ch].current_state = 0;
            br_ctrl[ch].new_state = -1;
            br_ctrl[ch].pulseCnt = 0;
            br_ctrl[ch].pulseLen = 50;
        }
        /* Read PIN Roles */
        ch = 0;
        for(i=0;i<PLATFORM_GPIO_MAX;i++)
        {
            if (PIN_GetPinRoleForPinIndex(i)==IOR_BridgeForward)
            {                
                br_ctrl[ch].GPIO_HLW_FWD = i;
                ch++;
                if (ch == ch_count)
                    break;
            }
        }
        ch = 0;
        for(i=0;i<PLATFORM_GPIO_MAX;i++)
        {
            if (PIN_GetPinRoleForPinIndex(i)==IOR_BridgeReverse)
            {
                br_ctrl[ch].GPIO_HLW_REV = i;
                ch++;
                if (ch == ch_count)
                    break;                
            }
        }
        /* Detect assigned channes */
        for(ch=0;ch<ch_count;ch++)
        {
            br_ctrl[ch].channel = PIN_GetPinChannelForPinIndex(br_ctrl[ch].GPIO_HLW_FWD);
            br_ctrl[ch].new_state = CHANNEL_Get(br_ctrl[ch].channel);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "BR%i GPIO_HLW_FWD = %i\n", ch, br_ctrl[ch].GPIO_HLW_FWD);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "BR%i GPIO_HLW_REV = %i\n", ch, br_ctrl[ch].GPIO_HLW_REV);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "BR%i Channel      = %i\n", ch, br_ctrl[ch].channel);
            addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "BR%i New State    = %i\n", ch, br_ctrl[ch].new_state);
        }
    } else {
        /* No Bridge drivers sefined */
        if (br_ctrl != NULL)
            os_free(br_ctrl);
        br_ctrl = NULL;
    }

    /* register commands */
}

/***************************************************************************************/
void Bridge_driver_DeInit()
{
    os_free(br_ctrl);
    br_ctrl = NULL;
    ch_count = 0;
}

/***************************************************************************************/
void Bridge_driver_QuickFrame() 
{
    int ch;

    if (br_ctrl != NULL)
    {
        for (ch=0;ch<ch_count;ch++)
        {
            if (br_ctrl[ch].pulseCnt>0)
            {
                /* Pulse timer */
                br_ctrl[ch].pulseCnt--;
                if (br_ctrl[ch].pulseCnt == 0)
                {
                    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Bridge Driver: %lu :PULSE Complete. HOLD\n", (unsigned long)xTaskGetTickCount());
                    HAL_PIN_SetOutputValue(br_ctrl[ch].GPIO_HLW_FWD, 0);
                    HAL_PIN_SetOutputValue(br_ctrl[ch].GPIO_HLW_REV, 0);
                }
            } 
            else if (br_ctrl[ch].current_state < br_ctrl[ch].new_state)
            {
                /* Detected change in state - Forward Move */
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Bridge Driver: %lu : FORWARD PULSE\n", (unsigned long)xTaskGetTickCount());
                br_ctrl[ch].pulseCnt = br_ctrl[ch].pulseLen;
                HAL_PIN_SetOutputValue(br_ctrl[ch].GPIO_HLW_FWD, 1);
                br_ctrl[ch].current_state = br_ctrl[ch].new_state;
                MQTT_ChannelChangeCallback(br_ctrl[ch].channel, br_ctrl[ch].new_state);
            }
            else if (br_ctrl[ch].current_state > br_ctrl[ch].new_state)
            {
                /* Detected change in state - Reverse Move */
                addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Bridge Driver: %lu : REVERSE PULSE\n", (unsigned long)xTaskGetTickCount());
                br_ctrl[ch].current_state = br_ctrl[ch].new_state;
                br_ctrl[ch].pulseCnt = br_ctrl[ch].pulseLen;
                HAL_PIN_SetOutputValue(br_ctrl[ch].GPIO_HLW_REV, 1);
                br_ctrl[ch].current_state = br_ctrl[ch].new_state;
                MQTT_ChannelChangeCallback(br_ctrl[ch].channel, br_ctrl[ch].new_state);
            }
        }
    }
}

/***************************************************************************************/
void Bridge_driver_OnChannelChanged(int ch, int value)
{
    int i;

    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Bridge Driver OnChannelChanged: CH:%i VAL:%i\n", ch, value);

    if (br_ctrl != NULL)
    {
        for (i=0;i<ch_count;i++)
        {
            if (br_ctrl[i].channel == ch)
            {
                br_ctrl[i].new_state = value;
            }
        }
    }
}

/***************************************************************************************/
OBK_Publish_Result Bridge_driver_ChannelPublish(int channel)
{
    int i;

    if (br_ctrl != NULL)
    {
        for (i=0;i<ch_count;i++)
        {
            if (br_ctrl[i].channel == channel)
            {
                return MQTT_ChannelChangeCallback(br_ctrl[i].channel, br_ctrl[i].current_state);
            }
        }
    }
    return OBK_PUBLISH_WAS_NOT_REQUIRED;
}

/***************************************************************************************/

