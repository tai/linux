#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "blit_ioctl.h"

/* function declaration ------------------------------------- */

/* valid returns 1, invalid returns 0 */
static s32 IsRectValid(CSBLIT_Bitmap_t *Img, CSBLIT_Rectangle_t *Rect) //Boolean
{
	if (Img == NULL || Rect == NULL) 
	{
		return 0;
	}
	else if (Rect->PositionX   < 0 || Rect->PositionX   > Img->Width  ||
		     Rect->PositionY    < 0 || Rect->PositionY    > Img->Height ||
		     Rect->PositionX + Rect->Width  < 0 || Rect->PositionX + Rect->Width  > Img->Width  ||
		     Rect->PositionY + Rect->Height < 0 || Rect->PositionY + Rect->Height > Img->Height )
	{
		return 0;
	}
	else if (!(Rect->PositionX  <= Rect->PositionX + Rect->Width) || 
		     !(Rect->PositionY   <= Rect->PositionY + Rect->Height) )
	{
		return 0;
	}
	else
	{
		return 1;
	}

}

/* compare rect, Equal return 1; not Equal return 0 */
static s32 CmpRect( CSBLIT_Rectangle_t *Rect0, CSBLIT_Rectangle_t *Rect1) 
{
	if (!(Rect0->PositionX <= Rect0->PositionX + Rect0->Width) || 
		!(Rect0->PositionY <= Rect0->PositionY + Rect0->Height) )
	{
		return 0;
	}
	else if (!(Rect1->PositionX <= Rect1->PositionX + Rect1->Width) || 
		     !(Rect1->PositionY <= Rect1->PositionY + Rect1->Height) )
	{
		return 0;
	}
	else if ((Rect0->Width != Rect1->Width) || (Rect0->Height != Rect1->Height))	
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/* valid returns 1, invalid returns 0 */
static s32 IsColorTypeValid( CSBLIT_ColorType_t ColorType)
{
	switch (ColorType)
	{
		case CSBLIT_COLOR_TYPE_RGB565:
		case CSBLIT_COLOR_TYPE_ARGB4444:
		case CSBLIT_COLOR_TYPE_A0:
		case CSBLIT_COLOR_TYPE_CLUT8:
		case CSBLIT_COLOR_TYPE_CLUT4:
		case CSBLIT_COLOR_TYPE_ARGB1555:
		case CSBLIT_COLOR_TYPE_ARGB8888:
			return 1;
		default:
			return 0;
	}
}

static int CalcuDramPara
(
    	CSBLIT_Bitmap_t 		*DesImg, 
    	CSBLIT_Rectangle_t      	*DesRect,	
    	s32        			IsVReverse,
	u32       			*pBaseAddr,          //Bytes Addr
	u32       			*pLinePitch,         //Bytes Addr
	u32       			*pSkipPixelLine      //Valid Only When Clut4
)
{
    u32 PixelBitWidth = 0;
    u32 BaseAddr      = 0;                        //Bytes Addr
    u32 LinePitch     = 0;                       //Bytes Addr
    u32 SkipPixelLine = 0;              //Valid Only When Clut4

    switch (DesImg->ColorType)
    {
	case CSBLIT_COLOR_TYPE_ARGB8888:
		PixelBitWidth = 32;
		break;
	case CSBLIT_COLOR_TYPE_RGB565:			
	case CSBLIT_COLOR_TYPE_ARGB4444:
	case CSBLIT_COLOR_TYPE_ARGB1555:
	case CSBLIT_COLOR_TYPE_A0:
	    PixelBitWidth = 16;
	    break;
	case CSBLIT_COLOR_TYPE_CLUT8:
	    PixelBitWidth =  8;
	    break;
	case CSBLIT_COLOR_TYPE_CLUT4:
	    PixelBitWidth =  4;
	    break;
	default:
	    PixelBitWidth =  0;
	    break;
    }

    if (PixelBitWidth == 0)
	return -EINVAL;

    /*if Clut4 and Width is odd, the Line width should be Bytes Aligned*/
    //LinePitch = (DesImg->Width * PixelBitWidth + 8 - 1) / 8;
    LinePitch = DesImg->Pitch;
    if (!IsVReverse)
    {
	BaseAddr = (u32)DesImg->Data_p + 
	    DesRect->PositionY * LinePitch +                
	    (DesRect->PositionX * PixelBitWidth) / 8;
    }
    else
    {
	u32 LineIdx 	= 0;
	LineIdx  	= DesRect->PositionY + DesRect->Height;
	if (LineIdx > 0) 
	    LineIdx--;
	BaseAddr 	= (u32)DesImg->Data_p + LineIdx* LinePitch +   \
	    			(DesRect->PositionX * PixelBitWidth) / 8;
    }
    SkipPixelLine   = ((DesRect->PositionX * PixelBitWidth) % 8) / PixelBitWidth;
    *pBaseAddr      = BaseAddr;                        //Bytes Addr
    *pLinePitch     = LinePitch;                       //Bytes Addr
    *pSkipPixelLine = SkipPixelLine;              //Valid Only When Clut4

    return 0;
}

static u32 SCALOR_COEFF( u32 SignBit, u32 Numerator, u32 Denominator) 
{
	return (((SignBit) << SCALOR_COEFF_POLARITY_BIT)  | \
		   ((((Numerator)) << SCALOR_COEFF_FRACTION_BIT_WIDTH ) / (Denominator)));
}

static u32 *GetDefaultScalorHFIRCoeff(void)
{
	static u32 HFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_H_FIR_TAP_NUM];
	u32 Phase = 0;
	
	/*assert(SCALOR_H_FIR_TAP_NUM == 4);*/
	for (Phase = 0 ; Phase < SCALOR_PAHSE_NUM; Phase++)
	{
		HFIRCoeffTable[Phase][0] = 0;
		HFIRCoeffTable[Phase][1] = SCALOR_COEFF( 0, (SCALOR_PAHSE_NUM - Phase), SCALOR_PAHSE_NUM);
		HFIRCoeffTable[Phase][2] = SCALOR_COEFF( 0, (Phase), SCALOR_PAHSE_NUM);
		HFIRCoeffTable[Phase][3] = 0;
	}
	return HFIRCoeffTable[0];
}

