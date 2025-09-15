#include "drv_hlw8112.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_HLW8112

#include <stdint.h>
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_spi.h"
#include "hal_pins.h"

#define HLW8112_SPI_BAUD_RATE_HZ (1000000)
#define HLW8112_PIN_SCSN (19)

typedef enum _hlw8112_err_e
{
    HLW8112_ERR_NONE,
} hlw8112_err_t;

static hlw8112_err_t _hlw8112_spi_write_reg(hlw8112_reg_t reg, const uint8_t *data, size_t len)
{
    hlw8112_err_t err = HLW8112_ERR_NONE;
    uint8_t _reg = reg | 0x80;

    HAL_PIN_SetOutputValue(HLW8112_PIN_SCSN, 0);
    err |= SPI_WriteBytes(&_reg, sizeof(_reg));
    err |= SPI_WriteBytes(data, len);
    HAL_PIN_SetOutputValue(HLW8112_PIN_SCSN, 1);

    return err;
}

static hlw8112_err_t _hlw8112_spi_read_reg(hlw8112_reg_t reg, uint8_t *data, size_t len)
{
    hlw8112_err_t err = HLW8112_ERR_NONE;
    uint8_t _reg = reg & ~0x80;
    
    HAL_PIN_SetOutputValue(HLW8112_PIN_SCSN, 0);
    err |= SPI_WriteBytes(&_reg, sizeof(_reg));
    err |= SPI_ReadBytes(data, len);
    HAL_PIN_SetOutputValue(HLW8112_PIN_SCSN, 1);

    return err;
}

static hlw8112_err_t _hlw8112_send_cmd(hlw8112_cmd_t cmd)
{
    uint8_t _cmd = cmd;
    return _hlw8112_spi_write_reg(HLW8112_REG_CMD, &_cmd, sizeof(_cmd));
}


commandResult_t _hlw8112_cmd_read(const void* context, const char* cmd, const char* args, int cmdFlags) {
    uint8_t reg;
    uint16_t regval;

    Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
    // following check must be done after 'Tokenizer_TokenizeString',
    // so we know arguments count in Tokenizer. 'cmd' argument is
    // only for warning display
    if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
        return CMD_RES_NOT_ENOUGH_ARGUMENTS;
    }
    reg = Tokenizer_GetArgInteger(0);

    regval = _hlw8112_spi_read_reg(reg, &regval, sizeof(regval));

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "Reg: %4x Value: %4x", reg, regval);
    return CMD_RES_OK;
}

void HLW8112_SPI_Init(void)
{
    SPI_DriverInit();
    spi_config_t cfg;
    cfg.role = SPI_ROLE_MASTER;
    cfg.bit_width = SPI_BIT_WIDTH_8BITS;
    cfg.polarity = SPI_POLARITY_LOW;
    cfg.phase = SPI_PHASE_1ST_EDGE;
    cfg.wire_mode = SPI_3WIRE_MODE;
    cfg.baud_rate = HLW8112_SPI_BAUD_RATE_HZ;
    cfg.bit_order = SPI_MSB_FIRST;
    SPI_Init(&cfg);

    HAL_PIN_Setup_Output(HLW8112_PIN_SCSN);
    HAL_PIN_SetOutputValue(HLW8112_PIN_SCSN, 1);
    // CMD
    
	CMD_RegisterCommand("HLW8112_READ", _hlw8112_cmd_read, NULL);
}

void HLW8112_SPI_RunEverySecond(void)
{

}

#endif