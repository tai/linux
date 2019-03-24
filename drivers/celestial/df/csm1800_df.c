/****************************************************************************
 * Copyright (C) 2008-2010 Celestial Semiconductor Inc.
 * All rights reserved
 *
 * [RELEASE HISTORY]
 * VERSION  DATE       AUTHOR                  DESCRIPTION
 * 0.1      10-03-10   Jia Ma           			Original
 ****************************************************************************
 */

#include <linux/device.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/fb.h>
#include <asm/uaccess.h>

#include <mach/df_reg_def.h>
#include <mach/df_reg_fmt.h>
#include <mach/csm1800_df.h>

#include "csm1800_df_clock.h"
#include "scalor_coeff.h"
#include "tve0_1800.h"
#include "tve1_1800.h"
#include "df_output_format.h"
#include "../video/csm1800_video.h"

#define DF_IRQ_DEBUG (0)

#ifdef CONFIG_CELESTIAL_DF_DEBUG
#define DEBUG_PRINTF  printk
#else
#define DEBUG_PRINTF(fmt,args...)
#endif

#if DF_IRQ_DEBUG
#define DF_IRQ_PRINTF printk
#else
#define DF_IRQ_PRINTF(fmt,args...)
#endif

#define BOOT_HAS_OUTPUT 0

#define DF_MAJOR			200

#define DF_MINOR_GFX0		0
#define DF_MINOR_GFX1		1
#define DF_MINOR_VID0		2
#define DF_MINOR_VID1		3
#define DF_MINOR_OUT0		4
#define DF_MINOR_OUT1		5
#define DF_MINOR_TVE0		6
#define DF_MINOR_TVE1		7
#define DF_MINOR_VIDAUX	8

static int __Is_hd2sd = 1;
static int __Is_ColorBar = 0;
static int __Is_first = 1;
int __VGA_Support = 0;
int __Is_TV = 1;

typedef enum _DAC1_MODE_
{
	_dac1_tve0_ypbpr =0,
	_dac1_tve1_ypbpr =1,
	_dac1_disp0_rgb  =2,
	_dac1_tve1_cvbs  =3,
	_dac1_disp1_rgb  =4,
}DAC1_MODE;

extern int pinmux_set_dac1mode(DAC1_MODE Mode);
extern _HOST_FW_IF_t *host2videofw_if;
extern unsigned int __video_write(unsigned int val, unsigned int addr);
extern unsigned int __video_read(unsigned int addr);

volatile unsigned char  *disp_base = NULL;
volatile unsigned char  *tve0_base = NULL;
volatile unsigned char  *tve1_base = NULL;

static dfdev df_dev[9];
static int tve_2_df[] = {
	DISP_YUV_PAL,
	DISP_YUV_NTSC,
	DISP_YUV_576P,
	DISP_YUV_480P,
	DISP_YUV_720P_50FPS,
	DISP_YUV_720P_60FPS,
	DISP_YUV_1080I_25FPS,
	DISP_YUV_1080I_30FPS,

	DISP_YUV_PAL,
	DISP_YUV_NTSC,
	DISP_YUV_PAL,
	DISP_YUV_PAL,
	DISP_YUV_1080P_24FPS,
	DISP_YUV_1080P_25FPS,
	DISP_YUV_1080P_30FPS,
	DISP_YUV_1080P_50FPS,
	DISP_YUV_1080P_60FPS,

	DISP_RGB_VGA_60FPS,
	DISP_RGB_CVT_800x600_60,
	DISP_RGB_XGA_60FPS,
	DISP_RGB_CVT_1280x720_60,
	DISP_RGB_800x480,
	DISP_RGB_DMT_1440x900_60,
	DISP_RGB_DMT_1280x1024_60,
	DISP_RGB_1366x768,
	DISP_RGB_1920x1080P30,
	DISP_RGB_1920x1080P60,
	DISP_RGB_1920x1080I30,
};

static int tve_2_clock[] = {
	_YUV_PAL,
	_YUV_NTSC,
	_YUV_576P,
	_YUV_480P,
	_YUV_720P_50Hz,
	_YUV_720P_60Hz,
	_YUV_1080I_50Hz,
	_YUV_1080I_60Hz,

	_YUV_PAL,
	_YUV_NTSC,
	_YUV_PAL,
	_YUV_PAL,
	_YUV_1080P_24Hz,
	_YUV_1080P_25Hz,
	_YUV_1080P_30Hz,
	_YUV_1080P_50Hz,
	_YUV_1080P_60Hz,

	_RGB_DMT_640x480_60Hz,
	_RGB_CVT_800x600_60Hz,
	_RGB_DMT_1024x768_60Hz,
	_RGB_CVT_1280x720_60Hz,
	_RGB_CVT_800x480_60Hz,
	_RGB_DMT_1440x900_60Hz,
	_RGB_DMT_1280x1024_60Hz,
	_RGB_CVT_1366x768_60Hz,
};

DEFINE_SPINLOCK(csm_df_lock);

#define REG_MAP_ADDR(x)	((((x)>>16) == 0x4180)  ? (disp_base+((x)&0xffff)) :  \
		((((x)>>12) == 0x10160) ? (tve1_base+((x)&0xfff)) :      \
		 (((x)>>12) == 0x10168) ? (tve0_base+((x)&0xfff)):0))

#define df_write8(a,v)	writeb(v,REG_MAP_ADDR(a))
#define df_write16(a,v)	writew(v,REG_MAP_ADDR(a))
#define df_write(a,v)	writel(v,REG_MAP_ADDR(a))
#define df_read8(a)	readb(REG_MAP_ADDR(a))
#define df_read16(a)	readw(REG_MAP_ADDR(a))
#define df_read(a)	readl(REG_MAP_ADDR(a))

df_reg_para dfreg;

#define USE_VIDEO_LAYER2 0

DF_OUTPUT_CONFIG_t df_output_config = {
	.gfx_output = {0,0},
#if	USE_VIDEO_LAYER2
	.vid_output = {0,0,1},
#else
	.vid_output = {0,1,0},
#endif
	.output_mode = {DF_OUTPUT_YPBPR, DF_OUTPUT_YPBPR},
};

unsigned int DF_Read(unsigned int addr)
{
	return df_read(addr);
}
EXPORT_SYMBOL(DF_Read);

void DF_Write(unsigned int addr, unsigned int data)
{
	df_write(addr, data);
	return;
}
EXPORT_SYMBOL(DF_Write);

unsigned char TVE_Read(unsigned int tveid, unsigned int addr)
{
	if(tveid == 0)
		return df_read8(0x10168000+addr);
	else if(tveid == 1)
		return df_read8(0x10160000+addr);
	else
		return (-1);
}

void TVE_Write(unsigned int tveid, unsigned int addr, unsigned char data)
{
	if(tveid == 0)
		df_write8((0x10168000+addr), data);
	else if(tveid == 1)
		df_write8((0x10160000+addr), data);
	else
		return;

	return;
}

static unsigned char __tve0_reg_read(unsigned char addr_base)
{
	unsigned char data_rd;
	unsigned int addr ;
	addr = TVE0_REG_BASE +(addr_base);
	data_rd = df_read8(addr);
	return data_rd;
}

static void _tve0_reg_write(unsigned char addr_base, unsigned char data)
{
	unsigned char data_rd;
	unsigned int addr ;
	addr = TVE0_REG_BASE +(addr_base);

	df_write8(addr,data);
	data_rd = df_read8(addr);

	if(data_rd != data)
	{
		DEBUG_PRINTF("TVE0: Reg Write ERROR: addr_base = %02x, wr_data =%08x,rd_data = %08x\n",
				addr_base,data,data_rd);
	}
}

int InitTVE0Raw(TVOUT_MODE DispFormat , int EnableColorBar)
{
	int i = 0;
	int VIDEO_TYPE  = TVOUT_MODE_576I;
	int IsHD = 0;
	int IsProgressive = 0;

	//Map the Display Format Value to TVE0 Format Value
	switch (DispFormat)
	{
		case TVOUT_MODE_SECAM:
			VIDEO_TYPE = TYPE_SECAM;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_N:
			VIDEO_TYPE = TYPE_PAL_N;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_M:
			VIDEO_TYPE = TYPE_PAL_M;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_CN:
			VIDEO_TYPE = TYPE_PAL_CN;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_576I:
			VIDEO_TYPE = TYPE_PAL;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_480I:
			VIDEO_TYPE = TYPE_NTSC;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_1080I30:
			VIDEO_TYPE = TYPE_1080i30;
			IsHD = 1;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_1080I25:
			VIDEO_TYPE = TYPE_1080i;
			IsHD = 1;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_720P60:
			VIDEO_TYPE = TYPE_720p60;
			IsHD = 1;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_720P50:
			VIDEO_TYPE = TYPE_720p50;
			IsHD = 1;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_480P:
		case TVOUT_MODE_576P:
			VIDEO_TYPE = TYPE_480p; //???
			IsHD = 0;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_1080P24:
		case TVOUT_MODE_1080P25:
		case TVOUT_MODE_1080P30:
		case TVOUT_MODE_1080P50:
		case TVOUT_MODE_1080P60:
			return 0;
		default:
			VIDEO_TYPE = TYPE_PAL;
			IsHD = 0;
			IsProgressive = 0;
			break;
	}

	//Hold TVE0 Timing
	_tve0_reg_write(TVE0_REG[0][REG_ADDR],0x1);

	//DEBUG_PRINTF("[TVE0: INFO]probe REV_ID ..%d\n",TVE0_REG[1][REG_ADDR]);

	for (i=2; i<TVE0_REG_NUM; i++)
	{
		//DEBUG_PRINTF("TVE0: ADDR=%03x,WriteDATA=%03x\n",TVE0_REG[i][REG_ADDR],TVE0_REG[i][VIDEO_TYPE]);
		_tve0_reg_write(TVE0_REG[i][REG_ADDR],TVE0_REG[i][VIDEO_TYPE]);
	}

	if (EnableColorBar)
	{
		unsigned char  RegValue = 0;
		RegValue = __tve0_reg_read(0x2);
		RegValue |= (1 << 5);
		_tve0_reg_write(0x2,RegValue);
	}
	_tve0_reg_write(TVE0_REG[0][REG_ADDR],0x0);
	return 0;
}

static unsigned char __tve1_reg_read(unsigned char addr_base)
{
	unsigned char data_rd;
	unsigned int addr ;
	addr = TVE1_REG_BASE +(addr_base);
	data_rd = df_read8(addr);
	return data_rd;
}

static void _tve1_reg_write(unsigned char addr_base, unsigned char data)
{
	unsigned char data_rd;
	unsigned int addr ;
	addr = TVE1_REG_BASE +(addr_base);

	df_write8(addr,data);
	data_rd = df_read8(addr);
	if(data_rd != data)
	{
		DEBUG_PRINTF("TVE1: Reg Write ERROR: addr_base = %02x, wr_data =%08x,rd_data = %08x\n",
				addr_base,data,data_rd);
	}
}

int InitTVE1Raw( TVOUT_MODE DispFormat , int EnableColorBar)
{
	int i = 0;
	int VIDEO_TYPE  = TVOUT_MODE_576I;
	int IsHD = 0;
	int IsProgressive = 0;

	//Map the Display Format Value to TVE1 Format Value
	switch (DispFormat)
	{
		case TVOUT_MODE_SECAM:
			VIDEO_TYPE = TYPE_SECAM;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_N:
			VIDEO_TYPE = TYPE_PAL_N;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_M:
			VIDEO_TYPE = TYPE_PAL_M;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_PAL_CN:
			VIDEO_TYPE = TYPE_PAL_CN;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_576I:
			VIDEO_TYPE = TYPE_PAL;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_480I:
			VIDEO_TYPE = TYPE_NTSC;
			IsHD = 0;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_1080I30:
		case TVOUT_MODE_1080I25:
			VIDEO_TYPE = TYPE_1080i;
			IsHD = 1;
			IsProgressive = 0;
			break;
		case TVOUT_MODE_720P60:
			VIDEO_TYPE = TYPE_720p60;
			IsHD = 1;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_720P50:
			VIDEO_TYPE = TYPE_720p50;
			IsHD = 1;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_480P:
			VIDEO_TYPE = TYPE_480p;
			IsHD = 0;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_576P:
			VIDEO_TYPE = TYPE_480p;
			IsHD = 0;
			IsProgressive = 1;
			break;
		case TVOUT_MODE_1080P24:
		case TVOUT_MODE_1080P25:
		case TVOUT_MODE_1080P30:
			return 0;
		default:
			VIDEO_TYPE = TYPE_PAL;
			IsHD = 0;
			IsProgressive = 0;
			break;
	}
	//printascii("1111111111\n");
	//Hold TVE1 Timing
	_tve1_reg_write(TVE1_REG[0][REG_ADDR],0x1);
	//DEBUG_PRINTF("[TVE1: INFO]probe REV_ID ..%d\n",TVE1_REG[1][REG_ADDR]);
	//printascii("222222222222\n");

	for (i=2; i<TVE1_REG_NUM; i++)
	{
		//DEBUG_PRINTF("TVE1: ADDR=%03x,WriteDATA=%03x\n",TVE1_REG[i][REG_ADDR],TVE1_REG[i][VIDEO_TYPE]);
		_tve1_reg_write(TVE1_REG[i][REG_ADDR],TVE1_REG[i][VIDEO_TYPE]);
	}

	if (EnableColorBar)
	{
		unsigned char RegValue = 0;
		RegValue = __tve1_reg_read(0x2);
		RegValue |= (1 << 5);
		_tve1_reg_write(0x2,RegValue);
	}
	_tve1_reg_write(TVE1_REG[0][REG_ADDR],0x0);

	return 0;
}

