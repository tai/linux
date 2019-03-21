#ifndef _DF_REG_DEF_H_
#define _DF_REG_DEF_H_

/****************Register IF******************************/

#define TVE0_REG_BASE		0x10168000
#define TVE0_REG_SIZE		0x00001000
#define TVE1_REG_BASE		0x10160000
#define TVE1_REG_SIZE		0x00001000

enum  _DF_REGISTER_BANK_
{
	DISP_REG_BASE             = 0x41800000,	
	DISP_REG_COMMON_BASE      = DISP_REG_BASE + (0x00 << 2),//0x0
	DISP_REG_GFX1_BASE        = DISP_REG_BASE + (0x10 << 2),//0x40
	DISP_REG_GFX2_BASE        = DISP_REG_BASE + (0x20 << 2),//0x80
	DISP_REG_VIDEO1_BASE      = DISP_REG_BASE + (0x30 << 2),//0xc0
	DISP_REG_VIDEO2_BASE      = DISP_REG_BASE + (0x50 << 2),//0x140
    	DISP_REG_VIDEO3_BASE      = DISP_REG_BASE + (0x70 << 2),
	DISP_REG_COMP1_BASE       = DISP_REG_BASE + (0x90 << 2),
	DISP_REG_COMP2_BASE       = DISP_REG_BASE + (0xA0 << 2),
	DISP_REG_HD2SD_BASE       = DISP_REG_BASE + (0xB0 << 2),
	DISP_REG_OUTIF1_BASE      = DISP_REG_BASE + (0xC0 << 2),
	DISP_REG_OUTIF2_BASE      = DISP_REG_BASE + (0xD0 << 2),
	
	/***************COMMON Register******************/
	DISP_UPDATE_REG           = ( DISP_REG_COMMON_BASE ) + ( 0 << 2), //0x41800000
	/*
	// Bit[   0] : Write 1 Before Update Any Register
	//             Write 0 After Finish Update Register
	*/
	DISP_STATUS               = ( DISP_REG_COMMON_BASE ) + ( 1 << 2), //0x41800004
	/* Readonly Status Register, donot need doubled
	// Bit[    0] : OutputIF1 Error, Reset Value 0;
	// Bit[7 : 1] : OutputIF1 Error Type, Reset Value 0;
	// Bit[    8] : OutputIF2 Error, Reset Value 0;
	// Bit[15: 9] : OutputIF2 Error Type, Reset Value 0;	
	// Bit[   16] : OutputIF1 Frame Sync Interrupt
	// Bit[   17] : OutputIF2 Frame Sync Interrupt
	// Bit[31:28] : HD2SD Error Type
	//              Bit[28]: Y Store FIFO Cleared when it is not empty
	//              Bit[29]: Y Store FIFO Overflow
	//              Bit[30]: C Store FIFO Cleared When it is not empty
	//              Bit[31]: C Store FIFO Overflow
	//       *Note: 
	*/	
	DISP_INT_CLEAR            = ( DISP_REG_COMMON_BASE ) + ( 2 << 2),//0x41800008
	/* ARM and RISC Interrupt Clear
    // Bit[    0] : Display OutIF #0 Interrupt For RISC Processor
	// Bit[    1] : Display OutIF #1 Interrupt For RISC Processor
	// Bit[    2] : Display OutIF #0 Interrupt For ARM Processor
	// Bit[    3] : Display OutIF #1 Interrupt For ARM Processor
	// Bit[31: 4] : Reserved,Read as Zero
	*/
	DISP_OUTIF1_INT_CLEAR     = ( DISP_REG_COMMON_BASE ) + ( 3 << 2),//0x4180000C
	DISP_OUTIF2_INT_CLEAR     = ( DISP_REG_COMMON_BASE ) + ( 4 << 2),//0x41800010
	/*
 	// Write Any Value to Clear the Output IF 1/2 Generated  
 	// Interrupt
 	*/
	DISP_OUTIF1_ERR_CLEAR     = ( DISP_REG_COMMON_BASE ) + ( 5 << 2),//0x41800014
	DISP_OUTIF2_ERR_CLEAR     = ( DISP_REG_COMMON_BASE ) + ( 6 << 2),//0x41800018
 	/*
 	// Write Any Value to Clear the Compositor1/2 to OutputIF1/2 
 	// FIFO Read Empty Error
 	*/
 	DISP_HD2SD_ERR_CLEAR     = ( DISP_REG_COMMON_BASE ) + ( 7 << 2),//0x4180001C

	DISP_SCA_COEF_IDX         = ( DISP_REG_COMMON_BASE ) + ( 8 << 2),//0x41800020
	/* Write this before update Any Coeff Values
	// Bit[ 1: 0] : Coeff Tap Idx, 0..3, Reset Value 0;
	// Bit[ 7: 2] : Reserved to be zero,
	// Bit[11: 8] : Coeff Phase Idx, 0..15, Reset Value 0;
	// Bit[15:12] : Reserved to be zero,
	// Bit[18:16] : Coeff Type: Reset Value 0;
	//              0 : Video 1 HFIR Coeff
	//              1 : Video 1 VFIR Coeff
	//              2 : Video 2 HFIR Coeff
	//              3 : Video 2 VFIR Coeff
	//              4 : HD2SD 1 HFIR Coeff
	//              5 : HD2SD 1 VFIR Coeff
	//           *Note: Other Value will not take any Effect
	*/
	DISP_SCA_COEF_DATA        = ( DISP_REG_COMMON_BASE ) + ( 9 << 2),//0x41800024
	/* Write or Read it to Update or Retrieve a Coeff Data
	// Bit[11: 0] : Coeff Data, Reset Value 0;
	// Bit[14:12] : Reserved to be zero;
	// Bit[   15] : Signed Bit, Reset Value 0;
  	//             1: Negtive Coeff, 0: Positive Coeff
  	// ***Important Note****
  	//            1. the Video Coeff can be Update at any time, but will be take effect in next frame.
  	//            2. the HD2SD Coeff can be Update at any time, but will be take effect in next frame.
	*/
	
