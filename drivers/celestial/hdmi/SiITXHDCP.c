//------------------------------------------------------------------------------
// Module Name: SiITXHDCP.c
// Module Description: SiITX HDCP
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/jiffies.h>

#include "TypeDefs.h"
#include "SiIIIC.h"
#include "SiITX.h"
#include "SiITXDefs.h"
#include "SiITXHDCP.h"
#include "SiITXInfoPkts.h"

TmpDType TmpD;

extern BYTE EDID_ERROR;
BYTE DownStreamAuthState = NO_HDCP;
BYTE MX_TX[8];

#ifdef REPEATER_SUPPORT
#include <stdlib.h>
#include <string.h>

#define HDCP1X_RETRY_TIMEOUT      2000
int hdcp1xInitiatedFromSource = 0;
BYTE BKSV_DATA[HDCP1X_KSV_SIZE]={0};
BYTE KSV_BUF[MAX_HDCP1X_KSV * HDCP1X_KSV_SIZE]={0};
#endif

#define GetAvailableBytesInFIFO()   hlReadByte_8BA( TX_SLV0, MDDC_FIFO_CNT_ADDR )

//------------------------------------------------------------------------------
void siiTXSetEncryption( BYTE OnOff ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, HDCP_CTRL_ADDR );
	if( OnOff )
		hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal | BIT_ENC_EN);
	else
		hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal & ~BIT_ENC_EN);
}

//------------------------------------------------------------------------------
static void ConvertMDDCStateIntoErrCode( BYTE * pErrSatus ){
	// process error conditions behind device (Master DDC)
	if (*pErrSatus) {
		if( *pErrSatus & BIT_MDDC_ST_I2C_LOW)
			*pErrSatus = _MDDC_CAPTURED;
		else if( *pErrSatus & BIT_MDDC_ST_NO_ACK)
			*pErrSatus = _MDDC_NOACK;
		else if( *pErrSatus & MASTER_FIFO_FULL)
			*pErrSatus = _MDDC_FIFO_FULL;
		else
			*pErrSatus = IIC_OK;
	}
}

//------------------------------------------------------------------------------
void BlockRead_MDDC( MDDCType * MDDCCmd, BYTE * pData )
{
	BYTE FIFO_Size, TimeOutCount, Status;
	EDID_ERROR = NO_EDID_ERROR;
	MDDCCommand(MASTER_CMD_CLEAR_FIFO);     // Abort Master DCC operation and Clear FIFO pointer
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 1, 5, (BYTE *)MDDCCmd); // sending header, it'll clear DDC Status register too
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 7, 1, &MDDCCmd->Cmd);

	//printk("TX_SLV0=%x,MASTERBASE=%x\n",TX_SLV0,MASTER_BASE);
	//for(i=0;i<=7;++i)
	// printk("MDDCCmd[%d]=%x\n",i,MDDCCmd[i]);
	//printk("Cmd=%x\n",MDDCCmd->Cmd);
	TimeOutCount = MDDCCmd->NBytesLSB + 3; // time_out is proportional to length
	// wait until the FIFO is filled with several bytes
	//halDelayMS(5); // also makes time alinging
	//hdmiudelay(5000);
	DEBUG_PRINTK("MDDC Read Start!\n");
	do
	{
		FIFO_Size = GetAvailableBytesInFIFO();
		if( FIFO_Size )
		{
			// if the FIFO has some bytes
			// read fifo_size bytes
			siiReadBlockHDMI( TX_SLV0, MDDC_FIFO_ADDR, 1, pData );
			MDDCCmd->NBytesLSB -= 1;
			pData += 1;
		}
		else
		{
			//halDelayMS(1); // note, the mime is aligned
			udelay(2000);
			TimeOutCount--;
		}
	}
	while( MDDCCmd->NBytesLSB && TimeOutCount );
	DEBUG_PRINTK("MDDC Read Finished!\n");
	if(!TimeOutCount)
	{
		DEBUG_PRINTK("###################DDC Time Out!################\n");
		EDID_ERROR = IS_EDID_ERROR;
		//hdmi_print_time();
	}
	Status = GetMDDCStatus();
	DEBUG_PRINTK("~~~~~~~~~~~~~~~~~~MDDC Read status=%x\n",Status);
	//hdmi_print_time();
	ConvertMDDCStateIntoErrCode( &Status );
	if( (!TimeOutCount)||Status )
	{
		if( Status == _MDDC_NOACK )
		{         // can happen if Rx is clock stretching the SCL line. DDC bus unusable
			siiTXSendCP_Packet( ON );               // mute audio and video
			siiTXSetEncryption( OFF );
		}
		else
		{
			MDDCCommand( MASTER_CMD_ABORT );
			MDDCCommand( MASTER_CMD_CLOCK );
			ClrManualCtrlReg();
		}
	}
}

