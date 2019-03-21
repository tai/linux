//---------------------------------------------------------------------------
// Module Name: SiITX.c
// Module Description: Intit SII HDMI TX
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//---------------------------------------------------------------------------

#include <linux/kernel.h>
#include <linux/delay.h>

#include "hdmi_api.h"
#include "TypeDefs.h"
#include "EDID.h"
#include "SiIIIC.h"
#include "SiITX.h"
#include "SiITXDefs.h"
#include "SiITXHDCP.h"
#include "SiITXInfoPkts.h"
#include "SiIVRTables.h"
#include "hdmi_sw_hw_if.h"
#include "si_apiCEC.h"

#ifdef USE_ABT1030
#include "ABT1030_Reslution.h"
#endif

extern unsigned int rgb_limited_range;
extern uint8_t SI_CecHandler (uint8_t currentPort, unsigned char returnTask);
extern int siiTXHDCP_GetAuthenticationPart2WDTime(void);

extern void clock_hdmi_reset(CLOCK_RESET ResetOrSet);

siiTXParameter siiTX;

WORD RiFailureCount = 0;
BYTE DSAuthPart1ErrorCounter = 0;
BYTE DSAuthPart2ErrorCounter = 0;

BOOL AssertedDSAuthPart2 = 0;
WORD TimeOutForDSAuthPart2 = 0;

const BYTE inRGB24[] = {
	BYPASS,                      // RGB24 out
	TX_RGBToYCbCr,        // YCbCr24 Out                    //Oscar 0x49
	TX_RGBToYCbCr | TX_444to422   // YC24 out   //Oscar 0x49
};

const BYTE inRGBDVO12[] = {
	TX_DVO,                        // RGB24 out
	TX_DVO | TX_RGBToYCbCr, // YCbCr24 Out                          //Oscar 0x49
	TX_DVO | TX_RGBToYCbCr |TX_444to422   // YC24 out   //Oscar 0x49
};

const BYTE inYCbCr24[] = {
	TX_YCbCrToRGB | TX_Dither,   // RGB24 out
	BYPASS,                       // YCbCr24 Out
	TX_444to422           // YC24 out   //Oscar 0x49
};

const BYTE inYC24[] = {
	TX_422to444 | TX_YCbCrToRGB | TX_Dither, // RGB24 out
	TX_422to444 | TX_Dither,                 // YCbCr24 out
	BYPASS                                   // YC24 out
};

const BYTE inYCMuxed12[] = {
	TX_DeMux | TX_422to444 | TX_YCbCrToRGB | TX_Dither, // RGB24 out
	TX_DeMux | TX_422to444 | TX_Dither,                 // YCbCr24 out
	TX_DeMux                                            // YC24 out
};

const BYTE inYCMuxed656_12[] = {
	TX_DeMux | TX_SyncExtr | TX_422to444 | TX_YCbCrToRGB | TX_Dither, // RGB24 out
	TX_DeMux | TX_SyncExtr | TX_422to444 | TX_Dither,                 // YCbCr24 out
	TX_DeMux | TX_SyncExtr                                            // YC24 out
};

const BYTE inYC656_24[] = {
	TX_SyncExtr | TX_422to444 | TX_YCbCrToRGB | TX_Dither, // RGB24 out
	TX_SyncExtr | TX_422to444 | TX_Dither,                 // YCbCr24 out
	TX_SyncExtr                                            // YC24 out
};

const unsigned char * const inMode[] = {
	inRGB24, inRGBDVO12, inYCbCr24, inYC24, inYCMuxed12, inYCMuxed656_12, inYC656_24
};

//---------------------------------------------------------------------------
// Function Name: siiTXInitParameter
// Function Description: Set siiTX default values
//---------------------------------------------------------------------------
void siiTXInitParameter( void )
{
	siiTX.siiTXInputVideoMode         = 4;   // 2=720p60   //if you change this outputmode.  then you must change this parame
	siiTX.siiTXInputVideoType         = 5;    //YCbCr24
	siiTX.siiTXInputColorDepth        = 0;    //24 bit input
	siiTX.siiTXInputPixRepl           = 1;    //no replication
	siiTX.siiTXInputClockEdge         = InputClockRisingEdge;

	siiTX.siiTXOutputVideoType        = 0;    //RGB24 output		//an xuqiu
	siiTX.siiTXOutputColorDepth       = 0;    //24 bit output

	siiTX.siiTXOutputMode             = None_Mode;
	siiTX.siiTXOutputState            = CABLE_UNPLUG_;

	siiTX.siiTXInputAudioType         = 1;             // I2S input
	siiTX.siiTXInputAudioFs           = 2;             // 48 KHz
	siiTX.siiTXInputAudioSampleLength = 0x0b;          // 24 bit
	siiTX.siiTXInputAudioChannel      = 1;             // 2 channels

	//use external DE
	//modify .from EXTERNAL_DE to EMBEDED_SYNC; because pin:hstnc,vsync,De is low.
	//siiTX.siiTXDEMode = EMBEDED_SYNC;
	siiTX.siiTXDEMode                 = EXTERNAL_DE;
	//siiTX.siiTXHDCPEnable             =0;
	if(siiTX.is_first_time != 1)
		siiTX.is_first_time = 0;
	//#endif
}
//.............................................................
//480i:
//siiTX.siiTXInputVideoMode = 4;
//siiTX.siiTXInputVideoType = 5;
//siiTX.siiTXInputPixRepl = 1;

//576i:
//siiTX.siiTXInputVideoMode = 15;
//siiTX.siiTXInputVideoType = 5;
//siiTX.siiTXInputPixRepl = 1;

//576p:
//siiTX.siiTXInputVideoMode = 12;
//siiTX.siiTXInputVideoType = 6;
//siiTX.siiTXInputPixRepl = 0;

//720p:
//siiTX.siiTXInputVideoMode = 2;
//siiTX.siiTXInputVideoType = 6;
//siiTX.siiTXInputPixRepl = 0;

//1080i:
//siiTX.siiTXInputVideoMode = 14;
//siiTX.siiTXInputVideoType = 6;
//siiTX.siiTXInputPixRepl = 0;

//.............................................................

void ChangeInitVideoMode(unsigned int videomote)
{

	switch(videomote)
	{
		case sii9030_V640x480p_60Hz:
			siiTX.siiTXInputVideoMode = 0;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V720x480p_60Hz_4x3:
		case sii9030_V720x480p_60Hz_16x9:
			siiTX.siiTXInputVideoMode = 1;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V720x480i_60Hz_4x3:
		case sii9030_V720x480i_60Hz_16x9:
			siiTX.siiTXInputVideoMode = 4;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 1;
			break;
		case sii9030_V720x576i_50Hz_4x3:
		case sii9030_V720x576i_50Hz_16x9:
			siiTX.siiTXInputVideoMode = 15;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 1;
			break;
		case sii9030_V720x576p_50Hz_4x3:
		case sii9030_V720x576p_50Hz_16x9:
			siiTX.siiTXInputVideoMode = 12;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1280x720p_50Hz:
			siiTX.siiTXInputVideoMode = 13;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1280x720p_60Hz:
			siiTX.siiTXInputVideoMode = 2;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080i_50Hz:
			siiTX.siiTXInputVideoMode = 14;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080i_60Hz:
			siiTX.siiTXInputVideoMode = 3;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_50Hz:
			siiTX.siiTXInputVideoMode = 24;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_60Hz:
			siiTX.siiTXInputVideoMode = 11;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_24Hz:
			siiTX.siiTXInputVideoMode = 25;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_30Hz:
			siiTX.siiTXInputVideoMode = 27;
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 0;
			break;

		case sii9030_V800x600p_60Hz:
			siiTX.siiTXInputVideoMode = 58;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1024x768p_60Hz:
			siiTX.siiTXInputVideoMode = 66;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1280x720p_rgb_60Hz:
			siiTX.siiTXInputVideoMode = 2;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V800x480p_60Hz:// majia:not correct
			siiTX.siiTXInputVideoMode = 93;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1440x900p_60Hz:// majia:not correct
			siiTX.siiTXInputVideoMode = 0;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1280x1024p_60Hz:
			siiTX.siiTXInputVideoMode = 73;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1360x768p_60Hz:
			siiTX.siiTXInputVideoMode = 95;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_rgb_30Hz:
			siiTX.siiTXInputVideoMode = 11;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080p_rgb_60Hz:
			siiTX.siiTXInputVideoMode = 11;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1920x1080i_rgb_60Hz:
			siiTX.siiTXInputVideoMode = 3;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		case sii9030_V1366x768p_60Hz:
			siiTX.siiTXInputVideoMode = 97;
			siiTX.siiTXInputVideoType = 0;
			siiTX.siiTXInputPixRepl   = 0;
			break;
		default:
			siiTX.siiTXInputVideoMode = 0;//15; majia add here
			siiTX.siiTXInputVideoType = 2;
			siiTX.siiTXInputPixRepl   = 1;
	}

	siiTX.siiTXInputVideoType  = 2; // only for 1800L
}

