#include <string.h>
#include <stdlib.h>
#include "be_object.h"
#include "be_strlib.h"
#include "be_mem.h"
#include "be_sys.h"
#include "../../driver/drv_local.h"
#include "../../logging/logging.h"

static int m_initI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 2 && be_isint(vm, 1) && be_isint(vm, 2) && be_isint(vm, 3))
	{
		softI2C_t* i2c = malloc(sizeof(softI2C_t));
		memset(i2c, 0, sizeof(softI2C_t));
		i2c->pin_clk = be_toint(vm, 1);
		i2c->pin_data = be_toint(vm, 2);
		if(be_isint(vm, 3))
		{
			i2c->address8bit = be_toint(vm, 3) << 1;
		}
		else
		{
			i2c->address8bit = 0xFF;
			be_writestring("No address configured, only scan is available");
		}
		Soft_I2C_PreInit(i2c);
		be_pushcomptr(vm, i2c);
		be_newcomobj(vm, i2c, &be_commonobj_destroy_generic);
		be_return(vm);
	}
	be_return_nil(vm);
}

static int m_scanI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 1 && be_iscomptr(vm, 1))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		for(int a = 1; a < 128; a++)
		{
			Soft_I2C_PreInit(i2c);
			bool bOk = Soft_I2C_Start(i2c, (a << 1) + 0);
			Soft_I2C_Stop(i2c);
			if(bOk)
			{
				ADDLOG_INFO(LOG_FEATURE_BERRY, "Address 0x%x (dec %i)\n", a, a);
			}
			rtos_delay_milliseconds(5);
		}
	}
	be_return_nil(vm);
}

static int m_startI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 2 && be_iscomptr(vm, 1) && be_isbool(vm, 2))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		bool isRead = be_tobool(vm, 2);
		Soft_I2C_Start(i2c, i2c->address8bit | (int)isRead);
	}
	be_return_nil(vm);
}

static int m_stopI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 1 && be_iscomptr(vm, 1))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		Soft_I2C_Stop(i2c);
	}
	be_return_nil(vm);
}

static int m_writeI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 2 && be_iscomptr(vm, 1) && be_isinstance(vm, 2))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		if(be_isinstance(vm, 2))
		{
			size_t len;
			uint8_t* buf = (uint8_t*)be_tobytes(vm, 2, &len);
			for(int i = 0; i < len; i++)
			{
				Soft_I2C_WriteByte(i2c, buf[i]);
			}
		}
		else if(be_isint(vm, 2))
		{
			int data = be_toint(vm, 2);
			Soft_I2C_WriteByte(i2c, data);
		}
	}
	be_return_nil(vm);
}

static int m_readByteI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 2 && be_iscomptr(vm, 1) && be_isbool(vm, 2))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		int nack = be_tobool(vm, 2);
		uint8_t data = Soft_I2C_ReadByte(i2c, nack);
		be_pushint(vm, data);
		be_return(vm);
	}
	be_return_nil(vm);
}

static int m_readBytesI2c(bvm* vm)
{
	int top = be_top(vm);
	if(top >= 2 && be_iscomptr(vm, 1) && be_isint(vm, 2))
	{
		softI2C_t* i2c = be_tocomptr(vm, 1);
		int size = be_toint(vm, 2);
		uint8_t* data = malloc(size * sizeof(uint8_t));
		Soft_I2C_ReadBytes(i2c, data, size);
		be_newobject(vm, "list");
		for(int i = 0; i < size; i++)
		{
			be_pushint(vm, data[i]);
			be_data_push(vm, -2);
			be_pop(vm, 1);
		}
		be_pop(vm, 1);
		free(data);
		be_return(vm);
	}
	be_return_nil(vm);
}

#include "../../../libraries/berry/generate/be_fixed_be_class_Wire.h"

/* @const_object_info_begin

class be_class_Wire (scope: global, name: Wire) {
	.p, var
	init, func(m_initI2c)
	scan, func(m_scanI2c)
	start, func(m_startI2c)
	stop, func(m_stopI2c)
	read, func(m_readByteI2c)
	read_bytes, func(m_readBytesI2c)
	write, func(m_writeI2c)
}
@const_object_info_end */