static u32 *GetDefaultScalorVFIRCoeff(void)
{
	static u32 VFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_V_FIR_TAP_NUM];
	u32 Phase = 0;
	
	/*assert(SCALOR_V_FIR_TAP_NUM == 2);*/
	for (Phase = 0 ; Phase < SCALOR_PAHSE_NUM; Phase++)
	{
		VFIRCoeffTable[Phase][0] = SCALOR_COEFF( 0, (SCALOR_PAHSE_NUM - Phase), SCALOR_PAHSE_NUM);
		VFIRCoeffTable[Phase][1] = SCALOR_COEFF( 0, (Phase), SCALOR_PAHSE_NUM);
	}
	return VFIRCoeffTable[0];	
}

static int InitScalorHFIRCoeff(CSBlit_HW_Device_t *hBlitDC, u32 *HFIRCoeff)
{
	if (hBlitDC == NULL || HFIRCoeff == NULL) 
	{
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
		u32 *p = NULL;
		for (p = hBlitDC->HFIRCoeffTable[0];
		     p < hBlitDC->HFIRCoeffTable[0] + SCALOR_PAHSE_NUM * SCALOR_H_FIR_TAP_NUM;
			 p++)
		{
			*p = *HFIRCoeff++;
		}
		hBlitDC->UpdateHFIRCoeff = TRUE;
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_SCALOR_HFIR_COFF))
		{
			Err = -EINVAL;
		}
		else
		{	
			Err = 0;
		}
		hBlitDC->UpdateHFIRCoeff = FALSE;
		return Err;
	}
}

static int InitScalorVFIRCoeff(CSBlit_HW_Device_t *hBlitDC, u32 *VFIRCoeff)
{
	if (hBlitDC == NULL || VFIRCoeff == NULL) 
		return -EINVAL;
	else
	{
		int Err = -EINVAL;
		u32 *p = NULL;
		for (p = hBlitDC->VFIRCoeffTable[0];
		     p < hBlitDC->VFIRCoeffTable[0] + SCALOR_PAHSE_NUM * SCALOR_V_FIR_TAP_NUM;
			 p++)
		{
			*p = *VFIRCoeff++;
		}
		hBlitDC->UpdateVFIRCoeff = TRUE;
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_SCALOR_VFIR_COFF))
		{
			Err = -EINVAL;
		}
		else
		{	
			Err = 0;
		}
		hBlitDC->UpdateVFIRCoeff = FALSE;

		return Err;
	}	
}

static int SetDefaultScalorHFIRCoeff(CSBlit_HW_Device_t *hBlitDC)
{
	if (hBlitDC == NULL)
	{
		return -EINVAL;
	}
	else //Scalor Coeffecient Setting
	{
		int Err = -EINVAL;
		u32 *HFIRCoeff = NULL;
		HFIRCoeff = GetDefaultScalorHFIRCoeff();
		if (HFIRCoeff == NULL)
		{
			return -EINVAL;
		}
		Err   = InitScalorHFIRCoeff(hBlitDC, HFIRCoeff);
		
		return Err;
	}
}

static int SetDefaultScalorVFIRCoeff(CSBlit_HW_Device_t *hBlitDC)
{
	if (hBlitDC == NULL)
	{
		return -EINVAL;
	}
	else	//Scalor Coeffecient Setting
	{
		int Err = -EINVAL;
		u32 *VFIRCoeff = NULL;
		VFIRCoeff = GetDefaultScalorVFIRCoeff();
		if (VFIRCoeff == NULL)
		{
			return -EINVAL;
		}
		Err   = InitScalorVFIRCoeff(hBlitDC, VFIRCoeff);
		return Err;
	}
}

//Hardware initialize
int blit_initialize(CSBlit_HW_Device_t *hBlitDC)
{
    HardWareReset(hBlitDC);
    if(hBlitDC->BlitType == BLIT_TYPE_1200)
    {
    	SWReset(hBlitDC);
	SetDefaultScalorVFIRCoeff(hBlitDC);
	SetDefaultScalorHFIRCoeff(hBlitDC);
    }
    return 0;
}