//---------------------------------------------------------------------------
// Function Name: HD_Mode
// Function Description:
//---------------------------------------------------------------------------
BOOL HD_Mode( void )
{
	DEBUG_PRINTK("11111111111111111111111111111111 = siiTX.siiTXInputVideoMode = %d\n",siiTX.siiTXInputVideoMode);
	if( (siiTX.siiTXInputVideoMode == 2) ||
			(siiTX.siiTXInputVideoMode == 3) ||
			(siiTX.siiTXInputVideoMode == 11) ||
			(siiTX.siiTXInputVideoMode == 13) ||
			(siiTX.siiTXInputVideoMode == 14) ||
			(siiTX.siiTXInputVideoMode == 24) ||
			(siiTX.siiTXInputVideoMode == 25) ||
			(siiTX.siiTXInputVideoMode == 26) ||
			(siiTX.siiTXInputVideoMode == 27) ||
			(siiTX.siiTXInputVideoMode == 30) ||
			(siiTX.siiTXInputVideoMode == 31) ||
			(siiTX.siiTXInputVideoMode == 32) ||
			(siiTX.siiTXInputVideoMode == 35) ||
			(siiTX.siiTXInputVideoMode == 36) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//---------------------------------------------------------------------------
// Function Name: siiTXHardwareReset
// Function Description: Hardware reset of TX
//---------------------------------------------------------------------------
void siiTXHardwareReset( void )
{
	clock_hdmi_reset(_do_reset);
	mdelay(18);
	clock_hdmi_reset(_do_set);
	mdelay(5);
	return;
}

//---------------------------------------------------------------------------
void siiTXSetToHDMIMode( BOOL bOn ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR );
	if(bOn)
		hlWriteByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR, RegVal | BIT_TXHDMI_MODE );
	else
		hlWriteByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR, RegVal & (~BIT_TXHDMI_MODE) );
}

//---------------------------------------------------------------------------
BYTE IsTXInHDMIMode( void ){
	return BIT_TXHDMI_MODE & hlReadByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR );
}

//---------------------------------------------------------------------------
void siiTXWriteNValue( DWORD N ){
	hlWriteWord_8BA( TX_SLV1, N_SVAL_ADDR, (WORD) N & 0xFFFF );
	hlWriteByte_8BA( TX_SLV1, N_SVAL_ADDR+2, (BYTE)(N >> 16) );
}

//---------------------------------------------------------------------------
void siiTXInitAudioPart( void ){

	if( siiTX.siiTXInputAudioType ){ //I2S input
		hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, 0xF1 ); // En.I2S SD0-SD3
		hlWriteByte_8BA( TX_SLV1, I2S_CHST4_ADDR, siiTX.siiTXInputAudioFs ); // Set I2S Fs
		hlWriteByte_8BA( TX_SLV1, I2S_CHST5_ADDR, (siiTX.siiTXInputAudioSampleLength | ((siiTX.siiTXInputAudioFs << 4) & 0xF0)));   // "Original Fs" and Length
		hlWriteByte_8BA( TX_SLV1, I2S_IN_LEN, siiTX.siiTXInputAudioSampleLength ); // Set Input I2S sample length
		hlWriteByte_8BA( TX_SLV1, I2S_IN_CTRL_ADDR, 0x41 );
	}
	else { //SPDIF input
		hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, 0x03 ); // En.SPDIF
	}

	siiTXWriteNValue( N_Val[siiTX.siiTXInputAudioFs] );
	hlWriteByte_8BA( TX_SLV1, FREQ_SVAL_ADDR, 0x01 ); // Set MCLK = 256*Fs
	hlWriteByte_8BA( TX_SLV1, ACR_CTRL_ADDR, 0x02 );  // En. N/CTS packets
}

//---------------------------------------------------------------------------
void UpdateTX_DE( void ){
	BYTE   Polarity, Interlace;
	WORD HStartPos, VStartPos;
	WORD HRes, VRes;
	BYTE   RegVal;

	Polarity =(~VModeTables[siiTX.siiTXInputVideoMode].Tag.RefrTypeVHPol )& 0x03;
	Interlace = VModeTables[siiTX.siiTXInputVideoMode].Tag.RefrTypeVHPol & 0x04;
	HStartPos = VModeTables[siiTX.siiTXInputVideoMode].Pos.H;
	VStartPos = VModeTables[siiTX.siiTXInputVideoMode].Pos.V;
	HRes = VModeTables[siiTX.siiTXInputVideoMode].Res.H;
	VRes = VModeTables[siiTX.siiTXInputVideoMode].Res.V;

	hlWriteByte_8BA( TX_SLV0, DE_HSTART_ADDR, HStartPos & 0xFF );
	hlWriteByte_8BA( TX_SLV0, DE_CNTRL_ADDR, (HStartPos>>8) & 0x03 );
	hlWriteWord_8BA( TX_SLV0, DE_HRES_ADDR, HRes );
	hlWriteByte_8BA( TX_SLV0, DE_VSTART_ADDR, VStartPos );
	if( Interlace ) // interlace
		hlWriteWord_8BA( TX_SLV0, DE_VRES_ADDR,VRes >> 1 );
	else
		hlWriteWord_8BA( TX_SLV0, DE_VRES_ADDR,VRes );

	RegVal = hlReadByte_8BA( TX_SLV0, DE_CNTRL_ADDR );
	hlWriteByte_8BA( TX_SLV0, DE_CNTRL_ADDR, (( Polarity << 4 )|RegVal) );
}

//---------------------------------------------------------------------------
void UpdateTX_656( void ){
	BYTE RegVal;
	BYTE Polarity;

	Polarity =(~VModeTables[siiTX.siiTXInputVideoMode].Tag.RefrTypeVHPol )& 0x03;
	RegVal = hlReadByte_8BA( TX_SLV0, DE_CNTRL_ADDR );
	hlWriteByte_8BA( TX_SLV0, DE_CNTRL_ADDR, (( Polarity << 4 )|RegVal) );

	RegVal = hlReadByte_8BA( TX_SLV0, INTERLACE_ADJ_MODE) & 0xF8;
	hlWriteByte_8BA( TX_SLV0, INTERLACE_ADJ_MODE, ((RegVal | (VModeTables[siiTX.siiTXInputVideoMode]._656.IntAdjMode)) & (~SET_DE_ADJ_BIT)));

	hlWriteWord_8BA( TX_SLV0, HBIT_TO_HSYNC_ADDR, VModeTables[siiTX.siiTXInputVideoMode]._656.HBit2HSync );
	hlWriteWord_8BA( TX_SLV0, FIELD2_HSYNC_OFFSET_ADDR, VModeTables[siiTX.siiTXInputVideoMode]._656.Field2Offset );
	hlWriteWord_8BA( TX_SLV0, HLENGTH_ADDR, VModeTables[siiTX.siiTXInputVideoMode]._656.HLength );
	hlWriteByte_8BA( TX_SLV0, VBIT_TO_VSYNC_ADDR,  VModeTables[siiTX.siiTXInputVideoMode]._656.VBit2VSync );
	hlWriteByte_8BA( TX_SLV0, VLENGTH_ADDR, VModeTables[siiTX.siiTXInputVideoMode]._656.VLength );
}

