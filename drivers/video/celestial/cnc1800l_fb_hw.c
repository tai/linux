
/*
 *  linux/drivers/video/celestial/cnc1800l_fb_hw.c -- Cavium celestial frame buffer device
 *
 *  Copyright (C) 2011 Cavium
 *  Author: Xiaodong Fan
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

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

#include "cncfb.h"
#include <mach/cnc1800l_df.h>
#include <mach/df_reg_def_1800l.h>
#include <mach/df_reg_fmt_1800l.h>
#include "cnc18xx_fb_hw.h"
#include <mach/cnc1800l_power_clk.h>

//#define FB_DEBUG
#ifdef FB_DEBUG
#define DEBUG(args...) 		\
 	do {						\
        printk(KERN_INFO args); \
	} while(0)
#else
#define DEBUG(args...) do { } while(0)
#endif

extern CNC_FB_INTERRUPT cncdf_1800_vblank_int[2];
extern int __Is_TV;
extern df_reg_para dfreg;
extern int Get_Outif_Id(int gfx_id);
extern void __df_update_start(void);
extern void __df_update_end(void);

extern unsigned int DF_Read(unsigned int addr);

extern void DF_Write(unsigned int addr, unsigned int data);
extern void DFGfxEna(int GfxId, int IsEna, int IsRGB2YUVEna);
extern void DFGfxSetAlpha(int GfxId, unsigned short DefaultAlpha, unsigned short ARGB1555Alpha0, unsigned short ARGB1555Alpha1);
extern void DFGfxGetAlpha(int GfxId, unsigned short *DefaultAlpha, unsigned short *ARGB1555Alpha0, unsigned short *ARGB1555Alpha1);
extern void DFGfxColorKeyEna(int GfxId,int IsEna);
extern void DFGfxSetColorKey(int GfxId, unsigned char KeyRMin, unsigned char KeyRMax, unsigned char KeyGMin, unsigned char KeyGMax, unsigned char KeyBMin, unsigned char KeyBMax);
extern void DFSetZOrder(int OutIFId, int Gfx1ZOrder, int Gfx2ZOrder, int Video1ZOrder, int Video2ZOrder);
extern void DFSetBackGroud(int OutIFId, unsigned char BGY, unsigned char BGU, unsigned char BGV);


//#define DF_GFX_Read(addr) (printk("df gfx addr=0x%x\n", addr),DF_Read(addr))
#define DF_GFX_Read(addr) (DF_Read(addr))

typedef enum _DF_GFX_COLOR_FORMAT_
{
	DF_GFX_CLUT4     = 0,
	DF_GFX_CLUT8     = 1,
	DF_GFX_RGB565    = 2,
	DF_GFX_ARGB4444  = 3,
	DF_GFX_A0        = 4,
	DF_GFX_ARGB1555  = 5,
	DF_GFX_ARGB8888  = 6,
	DF_GFX_XRGB8888  = 7,
	DF_GFX_FORMA_MAX,

} DF_GFX_COLOR_FORMAT;

static int dfreg_gfx[2][13] =
{
	{DISP_GFX1_CTRL,	//=0
	DISP_GFX1_FORMAT,	//=1
	DISP_GFX1_ALPHA_CTRL,	//=2
	DISP_GFX1_KEY_RED,	//=3
	DISP_GFX1_KEY_BLUE,	//=4
	DISP_GFX1_KEY_GREEN,	//=5
	DISP_GFX1_BUF_START,	//=6
	DISP_GFX1_LINE_PITCH,	//=7
	DISP_GFX1_X_POSITON,	//=8
	DISP_GFX1_Y_POSITON,	//=9
	},
	{DISP_GFX2_CTRL,
	DISP_GFX2_FORMAT,
	DISP_GFX2_ALPHA_CTRL,
	DISP_GFX2_KEY_RED,
	DISP_GFX2_KEY_BLUE,
	DISP_GFX2_KEY_GREEN,
	DISP_GFX2_BUF_START,
	DISP_GFX2_LINE_PITCH,
	DISP_GFX2_X_POSITON,
	DISP_GFX2_Y_POSITON,
	},
};

/************************************* FUNCTION *****************************************/
int update_gfx_layer(struct fb_info *info, int init, struct cnc_info * cncfbinfo)
{
	unsigned int outif_id = 0;
	int i_node = info->node;

	if (i_node > 1) i_node -= 2;

	DEBUG("node=%d: update gfx register!\n", info->node);

	__df_update_start();

	/* if info->node > 1,  only update gfx color format */
	if (info->node == 2 || info->node == 3) {

		dfreg.Gfx[info->node -2].df_gfx_format_reg.val = DF_GFX_Read(dfreg_gfx[info->node -2][1]);
		dfreg.Gfx[info->node -2].iLinePitch = DF_GFX_Read(dfreg_gfx[info->node -2][7]);

		dfreg.Gfx[info->node - 2].df_gfx_format_reg.bits.iColorFormat =
			(info->var.bits_per_pixel == 4) ? DF_GFX_CLUT4 :
			(info->var.bits_per_pixel == 8) ? DF_GFX_CLUT8 :
			(info->var.transp.length  == 0) ? DF_GFX_RGB565 :
			(info->var.transp.length  == 1) ? DF_GFX_ARGB1555 :
			(info->var.transp.length  == 4) ? DF_GFX_ARGB4444 :
			(info->var.transp.length  == 8) ? DF_GFX_ARGB8888 :
			(info->var.transp.length  == 32) ? DF_GFX_A0 :
			dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iColorFormat;


		DF_Write((dfreg_gfx[info->node -2][1]), dfreg.Gfx[info->node -2].df_gfx_format_reg.val);
        __df_update_end();
        return 0;
    }

	dfreg.Gfx[info->node].df_gfx_format_reg.val = DF_GFX_Read(dfreg_gfx[info->node][1]);
	dfreg.Gfx[info->node].iStartAddr = DF_GFX_Read(dfreg_gfx[info->node][6]);
	dfreg.Gfx[info->node].iLinePitch = DF_GFX_Read(dfreg_gfx[info->node][7]);
	dfreg.Gfx[info->node].df_gfx_x_position_reg.val = DF_GFX_Read(dfreg_gfx[info->node][8]);
	dfreg.Gfx[info->node].df_gfx_y_position_reg.val = DF_GFX_Read(dfreg_gfx[info->node][9]);

	dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat =
		(info->var.bits_per_pixel == 4) ? DF_GFX_CLUT4 :
		(info->var.bits_per_pixel == 8) ? DF_GFX_CLUT8 :
		(info->var.transp.length == 0) ? DF_GFX_RGB565 :
		(info->var.transp.length == 1) ? DF_GFX_ARGB1555 :
		(info->var.transp.length == 4) ? DF_GFX_ARGB4444 :
		(info->var.transp.length == 8) ? DF_GFX_ARGB8888 :
		(info->var.transp.length == 32) ? DF_GFX_A0 :
		dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat;

	dfreg.Gfx[info->node].iStartAddr =
        (info->fix.smem_start +
         info->fix.line_length * info->var.yoffset +
         info->var.xoffset * info->var.bits_per_pixel / 8);

	dfreg.Gfx[info->node].iLinePitch = info->fix.line_length;
	DEBUG("node=%d; iLinePitch=%d\n", info->node, info->fix.line_length);

	dfreg.Gfx[info->node].df_gfx_x_position_reg.bits.iXStart = info->var.left_margin;
	dfreg.Gfx[info->node].df_gfx_x_position_reg.bits.iXEnd = (info->var.left_margin + info->var.xres) - 1;
	dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYStart	=
		info->var.upper_margin / (info->var.vmode & FB_VMODE_INTERLACED ? 2 : 1);
	dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYEnd =
		(info->var.upper_margin + info->var.yres) /(info->var.vmode & FB_VMODE_INTERLACED ? 2 : 1);

	if(init) {
		DFGfxSetAlpha(info->node, 0x3ff, 0, 0);
		DFGfxEna(info->node, 1, __Is_TV);
		DFGfxSetColorKey(info->node, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
		DFGfxColorKeyEna(info->node, 1);
	}

	DF_Write((dfreg_gfx[info->node][6]), dfreg.Gfx[info->node].iStartAddr);
	DF_Write((dfreg_gfx[info->node][7]), dfreg.Gfx[info->node].iLinePitch);
	DF_Write((dfreg_gfx[info->node][8]), dfreg.Gfx[info->node].df_gfx_x_position_reg.val);
	DF_Write((dfreg_gfx[info->node][9]), dfreg.Gfx[info->node].df_gfx_y_position_reg.val);
	DF_Write((dfreg_gfx[info->node][1]), dfreg.Gfx[info->node].df_gfx_format_reg.val);

	__df_update_end();

	if (i_node > 1) i_node -= 2;
	outif_id = Get_Outif_Id(i_node);

	if(cncdf_1800_vblank_int[outif_id].is_display == 1){
		wait_event_interruptible(cncdf_1800_vblank_int[outif_id].wait, cncdf_1800_vblank_int[outif_id].is_display == 0);
	}

	return 0;
}

int cncfb_hw_init(struct fb_info *info, int en_flags, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	clock_gfx_clockena(_clock_enable);


	init_waitqueue_head(&cncdf_1800_vblank_int[i_node].wait);
	cncdf_1800_vblank_int[i_node].is_display = 0;
	cncdf_1800_vblank_int[i_node].count = 0;
	cncdf_1800_vblank_int[i_node].cur_fb_info = info;

	if(en_flags){
		update_gfx_layer(info, en_flags, cncfbinfo);
	}
	return 0;
}

void cncfb_hw_exit(void)
{
	return ;
}

int cncfb_hw_gfx_on(struct fb_info *info, unsigned int arg, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;



    printk("hw gfx on:inode=%d, arg=%d\n",i_node, arg);
    __df_update_start();
	DFGfxEna(i_node, arg, __Is_TV);
	cncfbinfo->fbs_lst[i_node].onoff = arg;
    __df_update_end();

	return 0;
}

int cncfb_hw_gfx_alpha(struct fb_info *info, unsigned int arg,struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	arg &= 0x3ff;

	__df_update_start();

	DFGfxSetAlpha(i_node, arg, 0, 0);

	__df_update_end();

	return 0;
}

int cncfb_hw_colorkey_on(struct fb_info *info, unsigned int arg, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	__df_update_start();

	DFGfxColorKeyEna(i_node, arg);

	__df_update_end();

	return 0;
}

int cncfb_hw_colorkey_val(struct fb_info *info, gfx_colorkey *col_key, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	__df_update_start();

	DFGfxSetColorKey(i_node, col_key->r_min, col_key->r_max, col_key->g_min, col_key->g_max, col_key->b_min, col_key->b_max);

	__df_update_end();

	return 0;
}

int cncfb_hw_z_order(struct fb_info *info, unsigned int arg)
{
	int Video1ZOrder = (arg >>  0) & 0x3;
	int Video2ZOrder = (arg >>  4) & 0x3;
	int Gfx1ZOrder = (arg >> 8) & 0x3;
	int Gfx2ZOrder = (arg >> 12) & 0x3;
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	__df_update_start();
	DFSetZOrder(i_node, Gfx1ZOrder, Gfx2ZOrder, Video1ZOrder, Video2ZOrder);
	__df_update_end();

	return 0;
}

int cncfb_hw_set_par(struct fb_info *info, struct cnc_info * cncfbinfo)
{
	return update_gfx_layer(info, 0, cncfbinfo);
}

int cncfb_hw_pan_display(struct fb_info *info, struct cnc_info * cncfbinfo)
{
	return update_gfx_layer(info, 0, cncfbinfo);
}

int cncfb_hw_waitforvsync(struct fb_info *info)
{
	unsigned int outif_id = 0;
	int i_node = info->node;

	if (i_node > 1) i_node -= 2;
	outif_id = Get_Outif_Id(i_node);

	cncdf_1800_vblank_int[outif_id].is_display = 1;

	return 0;
}

int cncfb_hw_setbgcolor(struct fb_info *info, unsigned int arg)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;
	printk("i_node = 0x%x\n",i_node);
	__df_update_start();
	DFSetBackGroud(i_node, (arg>>16)&0xff,  (arg>>8)&0xff, arg&0xff);
	__df_update_end();
	return 0;
}

int cncfb_hw_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                         u_int transp, struct fb_info *info, struct cnc_info * cncfbinfo)
{
	/* TODO: implement CLUT table updating */
	return 0;
}

int cncfb_hw_blank(int blank)
{
	/* TODO: to call an external functions from TVE modules */
	return 0;
}

void df_writel(int addr, int val)
{
	DF_Write(addr, val);
}

int df_readl(int addr)
{
	return DF_GFX_Read(addr);
}
