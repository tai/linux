/*******************************************************************************

File name   : blit_cmd_reg_def.h

Description : blitter defs head file

COPYRIGHT (C) Celestial Semiconductor 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                           XM.Chen
*******************************************************************************/

#ifndef _BLIT_CMD_REG_DEF_H_
#define _BLIT_CMD_REG_DEF_H_

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _BLIT_COLOR_BIT_WIDTH_   //ARGB8565
{
	BLIT_ALPHA_BIT_WIDTH = 8,
	BLIT_RED_BIT_WIDTH   = 5,
	BLIT_GREEN_BIT_WIDTH = 6,
	BLIT_BLUE_BIT_WIDTH  = 5,
} BLIT_COLOR_BIT_WIDTH;

typedef enum _CLUT_CONST_
{
	CLUT4_TABLE_LEN  = 16,
	CLUT8_TABLE_LEN  = 256,
} CLUT_CONST;

typedef enum _BLIT_COMMAND_INDEX_
{
    CMD_STARTUP            = 0x00 ,
    CMD_S0_ADDR_INFO       = 0x04 ,
    CMD_S0_ADDR_INFO_L     = 0x05 ,
    CMD_S0_FORMAT          = 0x06 ,
    CMD_S0_FORMAT_L        = 0x07 ,
    CMD_S0_COLOR_KEY_MIN   = 0x08 ,
    CMD_S0_COLOR_KEY_MAX   = 0x0A ,
    CMD_S1_ADDR_INFO       = 0x0C ,
    CMD_S1_ADDR_INFO_L     = 0x0D ,
    CMD_S1_FORMAT          = 0x0E ,
    CMD_S1_FORMAT_L        = 0x0F ,
    CMD_S1_COLOR_KEY_MIN   = 0x10 ,
    CMD_S1_COLOR_KEY_MAX   = 0x12 ,
    CMD_D_ADDR_INFO        = 0x14 ,
    CMD_D_ADDR_INFO_L      = 0x15 ,
    CMD_D_FORMAT           = 0x16 ,
    //CMD_PIXEL_MAP          = 0x18 ,
    CMD_PIXEL_NUM_PER_LINE = 0x18,
    CMD_TOTAL_LINE_NUM     = 0x19,    

    CMD_ENDIAN_CTRL        = 0x1A ,
    CMD_CLUT4_TAB          = 0x1C ,
    CMD_CLUT4_IDX          = 0x1D ,
    CMD_CLUT8_TAB          = 0x1E ,
    CMD_CLUT8_IDX          = 0x1F ,

    //SCALOR Related
    CMD_SCALOR_S_PIXEL_NUM_PER_LINE = 0x30,
    CMD_SCALOR_S_TOTAL_LINE_NUM     = 0x31,
    CMD_SCALOR_INITIAL_PHASE        = 0x38,
    CMD_HFIR_COEFFICIETN_DATA       = 0x3B,
    CMD_VFIR_COEFFICIETN_DATA       = 0x3D,
    CMD_VALID_MAX,
    
} BLIT_COMMAND_INDEX;

typedef enum _BLIT_REG_STATUS_FORMAT_
{
	BLIT_STATUS_BIT     = 0,
	BLIT_INT_STATUS_BIT = 1,
	BLIT_ERR_BIT        = 2,
	BLIT_AHB_ERR_BIT    = 3, 
}BLIT_REG_STATUS_FORMAT;

typedef enum _BLIT_REG_CMDQUE_EMPTY_CNT_FORMAT_
{
	BLIT_REG_CMDQUE_EMPTY_CNT_LSB   = 0,
	BLIT_REG_CMDQUE_EMPTY_CNT_WIDTH = 9,	
}BLIT_REG_CMDQUE_EMPTY_CNT_FORMAT;

typedef enum _COMPOSITOR_OPT_FORMAT_
{
	COMPOSITOR_ENABLE_BIT  = 3,
	ROP_ENABLE_BIT         = 3,
	S0_ON_TOP_S1_BIT       = 0,
}COMPOSITOR_OPT_FORMAT;

