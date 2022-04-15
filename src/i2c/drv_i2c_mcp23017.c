#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"
// addresses, banks, etc, defines
#include "drv_i2c_mcp23017.h"

static byte MCP23017_readByte(i2cDevice_MCP23017_t *mcp, byte tgRegAddr )
{
    byte bufferRead;

	DRV_I2C_Begin(mcp->base.addr, mcp->base.busType);
    DRV_I2C_Read( tgRegAddr, &bufferRead );
	DRV_I2C_Close();

    return bufferRead;
}
static void MCP23017_writeByte( i2cDevice_MCP23017_t *mcp, byte tgRegAddr, byte toWrite )
{
	DRV_I2C_Begin(mcp->base.addr, mcp->base.busType);
    DRV_I2C_Write( tgRegAddr, toWrite );
	DRV_I2C_Close();
}
static void MCP23017_setBits( i2cDevice_MCP23017_t *mcp, byte tgRegAddr, byte bitMask )
{
    byte temp;

    temp = MCP23017_readByte( mcp, tgRegAddr );
    
    temp |= bitMask;
    
    MCP23017_writeByte( mcp, tgRegAddr, temp );
}
static void MCP23017_clearBits( i2cDevice_MCP23017_t *mcp, byte tgRegAddr, byte bitMask )
{
    byte temp;

    temp = MCP23017_readByte( mcp, tgRegAddr );
    
    temp &= ~bitMask;
    
    MCP23017_writeByte( mcp, tgRegAddr, temp );
}
static void MCP23017_toggleBits( i2cDevice_MCP23017_t *mcp, byte tgRegAddr, byte bitMask )
{
    byte temp;

    temp = MCP23017_readByte( mcp, tgRegAddr );
    
    temp ^= bitMask;
    
    MCP23017_writeByte( mcp, tgRegAddr, temp );
}
static void MCP23017_setDirectionPortA( i2cDevice_MCP23017_t *mcp, byte toWrite )
{
    MCP23017_writeByte( mcp, _MCP23017_IODIRA_BANK0, toWrite );
}

static void MCP23017_setDirectionPortB( i2cDevice_MCP23017_t *mcp, byte toWrite )
{
    MCP23017_writeByte( mcp, _MCP23017_IODIRB_BANK0, toWrite );
}
static void MCP23017_writePortA( i2cDevice_MCP23017_t *mcp, byte toWrite )
{
    MCP23017_writeByte( mcp, _MCP23017_OLATA_BANK0, toWrite );
}
static void MCP23017_toggleBitPortA(  i2cDevice_MCP23017_t *mcp, byte bitMask )
{
    MCP23017_toggleBits( mcp, _MCP23017_OLATA_BANK0, bitMask );
}


void DRV_I2C_MCP23017_OnChannelChanged(i2cDevice_t *dev, int channel, int iVal)
{
	i2cDevice_MCP23017_t *mcp;
	int i;
	int bank;
	int localBitIndex;
	int mask;

	mcp = (i2cDevice_MCP23017_t*)dev;

	MCP23017_setDirectionPortA(mcp, _MCP23017_PORT_DIRECTION_OUTPUT);
	MCP23017_setDirectionPortB(mcp, _MCP23017_PORT_DIRECTION_OUTPUT);

	// test only
	//MCP23017_toggleBitPortA(mcp, 0xFF);

	for(i = 0; i < 16; i++) {
		if(mcp->pinMapping[i] == channel) {
			addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_OnChannelChanged: will set pin %i to %i for ch %i\n", i, iVal, channel);

			// split 0-16 indices into two 8 bit ports - port A and port B
			if(i>=8) {
				localBitIndex = i - 8;
				bank = _MCP23017_OLATB_BANK0;
			} else {
				localBitIndex = i;
				bank = _MCP23017_OLATA_BANK0;
			}
			mask = 1 << localBitIndex;
			if(iVal) {
				//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_OnChannelChanged: calling MCP23017_setBits mask %i\n", mask);
				MCP23017_setBits(mcp, bank, mask);
			} else {
				//addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_OnChannelChanged: calling MCP23017_clearBits mask %i\n", mask);
				MCP23017_clearBits(mcp, bank, mask);
			}
		}
	}
}
int DRV_I2C_MCP23017_MapPinToChannel(const void *context, const char *cmd, const char *args) {
	const char *i2cModuleStr;
	int address;
	int targetPin;
	int targetChannel;
	i2cBusType_t busType;
	i2cDevice_MCP23017_t *mcp;

	Tokenizer_TokenizeString(args);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);
	targetPin = Tokenizer_GetArgInteger(2);
	targetChannel = Tokenizer_GetArgInteger(3);

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_MapPinToChannel: module %s, address %i, pin %i, ch %i\n", i2cModuleStr, address,targetPin,targetChannel );

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	mcp = (i2cDevice_MCP23017_t *)DRV_I2C_FindDeviceExt( busType, address,I2CDEV_MCP23017);
	if(mcp == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_MapPinToChannel: no such device exists\n" );
		return 0;
	}

	mcp->pinMapping[targetPin] = targetChannel;

	// send refresh
	DRV_I2C_MCP23017_OnChannelChanged((i2cDevice_t*)mcp, targetChannel, CHANNEL_Get(targetChannel));

	return 1;
}
void DRV_I2C_MCP23017_RunDevice(i2cDevice_t *dev)
{
	i2cDevice_MCP23017_t *mcp;

	mcp = (i2cDevice_MCP23017_t*)dev;

	// old test code, used only to make MCP23017 blink
#if 0
	MCP23017_setDirectionPortA(mcp, _MCP23017_PORT_DIRECTION_OUTPUT);
	MCP23017_toggleBitPortA(mcp, 0xFF);
#endif

	// Nothing to do here right now, as we are using on change pin callback
}