/*
 *  linux/drivers/video/celestial/cncfb.c -- Cavium celestial frame buffer device
 *
 *  Copyright (C) 2010 Celestial Semiconductor
 *  Copyright (C) 2011 Cavium
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
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/mem_define.h>
#include <mach/cnc1800l_df.h>
#include "cncfb.h"

#include "cnc18xx_fb_hw.h"

#include "cnc_2d.c"

#define FB_ACCEL_CNC_2DGFX	0xff

#define DEVICE_NAME 	"cncfb"

/* There are FB_NUMS framebuffers, each represented by an fb_info. */
#define GFX0_FBNAME	"gfx0"
#define GFX1_FBNAME	"gfx1"
#define GFX2_FBNAME	"gfx2"
#define GFX3_FBNAME	"gfx3"

int outputmode = -1;
int colorformat = -1;
EXPORT_SYMBOL(outputmode);
static int __init root_outputmode_setup(char *str)
{
	outputmode = simple_strtoul(str, NULL, 0);
	return 1;
}

static int __init root_colorformat_setup(char *str)
{
	colorformat = simple_strtoul(str, NULL, 0);
	return 1;
}

__setup("outputmode=", root_outputmode_setup);
__setup("colorformat=", root_colorformat_setup);

//#define FB_DEBUG
#ifdef FB_DEBUG
#define DEBUG(args...) 		\
 	do {						\
        printk(KERN_INFO args); \
	} while(0)
#else
#define DEBUG(args...) do { } while(0)
#endif

struct cnc_info cncfb_static;
struct cnc_info *cncfbinfo = &cncfb_static;

struct fb_var_screeninfo cncfb_default = {
	.xres =         1280,
	.yres =         720,
	.xres_virtual = 1280,
	.yres_virtual = 720,
	.bits_per_pixel = 32,

#ifdef CONFIG_CELESTIAL_GFX_ARGB_MODE

	.transp =		{ 24, 8, 0 },
	.red =          { 16, 8, 0 },
	.green =        {  8, 8, 0 },
	.blue =         {  0, 8, 0 },

#else

	.transp =		{ 0, 8, 0 },
	.red =          { 8, 8, 0 },
	.green =        {  16, 8, 0 },
	.blue =         {  24, 8, 0 },

#endif

	.height =       -1,
	.width =        -1,
	.pixclock =     74074,
	.left_margin =  0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len =    144,
	.vsync_len =    49,
	.vmode =        FB_VMODE_NONINTERLACED,
    .activate =     FB_ACTIVATE_NOW,
};

static struct fb_fix_screeninfo cncfb_fix __initdata = {
	.id =           DEVICE_NAME,
	.smem_start =   FB0_REGION,
	.smem_len   =   FB0_SIZE,
	.type =         FB_TYPE_PACKED_PIXELS,
	.visual =       FB_VISUAL_TRUECOLOR,
	.xpanstep =     32,
	.ypanstep =     1,
	.ywrapstep =    0,
	.line_length =  5120,
	.mmio_start  =  PA_IO_GRAPHICS_BASE,
	.mmio_len    =  IO_GRAPHICS_SIZE,
	.accel =        FB_ACCEL_CNC_2DGFX,
};

static struct fb_fix_screeninfo cncfb_fix2 __initdata = {
	.id =           DEVICE_NAME,
	.smem_start =   FB1_REGION,
	.smem_len   =   FB1_SIZE,
	.type =         FB_TYPE_PACKED_PIXELS,
	.visual =       FB_VISUAL_TRUECOLOR,
	.xpanstep =     32,
	.ypanstep =     1,
	.ywrapstep =    0,
	.line_length =  5120,
	.mmio_start  =  PA_IO_GRAPHICS_BASE,
	.mmio_len    =  IO_GRAPHICS_SIZE,
	.accel =        FB_ACCEL_CNC_2DGFX,
};

static struct fb_fix_screeninfo cncfb_fix3 __initdata = {
	.id =           DEVICE_NAME,
	.smem_start =   FB2_REGION,
	.smem_len   =   FB2_SIZE,
	.type =         FB_TYPE_PACKED_PIXELS,
	.visual =       FB_VISUAL_TRUECOLOR,
	.xpanstep =     32,
	.ypanstep =     1,
	.ywrapstep =    0,
	.line_length =  5120,
	.mmio_start  =  PA_IO_GRAPHICS_BASE,
	.mmio_len    =  IO_GRAPHICS_SIZE,
	.accel =        FB_ACCEL_CNC_2DGFX,
};

static struct fb_fix_screeninfo cncfb_fix4 __initdata = {
	.id =           DEVICE_NAME,
	.smem_start =   FB3_REGION,
	.smem_len   =   FB3_SIZE,
	.type =         FB_TYPE_PACKED_PIXELS,
	.visual =       FB_VISUAL_TRUECOLOR,
	.xpanstep =     32,
	.ypanstep =     1,
	.ywrapstep =    0,
	.line_length =  5120,
	.mmio_start  =  PA_IO_GRAPHICS_BASE,
	.mmio_len    =  IO_GRAPHICS_SIZE,
	.accel =        FB_ACCEL_CNC_2DGFX,
};

static struct res_i {
	u_long xres;
	u_long yres;
} res_t[] = {
	{720,	576},
	{720,	480},
	{1280,	720},
	{1920,	1080},
	{640,	480},
	{800,	600},
	{1024,	768},
	{1280,	1024},
	{1600,	1000},
	{1280,	720},
	{848,	480},
	{800,	480},
	{1440,	900},
	{1360,	768},
	{1366,	768}};

