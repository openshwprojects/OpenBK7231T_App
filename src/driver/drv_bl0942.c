#include "drv_bl0942.h"

#include "../logging/logging.h"
#include "../new_pins.h"
#include "drv_bl_shared.h"
#include "drv_pwrCal.h"
#include "drv_spi.h"
#include "drv_uart.h"

#define BL0942_UART_BAUD_RATE 4800
#define BL0942_UART_RECEIVE_BUFFER_SIZE 256
#define BL0942_UART_ADDR 0 // 0 - 3
#define BL0942_UART_CMD_READ 0x58
#define BL0942_UART_REG_PACKET 0xAA
#define BL0942_UART_PACKET_HEAD 0x55
#define BL0942_UART_PACKET_LEN 23

// Datasheet says 900 kHz is supported, but it produced ~50% check sum errors  
#define BL0942_SPI_BAUD_RATE 800000 // 900000
#define BL0942_SPI_CMD_READ 0x58

#define BL0942_REG_I_RMS 0x03
#define BL0942_REG_V_RMS 0x04
#define BL0942_REG_WATT 0x06
#define BL0942_REG_FREQ 0x08

#define DEFAULT_VOLTAGE_CAL 15188
#define DEFAULT_CURRENT_CAL 251210
#define DEFAULT_POWER_CAL 598

static void ScaleAndUpdate(int raw_voltage, int raw_current, int raw_power,
                           int raw_frequency) {
    // those are not values like 230V, but unscaled
    ADDLOG_EXTRADEBUG(LOG_FEATURE_ENERGYMETER,
                      "Unscaled current %d, voltage %d, power %d, freq %d\n",
                      raw_current, raw_voltage, raw_power, raw_frequency);
    ADDLOG_EXTRADEBUG(LOG_FEATURE_ENERGYMETER,
                      "HEX Current: %08lX; "
                      "Voltage: %08lX; Power: %08lX;\n",
                      (unsigned long)raw_current, (unsigned long)raw_voltage,
                      (unsigned long)raw_power);

    // those are final values, like 230V
    float voltage, current, power;
    PwrCal_Scale(raw_voltage, raw_current, raw_power, &voltage, &current,
                 &power);
    float frequency = 2 * 500000.0 / raw_frequency;
	
    ADDLOG_DEBUG(LOG_FEATURE_ENERGYMETER,
                 "Real current %1.3lf, voltage %1.1lf, power %1.1lf, "
                 "frequency %1.2lf\n",
                 current, voltage, power, frequency);

    BL_ProcessUpdate(voltage, current, power, frequency);
}

static int UART_TryToGetNextPacket(void) {
	int cs;
	int i;
	int c_garbage_consumed = 0;
	byte a;
	byte checksum;

	cs = UART_GetDataSize();

	if(cs < BL0942_UART_PACKET_LEN) {
		return 0;
	}
	// skip garbage data (should not happen)
	while(cs > 0) {
		a = UART_GetNextByte(0);
		if(a != BL0942_UART_PACKET_HEAD) {
			UART_ConsumeBytes(1);
			c_garbage_consumed++;
			cs--;
		} else {
			break;
		}
	}
	if(c_garbage_consumed > 0){
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Consumed %i unwanted non-header byte in BL0942 buffer\n", c_garbage_consumed);
	}
	if(cs < BL0942_UART_PACKET_LEN) {
		return 0;
	}
	a = UART_GetNextByte(0);
	if(a != 0x55) {
		return 0;
	}
	checksum = BL0942_UART_CMD_READ;

	for(i = 0; i < BL0942_UART_PACKET_LEN-1; i++) {
		checksum += UART_GetNextByte(i);
	}
	checksum ^= 0xFF;

#if 1
    {
		char buffer_for_log[128];
		char buffer2[32];
		buffer_for_log[0] = 0;
		for(i = 0; i < BL0942_UART_PACKET_LEN; i++) {
			snprintf(buffer2, sizeof(buffer2), "%02X ",UART_GetNextByte(i));
			strcat_safe(buffer_for_log,buffer2,sizeof(buffer_for_log));
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"BL0942 received: %s\n", buffer_for_log);
	}
#endif

	if(checksum != UART_GetNextByte(BL0942_UART_PACKET_LEN-1)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"Skipping packet with bad checksum %02X wanted %02X\n",checksum,UART_GetNextByte(BL0942_UART_PACKET_LEN-1));
		UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
		return 1;
	}

    int voltage, current, power, frequency;
    current = (UART_GetNextByte(3) << 16) | (UART_GetNextByte(2) << 8) |
              UART_GetNextByte(1);
    voltage = (UART_GetNextByte(6) << 16) | (UART_GetNextByte(5) << 8) |
              UART_GetNextByte(4);
    power = (UART_GetNextByte(12) << 24) | (UART_GetNextByte(11) << 16) |
            (UART_GetNextByte(10) << 8);
    power = (power >> 8);

    frequency = (UART_GetNextByte(17) << 8) | UART_GetNextByte(16);

    ScaleAndUpdate(voltage, current, power, frequency);

