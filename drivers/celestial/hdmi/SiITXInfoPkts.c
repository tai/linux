//------------------------------------------------------------------------------
// Module Name: SiITXInfoPkts.c
// Module Description: SiITX Infoframe Packets
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/delay.h>

#include "TypeDefs.h"
#include "SiIIIC.h"
#include "SiITX.h"
#include "SiITXDefs.h"
#include "SiITXInfoPkts.h"
#include "SiIVRTables.h"

extern unsigned int rgb_limited_range;
// Default channel mapping per number of channels scenario
// Example 1: 2 Channels audioInfoFrameChannelMapping[1] = 0x0
// Example 2: 5.1 (6 Channels) audioInfoFrameChannelMapping[5] = 0xB
int audioInfoFrameChannelMapping[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0xB, 0x0, 0x0};

// Function to change default channel mappings
void setAudioInfoFrameChannelMapping(int nCh, int chMap) {
    audioInfoFrameChannelMapping[nCh] = chMap;
    return;
}

//-----------------------------------------------------------
// Audio contain protecton packet
const BYTE AudioCP[31] = {
	0x04,
	0x00, // status
	0x00, // reserved
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // data
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 // reserved data
};

//-----------------------------------------------------------
const BYTE ISRC1[31] = {
	0x05,
	0x00, // status
	0x00, // reserved
	1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // data
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 // reserved data
};

const BYTE ISRC2[31] = {
	0x06,
	0x00, // reserved
	0x00, // reserved
	2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // data
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 // reserved data
};

//----------------------------------------------------------------------------------
BOOL GetInfoFrameMapAddr( BYTE InfoType, InfoMapType * InfoMap )
{
	BOOL Result = TRUE;

	if( InfoType == AVI_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_AVI_EN_REPEAT;
		InfoMap->DestAddr =  AVI_IF_ADDR;
		InfoMap->BlockSize = AVI_SIZE;
	}
	else if( InfoType == SPD_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_SPD_EN_REPEAT;
		InfoMap->DestAddr = SPD_IF_ADDR;
		InfoMap->BlockSize = SPD_IF_SIZE;
	}
	else if( InfoType == AUD_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_AUD_EN_REPEAT;
		InfoMap->DestAddr = AUD_IF_ADDR;
		InfoMap->BlockSize = AUD_IF_SIZE;
	}
	else if( InfoType == MPEG_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_MPEG_EN_REPEAT;
		InfoMap->DestAddr = MPEG_IF_ADDR;
		InfoMap->BlockSize = MPEG_IF_SIZE;
	}
	else if ( InfoType == GENERIC1_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_GENERIC1_EN_REPEAT;
		InfoMap->DestAddr = GENERIC1_IF_ADDR;
		InfoMap->BlockSize = GENERIC1_IF_SIZE;
	}
	else if ( InfoType == GENERIC2_TYPE ){
		InfoMap->CtrlBitsAddr = BIT_GENERIC2_EN_REPEAT;
		InfoMap->DestAddr = GENERIC2_IF_ADDR;
		InfoMap->BlockSize = GENERIC2_IF_SIZE;
	}
    else if ( InfoType == VENDORSPEC_TYPE ) {   // 2012-02-20 by xp, use SPD registers to send vendor specific IF
        InfoMap->CtrlBitsAddr = BIT_SPD_EN_REPEAT;
		InfoMap->DestAddr = SPD_IF_ADDR;
		InfoMap->BlockSize = SPD_IF_SIZE;
    }
	else Result = FALSE;
	return Result;
}