static int cncfb_check_var(struct fb_var_screeninfo *var,
		struct fb_info *info);
static int cncfb_set_par(struct fb_info *info);
static int cncfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int transp, struct fb_info *info);
static int cncfb_pan_display(struct fb_var_screeninfo *var,
		struct fb_info *info);
static int cncfb_blank(int blank, struct fb_info *info);
static int cncfb_ioctl(struct fb_info* info, u_int cmd, u_long arg);
static int cncfb_open(struct fb_info *info, int user);
static int cncfb_release(struct fb_info *info, int user);

static struct fb_ops cncfb_ops = {
    .fb_open        = cncfb_open,
    .fb_release     = cncfb_release,
	.fb_check_var   = cncfb_check_var,
	.fb_set_par     = cncfb_set_par,
	.fb_setcolreg   = cncfb_setcolreg,
	.fb_pan_display = cncfb_pan_display,
	.fb_blank       = cncfb_blank,
	.fb_ioctl       = cncfb_ioctl,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit
    //	.fb_cursor      = soft_cursor
};

/*
 * Internal routines
 */
static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
#ifdef __ASM_ARCH_CNC1800L_H
	length = (length + 15) & ~15;
#else
	length = (length + 127) & ~127;
#endif
	length >>= 3;

	return length;
}


/* open fb device,  empty to do somthing in the future */
static int cncfb_open(struct fb_info *info, int user)
{
//	Gfx_2DInit();
	DEBUG("framebuff open!\n");
	return 0;
}

/* close fb device, empty to do somthing in the future */
static int cncfb_release(struct fb_info *info, int user)
{
	return 0;
}


/*
 * Setting the video mode has been split into two parts.
 * First part, xxxfb_check_var, must not write anything
 * to hardware, it should only verify and adjust var.
 * This means it doesn't alter par but it does use hardware
 * data from it to check this var.
 */
static int cncfb_check_var(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	u_long i;
	u_long line_length;
	u_long valid_res = 0;

    if (var == NULL || info == NULL ) {
		DEBUG("CNCFB: %s failed!\n", __FUNCTION__);
        return -EINVAL;
	}
	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */
    DEBUG("[checkvar-node:%d] var.activate=%d, info.var.activate=%d\n",info->node, var->activate, info->var.activate);
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/* minimal resolution */
	if (!var->xres)
		var->xres = 32;
	if (!var->yres)
		var->yres = 2;

	/* validate resolutions */
	for(i = 0; i < sizeof(res_t)/sizeof(struct res_i); i++) {
		if(res_t[i].yres == (var->upper_margin + var->yres + var->lower_margin) &&
           res_t[i].xres == (var->left_margin  + var->xres + var->right_margin )) {
			valid_res = 1;
			break;
		}
	}

	if(!valid_res)
	{
		return -EINVAL;
	}

	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;
	if (var->bits_per_pixel <= 4)
		var->bits_per_pixel = 4;
	else if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;

	else
	{
		return -EINVAL;
	}

	/* width must be 16byte aligned */
#ifndef __ASM_ARCH_CNC1800L_H //majia add this.
	if(var->xres % (16 * 8 / var->bits_per_pixel) != 0)
	{
		return -EINVAL;
	}
#endif

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	line_length = get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > info->fix.smem_len)
	{
		return -ENOMEM;
	}

	/*
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it
	 * work. This way we let the user know what is acceptable.
	 */
	switch (var->bits_per_pixel)
	{
		case 8:
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 0;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 0;
			var->transp.length = 0;
			break;

		case 16:
			if (var->transp.length == 4) {    /* ARGB4444 */
				var->red.offset = 8;
				var->red.length = 4;
				var->green.offset = 4;
				var->green.length = 4;
				var->blue.offset = 0;
				var->blue.length = 4;
				var->transp.offset = 12;
				var->transp.length = 4;
			} else if(var->transp.length) {   /* ARGB1555 */
				var->red.offset = 10;
				var->red.length = 5;
				var->green.offset = 5;
				var->green.length = 5;
				var->blue.offset = 0;
				var->blue.length = 5;
				var->transp.offset = 15;
				var->transp.length = 1;
			} else {        		  /* RGB 565 */
				var->red.offset = 11;
				var->red.length = 5;
				var->green.offset = 5;
				var->green.length = 6;
				var->blue.offset = 0;
				var->blue.length = 5;
				var->transp.offset = 0;
				var->transp.length = 0;
			}
			break;
		case 32:

#ifdef CONFIG_CELESTIAL_GFX_ARGB_MODE /* default for CNC1800L */
            var->red.offset = 16;
            var->red.length = 8;
            var->green.offset = 8;
            var->green.length = 8;
            var->blue.offset = 0;
            var->blue.length = 8;
            var->transp.offset = 24;
            var->transp.length = 8;

#else /*BGRA default for CNC1800H */
			var->transp.offset = 0;
			var->transp.length = 8;
			var->red.offset = 8;
			var->red.length = 8;
			var->green.offset = 16;
			var->green.length = 8;
			var->blue.offset = 24;
			var->blue.length = 8;
#endif

			break;
	}

	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;
    //    var->activate = FB_ACTIVATE_FORCE;
	return 0;
}

/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the
 * change in par. For this driver it doesn't do much.
 */
