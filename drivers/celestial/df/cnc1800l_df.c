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
#include <mach/mux.h>
#include <mach/df_reg_def_1800l.h>
#include <mach/df_reg_fmt_1800l.h>
#include <mach/cnc1800l_df.h>
#include <mach/cnc1800l_power_clk.h>

#include "tve0_1800.h"
#include "tve1_1800.h"
#include "df_output_format.h"
#include "../video/cnc1800l_video.h"
#include "../video/video_reg_def_cnc1800l.h"

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

#ifdef CONFIG_CELESTIAL_TIGA_MINI
static int __Is_hd2sd = 0;
#else
static int __Is_hd2sd = 1;
#endif
static int __Is_ColorBar = 0;
static int __Is_first = 1;
int __VGA_Support = 0;
int __Is_TV = 1;

extern int outputmode;
extern _HOST_FW_IF_t *host2videofw_if;
extern unsigned int __video_write(unsigned int val, unsigned int addr);
extern unsigned int __video_read(unsigned int addr);
extern unsigned int clock_read_fixme(unsigned int idcs_reg_addr);

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
	DISP_RGB_1360x768,
	DISP_RGB_1920x1080P30,
	DISP_RGB_1920x1080P60,
	DISP_RGB_1920x1080I30,
	DISP_RGB_1366x768,
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
	_RGB_DMT_1360x768_60Hz,
	_YUV_1080P_30Hz,
	_YUV_1080P_60Hz,
	_YUV_1080I_60Hz,
	_RGB_CVT_1366x768_60Hz,
};

DEFINE_SPINLOCK(cnc_df_lock);

#define REG_MAP_ADDR(x)	(((((x)>>16)&0xffff) == 0xb180)  ? (disp_base+((x)&0xffff)) :  \
		(((((x)>>12)&0xfffff) == 0x80160) ? (tve1_base+((x)&0xfff)) :      \
		 ((((x)>>12)&0xfffff) == 0x80168) ? (tve0_base+((x)&0xfff)):0))

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
		return df_read8(0x80168000+addr);
	else if(tveid == 1)
		return df_read8(0x80160000+addr);
	else
		return (-1);
}