	/***************GFX1 Register******************/
	DISP_GFX1_CTRL            = ( DISP_REG_GFX1_BASE ) + ( 0 << 2),//0x41800040
	/*
	// Bit[1:0]  : GFX_EN,    0/3: Disable, 1: Gfx on Output1, 2: Gfx on Output2, Reset Value: 0;
	// Bit[2]    : SCALER_EN,  0: Disable, 1: Enable, Reset Value 0;
	// ***Important Note***
	//             the Gfx Horizontal Scaler Cannot do any Scaler down Operation.
	// Bit[3]    : COLOR_KEYING_EN,  0: Disable, 1: Enable, Reset Value 0;
	// Bit[5:4]  : FETCH_TYPE, Decide the Fetch Type when Interlaced Display, no effect when Progressive display,
	//                       0/3: Auto, 1; Fetch Top Field Only, 2: Fetch Bot Field only, Reset Value: 0;
	// Bit[6]    : CONV_EN, 1: Enable the RGB to YUV or YUV to RGB Conversion,this convertion depend on the FORMAT Register 
	//                       0: Disable
	//                       Reset value: 1;
	// Bit[7]    : Reseverd to be Zero,
	// Bit[31:12]: Reseverd to be Zero,
 	*/
 	DISP_GFX1_FORMAT          = ( DISP_REG_GFX1_BASE ) + ( 1 << 2),//0x41800044
 	/*
 	// Bit[ 2: 0] : FORMAT, 
	//               CLU4      3'b000
	//               CLU8      3'b001
	//               RGB565    3'b010
	//`              ARGB4444  3'b011
	//               A0        3'b100
	//               ARGB1555  3'b101
	//               ARGB8888  3'b110
	//               YUY2      3'b111
 	// Bit[ 7: 3] : Reseverd to be Zero,
	// Bit[10: 8] : GFX1_BYTE_ENDIAN, 
	//              Bit[8] : Byte_endian, 32bit Bytes Endian,  Reset Value: 0
	//              Bit[9] : word_endian, 128bit 32bit Endian, Reset Value: 1
	//              Bit[10]: 16bit Bytes Endian,  Reset Value: 1
	//              *Note  : For All Endian Bit, if 0 change bit Order, 1 unchanged
	// Bit[   11] : GFX1_NIBBLE_ENDIAN, Reset Value 0
 	*/
 	DISP_GFX1_ALPHA_CTRL       = ( DISP_REG_GFX1_BASE ) + ( 2 << 2),//0x41800048
 	/*
 	// Bit[ 7: 0] : DEFAULT_ALPHA, Reset Value: 0,
 	// Bit[15: 8] : KEY_ALPHA, Reset Value:0
 	// Bit[23:16] : ARGB1555_ALPHA0,  Reset Value: 0,
 	// Bit[31:24] : ARGB1555_ALPHA1,  Reset Value: 255,
 	*/
 	DISP_GFX1_KEY_RED         = ( DISP_REG_GFX1_BASE ) + ( 3 << 2), //0x4180004C
 	/*
 	// Bit[ 7: 0] : RED_MIN, Reset Value: 0,
 	// Bit[15: 8] : RED_MAX, Reset Value: 255,
 	*/
 	DISP_GFX1_KEY_BLUE        = ( DISP_REG_GFX1_BASE ) + ( 4 << 2),//0x41800050 
 	/*
 	// Bit[ 7: 0] : BLUE_MIN, Reset Value: 0,
 	// Bit[15: 8] : BLUE_MAX, Reset Value: 255,
 	*/
 	DISP_GFX1_KEY_GREEN       = ( DISP_REG_GFX1_BASE ) + ( 5 << 2), //0x41800054
 	/*
 	// Bit[ 7: 0] : GREEN_MIN, Reset Value: 0,
 	// Bit[15: 8] : GREEN_MAX, Reset Value: 255,
 	*/
 	DISP_GFX1_BUF_START       = ( DISP_REG_GFX1_BASE ) + ( 6 << 2), //0x41800058
 	/*
 	// Bit[23: 0] : Gfx FrameBuf Start Address, 
 	//              128Bit Word Address(Bytes Address >> 4), Reset Value: 0
 	*/
 	DISP_GFX1_LINE_PITCH      = ( DISP_REG_GFX1_BASE ) + ( 7 << 2), //0x4180005C
	/*
 	// Bit[19: 0] : Gfx FrameBuf LinePitch, 
 	//              128Bit Word Address(Bytes Address >> 4), Reset Value: 0
 	// Bit[23:20] : Reserved to be zero
 	// Bit[28:24] : blank_pixel:  Gfx Layer Blank Pixel Num in each Line Head, 
 	//              if Scaler on Gfx, it's the Destination Line Blank numbers
 	//              Reset Value: 0;
 	// Bit[31   ] : DRAM_MAPPING: Never Used in Current Version, 
 	//              Please set it to Zero, Reset Value: 1.
 	*/
 	DISP_GFX1_X_POSITON       = ( DISP_REG_GFX1_BASE ) + ( 8 << 2), //0x41800060
 	/*
 	// Bit[10: 0] : X_START, Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : X_END, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) X_END = X_START + SrcWidth - 1, 
 	//                   SrcWidth multiply with Pixel Bit Width should be a multiply of 128bits
 	//               (2) If Gfx Horizontal Scaler is not Enabled,
 	//                   X_START and X_END indict the Gfx Layer Display X Location in Screen.
 	//               (3) If Gfx Horizontal Scaler is Eanbled, these two decide the SrcWidth only
 	*/
 	DISP_GFX1_Y_POSITON       = ( DISP_REG_GFX1_BASE ) + ( 9 << 2), //0x41800064
 	/*
 	// Bit[10: 0] : Y_START, Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : Y_END, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) If Progressive Displayed, 
 	//                   Y_START = Gfx Layer Screen Y OffSet,
 	//                   Y_END   = Y_START + GfxSrcHeight;
 	//               (2) If Interlaced Displayed, 
 	//                   Y_START = Gfx Layer Screen Y OffSet / 2
 	//                   Y_END   = (Y_START + GfxSrcHeight)  / 2; 
 	*/
 	DISP_GFX1_SCL_X_POSITON   = ( DISP_REG_GFX1_BASE ) + (10 << 2), //0x41800068
 	/*
 	// Bit[10: 0] : SCL_X_START, Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : SCL_X_END, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) SCL_X_END = SCL_X_START + DispWidth - 1, 
 	//                   SrcWidth multiply with Pixel Bit Width should be a multiply of 128bits
 	//               (2) If Gfx Horizontal Scaler is not Enabled, 
 	//                   SCL_X_START and SCL_XEND is unused.
 	//               (3) If Gfx Horizontal Scaler is Eanbled, 
 	//                   SCL_X_START and SCL_XEND indict the Gfx Layer Display X Location in Screen.
 	*/
 	DISP_GFX1_CLUT_ADDR       = ( DISP_REG_GFX1_BASE ) + (11 << 2),//0x4180006C
 	/* Clut Table Item Idx, Write it to Update or Read the correspond Item Data
 	// Bit[ 7: 0] : Clut_Idx, For Clut4, Valid Value is 0 to 15, 
 	//              for Clut8, Valid Value is 0 to 255
 	*/
 	DISP_GFX1_CLUT_DATA       = ( DISP_REG_GFX1_BASE ) + (12 << 2), //0x41800070
 	/* Clut Talbe Item Data, Write it or Read it after Set the CLUT_ADDR
 	// Bit[7 : 0] : Blue
 	// Bit[15: 8] : Green
 	// Bit[23:16] : Red
 	// Bit[31:24] : Alpha
 	*/
 	/***************GFX2 Register******************/
 	/*
 	// *Note: All the Bits of GFX2 Registers are same with GFX1 Registers
 	*/
	DISP_GFX2_CTRL            = ( DISP_REG_GFX2_BASE ) + ( 0 << 2),//0x41800080
	DISP_GFX2_FORMAT          = ( DISP_REG_GFX2_BASE ) + ( 1 << 2),//0x41800084
	DISP_GFX2_ALPHA_CTRL      = ( DISP_REG_GFX2_BASE ) + ( 2 << 2),//0x41800088
	DISP_GFX2_KEY_RED         = ( DISP_REG_GFX2_BASE ) + ( 3 << 2),//0x4180008C 
	DISP_GFX2_KEY_BLUE        = ( DISP_REG_GFX2_BASE ) + ( 4 << 2),//0x41800090
	DISP_GFX2_KEY_GREEN       = ( DISP_REG_GFX2_BASE ) + ( 5 << 2),//0x41800094 
	DISP_GFX2_BUF_START       = ( DISP_REG_GFX2_BASE ) + ( 6 << 2),//0x41800098 
	DISP_GFX2_LINE_PITCH      = ( DISP_REG_GFX2_BASE ) + ( 7 << 2),//0x4180009C 
	DISP_GFX2_X_POSITON       = ( DISP_REG_GFX2_BASE ) + ( 8 << 2),//0x418000A0 
	DISP_GFX2_Y_POSITON       = ( DISP_REG_GFX2_BASE ) + ( 9 << 2),//0x418000A4 
	DISP_GFX2_SCL_X_POSITON   = ( DISP_REG_GFX2_BASE ) + (10 << 2),//0x418000A8 
	DISP_GFX2_CLUT_ADDR       = ( DISP_REG_GFX2_BASE ) + (11 << 2),//0x418000AC
	DISP_GFX2_CLUT_DATA       = ( DISP_REG_GFX2_BASE ) + (12 << 2),//0x418000B0
	
