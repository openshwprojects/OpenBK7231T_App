#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../cmnds/cmd_local.h"
#include "../logging/logging.h"

#if PLATFORM_BK7231T | PLATFORM_BK7231N
#include "../../beken378/func/user_driver/BkDriverUart.h"
#endif

#if PLATFORM_BL602
#include <vfs.h>
#include <bl_uart.h>
#include <bl_irq.h>
#include <event_device.h>
#include <cli.h>
#include <aos/kernel.h>
#include <aos/yloop.h>

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include <netutils/netutils.h>

#include <bl602_glb.h>
#include <bl602_hbn.h>

#include <bl_uart.h>
#include <bl_chip.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_sec.h>
#include <bl_cks.h>
#include <bl_irq.h>
#include <bl_dma.h>
#include <bl_timer.h>
#include <bl_gpio_cli.h>
#include <bl_wdt_cli.h>
#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_gpio.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <looprt.h>
#include <loopset.h>
#include <sntp.h>
#include <bl_sys_time.h>
#include <bl_sys.h>
#include <bl_sys_ota.h>
#include <bl_romfs.h>
#include <fdt.h>
#include <device/vfs_uart.h>
#include <utils_log.h>
#include <bl602_uart.h>

#include <easyflash.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>
#include <utils_log.h>
#include <libfdt.h>
#include <blog.h>
// backlog logtype none; startDriver BL0942
#endif
#if PLATFORM_BK7231T | PLATFORM_BK7231N
// from uart_bk.c
extern void bk_send_byte(UINT8 uport, UINT8 data);
int g_chosenUART = BK_UART_1;
#elif WINDOWS

#elif PLATFORM_BL602
//int g_fd;
uint8_t g_id = 1;
int fd_console = -1;
#elif PLATFORM_ESPIDF
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
#else
#endif

static byte *g_recvBuf = 0;
static int g_recvBufSize = 0;
static int g_recvBufIn = 0;
static int g_recvBufOut = 0;
// used to detect uart reinit
int g_uart_init_counter = 0;
// used to detect uart manual mode
int g_uart_manualInitCounter = -1;

void UART_InitReceiveRingBuffer(int size){
	if(g_recvBuf!=0)
        free(g_recvBuf);
	g_recvBuf = (byte*)malloc(size);
	memset(g_recvBuf,0,size);
    g_recvBufSize = size;
    g_recvBufIn = 0;
    g_recvBufOut = 0;
}

int UART_GetDataSize() {
    return (g_recvBufIn >= g_recvBufOut
                ? g_recvBufIn - g_recvBufOut
                : g_recvBufIn + (g_recvBufSize - g_recvBufOut) + 1);
}

byte UART_GetByte(int idx) {
    return g_recvBuf[(g_recvBufOut + idx) % g_recvBufSize];
}

void UART_ConsumeBytes(int idx) {
    g_recvBufOut += idx;
	g_recvBufOut %= g_recvBufSize;
}

void UART_AppendByteToReceiveRingBuffer(int rc) {
    if (UART_GetDataSize() < (g_recvBufSize - 1)) {
        g_recvBuf[g_recvBufIn++] = rc;
        g_recvBufIn %= g_recvBufSize;
    }
}

#if PLATFORM_BK7231T | PLATFORM_BK7231N
void test_ty_read_uart_data_to_buffer(int port, void* param)
{
    int rc = 0;

    while((rc = uart_read_byte(port)) != -1)
		UART_AppendByteToReceiveRingBuffer(rc);
}
#endif

#ifdef PLATFORM_BL602
//void UART_RunQuickTick() {
//}
//void MY_UART1_IRQHandler(void)
//{
//	int length;
//	byte buffer[16];
//	//length = aos_read(g_fd, buffer, 1);
//	//if (length > 0) {
//	//	UART_AppendByteToReceiveRingBuffer(buffer[0]);
//	//}
//	int res = bl_uart_data_recv(g_id);
//	if (res >= 0) {
//		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "UART received: %i\n", res);
//		UART_AppendByteToReceiveRingBuffer(res);
//	}
//}

static void console_cb_read(int fd, void *param)
{
	char buffer[64];  /* adapt to usb cdc since usb fifo is 64 bytes */
    int ret;
    int i;

    ret = aos_read(fd, buffer, sizeof(buffer));
    if (ret > 0) {
        if (ret < sizeof(buffer)) {
            fd_console = fd;
            buffer[ret] = 0;
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "BL602 received: %s\n", buffer);
            for (i = 0; i < ret; i++) {
				UART_AppendByteToReceiveRingBuffer(buffer[i]);
            }
		}
		else {
            printf("-------------BUG from aos_read for ret\r\n");
        }
    }
}
#endif