typedef enum _S_OPT_FORMAT_
{
	S_ENABLE_BIT          = 3,
	S_COLORKEY_ENABLE_BIT = 2,
	S_CLUT_ENABLE_BIT     = 1,
	S_FETCH_DATA_BIT      = 0,	
}S_OPT_FORMAT;
typedef enum _S_SCAN_CTRL_FORMAT_
{
	S0_REVERSE_SCAN_BIT   = 0,
	S1_REVERSE_SCAN_BIT   = 1,
	D_REVERSE_SCAN_BIT    = 2,
}S_SCAN_CTRL_FORMAT;

typedef enum _ENDIAN_CTRL_FORMAT_
{
	ENDIAN_ENABLE_BIT   =  3,//Global no Enable Bit
	TWO_BYTE_ENDIAN_BIT =  2,
	NIBBLE_ENDIAN_BIT   =  1,
	BYTE_ENDIAN_BIT     =  0,	
}ENDIAN_CTRL_FORMAT;

typedef enum _BLIT_REG_FORMAT_
{
	REG_INFO_CMD_CNT_WIDTH   =  4,
	REG_INFO_CMD_CNT_LSB     = 28,
	
	REG_ENDIAN_CTRL_D_WIDTH  =  3,
	REG_ENDIAN_CTRL_D_LSB    =  0,
	REG_ENDIAN_CTRL_S1_WIDTH =  3,
	REG_ENDIAN_CTRL_S1_LSB   =  3,
	REG_ENDIAN_CTRL_S0_WIDTH =  3,
	REG_ENDIAN_CTRL_S0_LSB   =  6,		
	
}BLIT_REG_FORMAT;

typedef enum _BLIT_CMD_FORMAT_
{
	//ALL
	CMD_IDX_WIDTH =  8,
	CMD_IDX_LSB   = 24,  //Start up Bit
	//CMD_STARTUP
	SCAN_CTRL_WIDTH  =  4,
	SCAN_CTRL_LSB    = 20,
	S0_SCAL_ENA_BIT  = 17, 
	INT_ENABLE_WIDTH =  1,
	INT_ENABLE_LSB   = 16,
	ROP_ALPHA_CTRL_WIDTH = 2,
	ROP_ALPHA_CTRL_LSB   = 18,
	ROP_VAL_WIDTH    =  4,
	ROP_VAL_LSB      = 12,
	CMP_OPT_WIDTH    =  4,//Compositor Operation
	CMP_OPT_LSB      =  8,
	S1_OPT_WIDTH     =  4,
	S1_OPT_LSB       =  4,
	S0_OPT_WIDTH     =  4,
	S0_OPT_LSB       =  0, 
	
	//CMD_XX_FORMAT    
	CMD_FORMAT_WIDTH        =  4,
	CMD_FORMAT_LSB          =  0,
	CMD_FORMAT_ALPHA0_LSB   =  8,
	CMD_FORMAT_ALPHA0_WIDTH =  8,
	CMD_FORMAT_ALPHA1_LSB   = 16,
	CMD_FORMAT_ALPHA1_WIDTH =  8,

	CMD_FORMAT_ALPHA0_MIN_LSB   =  8,
	CMD_FORMAT_ALPHA0_MIN_WIDTH =  8,
	CMD_FORMAT_ALPHA0_MAX_LSB   = 16,
	CMD_FORMAT_ALPHA0_MAX_WIDTH =  8,
	

	//CMD_XX_ADDR_INFO
	CMD_SKIP_PIXEL_WIDTH =  1,
	CMD_SKIP_PIXEL_LSB   = 16,
	CMD_PITCH_WIDTH      = 16,
	CMD_PITCH_LSB        =  0,

	//CMD_XX_COLOR_KEY_MAX
	//CMD_XX_COLOR_KEY_MIN
	//CMD_XX_DEFAULT_COLOR
	CMD_ALPHA_WIDTH     =  8,
	CMD_ALPHA_LSB       = 24,
	CMD_RED_WIDTH       =  8,
	CMD_RED_LSB         = 16,
	CMD_GREEN_WIDTH     =  8,
	CMD_GREEN_LSB       =  8,
	CMD_BLUE_WIDTH      =  8,
	CMD_BLUE_LSB        =  0,

	//CMD_PIXEL_MAP
	CMD_LPIXEL_NUM_WIDTH = 16,
	CMD_LPIXEL_NUM_LSB   = 0,
	CMD_LINE_NUM_WIDTH   = 16,
	CMD_LINE_NUM_LSB     =  0,
	
	//CMD_CLUT4_IDX
	CMD_CLUT4_IDX_WIDTH  =  4,
	CMD_CLUT4_IDX_LSB    =  0,
	
	//CMD_CLUT8_IDX	
	CMD_CLUT8_IDX_WIDTH  =  8,
	CMD_CLUT8_IDX_LSB    =  0,
	
	//CMD_ENDIAN_CTRL
	//Extend for S0 and S1 and D ENDIAN
	CMD_GLOBAL_ENDIAN_WIDTH   =  3,
	CMD_GLOBAL_ENDIAN_LSB     =  0,

	CMD_S0_ENDIAN_WIDTH       =  4,
	CMD_S0_ENDIAN_LSB         =  4,

	CMD_S1_ENDIAN_WIDTH       =  4,
	CMD_S1_ENDIAN_LSB         =  8,

	CMD_D_ENDIAN_WIDTH        =  4,
	CMD_D_ENDIAN_LSB          = 12,

	//CMD_SCALOR_INITIAL_PHASE
	CMD_SCALOR_V_INIT_PHASE_WIDTH = 3,
	CMD_SCALOR_V_INIT_PHASE_LSB   = 8,
	CMD_SCALOR_H_INIT_PHASE_WIDTH = 3,
	CMD_SCALOR_H_INIT_PHASE_LSB   = 0,

	//CMD_HFIR_COEFFICIETN_DATA
	//CMD_VFIR_COEFFICIETN_DATA
	CMD_FIR_COEFF_PHASE_IDX_WIDTH =  3,
	CMD_FIR_COEFF_PHASE_IDX_LSB   = 20,
	CMD_FIR_COEFF_TAP_IDX_WIDTH   =  2,
	CMD_FIR_COEFF_TAP_IDX_LSB     = 16,
	CMD_FIR_COEFF_VALUE_WIDTH     = 16,
	CMD_FIR_COEFF_VALUE_LSB       =  0,
}BLIT_CMD_FORMAT;