int Fill
( 
    CSBlit_HW_Device_t 		*hBlitDC, 
    s32                 	BlitSrcId,
    CSBLIT_Bitmap_t       	*DesImg, 
    CSBLIT_Rectangle_t    	*DesRect, 
    CSBLIT_Color_t 		*FillColor,
    CSBLIT_AluMode_t 		AluMode
)
{
	DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
	if (hBlitDC  == NULL || BlitSrcId > 1 || DesImg  == NULL || DesRect == NULL || FillColor == NULL)
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsColorTypeValid(DesImg->ColorType))
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsRectValid(DesImg, DesRect))
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
	//S0 Control
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		if (BlitSrcId == 0)
		{
			hBlitDC->S0Enable       = TRUE;
			hBlitDC->S0VReverseScan = FALSE;
			hBlitDC->S0FetchDram    = FALSE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S0 Using Default Color then S0ColorType should be or A0
			hBlitDC->S0ColorType  = CSBLIT_COLOR_TYPE_A0;  //Must

			hBlitDC->S0DefaultColor.Type =  CSBLIT_COLOR_TYPE_ARGB8888;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha = FillColor->Value.ARGB8888.Alpha;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Value.ARGB8888.R = FillColor->Value.ARGB8888.R;       //ARGB8888 Format
			hBlitDC->S0DefaultColor.Value.ARGB8888.G = FillColor->Value.ARGB8888.G;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Value.ARGB8888.B = FillColor->Value.ARGB8888.B;      //ARGB8888 Format
			/*
			hBlitDC->S0BaseAddr;                      //Bytes Addr
			hBlitDC->S0LinePitch;                     //Bytes Addr
			hBlitDC->S0SkipPixelLine;            //Valid Only When Clut4
			hBlitDC->S0PixelNumOneLine;
			hBlitDC->S0TotalLineNum;	
			hBlitDC->S0ByteLittleNotBigEndian;
			hBlitDC->S0NibbleLittleNotBigEndian;
			*/
			hBlitDC->S0ClutEnable     = FALSE;
			hBlitDC->S0ColorKeyRGB.RGBEnable = FALSE;
			/*
			hBlitDC->S0ColorKeyMin;
			hBlitDC->S0ColorKeyMax;
			*/
		}
		else
		{
			hBlitDC->S0Enable       =  FALSE;
			hBlitDC->S0FetchDram    =  FALSE;
		}
	//S1 Control
		if (BlitSrcId == 1)
		{
			hBlitDC->S1Enable       = TRUE;
			hBlitDC->S1VReverseScan =  FALSE;
			hBlitDC->S1FetchDram    =  FALSE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S1 Using Default Color then S1ColorType should be or A0
			hBlitDC->S1ColorType  = CSBLIT_COLOR_TYPE_A0;  //Must

			hBlitDC->S1DefaultColor.Type =  CSBLIT_COLOR_TYPE_ARGB8888;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha = FillColor->Value.ARGB8888.Alpha;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Value.ARGB8888.R = FillColor->Value.ARGB8888.R;       //ARGB8888 Format
			hBlitDC->S1DefaultColor.Value.ARGB8888.G = FillColor->Value.ARGB8888.G;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Value.ARGB8888.B = FillColor->Value.ARGB8888.B;      //ARGB8888 Format
/*--------------------------------------------------
* printf("R:0x%x G:0x%x B: 0x%x A:0x%x\n",hBlitDC->S1DefaultColor.Value.ARGB8888.R,hBlitDC->S1DefaultColor.Value.ARGB8888.G,hBlitDC->S1DefaultColor.Value.ARGB8888.B,hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha);
*--------------------------------------------------*/
			/*
			hBlitDC->S1BaseAddr;                      //Bytes Addr
			hBlitDC->S1LinePitch;                     //Bytes Addr
			hBlitDC->S1SkipPixelLine;            //Valid Only When Clut4
			hBlitDC->S1PixelNumOneLine;
			hBlitDC->S1TotalLineNum;	
			hBlitDC->S1ByteLittleNotBigEndian;
			hBlitDC->S1NibbleLittleNotBigEndian;
			*/
			hBlitDC->S1ClutEnable     = FALSE;
			hBlitDC->S1ColorKeyRGB.RGBEnable = FALSE;
			/*
			hBlitDC->S1ColorKeyMin;
			hBlitDC->S1ColorKeyMax;
			*/
		}
		else
		{
			hBlitDC->S1Enable       = FALSE;
			hBlitDC->S1FetchDram    = FALSE; 			
		}

	    //ROP and Compositor
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			hBlitDC->ROPAlphaCtrl       = FALSE;
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			hBlitDC->ROPAlphaCtrl       = TRUE;
		}
		hBlitDC->CompositorEnable   = FALSE;
		hBlitDC->RopValue = AluMode;
		/*hBlitDC->S0OnTopS1;*/                        // 1 S0 On Top S1
		/*D Dram Para*/		
		if(CalcuDramPara (
		    DesImg, 
		    DesRect,	
		    hBlitDC->DVReverseScan = FALSE,
			&hBlitDC->DBaseAddr,     /*Bytes Addr*/
			&hBlitDC->DLinePitch,    /*Bytes Addr*/
			&hBlitDC->DSkipPixelLine /*Valid Only When Clut4*/) != 0)
		{
			DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
			return -EINVAL;
		}
		hBlitDC->DPixelNumOneLine    = DesRect->Width;
		hBlitDC->DTotalLineNum       = DesRect->Height;
		
		hBlitDC->DColorType        = DesImg->ColorType;
		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DesImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable  = FALSE;
		//hBlitDC->HInitialPhase   = 0;
		//hBlitDC->VInitialPhase   = 0;
		hBlitDC->UpdateHFIRCoeff = FALSE;
		hBlitDC->UpdateVFIRCoeff = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;

		//active DC
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
			Err = -EINVAL;
		}
		//Waitting for Idiel
		else if (!WaitBlitToBeIdle(hBlitDC))
		{
			Err = -EINVAL;
		}
		else
		{
			Err = 0;
		}

		DEBUG_PRINTF("ORION_BLIT : %s, %d , Err = %d\n",__FUNCTION__,__LINE__,Err);
		return Err;
	}
}

int Copy
( 
    CSBlit_HW_Device_t  	*hBlitDC, 
    s32                  	BlitSrcId,
    CSBLIT_Bitmap_t			*SrcImg, 
    CSBLIT_Rectangle_t		*SrcRect, 
    s32                  	SrcVReverse,
    CSBLIT_Bitmap_t			*DesImg, 
    CSBLIT_Rectangle_t		*DesRect, 
    CSBLIT_AluMode_t 		AluMode,
    s32                  	DesVReverse
)
{
	if (hBlitDC  == NULL || SrcImg  == NULL || SrcRect == NULL || DesImg  == NULL || DesRect == NULL)
	{
		return -EINVAL;
	}
	if(BlitSrcId != 0 && BlitSrcId != 1)
	{
		return -EINVAL;
	}
	else if (!IsColorTypeValid(SrcImg->ColorType) || //Scalor RTL S0 Support ARGB4444 and RGB565 Only
		     !IsColorTypeValid(DesImg->ColorType)  )
	{
		return -EINVAL;
	}
	else if (!IsRectValid(SrcImg, SrcRect))
	{
		return -EINVAL;
	}
	else if (!IsRectValid(DesImg, DesRect))
	{
		return -EINVAL;
	}
	else if (!CmpRect(SrcRect, DesRect))//Check SrcRect and DesRect
	{
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
	//S0 Control
		if (BlitSrcId == 0)
		{
			hBlitDC->S0Enable       = TRUE;
			hBlitDC->S0VReverseScan = SrcVReverse;
			hBlitDC->S0FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S0 Using Default Color then S0ColorType should be or A0
			hBlitDC->S0ColorType  = SrcImg->ColorType;  //Must
			hBlitDC->S0DefaultColor.Type =  CSBLIT_COLOR_TYPE_ARGB8888;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha = 0xFF;	/*FillColor->Value.ARGB8888.Alpha;*/     //ARGB8888 Format
			//hBlitDC->S0DefaultColor.Alpha = 0xFF;     //ARGB8888 Format, For Copy Don't change Source Alpha
			/*
			hBlitDC->S0DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S0DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    SrcImg, 
			    SrcRect,	
			    hBlitDC->S0VReverseScan,
				&hBlitDC->S0BaseAddr,              //Bytes Addr
				&hBlitDC->S0LinePitch,             //Bytes Addr
				&hBlitDC->S0SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S0PixelNumOneLine    = SrcRect->Width;
			hBlitDC->S0TotalLineNum       = SrcRect->Height;

			hBlitDC->S0NibbleLittleNotBigEndian = 0;	
			hBlitDC->S0ByteLittleNotBigEndian = 1;
			hBlitDC->S0TwoBytesLittleNotBigEndian = 1;
			if(SrcImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S0TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S0ClutEnable     = (SrcImg->ColorType == CSBLIT_COLOR_TYPE_CLUT4 ||
				                        SrcImg->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			hBlitDC->S0ColorKeyRGB.RGBEnable = FALSE;
			/*
			hBlitDC->S0ColorKeyMin;
			hBlitDC->S0ColorKeyMax;
			*/
		}
		else
		{
			hBlitDC->S0Enable       = FALSE;
			hBlitDC->S0FetchDram    = FALSE;
		}
	//S1 Control
		if (BlitSrcId == 1)
		{
			hBlitDC->S1Enable       = TRUE;
			hBlitDC->S1VReverseScan = SrcVReverse;
			hBlitDC->S1FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S1 Using Default Color then S1ColorType should be or A0
			hBlitDC->S1ColorType  = SrcImg->ColorType;  //Must
			hBlitDC->S1DefaultColor.Type =  CSBLIT_COLOR_TYPE_ARGB8888;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha = 0xFF;	/*FillColor->Value.ARGB8888.Alpha;*/     //ARGB8888 Format
			//hBlitDC->S1DefaultColor.Alpha = 0xFF;     //ARGB8888 Format, For Copy Don't change Source Alpha
			/*
			hBlitDC->S1DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S1DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    SrcImg, 
			    SrcRect,	
			    hBlitDC->S1VReverseScan,
				&hBlitDC->S1BaseAddr,              //Bytes Addr
				&hBlitDC->S1LinePitch,             //Bytes Addr
				&hBlitDC->S1SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S1PixelNumOneLine    = SrcRect->Width;
			hBlitDC->S1TotalLineNum       = SrcRect->Height;

			hBlitDC->S1NibbleLittleNotBigEndian = 0;	
			hBlitDC->S1ByteLittleNotBigEndian = 1;
			hBlitDC->S1TwoBytesLittleNotBigEndian = 1;
			if(SrcImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S1TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S1ClutEnable     = (SrcImg->ColorType == CSBLIT_COLOR_TYPE_CLUT4 ||
				                        SrcImg->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			hBlitDC->S1ColorKeyRGB.RGBEnable = FALSE;
			/*
			hBlitDC->S1ColorKeyMin;
			hBlitDC->S1ColorKeyMax;
			*/
		}
		else
		{
			hBlitDC->S1Enable       = FALSE;
			hBlitDC->S1FetchDram    = FALSE;
		}

		//ROP and Compositor
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			hBlitDC->ROPAlphaCtrl       = FALSE;
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			hBlitDC->ROPAlphaCtrl       = TRUE;
		}
		hBlitDC->CompositorEnable   = FALSE;
		hBlitDC->RopValue = AluMode;
		//hBlitDC->S0OnTopS1;                        // 1 S0 On Top S1
		//D Dram Para		
		CalcuDramPara
		(
		    DesImg, 
		    DesRect,	
		    hBlitDC->DVReverseScan = DesVReverse,
			&hBlitDC->DBaseAddr,              //Bytes Addr
			&hBlitDC->DLinePitch,             //Bytes Addr
			&hBlitDC->DSkipPixelLine         //Valid Only When Clut4
		);
		hBlitDC->DPixelNumOneLine    = DesRect->Width;
		hBlitDC->DTotalLineNum       = DesRect->Height;
		
		hBlitDC->DColorType        = DesImg->ColorType;

		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DesImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable   = FALSE;
		//hBlitDC->HInitialPhase   = 0;
		//hBlitDC->VInitialPhase   = 0;
		hBlitDC->UpdateHFIRCoeff  = FALSE;
		hBlitDC->UpdateVFIRCoeff  = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;
		
		//active DC
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
			Err = -EINVAL;
		}
		else if (!WaitBlitToBeIdle(hBlitDC))
		{
			Err = -EINVAL;
		}
		else
		{
			Err = 0;
		}
		return Err;
	}
}