//------------------------------------------------------------------------------
void BlockWrite_MDDC( MDDCType * MDDCCmd ){
	BYTE TimeOutCount, Status;
	int i;
	MDDCCommand(MASTER_CMD_CLEAR_FIFO);
	//tmp.Status = GetMDDCStatus(); //Oscar remove on 05/23/2008
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 1, 5, (BYTE *)MDDCCmd ); // sending header, it'll clear DDC Status register too
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 7, 1,  &MDDCCmd->Cmd);
	for(i=0;i<MDDCCmd->NBytesLSB;++i)
	{
		siiWriteBlockHDMI( TX_SLV0, MDDC_FIFO_ADDR, 1, (BYTE *)&MDDCCmd->PData[i] ); // put the data into FIFO
	}
	MDDCCommand(MASTER_CMD_SEQ_WR); // start the writting    //Oscar add on 05/23/2008

	TimeOutCount = MDDCCmd->NBytesLSB;

	do {
		udelay(2000);
		TimeOutCount-- ;
	}
	while( IsMDDCReadInProgress() && TimeOutCount); //wait data has been written in ready

	Status = GetMDDCStatus();
	//printk("~~~~~~~~~~~~~~~~~~MDDC Write status=%x\n",Status);
	//hdmi_print_time();
	ConvertMDDCStateIntoErrCode( &Status );
	if( Status == _MDDC_NOACK ){         // can happen if Rx is clock stretching the SCL line. DDC bus unusable
		siiTXSendCP_Packet( ON );               // mute audio and video
		siiTXSetEncryption( OFF );
		//DownStreamAuthState = REQ_AUTHENTICATION;     // force re-authentication
	}
	else{
		MDDCCommand( MASTER_CMD_ABORT );
		MDDCCommand( MASTER_CMD_CLOCK );
		ClrManualCtrlReg();
		udelay(2000);
	}
}

//------------------------------------------------------------------------------
void MDDCBlockReadHDCPRX( BYTE NBytes, BYTE Addr, BYTE * pData ){

	TmpD.MDDC.SlaveAddr = HDCPRX_SLV;
	TmpD.MDDC.Offset = 0;
	TmpD.MDDC.RegAddr = Addr;
	TmpD.MDDC.NBytesLSB = NBytes;
	TmpD.MDDC.NBytesMSB = 0;
	TmpD.MDDC.Dummy = 0;
	TmpD.MDDC.Cmd = MASTER_CMD_SEQ_RD;
	//TmpD.MDDC.PData = pData; //Oscar remove on 05/23/2008
	BlockRead_MDDC(&TmpD.MDDC, pData);
}

//------------------------------------------------------------------------------
void MDDCBlockWriteHDCPRX( BYTE NBytes, BYTE Addr, BYTE * pData ){

	TmpD.MDDC.SlaveAddr = HDCPRX_SLV;
	TmpD.MDDC.Offset = 0;
	TmpD.MDDC.RegAddr = Addr;
	TmpD.MDDC.NBytesLSB = NBytes;
	TmpD.MDDC.NBytesMSB = 0;
	TmpD.MDDC.Dummy = 0;
	//TmpD.MDDC.Cmd = MASTER_CMD_SEQ_WR;
	TmpD.MDDC.Cmd = MASTER_CMD_CLEAR_FIFO; //Oscar add on 05/23/2008
	TmpD.MDDC.PData = pData;
	BlockWrite_MDDC(&TmpD.MDDC);
}

//------------------------------------------------------------------------------
void MDDCInitReadBlockFromFIFO( BYTE Addr, WORD NBytes )
{
	MDDCCommand( MASTER_CMD_ABORT );
	MDDCCommand( MASTER_CMD_CLEAR_FIFO );
	TmpD.MDDC.SlaveAddr = HDCPRX_SLV;
	TmpD.MDDC.Offset = 0;
	TmpD.MDDC.RegAddr = Addr;
	TmpD.MDDC.NBytesLSB = (BYTE) (NBytes & 0xFF);
	TmpD.MDDC.NBytesMSB = (BYTE) (NBytes >> 8);
	TmpD.MDDC.Dummy = 0;
	TmpD.MDDC.Cmd = MASTER_CMD_SEQ_RD;
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 1, 5, (BYTE *)&TmpD.MDDC);
	siiWriteBlockHDMI( TX_SLV0, MASTER_BASE + 7, 1, &TmpD.MDDC.Cmd);
}

//------------------------------------------------------------------------------
void MDDCTakeBlockFromFIFO( BYTE NBytes, BYTE * Data ){
	BYTE TimeOutCount;
	int i;

	TimeOutCount = NBytes * 2;
	while((GetAvailableBytesInFIFO() < NBytes) && TimeOutCount--)
		udelay(1000);
	if( TimeOutCount )
	{
		for (i=0; i<NBytes; i++)
			siiReadBlockHDMI( TX_SLV0, MDDC_FIFO_ADDR, 1, Data+i);
	}
}

//------------------------------------------------------------------------------
BYTE CheckValidityHDCPDevice( BYTE * Data )
{
	BYTE i, j, Ones = 0, Mask = 0x01;
	BYTE Result;

	for( i = 0; i < 5; i++ ){
		Mask = 0x01;
		for( j = 0; j < 8; j++ ){
			if( Data[i] & Mask )
				Ones++;
			Mask <<= 1;
		}
	}
	if(Ones == 20)
		Result = TRUE;
	else
		Result = FALSE;

	return Result;
}

//------------------------------------------------------------------------------
void ReleaseCPReset( void ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, HDCP_CTRL_ADDR );
	hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal | BIT_CP_RESET_N );
}

//------------------------------------------------------------------------------
static BYTE IsRepeater( void ){
	BYTE RegVal;

	MDDCBlockReadHDCPRX(1, DDC_BCAPS_ADDR , &RegVal);
	return RegVal & DDC_BIT_REPEATER;
}