#define SCALOR_FIR_COEFF_RED_REG_AVAILABLE (0)
//Register Define
typedef enum _BLIT_REG_ADDR_DEF_
{
// Register address
  BLIT_REG_CMD_QUE                = 0x000000 << 2          , // 32'b4000_0000
  BLIT_REG_STATUS                 = 0x000001 << 2         , // 32'b4000_0004
  BLIT_REG_CMDQUE_EMPTY_CNT       = 0x000002 << 2         , // 32'b4000_0008
  BLIT_REG_CMD_INFO               = 0x000003 << 2         , // 32'b4000_000C
  BLIT_REG_CLUT_IDX               = 0x000004 << 2         , // 32'b4000_0010
  BLIT_REG_CLUT_ENTRY             = 0x000005 << 2         , // 32'b4000_0014
  BLIT_REG_S0_ADDR                = 0x000008 << 2         , // 32'b4000_0020
  BLIT_REG_S0_LINE_PITCH          = 0x000009 << 2         , // 32'b4000_0024
  BLIT_REG_S0_SKIP_PIXEL          = 0x00000A << 2         , // 32'b4000_0028
  BLIT_REG_S0_COLOR_FORMAT        = 0x00000B << 2         , // 32'b4000_002C
  BLIT_REG_S0_COLOR_KEY_MIN       = 0x00000C << 2         , // 32'b4000_0030
  BLIT_REG_S0_COLOR_KEY_MAX       = 0x00000D << 2         , // 32'b4000_0034
  BLIT_REG_S0_DEFAULT_COLOR       = 0x00000E << 2         , // 32'b4000_0038
  BLIT_REG_S1_ADDR                = 0x00000F << 2         , // 32'b4000_003C
  BLIT_REG_S1_LINE_PITCH          = 0x000010 << 2         , //  32'b4000_0040
  BLIT_REG_S1_SKIP_PIXEL          = 0x000011 << 2         , //  32'b4000_0044
  BLIT_REG_S1_COLOR_FORMAT        = 0x000012 << 2         , //  32'b4000_0048
  BLIT_REG_S1_COLOR_KEY_MIN       = 0x000013 << 2         , //  32'b4000_004C
  BLIT_REG_S1_COLOR_KEY_MAX       = 0x000014 << 2         , //  32'b4000_0050
  BLIT_REG_S1_DEFAULT_COLOR       = 0x000015 << 2         , //  32'b4000_0054
  BLIT_REG_D_ADDR                 = 0x000016 << 2         , //  32'b4000_0058
  BLIT_REG_D_LINE_PITCH           = 0x000017 << 2         , //  32'b4000_005C
  BLIT_REG_D_SKIP_PIXEL           = 0x000018 << 2         , //  32'b4000_0060
  BLIT_REG_D_COLOR_FORMAT         = 0x000019 << 2         , //  32'b4000_0064
  BLIT_REG_PIXEL_NUM_PER_LINE     = 0x00001A << 2         , //  32'b4000_0068
  BLIT_REG_TOTAL_LINE_NUM         = 0x00001B << 2         , //  32'b4000_006C
  BLIT_REG_ENDIAN_CTRL            = 0x00001C << 2         , //  32'b4000_0070
  BLIT_REG_INT_CLEAR              = 0x00001D << 2         , //  32'b4000_0074
  BLIT_REG_SW_RESET               = 0x00001E << 2         , //  32'b4000_0078
	//Scalor Related Register
  BLIT_REG_S0_PIXEL_NUM_PER_LINE  = 0x000020 << 2         , //  32'b4000_0080
  BLIT_REG_S0_TOTAL_LINE_NUM      = 0x000021 << 2         , //  32'b4000_0084
  
  BLIT_REG_SCALOR_HORIZONTAL_INITIAL_PHASE
                                 = 0x000022 << 2         , //  32'b4000_0088
  BLIT_REG_SCALOR_VERTICAL_INITIAL_PHASE
                                 = 0x000023 << 2         , //  32'b4000_008C
#if SCALOR_FIR_COEFF_RED_REG_AVAILABLE                                 
  //For Orion 1.4 Version SCalor FIR Coefficient Read not available in HW GfxCore
  BLIT_REG_SCALER_HORIZONTAL_FIR_COEFFICIENT
                                 = 0x000024 << 2         , //  32'b4000_0090
  BLIT_REG_SCALER_VERTICAL_FIR_COEFFICIENT
                                 = 0x000025 << 2         , //  32'b4000_0094
#endif
}BLIT_REG_ADDR_DEF;

