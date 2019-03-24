/*******************************************************************************
File name   : blit_hw.h

Description : hardware related header file

COPYRIGHT (C) Celestial Semiconductor 2007.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2007        Created                                           XM.Chen
20 Oct 2009		   Modified							 			   Jia.Ma
*******************************************************************************/

#ifndef _BLIT_HW_H_
#define _BLIT_HW_H_

#include "cnc18xx_gfxobj.h"
#include "blit_cmd_reg_def.h"

#ifdef CONFIG_FB_ORION_BLIT_DEBUG
#define DEBUG_PRINTF  printk
#else
#define DEBUG_PRINTF(fmt,args...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CSBLIT_DEVICE_TYPE_1_3 		BLIT_TYPE_1100
#define CSBLIT_DEVICE_TYPE_1_3_1 	BLIT_TYPE_1200

typedef enum _BLIT_DC_UPDATE_TYPE_
{
	BLIT_DC_S0_COLOR_FORMAT   = (1 << 0),
	BLIT_DC_S0_DEFAULT_COLOR  = (1 << 1),
	BLIT_DC_S0_COLORKEY_PARA  = (1 << 2),
	BLIT_DC_S0_DRAM_PARA      = (1 << 3),

	BLIT_DC_S1_COLOR_FORMAT   = (1 << 4),
	BLIT_DC_S1_DEFAULT_COLOR  = (1 << 5),
	BLIT_DC_S1_COLORKEY_PARA  = (1 << 6),
	BLIT_DC_S1_DRAM_PARA      = (1 << 7),

	BLIT_DC_D_COLOR_FORMAT    = (1 << 8),
	BLIT_DC_D_DRAM_PARA       = (1 << 9),

	BLIT_DC_CLUT4_TABLE       = (1 << 10),
	BLIT_DC_CLUT8_TABLE       = (1 << 11),
	//Scalor
	BLIT_DC_SCALOR_HFIR_COFF  = (1 << 12),
	BLIT_DC_SCALOR_VFIR_COFF  = (1 << 13),
	BLIT_DC_SCALOR_INIT_PHASE = (1 << 14),
	BLIT_DC_CTRL              = (1 << 31),	
	BLIT_DC_ALL               = (~0x0),
	
}BLIT_DC_UPDATE_TYPE;

typedef enum  CSBLIT_AluMode_e
{
    CSBLIT_ALU_CLEAR            = 0,      //0      0000
    CSBLIT_ALU_NOR              = 1,      //~(D|P) 0001
    CSBLIT_ALU_AND_INVERT       = 2,      //D&~P   0010
    CSBLIT_ALU_COPY_INVERT      = 3,      //~P     0011
    CSBLIT_ALU_AND_REV          = 4,      //P&~D   0100
    CSBLIT_ALU_INVERT           = 5,      //~D     0101
    CSBLIT_ALU_XOR              = 6,      //D^P    0110
    CSBLIT_ALU_NAND             = 7,      //~(D&P) 0111
    CSBLIT_ALU_AND              = 8,      //D&P    1000
    CSBLIT_ALU_EQUIV            = 9,      //~(P^D) 1001
    CSBLIT_ALU_NOOP             = 10,     //D      1010
    CSBLIT_ALU_OR_INVERT        = 11,     //D|~P   1011
    CSBLIT_ALU_COPY             = 12,     //P      1100
    CSBLIT_ALU_OR_REVERSE       = 13,     //P|~D   1101
    CSBLIT_ALU_OR               = 14,     //D|P    1110
    CSBLIT_ALU_SET              = 15      //1      1111
//    CSBLIT_ALU_ALPHA_BLEND      = 16
} CSBLIT_AluMode_t;

enum
{
	BLIT_INTERRUPT_MODE   = (0), //1 Enable, 0 Disable
};

typedef enum CSBLIT_DeviceType_e
{
	BLIT_TYPE_1100   = 1, 
	BLIT_TYPE_1200   = 2
} CSBLIT_DeviceType_t;

