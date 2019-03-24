/*
 *****************************************************************************
 *
 * Copyright 2010, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *****************************************************************************
 */
/*
 *****************************************************************************
 * @file  si_apiCpi.c
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
 */

#include <linux/kernel.h>
#include <linux/types.h>

#include "si_apiCpi.h"
#include "si_apiCEC.h"
#include "si_cpi_regs.h"
#include "si_cec_enums.h"
#include "hdmi_i2c.h"
#include "hdmi_global.h"

//***************************************************************************
//!file     si_apiCpi.c
//!brief    CP 9387 Starter Kit CPI functions.
//
// No part of this work may be reproduced, modified, distributed,
// transmitted, transcribed, or translated into any language or computer
// format, in any form or by any means without written permission of
// Silicon Image, Inc., 1060 East Arques Avenue, Sunnyvale, California 94085
//
// Copyright 2002-2009, Silicon Image, Inc.  All rights reserved.
//***************************************************************************/
#define  HDMI_REG_BASE  (0x80400000)
/*
   unsigned int HW_REG_READ(unsigned int ByteAddr)
   {
   unsigned int offset = ByteAddr - HDMI_REG_BASE;

   return *((unsigned int *)hdmi_reg_addr + (offset>>2));
   }
   void HW_REG_WRITE(unsigned int ByteAddr, unsigned int Data)
   {
   unsigned int offset = ByteAddr - HDMI_REG_BASE;

 *((unsigned int *)hdmi_reg_addr + (offset>>2)) = Data;
 }

 unsigned char HDMI_I2C_SeqRead(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum)
 {
 unsigned int BaseAddr =0;
 unsigned int RegAddr  =0;
 unsigned char  i        =0;

 if(SlvAddr==0x39)
 {
 BaseAddr =HDMI_REG_BASE>>2;
 }else if(SlvAddr==0xff)
 {
 BaseAddr  =(HDMI_REG_BASE>>2)+0x400;
 }
 else
 {
 BaseAddr =(HDMI_REG_BASE>>2)+0x100;
 }

 for(i=0;i<ByteNum;++i)
 {
 RegAddr  =(BaseAddr+DevOffset+i)<<2;
 Data[i]  =HW_REG_READ(RegAddr) & 0xFF;
 printk("REG[%x] == %x ,DevOffset == %x \n",RegAddr,Data[i],DevOffset);
 }

 return ByteNum;
 }


 unsigned char HDMI_I2C_SeqWrite(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum)
 {
 unsigned int BaseAddr =0;
 unsigned int RegAddr  =0;
 unsigned char  i        =0;
//assert(Data!=NULL);
if(SlvAddr==0x39)
{
BaseAddr =HDMI_REG_BASE>>2;
}else if(SlvAddr==0xff)
{
BaseAddr  =(HDMI_REG_BASE>>2)+0x400;
}
else
{
BaseAddr =(HDMI_REG_BASE>>2)+0x100;
}

for(i=0;i<ByteNum;++i)
{
printk("Data[i]  == %x \n",Data[i]);
RegAddr  =(BaseAddr+DevOffset+i)<<2;
HW_REG_WRITE(RegAddr,Data[i]);
}
return ByteNum;
}

*/
//------------------------------------------------------------------------------
// Function:    SiIRegioReadBlock
// Description: Read a block of registers starting with the specified register.
//              The register address parameter is translated into an I2C
//              slave address and offset.
//              The block of data bytes is read from the I2C slave address
//              and offset.
//------------------------------------------------------------------------------

void SiIRegioReadBlock ( uint16_t regAddr, uint8_t *buffer, uint16_t length)
{
	// I2C_ReadBlock(SA_TX_CPI_Primary, (uint8_t)regAddr, buffer, length);
	//printk("buffer[0] == %x,buffer[1]  == %x ,regAddr== %x \n",buffer[0],buffer[1],regAddr);
	HDMI_I2C_SeqRead(0xff,(uint8_t)regAddr,buffer,length);
	//    HalI2cBus0ReadBlock( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr, buffer, length);
}
//------------------------------------------------------------------------------
// Function:    SiIRegioWriteBlock
// Description: Write a block of registers starting with the specified register.
//              The register address parameter is translated into an I2C slave
//              address and offset.
//              The block of data bytes is written to the I2C slave address
//              and offset.
//------------------------------------------------------------------------------