//---------------------------------------------------------------------------
void SetDE( BYTE DE_Enabled ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, DE_CNTRL_ADDR );
	if( DE_Enabled )
		RegVal |= BIT_DE_ENABLED;
	else
		RegVal &= (~BIT_DE_ENABLED);
	hlWriteByte_8BA( TX_SLV0, DE_CNTRL_ADDR, RegVal );
}

//---------------------------------------------------------------------------
// Function Name: siiTXDEconfig
// Function Description: Config TX DE setting
//---------------------------------------------------------------------------
void siiTXDEconfig( void ){
#if 0
	if( siiTX.siiTXDEMode == INTERNAL_DE ){
		UpdateTX_DE();    // HV Sync In DE Out
		SetDE(ON);            // Use HV Sync DE Gen
	}
	else if( siiTX.siiTXDEMode == EMBEDED_SYNC ){
		UpdateTX_656();  // Embedded sync In DE Out
		SetDE(OFF);           // Use Emb. Sync DE Gen
	}
#endif
	if( siiTX.siiTXDEMode == EMBEDED_SYNC )
	{
		UpdateTX_656();  // Embedded sync In DE Out
		//UpdateTX_DE();    // HV Sync In DE Out
		//SetDE(ON);            // Use HV Sync DE Gen
	}
	else if( siiTX.siiTXDEMode == EXTERNAL_DE )
	{
		SetDE(OFF);           // Use External DE
	}
}

//---------------------------------------------------------------------------
void siiTXSetIClk( BYTE IClk )
{
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, TX_VID_CTRL_ADDR) & 0xFC;
	hlWriteByte_8BA( TX_SLV0, TX_VID_CTRL_ADDR, (RegVal | IClk) );
}

//---------------------------------------------------------------------------
// Function Name: siiTXConfigVideoMode
// Function Description:  Config TX video mode
//---------------------------------------------------------------------------
BYTE siiTXConfigVideoMode( BYTE In, BYTE Out )
{
	BYTE TblVal, RegVal;

	TblVal = *(inMode[In] + Out);
	//--------------- Set Dev0x72_Reg0x4A ---------------
	RegVal = hlReadByte_8BA( TX_SLV0, TX_VID_MODE_ADDR );
	if( TblVal & TX_DeMux)
		RegVal |=BIT_TX_DEMUX_YC;
	else
		RegVal &= (~BIT_TX_DEMUX_YC);

	if( TblVal & TX_SyncExtr)
		RegVal |=BIT_TX_SYNC_EXTR;
	else
		RegVal &= (~BIT_TX_SYNC_EXTR);

	if( TblVal & TX_YCbCrToRGB)
	{
		RegVal |= BIT_TX_CSC;
		if(0 == rgb_limited_range){
			RegVal |= BIT_TX_16_235_RANGE;  // expand range
		}
	}
	else
	{
		RegVal &= (~BIT_TX_CSC);
		RegVal &= (~BIT_TX_16_235_RANGE);
		//RegVal |= BIT_TX_16_235_RANGE;  // expand range
	}

	if( TblVal & TX_Dither)
		RegVal |=BIT_DITHER_EN;
	else
		RegVal &= (~BIT_DITHER_EN);

	if( TblVal & TX_422to444)
		RegVal |= BIT_TX_422to444;
	else
		RegVal &= (~BIT_TX_422to444);

	hlWriteByte_8BA( TX_SLV0, TX_VID_MODE_ADDR, RegVal );

	//--------------- Set Dev0x72_Reg0x08 ---------------
	RegVal = hlReadByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR );
	if( TblVal & TX_DVO)
		RegVal &= (~BIT_BSEL24BITS);
	else
		RegVal |= BIT_BSEL24BITS;

	hlWriteByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR, RegVal );

	//--------------- Set Dev0x72_Reg0x48 ---------------
	RegVal = hlReadByte_8BA( TX_SLV0, TX_VID_CTRL_ADDR );
	if( HD_Mode() )
		RegVal |= BIT_CSCSEL;
	else
		RegVal &= (~BIT_CSCSEL);

	hlWriteByte_8BA( TX_SLV0, TX_VID_CTRL_ADDR, RegVal );
	//--------------------------------------------------
	//--------------- Set Dev0x72_Reg0x49 ---------------
	RegVal = hlReadByte_8BA( TX_SLV0, VID_ACEN_ADDR );
	if( TblVal & TX_RGBToYCbCr )
		RegVal |= BIT_VID_ACEN_RGB2YCBCR;
	else
		RegVal &= (~BIT_VID_ACEN_RGB2YCBCR);

	if( TblVal & TX_444to422 )
		RegVal |= BIT_VID_ACEN_DWN_SAMPLE;
	else
		RegVal &= (~BIT_VID_ACEN_DWN_SAMPLE);

	RegVal |= BIT_VID_ACEN_WIDE_BUS_MODE1;

	if( (TblVal & TX_YCbCrToRGB)&&(1 == rgb_limited_range)){
		RegVal |= BIT_VID_ACEN_RANGE_COMP;
		DEBUG_PRINTK("HDMI DEBUG : RegVal |= BIT_VID_ACEN_RANGE_COMP\n");
	}
	hlWriteByte_8BA( TX_SLV0, VID_ACEN_ADDR, RegVal );

	return TblVal;
}

//---------------------------------------------------------------------------
// Function Name: siiTXSetVideoBlank
// Function Description:  This function sets Video Blank
//---------------------------------------------------------------------------
void siiTXSetVideoBlank( BYTE Blank1, BYTE Blank2, BYTE Blank3 )
{
	hlWriteByte_8BA( TX_SLV0, TX_VID_BLANK1, Blank1 );
	hlWriteByte_8BA( TX_SLV0, TX_VID_BLANK2, Blank2 );
	hlWriteByte_8BA( TX_SLV0, TX_VID_BLANK3, Blank3 );

	return;
}

//---------------------------------------------------------------------------
// Function Name: SetInputClockEdge
// Function Description:  This function sets Input Clock Edge
//---------------------------------------------------------------------------
void siiTXSetInputClockEdge( BYTE bOn ){
	BYTE bRegVal;

	bRegVal = hlReadByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR );
	if( bOn )
		bRegVal |= BIT_TX_CLOCK_RISING_EDGE;
	else
		bRegVal &= (~BIT_TX_CLOCK_RISING_EDGE);
	hlWriteByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR, bRegVal);
}

//---------------------------------------------------------------------------
// Function Name: SetDeepColor
// Function Description:  This function sets Deep Color
//---------------------------------------------------------------------------
void siiTXSetDeepColor ( BYTE bDeepColor ){
	BYTE bRegVal, bTmpDeepColor;
	BYTE bRegVal2;

	bTmpDeepColor = bDeepColor;
	if (bTmpDeepColor == SiI_DeepColor_Off) {
		bDeepColor = SiI_DeepColor_24bit;       // Setup everything as 24bpp but do not turn deep color on
	}

	// VID_ACEN_ADDR
	//  [7:6] - Wide Bus
	//          0b00 = 24 bits
	//          0b01 = 30 bits
	//          0b10 = 36 bits
	bRegVal = (hlReadByte_8BA( TX_SLV0, VID_ACEN_ADDR ) & VID_ACEN_DEEP_COLOR_CLR);
	bRegVal |= (bDeepColor << 6);
	hlWriteByte_8BA( TX_SLV0, VID_ACEN_ADDR, bRegVal );

	// AUDP_TXCTRL_ADDR
	//  [5:3] - PACKET_MODE
	//          0b100 = 24 bits
	//          0b101 = 30 bits
	//          0b110 = 36 bits
	// Set / Clear bit 5 separately below.
	bRegVal = ( hlReadByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR ) & 0xE7 );
	bRegVal |= (bDeepColor << 3);

	if (bTmpDeepColor != SiI_DeepColor_Off){
		bRegVal |= BIT_DEEPCOLOR_EN;
		bRegVal |= 0x20;

		// Enable dithering and set Dither Mode for Deep Color:
		bRegVal2 = (hlReadByte_8BA( TX_SLV0, TX_VID_MODE_ADDR ) & CLR_BITS_7_6 );
		bRegVal2 |= (bDeepColor << 6);
		bRegVal2 |= BIT_DITHER_EN;
	}
	else{
		bRegVal &= (~BIT_DEEPCOLOR_EN);
		bRegVal &= (~0x20);
		bRegVal2 = (hlReadByte_8BA ( TX_SLV0, TX_VID_MODE_ADDR ) & CLR_BITS_7_6 );    // Disable dithering if not DC
	}

	hlWriteByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR, bRegVal );
	hlWriteByte_8BA( TX_SLV0, TX_VID_MODE_ADDR, bRegVal2 );
}