void TVE_Write(unsigned int tveid, unsigned int addr, unsigned char data)
{
	if(tveid == 0)
		df_write8((0x80168000+addr), data);
	else if(tveid == 1)
		df_write8((0x80160000+addr), data);
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
	if(data_rd != data){
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
	switch (DispFormat){
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

static int dfreg_ctrl[7] =
{
	DISP_UPDATE_REG,DISP_UPDATE_RT_REG,DISP_INT_STA_COP,DISP_INT_MSK_COP,DISP_INT_STA_HOST,DISP_INT_MSK_HOST,
	DISP_ARBITOR_QUANTUM
};

static int dfreg_gfx[2][10] =
{
	{DISP_GFX1_CTRL,DISP_GFX1_FORMAT,DISP_GFX1_ALPHA_CTRL,DISP_GFX1_KEY_RED,DISP_GFX1_KEY_BLUE,DISP_GFX1_KEY_GREEN,
		DISP_GFX1_BUF_START,DISP_GFX1_LINE_PITCH,DISP_GFX1_X_POSITON,DISP_GFX1_Y_POSITON,
	},
	{DISP_GFX2_CTRL,DISP_GFX2_FORMAT,DISP_GFX2_ALPHA_CTRL,DISP_GFX2_KEY_RED,DISP_GFX2_KEY_BLUE,DISP_GFX2_KEY_GREEN,
		DISP_GFX2_BUF_START,DISP_GFX2_LINE_PITCH,DISP_GFX2_X_POSITON,DISP_GFX2_Y_POSITON,
	},
};

static int dfreg_video0[41] =
{
	DISP_VIDEO1_CTRL,DISP_VIDEO1_ALPHA_CTRL,DISP_VIDEO1_X_POSITON,DISP_VIDEO1_Y_POSITON,DISP_VIDEO1_SRC_X_CROP,DISP_VIDEO1_SRC_Y_CROP,
	DISP_VIDEO1_LAI_PARAM,DISP_VIDEO1_YV_RATIO,DISP_VIDEO1_YH_RATIO,DISP_VIDEO1_CV_RATIO,DISP_VIDEO1_CH_RATIO,DISP_VIDEO1_Y_NOLINE_D,
	DISP_VIDEO1_C_NOLINE_D,DISP_VIDEO1_Y_NOLINE_K,DISP_VIDEO1_C_NOLINE_K,DISP_VIDEO1_HOR_PHASE,DISP_VIDEO1_VER_PHASE,
	DISP_VIDEO1_CUR_YT_ADDR,DISP_VIDEO1_CUR_CT_ADDR,DISP_VIDEO1_CUR_YB_ADDR,DISP_VIDEO1_CUR_CB_ADDR,DISP_VIDEO1_PRE_YT_ADDR,
	DISP_VIDEO1_PRE_CT_ADDR,DISP_VIDEO1_PRE_YB_ADDR,DISP_VIDEO1_PRE_CB_ADDR,DISP_VIDEO1_POS_YT_ADDR,DISP_VIDEO1_POS_CT_ADDR,
	DISP_VIDEO1_POS_YB_ADDR,DISP_VIDEO1_POS_CB_ADDR,DISP_VIDEO1_PPR_YT_ADDR,DISP_VIDEO1_PPR_CT_ADDR,DISP_VIDEO1_PPR_YB_ADDR,
	DISP_VIDEO1_PPR_CB_ADDR,DISP_VIDEO1_STORE_Y_ADDR,DISP_VIDEO1_STORE_C_ADDR,DISP_VIDEO1_SRC_CTRL,DISP_VIDEO1_COEFF_UP,
	DISP_VIDEO1_COEFF_ADDR,DISP_VIDEO1_F2F_START,DISP_VIDEO1_STA_F2F,DISP_VIDEO1_STA_DISP_NUM
};

static int dfreg_video1[9] =
{
	DISP_VIDEO2_CTRL,DISP_VIDEO2_X_POSITON,DISP_VIDEO2_Y_POSITON,DISP_VIDEO2_YT_ADDR,DISP_VIDEO2_CT_ADDR,DISP_VIDEO2_YB_ADDR,
	DISP_VIDEO2_CB_ADDR,DISP_VIDEO2_SRC_CTRL,DISP_VIDEO2_STA_DISP_NUM
};

static int dfreg_video2[33] =
{
	DISP_VIDEO3_CTRL,DISP_VIDEO3_X_POSITON,DISP_VIDEO3_Y_POSITON,DISP_VIDEO3_SRC_X_CROP,DISP_VIDEO3_SRC_Y_CROP,DISP_VIDEO3_YV_RATIO,
	DISP_VIDEO3_YH_RATIO,DISP_VIDEO3_CV_RATIO,DISP_VIDEO3_CH_RATIO,DISP_VIDEO3_Y_NOLINE_D,DISP_VIDEO3_C_NOLINE_D,
	DISP_VIDEO3_Y_NOLINE_K,DISP_VIDEO3_C_NOLINE_K,DISP_VIDEO3_HOR_PHASE,DISP_VIDEO3_VER_PHASE,DISP_VIDEO3_YT_ADDR,
	DISP_VIDEO3_CT_ADDR,DISP_VIDEO3_YB_ADDR,DISP_VIDEO3_CB_ADDR,DISP_VIDEO3_SRC_CTRL,DISP_VIDEO3_STA_DISP_NUM
};

static int dfreg_comp[5] =
{
	DISP_COMP_BACK_GROUND,DISP_COMP_Z_ORDER,DISP_COMP_COLOR_ADJUST,DISP_COMP_SHARP_Y_CTRL,DISP_COMP_SHARP_C_CTRL
};

static int dfreg_hd2sd[20] =
{
	DISP_HD2SD_CTRL,DISP_HD2SD_DES_SIZE,DISP_HD2SD_YV_RATIO,DISP_HD2SD_YH_RATIO,DISP_HD2SD_CV_RATIO,DISP_HD2SD_CH_RATIO,
	DISP_HD2SD_Y_NOLINE_D,DISP_HD2SD_C_NOLINE_D,DISP_HD2SD_Y_NOLINE_K,DISP_HD2SD_C_NOLINE_K,DISP_HD2SD_HOR_PHASE,
	DISP_HD2SD_VER_PHASE,DISP_HD2SD_YT_ADDR,DISP_HD2SD_CT_ADDR,DISP_HD2SD_YB_ADDR,DISP_HD2SD_CB_ADDR,DISP_HD2SD_COEFF_UP,
	DISP_HD2SD_COEFF_ADDR,DISP_HD2SD_STA_STORE,DISP_HD2SD_STA_STORE_NUM
};

static int dfreg_outif[2][8] =
{
	{DISP_OUTIF1_CTRL,DISP_OUTIF1_X_SIZE,DISP_OUTIF1_Y_SIZE,DISP_OUTIF1_HSYNC,DISP_OUTIF1_VSYNC,
		DISP_OUTIF1_VGA_LEVEL,DISP_OUTIF1_STA_DISP_SIZE,DISP_OUTIF1_STA_LINE
	},
	{DISP_OUTIF2_CTRL,DISP_OUTIF2_X_SIZE,DISP_OUTIF2_Y_SIZE,DISP_OUTIF2_HSYNC,DISP_OUTIF2_VSYNC,
		DISP_OUTIF2_VGA_LEVEL,DISP_OUTIF2_STA_DISP_SIZE,DISP_OUTIF2_STA_LINE
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

void DFSetOutIFVideoFmt(int OutIFId, DF_VIDEO_FORMAT VideoFmt)
{
	//For Safety Format Switching It's better Disable the output clk First
	df_outif_reg *OutIFReg= NULL;

	OutIFId = OutIFId & 0x1;
	OutIFReg = &dfreg.OutIF[OutIFId];

	OutIFReg->df_outif_control_reg.val = df_read(dfreg_outif[OutIFId][0]);
	OutIFReg->df_outif_x_size_reg.val = df_read(dfreg_outif[OutIFId][1]);
	OutIFReg->df_outif_y_size_reg.val = df_read(dfreg_outif[OutIFId][2]);
	OutIFReg->df_outif_hsync_reg.val = df_read(dfreg_outif[OutIFId][3]);
	OutIFReg->df_outif_vsync_reg.val = df_read(dfreg_outif[OutIFId][4]);

	//	OutIFReg->df_outif_vga_level_reg = df_read(dfreg_outif[OutIFId][5]);
	OutIFReg->df_outif_control_reg.bits.iDispEn = 1; // majia add here for debug, need move to other place.
	//	OutIFReg->df_outif_control.bits.iColorSpace =!outputFormatInfo[VideoFmt].mIsYUVorRGB;
	OutIFReg->df_outif_control_reg.bits.iIsHD        =outputFormatInfo[VideoFmt].mIsHD;
	OutIFReg->df_outif_control_reg.bits.iPALFmt      =outputFormatInfo[VideoFmt].mPALFmt;
	//	OutIFReg->df_outif_control.bits.iRepeatTimes =outputFormatInfo[VideoFmt].mVidrptr;
	OutIFReg->df_outif_control_reg.bits.iIsInterlaced     =outputFormatInfo[VideoFmt].mVidType;
	//	OutIFReg->df_outif_control.bits.iYCMux =outputFormatInfo[VideoFmt].mYCMuxEn;
	OutIFReg->df_outif_control_reg.bits.iHSyncPol    =outputFormatInfo[VideoFmt].mHSyncPol;
	OutIFReg->df_outif_control_reg.bits.iVSyncPol    =outputFormatInfo[VideoFmt].mVSyncPol;
	OutIFReg->df_outif_hsync_reg.bits.iHSyncBP    =outputFormatInfo[VideoFmt].mHSyncBP;
	OutIFReg->df_outif_hsync_reg.bits.iHSyncFP     =outputFormatInfo[VideoFmt].mHSyncFP;
	OutIFReg->df_outif_hsync_reg.bits.iHSyncWidth  = outputFormatInfo[VideoFmt].mHSyncWidth;
	OutIFReg->df_outif_x_size_reg.bits.iActPixNum   =outputFormatInfo[VideoFmt].mHActive;
	OutIFReg->df_outif_x_size_reg.bits.iHTotal      =outputFormatInfo[VideoFmt].mHRes;
	OutIFReg->df_outif_vsync_reg.bits.iVSyncFP     =outputFormatInfo[VideoFmt].mVSyncFP;
	OutIFReg->df_outif_vsync_reg.bits.iVSyncWidth  =outputFormatInfo[VideoFmt].mVSyncWidth;
	OutIFReg->df_outif_vsync_reg.bits.iVSyncBP     =outputFormatInfo[VideoFmt].mVSyncBP;
	OutIFReg->df_outif_y_size_reg.bits.iActLineNum  =outputFormatInfo[VideoFmt].mVActline;
	OutIFReg->df_outif_y_size_reg.bits.iVTotal      =outputFormatInfo[VideoFmt].mVRes;
	OutIFReg->df_outif_control_reg.bits.iDownMode    	=1;
	OutIFReg->df_outif_control_reg.bits.iDitherMode      =   0;
	OutIFReg->df_outif_control_reg.bits.iDitherEna       =   1;
	OutIFReg->df_outif_vga_level_reg.bits.iBlackLevel	=	252;
	OutIFReg->df_outif_vga_level_reg.bits.iWhiteLevel    =   800;
#if 0
	OutIFReg->df_outif_control.bits.iChoromaDrop = 1;
	OutIFReg->df_outif_control.bits.iTopOrFrameIntEna = 1;
	OutIFReg->df_outif_control.bits.iBotIntEna = 1;
	OutIFReg->df_outif_control.bits.iDispEna = 1;
	OutIFReg->df_outif_control.bits.iColorModulator = !outputFormatInfo[VideoFmt].mIsYUVorRGB;
#endif

	df_write(dfreg_outif[OutIFId][0], dfreg.OutIF[OutIFId].df_outif_control_reg.val);
	df_write(dfreg_outif[OutIFId][1], dfreg.OutIF[OutIFId].df_outif_x_size_reg.val);
	df_write(dfreg_outif[OutIFId][2], dfreg.OutIF[OutIFId].df_outif_y_size_reg.val);
	df_write(dfreg_outif[OutIFId][3], dfreg.OutIF[OutIFId].df_outif_hsync_reg.val);
	df_write(dfreg_outif[OutIFId][4], dfreg.OutIF[OutIFId].df_outif_vsync_reg.val);

	if(OutIFId == 0){
		df_write(dfreg_outif[OutIFId][5], dfreg.OutIF[OutIFId].df_outif_vga_level_reg.val);
	}

	return;
}

void DFOutIFEna(int OutIFId, int IsEna)
{
	df_outif_reg *OutIFReg= NULL;

	OutIFId = OutIFId & 0x1;
	OutIFReg = &dfreg.OutIF[OutIFId];
	OutIFReg->df_outif_control_reg.val = df_read(dfreg_outif[OutIFId][0]);

	OutIFReg->df_outif_control_reg.bits.iDispEn = IsEna;

	df_write(dfreg_outif[OutIFId][0], OutIFReg->df_outif_control_reg.val);

	return;
}

void DFSetZOrder(int OutIFId, int Gfx1ZOrder, int Gfx2ZOrder, int Video1ZOrder)
{
	df_compositor_reg *Reg= NULL;

	Reg = &dfreg.Comp;
	Reg->df_comp_z_order_reg.val = df_read(dfreg_comp[1]);

	Reg->df_comp_z_order_reg.bits.iGfx1ZOrder   = Gfx1ZOrder;
	Reg->df_comp_z_order_reg.bits.iGfx2ZOrder   = Gfx2ZOrder;
	Reg->df_comp_z_order_reg.bits.iVideo1ZOrder = Video1ZOrder;
	//	Reg->df_comp_z_order_reg.bits.iVideo2ZOrder = Video2ZOrder;

	df_write(dfreg_comp[1], Reg->df_comp_z_order_reg.val);
	return;
}

void DFSetBackGroud(int OutIFId, unsigned char BGY, unsigned char BGU, unsigned char BGV)
{
	df_compositor_reg *Reg= NULL;

	Reg = &dfreg.Comp;
	Reg->df_comp_back_ground_reg.val = df_read(dfreg_comp[0]);

	Reg->df_comp_back_ground_reg.bits.iBGY = BGY;
	Reg->df_comp_back_ground_reg.bits.iBGU = BGU;
	Reg->df_comp_back_ground_reg.bits.iBGV = BGV;

	df_write(dfreg_comp[0], Reg->df_comp_back_ground_reg.val);
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

void DFGfxSetAlpha(int GfxId, unsigned short DefaultAlpha, unsigned short ARGB1555Alpha0, unsigned short ARGB1555Alpha1)
{
	df_gfx_reg *Reg= NULL;

	GfxId   = GfxId & 0x1;
	Reg = &dfreg.Gfx[GfxId];

	Reg->df_gfx_alpha_control_reg.val = df_read(dfreg_gfx[GfxId][2]);

	Reg->df_gfx_alpha_control_reg.bits.iDefaultAlpha   = DefaultAlpha & 0x3ff;
	Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha0 = ARGB1555Alpha0 & 0x3ff;
	Reg->df_gfx_alpha_control_reg.bits.iArgb1555Alpha1 = ARGB1555Alpha1 & 0x3ff;

	df_write(dfreg_gfx[GfxId][2], Reg->df_gfx_alpha_control_reg.val);
	return;
}

void DFGfxGetAlpha(int GfxId, unsigned short *DefaultAlpha, unsigned short *ARGB1555Alpha0, unsigned short *ARGB1555Alpha1)
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

void DFHD2SDCaptureEna(int IsEnable)
{
	df_hd2sd_reg *Reg = NULL;

	Reg = &dfreg.HD2SD;
	Reg->df_hd2sd_control.val = df_read(dfreg_hd2sd[0]);
	Reg->df_hd2sd_control.bits.iHD2SDEna = IsEnable;
	df_write(dfreg_hd2sd[0], Reg->df_hd2sd_control.val);
}

void DFVideoEna(int VideoId, int IsEna)
{
	df_video1_reg *reg_video1 = NULL;
	df_video2_reg *reg_video2 = NULL;
	df_video3_reg *reg_video3 = NULL;

	switch(VideoId)
	{
		case 0:
			reg_video1 = &dfreg.Video1;
			reg_video1->df_video_control_reg.val = df_read(dfreg_video0[0]);
			reg_video1->df_video_control_reg.bits.iVideoEnable = IsEna;
			df_write(dfreg_video0[0], reg_video1->df_video_control_reg.val);
			break;
		case 1:
			reg_video2 = &dfreg.Video2;
			reg_video2->df_video_control_reg.val = df_read(dfreg_video1[0]);
			reg_video2->df_video_control_reg.bits.iVideoEnable = IsEna;
			df_write(dfreg_video1[0], reg_video2->df_video_control_reg.val);
			break;
		case 2:
			reg_video3 = &dfreg.Video3;
			reg_video3->df_video_control_reg.val = df_read(dfreg_video2[0]);
			reg_video3->df_video_control_reg.bits.iVideoEnable = IsEna;
			df_write(dfreg_video2[0], reg_video3->df_video_control_reg.val);
			break;
		default:
			break;
	}

	return;
}
#if 0
void DFVideoSetLayerAlpha(int VideoId, unsigned int Alpha)
{
	df_video1_reg *Reg = NULL;

	VideoId = VideoId & 0x3;
	if(VideoId){
		printk("Kernel : function %s : VideoId = %d\n",__FUNCTION__,VideoId);
		return;
	}

	Reg = &dfreg.Video1;
	Reg->df_video_alpha_control_reg.val = df_read(dfreg_video0[1]);
	Reg->df_video_alpha_control_reg.bits.iDefaultAlpha = Alpha & 0x3FF;
	df_write(dfreg_video0[1], Reg->df_video_alpha_control_reg.val);

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
		case TVOUT_RGB_1360X768_60FPS:
			pos->src.top = pos->dst.top = 0;
			pos->src.bottom = pos->dst.bottom = 768;
			pos->src.left = pos->dst.left = 0;
			pos->src.right = pos->dst.right = 1360;
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

#define HAND_SHAKE_WITH_VIDEO_FIRMWARE DF_STATUS_SWITCH_REG //0x48 //0x23

static int cnc_tve_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
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
#ifndef CONFIG_CELESTIAL_TIGA_MINI
				clock_dispset_dac1mode(_clock_wake);
#endif
				clock_hdmi_reset(_do_set);
			}
			else if(tveid == 1){
#ifndef CONFIG_HD2SD_ENABLE
				clock_dispset_dac0mode(_clock_wake);
#else
				;
#endif
			}
			else
				printk("parameters errorn\n!");
			break;

		case CSTVE_IOC_DISABLE:
			if(tveid == 0){
#ifndef CONFIG_CELESTIAL_TIGA_MINI
				clock_dispset_dac1mode(_clock_sleep);
#endif
				clock_hdmi_reset(_do_reset);
			}
			else if(tveid == 1){
#ifndef CONFIG_HD2SD_ENABLE
				clock_dispset_dac0mode(_clock_sleep);
#else
				;
#endif
			}
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
				if(__Is_first){
#ifndef CONFIG_HD2SD_ENABLE
					clock_dispset_dac0mode(_clock_wake);
#endif
#ifndef CONFIG_CELESTIAL_TIGA_MINI
					clock_dispset_dac1mode(_clock_wake);
#endif
					__df_update_start();
					if(DF_OUTPUT_VGA == df_output_config.output_mode[tveid]){
						//				DFSetBackGroud(0, 0x0, 0x0, 0x0);
					}
					else{
						//				DFSetBackGroud(0, 0x10, 0x80, 0x80);
					}
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
					case TVOUT_MODE_1080P30:
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
						pinmux_set_dac1mode(_dac1_disp0_rgb);
						__Is_TV = 0;
						break;

					case TVOUT_RGB_1024X768_60FPS:
					case TVOUT_RGB_1280X720_60FPS:
					case TVOUT_RGB_1440X900_60FPS:
					case TVOUT_RGB_1280X1024_60FPS:
					case TVOUT_RGB_1360X768_60FPS:
					case TVOUT_RGB_1920X1080P30:
					case TVOUT_RGB_1920X1080P60:
					case TVOUT_RGB_1920X1080I30:
					case TVOUT_RGB_1366X768_60FPS:
						pinmux_set_dac1mode(_dac1_disp0_rgb);
						__Is_TV = 0;
						break;

					default:
						break;
				}
				DEBUG_PRINTF("tevid = %d, mode = %d\n",tveid,(int)arg);

				__df_calc_video_position(df_dev[minor_id].tve_format,arg,&pos);
				df_dev[minor_id].tve_format = arg;
				outputmode = arg; // fix a bug: user space mutilp call CSTVOUT_Open will casue CSTVOUT_SetMode failed.

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
					clock_dispset_clockmode0(tveid, tve_2_clock[df_dev[minor_id].tve_format]);
					clock_hdmiset_clockmode(tve_2_clock[df_dev[minor_id].tve_format], _1x_disp_clk);
					mdelay(10);
#endif
				}
				else{
					clock_dispset_clockmode0(tveid, tve_2_clock[df_dev[minor_id].tve_format]);
					clock_hdmiset_clockmode(tve_2_clock[df_dev[minor_id].tve_format], _1x_disp_clk);

					mdelay(10);
				}

				if(__Is_TV){
					if(tveid == 0){
						if(__Is_first){
#if BOOT_HAS_OUTPUT
							;
#else
#ifndef CONFIG_CELESTIAL_TIGA_MINI
							InitTVE0Raw(df_dev[minor_id].tve_format, __Is_ColorBar);
#else
							;
#endif
#endif
						}
						else{
#ifndef CONFIG_CELESTIAL_TIGA_MINI
							InitTVE0Raw(df_dev[minor_id].tve_format, __Is_ColorBar);
#else
							;
#endif
						}
					}
					else if(tveid == 1){
#ifndef CONFIG_HD2SD_ENABLE
						InitTVE1Raw(df_dev[minor_id].tve_format, __Is_ColorBar);
#else
						;
#endif
					}
					mdelay(10);
				}

				__df_update_start();
				DFSetOutIFVideoFmt(tveid, tve_2_df[df_dev[minor_id].tve_format]);
				__df_update_end();
				mdelay(10);

				reg_val = __video_read(0x39);
				while(reg_val){
					wait_times--;
					udelay(1000);
					if (wait_times <= 0){
						//printk("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					reg_val = __video_read(0x39);
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

				__video_write(1<<16,0x39);

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

				if((tveid == 0) && (__Is_hd2sd) && (HD2SD_DATA_SIZE != 0)){
					switch (df_dev[minor_id].tve_format){
						case TVOUT_MODE_SECAM:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_SECAM;
							break;
						case TVOUT_MODE_PAL_CN:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_CN;
							break;
						case TVOUT_MODE_PAL_N:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_N;
							break;
						case TVOUT_MODE_PAL_M:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_M;
							break;
						case TVOUT_MODE_576I:
						case TVOUT_MODE_1080I25:
						case TVOUT_MODE_576P:
						case TVOUT_MODE_720P50:
						case TVOUT_MODE_1080P25:
						case TVOUT_MODE_1080P50:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							break;
						case TVOUT_MODE_480I:
						case TVOUT_MODE_480P:
						case TVOUT_MODE_720P60:
						case TVOUT_MODE_1080I30:
						case TVOUT_MODE_1080P24:
						case TVOUT_MODE_1080P30:
						case TVOUT_MODE_1080P60:
						case TVOUT_RGB_640X480_60FPS:
						case TVOUT_RGB_800X600_60FPS:
						case TVOUT_RGB_1024X768_60FPS:
						case TVOUT_RGB_1280X720_60FPS:
						case TVOUT_RGB_800X480_60FPS:
						case TVOUT_RGB_1440X900_60FPS:
						case TVOUT_RGB_1280X1024_60FPS:
						case TVOUT_RGB_1360X768_60FPS:
						case TVOUT_RGB_1920X1080P30:
						case TVOUT_RGB_1920X1080P60:
						case TVOUT_RGB_1920X1080I30:
						case TVOUT_RGB_1366X768_60FPS:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
							break;

						default:
							df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
							DEBUG_PRINTF("kernel: TVOUT_MODE_576I\n");
							break;
					}

					clock_dispset_clockmode0(1, tve_2_clock[df_dev[minor_id+1].tve_format]);
					mdelay(10);
					__df_update_start();
					DFSetOutIFVideoFmt(1, tve_2_df[df_dev[minor_id+1].tve_format]);
					__df_update_end();
					mdelay(10);
#ifndef CONFIG_HD2SD_ENABLE
					InitTVE1Raw(df_dev[minor_id+1].tve_format, __Is_ColorBar);
#endif
					mdelay(10);

					__df_update_start();
					DFVideoEna(1, 1);
					__df_update_end();

					__df_update_start();
					DFHD2SDCaptureEna(1);
					__df_update_end();
				}

				__Is_first = 0;
			}
			break;

		case CSTVE_IOC_GET_MODE:
			__put_user(df_dev[minor_id].tve_format, (int __user *) arg);
			break;

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

		case CSDF_IOC_SET_BRIGHTNESS:
			{
				char brightness;
				arg &= 0xff;
				if (arg == 128)
					brightness = 0;
				else if (arg >=1 && arg <=127)
					brightness = arg * (-1);
				else
					brightness = arg - 128;
				reg_val = df_read(dfreg_comp[2]);
				__df_update_start();
				df_write(dfreg_comp[2], (reg_val & ~(0xff)) | brightness);
				__df_update_end();
			}
			break;

		case CSDF_IOC_GET_BRIGHTNESS:
			reg_val = df_read(dfreg_comp[2]) & 0xff;
			if (reg_val == 0)
				reg_val = 128;
			else if (reg_val >=0 && reg_val <=127)
				reg_val = reg_val + 128;
			else
				reg_val = reg_val * (-1);

			__put_user(reg_val, (int __user *) arg);
			break;

		case CSDF_IOC_SET_SATURATION:
			reg_val = df_read(dfreg_comp[2]);
			__df_update_start();
			df_write(dfreg_comp[2], (reg_val & ~(0x00ff0000)) | (((arg&0xff)<<16)&0x00ff0000));
			__df_update_end();
			break;

		case CSDF_IOC_GET_SATURATION:
			reg_val = (df_read(dfreg_comp[2]) >> 16) & 0xff;
			__put_user(reg_val, (int __user *) arg);
			break;

		case CSDF_IOC_SET_CONTRAST:
			{
				char contrast;
				arg &= 0xff;
				contrast = arg * 64 / 256;
				reg_val = df_read(dfreg_comp[2]);
				__df_update_start();
				df_write(dfreg_comp[2], (reg_val & ~(0x3f00)) | ((contrast<<8)&0x3f00));
				__df_update_end();
			}
			break;

		case CSDF_IOC_GET_CONTRAST:
			reg_val = (df_read(dfreg_comp[2]) >> 8) & 0x3f;
			reg_val = reg_val * 256 / 64;
			__put_user(reg_val, (int __user *) arg);
			break;

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
				DFHD2SDCaptureEna(0);
			}

			if((tveid == 0) && (__Is_hd2sd) && (HD2SD_DATA_SIZE != 0)) {
				switch (df_dev[minor_id].tve_format){
					case TVOUT_MODE_SECAM:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_SECAM;
						break;
					case TVOUT_MODE_PAL_CN:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_CN;
						break;
					case TVOUT_MODE_PAL_N:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_N;
						break;
					case TVOUT_MODE_PAL_M:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_PAL_M;
						break;
					case TVOUT_MODE_576I:
					case TVOUT_MODE_1080I25:
					case TVOUT_MODE_576P:
					case TVOUT_MODE_720P50:
					case TVOUT_MODE_1080P25:
					case TVOUT_MODE_1080P50:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						break;
					case TVOUT_MODE_480I:
					case TVOUT_MODE_480P:
					case TVOUT_MODE_720P60:
					case TVOUT_MODE_1080I30:
					case TVOUT_MODE_1080P24:
					case TVOUT_MODE_1080P30:
					case TVOUT_MODE_1080P60:
					case TVOUT_RGB_640X480_60FPS:
					case TVOUT_RGB_800X600_60FPS:
					case TVOUT_RGB_1024X768_60FPS:
					case TVOUT_RGB_1280X720_60FPS:
					case TVOUT_RGB_800X480_60FPS:
					case TVOUT_RGB_1440X900_60FPS:
					case TVOUT_RGB_1280X1024_60FPS:
					case TVOUT_RGB_1360X768_60FPS:
					case TVOUT_RGB_1920X1080P30:
					case TVOUT_RGB_1920X1080P60:
					case TVOUT_RGB_1920X1080I30:
					case TVOUT_RGB_1366X768_60FPS:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_480I;
						break;

					default:
						df_dev[minor_id+1].tve_format = TVOUT_MODE_576I;
						DEBUG_PRINTF("kernel: TVOUT_MODE_576I\n");
						break;
				}

				clock_dispset_clockmode0(1, tve_2_clock[df_dev[minor_id+1].tve_format]);
				mdelay(10);
				__df_update_start();
				DFSetOutIFVideoFmt(1, tve_2_df[df_dev[minor_id+1].tve_format]);
				__df_update_end();
				mdelay(10);
#ifndef CONFIG_CELESTIAL_TIGA_MINI
				InitTVE1Raw(df_dev[minor_id+1].tve_format, __Is_ColorBar);
#endif
				mdelay(10);

				__df_update_start();
				DFVideoEna(1, 1);
				__df_update_end();

				__df_update_start();
				DFHD2SDCaptureEna(1);
				__df_update_end();
			}
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

		case CSDF_IOC_Z_ORDER:
			__df_update_start();
			df_write(dfreg_comp[1], arg);
			__df_update_end();
			break;

		case CSDF_IOC_GET_INIT_OUTPUTMODE:
			__put_user(outputmode, (int __user *) arg);
			break;

		case CSDF_IOC_GET_IDCS_DDR_DIV_VAL:
			__put_user(clock_read_fixme(0xb2100100), (int __user *) arg);
			break;

		default:
			break;
	}

	return 0;
}

static int cnc_df_gfx_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	printk("This node(Gfx layer) is not used!\n");
	return 0;
}

static int cnc_df_vid_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
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

	DEBUG_PRINTF("cnc_df_vid_ioctl : df video layer id = %d\n",vid_id);
#if 0
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
			DFVideoSetLayerAlpha(vid_id, arg);
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
#endif
	return 0;
}

static int cnc_df_outif_ioctl(unsigned int cmd, unsigned long arg, unsigned int minor_id)
{
	printk("This node(outif) is not used!\n");
	return 0;
}

static int cnc_df_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
	unsigned int minor_id = iminor(inode);

	if (minor_id == DF_MINOR_TVE0 || minor_id == DF_MINOR_TVE1){
		cnc_tve_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_GFX0 || minor_id == DF_MINOR_GFX1){
		cnc_df_gfx_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_VID0 || minor_id == DF_MINOR_VID1){
		cnc_df_vid_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_VIDAUX){
		cnc_df_vid_ioctl(cmd, arg, minor_id);
	}

	else if (minor_id == DF_MINOR_OUT0 || minor_id == DF_MINOR_OUT1){
		cnc_df_outif_ioctl(cmd, arg, minor_id);
	}

	return 0;
}