//------------------------------------------------------------------------------
static void RepeaterBitInTX( BYTE RXMode ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, HDCP_CTRL_ADDR );
	if (RXMode)
		hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal | BIT_RX_REPEATER );
	else
		hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal & (~BIT_RX_REPEATER) );
}

//------------------------------------------------------------------------------
static void GenerateAN( void ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, HDCP_CTRL_ADDR);
	hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal & (~ BIT_AN_STOP)); // Start AN Gen
	//halDelayMS(10);
	mdelay(1000); //duliguo delete for speed up simulation
	hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal | BIT_AN_STOP); // Stop AN Gen
	//hlWriteByte_8BA( TX_SLV0, HDCP_CTRL_ADDR, RegVal | BIT_AN_STOP); // Stop AN Gen(need to write 2 times refer to PR doc)
}

//------------------------------------------------------------------------------
static BYTE IsBKSVError( void ){
	return hlReadByte_8BA( TX_SLV0, HDCP_CTRL_ADDR) & BIT_BKSV_ERROR;
}

//------------------------------------------------------------------------------
void AreR0sMatch( BYTE * Match ){
	WORD R0RX, R0TX;

	MDDCBlockReadHDCPRX( 2, DDC_Ri_ADDR, (BYTE *)&R0RX );
	siiReadBlockHDMI( TX_SLV0, Ri_ADDR, 2, (BYTE *)&R0TX );
	if(R0RX == R0TX)
		*Match = TRUE;
	else
		*Match = FALSE;
	DEBUG_PRINTK("----------------HDCP R0RX is 0x%04x,R0TX is 0x%04x-------------\n",R0RX,R0TX);

}

//------------------------------------------------------------------------------
static void MakeCopyM0( void ){
	int i=0;
	hlWriteByte_8BA( TX_SLV0, SHA_CONTROL_ADDR, ( hlReadByte_8BA( TX_SLV0, SHA_CONTROL_ADDR ) | BIT_M0_READ_EN ) );
	siiReadBlockHDMI( TX_SLV0, AN_ADDR, 8, MX_TX );
#if 1
	for(i=0;i<=7;++i)
	{
		DEBUG_PRINTK("HDCP TX M[%d]=%x\n",i,MX_TX[i]);
	}
#endif
	hlWriteByte_8BA( TX_SLV0, SHA_CONTROL_ADDR, ( hlReadByte_8BA( TX_SLV0, SHA_CONTROL_ADDR ) | ~BIT_M0_READ_EN ) ); //Oscar
}

static  BYTE IsRiReady(void)
{
	return hlReadByte_8BA(TX_SLV0,HDCP_CTRL_ADDR)& BIT_RiREADY;
}
//------------------------------------------------------------------------------
void RiCheckInterruptMask( BYTE Enabled ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, HDMI_INT1_MASK );
	if( Enabled )
		hlWriteByte_8BA( TX_SLV0, HDMI_INT1_MASK, RegVal | BIT_INT_Ri_CHECK );
	else
		hlWriteByte_8BA( TX_SLV0, HDMI_INT1_MASK, RegVal &(~ BIT_INT_Ri_CHECK) );
}

//#define SII_DUMP_UART
void printAuthState( void )
{
#ifdef SII_DUMP_UART
	printk("{DownStreamAuthState}: ");
	switch( DownStreamAuthState )
	{
		case NO_HDCP:
			printk("No HDCP");
			break;
		case NO_DEVICE_WITH_HDCP_SLAVE_ADDR:
			printk("No HDCP Dev with HDCP Slave Addr");
			break;
		case NO_ACK_FROM_HDCP_DEV:
			printk("No ACK from HDCP dev");
			break;
		case BKSV_ERROR:
			printk("BKSV Err");
			break;
		case R0s_ARE_MISSMATCH:
			printk("R0 Err");
			break;
		case RIs_ARE_MISSMATCH:
			printk("Ri Err");
			break;
		case AUTHENTICATED:
			printk("Authenticated");
			break;
		case REPEATER_AUTH_REQ:
			printk("Rptr Auth. Req");
			break;
		case REQ_SHA_CALC:
			printk("Req SHA calc");
			break;
		case REQ_SHA_HW_CALC:
			printk("Req SHA HW calc");
			break;
		case FAILED_ViERROR:
			printk("V Err");
			break;
		case REAUTHENTATION_REQ:
			printk("Reauthentication Req");
			break;
		case REQ_AUTHENTICATION:
			printk("Req authentication");
			break;
		default:
			printk("Unknown %02i", DownStreamAuthState);
			break;
	}
	printk("\n");
#else
	if (DownStreamAuthState == AUTHENTICATED)
		printk("HDCP Authenticated\n");
	return;
#endif
}

BYTE siiTXHDCP_CheckHPDChanged( void )
{
	BYTE INT_HPD;

	INT_HPD = hlReadByte_8BA( TX_SLV0, HDMI_INT1_ADDR );
	if ((INT_HPD >> 6) & 1) // HPD Changed
	{
		if (!(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN))   // HPD deasserted
		{
			// clear irq
			//INT_HPD &= 1<<6;
			//hlWriteByte_8BA(TX_SLV0, HDMI_INT1_ADDR, INT_HPD);
			return NO_DEVICE_WITH_HDCP_SLAVE_ADDR;
		}
		else    // HPD asserted
		{
			// clear irq
			//INT_HPD &= 1<<6;
			//hlWriteByte_8BA(TX_SLV0, HDMI_INT1_ADDR, INT_HPD);
			return REAUTHENTATION_REQ;
		}
	}

	return HPD_NOT_CHANGED;
}