//---------------------------------------------------------------------------
// siiTX_TMDS_Setup
//---------------------------------------------------------------------------
BYTE siiTX_TMDS_Setup( BYTE bVMode ){
	int idclk_freq = 0, iLowRange = 0, iMidRange1 = 0, iMidRange2 = 0, iHghRange = 0;
	TCLK_SEL tclk;
	BYTE bRegVal;
	BYTE bRegVal2;
	BYTE nIPLLF = 0, nFFRCOUNT = 0, nFFBCOUNT = 0, nFPOSTCOUNT = 0;

	DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup(%d)): Start...\n",bVMode);
	idclk_freq = VModeTables[bVMode].PixClk;

	bRegVal = hlReadByte_8BA ( TX_SLV0, TX_TMDS_CTRL_ADDR ) & 0x60;
	switch (bRegVal) {
		case 0x00:
			tclk = x0_5;
			DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): 0.5x tclk\n"); break;
		default:
		case 0x20:
			tclk = x1;
			DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): 1.0x tclk\n"); break;
		case 0x40:
			tclk = x2;
			DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): 2.0x tclk\n"); break;
		case 0x60:
			tclk = x4;
			DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): 4.0x tclk\n"); break;
	}

	bRegVal = hlReadByte_8BA ( TX_SLV0, VID_ACEN_ADDR );
	bRegVal = bRegVal & (~VID_ACEN_DEEP_COLOR_CLR);
	bRegVal = bRegVal >> 6;

	iLowRange = 25;
	switch (bRegVal) {
		case SiI_DeepColor_24bit:
			nFFBCOUNT = 0x03;
			iMidRange1 =  64;
			iMidRange2 = 126;
			iHghRange  = 270;
			break;
		case SiI_DeepColor_30bit:
			nFFBCOUNT = 0x04;
			iMidRange1 =  53;
			iMidRange2 = 104;
			iHghRange  = 203;
			break;
		case SiI_DeepColor_36bit:
			nFFBCOUNT = 0x05;
			iMidRange1 =  44;
			iMidRange2 =  86;
			iHghRange  = 168;
			break;
	}
	// Set FFBCount field in 0x72:0x83:
	// NOTE: UP TO BUILD 14, nFFBCOUNT value caldulted in the switch statement above was not used.
	// Instead, the code forced it to 0x03 (line 303 below).This line sets it to the calculated value.
	// Line 303 was modified accordingly. Consult DD.
	bRegVal2 = hlReadByte_8BA ( TX_SLV0, TX_TMDS_CTRL2_ADDR );
	bRegVal2 &= CLR_BITS_5_4_3;
	bRegVal2 |= (nFFBCOUNT << 3);
	hlWriteByte_8BA ( TX_SLV0, TX_TMDS_CTRL2_ADDR, bRegVal2 );

	nIPLLF = 0x01;
	switch (tclk) {
		case x0_5:  nFPOSTCOUNT = 0x07; break;
		case x1:    nFPOSTCOUNT = 0x03; break;
		case x2:    nFPOSTCOUNT = 0x01; break;
		case x4:    nFPOSTCOUNT = 0x00; break;
	}

	DEBUG_PRINTK("idclk_freq = %d,iLowRange= %d, iHghRange = %d\n",idclk_freq,iLowRange,iHghRange);

	// Out of Range
	if ((idclk_freq < iLowRange) || (idclk_freq > iHghRange)) {
		DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): Out of Range\n");
		return TMDS_SETUP_FAILED;
	}

	// Blue range
	if ((idclk_freq >= iLowRange) && (idclk_freq <= iMidRange1)) {
		nFFRCOUNT = 0x00;
		SiI_Mpll_setup(blue);
		SiI_FApost_setup(blue, idclk_freq, bRegVal);
	}
	else
		// Yellow range
		if ((idclk_freq > iMidRange1) && (idclk_freq <= iMidRange2)) {
			if (tclk == x4) {
				DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): Yellow range\n");
				return TMDS_SETUP_FAILED;
			}
			nFFRCOUNT = 0x01;
			nFPOSTCOUNT >>= 1;
			SiI_Mpll_setup(yellow);
			SiI_FApost_setup(yellow, idclk_freq, bRegVal);
		}
		else
			// Orange range
			if ((idclk_freq > iMidRange2) && (idclk_freq <= iHghRange)) {
				if ((tclk == x4) || (tclk == x2)) {
					DEBUG_PRINTK ("[TXVIDP.C](SiI_TMDS_setup): Orange range\n");
					return TMDS_SETUP_FAILED;
				}
				nFFRCOUNT = 0x03;
				nFPOSTCOUNT >>= 2;
				SiI_Mpll_setup(orange);
				SiI_FApost_setup(orange, idclk_freq, bRegVal);
			}

	// TX_SYS_CTRL4_ADDR
	//  [7:5]   reserved
	//  [4:1]   IPLLF = 0x01*
	//  [0]     reserved
	hlWriteByte_8BA (TX_SLV0, TX_SYS_CTRL4_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_SYS_CTRL4_ADDR ) & 0xE1 ) | (0x01 << 1)));

	// TX_TMDS_CTRL2_ADDR
	//  [7:6]   TPOSTCOUNT
	//  [5:3]   FFBCOUNT = 0x03*
	//  [2:0]   FFRCOUNT*
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CTRL2_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CTRL2_ADDR ) & 0xF8) | (nFFRCOUNT)));  // Use calculated nFFBCOUNT value rather
	// than 0x03. Value set after the
	// "switch (bRegVal)" statement in this function.
	//Consult DD.
	// TX_TMDS_CTRL3_ADDR
	//  [7]     reserved
	//  [6:3]   ITPLL
	//  [2:0]   FPOSTCOUNT*
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CTRL3_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CTRL3_ADDR ) & 0xF8)  | (nFPOSTCOUNT)));

	return TMDS_SETUP_PASSED;
}

//---------------------------------------------------------------------------
// SiI_Mpll_setup
//---------------------------------------------------------------------------
void SiI_Mpll_setup( BYTE MpllSet ){
	BYTE itpll, tpostcount, tfrcount;

	itpll = 0x06;
	switch (MpllSet) {
		default:
		case blue:
			tpostcount = 0x02;
			tfrcount   = 0x00;
			break;
		case yellow:
			tpostcount = 0x01;
			tfrcount   = 0x01;
			break;
		case orange:
			tpostcount = 0x00;
			tfrcount   = 0x02;
			break;
	}
	// TX_TMDS_CTRL2_ADDR
	//  [7:6]   TPOSTCOUNT*
	//  [5:3]   FFBCOUNT
	//  [2:0]   FFRCOUNT
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CTRL2_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CTRL2_ADDR ) & 0x3F) | (tpostcount << 6)));

	// TX_TMDS_CTRL3_ADDR
	//  [7]     reserved
	//  [6:3]   ITPLL*
	//  [2:0]   FPOSTCOUNT
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CTRL3_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CTRL3_ADDR ) & 0x87)  | (itpll << 3)));

	// TX_TMDS_CTRL4_ADDR
	//  [7:2]   reserved
	//  [1:0]   TFRPOSTCOUNT*
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CTRL4_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CTRL4_ADDR) & 0xFC) | (tfrcount)));
}