static int cncfb_set_par(struct fb_info *info)
{
    if (info == NULL ) {
		printk("CNCFB: set par error! info==NULL!\n");
        return -EINVAL;
	}
    if (cncfbinfo->fbs_lst[info->node].info == NULL)
        return -EINVAL;

	DEBUG("[setpar-node:%d]var:bits_per_pixel=%d, var:xres=%d, var:yres=%d, var:xres_virtual=%d, var.vmode=%d(0:non, 1:laced)\n",
          info->node, info->var.bits_per_pixel, info->var.xres, info->var.yres, info->var.xres_virtual, info->var.vmode);


	if(info->node >1 ){

        if (cncfbinfo->fbs_lst[info->node -2].info == NULL ) {
            return 0;
        }else {

            cncfbinfo->fbs_lst[info->node -2].info->fix.line_length    = get_line_length(cncfbinfo->fbs_lst[info->node -2].info->var.xres_virtual, info->var.bits_per_pixel);

            cncfbinfo->fbs_lst[info->node -2].info->var.bits_per_pixel   = info->var.bits_per_pixel;
            cncfbinfo->fbs_lst[info->node -2].info->var.transp.offset    = info->var.transp.offset;
            cncfbinfo->fbs_lst[info->node -2].info->var.transp.length    = info->var.transp.length;
            cncfbinfo->fbs_lst[info->node -2].info->var.transp.msb_right = info->var.transp.msb_right;
            cncfbinfo->fbs_lst[info->node -2].info->var.red.offset       = info->var.red.offset;
            cncfbinfo->fbs_lst[info->node -2].info->var.red.length       = info->var.red.length;
            cncfbinfo->fbs_lst[info->node -2].info->var.red.msb_right    = info->var.red.msb_right;
            cncfbinfo->fbs_lst[info->node -2].info->var.green.offset     = info->var.green.offset;
            cncfbinfo->fbs_lst[info->node -2].info->var.green.length     = info->var.green.length;
            cncfbinfo->fbs_lst[info->node -2].info->var.green.msb_right  = info->var.green.msb_right;
            cncfbinfo->fbs_lst[info->node -2].info->var.blue.offset      = info->var.blue.offset;
            cncfbinfo->fbs_lst[info->node -2].info->var.blue.length      = info->var.blue.length;
            cncfbinfo->fbs_lst[info->node -2].info->var.blue.msb_right   = info->var.blue.msb_right;
            //            cncfbinfo->fbs_lst[info->node -2].info->var.yres_virtual     = cncfbinfo->fbs_lst[info->node -2].info->fix.smem_len / cncfbinfo->fbs_lst[info->node -2].info->fix.line_length ;
            //cncfbinfo->fbs_lst[info->node -2].info->var.vmode   = info->var.vmode;
        }
	} else {
            cncfbinfo->fbs_lst[info->node ].info->fix.line_length    = get_line_length(info->var.xres_virtual, info->var.bits_per_pixel);
            memcpy(&(cncfbinfo->fbs_lst[info->node ].info->var), &(info->var), sizeof(struct fb_var_screeninfo));
            //            cncfbinfo->fbs_lst[info->node].info->var.yres_virtual     = info->fix.smem_len / cncfbinfo->fbs_lst[info->node].info->fix.line_length ;
    }
    info->fix.visual = (info->var.bits_per_pixel <= 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;
    if (info->var.bits_per_pixel != 0) {
        info->fix.line_length = get_line_length(info->var.xres_virtual, info->var.bits_per_pixel);
        //        info->var.yres_virtual = info->fix.smem_len / info->fix.line_length;
        DEBUG("[set_par]fix.line_length=0x%x, var.activate=%d\n", info->fix.line_length, info->var.activate);
    }

    return cncfb_hw_set_par(info, cncfbinfo);

}

/*
 * Set a single color register. The values supplied are already
 * rounded down to the hardware's capabilities (according to the
 * entries in the var structure). Return != 0 for invalid regno.
 */
static int cncfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int transp, struct fb_info *info)
{
    if (info == NULL ) {
		DEBUG("CNCFB: %s failed!\n", __FUNCTION__);
		return -EINVAL;

	}
	if (regno >= 256) return 1; /* no. of hw registers */
	/*
	 * Program hardware... do anything you want with transp
	 */

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue =
			(red * 77 + green * 151 + blue * 28) >> 8;
	}

	return cncfb_hw_setcolreg(regno, red, green, blue, transp, info, cncfbinfo);
}

/*
 * Pan or Wrap the Display
 *
 * This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 */
static int cncfb_pan_display(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
    if (var== NULL || info == NULL)
        return -EINVAL;

	if (var->vmode & FB_VMODE_YWRAP) {
		return -EINVAL;
	} else {
		if (var->xoffset + var->xres > info->var.xres_virtual ||
				var->yoffset + var->yres > info->var.yres_virtual)
			return -EINVAL;
	}

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	return cncfb_hw_pan_display(info, cncfbinfo);
}

/*
 * Blank (disable) the Graphics Layer
 *
 */
static int cncfb_blank(int blank, struct fb_info *info)
{
    if (info == NULL ){
		DEBUG("CNCFB: %s failed!\n", __FUNCTION__);
		return -EINVAL;
	}
	DEBUG("CNCFB: %s in!\n", __FUNCTION__);
	return cncfb_hw_blank(blank);
}


