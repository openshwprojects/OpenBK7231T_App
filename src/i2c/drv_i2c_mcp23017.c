//
//	MCP23017 - I2C 16 bit port expander - both inputs and outputs, right now we use it only as outputs
//
// MCP23017 with A0=1, A1=1, A2=1
// addI2CDevice_MCP23017 I2C1 0x27

// maps channels 5 and 6 to GPIO A7 and A6 of MCP23017
// backlog setChannelType 5 toggle; setChannelType 6 toggle; addI2CDevice_MCP23017 I2C1 0x27; MCP23017_MapPinToChannel I2C1 0x27 7 5; MCP23017_MapPinToChannel I2C1 0x27 6 6

// maps channels 5 6 7 8 etc
// backlog setChannelType 5 toggle; setChannelType 6 toggle; setChannelType 7 toggle; setChannelType 8 toggle; setChannelType 9 toggle; setChannelType 10 toggle; setChannelType 11 toggle; addI2CDevice_MCP23017 I2C1 0x27; MCP23017_MapPinToChannel I2C1 0x27 7 5; MCP23017_MapPinToChannel I2C1 0x27 6 6; MCP23017_MapPinToChannel I2C1 0x27 5 7; MCP23017_MapPinToChannel I2C1 0x27 4 8; MCP23017_MapPinToChannel I2C1 0x27 3 9; MCP23017_MapPinToChannel I2C1 0x27 2 10; MCP23017_MapPinToChannel I2C1 0x27 1 11


#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_i2c_local.h"
// addresses, banks, etc, defines
#include "drv_i2c_mcp23017.h"


// Right now, MCP23017 port expander supports only output mode
// (map channel to MCP23017 output)
typedef struct i2cDevice_MCP23017_s {
	i2cDevice_t base;
	// private MCP23017 variables
	// Channel indices (0xff = none)
	byte pinMapping[16];
	// is pin an output or input?
	//int pinDirections;
} i2cDevice_MCP23017_t;



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
commandResult_t DRV_I2C_MCP23017_MapPinToChannel(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	int targetPin;
	int targetChannel;
	i2cBusType_t busType;
	i2cDevice_MCP23017_t *mcp;

	Tokenizer_TokenizeString(args,0);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);
	targetPin = Tokenizer_GetArgInteger(2);
	targetChannel = Tokenizer_GetArgInteger(3);

	addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_MapPinToChannel: module %s, address %i, pin %i, ch %i\n", i2cModuleStr, address,targetPin,targetChannel );

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	mcp = (i2cDevice_MCP23017_t *)DRV_I2C_FindDeviceExt( busType, address,I2CDEV_MCP23017);
	if(mcp == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C,"DRV_I2C_MCP23017_MapPinToChannel: no such device exists\n" );
		return CMD_RES_BAD_ARGUMENT;
	}

	mcp->pinMapping[targetPin] = targetChannel;

	// send refresh
	DRV_I2C_MCP23017_OnChannelChanged((i2cDevice_t*)mcp, targetChannel, CHANNEL_Get(targetChannel));

	return CMD_RES_OK;
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

void DRV_I2C_AddDevice_MCP23017_Internal(int busType, int address) {
	i2cDevice_MCP23017_t *dev;

	dev = malloc(sizeof(i2cDevice_MCP23017_t));

	dev->base.addr = address;
	dev->base.busType = busType;
	dev->base.type = I2CDEV_MCP23017;
	dev->base.next = 0;
	dev->base.runFrame = DRV_I2C_MCP23017_RunDevice;
	dev->base.channelChange = DRV_I2C_MCP23017_OnChannelChanged;
	memset(dev->pinMapping, 0xff, sizeof(dev->pinMapping));

	DRV_I2C_AddNextDevice((i2cDevice_t*)dev);
}
commandResult_t DRV_I2C_AddDevice_MCP23017(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *i2cModuleStr;
	int address;
	i2cBusType_t busType;

	Tokenizer_TokenizeString(args, 0);
	i2cModuleStr = Tokenizer_GetArg(0);
	address = Tokenizer_GetArgInteger(1);

	busType = DRV_I2C_ParseBusType(i2cModuleStr);

	if (DRV_I2C_FindDevice(busType, address)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_MCP23017: there is already some device on this bus with such addr\n");
		return CMD_RES_BAD_ARGUMENT;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_I2C, "DRV_I2C_AddDevice_MCP23017: module %s, address %i\n", i2cModuleStr, address);


	DRV_I2C_AddDevice_MCP23017_Internal(busType, address);

	return CMD_RES_OK;
}


void DRV_I2C_MCP23017_PreInit() {

	//cmddetail:{"name":"MCP23017_MapPinToChannel","args":"",
	//cmddetail:"descr":"Maps port expander bit to OBK channel",
	//cmddetail:"fn":"DRV_I2C_MCP23017_MapPinToChannel","file":"i2c/drv_i2c_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MCP23017_MapPinToChannel", DRV_I2C_MCP23017_MapPinToChannel, NULL);
	//cmddetail:{"name":"addI2CDevice_MCP23017","args":"",
	//cmddetail:"descr":"Adds a new I2C device - MCP23017",
	//cmddetail:"fn":"DRV_I2C_AddDevice_MCP23017","file":"i2c/drv_i2c_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("addI2CDevice_MCP23017", DRV_I2C_AddDevice_MCP23017, NULL);
}


