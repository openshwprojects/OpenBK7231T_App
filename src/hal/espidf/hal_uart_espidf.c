#ifdef PLATFORM_ESPIDF

#include "../hal_uart.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#ifdef CONFIG_IDF_TARGET_ESP32C6
#define RX1_PIN GPIO_NUM_7
#define TX1_PIN GPIO_NUM_5
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2
#define RX1_PIN GPIO_NUM_6
#define TX1_PIN GPIO_NUM_7
#else
#define RX1_PIN UART_PIN_NO_CHANGE
#define TX1_PIN UART_PIN_NO_CHANGE
#endif

uart_port_t uartnum = UART_NUM_0;
static QueueHandle_t uart_queue;
uint8_t* data = NULL;

static void uart_event_task(void* pvParameters)
{
	uart_event_t event;
	for(;;)
	{
		if(xQueueReceive(uart_queue, (void*)&event, (TickType_t)portMAX_DELAY))
		{
			bzero(data, 512);
			switch(event.type)
			{
				case UART_DATA:
					uart_read_bytes(uartnum, data, event.size, portMAX_DELAY);
					for(int i = 0; i < event.size; i++)
					{
						UART_AppendByteToReceiveRingBuffer(data[i]);
					}
					break;
				case UART_BUFFER_FULL:
				case UART_FIFO_OVF:
					addLogAdv(LOG_WARN, LOG_FEATURE_CMD, "%s", event.type == UART_BUFFER_FULL ? "UART_BUFFER_FULL" : "UART_FIFO_OVF");
					uart_flush_input(uartnum);
					xQueueReset(uart_queue);
					break;
				default:
					break;
			}
		}
	}
	free(data);
	data = NULL;
	vTaskDelete(NULL);
}

void HAL_UART_SendByte(byte b)
{
	uart_write_bytes(uartnum, &b, 1);
}

int HAL_UART_Init(int baud, int parity)
{
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		uartnum = UART_NUM_1;
		esp_log_level_set("*", ESP_LOG_INFO);
	}
	else
	{
		uartnum = UART_NUM_0;
		esp_log_level_set("*", ESP_LOG_NONE);
	}
	if(uart_is_driver_installed(uartnum))
	{
		uart_disable_rx_intr(uartnum);
		uart_driver_delete(uartnum);
	}
	uart_config_t uart_config =
	{
		.baud_rate = baud,
		.data_bits = UART_DATA_8_BITS,
		.parity = parity > 0 ? parity + 1 : parity,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	uart_driver_install(uartnum, 512, 0, 20, &uart_queue, 0);
	uart_param_config(uartnum, &uart_config);
	if(uartnum == UART_NUM_0)
	{
		uart_set_pin(uartnum, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	}
	else
	{
		uart_set_pin(uartnum, TX1_PIN, RX1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	}
	if(data == NULL)
	{
		data = (uint8_t*)malloc(512);
		xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 16, NULL);
		uart_enable_rx_intr(uartnum);
	}
	return 1;
}

#endif