//----------------------------------------------------------------------------------
BOOL GetRestOfInfoFrameHeader( HeaderType * Header )
{
	BOOL Result = TRUE;

	if( Header->Type == AVI_TYPE) {
		Header->Version = 2 ;
		Header->Length = 13; //15 //Oscar modify 20080620;
	}
	else if( Header->Type == SPD_TYPE){
		Header->Version = 1;
		Header->Length = 27;
	}
	else if( Header->Type == AUD_TYPE){
		Header->Version = 1 ;
		Header->Length = 10;
	}
	else if( Header->Type == MPEG_TYPE){
		Header->Version = 1;
		Header->Length = 27;
	}
    else if (Header->Type == VENDORSPEC_TYPE) {
        Header->Version = 1;
		Header->Length = 7;
    }
	else
		Result = FALSE;
	return Result;
}

//----------------------------------------------------------------------------------
BOOL WaitBuffReady( WORD CtrInfoBits )
{
	WORD RegVal, RptBitMsk, EnBitMask;
	BYTE TimeOutCount = 60;
	BOOL Result = TRUE;

	RptBitMsk = (CtrInfoBits >> 1) & CtrInfoBits;
	EnBitMask = (CtrInfoBits << 1) & CtrInfoBits;
	RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1 );
	if ( RegVal & CtrInfoBits ) {
		if ( RegVal & RptBitMsk )
			hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal & (~RptBitMsk)); // Clear reapet bit
		while(TimeOutCount--) {
			if( !(hlReadWord_8BA( TX_SLV1, INF_CTRL1 ) & EnBitMask))
				break;
			udelay(1000);
		}
		if(TimeOutCount)
			Result = TRUE; // ready to rewrite data buffer
		else
			Result = FALSE; // repeat bit is stuck, not ready to write into the buffer
	}
	return Result;
}

//----------------------------------------------------------------------------------
void CalcCheckSumIFPacket( HeaderType * Header, BYTE * InfoFramePacket )
{
	BYTE i;
	// byte 2 length
	// byte 3 checksum
	Header->CheckSum = Header->Type + Header->Length + Header->Version;

	for (i = 0; i < Header->Length; i++ )
		Header->CheckSum += InfoFramePacket[i];
	Header->CheckSum =  0x100 - Header->CheckSum;
}

//----------------------------------------------------------------------------------
BYTE siiTXSendInfoFrame( BYTE bIFType, BYTE * InfoFramePacket )
{
	BYTE ErrNo = 0;
	InfoMapType InfoMap;
	HeaderType IFHeader;
	WORD RegVal;

	if( GetInfoFrameMapAddr( bIFType, &InfoMap ) ){
		IFHeader.Type = bIFType;
		GetRestOfInfoFrameHeader( &IFHeader );
		if ( !( hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_P_STABLE ) )
			ErrNo = UNSTABLE_TCLK;
		else if ( !WaitBuffReady(InfoMap.CtrlBitsAddr ) )
			ErrNo = EN_BIT_STUCK;

		CalcCheckSumIFPacket( &IFHeader, InfoFramePacket );
		siiWriteBlockHDMI( TX_SLV1, InfoMap.DestAddr, 4, ( BYTE * ) &IFHeader );
		siiWriteBlockHDMI( TX_SLV1, InfoMap.DestAddr + 4, IFHeader.Length, InfoFramePacket ); // Send Info
		if ( !ErrNo ) {
			RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1 );
			hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal | InfoMap.CtrlBitsAddr );
		}
	}
	else
		ErrNo = WRGN_IFR_TYPE;
	return ErrNo;

}

//------------------------------------------------------------------------------
// Function Name: siiTXSetAudioInfoFramePacket
// Function Description:
//------------------------------------------------------------------------------
void siiTXSetAudioInfoFramePacket ( void )
{
	BYTE abData [10], i;

	for ( i = 0; i < 10; i++ ) // Clear InfoFrame Data
		abData [i] = 0x00;

	abData [0] = siiTX.siiTXInputAudioChannel;
	abData [3] = audioInfoFrameChannelMapping[siiTX.siiTXInputAudioChannel];

	DEBUG_PRINTK("\n*** AUDIO INFO FRAMES. Num. Channels: %d, Audio Channels Mapping %X \n\n", abData[0], abData[3]);

	siiTXSendInfoFrame( AUD_TYPE, abData );
}