#if 0
	{
		char res[128];
		// V=245.107925,I=109.921143,P=0.035618
		snprintf(res, sizeof(res),"V=%f,I=%f,P=%f\n",lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT],lastReadings[OBK_POWER]);
		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,res );
	}
#endif

	UART_ConsumeBytes(BL0942_UART_PACKET_LEN);
	return BL0942_UART_PACKET_LEN;
}

static void UART_SendRequest(void) {
	UART_InitUART(BL0942_UART_BAUD_RATE);
	UART_SendByte(BL0942_UART_CMD_READ);
	UART_SendByte(BL0942_UART_REG_PACKET);
}

static int SPI_ReadReg(uint8_t reg, uint32_t *val, uint8_t signed24) {
	uint8_t send[2];
	uint8_t recv[4];
	send[0] = BL0942_SPI_CMD_READ;
	send[1] = reg;
	SPI_Transmit(send, sizeof(send), recv, sizeof(recv));

	uint8_t checksum = send[0] + send[1] + recv[0] + recv[1] + recv[2];
	checksum ^= 0xFF;
	if (recv[3] != checksum) {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
			"Failed to read reg 0x%X: Bad checksum %02X wanted %02X", reg,
			recv[3], checksum);
		return -1;
	}

	*val = (recv[0] << 16) | (recv[1] << 8) | recv[2];
	if (signed24 && (recv[0] & 0x80))
		*val |= (0xFF << 24);

	return 0;
}

static void Init(void) {
    BL_Shared_Init();

    PwrCal_Init(PWR_CAL_DIVIDE, DEFAULT_VOLTAGE_CAL, DEFAULT_CURRENT_CAL,
                DEFAULT_POWER_CAL);
}

void BL0942_UART_Init(void) {
	Init();

	UART_InitUART(BL0942_UART_BAUD_RATE);
	UART_InitReceiveRingBuffer(BL0942_UART_RECEIVE_BUFFER_SIZE);
}

void BL0942_UART_RunFrame(void) {
	int len;

	//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER,"UART buffer size %i\n", UART_GetDataSize());

	len = UART_TryToGetNextPacket();
	// FIXME: BL0942_UART_RunFrame is called every second. With this logic
	// only every second second a package is requested. Is this on purpose?
	if(len > 0) {

	} else {
		UART_SendRequest();
	}
}

void BL0942_SPI_Init(void) {
	Init();

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
}

void BL0942_SPI_RunFrame(void) {
    int voltage, current, power, frequency;
    SPI_ReadReg(BL0942_REG_I_RMS, (uint32_t *)&current, 0);
    SPI_ReadReg(BL0942_REG_V_RMS, (uint32_t *)&voltage, 0);
    SPI_ReadReg(BL0942_REG_WATT, (uint32_t *)&power, 1);
    SPI_ReadReg(BL0942_REG_FREQ, (uint32_t *)&frequency, 0);

    ScaleAndUpdate(voltage, current, power, frequency);
}
