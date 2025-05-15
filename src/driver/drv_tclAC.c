// https://github.com/lNikazzzl/tcl_ac_esphome/tree/master

#include "../obk_config.h"


#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"

#define TCL_UART_PACKET_LEN 1
#define TCL_UART_PACKET_HEAD 0xff
#define TCL_UART_RECEIVE_BUFFER_SIZE 256
#define TCL_baudRate	9600

#include "drv_tclAC.h"

static int TCL_UART_TryToGetNextPacket() {
  int cs;
  int i;
  int c_garbage_consumed = 0;
  byte checksum;

  cs = UART_GetDataSize();

  if(cs < TCL_UART_PACKET_LEN) {
    return 0;
  }
  // skip garbage data (should not happen)
  while(cs > 0) {
    //if (UART_GetByte(0) != TCL_UART_PACKET_HEAD)
	  if(1)
	{
      UART_ConsumeBytes(1);
      c_garbage_consumed++;
      cs--;
    } else {
      break;
    }
  }
  if(c_garbage_consumed > 0){
    ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
      "Consumed %i unwanted non-header byte in TCL buffer\n",
      c_garbage_consumed);
  }
  if(cs < TCL_UART_PACKET_LEN) {
    return 0;
  }
  //if (UART_GetByte(0) != 0x55)
  //  return 0;


  //if (checksum != UART_GetByte(TCL_UART_PACKET_LEN - 1)) {
  //  ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
  //    "Skipping packet with bad checksum %02X wanted %02X\n",
  //    UART_GetByte(TCL_UART_PACKET_LEN - 1), checksum);
  //  UART_ConsumeBytes(TCL_UART_PACKET_LEN);
  //  return 1;
  //}



  UART_ConsumeBytes(TCL_UART_PACKET_LEN);
  return TCL_UART_PACKET_LEN;
}




uint8_t set_cmd_base[35] = { 0xBB, 0x00, 0x01, 0x03, 0x1D, 0x00, 0x00, 0x64, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
bool ready_to_send_set_cmd_flag = false;
set_cmd_t m_set_cmd = { 0 };
get_cmd_resp_t m_get_cmd_resp = { 0 };

typedef enum {
	CLIMATE_MODE_OFF,
	CLIMATE_MODE_COOL,
	CLIMATE_MODE_DRY,
	CLIMATE_MODE_FAN_ONLY,
	CLIMATE_MODE_HEAT,
	CLIMATE_MODE_HEAT_COOL,
	CLIMATE_MODE_AUTO,
} climateMode_e;
void build_set_cmd(get_cmd_resp_t * get_cmd_resp) {
	memcpy(m_set_cmd.raw, set_cmd_base, sizeof(m_set_cmd.raw));


	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "build_set_cmd: sizeof(get_cmd_resp_t) = %i, sizeof(m_set_cmd.data) = %i, sizeof(m_set_cmd.raw) = %i", 
		sizeof(get_cmd_resp_t), sizeof(m_set_cmd.data), sizeof(m_set_cmd.raw));
	m_set_cmd.data.power = get_cmd_resp->data.power;
	m_set_cmd.data.off_timer_en = 0;
	m_set_cmd.data.on_timer_en = 0;
	m_set_cmd.data.beep = 1;
	m_set_cmd.data.disp = 1;
	m_set_cmd.data.eco = 0;

	switch (get_cmd_resp->data.mode) {
	case 0x01:
		m_set_cmd.data.mode = 0x03;
		break;
	case 0x03:
		m_set_cmd.data.mode = 0x02;
		break;
	case 0x02:
		m_set_cmd.data.mode = 0x07;
		break;
	case 0x04:
		m_set_cmd.data.mode = 0x01;
		break;
	case 0x05:
		m_set_cmd.data.mode = 0x08;
		break;
	}

	m_set_cmd.data.turbo = get_cmd_resp->data.turbo;
	m_set_cmd.data.mute = get_cmd_resp->data.mute;
	m_set_cmd.data.temp = 15 - get_cmd_resp->data.temp;

	switch (get_cmd_resp->data.fan) {
	case 0x00:
		m_set_cmd.data.fan = 0x00;
		break;
	case 0x01:
		m_set_cmd.data.fan = 0x02;
		break;
	case 0x04:
		m_set_cmd.data.fan = 0x06;
		break;
	case 0x02:
		m_set_cmd.data.fan = 0x03;
		break;
	case 0x05:
		m_set_cmd.data.fan = 0x07;
		break;
	case 0x03:
		m_set_cmd.data.fan = 0x05;
		break;
	}

	//m_set_cmd.data.vswing = get_cmd_resp->data.vswing ? 0x07 : 0x00;
	//m_set_cmd.data.hswing = get_cmd_resp->data.hswing;

	if (get_cmd_resp->data.vswing_mv) {
		m_set_cmd.data.vswing = 0x07;
		m_set_cmd.data.vswing_fix = 0;
		m_set_cmd.data.vswing_mv = get_cmd_resp->data.vswing_mv;
	}
	else if (get_cmd_resp->data.vswing_fix) {
		m_set_cmd.data.vswing = 0;
		m_set_cmd.data.vswing_fix = get_cmd_resp->data.vswing_fix;
		m_set_cmd.data.vswing_mv = 0;
	}

	if (get_cmd_resp->data.hswing_mv) {
		m_set_cmd.data.hswing = 0x01;
		m_set_cmd.data.hswing_fix = 0;
		m_set_cmd.data.hswing_mv = get_cmd_resp->data.hswing_mv;
	}
	else if (get_cmd_resp->data.hswing_fix) {
		m_set_cmd.data.hswing = 0;
		m_set_cmd.data.hswing_fix = get_cmd_resp->data.hswing_fix;
		m_set_cmd.data.hswing_mv = 0;
	}

	m_set_cmd.data.half_degree = 0;

	for (int i = 0; i < sizeof(m_set_cmd.raw) - 1; i++) m_set_cmd.raw[sizeof(m_set_cmd.raw) - 1] ^= m_set_cmd.raw[i];
}
void OBK_SetClimate(climateMode_e climate_mode)
{
	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "User set mode %i", climate_mode);

	if (climate_mode == CLIMATE_MODE_OFF) {
		get_cmd_resp.data.power = 0x00;
	}
	else {
		get_cmd_resp.data.power = 0x01;
		switch (climate_mode) {
		case CLIMATE_MODE_COOL:
			get_cmd_resp.data.mode = 0x01;
			break;
		case CLIMATE_MODE_DRY:
			get_cmd_resp.data.mode = 0x03;
			break;
		case CLIMATE_MODE_FAN_ONLY:
			get_cmd_resp.data.mode = 0x02;
			break;
		case CLIMATE_MODE_HEAT:
		case CLIMATE_MODE_HEAT_COOL:
			get_cmd_resp.data.mode = 0x04;
			break;
		case CLIMATE_MODE_AUTO:
			get_cmd_resp.data.mode = 0x05;
			break;
		case CLIMATE_MODE_OFF:
			get_cmd_resp.data.power = 0x00;
			break;
		}
	}

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}
void write_array(byte *b, int s) {
	for (int i = 0; i < s; i++) {
		UART_SendByte(b[i]);
	}
}
void TCL_UART_RunEverySecond(void) {
	uint8_t req_cmd[] = { 0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD };


	if (ready_to_send_set_cmd_flag) {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "Sending data");
		ready_to_send_set_cmd_flag = false;
		write_array(m_set_cmd.raw, sizeof(m_set_cmd.raw));
	}
	else {
		write_array(req_cmd, sizeof(req_cmd));
	}
	TCL_UART_TryToGetNextPacket();
}

static commandResult_t CMD_ACMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	mode = Tokenizer_GetArgInteger(0);
	OBK_SetClimate(mode);
	return CMD_RES_OK;
}
void TCL_Init(void) {

	UART_InitUART(TCL_baudRate, 0, false);
	UART_InitReceiveRingBuffer(TCL_UART_RECEIVE_BUFFER_SIZE);

	CMD_RegisterCommand("ACMode", CMD_ACMode, NULL);
}