typedef enum _BLIT_REG_MASK_
{
	BLIT_COLOR_REG_MASK = ~((((1 << (CMD_ALPHA_WIDTH - BLIT_ALPHA_BIT_WIDTH)) - 1) << CMD_ALPHA_LSB) |
		                   (((1 << (CMD_RED_WIDTH   - BLIT_RED_BIT_WIDTH  )) - 1) << CMD_RED_LSB  ) |
		                   (((1 << (CMD_GREEN_WIDTH - BLIT_GREEN_BIT_WIDTH)) - 1) << CMD_GREEN_LSB) |
		                   (((1 << (CMD_BLUE_WIDTH  - BLIT_BLUE_BIT_WIDTH )) - 1) << CMD_BLUE_LSB ) ),
}BLIT_REG_MASk;


typedef enum _BLIT_SCALOR_CONST_
{
	SCALOR_H_FIR_TAP_NUM            = 4,//Max >=2
	SCALOR_V_FIR_TAP_NUM            = 2,
	SCALOR_COEFF_FRACTION_BIT_WIDTH = 10,//
	SCALOR_COEFF_INTREGER_BIT_WIDTH = 1,
	SCALOR_COEFF_POLARITY_BIT       = 15,
	
	SCALOR_PAHSE_BIT_WIDTH          = 3,
	SCALOR_PAHSE_NUM                = (1 << SCALOR_PAHSE_BIT_WIDTH),
	SCALOR_STEP_FRACTION_BIT_WIDTH  = 13,

	SCALOR_HFIR_DATA_BUF_LEN = SCALOR_H_FIR_TAP_NUM,
	
} BLIT_SCALOR_CONST;

typedef enum _BLIT_SCALOR_FIR_IDX_
{
	SCALOR_HORIZON_FIR  = 0,
	SCALOR_VERTICAL_FIR = 1,
	
}BLIT_SCALOR_FIR_IDX;

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif

