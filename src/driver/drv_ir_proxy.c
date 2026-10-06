#include "../obk_config.h"

#if ENABLE_DRIVER_IR_PROXY

#include <limits.h>
#include "drv_local.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../cmnds/cmd_public.h"
#include "../hal/hal_hwtimer.h"
#include "../hal/hal_pins.h"
#include "../mqtt/new_mqtt.h"
#include "../httpserver/hass.h"

#if PLATFORM_BEKEN
#include "include.h"
#include "arm_arch.h"
#include <gpio_pub.h>
#include "../../beken378/driver/gpio/gpio.h"
#elif PLATFORM_BL602
#include "bl602_glb.h"
#endif

#if PLATFORM_ESPIDF
#include "freertos/task.h"
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define noInterrupts() taskENTER_CRITICAL(&mux)
#define interrupts() taskEXIT_CRITICAL(&mux)
#elif LINUX || WINDOWS
#define noInterrupts() 
#define interrupts() 
#elif PLATFORM_TXW81X || PLATFORM_RDA5981
#define noInterrupts()
#define interrupts()
#else
#include <task.h>
#define noInterrupts() taskENTER_CRITICAL()
#define interrupts() taskEXIT_CRITICAL()
#endif

#define IR_TIMING_CAP 4096U
#define IR_MAX_FRAME_TIMINGS (IR_TIMING_CAP - 1U)
#define IR_FRAME_QUEUE_CAP 8U
#define IR_SEND_TIMINGS_CAP 700U
#define IR_IDLE_LEVEL 1U
#define IR_FRAME_GAP_US 8000.0f
#define IR_MODULATION_HZ 38000U

typedef int16_t ir_tick_t;

typedef struct
{
	uint16_t start;
	uint16_t count;
} ir_frame_desc_t;

static int8_t recvpin = -1;
static int8_t sendpin = -1;
static int8_t ir_chan = -1;

static volatile float tick_us;
static volatile uint32_t frame_gap_ticks;
static volatile uint16_t ir_timing_rd;
static volatile uint16_t ir_timing_wr;

static volatile ir_frame_desc_t ir_frames[IR_FRAME_QUEUE_CAP];
static volatile uint8_t ir_frame_rd;
static volatile uint8_t ir_frame_wr;

static volatile bool ir_capturing;
static volatile bool ir_discarding;
static volatile bool ir_capture_overflow;

static volatile uint16_t ir_frame_start;
static volatile uint16_t ir_frame_count;
static volatile uint32_t ir_ticks;
static volatile uint8_t ir_last_level;

static volatile bool ir_sending;
static volatile uint16_t ir_send_index;
static volatile uint16_t ir_send_count;
static volatile uint32_t ir_send_ticks;
static volatile uint16_t ir_send_repeats;

static ir_tick_t* ir_timing = NULL;
static int32_t* ir_send_buffer = NULL;
static char* mqtt_publish_buffer = NULL;

static inline uint16_t IR_TimingNext(uint16_t index)
{
	return index + 1U == IR_TIMING_CAP ? 0U : index + 1U;
}

static inline uint8_t IR_FrameNext(uint8_t index)
{
	return index + 1U == IR_FRAME_QUEUE_CAP ? 0U : index + 1U;
}

static inline bool IR_FrameSlotAvailable(void)
{
	return IR_FrameNext(ir_frame_wr) != ir_frame_rd;
}

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

static inline bool IR_TimingPush(ir_tick_t value)
{
	uint16_t next = IR_TimingNext(ir_timing_wr);

	if(next == ir_timing_rd)
	{
		return false;
	}

	ir_timing[ir_timing_wr] = value;
	ir_timing_wr = next;

	return true;
}

static void IR_Discard(void)
{
	ir_timing_wr = ir_frame_start;
	ir_frame_count = 0;
	ir_capture_overflow = false;
	ir_capturing = false;
	ir_discarding = false;
	ir_ticks = 0;
}

static inline void IR_StartCapture(uint8_t level)
{
	if(!IR_FrameSlotAvailable())
	{
		ir_discarding = true;
		ir_ticks = 0;
		return;
	}

	ir_frame_start = ir_timing_wr;
	ir_frame_count = 0;
	ir_capture_overflow = false;
	ir_discarding = false;
	ir_capturing = true;
	ir_last_level = level;
	ir_ticks = 1U;
}