static int dfreg_ctrl[10] =
{
	DISP_UPDATE_REG,DISP_STATUS,DISP_INT_CLEAR,DISP_OUTIF1_INT_CLEAR,DISP_OUTIF2_INT_CLEAR,DISP_OUTIF1_ERR_CLEAR,
	DISP_OUTIF2_ERR_CLEAR,DISP_HD2SD_ERR_CLEAR,DISP_SCA_COEF_IDX,DISP_SCA_COEF_DATA
};

static int dfreg_gfx[2][13] =
{
	{DISP_GFX1_CTRL,DISP_GFX1_FORMAT,DISP_GFX1_ALPHA_CTRL,DISP_GFX1_KEY_RED,DISP_GFX1_KEY_BLUE,DISP_GFX1_KEY_GREEN,
		DISP_GFX1_BUF_START,DISP_GFX1_LINE_PITCH,DISP_GFX1_X_POSITON,DISP_GFX1_Y_POSITON,DISP_GFX1_SCL_X_POSITON,
		DISP_GFX1_CLUT_ADDR,DISP_GFX1_CLUT_DATA
	},
	{DISP_GFX2_CTRL,DISP_GFX2_FORMAT,DISP_GFX2_ALPHA_CTRL,DISP_GFX2_KEY_RED,DISP_GFX2_KEY_BLUE,DISP_GFX2_KEY_GREEN,
		DISP_GFX2_BUF_START,DISP_GFX2_LINE_PITCH,DISP_GFX2_X_POSITON,DISP_GFX2_Y_POSITON,DISP_GFX2_SCL_X_POSITON,
		DISP_GFX2_CLUT_ADDR,DISP_GFX2_CLUT_DATA
	},
};

static int dfreg_video[3][19] =
{
	{DISP_VIDEO1_CTRL,DISP_VIDEO1_ALPHA_CTRL,DISP_VIDEO1_KEY_LUMA,DISP_VIDEO1_X_POSITON,DISP_VIDEO1_Y_POSITON,
		DISP_VIDEO1_SRC_X_CROP,DISP_VIDEO1_SRC_Y_CROP,DISP_VIDEO1_CMD,DISP_VIDEO1_STA_ADDR,DISP_VIDEO1_STA0,
		DISP_VIDEO1_STA1,DISP_VIDEO1_STA2,DISP_VIDEO1_STA_IMG_SIZE,DISP_VIDEO1_STA_FRM_INFO,DISP_VIDEO1_STA_Y_TOPADDR,
		DISP_VIDEO1_STA_Y_BOTADDR,DISP_VIDEO1_STA_C_TOPADDR,DISP_VIDEO1_STA_C_BOTADDR,DISP_VIDEO1_STA_DISP_NUM
	},
	{DISP_VIDEO2_CTRL,DISP_VIDEO2_ALPHA_CTRL,DISP_VIDEO2_KEY_LUMA,DISP_VIDEO2_X_POSITON,DISP_VIDEO2_Y_POSITON,
		DISP_VIDEO2_SRC_X_CROP,DISP_VIDEO2_SRC_Y_CROP,DISP_VIDEO2_CMD,DISP_VIDEO2_STA_ADDR,DISP_VIDEO2_STA0,
		DISP_VIDEO2_STA1,DISP_VIDEO2_STA2,DISP_VIDEO2_STA_IMG_SIZE,DISP_VIDEO2_STA_FRM_INFO,DISP_VIDEO2_STA_Y_TOPADDR,
		DISP_VIDEO2_STA_Y_BOTADDR,DISP_VIDEO2_STA_C_TOPADDR,DISP_VIDEO2_STA_C_BOTADDR,DISP_VIDEO2_STA_DISP_NUM
	},
	{DISP_VIDEO3_CTRL,DISP_VIDEO3_ALPHA_CTRL,0x0,DISP_VIDEO3_X_POSITON,DISP_VIDEO3_Y_POSITON,
		DISP_VIDEO3_SRC_X_CROP,DISP_VIDEO3_SRC_Y_CROP,DISP_VIDEO3_CMD,DISP_VIDEO3_STA_ADDR,DISP_VIDEO3_STA0,
		DISP_VIDEO3_STA1,DISP_VIDEO3_STA2,DISP_VIDEO3_STA_IMG_SIZE,DISP_VIDEO3_STA_FRM_INFO,DISP_VIDEO3_STA_Y_TOPADDR,
		DISP_VIDEO3_STA_Y_BOTADDR,DISP_VIDEO3_STA_C_TOPADDR,DISP_VIDEO3_STA_C_BOTADDR,DISP_VIDEO3_STA_DISP_NUM
	},
};
static int dfreg_comp[2][2] =
{
	{DISP_COMP1_BACK_GROUND,DISP_COMP1_Z_ORDER
	},
	{DISP_COMP2_BACK_GROUND,DISP_COMP2_Z_ORDER
	},
};
static int dfreg_hd2sd[6] =
{
	DISP_HD2SD_CTRL,DISP_HD2SD_DES_SIZE,DISP_HD2SD_ADDR_Y,DISP_HD2SD_ADDR_C,DISP_HD2SD_BUF_PITCH,
	DISP_HD2SD_STATUS
};
static int dfreg_outif[2][14] =
{
	{DISP_OUTIF1_CTRL,DISP_OUTIF1_X_SIZE,DISP_OUTIF1_Y_SIZE,DISP_OUTIF1_HSYNC,DISP_OUTIF1_VSYNC,DISP_OUTIF1_CLIP,
		DISP_OUTIF1_CM_COEF0_012,DISP_OUTIF1_CM_COEF0_3,DISP_OUTIF1_CM_COEF1_012,DISP_OUTIF1_CM_COEF1_3,
		DISP_OUTIF1_CM_COEF2_012,DISP_OUTIF1_CM_COEF2_3,DISP_OUTIF1_STA_DISP_SIZE,DISP_OUTIF1_STA_LINE
	},
	{DISP_OUTIF2_CTRL,DISP_OUTIF2_X_SIZE,DISP_OUTIF2_Y_SIZE,DISP_OUTIF2_HSYNC,DISP_OUTIF2_VSYNC,DISP_OUTIF2_CLIP,
		DISP_OUTIF2_CM_COEF0_012,DISP_OUTIF2_CM_COEF0_3,DISP_OUTIF2_CM_COEF1_012,DISP_OUTIF2_CM_COEF1_3,
		DISP_OUTIF2_CM_COEF2_012,DISP_OUTIF2_CM_COEF2_3,DISP_OUTIF2_STA_DISP_SIZE,DISP_OUTIF2_STA_LINE
	},
};

void __df_update_start(void)
{
	df_write(dfreg_ctrl[0], 1);
}
EXPORT_SYMBOL(__df_update_start);

void __df_update_end(void)
{
	df_write(dfreg_ctrl[0], 0);
}
EXPORT_SYMBOL(__df_update_end);

static unsigned int YHFIRCoeff[DF_VIDEO_SCL_LUMA_HFIR_PHASE_NUM][DF_VIDEO_SCL_LUMA_HFIR_TAP_NUM] =
{
	{0, 1024, 0, 0},
	{32796, 1014, 39, 32769},
	{32817, 987, 93, 32775},
	{32831, 944, 157, 32782},
	{32840, 888, 232, 32792},
	{32843, 820, 313, 32802},
	{32843, 745, 399, 32813},
	{32838, 662, 487, 32823},
	{32832, 576, 576, 32832},
	{32823, 487, 662, 32838},
	{32813, 399, 745, 32843},
	{32802, 313, 820, 32843},
	{32792, 232, 888, 32840},
	{32782, 157, 944, 32831},
	{32775, 93, 987, 32817},
	{32769, 39, 1014, 32796}
};

static unsigned int YVFIRCoeff[DF_VIDEO_SCL_LUMA_VFIR_PHASE_NUM][DF_VIDEO_SCL_LUMA_VFIR_TAP_NUM] =
{
	{0, 1024, 0, 0},
	{0, 960, 64, 0},
	{0, 896, 128, 0},
	{0, 832, 192, 0},
	{0, 768, 256, 0},
	{0, 704, 320, 0},
	{0, 640, 384, 0},
	{0, 576, 448, 0},
	{0, 512, 512, 0},
	{0, 448, 576, 0},
	{0, 384, 640, 0},
	{0, 320, 704, 0},
	{0, 256, 768, 0},
	{0, 192, 832, 0},
	{0, 128, 896, 0},
	{0, 64, 960, 0}
};

void DFGetScalerCfg(int ScalerType, int *TapNum, int *PhaseNum)
{
	if((TapNum == NULL)||(PhaseNum == NULL)){
		DEBUG_PRINTF("function : %s in file %s line %d\n",__FUNCTION__,__FILE__,__LINE__);
		return;
	}

	switch (ScalerType){
		case DF_VIDEO1_Y_HFIR_COEFF_IDX:
		case DF_VIDEO2_Y_HFIR_COEFF_IDX:
			*TapNum   = DF_VIDEO_SCL_LUMA_HFIR_TAP_NUM;
			*PhaseNum = DF_VIDEO_SCL_LUMA_HFIR_PHASE_NUM;
			break;
		case DF_VIDEO1_Y_VFIR_COEFF_IDX:
		case DF_VIDEO2_Y_VFIR_COEFF_IDX:
			*TapNum   = DF_VIDEO_SCL_LUMA_VFIR_TAP_NUM;
			*PhaseNum = DF_VIDEO_SCL_LUMA_VFIR_PHASE_NUM;
			break;
		case DF_HD2SD_Y_HFIR_COEFF_IDX :
			*TapNum   = DF_HD2SD_SCL_LUMA_HFIR_TAP_NUM;
			*PhaseNum = DF_HD2SD_SCL_LUMA_HFIR_PHASE_NUM;
			break;
		case DF_HD2SD_Y_VFIR_COEFF_IDX :
			*TapNum   = DF_HD2SD_SCL_LUMA_VFIR_TAP_NUM;
			*PhaseNum = DF_HD2SD_SCL_LUMA_VFIR_PHASE_NUM;
			break;
	}

	return;
}

int DFSetScalerCoeff(int VideoId, unsigned int *Coeff, int ScalerCoefType)
{
	int TapIdx = 0;
	int PhaseIdx = 0;
	int TapNum = 0, PhaseNum  = 0;

	DFGetScalerCfg(ScalerCoefType, &TapNum, &PhaseNum);

	for (PhaseIdx = 0; PhaseIdx < PhaseNum; PhaseIdx++)
	{
		for (TapIdx = 0; TapIdx < TapNum; TapIdx++)
		{
			int CoeffIdx  = 0;
			unsigned int CoeffData = 0;

			CoeffIdx = PhaseIdx * TapNum + TapIdx;
			CoeffData = Coeff[CoeffIdx];

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
			//       *Note: Other Value will not take any Effect
			df_write(dfreg_ctrl[8], (TapIdx & 0x3) | ((PhaseIdx & 0xf) << 8) | ((ScalerCoefType & 0x7) << 16));

			// Bit[11: 0] : Coeff Data, Reset Value 0;
			// Bit[14:12] : Reserved to be zero;
			// Bit[   15] : Signed Bit, Reset Value 0;
			//              1: Negtive Coeff, 0: Positive Coeff
			df_write(dfreg_ctrl[9], CoeffData);
		}
	}

	// df_write(dfreg_ctrl[0], 0); // finishd registers update

	return 0;
}

int DFVideoSetDefaultCoeffRaw
(
 int VideoId,
 int SrcCropXWidth ,int DispWinXWidth,
 int SrcCropYHeight,int DispWinYHeight,
 int Is4TapOr2Tap //Vertical Only
 )
{
	int rt = 0, tmp;
	int CoeffType = 0;

	VideoId = VideoId & 0x1;
	if (SrcCropXWidth  == 0) SrcCropXWidth  = 1024;
	if (DispWinXWidth  == 0) DispWinXWidth  = 1024;
	if (SrcCropYHeight == 0) SrcCropYHeight = 1024;
	if (DispWinYHeight == 0) DispWinYHeight = 1024;

	if ( SrcCropXWidth > 1920/2){
		/* video ctrl regs, scalor VFIR tap num sel = 1 */
		tmp = df_read(dfreg_video[VideoId][0]);
		tmp |= 0x10;
		df_write(dfreg_video[VideoId][0], tmp);
	}
	else{
		/* video ctrl regs, scalor VFIR tap num sel = 0 */
		tmp = df_read(dfreg_video[VideoId][0]);
		tmp &= ~0x10;
		df_write(dfreg_video[VideoId][0], tmp);
	}

	// Load the Coeff
	CoeffType = (VideoId == 0) ? DF_VIDEO1_Y_HFIR_COEFF_IDX : DF_VIDEO2_Y_HFIR_COEFF_IDX;
	rt = DFSetScalerCoeff(VideoId, YHFIRCoeff[0], CoeffType);

	CoeffType = (VideoId == 0) ? DF_VIDEO1_Y_VFIR_COEFF_IDX : DF_VIDEO2_Y_VFIR_COEFF_IDX;
	rt = DFSetScalerCoeff(VideoId, YVFIRCoeff[0], CoeffType);

	return rt;
}

int DFVideoSetDefaultCoeff(int VideoId)
{
	return DFVideoSetDefaultCoeffRaw
		(
		 VideoId,
		 1024,  1024,
		 1024,  1024,
		 1 //Select 4TapCoeff
		);
}