//static struct timeval tm_start_part2;
//static struct timeval tm_now_part2;
unsigned long tm_start_part2, tm_now_part2;
//------------------------------------------------------------------------------
static BYTE siiTXHDCP_Authentication( void )
{
	int i = 0;
	int n = 0;
	BYTE AN_KSV_Data[8];
	BYTE R0sMatch;
	BYTE HPDStatus;

	DEBUG_PRINTK("HDCP Authentication Begin...\n");
	// Check BKSV validity (20 bits "1" and 20 bits "0")

	n = 20;
	while(n--)
	{
		HPDStatus = siiTXHDCP_CheckHPDChanged();
		if (HPDStatus != HPD_NOT_CHANGED)
		{
			printk("----------------Return : HPD Changed -------------------------\n");
			return HPDStatus;
		}
		udelay(1000);
	}

	MDDCBlockReadHDCPRX( 5, DDC_BKSV_ADDR , AN_KSV_Data ); // Read BKSV from RX
	if( !CheckValidityHDCPDevice(AN_KSV_Data) )    // Check BKSV validity
		return NO_DEVICE_WITH_HDCP_SLAVE_ADDR;  // No 20 ones and zeros

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	DEBUG_PRINTK("----------------HDCP Release CP and Generate AN----------------\n");
	ReleaseCPReset();

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	// Set TX repeater bit if downstream device is a repeater
	if (IsRepeater())
	{
		RepeaterBitInTX( ON );
	}
	else
	{
		RepeaterBitInTX( OFF );
	}

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	// Generate 8 bytes random data and write them to RX
	GenerateAN();
	siiReadBlockHDMI( TX_SLV0, AN_ADDR, 8, AN_KSV_Data );     // Read AN

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	//test only
#if 0
	AN_KSV_Data[0] =0x03;
	AN_KSV_Data[1] =0x04;
	AN_KSV_Data[2] =0x07;
	AN_KSV_Data[3] =0x0c;
	AN_KSV_Data[4] =0x13;
	AN_KSV_Data[5] =0x1c;
	AN_KSV_Data[6] =0x27;
	AN_KSV_Data[7] =0x34;

	siiWriteBlockHDMI( TX_SLV0, AN_ADDR, 8, AN_KSV_Data );
	siiReadBlockHDMI( TX_SLV0, AN_ADDR, 8, AN_KSV_Data );
#endif
#if 1
	for(i=0;i<=7;++i)
	{
		DEBUG_PRINTK("HDCP TX AN[%d]=%x\n",i,AN_KSV_Data[i]);
	}
#endif
	DEBUG_PRINTK("----------------HDCP Write AN to RX----------------------------\n");
	MDDCBlockWriteHDCPRX( 8, DDC_AN_ADDR, AN_KSV_Data ); // Write AN to RX

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	// Exchange KSVs values
	siiReadBlockHDMI( TX_SLV0, AKSV_ADDR, 5, AN_KSV_Data );     // Read AKSV from TX
#if 1
	for(i=0;i<=4;++i)
	{
		DEBUG_PRINTK("HDCP TX AKSV[%d]=%x\n",i,AN_KSV_Data[i]);
	}
#endif

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	DEBUG_PRINTK("----------------HDCP Write AKSV to RX--------------------------\n");
	MDDCBlockWriteHDCPRX( 5, DDC_AKSV_ADDR, AN_KSV_Data ); // Write AKSV into RX

	n = 150;
	while(n--)
	{
		HPDStatus = siiTXHDCP_CheckHPDChanged();
		if (HPDStatus != HPD_NOT_CHANGED)
		{
			printk("----------------Return : HPD Changed -------------------------\n");
			return HPDStatus;
		}
		udelay(1000);
	}

	DEBUG_PRINTK("----------------HDCP Read BKSV From RX-------------------------\n");
	MDDCBlockReadHDCPRX( 5, DDC_BKSV_ADDR , AN_KSV_Data ); // Read BKSV from RX
#ifdef REPEATER_SUPPORT
	memcpy(BKSV_DATA, AN_KSV_Data, HDCP1X_KSV_SIZE);
#endif
#if 1
	for(i=0;i<=4;++i)
	{
		DEBUG_PRINTK("HDCP RX BKSV[%d]=%x\n",i, AN_KSV_Data[i]);
	}
#endif
	if( !CheckValidityHDCPDevice(AN_KSV_Data) )    // Check BKSV validity again for fixing HDMI-ATC UITA2000 reset BKSV issue
		return NO_DEVICE_WITH_HDCP_SLAVE_ADDR;  // No 20 ones and zeros

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	siiWriteBlockHDMI( TX_SLV0, BKSV_ADDR, 5, AN_KSV_Data ); // Write BKSV into TX / This writting also triggers R0 calculation
	if( IsBKSVError() ){
		return BKSV_ERROR; // If BKSV_ERROR is set, and TX will perform an authentication twice with a valid BKSV value.
	}

	// halDelayMS(100); // Delay for R0 calculation at least 100ms for HDCP spec.
	while(!IsRiReady())
	{
		DEBUG_PRINTK("HDCP wait Ri Ready\n");
		udelay(1000);
	}
	AreR0sMatch( &R0sMatch );
	if( !R0sMatch )
		return R0s_ARE_MISSMATCH;

	n = 20;
	while(n--)
	{
		HPDStatus = siiTXHDCP_CheckHPDChanged();
		if (HPDStatus != HPD_NOT_CHANGED)
		{
			printk("----------------Return : HPD Changed -------------------------\n");
			return HPDStatus;
		}
		udelay(1000);
	}

	MakeCopyM0(); //Copy An as M0 for SHA.

	HPDStatus = siiTXHDCP_CheckHPDChanged();
	if (HPDStatus != HPD_NOT_CHANGED)
		return HPDStatus;

	siiTXSetEncryption( ON );

	if (IsRepeater())   // here need to read Bcaps another time, because 1B-03 test start to time the WD here.
	{
		if (AssertedDSAuthPart2 == 0)
		{
			//gettimeofday(&tm_start_part2, NULL);
			tm_start_part2 = jiffies;

			// Initialize a 5s watch dog
			AssertedDSAuthPart2 = 1;   // HDCP authentication part 2(SHA) timeout is 5 sec according to the HDCP spec.
		}
		return REPEATER_AUTH_REQ;
	}
	else
	{
		return  AUTHENTICATED;
	}
}