	/***************VIDEO1 Register******************/
	DISP_VIDEO1_CTRL          = ( DISP_REG_VIDEO1_BASE ) + ( 0 << 2),//0x418000C0
	/*
	// Bit[ 1: 0]: VIDEO_EN,    0/3: Disable, 
	//                            1: Video on Output1, 
	//                            2: Video on Output2, Reset Value: 0;
	// Bit[2]    : LUMA_KEY_EN    0: Disable Luma Keying, 
	//                            1: Enable Luma Keying, Reset Value: 0;
	// Bit[3]    : Auto Correct Top Bottom Field Enable: 
	//                            0: Disable Hardware Top Bottom Field Correction, 
	//                            1: Enable Hardware Top Bottom Field Correction, Reset Value 1:
	// Bit[4]    : LUMA_SCAL_VFIR_TAP_NUM_SEL: 
	//                            1: Vertical 2Tap,
	//                            0: Vertical 4Tap, Reset Value 0:
	//      *Note: the Video Source Width is indicat by the SrcCropWidth, 
	//             if SrcCropWidth > 960, the Vertical Scaler will be set to be 0 auto,
	//             Because the Video LineBuf Width is 1024 * 4, 
	//             which can only contain 2 HD Lines and 4 SD Lines.
	// Bit[31: 6]: Reseverd to be Zero,
 	*/	 
	DISP_VIDEO1_ALPHA_CTRL    = ( DISP_REG_VIDEO1_BASE ) + ( 1 << 2),//0x418000C4
 	/*
 	// Bit[ 7: 0] : DEFAULT_ALPHA, Reset Value: 0,
 	// Bit[15: 8] : Reseverd to be Zero,
 	// Bit[23:16] : LUMA_KEY_ALPHA0,  Reset Value: 0,
 	// Bit[31:24] : LUMA_KEY_ALPHA1,  Reset Value: 255,
 	//      *Note : (1) If the Video Layer Luma Keying is Disabled,
 	//                  each pixel's alpha value of video layer is DEFAULT_ALPHA
 	//              (2) Luma Keying is Enabled, each pixel's alpha value is 
 	//                  decided by the keying result.
 	//                  The Luma Keying Condition is: 
 	//                  IsKeying = LUMA_KEY_MIN <= InputLuma <= LUMA_KEY_MAX
 	//                  if (IsKeying) the Pixel's Alpha is LUMA_KEY_ALPHA0,
 	//                  else the Pixel's Alpha is LUMA_KEY_ALPHA1
 	*/
 	DISP_VIDEO1_KEY_LUMA      = ( DISP_REG_VIDEO1_BASE ) + ( 2 << 2),//0x418000C8 
 	/*
 	// Bit[ 7: 0] : LUMA_KEY_MIN, Reset Value: 0,
 	// Bit[15: 8] : LUMA_KEY_MAX, Reset Value: 255,
 	*/
 	DISP_VIDEO1_X_POSITON     = ( DISP_REG_VIDEO1_BASE ) + ( 3 << 2),//0x418000CC 
 	/*
 	// Bit[10: 0] : X_START, Reset Value 0
 	// Bit[14:11] : X_START_CROP_PIXEL_NUM, Reset Value 0
 	// Bit[15   ] : Reserved to be zero
 	// Bit[26:16] : X_END, Reset Value 0
 	// Bit[30:27] : X_END_CROP_PIXEL_NUM, Reset Value 0
 	// Bit[31   ] : ENA_X_START_END_CROP, Reset Value 0
 	//      *Note :  (1) X_START and X_END indict the Video Layer Horizontal Location is Screen
 	//               (2) The Video Display Width = X_END - X_START
 	//               (3) X_START and X_END Can be one Pixel Aligned!
 	//      *Note : About the X_START_END CROP Pixel Number,
 	//               (1) if ENA_X_START_END_CROP is Enabled, the final display Width is 
 	//      X_END - X_START + X_START_CROP_PIXEL_NUM + X_END_CROP_PIXEL_NUM;
 	//               (2) but First TX_START_CROP_PIXEL_NUM Pixels and 
 	//      Last X_END_CROP_PIXEL_NUM pixels will be discard to displayed in the Screen.
 	//               (3) Make Sure the Total Width should not exceed 1920.
 	*/
 	DISP_VIDEO1_Y_POSITON     = ( DISP_REG_VIDEO1_BASE ) + ( 4 << 2),//0x418000D0 
 	/*
 	// Bit[10: 0] : Y_START, Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : Y_END, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) Y_START and Y_END indict the Video Layer Vertical Location is Screen
 	//               (2) The Video Display Height = Y_END - Y_START
 	//               (3) Y_START and X_END Must be a Multiply of 4!    
 	*/
 	DISP_VIDEO1_SRC_X_CROP    = ( DISP_REG_VIDEO1_BASE ) + ( 5 << 2),//0x418000D4     
 	/* Define the Video Soruce Cropping Windows Horizontal Location
 	// Bit[10: 0] : CROP_X_OFF  , Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : CROP_X_WIDTH, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) CROP_X_OFF and CROP_X_WIDTH'w lower 4Bits will be Always zero by Hardware
 	*/
  	DISP_VIDEO1_SRC_Y_CROP    = ( DISP_REG_VIDEO1_BASE ) + ( 6 << 2),//0x418000D8
  	/* Define the Video Soruce Cropping Windows Vertical Location
 	// Bit[10: 0] : CROP_Y_OFF  , Reset Value 0
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : CROP_Y_HEIGHT, Reset Value 0
 	// Bit[31:27] : Reserved to be zero
 	//      *Note :  (1) CROP_Y_OFF and CROP_Y_HEIGHT'w lower 2Bits will be Always zero by Hardware
 	*/
	DISP_VIDEO1_CMD           = (DISP_REG_VIDEO1_BASE) + ( 7 << 2),//0x418000DC
	/* Video Command Register
	// Bit[27: 0] : Command Data
	// Bit[29:28] : Command Index
	// Bit[31:30] : Command Level
	//
	//       *Note: Level	Index	Name	        Description
	//                 0	  1	  DF_CMD_INIT	    Initialization level parameter
	//                 1	  0	  DF_CMD_SEQ	    Sequence level parameter
	//                 2	  0	  DF_CMD_FM_YT	    Frame level Command for Luma Top Field
	//                 2	  1	  DF_CMD_FM_YB	    Frame level Command for Luma Bottom Field
	//                 2	  2	  DF_CMD_FM_CT	    Frame level Command for Chroma Top Field
	//                 2      3	  DF_CMD_FM_CB	    Frame level Command for Chroma Bottom Field
	*/
 	