int __df_mem_initialize(void)
{
	if (NULL == request_mem_region(DISP_REG_BASE, DISP_REG_SIZE, "CNC1800L DF space")) {
		goto INIT_ERR0;
	}

	if (NULL == disp_base){
		if(!(disp_base = (unsigned char *)ioremap(DISP_REG_BASE, DISP_REG_SIZE))) {
			goto INIT_ERR1;
		}
	}

	if (NULL == request_mem_region(TVE0_REG_BASE, TVE0_REG_SIZE, "CNC1800L TVE0 space")) {
		goto INIT_ERR2;
	}

	if (NULL == tve0_base){
		if(!(tve0_base = (unsigned char *)ioremap(TVE0_REG_BASE, TVE0_REG_SIZE))) {
			goto INIT_ERR3;
		}
	}

	if (NULL == request_mem_region(TVE1_REG_BASE, TVE1_REG_SIZE, "CNC1800L TVE1 space")) {
		goto INIT_ERR4;
	}

	if (NULL == tve1_base){
		if(!(tve1_base = (unsigned char *)ioremap(TVE1_REG_BASE, TVE1_REG_SIZE))) {
			goto INIT_ERR5;
		}
	}

	printk("disp_base 0x%x, tve0_base 0x%x, tve1_base 0x%x\n",(unsigned int)disp_base,(unsigned int)tve0_base,(unsigned int)tve1_base);
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

		clock_dispset_dac0mode(_clock_wake);
		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_NTSC);
		clock_dispset_clockmode0(_disp1, _YUV_NTSC);
		clock_hdmiset_clockmode(_YUV_NTSC, _1x_disp_clk);

		InitTVE0Raw(TVOUT_MODE_480I, 1);
		InitTVE1Raw(TVOUT_MODE_480I, 1);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_NTSC);
		DFSetOutIFVideoFmt(1, DISP_YUV_NTSC);
		DFOutIFEna(0, 1);
		DFOutIFEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("480p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 480;

		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_480P);
		clock_hdmiset_clockmode(_YUV_480P, _1x_disp_clk);

		InitTVE0Raw(TVOUT_MODE_480P, 1);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_480P);
		DFOutIFEna(0, 1);
		__df_update_end();

	}
	else if(strncmp("576i", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		clock_dispset_dac0mode(_clock_wake);
		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_PAL);
		clock_dispset_clockmode0(_disp1, _YUV_PAL);
		clock_hdmiset_clockmode(_YUV_PAL, _1x_disp_clk);

		InitTVE0Raw(TVOUT_MODE_576I, 1);
		InitTVE1Raw(TVOUT_MODE_576I, 1);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_PAL);
		DFSetOutIFVideoFmt(1, DISP_YUV_PAL);
		DFOutIFEna(0, 1);
		DFOutIFEna(1, 1);
		__df_update_end();

	}
	else if(strncmp("576p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_576P);
		clock_hdmiset_clockmode(_YUV_576P, _1x_disp_clk);

		InitTVE0Raw(TVOUT_MODE_576P, 1);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_576P);
		DFOutIFEna(0, 1);
		__df_update_end();

	}
	else if(strncmp("720p", cmd_line, 4) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_720P_60Hz);
		InitTVE0Raw(TVOUT_MODE_720P60, 1);
		clock_hdmiset_clockmode(_YUV_720P_60Hz, _1x_disp_clk);

		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_720P_60FPS);
		DFOutIFEna(0, 1);
		__df_update_end();

	}
	else if(strncmp("1080p", cmd_line, 5) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _YUV_1080P_60Hz);
		clock_hdmiset_clockmode(_YUV_1080P_60Hz, _1x_disp_clk);

		//	InitTVE0Raw(TVOUT_MODE_1, 0);
		//	InitTVE1Raw(TVOUT_MODE_576P, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_YUV_1080P_60FPS);
		DFOutIFEna(0, 1);
		__df_update_end();

	}
