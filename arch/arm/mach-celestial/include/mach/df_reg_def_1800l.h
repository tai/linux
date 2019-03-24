#ifndef _DF_REG_DEF_H_
#define _DF_REG_DEF_H_

#define TVE0_REG_BASE		0x80168000
#define TVE0_REG_SIZE		0x00001000
#define TVE1_REG_BASE		0x80160000
#define TVE1_REG_SIZE		0x00001000

/****************Register IF******************************/
/*
// (1) Re-map all the Bits Defination
// (2) Add Status Register of Each Model
// (3) Add Interrupt Control
*/

enum _ADDRESS_SPACE_
{
    HW_REG_ADDR_MASK = ~(0xFFFFF),
};

#define DF_INT_OUTIF1_UNDERFLOW     0x1
#define DF_INT_OUTIF2_UNDERFLOW     0x2
#define DF_INT_HD2SD_OVERFLOW       0x4
#define DF_INT_OUTIF1_VSYNC_TOP     0x8
#define DF_INT_OUTIF1_VSYNC_BOT     0x10
#define DF_INT_OUTIF2_VSYNC_TOP     0x20
#define DF_INT_OUTIF2_VSYNC_BOT     0x40
#define DF_INT_HD2SD_FINISH         0x80
#define DF_INT_F2F_FINISH           0x100

enum  _DF_REGISTER_BANK_
{
    DISP_REG_BASE               =   0xb1800000,

    DISP_REG_COMMON_BASE        =   DISP_REG_BASE + (0x00 << 2),  //0x0
    DISP_REG_GFX1_BASE          =   DISP_REG_BASE + (0x10 << 2), //0x40
    DISP_REG_GFX2_BASE          =   DISP_REG_BASE + (0x20 << 2), //0x80
    DISP_REG_VIDEO1_BASE        =   DISP_REG_BASE + (0x30 << 2), //0xc0
    DISP_REG_VIDEO2_BASE        =   DISP_REG_BASE + (0x60 << 2),//0x180
    DISP_REG_VIDEO3_BASE        =   DISP_REG_BASE + (0x70 << 2),//0x1c0
    DISP_REG_COMP_BASE          =   DISP_REG_BASE + (0xA0 << 2),//0x280
    DISP_REG_HD2SD_BASE         =   DISP_REG_BASE + (0xB0 << 2),//0x2c0
    DISP_REG_OUTIF1_BASE        =   DISP_REG_BASE + (0xD0 << 2),//0x340
    DISP_REG_OUTIF2_BASE        =   DISP_REG_BASE + (0xE0 << 2),//0x380

    /***************COMMON Register******************/
    DISP_UPDATE_REG             =   ( DISP_REG_COMMON_BASE ) + ( 0 << 2),
    /*
    //  Bit[            0   ]   :   Write 1 Before Update  Registers
    //                              Write 0 After Finish Update Registers, Reset Value :0
    */
    DISP_UPDATE_RT_REG			=	( DISP_REG_COMMON_BASE ) + ( 1 << 2),
    /*  Realtime Register update, used for Video & HD2SD Registers
    //  Bit[            0   ]   :   Write 1 Before Update  Registers
    //                              Write 0 After Finish Update Registers, Reset Value :0
    */
    DISP_INT_STA_COP            =   ( DISP_REG_COMMON_BASE ) + ( 2 << 2),
    /* Interupt Status for Video RISC coprocessor,write coresponding bits 1 to clear it
    //  Bit[            0   ]   :   OutIF #1 FIFO Underflow, Reset Value:0
    //  Bit[            1   ]   :   OutIF #2 FIFO Underflow, Reset Value:0
    //  Bit[            2   ]   :   HD2SD Store FIFO Overflow, Reset Value:0
    //  Bit[            3   ]   :   OutIF #1 Top Field/Frame VSync, Reset Value:0
    //  Bit[			4	]	:	OutIF #1 Bottom Field VSync, Reset Value:0
    //  Bit[            5   ]   :   OutIF #2 Top Field/Frame VSync, Reset Value:0
    //  Bit[			6	]	:	OutIF #2 Bottom Field VSync, Reset Value:0
    //  Bit[            7   ]   :   HD2SD Store a Field/Frame, Reset Value:0
    //  Bit[            8   ]   :   Video Store a Field/Frame, Reset Value:0      
    */ 
    DISP_INT_MSK_COP            =   ( DISP_REG_COMMON_BASE ) + ( 3 << 2),
    /* Interrupt Mask for  DISP_INT_STA_COP,0 is mask, 1 is unmask ,Reset Value:0 
    */
    DISP_INT_STA_HOST           =   ( DISP_REG_COMMON_BASE ) + ( 4 << 2),
    /* Interupt Status for Host CPU,write coresponding bits 1 to clear it
    //  Bit[            0   ]   :   OutIF #1 FIFO Underflow, Reset Value:0
    //  Bit[            1   ]   :   OutIF #2 FIFO Underflow, Reset Value:0
    //  Bit[            2   ]   :   HD2SD Store FIFO Overflow, Reset Value:0
    //  Bit[            3   ]   :   OutIF #1 Top Field/Frame VSync, Reset Value:0
    //  Bit[			4	]	:	OutIF #1 Bottom Field VSync, Reset Value:0
    //  Bit[            5   ]   :   OutIF #2 Top Field/Frame VSync, Reset Value:0
    //  Bit[			6	]	:	OutIF #2 Bottom Field VSync, Reset Value:0
    //  Bit[            7   ]   :   HD2SD Store a Field/Frame, Reset Value:0
    //  Bit[            8   ]   :   Video Store a Field/Frame, Reset Value:0      
    */

