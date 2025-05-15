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


void TCL_UART_Init(void) {

  UART_InitUART(TCL_baudRate, 0, false);
  UART_InitReceiveRingBuffer(TCL_UART_RECEIVE_BUFFER_SIZE);

}

void TCL_UART_RunEverySecond(void) {
  TCL_UART_TryToGetNextPacket();
  UART_InitUART(TCL_baudRate, 0, false);
}