int siiTXHDCP_GetAuthenticationPart2WDTime(void)
{
#if 0
	gettimeofday(&tm_now_part2, NULL);
	TimeOutForDSAuthPart2 = ((tm_now_part2.tv_sec - tm_start_part2.tv_sec) * 1000 + (tm_now_part2.tv_usec - tm_start_part2.tv_usec) / 1000);
#endif
	tm_now_part2 = jiffies;
	TimeOutForDSAuthPart2 = (tm_now_part2 - tm_start_part2) * 1000 / HZ;

	return TimeOutForDSAuthPart2;
}

//------------------------------------------------------------------------------
void siiTXHDCP_AuthenticationPart1( void )
{
	BYTE Mode;

	if( IsTXInHDMIMode() )
		Mode = HDMI_Mode;
	else
	{
		Mode = DVI_Mode;
		//printk("[SiITXHDCP.C](siiTXHDCP_Authentication1): Call siiTX_DVI_Init(). #1\n");
		//siiTX_DVI_Init();
	}

	siiTXSendCP_Packet( ON );
	siiTXSetEncryption( OFF );     //Must turn encryption off when AVMUTE


	if( Mode != HDMI_Mode )	 // Reinitialization in DVI mode, to make clear CP reset bit
	{
		//printk("[SiITXHDCP.C](siiTXHDCP_Authentication1): Call siiTX_DVI_Init(). #2\n");
		//siiTX_DVI_Init();
	}

	DownStreamAuthState = siiTXHDCP_Authentication();
	printAuthState();

	if( (DownStreamAuthState == AUTHENTICATED)
			||(DownStreamAuthState == REPEATER_AUTH_REQ) )
	{
		siiTXSendCP_Packet( OFF );
		RiCheckInterruptMask( ON );
	}
}

////////////////////////////////////////////////////////////////////////////////
/**************************  Transmitter Software SHA  ****************************/
/**************** All rights reserved contains SHA1 code adapted from *****************/
/****************        http://www.di-mgt.com.au/src/sha1.c.txt        *****************/
///////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
static BYTE GetFIFOSize( void )
{
	BYTE BStatusReg;
	MDDCBlockReadHDCPRX(1, DDC_BSTATUS1_ADDR, &BStatusReg);
	return BStatusReg & 0x7F;
}

static BYTE GetDeviceCountExceeded( void )
{
	BYTE BStatusReg;
	MDDCBlockReadHDCPRX(1, DDC_BSTATUS1_ADDR, &BStatusReg);
	return BStatusReg & 0x80;
}

static BYTE GetCascadeDepth( void )
{
	BYTE BStatusReg;
	MDDCBlockReadHDCPRX(1, DDC_BSTATUS2_ADDR, &BStatusReg);
	return BStatusReg & 0x07;
}

static BYTE GetCascadeDepthExceeded( void )
{
	BYTE BStatusReg;
	MDDCBlockReadHDCPRX(1, DDC_BSTATUS2_ADDR, &BStatusReg);
	return BStatusReg & 0x08;
}

//------------------------------------------------------------------------------
static void SHAInit( SHA_CTX  *shsInfo )
{
	// Set the h-vars to their initial values
	shsInfo->Digest[ 0 ] = h0init;
	shsInfo->Digest[ 1 ] = h1init;
	shsInfo->Digest[ 2 ] = h2init;
	shsInfo->Digest[ 3 ] = h3init;
	shsInfo->Digest[ 4 ] = h4init;

	// Initialise bit count
	shsInfo->byteCounter = 0;
	shsInfo->BStatusMXCounter = 0;
}

//------------------------------------------------------------------------------
static void LEndianBEndianConvert( BYTE * Number )
{
	BYTE TmpVal;

	TmpVal = Number[0];
	Number[0] = Number[3];
	Number[3] = TmpVal;

	TmpVal = Number[1];
	Number[1] = Number[2];
	Number[2] = TmpVal;
}