void GenCoeff(unsigned int SrcSize, unsigned int DesSize, unsigned char CoeffType)
{
	unsigned int scaleratio;
	int phaseindex;
	int tapindex;
	int samplePos;
	int Offset;
	int sampleInteral;
	unsigned int hi, lo;
	unsigned int reg;
	int TapNum = 0, PhaseNum  = 0;

	//	int tmp =0;

	DFGetScalerCfg(CoeffType, &TapNum,&PhaseNum);

	if (DesSize >= SrcSize){ // scale down,should spread the interpolator kernel
		scaleratio = 0xFFFFFFFF; // scaleratio = 1; Q32
	}
	else{
		mpa_mulu(0xFFFFFFFF, DesSize, &hi, &lo);
		scaleratio = mpa_div64(hi, lo, SrcSize);/* max width: 1920  min width: 320
							   max height: 1080 min height: 240 */
	}

	Offset = (TapNum - 1) >> 1 << 28; // (TapNum-1)/2 --> Q3.28
	sampleInteral = 1 << (28 - TapNum); // 1/PhaseNum --> Q0.28

	for (phaseindex = 0; phaseindex < PhaseNum; phaseindex++){
		int fCoeff[8];
		int sum = 0;
		int Coeff_32[8];

		samplePos = Offset + (sampleInteral * phaseindex); // Q3.28
		for(tapindex = 0; tapindex < TapNum; tapindex++){
			if(((SrcSize > 960) && ((CoeffType & 0x1) == 0x1))
					&& (CoeffType < DF_HD2SD_Y_HFIR_COEFF_IDX)) // v modify by ying @20100722
				fCoeff[tapindex] = Linear(samplePos); // Q0.28
			else
				fCoeff[tapindex] = Cubic(samplePos, scaleratio); // Q0.28
			sum += fCoeff[tapindex]; // Q3.28
			samplePos = samplePos - 0x10000000;
		}

		for(tapindex = 0; tapindex < TapNum; tapindex++){
			unsigned int ufCoeff;
			unsigned int uCoeff = ABS(fCoeff[tapindex]);

			mpa_mulu(0x400, uCoeff, &hi, &lo);
			ufCoeff =  mpa_div64(hi, lo, sum);	// normalization --> Q5.10
			if (uCoeff != fCoeff[tapindex]){
				Coeff_32[tapindex] = (int)(-1 * ufCoeff);
			}
			else{
				Coeff_32[tapindex] = (int)ufCoeff;
			}
		}

		// check sum
		while (1)
		{
			int i = 3;

			sum = 0;
			for (tapindex = 0; tapindex < TapNum; tapindex++) {
				sum += Coeff_32[tapindex];
			}
			if (sum < 0x400) {
				Coeff_32[i] += 1;
			}
			else if (sum > 0x400) {
				Coeff_32[i] -= 1;
			}
			else { //sum == 0x400
				break;
			}

			i++;
		}

		for (tapindex = 0; tapindex < TapNum; tapindex++) {
			reg = ((tapindex) <<  0) | ((phaseindex) <<  8) | ((CoeffType) << 16) ;
			if (Coeff_32[tapindex] < 0){
				Coeff_32[tapindex] = 32768 + (-1 * Coeff_32[tapindex]);
			}

			df_write(dfreg_ctrl[8], ((tapindex & 0x3) | ((phaseindex & 0xf) << 8) | ((CoeffType & 0x7) << 16)));
			df_write(dfreg_ctrl[9], Coeff_32[tapindex]);
			//			printk("%d,\t",Coeff_32[tapindex]);
		}
		//		printk("\n");
	}
	return;
}

void DFOutIFEnaColorModulator(int OutIFId, int IsEnable)
{
	df_outif_reg *Reg= NULL;

	OutIFId = OutIFId & 0x1;
	Reg = &dfreg.OutIF[OutIFId];

	Reg->df_outif_control.val = df_read(dfreg_outif[OutIFId][0]);
	Reg->df_outif_control.bits.iColorModulator = IsEnable;
	df_write(dfreg_outif[OutIFId][0], Reg->df_outif_control.val);

	return;
}

enum _YUV2RGB_COEFF_IDX_
{
	YUV_SD_TO_RGB = 0,
	YUV_HD_TO_RGB = 1,
	YUV_SD_TO_YUV_HD = 2,
	YUV_HD_TO_YUV_SD = 3,
	YUV2RGB_COEFF_NUM,
};

static int ColorModulatorCoeff[YUV2RGB_COEFF_NUM][12] =
{
	{
		74,		0,		102,	4319,
		74,		281,    308,	135,
		74,		129,	0,  	4373,
	},
	{
		74,		0,		115,	4344,
		74,		270,	290,	77,
		74,		135,	0,		4385,
	},
	{
		64,		263,	269,	41,
		0,		65,		7,		4113,
		0,		5,		66,		4109,
	},
	{
		64,		6,		12,		4133,
		0,		63,		263,	15,
		0,		261,	63,		11,
	},
};

void DFOutIFSetColorModulator(int OutIFId, unsigned int cm_index)
{
	df_outif_reg *Reg= NULL;

	OutIFId = OutIFId & 0x1;
	Reg = &dfreg.OutIF[OutIFId];

	Reg->df_outif_cm_coef0_012.val = df_read(dfreg_outif[OutIFId][6]);
	Reg->df_outif_cm_coef0_3.val   = df_read(dfreg_outif[OutIFId][7]);
	Reg->df_outif_cm_coef1_012.val = df_read(dfreg_outif[OutIFId][8]);
	Reg->df_outif_cm_coef1_3.val   = df_read(dfreg_outif[OutIFId][9]);
	Reg->df_outif_cm_coef2_012.val = df_read(dfreg_outif[OutIFId][10]);
	Reg->df_outif_cm_coef2_3.val   = df_read(dfreg_outif[OutIFId][11]);

	Reg->df_outif_cm_coef0_012.bits.iCoef00 = ColorModulatorCoeff[cm_index][0];
	Reg->df_outif_cm_coef0_012.bits.iCoef01 = ColorModulatorCoeff[cm_index][1];
	Reg->df_outif_cm_coef0_012.bits.iCoef02 = ColorModulatorCoeff[cm_index][2];
	Reg->df_outif_cm_coef0_3.bits.iCoef03   = ColorModulatorCoeff[cm_index][3];
	Reg->df_outif_cm_coef1_012.bits.iCoef10 = ColorModulatorCoeff[cm_index][4];
	Reg->df_outif_cm_coef1_012.bits.iCoef11 = ColorModulatorCoeff[cm_index][5];
	Reg->df_outif_cm_coef1_012.bits.iCoef12 = ColorModulatorCoeff[cm_index][6];
	Reg->df_outif_cm_coef1_3.bits.iCoef13   = ColorModulatorCoeff[cm_index][7];
	Reg->df_outif_cm_coef2_012.bits.iCoef20 = ColorModulatorCoeff[cm_index][8];
	Reg->df_outif_cm_coef2_012.bits.iCoef21 = ColorModulatorCoeff[cm_index][9];
	Reg->df_outif_cm_coef2_012.bits.iCoef22 = ColorModulatorCoeff[cm_index][10];
	Reg->df_outif_cm_coef2_3.bits.iCoef23   = ColorModulatorCoeff[cm_index][11];

	df_write(dfreg_outif[OutIFId][6], Reg->df_outif_cm_coef0_012.val);
	df_write(dfreg_outif[OutIFId][7], Reg->df_outif_cm_coef0_3.val);
	df_write(dfreg_outif[OutIFId][8], Reg->df_outif_cm_coef1_012.val);
	df_write(dfreg_outif[OutIFId][9], Reg->df_outif_cm_coef1_3.val);
	df_write(dfreg_outif[OutIFId][10], Reg->df_outif_cm_coef2_012.val);
	df_write(dfreg_outif[OutIFId][11], Reg->df_outif_cm_coef2_3.val);

	return;
}
void DFOutIFSetCustomColorModulator(int OutIFId, unsigned int *MCoeff)
{
	df_outif_reg *Reg= NULL;

	OutIFId = OutIFId & 0x1;
	Reg = &dfreg.OutIF[OutIFId];

	Reg->df_outif_cm_coef0_012.val = df_read(dfreg_outif[OutIFId][6]);
	Reg->df_outif_cm_coef0_3.val   = df_read(dfreg_outif[OutIFId][7]);
	Reg->df_outif_cm_coef1_012.val = df_read(dfreg_outif[OutIFId][8]);
	Reg->df_outif_cm_coef1_3.val   = df_read(dfreg_outif[OutIFId][9]);
	Reg->df_outif_cm_coef2_012.val = df_read(dfreg_outif[OutIFId][10]);
	Reg->df_outif_cm_coef2_3.val   = df_read(dfreg_outif[OutIFId][11]);

	Reg->df_outif_cm_coef0_012.bits.iCoef00 = MCoeff[0];
	Reg->df_outif_cm_coef0_012.bits.iCoef01 = MCoeff[1];
	Reg->df_outif_cm_coef0_012.bits.iCoef02 = MCoeff[2];
	Reg->df_outif_cm_coef0_3.bits.iCoef03   = MCoeff[3];
	Reg->df_outif_cm_coef1_012.bits.iCoef10 = MCoeff[4];
	Reg->df_outif_cm_coef1_012.bits.iCoef11 = MCoeff[5];
	Reg->df_outif_cm_coef1_012.bits.iCoef12 = MCoeff[6];
	Reg->df_outif_cm_coef1_3.bits.iCoef13   = MCoeff[7];
	Reg->df_outif_cm_coef2_012.bits.iCoef20 = MCoeff[8];
	Reg->df_outif_cm_coef2_012.bits.iCoef21 = MCoeff[9];
	Reg->df_outif_cm_coef2_012.bits.iCoef22 = MCoeff[10];
	Reg->df_outif_cm_coef2_3.bits.iCoef23   = MCoeff[11];

	df_write(dfreg_outif[OutIFId][6], Reg->df_outif_cm_coef0_012.val);
	df_write(dfreg_outif[OutIFId][7], Reg->df_outif_cm_coef0_3.val);
	df_write(dfreg_outif[OutIFId][8], Reg->df_outif_cm_coef1_012.val);
	df_write(dfreg_outif[OutIFId][9], Reg->df_outif_cm_coef1_3.val);
	df_write(dfreg_outif[OutIFId][10], Reg->df_outif_cm_coef2_012.val);
	df_write(dfreg_outif[OutIFId][11], Reg->df_outif_cm_coef2_3.val);

	return;
}

void DFSetOutIFVideoFmt(int OutIFId, DF_VIDEO_FORMAT VideoFmt)
{
	//For Safety Format Switching It's better Disable the output clk First
	df_outif_reg *OutIFReg= NULL;

	OutIFId = OutIFId & 0x1;
	OutIFReg = &dfreg.OutIF[OutIFId];

	OutIFReg->df_outif_control.val = df_read(dfreg_outif[OutIFId][0]);
	OutIFReg->df_outif_x_size.val = df_read(dfreg_outif[OutIFId][1]);
	OutIFReg->df_outif_y_size.val = df_read(dfreg_outif[OutIFId][2]);
	OutIFReg->df_outif_hsync.val = df_read(dfreg_outif[OutIFId][3]);
	OutIFReg->df_outif_vsync.val = df_read(dfreg_outif[OutIFId][4]);

	OutIFReg->df_outif_control.bits.iColorSpace =!outputFormatInfo[VideoFmt].mIsYUVorRGB;
	OutIFReg->df_outif_control.bits.iIsHD        =outputFormatInfo[VideoFmt].mIsHD;
	OutIFReg->df_outif_control.bits.iIsPal      =outputFormatInfo[VideoFmt].mPALFmt;
	OutIFReg->df_outif_control.bits.iRepeatTimes =outputFormatInfo[VideoFmt].mVidrptr;
	OutIFReg->df_outif_control.bits.iIsInterlaced     =outputFormatInfo[VideoFmt].mVidType;
	OutIFReg->df_outif_control.bits.iYCMux =outputFormatInfo[VideoFmt].mYCMuxEn;
	OutIFReg->df_outif_control.bits.iHSyncPolarity    =outputFormatInfo[VideoFmt].mHSyncPol;
	OutIFReg->df_outif_control.bits.iVSyncPolarity    =outputFormatInfo[VideoFmt].mVSyncPol;
	OutIFReg->df_outif_hsync.bits.iHSyncBackProch    =outputFormatInfo[VideoFmt].mHSyncBP;
	OutIFReg->df_outif_hsync.bits.iHSyncFrontProch     =outputFormatInfo[VideoFmt].mHSyncFP;
	OutIFReg->df_outif_hsync.bits.iHSyncWidth  = outputFormatInfo[VideoFmt].mHSyncWidth;
	OutIFReg->df_outif_x_size.bits.iXActiveStart   =outputFormatInfo[VideoFmt].mHActive;
	OutIFReg->df_outif_x_size.bits.iXTotal      =outputFormatInfo[VideoFmt].mHRes;
	OutIFReg->df_outif_vsync.bits.iVSyncFrontProch     =outputFormatInfo[VideoFmt].mVSyncFP;
	OutIFReg->df_outif_vsync.bits.iVSyncWidth  =outputFormatInfo[VideoFmt].mVSyncWidth;
	OutIFReg->df_outif_vsync.bits.iVSyncBackProch     =outputFormatInfo[VideoFmt].mVSyncBP;
	OutIFReg->df_outif_y_size.bits.iYActiveStart  =outputFormatInfo[VideoFmt].mVActline;
	OutIFReg->df_outif_y_size.bits.iYTotal      =outputFormatInfo[VideoFmt].mVRes;
	OutIFReg->df_outif_control.bits.iChoromaDrop = 1;
	OutIFReg->df_outif_control.bits.iTopOrFrameIntEna = 1;
	OutIFReg->df_outif_control.bits.iBotIntEna = 1;
	OutIFReg->df_outif_control.bits.iDispEna = 1;
	OutIFReg->df_outif_control.bits.iColorModulator = !outputFormatInfo[VideoFmt].mIsYUVorRGB;

	df_write(dfreg_outif[OutIFId][0], dfreg.OutIF[OutIFId].df_outif_control.val);
	df_write(dfreg_outif[OutIFId][1], dfreg.OutIF[OutIFId].df_outif_x_size.val);
	df_write(dfreg_outif[OutIFId][2], dfreg.OutIF[OutIFId].df_outif_y_size.val);
	df_write(dfreg_outif[OutIFId][3], dfreg.OutIF[OutIFId].df_outif_hsync.val);
	df_write(dfreg_outif[OutIFId][4], dfreg.OutIF[OutIFId].df_outif_vsync.val);

	return;
}

