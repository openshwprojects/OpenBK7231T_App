#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../cmnds/cmd_public.h"
#include "../hal/hal_pins.h"
#include "drv_local.h"

#if ENABLE_DRIVER_SLINK

#if PLATFORM_BL602

    #include <FreeRTOS.h>
    #include <task.h>
    #include <bl_timer.h>

    #define SLINK_DELAY_US(us)     bl_timer_delay_us(us)
    #define SLINK_NOW_US()         bl_timer_now_us()
    #define SLINK_CRIT_ENTER()     taskENTER_CRITICAL()
    #define SLINK_CRIT_EXIT()      taskEXIT_CRITICAL()

#if PLATFORM_ESPIDF

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_timer.h"
    #include "esp_rom_sys.h"

    static portMUX_TYPE g_slink_mux = portMUX_INITIALIZER_UNLOCKED;

    #define SLINK_DELAY_US(us)     esp_rom_delay_us(us)
    #define SLINK_NOW_US()         ((uint32_t)esp_timer_get_time())
    #define SLINK_CRIT_ENTER()     portENTER_CRITICAL(&g_slink_mux)
    #define SLINK_CRIT_EXIT()      portEXIT_CRITICAL(&g_slink_mux)

#elif PLATFORM_RTL8720D || PLATFORM_RTL87X0C || PLATFORM_RTL8710B

    #include <FreeRTOS.h>
    #include <task.h>
    #include "timer_api.h"

    #define SLINK_DELAY_US(us)     DelayUs(us)
    #define SLINK_NOW_US()         (xTaskGetTickCount() * 1000UL) // 1ms tick is sufficient
    #define SLINK_CRIT_ENTER()     taskENTER_CRITICAL()
    #define SLINK_CRIT_EXIT()      taskEXIT_CRITICAL()

#elif PLATFORM_BK7231T || PLATFORM_BK7231N

    #include "arm_arch.h"
    #include "../hal/hal_generic.h"

    #define SLINK_DELAY_US(us)     usleep(us)
    #define SLINK_NOW_US()         (HAL_GetTicks() * 1000UL)
    #define SLINK_CRIT_ENTER()     GLOBAL_INT_DECLARATION(); GLOBAL_INT_DISABLE()
    #define SLINK_CRIT_EXIT()      GLOBAL_INT_RESTORE()

#else

    // Generic fallback for W800, LN882H, XR809, etc.
    #include "../hal/hal_generic.h"

    #define SLINK_DELAY_US(us)     usleep(us)
    #define SLINK_NOW_US()         (HAL_GetTicks() * 1000UL)
    #define SLINK_CRIT_ENTER()
    #define SLINK_CRIT_EXIT()

#endif

// Sony Control-A1 / S-Link Device & Commands
#define SLINK_DEVICE_AMP             0xC0 // 192
#define SLINK_DEVICE_MD              0xB0 // 176
#define SLINK_DEVICE_TUNER           0xC1 // 193
#define SLINK_DEVICE_AMP_NEW         0x70 // 112
#define SLINK_DEVICE_SURROUND        0xC3 // 194
#define SLINK_DEVICE_CDP_CX1L        0x90 // 200

#define SLINK_CMD_AMP_VOLUME_UP      0x14 // 20
#define SLINK_CMD_AMP_VOLUME_DOWN    0x15 // 21
#define SLINK_CMD_AMP_POWER_ON       0x2E // 46
#define SLINK_CMD_AMP_POWER_OFF      0x2F // 47
#define SLINK_CMD_AMP_MUTE_ON        0x06 // 6
#define SLINK_CMD_AMP_MUTE_OFF       0x07 // 7
#define SLINK_CMD_AMP_5_1_IN_ON      0x0C // 12
#define SLINK_CMD_AMP_5_1_IN_OFF     0x0D // 13
#define SLINK_CMD_AMP_SET_INPUT_CHAN 0x50 // 80

// S-Link Protocol Timings (microseconds)
#define SLINK_MARK_SYNC_US           2400
#define SLINK_MARK_ONE_US            1200
#define SLINK_MARK_ZERO_US           600
#define SLINK_MARK_DELIMITER_US      600
#define SLINK_WORD_DELIMITER_US      30000
#define SLINK_LINE_READY_US          3000
#define SLINK_LOOP_DELAY_US          25
#define SLINK_LOOP_TIMEOUT_US        500000

static int g_slink_pin_out = -1;
static int g_slink_pin_in  = -1;

static void slink_line_check(void)
{
    if (g_slink_pin_in < 0)
    {
        return; // TX-only mode if no RX pin is configured
    }

    uint32_t start = SLINK_NOW_US();
    uint32_t beginTimeout = start;

    do
    {
        SLINK_DELAY_US(SLINK_LOOP_DELAY_US);
        if (HAL_PIN_ReadDigitalInput(g_slink_pin_in) == 0)
        {
          start = SLINK_NOW_US();
        }
    } while (((uint32_t)(SLINK_NOW_US() - start) < SLINK_LINE_READY_US) && ((uint32_t)(SLINK_NOW_US() - beginTimeout) < SLINK_LOOP_TIMEOUT_US));
}