void SiIRegioWriteBlock ( uint16_t regAddr, uint8_t* buffer, uint16_t length)
{
	//I2C_WriteBlock(SA_TX_CPI_Primary, (uint8_t)regAddr, buffer, length);
	HDMI_I2C_SeqWrite(0xff,regAddr,buffer,length);
	//HalI2cBus0WriteBlock( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr, buffer, length);
}

//------------------------------------------------------------------------------
// Function:    SiIRegioRead
// Description: Read a one byte register.
//              The register address parameter is translated into an I2C slave
//              address and offset. The I2C slave address and offset are used
//              to perform an I2C read operation.
//------------------------------------------------------------------------------

uint8_t SiIRegioRead ( uint16_t regAddr )
{
	uint8_t buffer;
	HDMI_I2C_SeqRead(0xff,regAddr,&buffer,1);
	return buffer;
}

//------------------------------------------------------------------------------
// Function:    SiIRegioWrite
// Description: Write a one byte register.
//              The register address parameter is translated into an I2C
//              slave address and offset. The I2C slave address and offset
//              are used to perform an I2C write operation.
//------------------------------------------------------------------------------

void SiIRegioWrite ( uint16_t regAddr, uint8_t value )
{
	HDMI_I2C_SeqWrite(0xff,regAddr,&value,1);
}


//------------------------------------------------------------------------------
// Function:    SiIRegioModify
// Description: Modify a one byte register under mask.
//              The register address parameter is translated into an I2C
//              slave address and offset. The I2C slave address and offset are
//              used to perform I2C read and write operations.
//
//              All bits specified in the mask are set in the register
//              according to the value specified.
//              A mask of 0x00 does not change any bits.
//              A mask of 0xFF is the same a writing a byte - all bits
//              are set to the value given.
//              When only some bits in the mask are set, only those bits are
//              changed to the values given.
//------------------------------------------------------------------------------

void SiIRegioModify ( uint16_t regAddr, uint8_t mask, uint8_t value)
{
	uint8_t abyte;

	HDMI_I2C_SeqRead(0xff,regAddr,&abyte,1);
	abyte &= (~mask);                                       //first clear all bits in mask
	abyte |= (mask & value);                                //then set bits from value
	HDMI_I2C_SeqWrite(0xff,regAddr,&abyte,1);
}

//------------------------------------------------------------------------------
// Function:    SI_CpiSetLogicalAddr
// Description: Configure the CPI subsystem to respond to a specific CEC
//              logical address.
//------------------------------------------------------------------------------

