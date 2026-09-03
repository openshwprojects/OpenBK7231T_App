#if PLATFORM_ESPIDF || PLATFORM_ESP8266

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

#if 0//PLATFORM_ESPIDF

static QueueHandle_t uart_queue;
uint8_t* data = NULL;

static void uart_event_task(void* pvParameters)
{
	uart_event_t event;
	for (;;)
	{
		if (xQueueReceive(uart_queue, (void*)&event, (TickType_t)portMAX_DELAY))
		{
			bzero(data, 512);
			switch (event.type)
			{
			case UART_DATA:
				uart_read_bytes(uartnum, data, event.size, portMAX_DELAY);
				for (int i = 0; i < event.size; i++)
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
				addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "uart event type: %d", event.type);
				break;
			}
		}
	}
	free(data);
	data = NULL;
	vTaskDelete(NULL);
}

#else

//#include "esp8266/uart_register.h"
//#include "esp8266/uart_struct.h"
//static DRAM_ATTR uart_dev_t* const UART[UART_NUM_MAX] = { &uart0, &uart1 };
//
//static void IRAM_ATTR uart_intr_handle(void* arg)
//{	
//	uart_port_t uartn = *((uart_port_t*)arg);
//	uint16_t status = UART[uartn]->int_st.val;
//	uint16_t rx_fifo_len = UART[uartn]->status.rxfifo_cnt;
//	while(rx_fifo_len)
//	{
//		UART_AppendByteToReceiveRingBuffer(UART[uartn]->fifo.rw_byte);
//		rx_fifo_len--;
//	}
//	uart_clear_intr_status(uartnum, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
//}

#endif

static void uart_event_task(void* pvParameters)
{
	uint8_t* data = (uint8_t*)malloc(512);
	while (1)
	{
		int len = uart_read_bytes(uartnum, data, 512, 20 / portTICK_RATE_MS);
		if (len)
		{
			for (int i = 0; i < len; i++)
			{
				UART_AppendByteToReceiveRingBuffer(data[i]);
			}
		}
	}
}

void HAL_UART_SendByte(byte b)
{
	uart_write_bytes(uartnum, &b, 1);
}

void HAL_UART_Flush(void)
{
	uart_wait_tx_done(uartnum, pdMS_TO_TICKS(100));
}
void HAL_SetBaud(uint32_t baud)
{
	uart_set_baudrate(uartnum, baud);
}
int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if (CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		uartnum = UART_NUM_1;
		esp_log_level_set("*", ESP_LOG_INFO);
	}
	else
	{
		uartnum = UART_NUM_0;
		esp_log_level_set("*", ESP_LOG_NONE);
	}
	if (uart_is_driver_installed(uartnum))
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
		.flow_ctrl = hwflowc == false ? UART_HW_FLOWCTRL_DISABLE : UART_HW_FLOWCTRL_CTS_RTS,
#if PLATFORM_ESPIDF
		.source_clk = UART_SCLK_DEFAULT,
#endif
	};
	uart_param_config(uartnum, &uart_config);
	uart_driver_install(uartnum, 4096, 4096, 0, NULL, 0);
	uart_enable_rx_intr(uartnum);

#if PLATFORM_ESPIDF
	if (txOverride != -1) {
		uart_set_pin(uartnum,
			txOverride, 21,
			UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	}
#endif

#if 0//PLATFORM_ESPIDF
	if (uartnum == UART_NUM_0)
	{
		uart_set_pin(uartnum, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	}
	else
	{
		uart_set_pin(uartnum, TX1_PIN, RX1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	}
	if (data == NULL)
	{
		//data = (uint8_t*)malloc(512);
		xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 16, NULL);
	}
#else
	//uart_isr_register(uartnum, uart_intr_handle, &uartnum);
#endif
	xTaskCreate(uart_event_task, "uart_event_task", 1024, NULL, 16, NULL);
	return 1;
}

#endif