void DFOutIFEna(int OutIFId, int IsEna)
{
	df_outif_reg *OutIFReg= NULL;

	OutIFId = OutIFId & 0x1;
	OutIFReg = &dfreg.OutIF[OutIFId];
	OutIFReg->df_outif_control.val = df_read(dfreg_outif[OutIFId][0]);

	OutIFReg->df_outif_control.bits.iDispEna = IsEna;

	df_write(dfreg_outif[OutIFId][0], OutIFReg->df_outif_control.val);

	return;
}

void DFSetZOrder(int OutIFId, int Gfx1ZOrder, int Gfx2ZOrder, int Video1ZOrder, int Video2ZOrder)
{
	df_compositor_reg *Reg= NULL;

	OutIFId = OutIFId & 0x1;
	Reg = &dfreg.Comp[OutIFId];
	Reg->df_comp_z_order.val = df_read(dfreg_comp[OutIFId][1]);

	Reg->df_comp_z_order.bits.iGfx1ZOrder   = Gfx1ZOrder;
	Reg->df_comp_z_order.bits.iGfx2ZOrder   = Gfx2ZOrder;
	Reg->df_comp_z_order.bits.iVideo1ZOrder = Video1ZOrder;
	Reg->df_comp_z_order.bits.iVideo2ZOrder = Video2ZOrder;

	df_write(dfreg_comp[OutIFId][1], Reg->df_comp_z_order.val);
	return;
}

void DFSetBackGroud(int OutIFId, unsigned char BGY, unsigned char BGU, unsigned char BGV)
{
	df_compositor_reg *Reg= NULL;

	OutIFId = OutIFId & 0x1;
	Reg = &dfreg.Comp[OutIFId];
	Reg->df_comp_back_ground.val = df_read(dfreg_comp[OutIFId][0]);

	Reg->df_comp_back_ground.bits.iBGY = BGY;
	Reg->df_comp_back_ground.bits.iBGU = BGU;
	Reg->df_comp_back_ground.bits.iBGV = BGV;

	df_write(dfreg_comp[OutIFId][0], Reg->df_comp_back_ground.val);
	return;
}

void DFOutIFClipCfg(int OutIFId, int iClipEna, unsigned char  iClipYLow, unsigned char  iClipYRange, unsigned char  iClipCRange)
{
	OutIFId = OutIFId & 0x1;
	dfreg.OutIF[OutIFId].df_outif_clip.val = df_read(dfreg_outif[OutIFId][5]);

	dfreg.OutIF[OutIFId].df_outif_clip.bits.iClipEnable    = iClipEna;
	dfreg.OutIF[OutIFId].df_outif_clip.bits.iYLow   = iClipYLow;
	dfreg.OutIF[OutIFId].df_outif_clip.bits.iYRange = iClipYRange;
	dfreg.OutIF[OutIFId].df_outif_clip.bits.iCRange = iClipCRange;

	df_write(dfreg_outif[OutIFId][5], dfreg.OutIF[OutIFId].df_outif_clip.val);
	return;
}

void DFVideoEna(int VideoId, int IsEna)
{
	df_video_reg *Reg= NULL;

	VideoId = VideoId & 0x3;
	Reg = &dfreg.Video[VideoId];
	Reg->df_video_control_reg.val = df_read(dfreg_video[VideoId][0]);

	Reg->df_video_control_reg.bits.iVideoEna = (IsEna? (1 << df_output_config.vid_output[VideoId]):0);

	DEBUG_PRINTF("layer_output.vid_output[%d] = %d\n",VideoId,df_output_config.vid_output[VideoId]);
	DEBUG_PRINTF("Reg->df_video_control_reg.bits.iVideoEna = %d\n",Reg->df_video_control_reg.bits.iVideoEna);

	if(IsEna)
		Reg->df_video_control_reg.bits.iAutoCorrectTopBottomField = 1;

	df_write(dfreg_video[VideoId][0], Reg->df_video_control_reg.val);

	return;
}

void DFVideoSetLayerAlpha(int VideoId, unsigned char Alpha, unsigned char KeyAlpha0, unsigned char KeyAlpha1)
{
	df_video_reg *Reg= NULL;

	VideoId = VideoId & 0x3;
	Reg = &dfreg.Video[VideoId];
	Reg->df_video_alpha_control_reg.val = df_read(dfreg_video[VideoId][1]);

	if(VideoId == 2){
		Reg->df_video_alpha_control_reg.bits.iDefaultAlpha = Alpha & 0xFF;
	}
	else{
		Reg->df_video_alpha_control_reg.bits.iDefaultAlpha = Alpha & 0xFF;
		Reg->df_video_alpha_control_reg.bits.iLumaKeyAlpha0 = KeyAlpha0;
		Reg->df_video_alpha_control_reg.bits.iLumaKeyAlpha1 = KeyAlpha1;
	}

	df_write(dfreg_video[VideoId][1], Reg->df_video_alpha_control_reg.val);

	return;
}

void DFVideoSetLumaKey(int VideoId, int IsKeyEna, unsigned char LumaKeyMin, unsigned char LumaKeyMax)
{
	df_video_reg *Reg= NULL;

	VideoId = VideoId & 0x3;

	if(VideoId == 2){
		DEBUG_PRINTF("AUX Video Layer do not support Luma!\n");
		return;
	}

	Reg = &dfreg.Video[VideoId];
	Reg->df_video_luma_key_reg.val = df_read(dfreg_video[VideoId][2]);
	Reg->df_video_control_reg.val = df_read(dfreg_video[VideoId][0]);

	if(IsKeyEna)
	{
		Reg->df_video_control_reg.bits.iLumaKeyEna = 1;
		Reg->df_video_luma_key_reg.bits.iLumaKeyMin = LumaKeyMin;
		Reg->df_video_luma_key_reg.bits.iLumaKeyMax = LumaKeyMax;
	}
	else
	{
		Reg->df_video_control_reg.bits.iLumaKeyEna = 0;
	}

	df_write(dfreg_video[VideoId][2], Reg->df_video_luma_key_reg.val);
	df_write(dfreg_video[VideoId][0], Reg->df_video_control_reg.val);

	return;
}

void DFVideoSetSrcCrop(int VideoId, df_rect CropRect)
{
	df_video_reg *Reg= NULL;
	df_videoaux_reg *AUXReg = NULL;

	if(VideoId == 2){
		AUXReg = & (dfreg.VideoAUX[VideoId]);
	}
	else{
		VideoId = VideoId & 0x1;
		Reg = &dfreg.Video[VideoId];

	}

	if(VideoId == 2){
		AUXReg->df_video_src_x_crop_reg.val = df_read(dfreg_video[VideoId][5]);
		AUXReg->df_video_src_y_crop_reg.val = df_read(dfreg_video[VideoId][6]);
	}
	else {
		Reg->df_video_src_x_crop_reg.val = df_read(dfreg_video[VideoId][5]);
		Reg->df_video_src_y_crop_reg.val = df_read(dfreg_video[VideoId][6]);
	}

	if(((CropRect.left&0xf) > 0x8)&&((CropRect.left&0xf) < 0x10)){
		CropRect.left = CropRect.left & (~0xF);
		CropRect.left += 0x10;
	}
	else if(((CropRect.left&0xf) <= 0x8)&&((CropRect.left&0xf) > 0)){
		CropRect.left = CropRect.left & (~0xF);
	}

	if(((CropRect.right&0xf) > 0x8)&&((CropRect.right&0xf) < 0x10)){
		CropRect.right = CropRect.right & (~0xF);
		CropRect.right += 0x10;
	}
	else if(((CropRect.right&0xf) <= 0x8)&&((CropRect.right&0xf) > 0)){
		CropRect.right = CropRect.right & (~0xF);
	}

	if(VideoId == 2){
		AUXReg->df_video_src_x_crop_reg.bits.iSrcCropXOff    = CropRect.left & (~0xF);
		//        AUXReg->df_video_src_x_crop_reg.bits.iSrcCropXWidth  = CropRect.right & (~0xF);
		AUXReg->df_video_src_y_crop_reg.bits.iSrcCropYOff    = CropRect.top & (~0x3);
		//  AUXReg->df_video_src_y_crop_reg.bits.iSrcCropYHeight = CropRect.bottom & (~0x3);

		df_write(dfreg_video[VideoId][5], AUXReg->df_video_src_x_crop_reg.val);
		df_write(dfreg_video[VideoId][6], AUXReg->df_video_src_y_crop_reg.val);
	}
	else {
		Reg->df_video_src_x_crop_reg.bits.iSrcCropXOff    = CropRect.left & (~0xF);
		Reg->df_video_src_x_crop_reg.bits.iSrcCropXWidth  = CropRect.right & (~0xF);
		Reg->df_video_src_y_crop_reg.bits.iSrcCropYOff    = CropRect.top & (~0x3);
		Reg->df_video_src_y_crop_reg.bits.iSrcCropYHeight = CropRect.bottom & (~0x3);

		df_write(dfreg_video[VideoId][5], Reg->df_video_src_x_crop_reg.val);
		df_write(dfreg_video[VideoId][6], Reg->df_video_src_y_crop_reg.val);
	}
	return;
}

void DFVideoSetDispWin(int VideoId, df_rect DispRect)
{
	df_video_reg *Reg= NULL;
	df_videoaux_reg *AUXReg = NULL;
	if(VideoId == 2){
		AUXReg = &dfreg.VideoAUX[VideoId];
		DispRect.top &= (~0x3);
		DispRect.bottom &= (~0x3);
		AUXReg->df_video_x_position_reg.val = df_read(dfreg_video[VideoId][3]);
		AUXReg->df_video_y_position_reg.val = df_read(dfreg_video[VideoId][4]);

		AUXReg->df_video_x_position_reg.bits.iDispXCropEnable = 0;
		AUXReg->df_video_x_position_reg.bits.iDispXStart = DispRect.left;
		AUXReg->df_video_x_position_reg.bits.iDispXEnd   = DispRect.right;
		AUXReg->df_video_y_position_reg.bits.iDispYStart = DispRect.top;
		AUXReg->df_video_y_position_reg.bits.iDispYEnd   = DispRect.bottom;
		//Any Error Check with Screen Size?

		df_write(dfreg_video[VideoId][3], AUXReg->df_video_x_position_reg.val);
		df_write(dfreg_video[VideoId][4], AUXReg->df_video_y_position_reg.val);

		AUXReg->df_video_x_position_reg.val = df_read(dfreg_video[VideoId][3]);
		AUXReg->df_video_y_position_reg.val = df_read(dfreg_video[VideoId][4]);


	}
	else{
		VideoId = VideoId & 0x1;
		Reg = &dfreg.Video[VideoId];
		DispRect.top &= (~0x3);
		DispRect.bottom &= (~0x3);
		Reg->df_video_x_position_reg.val = df_read(dfreg_video[VideoId][3]);
		Reg->df_video_y_position_reg.val = df_read(dfreg_video[VideoId][4]);

		Reg->df_video_x_position_reg.bits.iDispXCropEnable = 0;
		Reg->df_video_x_position_reg.bits.iDispXStart = DispRect.left;
		Reg->df_video_x_position_reg.bits.iDispXEnd   = DispRect.right;
		Reg->df_video_y_position_reg.bits.iDispYStart = DispRect.top;
		Reg->df_video_y_position_reg.bits.iDispYEnd   = DispRect.bottom;
		//Any Error Check with Screen Size?

		df_write(dfreg_video[VideoId][3], Reg->df_video_x_position_reg.val);
		df_write(dfreg_video[VideoId][4], Reg->df_video_y_position_reg.val);

		Reg->df_video_x_position_reg.val = df_read(dfreg_video[VideoId][3]);
		Reg->df_video_y_position_reg.val = df_read(dfreg_video[VideoId][4]);
	}


	return;
}

void DFVideoSetDispWinWithXCrop(int VideoId, df_rect DispRect, unsigned int DispXStartCrop, unsigned int DispXEndCrop)
{
	df_video_reg *Reg= NULL;

	VideoId = VideoId & 0x1;
	Reg = &dfreg.Video[VideoId];

	DispRect.top &= (~0x3);
	DispRect.bottom &= (~0x3);

	Reg->df_video_x_position_reg.val = df_read(dfreg_video[VideoId][3]);
	Reg->df_video_y_position_reg.val = df_read(dfreg_video[VideoId][4]);

	Reg->df_video_x_position_reg.bits.iDispXCropEnable   = 1;
	Reg->df_video_x_position_reg.bits.iDispXStartCropPixelNum = DispXStartCrop;
	Reg->df_video_x_position_reg.bits.iDispXEndCropPixelNum  = DispXEndCrop;
	Reg->df_video_x_position_reg.bits.iDispXStart = DispRect.left;
	Reg->df_video_x_position_reg.bits.iDispXEnd   = DispRect.left + DispRect.right;
	Reg->df_video_y_position_reg.bits.iDispYStart = DispRect.top;
	Reg->df_video_y_position_reg.bits.iDispYEnd   = DispRect.top + DispRect.bottom;
	//Any Error Check with Screen Size?

	df_write(dfreg_video[VideoId][3], Reg->df_video_x_position_reg.val);
	df_write(dfreg_video[VideoId][4], Reg->df_video_y_position_reg.val);

	return;
}