int Scalor
( 
    CSBlit_HW_Device_t  	*hBlitDC, 
    CSBLIT_Source_t		*SrcPara,    
    CSBLIT_Destination_t 	*DesPara,
    u32                  	VInitialPhase, 
    u32                  	HInitialPhase,
    CSBLIT_AluMode_t 		AluMode
)
{
    CSBLIT_Bitmap_t     	*SrcImg = &SrcPara->Data.Bitmap;
    CSBLIT_Rectangle_t  	*SrcRect = &SrcPara->Rectangle; 
    CSBLIT_Bitmap_t		   	*DesImg = &DesPara->Bitmap; 
   	CSBLIT_Rectangle_t		*DesRect = &DesPara->Rectangle;

	if (hBlitDC == NULL || SrcPara == NULL || DesPara == NULL ||
			SrcImg == NULL || SrcRect == NULL || 
			DesImg == NULL || DesRect == NULL)
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsColorTypeValid(SrcImg->ColorType)) //Scalor RTL S0 Support ARGB4444 and RGB565 Only
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsRectValid(SrcImg, SrcRect))
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsRectValid(DesImg, DesRect))
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
		{	//S0 Control
			hBlitDC->S0Enable       = TRUE;
			hBlitDC->S0VReverseScan = SrcPara->VReverse;
			hBlitDC->S0FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S0 Using Default Color then S0ColorType should be ARGB565 or A0
			hBlitDC->S0ColorType  = SrcImg->ColorType;  //Must
			//hBlitDC->S0DefaultColor.Alpha = SrcImg->DefaultColor.Alpha;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Type = CSBLIT_COLOR_TYPE_ARGB8888;
			hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha = SrcPara->DefaultColor.Value.ARGB8888.Alpha;     //ARGB8888 Format
			/*
			hBlitDC->S0DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S0DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    SrcImg, 
			    SrcRect,	
			    hBlitDC->S0VReverseScan,
				&hBlitDC->S0BaseAddr,              //Bytes Addr
				&hBlitDC->S0LinePitch,             //Bytes Addr
				&hBlitDC->S0SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S0PixelNumOneLine    = SrcRect->Width;
			hBlitDC->S0TotalLineNum       = SrcRect->Height;

			hBlitDC->S0NibbleLittleNotBigEndian = 0;	
			hBlitDC->S0ByteLittleNotBigEndian = 1;
			hBlitDC->S0TwoBytesLittleNotBigEndian = 1;
			if(SrcImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S0TwoBytesLittleNotBigEndian = 0;
			}
			
			hBlitDC->S0ClutEnable     = FALSE; //Do not support Clut Type In Scalor
			hBlitDC->S0ColorKeyRGB.RGBEnable = FALSE; 
			/*
			hBlitDC->S0ColorKeyMin;
			hBlitDC->S0ColorKeyMax;
			*/
		}
		//S1 Control
		{
			hBlitDC->S1Enable       = FALSE;
			hBlitDC->S1FetchDram    = FALSE;
		}

	    //ROP and Compositor
		if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3)
		{
			hBlitDC->ROPAlphaCtrl       = FALSE;
		}
		else if(hBlitDC->BlitType == CSBLIT_DEVICE_TYPE_1_3_1)
		{
			hBlitDC->ROPAlphaCtrl       = TRUE;
		}
		hBlitDC->CompositorEnable   = FALSE;
		hBlitDC->RopValue = AluMode;
		//hBlitDC->S0OnTopS1;                        // 1 S0 On Top S1
		//D Dram Para		
		CalcuDramPara
		(
		    DesImg, 
		    DesRect,	
		    hBlitDC->DVReverseScan = DesPara->VReverse,
			&hBlitDC->DBaseAddr,              //Bytes Addr
			&hBlitDC->DLinePitch,             //Bytes Addr
			&hBlitDC->DSkipPixelLine          //Valid Only When Clut4
		);
		hBlitDC->DPixelNumOneLine    = DesRect->Width;
		hBlitDC->DTotalLineNum       = DesRect->Height;
		
		hBlitDC->DColorType        = DesImg->ColorType;

		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DesImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable   = TRUE;
		hBlitDC->HInitialPhase    = HInitialPhase;
		hBlitDC->VInitialPhase    = VInitialPhase;
		hBlitDC->UpdateHFIRCoeff  = FALSE;
		hBlitDC->UpdateVFIRCoeff  = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;
		
		//active DC
		/*--------------------------------------------------
		* if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		* {
		* 	Err = -EINVAL;
		* }
		* else if (!WaitBlitToBeIdle(hBlitDC))
		* {
		* 	Err = -EINVAL;
		* }
		* else
		* {
		* 	Err = 0;
		* }
		*--------------------------------------------------*/

		if (ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
		    if (!WaitBlitScalorToBeIdle(hBlitDC))
		    {
			/* Blit maybe reset, so setup FIR Coeffient repeatedly */
			SetDefaultScalorVFIRCoeff(hBlitDC);
			SetDefaultScalorHFIRCoeff(hBlitDC);
			Err = -EINVAL;
		    }
		    Err = 0;
		}
		else
		{
		    Err = -EINVAL;
		}
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return Err;	
	}	
}

