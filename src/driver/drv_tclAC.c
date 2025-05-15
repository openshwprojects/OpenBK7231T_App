
#include "../obk_config.h"


#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include "../cmnds/cmd_public.h"
#include "drv_uart.h"

#define TCL_UART_PACKET_LEN 1
#define TCL_UART_PACKET_HEAD 0xff

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
  if (UART_GetByte(0) != 0x55)
    return 0;
  checksum = TCL_UART_CMD_READ(TCL_UART_ADDR);

  for(i = 0; i < TCL_UART_PACKET_LEN-1; i++) {
    checksum += UART_GetByte(i);
  }
  checksum ^= 0xFF;

  if (checksum != UART_GetByte(TCL_UART_PACKET_LEN - 1)) {
    ADDLOG_WARN(LOG_FEATURE_ENERGYMETER,
      "Skipping packet with bad checksum %02X wanted %02X\n",
      UART_GetByte(TCL_UART_PACKET_LEN - 1), checksum);
    UART_ConsumeBytes(TCL_UART_PACKET_LEN);
    return 1;
  }



  UART_ConsumeBytes(TCL_UART_PACKET_LEN);
  return TCL_UART_PACKET_LEN;
}

void control_horizontal_swing(const std::string &swing_mode) {
	// Implement the vertical swing control logic
	ADDLOG_WARN(LOG_FEATURE_ENERGYMETER, "Horizontal swing set to: %s", swing_mode.c_str());

	get_cmd_resp_t get_cmd_resp;
	memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

	get_cmd_resp.data.hswing_mv = 0;
	get_cmd_resp.data.hswing_fix = 0;

	if (swing_mode == "Move full") get_cmd_resp.data.hswing_mv = 0x01;
	else if (swing_mode == "Move left") get_cmd_resp.data.hswing_mv = 0x02;
	else if (swing_mode == "Move mid") get_cmd_resp.data.hswing_mv = 0x03;
	else if (swing_mode == "Move right") get_cmd_resp.data.hswing_mv = 0x04;
	else if (swing_mode == "Fix left") get_cmd_resp.data.hswing_fix = 0x01;
	else if (swing_mode == "Fix mid left") get_cmd_resp.data.hswing_fix = 0x02;
	else if (swing_mode == "Fix mid") get_cmd_resp.data.hswing_fix = 0x03;
	else if (swing_mode == "Fix mid right") get_cmd_resp.data.hswing_fix = 0x04;
	else if (swing_mode == "Fix right") get_cmd_resp.data.hswing_fix = 0x05;

	if (get_cmd_resp.data.vswing_mv) get_cmd_resp.data.hswing = 0x01;
	else  get_cmd_resp.data.hswing = 0;

	build_set_cmd(&get_cmd_resp);
	ready_to_send_set_cmd_flag = true;
}

void TCL_UART_Init(void) {

  UART_InitUART(TCL_baudRate, 0, false);
  UART_InitReceiveRingBuffer(TCL_UART_RECEIVE_BUFFER_SIZE);

}

void TCL_UART_RunEverySecond(void) {
  TCL_UART_TryToGetNextPacket();
  UART_InitUART(TCL_baudRate, 0, false);
  UART_SendByte(TCL_UART_CMD_READ(TCL_UART_ADDR));
  UART_SendByte(TCL_UART_REG_PACKET);
}
