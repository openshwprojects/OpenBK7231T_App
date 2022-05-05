// 2x16 LCD with I2C backpack HD44780U/PCF8574T
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"

// Line addresses for LCDs which use
// the Hitachi HD44780U controller chip
#define LCD_PIC_LINE_1_ADDRESS 0x00
#define LCD_PIC_LINE_2_ADDRESS 0x40
#define LCD_PIC_LINE_3_ADDRESS 0x14
#define LCD_PIC_LINE_4_ADDRESS 0x54


static byte PCF8574_LCD_Build_Byte(i2cDevice_PCF8574_t *lcd)
{
    byte ret = 0x00;

    ret |= lcd->pin_E    << 2;
    ret |= lcd->pin_RW   << 1;
    ret |= lcd->pin_RS   << 0;
    ret |= lcd->pin_D4   << 4;
    ret |= lcd->pin_D5   << 5;
    ret |= lcd->pin_D6   << 6;
    ret |= lcd->pin_D7   << 7;
    ret |= lcd->pin_BL   << 3;

    return ret;
}
static void PCF8574_LCD_Write_Upper_Nibble(i2cDevice_PCF8574_t *lcd, byte u)
{
	byte toSend[3];

    // Send upper nibble
    if(BIT_CHECK(u,7))
        lcd->pin_D7=1;
    else
        lcd->pin_D7=0;

    if(BIT_CHECK(u,6))
        lcd->pin_D6=1;
    else
        lcd->pin_D6=0;

    if(BIT_CHECK(u,5))
        lcd->pin_D5=1;
    else
        lcd->pin_D5=0;

    if(BIT_CHECK(u,4))
        lcd->pin_D4=1;
    else
        lcd->pin_D4=0;

   lcd->pin_E = 0;
   toSend[0] = (PCF8574_LCD_Build_Byte(lcd));
   lcd->pin_E = 1;
   toSend[1] = (PCF8574_LCD_Build_Byte(lcd));
   lcd->pin_E = 0;
   toSend[2] = (PCF8574_LCD_Build_Byte(lcd));

   DRV_I2C_WriteBytes(toSend[0],&toSend[1],2);
}

static void PCF8574_LCD_Write_Lower_Nibble(i2cDevice_PCF8574_t *lcd, byte l)
{
	byte toSend[3];

    // Send lower nibble
    if(BIT_CHECK(l,3))
        lcd->pin_D7=1;
    else
        lcd->pin_D7=0;

    if(BIT_CHECK(l,2))
        lcd->pin_D6=1;
    else
        lcd->pin_D6=0;

    if(BIT_CHECK(l,1))
        lcd->pin_D5=1;
    else
        lcd->pin_D5=0;

    if(BIT_CHECK(l,0))
        lcd->pin_D4=1;
    else
        lcd->pin_D4=0;

    lcd->pin_E = 0;
    toSend[0]=(PCF8574_LCD_Build_Byte(lcd));
    lcd->pin_E = 1;
     toSend[1]=(PCF8574_LCD_Build_Byte(lcd));
    lcd->pin_E = 0;
     toSend[2]=(PCF8574_LCD_Build_Byte(lcd));

   DRV_I2C_WriteBytes(toSend[0],&toSend[1],2);
}

static void PCF8574_LCD_Write_Byte(i2cDevice_PCF8574_t *lcd, byte address, byte n)
{
    if (address)
    {
        lcd->pin_RS=1;   // Data
    }
    else
    {
        lcd->pin_RS=0;   // Command
    }

    lcd->pin_RW  = 0;
    lcd->pin_E   = 0;
    lcd->pin_BL  = lcd->LCD_BL_Status;

    // Send upper nibble
   PCF8574_LCD_Write_Upper_Nibble(lcd,n);

    // Send lower nibble
   PCF8574_LCD_Write_Lower_Nibble(lcd,n);
}
static void PCF8574_LCD_Goto(i2cDevice_PCF8574_t *lcd, byte x, byte y)
{
	byte address;

   switch(y)
     {
      case 1:
        address = LCD_PIC_LINE_1_ADDRESS;
        break;
      case 2:
        address = LCD_PIC_LINE_2_ADDRESS;
        break;
      case 3:
        address = LCD_PIC_LINE_3_ADDRESS;
        break;
      case 4:
        address = LCD_PIC_LINE_4_ADDRESS;
        break;
      default:
        address = LCD_PIC_LINE_1_ADDRESS;
        break;
     }

   address += x-1;
   PCF8574_LCD_Write_Byte(lcd,0, 0x80 | address);
}

static int PCF8574_LCD_Open(i2cDevice_PCF8574_t *lcd)
{
    int status;
	DRV_I2C_Begin(lcd->base.addr, lcd->base.busType);
	status = 1;
   return status;
}
static void PCF8574_LCD_Close(i2cDevice_PCF8574_t *lcd)
{
	DRV_I2C_Close();
}

static void PCF8574_LCD_BL(i2cDevice_PCF8574_t *lcd, byte status)
{
    lcd->LCD_BL_Status = status;
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x00);
}