//---------------------------------------------------------------------------
// SiI_FApost_setup
//---------------------------------------------------------------------------
void SiI_FApost_setup( BYTE RangeSet, int idclk_freq, BYTE bpp ){
	BYTE nFAPOSTCOUNT = 0;
	switch (RangeSet) {
		default:
		case blue:
			switch (bpp) {
				default:
				case SiI_DeepColor_Off:
				case SiI_DeepColor_24bit: if (idclk_freq >= 44) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_30bit: if (idclk_freq >= 33) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_36bit: if (idclk_freq >= 30) nFAPOSTCOUNT = 1; break;
			}
			break;
		case yellow:
			switch (bpp) {
				default:
				case SiI_DeepColor_Off:
				case SiI_DeepColor_24bit: if (idclk_freq >= 86) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_30bit: if (idclk_freq >= 71) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_36bit: if (idclk_freq >= 58) nFAPOSTCOUNT = 1; break;
			}
			break;
		case orange:
			switch (bpp) {
				default:
				case SiI_DeepColor_Off:
				case SiI_DeepColor_24bit: if (idclk_freq >= 168) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_30bit: if (idclk_freq >= 139) nFAPOSTCOUNT = 1; break;
				case SiI_DeepColor_36bit: if (idclk_freq >= 114) nFAPOSTCOUNT = 1; break;
			}
			break;
	}
	// TX_TMDS_CCTRL_ADDR
	//  [7:6]   reserved
	//  [5]     FAPOSTCOUNT*
	//  [4:0]   reserved
	hlWriteByte_8BA (TX_SLV0, TX_TMDS_CCTRL_ADDR, ((hlReadByte_8BA( TX_SLV0, TX_TMDS_CCTRL_ADDR) & 0xDF) | (nFAPOSTCOUNT << 5)));
}

//---------------------------------------------------------------------------
void AssertHDMITX_SWReset( BYTE SoftReset ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, TX_SWRST_ADDR );
	RegVal |= SoftReset;
	hlWriteByte_8BA( TX_SLV0, TX_SWRST_ADDR, SoftReset);
}

//---------------------------------------------------------------------------
void ReleaseHDMITX_SWReset( BYTE SoftReset ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, TX_SWRST_ADDR );
	RegVal &= (~SoftReset);
	hlWriteByte_8BA( TX_SLV0, TX_SWRST_ADDR, 0);
}

//---------------------------------------------------------------------------
void siiTX_SW_Reset( void ){
	//BYTE TimeOut = 255;

	//while ( !(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_P_STABLE ) && TimeOut-- ) {};         // wait for input pixel clock to stabilze
	//if (TimeOut){
	AssertHDMITX_SWReset(BIT_TX_SW_RST | BIT_TX_FIFO_RST);
	//halDelayMS(1);
	udelay(1000);
	ReleaseHDMITX_SWReset(BIT_TX_SW_RST | BIT_TX_FIFO_RST);
	// halDelayMS(64);          // allow TCLK (sent to Rx across the HDMS link) to stabilize
	mdelay(64);           // Du liguo commet this for speedup
	//}
}

//---------------------------------------------------------------------------
// Function Name: siiSetVideoPath
// Function Description: This functiom sets video path, for for different Input/Output
//                       Video formats, it doesn't include setting Resolution dependent parameters
// Accepts: poinetr on Video path parameters
// Returns: none
// Globals: none
//---------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////
// siiTXInputVideoType: 0 - RGB24; 1 - RGB DVO 12; 2 - YCbCr24; 3 - YC24; 4 - YCbCr4:2:2
// siiTXOutputVideoType: 0 - RGB out; 1 - YCbr (4:4:4) out
// siiTXInputClockEdge: 0 - Falling edge; 1 - Rising Edge
// siiTXOutputColorDepth: 0 - 24 bit; 1 - 30 bits; 2 - 36 bits
////////////////////////////////////////////////////////////////////////////////////////////
void siiTXSetVideoPath ( void )
{
	static BYTE InfCtrl1, InfCtrl2;

	siiTXConfigVideoMode( siiTX.siiTXInputVideoType, siiTX.siiTXOutputVideoType );
	siiTXSetInputClockEdge ( siiTX.siiTXInputClockEdge ); //0x01: Clock rising edge; 0x00: falling edge
	//siiTXSetDeepColor ( siiTX.siiTXOutputColorDepth );
	InfCtrl1 = hlReadByte_8BA( TX_SLV1, INF_CTRL1);    // save packet buffer control regs
	InfCtrl2 = hlReadByte_8BA( TX_SLV1, INF_CTRL2);

	siiTX_SW_Reset();               // Reset internal state machines and allow TCLK to Rx to stabilize

	hlWriteByte_8BA( TX_SLV1, INF_CTRL1, InfCtrl1 );        // Retrieve packet buffer control regs
	hlWriteByte_8BA( TX_SLV1, INF_CTRL2, InfCtrl2 );
}

//---------------------------------------------------------------------------
BYTE FindFsFromSPDIF( void ){
	BYTE Fs;
	hlReadByte_8BA( TX_SLV1,SPDIF_CTRL_ADDR);
	Fs = hlReadByte_8BA( TX_SLV1, SPDIF_HWFS_ADDR) & 0x0F; // Fs extracted from the S/PDIF input channel status bits 24-27.
	//hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, 0x03);     // En.SPDIF
	return Fs;
}

//extern pthread_mutex_t hdmi_mutex;

void csmTXDisableAudio(void)
{
	//BYTE bRegVal;

	//   	pthread_mutex_lock(&hdmi_mutex);

	//	bRegVal = hlReadByte_8BA( TX_SLV1, AUD_MODE_ADDR);      // 0x7A:0x14
	//	bRegVal &= ~BIT_EN_AUDIO; 							  // disable audio input stream
	//	hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, bRegVal);
	//	hlWriteByte_8BA( TX_SLV0, DATA_CTRL_ADDR, MUTE_AUDIO); // disable output audio packets
	//	printk("zzzzzzzz\n");
	//	sleep(1);
	//	bRegVal = hlReadByte_8BA( TX_SLV1, INF_CTRL1);
	//	printk("xxxxxxxxx\n");
	//	bRegVal&= ~0x20;
	//	sleep(1);
	//	hlWriteByte_8BA(TX_SLV1, INF_CTRL1, bRegVal);
	//	printk("cccccccccccc\n");

	//	bRegVal = hlReadByte_8BA( TX_SLV1, ACR_CTRL_ADDR);
	//	bRegVal&= ~0x2;
	//	hlWriteByte_8BA(TX_SLV1, ACR_CTRL_ADDR, bRegVal);
	//	udelay(6000);
}

void csmTXEnableAudio(void)
{
	//	BYTE bRegVal;
	//	printk("rrrrrrrrrrrrr\n");
	//	bRegVal = hlReadByte_8BA( TX_SLV1, AUD_MODE_ADDR);      // 0x7A:0x14
	//	bRegVal |= BIT_EN_AUDIO; 							  // disable audio input stream
	//	hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, bRegVal);
	//	hlWriteByte_8BA( TX_SLV0, DATA_CTRL_ADDR, ENABLE_ALL); // disable output audio packets

	//	bRegVal = hlReadByte_8BA( TX_SLV1, INF_CTRL1);
	//	bRegVal |= 0x20;
	//	hlWriteByte_8BA(TX_SLV1, INF_CTRL1, bRegVal);

	//	bRegVal = hlReadByte_8BA( TX_SLV1, ACR_CTRL_ADDR);
	//	bRegVal |= 0x2;
	//	hlWriteByte_8BA(TX_SLV1, ACR_CTRL_ADDR, bRegVal);
	//	udelay(6000);
	//	pthread_mutex_unlock(&hdmi_mutex);
}

