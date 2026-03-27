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

// Global UART port used by the driver (can be UART_NUM_0 or UART_NUM_1/2)
static uart_port_t uartnum = UART_NUM_0;

// Event queue handle and task handle
static QueueHandle_t uart_queue = NULL;
static TaskHandle_t g_uartEventTaskHandle = NULL;

// Forward declaration – MUST match drv_uart.h exactly
void UART_AppendByteToReceiveRingBuffer(int rc);

static void uart_event_task(void* pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*)malloc(512);   // temporary buffer for one read burst

    for (;;) {
        if (xQueueReceive(uart_queue, (void*)&event, portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                // Read all bytes that are currently available
                while (event.size) {
                    int len = uart_read_bytes(uartnum, dtmp,
                                              (event.size > 512) ? 512 : event.size,
                                              0);
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
                          "UART %s - buffer full / FIFO overflow - flushing", 
                          (event.type == UART_BUFFER_FULL) ? "BUFFER_FULL" : "FIFO_OVF");
                uart_flush_input(uartnum);
                xQueueReset(uart_queue);
                break;

            case UART_BREAK:
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                addLogAdv(LOG_WARN, LOG_FEATURE_CMD, "UART error event %d", event.type);
                uart_flush_input(uartnum);
                break;

            default:
                addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU,
                          "UART event type: %d (ignored)", event.type);
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
        esp_log_level_set("*", ESP_LOG_INFO);
    } else {
        target_uartnum = UART_NUM_0;
        esp_log_level_set("*", ESP_LOG_NONE);
    }

    HAL_UART_StopReaderTask();

    uartnum = target_uartnum;

    // If driver is already installed, uninstall first
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

    // Install driver with a generous RX buffer (4096 bytes) and enable event queue
    ESP_ERROR_CHECK(uart_driver_install(uartnum,
                                        4096,          // RX buffer
                                        0,             // TX buffer (we don't use TX queue)
                                        20,            // event queue size
                                        &uart_queue,
                                        0));           // no flags

    ESP_ERROR_CHECK(uart_param_config(uartnum, &uart_config));

    // Pin mapping (respect overrides from higher-level drivers)
    int tx_pin = (uartnum == UART_NUM_0) ? UART_PIN_NO_CHANGE : TX1_PIN;
    int rx_pin = (uartnum == UART_NUM_0) ? UART_PIN_NO_CHANGE : RX1_PIN;

    if (txOverride != -1) tx_pin = txOverride;
    if (rxOverride != -1) rx_pin = rxOverride;

    ESP_ERROR_CHECK(uart_set_pin(uartnum, tx_pin, rx_pin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Start the event processing task (higher priority than normal user tasks)
    xTaskCreate(uart_event_task, "uart_event_task",
                2048, NULL, 16, &g_uartEventTaskHandle);

    addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
              "HAL_UART_Init: UART%d initialized @ %d baud (event queue mode)", uartnum, baud);

    return 1;
}

#endif   // PLATFORM_ESPIDF || PLATFORM_ESP8266