	/***************Video 1 STATUS(Command Info) Register******************/
 	/*  Note: 
 	//  (1) All the Status Register is Read only and do not need to be doubled
	/   (2) Most of the infomation comes from the current used video commands
	*/
	DISP_VIDEO1_STA_ADDR       = ( DISP_REG_VIDEO1_BASE ) + (8 << 2),//0x418000E0
	/* Video Address which is fetching
	// Bit[27: 0] : CURRENT_VIDEO_ADDR
	// Bit[31:28] : Reserved, Read as zero
	*/
	DISP_VIDEO1_STA0           = ( DISP_REG_VIDEO1_BASE ) + (9 << 2),//0x418000E4 
	/* Video Status #0
	// Bit[    0] : Video Display Enable Status 
	// Bit[    1] : Video Enable Status
    // Bit[    2] : Video Address Valid
	// Bit[    3] : Reserved,Read as zero
	// Bit[    4] : OutIF is HD
	// Bit[    5] : Video Display is Interlaced
	// Bit[    6] : Reserved, Read as zero
	// Bit[ 8: 7] : Compositor Error
	// Bit[30: 9] : Reserved, Read as zero
    // Bit[   31] : Command FIFO Full
	*/
	DISP_VIDEO1_STA1           = ( DISP_REG_VIDEO1_BASE ) + (10<< 2),//0x418000E8 
    /* Video Status #1
	// Bit[    0] : OutIF  VSync
	// Bit[    1] : Reserved, Read as zero
	// Bit[    2] : OutIF Bottom Flag
	// Bit[    3] : Reserved, Read as zero
	// Bit[15: 4] : OutIF Line Count
	// Bit[20:16] : Video Command FIFO Size
	// Bit[22:21] : Video FrameBuf Id
	// Bit[28:23] : Video Frame Repeat Count
	// Bit[31:29] : Reserved, Read as zero
	*/
    DISP_VIDEO1_STA2           = ( DISP_REG_VIDEO1_BASE ) + (11 << 2),//0x418000EC
    /*Video Status #2
    // Bit[31: 0] : Video Display Frame Number,Reset value zero
    */
 	DISP_VIDEO1_STA_IMG_SIZE   = ( DISP_REG_VIDEO1_BASE ) + (12 << 2),//0x418000F0 
 	/*
 	// Bit[10: 0] : image_width   : Reset Value 1920;
 	// Bit[15:11] : Reserved to be zero
 	// Bit[26:16] : image_height  : Reset Value 1088;
 	// Bit[31:27] : Reserved to be zero
 	*/
 	DISP_VIDEO1_STA_FRM_INFO   = ( DISP_REG_VIDEO1_BASE ) + (13 << 2),//0x418000F4 
 	/* Current Display Frame/Field Info from CMD
 	// Bit[    0] : IsHD (mem_width_sel) : Reset Value 0;
	// Bit[    1] : VIDEO_TYPE     : 0: M2VD Fmt, 1: H264 Fmt, Reset Value 0,//YTop CMD Bit[2]
	// Bit[    2] : H264_MAP       : 0: H264 2D , 1: H264 1D,  Reset Value 0,//YTop CMD Bit[3]
	//     *Note  : This Bit is Valid Onlywhen the VIDEO_TYPPE is H264;
	// Bit[    3] : cmd_fifo_full  : Reset value 0
	// Bit[    4] : is_progress_seq: Reset Value 0 //YTop CMD Bit[0]
	// Bit[    5] : is_top_field   : Reset Value 1 //YTop CMD Bit[1] Inverse
	// Bit[7 : 6] : Reserved to Be Zero
	// Bit[11: 8] : sca_ver_init_phase: Reset Value 0 //YBot CMD Bit[3:0]
	// Bit[15:12] : sca_hor_init_phase: Reset Value 0 //CTop CMD Bit[3:0]
	// Bit[20:16] : repeat_cnt        : Reset Value 0;
	// Bit[23:21] : Reserved to be zero
	// Bit[28:24] : cmd_fifo_size     : Reset Value 16;
	*/
	DISP_VIDEO1_STA_Y_TOPADDR  = ( DISP_REG_VIDEO1_BASE ) + (14 << 2),//0x418000F8 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_yt, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO1_STA_Y_BOTADDR  = ( DISP_REG_VIDEO1_BASE ) + (15 << 2),//0x418000FC 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_yb, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO1_STA_C_TOPADDR  = ( DISP_REG_VIDEO1_BASE ) + (16 << 2),//0x41800100 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_ct, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO1_STA_C_BOTADDR  = ( DISP_REG_VIDEO1_BASE ) + (17 << 2),//0x41800104 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_cb, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO1_STA_DISP_NUM   = ( DISP_REG_VIDEO1_BASE ) + (18 << 2),//0x41800108 
	/* Status Register, Read only, do not need doubled
	// Bit[31: 0] : disp_num, Reset value 0
	*/
/*************VIDEO COMMAND Format******************/
	/*
	// 1. Add VIDEO_TYPE to YTopCMD Bit[2]:
	//    0: M2VD Fmt, 1: H264 Fmt, Reset Value 0,
	// 2. Add H264_MAP   to YTopCMD Bit[3]:
	//    0: H264 1D , 1: H264 2D,  Reset Value 0,//YTop CMD Bit[3]
	//    *Note  : This Bit is Only Valid when the VIDEO_TYPPE is H264;
	//
	//Bit[31:30] : CMD_LEVEL: 
	0 : Inital
	1 : Sequence Level
	2 : Picture
	others : Invalid
	//---------Intial Level Command Format----------------//
	//Bit[31:30]: CMD_LEVEL, Must be 0;
	//Bit[29:28]: CMD_IDX  , Must be 1;
	//Bit[ 4: 0]: HD_FLAG  , == 0, SD Format != 0 HD Format
	//            Reset Value 0;

	//--------Sequece Level Command Format---------------//
	//Bit[31:30]: CMD_LEVEL, Must be 1;
	//Bit[29:28]: CMD_IDX  , Must be 0;
	//Bit[24:14]: Video Source Width , Reset Value 1920(0x780)
	//Bit[10: 0]: Video Soruce Height, Reset Value 1088(0x440)

	//--------Picture Level Command Format: YTop Field Info----//
	//Bit[31:30]: CMD_LEVEL, Must be 2;
	//Bit[29:28]: CMD_IDX  , Must be 0;
	//Bit[27: 4]: YTop Field Address, 128BitWord Address
	== BytesAdress >> 4, Reset Value 0x0;
	//Bit[    3]: Is2Dor1D: 
	For H264 Decoded DPB: 1,
	For MPEG2 and H264 1D Format: 0,
	Reset Value 0;
	//Bit[    2]: IsH264OrMPEG2: H264 : 1, MPEG2: 0, 
	Reset Value 0;
	//Bit[    1]: Fetch Bottom Field: 1,

	Fetch Top Field: 0,
	This Bit Valid only if the Bit[0] is 0;
	Reset Value 0;
	//Bit[    0]: Fetch the Whole Frame: 1
	Fetch only one Field : 0, 
	The Field Id decided by Bit[1],
	Reset Value 0;

	//--------Picture Level Command Format: YBot Field Info----//
	//Bit[31:30]: CMD_LEVEL, Must be 2;
	//Bit[29:28]: CMD_IDX  , Must be 1;
	//Bit[27: 4]: YBot Field Address, 128BitWord Address
	== BytesAdress >> 4, Reset Value 0x0;
	//Bit[ 3: 0]: Video Scaler Vertical Intial Phase: 0..15,
	Reset Value 0;

	//--------Picture Level Command Format: CTop Field Info----//
	//Bit[31:30]: CMD_LEVEL, Must be 2;
	//Bit[29:28]: CMD_IDX  , Must be 2;
	//Bit[27: 4]: CTop Field Address, 128BitWord Address
	== BytesAdress >> 4, Reset Value 0x0;
	//Bit[ 3: 0]: Video Scaler Horizontal Intial Phase: 0..15,
	Reset Value 0;

	//--------Picture Level Command Format: CBot Field Info----//
	//Bit[31:30]: CMD_LEVEL, Must be 2;
	//Bit[29:28]: CMD_IDX  , Must be 2;
	//Bit[27: 4]: CBot Field Address, 128BitWord Address
	== BytesAdress >> 4, Reset Value 0x0;

	//Note: Command Send Sequence:
	//1st: When Source Video Size changed Send the Intial and Sequence
	Level Command.
	//2nd: Send the Picture Level Command For Every Display Unit:
	a Display Unit can be: a Frame, a top Field and a Bottom Field
	The Picture Level Command Should be send in the following Order:
	0: YTopField CMD
	1: YBotField CMD
	2: CTopField CMD
	3: CBotField CMD
	*/
	