#if 0
	else if(strncmp("vga", cmd_line, 3) == 0){
		df_rect DispRect;
		DispRect.top = 0;
		DispRect.left = 0;
		DispRect.right = 720;
		DispRect.bottom = 576;

		pinmux_set_dac1mode(_dac1_disp0_rgb);
		clock_dispset_dac0mode(_clock_wake);
		clock_dispset_dac1mode(_clock_wake);
		clock_dispset_clockmode0(_disp0, _RGB_CVT_1366x768_60Hz);
		//		clock_dispset_clockmode0(_disp1, _YUV_1080P_60Hz);
		//	InitTVE0Raw(TVOUT_MODE_1, 0);
		//	InitTVE1Raw(TVOUT_MODE_576P, 0);
		__df_update_start();
		DFSetOutIFVideoFmt(0, DISP_RGB_1366x768);
		__df_update_end();

	}
#endif
	else if((strncmp("df_clock",cmd_line,8) == 0)){
		CLOCK_ENA ClockEna = _clock_enable;
		ClockEna = simple_strtol(&buffer[9], NULL, 16);
		printk(" ClockEna = %d \n", ClockEna);
		clock_df_clockena(ClockEna);
	}
	else if((strncmp("clock_tms_clockena",cmd_line,18) == 0)){
		CLOCK_ENA ClockEna = _clock_enable;
		ClockEna = simple_strtol(&buffer[19], NULL, 16);
		printk(" ClockEna = %d \n", ClockEna);
		clock_tms_clockena(ClockEna);
	}
	else if((strncmp("clock_cec_clockena",cmd_line,18) == 0)){
		CLOCK_ENA ClockEna = _clock_enable;
		ClockEna = simple_strtol(&buffer[19], NULL, 16);
		printk(" ClockEna = %d \n", ClockEna);
		clock_cec_clockena(ClockEna);
	}
	else if((strncmp("dumpreg",cmd_line,7) == 0)){
		unsigned int count = 0;

		for(count = 0; count < 0x400; count +=4){
			switch(count){
				case 0x0:
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
				case 0x180:
					printk("\n DISP_REG_VIDEO2\n");
					break;
				case 0x1c0:
					printk("\n DISP_REG_VIDEO3\n");
					break;
				case 0x280:
					printk("\n DISP_REG_COMP1\n");
					break;
				case 0x2c0:
					printk("\n DISP_REG_HD2SD\n");
					break;
				case 0x340:
					printk("\n DISP_REG_OUTIF1\n");
					break;
				case 0x380:
					printk("\n DISP_REG_OUTIF2\n");
					break;
			}
			printk(" addr[0x%08x] = 0x%08x \n", DISP_REG_BASE + count, df_read(DISP_REG_BASE+count));
		}

	}

	return count;
}