//---------------------------------------------------------------------------
// Function Name: siiTXSetAudioPath
// Function Description: This functiom selects audio path, for I2S/DSD oe SPDIF audio
//                       for I2S DSD Audio channel status registers must be set (Fs/length)
//                       for I2S input config must be set
//
// Called ONLY when TxAPI is enabled.
//
// Accepts: Audio path parameters
// Returns: none
// Globals: none
//---------------------------------------------------------------------------
void siiTXSetAudioPath ( void ){
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// siiTXInputAudioType:  0 - SPDIF;  1 - I2S;    2 - DSD;    3 - HBR
	// siiTXInputAudioChannel: 1 - 2 channels; 2 - 3 channels; 3 - 4 channels; 4 - 5 channels;
	//                                     5 - 6 channels; 6 - 7 channels; 7 - 8 channels;
	// siiTXInputAudioFs:  0 - 44KHz;   2 - 48KHz ...
	// siiTXInputAudioSampleLength: 2, 4, 8, 0xA, 0xC +1 for Max 24. Even values are Max 20. Odd: Max 24.
	// abAudioPath[3] = I2S control bits (for 0x7A:0x1D)
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	BYTE bRegVal1, bRegVal2, bRegVal3;
	BYTE bAudioModes;

	// Disable audio to allow FIFO to flush:
	bRegVal1 = hlReadByte_8BA( TX_SLV1, AUD_MODE_ADDR);      // 0x7A:0x14
	bRegVal1 &= ~BIT_EN_AUDIO; 							  // disable audio input stream
	hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, bRegVal1);
	bRegVal3 = hlReadByte_8BA( TX_SLV0, DATA_CTRL_ADDR);
	bRegVal3 |= MUTE_AUDIO;
	hlWriteByte_8BA( TX_SLV0, DATA_CTRL_ADDR, bRegVal3); // disable output audio packets

	hlWriteByte_8BA( TX_SLV1, I2S_CHST4_ADDR, siiTX.siiTXInputAudioFs ); //Set FS

	bAudioModes = siiTX.siiTXInputAudioType & SiI_AudioModesSelect;   // Input audio format

	bRegVal1 = hlReadByte_8BA( TX_SLV1, AUD_MODE_ADDR );          // 0x7A:0x14
	bRegVal2 = hlReadByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR );       // 0x7A:0x2F

	if (bAudioModes == SiI_SPDIF){ //SPDIF input
		bRegVal2 &= (~BIT_LAYOUT1);
		bRegVal1 &= (~SD_0_3_EN);
		bRegVal1 |= BIT_SPDIF_SELECT;
	}

	else{                                // not SPDIF
		bRegVal3 = siiTX.siiTXInputAudioSampleLength;             // Input I2S sample length
		hlWriteByte_8BA( TX_SLV1, I2S_IN_LEN, bRegVal3);

		bRegVal1 &= (~BIT_SPDIF_SELECT);
		bRegVal1 &= (~SD_0_3_EN);       // Clear bits 7:4

		if (siiTX.siiTXInputAudioChannel > 1){
			bRegVal1 |= SD_0_3_EN;          // All other modes need to enable I2S channel inputs
			bRegVal2 |= BIT_LAYOUT1;
		}
		else{
			bRegVal1 |= SD_0;          // All other modes need to enable I2S channel inputs
			bRegVal2 &= (~BIT_LAYOUT1);
		}
		if (bAudioModes == SiI_DSD)   // DSD Audio
			bRegVal1 |= BIT_DSD_SELECT;

		else{                            // Not DSD Audio. Could be I2S or HBR
			bRegVal1 &= (~BIT_DSD_SELECT);

			if (bAudioModes == SiI_I2S){
				hlWriteByte_8BA( TX_SLV1, I2S_CHST4_ADDR, siiTX.siiTXInputAudioFs );   // Fs
				hlWriteByte_8BA( TX_SLV1, I2S_CHST5_ADDR, (siiTX.siiTXInputAudioSampleLength | ((siiTX.siiTXInputAudioFs << 4) & 0xF0)));   // "Original Fs" and Length
				hlWriteByte_8BA( TX_SLV1, I2S_IN_CTRL_ADDR, 0x41 );
			}
			else{                        // HBRA
				bRegVal3 = (BIT_HBR_ON | SCK_RISING | BIT_CBIT_ORDER | BIT_COMPRESSED);
				hlWriteByte_8BA( TX_SLV1, I2S_IN_CTRL_ADDR, bRegVal3);		     	// Write 0xF0 to 0x7A:0x1D

				bRegVal1 = SETUP_ENABLE_HBRA;                       		                            // Write 0xF1 to 0x7A:0x14
				hlWriteByte_8BA( TX_SLV1, I2S_IN_LEN, HBRA_IN_CTRL_VAL);   		// Write 0x92 to 0x7A:0x24
				bRegVal2 = HBRA_ZERO_PLUS;                          		                            // Write 0x01 to0x7A:0x2F

				hlWriteByte_8BA( TX_SLV1, I2S_CHST0_ADDR, NON_PCM_TYPE ); 			// Write 0x04 to0x7A:0x1E to set 0x1E[1] (workaround)
				hlWriteByte_8BA( TX_SLV1, I2S_CHST4_ADDR, HBRA_FOR_CHST4 ); 		// Write 0x09 to0x7A:0x21
				hlWriteByte_8BA( TX_SLV1, I2S_CHST5_ADDR, HBRA_FOR_CHST5 ); 		// Write 0xE2 to0x7A:0x22
				hlWriteByte_8BA( TX_SLV1, SAMPLE_RATE_CONVERSION, HBR_SPR_MASK);	// Write 0x0 to 0x7A:0x23
			}
		}
	}

	//halDelayMS(6); 										// allow FIFO to flush
	mdelay(6);
	bRegVal1 |= BIT_EN_AUDIO; 								// Enable audio
	hlWriteByte_8BA( TX_SLV1, AUD_MODE_ADDR, bRegVal1 );
	hlWriteByte_8BA( TX_SLV1, AUDP_TXCTRL_ADDR , bRegVal2 );

	bRegVal3 = hlReadByte_8BA( TX_SLV0, DATA_CTRL_ADDR);
	bRegVal3 &= ~MUTE_AUDIO;
	hlWriteByte_8BA( TX_SLV0, DATA_CTRL_ADDR, bRegVal3); // Enable output audio packets
}

//---------------------------------------------------------------------------
void siiTXWakeUp( BOOL qOn ){
	BYTE RegVal;

	RegVal = hlReadByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR );
	if( qOn ){
		hlWriteByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR, RegVal | BIT_TX_PD);
		hlWriteByte_8BA( TX_SLV0, INT_CNTRL_ADDR, INT_CONTROL);  // Interrupt pin control defined per board
	}
	else
		hlWriteByte_8BA( TX_SLV0, TX_SYS_CTRL1_ADDR, RegVal & ~BIT_TX_PD);
}

//---------------------------------------------------------------------------
void siiInitDVIInterruptMasks( void )
{
	// make enable VSync, Ri check and HotPlug Interrupts
	siiModifyBits( TX_SLV0, HDMI_INT1_ADDR, DVI_CLR_MASK, SET );
	siiModifyBits( TX_SLV0, HDMI_INT1_MASK, DVI_CLR_MASK, SET );
}

//---------------------------------------------------------------------------
// Function Name: siiTXSystemInit
// Function Description: This functiom initializes SIITX to workable mode before a complete initialization.
//---------------------------------------------------------------------------
void siiTXSystemInit( void ){
	siiTXHardwareReset();  //Hardware reset.
	siiTXInitParameter();
}