static int PCF8574_LCD_Init(i2cDevice_PCF8574_t *lcd)
{
    int s = PCF8574_LCD_Open(lcd);
     if(s == 0)
     return 0;
    // return 1;
    // Following bytes are all Command bytes, i.e. address = 0x00
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x03);   // Write Nibble 0x03 three times (per HD44780U initialization spec)
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x03);   //
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x03);   //
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x02);   // Write Nibble 0x02 once (per HD44780U initialization spec)
    delay_ms(5);
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x02);   // Write Nibble 0x02 once (per HD44780U initialization spec)
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x01);   // Set mode: 4-bit, 2+lines, 5x8 dots
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x0C);   // Display ON 0x0C
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x01);   // Clear display
    delay_ms(5);                // (per HD44780U initialization spec)
    PCF8574_LCD_Write_Byte(lcd,0x00, 0x06);   // Set cursor to increment
    delay_ms(5);                // (per HD44780U initialization spec)

  //  PCF8574_LCD_Close();
     return 1;
}
static void PCF8574_LCD_Clear(i2cDevice_PCF8574_t *lcd)
{
    PCF8574_LCD_Write_Byte(lcd,0x00,0x01);
    delay_ms(5);
}
static void PCF8574_LCD_Write_String(i2cDevice_PCF8574_t *lcd, const char *str)
{
   // Writes a string text[] to LCD via I2C
   lcd->pin_RS  = 1;
   lcd->pin_RW  = 0;
   lcd->pin_E   = 0;
   lcd->pin_BL  = lcd->LCD_BL_Status;

   while (*str)
   {
        // Send upper nibble
        PCF8574_LCD_Write_Upper_Nibble(lcd,*str);

        // Send lower nibble
        PCF8574_LCD_Write_Lower_Nibble(lcd,*str);

        str++;
   }
}

int DRV_I2C_LCD_PCF8574_GoTo_Internal(int bClear, const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;
	i2cDevice_t *dev;
	i2cDevice_PCF8574_t *lcd;
	byte x, y;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	dev = DRV_I2C_FindDeviceExt(busType,address,I2CDEV_LCD_PCF8574);

	if(dev == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_GoTo: there is already some device on this bus with such addr\n");
		return 1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_GoTo: module %s, address %i\n", i2cModuleStr, address);

	lcd = (i2cDevice_PCF8574_t *)dev;

	x = Tokenizer_GetArgInteger(2);
	y = Tokenizer_GetArgInteger(3);

	PCF8574_LCD_Open(lcd);
	if(bClear){
		PCF8574_LCD_Clear(lcd);
	}
	PCF8574_LCD_Goto(lcd,x,y);
	PCF8574_LCD_Close(lcd);
	return 1;
}
int DRV_I2C_LCD_PCF8574_GoTo(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return DRV_I2C_LCD_PCF8574_GoTo_Internal(0,context,cmd,args,cmdFlags);
}
int DRV_I2C_LCD_PCF8574_ClearAndGoTo(const void *context, const char *cmd, const char *args, int cmdFlags) {
	return DRV_I2C_LCD_PCF8574_GoTo_Internal(1,context,cmd,args,cmdFlags);
}


int DRV_I2C_LCD_PCF8574_Print(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;
	i2cDevice_t *dev;
	i2cDevice_PCF8574_t *lcd;
	const char *msg;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	dev = DRV_I2C_FindDeviceExt(busType,address,I2CDEV_LCD_PCF8574);

	if(dev == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_Print: there is already some device on this bus with such addr\n");
		return 1;
	}
	lcd = (i2cDevice_PCF8574_t *)dev;

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_Print: module %s, address %i\n", i2cModuleStr, address);
	msg = Tokenizer_GetArgFrom(2);

	PCF8574_LCD_Open(lcd);
	PCF8574_LCD_Write_String(lcd,msg);
	PCF8574_LCD_Close(lcd);

	return 1;
}
int DRV_I2C_LCD_PCF8574_Clear(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;
	i2cDevice_t *dev;
	i2cDevice_PCF8574_t *lcd;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	dev = DRV_I2C_FindDeviceExt(busType,address,I2CDEV_LCD_PCF8574);

	if(dev == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_Clear: there is already some device on this bus with such addr\n");
		return 1;
	}
	lcd = (i2cDevice_PCF8574_t *)dev;

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_LCD_PCF8574_Clear: module %s, address %i\n", i2cModuleStr, address);

	PCF8574_LCD_Open(lcd);
	PCF8574_LCD_Clear(lcd);
	PCF8574_LCD_Close(lcd);

	return 1;
}
static int g_i2c_cmds_lcd_init = 0;

void DRV_I2C_Commands_Init() {

	CMD_RegisterCommand("lcd_clearAndGoto","",DRV_I2C_LCD_PCF8574_ClearAndGoTo, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("lcd_goto","",DRV_I2C_LCD_PCF8574_GoTo, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("lcd_print","",DRV_I2C_LCD_PCF8574_Print, "Adds a new I2C device", NULL);
	CMD_RegisterCommand("lcd_clear","",DRV_I2C_LCD_PCF8574_Clear, "Adds a new I2C device", NULL);
}
/// backlog startDriver I2C; addI2CDevice_LCD_PCF8574 I2C1 0x23 0 0 0
// lcd_print I2C1 0x23 Hello123
// backlog lcd_goto I2C1 0x23 2 2; lcd_print I2C1 0x23 Teste123
// lcd_clear I2C1 0x23
int c = 0;
void DRV_I2C_LCD_PCF8574_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_PCF8574_t *lcd;

	if(g_i2c_cmds_lcd_init==0) {
		DRV_I2C_Commands_Init();
		g_i2c_cmds_lcd_init = 1;
	}
	lcd = (i2cDevice_PCF8574_t*)dev;

	if(c == 0){
		PCF8574_LCD_Init(lcd);
		 delay_ms(115);
		PCF8574_LCD_Clear(lcd);
		delay_ms(115);
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"Testing lcd\n" );
	 
		PCF8574_LCD_Write_String(lcd,"OpenBeken BK7231T LCD");
		delay_ms(115);
		PCF8574_LCD_Goto(lcd,2,2);
		PCF8574_LCD_Write_String(lcd,"Elektroda.com");
		delay_ms(115);
		PCF8574_LCD_Close(lcd);
	}
	c++;

}

