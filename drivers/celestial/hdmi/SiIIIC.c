#include <linux/kernel.h>
#include <linux/string.h>

#include "SiIIIC.h"
#include "hdmi_i2c.h"

BYTE hlReadByte_8BA ( BYTE SlaveAddr, BYTE RegAddr )
{
	char Data = 0x0000;

	hdmi_i2c_read_byte(SlaveAddr, (int)RegAddr, &Data);

	//DEBUG_PRINTK("read  SlaveAddr = %x; RegAddr = %x; Data=%x\n",  SlaveAddr, RegAddr,Data);
	return ( BYTE )Data;
}

WORD hlReadWord_8BA( BYTE SlaveAddr, BYTE RegAddr )
{
	char Buffer[2] ={0, 0};
	WORD Data = 0;
	
	hdmi_i2c_read(SlaveAddr, (int)RegAddr, Buffer, 2);
	Data = Buffer[1];
	Data = (Data<<8 ) | Buffer[0];
	return Data;
}

BYTE hlWriteByte_8BA ( BYTE SlaveAddr, BYTE RegAddr, BYTE Data ) 
{
	hdmi_i2c_write_byte(SlaveAddr,  (unsigned int)RegAddr, Data);
	return 0;
}

void hlWriteWord_8BA( BYTE SlaveAddr, BYTE RegAddr, WORD Data )
{
	unsigned char buffer[2] = {0, 0};
	buffer[0] = Data & 0xff;
	buffer[1] = Data >> 8;
	hdmi_i2c_write(SlaveAddr, (unsigned int )RegAddr, buffer, 2);
}

//------------------------------------------------------------------------------
// Function Name: siiReadBlockHDMI
// Function Description: Reads block of data from HDMI RX (page 1)
//------------------------------------------------------------------------------
void siiReadBlockHDMI( BYTE bSlaveAddr, BYTE bAddr, BYTE bNBytes, BYTE * pbData )
{
	char buffer[256];	//max block reg number is 256;
	int i = 0;
	memset(buffer, 0, sizeof(buffer));
	hdmi_i2c_read(bSlaveAddr, (int)bAddr, buffer,  (unsigned int)bNBytes);
	for (i = 0; i <bNBytes  ; i++)
	{
		pbData[i] = buffer[i];
	}
}

//------------------------------------------------------------------------------
// Function Name:  siiWriteBlockHDMI
// Function Description: Writes block of data from HDMI Device
//------------------------------------------------------------------------------
void siiWriteBlockHDMI( BYTE bSlaveAddr, BYTE bRegAddr, BYTE bNBytes, BYTE * pbData )
{
	unsigned char buffer[256];	//max block reg number is 256;
	int i = 0;
	memset(buffer, 0, sizeof(buffer));
	for (i = 0; i <bNBytes  ; i++)
	{
		buffer[i] = pbData[i];
	}
	hdmi_i2c_write(bSlaveAddr, (unsigned int)bRegAddr, buffer,  (unsigned int)bNBytes);
}

//------------------------------------------------------------------------------
// Function Name: siiModifyBits
// Function Description:  this function reads byte from i2c device, modifys bit
//                                 in the byte, then writes it back
//------------------------------------------------------------------------------
void siiModifyBits( BYTE bSlaveAddr, BYTE bRegAddr, BYTE bModVal, BOOL qSet )
{
	BYTE Data = 0x0000;
	hdmi_i2c_read_byte(bSlaveAddr,  (unsigned int)bRegAddr, (char *)&Data);
	if ( qSet )
	{
		Data |= bModVal;
	}
	else
	{
		Data &= (~bModVal);
	}
	hdmi_i2c_write_byte(bSlaveAddr,  (unsigned int)bRegAddr, Data);
}

int hdmi_i2c_read(unsigned char address, int subaddr, char *buffer, unsigned int num)
{
	HDMI_I2C_SeqRead(address,subaddr, (unsigned char *)buffer,num);
	return 0;
}


int hdmi_i2c_read_byte(unsigned char address, int subaddr, char *buffer)
{
	HDMI_I2C_SeqRead(address,subaddr, (unsigned char *)buffer,1);
	return 0;
}

int hdmi_i2c_write(unsigned char address, unsigned int subaddr, unsigned char *buffer, unsigned int num)
{
	HDMI_I2C_SeqWrite(address,subaddr, buffer,num); 
	return 0;
}


int hdmi_i2c_write_byte(unsigned char address, unsigned int subaddr, unsigned char byte)
{
	HDMI_I2C_SeqWrite(address,subaddr, &byte,1); 
	return 0;
}