    /***************VIDEO2 Register******************/
	/* Refer the Define of Video 1 Registes*/
	DISP_VIDEO2_CTRL           = ( DISP_REG_VIDEO2_BASE ) + ( 0 << 2),//0x41800140	
 	DISP_VIDEO2_ALPHA_CTRL     = ( DISP_REG_VIDEO2_BASE ) + ( 1 << 2),//0x41800144	
 	DISP_VIDEO2_KEY_LUMA       = ( DISP_REG_VIDEO2_BASE ) + ( 2 << 2),//0x41800148	
 	DISP_VIDEO2_X_POSITON      = ( DISP_REG_VIDEO2_BASE ) + ( 3 << 2),//0x4180014C	 
 	DISP_VIDEO2_Y_POSITON      = ( DISP_REG_VIDEO2_BASE ) + ( 4 << 2),//0x41800150	 
 	DISP_VIDEO2_SRC_X_CROP     = ( DISP_REG_VIDEO2_BASE ) + ( 5 << 2),//0x41800154	
 	DISP_VIDEO2_SRC_Y_CROP     = ( DISP_REG_VIDEO2_BASE ) + ( 6 << 2),//0x41800158	
	DISP_VIDEO2_CMD            = ( DISP_REG_VIDEO2_BASE ) + ( 7 << 2),//0x4180015C	
	DISP_VIDEO2_STA_ADDR       = ( DISP_REG_VIDEO2_BASE ) + ( 8 << 2),//0x41800160	
	DISP_VIDEO2_STA0           = ( DISP_REG_VIDEO2_BASE ) + ( 9 << 2),//0x41800164	
	DISP_VIDEO2_STA1           = ( DISP_REG_VIDEO2_BASE ) + (10 << 2),//0x41800168	
	DISP_VIDEO2_STA2           = ( DISP_REG_VIDEO2_BASE ) + (11 << 2),//0x4180016C	
 	DISP_VIDEO2_STA_IMG_SIZE   = ( DISP_REG_VIDEO2_BASE ) + (12 << 2),//0x41800170	
	DISP_VIDEO2_STA_FRM_INFO   = ( DISP_REG_VIDEO2_BASE ) + (13 << 2),//0x41800174	
	DISP_VIDEO2_STA_Y_TOPADDR  = ( DISP_REG_VIDEO2_BASE ) + (14 << 2),//0x41800178	
	DISP_VIDEO2_STA_Y_BOTADDR  = ( DISP_REG_VIDEO2_BASE ) + (15 << 2),//0x4180017C	
	DISP_VIDEO2_STA_C_TOPADDR  = ( DISP_REG_VIDEO2_BASE ) + (16 << 2),//0x41800180	
	DISP_VIDEO2_STA_C_BOTADDR  = ( DISP_REG_VIDEO2_BASE ) + (17 << 2),//0x41800184		
	DISP_VIDEO2_STA_DISP_NUM   = ( DISP_REG_VIDEO2_BASE ) + (18 << 2),//0x41800188	

	/***************VIDEO3 Register******************/
	DISP_VIDEO3_CTRL          = ( DISP_REG_VIDEO3_BASE ) + ( 0 << 2),//0x418001C0
	/*
	// Bit[ 1: 0]: VIDEO_EN,    0/3: Disable, 
	//                            1: Video on OutIF#0
	//                            2: Video on OutIF#1, Reset Value: 0;
	//      *Note: if video 3 is enabled on OutIF #0.then video2 on OutIF #0 is not valid
	//             if video 3 is enabled on OutIF #1,then video2 on OutIF #1 is not valid
    // Bit[ 4: 2]: Reserved,Read as zero 
	// Bit[    5]: Auto Correct Top Bottom Field Enable,Reset Value: 1
	// Bit[31: 6]: Reseverd to be Zero,
	*/	 
	DISP_VIDEO3_ALPHA_CTRL    = ( DISP_REG_VIDEO3_BASE ) + ( 1 << 2),//0x418001C4
	/*
	// Bit[ 7: 0] : DEFAULT_ALPHA, Reset Value: 0,
	// Bit[31: 8] : Reseverd to be Zero,
	*/
	DISP_VIDEO3_X_POSITON     = ( DISP_REG_VIDEO3_BASE ) + ( 3 << 2),//0x418001CC 
	/*
	// Bit[10: 0] : X_START, Reset Value 0
	// Bit[14:11] : X_START_CROP_PIXEL_NUM, Reset Value 0
	// Bit[15   ] : Reserved to be zero
	// Bit[26:16] : X_END, Reset Value 0
	// Bit[30:27] : X_END_CROP_PIXEL_NUM, Reset Value 0
	// Bit[31   ] : ENA_X_START_END_CROP, Reset Value 0
	//      *Note :  (1) X_START and X_END indict the Video Layer Horizontal Location is Screen
	//               (2) The Video Display Width = X_END - X_START
	//               (3) X_START and X_END Can be one Pixel Aligned!
	//      *Note : About the X_START_END CROP Pixel Number,
	//               (1) if ENA_X_START_END_CROP is Enabled, the final display Width is 
	//      X_END - X_START + X_START_CROP_PIXEL_NUM + X_END_CROP_PIXEL_NUM;
	//               (2) but First TX_START_CROP_PIXEL_NUM Pixels and 
	//      Last X_END_CROP_PIXEL_NUM pixels will be discard to displayed in the Screen.
	//               (3) Make Sure the Total Width should not exceed 1920.
	*/
	DISP_VIDEO3_Y_POSITON     = ( DISP_REG_VIDEO3_BASE ) + ( 4 << 2),//0x418001D0 
	/*
	// Bit[10: 0] : Y_START, Reset Value 0
	// Bit[15:11] : Reserved to be zero
	// Bit[26:16] : Y_END, Reset Value 0
	// Bit[31:27] : Reserved to be zero
	//      *Note :  (1) Y_START and Y_END indict the Video Layer Vertical Location is Screen
	//               (2) The Video Display Height = Y_END - Y_START
	//               (3) Y_START and X_END Must be a Multiply of 4!    
	*/
	DISP_VIDEO3_SRC_X_CROP    = ( DISP_REG_VIDEO3_BASE ) + ( 5 << 2),//0x418001D4      
	/* Define the Video Soruce Cropping Windows Horizontal Location
	// Bit[10: 0] : CROP_X_OFF  , Reset Value 0
	// Bit[31:11] : Reserved to be zero
	//      *Note :  (1) CROP_X_OFF  lower 4Bits will be Always zero by Hardware
	*/
	DISP_VIDEO3_SRC_Y_CROP    = ( DISP_REG_VIDEO3_BASE ) + ( 6 << 2),//0x418001D8
	/* Define the Video Soruce Cropping Windows Vertical Location
	// Bit[10: 0] : CROP_Y_OFF  , Reset Value 0
	// Bit[31:11] : Reserved to be zero
	//      *Note :  (1) CROP_Y_OFF lower 2Bits will be Always zero by Hardware
	*/
	DISP_VIDEO3_CMD           = (DISP_REG_VIDEO3_BASE) + ( 7 << 2),//0x418001DC
	/* Video Command Register
	// Bit[27: 0] : Command Data
	// Bit[29:28] : Command Index
	// Bit[31:30] : Command Level
	//
	//       *Note: Level	Index	Name	        Description
	//                 0	  1	  DF_CMD_INIT	    Initialization level parameter
	//                 1	  0	  DF_CMD_SEQ	    Sequence level parameter
	//                 2	  0	  DF_CMD_FM_YT	    Frame level Command for Luma Top Field
	//                 2	  1	  DF_CMD_FM_YB	    Frame level Command for Luma Bottom Field
	//                 2	  2	  DF_CMD_FM_CT	    Frame level Command for Chroma Top Field
	//                 2      3	  DF_CMD_FM_CB	    Frame level Command for Chroma Bottom Field
	*/
	
	

