#include "drv_bl0942.h"

#include <math.h>
#include <stdint.h>

#include "../logging/logging.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_spi.h"
#include "drv_uart.h"

static unsigned short bl0942_baudRate = 4800;

#define BL0942_UART_RECEIVE_BUFFER_SIZE 256
#define BL0942_UART_ADDR 0 // 0 - 3
#define BL0942_UART_CMD_READ(addr) (0x58 | addr)
#define BL0942_UART_CMD_WRITE(addr) (0xA8 | addr)
#define BL0942_UART_REG_PACKET 0xAA
#define BL0942_UART_PACKET_HEAD 0x55
#define BL0942_UART_PACKET_LEN 23

// Datasheet says 900 kHz is supported, but it produced ~50% check sum errors  
#define BL0942_SPI_BAUD_RATE 800000 // 900000
#define BL0942_SPI_CMD_READ 0x58
#define BL0942_SPI_CMD_WRITE 0xA8

// Electric parameter register (read only)
#define BL0942_REG_I_RMS 0x03
#define BL0942_REG_V_RMS 0x04
#define BL0942_REG_WATT 0x06
#define BL0942_REG_CF_CNT 0x7
#define BL0942_REG_FREQ 0x08
#define BL0942_REG_USR_WRPROT 0x1D
#define BL0942_USR_WRPROT_DISABLE 0x55

// User operation register (read and write)
#define BL0942_REG_MODE 0x19
#define BL0942_MODE_DEFAULT 0x87
#define BL0942_MODE_RMS_UPDATE_SEL_800_MS (1 << 3)

#define DEFAULT_VOLTAGE_CAL 15188
#define DEFAULT_CURRENT_CAL 251210
#define DEFAULT_POWER_CAL 598

#define CF_CNT_INVALID (1 << 31)

typedef struct {
    uint32_t i_rms;
    uint32_t v_rms;
    int32_t watt;
    uint32_t cf_cnt;
    uint32_t freq;
} bl0942_data_t;

static uint32_t PrevCfCnt = CF_CNT_INVALID;

static int32_t Int24ToInt32(int32_t val) {
    return (val & (1 << 23) ? val | (0xFF << 24) : val);
}

static void ScaleAndUpdate(bl0942_data_t *data) {
    float voltage, current, power;
    PwrCal_Scale(data->v_rms, data->i_rms, data->watt, &voltage, &current,
                 &power);

    float frequency = 2 * 500000.0f / data->freq;

    float energyWh = 0;
    if (PrevCfCnt != CF_CNT_INVALID) {
        int diff = (data->cf_cnt < PrevCfCnt
                        ? data->cf_cnt + (0xFFFFFF - PrevCfCnt) + 1
                        : data->cf_cnt - PrevCfCnt);
        energyWh =
            fabsf(PwrCal_ScalePowerOnly(diff)) * 1638.4f * 256.0f / 3600.0f;
    }
    PrevCfCnt = data->cf_cnt;

    BL_ProcessUpdate(voltage, current, power, frequency, energyWh);
}

static int BL0942_UART_TryToGetNextPacket(void) {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte checksum;

	cs = UART_GetDataSize();

	if(cs < BL0942_UART_PACKET_LEN) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
        if (UART_GetByte(0) != BL0942_UART_PACKET_HEAD) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
        ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
                    "Consumed %i unwanted non-header byte in BL0942 buffer\n",
                    c_garbage_consumed);
	}
	if(cs < BL0942_UART_PACKET_LEN) {
		return 0;
	}
    if (UART_GetByte(0) != 0x55)
		return 0;
    checksum = BL0942_UART_CMD_READ(BL0942_UART_ADDR);

    for(i = 0; i < BL0942_UART_PACKET_LEN-1; i++) {
        checksum += UART_GetByte(i);
	}
	checksum ^= 0xFF;

    if (checksum != UART_GetByte(BL0942_UART_PACKET_LEN - 1)) {
        ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
                    "Skipping packet with bad checksum %02X wanted %02X\n",
                    UART_GetByte(BL0942_UART_PACKET_LEN - 1), checksum);
        UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
		return 1;
	}

    bl0942_data_t data;
    data.i_rms =
        (UART_GetByte(3) << 16) | (UART_GetByte(2) << 8) | UART_GetByte(1);
    data.v_rms =
        (UART_GetByte(6) << 16) | (UART_GetByte(5) << 8) | UART_GetByte(4);
    data.watt = Int24ToInt32((UART_GetByte(12) << 16) |
                             (UART_GetByte(11) << 8) | UART_GetByte(10));
    data.cf_cnt =
        (UART_GetByte(15) << 16) | (UART_GetByte(14) << 8) | UART_GetByte(13);
    data.freq = (UART_GetByte(17) << 8) | UART_GetByte(16);
    ScaleAndUpdate(&data);

    UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
	return BL0942_UART_PACKET_LEN;
}