static inline void IR_StoreDuration(void)
{
	ir_tick_t duration;

	if(ir_ticks > INT16_MAX)
	{
		ir_capture_overflow = true;
		return;
	}

	duration = (ir_tick_t)ir_ticks;

	if(ir_last_level == IR_IDLE_LEVEL)
		duration = (ir_tick_t)-duration;

	if(!IR_TimingPush(duration))
	{
		ir_capture_overflow = true;
		return;
	}

	if(ir_frame_count == IR_MAX_FRAME_TIMINGS)
	{
		ir_capture_overflow = true;
		return;
	}

	ir_frame_count++;
}

static inline void IR_Commit(void)
{
	uint8_t next = IR_FrameNext(ir_frame_wr);

	if(ir_capture_overflow || ir_frame_count == 0U)
	{
		IR_Discard();
		return;
	}

	if(next == ir_frame_rd)
	{
		IR_Discard();
		return;
	}

	ir_frames[ir_frame_wr].start = ir_frame_start;
	ir_frames[ir_frame_wr].count = ir_frame_count;
	ir_frame_wr = next;

	ir_frame_count = 0;
	ir_capture_overflow = false;
	ir_capturing = false;
	ir_discarding = false;
	ir_ticks = 0;
}

static inline void IR_RecvISR(void)
{
	uint8_t level = digitalReadFast(recvpin);

	if(!ir_capturing)
	{
		if(ir_discarding)
		{
			if(level == IR_IDLE_LEVEL)
			{
				if(ir_ticks < UINT32_MAX)
					ir_ticks++;

				if(ir_ticks >= frame_gap_ticks)
				{
					ir_discarding = false;
					ir_ticks = 0;
				}
			}
			else
			{
				ir_ticks = 0;
			}

			return;
		}

		if(level != IR_IDLE_LEVEL)
			IR_StartCapture(level);

		return;
	}

	if(level == ir_last_level)
	{
		if(ir_ticks < UINT32_MAX)
			ir_ticks++;

		if(level == IR_IDLE_LEVEL && ir_ticks >= frame_gap_ticks)
		{
			IR_Commit();
			return;
		}

		return;
	}

	if(!ir_capture_overflow)
		IR_StoreDuration();

	ir_last_level = level;
	ir_ticks = 1U;
}

static void IR_SendLoadCurrent(void)
{
	int32_t duration = ir_send_buffer[ir_send_index];

	uint32_t us = duration < 0 ? (uint32_t)(-duration) : (uint32_t)duration;

	ir_send_ticks = (uint32_t)(us / tick_us + 0.5f);

	if(ir_send_ticks == 0)
		ir_send_ticks = 1;

	if(duration > 0)
	{
		HAL_PIN_PWM_Update(sendpin, 50);
	}
	else
	{
		HAL_PIN_PWM_Update(sendpin, 0);
	}
}

static inline void IR_SendISR(void)
{
	if(!ir_sending)
		return;

	if(ir_send_ticks > 0)
		ir_send_ticks--;

	if(ir_send_ticks != 0)
		return;

	ir_send_index++;

	if(ir_send_index >= ir_send_count)
	{
		HAL_PIN_PWM_Update(sendpin, 0);

		if(ir_send_repeats > 0)
		{
			ir_send_repeats--;
			ir_send_index = 0;
			IR_SendLoadCurrent();
			return;
		}

		ir_sending = false;
		ir_send_index = 0;
		ir_send_count = 0;
		ir_send_ticks = 0;

		return;
	}

	IR_SendLoadCurrent();
}

static void DRV_IR_Proxy_ISR(void* arg)
{
	if(ir_sending)
	{
		IR_SendISR();
		return;
	}
	if(recvpin >= 0)
		IR_RecvISR();
}