	/***************Video 1 STATUS(Command Info) Register******************/
	/*  Note: 
	//  (1) All the Status Register is Read only and do not need to be doubled
	/   (2) Most of the infomation comes from the current used video commands
	*/
	DISP_VIDEO3_STA_ADDR       = ( DISP_REG_VIDEO3_BASE ) + (8 << 2),//0x418001E0 
	/* Video Address which is fetching
	// Bit[27: 0] : CURRENT_VIDEO_ADDR
	// Bit[31:28] : Reserved, Read as zero
	*/
	DISP_VIDEO3_STA0           = ( DISP_REG_VIDEO3_BASE ) + (9 << 2),//0x418001E4 
	/* Video Status #0
	// Bit[    0] : Video Display Enable Status 
	// Bit[    1] : Video Enable Status
	// Bit[    2] : Video Address Valid
	// Bit[    3] : Reserved,Read as zero
	// Bit[    4] : OutIF is HD
	// Bit[    5] : Video Display is Interlaced
	// Bit[    6] : Reserved, Read as zero
	// Bit[ 8: 7] : Compositor Error
	// Bit[31: 9] : Reserved, Read as zero
	*/
	DISP_VIDEO3_STA1           = ( DISP_REG_VIDEO3_BASE ) + (10<< 2),//0x418001E8 
	/* Video Status #1
	// Bit[    0] : OutIF  VSync
	// Bit[    1] : Reserved, Read as zero
	// Bit[    2] : OutIF Bottom Flag
	// Bit[    3] : Reserved, Read as zero
	// Bit[15: 4] : OutIF Line Count
	// Bit[20:16] : Video Command FIFO Size
	// Bit[22:21] : Video FrameBuf Id
	// Bit[28:23] : Video Frame Repeat Count
	// Bit[31:29] : Reserved, Read as zero
	*/
    DISP_VIDEO3_STA2           = ( DISP_REG_VIDEO3_BASE ) + (11 << 2),//0x418001EC 
    /*Video Status #2
    // Bit[31: 0] : Video Display Frame Number,Reset value zero
     */
	DISP_VIDEO3_STA_IMG_SIZE   = ( DISP_REG_VIDEO3_BASE ) + (12<< 2),//0x418001F0 
	/*
	// Bit[10: 0] : image_width   : Reset Value 1920;
	// Bit[15:11] : Reserved to be zero
	// Bit[26:16] : image_height  : Reset Value 1088;
	// Bit[31:27] : Reserved to be zero
	*/
	DISP_VIDEO3_STA_FRM_INFO   = ( DISP_REG_VIDEO3_BASE ) + (13 << 2),//0x418001F4 
	/* Current Display Frame/Field Info from CMD
	// Bit[    0] : IsHD (mem_width_sel) : Reset Value 0;
	// Bit[    1] : VIDEO_TYPE     : 0: M2VD Fmt, 1: H264 Fmt, Reset Value 0,//YTop CMD Bit[2]
	// Bit[    2] : H264_MAP       : 0: H264 2D , 1: H264 1D,  Reset Value 0,//YTop CMD Bit[3]
	//     *Note  : This Bit is Valid Onlywhen the VIDEO_TYPPE is H264;
	// Bit[    3] : cmd_fifo_full  : Reset value 0
	// Bit[    4] : is_progress_seq: Reset Value 0 //YTop CMD Bit[0]
	// Bit[    5] : is_top_field   : Reset Value 1 //YTop CMD Bit[1] Inverse
	// Bit[7 : 6] : Reserved to Be Zero
	// Bit[11: 8] : sca_ver_init_phase: Reset Value 0 //YBot CMD Bit[3:0]
	// Bit[15:12] : sca_hor_init_phase: Reset Value 0 //CTop CMD Bit[3:0]
	// Bit[20:16] : repeat_cnt        : Reset Value 0;
	// Bit[23:21] : Reserved to be zero
	// Bit[28:24] : cmd_fifo_size     : Reset Value 16;
	*/
	DISP_VIDEO3_STA_Y_TOPADDR  = ( DISP_REG_VIDEO3_BASE ) + (14 << 2),//0x418001F8 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_yt, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO3_STA_Y_BOTADDR  = ( DISP_REG_VIDEO3_BASE ) + (15 << 2),//0x418001FC 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_yb, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO3_STA_C_TOPADDR  = ( DISP_REG_VIDEO3_BASE ) + (16 << 2),//0x41800200 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_ct, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO3_STA_C_BOTADDR  = ( DISP_REG_VIDEO3_BASE ) + (17 << 2),//0x41800204 
	/*
	// Bit[3 : 0] : Reserved to be zero
	// Bit[28: 4] : o_base_addr_cb, Reset Value 0
	// Bit[31:29] : Reserved to be zero
	*/
	DISP_VIDEO3_STA_DISP_NUM   = ( DISP_REG_VIDEO3_BASE ) + (18 << 2),//0x41800208 
	/* Status Register, Read only, do not need doubled
	// Bit[31: 0] : disp_num, Reset value 0
	*/
	
 	/***************COMPOSITOR1 Register******************/
 	DISP_COMP1_BACK_GROUND    = ( DISP_REG_COMP1_BASE ) + ( 0 << 2),//0x41800240 
 	/*
 	// Bit[7 : 0] : BG_Y : Reset Value: 0x10;
 	// Bit[15: 8] : BG_U : Reset Value: 0x80;
 	// Bit[23:16] : BG_V : Reset Value: 0x80;
 	*/ 
 	DISP_COMP1_Z_ORDER        = ( DISP_REG_COMP1_BASE ) + ( 1 << 2),//0x41800244 
 	/* Four Layer's Order, Reset Order, BackGroud, Video1, Video2, Gfx1, Gfx2
 	// Bit[1 : 0] : Video1 ZOrder, Reset Value: 0;
 	// Bit[3 : 2] : Reserved to Zero; 
 	// Bit[5 : 4] : Video2 ZOrder, Reset Value: 0;
 	// Bit[7 : 6] : Reserved to Be Zero;
 	// Bit[9 : 8] : Gfx1 ZOrder, Reset Value: 0;
 	// Bit[11:10] : Reserved to Be Zero;
 	// Bit[13:12] : Gfx2 ZOrder, Reset Value: 0;
 	// Bit[15:14] : Reserved to Be Zero;
 	*/
 	
