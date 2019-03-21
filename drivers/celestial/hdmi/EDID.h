//---------------------------------------------------------------------------
// Module Name: EDID.h
// Module Description: EDID header file
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//---------------------------------------------------------------------------
#include "TypeDefs.h"

#define SiI9034_App

#define EDID_SLV     0xA0
#define OFFSET_SLV 0x60

#define POLYNOM      0x1021
#define MASK_LSBit  0x0001

#define IS_EDID_ERROR 1
#define NO_EDID_ERROR 0

#define ReadFullEDIDParserType()  1

typedef struct {
 BYTE ErrorCode;
 BYTE DisplayType;
 WORD CRC16;
} EDIDParsedDataType;
typedef struct
{							
	unsigned char CEC_A_B;								// CEC Physical address. See HDMI 1.3 Table 8-6
	unsigned char CEC_C_D; 
	BOOL HDMI_Sink;
} Type_EDID_Descriptors;

// EDID Addresses
#define VER_ADDR 0x12
#define NUM_EXTENSIONS_ADDR 0x7E

#define EXTENSION_ADDR 0x80
#define CEA_DATA_BLOCK_COLLECTION_ADDR 0x84

#define EXTENSION_ADDR_1StP 0x00
#define CEA_DATA_BLOCK_COLLECTION_ADDR_1StP 0x04

#define TAG_AUDIO_DATA_BLOCK   0x20
#define TAG_VIDEO_DATA_BLOCK   0x40
#define TAG_VENDOR_DATA_BLOCK  0x60
#define TAG_SPEAKER_DATA_BLOCK  0x80

// Codes of EDID Errors

#define NO_ERR 0
// 1-7 reserved for I2C Errors
#define BAD_HEADER 8
#define VER_DONT_SUPPORT_861B 9
#define _1ST_BLOCK_CRC_ERROR 10
#define _2ND_BLOCK_CRC_ERROR 11
#define EXTENSION_BLOCK_CRC_ERROR 11
#define NO_861B_EXTENSION 12
#define NO_HDMI_SIGNATURE 13
#define BLOCK_MAP_ERROR 14
#define CRC_CEA861EXTENSION_ERROR 15
//#define NO_HDMI_SIGNATURE 16

// DisplayType
#define UNKNOWN_DISPLAY 0
#define DVI_DISPLAY 1
#define HDMI_DISPLAY 2

#define VIDEO_TAG 0x40
#define AUDIO_TAG 0x20
#define VENDOR_TAG 0x60
#define SPEAKER_TAG 0x80
#define USE_EXTENDED_TAG 0xe0

#define BLOCK_HEADER	0x02
#define VERSION_THREE	0x03
#define THREE_MSB		0xE0
#define FIVE_LSB		       0x1F
#define SVDB_TAG		0x03

void EDIDProcessing( void );
unsigned char BlockReadEDID( unsigned char NBytes, unsigned char Addr, unsigned char * Data );
void UpdateCRC16WithBlock( unsigned short int * pCRC, unsigned char NBytes, unsigned char * Data );
