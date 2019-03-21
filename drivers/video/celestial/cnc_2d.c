#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <mach/hardware.h>


/* color format */
typedef enum
{
	GFX_CF_RGB565      = 0,
	GFX_CF_ARGB4444    = 1,
	GFX_CF_A0          = 2,
	GFX_CF_ARGB1555    = 3,
	GFX_CF_CLUT4       = 4,
	GFX_CF_CLUT8       = 5,
	GFX_CF_ARGB8888    = 6,

	GFX_CF_XRGB        = 7,
}GFX_COLOR_FORMAT;

/* RGB8888 */
typedef struct _GFX_ARGB_COLOR_
{
	u8  Alpha;
	u8  Red;
	u8  Green;
	u8  Blue;

}GFX_ARGB_COLOR;

/* ARGB8565 */
typedef enum 
{
	GFX_ALPHA_BIT_WIDTH = 8,
	GFX_RED_BIT_WIDTH   = 5,
	GFX_GREEN_BIT_WIDTH = 6,
	GFX_BLUE_BIT_WIDTH  = 5,
}GFX_COLOR_BIT_WIDTH;

/* color const */
typedef enum 
{
	CLUT4_TABLE_LEN  = 16,
	CLUT8_TABLE_LEN  = 256,
}CLUT_CONST;

typedef enum
{
	BYTE_BIG_ENDIAN    = 0,
	BYTE_LITTLE_ENDIAN = 1,

}GFX_MEM_BYTE_ENDIAN;

typedef enum
{
	NIBBLE_BIG_ENDIAN     = 0,
	NIBBLE_LITTLE_ENDIAN  = 1,

}GFX_MEM_NIBBLE_ENDIAN;

typedef enum
{
	TWO_BYTES_BIG_ENDIAN     = 0,
	TWO_BYTES_LITTLE_ENDIAN  = 1,

}GFX_16BIT_ENDIAN;


typedef enum
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
	CMD_PIXEL_NUM_PER_LINE = 0x18,
	CMD_TOTAL_LINE_NUM     = 0x19,    

	CMD_ENDIAN_CTRL        = 0x1A ,
	CMD_CLUT4_TAB          = 0x1C ,
	CMD_CLUT4_IDX          = 0x1D ,
	CMD_CLUT8_TAB          = 0x1E ,
	CMD_CLUT8_IDX          = 0x1F ,

	/* scalor related */
	CMD_SCALOR_S_PIXEL_NUM_PER_LINE = 0x30,
	CMD_SCALOR_S_TOTAL_LINE_NUM     = 0x31,
	CMD_SCALOR_INITIAL_PHASE        = 0x38,
	CMD_HFIR_COEFFICIETN_DATA       = 0x3B,
	CMD_VFIR_COEFFICIETN_DATA       = 0x3D,
	CMD_VALID_MAX,

}GFX_COMMAND_INDEX;

typedef enum
{
	GFX_STATUS_BIT     = 0,
	GFX_INT_STATUS_BIT = 1,
	GFX_ERR_BIT        = 2,
	GFX_AHB_ERR_BIT    = 3, 
}_GFX_REG_STATUS;

typedef enum 
{
	GFX_REG_CMDQUE_EMPTY_CNT_LSB   = 0,
	GFX_REG_CMDQUE_EMPTY_CNT_WIDTH = 9,	
}GFX_REG_CMDQUE_EMPTY_CNT_FORMAT;

typedef enum
{
	COMPOSITOR_ENABLE_BIT  = 3,
	ROP_ENABLE_BIT         = 3,
	S0_ON_TOP_S1_BIT       = 0,
}COMPOSITOR_OPT_FORMAT;

typedef enum
{
	S_ENABLE_BIT          = 3,
	S_COLORKEY_ENABLE_BIT = 2,
	S_CLUT_ENABLE_BIT     = 1,
	S_FETCH_DATA_BIT      = 0,	
}S_OPT_FORMAT;

typedef enum 
{
	S0_REVERSE_SCAN_BIT   = 0,
	S1_REVERSE_SCAN_BIT   = 1,
	D_REVERSE_SCAN_BIT    = 2,
}S_SCAN_CTRL_FORMAT;

typedef enum
{
	ENDIAN_ENABLE_BIT   =  3,
	TWO_BYTE_ENDIAN_BIT =  2,
	NIBBLE_ENDIAN_BIT   =  1,
	BYTE_ENDIAN_BIT     =  0,	
}ENDIAN_CTRL_FORMAT;

typedef enum
{
	REG_INFO_CMD_CNT_WIDTH   =  4,
	REG_INFO_CMD_CNT_LSB     = 28,

	REG_ENDIAN_CTRL_D_WIDTH  =  3,
	REG_ENDIAN_CTRL_D_LSB    =  0,
	REG_ENDIAN_CTRL_S1_WIDTH =  3,
	REG_ENDIAN_CTRL_S1_LSB   =  3,
	REG_ENDIAN_CTRL_S0_WIDTH =  3,
	REG_ENDIAN_CTRL_S0_LSB   =  6,		

}GFX_REG_FORMAT;

typedef enum
{
	/* all */
	CMD_IDX_WIDTH =  8,
	CMD_IDX_LSB   = 24, /* Start up Bit */

	/* CMD_STARTUP */
	SCAN_CTRL_WIDTH  =  4,
	SCAN_CTRL_LSB    = 20,
	S0_SCAL_ENA_BIT  = 17, 
	INT_ENABLE_WIDTH =  1,
	INT_ENABLE_LSB   = 16,
	ROP_ALPHA_CTRL_WIDTH = 2,
	ROP_ALPHA_CTRL_LSB   = 18,
	ROP_VAL_WIDTH    =  4,
	ROP_VAL_LSB      = 12,
	CMP_OPT_WIDTH    =  4, /* Compositor Operation */
	CMP_OPT_LSB      =  8,
	S1_OPT_WIDTH     =  4,
	S1_OPT_LSB       =  4,
	S0_OPT_WIDTH     =  4,
	S0_OPT_LSB       =  0, 

	/* CMD_XX_FORMAT  */
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


	/* CMD_XX_ADDR_INFO */
	CMD_SKIP_PIXEL_WIDTH =  1,
	CMD_SKIP_PIXEL_LSB   = 16,
	CMD_PITCH_WIDTH      = 16,
	CMD_PITCH_LSB        =  0,

	CMD_ALPHA_WIDTH     =  8,
	CMD_ALPHA_LSB       = 24,
	CMD_RED_WIDTH       =  8,
	CMD_RED_LSB         = 16,
	CMD_GREEN_WIDTH     =  8,
	CMD_GREEN_LSB       =  8,
	CMD_BLUE_WIDTH      =  8,
	CMD_BLUE_LSB        =  0,

	/* CMD_PIXEL_MAP */
	CMD_LPIXEL_NUM_WIDTH = 16,
	CMD_LPIXEL_NUM_LSB   = 0,
	CMD_LINE_NUM_WIDTH   = 16,
	CMD_LINE_NUM_LSB     =  0,

	/* CMD_CLUT4_IDX */
	CMD_CLUT4_IDX_WIDTH  =  4,
	CMD_CLUT4_IDX_LSB    =  0,

	/* CMD_CLUT8_IDX */
	CMD_CLUT8_IDX_WIDTH  =  8,
	CMD_CLUT8_IDX_LSB    =  0,

	/* CMD_ENDIAN_CTRL */
	CMD_GLOBAL_ENDIAN_WIDTH   =  3,
	CMD_GLOBAL_ENDIAN_LSB     =  0,

	CMD_S0_ENDIAN_WIDTH       =  4,
	CMD_S0_ENDIAN_LSB         =  4,

	CMD_S1_ENDIAN_WIDTH       =  4,
	CMD_S1_ENDIAN_LSB         =  8,

	CMD_D_ENDIAN_WIDTH        =  4,
	CMD_D_ENDIAN_LSB          = 12,

	/* CMD_SCALOR_INITIAL_PHASE */
	CMD_SCALOR_V_INIT_PHASE_WIDTH = 3,
	CMD_SCALOR_V_INIT_PHASE_LSB   = 8,
	CMD_SCALOR_H_INIT_PHASE_WIDTH = 3,
	CMD_SCALOR_H_INIT_PHASE_LSB   = 0,

	CMD_FIR_COEFF_PHASE_IDX_WIDTH =  3,
	CMD_FIR_COEFF_PHASE_IDX_LSB   = 20,
	CMD_FIR_COEFF_TAP_IDX_WIDTH   =  2,
	CMD_FIR_COEFF_TAP_IDX_LSB     = 16,
	CMD_FIR_COEFF_VALUE_WIDTH     = 16,
	CMD_FIR_COEFF_VALUE_LSB       =  0,
} GFX_CMD_FORMAT;


#define SCALOR_FIR_COEFF_RED_REG_AVAILABLE (0)

/* 2D-BLTer Register Map */
typedef enum
{	
	GFX_REG_CMD_QUE                = 0x000000,
	GFX_REG_STATUS                 = 0x000001,
	GFX_REG_CMDQUE_EMPTY_CNT       = 0x000002,
	GFX_REG_CMD_INFO               = 0x000003,
	GFX_REG_CLUT_IDX               = 0x000004,
	GFX_REG_CLUT_ENTRY             = 0x000005,
	GFX_REG_S0_ADDR                = 0x000008,
	GFX_REG_S0_LINE_PITCH          = 0x000009,
	GFX_REG_S0_SKIP_PIXEL          = 0x00000A,
	GFX_REG_S0_COLOR_FORMAT        = 0x00000B,
	GFX_REG_S0_COLOR_KEY_MIN       = 0x00000C,
	GFX_REG_S0_COLOR_KEY_MAX       = 0x00000D,
	GFX_REG_S0_DEFAULT_COLOR       = 0x00000E,
	GFX_REG_S1_ADDR                = 0x00000F,
	GFX_REG_S1_LINE_PITCH          = 0x000010,
	GFX_REG_S1_SKIP_PIXEL          = 0x000011,
	GFX_REG_S1_COLOR_FORMAT        = 0x000012,
	GFX_REG_S1_COLOR_KEY_MIN       = 0x000013,
	GFX_REG_S1_COLOR_KEY_MAX       = 0x000014,
	GFX_REG_S1_DEFAULT_COLOR       = 0x000015,
	GFX_REG_D_ADDR                 = 0x000016,
	GFX_REG_D_LINE_PITCH           = 0x000017,
	GFX_REG_D_SKIP_PIXEL           = 0x000018,
	GFX_REG_D_COLOR_FORMAT         = 0x000019,
	GFX_REG_PIXEL_NUM_PER_LINE     = 0x00001A,
	GFX_REG_TOTAL_LINE_NUM         = 0x00001B,
	GFX_REG_ENDIAN_CTRL            = 0x00001C,
	GFX_REG_INT_CLEAR              = 0x00001D,
	GFX_REG_SW_RESET               = 0x00001E,

	/* alor Related Register */
	GFX_REG_S0_PIXEL_NUM_PER_LINE  = 0x000020,
	GFX_REG_S0_TOTAL_LINE_NUM      = 0x000021,

	GFX_REG_SCALOR_HORIZONTAL_INITIAL_PHASE
		= 0x000022,
	GFX_REG_SCALOR_VERTICAL_INITIAL_PHASE
		= 0x000023,

#if SCALOR_FIR_COEFF_RED_REG_AVAILABLE                                 
	/*r Orion 1.4 Version SCalor FIR Coefficient Read not available in HW GfxCore */
	GFX_REG_SCALER_HORIZONTAL_FIR_COEFFICIENT
		= 0x000024,
	GFX_REG_SCALER_VERTICAL_FIR_COEFFICIENT
		= 0x000025,
#endif
} GFX_REG_ADDR_DEF;