int Composite
(
    CSBlit_HW_Device_t 		*hBlitDC, 
    CSBLIT_Source_t 		*Src0Para,
    CSBLIT_Source_t 		*Src1Para, 
    CSBLIT_Destination_t 	*DesPara,
    CSBLIT_AluMode_t 		AluMode,
    boolean 			BlendEnable, 
    boolean 			IsS0OnTopS1, 	
    u32 			ROPAlphaCtrl
)
{
	if (hBlitDC == NULL || Src0Para == NULL || Src1Para == NULL || DesPara == NULL)
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsColorTypeValid(Src0Para->Data.Bitmap.ColorType) || 
	         !IsColorTypeValid(Src1Para->Data.Bitmap.ColorType) || 
	         !IsColorTypeValid(DesPara->Bitmap.ColorType)  )
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsRectValid(&Src0Para->Data.Bitmap, &Src0Para->Rectangle) || 
		     !IsRectValid(&Src1Para->Data.Bitmap, &Src1Para->Rectangle) )
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!IsRectValid(&DesPara->Bitmap, &DesPara->Rectangle) )
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else if (!CmpRect(&Src0Para->Rectangle, &Src1Para->Rectangle) || 
		     !CmpRect(&Src0Para->Rectangle, &DesPara->Rectangle)  )
	{
		DEBUG_PRINTF("ORION_BLIT : %s, %d\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
		CSBLIT_Bitmap_t *S0Img = NULL, *S1Img = NULL, *DImg = NULL;
		S0Img = &Src0Para->Data.Bitmap;
		S1Img = &Src1Para->Data.Bitmap;
		DImg  = &DesPara->Bitmap;
	//S0 Control
		{
			hBlitDC->S0Enable       = TRUE;
			hBlitDC->S0VReverseScan = Src0Para->VReverse;
			hBlitDC->S0FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S0 Using Default Color then S0ColorType should be ARGB565 or A0
			hBlitDC->S0ColorType  = S0Img->ColorType;  //Must
			hBlitDC->S0DefaultColor.Type = CSBLIT_COLOR_TYPE_ARGB8888;
			hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha = Src0Para->DefaultColor.Value.ARGB8888.Alpha;     //ARGB8888 Format
			/*
			hBlitDC->S0DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S0DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    S0Img, 
			    &Src0Para->Rectangle,	
			    hBlitDC->S0VReverseScan,
				&hBlitDC->S0BaseAddr,              //Bytes Addr
				&hBlitDC->S0LinePitch,             //Bytes Addr
				&hBlitDC->S0SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S0PixelNumOneLine    = Src0Para->Rectangle.Width;
			hBlitDC->S0TotalLineNum       = Src0Para->Rectangle.Height;

			hBlitDC->S0NibbleLittleNotBigEndian = 0;	
			hBlitDC->S0ByteLittleNotBigEndian = 1;
			hBlitDC->S0TwoBytesLittleNotBigEndian = 1;
			if(S0Img->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S0TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S0ClutEnable     = (S0Img->ColorType == CSBLIT_COLOR_TYPE_CLUT4 || 
				                        S0Img->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			//hBlitDC->S0ColorKeyEnable = Src0Para->ColorKeyRGB.RGBEnable;
			//memcpy(&hBlitDC->S0ColorKeyMin, &Src0Para->MinColor, sizeof(CSBLIT_Corlor_t));
			//memcpy(&hBlitDC->S0ColorKeyMax, &Src0Para->MaxColor, sizeof(CSBLIT_Corlor_t));
			memcpy(&hBlitDC->S0ColorKeyRGB, &Src0Para->ColorKeyRGB, sizeof(CSBLIT_ColorKeyRGB_t));
		}
	//S1 Control
		{
			hBlitDC->S1Enable       = TRUE;
			hBlitDC->S1VReverseScan = Src1Para->VReverse;
			hBlitDC->S1FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S1 Using Default Color then S1ColorType should be ARGB565 or A0

			hBlitDC->S1ColorType  = S1Img->ColorType;  //Must
			hBlitDC->S1DefaultColor.Type = CSBLIT_COLOR_TYPE_ARGB8888;
			hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha = Src1Para->DefaultColor.Value.ARGB8888.Alpha;     //ARGB8888 Format
			/*
			hBlitDC->S1DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S1DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    S1Img, 
			    &Src1Para->Rectangle,	
			    hBlitDC->S1VReverseScan,
				&hBlitDC->S1BaseAddr,              //Bytes Addr
				&hBlitDC->S1LinePitch,             //Bytes Addr
				&hBlitDC->S1SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S1PixelNumOneLine    = Src1Para->Rectangle.Width;
			hBlitDC->S1TotalLineNum       = Src1Para->Rectangle.Height;

			hBlitDC->S1NibbleLittleNotBigEndian = 0;	
			hBlitDC->S1ByteLittleNotBigEndian = 1;
			hBlitDC->S1TwoBytesLittleNotBigEndian = 1;
			if(S1Img->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S1TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S1ClutEnable     = (S1Img->ColorType == CSBLIT_COLOR_TYPE_CLUT4 || 
				                        S1Img->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			//hBlitDC->S1ColorKeyEnable = Src1Para->ColorKeyEnable;
			//memcpy(&hBlitDC->S1ColorKeyMin, &Src1Para->MinColor, sizeof(CSBLIT_Corlor_t));
			//memcpy(&hBlitDC->S1ColorKeyMax, &Src1Para->MaxColor, sizeof(CSBLIT_Corlor_t));
			memcpy(&hBlitDC->S1ColorKeyRGB, &Src1Para->ColorKeyRGB, sizeof(CSBLIT_ColorKeyRGB_t));
		}
	    //ROP and Compositor
		hBlitDC->CompositorEnable   = BlendEnable;
		hBlitDC->ROPAlphaCtrl       = ROPAlphaCtrl;
		hBlitDC->RopValue           = AluMode;
		hBlitDC->S0OnTopS1          = IsS0OnTopS1;                        // 1 S0 On Top S1
		//D Dram Para		
		CalcuDramPara
		(
		    DImg, 
		    &DesPara->Rectangle,	
		    hBlitDC->DVReverseScan = DesPara->VReverse,
			&hBlitDC->DBaseAddr,              //Bytes Addr
			&hBlitDC->DLinePitch,             //Bytes Addr
			&hBlitDC->DSkipPixelLine          //Valid Only When Clut4
		);
		hBlitDC->DPixelNumOneLine    = DesPara->Rectangle.Width;
		hBlitDC->DTotalLineNum       = DesPara->Rectangle.Height;
		
		hBlitDC->DColorType        = DImg->ColorType;
		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable   = FALSE;
		//hBlitDC->HInitialPhase   = FALSE;
		//hBlitDC->VInitialPhase   = FALSE;
		hBlitDC->UpdateHFIRCoeff  = FALSE;
		hBlitDC->UpdateVFIRCoeff  = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;
		
		//active DC
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
			Err = -EINVAL;
		}
		else if (!WaitBlitToBeIdle(hBlitDC))
		{
			Err = -EINVAL;
		}
		else
		{
			Err = 0;
		}
		return Err;
	}
}

int CompositeSrc0
(
    CSBlit_HW_Device_t 		*hBlitDC, 
    CSBLIT_Source_t  		*Src0Para,
    CSBLIT_Source_t     	*Src1Para,
    CSBLIT_Destination_t  	*DesPara,
    CSBLIT_AluMode_t 		AluMode,
    boolean 			BlendEnable, 
    boolean 			IsS0OnTopS1, 
    u32 			ROPAlphaCtrl
)
{
	if (hBlitDC  == NULL || Src0Para  == NULL || DesPara == NULL)
	{
		return -EINVAL;
	}
	else if (!IsColorTypeValid(Src0Para->Data.Bitmap.ColorType) || 
		     !IsColorTypeValid(DesPara->Bitmap.ColorType)  )
	{
		return -EINVAL;
	}
	else if (!IsRectValid(&Src0Para->Data.Bitmap, &Src0Para->Rectangle))
	{
		return -EINVAL;
	}
	else if (!IsRectValid(&DesPara->Bitmap,  &DesPara->Rectangle) )
	{
		return -EINVAL;
	}
	else if (!CmpRect(&Src0Para->Rectangle, &DesPara->Rectangle)  )
	{
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
		CSBLIT_Bitmap_t *S0Img = NULL, *DImg = NULL;
		S0Img = &Src0Para->Data.Bitmap;
		DImg  = &DesPara->Bitmap;
	//S0 Control
		{
			hBlitDC->S0Enable       = TRUE;
			hBlitDC->S0VReverseScan = Src0Para->VReverse;
			hBlitDC->S0FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			                                     // if S0 Using Default Color then S0ColorType should be ARGB565 or A0
			hBlitDC->S0ColorType  = S0Img->ColorType;  //Must
			//hBlitDC->S0DefaultColor.Alpha = S0Img->DefaultColor.Alpha;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Type = CSBLIT_COLOR_TYPE_ARGB8888;
			hBlitDC->S0DefaultColor.Value.ARGB8888.Alpha = Src0Para->DefaultColor.Value.ARGB8888.Alpha;     //ARGB8888 Format
			/*
			hBlitDC->S0DefaultColor.Red   = SrcImg->DefaultColor->Red;       //ARGB8888 Format
			hBlitDC->S0DefaultColor.Green = SrcImg->DefaultColor->Green;     //ARGB8888 Format
			hBlitDC->S0DefaultColor.Blue  = SrcImg->DefaultColor->Blue;      //ARGB8888 Format
			*/
			CalcuDramPara
			(
			    S0Img, 
			    &Src0Para->Rectangle,	
			    hBlitDC->S0VReverseScan,
				&hBlitDC->S0BaseAddr,              //Bytes Addr
				&hBlitDC->S0LinePitch,             //Bytes Addr
				&hBlitDC->S0SkipPixelLine         //Valid Only When Clut4
			);
			hBlitDC->S0PixelNumOneLine    = Src0Para->Rectangle.Width;
			hBlitDC->S0TotalLineNum       = Src0Para->Rectangle.Height;
			hBlitDC->S0NibbleLittleNotBigEndian = 0;	
			hBlitDC->S0ByteLittleNotBigEndian = 1;
			hBlitDC->S0TwoBytesLittleNotBigEndian = 1;
			if(S0Img->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S0TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S0ClutEnable     = (S0Img->ColorType == CSBLIT_COLOR_TYPE_CLUT4 || 
				                        S0Img->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			memcpy(&hBlitDC->S0ColorKeyRGB, &Src0Para->ColorKeyRGB, sizeof(CSBLIT_ColorKeyRGB_t));
		}
	//S1 Control
		{
			hBlitDC->S1Enable       =  BlendEnable;	/*if want to blend, then must enable S1*/
			hBlitDC->S1FetchDram    = FALSE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			hBlitDC->S1ColorType  = CSBLIT_COLOR_TYPE_A0;                                     // if S1 Using Default Color then S1ColorType should be ARGB565 or A0
			memcpy(&hBlitDC->S0DefaultColor, &Src1Para->Data.Color, sizeof(CSBLIT_Color_t));
			hBlitDC->S1ClutEnable     = FALSE;
			hBlitDC->S1ColorKeyRGB.RGBEnable = FALSE;
		}
	    //ROP and Compositor
		hBlitDC->CompositorEnable   = BlendEnable;
		hBlitDC->ROPAlphaCtrl       = ROPAlphaCtrl;
		hBlitDC->RopValue           = AluMode;		
		hBlitDC->S0OnTopS1          = IsS0OnTopS1;                        // 1 S0 On Top S1
		//D Dram Para		
		CalcuDramPara
		(
		    DImg, 
		    &DesPara->Rectangle,	
		    hBlitDC->DVReverseScan = DesPara->VReverse,
			&hBlitDC->DBaseAddr,              //Bytes Addr
			&hBlitDC->DLinePitch,             //Bytes Addr
			&hBlitDC->DSkipPixelLine         //Valid Only When Clut4
		);
		hBlitDC->DPixelNumOneLine    = DesPara->Rectangle.Width;
		hBlitDC->DTotalLineNum       = DesPara->Rectangle.Height;
		
		hBlitDC->DColorType        = DImg->ColorType;
		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable   = FALSE;
		//hBlitDC->HInitialPhase   = FALSE;
		//hBlitDC->VInitialPhase   = FALSE;
		hBlitDC->UpdateHFIRCoeff  = FALSE;
		hBlitDC->UpdateVFIRCoeff  = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;
		
		//active DC
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
			Err = -EINVAL;
		}
		else if (!WaitBlitToBeIdle(hBlitDC))
		{
			Err = -EINVAL;
		}
		else
		{
			Err = 0;
		}
		return Err;
	}
}

int CompositeSrc1
(
    CSBlit_HW_Device_t 		*hBlitDC, 
    CSBLIT_Source_t     	*Src0Para, 
    CSBLIT_Source_t     	*Src1Para,
    CSBLIT_Destination_t  	*DesPara,
    CSBLIT_AluMode_t 		AluMode,
    boolean 			BlendEnable, 
    boolean 			IsS0OnTopS1, 
    u32 			ROPAlphaCtrl
)
{
	if (hBlitDC  == NULL || Src1Para  == NULL || DesPara == NULL)
	{
		return -EINVAL;
	}
	else if (!IsColorTypeValid(Src1Para->Data.Bitmap.ColorType) || 
		     !IsColorTypeValid(DesPara->Bitmap.ColorType)  )
	{
		return -EINVAL;
	}
	else if (!IsRectValid(&Src1Para->Data.Bitmap, &Src1Para->Rectangle))
	{
		return -EINVAL;
	}
	else if (!IsRectValid(&DesPara->Bitmap,  &DesPara->Rectangle) )
	{
		return -EINVAL;
	}
	else if (!CmpRect(&Src1Para->Rectangle, &DesPara->Rectangle)  )
	{
		return -EINVAL;
	}
	else
	{
		int Err = -EINVAL;
		CSBLIT_Bitmap_t *S1Img = NULL, *DImg = NULL;
		S1Img = &Src1Para->Data.Bitmap;
		DImg  = &DesPara->Bitmap;
		//S0 Control
		{
			hBlitDC->S0Enable       = BlendEnable;	/*if Want to blend,then must enable s0*/
			hBlitDC->S0FetchDram    = FALSE;        // TRUE Fetch, FALSE do not Fetch Using Default Color in ARGB
			// if S0 Using Default Color then S0ColorType should be ARGB565 or A0
			hBlitDC->S0ColorType  = CSBLIT_COLOR_TYPE_A0;
			memcpy(&hBlitDC->S0DefaultColor, &Src0Para->Data.Color, sizeof(CSBLIT_Color_t));
			hBlitDC->S0ClutEnable     = FALSE;
			hBlitDC->S0ColorKeyRGB.RGBEnable = FALSE;
		}

		//S1 Control
		{
			hBlitDC->S1Enable       = TRUE;
			hBlitDC->S1VReverseScan = Src1Para->VReverse;
			hBlitDC->S1FetchDram    = TRUE;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB
			// if S1 Using Default Color then S1ColorType should be ARGB565 or A0
			hBlitDC->S1ColorType  = S1Img->ColorType;  //Must
			//hBlitDC->S1DefaultColor.Alpha = S1Img->DefaultColor.Alpha;     //ARGB8888 Format
			hBlitDC->S1DefaultColor.Type = CSBLIT_COLOR_TYPE_ARGB8888;
			hBlitDC->S1DefaultColor.Value.ARGB8888.Alpha = Src1Para->DefaultColor.Value.ARGB8888.Alpha;     //ARGB8888 Format
			/*
			   hBlitDC->S1DefaultColor.Red   = SrcImg->DefaultColor->Red;      //ARGB8888 Format
			   hBlitDC->S1DefaultColor.Green = SrcImg->DefaultColor->Green;    //ARGB8888 Format
			   hBlitDC->S1DefaultColor.Blue  = SrcImg->DefaultColor->Blue;     //ARGB8888 Format
			   */
			CalcuDramPara
				(
				 S1Img, 
				 &Src1Para->Rectangle,	
				 hBlitDC->S1VReverseScan,
				 &hBlitDC->S1BaseAddr,              //Bytes Addr
				 &hBlitDC->S1LinePitch,             //Bytes Addr
				 &hBlitDC->S1SkipPixelLine         //Valid Only When Clut4
				);
			hBlitDC->S1PixelNumOneLine    = Src1Para->Rectangle.Width;
			hBlitDC->S1TotalLineNum       = Src1Para->Rectangle.Height;

			hBlitDC->S1NibbleLittleNotBigEndian = 0;	
			hBlitDC->S1ByteLittleNotBigEndian = 1;
			hBlitDC->S1TwoBytesLittleNotBigEndian = 1;
			if(S1Img->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
				hBlitDC->S1TwoBytesLittleNotBigEndian = 0;
			}

			hBlitDC->S1ClutEnable     = (S1Img->ColorType == CSBLIT_COLOR_TYPE_CLUT4 || 
					S1Img->ColorType == CSBLIT_COLOR_TYPE_CLUT8 );
			//hBlitDC->S1ColorKeyEnable = Src1Para->ColorKeyEnable;
			//memcpy(&hBlitDC->S1ColorKeyMin, &Src1Para->MinColor, sizeof(CSBLIT_Corlor_t));
			//memcpy(&hBlitDC->S1ColorKeyMax, &Src1Para->MaxColor, sizeof(CSBLIT_Corlor_t));
			memcpy(&hBlitDC->S1ColorKeyRGB, &Src1Para->ColorKeyRGB, sizeof(CSBLIT_ColorKeyRGB_t));
		}
	    //ROP and Compositor
		hBlitDC->CompositorEnable   = BlendEnable;
		hBlitDC->ROPAlphaCtrl       = ROPAlphaCtrl;
		hBlitDC->RopValue           = AluMode;		
		hBlitDC->S0OnTopS1          = IsS0OnTopS1;                        // 1 S0 On Top S1
		//D Dram Para		
		CalcuDramPara
		(
		    DImg, 
		    &DesPara->Rectangle,	
		    hBlitDC->DVReverseScan = DesPara->VReverse,
			&hBlitDC->DBaseAddr,              //Bytes Addr
			&hBlitDC->DLinePitch,             //Bytes Addr
			&hBlitDC->DSkipPixelLine         //Valid Only When Clut4
		);
		hBlitDC->DPixelNumOneLine    = DesPara->Rectangle.Width;
		hBlitDC->DTotalLineNum       = DesPara->Rectangle.Height;
		
		hBlitDC->DColorType        = DImg->ColorType;
		hBlitDC->DNibbleLittleNotBigEndian = 0;	
		hBlitDC->DByteLittleNotBigEndian = 1;
		hBlitDC->DTwoBytesLittleNotBigEndian = 1;
		if(DImg->ColorType == CSBLIT_COLOR_TYPE_ARGB8888){
			hBlitDC->DTwoBytesLittleNotBigEndian = 0;
		}

		//Scalor Control
		hBlitDC->S0ScalorEnable   = FALSE;
		//hBlitDC->HInitialPhase   = FALSE;
		//hBlitDC->VInitialPhase   = FALSE;
		hBlitDC->UpdateHFIRCoeff  = FALSE;
		hBlitDC->UpdateVFIRCoeff  = FALSE;

		//Clut para
		hBlitDC->UpdateClut4Table = FALSE;
		hBlitDC->UpdateClut8Table = FALSE;
		//RunTimeControl
		hBlitDC->InterruptEnable  = BLIT_INTERRUPT_MODE;
		
		//active DC
		if (!ActiveDeviceContext(hBlitDC, BLIT_DC_ALL))
		{
			Err = -EINVAL;
		}
		else if (!WaitBlitToBeIdle(hBlitDC))
		{
			Err = -EINVAL;
		}
		else
		{
			Err = 0;
		}
		return Err;
	}
}

/*end of blit_ioctl.c*/