static int USE2D_COMP_NOTSCALOR = 1; /* 2D scalor or comp while 576i ? 0:scalor, 1:comp*/
static int cncfb_ioctl(struct fb_info* info, unsigned int cmd, unsigned long arg)
{

    if (info == NULL )
        return -EINVAL;
    if (cncfbinfo->fbs_lst[info->node].info == NULL)
        return -EINVAL;
#if !defined(CONFIG_FORCE_TWO_OSD_LAYER) && !defined(CONFIG_TWO_OSD_LAYER_SUPPORT)
    if (cncfbinfo->fbs_lst[0].info != NULL) {
        if ( cncfbinfo->fbs_lst[0].info->var.bits_per_pixel == 32 &&
             (info->node == 1 || info->node == 3))
            return 0;
	}
#endif
    switch(cmd) {
		case FBIO_WAITFORVSYNC:
			return cncfb_hw_waitforvsync(info);

		case FBIO_GFX_ON:
			return cncfb_hw_gfx_on(info, arg, cncfbinfo);

		case FBIO_GFX_ALPHA:
			return cncfb_hw_gfx_alpha(info, arg, cncfbinfo);

		case FBIO_GFX_BGCOLOR:

			return cncfb_hw_setbgcolor(info,arg);


		case FBIO_Z_ORDER:
			return cncfb_hw_z_order(info, arg);

		case FBIO_GFX_COLORKEY_ON:
			return cncfb_hw_colorkey_on(info, arg, cncfbinfo);

		case FBIO_GFX_COLORKEY_VAL:
			{
				gfx_colorkey col_key = {
					0x00, 0x00, 0x00,
					0x00, 0x00, 0x00
				};

				if (NULL != (void *)arg)
					if (copy_from_user(&col_key, (void*)arg, sizeof(col_key))) return -EFAULT;

				return cncfb_hw_colorkey_val(info, &col_key, cncfbinfo);
			}

		case FBIO_GFX_FLIP:
			{
				gfx2d_scalor_params blit_conf, blit_conf2;

				if (info->node < 2) return -EINVAL; /* FIXME@zhongkai's ugly code. */

				blit_conf.src_rect.left = 0;
				blit_conf.src_rect.top = 0;
				blit_conf.src_rect.right = info->var.xres;
				blit_conf.src_rect.bottom = info->var.yres;
				blit_conf.src_width = info->var.xres;
				blit_conf.src_height = info->var.yres;
				blit_conf.src_color_format = (info->var.bits_per_pixel == 32) ? 6 /* GFX_FMT_ARGB8888 */
					: (info->var.bits_per_pixel == 8) ? 5 /* GFX_FMT_CLUT8 */
					: (info->var.transp.length == 0)  ? 0 /* GFX_FMT_RGB565 */
					: (info->var.transp.length == 1)  ? 3 /* GFX_FMT_ARGB1555 */
					: (info->var.transp.length == 4)  ? 1 /* GFX_FMT_ARGB4444 */
					: 4; /* GFX_FMT_CLUT4 */
				blit_conf.src_pitch_line = info->fix.line_length;
				blit_conf.src_bits_pixel = info->var.bits_per_pixel;
				blit_conf.src_phy_addr = info->fix.smem_start;

				blit_conf.dst_rect.left = 0;
				blit_conf.dst_rect.top = 0;
				blit_conf.dst_rect.right = cncfbinfo->fbs_lst[info->node-2].info->var.xres;
				blit_conf.dst_rect.bottom = cncfbinfo->fbs_lst[info->node-2].info->var.yres;
				blit_conf.dst_width = cncfbinfo->fbs_lst[info->node-2].info->var.xres;
				blit_conf.dst_height = cncfbinfo->fbs_lst[info->node-2].info->var.yres;
				blit_conf.dst_color_format = blit_conf.src_color_format;
				blit_conf.dst_pitch_line = cncfbinfo->fbs_lst[info->node-2].info->fix.line_length;
				blit_conf.dst_bits_pixel = cncfbinfo->fbs_lst[info->node-2].info->var.bits_per_pixel;
//				blit_conf.dst_phy_addr = cncfbinfo->fbs_lst[info->node-2].info->fix.smem_start;
				blit_conf.dst_phy_addr = (cncfbinfo->fbs_lst[info->node-2].info->fix.smem_start +
				cncfbinfo->fbs_lst[info->node-2].info->fix.line_length * cncfbinfo->fbs_lst[info->node-2].info->var.yoffset +
				cncfbinfo->fbs_lst[info->node-2].info->var.xoffset * cncfbinfo->fbs_lst[info->node-2].info->var.bits_per_pixel / 8);
//				printk("blit_conf.dst_phy_addr = 0x%x, info->var.yoffset  = %d\n",blit_conf.dst_phy_addr, cncfbinfo->fbs_lst[info->node-2].info->var.yoffset );

				if (NULL != (void *)arg) {
					if (copy_from_user(&blit_conf2, (void*)arg, sizeof(blit_conf)))
						return -EFAULT;

					blit_conf.src_rect.left = blit_conf2.src_rect.left;
					blit_conf.src_rect.top = blit_conf2.src_rect.top;
					blit_conf.src_rect.right = blit_conf2.src_rect.right;
					blit_conf.src_rect.bottom = blit_conf2.src_rect.bottom;

					blit_conf.dst_rect.left = blit_conf2.dst_rect.left;
					blit_conf.dst_rect.top = blit_conf2.dst_rect.top;
					blit_conf.dst_rect.right = blit_conf2.dst_rect.right;
					blit_conf.dst_rect.bottom = blit_conf2.dst_rect.bottom;

					goto do_scaler;
				}

				/* for 576&480 */
				if (((blit_conf.src_rect.top-blit_conf.src_rect.bottom) !=
				     (blit_conf.dst_rect.top-blit_conf.dst_rect.bottom)) ||
				    ((blit_conf.src_rect.right-blit_conf.src_rect.left) !=
				     (blit_conf.dst_rect.right-blit_conf.dst_rect.left)))

					goto do_scaler;

				if ((cncfbinfo->fbs_lst[info->node-2].info->var.yres) <= 576 &&
				    (1 == cncfbinfo->fbs_lst[info->node-2].info->var.vmode)) {

					if(USE2D_COMP_NOTSCALOR == 0) { /* force 2D scalor */
						blit_conf.src_rect.left = 1;
						blit_conf.src_rect.top = 1;
						blit_conf.src_rect.right = info->var.xres - 1;
						blit_conf.src_rect.bottom = info->var.yres - 1;

					} else if(USE2D_COMP_NOTSCALOR == 1) { /* 2D comp */
						gfx2d_comp_params comp_param;
						comp_param.src1_rect.left = comp_param.src0_rect.left = blit_conf.src_rect.left;
						comp_param.src1_rect.top = comp_param.src0_rect.top = blit_conf.src_rect.top;
						comp_param.src1_rect.right = comp_param.src0_rect.right = blit_conf.src_rect.right;
						comp_param.src1_rect.bottom = comp_param.src0_rect.bottom = blit_conf.src_rect.bottom;
						comp_param.src1_width = comp_param.src0_width =
							blit_conf.src_rect.right - blit_conf.src_rect.left;// info->var.xres;
						comp_param.src1_height = comp_param.src0_height =
							blit_conf.src_rect.bottom - blit_conf.src_rect.top;//info->var.yres;
						comp_param.src1_color_format = comp_param.src0_color_format =
									       (info->var.bits_per_pixel == 32)  ? 6 /* GFX_FMT_ARGB8888 */
									       : (info->var.bits_per_pixel == 8) ? 5 /* GFX_FMT_CLUT8 */
									       : (info->var.transp.length == 0)  ? 0 /* GFX_FMT_RGB565 */
									       : (info->var.transp.length == 1)  ? 3 /* GFX_FMT_ARGB1555 */
									       : (info->var.transp.length == 4)  ? 1 /* GFX_FMT_ARGB4444 */
									       : 4; /* GFX_FMT_CLUT4 */
						comp_param.src1_pitch_line = comp_param.src0_pitch_line = info->fix.line_length;
						comp_param.src1_bits_pixel = comp_param.src0_bits_pixel = info->var.bits_per_pixel;
						comp_param.src0_phy_addr = info->fix.smem_start;
						comp_param.src1_phy_addr = info->fix.smem_start + comp_param.src1_pitch_line;

						comp_param.dst_rect.left = blit_conf.dst_rect.left;
						comp_param.dst_rect.top = blit_conf.dst_rect.top;
						comp_param.dst_rect.right = blit_conf.dst_rect.right;
						comp_param.dst_rect.bottom = blit_conf.dst_rect.bottom;
						comp_param.dst_width =
							blit_conf.dst_rect.right - blit_conf.dst_rect.left;// cncfbinfo->fbs_lst[info->node-2].info->var.xres;
						comp_param.dst_height =
							blit_conf.dst_rect.bottom - blit_conf.dst_rect.top;// cncfbinfo->fbs_lst[info->node-2].info->var.yres;
						comp_param.dst_color_format = comp_param.src0_color_format;
						comp_param.dst_pitch_line = cncfbinfo->fbs_lst[info->node-2].info->fix.line_length;
						comp_param.dst_bits_pixel = cncfbinfo->fbs_lst[info->node-2].info->var.bits_per_pixel;
						comp_param.dst_phy_addr = cncfbinfo->fbs_lst[info->node-2].info->fix.smem_start;

						DFB_2DComposite(&comp_param);

						return 0;
					}
				}
do_scaler:
				DFB_2DScalor(&blit_conf);
				break;
			}
		default:
			return -EINVAL;
	}

	return 0;
}

