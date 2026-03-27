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
#elif CONFIG_IDF_TARGET_ESP32
#define RX1_PIN GPIO_NUM_16
#define TX1_PIN GPIO_NUM_17
#else
#define RX1_PIN UART_PIN_NO_CHANGE
#define TX1_PIN UART_PIN_NO_CHANGE
#endif

static uart_port_t uartnum = UART_NUM_0;
static QueueHandle_t uart_queue = NULL;
static TaskHandle_t g_uartEventTaskHandle = NULL;

// Forward declaration – MUST match drv_uart.h exactly
void UART_AppendByteToReceiveRingBuffer(int rc);

static void uart_event_task(void* pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*)malloc(1024);

    for (;;) {
        if (xQueueReceive(uart_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                while (event.size > 0) {
                    int len = uart_read_bytes(uartnum, dtmp,
                                              (event.size > 1024) ? 1024 : event.size, 0);
                    if (len <= 0) break;
                    for (int i = 0; i < len; i++) {
                        UART_AppendByteToReceiveRingBuffer(dtmp[i]);
                    }
                    event.size -= len;
                }
                break;

            case UART_BUFFER_FULL:
            case UART_FIFO_OVF:
                addLogAdv(LOG_WARN, LOG_FEATURE_CMD,
                          "UART %s - flushing input",
                          (event.type == UART_BUFFER_FULL) ? "BUFFER_FULL" : "FIFO_OVF");
                uart_flush_input(uartnum);
                xQueueReset(uart_queue);
                break;

            case UART_BREAK:
                addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
                          "UART BREAK (normal Dreo short idle gap)");
                break;

            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                addLogAdv(LOG_WARN, LOG_FEATURE_CMD, "UART error %d", event.type);
                uart_flush_input(uartnum);
                break;

            default:
                addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "UART event %d", event.type);
                break;
            }
        }
    }

    free(dtmp);
    vTaskDelete(NULL);
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

static void HAL_UART_StopReaderTask(void)
{
    if (g_uartEventTaskHandle != NULL) {
        vTaskDelete(g_uartEventTaskHandle);
        g_uartEventTaskHandle = NULL;
    }
    if (uart_queue != NULL) {
        vQueueDelete(uart_queue);
        uart_queue = NULL;
    }
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
    uart_port_t target_uartnum;

    if (CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART)) {
#ifdef CONFIG_IDF_TARGET_ESP32
        target_uartnum = UART_NUM_2;
#else
        target_uartnum = UART_NUM_1;
#endif
    } else {
        target_uartnum = UART_NUM_0;
    }

    HAL_UART_StopReaderTask();
    uartnum = target_uartnum;

    if (uart_is_driver_installed(uartnum)) {
        uart_driver_delete(uartnum);
    }

    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = (parity > 0) ? (parity + 1) : parity,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = hwflowc ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
#if PLATFORM_ESPIDF
        .source_clk = UART_SCLK_DEFAULT,
#endif
    };

    ESP_ERROR_CHECK(uart_driver_install(uartnum, 8192, 0, 30, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(uartnum, &uart_config));

    // Safe value that works on all ESP32 variants (max is usually 126)
    ESP_ERROR_CHECK(uart_set_rx_timeout(uartnum, 30));

    int tx_pin = (uartnum == UART_NUM_0) ? UART_PIN_NO_CHANGE : TX1_PIN;
    int rx_pin = (uartnum == UART_NUM_0) ? UART_PIN_NO_CHANGE : RX1_PIN;
    if (txOverride != -1) tx_pin = txOverride;
    if (rxOverride != -1) rx_pin = rxOverride;

    ESP_ERROR_CHECK(uart_set_pin(uartnum, tx_pin, rx_pin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 18, &g_uartEventTaskHandle);

    addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
              "HAL_UART_Init: UART%d @ %d baud (event queue + rx_timeout=30)", uartnum, baud);

    return 1;
}

#endif   // PLATFORM_ESPIDF || PLATFORM_ESP8266