//------------------------------------------------------------------------------
static BYTE GetKSVList( SHA_CTX  * Sha, WORD i ){
	BYTE Aux = 0;
	BYTE BufStatus;

	BufStatus = 0;

	for(Aux = 0; Aux < 16; Aux++) // clear data buffer
		Sha->Data[Aux]=0;
	// Check how many bytes remain to read
	// total number = KSV list (up to  5 * 127 = 635) +
	//                BStatus (2 bytes) +
	//                MX (8 bytes)
	i = i - Sha->byteCounter;   // Get how many bytes KSV list remain to receive
	Aux = 0;

	while( Aux < 16 ){
		if( i==0 ){  // no bytes remain in
			BufStatus = KSV_FINISHED;
			break;
		}
		else{
			if( i > 4 ){
				MDDCTakeBlockFromFIFO( 4, (BYTE *)&Sha->Data[Aux] );
				/* Important to do the below copy before LE->BE conversion. */
				//memcpy(&KSV_BUF[Sha->byteCounter], (BYTE *)&Sha->Data[Aux], 4);
				LEndianBEndianConvert((BYTE *)&Sha->Data[Aux]);
				Sha->byteCounter+=4;
				i-=4;
				if( i==0 ){ // no bytes remain in
					BufStatus = KSV_FINISHED;
					break;
				}
			}
			else{
				MDDCTakeBlockFromFIFO( i, (BYTE *)&Sha->Data[Aux] );
				/* Important to do the below copy before LE->BE conversion. */
				//memcpy(&KSV_BUF[Sha->byteCounter], (BYTE *)&Sha->Data[Aux], i);
				LEndianBEndianConvert((BYTE *)&Sha->Data[Aux]);
				Sha->byteCounter+=i;
				BufStatus = KSV_FINISHED;
				break;
			}
		}
		Aux++;
	}

	return BufStatus;
}

//------------------------------------------------------------------------------
static void SHSTransform( SHA_CTX  *sha ){
	DWORD A, B, C, D, E, TEMP;     // Local vars
	BYTE i, s;

	A = sha->Digest[ 0 ];
	B = sha->Digest[ 1 ];
	C = sha->Digest[ 2 ];
	D = sha->Digest[ 3 ];
	E = sha->Digest[ 4 ];

	for( i = 0; i<80; i++ ){
		s = i & 15;
		if( i>=16 ){
			sha->Data[s]= sha->Data[(s+13)&15]^sha->Data[(s+8)&15]^sha->Data[(s+2)&15]^sha->Data[s];
			sha->Data[s]= (sha->Data[s]<<1)|(sha->Data[s]>>31);
		}
		TEMP = ((A<<5)|(A>>27)) + E + sha->Data[s];
		if( i<=19 ){
			TEMP+=K1;
			TEMP+=f1(B,C,D);
		}
		else if( i<=39 ){
			TEMP+=K2;
			TEMP+=f2(B,C,D);
		}
		else if( i<=59 ){
			TEMP+=K3;
			TEMP+=f3(B,C,D);
		}
		else{
			TEMP+=K4;
			TEMP+=f4(B,C,D);
		}
		E = D;
		D = C;
		C = (B<<30)|(B>>2);
		B = A;
		A = TEMP;
	}

	// Build message digest
	sha->Digest[ 0 ] += A;
	sha->Digest[ 1 ] += B;
	sha->Digest[ 2 ] += C;
	sha->Digest[ 3 ] += D;
	sha->Digest[ 4 ] += E;
}

//------------------------------------------------------------------------------
static BYTE GetByteBStatusMX( BYTE Addr ){
	BYTE Data[2];

	if( Addr <2 ){ // Read B Status
		MDDCBlockReadHDCPRX( 2, DDC_BSTATUS1_ADDR, Data );
		return Data[Addr];
	}
	else          //MX
		return MX_TX[Addr-2];
}