/*
 * Initialisation
 */
static void cncfb_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero. */
}

struct fb_info *gfx = NULL, *gfx2 = NULL, *gfx3 = NULL, *gfx4=NULL;
static struct proc_dir_entry *fb_proc_entry = NULL;

static int fb_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u32 addr;
	u32 val;

	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = df_readl(addr);
		printk(" readw [0x%04x] = 0x%08x \n", addr, val);
	}
	else if (strncmp("wl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = simple_strtol(&cmd_line[7], NULL, 16);
		df_writel(addr, val);
	}
	else if (strncmp("2dst", cmd_line, 4) == 0) {
		int i;
		for (i = 0; i <= 0x8c; i+=4) {
			val = Gfx_ReadRegs(i/4);
			printk(" 2d status [0x%04x] = 0x%08x \n", i, val);
		}
	}
	else if (strncmp("0", cmd_line, 1) == 0) {
		USE2D_COMP_NOTSCALOR = 0;
	}
	else if (strncmp("1", cmd_line, 1) == 0) {
		USE2D_COMP_NOTSCALOR = 1;
	}

	return count;
}

void __init fix_cncfb_default_and_fix(int mode, int color_format)
{
	switch(mode){
		case TVOUT_MODE_576I:
		case TVOUT_MODE_PAL_N:
		case TVOUT_MODE_PAL_CN:
		case TVOUT_MODE_SECAM:
			cncfb_default.xres = 720;
			cncfb_default.yres = 576;
			cncfb_default.xres_virtual = 720;
			cncfb_default.yres_virtual = 576;
			cncfb_default.vmode = FB_VMODE_INTERLACED;
			cncfb_fix.line_length = 1440;
			break;
		case TVOUT_MODE_576P:
			cncfb_default.xres = 720;
			cncfb_default.yres = 576;
			cncfb_default.xres_virtual = 720;
			cncfb_default.yres_virtual = 576;
			cncfb_fix.line_length = 1440;
			break;
		case TVOUT_MODE_480I:
		case TVOUT_MODE_PAL_M:
			cncfb_default.xres = 720;
			cncfb_default.yres = 480;
			cncfb_default.xres_virtual = 720;
			cncfb_default.yres_virtual = 480;
			cncfb_default.vmode = FB_VMODE_INTERLACED;
			cncfb_fix.line_length = 1440;
			break;
		case TVOUT_MODE_480P:
			cncfb_default.xres = 720;
			cncfb_default.yres = 480;
			cncfb_default.xres_virtual = 720;
			cncfb_default.yres_virtual = 480;
			cncfb_fix.line_length = 1440;
			break;
		case TVOUT_MODE_720P50:
		case TVOUT_MODE_720P60:
		case TVOUT_RGB_1280X720_60FPS:
			cncfb_default.xres = 1280;
			cncfb_default.yres = 720;
			cncfb_default.xres_virtual = 1280;
			cncfb_default.yres_virtual = 720;
			cncfb_fix.line_length = 2560;
			break;
		case TVOUT_MODE_1080I25:
		case TVOUT_MODE_1080I30:
			cncfb_default.xres = 1920;
			cncfb_default.yres = 1080;
			cncfb_default.xres_virtual = 1920;
			cncfb_default.yres_virtual = 1080;
			cncfb_default.vmode = FB_VMODE_INTERLACED;
			cncfb_fix.line_length = 3840;
			break;
		case TVOUT_MODE_1080P24:
		case TVOUT_MODE_1080P25:
		case TVOUT_MODE_1080P30:
		case TVOUT_MODE_1080P50:
		case TVOUT_MODE_1080P60:
		case TVOUT_RGB_1920X1080I30:
		case TVOUT_RGB_1920X1080P60:
		case TVOUT_RGB_1920X1080P30:
			cncfb_default.xres = 1920;
			cncfb_default.yres = 1080;
			cncfb_default.xres_virtual = 1920;
			cncfb_default.yres_virtual = 1080;
			cncfb_fix.line_length = 3840;
			break;
		case TVOUT_RGB_640X480_60FPS:
			cncfb_default.xres = 640;
			cncfb_default.yres = 480;
			cncfb_default.xres_virtual = 640;
			cncfb_default.yres_virtual = 480;
			cncfb_fix.line_length = 1280;
			break;
		case TVOUT_RGB_800X600_60FPS:
			cncfb_default.xres = 800;
			cncfb_default.yres = 600;
			cncfb_default.xres_virtual = 800;
			cncfb_default.yres_virtual = 600;
			cncfb_fix.line_length = 1600;
			break;
		case TVOUT_RGB_1024X768_60FPS:
			cncfb_default.xres = 1024;
			cncfb_default.yres = 768;
			cncfb_default.xres_virtual = 1024;
			cncfb_default.yres_virtual = 768;
			cncfb_fix.line_length = 2048;
			break;
		case TVOUT_RGB_1280X1024_60FPS:
			cncfb_default.xres = 1280;
			cncfb_default.yres = 1024;
			cncfb_default.xres_virtual = 1280;
			cncfb_default.yres_virtual = 1024;
			cncfb_fix.line_length = 3560;
			break;
		case TVOUT_RGB_1360X768_60FPS:
			cncfb_default.xres = 1360;
			cncfb_default.yres = 768;
			cncfb_default.xres_virtual = 1360;
			cncfb_default.yres_virtual = 768;
			cncfb_fix.line_length = 3720;
			break;
		case TVOUT_RGB_1366X768_60FPS:
			cncfb_default.xres = 1366;
			cncfb_default.yres = 768;
			cncfb_default.xres_virtual = 1366;
			cncfb_default.yres_virtual = 768;
			cncfb_fix.line_length = 3732;
			break;
		case TVOUT_RGB_1440X900_60FPS:
			cncfb_default.xres = 1440;
			cncfb_default.yres = 900;
			cncfb_default.xres_virtual = 1440;
			cncfb_default.yres_virtual = 900;
			cncfb_fix.line_length = 2880;
			break;
		default:
			cncfb_default.xres = 1920;
			cncfb_default.yres = 1080;
			cncfb_default.xres_virtual = 1920;
			cncfb_default.yres_virtual = 1080;
			cncfb_fix.line_length = 3840;
			break;
	}

	switch (color_format){
		case 2:
			cncfb_default.bits_per_pixel = 16;
			cncfb_default.red.offset = 11;
			cncfb_default.red.length = 5;
			cncfb_default.green.offset = 5;
			cncfb_default.green.length = 6;
			cncfb_default.blue.offset = 0;
			cncfb_default.blue.length = 5;
			cncfb_default.transp.offset = 0;
			cncfb_default.transp.length = 0;
			break;
		case 6:
			cncfb_default.bits_per_pixel = 32;
			cncfb_default.red.offset = 16;
			cncfb_default.red.length = 8;
			cncfb_default.green.offset = 8;
			cncfb_default.green.length = 8;
			cncfb_default.blue.offset = 0;
			cncfb_default.blue.length = 8;
			cncfb_default.transp.offset = 24;
			cncfb_default.transp.length = 8;
			cncfb_fix.line_length = cncfb_fix.line_length*2;
			break;
		default:
			cncfb_default.bits_per_pixel = 32;
			cncfb_default.red.offset = 16;
			cncfb_default.red.length = 8;
			cncfb_default.green.offset = 8;
			cncfb_default.green.length = 8;
			cncfb_default.blue.offset = 0;
			cncfb_default.blue.length = 8;
			cncfb_default.transp.offset = 24;
			cncfb_default.transp.length = 8;
			cncfb_fix.line_length = cncfb_fix.line_length*2;
			break;
	}
}