typedef enum
{
	GFX_COLOR_REG_MASK = ~((((1 << (CMD_ALPHA_WIDTH - GFX_ALPHA_BIT_WIDTH)) - 1) << CMD_ALPHA_LSB) |
			(((1 << (CMD_RED_WIDTH   - GFX_RED_BIT_WIDTH  )) - 1) << CMD_RED_LSB  ) |
			(((1 << (CMD_GREEN_WIDTH - GFX_GREEN_BIT_WIDTH)) - 1) << CMD_GREEN_LSB) |
			(((1 << (CMD_BLUE_WIDTH  - GFX_BLUE_BIT_WIDTH )) - 1) << CMD_BLUE_LSB ) ),
}GFX_REG_MASk;

/* ROP Type Define */
typedef enum
{
	ROP_R2_BLACK       = 0  ,
	ROP_R2_NOTMERGEPEN = 1  ,
	ROP_R2_MASKNOTPEN  = 2  ,
	ROP_R2_NOTCOPYPEN  = 3  ,
	ROP_R2_MASKPENNOT  = 4  ,
	ROP_R2_NOT         = 5  ,
	ROP_R2_XORPEN      = 6  ,
	ROP_R2_NOTMASKPEN  = 7  ,
	ROP_R2_MASKPEN     = 8  ,
	ROP_R2_NOTXORPEN   = 9  ,
	ROP_R2_NOP         = 10 ,
	ROP_R2_MERGENOTPEN = 11 ,
	ROP_R2_COPYPEN     = 12 ,
	ROP_R2_MERGEPENNOT = 13 ,
	ROP_R2_MERGEPEN    = 14 ,
	ROP_R2_WHITE       = 15
}ROP_OPT;


typedef enum
{
	SCALOR_H_FIR_TAP_NUM            = 4,/* Max >=2 */
	SCALOR_V_FIR_TAP_NUM            = 2,
	SCALOR_COEFF_FRACTION_BIT_WIDTH = 10,
	SCALOR_COEFF_INTREGER_BIT_WIDTH = 1,
	SCALOR_COEFF_POLARITY_BIT       = 15,

	SCALOR_PAHSE_BIT_WIDTH          = 3,
	SCALOR_PAHSE_NUM                = (1 << SCALOR_PAHSE_BIT_WIDTH),
	SCALOR_STEP_FRACTION_BIT_WIDTH  = 13,

	SCALOR_HFIR_DATA_BUF_LEN = SCALOR_H_FIR_TAP_NUM,

} GFX_SCALOR_CONST;

typedef enum
{
	SCALOR_HORIZON_FIR  = 0,
	SCALOR_VERTICAL_FIR = 1,

}GFX_SCALOR_FIR_IDX;

/* 
 * the implementation of orionfb_2d.c 
 */
typedef struct _COLOR_IMG_
{
	u32 StartAddress;
	u32 PixelWidth; 
	u32 PixelHeight;
	u32 LinePitch;

	GFX_COLOR_FORMAT ColorFormat;
	GFX_ARGB_COLOR DefaultColor;

	u32 InitialType; /* 0 Stable, 1 Zero Increase, 2,Do not Initial */

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	GFX_MEM_BYTE_ENDIAN	ByteEndian;
	GFX_MEM_NIBBLE_ENDIAN NibbleEndian;
	GFX_16BIT_ENDIAN TwoBytesEndian;
#endif
	signed int IsMemAllocated;
}COLOR_IMG;

typedef struct tagRECT
{
	u32 left;
	u32 top;
	u32 right;
	u32 bottom;
} RECT;

typedef enum
{
	GFX_DC_S0_COLOR_FORMAT   = (1 << 0),
	GFX_DC_S0_DEFAULT_COLOR  = (1 << 1),
	GFX_DC_S0_COLORKEY_PARA  = (1 << 2),
	GFX_DC_S0_DRAM_PARA      = (1 << 3),

	GFX_DC_S1_COLOR_FORMAT   = (1 << 4),
	GFX_DC_S1_DEFAULT_COLOR  = (1 << 5),
	GFX_DC_S1_COLORKEY_PARA  = (1 << 6),
	GFX_DC_S1_DRAM_PARA      = (1 << 7),

	GFX_DC_D_COLOR_FORMAT    = (1 << 8),
	GFX_DC_D_DRAM_PARA       = (1 << 9),

	GFX_DC_CLUT4_TABLE       = (1 << 10),
	GFX_DC_CLUT8_TABLE       = (1 << 11),
	//Scalor
	GFX_DC_SCALOR_HFIR_COFF  = (1 << 12),
	GFX_DC_SCALOR_VFIR_COFF  = (1 << 13),
	GFX_DC_SCALOR_INIT_PHASE = (1 << 14),
	GFX_DC_CTRL              = (1 << 31),	
	GFX_DC_ALL               = (~0x0),

}GFX_DC_UPDATE_TYPE;

typedef struct _GFX_DEVICE_CONTEXT_
{
	/* Run Time Info */
	signed int GfxIdelStatus;
	signed int AHBErrStatus;
	signed int GfxErrStatus;
	signed int InterruptStatus;
	u32 CMDCnt;

	/* Clut Table: */
	signed int UpdateClut4Table;
	signed int UpdateClut4TableMode;
	u32 Clut4TableIdx;
	u32 Clut4Table[CLUT4_TABLE_LEN];
	signed int UpdateClut8Table;
	signed int UpdateClut8TableMode;
	u32 Clut8TableIdx;
	u32 Clut8Table[CLUT8_TABLE_LEN];

	/* RunTimeControl */
	signed int InterruptEnable;

	/* S0 Control */
	signed int S0Enable;
	signed int S0VReverseScan;
	signed int S0FetchDram;

	GFX_COLOR_FORMAT S0ColorFormat;
	u32 S0Alpha0;
	u32 S0Alpha1;
	GFX_ARGB_COLOR S0DefaultColor;
	u32 S0BaseAddr;
	u32 S0LinePitch;
	u32 S0SkipPixelLine;
	u32 S0PixelNumOneLine;
	u32 S0TotalLineNum;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	GFX_MEM_BYTE_ENDIAN S0ByteEndian;
	GFX_MEM_NIBBLE_ENDIAN S0NibbleEndian;
	GFX_16BIT_ENDIAN S016BitEndian;
#endif
	
	signed int S0ClutEnable;
	signed int S0ColorKeyEnable;
	GFX_ARGB_COLOR S0ColorKeyMin;
	GFX_ARGB_COLOR S0ColorKeyMax;

	/* S1 Control */
	signed int S1Enable;
	signed int S1VReverseScan;
	signed int S1FetchDram;
	GFX_COLOR_FORMAT S1ColorFormat;
	u32 S1Alpha0;
	u32 S1Alpha1;

	GFX_ARGB_COLOR S1DefaultColor;
	u32 S1BaseAddr;
	u32 S1LinePitch;
	u32 S1SkipPixelLine;
	u32 S1PixelNumOneLine;
	u32 S1TotalLineNum;	
	
#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)
    GFX_MEM_BYTE_ENDIAN S1ByteEndian;
	GFX_MEM_NIBBLE_ENDIAN S1NibbleEndian;
	GFX_16BIT_ENDIAN S116BitEndian;
#endif
	signed int S1ClutEnable;
	signed int S1ColorKeyEnable;
	GFX_ARGB_COLOR S1ColorKeyMin;
	GFX_ARGB_COLOR S1ColorKeyMax;

	/* ROP and Compositor */
	signed int CompositorEnable;
	u32 ROPAlphaCtrl;
	ROP_OPT RopValue;
	signed int S0OnTopS1;

	/* D Dram Para */
	signed int DVReverseScan;
	u32 DBaseAddr;
	u32 DLinePitch;
	u32 DSkipPixelLine;
	u32 DPixelNumOneLine;
	u32 DTotalLineNum;
	GFX_COLOR_FORMAT DColorFormat;
	u32 DAlpha0Min;
	u32 DAlpha0Max;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	GFX_MEM_BYTE_ENDIAN DByteEndian;
	GFX_MEM_NIBBLE_ENDIAN DNibbleEndian;	
	GFX_16BIT_ENDIAN D16BitEndian;

	/* Endian Ctrl */
	signed int IsUsingSDSpecificEndian;
#endif
	/* Scalor Control */
	u32 S0ScalorEnable;
	u32 HInitialPhase;
	u32 VInitialPhase;
	signed int UpdateHFIRCoeff;
	signed int UpdateVFIRCoeff;
	u32 HFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_H_FIR_TAP_NUM];
	u32 VFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_V_FIR_TAP_NUM];
	GFX_DC_UPDATE_TYPE GfxDCType;
}GFX_DEVICE_CONTEXT;

static u32 sGfxCMDGroupCnt = 0;
static u32 sGfxCMDQueueDepth = 0x100; /* 0x100 is a HW Reset Value means not initial */
static volatile u32 *sGfxBaseAddr = NULL;

static GFX_DEVICE_CONTEXT Gfx_DC;