static bool IR_Proxy_Send(uint16_t count, uint16_t repeat_count)
{
	if(count == 0 || count > IR_SEND_TIMINGS_CAP)
		return false;

	if(ir_sending)
	{
		return false;
	}

	noInterrupts();

	ir_send_count = count;
	ir_send_repeats = repeat_count;
	ir_send_index = 0;
	ir_sending = true;

	IR_SendLoadCurrent();

	interrupts();

	return true;
}

static inline int IR_TicksToUs(ir_tick_t ticks)
{
	float value = (float)ticks * tick_us;

	if(value >= 0.0f)
		return (int)(value + 0.5f);

	return (int)(value - 0.5f);
}

static inline bool IR_GetNextFrame(ir_frame_desc_t* frame)
{
	bool available;

	noInterrupts();

	available = ir_frame_rd != ir_frame_wr;

	if(available)
	{
		*frame = ir_frames[ir_frame_rd];
		ir_frame_rd = IR_FrameNext(ir_frame_rd);
	}

	interrupts();

	return available;
}

int IRSendMQTTMessage(obk_mqtt_request_t* request)
{
	while(ir_sending) rtos_delay_milliseconds(5);

	//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "IRSendMQTTMessage %s", request->received);
	cJSON* root = cJSON_Parse((const char*)request->received);
	if(root == NULL)
	{
		return 0;
	}
	cJSON* timings = cJSON_GetObjectItem(root, "timings");
	if(!cJSON_IsArray(timings))
	{
		cJSON_Delete(root);
		return 0;
	}
	int count = cJSON_GetArraySize(timings);
	if(count > IR_SEND_TIMINGS_CAP)
	{
		ADDLOG_ERROR(LOG_FEATURE_MQTT, "IR message is too long! %i entries, max %i", count, IR_SEND_TIMINGS_CAP);
		cJSON_Delete(root);
		return 1;
	}

	uint16_t repeat_count = 0;
	cJSON* repeat = cJSON_GetObjectItem(root, "repeat_count");
	if(repeat)
		repeat_count = (uint16_t)repeat->valueint;

	for(uint16_t i = 0; i < count; i++)
		ir_send_buffer[i] = cJSON_GetArrayItem(timings, i)->valueint;

	cJSON_Delete(root);

	IR_Proxy_Send(count, repeat_count);
	return 1;
}

void IR_Proxy_Init(void)
{
	recvpin = PIN_FindPinIndexForRole(IOR_IRRecv, recvpin);
	if(recvpin == -1)
	{
		recvpin = PIN_FindPinIndexForRole(IOR_IRRecv_nPup, recvpin);
		if(recvpin >= 0) HAL_PIN_Setup_Input(recvpin);
	}
	else HAL_PIN_Setup_Input_Pullup(recvpin);
	sendpin = PIN_FindPinIndexForRole(IOR_IRSend, sendpin);

	if(recvpin == -1 && sendpin == -1) return;

	float real_period;
	ir_chan = HAL_RequestHWTimer(30, &real_period, DRV_IR_Proxy_ISR, NULL);

	tick_us = real_period;
	frame_gap_ticks = (uint32_t)(IR_FRAME_GAP_US / real_period + 0.5f);
	ir_sending = false;

	if(recvpin >= 0)
	{
		ir_timing_rd = 0;
		ir_timing_wr = 0;
		ir_frame_rd = 0;
		ir_frame_wr = 0;

		ir_capturing = false;
		ir_discarding = false;
		ir_capture_overflow = false;

		ir_frame_start = 0;
		ir_frame_count = 0;
		ir_ticks = 0;
		ir_last_level = digitalReadFast(recvpin);

		ir_timing = os_malloc(sizeof(ir_tick_t) * IR_TIMING_CAP);
		mqtt_publish_buffer = os_malloc(IR_TIMING_CAP);

		if(!ir_timing || !mqtt_publish_buffer)
		{
			IR_Proxy_Deinit();
			return;
		}
	}
	if(sendpin >= 0)
	{
		ir_send_buffer = os_malloc(sizeof(int32_t) * IR_SEND_TIMINGS_CAP);

		ir_send_index = 0;
		ir_send_count = 0;
		ir_send_ticks = 0;
		if(!ir_send_buffer)
		{
			sendpin = -1;
			IR_Proxy_Deinit();
			return;
		}
		char cbtopicbase[CGF_MQTT_CLIENT_ID_SIZE + 16];
		char cbtopicsub[CGF_MQTT_CLIENT_ID_SIZE + 16];
		const char* clientId = CFG_GetMQTTClientId();
		snprintf(cbtopicbase, sizeof(cbtopicbase), "%s/infrared/send", clientId);
		snprintf(cbtopicsub, sizeof(cbtopicsub), "%s/infrared/send", clientId);
		MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 0, IRSendMQTTMessage); // FIXME: ID 0 because otherwise mqtt message will be caught by channelGet handler
		HAL_PIN_PWM_Start(sendpin, IR_MODULATION_HZ);
	}

	HAL_HWTimerStart(ir_chan);
}