static int __init cncfb_probe(struct platform_device *device)
{
	int retval = -ENOMEM;

	if (cncfb_fix.smem_len >0) {
	        gfx = framebuffer_alloc(sizeof(unsigned int) * 256, &(device->dev));
	        if (NULL == gfx)
	            goto GFX_ERR;

	        if (NULL == (gfx->screen_base = ioremap(cncfb_fix.smem_start, cncfb_fix.smem_len)))
	            goto MMAP_ERR;

	        fb_proc_entry = create_proc_entry("fb_io", 0, NULL);
	        if (NULL != fb_proc_entry) {
	            fb_proc_entry->write_proc = &fb_proc_write;
	        }

		fix_cncfb_default_and_fix(outputmode, colorformat);

		gfx->fbops = &cncfb_ops;
		gfx->var = cncfb_default;
		gfx->fix = cncfb_fix;
		gfx->pseudo_palette = gfx->par;
		gfx->par = NULL;
		gfx->flags = FBINFO_FLAG_DEFAULT;
		/* update default xres_virtual and yres_virtual following fix screen info */
		//        gfx->var.xres_virtual = gfx->var.xres; /* we don't enable now x pan display */

		gfx->fix.line_length =  get_line_length(gfx->var.xres_virtual, gfx->var.bits_per_pixel);
		gfx->var.yres_virtual = gfx->fix.smem_len / gfx->fix.line_length;
		if (fb_alloc_cmap(&gfx->cmap, 256, 0) < 0) goto CMAP_ERR;
		if (register_framebuffer(gfx) < 0) goto RF_ERR;

		cncfbinfo->fbs_lst[0].onoff = 0;

		cncfbinfo->fbs_lst[0].info = gfx;

		if(-1 == outputmode)
			memset(gfx->screen_base, 0, FB0_SIZE);

		printk(KERN_INFO "fb%d: CNC frame buffer @[0x%lx, 0x%lx] size 0x%lx\n",
		gfx->node, gfx->fix.smem_start, (u_long)gfx->screen_base, (u_long)gfx->fix.smem_len);
		Gfx_2DInit();
	}
	else {
		gfx = NULL;
		cncfbinfo->fbs_lst[0].info = gfx;
		cncfbinfo->fbs_lst[0].onoff = 0;
	}

	/* to register the second framebuffer */
	if ( gfx != NULL) {
		if (NULL == (gfx2 = framebuffer_alloc(sizeof(u32) * 256, &device->dev)))
			goto GFX2_ERR;
		if (cncfb_fix2.smem_len > 0) {
			if (NULL == (gfx2->screen_base = ioremap(cncfb_fix2.smem_start, cncfb_fix2.smem_len)))
				goto MMAP2_ERR;
	//         memset(gfx2->screen_base, 0, FB1_SIZE);
	//          gfx2->fix = cncfb_fix2;
		} else {
			gfx2->screen_base = gfx->screen_base;  // if fb1 buffer size is 0, then it pointe fb0
			gfx2->fix = cncfb_fix;
		}

		gfx2->fbops = &cncfb_ops;
		gfx2->var = cncfb_default;
		/* update default xres_virtual and yres_virtual following fix screen info */
		//        gfx->var.xres_virtual = gfx->var.xres; /* we don't enable now x pan display */

		gfx2->fix.line_length =  get_line_length(gfx2->var.xres_virtual, gfx2->var.bits_per_pixel);
		gfx2->var.yres_virtual = gfx2->fix.smem_len / gfx2->fix.line_length;
		gfx2->pseudo_palette = gfx2->par;
		gfx2->par = NULL;
		gfx2->flags = FBINFO_FLAG_DEFAULT;

		if (fb_alloc_cmap(&gfx2->cmap, 256, 0) < 0) goto CMAP2_ERR;
		if (register_framebuffer(gfx2) < 0) goto RF2_ERR;

		cncfbinfo->fbs_lst[1].info = gfx2;
		cncfbinfo->fbs_lst[1].onoff = 0;

		printk(KERN_INFO "fb%d: CNC frame buffer @[0x%lx, 0x%lx] size 0x%lx\n",
		gfx2->node, gfx2->fix.smem_start, (u_long)gfx2->screen_base, (u_long)gfx2->fix.smem_len);
	}
	else {
		gfx2 = NULL;
		cncfbinfo->fbs_lst[1].info = gfx2;
		cncfbinfo->fbs_lst[1].onoff = 0;
	}

	/* to register the third framebuffer */
	if (cncfb_fix3.smem_len >0 && gfx!= NULL) {
		if (NULL == (gfx3 = framebuffer_alloc(sizeof(u32) * 256, &device->dev))) goto GFX3_ERR;
		if (NULL == (gfx3->screen_base = ioremap(cncfb_fix3.smem_start, cncfb_fix3.smem_len))) goto MMAP3_ERR;

		gfx3->fbops = &cncfb_ops;
		gfx3->var = cncfb_default;
		gfx3->fix = cncfb_fix3;
		gfx3->pseudo_palette = gfx3->par;
		gfx3->par = NULL;
		gfx3->flags = FBINFO_FLAG_DEFAULT;
		/* update default xres_virtual and yres_virtual following fix screen info */
		//        gfx->var.xres_virtual = gfx->var.xres; /* we don't enable now x pan display */

		gfx3->fix.line_length =  get_line_length(gfx3->var.xres_virtual, gfx3->var.bits_per_pixel);
		gfx3->var.yres_virtual = gfx3->fix.smem_len / gfx3->fix.line_length;
		if (fb_alloc_cmap(&gfx3->cmap, 256, 0) < 0) goto CMAP3_ERR;
		if (register_framebuffer(gfx3) < 0) goto RF3_ERR;

		cncfbinfo->fbs_lst[2].onoff = 0;
		cncfbinfo->fbs_lst[2].info = gfx3;

		if(-1 == outputmode)
			memset(gfx3->screen_base, 0, cncfb_fix3.smem_len);

		printk(KERN_INFO "fb%d: CNC frame buffer @[0x%lx, 0x%lx] size 0x%lx\n",
		gfx3->node, gfx3->fix.smem_start, (u_long)gfx3->screen_base, (u_long)gfx3->fix.smem_len);
	}
	else {
		gfx3= NULL;
		cncfbinfo->fbs_lst[2].info = gfx3;
		cncfbinfo->fbs_lst[2].onoff = 0;
	}

	/* to register the 4th framebuffer */
	if (cncfb_fix4.smem_len >0 && gfx2 != NULL) {
		if (NULL == (gfx4 = framebuffer_alloc(sizeof(u32) * 256, &device->dev))) goto GFX4_ERR;
		if (NULL == (gfx4->screen_base = ioremap(cncfb_fix4.smem_start, cncfb_fix4.smem_len))) goto MMAP4_ERR;

		gfx4->fbops = &cncfb_ops;
		gfx4->var = cncfb_default;
		gfx4->fix = cncfb_fix4;
		gfx4->pseudo_palette = gfx4->par;
		gfx4->par = NULL;
		gfx4->flags = FBINFO_FLAG_DEFAULT;
		cncfbinfo->fbs_lst[3].onoff = 0;
		/* update default xres_virtual and yres_virtual following fix screen info */
		//        gfx->var.xres_virtual = gfx->var.xres; /* we don't enable now x pan display */
		gfx4->var.yres_virtual = gfx4->fix.smem_len / (gfx4->var.xres_virtual * (gfx4->var.bits_per_pixel / 8 ));

		gfx4->fix.line_length =  get_line_length(gfx4->var.xres_virtual, gfx4->var.bits_per_pixel);
		gfx4->var.yres_virtual = gfx4->fix.smem_len / gfx4->fix.line_length;
		if (fb_alloc_cmap(&gfx4->cmap, 256, 0) < 0) goto CMAP4_ERR;
		if (register_framebuffer(gfx4) < 0) goto RF4_ERR;

		cncfbinfo->fbs_lst[3].info = gfx4;

		if(-1 == outputmode)
			memset(gfx4->screen_base, 0, cncfb_fix4.smem_len);
	}
	else {
		gfx4= NULL;
		cncfbinfo->fbs_lst[3].info = gfx4;
		cncfbinfo->fbs_lst[3].onoff = 0;
	}

	/* to initialize the first framebuffer */
	if (gfx != NULL){
		cncfb_hw_init(gfx, 0, cncfbinfo);
	}
	if (gfx2 != NULL)
		cncfb_hw_init(gfx2, 0, cncfbinfo);

	return 0;

RF4_ERR:
CMAP4_ERR:
MMAP4_ERR:
GFX4_ERR:
	/* do something here. */
	printk("Init framebuffer 4 failed!\n");
RF3_ERR:
CMAP3_ERR:
MMAP3_ERR:
GFX3_ERR:
	/* do something here. */
	printk("Init framebuffer 3 failed!\n");
RF2_ERR:
	if (gfx2 != NULL)
		fb_dealloc_cmap(&gfx2->cmap);
CMAP2_ERR:
	if (gfx2 != NULL)
		iounmap(gfx2->screen_base);
MMAP2_ERR:

GFX2_ERR:
	if (gfx != NULL)
		framebuffer_release(gfx);
RF_ERR:
	if (gfx != NULL)
		fb_dealloc_cmap(&gfx->cmap);
CMAP_ERR:
	if (gfx != NULL)
		iounmap(gfx->screen_base);
MMAP_ERR:
GFX_ERR:
	return retval;
}

