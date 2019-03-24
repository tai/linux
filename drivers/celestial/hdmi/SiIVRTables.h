//------------------------------------------------------------------------------
// Module Name:  SiIVRTables.h
// Module Description:  SiIVRTable header file
//
// Copyright © 2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------

#include "TypeDefs.h"

#define NSM    0 // no sub. mode
#define VGA 0

#define PC_BASE ( 28 + 15 )
#define NMODES (PC_BASE + 55)        // CEA 861C + PC Video Modes

// Aspect ratio
#define _4     0  // 4:3
#define _4or16 1  // 4:3 or 16:9
#define _16    2  // 16:9

#define ProgrVPosHPos 0x03
#define ProgrVPosHNeg 0x02
#define ProgrVNegHPos 0x01
#define ProgrVNegHNeg 0x00

#define InterlaceVPosHPos 0x07
#define InterlaceVPosHNeg 0x06
#define InterlaceVNegHPos 0x05
#define InterlaceVNegHNeg 0x04
#define SiI_InterlaceMask 0x04

//--------------------------------------------------------------------
typedef struct {
    BYTE Mode_C1;
    BYTE Mode_C2;
    BYTE SubMode;
}ModeIdType;

//--------------------------------------------------------------------
typedef struct {
    WORD Pixels;
    WORD Lines;
} PxlLnTotalType;

//--------------------------------------------------------------------
typedef struct{
    BYTE RefrTypeVHPol;
    WORD VFreq;
    PxlLnTotalType Total;
} TagType;

//--------------------------------------------------------------------
typedef struct {
    WORD H;
    WORD V;
} HVPositionType;

//--------------------------------------------------------------------
typedef struct {
    WORD H;
    WORD V;
} HVResolutionType;

//--------------------------------------------------------------------
typedef struct {
    BYTE IntAdjMode;
    WORD HLength;
    WORD VLength;
    WORD Top;
    WORD Dly;
    WORD HBit2HSync;
    WORD VBit2VSync;
    WORD Field2Offset;
}  _656Type;

//--------------------------------------------------------------------
typedef struct {
    ModeIdType ModeId;
    BYTE PixClk;
    TagType Tag;
    HVPositionType Pos;
    HVResolutionType Res;
    BYTE AspectRatio;
    _656Type _656;
    BYTE PixRepl;
} VModeInfoType;

extern  const DWORD N_Val[];
extern  const BYTE VICTables[];
extern  const VModeInfoType VModeTables[NMODES];