//---------------------------------------------------------------------------
// Function Name: siiTX_DVI_Init
// Function Description: This functiom initializes SIITX to DVI mode.
//---------------------------------------------------------------------------
void siiTX_DVI_Init( void ){
	BYTE bError;

	siiTXHardwareReset();

	// FPLL is 1.0*IDCK/Internal source termination enabled/Driver level shifter bias enabled.
	hlWriteByte_8BA( TX_SLV0, TX_TMDS_CTRL_ADDR, 0x25 );

	siiTXSetToHDMIMode( OFF );       // Reset to DVI mode

	siiTXWakeUp( ON );
	siiTX_SW_Reset();

	siiTXDEconfig();
	//siiTXSetIClk( siiTX.siiTXInputPixRepl ); //Set pixel data replication according to RX AVI.
	siiTX.siiTXInputVideoType = 2;//majia add this for DVI mode
	siiTX.siiTXOutputVideoType = 0;// majia add this for DVI mode
	siiTXSetVideoPath();

	DEBUG_PRINTK ("[SiITX.C]: Call siiTX_TMDS_Setup()\n");
	bError = siiTX_TMDS_Setup ( siiTX.siiTXInputVideoMode );
	if( bError == TMDS_SETUP_FAILED ){
		DEBUG_PRINTK ("[SiITX.C]: TMDS_SETUP_FAILED\n");
	}

	siiInitDVIInterruptMasks();

	siiTX.siiTXOutputState = CABLE_PLUGIN_DVI_OK;
	siiTX.siiTXOutputMode = DVI_Mode;
	DownStreamAuthState = REQ_AUTHENTICATION; // downstream HDCP auth start
	DEBUG_PRINTK ("[SiITX.C]: DVI Init Done.\n");
	//halDelayMS(50);
	mdelay(50);
}

//---------------------------------------------------------------------------
// Function Name: siiTX_HDMI_Init
// Function Description: This functiom initializes SIITX to HDMI mode.
//---------------------------------------------------------------------------
void siiTX_HDMI_Init( void )
{
    DEBUG_PRINTK("@@@siiTX_HDMI_Init\n");
	siiTXHardwareReset();

#if 1
	if (CECIsSupported())
	{
		if (SI_CecGetManualSetCaptureIDFlag())
			SI_CecSetCaptureIDToHW();

		if (!SI_CecGetManualSetLAFlag())
			SI_CecEnumerate();
		else
			SI_CecSetSrcLAToHW();
	}
#endif

	// 2012-5-29 added by xp, set the default output color to black when HDCP authentication not success.
	siiTXSetVideoBlank(0x80, 0x10, 0x80);

	// FPLL is 1.0*IDCK/Internal source termination enabled/Driver level shifter bias enabled.
	hlWriteByte_8BA( TX_SLV0, TX_TMDS_CTRL_ADDR, 0x25 );

	if( (siiTX.siiTXOutputState == CABLE_PLUGIN_HDMI_OUT) ||
			(siiTX.siiTXOutputState == CABLE_PLUGIN_HDMI_OK) )
	{
		DEBUG_PRINTK("[SiITX.C]: Set TX to HDMI mode.\n");
		siiTXSetToHDMIMode(ON);
	}
	else if( (siiTX.siiTXOutputState == CABLE_PLUGIN_DVI_OUT) ||
			(siiTX.siiTXOutputState == CABLE_PLUGIN_DVI_OK) )
	{
		DEBUG_PRINTK("[SiITX.C]: Set TX to DVI mode.\n");
		siiTX_DVI_Init();
		return;
	}

	siiTXInitAudioPart();

	siiTXDEconfig();
	//siiTXSetIClk( siiTX.siiTXInputPixRepl ); //Set pixel data replication according to RX AVI.

	//Video path setup.
	DEBUG_PRINTK("[SiITX.C]: siiTXInputVideoMode is %d, siiTXOutputColorDepth is %d.\n", (int)siiTX.siiTXInputVideoMode, (int)siiTX.siiTXOutputColorDepth);
	siiTXSetVideoPath();
#if 0
	hlWriteByte_8BA( TX_SLV0, 0x48, 0x30);

	if((siiTX.siiTXInputVideoMode == 4) ||(siiTX.siiTXInputVideoMode == 15))  //for 480i and 576i
		hlWriteByte_8BA( TX_SLV0, 0x4a, 0x3f);
	else
		hlWriteByte_8BA( TX_SLV0, 0x4a, 0x3d);
#endif
	//Audio Infoframe packet setup.
	siiTXSetAudioInfoFramePacket();

	//Audio path setup.
	siiTXSetAudioPath();

	siiTX_SW_Reset();       //Must be done AFTER setting up audio and video paths and BEFORE starting to send InfoFrames.

	//AVI Infoframe packet setup.
	siiTXSetAVIInfoFramePacket();

	DEBUG_PRINTK ("[SiITX.C]: Call WakeUpHDMITX.\n");
	siiTXWakeUp( ON );
	siiTXEnableInfoFrame( AVI_TYPE ); //InfoFrame cannot be enabled at Power Down mode.
	siiTXSendCP_Packet( ON );
	siiTXSetEncryption( OFF );     //Must turn encryption off when AVMUTE

	if( (GetSiITXRevId() >= 0x02) ) 	// old version doesn't support new infopackets
	{
		siiTXSendInfoPacket( (unsigned char *)AudioCP, 31, MPEG_TYPE, 0 );
		siiTXSendInfoPacket( (unsigned char *)ISRC2, 31, GENERIC2_TYPE, 0 );
		siiTXSendInfoPacket( (unsigned char *)ISRC1, 31, GENERIC1_TYPE, 1 );
		siiTXDisableInfoPacket( GENERIC1_TYPE );
		siiTXEnableInfoPacket ( GENERIC1_TYPE );
	}

	siiTXDisableInfoFrame( AUD_TYPE );
	siiTXEnableInfoFrame( AUD_TYPE );

	// Clear and Enable Interrupts: Drop_Sample, Bi_Phase_Err
	siiModifyBits( TX_SLV0, HDMI_INT1_ADDR, CLR_MASK, SET );
	siiModifyBits( TX_SLV0, HDMI_INT1_MASK, CLR_MASK, SET );

	siiTXSendCP_Packet( OFF );

	siiTX.siiTXOutputState = CABLE_PLUGIN_HDMI_OK;
	siiTX.siiTXOutputMode = HDMI_Mode;
	DownStreamAuthState = REQ_AUTHENTICATION; // downstream HDCP auth start
	DEBUG_PRINTK ("[SiITX.C]: HDMI Init Done.\n");
	//halDelayMS(50);
	mdelay(50);
}

//---------------------------------------------------------------------------
static void siiTXHPDInterrupt( BOOL qOn )
{
	if( qOn )
		siiModifyBits( TX_SLV0, HDMI_INT1_MASK, BIT_INT_HOT_PLUG, SET );
	else
		siiModifyBits( TX_SLV0, HDMI_INT1_MASK, BIT_INT_HOT_PLUG, CLR );
}

//---------------------------------------------------------------------------
void HotPlugInOrOut( void )
{
	BYTE i, HPDCounter = 0;
	//DEBUG_PRINTK( " siiTX.siiTXOutputState = %d\n\n",siiTX.siiTXOutputState);
	if( (siiTX.siiTXOutputState == CABLE_UNPLUG_)
			&& (hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN) )
		//if(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN)
		//end modify.......................................
	{
		for( i=0; i<5; i++ )
		{
			// in order not to fail bouncing detection.
			if( hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN )
				HPDCounter++;
			mdelay(10);
		}
		if( HPDCounter >= 4 )
		{
			siiTX.siiTXOutputState = CABLE_PLUGIN_CHECK_EDID;//CABLE_PLUGIN_CHECK_EDID;majia
			clock_tms_clockena(1);
			DEBUG_PRINTK("clock_tms_clockena(1)\n");
			mdelay(10);
			siiTXWakeUp( ON ); // Oscar Ding Added on 20080911.
			siiTXHPDInterrupt( ON );
			DEBUG_PRINTK( "Sink CABLE_PLUGIN_CHECK_EDID.\n");
            hdmi_wake_up(HDMI_EVENT_HPD_HIGH);
		}
		return;
	}
}

//---------------------------------------------------------------------------
void siiTXHotPlugInOrOutHandler( void ){
	BYTE i, HPDCounter = 0;

	if( (siiTX.siiTXOutputState != CABLE_UNPLUG_)
			&& (!(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN)) )
	{
		for( i=0; i<5; i++ )
		{
			// in order not to fail bouncing detection.
			if( !(hlReadByte_8BA( TX_SLV0, TX_STAT_ADDR ) & BIT_HPD_PIN) )   // Oscar Ding Added 20080917
				HPDCounter++;
			mdelay(10);
		}

		if( HPDCounter >= 4 )
		{
			siiTX.siiTXOutputState = CABLE_UNPLUG_;
			siiTX.siiTXOutputMode = None_Mode;
			siiTXHPDInterrupt( OFF );
			siiTXWakeUp( OFF );
			DEBUG_PRINTK( "Sink CABLE_UNPLUG_.\n");
			clock_tms_clockena(0);
			DEBUG_PRINTK("clock_tms_clockena(0)\n");
            hdmi_wake_up(HDMI_EVENT_HPD_LOW);
		}
	}
}