 	/***************COMPOSITOR2 Register******************/
 	/* Refert the COMPOSITOR1 Register Define*/
 	DISP_COMP2_BACK_GROUND    = ( DISP_REG_COMP2_BASE ) + ( 0 << 2),//0x41800280 
 	DISP_COMP2_Z_ORDER        = ( DISP_REG_COMP2_BASE ) + ( 1 << 2),//0x41800284  	
 	
 	/***************HD2SD_CAPTURE Register******************/
 	DISP_HD2SD_CTRL       	  = ( DISP_REG_HD2SD_BASE ) + ( 0 << 2),//0x418002C0 
 	/*
 	// Bit[    0] : HD2SD_ENABLE: 0: Disable, 1: Eanble, Reset Value: 0;
 	// Bit[    1] : Capture Data Source Select: 0:OutIF #0,1: OutIF #1
 	// Bit[    2] : BYPASS_SCA  : 0: Do HD2SD Scaler, 1; ByPass HD2SD Scaler
 	//  ***Important Note***
 	//     if BYPASS_SCA is 0, 
 	//     the Destination Width and Height should both Less than source with and height.
 	// Bit[    3] : Reversed to be Zero;
 	// Bit[    4] : 1: Vertical Reverse Store, 0: Vertical Normal Store, 
 	//              Reset value 0;
 	// Bit[    5] : 1: Horizontal Reverse Store, 0: Horizontal Normal Store, 
 	//              Reset value 0;
 	// Bit[    6] : IS_FRAME: Set the Store Mode
 	//              0: Interlaced Store, 
 	//                 When Interlaced display, Top and Bot Field Merge to a Frame
 	//                 When Progressive display, Store Top Field into a Frame Only
 	//              1: Progressive Store,
 	//                 When Interlaced display, Top and Bot Field Store to seperated Frame
 	//                 When Progressive display, Frame store into a Frame
 	// Bit[    7] : IS_HD   : Control the LinePitch
 	//              0: LinePitch for a Frame is 1024Bytes
 	//              1: LinePitch for a Frame is 2048Bytes
 	// Bit[11: 8] : Store Dram Buffer FIFO Depth Minus 1, Reset Value 0,
 	// Bit[15:12] : Reversed to be Zero;
 	// Bit[19:16] : Vertical Initial Phase for Scaler, Max 15, Reset Value 0;
 	// Bit[23:20] : Horizontal Initial Phase for Scaler, Max 15, Reset Value 0;
 	// Bit[31:24] : Reversed to be Zero;
 	*/
 	DISP_HD2SD_DES_SIZE       = ( DISP_REG_HD2SD_BASE ) + ( 1 << 2),//0x418002C4 
 	/*
 	// Bit[10: 0] : Capture Destination Width , Should be 8Pixel Aligned,
 	//              Reset Value 0;
 	// Bit[26:16] : Capture Destination Height, Should be 2 Lines Aligned,
 	//       *Note: IF Interlaced Display, the Height should be a Field Height
 	*/
 	DISP_HD2SD_ADDR_Y         = ( DISP_REG_HD2SD_BASE ) + ( 2 << 2),//0x418002C8 
 	/* The First Capture Buffer Luma Start Address 
 	// Bit[24: 0] : 64Bit Word Address, Reset Value 0
 	*/
 	DISP_HD2SD_ADDR_C         = ( DISP_REG_HD2SD_BASE ) + ( 3 << 2),//0x418002CC 
 	/* The Fisrt Caputure Buffer Chroma Start Address
 	// Bit[24: 0] : 64Bit Word Address, Reset Value 0
 	*/
 	DISP_HD2SD_BUF_PITCH      = ( DISP_REG_HD2SD_BASE ) + ( 4 << 2),//0x418002D0 
 	/* Capture Frame Buffer Pitch
 	// Bit[24: 0] : 64Bit Word Address, Reset Value 0
 	*/
 	DISP_HD2SD_STATUS         = ( DISP_REG_HD2SD_BASE ) + ( 5 << 2),//0x418002D4 
 	/* HD2SD Runtime Status, Read only, do not need to be Doubled
 	// Bit[3 : 0]: hd2sd_current_buf, Reset 0, Update after capture one Frame
 	// Bit[31: 4]: Reserved to be Zero
 	*/
 	/***************OUTIF1 Register******************/
 	DISP_OUTIF1_CTRL          = ( DISP_REG_OUTIF1_BASE ) + ( 0 << 2),//0x41800300 
 	/*
 	// Bit[    0] : DISP_EN: Reset Value 0,
 	//              0: Disable Current Output, 1: Enable Current Output;
 	// Bit[    1] : Output Display Clock Phase
	//              0: Use Rise Edge to Capture Data, 1: Use Fall Edge to Capture Data
	// Bit[3:  2] : Pixel Repeat Times
	//              0,2: None Repeat, 1: Repeat 1 time, 3: Repeat 3 times  
 	// Bit[    4] : IS_HD: 0: Output SD Signal, 1: Output HD Signal, Reset Value 0;
 	// Bit[    5] : IS_INTERLACED: Reset Value 0;
 	//              0: Output Progressive Signal, 1: Output Interlaced Signal
 	// Bit[    6] : YC Mux Enable: Reset Value 0;
	// Bit[    7] : IS_PAL: Reset Value 0;
 	// Bit[    8] : TopField/Frame Interrupt Enable, 1: Enable, 0 Disable, Reset 0;
 	// Bit[    9] : BotField Interrupt Enable, 1: Enable, 0 Disable, Reset 0;
 	//      *Note : (1) Top Field /Frame Interrupt happened when ??
 	//              (2) Bot Field /Frame Interrupt happened when ??
 	// Bit[   10] : YUV444 to YUV422 ChromaConvert Type, 
 	//              0: Average(U0 + U1 /2 ),
 	//              1: Drop (U0 Only, Drop U1),
 	//              Reset Value 0;
 	// Bit[   11] : Color Space Format : RGB(1) or YUV(0),Reset Value 0
	// Bit[   12] : H_SYNC_POLARITY: Reset Value 0;
	// Bit[   13] : V_SYNC_POLARITY: Reset Value 0;
	// Bit[   14] : Color Modulator Enable,Reset Value 0
 	// Bit[31:15] : Reserved to be zero
 	*/
 	DISP_OUTIF1_X_SIZE        = ( DISP_REG_OUTIF1_BASE ) + ( 1 << 2),//0x41800304 
 	/*
 	// Bit[11: 0] : Pixel Number For one line    : Reset Value 0;
 	// Bit[15:13] : Reserved to be Zero;
 	// Bit[27:16] : Active Pixels for one line   : Reset Value 0;
 	// Bit[31:28] : Reserved to be Zero;
 	*/
 	DISP_OUTIF1_Y_SIZE       = ( DISP_REG_OUTIF1_BASE ) + ( 2 << 2),//0x41800308 
 	/*
 	// Bit[11: 0] : Line Numer for one Frame
	// Bit[15:12] : Reserved to be Zero
	// Bit[26:16] : Active Line Number for one Frame
 	// Bit[31:27] : Reserved to be Zero
 	*/
 	DISP_OUTIF1_HSYNC        = ( DISP_REG_OUTIF1_BASE ) + ( 3 << 2),//0x4180030C
 	/*
 	// Bit[9: 0]  : Horizontal Sync Front Porch : Reset Value 0;
 	// Bit[19:10] : Horizontal Sync Width       : Reset Value 0;
 	// Bit[29:20] : Horizontal Sync Back Porch  : Reset Value 0;
 	// Bit[31:30] : Reserved to be zero
 	*/
 	DISP_OUTIF1_VSYNC    = ( DISP_REG_OUTIF1_BASE ) + ( 4 << 2),//0x41800310  	 	
 	/*
 	// Bit[7:  0] : Vertical Sync Front Porch : Reset Value 0;
 	// Bit[15: 8] : Vertical Sync Width       : Reset Value 0; 
 	// Bit[23:16] : Vertical Sync Back Porch   : Reset Value 0;
 	// Bit[31:27] : Reserved to be zero
 	*/
	DISP_OUTIF1_CLIP           = ( DISP_REG_OUTIF1_BASE ) + ( 5 << 2),//0x41800314 
	/*
	// Bit[    0] : YUV Clip Enable, 1: Enable, 0: Disable, Reset Value: 1
	// Bit[15: 8] : CLIP_Y_LOW  : Reset Value:  16
	// Bit[23:16] : CLIP_Y_RANGE: Reset Value: 219
	// Bit[31:24] : CLIP_C_RANGE: Reset Value: 224
	*/