//------------------------------------------------------------------------------
static BYTE GetBStatusMX( SHA_CTX  * Sha )
{
	BYTE i, bytePos;

	if( (Sha->byteCounter&0x0003F)== 0 ) // New data frame
		for(i = 0; i < 16; i++)           // clear data buffer
			Sha->Data[i]=0;

	bytePos = (BYTE)Sha->byteCounter&0x0003;   // get posstion in SHA word
	i = (BYTE)((Sha->byteCounter&0x0003F)>>2); // get SHA's word address
	if( bytePos == 1 ){
		Sha->Data[i] |= ((DWORD)GetByteBStatusMX(Sha->BStatusMXCounter)<<16);
		Sha->BStatusMXCounter++;
		Sha->byteCounter++;
		if( Sha->BStatusMXCounter!=BSTATMX_SIZE ){
			Sha->Data[i] |= ((DWORD)GetByteBStatusMX(Sha->BStatusMXCounter)<<8);
			Sha->BStatusMXCounter++;
			Sha->byteCounter++;
		}
		if( Sha->BStatusMXCounter!=BSTATMX_SIZE ){
			Sha->Data[i] |= (DWORD)GetByteBStatusMX( Sha->BStatusMXCounter);
			Sha->BStatusMXCounter++;
			Sha->byteCounter++;
		}
	}
	if( bytePos == 2 ){
		Sha->Data[i] |= ((DWORD)GetByteBStatusMX( Sha->BStatusMXCounter)<<8);
		Sha->BStatusMXCounter++;
		Sha->byteCounter++;
		if( Sha->BStatusMXCounter ){
			Sha->Data[i] |= (DWORD)GetByteBStatusMX( Sha->BStatusMXCounter);
			Sha->BStatusMXCounter++;
			Sha->byteCounter++;
		}
	}
	if( bytePos == 3 ){
		Sha->Data[i] |= (DWORD)GetByteBStatusMX( Sha->BStatusMXCounter);
		Sha->BStatusMXCounter++;
		Sha->byteCounter++;
	}
	if( bytePos !=0 )
		i++;

	while( i < 16 ){
		if( Sha->BStatusMXCounter==BSTATMX_SIZE ){ // no bytes remain in
			return BSTATMX_FINISHED;
		}
		else{
			Sha->Data[i] |= ((DWORD)GetByteBStatusMX(Sha->BStatusMXCounter)<<24);
			Sha->byteCounter++;
			Sha->BStatusMXCounter++;
			if( Sha->BStatusMXCounter==BSTATMX_SIZE ){// no bytes remain in
				return BSTATMX_FINISHED;
			}
			else{
				Sha->Data[i]|= ((DWORD)GetByteBStatusMX(Sha->BStatusMXCounter)<<16);
				Sha->byteCounter++;
				Sha->BStatusMXCounter++;
				if( Sha->BStatusMXCounter==BSTATMX_SIZE ){// no bytes remain in
					return BSTATMX_FINISHED;
				}
				else{
					Sha->Data[i] |= ((DWORD)GetByteBStatusMX( Sha->BStatusMXCounter)<<8);
					Sha->byteCounter++;
					Sha->BStatusMXCounter++;
					if( Sha->BStatusMXCounter==BSTATMX_SIZE ){// no bytes remain in
						return BSTATMX_FINISHED;
					}
					else{
						Sha->Data[i] |= (DWORD)GetByteBStatusMX( Sha->BStatusMXCounter);
						Sha->byteCounter ++;
						Sha->BStatusMXCounter++;
					}
				}
			}
		}
		i++;
	}

	return    BSTATMX_PULLED;
}

//------------------------------------------------------------------------------
static BYTE PlacePadBitCounter( SHA_CTX * Sha ){
	BYTE i, bytePos;
	BYTE Status;

	bytePos = (BYTE)Sha->byteCounter&0x0003;   // get posstion in SHA word
	i = (BYTE)((Sha->byteCounter&0x0003F)>>2); // get SHA's word address

	if( bytePos == 0 )
		Sha->Data[i]|=0x80000000;
	else if( bytePos == 1 )
		Sha->Data[i]|=0x00800000;
	else if( bytePos == 2 )
		Sha->Data[i]|=0x00008000;
	else if( bytePos == 3 )
		Sha->Data[i]|=0x00000080;
	Status = PADED;
	if( ((Sha->byteCounter & 0x3f)>>2)< 14 ){
		Sha->Data[15] = Sha->byteCounter*8;
		Status =  END_PLACED;
	}
	return Status;
}

//------------------------------------------------------------------------------
static void PlaceBitCounter( SHA_CTX * Sha )
{
	BYTE i;

	for(i = 0; i < 16; i++) // clear data buffer
		Sha->Data[i]=0;
	Sha->Data[15] = Sha->byteCounter*8;
}

//------------------------------------------------------------------------------
static BOOL CompareVi( SHA_CTX  * Sha )
{
	DWORD VPrime;
	BOOL Result = TRUE;
	BYTE i;

	MDDCInitReadBlockFromFIFO( DDC_V_ADDR, 20 );
	for( i = 0; i < 5; i ++ ){
		MDDCTakeBlockFromFIFO(4, (BYTE *)&VPrime);
		//LEndianBEndianConvert((BYTE *)&VPrime);
		if( Sha->Digest[i] != VPrime ){
			Result = FALSE;
			break;
		}
	}
	return Result;
}

//------------------------------------------------------------------------------
BYTE SHAHandler( void )
{
	BYTE HPDStatus;
	SHA_CTX  sha;
	BYTE BufStatus;
	WORD KSVSize;

	if( DownStreamAuthState == REQ_SHA_CALC ){
		KSVSize =  GetFIFOSize()*5;         //Get FIFO size (number of KSV) /// // Must be done BEFORE reading KSV FIFO!!! - 8/24/07

		if( KSVSize ){
			MDDCInitReadBlockFromFIFO( DDC_KSV_FIFO_ADDR, KSVSize );
		}

		SHAInit( &sha );
		while (GetKSVList( &sha, KSVSize )==KSV_PULLED ){
			SHSTransform( &sha );
		}

		if( BSTATMX_PULLED==GetBStatusMX(&sha) ){	/// Got here if less than 512 bits that were left in FIFO were just read into buffer,
			SHSTransform( &sha );					       /// meaning that the buffer has room for more data. Now attempt to copy  into the
			/// same buffer the 2 bytes of BStatus and 8 of MX. if BSTATMX_PULLED, it means that
			/// the last KSV's + BSttus + MX exceeded 512 bit. Transform W/O padding.
			GetBStatusMX( &sha );		      /// Now get the leftovers of KSV BStatus and MX. Will never reach 14 DWords. Must pad.
			PlacePadBitCounter( &sha );	      /// Pad down to DWord 14. Store length in DWord 15.
			SHSTransform( &sha );		      /// Hash this very last block after padding it.
		} else {
			BufStatus = PlacePadBitCounter( &sha );
			SHSTransform( &sha );
			if( BufStatus != END_PLACED ){
				PlaceBitCounter(&sha);
				SHSTransform(&sha);
			}
		}

		HPDStatus = siiTXHDCP_CheckHPDChanged();
		if (HPDStatus != HPD_NOT_CHANGED)
			return HPDStatus;

		if( CompareVi(&sha) ){
			RiCheckInterruptMask( ON );
			return AUTHENTICATED;
		}
		else
			return FAILED_ViERROR;
	}

	return AUTHENTICATED; //should not be there. Ma Jia add this for avoid compile warnning.
}