//---------------------------------------------------------------------------
BYTE CheckFIFOReady( void )
{
	BYTE RegVal;

	MDDCBlockReadHDCPRX( 1, DDC_BCAPS_ADDR, &RegVal );
	return RegVal & DDC_BIT_FIFO_READY;
}

//---------------------------------------------------------------------------
BYTE CheckEXCEEDED( void )
{
	BYTE BStatus[2];

	MDDCBlockReadHDCPRX( 2, DDC_BSTATUS1_ADDR, BStatus );
	return (BStatus[0] & DDC_BIT_BSTATUS1_DEV_EXC) || (BStatus[1] & DDC_BIT_BSTATUS2_CAS_EXC);
}


//---------------------------------------------------------------------------
void HDCPHandler( void )
{
	BYTE Match;

	AreR0sMatch( &Match );
	if( !Match )
		DownStreamAuthState = REQ_AUTHENTICATION;
}

//---------------------------------------------------------------------------
// Function Name: siiTXInterruptHandler
// Function Description:
//---------------------------------------------------------------------------
void siiTXInterruptHandler( void )
{
	BYTE IntReg[2];

	if( hlReadByte_8BA( TX_SLV0, HDMI_INT_STATE) && (siiTX.siiTXOutputState != CABLE_UNPLUG_) )
	{
		siiReadBlockHDMI( TX_SLV0, HDMI_INT1_ADDR, 2, IntReg );
		//Clear interrupt.
		siiWriteBlockHDMI( TX_SLV0, HDMI_INT1_ADDR, 2, IntReg);

		//HPD interrupt handler.
		if( IntReg[0] & BIT_INT_HOT_PLUG )
		{
			siiTXHotPlugInOrOutHandler();
		}

		//HDCP Ri check.
		if( (IntReg[0] & BIT_INT_Ri_CHECK)
				&& (DownStreamAuthState == AUTHENTICATED) )
		{
			HDCPHandler();
		}
		//SPDIF error check.
		if((IntReg[0] & BIT_BIPHASE_ERROR) ||(IntReg[0] & BIT_DROP_SAMPLE) )
			siiTXInitAudioPart();

		//Clear interrupt.
		siiWriteBlockHDMI( TX_SLV0, HDMI_INT1_ADDR, 2, IntReg);
	}

}

void siiTXTaskHandler( void )
{
	BYTE DVICount = 0;

	//printk("@@@@ siiTX.siiTXOutputState is %d\n", siiTX.siiTXOutputState);
	siiTXInterruptHandler();

	//polling HPD stauts bit for detecting downstream device is attached or detached.
	HotPlugInOrOut();

	if( siiTX.siiTXOutputState == CABLE_PLUGIN_CHECK_EDID )
	{
		EDIDProcessing();
	}
#if 0
	while (siiTX.siiTXOutputState == CABLE_PLUGIN_DVI_OUT)
	{
		siiTX.siiTXOutputState = CABLE_PLUGIN_CHECK_EDID;
		EDIDProcessing();
		DVICount++;
		if (DVICount == 3)
			break;
	}
#endif
	if( (siiTX.siiTXOutputState == CABLE_PLUGIN_HDMI_OUT) || (siiTX.siiTXOutputState == CABLE_PLUGIN_DVI_OUT) )
	{
        //printk("@@@siiTX.siiTXOutputState is %s\n", (siiTX.siiTXOutputState == CABLE_PLUGIN_HDMI_OUT)?"CABLE_PLUGIN_HDMI_OUT":"CABLE_PLUGIN_DVI_OUT");
		if(siiTX.is_first_time == 0)
		{
			DEBUG_PRINTK("is_first_time = %d\n", siiTX.is_first_time);
			siiTX_HDMI_Init();
		}
        //printk("@@@siiTX.is_first_time is %d\n", siiTX.is_first_time);
	}

	if (siiTX.siiTXOutputState == CABLE_PLUGIN_HDMI_OK || siiTX.siiTXOutputState == CABLE_PLUGIN_DVI_OK)
	{
		if (CECIsSupported())
		{
			SI_CecHandler(0, 0);
		}
	}

	// HDCP
	//printk("@@@start to check HDCP, now siiTX.siiTXOutputMode is %d, siiTX.siiTXHDCPEnable is %d\n", siiTX.siiTXOutputMode, siiTX.siiTXHDCPEnable);
	if( (siiTX.siiTXOutputMode != None_Mode) && (siiTX.siiTXHDCPEnable != 0) )
	{
		if( DownStreamAuthState == REQ_AUTHENTICATION )
		{
			siiTXHDCP_AuthenticationPart1();
			if( (DownStreamAuthState != AUTHENTICATED) && (DownStreamAuthState != REPEATER_AUTH_REQ) )
			{
				DownStreamAuthState = REQ_AUTHENTICATION;
#if 0
				DSAuthPart1ErrorCounter++;

				if( DSAuthPart1ErrorCounter == 5 )
				{
					DSAuthPart1ErrorCounter = 0;
					DownStreamAuthState = R0s_ARE_MISSMATCH;
					siiTXSendCP_Packet( ON );
					siiTXSetEncryption( OFF );     //Must turn encryption off when AVMUTE
				}
#endif
			}
		}

		//Downstream HDCP KSV ready.
		if( (DownStreamAuthState == REPEATER_AUTH_REQ) && (siiTXHDCP_GetAuthenticationPart2WDTime() <= 5000) )
		{
			if( CheckFIFOReady() )
			{
				if ( CheckEXCEEDED() )
				{
					AssertedDSAuthPart2 = 0;
					TimeOutForDSAuthPart2 = 0;
					siiTXSendCP_Packet( ON );
					siiTXSetEncryption( OFF );
					DownStreamAuthState = REQ_AUTHENTICATION;
				}
				else
				{
					DownStreamAuthState = REQ_SHA_CALC;
					AssertedDSAuthPart2 = 0;
					TimeOutForDSAuthPart2 = 0;
				}
			}
		}

		if( DownStreamAuthState == REQ_SHA_CALC )
		{
			DownStreamAuthState = SHAHandler();
			printAuthState();

			if( DownStreamAuthState != AUTHENTICATED)
			{
				AssertedDSAuthPart2 = 0;
				TimeOutForDSAuthPart2 = 0;
				siiTXSendCP_Packet( ON );
				siiTXSetEncryption( OFF );
				DownStreamAuthState = REQ_AUTHENTICATION;
#if 0
				DSAuthPart2ErrorCounter++;

				if( DSAuthPart2ErrorCounter == 5 )
				{
					DSAuthPart2ErrorCounter = 0;
					DownStreamAuthState = FAILED_ViERROR;
					siiTXSendCP_Packet( ON );
					siiTXSetEncryption( OFF );     //Must turn encryption off when AVMUTE
				}
#endif
			}
			else
			{
				AssertedDSAuthPart2 = 0;
				TimeOutForDSAuthPart2 = 0;
			}
		}

		if( AssertedDSAuthPart2 && siiTXHDCP_GetAuthenticationPart2WDTime() > 5000 )
		{
			// 5s Time Out For DS Auth Part2
			DEBUG_PRINTK ("DSAuthPart2 TimeOut 5 seconds.\n");
			AssertedDSAuthPart2 = 0;
			TimeOutForDSAuthPart2 = 0;
			siiTXSendCP_Packet( ON );
			siiTXSetEncryption( OFF );     //Must turn encryption off when AVMUTE
			DownStreamAuthState = REQ_AUTHENTICATION;
		}
	}

	//DEBUG_PRINTK("Now Out Task Processing\n");
}

