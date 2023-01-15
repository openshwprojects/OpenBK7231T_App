// My MAX72XX old library
// See: https://www.elektroda.pl/rtvforum/viewtopic.php?p=18040628#18040628

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_max72xx_internal.h"

#define LSBFIRST 0
#define MSBFIRST 1
#define HIGH 1
#define LOW 0

//the opcodes for the MAX7221 and MAX7219
#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

void PORT_shiftOut(int dataPin, int clockPin, int bitOrder, int val)
{
    int i;

    for (i = 0; i < 8; i++)  {
		usleep(20);
        if (bitOrder == LSBFIRST)
            HAL_PIN_SetOutputValue(dataPin, !!(val & (1 << i)));
        else
            HAL_PIN_SetOutputValue(dataPin, !!(val & (1 << (7 - i))));

		usleep(20);
        HAL_PIN_SetOutputValue(clockPin, HIGH);
		usleep(20);
        HAL_PIN_SetOutputValue(clockPin, LOW);
    }
}

void MAX72XX_spiTransfer(max72XX_t *led, int adddr,  unsigned char opcode,  byte datta) {
    int offset, maxbytes;
    int i;

    offset=adddr*2;
    maxbytes=led->maxDevices*2;

    for( i=0;i<maxbytes;i++)
        led->spidata[i]=(byte)0;
    led->spidata[offset+1]=opcode;
    led->spidata[offset]=datta;
    HAL_PIN_SetOutputValue(led->port_cs,LOW);
    for( i=maxbytes;i>0;i--)
        PORT_shiftOut(led->port_mosi,led->port_clk,MSBFIRST,led->spidata[i-1]);
   HAL_PIN_SetOutputValue(led->port_cs,HIGH);
}
void MAX72XX_shutdown(max72XX_t *led, int addr, bool b) {
    if(addr<0 || addr>=led->maxDevices)
        return;
    if(b)
        MAX72XX_spiTransfer(led,addr, OP_SHUTDOWN,0);
    else
        MAX72XX_spiTransfer(led,addr, OP_SHUTDOWN,1);
}
void MAX72XX_setScanLimit(max72XX_t *led, int addr, int limit) {
    if(addr<0 || addr>=led->maxDevices)
        return;
    if(limit>=0 && limit<8)
        MAX72XX_spiTransfer(led, addr, OP_SCANLIMIT,limit);
}
void MAX72XX_clearDisplay(max72XX_t *led,int addr) {
    int offset;
    int i;

    if(addr<0 || addr>=led->maxDevices)
        return;
    offset=addr*8;
    for( i=0;i<8;i++) {
        led->led_status[offset+i]=0;
        MAX72XX_spiTransfer(led,addr, i+1,led->led_status[offset+i]);
    }
}

void MAX72XX_setIntensity(max72XX_t *led,int addr, int intensity) {
    if(addr<0 || addr>=led->maxDevices)
        return;
    if(intensity>=0 && intensity<16)
        MAX72XX_spiTransfer(led,addr, OP_INTENSITY,intensity);
}
void MAX72XX_refresh(max72XX_t *led) {
    int i;
    //int mx;
    //byte tmp;
    int offset;
    int row;
    for(i = 0; i < led->maxDevices; i++)
    {
         for(row = 0; row < 8; row++)
         {
            offset=i*8;
            MAX72XX_spiTransfer(led,i, row+1,led->led_status[offset+row]);
         }
    }
}
void MAX72XX_displayArray(max72XX_t* led, byte *p, int devs, int ofs)
{
   memcpy(led->led_status+ofs*8,p,devs*8);
   MAX72XX_refresh(led);
}

void MAX72XX_shift(max72XX_t *led,int d) {
    int i;
    int mx;
   // byte tmp;
    //int offset;
    //int row;
     byte na[64];
    mx = led->maxDevices * 8;
    for(i = 0; i < mx; i++)
    {
      na[i] = 0;
    }
    if(d==1)
    {
        for(i = 0; i < mx; i++)
        {
         na[i] |= led->led_status[i]<<1;
         if(led->led_status[i] & 0b10000000 && i>0)
         {
            na[(i+8)%mx] |= 0b00000001;
         }
        }
    }
    else
    {
        for(i = 0; i < mx; i++)
        {
         na[i] |= led->led_status[i]>>1;
         if(led->led_status[i] & 0b00000001 && i>0)
         {
            na[(i-8+mx)%mx] |= 0b10000000;
         }
        }
    }
    for(i = 0; i < mx; i++)
    {
      led->led_status[i] = na[i];
    }
  //  for(i = 0; i < mx; i++)
  //  {
  //    led->led_status[i] = 0;
  //    if(i%2==0)
  //        led->led_status[i] =  0xff;
  //  }
    //led->led_status[mx] = tmp;
    MAX72XX_refresh(led);

}
void MAX72XX_setLed(max72XX_t *led,int addr, int row, int column, bool state) {
    int offset;
    byte val=0x00;

    if(addr<0 || addr>=led->maxDevices)
        return;
    if(row<0 || row>7 || column<0 || column>7)
        return;
    offset=addr*8;
    val=0b10000000 >> column;
    if(state)
        led->led_status[offset+row]=led->led_status[offset+row]|val;
    else {
        val=~val;
        led->led_status[offset+row]=led->led_status[offset+row]&val;
    }
    MAX72XX_spiTransfer(led,addr, row+1,led->led_status[offset+row]);
}
void MAX72XX_init(max72XX_t *led) {
    int i;

    HAL_PIN_SetOutputValue(led->port_cs,HIGH);
    for( i=0;i<64;i++)
        led->led_status[i]=0x00;
    for( i=0;i<led->maxDevices;i++) {
        MAX72XX_spiTransfer(led,i,OP_DISPLAYTEST,0);
        //scanlimit is set to max on startup
        MAX72XX_setScanLimit(led,i,7);
        //decode is done in source
        MAX72XX_spiTransfer(led,i,OP_DECODEMODE,0);
        MAX72XX_clearDisplay(led,i);
        //we go into shutdown-mode on startup
        MAX72XX_shutdown(led,i,1);
    }
    for( i=0;i<led->maxDevices;i++) {
        /*The MAX72XX is in power-saving mode on startup*/
        MAX72XX_shutdown(led, i,0);
        /* Set the brightness to a medium values */
        MAX72XX_setIntensity(led,i,8);
        /* and clear the display */
        MAX72XX_clearDisplay(led,i);
    }
}
void MAX72XX_setupPins(max72XX_t *led,  int csi, int clki, int mosii, int maxDevices)
{
	led->maxDevices = maxDevices;
	led->port_cs = csi;
	led->port_clk = clki;
	led->port_mosi = mosii;
	HAL_PIN_Setup_Output(csi);
	HAL_PIN_SetOutputValue(csi, 0);
	HAL_PIN_Setup_Output(clki);
	HAL_PIN_SetOutputValue(clki, 0);
	HAL_PIN_Setup_Output(mosii);
	HAL_PIN_SetOutputValue(mosii, 0);
}
max72XX_t *MAX72XX_alloc() {
	max72XX_t *ret = (max72XX_t*)malloc(sizeof(max72XX_t));
	if (ret == 0)
		return 0;
	memset(ret, 0, sizeof(max72XX_t));
	return ret;
}