#ifdef REPEATER_SUPPORT
static inline void endian_swap_5B_data(BYTE *data)
{
	BYTE tmp[HDCP1X_KSV_SIZE];
	int i,j;
	for ( i=0, j=(HDCP1X_KSV_SIZE-1); i < HDCP1X_KSV_SIZE; i++, j-- )
	{
		tmp[i] = data[j];
	}
	memcpy(data, tmp, HDCP1X_KSV_SIZE);
}

void convert_receiverid_format_2_to_1x(BYTE *receiver_id)
{
	endian_swap_5B_data(receiver_id);
}

void convert_receiverid_format_1x_to_2(BYTE *hdcp1x_bksv)
{
	endian_swap_5B_data(hdcp1x_bksv);
}

int getHDCP1xDownstreamInfo(unsigned char *receiver_id_list)
{
	struct timeval tm_start;
	struct timeval tm_now;
	unsigned int TimeTaken;
	int noOfReattempt=0;

	hdcp1xInitiatedFromSource = 0;
	gettimeofday(&tm_start, NULL);

	//udelay(150000);
	/* Reattempt HDCP 1.x authentication till we receive fresh request from Source or timeout period elapse.
	 * This Logic will improve end-end time taken to perform HDCP authentication */
	while(si_hdmi_tx_hdcp_get_downstream_info(receiver_id_list) != AUTHENTICATED)
	{
		gettimeofday(&tm_now, NULL);
		TimeTaken = ((tm_now.tv_sec - tm_start.tv_sec) * 1000 + (tm_now.tv_usec - tm_start.tv_usec) / 1000);
		if (hdcp1xInitiatedFromSource || (TimeTaken > HDCP1X_RETRY_TIMEOUT))
		{
			break;
		}
		//DownStreamAuthState = REQ_AUTHENTICATION;
		mdelay(200);
		noOfReattempt++;
		printk("HDCP 1.x auth status polled : %d time\n", noOfReattempt);
	}
	return 0;
}

int setHDCP1xAuthState (char state)
{
	DEBUG_PRINTK ("TRIGGER HDCP 1.x AUTHENTICATION.\n");
	hdcp1xInitiatedFromSource = 1;
	DownStreamAuthState = state;

	return 0;
}

int si_hdmi_tx_hdcp_get_downstream_info(unsigned char *receiver_id_list)
{
	unsigned int  index;
	downstream_info_ksv_list_t *ptr_downstream_ksv_info = (downstream_info_ksv_list_t *)receiver_id_list;

	memset(ptr_downstream_ksv_info, 0, sizeof(downstream_info_ksv_list_t));
	if (DownStreamAuthState == AUTHENTICATED)
	{
		memcpy(&ptr_downstream_ksv_info->bksv[0], BKSV_DATA, HDCP1X_KSV_SIZE);
		convert_receiverid_format_1x_to_2(&ptr_downstream_ksv_info->bksv[0]);
		DEBUG_PRINTK("\nTV BKSV=%2x:%2x:%2x:%2x:%2x\n",BKSV_DATA[0],BKSV_DATA[1],BKSV_DATA[2],BKSV_DATA[3],BKSV_DATA[4]);

		if (IsRepeater())
		{
			ptr_downstream_ksv_info->rptr_dev_cnt = GetFIFOSize();
			ptr_downstream_ksv_info->rptr_dev_exd = GetDeviceCountExceeded();
			ptr_downstream_ksv_info->rptr_casc_exd = GetCascadeDepthExceeded();
			ptr_downstream_ksv_info->rptr_depth = GetCascadeDepth();

			//memcpy(&ptr_downstream_ksv_info->ksv_list, &KSV_BUF[0], (ptr_downstream_ksv_info->rptr_dev_cnt * HDCP1X_KSV_SIZE));

			for ( index = 0; index < (ptr_downstream_ksv_info->rptr_dev_cnt*HDCP1X_KSV_SIZE); index+=HDCP1X_KSV_SIZE )
				convert_receiverid_format_1x_to_2(ptr_downstream_ksv_info->ksv_list + index);

			DEBUG_PRINTK("Downstream device is a repeater\n");
			DEBUG_PRINTK("Downstream device count : %d\n", ptr_downstream_ksv_info->rptr_dev_cnt);
			DEBUG_PRINTK("Downstream device count exceeded : %d\n", ptr_downstream_ksv_info->rptr_dev_exd);
			DEBUG_PRINTK("Downstream cascade depth : %d\n", ptr_downstream_ksv_info->rptr_depth);
			DEBUG_PRINTK("Downstream cascade depth exceeded : %d\n", ptr_downstream_ksv_info->rptr_casc_exd);
		}
	}

	return DownStreamAuthState;
}
#endif