#ifdef PLATFORM_ESPIDF

static void uart_event_task(void* pvParameters)
{
    uart_event_t event;
    for(;;)
    {
        if(xQueueReceive(uart_queue, (void*)&event, (TickType_t)portMAX_DELAY))
        {
            bzero(data, 256);
            switch(event.type)
            {
            case UART_DATA:
                uart_read_bytes(uartnum, data, event.size, portMAX_DELAY);
                for(int i = 0; i < event.size; i++)
                {
                    UART_AppendByteToReceiveRingBuffer(data[i]);
                    vTaskDelay(3);
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

#endif

void UART_SendByte(byte b) {
#if PLATFORM_BK7231T | PLATFORM_BK7231N
    // BK_UART_1 is defined to 0
    bk_send_byte(g_chosenUART, b);
#elif WINDOWS
    void SIM_AppendUARTByte(byte b);
    // STUB - for testing
    SIM_AppendUARTByte(b);
#if 1
    printf("%02X", b);
#endif
    //addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"%02X", b);
#elif PLATFORM_BL602
    aos_write(fd_console, &b, 1);
	//bl_uart_data_send(g_id, b);
#elif PLATFORM_ESPIDF
    uart_write_bytes(uartnum, &b, 1);
#endif
}
commandResult_t CMD_UART_Send_Hex(const void *context, const char *cmd, const char *args, int cmdFlags) {
    if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_Send_Hex: requires 1 argument (hex string, like FFAABB00CCDD\n");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    while (*args) {
		byte b = CMD_ParseOrExpandHexByte(&args);
        UART_SendByte(b);
    }
    return CMD_RES_OK;
}

// This is a tool for debugging.
// It simulates OpenBeken receiving packet from UART.
// For example, you can do:
/*
1. You can simulate TuyaMCU battery powered device packet:

uartFakeHex 55 AA 00 05 00 05 01 04 00 01 01 10 55
55 AA	00	05		00 05	0104000101	10
HEADER	VER=00	Unk		LEN	dpId=1 Enum V=1	CHK

backlog startDriver TuyaMCU; uartFakeHex 55 AA 00 05 00 05 01 04 00 01 01 10 55
*/

commandResult_t CMD_UART_FakeHex(const void *context, const char *cmd, const char *args, int cmdFlags) {
	//const char *args = CMD_GetArg(1);
	//byte rawData[128];
	//int curCnt;

	//curCnt = 0;
    if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_FakeHex: requires 1 argument (hex string, like FFAABB00CCDD\n");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    while (*args) {
        byte b;
        if (*args == ' ') {
            args++;
            continue;
        }
        b = hexbyte(args);

		//rawData[curCnt] = b;
		//curCnt++;
		//if(curCnt>=sizeof(rawData)) {
		//  addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"CMD_UART_FakeHex: sorry, given string is too long\n");
		//  return -1;
		//}

        UART_AppendByteToReceiveRingBuffer(b);

        args += 2;
    }
    return 1;
}

// uartSendASCII test123
commandResult_t CMD_UART_Send_ASCII(const void *context, const char *cmd, const char *args, int cmdFlags) {
	//const char *args = CMD_GetArg(1);
    if (!(*args)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU, "CMD_UART_Send_ASCII: requires 1 argument (hex string, like hellp world\n");
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    while (*args) {

        UART_SendByte(*args);

        args++;
    }
    return CMD_RES_OK;
}

void UART_ResetForSimulator() {
	g_uart_init_counter = 0;
}

int UART_InitUART(int baud, int parity)
{
    g_uart_init_counter++;
#if PLATFORM_BK7231T | PLATFORM_BK7231N
    bk_uart_config_t config;

    config.baud_rate = baud;
    config.data_width = 0x03;
    config.parity = parity;    //0:no parity,1:odd,2:even
    config.stop_bits = 0;   //0:1bit,1:2bit
    config.flow_control = 0;   //FLOW_CTRL_DISABLED
    config.flags = 0;


    // BK_UART_1 is defined to 0
    if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
    {
        g_chosenUART = BK_UART_2;
    }
    else
    {
        g_chosenUART = BK_UART_1;
    }
    bk_uart_initialize(g_chosenUART, &config, NULL);
    bk_uart_set_rx_callback(g_chosenUART, test_ty_read_uart_data_to_buffer, NULL);
#elif PLATFORM_BL602
    if(fd_console < 0)
    {
        //uint8_t tx_pin = 16;
        //uint8_t rx_pin = 7;
        //bl_uart_init(g_id, tx_pin, rx_pin, 0, 0, baud);
        //g_fd = aos_open(name, 0);
        //bl_uart_int_rx_enable(1);
        //bl_irq_register(UART1_IRQn, MY_UART1_IRQHandler);
        //bl_irq_enable(UART1_IRQn);
        //vfs_uart_init_simple_mode(0, 7, 16, baud, "/dev/ttyS0");

        if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
        {
            fd_console = aos_open("/dev/ttyS1", 0);
        }
        else
        {
            fd_console = aos_open("/dev/ttyS0", 0);
        }
        if(fd_console >= 0)
        {
            aos_ioctl(fd_console, IOCTL_UART_IOC_BAUD_MODE, baud);
            addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Init CLI with event Driven\r\n");
            aos_cli_init(0);
            aos_poll_read_fd(fd_console, console_cb_read, (void*)0x12345678);
        }
        else
        {
            addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "failed CLI with event Driven\r\n");
        }
    }
#elif PLATFORM_ESPIDF
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
    uart_driver_install(uartnum, 256, 0, 20, &uart_queue, 0);
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
        data = (uint8_t*)malloc(256);
        xTaskCreate(uart_event_task, "uart_event_task", 1024, NULL, 16, NULL);
    }
#endif
    return g_uart_init_counter;
}

void UART_DebugTool_Run() {
    byte b;
    char tmp[128];
    char *p = tmp;
    int i;

    for (i = 0; i < sizeof(tmp) - 4; i++) {
		if (UART_GetDataSize()==0) {
            break;
        }
        b = UART_GetByte(0);
        if (i) {
            *p = ' ';
            p++;
        }
        sprintf(p, "%02X", b);
        p += 2;
        UART_ConsumeBytes(1);
    }
    *p = 0;
    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "UART received: %s\n", tmp);
}

void UART_RunEverySecond() {
    if (g_uart_manualInitCounter == g_uart_init_counter) {
        UART_DebugTool_Run();
    }
}

commandResult_t CMD_UART_Init(const void *context, const char *cmd, const char *args, int cmdFlags) {
    int baud;

    Tokenizer_TokenizeString(args, 0);
    // following check must be done after 'Tokenizer_TokenizeString',
    // so we know arguments count in Tokenizer. 'cmd' argument is
    // only for warning display
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    baud = Tokenizer_GetArgInteger(0);

    UART_InitReceiveRingBuffer(512);

    UART_InitUART(baud, 0);
    g_uart_manualInitCounter = g_uart_init_counter;

    return CMD_RES_OK;
}

void UART_AddCommands() {
	//cmddetail:{"name":"uartSendHex","args":"[HexString]",
	//cmddetail:"descr":"Sends raw data by UART, can be used to send TuyaMCU data, but you must write whole packet with checksum yourself",
	//cmddetail:"fn":"CMD_UART_Send_Hex","file":"driver/drv_tuyaMCU.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartSendHex", CMD_UART_Send_Hex, NULL);
	//cmddetail:{"name":"uartSendASCII","args":"[AsciiString]",
	//cmddetail:"descr":"Sends given string by UART.",
	//cmddetail:"fn":"CMD_UART_Send_ASCII","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartSendASCII", CMD_UART_Send_ASCII, NULL);
	//cmddetail:{"name":"uartFakeHex","args":"[HexString]",
	//cmddetail:"descr":"Spoofs a fake hex packet so it looks like TuyaMCU send that to us. Used for testing.",
	//cmddetail:"fn":"CMD_UART_FakeHex","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartFakeHex", CMD_UART_FakeHex, NULL);
	//cmddetail:{"name":"uartInit","args":"[BaudRate]",
	//cmddetail:"descr":"Manually starts UART1 port. Keep in mind that you don't need to do it for TuyaMCU and BL0942, those drivers do it automatically.",
	//cmddetail:"fn":"CMD_UART_Init","file":"driver/drv_uart.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("uartInit", CMD_UART_Init, NULL);
}