static int cncfb_remove(struct platform_device *device)
{
	int i;
	struct fb_info *info;

	for (i = 0; i < FB_NUMS; i++) {
		info = cncfbinfo->fbs_lst[i].info;
		if (info) {
			unregister_framebuffer(info);
			iounmap(info->screen_base);
			framebuffer_release(info);
		}
	}

	return 0;
}

static struct platform_driver cncfb_driver = {
	.probe  = cncfb_probe,
	.remove = cncfb_remove,
	.driver = {
		.name   = "cncfb",
		.owner  = THIS_MODULE,
	}
};

static struct platform_device cncfb_device = {
	.name   = "cncfb",
	.id     = 0,
	// .dev    = {
	//     //    .dma_mask = ~(u32)0;

	// }
	.num_resources = 0,
};

static int __init cncfb_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&cncfb_driver);
	if (ret) {
		printk(KERN_ERR "failed to register cnc framebuffer driver\n");
		return -ENODEV;
	}

	if (!ret) {
		ret = platform_device_register(&cncfb_device);
		if (ret) {
			platform_driver_unregister(&cncfb_driver);
			printk(KERN_ERR "failed to register cnc framebuffer device\n");
		}
	}

	return ret;
}

module_init(cncfb_init);

#ifdef MODULE
static void __exit cncfb_exit(void)
{
	cncfb_hw_exit();

	platform_device_unregister(&cncfb_device);
	platform_driver_unregister(&cncfb_driver);
}

module_exit(cncfb_exit);

MODULE_LICENSE("GPL");

#endif /* MODULE */