static  u32 _GfxCLUT8Pal[CLUT8_TABLE_LEN] = 
{
	0xff000000,  //  000
	0xff000033,  //  001
	0xff000066,  //  002
	0xff000099,  //  003
	0xff0000cc,  //  004
	0xff0000ff,  //  005
	0xff003300,  //  006
	0xff003333,  //  007
	0xff003366,  //  008
	0xff003399,  //  009
	0xff0033cc,  //  010
	0xff0033ff,  //  011
	0xff006600,  //  012
	0xff006633,  //  013
	0xff006666,  //  014
	0xff006699,  //  015
	0xff0066cc,  //  016
	0xff0066ff,  //  017
	0xff009900,  //  018
	0xff009933,  //  019
	0xff009966,  //  020
	0xff009999,  //  021
	0xff0099cc,  //  022
	0xff0099ff,  //  023
	0xff00cc00,  //  024
	0xff00cc33,  //  025
	0xff00cc66,  //  026
	0xff00cc99,  //  027
	0xff00cccc,  //  028
	0xff00ccff,  //  029
	0xff00ff00,  //  030
	0xff00ff33,  //  031
	0xff00ff66,  //  032
	0xff00ff99,  //  033
	0xff00ffcc,  //  034
	0xff00ffff,  //  035
	0xff330000,  //  036
	0xff330033,  //  037
	0xff330066,  //  038
	0xff330099,  //  039
	0xff3300cc,  //  040
	0xff3300ff,  //  041
	0xff333300,  //  042
	0xff333333,  //  043
	0xff333366,  //  044
	0xff333399,  //  045
	0xff3333cc,  //  046
	0xff3333ff,  //  047
	0xff336600,  //  048
	0xff336633,  //  049
	0xff336666,  //  050
	0xff336699,  //  051
	0xff3366cc,  //  052
	0xff3366ff,  //  053
	0xff339900,  //  054
	0xff339933,  //  055
	0xff339966,  //  056
	0xff339999,  //  057
	0xff3399cc,  //  058
	0xff3399ff,  //  059
	0xff33cc00,  //  060
	0xff33cc33,  //  061
	0xff33cc66,  //  062
	0xff33cc99,  //  063
	0xff33cccc,  //  064
	0xff33ccff,  //  065
	0xff33ff00,  //  066
	0xff33ff33,  //  067
	0xff33ff66,  //  068
	0xff33ff99,  //  069
	0xff33ffcc,  //  070
	0xff33ffff,  //  071
	0xff660000,  //  072
	0xff660033,  //  073
	0xff660066,  //  074
	0xff660099,  //  075
	0xff6600cc,  //  076
	0xff6600ff,  //  077
	0xff663300,  //  078
	0xff663333,  //  079
	0xff663366,  //  080
	0xff663399,  //  081
	0xff6633cc,  //  082
	0xff6633ff,  //  083
	0xff666600,  //  084
	0xff666633,  //  085
	0xff666666,  //  086
	0xff666699,  //  087
	0xff6666cc,  //  088
	0xff6666ff,  //  089
	0xff669900,  //  090
	0xff669933,  //  091
	0xff669966,  //  092
	0xff669999,  //  093
	0xff6699cc,  //  094
	0xff6699ff,  //  095
	0xff66cc00,  //  096
	0xff66cc33,  //  097
	0xff66cc66,  //  098
	0xff66cc99,  //  099
	0xff66cccc,  //  100
	0xff66ccff,  //  101
	0xff66ff00,  //  102
	0xff66ff33,  //  103
	0xff66ff66,  //  104
	0xff66ff99,  //  105
	0xff66ffcc,  //  106
	0xff66ffff,  //  107
	0xff990000,  //  108
	0xff990033,  //  109
	0xff990066,  //  110
	0xff990099,  //  111
	0xff9900cc,  //  112
	0xff9900ff,  //  113
	0xff993300,  //  114
	0xff993333,  //  115
	0xff993366,  //  116
	0xff993399,  //  117
	0xff9933cc,  //  118
	0xff9933ff,  //  119
	0xff996600,  //  120
	0xff996633,  //  121
	0xff996666,  //  122
	0xff996699,  //  123
	0xff9966cc,  //  124
	0xff9966ff,  //  125
	0xff999900,  //  126
	0xff999933,  //  127
	0xff999966,  //  128
	0xff999999,  //  129
	0xff9999cc,  //  130
	0xff9999ff,  //  131
	0xff99cc00,  //  132
	0xff99cc33,  //  133
	0xff99cc66,  //  134
	0xff99cc99,  //  135
	0xff99cccc,  //  136
	0xff99ccff,  //  137
	0xff99ff00,  //  138
	0xff99ff33,  //  139
	0xff99ff66,  //  140
	0xff99ff99,  //  141
	0xff99ffcc,  //  142
	0xff99ffff,  //  143
	0xffcc0000,  //  144
	0xffcc0033,  //  145
	0xffcc0066,  //  146
	0xffcc0099,  //  147
	0xffcc00cc,  //  148
	0xffcc00ff,  //  149
	0xffcc3300,  //  150
	0xffcc3333,  //  151
	0xffcc3366,  //  152
	0xffcc3399,  //  153
	0xffcc33cc,  //  154
	0xffcc33ff,  //  155
	0xffcc6600,  //  156
	0xffcc6633,  //  157
	0xffcc6666,  //  158
	0xffcc6699,  //  159
	0xffcc66cc,  //  160
	0xffcc66ff,  //  161
	0xffcc9900,  //  162
	0xffcc9933,  //  163
	0xffcc9966,  //  164
	0xffcc9999,  //  165
	0xffcc99cc,  //  166
	0xffcc99ff,  //  167
	0xffcccc00,  //  168
	0xffcccc33,  //  169
	0xffcccc66,  //  170
	0xffcccc99,  //  171
	0xffcccccc,  //  172
	0xffccccff,  //  173
	0xffccff00,  //  174
	0xffccff33,  //  175
	0xffccff66,  //  176
	0xffccff99,  //  177
	0xffccffcc,  //  178
	0xffccffff,  //  179
	0xffff0000,  //  180
	0xffff0033,  //  181
	0xffff0066,  //  182
	0xffff0099,  //  183
	0xffff00cc,  //  184
	0xffff00ff,  //  185
	0xffff3300,  //  186
	0xffff3333,  //  187
	0xffff3366,  //  188
	0xffff3399,  //  189
	0xffff33cc,  //  190
	0xffff33ff,  //  191
	0xffff6600,  //  192
	0xffff6633,  //  193
	0xffff6666,  //  194
	0xffff6699,  //  195
	0xffff66cc,  //  196
	0xffff66ff,  //  197
	0xffff9900,  //  198
	0xffff9933,  //  199
	0xffff9966,  //  200
	0xffff9999,  //  201
	0xffff99cc,  //  202
	0xffff99ff,  //  203
	0xffffcc00,  //  204
	0xffffcc33,  //  205
	0xffffcc66,  //  206
	0xffffcc99,  //  207
	0xffffcccc,  //  208
	0xffffccff,  //  209
	0xffffff00,  //  210
	0xffffff33,  //  211
	0xffffff66,  //  212
	0xffffff99,  //  213
	0xffffffcc,  //  214
	0xffffffff,  //  215

	/*  from 2 o 248,  32 level grey scale */
	0xff000000,  //  216
	0xff080808,  //  217
	0xff101010,  //  218
	0xff181818,  //  219
	0xff202020,  //  220
	0xff282828,  //  221
	0xff303030,  //  222
	0xff383838,  //  223
	0xff404040,  //  224
	0xff484848,  //  225
	0xff505050,  //  226
	0xff585858,  //  227
	0xff606060,  //  228
	0xff686868,  //  229
	0xff707070,  //  230
	0xff787878,  //  231
	0xff808080,  //  232
	0xff888888,  //  233
	0xff909090,  //  234
	0xff989898,  //  235
	0xffa0a0a0,  //  236
	0xffa8a8a8,  //  237
	0xffb0b0b0,  //  238
	0xffb8b8b8,  //  239
	0xffc0c0c0,  //  240
	0xffc8c8c8,  //  241
	0xffd0d0d0,  //  242
	0xffd8d8d8,  //  243
	0xffe0e0e0,  //  244
	0xffe8e8e8,  //  245
	0xfff0f0f0,  //  246
	0xfff8f8f8,  //  247
	0xffffffff,  //  248

	/* from 24t 255, 7 color rainbow */
	0xff0000ff,  //  249
	0xff00ff00,  //  250
	0xff00ffff,  //  251
	0xffff0000,  //  252
	0xffff00ff,  //  253
	0xffffff00,  //  254
	0xffffffff,  //  255
};    

static inline void Gfx_WriteRegs(int offset, int val);
static inline u32 Gfx_ReadRegs(int offset);
static void Calc_DramPara(
		COLOR_IMG *DesImg, 
		RECT      *DesRect,	
		signed int        IsVReverse,
		u32       *pBaseAddr,
		u32       *pLinePitch,
		u32       *pSkipPixelLine);
static int Is_RectValid(COLOR_IMG *Img, RECT *Rect);
static signed int Gfx_ActiveDeviceContext(GFX_DEVICE_CONTEXT *hGfxDeviceContext, GFX_DC_UPDATE_TYPE GfxDCType);
static signed int Gfx_Send2DCmd(u32 GfxCMD);
static unsigned int Gfx_WaitForIdle(GFX_DEVICE_CONTEXT *hGfxDeviceContext);
static void Gfx_2DReset(void);
static void Gfx_SetCMDQueueDepth(u32 GfxCMDQueueDepth);
static void Gfx_CMDGroupCntSyncHW(void);
static int Gfx_SetDefaultScalorHFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC);
static u32 *Gfx_GetDefaultScalorHFIRCoeff(void);
static int Gfx_InitScalorHFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC, u32 *HFIRCoeff);
static int Gfx_SetDefaultScalorVFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC);
static u32 *Gfx_GetDefaultScalorVFIRCoeff(void);
static int Gfx_InitScalorVFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC, u32 *VFIRCoeff);
static u32 SCALOR_COEFF( u32 SignBit, u32 Nmerator, u32 denominator) ;
static u32 *Gfx_GetDefaultClut8Table(void);
static int Gfx_InitClut8Tab( GFX_DEVICE_CONTEXT * hGfxDC, u32 *Clut8Table);
static int Gfx_SetDefaultClut8Table(GFX_DEVICE_CONTEXT * hGfxDC);

int Gfx_2DInit(void);
static int Gfx_2DScalor( 
		GFX_DEVICE_CONTEXT  *hGfxDC, 
		COLOR_IMG           *SrcImg, 
		RECT                *SrcRect, 
		signed int           SrcVReverse,
		COLOR_IMG           *DesImg, 
		RECT                *DesRect, 
		signed int           DesVReverse,
		ROP_OPT              RopValue, 
		u32                  VInitialPhase, 
		u32                  HInitialPhase);

static inline u32 Gfx_ReadRegs(int offset)
{
	return *((volatile u32*)(sGfxBaseAddr + offset));
}

static inline void Gfx_WriteRegs(int offset, int val)
{
	*((volatile u32*)(sGfxBaseAddr + offset)) = (val);
	udelay(3);
}

static int Is_RectValid(COLOR_IMG *Img, RECT *Rect)
{
	if (Img == NULL || Rect == NULL) 
	{
		return 0;
	}
	else if (Rect->left   < 0 || Rect->left   > Img->PixelWidth  ||
			Rect->top    < 0 || Rect->top    > Img->PixelHeight ||
			Rect->right  < 0 || Rect->right  > Img->PixelWidth  ||
			Rect->bottom < 0 || Rect->bottom > Img->PixelHeight)
	{
		return 0;
	}
	else if (!(Rect->left  <= Rect->right) || 
			!(Rect->top   <= Rect->bottom))
	{
		return 0;
	}

	return 1;
}