typedef struct CSBlit_HW_Device 
{
	u32						RegBaseAddr;	//blitter Register Base address after ioremapped
	CSBLIT_DeviceType_t 	BlitType;
	
	//Run Time Info
	s32 			BlitIdelStatus;
	s32 			AHBErrStatus;
	s32 			BlitErrStatus;
	s32 			InterruptStatus;    //Interrupt Satatus
	u32 			CMDCnt;			//CMD count
	u8				UpdateClut4Table;	//1 Update, 0 donot Update
	u8				UpdateClut4TableMode;            //0 Update all, 1 Update accoding Idx                               
	u32 			Clut4TableIdx;
	u32 			Clut4Table[CLUT4_TABLE_LEN];
	u8				UpdateClut8Table;                //1 Update, 0 donot Update
	u8				UpdateClut8TableMode;            //0 Update all, 1 Update accoding Idx
	u32 			Clut8TableIdx;
	u32 			Clut8Table[CLUT8_TABLE_LEN];
	u8				InterruptEnable; 	// 1 interrupt enable
	
	//S0 Control
	u8 			S0Enable;
	u8 			S0VReverseScan;
	u8 			S0FetchDram;      // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
						//if S0 Using Default Color then S0ColorType should be ARGB565 or A0
	CSBLIT_ColorType_t 	S0ColorType;
	u32					S0Alpha0;   		//For ARGB1555 Alpha Value 0
	u32					S0Alpha1;			//For ARGB1555 Alpha Value 1
	CSBLIT_Color_t   	S0DefaultColor; //ARGB8888 Type
	u32					S0BaseAddr; 	//Bytes Addr
	u32					S0LinePitch;    //Bytes Addr
	u32					S0SkipPixelLine; //Valid Only When Clut4
	u32					S0PixelNumOneLine;
	u32					S0TotalLineNum;	
	u8					S0ByteLittleNotBigEndian;
	u8					S0NibbleLittleNotBigEndian;
	u8					S0TwoBytesLittleNotBigEndian;

	u8					S0ClutEnable;
	//u8				S0ColorKeyEnable;
	//CSBLIT_Color_t 	S0ColorKeyMin;
	//CSBLIT_Color_t 	S0ColorKeyMax;
	CSBLIT_ColorKeyRGB_t 	S0ColorKeyRGB;
	
	//S1 Control
	u8					S1Enable;
	u8					S1VReverseScan;
	u8					S1FetchDram;      // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
						//if S1 Using Default Color then S1ColorType should be ARGB565 or A0
	CSBLIT_ColorType_t 	S1ColorType;
	u32					S1Alpha0;   //For ARGB1555 Alpha Value 0
	u32					S1Alpha1;	//For ARGB1555 Alpha Value 1

	CSBLIT_Color_t   	S1DefaultColor;
	u32					S1BaseAddr;    //Bytes Addr
	u32					S1LinePitch;   //Bytes Addr
	u32					S1SkipPixelLine;  //Valid Only When Clut4
	u32					S1PixelNumOneLine;
	u32					S1TotalLineNum;	
	u8					S1ByteLittleNotBigEndian;
	u8					S1NibbleLittleNotBigEndian;
	u8					S1TwoBytesLittleNotBigEndian;

	u8					S1ClutEnable;
	//u8				S1ColorKeyEnable;
	//CSBLIT_Color_t 	S1ColorKeyMin;
	//CSBLIT_Color_t 	S1ColorKeyMax;
	CSBLIT_ColorKeyRGB_t 	S1ColorKeyRGB;

    	//ROP and Compositor
	u8					CompositorEnable;
	u32					ROPAlphaCtrl;

	/*ROP_OPT*/
    CSBLIT_AluMode_t 	RopValue;
	u8					S0OnTopS1;  // 1 S0 On Top S1

	//D Dram Para
	u8					DVReverseScan;
	u32					DBaseAddr;  //Bytes Addr
	u32					DLinePitch; //Bytes Addr
	u32					DSkipPixelLine; //Valid Only When Clut4
	u32					DPixelNumOneLine;
	u32					DTotalLineNum;
	CSBLIT_ColorType_t 	DColorType;
	u32					DAlpha0Min;
	u32					DAlpha0Max;
	u8					DByteLittleNotBigEndian;
	u8					DNibbleLittleNotBigEndian;
	u8					DTwoBytesLittleNotBigEndian;

	//Special Endian Ctrl
	u8					IsUsingSDSpecificEndian; //0, Global Endian, 1 Specifica Endian

	//Scalor Control
	u8					S0ScalorEnable;
	u32					HInitialPhase;
	u32					VInitialPhase;
	u8					UpdateHFIRCoeff;
	u8					UpdateVFIRCoeff;
	u32					HFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_H_FIR_TAP_NUM];
	u32					VFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_V_FIR_TAP_NUM];
	BLIT_DC_UPDATE_TYPE 	BlitDCType;
} CSBlit_HW_Device_t;

void HardWareReset(CSBlit_HW_Device_t *hBlitDC);
u32 GetCMDQueueDepth(void);
void SWReset(CSBlit_HW_Device_t *hBlitDC);
s32 WaitCMDQueueEmpty(CSBlit_HW_Device_t *hBlitDC);

u32 GetCMDGroupCnt(void);
void CMDGroupCntSyncHW(CSBlit_HW_Device_t *hBlitDC);
s32 SendCMD(CSBlit_HW_Device_t *hBlitDC, u32 BlitCMD);
s32 WaitBlitToBeIdle(CSBlit_HW_Device_t *hBlitDC); //Boolean Type Return
s32 WaitBlitScalorToBeIdle(CSBlit_HW_Device_t *hBlitDC); //Boolean Type Return
s32 ActiveDeviceContext( CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType);

s32 RegCheckClutTable(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType, s32 IsClut4orClut8); //0: Clut4, 1: Clut8;
s32 RegCheckPosStartup(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType);
s32 RegCheckPreStartup(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType); //Boolean Type Return

#ifdef __cplusplus
}
#endif

#endif //_BLIT_HW_H_