	DISP_OUTIF1_CM_COEF0_012  = ( DISP_REG_OUTIF1_BASE ) + ( 6 << 2),//0x41800318 
	/* Color Modulator Coeff
	// Bit[8 : 0] : CM_COEFF00, Reset Value 0
	// Bit[18:10] : CM_COEFF01, Reset Value 0
	// Bit[28:20] : CM_COEFF02, Reset Value 0 
	//       *Note: The Three Coeff is 9 Bits:
	//              Bit[8] is Sign Bit, 0 Positive, 1 Negative
	//              Bit[7..6] is Integer Bits
	//              Bit[5..0] is Fraction Bits
	*/
	DISP_OUTIF1_CM_COEF0_3    = ( DISP_REG_OUTIF1_BASE ) + ( 7 << 2),//0x4180031C 
	/* Color Modulator Coeff
	// Bit[12: 0] : CM_COEFF03, Reset Value 0
	//       *Note: Bit[12] is Sign Bit, 0 Positive, 1 Negative
	//              Bit[11:0] is Integer Bits
	*/
	DISP_OUTIF1_CM_COEF1_012  = ( DISP_REG_OUTIF1_BASE ) + ( 8 << 2),//0x41800320 
	/* Color Modulator Coeff
	// Bit[8 : 0] : CM_COEFF10, Reset Value 0
	// Bit[18:10] : CM_COEFF11, Reset Value 0
	// Bit[28:20] : CM_COEFF12, Reset Value 0 
	*/
	DISP_OUTIF1_CM_COEF1_3    = ( DISP_REG_OUTIF1_BASE ) + (9 << 2),//0x41800324 
	/* Color Modulator Coeff
	// Bit[12: 0] : CM_COEFF13, Reset Value 0
	*/
	DISP_OUTIF1_CM_COEF2_012  = ( DISP_REG_OUTIF1_BASE ) + (10 << 2),//0x41800328 
	/* Color Modulator Coeff
	// Bit[8 : 0] : CM_COEFF20, Reset Value 0
	// Bit[18:10] : CM_COEFF21, Reset Value 0
	// Bit[28:20] : CM_COEFF22, Reset Value 0 
	*/
	DISP_OUTIF1_CM_COEF2_3    = ( DISP_REG_OUTIF1_BASE ) + (11 << 2),//0x4180032C 
	/* Color Modulator Coeff
	// Bit[12: 0] : CM_COEFF03, Reset Value 0
	*/

 	DISP_OUTIF1_STA_DISP_SIZE = ( DISP_REG_OUTIF1_BASE ) + (12 << 2),//0x41800330 
 	/* Runtime Status, read only, donot need to be doubled
 	// Bit[11: 0] : DISP_WIDTH : Reset Value 0;
 	// Bit[15:12] : Reserved to be zero
 	// Bit[27:16] : DISP_HEIGHT: Reset Value 0;
 	// Bit[31:28] : Reserved to be zero
 	*/
 	DISP_OUTIF1_STA_LINE      = ( DISP_REG_OUTIF1_BASE ) + (13<< 2),//0x41800334 
 	/* Runtime Status, read only, donot need to be doubled
 	// Bit[    0] : sys_bottom_flag: Reset Value 0
 	//              0: Current Display is Top Field or Frame
 	//              1: Current Display is Bot Field
 	// Bit[    1] : sys_frame_sync : Reset Value 0
 	// Bit[    2] : sys_vsync      : Reset Value 0
 	// Bit[    3] : Active Line    : Reset Value 0
 	// Bit[15: 4] : Reserved to be zero
 	// Bit[28:16] : sys_line_count : Reset Value 0
 	//              Current Display v_cnt
 	// Bit[31:27] : Reserved to be zero
 	*/ 	
 	/***************OUTIF2 Register******************/
 	DISP_OUTIF2_CTRL           = ( DISP_REG_OUTIF2_BASE ) + ( 0 << 2),//0x41800340  
 	DISP_OUTIF2_X_SIZE         = ( DISP_REG_OUTIF2_BASE ) + ( 1 << 2),//0x41800344  
 	DISP_OUTIF2_Y_SIZE         = ( DISP_REG_OUTIF2_BASE ) + ( 2 << 2),//0x41800348  
 	DISP_OUTIF2_HSYNC          = ( DISP_REG_OUTIF2_BASE ) + ( 3 << 2),//0x4180034C  
 	DISP_OUTIF2_VSYNC          = ( DISP_REG_OUTIF2_BASE ) + ( 4 << 2),//0x41800350  
	DISP_OUTIF2_CLIP           = ( DISP_REG_OUTIF2_BASE ) + ( 5 << 2),//0x41800354  
	DISP_OUTIF2_CM_COEF0_012   = ( DISP_REG_OUTIF2_BASE ) + ( 6 << 2),//0x41800358  
	DISP_OUTIF2_CM_COEF0_3     = ( DISP_REG_OUTIF2_BASE ) + ( 7 << 2),//0x4180035C  
	DISP_OUTIF2_CM_COEF1_012   = ( DISP_REG_OUTIF2_BASE ) + ( 8 << 2),//0x41800360  
	DISP_OUTIF2_CM_COEF1_3     = ( DISP_REG_OUTIF2_BASE ) + ( 9 << 2),//0x41800364  
	DISP_OUTIF2_CM_COEF2_012   = ( DISP_REG_OUTIF2_BASE ) + (10 << 2),//0x41800368  
	DISP_OUTIF2_CM_COEF2_3     = ( DISP_REG_OUTIF2_BASE ) + (11 << 2),//0x4180036C  
 	DISP_OUTIF2_STA_DISP_SIZE  = ( DISP_REG_OUTIF2_BASE ) + (12 << 2),//0x41800370  
 	DISP_OUTIF2_STA_LINE       = ( DISP_REG_OUTIF2_BASE ) + (13 << 2),//0x41800374 

	DISP_REG_SIZE = 0x1000,
};

#endif