unsigned char SI_CpiSetLogicalAddr ( uint8_t logicalAddress )
{
	uint8_t capture_address[2];
	uint8_t capture_addr_sel = 0x01;

    if (!SI_CecGetManualSetCaptureIDFlag())
    {
    	capture_address[0] = 0;
    	capture_address[1] = 0;
    	if( logicalAddress < 8 )
    	{
    		capture_addr_sel = capture_addr_sel << logicalAddress;
    		capture_address[0] = capture_addr_sel;
    	}
    	else
    	{
    		capture_addr_sel   = capture_addr_sel << ( logicalAddress - 8 );
    		capture_address[1] = capture_addr_sel;
    	}

    	// Set Capture Address
    	SiIRegioWriteBlock(REG_CEC_CAPTURE_ID0, capture_address, 1 );
    	SiIRegioWriteBlock(REG_CEC_CAPTURE_ID1, &capture_address[1], 1 );
    }

	SiIRegioWrite( REG_CEC_TX_INIT, logicalAddress );
	//DEBUG_PRINT(MSG_STAT,("CEC: logicalAddress: 0x%x\n", (int)logicalAddress));

	return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiSetCaptureID
// Description: set the received commands that will be captured in the receive
//              FIFO and acknowledged.
//------------------------------------------------------------------------------

unsigned char SI_CpiSetCaptureID ( uint8_t captureID[2] )
{
	// Set Capture Address
	SiIRegioWriteBlock(REG_CEC_CAPTURE_ID0, &captureID[0], 1 );
	SiIRegioWriteBlock(REG_CEC_CAPTURE_ID1, &captureID[1], 1 );

	return( true );
}


//------------------------------------------------------------------------------
// Function:    SI_CpiSendPing
// Description: Initiate sending a ping, and used for checking available
//                       CEC devices
//------------------------------------------------------------------------------

void SI_CpiSendPing ( uint8_t bCECLogAddr )
{
	SiIRegioWrite( REG_CEC_TX_DEST, BIT_SEND_POLL | bCECLogAddr );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiWrite
// Description: Send CEC command via CPI register set
//------------------------------------------------------------------------------

unsigned char SI_CpiWrite( SI_CpiData_t *pCpi )
{
	uint8_t cec_int_status_reg[2];
	uint8_t i;

	SiIRegioModify( REG_CEC_DEBUG_3, BIT_FLUSH_TX_FIFO, BIT_FLUSH_TX_FIFO );  // Clear Tx Buffer REG_CEC_DEBUG_3 where ????

	/* Clear Tx-related buffers; write 1 to bits to be cleared. */
	cec_int_status_reg[0] = 0x64 ; // Clear Tx Transmit Buffer Full Bit, Tx msg Sent Event Bit, and Tx FIFO Empty Event Bit
	cec_int_status_reg[1] = 0x02 ; // Clear Tx Frame Retranmit Count Exceeded Bit.
	SiIRegioWriteBlock( REG_CEC_INT_STATUS_0, cec_int_status_reg, 1 );
	SiIRegioWriteBlock( REG_CEC_INT_STATUS_1, &cec_int_status_reg[1], 1 );

	/* Send the command */
	SiIRegioWrite( REG_CEC_TX_DEST, pCpi->srcDestAddr & 0x0F );           // Destination
	SiIRegioWrite( REG_CEC_TX_COMMAND, pCpi->opcode );

	for(i=0; i<pCpi->argCount; i++)
	{
		SiIRegioWriteBlock( REG_CEC_TX_OPERAND(i), &pCpi->args[i], 1 );
	}
	SiIRegioWrite( REG_CEC_TRANSMIT_DATA, BIT_TRANSMIT_CMD | pCpi->argCount );

	return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiRead
// Description: Reads a CEC message from the CPI read FIFO, if present.
//------------------------------------------------------------------------------

unsigned char SI_CpiRead( SI_CpiData_t *pCpi )
{
	unsigned char    error = false;
	uint8_t argCount;

	argCount = SiIRegioRead( REG_CEC_RX_COUNT );

	if ( argCount & BIT_MSG_ERROR )
	{
		error = true;
	}
	else
	{
		pCpi->argCount = argCount & 0x0F;
		pCpi->srcDestAddr = SiIRegioRead( REG_CEC_RX_CMD_HEADER );
		pCpi->opcode = SiIRegioRead( REG_CEC_RX_OPCODE );
		if ( pCpi->argCount )
		{
			SiIRegioReadBlock( REG_CEC_RX_OPERAND_0, pCpi->args, pCpi->argCount );
		}
	}

	// Clear CLR_RX_FIFO_CUR;
	// Clear current frame from Rx FIFO

	SiIRegioModify( REG_CEC_RX_CONTROL, BIT_CLR_RX_FIFO_CUR, BIT_CLR_RX_FIFO_CUR );

	return( error );
}

void enable_cec_interrupts(void)
{
	uint8_t a = 0xff;
	SiIRegioWriteBlock(REG_CEC_INT_ENABLE_0, &a, 1);
}

//------------------------------------------------------------------------------
// Function:    SI_CpiStatus
// Description: Check CPI registers for a CEC event
//------------------------------------------------------------------------------

unsigned char SI_CpiStatus( SI_CpiStatus_t *pStatus )
{
	uint8_t cecStatus[2];
	cecStatus[0]=0;
	cecStatus[1]=0;

	pStatus->txState    = 0;
	pStatus->cecError   = 0;
	pStatus->rxState    = 0;

	SiIRegioReadBlock( REG_CEC_INT_STATUS_0, cecStatus, 1);
	SiIRegioReadBlock( REG_CEC_INT_STATUS_1, &cecStatus[1], 1);

	if ( (cecStatus[0] & 0x7F) || cecStatus[1] )
	{
		// DEBUG_PRINT(MSG_STAT,("\nCEC Status: %02X %02X\n", (int) cecStatus[0], (int) cecStatus[1]));

		// Clear interrupts

		if ( cecStatus[1] & BIT_FRAME_RETRANSM_OV )
		{
			//DEBUG_PRINT(MSG_DBG,("\n!TX retry count exceeded! [%02X][%02X]\n",(int) cecStatus[0], (int) cecStatus[1]));

			/* Flush TX, otherwise after clearing the BIT_FRAME_RETRANSM_OV */
			/* interrupt, the TX command will be re-sent.                   */
			SiIRegioModify( REG_CEC_DEBUG_3,BIT_FLUSH_TX_FIFO, BIT_FLUSH_TX_FIFO );//where REG_CEC_DEBUG_3???
		}

		// Clear set bits that are set
		SiIRegioWriteBlock( REG_CEC_INT_STATUS_0, cecStatus, 1 );
		SiIRegioWriteBlock( REG_CEC_INT_STATUS_1, &cecStatus[1], 1 );

		// RX Processing

		if ( cecStatus[0] & BIT_RX_MSG_RECEIVED )
		{
			pStatus->rxState = 1;   // Flag caller that CEC frames are present in RX FIFO
		}

		// RX Errors processing

		if ( cecStatus[1] & BIT_SHORT_PULSE_DET )
		{
			pStatus->cecError |= SI_CEC_SHORTPULSE;
		}

		if ( cecStatus[1] & BIT_START_IRREGULAR )
		{
			pStatus->cecError |= SI_CEC_BADSTART;
		}

		if ( cecStatus[1] & BIT_RX_FIFO_OVERRUN )
		{
			pStatus->cecError |= SI_CEC_RXOVERFLOW;
		}

		// TX Processing

		if ( cecStatus[0] & BIT_TX_FIFO_EMPTY )
		{
			pStatus->txState = SI_TX_WAITCMD;
		}
		if ( cecStatus[0] & BIT_TX_MESSAGE_SENT )
		{
			pStatus->txState = SI_TX_SENDACKED;
		}
		if ( cecStatus[1] & BIT_FRAME_RETRANSM_OV )
		{
			pStatus->txState = SI_TX_SENDFAILED;
		}
	}

	return( true );
}

//------------------------------------------------------------------------------
// Function:    SI_CpiGetLogicalAddr
// Description: Get Logical Address
//------------------------------------------------------------------------------

uint8_t SI_CpiGetLogicalAddr( void )
{
	char val;
	val = SiIRegioRead( REG_CEC_TX_INIT );
	//printk("SiIRegioRead(REG_CEC_TX_INIT) == %x \n",val);
	return val;
}

//------------------------------------------------------------------------------
// Function:    SI_CpiInit
// Description: Initialize the CPI subsystem for communicating via CEC
//------------------------------------------------------------------------------

unsigned char SI_CpiInit( void )//num1
{
    DEBUG_PRINTK("SI_CpiInit\n");
#ifdef DEV_SUPPORT_CEC_FEATURE_ABORT
	// Turn on CEC auto response to <Abort> command.
    DEBUG_PRINTK("SI_CpiInit: Writing CEC_OP_ABORT register\n");
	SiIRegioWrite( CEC_OP_ABORT_31, BIT7 );
#else
	// Turn off CEC auto response to <Abort> command.
    DEBUG_PRINTK("SI_CpiInit: Writing CEC_OP_ABORT_31 register\n");
	SiIRegioWrite( CEC_OP_ABORT_31, CLEAR_BITS );
#endif

#ifdef DEV_SUPPORT_CEC_CONFIG_CPI_0
	// Bit 4 of the CC Config reister must be cleared to enable CEC
    DEBUG_PRINTK("SI_CpiInit: Writing REG_CEC_CONFIG_CPI register\n");
	SiIRegioModify (REG_CEC_CONFIG_CPI, 0x10, 0x00);
#endif

	/*
	 * modified by xp, 20111101, give a CEC_LOGADDR_FREEUSE LA by default,
	 * and CecTaskEnumerate will get us a proper LA
	 */
    DEBUG_PRINTK("SI_CpiInit: Setting Logical Address\n");
	if ( !SI_CpiSetLogicalAddr( CEC_LOGADDR_FREEUSE ))
	{
		return( false );
	}

	return( true );
}

