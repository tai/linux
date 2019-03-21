/*******************************************************************************

File name   : blit_hw.c

Description : blitter hardware related source file

COPYRIGHT (C) Celestial Semiconductor 2007.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2007        Created                                           XM.Chen
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <asm/io.h>
#include "blit_hw.h"

#define ReadRegMem32LE(addr) ioread32((void*)(addr))
#define WriteRegMem32LE(addr, val) iowrite32((val),(void*)(addr))
/*--------------------------------------------------
* #define CHECK_REG_AFTER_SEND_CMD
* #define RESET_BLIT_ONLY_ONE_TIME
* #define CHECK_REG_BEFORE_SEND_CMD
*--------------------------------------------------*/

#ifdef RESET_BLIT_ONLY_ONE_TIME
static s32 IsBlitHWReset = 0;
static s32 IsBlitSWReset = 0;
#endif

//Hardware Reset
void HardWareReset(CSBlit_HW_Device_t *hBlitDC)
{
	DEBUG_PRINTF("2BlitIdelStatus = 0x%x, BlitCMDQueuSpace = 0x%x\n",ioread32(hBlitDC->RegBaseAddr + BLIT_REG_STATUS),ioread32(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT));
#ifdef RESET_BLIT_ONLY_ONE_TIME
	if(IsBlitHWReset)
	{
		return;
	}
	else
	{
		IsBlitHWReset++;
	}	
#endif
	/*BlitHWReset();*/
	CMDGroupCntSyncHW(hBlitDC);
}
/*
//Return BlitCMDQueuSpace, 0 means Command Queue Full
*/

s32 SendCMD(CSBlit_HW_Device_t *hBlitDC, u32 BlitCMD)
{
	//Check Available Space
	u32 BlitCMDQueuSpace = 0;
	u32 BlitStatusRegValue = 0;
	s32 AHBErrStatus    = 0;
	s32 BlitErrStatus    = 0;
	s32 InterruptStatus = 0; 
	
	//Clear Interrupt and ERR
	BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
	AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
	BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT) &(1);
	InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
	if(AHBErrStatus)
	{
		printk("##ERROR: Blit AHB status Err\n");
		return 0;		/* error happened */

	}
	if (InterruptStatus == 1)
	{
		if (BlitErrStatus)
		{				
			printk("##ERROR: SendCMD Blit Command Interpret Err\n");
		}

		//Clear Interrupt
		WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_INT_CLEAR, 1);//Write Any Value
	}
	
	do
	{
		BlitCMDQueuSpace = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT) >> 
		                   BLIT_REG_CMDQUE_EMPTY_CNT_LSB) &
		                  ((1 << BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
		if (BlitCMDQueuSpace <= 0)
		{
			//Some User Defined Sleep Function
			printk("Blit Command Queue Full, Waiting.....\n");
		}
	}while(BlitCMDQueuSpace <= 0);
	
	if (BlitCMDQueuSpace > 0)
	{
/*--------------------------------------------------
* printk("CMD: 0x%8x\n",BlitCMD);
*--------------------------------------------------*/
/*--------------------------------------------------
* HAL_DCACHE_SYNC();
*--------------------------------------------------*/
		DEBUG_PRINTF("dump : 2D CMD 0x%x\n",BlitCMD);
		WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMD_QUE, BlitCMD);
	}

	return BlitCMDQueuSpace;
}

/*
//Before Call Set Active Context, make sure the the Command Queue Have enough Space 
//and Blit's Interrupt Has been Cleared, it's better that the Status is Idle
*/
static u32 sBlitCMDGroupCnt = 0;
static void CMDGroupCntInc(void)
{
	sBlitCMDGroupCnt++;
}

void CMDGroupCntSyncHW(CSBlit_HW_Device_t *hBlitDC)
{
	//Sync with Hardware
	sBlitCMDGroupCnt = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMD_INFO) >> REG_INFO_CMD_CNT_LSB) & 
	                  ((1 << REG_INFO_CMD_CNT_WIDTH) -1 );
}

u32 GetCMDGroupCnt(void)
{
	return sBlitCMDGroupCnt;
}