//------------------------------------------------------------------------------
// Function Name: siiTXSetAVIInfoFramePacket
// Function Description:
//------------------------------------------------------------------------------
void siiTXSetAVIInfoFramePacket( void )
{
	BYTE abData [13], i;

	for ( i = 0; i < 13; i++ ) // Clear InfoFrame Data
		abData [i] = 0x00;

	abData [0] = 0x11;  // Oscar Ding Added on 20080911.
	abData [1] = 0x08;

	// Color space set.
	if( siiTX.siiTXOutputVideoType == 0 ) // RGB
		//abData [0] = 0x11;
		abData [0] |= 0x00;

	else if( siiTX.siiTXOutputVideoType == 1 ) // YUV444
		//abData [0] = 0x51;
		abData [0] |= 0x40;

	else if( siiTX.siiTXOutputVideoType == 2 ) // YUV422
		//abData [0] = 0x31;
		abData [0] |= 0x20;

	// Color Colorimetry set.
	if( HD_Mode() )
		abData [1] |= 0x80;
	else
		abData [1] |= 0x40;
	// Color Picture Aspect Ratio set.
	if( PictureAspectRatio == 0 ) // 4:3
		abData [1] |= 0x10;
	else if( PictureAspectRatio == 1 ) // 16:9
		abData [1] |= 0x20;

	if (siiTX.siiTXOutputVideoType == 0){
		if(0 == rgb_limited_range){
			abData [2] = 0x08;
		}
		else{
			abData [2] = 0x04;
		}
	}
	abData [3] = VICTables[siiTX.siiTXInputVideoMode];
	abData [4] = siiTX.siiTXInputPixRepl;

	siiTXSendInfoFrame( AVI_TYPE, abData );
}

//------------------------------------------------------------------------------
// Function Name: siiTXSetVendorSpecificInfoFramePacket
// Function Description:
//------------------------------------------------------------------------------
void siiTXSetVendorSpecificInfoFramePacket( int Format_3D )
{
	BYTE abData [27], i;

	for ( i = 0; i < 27; i++ ) // Clear InfoFrame Data
		abData [i] = 0x00;

    abData[0] = 0x03;
    abData[1] = 0x0c;
    abData[2] = 0x00;   // 24bit IEEE Registration Identifier

	abData [3] = 0x40;  // HDMI_Video_Format, 0x40 means 3D format indication present.
	switch (Format_3D) {
    case SIDE_BY_SIDE_HALF:
    	abData [4] = 0x80;   // 3D_Structure
    	abData [5] = 0x80;   // 3D_Ext_Data
    	break;
    case TOP_AND_BOTTOM:
        abData [4] = 0x60;   // 3D_Structure
    	abData [5] = 0x0;   // 3D_Ext_Data
    	break;
    default:
        abData [4] = 0x80;   // 3D_Structure
    	abData [5] = 0x80;   // 3D_Ext_Data
    	break;
	}
	siiTXSendInfoFrame( VENDORSPEC_TYPE, abData );
}

//----------------------------------------------------------------------------------
// InfoFrame will be disabled, don't call in PD mode
//----------------------------------------------------------------------------------
BOOL siiTXDisableInfoFrame( BYTE InfoFrameType )
{
	InfoMapType InfoMap;
	WORD RegVal;
	BOOL  Result = FALSE;

	RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1 );
	if( GetInfoFrameMapAddr( InfoFrameType, &InfoMap) ){
        hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal & (~InfoMap.CtrlBitsAddr) );
		Result = TRUE;
	}
	return Result;
}

//----------------------------------------------------------------------------------
// InfoFrame will be enable and set repeat property, InfoFrame cannot be enabled at Power Down mode
//----------------------------------------------------------------------------------
BOOL siiTXEnableInfoFrame( BYTE InfoFrameType )
{
	InfoMapType InfoMap;
	WORD RegVal;
	BOOL  Result = FALSE;

	RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1 );
	if( GetInfoFrameMapAddr( InfoFrameType, &InfoMap) ){
		hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal | InfoMap.CtrlBitsAddr );
		Result = TRUE;
	}
	return Result;
}

