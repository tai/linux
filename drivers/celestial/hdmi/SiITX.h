//------------------------------------------------------------------------------
// Module Name: SiITX.h
// Module Description: SiITX header file
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#include "TypeDefs.h"
#include "SiIIIC.h"

#define GetSiITXRevId()  hlReadByte_8BA( TX_SLV0, 0x04)

#define InputClockFallingEdge       0x00
#define InputClockRisingEdge        0x01
#define CLR_BITS_7_6		        0x3F
#define CLR_BITS_5_4_3		        0xC7

#define TMDS_SETUP_FAILED	        1
#define TMDS_SETUP_PASSED	        0

#define BIT_DC_EN				    0x40
#define PACKET_MODE				    0x18
#define THIRTY_BITS_PER_PIXEL	    0x08
#define THIRTY_SIX_BITS_PER_PIXEL	0x10
#define MUTE_AUDIO					0x02
#define ENABLE_ALL					0x00

#define INT_CONTROL                 0x06 // Interrupt pin is open drain and active low (this is normally 0x06)
#define CLR_MASK         BIT_INT_HOT_PLUG | BIT_BIPHASE_ERROR | BIT_DROP_SAMPLE
//#define CLR_MASK         BIT_BIPHASE_ERROR | BIT_DROP_SAMPLE
#define DVI_CLR_MASK BIT_INT_HOT_PLUG

// siiTXOutputMode
#define None_Mode    0x00
#define DVI_Mode     0x01
#define HDMI_Mode    0x02

// siiTXDEMode
#define  INTERNAL_DE 0x00
#define  EXTERNAL_DE 0x01
#define  EMBEDED_SYNC 0x02

// siiTXOutputState
#define CABLE_UNPLUG_ 0
#define CABLE_PLUGIN_CHECK_EDID 1
#define CABLE_PLUGIN_CHECK_EDID_OK 2
#define CABLE_PLUGIN_CHECK_EDID_CORRUPTED 3
#define CABLE_PLUGIN_HDMI_OUT  4
#define CABLE_PLUGIN_DVI_OUT    5
#define CABLE_PLUGIN_HDMI_OK    6
#define CABLE_PLUGIN_DVI_OK       7

#define BYPASS               0x00
#define TX_DeMux             0x02
#define TX_SyncExtr          0x01
#define TX_YCbCrToRGB        0x04
#define TX_Dither            0x08
#define TX_422to444          0x10
#define TX_DVO               0x20
#define TX_444to422          0x40
#define TX_RGBToYCbCr        0x80

//-----------------------------------------------------------------------------
// These defines are used for Deep Color  (Video Path)
//-----------------------------------------------------------------------------
enum SiI_DeepColor {
    SiI_DeepColor_24bit = 0x00,
    SiI_DeepColor_30bit = 0x01,
    SiI_DeepColor_36bit = 0x02,
    SiI_DeepColor_Off   = 0xFF
};

typedef enum  { x0_5 = 0x00, x1 = 0x20, x2 = 0x40, x4 = 0x60 } TCLK_SEL;
typedef enum  { blue, yellow, orange }RANGE;

//-----------------------------------------------------------------------------
// These defines are used for Audio Path
//-----------------------------------------------------------------------------
enum SiI_AudioPath {
    SiI_SPDIF,
    SiI_I2S,
    SiI_DSD,
    SiI_HBAudio,
    SiI_AudioModesSelect = 0x0F,   // first four bits used for type of audio interface
    SiI_MultyChannel = 0x80
};
BOOL HD_Mode( void );
void siiTXHardwareReset( void );
void siiTXSetToHDMIMode( BOOL );
BYTE IsTXInHDMIMode( void );
void siiTXWriteNValue( DWORD );
void siiTXInitAudioPart( void );
void UpdateTX_DE( void );
void UpdateTX_656( void );
void SetDE( BYTE );
void siiTXDEconfig( void );
void siiTXSetIClk( BYTE );
void siiTXSetVideoBlank( BYTE Blank1, BYTE Blank2, BYTE Blank3 );
void siiTXSetInputClockEdge( BYTE );
void siiTXSetDeepColor ( BYTE );
BYTE siiTX_TMDS_Setup( BYTE );
void SiI_Mpll_setup( BYTE );
void SiI_FApost_setup( BYTE, int, BYTE );
void AssertHDMITX_SWReset( BYTE );
void ReleaseHDMITX_SWReset( BYTE );
void siiTX_SW_Reset( void );
void siiTXSetVideoPath ( void );
BYTE FindFsFromSPDIF( void );
void siiTXSetAudioPath ( void );
void siiTXWakeUp( BOOL );
void siiInitDVIInterruptMasks( void );
void siiTXSystemInit( void );
void siiTX_DVI_Init( void );
void siiTX_HDMI_Init( void );
void HotPlugInOrOut( void );
void siiTXInterruptHandler( void );
void siiTXTaskHandler( void );
void siiTXInitParameter( void );
void ChangeInitVideoMode(unsigned int videomote);

