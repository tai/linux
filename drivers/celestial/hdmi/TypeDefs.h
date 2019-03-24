//------------------------------------------------------------------------------
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef _TYPEDEFS_
#define _TYPEDEFS_
#include "hdmi_global.h"
#ifndef WIN32
typedef unsigned char BOOL;
#else
typedef unsigned int BOOL;
#endif
typedef unsigned char BYTE;
typedef unsigned char U8BIT;
typedef unsigned short int WORD;
typedef unsigned short int U16BIT;
typedef unsigned long DWORD;
typedef unsigned long U32BIT;

//add for ABT1030...........................
typedef unsigned short  ABT1030_Addr_t;
typedef  unsigned long          UInt32;
typedef  unsigned short         UInt16;
typedef  unsigned char          UInt8;
//................
#define ABT1030_ADDR 0x30
//end add.......................................

#if 1
#define sii9030_V640x480p_60Hz 1
#define sii9030_V720x480p_60Hz_4x3 2
#define sii9030_V720x480p_60Hz_16x9 3

#define sii9030_V720x480i_60Hz_4x3 4
#define sii9030_V720x480i_60Hz_16x9 5

#define sii9030_V720x576i_50Hz_4x3 6
#define sii9030_V720x576i_50Hz_16x9 7

#define sii9030_V720x576p_50Hz_4x3 8
#define sii9030_V720x576p_50Hz_16x9 9

#define sii9030_V1280x720p_50Hz 10
#define sii9030_V1280x720p_60Hz 11

#define sii9030_V1920x1080i_50Hz 12
#define sii9030_V1920x1080i_60Hz 13

#define sii9030_V1920x1080p_50Hz 14
#define sii9030_V1920x1080p_60Hz 15
#define sii9030_V1920x1080p_24Hz 16

#define sii9030_V800x600p_60Hz 17
#define sii9030_V1024x768p_60Hz	 18
#define sii9030_V1280x720p_rgb_60Hz 19
#define sii9030_V800x480p_60Hz 20
#define sii9030_V1440x900p_60Hz	 21
#define sii9030_V1280x1024p_60Hz 22
#define sii9030_V1360x768p_60Hz	 23
#define sii9030_V1920x1080p_rgb_30Hz 24
#define sii9030_V1920x1080p_rgb_60Hz 25
#define sii9030_V1920x1080i_rgb_60Hz 26
#define sii9030_V1366x768p_60Hz	 27
#define sii9030_V1920x1080p_30Hz 28

typedef	enum
{
	eCS_DBU_DEFINITION_480I = 0,
	//eCS_DBU_DEFINITION_480P,		//add ...........
	eCS_DBU_DEFINITION_576I,
	eCS_DBU_DEFINITION_576P,
	eCS_DBU_DEFINITION_720P,
	eCS_DBU_DEFINITION_1080I,
	eCS_DBU_DEFINITION_1080P,	//add ...........
	eCS_DBU_DEFINITION_AUTOMATIC
}tCS_DBU_VideoDefinition; 	/*Í¼ÏñÇåÎú¶È*/
#endif



//add ..............................

typedef enum {
	VIdeoMOde_640x480p,
	VIdeoMOde_720x480i,
	VIdeoMOde_720x480p,
	VIdeoMOde_720x576i,
	VIdeoMOde_720x576p,
	VIdeoMOde_1280x720p,
	VIdeoMOde_1920x1080i,
	VIdeoMOde_1920x1080p,
	VIdeoMOde_Max
}EHDMI_VIDOMODE;

typedef struct VideoMode
{
	BOOL VideoMode43_169;
	BOOL VideoModeNtsc_Pal;
	EHDMI_VIDOMODE VideoMode_I_P;
} TVIDEOMODE;
//#define ROM   code       // 8051 type of ROM memory
//#define IRAM  idata      // 8051 type of RAM memory

#define FALSE  0
#define TRUE  1

#define ON 1    // Turn ON
#define OFF 0   // Turn OFF



#endif