static int CmpRect( RECT *Rect0, RECT *Rect1) 
{
	if (!(Rect0->left <= Rect0->right) || !(Rect0->top <= Rect0->bottom) )
	{
		return 0;
	}
	else if (!(Rect1->left <= Rect1->right) || !(Rect1->top <= Rect1->bottom) )
	{
		return 0;
	}
	else if ( ((Rect0->right - Rect0->left) != (Rect1->right - Rect1->left)) || 
			((Rect0->bottom - Rect0->top) != (Rect1->bottom - Rect1->top)) )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

static void Calc_DramPara(
		COLOR_IMG *DesImg, 
		RECT      *DesRect,	
		signed int        IsVReverse,
		u32       *pBaseAddr,
		u32       *pLinePitch,
		u32       *pSkipPixelLine)
{
	u32 PixelBitWidth = 0;
	u32 BaseAddr      = 0;
	u32 LinePitch     = 0;
	u32 SkipPixelLine = 0;

	switch (DesImg->ColorFormat)
	{
		case GFX_CF_XRGB:
		case GFX_CF_ARGB8888:
			PixelBitWidth = 32;
			break;

		case GFX_CF_RGB565:			
		case GFX_CF_ARGB4444:
		case GFX_CF_ARGB1555:
		case GFX_CF_A0:
			PixelBitWidth = 16;
			break;

		case GFX_CF_CLUT8:
			PixelBitWidth =  8;
			break;

		case GFX_CF_CLUT4:
			PixelBitWidth =  4;
			break;

		default:
			PixelBitWidth =  0;
			break;
	}

	if (PixelBitWidth == 0) return;

	/* if Clut4 and Width is odd, the Line width should be Bytes Aligned */
	LinePitch = DesImg->LinePitch;
	if (LinePitch < (DesImg->PixelWidth * PixelBitWidth + 8 - 1) / 8)
	{
		/* Warning, DesImg LinePitch(%d) < DesImg Width(%d) in Format(%d)\n */
	}

	if (!IsVReverse)
	{
		BaseAddr = DesImg->StartAddress + 
			DesRect->top * LinePitch +                
			(DesRect->left * PixelBitWidth) / 8;
	}
	else
	{
		u32 LineIdx = 0;
		LineIdx  = DesRect->bottom;

		if (LineIdx > 0) LineIdx--;

		BaseAddr = DesImg->StartAddress + 
			LineIdx* LinePitch +                
			(DesRect->left * PixelBitWidth) / 8;
	}

	SkipPixelLine   = ((DesRect->left * PixelBitWidth) % 8) / PixelBitWidth;
	*pBaseAddr      = BaseAddr;
	*pLinePitch     = LinePitch;
	*pSkipPixelLine = SkipPixelLine;

	return;
}

static void Gfx_CMDGroupCntSyncHW(void)
{
	sGfxCMDGroupCnt = (Gfx_ReadRegs( GFX_REG_CMD_INFO ) >> REG_INFO_CMD_CNT_LSB) & 
		((1 << REG_INFO_CMD_CNT_WIDTH) -1 );
}

static void Gfx_SetCMDQueueDepth(u32 GfxCMDQueueDepth)
{
	sGfxCMDQueueDepth = GfxCMDQueueDepth;
}

static void Gfx_2DReset(void)
{
	u32 GfxCMDQueueDepth = 0;

	Gfx_WriteRegs(GFX_REG_SW_RESET, 1);

	Gfx_WaitForIdle(NULL); /* To Ensure Reset is Stable, must Wait for the Idel */

	Gfx_WriteRegs(GFX_REG_SW_RESET, 0);

	GfxCMDQueueDepth = (Gfx_ReadRegs(GFX_REG_CMDQUE_EMPTY_CNT) 
			>> GFX_REG_CMDQUE_EMPTY_CNT_LSB) 
		& ((1 << GFX_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);

	Gfx_SetCMDQueueDepth(GfxCMDQueueDepth);

	Gfx_CMDGroupCntSyncHW();
}

static unsigned int Gfx_WaitForIdle(GFX_DEVICE_CONTEXT *hGfxDeviceContext)
{
	/* Run Time Info */
	signed int hResult = 1;
	signed int GfxIdelStatus   = 0;
	signed int AHBErrStatus    = 0;
	signed int GfxErrStatus    = 0;
	signed int InterruptStatus = 0; 
	u32 GfxCMDQueuSpace = 0;

	signed int InterruptEnable = 0;

	u32 TimeCnt = 0;
	u32 TimeOut = 0;

	u32 GfxStatusRegValue = 0;
	u32 GfxInfoRegValue   = 0;

	if (hGfxDeviceContext == NULL)
	{
		TimeOut = 0; /* no time out */
	}
	else
	{
		u32 MaxWidth  = 0;
		u32 MaxHeight = 0;

		MaxWidth  = hGfxDeviceContext->S0PixelNumOneLine > hGfxDeviceContext->S1PixelNumOneLine ?
			hGfxDeviceContext->S0PixelNumOneLine : hGfxDeviceContext->S1PixelNumOneLine ;
		MaxWidth  = MaxWidth > hGfxDeviceContext->DPixelNumOneLine ?
			MaxWidth : hGfxDeviceContext->DPixelNumOneLine ;
		MaxHeight = hGfxDeviceContext->S0TotalLineNum > hGfxDeviceContext->S1TotalLineNum ?
			hGfxDeviceContext->S0TotalLineNum : hGfxDeviceContext->S1TotalLineNum ;
		MaxHeight = MaxHeight > hGfxDeviceContext->DTotalLineNum ?
			MaxHeight : hGfxDeviceContext->DTotalLineNum ;

		TimeOut = MaxWidth * MaxHeight *10; /* One Circle One Pixel, for Scalor 3 Circle/pixel, so *10 is Enough */
		if (TimeOut < 100) TimeOut = 100;
	}

	do 
	{
		GfxInfoRegValue   = Gfx_ReadRegs(GFX_REG_CMD_INFO);
		InterruptEnable   = (GfxInfoRegValue >> INT_ENABLE_LSB) &((1 << INT_ENABLE_WIDTH) -1); 

		GfxStatusRegValue = Gfx_ReadRegs(GFX_REG_STATUS);
		GfxIdelStatus     = (GfxStatusRegValue >> GFX_STATUS_BIT ) &(1);
		AHBErrStatus      = (GfxStatusRegValue >> GFX_AHB_ERR_BIT) &(1);
		GfxErrStatus      = (GfxStatusRegValue >> GFX_ERR_BIT    ) &(1);
		InterruptStatus   = (GfxStatusRegValue >> GFX_INT_STATUS_BIT) &(1);
		GfxCMDQueuSpace   = (Gfx_ReadRegs(GFX_REG_CMDQUE_EMPTY_CNT) 
				>> GFX_REG_CMDQUE_EMPTY_CNT_LSB) 
			& ((1 << GFX_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
		if (AHBErrStatus) 
		{
			Gfx_2DReset();
			continue;
		}

		if (GfxErrStatus)
		{
			hResult = 0;
		}

		if (InterruptEnable || GfxErrStatus)
		{
			if (InterruptStatus == 1)
			{
				/* Clear Interrupt */
				Gfx_WriteRegs(GFX_REG_INT_CLEAR, 1); /* Write Any Value */
			}
		}
		TimeCnt++;
	}while(!GfxIdelStatus && (TimeCnt < TimeOut || TimeOut == 0));

	if(hGfxDeviceContext != NULL)
	{
		hGfxDeviceContext->GfxIdelStatus   = GfxIdelStatus;
		hGfxDeviceContext->AHBErrStatus    = AHBErrStatus;
		hGfxDeviceContext->GfxErrStatus    = GfxErrStatus;
		hGfxDeviceContext->InterruptStatus = InterruptStatus; 
		hGfxDeviceContext->CMDCnt          = GfxCMDQueuSpace;
	}
	if (TimeCnt >= TimeOut && TimeOut > 0)
	{
		int i = 0;
		Gfx_WriteRegs(GFX_REG_SW_RESET, 1);
		for (i =0; i < 10; i++)
		{
			GfxStatusRegValue = Gfx_ReadRegs(GFX_REG_STATUS);
			GfxIdelStatus     = (GfxStatusRegValue >> GFX_STATUS_BIT ) &(1);		
			if (GfxIdelStatus)
			{
				break;
			}
		}		
		AHBErrStatus      = (GfxStatusRegValue >> GFX_AHB_ERR_BIT) &(1);
		GfxErrStatus      = (GfxStatusRegValue >> GFX_ERR_BIT    ) &(1);
		InterruptStatus   = (GfxStatusRegValue >> GFX_INT_STATUS_BIT) &(1);
		if (!GfxIdelStatus || AHBErrStatus || GfxErrStatus || InterruptStatus)
		{
		}
		Gfx_WriteRegs(GFX_REG_SW_RESET, 0);

		Gfx_CMDGroupCntSyncHW();

		hResult = 0;
	}
	else
	{
		/* do nothing here... */
	}
	if(AHBErrStatus)
	{
		hResult = 0;
	}
	if (GfxErrStatus)
	{
		hResult = 0;
	}

	return hResult;
}

static signed int Gfx_Send2DCmd(u32 GfxCMD)
{
	/* Check Available Space */
	u32 GfxCMDQueuSpace = 0;

	do
	{
		GfxCMDQueuSpace = (Gfx_ReadRegs(GFX_REG_CMDQUE_EMPTY_CNT) >> GFX_REG_CMDQUE_EMPTY_CNT_LSB) &
			((1 << GFX_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
		if (GfxCMDQueuSpace <= 0)
		{
			u32 GfxStatusRegValue = 0;
			signed int AHBErrStatus    = 0;
			signed int GfxErrStatus    = 0;
			signed int InterruptStatus = 0; 
			signed int InterruptEnable = 0;

			/* Clear Interrupt and ERR */
			GfxStatusRegValue = Gfx_ReadRegs(GFX_REG_STATUS);
			AHBErrStatus = (GfxStatusRegValue >> GFX_AHB_ERR_BIT) &(1);
			GfxErrStatus = (GfxStatusRegValue >> GFX_ERR_BIT) &(1);
			InterruptStatus = (GfxStatusRegValue >> GFX_INT_STATUS_BIT) &(1);

			if (AHBErrStatus) 
			{
				Gfx_2DReset();
				continue;
			}

			if (GfxErrStatus)
			{
				/* ... ... */
			}

			if (InterruptEnable || GfxErrStatus)
			{
				if (InterruptStatus == 1)
				{
					Gfx_WriteRegs(GFX_REG_INT_CLEAR, 1); /* Write Any Value */
				}
			}
		}

		schedule();
		if(0){
			int i = 0;
			for(i = 0; i < 10; i++){
				udelay(1000);
			}
		}
	}while(GfxCMDQueuSpace <= 0);

	if (GfxCMDQueuSpace > 0)
	{
		Gfx_WriteRegs(GFX_REG_CMD_QUE, GfxCMD);
	}

	return GfxCMDQueuSpace;
}

static signed int Gfx_ActiveDeviceContext(GFX_DEVICE_CONTEXT *hGfxDeviceContext, GFX_DC_UPDATE_TYPE GfxDCType)
{
	/* Send S0 Related Command */
	u32 GfxCMD   = 0;
	u32 GfxCMDQueSpace = 0;
	u32 CompositorOperation = 0;

	u32 S0Operation     = 0;
	u32 S1Operation     = 0;
	u32 ScanCtrl        = 0;
#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	u32 S0EndianCtrl    = 0;
	u32 S1EndianCtrl    = 0;
	u32 DEndianCtrl     = 0;
	u32 GlobalEndianCtrl  = 0;
#endif
	if (hGfxDeviceContext == NULL)
	{
		return -1;
	}
	else  /* Check Command Queue */
	{
		/* Check Available Space */
		u32 GfxCMDQueuSpace = 0;
		u32 GfxStatusRegValue = 0;
		signed int AHBErrStatus    = 0;
		signed int GfxErrStatus    = 0;
		signed int InterruptStatus = 0; 
		signed int InterruptEnable = 0;

		do
		{
			/* Clear Interrupt and ERR */
			GfxStatusRegValue = Gfx_ReadRegs(GFX_REG_STATUS);
			AHBErrStatus      = (GfxStatusRegValue >> GFX_AHB_ERR_BIT) &(1);
			GfxErrStatus      = (GfxStatusRegValue >> GFX_ERR_BIT) &(1);
			InterruptStatus   = (GfxStatusRegValue >> GFX_INT_STATUS_BIT) &(1);

			if (AHBErrStatus) 
			{
				Gfx_2DReset();
				continue;
			}

			if (InterruptEnable || GfxErrStatus)
			{
				if (InterruptStatus == 1)
				{
					/* Clear Interrupt */
					signed int IsGfxGenINTToHost = 0;

					IsGfxGenINTToHost = 1; /* FIXME@zhongkai's code */

					Gfx_WriteRegs(GFX_REG_INT_CLEAR, 1); /* Write Any Value */
				}
			}

			GfxCMDQueuSpace = (Gfx_ReadRegs(GFX_REG_CMDQUE_EMPTY_CNT) >> 
					GFX_REG_CMDQUE_EMPTY_CNT_LSB) &
				((1 << GFX_REG_CMDQUE_EMPTY_CNT_WIDTH) - 1);
			if (GfxCMDQueuSpace < 32)
			{
				/* Gfx Command Queue Space Less than 32, Waiting... */
				schedule();
			}		
		}while(GfxCMDQueuSpace < 32);
	}

	if (!Gfx_WaitForIdle(NULL)) return -1;

	if (hGfxDeviceContext->S0Enable || hGfxDeviceContext->S0ScalorEnable)
	{
		if ((hGfxDeviceContext->S0FetchDram || hGfxDeviceContext->S0ScalorEnable)&& 
				(GfxDCType & GFX_DC_S0_DRAM_PARA))
		{			
			/* Dram Access Para */
			GfxCMD = ((CMD_S0_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S0SkipPixelLine & 
				  ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
				((hGfxDeviceContext->S0LinePitch & 
				  ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->S0BaseAddr);
#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

			if (hGfxDeviceContext->IsUsingSDSpecificEndian)
			{				
				S0EndianCtrl = ((hGfxDeviceContext->S0ByteEndian   & 0x1) << BYTE_ENDIAN_BIT  )|
					((hGfxDeviceContext->S0NibbleEndian & 0x1) << NIBBLE_ENDIAN_BIT)|
					((hGfxDeviceContext->S016BitEndian  & 0x1) << TWO_BYTE_ENDIAN_BIT)|
					((1 & 0x1) << ENDIAN_ENABLE_BIT);
			}
#endif
			GfxCMD = ((CMD_SCALOR_S_PIXEL_NUM_PER_LINE & 
						((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S0PixelNumOneLine & 
				  ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			GfxCMD = ((CMD_SCALOR_S_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S0TotalLineNum & ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
		}

		/* Color Info */
		if (GfxDCType & (GFX_DC_S0_COLOR_FORMAT))
		{
			if (GfxDCType & GFX_DC_S0_DEFAULT_COLOR)
			{
				u8 Alpha = 0;

				if (hGfxDeviceContext->S0ColorFormat != GFX_CF_ARGB1555)
				{
					GfxCMD = ((CMD_S0_FORMAT_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S0ColorFormat &
						  ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					hGfxDeviceContext->S0Alpha0 = 0;
					hGfxDeviceContext->S0Alpha1 = 0xff;
					GfxCMD = ((CMD_S0_FORMAT_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S0ColorFormat & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
						((hGfxDeviceContext->S0Alpha0 & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						((hGfxDeviceContext->S0Alpha1 & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

				Alpha = hGfxDeviceContext->S0DefaultColor.Alpha;

				GfxCMD = (((Alpha >> (8-8)) & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
					(((hGfxDeviceContext->S0DefaultColor.Red >> (8-8)) &
					  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB)|
					(((hGfxDeviceContext->S0DefaultColor.Green >> (8-8)) &
					  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
					(((hGfxDeviceContext->S0DefaultColor.Blue >> (8-8)) &
					  ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			}
			else
			{
				if (hGfxDeviceContext->S0ColorFormat != GFX_CF_ARGB1555)
				{
					GfxCMD = ((CMD_S0_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S0ColorFormat & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					GfxCMD = ((CMD_S0_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S0ColorFormat & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
						((hGfxDeviceContext->S0Alpha0 & ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						((hGfxDeviceContext->S0Alpha1 & ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			}
		}

		/* ColorKey ARGB8888 */
		if (hGfxDeviceContext->S0ColorKeyEnable && (GfxDCType & GFX_DC_S0_COLORKEY_PARA))
		{
			GfxCMD = ((CMD_S0_COLOR_KEY_MIN & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S0ColorKeyMin.Red & 
				  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB) |
				((hGfxDeviceContext->S0ColorKeyMin.Green &
				  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				((hGfxDeviceContext->S0ColorKeyMin.Blue &
				  ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			GfxCMD = ((CMD_S0_COLOR_KEY_MAX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S0ColorKeyMax.Red &
				  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB) |
				((hGfxDeviceContext->S0ColorKeyMax.Green &
				  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				((hGfxDeviceContext->S0ColorKeyMax.Blue & 
				  ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);		
		}		
	}

	/* Send S1 Related Command */
	if (hGfxDeviceContext->S1Enable)
	{
		if (hGfxDeviceContext->S1FetchDram && (GfxDCType & GFX_DC_S1_DRAM_PARA))
		{

			/* Dram Access Para */
			GfxCMD = ((CMD_S1_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S1SkipPixelLine &
				  ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
				((hGfxDeviceContext->S1LinePitch & 
				  ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->S1BaseAddr);

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

			if (hGfxDeviceContext->IsUsingSDSpecificEndian)
			{				
				S1EndianCtrl = ((hGfxDeviceContext->S1ByteEndian & 0x1) << BYTE_ENDIAN_BIT  )|
					((hGfxDeviceContext->S1NibbleEndian & 0x1) << NIBBLE_ENDIAN_BIT)|
					((hGfxDeviceContext->S116BitEndian & 0x1) << TWO_BYTE_ENDIAN_BIT)|
					((1 & 0x1) << ENDIAN_ENABLE_BIT);
			}
#endif
			GfxCMD = ((CMD_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S1PixelNumOneLine &
				  ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			GfxCMD = ((CMD_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S1TotalLineNum &
				  ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
		}

		/* Color Info */
		if (GfxDCType & GFX_DC_S1_COLOR_FORMAT)
		{
			if (GfxDCType & GFX_DC_S1_DEFAULT_COLOR)
			{
				u8 Alpha;

				if (hGfxDeviceContext->S1ColorFormat != GFX_CF_ARGB1555)
				{
					GfxCMD = ((CMD_S1_FORMAT_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S1ColorFormat &
						  ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					hGfxDeviceContext->S1Alpha0 = 0;
					hGfxDeviceContext->S1Alpha1 = 0xff;
					GfxCMD = ((CMD_S1_FORMAT_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S1ColorFormat &
						  ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB) |
						((hGfxDeviceContext->S1Alpha0 &
						  ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						((hGfxDeviceContext->S1Alpha1 &
						  ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}

				Alpha = hGfxDeviceContext->S1DefaultColor.Alpha;

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
				GfxCMD = (((Alpha >> (8-8)) & ((1 << CMD_ALPHA_WIDTH) -1)) << CMD_ALPHA_LSB)|
					(((hGfxDeviceContext->S1DefaultColor.Red >> (8-8)) &
					  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB) |
					(((hGfxDeviceContext->S1DefaultColor.Green >> (8-8)) &
					  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB) |
					(((hGfxDeviceContext->S1DefaultColor.Blue >> (8-8)) &
					  ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			}
			else
			{
				if (hGfxDeviceContext->S1ColorFormat != GFX_CF_ARGB1555)
				{
					GfxCMD = ((CMD_S1_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S1ColorFormat & 
						  ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
				}
				else
				{
					GfxCMD = ((CMD_S1_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
						((hGfxDeviceContext->S1ColorFormat &
						  ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
						((hGfxDeviceContext->S1Alpha0 &
						  ((1 << CMD_FORMAT_ALPHA0_WIDTH) -1)) << CMD_FORMAT_ALPHA0_LSB)|
						((hGfxDeviceContext->S1Alpha1 &
						  ((1 << CMD_FORMAT_ALPHA1_WIDTH) -1)) << CMD_FORMAT_ALPHA1_LSB);
				}

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);				
			}
		}

		//ColorKey
		if (hGfxDeviceContext->S1ColorKeyEnable && (GfxDCType & GFX_DC_S1_COLORKEY_PARA))
		{
			GfxCMD = ((CMD_S1_COLOR_KEY_MIN & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S1ColorKeyMin.Red &
				  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB) |
				((hGfxDeviceContext->S1ColorKeyMin.Green &
				  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB) |
				((hGfxDeviceContext->S1ColorKeyMin.Blue &
				  ((1 << CMD_BLUE_WIDTH) -1)) << CMD_BLUE_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			GfxCMD = ((CMD_S1_COLOR_KEY_MAX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->S1ColorKeyMax.Red &
				  ((1 << CMD_RED_WIDTH) -1)) << CMD_RED_LSB) |
				((hGfxDeviceContext->S1ColorKeyMax.Green &
				  ((1 << CMD_GREEN_WIDTH) -1)) << CMD_GREEN_LSB)|
				((hGfxDeviceContext->S1ColorKeyMax.Blue &
				  ((1 << CMD_BLUE_WIDTH) -1))  << CMD_BLUE_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);		
		}		
	}

	/* Clut Table Update */
	/* Clut4 */
	if (hGfxDeviceContext->UpdateClut4Table && (GfxDCType & GFX_DC_CLUT4_TABLE))
	{
		if (hGfxDeviceContext->UpdateClut4TableMode)
		{
			u32 Clut4TableIdx = 0;

			Clut4TableIdx = (hGfxDeviceContext->Clut4TableIdx & ((1 << CMD_CLUT4_IDX_WIDTH) -1));
			GfxCMD =((CMD_CLUT4_IDX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) | 
				( Clut4TableIdx << CMD_CLUT4_IDX_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			/* Send Clut4 Entry */
			GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->Clut4Table[Clut4TableIdx]);
		}
		else
		{
			u32 i = 0;

			GfxCMD =((CMD_CLUT4_TAB & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB);
			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			for (i = 0; i < CLUT4_TABLE_LEN; i++)
			{
				GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->Clut4Table[i]);
			}
		}

		/* add something here, to check clut4 table registers */
	}

	/* Clut8 */
	if (hGfxDeviceContext->UpdateClut8Table && (GfxDCType & GFX_DC_CLUT8_TABLE))
	{
		if (hGfxDeviceContext->UpdateClut8TableMode)
		{
			u32 Clut8TableIdx = 0;

			Clut8TableIdx = (hGfxDeviceContext->Clut8TableIdx & ((1 << CMD_CLUT8_IDX_WIDTH) -1));

			GfxCMD =((CMD_CLUT8_IDX & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) | 
				( Clut8TableIdx << CMD_CLUT8_IDX_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			/* Send Clut8 Entry */
			GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->Clut8Table[Clut8TableIdx]);
		}
		else
		{
			u32 i = 0;

			GfxCMD =((CMD_CLUT8_TAB & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB);
			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

			for (i = 0; i < CLUT8_TABLE_LEN; i++)
			{
				GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->Clut8Table[i]);
			}
		}

		/* do something here, to check clut8 table registers */
	}

	/* Send D Related Command */

	/* D Dram Para */
	if ((GfxDCType & GFX_DC_D_DRAM_PARA))
	{
		GfxCMD = ((CMD_D_ADDR_INFO_L & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
			((hGfxDeviceContext->DSkipPixelLine & 
			  ((1 << CMD_SKIP_PIXEL_WIDTH) -1)) << CMD_SKIP_PIXEL_LSB) |
			((hGfxDeviceContext->DLinePitch &
			  ((1 << CMD_PITCH_WIDTH) -1)) << CMD_PITCH_LSB);

		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
		GfxCMDQueSpace = Gfx_Send2DCmd(hGfxDeviceContext->DBaseAddr);

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)
	if (hGfxDeviceContext->IsUsingSDSpecificEndian)
		{				
			DEndianCtrl = ((hGfxDeviceContext->DByteEndian   & 0x1)    << BYTE_ENDIAN_BIT  )|
				((hGfxDeviceContext->DNibbleEndian & 0x1)    << NIBBLE_ENDIAN_BIT)|
				((hGfxDeviceContext->D16BitEndian  & 0x1)    << TWO_BYTE_ENDIAN_BIT)|
				((1 & 0x1) << ENDIAN_ENABLE_BIT);
		}
#endif
		GfxCMD = ((CMD_PIXEL_NUM_PER_LINE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
			((hGfxDeviceContext->DPixelNumOneLine &
			  ((1 << CMD_LPIXEL_NUM_WIDTH) -1)) << CMD_LPIXEL_NUM_LSB);
		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);

		GfxCMD = ((CMD_TOTAL_LINE_NUM & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
			((hGfxDeviceContext->DTotalLineNum &
			  ((1 << CMD_LINE_NUM_WIDTH) -1)) << CMD_LINE_NUM_LSB) ;
		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
	}

	/* D Color Info should not be A0, no Default Color */
	if ((GfxDCType & GFX_DC_D_COLOR_FORMAT))
	{
		if (hGfxDeviceContext->DColorFormat != GFX_CF_ARGB1555)
		{
			GfxCMD = ((CMD_D_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->DColorFormat & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB);
		}
		else
		{
			hGfxDeviceContext->DAlpha0Min = 0;
			hGfxDeviceContext->DAlpha0Max = 0;
			GfxCMD = ((CMD_D_FORMAT & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->DColorFormat & ((1 << CMD_FORMAT_WIDTH) -1)) << CMD_FORMAT_LSB)|
				((hGfxDeviceContext->DAlpha0Min & ((1 << CMD_FORMAT_ALPHA0_MIN_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MIN_LSB)|
				((hGfxDeviceContext->DAlpha0Max & ((1 << CMD_FORMAT_ALPHA0_MAX_WIDTH) -1)) << CMD_FORMAT_ALPHA0_MAX_LSB);
		}	
		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
	}

	/* Set Gfx Interrupt Mode and Pipeline Enable */
	/* Send Scalor Command */
	/* Update VFIR Coeff */
	if ( (hGfxDeviceContext->UpdateHFIRCoeff) && (GfxDCType & GFX_DC_SCALOR_HFIR_COFF))
	{
		u32 Phase = 0, Tap = 0;

		for (Phase = 0; Phase < SCALOR_PAHSE_NUM; Phase++)
		{
			for (Tap =0; Tap < SCALOR_H_FIR_TAP_NUM; Tap++)
			{
				GfxCMD = ((CMD_HFIR_COEFFICIETN_DATA & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((Phase & ((1 << CMD_FIR_COEFF_PHASE_IDX_WIDTH) -1)) << CMD_FIR_COEFF_PHASE_IDX_LSB)|
					((Tap & ((1 << CMD_FIR_COEFF_TAP_IDX_WIDTH) -1)) << CMD_FIR_COEFF_TAP_IDX_LSB)|
					((hGfxDeviceContext->HFIRCoeffTable[Phase][Tap] & 
					  ((1 << CMD_FIR_COEFF_VALUE_WIDTH) -1)) << CMD_FIR_COEFF_VALUE_LSB);

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			}
		}
	}

	/* Update HFIR Coeff */
	if ((hGfxDeviceContext->UpdateVFIRCoeff) && (GfxDCType & GFX_DC_SCALOR_VFIR_COFF))
	{
		u32 Phase = 0;
		u32 Tap   = 0;
		for (Phase = 0; Phase < SCALOR_PAHSE_NUM; Phase++)
		{
			for (Tap =0; Tap < SCALOR_V_FIR_TAP_NUM; Tap++)
			{
				GfxCMD = ((CMD_VFIR_COEFFICIETN_DATA & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
					((Phase & ((1 << CMD_FIR_COEFF_PHASE_IDX_WIDTH) -1)) << CMD_FIR_COEFF_PHASE_IDX_LSB)|
					((Tap & ((1 << CMD_FIR_COEFF_TAP_IDX_WIDTH) -1)) << CMD_FIR_COEFF_TAP_IDX_LSB)|
					((hGfxDeviceContext->VFIRCoeffTable[Phase][Tap] &
					  ((1 << CMD_FIR_COEFF_VALUE_WIDTH) -1)) << CMD_FIR_COEFF_VALUE_LSB);

				GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
			}
		}
	}

	if (hGfxDeviceContext->S0ScalorEnable)
	{
		/* Scalor Source Pixel Map */
		/* Send InitialPhase */
		if ((GfxDCType & GFX_DC_SCALOR_INIT_PHASE))
		{
			GfxCMD = ((CMD_SCALOR_INITIAL_PHASE & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
				((hGfxDeviceContext->HInitialPhase & ((1 << CMD_SCALOR_H_INIT_PHASE_WIDTH) -1)) << CMD_SCALOR_H_INIT_PHASE_LSB)|
				((hGfxDeviceContext->VInitialPhase & ((1 << CMD_SCALOR_V_INIT_PHASE_WIDTH) -1)) << CMD_SCALOR_V_INIT_PHASE_LSB);

			GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
		}
	}

	/* Send Startup Command */
	if ((GfxDCType & GFX_DC_CTRL))
	{
#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

		//Send Endian Mode		
		GlobalEndianCtrl = ((hGfxDeviceContext->DByteEndian   & 0x1) << BYTE_ENDIAN_BIT  )|
			((hGfxDeviceContext->DNibbleEndian & 0x1) << NIBBLE_ENDIAN_BIT)|
			((hGfxDeviceContext->D16BitEndian  & 0x1) << TWO_BYTE_ENDIAN_BIT);

		GfxCMD = ((CMD_ENDIAN_CTRL & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB)|
			((S0EndianCtrl & ((1 << CMD_S0_ENDIAN_WIDTH) -1)) << CMD_S0_ENDIAN_LSB)|
			((S1EndianCtrl & ((1 << CMD_S1_ENDIAN_WIDTH) -1)) << CMD_S1_ENDIAN_LSB)|
			((DEndianCtrl & ((1 << CMD_D_ENDIAN_WIDTH) -1)) << CMD_D_ENDIAN_LSB )|
			((GlobalEndianCtrl & ((1 << CMD_GLOBAL_ENDIAN_WIDTH) -1)) << CMD_GLOBAL_ENDIAN_LSB); 

		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
#endif
		/* Send Ctrl and Startup Cmd */
		ScanCtrl = ((hGfxDeviceContext->S0VReverseScan & (1)) << S0_REVERSE_SCAN_BIT) |
			((hGfxDeviceContext->S1VReverseScan & (1)) << S1_REVERSE_SCAN_BIT)  |
			((hGfxDeviceContext->DVReverseScan & (1)) << D_REVERSE_SCAN_BIT);

		CompositorOperation = ((hGfxDeviceContext->CompositorEnable & (1)) << COMPOSITOR_ENABLE_BIT)|
			((hGfxDeviceContext->S0OnTopS1 & (1)) << S0_ON_TOP_S1_BIT);

		S1Operation = ((hGfxDeviceContext->S1Enable & (1)) << S_ENABLE_BIT)|
			((hGfxDeviceContext->S1ColorKeyEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
			((hGfxDeviceContext->S1ClutEnable & (1)) << S_CLUT_ENABLE_BIT) |
			((hGfxDeviceContext->S1FetchDram & (1)) << S_FETCH_DATA_BIT);

		S0Operation = ((hGfxDeviceContext->S0Enable & (1)) << S_ENABLE_BIT) |
			((hGfxDeviceContext->S0ColorKeyEnable & (1)) << S_COLORKEY_ENABLE_BIT)|
			((hGfxDeviceContext->S0ClutEnable & (1)) << S_CLUT_ENABLE_BIT)    |
			((hGfxDeviceContext->S0FetchDram  & (1)) << S_FETCH_DATA_BIT);

		GfxCMD = ((CMD_STARTUP  & ((1 << CMD_IDX_WIDTH) -1)) << CMD_IDX_LSB) |
			((ScanCtrl & ((1 << SCAN_CTRL_WIDTH) -1)) << SCAN_CTRL_LSB) |
			((hGfxDeviceContext->S0ScalorEnable & (0x1)) << S0_SCAL_ENA_BIT) |
			((hGfxDeviceContext->InterruptEnable & ((1 << INT_ENABLE_WIDTH) -1)) << INT_ENABLE_LSB) |
			((hGfxDeviceContext->ROPAlphaCtrl & ((1 << ROP_ALPHA_CTRL_WIDTH) -1)) << ROP_ALPHA_CTRL_LSB) |
			((hGfxDeviceContext->RopValue & ((1 << ROP_VAL_WIDTH) -1)) << ROP_VAL_LSB) |
			((CompositorOperation & ((1 << CMP_OPT_WIDTH) -1)) << CMP_OPT_LSB) |
			((S1Operation  & ((1 << S1_OPT_WIDTH) -1)) << S1_OPT_LSB) |
			((S0Operation  & ((1 << S0_OPT_WIDTH) -1)) << S0_OPT_LSB);

		GfxCMDQueSpace = Gfx_Send2DCmd(GfxCMD);
	}

	hGfxDeviceContext->GfxDCType = GfxDCType;

	return 0;
}

static int Gfx_2DComposite
(
    GFX_DEVICE_CONTEXT  	*hGfxDC, 
    COLOR_IMG           	*Src0Img, 
    RECT                	*Src0Rect, 
    COLOR_IMG           	*Src1Img, 
    RECT                	*Src1Rect, 
    COLOR_IMG           	*DesImg, 
    RECT                	*DesRect, 
    ROP_OPT              	RopValue, 
    u32 			BlendEnable, 
    u32 			IsS0OnTopS1, 	
    u32 			ROPAlphaCtrl
)
{
    if (hGfxDC == NULL || Src0Img == NULL || Src1Img == NULL || \
	    Src0Rect == NULL || Src1Rect == NULL || DesImg == NULL)
    {
	return -1;
    }
    else if ( ((Src0Img->ColorFormat != GFX_CF_ARGB1555) &&
		(Src0Img->ColorFormat != GFX_CF_RGB565) &&
		(Src0Img->ColorFormat != GFX_CF_ARGB4444) &&
		(Src0Img->ColorFormat != GFX_CF_ARGB8888)) ||
	    ((Src1Img->ColorFormat != GFX_CF_ARGB1555) &&
	     (Src1Img->ColorFormat != GFX_CF_RGB565) &&
	     (Src1Img->ColorFormat != GFX_CF_ARGB4444) && 
	     (Src1Img->ColorFormat != GFX_CF_ARGB8888)) ||
	    ((DesImg->ColorFormat != GFX_CF_ARGB1555) &&
	     (DesImg->ColorFormat != GFX_CF_RGB565) &&
	     (DesImg->ColorFormat != GFX_CF_ARGB4444) && 
	     (DesImg->ColorFormat != GFX_CF_ARGB8888)) )
    {
	return -2; /* unsuported color format */
    }
    else if ( !Is_RectValid(Src0Img, Src0Rect) || 
	    !Is_RectValid(Src1Img, Src1Rect) || 
	    !Is_RectValid(DesImg, DesRect) )
    {
	return -3; /* invalid rectangle */
    }
    else if (!CmpRect(Src0Rect, Src1Rect)) // || !CmpRect(Src0Rect, DesRect))
    {
	return -4;
    }

    //S0 Control
    {
	hGfxDC->S0Enable       = 1;
	hGfxDC->S0VReverseScan = 0; 		//don't use reverse scan
	hGfxDC->S0FetchDram    = 1;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB

	// if S0 Using Default Color then S0ColorType should be ARGB565 or A0
	hGfxDC->S0ColorFormat  = Src0Img->ColorFormat; //Must
	hGfxDC->S0DefaultColor.Alpha = Src0Img->DefaultColor.Alpha;

	Calc_DramPara
	    (
	     Src0Img, 
	     Src0Rect,	
	     hGfxDC->S0VReverseScan,
	     &hGfxDC->S0BaseAddr,              //Bytes Addr
	     &hGfxDC->S0LinePitch,             //Bytes Addr
	     &hGfxDC->S0SkipPixelLine         //Valid Only When Clut4
	    );

	hGfxDC->S0PixelNumOneLine    = Src0Rect->right  - Src0Rect->left;;
	hGfxDC->S0TotalLineNum       = Src0Rect->bottom - Src0Rect->top;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	hGfxDC->S0ByteEndian   = Src0Img->ByteEndian;
	hGfxDC->S0NibbleEndian = Src0Img->NibbleEndian;
	hGfxDC->S016BitEndian  = Src0Img->TwoBytesEndian;
#endif

	hGfxDC->S0ClutEnable   = 0;
	hGfxDC->S0ColorKeyEnable= 0; 
    }

    //S1 Control
    {
	hGfxDC->S1Enable       = 1;
	hGfxDC->S1VReverseScan = 0;
	hGfxDC->S1FetchDram    = 1;          // 1 Fetch, 0 do not Fetch Using Default Color in ARGB

	// if S1 Using Default Color then S1ColorType should be ARGB565 or A0
	hGfxDC->S1ColorFormat  = Src1Img->ColorFormat; //Must
	hGfxDC->S1DefaultColor.Alpha = Src1Img->DefaultColor.Alpha;

	Calc_DramPara
	    (
	     Src1Img, 
	     Src1Rect,	
	     hGfxDC->S1VReverseScan,
	     &hGfxDC->S1BaseAddr,              //Bytes Addr
	     &hGfxDC->S1LinePitch,             //Bytes Addr
	     &hGfxDC->S1SkipPixelLine         //Valid Only When Clut4
	    );

	hGfxDC->S1PixelNumOneLine    = Src1Rect->right  - Src1Rect->left;;
	hGfxDC->S1TotalLineNum       = Src1Rect->bottom - Src1Rect->top;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	hGfxDC->S1ByteEndian   = Src1Img->ByteEndian;
	hGfxDC->S1NibbleEndian = Src1Img->NibbleEndian;
	hGfxDC->S116BitEndian  = Src1Img->TwoBytesEndian;
#endif


	hGfxDC->S1ClutEnable   = 0;
	hGfxDC->S1ColorKeyEnable= 0; 
    }

    /* ROP and Compositor */
    hGfxDC->CompositorEnable   = BlendEnable;
    hGfxDC->ROPAlphaCtrl       = ROPAlphaCtrl;
    hGfxDC->RopValue           = RopValue;
    hGfxDC->S0OnTopS1          = IsS0OnTopS1;             /* 1 S0 On Top S1 */

    /* D Dram Para */
    Calc_DramPara
	(
	 DesImg, 
	 DesRect,	
	 hGfxDC->DVReverseScan = 0,
	 &hGfxDC->DBaseAddr,              //Bytes Addr
	 &hGfxDC->DLinePitch,             //Bytes Addr
	 &hGfxDC->DSkipPixelLine          //Valid Only When Clut4
	);
    hGfxDC->DPixelNumOneLine    = DesRect->right  - DesRect->left;
    hGfxDC->DTotalLineNum       = DesRect->bottom - DesRect->top;
    hGfxDC->DColorFormat        = DesImg->ColorFormat;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	hGfxDC->DByteEndian         = DesImg->ByteEndian;
    hGfxDC->DNibbleEndian       = DesImg->NibbleEndian;		
    hGfxDC->D16BitEndian        = DesImg->TwoBytesEndian;
#endif
    //Scalor Control
    hGfxDC->S0ScalorEnable   = 0;
    hGfxDC->UpdateHFIRCoeff  = 0;
    hGfxDC->UpdateVFIRCoeff  = 0;

    //Clut para
    hGfxDC->UpdateClut4Table = 0;
    hGfxDC->UpdateClut8Table = 0;

    /* RunTimeControl */
    hGfxDC->InterruptEnable  = 0;

    /* active DC */
    Gfx_WaitForIdle(NULL);

    if (0 != Gfx_ActiveDeviceContext(hGfxDC, GFX_DC_ALL))
    {
	return -5; /* failed to active device context */
    }

    return 0;
}

typedef struct _2d_comp_params{
	RECT src0_rect;
	u32 src0_width;
	u32 src0_height;
	int src0_color_format;
	u32 src0_pitch_line;
	u32 src0_bits_pixel;
	u32 src0_phy_addr;

	RECT src1_rect;
	u32 src1_width;
	u32 src1_height;
	int src1_color_format;
	u32 src1_pitch_line;
	u32 src1_bits_pixel;
	u32 src1_phy_addr;

	RECT dst_rect;
	u32 dst_width;
	u32 dst_height;
	int dst_color_format;
	u32 dst_pitch_line;
	u32 dst_bits_pixel;
	u32 dst_phy_addr;
} gfx2d_comp_params;

int DFB_2DComposite(gfx2d_comp_params *params)
{
    COLOR_IMG Src0Img, Src1Img, DstImg;

    memset(&Src0Img, 0, sizeof(Src0Img));
    memset(&Src1Img, 0, sizeof(Src1Img));
    memset(&DstImg, 0, sizeof(DstImg));

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)
    Src0Img.ByteEndian     = 1;
    Src0Img.NibbleEndian   = 0;
    Src1Img.ByteEndian     = 1;
    Src1Img.NibbleEndian   = 0;

    Src0Img.TwoBytesEndian = 1;
    Src1Img.TwoBytesEndian = 1;
    if (GFX_CF_ARGB8888 == params->src0_color_format)
    {
    	Src0Img.TwoBytesEndian = 0;
    	Src1Img.TwoBytesEndian = 0;
    }

    DstImg.ByteEndian = Src0Img.ByteEndian;
    DstImg.NibbleEndian = Src0Img.NibbleEndian;
    DstImg.TwoBytesEndian = Src0Img.TwoBytesEndian;
#endif
	DstImg.ColorFormat = Src1Img.ColorFormat = Src0Img.ColorFormat = params->src0_color_format; /* GFX_CF_...*/

    /* the default color for fill or draw */
    //DstImg.DefaultColor.Alpha = Src1Img.DefaultColor.Alpha = Src0Img.DefaultColor.Alpha = 0x80;
    Src0Img.DefaultColor.Alpha = 0x80;
    Src1Img.DefaultColor.Alpha = 0xff;
    DstImg.DefaultColor.Alpha = 0xff;
    DstImg.DefaultColor.Red   = Src1Img.DefaultColor.Red  = Src0Img.DefaultColor.Red = 0xff;
    DstImg.DefaultColor.Green = Src1Img.DefaultColor.Green = Src0Img.DefaultColor.Green = 0xff;
    DstImg.DefaultColor.Blue  = Src1Img.DefaultColor.Blue = Src0Img.DefaultColor.Blue = 0xff;

    Src0Img.PixelWidth = params->src0_width;
    Src0Img.PixelHeight = params->src0_height;
    Src0Img.LinePitch = params->src0_pitch_line;		
    Src0Img.StartAddress = params->src0_phy_addr; 

    Src1Img.PixelWidth = params->src1_width;
    Src1Img.PixelHeight = params->src1_height;
    Src1Img.LinePitch = params->src1_pitch_line;		
    Src1Img.StartAddress = params->src1_phy_addr; 

    DstImg.PixelWidth = params->dst_width;
    DstImg.PixelHeight = params->dst_height;
    DstImg.LinePitch = params->dst_pitch_line;
    DstImg.StartAddress = params->dst_phy_addr;

    return Gfx_2DComposite
	(
	 &Gfx_DC,
	 &Src0Img, 
	 &params->src0_rect, 
	 &Src1Img, 
	 &params->src1_rect, 
	 &DstImg, 
	 &params->dst_rect, 
	 ROP_R2_COPYPEN, 
	 1, 1, 	1
	);
}


typedef struct _2d_scalor_params{
	RECT src_rect;
	u32 src_width;
	u32 src_height;
	int src_color_format;
	u32 src_pitch_line;
	u32 src_bits_pixel;
	u32 src_phy_addr;

	RECT dst_rect;
	u32 dst_width;
	u32 dst_height;
	int dst_color_format;
	u32 dst_pitch_line;
	u32 dst_bits_pixel;
	u32 dst_phy_addr;

}gfx2d_scalor_params;

int DFB_2DScalor(gfx2d_scalor_params *params)
{
	int hResult = -1;

	unsigned int VInitialPhase = 4;
	unsigned int HInitialPhase = 0;
	signed int SrcVReverse = 0;
	signed int DesVReverse = 0;

	COLOR_IMG SrcImg, DstImg;

	memset(&SrcImg, 0, sizeof(SrcImg));
	memset(&DstImg, 0, sizeof(DstImg));

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	SrcImg.ByteEndian     = 1;
	SrcImg.NibbleEndian   = 0;
	SrcImg.TwoBytesEndian = 1;
    
	if (GFX_CF_ARGB8888 == params->src_color_format)
    	{
    		SrcImg.TwoBytesEndian = 0;
    	}
   
	DstImg.ByteEndian = SrcImg.ByteEndian;
	DstImg.NibbleEndian = SrcImg.NibbleEndian;
	DstImg.TwoBytesEndian = SrcImg.TwoBytesEndian;
#endif
	DstImg.ColorFormat = SrcImg.ColorFormat = params->src_color_format; /* GFX_CF_...*/

	/* the default color for fill or draw */
	DstImg.DefaultColor.Alpha =  SrcImg.DefaultColor.Alpha = 0xff;
	DstImg.DefaultColor.Red   = SrcImg.DefaultColor.Red = 0xff;
	DstImg.DefaultColor.Green = SrcImg.DefaultColor.Green = 0xff;
	DstImg.DefaultColor.Blue  = SrcImg.DefaultColor.Blue = 0xff;

	SrcImg.PixelWidth = params->src_width;
	SrcImg.PixelHeight = params->src_height;
	SrcImg.LinePitch = params->src_pitch_line;		
	SrcImg.StartAddress = params->src_phy_addr; 

	DstImg.PixelWidth = params->dst_width;
	DstImg.PixelHeight = params->dst_height;
	DstImg.LinePitch = params->dst_pitch_line;
	DstImg.StartAddress = params->dst_phy_addr;

	hResult = Gfx_2DScalor(&Gfx_DC,
			&SrcImg,
			&params->src_rect,
			SrcVReverse,
			&DstImg,
			&params->dst_rect,
			DesVReverse,
			12, /* copy pen */
			VInitialPhase,
			HInitialPhase);

	return hResult;
}

static int Gfx_2DScalor( 
		GFX_DEVICE_CONTEXT  *hGfxDC, 
		COLOR_IMG           *SrcImg, 
		RECT                *SrcRect, 
		signed int           SrcVReverse,
		COLOR_IMG           *DesImg, 
		RECT                *DesRect, 
		signed int           DesVReverse,
		ROP_OPT              RopValue, 
		u32                  VInitialPhase, 
		u32                  HInitialPhase)
{
	if (SrcImg  == NULL ||
			SrcRect == NULL ||
			DesImg  == NULL ||
			DesRect == NULL)
	{
		return -1; /* invalid parameters */
	}
	else if ((SrcImg->ColorFormat != GFX_CF_ARGB1555) &&
			(SrcImg->ColorFormat != GFX_CF_RGB565) &&
			(SrcImg->ColorFormat != GFX_CF_ARGB4444) &&
			(SrcImg->ColorFormat != GFX_CF_ARGB8888))
	{
		return -2; /* unsuported color format */
	}
	else if ((DesImg->ColorFormat != GFX_CF_ARGB1555) &&
			(DesImg->ColorFormat != GFX_CF_RGB565) &&
			(DesImg->ColorFormat != GFX_CF_ARGB4444) &&
			(DesImg->ColorFormat != GFX_CF_ARGB8888))
	{
		return -2; /* unsupported color format */
	}
	else if (!Is_RectValid(SrcImg, SrcRect))
	{
		return -3; /* invalid rectangle */
	}
	else if (!Is_RectValid(DesImg, DesRect))
	{
		return -4; /* invalid dst rectangle */
	}

	/* S0 Control */
	{
		hGfxDC->S0Enable       = 1;
		hGfxDC->S0VReverseScan = SrcVReverse;
		hGfxDC->S0FetchDram    = 1;
		hGfxDC->S0ColorFormat  = SrcImg->ColorFormat;
		hGfxDC->S0DefaultColor.Alpha = SrcImg->DefaultColor.Alpha;

		Calc_DramPara(SrcImg, SrcRect,	
				hGfxDC->S0VReverseScan,
				&hGfxDC->S0BaseAddr,
				&hGfxDC->S0LinePitch,
				&hGfxDC->S0SkipPixelLine);

		hGfxDC->S0PixelNumOneLine= SrcRect->right  - SrcRect->left;;
		hGfxDC->S0TotalLineNum = SrcRect->bottom - SrcRect->top;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

		hGfxDC->S0ByteEndian   = SrcImg->ByteEndian;
		hGfxDC->S0NibbleEndian = SrcImg->NibbleEndian;
		hGfxDC->S016BitEndian  = SrcImg->TwoBytesEndian;
#endif
		hGfxDC->S0ClutEnable   = 0;
		hGfxDC->S0ColorKeyEnable= 0; 
	}

	/* S1 Control */
	{
		hGfxDC->S1Enable = 0;
		hGfxDC->S1FetchDram = 0;
	}

	/* ROP and Compositor */
	hGfxDC->CompositorEnable = 0;
	hGfxDC->ROPAlphaCtrl = 1;
	hGfxDC->RopValue = RopValue;

	/* S0 On Top S1 */

	/* Dram Para */	
	Calc_DramPara(DesImg,DesRect,
			hGfxDC->DVReverseScan = DesVReverse,
			&hGfxDC->DBaseAddr,
			&hGfxDC->DLinePitch,
			&hGfxDC->DSkipPixelLine);

	hGfxDC->DPixelNumOneLine    = DesRect->right  - DesRect->left;;
	hGfxDC->DTotalLineNum       = DesRect->bottom - DesRect->top;

	hGfxDC->DColorFormat        = DesImg->ColorFormat;
#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)

	hGfxDC->DByteEndian         = DesImg->ByteEndian;
	hGfxDC->DNibbleEndian       = DesImg->NibbleEndian;		
	hGfxDC->D16BitEndian        = DesImg->TwoBytesEndian;
#endif
	/* Scalor Control */
	hGfxDC->S0ScalorEnable   = 1;
	hGfxDC->HInitialPhase    = HInitialPhase;
	hGfxDC->VInitialPhase    = VInitialPhase;
	hGfxDC->UpdateHFIRCoeff  = 0;
	hGfxDC->UpdateVFIRCoeff  = 0;

	/* Clut para */
	hGfxDC->UpdateClut4Table = 0;
	hGfxDC->UpdateClut8Table = 0;

	/* RunTimeControl */
	hGfxDC->InterruptEnable  = 0;

	/* active DC */
	Gfx_WaitForIdle(NULL);

	if (0 != Gfx_ActiveDeviceContext(hGfxDC, GFX_DC_ALL))
	{
		return -5; /* failed to active device context */
	}

	return 0;
}

int Gfx_2DInit(void)
{
	sGfxBaseAddr = (u32 *) VA_IO_GRAPHICS_BASE;
	memset(&Gfx_DC, 0, sizeof(Gfx_DC));
	Gfx_CMDGroupCntSyncHW();
	
	Gfx_2DReset();

	Gfx_SetDefaultClut8Table(&Gfx_DC);

	Gfx_SetDefaultScalorHFIRCoeff(&Gfx_DC);
	Gfx_SetDefaultScalorVFIRCoeff(&Gfx_DC);

	return 0;

}

static int Gfx_SetDefaultClut8Table(GFX_DEVICE_CONTEXT * hGfxDC)
{
	int hResult = -1;
	u32 *Clut8Table = NULL;

	if (hGfxDC == NULL) return hResult;

	Clut8Table = Gfx_GetDefaultClut8Table();
	if (Clut8Table == NULL)
	{
		return -2;
	}

	hResult = Gfx_InitClut8Tab(hGfxDC, Clut8Table);

	return hResult;	
}

static int Gfx_InitClut8Tab( GFX_DEVICE_CONTEXT * hGfxDC, u32 *Clut8Table)
{
	if (hGfxDC == NULL || Clut8Table == NULL) return -1;

	{
		u32 *p = NULL;
		for (p = hGfxDC->Clut8Table; p < hGfxDC->Clut8Table + CLUT8_TABLE_LEN; p++)
		{
			*p = *Clut8Table++;
		}
	}

	/* Send CMD to Gfx HW */
	hGfxDC->UpdateClut8Table = 1;
	hGfxDC->UpdateClut8TableMode = 0;

	if (0 != Gfx_ActiveDeviceContext(hGfxDC, GFX_DC_CLUT8_TABLE))
	{
		return -2;
	}
	hGfxDC->UpdateClut8Table = 0;
	hGfxDC->UpdateClut8TableMode = 0;

	return 0;
}

static u32 *Gfx_GetDefaultClut8Table(void)
{
	return _GfxCLUT8Pal;
}

static u32 SCALOR_COEFF( u32 SignBit, u32 Nmerator, u32 denominator) 
{
	return (((SignBit) << SCALOR_COEFF_POLARITY_BIT)  | \
			((((Nmerator)) << SCALOR_COEFF_FRACTION_BIT_WIDTH ) / \
			 (denominator)));
}

static int Gfx_InitScalorVFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC, u32 *VFIRCoeff)
{
	int hResult = -1;

	if (hGfxDC == NULL || VFIRCoeff == NULL) 
	{
		return hResult;
	}
	else
	{
		u32 *p = NULL;

		for (p = hGfxDC->VFIRCoeffTable[0];
				p < hGfxDC->VFIRCoeffTable[0] + SCALOR_PAHSE_NUM * SCALOR_V_FIR_TAP_NUM;
				p++)
		{
			*p = *VFIRCoeff++;
		}

		hGfxDC->UpdateVFIRCoeff = 1;

		if (0 != Gfx_ActiveDeviceContext(hGfxDC, GFX_DC_SCALOR_VFIR_COFF))
		{
			hResult = -2;
		}
		else
		{	
			hResult = 0;
		}

		hGfxDC->UpdateVFIRCoeff = 0;

		return hResult;
	}	
}

static u32 *Gfx_GetDefaultScalorVFIRCoeff(void)
{
	static u32 VFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_V_FIR_TAP_NUM];
	u32 Phase = 0;

	for (Phase = 0 ; Phase < SCALOR_PAHSE_NUM; Phase++)
	{
		VFIRCoeffTable[Phase][0] = SCALOR_COEFF( 0, (SCALOR_PAHSE_NUM - Phase), SCALOR_PAHSE_NUM);
		VFIRCoeffTable[Phase][1] = SCALOR_COEFF( 0, (Phase), SCALOR_PAHSE_NUM);
	}
	return VFIRCoeffTable[0];	
}

static int Gfx_SetDefaultScalorVFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC)
{	
	if (hGfxDC == NULL)
	{
		return -1;
	}
	else /* Scalor Coeffecient Setting */
	{
		int hResult = -1;
		u32 *VFIRCoeff = NULL;

		VFIRCoeff = Gfx_GetDefaultScalorVFIRCoeff();

		if (VFIRCoeff == NULL)
		{
			return hResult;
		}

		hResult = Gfx_InitScalorVFIRCoeff(hGfxDC, VFIRCoeff);

		return hResult;
	}
}

static int Gfx_InitScalorHFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC, u32 *HFIRCoeff)
{
	if (hGfxDC == NULL || HFIRCoeff == NULL) 
	{
		return -1;
	}
	else
	{
		int hResult = -1;
		u32 *p = NULL;

		for (p = hGfxDC->HFIRCoeffTable[0];
				p < hGfxDC->HFIRCoeffTable[0] + SCALOR_PAHSE_NUM * SCALOR_H_FIR_TAP_NUM;
				p++)
		{
			*p = *HFIRCoeff++;
		}

		hGfxDC->UpdateHFIRCoeff = 1;
		if (0 != Gfx_ActiveDeviceContext(hGfxDC, GFX_DC_SCALOR_HFIR_COFF))
		{
			hResult = -2;
		}
		else
		{	
			hResult = 0;
		}

		hGfxDC->UpdateHFIRCoeff = 0;

		return hResult;
	}
}

static u32 *Gfx_GetDefaultScalorHFIRCoeff(void)
{
	u32 Phase = 0;
	static u32 HFIRCoeffTable[SCALOR_PAHSE_NUM][SCALOR_H_FIR_TAP_NUM];

	for (Phase = 0 ; Phase < SCALOR_PAHSE_NUM; Phase++)
	{
		HFIRCoeffTable[Phase][0] = 0;
		HFIRCoeffTable[Phase][1] = SCALOR_COEFF( 0, (SCALOR_PAHSE_NUM - Phase), SCALOR_PAHSE_NUM);
		HFIRCoeffTable[Phase][2] = SCALOR_COEFF( 0, (Phase), SCALOR_PAHSE_NUM);
		HFIRCoeffTable[Phase][3] = 0;
	}

	return HFIRCoeffTable[0];
}

static int Gfx_SetDefaultScalorHFIRCoeff(GFX_DEVICE_CONTEXT *hGfxDC)
{
	if (hGfxDC == NULL)
	{
		return -1;
	}
	else /* Scalor Coeffecient Setting */
	{
		int hResult = -1;
		u32 *HFIRCoeff = NULL;

		HFIRCoeff = Gfx_GetDefaultScalorHFIRCoeff();

		if (HFIRCoeff == NULL)
		{
			return hResult;
		}

		hResult = Gfx_InitScalorHFIRCoeff(hGfxDC, HFIRCoeff);

		return hResult;
	}
}

