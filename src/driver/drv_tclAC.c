// https://github.com/lNikazzzl/tcl_ac_esphome/tree/master

#include "../obk_config.h"


#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "drv_uart.h"

#define TCL_UART_PACKET_LEN 1
#define TCL_UART_PACKET_HEAD 0xff
#define TCL_UART_RECEIVE_BUFFER_SIZE 256
#define TCL_baudRate	9600

#include "drv_tclAC.h"

//static int TCL_UART_TryToGetNextPacket() {
//  int cs;
//  int i;
//  int c_garbage_consumed = 0;
//  byte checksum;
//
//  cs = UART_GetDataSize();
//
//  if(cs < TCL_UART_PACKET_LEN) {
//    return 0;
//  }
//  // skip garbage data (should not happen)
//  while(cs > 0) {
//    //if (UART_GetByte(0) != TCL_UART_PACKET_HEAD)
//	  if(1)
//	{
//      UART_ConsumeBytes(1);
//      c_garbage_consumed++;
//      cs--;
//    } else {
//      break;
//    }
//  }
//  if(c_garbage_consumed > 0){
//    ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
//      "Consumed %i unwanted non-header byte in TCL buffer\n",
//      c_garbage_consumed);
//  }
//  if(cs < TCL_UART_PACKET_LEN) {
//    return 0;
//  }
//  //if (UART_GetByte(0) != 0x55)
//  //  return 0;
//
//
//  //if (checksum != UART_GetByte(TCL_UART_PACKET_LEN - 1)) {
//  //  ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
//  //    "Skipping packet with bad checksum %02X wanted %02X\n",
//  //    UART_GetByte(TCL_UART_PACKET_LEN - 1), checksum);
//  //  UART_ConsumeBytes(TCL_UART_PACKET_LEN);
//  //  return 1;
//  //}
//
//
//
//  UART_ConsumeBytes(TCL_UART_PACKET_LEN);
//  return TCL_UART_PACKET_LEN;
//}




uint8_t set_cmd_base[35] = { 0xBB, 0x00, 0x01, 0x03, 0x1D, 0x00, 0x00, 0x64, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
bool ready_to_send_set_cmd_flag = false;
set_cmd_t m_set_cmd = { 0 };
get_cmd_resp_t m_get_cmd_resp = { 0 };
int g_buzzer = 1;
int g_disp = 1;

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
	m_set_cmd.data.beep = g_buzzer;
	m_set_cmd.data.disp = g_disp;
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
typedef enum {
	FAN_OFF,
	FAN_1, // 1
	FAN_2, // 2
	FAN_3, // 3
	FAN_4, // 4
	FAN_5, // 5

	FAN_MUTE, // 6
	FAN_TURBO,
	FAN_AUTOMATIC,

} fanMode_e;

void OBK_SetTargetTemperature(float temp) {
	// User requested target temperature change

	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	get_cmd_resp.data.temp = (uint8_t)(temp) - 16;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}
void OBK_SetFanMode(fanMode_e fan_mode) {

	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	get_cmd_resp.data.turbo = 0x00;
	get_cmd_resp.data.mute = 0x00;
	if (fan_mode == FAN_TURBO) {
		get_cmd_resp.data.fan = 0x03;
		get_cmd_resp.data.turbo = 0x01;
	}
	else if (fan_mode == FAN_MUTE) {
		get_cmd_resp.data.fan = 0x01;
		get_cmd_resp.data.mute = 0x01;
	}
	else if (fan_mode == FAN_AUTOMATIC) get_cmd_resp.data.fan = 0x00;
	else if (fan_mode == FAN_1) get_cmd_resp.data.fan = 0x01;
	else if (fan_mode == FAN_2) get_cmd_resp.data.fan = 0x04;
	else if (fan_mode == FAN_3) get_cmd_resp.data.fan = 0x02;
	else if (fan_mode == FAN_4) get_cmd_resp.data.fan = 0x05;
	else if (fan_mode == FAN_5) get_cmd_resp.data.fan = 0x03;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;

}
void OBK_SetBuzzer(int buzzer) {

	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	g_buzzer = buzzer;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}
void OBK_SetDisplay(int display) {

	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	g_disp = display;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
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

int read_data_line(int readch, uint8_t *buffer, int len)
{
	static int pos = 0;
	static bool wait_len = false;
	static int skipch = 0;

	//ESP_LOGI("custom", "%02X", readch);

	if (readch >= 0) {
		if (readch == 0xBB && skipch == 0 && !wait_len) {
			pos = 0;
			skipch = 3; // wait char with len
			wait_len = true;
			if (pos < len - 1) buffer[pos++] = readch;
		}
		else if (skipch == 0 && wait_len) {
			if (pos < len - 1) buffer[pos++] = readch;
			skipch = readch + 1; // +1 control sum
			wait_len = false;
		}
		else if (skipch > 0) {
			if (pos < len - 1) buffer[pos++] = readch;
			if (--skipch == 0 && !wait_len) return pos;
		}
	}
	// No end of line has been found, so return -1.
	return -1;
}

bool is_valid_xor(uint8_t *buffer, int len)
{
	uint8_t xor_byte = 0;
	for (int i = 0; i < len - 1; i++) xor_byte ^= buffer[i];
	if (xor_byte == buffer[len - 1]) return true;
	else {
		ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "No valid xor crc %02X (calculated %02X)", buffer[len], xor_byte);
		return false;
	}

}
typedef enum {
	VS_NONE,
	VS_MoveFull,
	VS_MoveUpper,
	VS_MoveLower,
	VS_FixTop,
	VS_FixUpper,
	VS_FixMid,
	VS_FixLower,
	VS_FixBottom
} VerticalSwingMode;
typedef enum {
	HS_NONE,
	HS_MOVE_FULL,
	HS_MOVE_LEFT,
	HS_MOVE_MID,
	HS_MOVE_RIGHT,
	HS_FIX_LEFT,
	HS_FIX_MID_LEFT,
	HS_FIX_MID,
	HS_FIX_MID_RIGHT,
	HS_FIX_RIGHT
} HorizontalSwing;
void control_vertical_swing(VerticalSwingMode swing_mode) {
	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	get_cmd_resp.data.vswing_mv = 0;
	get_cmd_resp.data.vswing_fix = 0;

	switch (swing_mode) {
	case VS_MoveFull:   get_cmd_resp.data.vswing_mv = 0x01; break;
	case VS_MoveUpper:  get_cmd_resp.data.vswing_mv = 0x02; break;
	case VS_MoveLower:  get_cmd_resp.data.vswing_mv = 0x03; break;
	case VS_FixTop:     get_cmd_resp.data.vswing_fix = 0x01; break;
	case VS_FixUpper:   get_cmd_resp.data.vswing_fix = 0x02; break;
	case VS_FixMid:     get_cmd_resp.data.vswing_fix = 0x03; break;
	case VS_FixLower:   get_cmd_resp.data.vswing_fix = 0x04; break;
	case VS_FixBottom:  get_cmd_resp.data.vswing_fix = 0x05; break;
	case VS_NONE: default:  break;
	}

	get_cmd_resp.data.vswing = (get_cmd_resp.data.vswing_mv != 0) ? 0x01 : 0;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}
void control_horizontal_swing(HorizontalSwing swing_mode) {
	get_cmd_resp_t get_cmd_resp = { 0 };
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	get_cmd_resp.data.hswing_mv = 0;
	get_cmd_resp.data.hswing_fix = 0;

	switch (swing_mode) {
	case HS_MOVE_FULL:      get_cmd_resp.data.hswing_mv = 0x01; break;
	case HS_MOVE_LEFT:      get_cmd_resp.data.hswing_mv = 0x02; break;
	case HS_MOVE_MID:       get_cmd_resp.data.hswing_mv = 0x03; break;
	case HS_MOVE_RIGHT:     get_cmd_resp.data.hswing_mv = 0x04; break;
	case HS_FIX_LEFT:       get_cmd_resp.data.hswing_fix = 0x01; break;
	case HS_FIX_MID_LEFT:   get_cmd_resp.data.hswing_fix = 0x02; break;
	case HS_FIX_MID:        get_cmd_resp.data.hswing_fix = 0x03; break;
	case HS_FIX_MID_RIGHT:  get_cmd_resp.data.hswing_fix = 0x04; break;
	case HS_FIX_RIGHT:      get_cmd_resp.data.hswing_fix = 0x05; break;
	case HS_NONE: default:  break;
	}

	if (get_cmd_resp.data.vswing_mv) get_cmd_resp.data.hswing = 0x01;
	else get_cmd_resp.data.hswing = 0;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}

void print_hex_str(uint8_t *buffer, int len)
{
	char str[250] = { 0 };
	char *pstr = str;
	if (len * 2 > sizeof(str)) ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "too long byte data");

	for (int i = 0; i < len; i++) {
		pstr += sprintf(pstr, "%02X ", buffer[i]);
	}

	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "%s", str);
}
bool is_changed;
float target_temperature;
float current_temperature;
void set_target_temperature(float newTemp) {
	if (target_temperature == newTemp)
		return;
	is_changed = true;
	target_temperature = newTemp;
}

void set_current_temperature(float newTemp) {
	if (current_temperature == newTemp) return;
	is_changed = true;
	current_temperature = newTemp;
}
void TCL_UART_TryToGetNextPacket() {

	#define max_line_length 100
	static uint8_t buffer[max_line_length];

	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "Initial size: %i", UART_GetDataSize());
	while (UART_GetDataSize()) {
		int r = UART_GetByte(0);
		UART_ConsumeBytes(1);
		int len = read_data_line(r, buffer, max_line_length);
		//printf("Len %i, buffer[3] = %i \n", len, buffer[3]);
		if (len == sizeof(m_get_cmd_resp) && buffer[3] == 0x04) {
			memcpy(m_get_cmd_resp.raw, buffer, len);
			print_hex_str(buffer, len);
			if (is_valid_xor(buffer, len)) {
				float curr_temp = (((buffer[17] << 8) | buffer[18]) / 374 - 32) / 1.8;
				is_changed = false;

				ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "Ok we got reply with mode %i, fan %i, turbo %i, mute %i",
					(int)m_get_cmd_resp.data.power, (int)m_get_cmd_resp.data.fan,
					(int)m_get_cmd_resp.data.turbo, (int)m_get_cmd_resp.data.mute);
				/*if (m_get_cmd_resp.data.power == 0x00) set_mode(CLIMATE_MODE_OFF);
				else if (m_get_cmd_resp.data.mode == 0x01) set_mode(CLIMATE_MODE_COOL);
				else if (m_get_cmd_resp.data.mode == 0x03) set_mode(CLIMATE_MODE_DRY);
				else if (m_get_cmd_resp.data.mode == 0x02) set_mode(CLIMATE_MODE_FAN_ONLY);
				else if (m_get_cmd_resp.data.mode == 0x04) set_mode(CLIMATE_MODE_HEAT);
				else if (m_get_cmd_resp.data.mode == 0x05) set_mode(CLIMATE_MODE_AUTO);


				if (m_get_cmd_resp.data.turbo) set_custom_fan_mode((FAN_TURBO));
				else if (m_get_cmd_resp.data.mute) set_custom_fan_mode((FAN_MUTE));
				else if (m_get_cmd_resp.data.fan == 0x00) set_custom_fan_mode((FAN_AUTOMATIC));
				else if (m_get_cmd_resp.data.fan == 0x01) set_custom_fan_mode((FAN_1));
				else if (m_get_cmd_resp.data.fan == 0x04) set_custom_fan_mode((FAN_2));
				else if (m_get_cmd_resp.data.fan == 0x02) set_custom_fan_mode((FAN_3));
				else if (m_get_cmd_resp.data.fan == 0x05) set_custom_fan_mode((FAN_4));
				else if (m_get_cmd_resp.data.fan == 0x03) set_custom_fan_mode((FAN_5));


				if (m_get_cmd_resp.data.hswing && m_get_cmd_resp.data.vswing) set_swing_mode(CLIMATE_SWING_BOTH);
				else if (!m_get_cmd_resp.data.hswing && !m_get_cmd_resp.data.vswing) set_swing_mode(CLIMATE_SWING_OFF);
				else if (m_get_cmd_resp.data.vswing) set_swing_mode(CLIMATE_SWING_VERTICAL);
				else if (m_get_cmd_resp.data.hswing) set_swing_mode(CLIMATE_SWING_HORIZONTAL);

				if (m_get_cmd_resp.data.vswing_mv == 0x01) set_vswing_pos("Move full");
				else if (m_get_cmd_resp.data.vswing_mv == 0x02) set_vswing_pos("Move upper");
				else if (m_get_cmd_resp.data.vswing_mv == 0x03) set_vswing_pos("Move lower");
				else if (m_get_cmd_resp.data.vswing_fix == 0x01) set_vswing_pos("Fix top");
				else if (m_get_cmd_resp.data.vswing_fix == 0x02) set_vswing_pos("Fix upper");
				else if (m_get_cmd_resp.data.vswing_fix == 0x03) set_vswing_pos("Fix mid");
				else if (m_get_cmd_resp.data.vswing_fix == 0x04) set_vswing_pos("Fix lower");
				else if (m_get_cmd_resp.data.vswing_fix == 0x05) set_vswing_pos("Fix bottom");
				else set_vswing_pos("Last position");

				if (m_get_cmd_resp.data.hswing_mv == 0x01) set_hswing_pos("Move full");
				else if (m_get_cmd_resp.data.hswing_mv == 0x02) set_hswing_pos("Move left");
				else if (m_get_cmd_resp.data.hswing_mv == 0x03) set_hswing_pos("Move mid");
				else if (m_get_cmd_resp.data.hswing_mv == 0x04) set_hswing_pos("Move right");
				else if (m_get_cmd_resp.data.hswing_fix == 0x01) set_hswing_pos("Fix left");
				else if (m_get_cmd_resp.data.hswing_fix == 0x02) set_hswing_pos("Fix mid left");
				else if (m_get_cmd_resp.data.hswing_fix == 0x03) set_hswing_pos("Fix mid");
				else if (m_get_cmd_resp.data.hswing_fix == 0x04) set_hswing_pos("Fix mid right");
				else if (m_get_cmd_resp.data.hswing_fix == 0x05) set_hswing_pos("Fix right");
				else set_hswing_pos("Last position");*/

				ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "fan %02X", m_get_cmd_resp.data.fan);
				ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "mode %02X", m_get_cmd_resp.data.mode);
				set_target_temperature((float)(m_get_cmd_resp.data.temp + 16));
				set_current_temperature(curr_temp);
				if (is_changed)
				{
					//publish_state();
				}
			}
			//publish_state(buffer);
		}
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
static commandResult_t CMD_FANMode(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	mode = Tokenizer_GetArgInteger(0);
	OBK_SetFanMode(mode);
	return CMD_RES_OK;
}

void TCL_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
	if (bPreState) {

	}
	else {

		hprintf255(request, "<h3>Current temperature: %f</h3>", current_temperature);
		hprintf255(request, "<h3>Target temperature: %f</h3>", target_temperature);
	}

}
static commandResult_t CMD_SwingH(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	mode = Tokenizer_GetArgInteger(0);
	control_horizontal_swing(mode);
	return CMD_RES_OK;
}
static commandResult_t CMD_TargetTemperature(const void* context, const char* cmd, const char* args, int cmdFlags) {
	float target;

	Tokenizer_TokenizeString(args, 0);

	target = Tokenizer_GetArgFloat(0);
	OBK_SetTargetTemperature(target);
	return CMD_RES_OK;
}
static commandResult_t CMD_SwingV(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int mode;

	Tokenizer_TokenizeString(args, 0);

	mode = Tokenizer_GetArgInteger(0);
	control_vertical_swing(mode);
	return CMD_RES_OK;
}
static commandResult_t CMD_Display(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int display;

	Tokenizer_TokenizeString(args, 0);

	display = Tokenizer_GetArgInteger(0);
	OBK_SetDisplay(display);
	return CMD_RES_OK;
}
static commandResult_t CMD_Buzzer(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int buzzer;

	Tokenizer_TokenizeString(args, 0);

	buzzer = Tokenizer_GetArgInteger(0);
	OBK_SetBuzzer(buzzer);
	return CMD_RES_OK;
}
void TCL_Init(void) {

	UART_InitUART(TCL_baudRate, 2, false);
	UART_InitReceiveRingBuffer(TCL_UART_RECEIVE_BUFFER_SIZE);

	CMD_RegisterCommand("ACMode", CMD_ACMode, NULL);
	CMD_RegisterCommand("FANMode", CMD_FANMode, NULL);
	CMD_RegisterCommand("SwingH", CMD_SwingH, NULL);
	CMD_RegisterCommand("SwingV", CMD_SwingV, NULL);
	CMD_RegisterCommand("TargetTemperature", CMD_TargetTemperature, NULL);
	CMD_RegisterCommand("Buzzer", CMD_Buzzer, NULL);
	CMD_RegisterCommand("Display", CMD_Display, NULL);
}