//----------------------------------------------------------------------------------
// Disable Info Packets
//----------------------------------------------------------------------------------
BOOL siiTXDisableInfoPacket( BYTE PacketId )
{
	InfoMapType InfoMap;
	WORD RegVal;
	BOOL  Result = FALSE;

	RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1);
	if( GetInfoFrameMapAddr( PacketId, &InfoMap) ){
		hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal & (~ InfoMap.CtrlBitsAddr) );
		Result = TRUE;
	}
	return Result;
}

//----------------------------------------------------------------------------------
// Enable Info Packets
//----------------------------------------------------------------------------------
BOOL siiTXEnableInfoPacket( BYTE PacketId )
{
	InfoMapType InfoMap;
	WORD RegVal;
	BOOL  Result = FALSE;

	RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1);
	if( GetInfoFrameMapAddr( PacketId, &InfoMap) ){
		if( WaitBuffReady(InfoMap.CtrlBitsAddr) ){
			hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal | InfoMap.CtrlBitsAddr );
			Result = TRUE;
		}
	}
	return Result;
}

//-------------------------------------------------------------------------------
void siiTXSendCP_Packet( BYTE On )
{
	BYTE RegVal, TimeOutCount = 64;

	if( IsTXInHDMIMode() ){				// Send CP packets only if in HDMI mode
		RegVal = hlReadByte_8BA( TX_SLV1, INF_CTRL2 );
		hlWriteByte_8BA( TX_SLV1, INF_CTRL2, RegVal & (~BIT_CP_REPEAT) );
		if( On )
			hlWriteByte_8BA( TX_SLV1, CP_IF_ADDR, BIT_CP_AVI_MUTE_SET );
		else
			hlWriteByte_8BA( TX_SLV1, CP_IF_ADDR, BIT_CP_AVI_MUTE_CLEAR );

                mdelay(50);  // Without this delay, AV Mute/Unmute causes snow on the TV !

		while(TimeOutCount--){
			if( !(hlReadByte_8BA( TX_SLV1, INF_CTRL2) & BIT_CP_ENABLE) )
				break;
		}

		if(TimeOutCount)
			hlWriteByte_8BA( TX_SLV1, INF_CTRL2, RegVal | BIT_CP_ENABLE | BIT_CP_REPEAT );
	}
}

//----------------------------------------------------------------------------------
// Send InfoPackets, the main difference compare to InfoFrames, there no check sums
//----------------------------------------------------------------------------------
BYTE siiTXSendInfoPacket(BYTE * Data, BYTE PcktSize, BYTE PacketId, BYTE Mode)
{
	InfoMapType InfoMap;
	BYTE   ErrNo = 0;
	WORD RegVal;

	if( GetInfoFrameMapAddr( PacketId, &InfoMap) ){
		if( PcktSize > InfoMap.BlockSize )
			ErrNo = BUF_SIZE_ERR;
		else {
			if( Mode == WR_IN_PD ){      // Write in PD mode
				siiWriteBlockHDMI( TX_SLV1, InfoMap.DestAddr, PcktSize, Data ); //Write Info packet
			}
			else  {
				if( WaitBuffReady(InfoMap.CtrlBitsAddr) ){
					siiWriteBlockHDMI( TX_SLV1, InfoMap.DestAddr, PcktSize, Data );  //Send Header
					RegVal = hlReadWord_8BA( TX_SLV1, INF_CTRL1 );
					hlWriteWord_8BA( TX_SLV1, INF_CTRL1, RegVal | InfoMap.CtrlBitsAddr );
				}
				else ErrNo = RPT_BIT_STUCK;
			}
		}
	}
	else ErrNo = WRNG_IPK_ID;
	return ErrNo;
}

