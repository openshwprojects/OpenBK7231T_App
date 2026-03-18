#include "../obk_config.h"

// Tiny IR NEC receiver. Should be enough for something like led strips
// BK7238
// arm-none-eabi-size drv_tinyir_nec.app.o
// text    data     bss     dec     hex filename
//  904       7      23     934     3a6 drv_tinyir_nec.app.o
#if ENABLE_DRIVER_TINYIR_NEC

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../cmnds/cmd_public.h"
#include "../hal/hal_hwtimer.h"
#include "../hal/hal_pins.h"
#include "../mqtt/new_mqtt.h"

#if PLATFORM_BEKEN
#include "include.h"
#include "arm_arch.h"
#include <gpio_pub.h>
#include "../../beken378/driver/gpio/gpio.h"
#elif PLATFORM_BL602
#include "bl602_glb.h"
#endif

static float ir_periodus = 50;
static uint32_t timer_cnt = 0;

#define TICKS(us) ((uint32_t)((us) / ir_periodus))

#define NEC_LEADER_MARK  TICKS(9000)
#define NEC_LEADER_SPACE TICKS(4500)
#define NEC_REPEAT_SPACE TICKS(2250)
#define NEC_BIT_MARK     TICKS(560)
#define NEC_BIT0_SPACE   TICKS(560)
#define NEC_BIT1_SPACE   TICKS(1690)

static uint32_t duration_ticks = 0;
static uint8_t bitCount = 0;
static union
{
	uint32_t all;
	uint8_t  byte[4];
} irSignal = { .all = 0 };

static uint32_t repeatCount = 0;
static bool possiblyHeld = false;

static uint32_t recvState = 0;
static bool irDataAvailable = false;
static struct
{
	uint16_t addr;
	uint8_t  cmd;
	bool     keyHeld;
} _irData;

static inline bool within(uint32_t measured, uint32_t target)
{
	uint32_t lower = (target * 70U) / 100U;
	uint32_t upper = (target * 130U) / 100U;
	return measured >= lower && measured <= upper;
}

static int8_t recvpin = -1;
static int8_t ir_chan = -1;

static inline unsigned char digitalReadFast(unsigned char P)
{
#if PLATFORM_BEKEN
	return bk_gpio_input((GPIO_INDEX)P);
#elif PLATFORM_BL602
	return GLB_GPIO_Read((GLB_GPIO_Type)P);
#else
	return HAL_PIN_ReadDigitalInput(P);
#endif
}

static bool last_level = true;

static void DRV_IR_ISR(void* arg)
{
	bool cur_level = (bool)digitalReadFast(recvpin);
	if(cur_level == last_level)
	{
		timer_cnt++;
		return;
	}

	duration_ticks = timer_cnt;
	timer_cnt = 0;

	if(last_level == false)
	{
		if(within(duration_ticks, NEC_LEADER_MARK))
		{
			// intentionally disable repeats
			possiblyHeld = false;
			recvState = 1;
		}
		else if(recvState != 2)
		{
			recvState = 0;
		}
	}
	else
	{
		switch(recvState)
		{
			case 1:
				if(within(duration_ticks, NEC_LEADER_SPACE))
				{
					bitCount = 0;
					irSignal.all = 0;
					repeatCount = 0;
					recvState = 2;
				}
				else if(within(duration_ticks, NEC_REPEAT_SPACE))
				{
					if(possiblyHeld)
					{
						repeatCount++;
						_irData.keyHeld = true;
						irDataAvailable = true;
					}
					recvState = 0;
				}
				else
				{
					recvState = 0;
				}
				break;

			case 2:
			{
				if(within(duration_ticks, NEC_BIT0_SPACE))
				{
					irSignal.all >>= 1;
					bitCount++;
				}
				else if(within(duration_ticks, NEC_BIT1_SPACE))
				{
					irSignal.all = (irSignal.all >> 1) | 0x80000000;
					bitCount++;
				}
				else
				{
					recvState = 0;
					break;
				}

				if(bitCount == 32)
				{
					if(irSignal.byte[2] == (uint8_t)~irSignal.byte[3])
					{
						_irData.addr = irSignal.byte[0] | (irSignal.byte[1] << 8);
						_irData.cmd = irSignal.byte[2];
						_irData.keyHeld = false;
						irDataAvailable = true;
						possiblyHeld = true;
					}
					recvState = 0;
				}
			}
			break;

			default:
				recvState = 0;
		}
	}

	last_level = cur_level;
}

void TinyIR_NEC_Init()
{
	recvpin = PIN_FindPinIndexForRole(IOR_IRRecv, recvpin);
	if(recvpin == -1)
	{
		recvpin = PIN_FindPinIndexForRole(IOR_IRRecv_nPup, recvpin);
		if(recvpin >= 0) HAL_PIN_Setup_Input_Pullup(recvpin);
	}
	else HAL_PIN_Setup_Input(recvpin);

	if(recvpin < 0) return;

	last_level = digitalReadFast(recvpin);
	ir_chan = HAL_RequestHWTimer(ir_periodus, &ir_periodus, DRV_IR_ISR, NULL);
	HAL_HWTimerStart(ir_chan);
	ADDLOG_INFO(LOG_FEATURE_DRV, "NEC %u %.2f", ir_chan, ir_periodus);
}

void TinyIR_NEC_Deinit()
{
	HAL_HWTimerStop(ir_chan);
	HAL_HWTimerDeinit(ir_chan);
}

void TinyIR_NEC_RunFrame()
{
	if(irDataAvailable)
	{
		ADDLOG_INFO(LOG_FEATURE_IR, "IR NEC 0x%X 0x%X %d", (unsigned int)_irData.addr, (unsigned int)_irData.cmd, _irData.keyHeld);

#if ENABLE_MQTT
		if(CFG_HasFlag(OBK_FLAG_IR_PUBLISH_RECEIVED_IN_JSON))
		{
			char out[128];
			// {"IrReceived":{"Protocol":"NEC","Bits":32,"Data":"0xB14E7F80","Repeat":0}}
			snprintf(out, sizeof(out), "{\"IrReceived\":{\"Protocol\":\"NEC\",\"Bits\":32,\"Data\":\"0x%08X\",\"Repeat\":%d}}",
				(unsigned int)irSignal.all, _irData.keyHeld);
			MQTT_PublishMain_StringString("RESULT", out, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
			//ADDLOG_INFO(LOG_FEATURE_IR, out);
		}
#endif
		EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, _irData.addr, _irData.cmd);

		irDataAvailable = false;
	}
}

#endif