    DISP_INT_MSK_HOST           =   ( DISP_REG_COMMON_BASE ) + ( 5 << 2),
    /* Interrupt Mask for DISP_INT_STA_HOST,0 is mask, 1 is unmask. Reset Value:0
    */
    DISP_ARBITOR_QUANTUM        =   ( DISP_REG_COMMON_BASE ) + ( 6 << 2),
    /*
    //  Bit[    15  :    0  ]   :   HD2SD Store Write Time Limit, Reset Value:0
    //  Bit[    23  :   16  ]   :   VIDEO_REQ_TIME_LIMIT,Video Read Memory Time Limit, Reset Value:0
    //  Bit[    28  :   24  ]   :   VIDEO_REQ_NUM_LIMIT,Video Read Memory Number Limit,Reset Value:0
    //                              *Note   :   There is a free-run counter, and its value is always at [0,VIDEO_REQ_TIME_LIMIT],
    //                                          When its value is less than or equal VIDEO_REQ_NUM_LIMIT,the arbiter will allow video1
    //                                          to generate a request.  
    */
    /***************GFX1 Register******************/
    DISP_GFX1_CTRL              =   ( DISP_REG_GFX1_BASE ) + ( 0 << 2),
    /*
    //  Bit[            0   ]   :   GFX_EN,   GFX1 Enable, Reset Value: 0;
    //  Bit[            3   ]   :   COLOR_KEYING_EN,  0: Disable, 1: Enable, Reset Value 0;
    //  Bit[    5   :   4   ]   :   FETCH_TYPE, Decide the Fetch Type when Interlaced Display, no effect when Progressive display,
    //                              0/3: Auto, 1; Fetch Top Field Only, 2: Fetch Bot Field only, Reset Value: 0;
    //  Bit[            6   ]   :   CONV_EN, 1: Enable the RGB to YUV Conversion,0: Disable Reset value: 1;
    //  Bit[    31  :   7   ]   :   Reseverd to be Zero
     */
    DISP_GFX1_FORMAT            =   ( DISP_REG_GFX1_BASE ) + ( 1 << 2),
    /*
    //  Bit[    2   :   0   ]   :   FORMAT, Reset Value :6
    //                              RGB565    3'b010
    //                              ARGB4444  3'b011
    //                              A0        3'b100
    //                              ARGB1555  3'b101
    //                              ARGB8888  3'b110
    //  Bit[            4   ]   :   Alpha Is Valid,Reset Value :1  
    */
    DISP_GFX1_ALPHA_CTRL        =   ( DISP_REG_GFX1_BASE ) + ( 2 << 2),
    /*
    //  Bit[    9   :   0   ]   :   DEFAULT_ALPHA,    Reset Value: 0,
    //  Bit[    19  :   10  ]   :   ARGB1555_ALPHA0,  Reset Value: 0,
    //  Bit[    29  :   20  ]   :   ARGB1555_ALPHA1,  Reset Value: 1023,
    */
    DISP_GFX1_KEY_RED           =   ( DISP_REG_GFX1_BASE ) + ( 3 << 2),
    /*
    //  Bit[    7   :   0   ]   :   RED_MIN, Reset Value: 0,
    //  Bit[    15  :   8   ]   :   RED_MAX, Reset Value: 255,
    */
    DISP_GFX1_KEY_BLUE          =   ( DISP_REG_GFX1_BASE ) + ( 4 << 2),
    /*
    //  Bit[    7   :   0   ]   :   BLUE_MIN, Reset Value: 0,
    //  Bit[    15  :   8   ]   :   BLUE_MAX, Reset Value: 255,
    */
    DISP_GFX1_KEY_GREEN         =   ( DISP_REG_GFX1_BASE ) + ( 5 << 2),
    /*
    //  Bit[    7   :   0   ]   :   GREEN_MIN, Reset Value: 0,
    //  Bit[    15  :   8   ]   :   GREEN_MAX, Reset Value: 255,
    */
    DISP_GFX1_BUF_START         =   ( DISP_REG_GFX1_BASE ) + ( 6 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Gfx FrameBuf Start Address
    */
    DISP_GFX1_LINE_PITCH        =   ( DISP_REG_GFX1_BASE ) + ( 7 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Gfx FrameBuf LinePitch(Bytes), Reset Value: 0
    //		*Note	:	(1)	For 32-bit Color the LinePitch must be 4 bytes allgined.
    //					(2)	For 16-bit Color the LinePitch must be 2 bytes alligned.
    */
    DISP_GFX1_X_POSITON         =   ( DISP_REG_GFX1_BASE ) + ( 8 << 2),
    /*
    //  Bit[    10  :   0   ]   :   X_START, Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    //      *Note :  (1) X_END = X_START + SrcWidth - 1, 
    //                   SrcWidth multiply with Pixel Bit Width should be a multiply of 128bits
    //               (2)
    //                   X_START and X_END indict the Gfx Layer Display X Location in Screen.
    */
    DISP_GFX1_Y_POSITON         =   ( DISP_REG_GFX1_BASE ) + ( 9 << 2),
    /*
    //  Bit[    10  :   0   ]   :   Y_START, Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   Y_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    //      *Note :  (1) If Progressive Displayed, 
    //                   Y_START = Gfx Layer Screen Y OffSet,
    //                   Y_END   = Y_START + GfxSrcHeight;
    //               (2) If Interlaced Displayed, 
    //                   Y_START = Gfx Layer Screen Y OffSet / 2
    //                   Y_END   = (Y_START + GfxSrcHeight)  / 2; 
    */

    /***************GFX2 Register******************/
    /*
    // *Note: All the Bits of GFX2 Registers are same with GFX1 Registers
    */
    
    DISP_GFX2_CTRL              =   ( DISP_REG_GFX2_BASE ) + ( 0 << 2),
    DISP_GFX2_FORMAT            =   ( DISP_REG_GFX2_BASE ) + ( 1 << 2),
    DISP_GFX2_ALPHA_CTRL        =   ( DISP_REG_GFX2_BASE ) + ( 2 << 2),
    DISP_GFX2_KEY_RED           =   ( DISP_REG_GFX2_BASE ) + ( 3 << 2),
    DISP_GFX2_KEY_BLUE          =   ( DISP_REG_GFX2_BASE ) + ( 4 << 2),
    DISP_GFX2_KEY_GREEN         =   ( DISP_REG_GFX2_BASE ) + ( 5 << 2),
    DISP_GFX2_BUF_START         =   ( DISP_REG_GFX2_BASE ) + ( 6 << 2),
    DISP_GFX2_LINE_PITCH        =   ( DISP_REG_GFX2_BASE ) + ( 7 << 2),
    DISP_GFX2_X_POSITON         =   ( DISP_REG_GFX2_BASE ) + ( 8 << 2),
    DISP_GFX2_Y_POSITON         =   ( DISP_REG_GFX2_BASE ) + ( 9 << 2),
    
    /***************VIDEO1 Register******************/
    DISP_VIDEO1_CTRL            =   ( DISP_REG_VIDEO1_BASE ) + ( 0 << 2),
    /*Video1 Control    Register
    //  Bit[            0   ]   :   1-  Enable      0- Disable(default)  
    //  Bit[            2   ]   :   1-  Luma Vertical Scaler is 2 taps
    //                              0-  Luma Vertical Scaler is 4 taps	(default)
    //  Bit[            3   ]   :   1-  Source is YUV422 Format 0-Source Is YUV420 Format
    //  Bit[            4   ]   :   F2F Store Mode: 0-Interlaced(default),1-Progressive
    //  Bit[            5   ]   :   F2F Interlaced Store Mode:0-to bottom field,1-to top field
    //  Bit[            6   ]   :   1-Enable Luma Direction Interpolation
    //                              0-Disable Luma Direction Interpolation (default)
    //  Bit[            7   ]   :   1-Enable Chroma Direction Interpolation
    //                              0-Disable Chroma Direction Interpolation (default)
    //  Bit[    9   :   8   ]   :   Luma Deintlace Mode:    0-  Disable
    //                                                      1-  3 Field
    //                                                      3-  4 Field
    //                                                      2-  Forbidden  	
    //  Bit[    11  :  10   ]   :   Chroma Deintlace Mode:  0-  Disable
    //                                                      1-  3 Field
    //                                                      2-  Forbidden
    //  Bit[    15  :   12  ]   :   STORE_LINE_PITCH:   0-2048(default),others - STORE_LINE_PITCH * 128
    //	Bit[	23	:	16	]	:	FETCH_LINE_PITCH:	0-32768,others - FETCH_LINE_PITCH * 128,default:16;
    //  Bit[    31  :   24  ]   :   MAX_DIFF_3D,default :0xFF
     */     
    DISP_VIDEO1_ALPHA_CTRL      =   ( DISP_REG_VIDEO1_BASE ) + ( 1 << 2),
    /*
    //  Bit[           16   ]   :   1-  F2F Mode    0- On the fly Mode (default)
    //  Bit[    9   :   0   ]   :   DEFAULT_ALPHA, Reset Value: 0,
    */
    DISP_VIDEO1_X_POSITON       =   ( DISP_REG_VIDEO1_BASE ) + ( 2 << 2),
    /*
    //  Bit[    10  :   0   ]   :   X_START, Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
    DISP_VIDEO1_Y_POSITON       =   ( DISP_REG_VIDEO1_BASE ) + ( 3 << 2),
    /*
    //  Bit[    10  :   0   ]   :   Y_START, Reset Value 0
    //  Bit[    26  :   16  ]   :   Y_END,Reset Valud 0
    */
    DISP_VIDEO1_SRC_X_CROP      =   ( DISP_REG_VIDEO1_BASE ) + ( 4 << 2),
    /* Define the Video Soruce Cropping Windows Horizontal Location
    //  Bit[    10  :   0   ]   :   SRC_X_START  , Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   SRC_X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
     DISP_VIDEO1_SRC_Y_CROP     =   ( DISP_REG_VIDEO1_BASE ) + ( 5 << 2),
    /* Define the Video Soruce Cropping Windows Vertical Location
    //  Bit[    10  :   0   ]   :   SRC_Y_START  , Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   SRC_Y_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    //      *Note :  If the source is interlaced,the SRC_Y_START and SRC_Y_END must be 2 lines aligned
    */
    DISP_VIDEO1_LAI_PARAM       =   (DISP_REG_VIDEO1_BASE )     +   (   6   <<  2   ),
    /*Video LAI Parameter Register
    //  Bit[    4   :    0  ]   :   MIN0 , Reset Value 0
    //  Bit[    12  :    8  ]   :   MIN1 , Reset Value 0
    //  Bit[    19  :    16 ]   :   K0 , Reset Value 0
    //  Bit[    27  :    24 ]   :   K1 , Reset Value 0
    //  Bit[    31  :   28  ]   :   DIR_MAX,Reset Value 0xf
     */
    DISP_VIDEO1_YV_RATIO        =   (DISP_REG_VIDEO1_BASE)    +    (7    <<    2),
    /*Video Scaler Ratio for Luma on vertical direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_Y_V_R0 , Reset Value 0
    //	Bit[	27	:	24	]	:	Luma Initial Center Line for Top Field/Frame,Reset Value 1
    //	Bit[	31	:	28	]	:	Luma Initial Center Line for Bottom Field,Reset Value 1
    */

    DISP_VIDEO1_YH_RATIO        =   (DISP_REG_VIDEO1_BASE)    +    (8    <<    2),
    /*Video Scaler Ratio for Luma on horizontal direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_Y_H_R0 , Reset Value 0
    */

    DISP_VIDEO1_CV_RATIO        =   (DISP_REG_VIDEO1_BASE)    +    (9    <<    2),
    /*Video Scaler Ratio for Chroma on vertical direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_C_V_R0 , Reset Value 0
    //	Bit[	27	:	24	]	:	Chroma Initial Center Line for Top Field/Frame,Reset Value 1
    //	Bit[	31	:	28	]	:	Chroma Initial Center Line for Bottom Field,Reset Value 1
    */

    DISP_VIDEO1_CH_RATIO        =   (DISP_REG_VIDEO1_BASE)    +    (10    <<    2),
    /*Video Scaler Ratio for Chroma on horizontal direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_C_H_R0 , Reset Value 0
    */
    DISP_VIDEO1_Y_NOLINE_D      =   (DISP_REG_VIDEO1_BASE)    +    (11    <<    2),
    /*
    //  Bit[    23  :    0  ]   :   VIDEO_SCA_Y_NL_D , Reset Value 0
    */
    DISP_VIDEO1_C_NOLINE_D      =   (DISP_REG_VIDEO1_BASE)    +    (12    <<    2),
    /*
    //  Bit[    23  :    0  ]   :   VIDEO_SCA_C_NL_D , Reset Value 0
    */
    DISP_VIDEO1_Y_NOLINE_K      =   (DISP_REG_VIDEO1_BASE)    +    (13    <<    2),
    /*
    //  Bit[    12  :    0  ]   :   VIDEO_SCA_Y_H_K_ABS, Reset Value 0
    //  Bit[             15 ]   :   VIDEO_SCA_Y_H_K_SIGN, Reset Value 0
    //  Bit[    20  :    16 ]   :   VIDEO_SCA_Y_H_K_RADIX, Reset Value 0
    //  Bit[    31  :    21 ]   :   VIDEO_SCA_Y_H_N1, Reset Value 0
    */

    DISP_VIDEO1_C_NOLINE_K      =   (DISP_REG_VIDEO1_BASE)    +    (14    <<    2),
    /*
    //  Bit[    12  :    0  ]   :   VIDEO_SCA_C_H_K_ABS, Reset Value 0
    //  Bit[             15 ]   :   VIDEO_SCA_C_H_K_SIGN, Reset Value 0
    //  Bit[    20  :    16 ]   :   VIDEO_SCA_C_H_K_RADIX, Reset Value 0
    //  Bit[    31  :    21 ]   :   VIDEO_SCA_C_H_N1, Reset Value 0
    */
    DISP_VIDEO1_HOR_PHASE       =   (DISP_REG_VIDEO1_BASE)    +    (15    <<    2),
    /*Video Scaler Horizontal Phase
    //  Bit[    5   :    0  ]   :   Video1 Horizontal Initial Phase for Luma, Reset Value 0
    //  Bit[    21  :    16 ]   :   Video1 Horizontal Initial Phase for Chroma, Reset Value 0
    */
    DISP_VIDEO1_VER_PHASE       =   (DISP_REG_VIDEO1_BASE)    +    (16    <<    2),
    /*Video1 Scaler Vertical Phase
    //  Bit[    5   :    0  ]   :   Video1 Top Field Vertical Initial Phase for Luma, Reset Value 0
    //  Bit[    13  :    8  ]   :   Video1 Bot Field Vertical Initial Phase for Luma, Reset Value 0
    //  Bit[    21  :    16 ]   :   Video1 Top Field Vertical Initial Phase for Chroma, Reset Value 0
    //  Bit[    29  :    24 ]   :   Video1 Bot Field Vertical Initial Phase for Chroma, Reset Value 0
    */
    DISP_VIDEO1_CUR_YT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (17    <<    2),
    /* Video1 Current Luma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_CUR_CT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (18    <<    2),
    /* Video1 Current Chroma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
     */
    DISP_VIDEO1_CUR_YB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (19    <<    2),
    /* Video1 Current Luma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_CUR_CB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (20    <<    2),
    /* Video1 Current Chroma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_PRE_YT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (21    <<    2),
    /* Video1 Previous Field for Current Luma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_PRE_CT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (22    <<    2),
    /* Video1 Previous Field for Current Chroma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_PRE_YB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (23    <<    2),
    /* Video1 Previous Field for Current  Luma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_PRE_CB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (24    <<    2),
    /* Video1 Previous Field for Current  Chroma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_POS_YT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (25    <<    2),
    /* Video1 Posterior Field for Current  Luma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_POS_CT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (26    <<    2),
    /* Video1 Posterior Field for Current  Chroma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_POS_YB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (27    <<    2),
    /* Video1 Posterior Field for Current  Luma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_POS_CB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (28    <<    2),
    /* Video1 Posterior Field for Current  Chroma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_PPR_YT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (29    <<    2),
    /* Video1 Previous-Previous Field for Current Luma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_PPR_CT_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (30    <<    2),
    /* Video1 Previous-Previous Field for Current Chroma Top Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */

    DISP_VIDEO1_PPR_YB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (31    <<    2),
    /* Video1 Previous-Previous Field for Current  Luma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_PPR_CB_ADDR     =   (DISP_REG_VIDEO1_BASE)    +    (32    <<    2),
    /* Video1 Previous-Previous Field for Current  Chroma Bot Field Base Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_STORE_Y_ADDR    =   (DISP_REG_VIDEO1_BASE)    +    (33    <<    2),
    /*  Video1  Luma Store Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_STORE_C_ADDR    =   (DISP_REG_VIDEO1_BASE)    +    (34    <<    2),
    /*  Video1 Chroma Store Address
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero, Reset Value 0
    */
    DISP_VIDEO1_SRC_CTRL        =   (DISP_REG_VIDEO1_BASE)    +    (35    <<    2),
    /* Video1 Source Control
    //  Bit[            0   ]   :   Video Source Is Frame Format, Reset Value 0
    //  Bit[            8   ]   :   Video Source Fetch Top Field,It's only valid when Bit[0]
    //                              is 0, Reset Value 0
    */
    DISP_VIDEO1_COEFF_UP        =   (DISP_REG_VIDEO1_BASE)    +    (36    <<    2),
    /*  Video1 Scaler Coeff update enable
    //  Bit[            0   ]   :   Start update scaler coeff,
    //								this bit will be cleared to zero when the coeff update end, Reset Value 0
    */
    DISP_VIDEO1_COEFF_ADDR      =   (DISP_REG_VIDEO1_BASE)    +    (37    <<    2),
    /*  Video1 Scaler Coeff Base Address
    //  Bit[    31  :   0   ]   :   Scaler Coeff Base Address, Reset Value 0
    */
    DISP_VIDEO1_F2F_START       =   (DISP_REG_VIDEO1_BASE)    +    (38    <<    2),
    /*  Video1 F2F Start Control
    //  Bit[            0   ]   :   F2F Start A Frame/Field, this bit will be always read as zero, Reset Value 0
    */ 
    DISP_VIDEO1_STA_F2F         =   (DISP_REG_VIDEO1_BASE)    +    (39    <<    2),
    /*  Video1 F2F Status
    //  Bit[            8   ]   :   Video1 Internal Mode:0-F2D,1-F2F      
    //  Bit[            0   ]   :   Video1 F2F STATUS:0-Busy 1-IDLE
    */
    DISP_VIDEO1_STA_DISP_NUM    =   (DISP_REG_VIDEO1_BASE)    +    (40    <<    2),
    /* Video1 Display Number
    //  Bit[    31  :   0   ]   :   Video1 Display Number, Reset Value 0
    */

    /***************VIDEO2 Register******************/
    /*  Refer the Define of Video 1 Registes*/
    DISP_VIDEO2_CTRL            =   ( DISP_REG_VIDEO2_BASE ) + ( 0 << 2),
    /*  Video2 Control Register
    //  Bit[            0   ]   :   Video Enable, Reset Value 0
    //	Bit[	19	:	16	]	:	FETCH_LINE_PITCH:	0-2048(default),others - FETCH_LINE_PITCH * 128
    */

    DISP_VIDEO2_X_POSITON       =   ( DISP_REG_VIDEO2_BASE ) + ( 1 << 2),
    /*
    //  Bit[    10  :   0   ]   :   X_START, Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
    DISP_VIDEO2_Y_POSITON       =   ( DISP_REG_VIDEO2_BASE ) + ( 2 << 2),
    /*
    //  Bit[    10  :   0   ]   :   Y_START, Reset Value 0
    //  Bit[    26  :   16  ]   :   Y_END,Reset Valud 0
    */

    DISP_VIDEO2_YT_ADDR         =    ( DISP_REG_VIDEO2_BASE ) + ( 3 << 2),
    /*  Video2 Luma Top Field Address
    //  Bit[    31  :   0   ]   :   Video Luma Top Field Address, Reset Value 0
     */

    DISP_VIDEO2_CT_ADDR         =    ( DISP_REG_VIDEO2_BASE ) + ( 4 << 2),
    /* Video2 Chroma Top Field Address
    //  Bit[    31  :   0   ]   :   Video Chroma Top Field Address, Reset Value 0
    */
    DISP_VIDEO2_YB_ADDR         =    ( DISP_REG_VIDEO2_BASE ) + ( 5 << 2),
    /*  Video2 Luma Bot Field Address
    //  Bit[    31  :   0   ]   :   Video Luma Top Field Address, Reset Value 0
    */

    DISP_VIDEO2_CB_ADDR         =   ( DISP_REG_VIDEO2_BASE ) + ( 6 << 2),
    /*  Video2 Chroma Bot Field Address
    //  Bit[    31  :   0   ]   :   Video Chroma Top Field Address, Reset Value 0
    */
    DISP_VIDEO2_SRC_CTRL        =   ( DISP_REG_VIDEO2_BASE ) + ( 7 << 2),
    /*  Video2 Source Control Register
    //  Bit[            0   ]   :   Source Is Frame Format, Reset Value 0
    //  Bit[            8   ]   :   Source Fetch Top Field, Reset Value 0
    */
    DISP_VIDEO2_STA_DISP_NUM    =   ( DISP_REG_VIDEO2_BASE ) + ( 8 << 2),
    /*  Video2 Display Number
    //  Bit[    31  :   0   ]   :   Video Display Field/Frame Numbers, Reset Value 0
    */

    /***************Video3 Register******************/

    DISP_VIDEO3_CTRL            =   (DISP_REG_VIDEO3_BASE)  +   (0  <<  2),
    /*  Video3 Control Register
    //  Bit[            0   ]   :   Video Enable , Reset Value 0
    //	Bit[	19	:	16	]	:	FETCH_LINE_PITCH:	0-2048(default),others - FETCH_LINE_PITCH * 128   
    */
    
    DISP_VIDEO3_X_POSITON       =   ( DISP_REG_VIDEO3_BASE ) + ( 2 << 2),
    /*
    //  Bit[    10  :   0   ]   :   X_START, Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
    DISP_VIDEO3_Y_POSITON       =   ( DISP_REG_VIDEO3_BASE ) + ( 3 << 2),
    /*
    //  Bit[    10  :   0   ]   :   Y_START, Reset Value 0
    //  Bit[    26  :   16  ]   :   Y_END,Reset Valud 0
    */
    DISP_VIDEO3_SRC_X_CROP      =   ( DISP_REG_VIDEO3_BASE ) + ( 4 << 2),
    /* Define the Video Soruce Cropping Windows Horizontal Location
    //  Bit[    10  :   0   ]   :   SRC_X_START  , Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   SRC_X_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
     DISP_VIDEO3_SRC_Y_CROP     =   ( DISP_REG_VIDEO3_BASE ) + ( 5 << 2),
    /* Define the Video Soruce Cropping Windows Vertical Location
    //  Bit[    10  :   0   ]   :   SRC_Y_START  , Reset Value 0
    //  Bit[    15  :   11  ]   :   Reserved to be zero
    //  Bit[    26  :   16  ]   :   SRC_Y_END, Reset Value 0
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    //      *Note :  If the source is interlaced,the SRC_Y_START and SRC_Y_END must be 2 lines aligned
    */
    DISP_VIDEO3_YV_RATIO        =    (DISP_REG_VIDEO3_BASE)    +    (7    <<    2),
    /*Video Scaler Ratio for Luma on vertical direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_Y_V_R0, Reset Value 0
    //  Bit[    27  :   24  ]   :   Luma Initial Center Line for Top Field/Frame,Reset Value 1
    //  Bit[    31  :   28  ]   :   Luma Initial Center Line for Bottom Field,Reset Value 1
    */

    DISP_VIDEO3_YH_RATIO        =    (DISP_REG_VIDEO3_BASE)    +    (8    <<    2),
    /*Video Scaler Ratio for Luma on horizontal direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_Y_H_R0, Reset Value 0
    */

    DISP_VIDEO3_CV_RATIO        =    (DISP_REG_VIDEO3_BASE)    +    (9    <<    2),
    /*Video Scaler Ratio for Chroma on vertical direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_C_V_R0, Reset Value 0
    //  Bit[    27  :   24  ]   :   Chroma Initial Center Line for Top Field/Frame,Reset Value 1
    //  Bit[    31  :   28  ]   :   Chroma Initial Center Line for Bottom Field,Reset Value 1
    */

    DISP_VIDEO3_CH_RATIO        =    (DISP_REG_VIDEO3_BASE)    +    (10    <<    2),
    /*Video Scaler Ratio for Chroma on horizontal direction
    //  Bit[    16  :    0  ]   :   VIDEO_SCA_C_H_R0, Reset Value 0
    */
    DISP_VIDEO3_Y_NOLINE_D      =    (DISP_REG_VIDEO3_BASE)    +    (11    <<    2),
    /*
    //  Bit[    23  :    0  ]   :   VIDEO_SCA_Y_NL_D, Reset Value 0
    */
    DISP_VIDEO3_C_NOLINE_D      =    (DISP_REG_VIDEO3_BASE)    +    (12    <<    2),
    /*
    //  Bit[    23  :    0  ]   :   VIDEO_SCA_C_NL_D, Reset Value 0
    */
    DISP_VIDEO3_Y_NOLINE_K      =    (DISP_REG_VIDEO3_BASE)    +    (13    <<    2),
    /*
    //  Bit[    12  :    0  ]   :   VIDEO_SCA_Y_H_K_ABS, Reset Value 0
    //  Bit[             15 ]   :   VIDEO_SCA_Y_H_K_SIGN, Reset Value 0
    //  Bit[    20  :    16 ]   :   VIDEO_SCA_Y_H_K_RADIX, Reset Value 0
    //  Bit[    31  :    21 ]   :   VIDEO_SCA_Y_H_N1, Reset Value 0
    */

    DISP_VIDEO3_C_NOLINE_K      =    (DISP_REG_VIDEO3_BASE)    +    (14    <<    2),
    /*
    //  Bit[    12  :    0  ]   :   VIDEO_SCA_C_H_K_ABS, Reset Value 0
    //  Bit[             15 ]   :   VIDEO_SCA_C_H_K_SIGN, Reset Value 0
    //  Bit[    20  :    16 ]   :   VIDEO_SCA_C_H_K_RADIX, Reset Value 0
    //  Bit[    31  :    21 ]   :   VIDEO_SCA_C_H_N1, Reset Value 0
    */
    DISP_VIDEO3_HOR_PHASE       =    (DISP_REG_VIDEO3_BASE)    +    (15    <<    2),
    /*Video Scaler Horizontal Phase
    //  Bit[    5   :    0  ]   :   Video Horizontal Initial Phase for Luma, Reset Value 0
    //  Bit[    21  :    16 ]   :   Video Horizontal Initial Phase for Chroma, Reset Value 0
    */
    DISP_VIDEO3_VER_PHASE       =    (DISP_REG_VIDEO3_BASE)    +    (16    <<    2),
    /*Video1 Scaler Vertical Phase
    //  Bit[    5   :    0  ]   :   Video Top Field Vertical Initial Phase for Luma, Reset Value 0
    //  Bit[    13  :    8  ]   :   Video Bot Field Vertical Initial Phase for Luma, Reset Value 0
    //  Bit[    21  :    16 ]   :   Video Top Field Vertical Initial Phase for Chroma, Reset Value 0
    //  Bit[    29  :    24 ]   :   Video Bot Field Vertical Initial Phase for Chroma, Reset Value 0
    */
    DISP_VIDEO3_YT_ADDR         =    (DISP_REG_VIDEO3_BASE)    +    (17    <<    2),
    /* Video1 Current Luma Top Field Base Address, Reset Value 0
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero
    */

    DISP_VIDEO3_CT_ADDR         =    (DISP_REG_VIDEO3_BASE)    +    (18    <<    2),
    /* Video1 Current Chroma Top Field Base Address, Reset Value 0
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero
     */
    DISP_VIDEO3_YB_ADDR         =    (DISP_REG_VIDEO3_BASE)    +    (19    <<    2),
    /* Video1 Current Luma Bot Field Base Address, Reset Value 0
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero
    */

    DISP_VIDEO3_CB_ADDR         =    (DISP_REG_VIDEO3_BASE)    +    (20    <<    2),
    /* Video1 Current Chroma Bot Field Base Address, Reset Value 0
    //  Bit[    31  :   0   ]   :   Bit[6:0]    will always be set by zero
    */
    DISP_VIDEO3_SRC_CTRL        =   (DISP_REG_VIDEO3_BASE)    +    (27    <<    2),
    /* Video1 Source Control
    //  Bit[            0   ]   :   Video Source Is Frame Format, Reset Value 0
    //  Bit[            8   ]   :   Video Source Fetch Top Field,It's only valid when Bit[0]
    //                              is 0, Reset Value 0
    */
    DISP_VIDEO3_STA_DISP_NUM    =   (DISP_REG_VIDEO3_BASE)    +    (32    <<    2),
    /* Video1 Display Number
    //  Bit[    31  :   0   ]   :   Video Display Number, Reset Value 0
    */
    /***************COMPOSITOR1 Register******************/
    DISP_COMP_BACK_GROUND       =     ( DISP_REG_COMP_BASE ) + ( 0 << 2),
    /*
    //  Bit[    9   :   0   ]   :   BG_Y : Reset Value: 0x040;
    //  Bit[    19  :   10  ]   :   BG_U : Reset Value: 0x200;
    //  Bit[    29  :   20  ]   :   BG_V : Reset Value: 0x200;
    */ 
    DISP_COMP_Z_ORDER           =     ( DISP_REG_COMP_BASE ) + ( 1 << 2),
    /* Three Layer's Order, Reset Order, BackGroud, Video, Gfx1, Gfx2
    //  Bit[    1   :   0   ]   :   Video1 ZOrder, Reset Value: 0;
    //  Bit[    9   :   8   ]   :   Gfx1 ZOrder, Reset Value: 0;
    //  Bit[    13  :   12  ]   :   Gfx2 ZOrder, Reset Value: 0;
    */
    DISP_COMP_COLOR_ADJUST      =    ( DISP_REG_COMP_BASE ) + ( 2 << 2),
    /* Color Adjust
    //  Bit[    7   :   0   ]   :    Brightness, Reset Value: 0
    //  Bit[    13  :   8   ]   :    Contrast, Reset Value: 32
    //  Bit[    23  :   16  ]   :    Saturation, Reset Value: 64
    */
    DISP_COMP_SHARP_Y_CTRL      =    ( DISP_REG_COMP_BASE ) + ( 3 << 2),
    /* Sharpness Control for Luma
    //  Bit[            0   ]   :    Sharpness Enable, Reset Value: 0
    //  Bit[            1   ]   :    Median Enable, Reset Value: 0
    //  Bit[            2   ]   :    Coring Soft, Reset Value: 1
    //  Bit[    6   :   4   ]   :    Peaking Shrink, Reset Value: 0
    //  Bit[    13  :   8   ]   :    Coring, Reset Value: 0
    //  Bit[    23  :   16  ]   :    bpw, Reset Value: 0
    //  Bit[    31  :   24  ]   :    hpw, Reset Value: 0
    */
    DISP_COMP_SHARP_C_CTRL      =    ( DISP_REG_COMP_BASE ) + ( 4 << 2),
    /* Sharpness Control for Chroma
    //  Bit[            0   ]   :    Sharpness Enable, Reset Value: 0
    //  Bit[            1   ]   :    Median Enable, Reset Value: 0
    //  Bit[            2   ]   :    Coring Soft, Reset Value: 1
    //  Bit[    6   :   4   ]   :    Peaking Shrink, Reset Value: 0
    //  Bit[    13  :   8   ]   :    Coring, Reset Value: 0
    //  Bit[    23  :   16  ]   :    bpw, Reset Value: 0
    //  Bit[    31  :   24  ]   :    hpw, Reset Value: 0
    */
   /***************HD2SD_CAPTURE Register******************/

    DISP_HD2SD_CTRL             =   ( DISP_REG_HD2SD_BASE )     +   ( 0 << 2),
    /*
    //  Bit[            0   ]   :   HD2SD_ENABLE:   0- Disable, 1- Eanble, Reset Value: 0;
    //  Bit[            4   ]   :   STORE_FRAME :   0- HD2SD Store Interlaced, 1- HD2SD Store Frame Format, Reset Value: 0
    //  Bit[            5   ]   :   STORE_TOP_FIELD :  0-Store to topfield,1-Store to bottom field,only valid when STORE_FRAME is zero, Reset Value: 0
	//	Bit[	19	:	16	]	:	STORE_LINE_PITCH:  0-2048(default),others - STORE_LINE_PITCH * 128,Reset Value: 0;
    */
    DISP_HD2SD_DES_SIZE         =   ( DISP_REG_HD2SD_BASE )     +   ( 1 << 2),
    /*
    //  Bit[    10  :   0   ]   :   Capture Destination Frame Width , Should be 2 Pixel Aligned,
    //                              Reset Value 0;
    //  Bit[    26  :   16  ]   :   Capture Destination Frame Height, Should be 2 Lines Aligned,
    //								Reset Value: 0;
    */
    DISP_HD2SD_YV_RATIO         =   ( DISP_REG_HD2SD_BASE )     +   ( 2 << 2),
    /*
    //  Bit[    16  :   0   ]   :   Luma Vertical Ratio, Reset Value: 0;
    //	Bit[	27	:	24	]	:	Luma Initial Center Line for Top Field/Frame,Reset Value 1
    //	Bit[	31	:	28	]	:	Luma Initial Center Line for Bottom Field,Reset Value 1 
    */
    DISP_HD2SD_YH_RATIO         =   ( DISP_REG_HD2SD_BASE )     +   ( 3 << 2),
    /*
    //  Bit[    16  :   0   ]   :   Luma Horizontal Ratio, Reset Value: 0;
    */
    DISP_HD2SD_CV_RATIO         =   ( DISP_REG_HD2SD_BASE)      +   ( 4 <<2 ),
    /*
    //  Bit[    16  :   0   ]   :   Chroma Vertical Ratio, Reset Value: 0;
    //	Bit[	27	:	24	]	:	Chroma Initial Center Line for Top Field/Frame,Reset Value 1
    //	Bit[	31	:	28	]	:	Chroma Initial Center Line for Bottom Field,Reset Value 1 
    */
    DISP_HD2SD_CH_RATIO         =   ( DISP_REG_HD2SD_BASE)      +   ( 5 <<2 ),
    /*
    //  Bit[    16  :   0   ]   :   Chroma Horizontal Ratio, Reset Value: 0;
    */
    DISP_HD2SD_Y_NOLINE_D       =   ( DISP_REG_HD2SD_BASE)      +   ( 6 <<2 ),
    /*
    //  Bit[    23  :   0   ]   :   Y_D0_ABS:   Luma Non-linear , Reset Value: 0;
    */
    DISP_HD2SD_C_NOLINE_D       =   ( DISP_REG_HD2SD_BASE)      +   ( 7 <<2 ),
    /*
    //  Bit[    23  :   0   ]   :   C_D0_ABS:   Chroma Non-linear, Reset Value: 0;
    */
    DISP_HD2SD_Y_NOLINE_K       =   ( DISP_REG_HD2SD_BASE)      +   ( 8 <<2 ),
    /*
    //  Bit[    12  :   0   ]   :   Y_H_K_ABS, Reset Value: 0;
    //  Bit[            15  ]   :   Y_H_K_SIGN, Reset Value: 0;
    //  Bit[    20  :   16  ]   :   Y_H_K_RADIX, Reset Value: 0;
    //  Bit[    31  :   21  ]   :   Y_H_N1, Reset Value: 0;
    */
    DISP_HD2SD_C_NOLINE_K       =   ( DISP_REG_HD2SD_BASE)      +   ( 9 <<2 ),
    /*
    //  Bit[    12  :   0   ]   :   C_H_K_ABS, Reset Value: 0;
    //  Bit[            15  ]   :   C_H_K_SIGN, Reset Value: 0;
    //  Bit[    20  :   16  ]   :   C_H_K_RADIX, Reset Value: 0;
    //  Bit[    31  :   21  ]   :   C_H_N1, Reset Value: 0;
    */
    DISP_HD2SD_HOR_PHASE        =   ( DISP_REG_HD2SD_BASE )     +   ( 10 << 2),
    /*  HD2SD Horizontal Initial Phase
    //  Bit[    5   :    0  ]   :   Luma Horizontal Initial Phase, Reset Value: 0;
    //  Bit[    21  :    16 ]   :   Chroma Horizontal Initial Phase, Reset Value: 0;
    */
    DISP_HD2SD_VER_PHASE        =   ( DISP_REG_HD2SD_BASE )     +   ( 11 << 2),
    /* HD2SD Horizontal Initial Phase
    //  Bit[    5   :    0  ]   :   Luma Top Field Vertical Initial Phase, Reset Value: 0;
    //  Bit[    13  :    8  ]   :   Luma Bot Field Vertical Initial Phase, Reset Value: 0;
    //  Bit[    21  :    16 ]   :   Chroma Top Field Vertical Initial Phase, Reset Value: 0;
    //  Bit[    29  :    24 ]   :   Chroma Bot Field Vertical Initial Phase, Reset Value: 0;
    */
    DISP_HD2SD_YT_ADDR          =   ( DISP_REG_HD2SD_BASE )     +   ( 12 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Luma Top Field  Address, Bit[6:0]   will always be set to zero, Reset Value: 0;    
    */
    DISP_HD2SD_CT_ADDR          =   ( DISP_REG_HD2SD_BASE )     +   ( 13 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Chroma Top Field Address, Bit[6:0]  will always be set to zero, Reset Value: 0;
    */

    DISP_HD2SD_YB_ADDR          =   ( DISP_REG_HD2SD_BASE )     +   ( 14 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Luma Bottom Field  Address, Bit[6:0]   will always be set to zero, Reset Value: 0;
    */
    DISP_HD2SD_CB_ADDR          =   ( DISP_REG_HD2SD_BASE )     +   ( 15 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Chroma Bottom  Field Address, Bit[6:0]  will always be set to zero, Reset Value: 0;
    */
    DISP_HD2SD_COEFF_UP         =   ( DISP_REG_HD2SD_BASE )     +   ( 16 << 2),
    /*
    //  Bit[            0   ]   :   Scaler Coeff Update Start, this bit will be set to zero when the scaler coeff update complete, Reset Value: 0;
    */
    DISP_HD2SD_COEFF_ADDR       =   ( DISP_REG_HD2SD_BASE )     +   ( 17 << 2),
    /*
    //  Bit[    31  :   0   ]   :   Scaler Coeff Address, Bit[6:0]  will always be set to zero, Reset Value: 0;
    */
    DISP_HD2SD_STA_STORE        =   ( DISP_REG_HD2SD_BASE )     +   ( 18 << 2),
    /*
    //  Bit[            0   ]   :   HD2SD Store Finish a Field/Frame, Reset Value: 0;
    */
    DISP_HD2SD_STA_STORE_NUM    =   ( DISP_REG_HD2SD_BASE )     +   ( 19 << 2),
    /*
    //  Bit[    31  :   0   ]   :   HD2SD Store Frame/Field Number, Reset Value: 0;
    */

    /***************OUTIF1 Register******************/
    DISP_OUTIF1_CTRL            =   ( DISP_REG_OUTIF1_BASE ) + ( 0 << 2), //0xb1800340
    /*
    //  Bit[            0   ]   :   DISP_EN: Reset Value 0,
    //                              0: Disable Current Output, 1: Enable Current Output;
    //  Bit[            2   ]   :   IS_HD
    //  Bit[            5   ]   :   IS_INTERLACED: Reset Value 0;
    //                              0: Output Progressive Signal, 1: Output Interlaced Signal
    //  Bit[            7   ]   :   IS_PAL: Reset Value 0;
    //  Bit[            8   ]   :   TopField/Frame Interrupt Enable, 1: Enable, 0 Disable, Reset 0;
    //  Bit[            9   ]   :   BotField Interrupt Enable, 1: Enable, 0 Disable, Reset 0;
    //                              *Note : (1) Top Field /Frame Interrupt happened when ??
    //                                      (2) Bot Field /Frame Interrupt happened when ??
    //  Bit[            10  ]   :   Chroma Down mode
    //  Bit[            11  ]   :   TTL Dither Mode,0:8 bit panel,1:6 bit panel,Reset 0; 
    //  Bit[            12  ]   :   H_SYNC_POLARITY: Reset Value 0;
    //  Bit[            13  ]   :   V_SYNC_POLARITY: Reset Value 0;
    //  Bit[            14  ]   :   Dither Enable: Reset Value 1
     */
    DISP_OUTIF1_X_SIZE          =   ( DISP_REG_OUTIF1_BASE ) + ( 1 << 2),//0xb1800344 
    /*
    //  Bit[    11  :   0   ]   :   Pixel Number For one line    : Reset Value 0;
    //  Bit[    15  :   13  ]   :   Reserved to be Zero;
    //  Bit[    27  :   16  ]   :   Active Pixels for one line   : Reset Value 0;
    //  Bit[    31  :   28  ]   :   Reserved to be Zero;
    */
    DISP_OUTIF1_Y_SIZE          =   ( DISP_REG_OUTIF1_BASE ) + ( 2 << 2),//0xb1800348 
    /*
    //  Bit[    11  :   0   ]   :   Line Numer for one Frame
    //  Bit[    15  :   12  ]   :   Reserved to be Zero
    //  Bit[    26  :   16  ]   :   Active Line Number for one Frame
    //  Bit[    31  :   27  ]   :   Reserved to be Zero
    */
    DISP_OUTIF1_HSYNC           =   ( DISP_REG_OUTIF1_BASE ) + ( 3 << 2),//0xb180034c 
    /*
    //  Bit[    9   :   0   ]   :   Horizontal Sync Front Porch : Reset Value 0;
    //  Bit[    19  :   10  ]   :   Horizontal Sync Width       : Reset Value 0;
    //  Bit[    29  :   20  ]   :   Horizontal Sync Back Porch  : Reset Value 0;
    //  Bit[    31  :   30  ]   :   Reserved to be zero
    */
    DISP_OUTIF1_VSYNC           =   ( DISP_REG_OUTIF1_BASE ) + ( 4 << 2), //0xb1800350
    /*
    //  Bit[    7   :   0   ]   :   Vertical Sync Front Porch : Reset Value 0;
    //  Bit[    15  :   8   ]   :   Vertical Sync Width       : Reset Value 0; 
    //  Bit[    23  :   16  ]   :   Vertical Sync Back Porch   : Reset Value 0;
    //  Bit[    31  :   27  ]   :   Reserved to be zero
    */
    DISP_OUTIF1_VGA_LEVEL       =   ( DISP_REG_OUTIF1_BASE ) + ( 5<< 2),//0xb1800354
    /*
    //  Bit[    9   :    0  ]   :   VGA Black Level, Reset Value: 252;
    //  Bit[    25  :    16 ]   :   VGA White Level, Reset Value: 800;
    */
    DISP_OUTIF1_STA_DISP_SIZE   =   ( DISP_REG_OUTIF1_BASE ) + (6 << 2),//0xb1800358
    /* Runtime Status, read only, donot need to be doubled
    //  Bit[    11  :   0   ]   :   DISP_WIDTH : Reset Value 0;
    //  Bit[    15  :   12  ]   :   Reserved to be zero
    //  Bit[    27  :   16  ]   :   DISP_HEIGHT: Reset Value 0;
    //  Bit[    31  :   28  ]   :   Reserved to be zero
    */
    DISP_OUTIF1_STA_LINE        =   ( DISP_REG_OUTIF1_BASE ) + (7<< 2),//0xb180035c
    /* Runtime Status, read only,
    //  Bit[    11  :   0   ]   :   sys_line_count : Reset Value 0
    //  Bit[            16  ]   :   Active Line
    //  Bit[            20  ]   :   Bottom Flag
    //  Bit[            24  ]   :   Vsync
    */     
    /***************OUTIF2 Register******************/
    DISP_OUTIF2_CTRL            =   ( DISP_REG_OUTIF2_BASE ) + ( 0 << 2), 
    DISP_OUTIF2_X_SIZE          =   ( DISP_REG_OUTIF2_BASE ) + ( 1 << 2), 
    DISP_OUTIF2_Y_SIZE          =   ( DISP_REG_OUTIF2_BASE ) + ( 2 << 2), 
    DISP_OUTIF2_HSYNC           =   ( DISP_REG_OUTIF2_BASE ) + ( 3 << 2), 
    DISP_OUTIF2_VSYNC           =   ( DISP_REG_OUTIF2_BASE ) + ( 4 << 2), 
    DISP_OUTIF2_VGA_LEVEL	=   ( DISP_REG_OUTIF2_BASE ) + ( 5 << 2), //not use
    DISP_OUTIF2_STA_DISP_SIZE   =   ( DISP_REG_OUTIF2_BASE ) + ( 6 << 2),
    DISP_OUTIF2_STA_LINE        =   ( DISP_REG_OUTIF2_BASE ) + ( 7 << 2),

	DISP_REG_SIZE = 0x1000,
};

#endif
