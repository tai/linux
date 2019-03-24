/*
 *  linux/drivers/video/celestial/cncfb.h -- celestial frame buffer device
 *
 *  Copyright (C) 2010 Celestial Semiconductor
 *  Copyright (C) 2011 Cavium
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */


#ifndef __CSMFB_DEV_H__
#define __CSMFB_DEV_H__

typedef struct _gfx_colorkey {
	char r_min;
	char r_max;

	char g_min;
	char g_max;

	char b_min;
	char b_max;
}gfx_colorkey;


#define FB_NUMS		4

typedef struct cnc_info 
{
	struct {		
		struct fb_info *info;

		/* framebuffer area */
		unsigned long fb_base_phys;
		unsigned long fb_base;
		unsigned long fb_size;

		/* to map the registers */
		dma_addr_t    mmio_base_phys;
		unsigned long mmio_base;
		unsigned long mmio_size;
		unsigned int  onoff;
	} fbs_lst[FB_NUMS];

	wait_queue_head_t vsync_wait;
	unsigned long     vsync_cnt;
	int timeout;
	struct device *dev;
}cncinfo;

/* 
 * Z-Order numbers are from hardware, don't redefine them! 
 */
#define Z_ORDER_V0_V1_G         0x0123
#define Z_ORDER_V0_G_V1         0x0213
#define Z_ORDER_V1_V0_G         0x1023
#define Z_ORDER_V1_G_V0         0x2013
#define Z_ORDER_G_V0_V1         0x1203
#define Z_ORDER_G_V1_V0         0x2103

#define FBIO_WAITFORVSYNC       _IOW('F', 0x20, u_int32_t)
#define FBIO_GFX_ON             _IOW('F', 0x21, u_int32_t)
#define FBIO_GFX_ALPHA          _IOW('F', 0x22, u_int32_t)
#define FBIO_Z_ORDER            _IOW('F', 0x50, u_int32_t)
#define FBIO_GFX_FLIP		    _IOW('F', 0x51, gfx2d_scalor_params)
#define FBIO_GFX_COLORKEY_ON	_IOW('F', 0x52, u_int32_t)
#define FBIO_GFX_COLORKEY_VAL	_IOW('F', 0x53, gfx_colorkey)
#define FBIO_GFX_BGCOLOR	    _IOW('F', 0x54, u_int32_t)

#endif