typedef struct {

// TX Video Input Setting
    BYTE siiTXInputVideoMode;        // Please see VRTables.
    BYTE siiTXInputVideoType;        // siiTXInputVideoType: 0-RGB24; 1-RGB DVO 12; 2-YCbCr24; 3-YC24; 4-YCMuxed12; 5-YCMuxed656_12; 6-YC656_24.
    BYTE siiTXInputColorDepth;       // siiTXInputColorDepth: 0-24 bit; 1-30 bits; 2-36 bits
    BYTE siiTXInputPixRepl;          // siiTXInputPixRepl: 0-no replication; 1-twice; 2-Reserved; 3-four times
    BYTE siiTXInputClockEdge;        // siiTXInputClockEdge: 0-InputClockFallingEdge; 1-InputClockRisingEdge

// TX Video Output Setting
    BYTE siiTXOutputVideoType;       // siiTXOutputVideoType: 0-RGB24;  1-YCbCr24; 2-YCbCr4:2:2
    BYTE siiTXOutputColorDepth;      // siiTXOutputColorDepth: 0-24 bit; 1-30 bits; 2-36 bits

// TX Output Status
    BYTE siiTXOutputMode;            // siiTXOutputMode: 0-None_Mode;  1-DVI_Mode; 2-HDMI_Mode
    BYTE siiTXOutputState;           // siiTXOutputState: 0-CABLE_UNPLUG_; 1-CABLE_PLUGIN_CHECK_EDID; 2-CABLE_PLUGIN_CHECK_EDID_OK; 3-CABLE_PLUGIN_CHECK_EDID_CORRUPTED; 4-CABLE_PLUGIN_HDMI_OUT; 5-CABLE_PLUGIN_DVI_OUT

// Audio Input Setting
    BYTE siiTXInputAudioType;         // siiTXInputAudioType: 0-SPDIF; 1-I2S; 2-DSD; 3-HBR
    BYTE siiTXInputAudioFs;           // siiTXInputAudioFs: 0-44.1kHz; 2-48kHz; 3-32kHz; 8-88.2kHz; 9-768kHz; A-96kHz; C-176.4kHz; E-192kHz; 1/4/5/6/7/B/D/F-not indicated
    BYTE siiTXInputAudioSampleLength; // siiTXInputAudioSampleLength: 0/1-not available; 2-16 bit; 4-18 bit; 8-19 bit; A-20 bit; C-17 bit; 3-20 bit; 5-22 bit; 9-23 bit; B-24 bit; D-21 bit
    BYTE siiTXInputAudioChannel;      // siiTXInputAudioChannel: 1-2 channels; 2-3 channels; 3-4 channels; 4-5 channels; 5-6 channels; 6-7 channels; 7-8 channels

// TX DE signal Setting
    BYTE siiTXDEMode;   // siiTXDEMode: 0-INTERNAL_DE; 1-EXTERNAL_DE; 2-EMBEDED_SYNC

// HDCP Enable
    BYTE siiTXHDCPEnable;

// APP
	BYTE is_first_time;
}siiTXParameter;

extern  siiTXParameter siiTX;

extern BOOL AssertedDSAuthPart2;
extern WORD TimeOutForDSAuthPart2;

