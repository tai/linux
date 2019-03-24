//------------------------------------------------------------------------------
// Module Name: SiITXInfoPkts.h
// Module Description: SiITX Infoframe Packets header file
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef _SiITX_INFOPKTS_
#define _SiITX_INFOPKTS_
#include "TypeDefs.h"

#define VENDORSPEC_TYPE 0x81    // 2012-02-20, added by xp
#define AVI_TYPE   0x82
#define SPD_TYPE   0x83
#define AUD_TYPE   0x84
#define MPEG_TYPE  0x85
#define GENERIC1_TYPE 0x00
#define GENERIC2_TYPE 0x01

//-----------------------------------
//  3D_Structure
//-----------------------------------
#define FRAME_PACKING 0x0
#define FIELD_ALTERNATIVE 0x1
#define LINE_ALTERNATIVE 0X2
#define SIDE_BY_SIDE_FULL 0x3
#define L_DEPTH 0x4
#define L_DEPTH_GRAPHICS_DEPTH 0x5
#define TOP_AND_BOTTOM 0x6
#define SIDE_BY_SIDE_HALF 0x8

//-----------------------------------
//  InfoFrame masks
//-----------------------------------
#define AVI_MASK 0x01
#define SP_MASK 0x02
#define AUD_MASK 0x04
#define MPEG_MASK 0x08
#define UNKNOWN_MASK 0x10
#define CP_MASK  0x80

#define BIT_AVI_EN_REPEAT  0x0003
#define BIT_SPD_EN_REPEAT  0x000C
#define BIT_AUD_EN_REPEAT  0x0030
#define BIT_MPEG_EN_REPEAT 0x00C0
#define BIT_GENERIC1_EN_REPEAT  0x0300
#define BIT_GENERIC2_EN_REPEAT  0x3000

#define  AVI_SIZE 13 //15 //Oscar modify 20080620
#define  SPD_IF_SIZE 31
#define  AUD_IF_SIZE  10
#define  MPEG_IF_SIZE  31
#define  GENERIC1_IF_SIZE 31
#define  GENERIC2_IF_SIZE 31


#define AVI_IF_ADDR 0x40
#define SPD_IF_ADDR  0x60
#define AUD_IF_ADDR 0x80
#define MPEG_IF_ADDR 0xA0
#define GENERIC1_IF_ADDR 0xC0
#define GENERIC2_IF_ADDR 0xE0
#define CP_IF_ADDR   0xDF // Contain Protect 1- byte Frame Info Frame

#define BIT_CP_AVI_MUTE_SET    0x01
#define BIT_CP_AVI_MUTE_CLEAR  0x10

#define WR_IN_PD       1
#define RPT_BIT_STUCK  2
#define EN_BIT_STUCK   3
#define WRGN_IFR_TYPE  4
#define WRNG_IPK_ID    5
#define BUF_SIZE_ERR   6
#define UNSTABLE_TCLK  7

typedef struct {
    BYTE Type;
    BYTE Version;
    BYTE Length;
    BYTE CheckSum;

}HeaderType;

typedef struct {
   BYTE DestAddr;			// InfoFrame Register address: 0xA0 for MPEG; 0x40 for AVI...
   WORD CtrlBitsAddr;		// Bit masks of 7A:3E..3F to enable & repeat InfoFrames
   BYTE BlockSize;			// InfoFrame size (bytes): AVI: 15; Audio: 10...
   BYTE CheckSumProp;
} InfoMapType;

extern  const BYTE AudioCP[31];
extern  const BYTE ISRC1[31];
extern  const BYTE ISRC2[31];
extern  unsigned int  PictureAspectRatio;

BOOL GetInfoFrameMapAddr( BYTE, InfoMapType * );
BOOL GetRestOfInfoFrameHeader( HeaderType * );
BOOL WaitBuffReady( WORD );
void CalcCheckSumIFPacket( HeaderType *, BYTE * );
BYTE siiTXSendInfoFrame( BYTE, BYTE * );
void siiTXSetAudioInfoFramePacket ( void );
void siiTXSetAVIInfoFramePacket( void );
BOOL siiTXDisableInfoFrame( BYTE );
BOOL siiTXEnableInfoFrame( BYTE );
BOOL siiTXDisableInfoPacket( BYTE );
BOOL siiTXEnableInfoPacket( BYTE );
void siiTXSendCP_Packet( BYTE );
BYTE siiTXSendInfoPacket( BYTE *, BYTE,  BYTE,  BYTE );
void dumpreg(int index);
void setAudioInfoFrameChannelMapping(int nCh, int chMap);


#endif