void DFGfxEna(int GfxId, int IsEna, int IsRGB2YUVEna)
{
	df_gfx_reg *Reg= NULL;

	GfxId   = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];

	Reg->df_gfx_control_reg.val = df_read(dfreg_gfx[GfxId][0]);

	if(DF_OUTPUT_VGA == df_output_config.output_mode[df_output_config.gfx_output[GfxId]]){
		IsRGB2YUVEna = 0;
	}
	else{
		IsRGB2YUVEna = 1;
	}

	if(IsEna)
	{
		Reg->df_gfx_control_reg.bits.iGfxEnable = (1 << df_output_config.gfx_output[GfxId]);
		Reg->df_gfx_control_reg.bits.iRGB2YUVConvertEna = IsRGB2YUVEna;
	}
	else
	{
		Reg->df_gfx_control_reg.bits.iGfxEnable = 0;
	}

	DEBUG_PRINTF("layer_output.gfx_output[%d] = %d\n",GfxId,df_output_config.gfx_output[GfxId]);
	DEBUG_PRINTF("Reg->df_gfx_control_reg.bits.iGfxEna = %d\n",Reg->df_gfx_control_reg.bits.iGfxEnable);

	df_write(dfreg_gfx[GfxId][0], Reg->df_gfx_control_reg.val);

	return;
}

void DFGfxSetAlpha(int GfxId, unsigned char DefaultAlpha, unsigned char ARGB1555Alpha0, unsigned char ARGB1555Alpha1)
{
	df_gfx_reg *Reg= NULL;

	GfxId   = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];

	Reg->df_gfx_alpha_control_reg.val = df_read(dfreg_gfx[GfxId][2]);

	Reg->df_gfx_alpha_control_reg.bits.iDefaultAlpha   = DefaultAlpha;
	Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha0 = ARGB1555Alpha0;
	Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha1 = ARGB1555Alpha1;

	df_write(dfreg_gfx[GfxId][2], Reg->df_gfx_alpha_control_reg.val);
	return;
}

void DFGfxGetAlpha(int GfxId, unsigned char *DefaultAlpha, unsigned char *ARGB1555Alpha0, unsigned char *ARGB1555Alpha1)
{
	df_gfx_reg *Reg= NULL;

	GfxId   = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];


	Reg->df_gfx_alpha_control_reg.val = df_read(dfreg_gfx[GfxId][2]);

	*DefaultAlpha = Reg->df_gfx_alpha_control_reg.bits.iDefaultAlpha;
	*ARGB1555Alpha0 = Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha0;
	*ARGB1555Alpha1 = Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha1;

	return;
}

void DFGfxSetColorKey(int GfxId, unsigned char KeyRMin, unsigned char KeyRMax, unsigned char KeyGMin, unsigned char KeyGMax, unsigned char KeyBMin, unsigned char KeyBMax)
{
	df_gfx_reg *Reg= NULL;

	GfxId   = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];

	Reg->df_gfx_key_red_reg.val = df_read(dfreg_gfx[GfxId][3]);
	Reg->df_gfx_key_blue_reg.val = df_read(dfreg_gfx[GfxId][4]);
	Reg->df_gfx_key_green_reg.val = df_read(dfreg_gfx[GfxId][5]);

	Reg->df_gfx_key_red_reg.bits.iKeyRedMin   = KeyRMin;
	Reg->df_gfx_key_red_reg.bits.iKeyRedMax   = KeyRMax;
	Reg->df_gfx_key_blue_reg.bits.iKeyBlueMin  = KeyBMin;
	Reg->df_gfx_key_blue_reg.bits.iKeyBlueMax  = KeyBMax;
	Reg->df_gfx_key_green_reg.bits.iKeyGreenMin = KeyGMin;
	Reg->df_gfx_key_green_reg.bits.iKeyGreenMax = KeyGMax;

	df_write(dfreg_gfx[GfxId][3], Reg->df_gfx_key_red_reg.val);
	df_write(dfreg_gfx[GfxId][4], Reg->df_gfx_key_blue_reg.val);
	df_write(dfreg_gfx[GfxId][5], Reg->df_gfx_key_green_reg.val);

	return;
}

void DFGfxColorKeyEna(int GfxId,int IsEna)
{
	df_gfx_reg*Reg = NULL;

	GfxId = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];
	Reg->df_gfx_control_reg.val = df_read(dfreg_gfx[GfxId][0]);

	Reg->df_gfx_control_reg.bits.iColorKeyEnable = IsEna;

	df_write(dfreg_gfx[GfxId][0], Reg->df_gfx_control_reg.val);

	return;
}



#if 1
#define VPhase    0
#define HPhase    0
#define Buffer_Depth  4	// 4
#define BUF_PITCH   (1024*704)
#define Y_Top_Addr   (HD2SD_DATA_REGION)
#define C_Top_Addr   (HD2SD_DATA_REGION + (BUF_PITCH * Buffer_Depth))

unsigned int hd2sd_Capture_width = 720;
unsigned int hd2sd_Capture_height  = 576;
unsigned int hd2sd_Scaler_width = 720;
unsigned int hd2sd_Scaler_height = 576;
unsigned int hd2sd_by_pass_sca = 0;
unsigned int hd2sd_is_frame = 0;
df_output_pos hd2sd_out_pos;

void DFHD2SDCaptureEna(int IsEnable, int OutIFId)
{
	df_hd2sd_reg *Reg= NULL;

	Reg = &dfreg.HD2SD;
	Reg->df_hd2sd_control.val = df_read(dfreg_hd2sd[0]);
	Reg->df_hd2sd_control.bits.iHD2SDEna = IsEnable;
	Reg->df_hd2sd_control.bits.iSourceSel = OutIFId&0x1;
	df_write(dfreg_hd2sd[0], Reg->df_hd2sd_control.val);
}

void DFHD2SDConfig(void)
{
	unsigned int reg = 0;

	df_hd2sd_reg *Reg= NULL;

	Reg = &dfreg.HD2SD;
	Reg->df_hd2sd_control.val = df_read(dfreg_hd2sd[0]);
	Reg->df_hd2sd_control.bits.iByPassScaler = hd2sd_by_pass_sca;
	Reg->df_hd2sd_control.bits.iIsFrame = hd2sd_is_frame;
	Reg->df_hd2sd_control.bits.iIsHD = (hd2sd_Capture_width > 1024);
	Reg->df_hd2sd_control.bits.iDramFIFODepthMinus1 = Buffer_Depth - 1;
	Reg->df_hd2sd_control.bits.iVInitPhase = VPhase;
	Reg->df_hd2sd_control.bits.iHInitPhase = HPhase;
	Reg->df_hd2sd_control.bits.iVerticalReverseStore = 0;
	Reg->df_hd2sd_control.bits.iHorizontalReverseStore = 0;
	df_write(dfreg_hd2sd[0], Reg->df_hd2sd_control.val);

	if (hd2sd_is_frame)
		reg = hd2sd_Capture_width | (hd2sd_Capture_height << 16);
	else
		reg = hd2sd_Capture_width | ((hd2sd_Capture_height >> 1) << 16);

	df_write(DISP_HD2SD_DES_SIZE, reg);

	reg = Y_Top_Addr >> 3;
	df_write(DISP_HD2SD_ADDR_Y, reg);

	reg = C_Top_Addr >> 3;
	df_write(DISP_HD2SD_ADDR_C, reg);

	reg = BUF_PITCH >> 3;
	df_write(DISP_HD2SD_BUF_PITCH, reg);

	if(0 == hd2sd_by_pass_sca){
		GenCoeff(hd2sd_Scaler_width, hd2sd_Capture_width, DF_HD2SD_Y_HFIR_COEFF_IDX);
		GenCoeff(hd2sd_Scaler_height, hd2sd_Capture_height, DF_HD2SD_Y_VFIR_COEFF_IDX);
	}

	return;
}
#endif

void __df_calc_video_position(TVOUT_MODE pre_mode,TVOUT_MODE cur_mode, df_output_pos *pos)
{
	switch(cur_mode){
		case TVOUT_MODE_576I:
		case TVOUT_MODE_576P:
		case TVOUT_MODE_PAL_N:
		case TVOUT_MODE_PAL_CN:
		case TVOUT_MODE_SECAM:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 576;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 720;
			break;
		case TVOUT_MODE_480I:
		case TVOUT_MODE_480P:
		case TVOUT_MODE_PAL_M:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 480;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 720;
			break;
		case TVOUT_MODE_720P50:
		case TVOUT_MODE_720P60:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 720;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1280;
			break;
		case TVOUT_MODE_1080I25:
		case TVOUT_MODE_1080I30:
		case TVOUT_MODE_1080P24:
		case TVOUT_MODE_1080P25:
		case TVOUT_MODE_1080P30:
		case TVOUT_MODE_1080P50:
		case TVOUT_MODE_1080P60:
		case TVOUT_RGB_1920X1080P30:
		case TVOUT_RGB_1920X1080P60:
		case TVOUT_RGB_1920X1080I30:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 1080;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1920;
			break;
		case TVOUT_RGB_640X480_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 480;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 640;
			break;
		case TVOUT_RGB_800X600_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 600;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 800;
			break;
		case TVOUT_RGB_1024X768_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 768;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1024;
			break;
#if 0
		case TVOUT_RGB_1280X1024_50FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 1024;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1280;
			break;
		
		case TVOUT_RGB_1600X1000_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 1000;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1600;
			break;
#endif
		case TVOUT_RGB_1280X720_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 720;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1280;
			break;
#if 0
		case TVOUT_RGB_848X480_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 848;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 480;
			break;
#endif
		case TVOUT_RGB_1280X1024_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 1024;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1280;
			break;
		case TVOUT_RGB_800X480_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 800;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 480;
			break;
		case TVOUT_RGB_1440X900_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 1440;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 900;
			break;
		case TVOUT_RGB_1366X768_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 768;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1366;
			break;

		default:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 576;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 720;
			break;
	}
}

int Get_Outif_Id(int gfx_id)
{
	return df_output_config.gfx_output[gfx_id];
}

int Get_Video_Id(int tve_id)
{
	if(tve_id == df_output_config.vid_output[0]) return 0;
	else if(tve_id == df_output_config.vid_output[1]) return 1;
	else if(tve_id == df_output_config.vid_output[2]) return 2;

	return -1;
}

int Get_Gfx_Id(int tve_id)
{
	if(tve_id == df_output_config.gfx_output[0]) return 0;
	else if(tve_id == df_output_config.gfx_output[1]) return 1;

	return -1;
}

EXPORT_SYMBOL(Get_Outif_Id);

#define HAND_SHAKE_WITH_VIDEO_FIRMWARE 0x23 //0x48 //0x23