static void UART_WriteReg(uint8_t reg, uint32_t val) {
    uint8_t send[5];
    send[0] = BL0942_UART_CMD_WRITE(BL0942_UART_ADDR);
    send[1] = reg;
    send[2] = (val & 0xFF);
    send[3] = ((val >> 8) & 0xFF);
    send[4] = ((val >> 16) & 0xFF);
    uint8_t crc = 0;

    for (int i = 0; i < sizeof(send); i++) {
        UART_SendByte(send[i]);
        crc += send[i];
    }

    UART_SendByte(crc ^ 0xFF);
}

static int SPI_ReadReg(uint8_t reg, uint32_t *val) {
	uint8_t send[2];
	uint8_t recv[4];
	send[0] = BL0942_SPI_CMD_READ;
	send[1] = reg;
	SPI_Transmit(send, sizeof(send), recv, sizeof(recv));

	uint8_t checksum = send[0] + send[1] + recv[0] + recv[1] + recv[2];
	checksum ^= 0xFF;
	if (recv[3] != checksum) {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
                    "Failed to read reg %02X: bad checksum %02X wanted %02X",
                    reg, recv[3], checksum);
		return -1;
	}

	*val = (recv[0] << 16) | (recv[1] << 8) | recv[2];
	return 0;
}

static int SPI_WriteReg(uint8_t reg, uint32_t val) {
    uint8_t send[6];
    send[0] = BL0942_SPI_CMD_WRITE;
    send[1] = reg;
    send[2] = ((val >> 16) & 0xFF);
    send[3] = ((val >> 8) & 0xFF);
    send[4] = (val & 0xFF);

    // checksum
    send[5] = send[0] + send[1] + send[2] + send[3] + send[4];
    send[5] ^= 0xFF;

    SPI_WriteBytes(send, sizeof(send));

    uint32_t read;
    SPI_ReadReg(reg, &read);
    if (read == val ||
        // REG_USR_WRPROT is read back as 0x1
        (reg == BL0942_REG_USR_WRPROT && val == BL0942_USR_WRPROT_DISABLE &&
         read == 0x1)) {
        return 0;
    }

    ADDLOG_ERROR(LOG_FEATURE_ENERGYMETER,
                 "Failed to write reg %02X val %02X: read %02X", reg, val,
                 read);
    return -1;
}

static void BL0942_Init(void) {
    PrevCfCnt = CF_CNT_INVALID;

    BL_Shared_Init();

    PwrCal_Init(PWR_CAL_DIVIDE, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
                DEFAULT_POWER_CAL);
}

// THIS IS called by 'startDriver BL0942' command
// You can set alternate baud with 'startDriver BL0942 9600' syntax
void BL0942_UART_Init(void) {
	BL0942_Init();

	bl0942_baudRate = Tokenizer_GetArgIntegerDefault(1, 4800);

	UART_InitUART(bl0942_baudRate, 0);
	UART_InitReceiveRingBuffer(BL0942_UART_RECEIVE_BUFFER_SIZE);

    UART_WriteReg(BL0942_REG_USR_WRPROT, BL0942_USR_WRPROT_DISABLE);
    UART_WriteReg(BL0942_REG_MODE,
                  BL0942_MODE_DEFAULT | BL0942_MODE_RMS_UPDATE_SEL_800_MS);
}

void BL0942_UART_RunEverySecond(void) {
    BL0942_UART_TryToGetNextPacket();

    UART_InitUART(bl0942_baudRate, 0);

    UART_SendByte(BL0942_UART_CMD_READ(BL0942_UART_ADDR));
    UART_SendByte(BL0942_UART_REG_PACKET);
}

void BL0942_SPI_Init(void) {
	BL0942_Init();

	SPI_DriverInit();
	spi_config_t cfg;
	cfg.role = SPI_ROLE_MASTER;
	cfg.bit_width = SPI_BIT_WIDTH_8BITS;
	cfg.polarity = SPI_POLARITY_LOW;
	cfg.phase = SPI_PHASE_2ND_EDGE;
	cfg.wire_mode = SPI_3WIRE_MODE;
	cfg.baud_rate = BL0942_SPI_BAUD_RATE;
	cfg.bit_order = SPI_MSB_FIRST;
	SPI_Init(&cfg);

    SPI_WriteReg(BL0942_REG_USR_WRPROT, BL0942_USR_WRPROT_DISABLE);
    SPI_WriteReg(BL0942_REG_MODE,
                 BL0942_MODE_DEFAULT | BL0942_MODE_RMS_UPDATE_SEL_800_MS);
}

void BL0942_SPI_RunEverySecond(void) {
    bl0942_data_t data;
    SPI_ReadReg(BL0942_REG_I_RMS, &data.i_rms);
    SPI_ReadReg(BL0942_REG_V_RMS, &data.v_rms);
    SPI_ReadReg(BL0942_REG_WATT, (uint32_t *)&data.watt);
    data.watt = Int24ToInt32(data.watt);
    SPI_ReadReg(BL0942_REG_CF_CNT, &data.cf_cnt);
    SPI_ReadReg(BL0942_REG_FREQ, &data.freq);
    ScaleAndUpdate(&data);
}