static int cnc_df_open(struct inode *inode, struct file *file)
{
	unsigned int minor_id = iminor(inode);

	spin_lock_init(&(df_dev[minor_id].spin_lock));
	df_dev[minor_id].enable = 1;
	df_dev[minor_id].tve_format = outputmode;

	DEBUG_PRINTF("%s was opened\n",df_dev[minor_id].name);

	//	clock_ddc_clockena(_clock_enable);
	//	clock_cec_clockena(_clock_enable);
	//	clock_tms_clockena(_clock_enable);

	return 0;
}

static int cnc_df_release(struct inode *inode, struct file *file)
{
	unsigned int minor_id = iminor(inode);

	df_dev[minor_id].enable = 0;

	DEBUG_PRINTF("%s was released\n",df_dev[minor_id].name);

	return 0;
}


static struct class *cnc_df_class;
static struct proc_dir_entry *cnc_df_proc_entry = NULL;
static struct file_operations cnc_df_fops = {
	.owner	= THIS_MODULE,
	.open	=  cnc_df_open,
	.release	= cnc_df_release,
	.ioctl		= cnc_df_ioctl,
};


CNC_FB_INTERRUPT cncdf_1800_vblank_int[2];

irqreturn_t cnc_outif0_irq(int irq, void *dev_id)
{
	DF_IRQ_PRINTF("cnc_outif0_irq!\n");

	if (DF_Read(DISP_INT_STA_HOST) & 0x8)
	{
		if(cncdf_1800_vblank_int[0].is_display == 1){
			cncdf_1800_vblank_int[0].is_display = 0;
			wake_up(&cncdf_1800_vblank_int[0].wait);
		}

		if(cncdf_1800_vblank_int[1].is_display == 1){
			cncdf_1800_vblank_int[1].is_display = 0;
			wake_up(&cncdf_1800_vblank_int[1].wait);
		}
	}

	DF_Write(DISP_INT_STA_HOST, DF_Read(DISP_INT_STA_HOST) | 0x8);

	return IRQ_HANDLED;
}