static int csm_tve_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	unsigned int reg_val;
	int tveid;

	tveid = (minor_id == DF_MINOR_TVE1)? 1:0;

	switch (cmd) {
		case CSTVE_IOC_BIND_GFX:
			{
				df_output_config.gfx_output[arg] = tveid;
				DEBUG_PRINTF("layer_output.gfx_output[%d] = %d\n",(int)arg,df_output_config.gfx_output[arg]);
				break;
			}

		case CSTVE_IOC_BIND_VID:
			{
				df_output_config.vid_output[arg] = tveid;
				DEBUG_PRINTF("layer_output.vid_output[%d] = %d\n",(int)arg,df_output_config.vid_output[arg]);
				break;
			}

		case CSTVE_IOC_SET_OUTPUT:
			df_output_config.output_mode[tveid] = arg;
			DEBUG_PRINTF("layer_output.output_mode[%d] = %d\n",tveid,(int)arg);
			break;

		case CSTVE_IOC_ENABLE:
			if(tveid == 0){
				ClockDispSetDAC1Mode(_clock_wake);
				Clock_hdmi_reset(_do_set);
			}
			else if(tveid == 1)
				ClockDispSetDAC0Mode(_clock_wake);
			else
				printk("parameters errorn\n!");
			break;

		case CSTVE_IOC_DISABLE:
			if(tveid == 0){
				ClockDispSetDAC1Mode(_clock_sleep);
				Clock_hdmi_reset(_do_reset);
			}
			else if(tveid == 1)
				ClockDispSetDAC0Mode(_clock_sleep);
			else
				printk("parameters errorn\n!");
			break;

		case CSTVE_IOC_SET_MODE:
			{
				df_output_pos pos;
				int is_timeout = 0;
				unsigned int changemode_ack = 0;
				unsigned long long time_start = 0;
				int i = 0;
				int wait_times = 10;
//printk("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n\n\n");
				if(__Is_first){
					if(DF_OUTPUT_VGA == df_output_config.output_mode[tveid]){
						__df_update_start();
						DFOutIFEnaColorModulator(tveid, 1);
						DFOutIFSetColorModulator(tveid, YUV_HD_TO_RGB);
						__df_update_end();
						//					pinmux_set_dac1mode(DAC1_MODE Mode)
					}
					else{
						ClockDispSetDAC0Mode(_clock_wake);
						ClockDispSetDAC1Mode(_clock_wake);
						//					ClockDispSetClockMode0(_disp0, _YUV_PAL);
						//					ClockDispSetClockMode0(_disp1, _YUV_PAL);
						//					InitTVE0Raw(TVOUT_MODE_576I, 0);
						//					InitTVE1Raw(TVOUT_MODE_576I, 0);
						//					__df_update_start();
						//					DFSetOutIFVideoFmt(0, DISP_YUV_PAL);
						//					DFSetOutIFVideoFmt(1, DISP_YUV_PAL);
						//					__df_update_end();
					}
					__df_update_start();

//					DFGfxEna(0, 0, 0);
//					DFGfxEna(1, 0, 0);
//					DFGfxSetColorKey(0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
//					DFGfxColorKeyEna(0, 1);
//					DFGfxSetColorKey(1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
//					DFGfxColorKeyEna(1, 1);
//					DFVideoEna(0, 0);
					if(DF_OUTPUT_VGA == df_output_config.output_mode[tveid]){
						DFSetBackGroud(0, 0x0, 0x0, 0x0);
						DFOutIFClipCfg(0, 0, 16, 219, 224);
					}
					else{
						DFSetBackGroud(0, 0x10, 0x80, 0x80);
						DFOutIFClipCfg(0, 0, 16, 219, 224);
					}

					pos.src.top = 0;
					pos.src.bottom = 0;
					pos.src.left = 0;
					pos.src.right = 0;
					pos.dst.top = 0;
					pos.dst.bottom = 576;
					pos.dst.left = 0;
					pos.dst.right = 720;
					DFVideoSetSrcCrop(0, pos.src);
					DFVideoSetSrcCrop(1, pos.src);
					DFVideoSetDispWin(0, pos.dst);
					DFVideoSetDispWin(1, pos.dst);
					DFVideoEna(1, 0);
					DFOutIFClipCfg(1, 0, 16, 219, 224);

					__df_update_end();
				}

				switch (arg){
					case TVOUT_MODE_SECAM:
					case TVOUT_MODE_PAL_N:
					case TVOUT_MODE_PAL_M:
					case TVOUT_MODE_PAL_CN:
					case TVOUT_MODE_576I:
					case TVOUT_MODE_576P:
					case TVOUT_MODE_480I:
					case TVOUT_MODE_480P:
					case TVOUT_MODE_720P50:
					case TVOUT_MODE_720P60:
					case TVOUT_MODE_1080I25:
					case TVOUT_MODE_1080I30:
					case TVOUT_MODE_1080P24:
					case TVOUT_MODE_1080P25:
//					case TVOUT_MODE_1080P30:
					case TVOUT_MODE_1080P50:
					case TVOUT_MODE_1080P60:
						if(DF_OUTPUT_CVBS_SVIDEO == df_output_config.output_mode[tveid]){
							pinmux_set_dac1mode(_dac1_tve1_cvbs);
						}
						else{
							pinmux_set_dac1mode(_dac1_tve0_ypbpr);
						}
						__Is_TV = 1;
						break;
					case TVOUT_RGB_640X480_60FPS:
					case TVOUT_RGB_800X600_60FPS:
					case TVOUT_RGB_800X480_60FPS:
						__df_update_start();
						DFOutIFEnaColorModulator(tveid, 1);
						DFOutIFSetColorModulator(tveid, YUV_SD_TO_RGB);
						__df_update_end();
						pinmux_set_dac1mode(_dac1_disp0_rgb);
						__Is_TV = 0;
						break;

					case TVOUT_RGB_1024X768_60FPS:
					case TVOUT_RGB_1280X720_60FPS:
					case TVOUT_RGB_1440X900_60FPS:
					case TVOUT_RGB_1280X1024_60FPS:
					case TVOUT_RGB_1366X768_60FPS:
					case TVOUT_RGB_1920X1080P30:
					case TVOUT_RGB_1920X1080P60:
					case TVOUT_RGB_1920X1080I30:
						__df_update_start();
						DFOutIFEnaColorModulator(tveid, 1);
						DFOutIFSetColorModulator(tveid, YUV_HD_TO_RGB);
						__df_update_end();
						pinmux_set_dac1mode(_dac1_disp0_rgb);
						__Is_TV = 0;
						break;

					default:
						break;
				}
				DEBUG_PRINTF("tevid = %d, mode = %d\n",tveid,(int)arg);

				__df_calc_video_position(df_dev[minor_id].tve_format,arg,&pos);
				df_dev[minor_id].tve_format = arg;

				if(tveid == 0){
					changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
					time_start = get_jiffies_64();
					while((changemode_ack>>13)&0x7){
						changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						if((get_jiffies_64() - time_start) > 4){
							__video_write(changemode_ack&0xFFFF1FFF,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
							DEBUG_PRINTF("ERROR1: Timeout\n");
							is_timeout = 1;
							break;
						}
					}

					if(is_timeout == 0){
						__video_write(changemode_ack|0x2000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						time_start = get_jiffies_64();
						changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						while(((changemode_ack>>13)&0x7) != 0x2){
							if((get_jiffies_64() - time_start) > 50){
								__video_write(changemode_ack|0x6000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
								DEBUG_PRINTF("ERROR2: Timeout\n");
								is_timeout = 1;
								break;
							}
							changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						}
					}
				}
				else if(tveid == 1){
					tveid = 1;
					changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);

					time_start = get_jiffies_64();
					while((changemode_ack>>29)&0x7){
						changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						if((get_jiffies_64() - time_start) > 4){
							__video_write(changemode_ack&0x1fffffff,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
							DEBUG_PRINTF("ERROR1: Timeout\n");
							is_timeout = 1;
							break;
						}
					}

					if(is_timeout == 0){
						__video_write(changemode_ack|0x20000000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						time_start = get_jiffies_64();
						changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						while(((changemode_ack>>29)&0x7) != 0x2){
							if((get_jiffies_64() - time_start) > 50){
								__video_write(changemode_ack|0x60000000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
								DEBUG_PRINTF("ERROR2: Timeout\n");
								is_timeout = 1;
								break;
							}
							changemode_ack = __video_read(HAND_SHAKE_WITH_VIDEO_FIRMWARE);
						}
					}
				}
				else
					DEBUG_PRINTF("ERROR: No such device in orion_df. Minor = %d\n",minor_id);

				if(__Is_first){
#if BOOT_HAS_OUTPUT
					;
#else
					ClockDispSetClockMode0(tveid, tve_2_clock[df_dev[minor_id].tve_format]);
					mdelay(10);
#endif
				}
				else{
					ClockDispSetClockMode0(tveid, tve_2_clock[df_dev[minor_id].tve_format]);
					mdelay(10);
				}

				if(__Is_TV){
					if(tveid == 0){
						if(__Is_first){
#if BOOT_HAS_OUTPUT
							;
#else
							InitTVE0Raw(df_dev[minor_id].tve_format, __Is_ColorBar);		
#endif
						}
						else{
							InitTVE0Raw(df_dev[minor_id].tve_format, __Is_ColorBar);
						}
					}
					else if(tveid == 1)
						InitTVE1Raw(df_dev[minor_id].tve_format, __Is_ColorBar);

					mdelay(10);
				}

				__df_update_start();
				DFSetOutIFVideoFmt(tveid, tve_2_df[df_dev[minor_id].tve_format]);
				__df_update_end();
				mdelay(10);
				__df_update_start();
				DFVideoSetDefaultCoeff(Get_Video_Id(tveid));
				DFVideoSetDispWin(Get_Video_Id(tveid), pos.dst);
				__df_update_end();

				reg_val = __video_read(0x21);
				while(reg_val){
					wait_times--;
					udelay(1000);
					if (wait_times <= 0){
						printk("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					reg_val = __video_read(0x21);
					//				printk("111HOST_IF_VID_MIPS_MAILBOX_1 = 0x%08x(current stc = 0x%08x)\n", reg_val, __video_read(0xc));
				}

				for(i = 0;i<3;i++){
					if(df_output_config.vid_output[i] == tveid){
						host2videofw_if->video_display_layer[i].LayerPosition.x_start = pos.dst.left;
						host2videofw_if->video_display_layer[i].LayerPosition.x_end = pos.dst.right;
						host2videofw_if->video_display_layer[i].LayerPosition.y_start = pos.dst.top;
						host2videofw_if->video_display_layer[i].LayerPosition.y_end = pos.dst.bottom;
					}
				}

				__video_write(1<<16,0x21);

				if(tveid == 0){
					changemode_ack &= 0xfffffc07;
					changemode_ack |= (tve_2_df[df_dev[minor_id].tve_format]<<3);
					__video_write(changemode_ack|0x6000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
				}
				else if(tveid == 1){
					changemode_ack &= 0xfc07ffff;
					changemode_ack |= (tve_2_df[df_dev[minor_id].tve_format]<<19);
					__video_write(changemode_ack|0x60000000,HAND_SHAKE_WITH_VIDEO_FIRMWARE);
				}

				if((tveid == 0)&&(__Is_TV == 1)&&(__Is_hd2sd)){//((tveid == 0)&&(__Is_TV == 1))
					switch (df_dev[minor_id].tve_format){
						case TVOUT_MODE_SECAM:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 576;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_SECAM;
							break;
						case TVOUT_MODE_PAL_CN:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 576;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_CN;
							break;
						case TVOUT_MODE_576I:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 576;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_PAL_N:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 576;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_N;
							break;
						case TVOUT_MODE_480I:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 480;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_MODE_PAL_M:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 480;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_M;
							break;
						case TVOUT_MODE_1080I25:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800; // modify by ying @20100722
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_1080I30:
							hd2sd_is_frame = 0;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_MODE_576P:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 576;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_480P:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
							hd2sd_Scaler_width = 720;
							hd2sd_Scaler_height = 480;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_MODE_720P50:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1280;
							hd2sd_Scaler_height = 720;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_1080P24:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_720P60:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1280;
							hd2sd_Scaler_height = 720;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_MODE_1080P25:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_1080P30:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_MODE_1080P50:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 576;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_1080P60:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
#if	USE_VIDEO_LAYER2
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 720;
#else
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
#endif
							hd2sd_Scaler_width = 1920;
							hd2sd_Scaler_height = 1080;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_RGB_640X480_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 640;
							hd2sd_Scaler_width = 640;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 640;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_RGB_800X600_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 600;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 800;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_RGB_1024X768_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1024;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
#if 0
						case TVOUT_RGB_1280X1024_50FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1280;
							hd2sd_out_pos.dst.bottom = 576;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_RGB_1280X1024_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1280;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
						case TVOUT_RGB_1600X1000_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1600;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
#endif
						case TVOUT_RGB_1280X720_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1280;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
#if 0
						case TVOUT_RGB_848X480_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 848;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							//df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;
#endif
						case TVOUT_RGB_800X480_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 800;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;

						case TVOUT_RGB_1440X900_60FPS:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 0;
							hd2sd_Capture_height = 704;
							hd2sd_Capture_width = 800;
							hd2sd_Scaler_width = 1440;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 720;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;

						default:
							hd2sd_is_frame = 1;
							hd2sd_by_pass_sca = 1;
							hd2sd_Capture_height = 480;
							hd2sd_Capture_width = 640;
							hd2sd_Scaler_width = 1024;
							hd2sd_out_pos.dst.bottom = 480;
							hd2sd_out_pos.dst.right = 640;
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							DEBUG_PRINTF("kernel: TVOUT_MODE_576I\n");
							break;
					}

					ClockDispSetClockMode0(1, tve_2_clock[df_dev[minor_id+1].tve_format]);
					mdelay(10);
					__df_update_start();
					DFSetOutIFVideoFmt(1, tve_2_df[df_dev[minor_id+1].tve_format]);
					__df_update_end();
					mdelay(10);
					InitTVE1Raw(df_dev[minor_id+1].tve_format, __Is_ColorBar);
					mdelay(10);
					hd2sd_out_pos.dst.top = 0;
					hd2sd_out_pos.dst.left = 0;

					hd2sd_out_pos.src.top = 0;
					hd2sd_out_pos.src.bottom = hd2sd_Capture_height;
					hd2sd_out_pos.src.left = 0;
					hd2sd_out_pos.src.right = hd2sd_Capture_width;

					__df_update_start();

#if	USE_VIDEO_LAYER2
					DFVideoSetSrcCrop(2, hd2sd_out_pos.src);
					DFVideoSetDispWin(2, hd2sd_out_pos.dst);
					DFVideoSetLayerAlpha(2, 0xff, 0, 0);
					DFVideoEna(2, 1);
#else
					{
						int tmp = 0;

						tmp = df_read(dfreg_video[1][0]);
						if(hd2sd_Capture_width > 1920/2){
							tmp |= 0x10;
						}
						else{
							tmp &= ~0x10;
						}
						df_write(dfreg_video[1][0], tmp);
					}
					GenCoeff(hd2sd_Capture_width, hd2sd_out_pos.dst.right - hd2sd_out_pos.dst.left, DF_VIDEO2_Y_HFIR_COEFF_IDX);
					GenCoeff(hd2sd_Capture_height, hd2sd_out_pos.dst.bottom - hd2sd_out_pos.dst.top, DF_VIDEO2_Y_VFIR_COEFF_IDX);
					//			DFVideoSetDefaultCoeff(1);
					DFVideoSetSrcCrop(1, hd2sd_out_pos.src);
					DFVideoSetDispWin(1, hd2sd_out_pos.dst);
					DFVideoSetLayerAlpha(1, 0xff, 0, 0);
					DFVideoEna(1, 1);
#endif
					__df_update_end();

					__df_update_start();
					DFHD2SDConfig();
					__df_update_end();

					__df_update_start();
					DFHD2SDCaptureEna(1, 0);
					__df_update_end();
				}

				__Is_first = 0;

				break;
			}

		case CSTVE_IOC_GET_MODE:
			{
				__put_user(df_dev[minor_id].tve_format, (int __user *) arg);
				break;
			}

			/* SunHe added 2008/11/11 15:37 */
		case CSTVE_IOC_SET_WHILE_LEVEL:
			TVE_Write(tveid, 0x27, (arg>>8)&0x03);
			TVE_Write(tveid, 0x28, arg&0xff);
			break;

		case CSTVE_IOC_GET_WHILE_LEVEL:
			reg_val = TVE_Read(tveid, 0x27);
			reg_val <<=8;
			reg_val |= TVE_Read(tveid, 0x28);
			reg_val &=0x3ff;
			__put_user(reg_val, (int __user *) arg);
			break;

		case CSTVE_IOC_SET_BLACK_LEVEL:
			TVE_Write(tveid, 0x29, (arg>>8)&0x03);
			TVE_Write(tveid, 0x2a, arg&0xff);
			break;

		case CSTVE_IOC_GET_BLACK_LEVEL:
			reg_val = TVE_Read(tveid, 0x29);
			reg_val <<=8;
			reg_val |= TVE_Read(tveid, 0x2a);
			reg_val &=0x3ff;
			__put_user(reg_val, (int __user *) arg);
			break;

		case CSTVE_IOC_SET_SATURATION_LEVEL:
			TVE_Write(tveid, 0x23, arg&0xff);
			TVE_Write(tveid, 0x24, arg&0xff);
			break;

		case CSTVE_IOC_GET_SATURATION_LEVEL:
			reg_val = TVE_Read(tveid, 0x23);
			__put_user(reg_val, (int __user *) arg);
			break;
			/* SunHe added 2008/11/11 15:37 */

#if 0
		case CSDF_IOC_WSS_CTRL:
			spin_lock_irqsave(&orion_tve_lock, flags);
			WSS_Ctrl((VBI_WssStandard_t)arg);
			spin_unlock_irqrestore(&orion_tve_lock, flags);
			retval = 0;
			break;

		case CSDF_IOC_WSS_SETCONFIG:
			spin_lock_irqsave(&orion_tve_lock, flags);
			WSS_SetConfig((VBI_WssStandard_t)arg);
			spin_unlock_irqrestore(&orion_tve_lock, flags);
			retval = 0;
			break;
		case CSDF_IOC_WSS_SETINFO:
			spin_lock_irqsave(&orion_tve_lock, flags);
			VBI_SetWssInfo((VBI_WssInfo_t *)arg);
			spin_unlock_irqrestore(&orion_tve_lock, flags);
			retval = 0;
			break;

		case CSTVE_IOC_TTX_CTRL:
			spin_lock_irqsave(&csm_df_lock, flags);
			ttx_control(arg);
			Is_TTX = arg;
			spin_unlock_irqrestore(&csm_df_lock, flags);
			break;

		case CSTVE_IOC_TTX_SETCONFIG:
			spin_lock_irqsave(&csm_df_lock, flags);
			ttx_setconfig(arg);
			spin_unlock_irqrestore(&csm_df_lock, flags);
			break;

		case CSTVE_IOC_TTX_SETINFO:
			spin_lock_irqsave(&csm_df_lock, flags);
			VBI_SetTxtPage((TTX_Page_t *)arg);
			spin_unlock_irqrestore(&csm_df_lock, flags);
			break;
#endif

		case CSTVE_IOC_GET_BIND_INF:
			if (copy_to_user((void *) arg, (void *) &df_output_config, sizeof(DF_OUTPUT_CONFIG_t))) {
				return -EFAULT;
			}
			break;

		case CSDF_IOC_DISP_HD2SD_ENABLE:
			if(arg == 1)
				__Is_hd2sd = 1;
			else if(arg == 0)
			{
				__Is_hd2sd = 0;
				DFVideoEna(1, 0);
				DFHD2SDCaptureEna(0, 0);
			}
#if 1
			if((tveid == 0) && (__Is_hd2sd == 1))
			{
				switch (df_dev[minor_id].tve_format){
					case TVOUT_MODE_SECAM:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 576;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 576;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_SECAM;
						break;
					case TVOUT_MODE_PAL_CN:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 576;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 576;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_CN;
						break;
					case TVOUT_MODE_576I:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 576;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 576;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_PAL_N:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 576;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 576;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_N;
						break;
					case TVOUT_MODE_480I:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 480;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_MODE_PAL_M:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 480;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_M;
						break;
					case TVOUT_MODE_1080I25:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1920;
						hd2sd_Scaler_height = 1080;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_1080I30:
						hd2sd_is_frame = 0;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1920;
						hd2sd_Scaler_height = 1080;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_MODE_576P:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 576;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 576;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_480P:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 720;
						hd2sd_Scaler_width = 720;
						hd2sd_Scaler_height = 480;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_MODE_720P50:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1280;
						hd2sd_Scaler_height = 720;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_1080P24:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1920;
						hd2sd_Scaler_height = 1080;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_720P60:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1280;
						hd2sd_Scaler_height = 720;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_RGB_640X480_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 640;
						hd2sd_Scaler_width = 640;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 640;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_RGB_800X600_60FPS://duliguo
						//case TVOUT_RGB_800X600_72FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 600;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 800;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_RGB_1024X768_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1024;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
#if 0
					case TVOUT_RGB_1280X1024_50FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 1024;
						hd2sd_Scaler_width = 1280;
						hd2sd_out_pos.dst.bottom = 576;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_RGB_1280X1024_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 1024;
						hd2sd_Scaler_width = 1280;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
					case TVOUT_RGB_1600X1000_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 1024;
						hd2sd_Scaler_width = 1600;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
#endif
					case TVOUT_RGB_1280X720_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 1024;
						hd2sd_Scaler_width = 1280;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
#if 0
					case TVOUT_RGB_848X480_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 848;
						hd2sd_Scaler_width = 848;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						//df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;
#endif
					case TVOUT_RGB_800X480_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 800;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						//df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;

					case TVOUT_RGB_1440X900_60FPS:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 0;
						hd2sd_Capture_height = 704;
						hd2sd_Capture_width = 800;
						hd2sd_Scaler_width = 1440;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 720;
						//df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;

					default:
						hd2sd_is_frame = 1;
						hd2sd_by_pass_sca = 1;
						hd2sd_Capture_height = 480;
						hd2sd_Capture_width = 640;
						hd2sd_Scaler_width = 1024;
						hd2sd_out_pos.dst.bottom = 480;
						hd2sd_out_pos.dst.right = 640;
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						DEBUG_PRINTF("kernel: TVOUT_MODE_576I\n");
						break;
				}

				ClockDispSetClockMode0(1, tve_2_clock[df_dev[minor_id+1].tve_format]);
				mdelay(10);
				__df_update_start();
				DFSetOutIFVideoFmt(1, tve_2_df[df_dev[minor_id+1].tve_format]);
				__df_update_end();
				mdelay(10);
				InitTVE1Raw(df_dev[minor_id+1].tve_format, __Is_ColorBar);
				mdelay(10);
				hd2sd_out_pos.dst.top = 0;
				hd2sd_out_pos.dst.left = 0;

				hd2sd_out_pos.src.top = 0;
				hd2sd_out_pos.src.bottom = hd2sd_Capture_height;
				hd2sd_out_pos.src.left = 0;
				hd2sd_out_pos.src.right = hd2sd_Capture_width;

				__df_update_start();
#if	USE_VIDEO_LAYER2
				DFVideoSetSrcCrop(2, hd2sd_out_pos.src);
				DFVideoSetDispWin(2, hd2sd_out_pos.dst);
				DFVideoSetLayerAlpha(2, 0xff, 0, 0);
				DFVideoEna(2, 1);
#else
				{
					int tmp = 0;

					tmp = df_read(dfreg_video[1][0]);
					if(hd2sd_Capture_width > 1920/2){
						tmp |= 0x10;
					}
					else{
						tmp &= ~0x10;
					}
					df_write(dfreg_video[1][0], tmp);
				}
				GenCoeff(hd2sd_Capture_width, hd2sd_out_pos.dst.right - hd2sd_out_pos.dst.left, DF_VIDEO2_Y_HFIR_COEFF_IDX);
				GenCoeff(hd2sd_Capture_height, hd2sd_out_pos.dst.bottom - hd2sd_out_pos.dst.top, DF_VIDEO2_Y_VFIR_COEFF_IDX);
				//          DFVideoSetDefaultCoeff(1);
				DFVideoSetSrcCrop(1, hd2sd_out_pos.src);
				DFVideoSetDispWin(1, hd2sd_out_pos.dst);
				DFVideoSetLayerAlpha(1, 0xff, 0, 0);
				DFVideoEna(1, 1);
#endif

				__df_update_end();

				__df_update_start();
				DFHD2SDConfig();
				__df_update_end();

				__df_update_start();
				DFHD2SDCaptureEna(1, 0);
				__df_update_end();
			}
#endif
			break;

		case CSTVE_IOC_SET_COMP_CHAN:
			{
				unsigned char reg_val = 0;

				reg_val = __tve1_reg_read(0x4);
				if(CSVOUT_COMP_YVU== arg){
					pinmux_set_dac1mode(_dac1_tve0_ypbpr);
					reg_val |= 0x8;
				}
				else if(CSVOUT_COMP_RGB== arg){
					pinmux_set_dac1mode(_dac1_tve1_ypbpr);
					reg_val &= ~0x8;
				}
				_tve1_reg_write(0x4, reg_val);
			}
			break;

		case CSTVE_IOC_SET_COLOR_MODULATOR_COEF:
			{
				unsigned int val[3][4];
				copy_from_user(&val, (void*)arg,sizeof(val));
				__df_update_start();
				DFOutIFEnaColorModulator(tveid,1);
				DFOutIFSetCustomColorModulator(tveid,&val[0][0]);
				__df_update_end();
			}
			break;

        case CSDF_IOC_Z_ORDER:
            __df_update_start();
            df_write(dfreg_comp[tveid][1], arg);
            __df_update_end();
            break;

		default:
			break;
	}

	return 0;
}

static int csm_df_gfx_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	printk("This node(Gfx layer) is not used!\n");
	return 0;
}

static int csm_df_vid_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	int vid_id;

	if(DF_MINOR_VID0 == minor_id)
		vid_id = 0;
	else if(DF_MINOR_VID1 == minor_id)
		vid_id = 1;
	else if(DF_MINOR_VIDAUX == minor_id)
		vid_id = 2;
	else
		return 0;

	DEBUG_PRINTF("csm_df_vid_ioctl : df video layer id = %d\n",vid_id);

	switch (cmd) {
		case CSDF_IOC_DISP_VID_ON:
			DEBUG_PRINTF("CSDF_IOC_DISP_VID_ON\n");
			__df_update_start();
			DFVideoEna(vid_id, 1);
			__df_update_end();
			break;

		case CSDF_IOC_DISP_VID_OFF:
			DEBUG_PRINTF("CSDF_IOC_DISP_VID_OFF\n");
			__df_update_start();
			DFVideoEna(vid_id, 0);
			__df_update_end();
			break;

		case CSDF_IOC_DISP_VID_ALPHA:
			DEBUG_PRINTF("CSDF_IOC_DISP_VID_ALPHA\n");
			__df_update_start();
			DFVideoSetLayerAlpha(vid_id, arg, 0x0, 0x0);
			__df_update_end();
			break;

		case CSDF_IOC_DISP_POS:
		{
			df_output_pos out_pos;
			DEBUG_PRINTF("CSDF_IOC_DISP_POS\n");
			if (copy_from_user(&out_pos, (void *) arg, sizeof(df_output_pos))){
				DEBUG_PRINTF("__copy_from_user error\n");
				return -EFAULT;
			}

			DEBUG_PRINTF("src : %d,%d,%d,%d\n",out_pos.src.left,out_pos.src.right,out_pos.src.top,out_pos.src.bottom);
			DEBUG_PRINTF("dst : %d,%d,%d,%d\n",out_pos.dst.left,out_pos.dst.right,out_pos.dst.top,out_pos.dst.bottom);
			__df_update_start();
			DFVideoSetSrcCrop(vid_id, out_pos.src);
			DFVideoSetDispWin(vid_id, out_pos.dst);
			__df_update_end();
			break;
		}

		default:
			DEBUG_PRINTF("unknown cmd 0x%x\n",cmd);
			break;
	}

	return 0;
}

static int csm_df_outif_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	printk("This node(outif) is not used!\n");
	return 0;
}

static int csm_df_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
	unsigned int minor_id = iminor(inode);

	if (minor_id == DF_MINOR_TVE0 || minor_id == DF_MINOR_TVE1){
		csm_tve_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_GFX0 || minor_id == DF_MINOR_GFX1){
		csm_df_gfx_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_VID0 || minor_id == DF_MINOR_VID1){
		csm_df_vid_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_VIDAUX){
		csm_df_vid_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_OUT0 || minor_id == DF_MINOR_OUT1){
		csm_df_outif_ioctl(cmd, arg, minor_id);
	}

	return 0;
}

int __df_mem_initialize(void)
{
	if (NULL == request_mem_region(DISP_REG_BASE, DISP_REG_SIZE, "csm1800 DF space")) {
		goto INIT_ERR0;
	}

	if (NULL == disp_base){
		if(!(disp_base = (unsigned char *)ioremap(DISP_REG_BASE, DISP_REG_SIZE))) {
			goto INIT_ERR1;
		}
	}

	if (NULL == request_mem_region(TVE0_REG_BASE, TVE0_REG_SIZE, "csm1800 TVE0 space")) {
		goto INIT_ERR2;
	}

	if (NULL == tve0_base){
		if(!(tve0_base = (unsigned char *)ioremap(TVE0_REG_BASE, TVE0_REG_SIZE))) {
			goto INIT_ERR3;
		}
	}

	if (NULL == request_mem_region(TVE1_REG_BASE, TVE1_REG_SIZE, "csm1800 TVE1 space")) {
		goto INIT_ERR4;
	}

	if (NULL == tve1_base){
		if(!(tve1_base = (unsigned char *)ioremap(TVE1_REG_BASE, TVE1_REG_SIZE))) {
			goto INIT_ERR5;
		}
	}

	DEBUG_PRINTF("disp_base 0x%x, tve0_base 0x%x, tve1_base 0x%x\n",(unsigned int)disp_base,(unsigned int)tve0_base,(unsigned int)tve1_base);
	return 0;

INIT_ERR5:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR5 ...\n");
	release_mem_region(TVE1_REG_BASE, TVE1_REG_SIZE);
INIT_ERR4:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR4 ...\n");
	iounmap((void *)tve0_base);
INIT_ERR3:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR3 ...\n");
	release_mem_region(TVE0_REG_BASE, TVE0_REG_SIZE);
INIT_ERR2:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR2 ...\n");
	iounmap((void *)disp_base);
INIT_ERR1:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR1 ...\n");
	release_mem_region(DISP_REG_BASE, DISP_REG_SIZE);
INIT_ERR0:
	printk(KERN_INFO "__df_mem_initialize INIT_ERR0 ...\n");
	return -1;
}

void __df_mem_destroy(void)
{
	iounmap((void *)tve1_base);
	release_mem_region(TVE1_REG_BASE, TVE1_REG_SIZE);
	iounmap((void *)tve0_base);
	release_mem_region(TVE0_REG_BASE, TVE0_REG_SIZE);
	iounmap((void *)disp_base);
	release_mem_region(DISP_REG_BASE, DISP_REG_SIZE);
}

static int df_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	const char *cmd_line = buffer;

	if (strncmp("tver1", cmd_line, 5) == 0) {
		int addr;
		addr = simple_strtol(&buffer[6], NULL, 16);
		printk(" tve1[0x%08x] = 0x%08x \n", addr&0xff, TVE_Read(1, addr&0xff));
	}
	else if (strncmp("tvew1", cmd_line, 5) == 0) {
		int addr, val;
		addr = simple_strtol(&buffer[6], NULL, 16);
		val = simple_strtol(&buffer[9], NULL, 16);
		__df_update_start();
		TVE_Write(1, addr&0xff, val&0xff);
		__df_update_end();
	}
	else if (strncmp("tver0", cmd_line, 5) == 0) {
		int addr;
		addr = simple_strtol(&buffer[6], NULL, 16);
		printk(" tve0[0x%08x] = 0x%08x \n", addr&0xff, TVE_Read(0, addr&0xff));
	}
	else if (strncmp("tvew0", cmd_line, 5) == 0) {
		int addr, val;
		addr = simple_strtol(&buffer[6], NULL, 16);
		val = simple_strtol(&buffer[9], NULL, 16);
		__df_update_start();
		TVE_Write(0, addr&0xff, val&0xff);
		__df_update_end();
	}
	else if (strncmp("rl", cmd_line, 2) == 0) {
		int addr;
		addr = simple_strtol(&buffer[3], NULL, 16);
		printk(" addr[0x%08x] = 0x%08x \n", addr, df_read(addr));
	}
	else if (strncmp("wl", cmd_line, 2) == 0) {
		int addr, val;
		addr = simple_strtol(&buffer[3], NULL, 16);
		val = simple_strtol(&buffer[12], NULL, 16);
		__df_update_start();
		df_write(addr, val);
		__df_update_end();
	}
	else if(strncmp("480i", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 480;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_NTSC);
		ClockDispSetClockMode0(_disp1, _YUV_NTSC);
		InitTVE0Raw(TVOUT_MODE_480I, 0);
		InitTVE1Raw(TVOUT_MODE_480I, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_NTSC);
		DFSetOutIFVideoFmt(1, DISP_YUV_NTSC);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("480p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 480;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_480P);
		ClockDispSetClockMode0(_disp1, _YUV_480P);
		InitTVE0Raw(TVOUT_MODE_480P, 0);
		InitTVE1Raw(TVOUT_MODE_480P, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_480P);
		DFSetOutIFVideoFmt(1, DISP_YUV_480P);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("576i", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_PAL);
		ClockDispSetClockMode0(_disp1, _YUV_PAL);
		InitTVE0Raw(TVOUT_MODE_576I, 0);
		InitTVE1Raw(TVOUT_MODE_576I, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_PAL);
		DFSetOutIFVideoFmt(1, DISP_YUV_PAL);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("576p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_576P);
		ClockDispSetClockMode0(_disp1, _YUV_576P);
		InitTVE0Raw(TVOUT_MODE_576P, 0);
		InitTVE1Raw(TVOUT_MODE_576P, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_576P);
		DFSetOutIFVideoFmt(1, DISP_YUV_576P);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("720p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_720P_60Hz);
		ClockDispSetClockMode0(_disp1, _YUV_720P_60Hz);
		InitTVE0Raw(TVOUT_MODE_720P60, 0);
		InitTVE1Raw(TVOUT_MODE_720P60, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_720P_60FPS);
		DFSetOutIFVideoFmt(1, DISP_YUV_720P_60FPS);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("1080p", cmd_line, 5) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _YUV_1080P_60Hz);
		ClockDispSetClockMode0(_disp1, _YUV_1080P_60Hz);
		//	InitTVE0Raw(TVOUT_MODE_1, 0);
		//	InitTVE1Raw(TVOUT_MODE_576P, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_1080P_60FPS);
		DFSetOutIFVideoFmt(1, DISP_YUV_1080P_60FPS);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("vga", cmd_line, 3) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		pinmux_set_dac1mode(_dac1_disp0_rgb);
		ClockDispSetDAC0Mode(_clock_wake);
		ClockDispSetDAC1Mode(_clock_wake);
		ClockDispSetClockMode0(_disp0, _RGB_CVT_1366x768_60Hz);
		//		ClockDispSetClockMode0(_disp1, _YUV_1080P_60Hz);
		//	InitTVE0Raw(TVOUT_MODE_1, 0);
		//	InitTVE1Raw(TVOUT_MODE_576P, 0);
		__df_update_start();
		DFOutIFEnaColorModulator(0, 1);
		DFOutIFSetColorModulator(0, YUV_HD_TO_RGB);
		DFSetOutIFVideoFmt(0, DISP_RGB_1366x768);
		//		DFSetOutIFVideoFmt(1, DISP_YUV_1080P_60FPS);
		DFVideoSetLayerAlpha(0, 0xff, 0xff, 0xff);
		DFVideoSetDispWin(0, DispRect);
		DFVideoEna(0, 1);
		DFVideoEna(1, 1);
		__df_update_end();

	}
	else if((strncmp("dumpreg",cmd_line,7) == 0)){
		unsigned int count = 0;

		for(count = 0; count < 0x400; count +=4){
			switch(count){
				case 0:
					printk("\n DISP_REG_COMMON\n");
					break;
				case 0x40:
					printk("\n DISP_REG_GFX1\n");
					break;
				case 0x80:
					printk("\n DISP_REG_GFX2\n");
					break;
				case 0xc0:
					printk("\n DISP_REG_VIDEO1\n");
					break;
				case 0x140:
					printk("\n DISP_REG_VIDEO2\n");
					break;
				case 0x1c0:
					printk("\n DISP_REG_VIDEO3\n");
					break;
				case 0x240:
					printk("\n DISP_REG_COMP1\n");
					break;
				case 0x280:
					printk("\n DISP_REG_COMP2\n");
					break;
				case 0x2c0:
					printk("\n DISP_REG_HD2SD\n");
					break;
				case 0x300:
					printk("\n DISP_REG_OUTIF1\n");
					break;
				case 0x340:
					printk("\n DISP_REG_OUTIF2\n");
					break;
			}
			printk(" addr[0x%08x] = 0x%08x \n", 0x41800000 + count, df_read(0x41800000+count));
		}

	}

	return count;
}

static int csm_df_open(struct inode *inode, struct file *file)
{
	unsigned int minor_id = iminor(inode);

	spin_lock_init(&(df_dev[minor_id].spin_lock));
	df_dev[minor_id].enable = 1;

	DEBUG_PRINTF("%s was opened\n",df_dev[minor_id].name);

	return 0;
}

static int csm_df_release(struct inode *inode, struct file *file)
{
	unsigned int minor_id = iminor(inode);

	df_dev[minor_id].enable = 0;

	DEBUG_PRINTF("%s was released\n",df_dev[minor_id].name);

	return 0;
}


static struct class *csm_df_class;
static struct proc_dir_entry *csm_df_proc_entry = NULL;
static struct file_operations csm_df_fops = {
	.owner	= THIS_MODULE,
	.open	=  csm_df_open,
	.release	= csm_df_release,
	.ioctl		= csm_df_ioctl,
};

CNC_FB_INTERRUPT csmdf_1800_vblank_int[2];
irqreturn_t csm_outif0_irq(int irq, void *dev_id)
{
	DF_IRQ_PRINTF("csm_outif0_irq!\n");

	if(csmdf_1800_vblank_int[0].is_display == 1){
		csmdf_1800_vblank_int[0].is_display = 0;
		wake_up(&csmdf_1800_vblank_int[0].wait);
	}

	DF_Write(DISP_INT_CLEAR, DF_Read(DISP_INT_CLEAR) |0x4);
	DF_Write(DISP_OUTIF1_INT_CLEAR, 0x0);

	return IRQ_HANDLED;
}

irqreturn_t csm_outif1_irq(int irq, void *dev_id)
{
	DF_IRQ_PRINTF("csm_outif1_irq!\n");

	if(csmdf_1800_vblank_int[1].is_display == 1){
		csmdf_1800_vblank_int[1].is_display = 0;
		wake_up(&csmdf_1800_vblank_int[1].wait);
	}

	DF_Write(DISP_INT_CLEAR, DF_Read(DISP_INT_CLEAR) |0x8);
	DF_Write(DISP_OUTIF2_INT_CLEAR, 0x0);

	return IRQ_HANDLED;
}


static void csm_df_setup_dev(unsigned int minor_id, char * dev_name)
{

	device_create(csm_df_class, NULL, MKDEV(DF_MAJOR, minor_id), NULL, dev_name);
	df_dev[minor_id].dev_minor = minor_id;
	df_dev[minor_id].dev_type = minor_id;
	df_dev[minor_id].enable = 0;
	df_dev[minor_id].tve_format = TVOUT_MODE_576I;
	df_dev[minor_id].name = dev_name;
}

static int __init csm_df_init(void)
{

	if (register_chrdev(DF_MAJOR, "csm_df", &csm_df_fops)){
		__df_mem_destroy();
		return -ENODEV;
	}

	csm_df_class = class_create(THIS_MODULE,"csm_df");
	if (IS_ERR(csm_df_class)){
		printk(KERN_ERR "DF: class create failed.\n");
		__df_mem_destroy();
		csm_df_class = NULL;
		return -ENODEV;
	}

	csm_df_setup_dev(DF_MINOR_GFX0,  "csm_df_gfx0");
	csm_df_setup_dev(DF_MINOR_GFX1,  "csm_df_gfx1" );
	csm_df_setup_dev(DF_MINOR_VID0,  "csm_df_video0");
	csm_df_setup_dev(DF_MINOR_VID1,  "csm_df_video1");
	csm_df_setup_dev(DF_MINOR_OUT0,  "csm_df_output0");
	csm_df_setup_dev(DF_MINOR_OUT1,  "csm_df_output1");
	csm_df_setup_dev(DF_MINOR_TVE0,  "csm_tvout0");
	csm_df_setup_dev(DF_MINOR_TVE1,  "csm_tvout1");
	csm_df_setup_dev(DF_MINOR_VIDAUX,  "csm_df_vidaux");

	__df_mem_initialize();
	CSM_DF_Clock_Init();

	DFOutIFSetColorModulator(0, YUV_HD_TO_RGB);
	DFOutIFSetColorModulator(1, YUV_HD_TO_RGB);

	csm_df_proc_entry = create_proc_entry("df18_io", 0, NULL);
	if (NULL != csm_df_proc_entry) {
		csm_df_proc_entry->write_proc = &df_proc_write;
	}

	if (request_irq(7, csm_outif0_irq, 0, "cs_df0", NULL)) {
		printk(KERN_ERR "csdrv_df: cannot register outif0 IRQ \n");
		return -EIO;
	}

	if (request_irq(25, csm_outif1_irq, 0, "cs_df1", NULL)) {
		printk(KERN_ERR "csdrv_df: cannot register outif1 IRQ \n");
		return -EIO;
	}

	printk("CSM1800 Display System Version : 0.1\n");

	printk(KERN_INFO "%s: CSM Display feeder driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"csm1800_df", DISP_REG_BASE, DISP_REG_SIZE);
	printk(KERN_INFO "%s: CSM TVE0 driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"csm1800_df", TVE0_REG_BASE, TVE0_REG_SIZE);
	printk(KERN_INFO "%s: CSM TVE1 driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"csm1800_df", TVE1_REG_BASE, TVE1_REG_SIZE);

	return 0;
}

inline static void csm_df_remove_dev(dev_t df_dev)
{
	device_destroy(csm_df_class,df_dev);
}

static void __exit csm_df_exit(void)
{
	__df_mem_destroy();

	if (NULL != csm_df_proc_entry)
		remove_proc_entry("df18_io", NULL);

	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_GFX0));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_GFX1));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VID0));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VID1));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_OUT0));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_OUT1));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_TVE0));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_TVE1));
	csm_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VIDAUX));

	class_destroy(csm_df_class);
	unregister_chrdev(DF_MAJOR, "csm_df");
	printk(KERN_INFO "CSM DF Exit ...... OK. \n");

	return;
}

postcore_initcall(csm_df_init);
// module_init(csm_df_init);

// module_exit(csm_df_exit);
MODULE_AUTHOR("Jia Ma, <jia.ma@celestialsemi.com>");
MODULE_DESCRIPTION("Celestial Semiconductor Display feeder sub-system");
MODULE_LICENSE("GPL");

