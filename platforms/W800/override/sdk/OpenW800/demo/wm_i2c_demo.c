/**************************************************************************//**
 * @file     wm_i2c_demo.c
 * @version  
 * @date 
 * @author    
 * @note
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd. All rights reserved.
 *****************************************************************************/
#include "wm_include.h"
#include "wm_i2c.h"
#include <string.h>
#include "wm_gpio_afsel.h"

#if DEMO_I2C

#define I2C_FREQ		(200000)

/**
 * @brief	read one byte from the specified address of the eeprom
 * @param addr the eeprom address will be read from
 * @retval     the read data
 */
u8 AT24CXX_ReadOneByte(u16 addr)
{				  
	u8 temp=0;		  	    																 
	//printf("\nread addr=%x\n",ReadAddr);
	tls_i2c_write_byte(0xA0,1);   
	tls_i2c_wait_ack(); 
    	tls_i2c_write_byte(addr,0);   
	tls_i2c_wait_ack();	    

	tls_i2c_write_byte(0xA1,1);
	tls_i2c_wait_ack();	 
	temp=tls_i2c_read_byte(0,1);
	//printf("\nread byte=%x\n",temp);
	return temp;
}

/**
 * @brief	read multibytes from the specified address of the eeprom
 * @param[in] addr the eeprom address will be read from
 * @param[in] buf 	Pointer to data buffer
 * @param[in] len	amount of data to be read
 * @retval    null
 */
void AT24CXX_ReadLenByte(u16 addr,u8 *buf,u16 len)
{				  
	//printf("\nread len addr=%x\n",ReadAddr);
	tls_i2c_write_byte(0xA0,1);   
	tls_i2c_wait_ack(); 
    	tls_i2c_write_byte(addr,0);   
	tls_i2c_wait_ack();	    
	tls_i2c_write_byte(0xA1,1);
	tls_i2c_wait_ack();	
	while(len > 1)
	{
		*buf++ = tls_i2c_read_byte(1,0);
		//printf("\nread byte=%x\n",*(pBuffer - 1));
		len --;
	}
   	*buf = tls_i2c_read_byte(0,1);
}

/**
 * @brief	write one byte to the specified address of the eeprom
 * @param addr the eeprom address will be write to
 * @retval     null
 */
void AT24CXX_WriteOneByte(u16 addr, u8 data)
{				   	  	    																 
	tls_i2c_write_byte(0XA0, 1); 
	tls_i2c_wait_ack();	   
	tls_i2c_write_byte(addr, 0);
	tls_i2c_wait_ack(); 	 										  		   
	tls_i2c_write_byte(data, 0); 				   
	tls_i2c_wait_ack();  	   
 	tls_i2c_stop();
	tls_os_time_delay(1);
}

/**
 * @brief	check the eeprom is normal or not
 * @retval 
 *     0---success
 *     1---failed
 * @note 
 *	different 24Cxx chip will use the different addr
 */
u8 AT24CXX_Check(void)
{
	u8 temp;
	temp=AT24CXX_ReadOneByte(255);
	if (temp==0x55)return 0;		   
	else
	{
		AT24CXX_WriteOneByte(255, 0x55);
		tls_os_time_delay(1);
		temp=AT24CXX_ReadOneByte(255);	  
		if (temp==0x55)return 0;
	}

	return 1;											  
}

/**
 * @brief	read multibytes from the specified address of the eeprom
 * @param[in] addr the eeprom address will be read from
 * @param[in] buf 	Pointer to data buffer
 * @param[in] len	amount of data to be read
 * @retval    null
 */
void AT24CXX_Read(u16 addr, u8 *buf, u16 len)
{
	while(len)
	{
		*buf++=AT24CXX_ReadOneByte(addr++);	
		len--;
	}
}  

/**
 * @brief	write multibytes from the specified address of the eeprom
 * @param[in] addr the eeprom address will be read from
 * @param[in] buf 	Pointer to data buffer
 * @param[in] len	amount of data to be write
 * @retval    null
 */
void AT24CXX_Write(u16 addr, u8 *buf, u16 len)
{
	while(len--)
	{
		AT24CXX_WriteOneByte(addr,*buf);
		addr++;
		buf++;
	}
} 

int i2c_demo(char *buf)
{
	u8 testbuf[] = {"AT24CXX I2C TEST OK"};
	u8 datatmp[32];

    wm_i2c_scl_config(WM_IO_PA_01);
    wm_i2c_sda_config(WM_IO_PA_04);
    
	tls_i2c_init(I2C_FREQ);

	while(AT24CXX_Check())
	{
		printf("\nAT24CXX check faild\n");
	}
	tls_os_time_delay(1);
	printf("\nAT24CXX check success\n");

	AT24CXX_Write(0,(u8 *)testbuf,sizeof(testbuf));
	tls_os_time_delay(1);
	memset(datatmp,0,sizeof(datatmp));
	//AT24CXX_Read(0,datatmp,sizeof(testbuf));
	AT24CXX_ReadLenByte(0,(u8 *)datatmp,sizeof(testbuf));
	printf("\nread data is:%s\n",datatmp);
	
	return WM_SUCCESS;
}

#endif

/*** (C) COPYRIGHT 2014 Winner Microelectronics Co., Ltd. ***/