#if 0
irqreturn_t cnc_outif1_irq(int irq, void *dev_id)
{
	DF_IRQ_PRINTF("cnc_outif1_irq!\n");

	if(cncdf_1800_vblank_int[1].is_display == 1){
		cncdf_1800_vblank_int[1].is_display = 0;
		wake_up(&cncdf_1800_vblank_int[1].wait);
	}

	DF_Write(DISP_INT_STA_HOST, DF_Read(DISP_INT_STA_HOST) | 0x20);

	return IRQ_HANDLED;
}
#endif

static void cnc_df_setup_dev(unsigned int minor_id, char * dev_name)
{
	device_create(cnc_df_class, NULL, MKDEV(DF_MAJOR, minor_id), NULL, dev_name);
	df_dev[minor_id].dev_minor = minor_id;
	df_dev[minor_id].dev_type = minor_id;
	df_dev[minor_id].enable = 0;
	df_dev[minor_id].tve_format = TVOUT_MODE_576I;
	df_dev[minor_id].name = dev_name;
}

static int __init cnc_df_init(void)
{

	if (register_chrdev(DF_MAJOR, "cnc_1800l_df", &cnc_df_fops)){
		__df_mem_destroy();
		return -ENODEV;
	}

	cnc_df_class = class_create(THIS_MODULE,"cnc_1800l_df");
	if (IS_ERR(cnc_df_class)){
		printk(KERN_ERR "DF: class create failed.\n");
		__df_mem_destroy();
		cnc_df_class = NULL;
		return -ENODEV;
	}

	cnc_df_setup_dev(DF_MINOR_GFX0,  "cnc_1800l_df_gfx0");
	cnc_df_setup_dev(DF_MINOR_GFX1,  "cnc_1800l_df_gfx1" );
	cnc_df_setup_dev(DF_MINOR_VID0,  "cnc_1800l_df_video0");
	cnc_df_setup_dev(DF_MINOR_VID1,  "cnc_1800l_df_video1");
	cnc_df_setup_dev(DF_MINOR_OUT0,  "cnc_1800l_df_output0");
	cnc_df_setup_dev(DF_MINOR_OUT1,  "cnc_1800l_df_output1");
	cnc_df_setup_dev(DF_MINOR_TVE0,  "cnc_1800l_tvout0");
	cnc_df_setup_dev(DF_MINOR_TVE1,  "cnc_1800l_tvout1");
	cnc_df_setup_dev(DF_MINOR_VIDAUX,  "cnc_1800l_df_vidaux");

	__df_mem_initialize();

	cnc_df_proc_entry = create_proc_entry("df18_io", 0, NULL);
	if (NULL != cnc_df_proc_entry) {
		cnc_df_proc_entry->write_proc = &df_proc_write;
	}

	DF_Write(DISP_INT_MSK_HOST, DF_Read(DISP_INT_MSK_HOST) | 0x28);

	if (request_irq(7, cnc_outif0_irq, 0, "cs_df0", NULL)) {
		printk(KERN_ERR "csdrv_df: cannot register outif0 IRQ \n");
		return -EIO;
	}

#if 0
	if (request_irq(25, cnc_outif1_irq, 0, "cs_df1", NULL)) {
		printk(KERN_ERR "csdrv_df: cannot register outif1 IRQ \n");
		return -EIO;
	}
#endif

	printk("CNC1800L Display System Version : 0.1\n");

	printk(KERN_INFO "%s: CNC Display feeder driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"cnc1800l_df", DISP_REG_BASE, DISP_REG_SIZE);
	printk(KERN_INFO "%s: CNC TVE0 driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"cnc1800l_df", TVE0_REG_BASE, TVE0_REG_SIZE);
	printk(KERN_INFO "%s: CNC TVE1 driver was initialized, at address@[phyical addr = %08x, size = %x] \n",
			"cnc1800l_df", TVE1_REG_BASE, TVE1_REG_SIZE);

	return 0;
}

inline static void cnc_df_remove_dev(dev_t df_dev)
{
	device_destroy(cnc_df_class,df_dev);
}

static void __exit cnc_df_exit(void)
{
	__df_mem_destroy();

	if (NULL != cnc_df_proc_entry)
		remove_proc_entry("df18_io", NULL);

	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_GFX0));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_GFX1));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VID0));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VID1));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_OUT0));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_OUT1));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_TVE0));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_TVE1));
	cnc_df_remove_dev(MKDEV(DF_MAJOR, DF_MINOR_VIDAUX));

	class_destroy(cnc_df_class);
	unregister_chrdev(DF_MAJOR, "cnc_df");
	printk(KERN_INFO "CNC DF Exit ...... OK. \n");

	return;
}

postcore_initcall(cnc_df_init);

MODULE_AUTHOR("Jia Ma, <jia.ma@caviumnetworks.com>");
MODULE_DESCRIPTION("Celestial Semiconductor Display feeder sub-system");
MODULE_LICENSE("GPL");