static void slink_write_sync(void)
{
    HAL_PIN_SetOutputValue(g_slink_pin_out, 1); // pull bus low
    SLINK_DELAY_US(SLINK_MARK_SYNC_US);
    HAL_PIN_SetOutputValue(g_slink_pin_out, 0); // release bus
    SLINK_DELAY_US(SLINK_MARK_DELIMITER_US);
}

static void slink_write_byte(uint8_t value)
{
    for (int i = 7; i >= 0; i--)
    {
        HAL_PIN_SetOutputValue(g_slink_pin_out, 1);
        SLINK_DELAY_US((value & (1 << i)) ? SLINK_MARK_ONE_US : SLINK_MARK_ZERO_US);
        HAL_PIN_SetOutputValue(g_slink_pin_out, 0);
        SLINK_DELAY_US(SLINK_MARK_DELIMITER_US);
    }
}

void SLink_SendCommand(uint8_t deviceId, uint8_t commandId)
{
    if (g_slink_pin_out < 0) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "SLink: TX pin not configured!");
        return;
    }

    slink_line_check();
    uint32_t start = SLINK_NOW_US();

    // Protect ~25ms pulse train from Wi-Fi / FreeRTOS jitter
    SLINK_CRIT_ENTER();
    slink_write_sync();
    slink_write_byte(deviceId);
    slink_write_byte(commandId);
    SLINK_CRIT_EXIT();

    // Post-frame padding (interruptible by other bus traffic)
    do
    {
        SLINK_DELAY_US(SLINK_LOOP_DELAY_US);

        if (g_slink_pin_in >= 0 && HAL_PIN_ReadDigitalInput(g_slink_pin_in) == 0)
        {
            break;
        }
    } while ((uint32_t)(SLINK_NOW_US() - start) < SLINK_WORD_DELIMITER_US);

    ADDLOG_INFO(LOG_FEATURE_DRV, "SLink: Sent 0x%02X 0x%02X", deviceId, commandId);
}

static commandResult_t CMD_SLink_VolUp(const void *context, const char *cmd, const char *args)
{
    SLink_SendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_VOLUME_UP);
    return CMD_RES_OK;
}

static commandResult_t CMD_SLink_VolDown(const void *context, const char *cmd, const char *args)
{
    SLink_SendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_VOLUME_DOWN);
    return CMD_RES_OK;
}

static commandResult_t CMD_SLink_PowerOn(const void *context, const char *cmd, const char *args)
{
    SLink_SendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_POWER_ON);
    return CMD_RES_OK;
}

static commandResult_t CMD_SLink_PowerOff(const void *context, const char *cmd, const char *args)
{
    SLink_SendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_POWER_OFF);
    return CMD_RES_OK;
}

// Generic command: SLinkSend <devDecOrHex> <cmdDecOrHex>
// (e.g. "SLinkSend 192 20")
static commandResult_t CMD_SLink_Send(const void *context, const char *cmd, const char *args)
{
    Tokenizer_TokenizeString(args, 0);

    if (Tokenizer_GetArgsCount() < 2)
    {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }

    uint8_t dev = (uint8_t)Tokenizer_GetArgInteger(0);
    uint8_t c   = (uint8_t)Tokenizer_GetArgInteger(1);
    SLink_SendCommand(dev, c);
    return CMD_RES_OK;
}

// Usage in autoexec.bat: startDriver SLink <txPin> [rxPin]
void DRV_SLink_Init(void)
{
    g_slink_pin_out = Tokenizer_GetArgIntegerDefault(1, -1);
    g_slink_pin_in  = Tokenizer_GetArgIntegerDefault(2, -1);

    if (g_slink_pin_out >= 0)
    {
        HAL_PIN_Setup_Output(g_slink_pin_out);
        HAL_PIN_SetOutputValue(g_slink_pin_out, 0);
    }

    if (g_slink_pin_in >= 0)
    {
        HAL_PIN_Setup_Input(g_slink_pin_in);
    }

    CMD_RegisterCommand("SLinkVolUp", CMD_SLink_VolUp, NULL);
    CMD_RegisterCommand("SLinkVolDown", CMD_SLink_VolDown, NULL);
    CMD_RegisterCommand("SLinkPowerOn", CMD_SLink_PowerOn, NULL);
    CMD_RegisterCommand("SLinkPowerOff", CMD_SLink_PowerOff, NULL);
    CMD_RegisterCommand("SLinkSend", CMD_SLink_Send, NULL);

    ADDLOG_INFO(LOG_FEATURE_DRV, "SLink: Initialized (TX=%d, RX=%d)", g_slink_pin_out, g_slink_pin_in);
}

#endif // ENABLE_DRIVER_SLINK