void IR_Proxy_Deinit(void)
{
	HAL_HWTimerStop(ir_chan);
	HAL_HWTimerDeinit(ir_chan);
	ir_chan = -1;
	if(ir_timing) os_free(ir_timing);
	ir_timing = NULL;
	if(mqtt_publish_buffer) os_free(mqtt_publish_buffer);
	mqtt_publish_buffer = NULL;
	if(sendpin >= 0)
	{
		MQTT_RemoveCallback(0);
		os_free(ir_send_buffer);
		HAL_PIN_PWM_Update(sendpin, 0);
		HAL_PIN_PWM_Stop(sendpin);
	}
}

void IR_Proxy_RunFrame(void)
{
	if(recvpin >= 0)
	{
		bool busy;
		noInterrupts();
		busy = ir_capturing || ir_discarding;
		interrupts();

		if(busy)
			return;

		for(;;)
		{
			ir_frame_desc_t frame;

			if(!IR_GetNextFrame(&frame))
				break;

			uint16_t index = frame.start;

#if ENABLE_MQTT
			if(MQTT_IsReady())
			{
				uint16_t pos = snprintf(mqtt_publish_buffer, IR_TIMING_CAP, "{\"timings\":[");
				for(uint16_t i = 0; i < frame.count; i++)
				{
					int written = snprintf(mqtt_publish_buffer + pos, IR_TIMING_CAP - pos,
						"%s%i", i ? "," : "", IR_TicksToUs(ir_timing[index]));
					if(written < 0)
						break;

					if(written >= IR_TIMING_CAP - pos)
					{
						pos = IR_TIMING_CAP;
						break;
					}

					pos += (uint16_t)written;
					index = IR_TimingNext(index);
				}
				if(pos < IR_TIMING_CAP - 21)
				{
					snprintf(mqtt_publish_buffer + pos, IR_TIMING_CAP - pos, "],\"modulation\":%u}", IR_MODULATION_HZ);
					MQTT_PublishMain_StringString("infrared/recv", mqtt_publish_buffer, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);
				}
			}
#endif

			uint16_t rd = frame.start + frame.count;

			while(rd >= IR_TIMING_CAP)
				rd -= IR_TIMING_CAP;

			noInterrupts();
			ir_timing_rd = rd;
			interrupts();
		}
	}
}

void IR_Proxy_HA_Discovery(const char* topic)
{
#if ENABLE_HA_DISCOVERY
	if(recvpin >= 0)
	{
		HassDeviceInfo* dev_info = hass_init_device_info(HASS_IRRECV, 0, NULL, NULL, 0, NULL);
		if(dev_info == 0)
			return;
		cJSON_AddStringToObject(dev_info->root, "platform", "infrared");
		cJSON_AddStringToObject(dev_info->root, "schema", "receiver");
		cJSON_AddStringToObject(dev_info->root, "state_topic", "~/infrared/recv");
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
	}
	if(sendpin >= 0)
	{
		HassDeviceInfo* dev_info = hass_init_device_info(HASS_IRSEND, 0, NULL, NULL, 0, NULL);
		if(dev_info == 0)
			return;
		cJSON_AddStringToObject(dev_info->root, "platform", "infrared");
		cJSON_AddStringToObject(dev_info->root, "schema", "emitter");
		cJSON_AddStringToObject(dev_info->root, "command_topic", "~/infrared/send");
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
	}
#endif
}

#endif