s32 ActiveDeviceContext( CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType)
{
	//Send S0 Related Command
	u32 BlitCMD   = 0;
	u32 BlitCMDQueSpace = 0;
	u32 CompositorOperation = 0;
	u32 S0Operation     = 0;
	u32 S1Operation     = 0;
	u32 ScanCtrl        = 0;
	u32 S0EndianCtrl    = 0;
	u32 S1EndianCtrl    = 0;
	u32 DEndianCtrl     = 0;
	u32 GlobalEndianCtrl  = 0;
	if (hBlitDC == NULL)
	{
		return 0;
	}

	if (hBlitDC->S0Enable || hBlitDC->S0ScalorEnable)
	{
		if ((hBlitDC->S0FetchDram || hBlitDC->S0ScalorEnable )&& 
			(BlitDCType & BLIT_DC_S0_DRAM_PARA) )
		{			
			//Dram Access Para
			if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
			{
				BlitCMD = ((CMD_S0_ADDR_INFO_L & ((1<< CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					(((hBlitDC->S0BaseAddr & 0x3) >> 1) << CMD_SKIP_PIXEL_LSB) |
					((hBlitDC->S0LinePitch 
					  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) >> 2 ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->S0BaseAddr >> 2);
			}
			else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
			{
				BlitCMD = ((CMD_S0_ADDR_INFO_L & ((1<< CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S0SkipPixelLine 
					  & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
					((hBlitDC->S0LinePitch 
					  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->S0BaseAddr);
			}

			if (hBlitDC->IsUsingSDSpecificEndian)
			{				
				S0EndianCtrl = ((hBlitDC->S0ByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
					           ((hBlitDC->S0NibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
					           ((hBlitDC->S0TwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
					           ((1 & 0x1) << ENDIAN_ENABLE_BIT);
			}

			if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
			{
				BlitCMD = ((CMD_SCALOR_S_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S0PixelNumOneLine
					  & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);

				BlitCMD = ((CMD_SCALOR_S_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S0TotalLineNum
					  & ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB) ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			}
		}

		//Color Info
		if (BlitDCType & (BLIT_DC_S0_COLOR_FORMAT))
		{
			if (BlitDCType & BLIT_DC_S0_DEFAULT_COLOR)
			{
				if (hBlitDC->S0ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
				{
					BlitCMD = ((CMD_S0_FORMAT_L   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S0ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					BlitCMD = ((CMD_S0_FORMAT_L   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S0ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
							 ((hBlitDC->S0Alpha0
							                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
							 ((hBlitDC->S0Alpha1
							                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}
			
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
				BlitCMD = (((hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha >> (8-8))
					                     & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.R >> (8-8))
					                     & ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.G >> (8-8))
					                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.B >> (8-8))
					                     & ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			}
			else
			{
				if (hBlitDC->S0ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
				{
					BlitCMD = ((CMD_S0_FORMAT     & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S0ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					BlitCMD = ((CMD_S0_FORMAT     & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S0ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
							 ((hBlitDC->S0Alpha0
							                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
							 ((hBlitDC->S0Alpha1
							                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}
			
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);				
			}
		}
		//ColorKey ARGB8888
		if (hBlitDC->S0ColorKeyRGB.RGBEnable && (BlitDCType & BLIT_DC_S0_COLORKEY_PARA))
		{
			BlitCMD = ((CMD_S0_COLOR_KEY_MIN & ((1 << CMD_IDX_WIDTH) -1))   << CMD_IDX_LSB)    |
				     ((hBlitDC->S0ColorKeyRGB.RMin
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				     ((hBlitDC->S0ColorKeyRGB.GMin
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				     ((hBlitDC->S0ColorKeyRGB.BMin
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			BlitCMD = ((CMD_S0_COLOR_KEY_MAX & ((1 << CMD_IDX_WIDTH) -1))   << CMD_IDX_LSB)    |
				     ((hBlitDC->S0ColorKeyRGB.RMax
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				     ((hBlitDC->S0ColorKeyRGB.GMax
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				     ((hBlitDC->S0ColorKeyRGB.BMax
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);		
		}		
	}
	//Send S1 Related Command
	if (hBlitDC->S1Enable)
	{
		if (hBlitDC->S1FetchDram && (BlitDCType & BLIT_DC_S1_DRAM_PARA) )
		{
			
			//Dram Access Para
			if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
			{
				BlitCMD = ((CMD_S1_ADDR_INFO_L & ((1<< CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					(((hBlitDC->S1BaseAddr & 0x3) >> 1) << CMD_SKIP_PIXEL_LSB) |
					((hBlitDC->S1LinePitch 
					  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) >> 2 ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->S1BaseAddr >> 2);
			}
			else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
			{
				BlitCMD = ((CMD_S1_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S1SkipPixelLine 
					  & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
					((hBlitDC->S1LinePitch 
					  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->S1BaseAddr);
			}
			
			if (hBlitDC->IsUsingSDSpecificEndian)
			{				
				S1EndianCtrl = ((hBlitDC->S1ByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
					           ((hBlitDC->S1NibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
					           ((hBlitDC->S1TwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
					           ((1 & 0x1)    << ENDIAN_ENABLE_BIT);
			}

			if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
			{
				BlitCMD = ((CMD_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S1PixelNumOneLine
					  & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);

				BlitCMD = ((CMD_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((hBlitDC->S1TotalLineNum
					  & ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB) ;
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			}
		}
/*--------------------------------------------------
* printk("R:0x%x G:0x%x B: 0x%x A:0x%x\n",hBlitDC->S1DefaultColor.Value.ARGB8888.R,hBlitDC->S1DefaultColor.Value.ARGB8888.G,hBlitDC->S1DefaultColor.Value.ARGB8888.B,hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha);
*--------------------------------------------------*/

		//Color Info
		if (BlitDCType & BLIT_DC_S1_COLOR_FORMAT)
		{
			if (BlitDCType & BLIT_DC_S1_DEFAULT_COLOR)
			{
				if (hBlitDC->S1ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
				{
/*--------------------------------------------------
* printk("R:0x%x G:0x%x B: 0x%x A:0x%x\n",hBlitDC->S1DefaultColor.Value.ARGB8888.R,hBlitDC->S1DefaultColor.Value.ARGB8888.G,hBlitDC->S1DefaultColor.Value.ARGB8888.B,hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha);
*--------------------------------------------------*/
					BlitCMD = ((CMD_S1_FORMAT_L   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S1ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					BlitCMD = ((CMD_S1_FORMAT_L   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S1ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
							 ((hBlitDC->S1Alpha0
							                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
							 ((hBlitDC->S1Alpha1
							                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);

				BlitCMD = (((hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha >> (8-8))
						                     & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
						 (((hBlitDC->S1DefaultColor.Value.ARGB8888.R >> (8-8))
						                     & ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB)|
						 (((hBlitDC->S1DefaultColor.Value.ARGB8888.G >> (8-8))
						                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
						 (((hBlitDC->S1DefaultColor.Value.ARGB8888.B >> (8-8))
						                     & ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);
				/*--------------------------------------------------
				* BlitCMD = ((((BlitCMD >> 16) & 0xFF) << 8 |
				*     ((BlitCMD >> 16) & 0xFF00) >> 8) << 16)
				*     |
				*     (((BlitCMD & 0xFFFF) & 0xFF) << 8 |
				*     ((BlitCMD & 0xFFFF) & 0xFF00) >> 8);
				*--------------------------------------------------*/
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);	
			}
			else
			{
				if (hBlitDC->S1ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
				{
					BlitCMD = ((CMD_S1_FORMAT     & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S1ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					BlitCMD = ((CMD_S1_FORMAT     & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
							 ((hBlitDC->S1ColorType
							                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
							 ((hBlitDC->S1Alpha0
							                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
							 ((hBlitDC->S1Alpha1
							                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}
			
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);				
			}
		}
		
		//ColorKey
		if (hBlitDC->S1ColorKeyRGB.RGBEnable && (BlitDCType & BLIT_DC_S1_COLORKEY_PARA))
		{
			BlitCMD = ((CMD_S1_COLOR_KEY_MIN & ((1 << CMD_IDX_WIDTH) -1))   << CMD_IDX_LSB)    |
				     ((hBlitDC->S1ColorKeyRGB.RMin
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				     ((hBlitDC->S1ColorKeyRGB.GMin
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				     ((hBlitDC->S1ColorKeyRGB.BMin
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			BlitCMD = ((CMD_S1_COLOR_KEY_MAX & ((1 << CMD_IDX_WIDTH) -1))   << CMD_IDX_LSB)    |
				     ((hBlitDC->S1ColorKeyRGB.RMax
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				     ((hBlitDC->S1ColorKeyRGB.GMax
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				     ((hBlitDC->S1ColorKeyRGB.BMax
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);		
		}		
	}

	//Clut Table Update
			//Clut4
	if (hBlitDC->UpdateClut4Table && (BlitDCType & BLIT_DC_CLUT4_TABLE))
	{
		if (hBlitDC->UpdateClut4TableMode)//Update According Idx
		{
			u32 Clut4TableIdx = 0;
			Clut4TableIdx = (hBlitDC->Clut4TableIdx & ((1 << CMD_CLUT4_IDX_WIDTH) -1));
			//assert(Clut4TableIdx < CLUT4_TABLE_LEN);
			if(Clut4TableIdx < CLUT4_TABLE_LEN) return 0;
			BlitCMD =((CMD_CLUT4_IDX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) | 
				    ( Clut4TableIdx << CMD_CLUT4_IDX_LSB);
				
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			//Send Clut4 Entry
			BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->Clut4Table[Clut4TableIdx]);
		}
		else
		{
			u32 i = 0;
			BlitCMD =((CMD_CLUT4_TAB & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			for (i = 0; i < CLUT4_TABLE_LEN; i++)
			{
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->Clut4Table[i]);
			}
		}
	#ifdef CHECK_REG_AFTER_SEND_CMD
		if (!RegCheckClutTable(hBlitDC, BlitDCType, 1))
		{
			printk("##Failed to Check Clut4 Table Register\n");
			return 0;
		}
	#endif
	}
		//Clut8
	if (hBlitDC->UpdateClut8Table && (BlitDCType & BLIT_DC_CLUT8_TABLE))
	{
		if (hBlitDC->UpdateClut8TableMode)//Update According Idx
		{
			u32 Clut8TableIdx = 0;
			Clut8TableIdx = (hBlitDC->Clut8TableIdx & ((1 << CMD_CLUT8_IDX_WIDTH) -1));
			//assert(Clut8TableIdx < CLUT8_TABLE_LEN);
			if(Clut8TableIdx < CLUT8_TABLE_LEN) return 0;
			BlitCMD =((CMD_CLUT8_IDX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) | 
				    ( Clut8TableIdx << CMD_CLUT8_IDX_LSB);
			
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			//Send Clut8 Entry
			BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->Clut8Table[Clut8TableIdx]);
		}
		else
		{
			u32 i = 0;
			BlitCMD =((CMD_CLUT8_TAB & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			for (i = 0; i < CLUT8_TABLE_LEN; i++)
			{
				BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->Clut8Table[i]);
			}
		}
	#ifdef CHECK_REG_AFTER_SEND_CMD
		if (!RegCheckClutTable(hBlitDC, BlitDCType, 0))
		{
			printk("##Failed to Check Clut8 Table Register\n");
			return 0;
		}
	#endif
	}

	//Send D Related Command
	//D Dram Para
	if ((BlitDCType & BLIT_DC_D_DRAM_PARA))
	{
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			BlitCMD = ((CMD_D_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				(((hBlitDC->DBaseAddr & 0x3) >> 1) << CMD_SKIP_PIXEL_LSB) |
				((hBlitDC->DLinePitch 
				  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) >> 2 ;
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->DBaseAddr >> 2);
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			BlitCMD = ((CMD_D_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hBlitDC->DSkipPixelLine 
				  & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
				((hBlitDC->DLinePitch 
				  & ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB) ;
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			BlitCMDQueSpace = SendCMD(hBlitDC, hBlitDC->DBaseAddr);
		}

		if (hBlitDC->IsUsingSDSpecificEndian)
		{				
			DEndianCtrl = ((hBlitDC->DByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
				          ((hBlitDC->DNibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
				          ((hBlitDC->DTwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
				          ((1 & 0x1)    << ENDIAN_ENABLE_BIT);
		}

		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			BlitCMD = ((CMD_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hBlitDC->DPixelNumOneLine & ((1 << 12) -1)) << 12)|
				((hBlitDC->DTotalLineNum & ((1 << 12) -1)) << 0) ;
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			BlitCMD = ((CMD_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
			     ((hBlitDC->DPixelNumOneLine
		                           & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);

			BlitCMD = ((CMD_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hBlitDC->DTotalLineNum
				  & ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB) ;
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
		}
	}

	/*D Color Info should not be A0, no Default Color*/
	if ((BlitDCType & BLIT_DC_D_COLOR_FORMAT))
	{
		if (hBlitDC->DColorType != CSBLIT_COLOR_TYPE_ARGB1555)
		{
			BlitCMD = ((CMD_D_FORMAT   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					 ((hBlitDC->DColorType
					                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
		}
		else
		{
			BlitCMD = ((CMD_D_FORMAT   & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					 ((hBlitDC->DColorType
					                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
					 ((hBlitDC->DAlpha0Min
					                     & ((1 << CMD_FORMAT_ALPHA0_MIN_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MIN_LSB)|
					 ((hBlitDC->DAlpha0Max
					                     & ((1 << CMD_FORMAT_ALPHA0_MAX_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MAX_LSB);
		}	
		BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
	}
	//Set Blit Interrupt Mode and Pipeline Enable
	//Send Scalor Command
/*
	u32 S0ScalorEnable;
	u32 IsUpdateHFIRCoeff;
	u32 IsUpdateVFIRCoeff;
	u32 HInitialPhase;
	u32 VInitialPhase;
	u32 HFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_H_FIR_TAP_NUM];
	u32 VFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_V_FIR_TAP_NUM];
*/
	//Update VFIR Coeff
	if ( (hBlitDC->UpdateHFIRCoeff) && (BlitDCType & BLIT_DC_SCALOR_HFIR_COFF))
	{
		u32 Phase = 0;
		u32 Tap   = 0;
		for (Phase = 0; Phase < SCALOR_PAHSE_NUM; Phase++)
		{
			for (Tap =0; Tap < SCALOR_H_FIR_TAP_NUM; Tap++)
			{
				BlitCMD = ((CMD_HFIR_COEFFICIETN_DATA & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				 		 ((Phase & ((1 << CMD_FIR_COEFF_PHASE_IDX_WIDTH) -1)) << CMD_FIR_COEFF_PHASE_IDX_LSB)|
				 		 ((Tap   & ((1 << CMD_FIR_COEFF_TAP_IDX_WIDTH) -1))   << CMD_FIR_COEFF_TAP_IDX_LSB)|
				 		 ((hBlitDC->HFIRCoeffTable[Phase][Tap]
				 		         & ((1 << CMD_FIR_COEFF_VALUE_WIDTH) -1))     << CMD_FIR_COEFF_VALUE_LSB);
				BlitCMDQueSpace = SendCMD (hBlitDC, BlitCMD);
			}
		}
	}
	//Update HFIR Coeff
	if ((hBlitDC->UpdateVFIRCoeff) && (BlitDCType & BLIT_DC_SCALOR_VFIR_COFF))
	{
		u32 Phase = 0;
		u32 Tap   = 0;
		for (Phase = 0; Phase < SCALOR_PAHSE_NUM; Phase++)
		{
			for (Tap =0; Tap < SCALOR_V_FIR_TAP_NUM; Tap++)
			{
				BlitCMD = ((CMD_VFIR_COEFFICIETN_DATA & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				 		 ((Phase & ((1 << CMD_FIR_COEFF_PHASE_IDX_WIDTH) -1)) << CMD_FIR_COEFF_PHASE_IDX_LSB)|
				 		 ((Tap   & ((1 << CMD_FIR_COEFF_TAP_IDX_WIDTH) -1))   << CMD_FIR_COEFF_TAP_IDX_LSB)|
				 		 ((hBlitDC->VFIRCoeffTable[Phase][Tap]
				 		         & ((1 << CMD_FIR_COEFF_VALUE_WIDTH) -1))     << CMD_FIR_COEFF_VALUE_LSB);
				BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
			}
		}
	}
	if (hBlitDC->S0ScalorEnable)
	{
		//Scalor Source Pixel Map
		//Send InitialPhase
		if ((BlitDCType & BLIT_DC_SCALOR_INIT_PHASE))
		{
			BlitCMD = ((CMD_SCALOR_INITIAL_PHASE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				 		 ((hBlitDC->HInitialPhase 
				 		         & ((1 << CMD_SCALOR_H_INIT_PHASE_WIDTH) -1)) << CMD_SCALOR_H_INIT_PHASE_LSB)|
				 		 ((hBlitDC->VInitialPhase    
				 		         & ((1 << CMD_SCALOR_V_INIT_PHASE_WIDTH) -1)) << CMD_SCALOR_V_INIT_PHASE_LSB);
			BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);
		}
	}	
	#ifdef CHECK_REG_AFTER_SEND_CMD
	if (!RegCheckPreStartup(hBlitDC, BlitDCType))
	{
		printk("##Failed to Check Register Before Startup\n");
		return 0;
	}
	#endif

	//Send Startup Command
	if ((BlitDCType & BLIT_DC_CTRL))
	{
		//Send Endian Mode		
		GlobalEndianCtrl = ((hBlitDC->DByteLittleNotBigEndian & 0x1) << BYTE_ENDIAN_BIT  )|
			((hBlitDC->DNibbleLittleNotBigEndian & 0x1) << NIBBLE_ENDIAN_BIT)|
			((hBlitDC->DTwoBytesLittleNotBigEndian  & 0x1) << TWO_BYTE_ENDIAN_BIT);

		BlitCMD     = ((CMD_ENDIAN_CTRL  & ((1 << CMD_IDX_WIDTH)-1)) << CMD_IDX_LSB)|
			((S0EndianCtrl     & ((1 << CMD_S0_ENDIAN_WIDTH)-1)) << CMD_S0_ENDIAN_LSB)|
			((S1EndianCtrl     & ((1 << CMD_S1_ENDIAN_WIDTH)-1)) << CMD_S1_ENDIAN_LSB)|
			((DEndianCtrl      & ((1 << CMD_D_ENDIAN_WIDTH)-1)) << CMD_D_ENDIAN_LSB )|
			((GlobalEndianCtrl & ((1 << CMD_GLOBAL_ENDIAN_WIDTH)-1)) << CMD_GLOBAL_ENDIAN_LSB); 

		BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD);	

		//Send Ctrl and Startup CMD
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			ScanCtrl = ((hBlitDC->S0VReverseScan   & (1)) << S0_REVERSE_SCAN_BIT)  |
				((hBlitDC->S1VReverseScan   & (1)) << S1_REVERSE_SCAN_BIT)  |
				((hBlitDC->DVReverseScan    & (1)) << D_REVERSE_SCAN_BIT);
		}

		CompositorOperation = 
			           ((hBlitDC->CompositorEnable & (1)) << COMPOSITOR_ENABLE_BIT)|
		               ((hBlitDC->S0OnTopS1        & (1)) << S0_ON_TOP_S1_BIT);

		S1Operation =  ((hBlitDC->S1Enable         & (1)) << S_ENABLE_BIT)|
		               ((hBlitDC->S1ColorKeyRGB.RGBEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
		               ((hBlitDC->S1ClutEnable     & (1)) << S_CLUT_ENABLE_BIT)    |
		               ((hBlitDC->S1FetchDram      & (1)) << S_FETCH_DATA_BIT);

		S0Operation =  ((hBlitDC->S0Enable         & (1)) << S_ENABLE_BIT)|
		               ((hBlitDC->S0ColorKeyRGB.RGBEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
		               ((hBlitDC->S0ClutEnable     & (1)) << S_CLUT_ENABLE_BIT)    |
		               ((hBlitDC->S0FetchDram      & (1)) << S_FETCH_DATA_BIT);

		BlitCMD = ((CMD_STARTUP  & ((1 << CMD_IDX_WIDTH)    -1)) << CMD_IDX_LSB)    |
				 ((ScanCtrl     & ((1 << SCAN_CTRL_WIDTH)  -1)) << SCAN_CTRL_LSB)  |
				 ((hBlitDC->S0ScalorEnable & (0x1))   << S0_SCAL_ENA_BIT)|
			     ((hBlitDC->InterruptEnable
				                & ((1 << INT_ENABLE_WIDTH) -1)) << INT_ENABLE_LSB) |
				 ((hBlitDC->ROPAlphaCtrl
				                & ((1 << ROP_ALPHA_CTRL_WIDTH)    -1)) << ROP_ALPHA_CTRL_LSB) |
				 ((hBlitDC->RopValue
				                & ((1 << ROP_VAL_WIDTH)    -1)) << ROP_VAL_LSB)    |
				 ((CompositorOperation
				                & ((1 << CMP_OPT_WIDTH)    -1)) << CMP_OPT_LSB)    |
				 ((S1Operation  & ((1 << S1_OPT_WIDTH)     -1)) << S1_OPT_LSB)     |
				 ((S0Operation  & ((1 << S0_OPT_WIDTH)     -1)) << S0_OPT_LSB);
		//Check Point
		BlitCMDQueSpace = SendCMD(hBlitDC, BlitCMD/*0xa080*/);
		CMDGroupCntInc();
	}
	
	/* store the BlitDCType */
	hBlitDC->BlitDCType = BlitDCType;

	return BlitCMDQueSpace;
}

s32 WaitBlitToBeIdle(CSBlit_HW_Device_t *hBlitDC) //Boolean Type Return
{
	//Run Time Info
	s32 hResult = 1;
	s32 BlitIdelStatus   = 0;
	s32 AHBErrStatus    = 0;
	s32 BlitErrStatus    = 0;
	s32 InterruptStatus = 0; 
	u32 BlitCMDQueuSpace = 0;

	s32 InterruptEnable = 0;

	u32 BlitStatusRegValue = 0;
	u32 BlitInfoRegValue   = 0;
	u32 TimeCnt = 0;
	u32 TimeOut = 0;
	if (hBlitDC == NULL)
	{
		TimeOut = 100;
	}
	else
	{
		u32 MaxWidth  = 0;
		u32 MaxHeight = 0;
		MaxWidth  = hBlitDC->S0PixelNumOneLine > hBlitDC->S1PixelNumOneLine ?
			        hBlitDC->S0PixelNumOneLine : hBlitDC->S1PixelNumOneLine ;
		MaxWidth  = MaxWidth > hBlitDC->DPixelNumOneLine ?
			        MaxWidth : hBlitDC->DPixelNumOneLine ;
		MaxHeight = hBlitDC->S0TotalLineNum > hBlitDC->S1TotalLineNum ?
			        hBlitDC->S0TotalLineNum : hBlitDC->S1TotalLineNum ;
		MaxHeight = MaxHeight > hBlitDC->DTotalLineNum ?
			        MaxHeight : hBlitDC->DTotalLineNum ;
		TimeOut = MaxWidth * MaxHeight *10; //One Circle One Pixel, for Scalor 3 Circle/pixel, so *10 is Enough
		if (TimeOut < 100) TimeOut = 100;
	}

	do 
	{
		BlitInfoRegValue   = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMD_INFO);
		InterruptEnable   = (BlitInfoRegValue >> INT_ENABLE_LSB) &((1 << INT_ENABLE_WIDTH) -1); 
		//Check Blit Status
		BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
		BlitIdelStatus     = (BlitStatusRegValue >> BLIT_STATUS_BIT ) &(1);
		AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
		BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT    ) &(1);
		InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
		BlitCMDQueuSpace   = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT) 
			                                   >> BLIT_REG_CMDQUE_EMPTY_CNT_LSB) 
			                                   & ((1 << BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
		DEBUG_PRINTF("3BlitIdelStatus = 0x%x, BlitCMDQueuSpace = 0x%x\n",BlitIdelStatus,BlitCMDQueuSpace);
		if(AHBErrStatus)
		{
			printk("##ERROR: Blit AHB status Err\n");
			return 0;
		}
		if (BlitErrStatus)
		{
			printk("##ERROR: Blit Command Interpret Err\n");
			hResult = 0;
		}
		if (InterruptEnable || BlitErrStatus) //if Err Must Be a Interrupt 
		{
			if (InterruptStatus == 1)
			{
				//Clear Interrupt
				WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_INT_CLEAR, 1);//Write Any Value
			}
		}
		TimeCnt++;
		//Some User Definde Sleep Function	
	}while(!BlitIdelStatus && TimeCnt < TimeOut);

	if(hBlitDC != NULL)
	{
		hBlitDC->BlitIdelStatus   = BlitIdelStatus;
		hBlitDC->AHBErrStatus    = AHBErrStatus;
		hBlitDC->BlitErrStatus    = BlitErrStatus;
		hBlitDC->InterruptStatus = InterruptStatus; 
		hBlitDC->CMDCnt          = BlitCMDQueuSpace;
	}

	if (TimeCnt >= TimeOut)
	{
		int i = 0;
		printk("##ERROR: TimeOut (TimeCnt = %d)\n", TimeCnt);
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			hResult = 0;
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			printk("Try To Reset Blit Engine\n");
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 1);
			for (i =0; i < 10; i++)
			{
				BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
				BlitIdelStatus     = (BlitStatusRegValue >> BLIT_STATUS_BIT ) &(1);		
				if (BlitIdelStatus)
				{
					break;
				}
			}		
			AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
			BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT    ) &(1);
			InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
			if (!BlitIdelStatus || AHBErrStatus || BlitErrStatus || InterruptStatus)
			{
				printk("##ERROR: Failed to Reset Blit AfterTimeOut\n");
			}
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 0);
			CMDGroupCntSyncHW(hBlitDC);
			hResult = 0;
		}
	}
	else
	{
		/*--------------------------------------------------
		* printk("Blit Idel Waitting Time = %d\n", TimeCnt);
		*--------------------------------------------------*/
	}
	if(AHBErrStatus)
	{
		printk("##ERROR: Blit AHBErr\n");
		hResult = 0;
	}
	if (BlitErrStatus)
	{
		printk("##ERROR: Blit Command Interpret Err\n");
		hResult = 0;
	}

	return hResult;
}

/* This 'waittobeidle' only for scalor operation */
s32 WaitBlitScalorToBeIdle(CSBlit_HW_Device_t *hBlitDC) //Boolean Type Return
{
	//Run Time Info
	s32 hResult = 1;
	s32 BlitIdelStatus   = 0;
	s32 AHBErrStatus    = 0;
	s32 BlitErrStatus    = 0;
	s32 InterruptStatus = 0; 
	u32 BlitCMDQueuSpace = 0;

	s32 InterruptEnable = 0;

	u32 BlitStatusRegValue = 0;
	u32 BlitInfoRegValue   = 0;
	u32 TimeCnt = 0;
	u32 TimeOut = 0;
	if (hBlitDC == NULL)
	{
		TimeOut = 100;
	}
	else
	{
		u32 MaxWidth  = 0;
		u32 MaxHeight = 0;
		MaxWidth  = hBlitDC->S0PixelNumOneLine > hBlitDC->S1PixelNumOneLine ?
			        hBlitDC->S0PixelNumOneLine : hBlitDC->S1PixelNumOneLine ;
		MaxWidth  = MaxWidth > hBlitDC->DPixelNumOneLine ?
			        MaxWidth : hBlitDC->DPixelNumOneLine ;
		MaxHeight = hBlitDC->S0TotalLineNum > hBlitDC->S1TotalLineNum ?
			        hBlitDC->S0TotalLineNum : hBlitDC->S1TotalLineNum ;
		MaxHeight = MaxHeight > hBlitDC->DTotalLineNum ?
			        MaxHeight : hBlitDC->DTotalLineNum ;
		TimeOut = MaxWidth * MaxHeight /8; //One Circle One Pixel, for Scalor 3 Circle/pixel, so *10 is Enough
		if (TimeOut < 100) TimeOut = 100;
	}

	do
	{
		BlitInfoRegValue   = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMD_INFO);
		InterruptEnable   = (BlitInfoRegValue >> INT_ENABLE_LSB) &((1 << INT_ENABLE_WIDTH) -1); 
		//Check Blit Status
		BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
		BlitIdelStatus     = (BlitStatusRegValue >> BLIT_STATUS_BIT ) &(1);
		AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
		BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT    ) &(1);
		InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
		BlitCMDQueuSpace   = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT) 
			                                   >> BLIT_REG_CMDQUE_EMPTY_CNT_LSB) 
			                                   & ((1 << BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
		if(AHBErrStatus)
		{
			printk("##ERROR: Blit AHB status Err\n");
			return 0;
		}
		if (BlitErrStatus)
		{
			printk("##ERROR: Blit Command Interpret Err\n");
			hResult = 0;
		}
		if (InterruptEnable || BlitErrStatus) //if Err Must Be a Interrupt 
		{
			if (InterruptStatus == 1)
			{
				//Clear Interrupt
				WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_INT_CLEAR, 1);//Write Any Value
			}
		}
		TimeCnt++;
		//Some User Definde Sleep Function	
	}while(!BlitIdelStatus && TimeCnt < TimeOut);

	if(hBlitDC != NULL)
	{
		hBlitDC->BlitIdelStatus   = BlitIdelStatus;
		hBlitDC->AHBErrStatus    = AHBErrStatus;
		hBlitDC->BlitErrStatus    = BlitErrStatus;
		hBlitDC->InterruptStatus = InterruptStatus; 
		hBlitDC->CMDCnt          = BlitCMDQueuSpace;
	}

	if (TimeCnt >= TimeOut)
	{
		int i = 0;
		printk("##ERROR: TimeOut (TimeCnt = %d)\n", TimeCnt);
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			hResult = 0;
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			printk("Try To Reset Blit Engine\n");
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 1);
			for (i =0; i < 10; i++)
			{
				BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
				BlitIdelStatus     = (BlitStatusRegValue >> BLIT_STATUS_BIT ) &(1);		
				if (BlitIdelStatus)
				{
					break;
				}
			}		
			AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
			BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT    ) &(1);
			InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
			if (!BlitIdelStatus || AHBErrStatus || BlitErrStatus || InterruptStatus)
			{
				printk("##ERROR: Failed to Reset Blit AfterTimeOut\n");
			}
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 0);
			CMDGroupCntSyncHW(hBlitDC);
			hResult = 0;
		}
	}

	if(AHBErrStatus)
	{
		printk("##ERROR: Blit AHBErr\n");
		hResult = 0;
	}
	if (BlitErrStatus)
	{
		printk("##ERROR: Blit Command Interpret Err\n");
		hResult = 0;
	}

	return hResult;
}

static u32 sBlitCMDQueueDepth = 0; // 0 means not initial
static void SetCMDQueueDepth(u32 BlitCMDQueueDepth)
{
	sBlitCMDQueueDepth = BlitCMDQueueDepth;
}

u32 GetCMDQueueDepth()
{
	return sBlitCMDQueueDepth ;
}

void SWReset(CSBlit_HW_Device_t *hBlitDC)
{
	u32 BlitCMDQueueDepth = 0;
#ifdef RESET_GFX_ONLY_ONE_TIME
	if(IsBlitSWReset && IsBlitHWReset)
	{
		return;
	}
	else
	{
		IsBlitSWReset++;
	}	
#endif
	WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 1);
	//Waitting Blit to be Idel
	WaitBlitToBeIdle(hBlitDC); // To Ensure Reset is Stable, must Wait for the Idel
	WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_SW_RESET, 0);
	BlitCMDQueueDepth = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT) 
			              >> BLIT_REG_CMDQUE_EMPTY_CNT_LSB) 
			              & ((1 << BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
	/*assert(BlitCMDQueueDepth != 0);*/
	SetCMDQueueDepth(BlitCMDQueueDepth);
	CMDGroupCntSyncHW(hBlitDC);
}

s32 WaitCMDQueueEmpty(CSBlit_HW_Device_t *hBlitDC)
{
	u32 BlitCMDQueueDepth = 0;
	u32 BlitCMDQueuSpace  = 0;
	u32 BlitStatusRegValue = 0;
	s32 AHBErrStatus    = 0;
	s32 BlitErrStatus    = 0;
	s32 InterruptStatus = 0; 
	//s32 InterruptEnable = 0;
	//Clear Interrupt and ERR
	BlitStatusRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_STATUS);
	AHBErrStatus      = (BlitStatusRegValue >> BLIT_AHB_ERR_BIT) &(1);
	BlitErrStatus      = (BlitStatusRegValue >> BLIT_ERR_BIT    ) &(1);
	InterruptStatus   = (BlitStatusRegValue >> BLIT_INT_STATUS_BIT) &(1);
	if(AHBErrStatus)
	{
		printk("##ERROR: Blit AHB status Err\n");
		return 0;		/* error happened */

	}
	if (InterruptStatus == 1)
	{
		if (BlitErrStatus)
		{				
			printk("##ERROR: Blit Command Interpret Err\n");
		}
		//Clear Interrupt		
		WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_INT_CLEAR, 1);//Write Any Value
	}
	BlitCMDQueueDepth = GetCMDQueueDepth();
	//assert(BlitCMDQueueDepth != 0);
	if (BlitCMDQueueDepth == 0)
	{
		return 0;
	}
	do
	{
		BlitCMDQueuSpace = (ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CMDQUE_EMPTY_CNT) 
			              >> BLIT_REG_CMDQUE_EMPTY_CNT_LSB) 
			              & ((1 << BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
	}while(BlitCMDQueuSpace < BlitCMDQueueDepth );
	/*assert(BlitCMDQueuSpace == BlitCMDQueueDepth );*/
	/*if (BlitCMDQueuSpace > BlitCMDQueueDepth )*/
	if (BlitCMDQueuSpace != BlitCMDQueueDepth )
	{		
		printk("##ERROR: BlitCMDQueuSpace != BlitCMDQueueDepth \n");
		return 0;
	}
	return (BlitCMDQueuSpace == BlitCMDQueueDepth );	
}

static s32 BlitRegCheck (CSBlit_HW_Device_t *hBlitDC, u32 RegAddr, u32 ExpectedValue)  
{ 
	u32 BlitRegValue = 0;
	BlitRegValue = ReadRegMem32LE(hBlitDC->RegBaseAddr + (RegAddr));
	if (BlitRegValue != ExpectedValue)
	{
		printk("RegAddr(%08x),BlitRegValue(%08x)!= ExpectedValue(%08x)\n",
			         RegAddr,BlitRegValue, ExpectedValue);
	}
	//assert(BlitRegValue == (ExpectedValue));
	return(BlitRegValue == (ExpectedValue));
}

s32 RegCheckClutTable(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType, s32 IsClut4orClut8) //0: Clut4, 1: Clut8
{
	if (hBlitDC == NULL) return 0;
	if (!WaitCMDQueueEmpty(hBlitDC)) return 0;
	//Clut Table Update
	//Clut4
	if (hBlitDC->UpdateClut4Table && (BlitDCType & BLIT_DC_CLUT4_TABLE) && (IsClut4orClut8 == 1))
	{
		u32 Clut4TableIdx  = 0;
		u32 Clut4TableData = 0;
		if (hBlitDC->UpdateClut4TableMode)//Update According Idx
		{
			Clut4TableIdx = (hBlitDC->Clut4TableIdx & ((1 << CMD_CLUT4_IDX_WIDTH) -1));
			//assert(Clut4TableIdx < CLUT4_TABLE_LEN);
			if(Clut4TableIdx < CLUT4_TABLE_LEN) return 0;
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CLUT_IDX, Clut4TableIdx);
			//Change to ARGB8565 Type
			Clut4TableData = (hBlitDC->Clut4Table[Clut4TableIdx]) & 
			                 ((0xFF000000)|(0x1F0000 << 3)|(0x3F00 << 2)|(0x1F<<3));
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_CLUT_ENTRY), (Clut4TableData))) return 0;			
		}
		else
		{
			for (Clut4TableIdx = 0; Clut4TableIdx < CLUT4_TABLE_LEN; Clut4TableIdx++)
			{
				WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CLUT_IDX, Clut4TableIdx);
				//Change to ARGB8565 Type
				Clut4TableData = (hBlitDC->Clut4Table[Clut4TableIdx]) & 
				                 ((0xFF000000)|(0x1F0000 << 3)|(0x3F00 << 2)|(0x1F<<3));
				if (!BlitRegCheck(hBlitDC, (BLIT_REG_CLUT_ENTRY), (Clut4TableData))) return 0;			
			}
		}
	}
		//Clut8
	if (hBlitDC->UpdateClut8Table && (BlitDCType & BLIT_DC_CLUT8_TABLE) && (IsClut4orClut8 == 0))
	{
		u32 Clut8TableIdx = 0;
		u32 Clut8TableData = 0;
		if (hBlitDC->UpdateClut8TableMode)//Update According Idx
		{
			Clut8TableIdx = (hBlitDC->Clut8TableIdx & ((1 << CMD_CLUT8_IDX_WIDTH) -1));
			//assert(Clut8TableIdx < CLUT8_TABLE_LEN);
			if(Clut8TableIdx < CLUT8_TABLE_LEN) return 0;
			WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CLUT_IDX, Clut8TableIdx);
			//Change to ARGB8565 Type
			Clut8TableData = (hBlitDC->Clut8Table[Clut8TableIdx]) & 
			                 ((0xFF000000)|(0x1F0000 << 3)|(0x3F00 << 2)|(0x1F<<3));
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_CLUT_ENTRY), (Clut8TableData))) return 0;			
		}
		else
		{
			for (Clut8TableIdx = 0; Clut8TableIdx < CLUT8_TABLE_LEN; Clut8TableIdx++)
			{
				WriteRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_CLUT_IDX, Clut8TableIdx);
				//Change to ARGB8565 Type
				Clut8TableData = (hBlitDC->Clut8Table[Clut8TableIdx]) & 
				                 ((0xFF000000)|(0x1F0000 << 3)|(0x3F00 << 2)|(0x1F<<3));
				if (!BlitRegCheck(hBlitDC, (BLIT_REG_CLUT_ENTRY), (Clut8TableData))) return 0;			
			}
		}
	}	
	return 1;
}


s32 RegCheckPosStartup(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType)
{
	u32 S0EndianCtrl     = 0;
	u32 S1EndianCtrl     = 0;
	u32 DEndianCtrl      = 0;
	u32 GlobalEndianCtrl = 0;
	u32 BlitRegValue      = 0;
	if (hBlitDC == NULL) return 0;
	if (!WaitCMDQueueEmpty(hBlitDC)) return 0;
	
	if (hBlitDC->S0Enable || hBlitDC->S0ScalorEnable)
	{
		if ((hBlitDC->S0FetchDram || hBlitDC->S0ScalorEnable) && 
			(BlitDCType & BLIT_DC_S0_DRAM_PARA) )
		{
			if (hBlitDC->IsUsingSDSpecificEndian)
			{
				S0EndianCtrl = ((hBlitDC->S0ByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
					           ((hBlitDC->S0NibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
					           ((hBlitDC->S0TwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
					           ((1 & 0x1)    << ENDIAN_ENABLE_BIT);
			}
		}
	}
	if (hBlitDC->S1Enable)
	{
		if (hBlitDC->S1FetchDram && (BlitDCType & BLIT_DC_S1_DRAM_PARA) )
		{
			if (hBlitDC->IsUsingSDSpecificEndian)
			{
				S1EndianCtrl = ((hBlitDC->S1ByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
					           ((hBlitDC->S1NibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
					           ((hBlitDC->S1TwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
					           ((1 & 0x1)    << ENDIAN_ENABLE_BIT);
			}
		}
	}
	if ((BlitDCType & BLIT_DC_D_DRAM_PARA))
	{
		if (hBlitDC->IsUsingSDSpecificEndian)
		{
			DEndianCtrl = ((hBlitDC->DByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
					      ((hBlitDC->DNibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
					      ((hBlitDC->DTwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
					      ((1 & 0x1)    << ENDIAN_ENABLE_BIT);
		}	
	}
	//Scalor S0, S1 and D Dram Access Para Check
	if (!hBlitDC->S0ScalorEnable)
	{
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_PIXEL_NUM_PER_LINE), ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_PIXEL_NUM_PER_LINE))) return 0;

		if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_TOTAL_LINE_NUM)    , ReadRegMem32LE(hBlitDC->RegBaseAddr + BLIT_REG_TOTAL_LINE_NUM))) return 0;
	}
	//Ctrl Status Check
	if ((BlitDCType & BLIT_DC_CTRL))
	{
		u32 CompositorOperation = 0;
		u32 S0Operation     = 0;
		u32 S1Operation     = 0;
		u32 ScanCtrl        = 0;
		u32 BlitCMDGroupCnt  = 0;
		//Send Endian Mode
		//Endian Check
		GlobalEndianCtrl = ((hBlitDC->DByteLittleNotBigEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
			               ((hBlitDC->DNibbleLittleNotBigEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
			               ((hBlitDC->DTwoBytesLittleNotBigEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT);
		S0EndianCtrl = ((S0EndianCtrl >> ENDIAN_ENABLE_BIT)& 0x1) ?
			            (S0EndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) : 
					    (GlobalEndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) ;
	  	S1EndianCtrl = ((S1EndianCtrl >> ENDIAN_ENABLE_BIT)& 0x1) ?
						(S1EndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) : 
						(GlobalEndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) ;
		DEndianCtrl  = ((DEndianCtrl >> ENDIAN_ENABLE_BIT)& 0x1) ?
						(DEndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) : 
						(GlobalEndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) ;
		if (hBlitDC->S0ScalorEnable)
		{
			S1EndianCtrl = S0EndianCtrl;
		}
		BlitRegValue = ((S0EndianCtrl & ((1 << REG_ENDIAN_CTRL_S0_WIDTH) - 1)) << (REG_ENDIAN_CTRL_S0_LSB)) |
					  ((S1EndianCtrl & ((1 << REG_ENDIAN_CTRL_S1_WIDTH) - 1)) << (REG_ENDIAN_CTRL_S1_LSB)) |
					  ((DEndianCtrl  & ((1 << REG_ENDIAN_CTRL_D_WIDTH ) - 1)) << (REG_ENDIAN_CTRL_D_LSB)) ;

		if (!BlitRegCheck(hBlitDC, (BLIT_REG_ENDIAN_CTRL), (BlitRegValue))) return 0;
		
		//Send Ctrl and Startup CMD
		if (hBlitDC->S0ScalorEnable)
		{
			ScanCtrl   =   ((hBlitDC->S0VReverseScan   & (1)) << S0_REVERSE_SCAN_BIT)  |
				           ((hBlitDC->S0VReverseScan   & (1)) << S1_REVERSE_SCAN_BIT)  | //Scalor S0 S1 Scan the Same
				           ((hBlitDC->DVReverseScan    & (1)) << D_REVERSE_SCAN_BIT);
		}
		else
		{
			ScanCtrl   =   ((hBlitDC->S0VReverseScan   & (1)) << S0_REVERSE_SCAN_BIT)  |
				           ((hBlitDC->S1VReverseScan   & (1)) << S1_REVERSE_SCAN_BIT)  |
				           ((hBlitDC->DVReverseScan    & (1)) << D_REVERSE_SCAN_BIT);
		}
		if (hBlitDC->S0ScalorEnable)
		{
			CompositorOperation = 0;		
		}
		else
		{
			CompositorOperation = 
				           ((hBlitDC->CompositorEnable & (1)) << COMPOSITOR_ENABLE_BIT)|
			               ((hBlitDC->S0OnTopS1        & (1)) << S0_ON_TOP_S1_BIT);
		}
		if (hBlitDC->S0ScalorEnable)
		{
			S1Operation = 0;
		}
		else
		{
			S1Operation =  ((hBlitDC->S1Enable         & (1)) << S_ENABLE_BIT)|
		                   ((hBlitDC->S1ColorKeyRGB.RGBEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
		                   ((hBlitDC->S1ClutEnable     & (1)) << S_CLUT_ENABLE_BIT)    |
		                   ((hBlitDC->S1FetchDram      & (1)) << S_FETCH_DATA_BIT);
			
		}

		S0Operation =  ((hBlitDC->S0Enable         & (1)) << S_ENABLE_BIT)|
		               ((hBlitDC->S0ColorKeyRGB.RGBEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
		               ((hBlitDC->S0ClutEnable     & (1)) << S_CLUT_ENABLE_BIT)    |
		               ((hBlitDC->S0FetchDram      & (1)) << S_FETCH_DATA_BIT);
		
		BlitCMDGroupCnt = GetCMDGroupCnt();
		BlitRegValue  = ((BlitCMDGroupCnt & ((1 << REG_INFO_CMD_CNT_WIDTH)- 1)) << REG_INFO_CMD_CNT_LSB)|
				       ((ScanCtrl     & ((1 << SCAN_CTRL_WIDTH)  -1)) << SCAN_CTRL_LSB)  |
				       ((hBlitDC->S0ScalorEnable & (0x1))   << S0_SCAL_ENA_BIT)|
			           ((hBlitDC->InterruptEnable
				                & ((1 << INT_ENABLE_WIDTH) -1)) << INT_ENABLE_LSB) |
				       ((hBlitDC->ROPAlphaCtrl
				                & ((1 << ROP_ALPHA_CTRL_WIDTH)    -1)) << ROP_ALPHA_CTRL_LSB) |
				       ((hBlitDC->RopValue
				                & ((1 << ROP_VAL_WIDTH)    -1)) << ROP_VAL_LSB)    |
				       ((CompositorOperation
				                & ((1 << CMP_OPT_WIDTH)    -1)) << CMP_OPT_LSB)    |
				       ((S1Operation  & ((1 << S1_OPT_WIDTH)     -1)) << S1_OPT_LSB)     |
				       ((S0Operation  & ((1 << S0_OPT_WIDTH)     -1)) << S0_OPT_LSB);
		//Check Point
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_CMD_INFO), (BlitRegValue))) return 0;
	}
	return 1;
	
}

s32 RegCheckPreStartup(CSBlit_HW_Device_t *hBlitDC, BLIT_DC_UPDATE_TYPE BlitDCType) //Boolean Type Return
{
	u32 BlitRegValue      = 0;
	if (hBlitDC == NULL) return 0;
	if (!WaitCMDQueueEmpty(hBlitDC)) return 0;
		
	if (hBlitDC->S0Enable || hBlitDC->S0ScalorEnable)
	{
		if ((hBlitDC->S0FetchDram || hBlitDC->S0ScalorEnable) && 
			(BlitDCType & BLIT_DC_S0_DRAM_PARA) )
		{	
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_SKIP_PIXEL), 
				(hBlitDC->S0SkipPixelLine & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)))) return 0;
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_LINE_PITCH), 
				(hBlitDC->S0LinePitch & ((1 << CMD_PITCH_WIDTH) -1)))) return 0;
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_ADDR)      , 
				(hBlitDC->S0BaseAddr))) return 0;			
			
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_PIXEL_NUM_PER_LINE), 
				(hBlitDC->S0PixelNumOneLine & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)))) return 0;

			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_TOTAL_LINE_NUM), 
				(hBlitDC->S0TotalLineNum & ((1 << CMD_LINE_NUM_WIDTH) -1)))) return 0;
		}
		//Color Info
		if (BlitDCType & (BLIT_DC_S0_COLOR_FORMAT))
		{
			if (hBlitDC->S0ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
			{
				BlitRegValue = ((hBlitDC->S0ColorType
						                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
			}
			else
			{
				BlitRegValue = ((hBlitDC->S0ColorType
						                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
						      ((hBlitDC->S0Alpha0
						                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						      ((hBlitDC->S0Alpha1
						                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
			}				
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_COLOR_FORMAT), BlitRegValue)) return 0;
				
			if (BlitDCType & BLIT_DC_S0_DEFAULT_COLOR)
			{	
				BlitRegValue = (((hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha >> (8-8))
					                     & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.R >> (8-8))
					                     & ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.G >> (8-8))
					                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
					     (((hBlitDC->S0DefaultColor.Value.ARGB8888.B >> (8-8))
					                     & ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);
				
				if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_DEFAULT_COLOR), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
			}
		}
		
		//ColorKey ARGB8888
		if (hBlitDC->S0ColorKeyRGB.RGBEnable && (BlitDCType & BLIT_DC_S0_COLORKEY_PARA))
		{
			BlitRegValue =  ((hBlitDC->S0ColorKeyRGB.RMin
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				           ((hBlitDC->S0ColorKeyRGB.GMin
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				           ((hBlitDC->S0ColorKeyRGB.BMin
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_COLOR_KEY_MIN), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
			
			BlitRegValue = ((hBlitDC->S0ColorKeyRGB.RMax
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				          ((hBlitDC->S0ColorKeyRGB.GMax
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				          ((hBlitDC->S0ColorKeyRGB.BMax
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S0_COLOR_KEY_MAX), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
		}		
	}	
	//S1 Related Register Check
	if (hBlitDC->S1Enable)
	{
		if (hBlitDC->S1FetchDram && (BlitDCType & BLIT_DC_S1_DRAM_PARA) )
		{	
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_SKIP_PIXEL), 
				(hBlitDC->S1SkipPixelLine & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)))) return 0;
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_LINE_PITCH), 
				(hBlitDC->S1LinePitch & ((1 << CMD_PITCH_WIDTH) -1)))) return 0;
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_ADDR)      , 
				(hBlitDC->S1BaseAddr))) return 0;
			
			if (!(BlitDCType & BLIT_DC_D_DRAM_PARA))
			{
				if (!BlitRegCheck(hBlitDC, (BLIT_REG_PIXEL_NUM_PER_LINE), 
					(hBlitDC->S1PixelNumOneLine & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)))) return 0;

				if (!BlitRegCheck(hBlitDC, (BLIT_REG_TOTAL_LINE_NUM), 
					(hBlitDC->S1TotalLineNum & ((1 << CMD_LINE_NUM_WIDTH) -1)))) return 0;
			}
		}
		//Color Info
		if (BlitDCType & (BLIT_DC_S1_COLOR_FORMAT))
		{
			if (hBlitDC->S1ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
			{
				BlitRegValue = ((hBlitDC->S1ColorType
						                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
			}
			else
			{
				BlitRegValue = ((hBlitDC->S1ColorType
						                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
						      ((hBlitDC->S1Alpha0
						                     & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						      ((hBlitDC->S1Alpha1
						                     & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
			}				
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_COLOR_FORMAT), BlitRegValue)) return 0;
			
			if (BlitDCType & BLIT_DC_S1_DEFAULT_COLOR)
			{
				
				BlitRegValue = (((hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha >> (8-8))
					                     & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
					     (((hBlitDC->S1DefaultColor.Value.ARGB8888.R >> (8-8))
					                     & ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB)|
					     (((hBlitDC->S1DefaultColor.Value.ARGB8888.G >> (8-8))
					                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
					     (((hBlitDC->S1DefaultColor.Value.ARGB8888.B >> (8-8))
					                     & ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);
				
				if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_DEFAULT_COLOR), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
			}
		}
		
		//ColorKey ARGB8888
		if (hBlitDC->S1ColorKeyRGB.RGBEnable && (BlitDCType & BLIT_DC_S1_COLORKEY_PARA))
		{
			BlitRegValue =  ((hBlitDC->S1ColorKeyRGB.RMin
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				           ((hBlitDC->S1ColorKeyRGB.GMin
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				           ((hBlitDC->S1ColorKeyRGB.BMin
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_COLOR_KEY_MIN), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
			
			BlitRegValue = ((hBlitDC->S1ColorKeyRGB.RMax
				                     & ((1 << CMD_RED_WIDTH) -1))   << CMD_RED_LSB)    |
				          ((hBlitDC->S1ColorKeyRGB.GMax
				                     & ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				          ((hBlitDC->S1ColorKeyRGB.BMax
				                     & ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_S1_COLOR_KEY_MAX), (BlitRegValue & BLIT_COLOR_REG_MASK))) return 0;
		}		
	}
	//Send D Related Command
	
	//D Dram Para
	if ((BlitDCType & BLIT_DC_D_DRAM_PARA))
	{
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_D_SKIP_PIXEL), 
				(hBlitDC->DSkipPixelLine & ((1 << CMD_SKIP_PIXEL_WIDTH) -1)))) return 0;
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_D_LINE_PITCH), 
				(hBlitDC->DLinePitch & ((1 << CMD_PITCH_WIDTH) -1)))) return 0;
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_D_ADDR)      , 
				(hBlitDC->DBaseAddr))) return 0;
		
		if (!(BlitDCType & BLIT_DC_D_DRAM_PARA))
		{
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_PIXEL_NUM_PER_LINE), 
					(hBlitDC->DPixelNumOneLine & ((1 << CMD_LPIXEL_NUM_WIDTH) -1)))) return 0;

			if (!BlitRegCheck(hBlitDC, (BLIT_REG_TOTAL_LINE_NUM), 
					(hBlitDC->DTotalLineNum & ((1 << CMD_LINE_NUM_WIDTH) -1)))) return 0;
		}

	}
	//D Color Info should not be A0, no Default Color
	if ((BlitDCType & BLIT_DC_D_COLOR_FORMAT))
	{
		if (hBlitDC->S0ColorType != CSBLIT_COLOR_TYPE_ARGB1555)
		{
			BlitRegValue = ((hBlitDC->DColorType
					                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
		}
		else
		{
			BlitRegValue = ((hBlitDC->DColorType
					                     & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
					      ((hBlitDC->DAlpha0Min
					                     & ((1 << CMD_FORMAT_ALPHA0_MIN_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MIN_LSB)|
					      ((hBlitDC->DAlpha0Max
					                     & ((1 << CMD_FORMAT_ALPHA0_MAX_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MAX_LSB);
		}				
		if (!BlitRegCheck(hBlitDC, (BLIT_REG_D_COLOR_FORMAT), BlitRegValue)) return 0;
	}
	//Clut
	//Check it Separately
	//Scalor
	if (hBlitDC->S0ScalorEnable)
	{
		//Scalor Source Pixel Map
		//Send InitialPhase
		if ((BlitDCType & BLIT_DC_SCALOR_INIT_PHASE))
		{			
			BlitRegValue = (hBlitDC->HInitialPhase 
				 		         & ((1 << CMD_SCALOR_H_INIT_PHASE_WIDTH) -1));
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_SCALOR_HORIZONTAL_INITIAL_PHASE), (BlitRegValue))) return 0;
			
			BlitRegValue = ((hBlitDC->VInitialPhase    
				 		         & ((1 << CMD_SCALOR_V_INIT_PHASE_WIDTH) -1))) ;
			if (!BlitRegCheck(hBlitDC, (BLIT_REG_SCALOR_VERTICAL_INITIAL_PHASE), (BlitRegValue))) return 0;
		}	
	}
	return 1;
}

