/*
 *  linux/drivers/video/celestial/cnc1800h_fb_hw.c -- celestial frame buffer device
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

#include "cncfb.h"
#include <mach/cnc1800h_df.h>
#include <mach/df_reg_def.h>
#include <mach/df_reg_fmt.h>

#define FB_XRES_LIMIT_SIZE 1024

extern CNC_FB_INTERRUPT cncdf_1800_vblank_int[2];
extern int __Is_TV;
extern df_reg_para dfreg;
extern int Get_Outif_Id(int gfx_id);
extern void __df_update_start(void);
extern void __df_update_end(void);
extern unsigned int DF_Read(unsigned int addr);
extern void DF_Write(unsigned int addr, unsigned int data);
extern void DFGfxEna(int GfxId, int IsEna, int IsRGB2YUVEna);
extern void DFGfxSetAlpha(int GfxId, unsigned char DefaultAlpha, unsigned char ARGB1555Alpha0, unsigned char ARGB1555Alpha1);
extern void DFGfxGetAlpha(int GfxId, unsigned char *DefaultAlpha, unsigned char *ARGB1555Alpha0, unsigned char *ARGB1555Alpha1);
extern void DFGfxColorKeyEna(int GfxId,int IsEna);
extern void DFGfxSetColorKey(int GfxId, unsigned char KeyRMin, unsigned char KeyRMax, unsigned char KeyGMin, unsigned char KeyGMax, unsigned char KeyBMin, unsigned char KeyBMax);
extern void DFSetZOrder(int OutIFId, int Gfx1ZOrder, int Gfx2ZOrder, int Video1ZOrder, int Video2ZOrder);
extern void DFSetBackGroud(int OutIFId, unsigned char BGY, unsigned char BGU, unsigned char BGV);


typedef enum _DF_GFX_COLOR_FORMAT_
{
	DF_GFX_CLUT4     = 0,
	DF_GFX_CLUT8     = 1,
	DF_GFX_RGB565    = 2,
	DF_GFX_ARGB4444  = 3,
	DF_GFX_A0        = 4,
	DF_GFX_ARGB1555  = 5,
	DF_GFX_ARGB8888  = 6,
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
	DISP_GFX1_SCL_X_POSITON,	//=10
	DISP_GFX1_CLUT_ADDR,	//=11
	DISP_GFX1_CLUT_DATA	//=12
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
	DISP_GFX2_SCL_X_POSITON,
	DISP_GFX2_CLUT_ADDR,
	DISP_GFX2_CLUT_DATA
	},
};



/************************************* FUNCTION *****************************************/


int update_gfx_layer(struct fb_info *info, int init, struct cnc_info * cncfbinfo)
{
	unsigned int outif_id = 0;
	int i_node = info->node;
    unsigned int display_bandwidth_factor = 1;
	unsigned char defaultalpha, argb1555a0,argb1555a1;

	if (i_node > 1) i_node -= 2;
//	outif_id = Get_Outif_Id(i_node);

	//if (info->node > 1) return 0; /* fb2 and fb3 are virtual framebuffer */
	__df_update_start();

	/* if info->node > 1,  only update gfx color format */

#ifndef CONFIG_FORCE_TWO_OSD_LAYER
    /* disable the second osd layer with 32bit true color */
    if ((info->node == 1 || info->node == 3) && info->var.bits_per_pixel == 32) {
        __df_update_end();
        return 0;
    }


	//    printk("info->node=%d\n, xres=%d\n",info->node, info->var.xres);
    if (info->node == 0 && info->var.bits_per_pixel == 32) {
        display_bandwidth_factor = 2;

    }
    else {
        display_bandwidth_factor = 1;
    }


#endif

	if (info->node == 2 || info->node == 3) {
		dfreg.Gfx[info->node -2].df_gfx_format_reg.val = DF_Read(dfreg_gfx[info->node -2][1]);
		dfreg.Gfx[info->node -2].df_gfx_line_pitch_reg.val = DF_Read(dfreg_gfx[info->node -2][7]);

        //		dfreg.Gfx[info->node -2].df_gfx_line_pitch_reg.bits.iLinePitch = info->fix.line_length >> 4;
		dfreg.Gfx[info->node - 2].df_gfx_format_reg.bits.iColorFormat =
			(info->var.bits_per_pixel == 4) ? DF_GFX_CLUT4 :
			(info->var.bits_per_pixel == 8) ? DF_GFX_CLUT8 :
			(info->var.transp.length == 0) ? DF_GFX_RGB565 :
			(info->var.transp.length == 1) ? DF_GFX_ARGB1555 :
			(info->var.transp.length == 4) ? DF_GFX_ARGB4444 :

			(info->var.transp.length == 8) ? DF_GFX_ARGB8888 :
			(info->var.transp.length == 32) ? DF_GFX_A0 :

			dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iColorFormat;

		if(dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iColorFormat == DF_GFX_ARGB8888){

#ifdef CONFIG_CNC1800H_GFX_ENABLE_ARGB

			dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i16BitEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i128BitEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iByteEndian = 0;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian = 1;

#else
			dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i16BitEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i128BitEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iByteEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian = 0;
#endif
        }
		else{
			dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i16BitEndian = 0;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i128BitEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iByteEndian = 1;
            dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian = 0;
        }

		DF_Write((dfreg_gfx[info->node -2][1]), dfreg.Gfx[info->node -2].df_gfx_format_reg.val);
		//DF_Write((dfreg_gfx[info->node -2][7]), dfreg.Gfx[info->node -2].df_gfx_line_pitch_reg.val);

        if (display_bandwidth_factor == 2 && info->node == 2) {
            dfreg.Gfx[info->node -1].df_gfx_format_reg.val = DF_Read(dfreg_gfx[info->node -1][1]);
            //		dfreg.Gfx[info->node -2].df_gfx_line_pitch_reg.bits.iLinePitch = info->fix.line_length >> 4;
            dfreg.Gfx[info->node - 1].df_gfx_format_reg.bits.iColorFormat =
                dfreg.Gfx[info->node - 2].df_gfx_format_reg.bits.iColorFormat;


			dfreg.Gfx[info->node -1].df_gfx_format_reg.bits.i16BitEndian = dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i16BitEndian;
            dfreg.Gfx[info->node -1].df_gfx_format_reg.bits.i128BitEndian = dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.i128BitEndian;
            dfreg.Gfx[info->node -1].df_gfx_format_reg.bits.iByteEndian = dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iByteEndian;
            dfreg.Gfx[info->node -1].df_gfx_format_reg.bits.iNibbleEndian = dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian;

            // dfreg.Gfx[0].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[0][8]); // read osd 0 value
            // dfreg.Gfx[1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[1][8]); // read osd 1 value
            // dfreg.Gfx[0].df_gfx_buf_start_addr_reg.val = DF_Read(dfreg_gfx[0][6]);
            // dfreg.Gfx[1].df_gfx_buf_start_addr_reg.val = DF_Read(dfreg_gfx[1][6]);
            // dfreg.Gfx[0].df_gfx_control_reg.val = DF_Read(dfreg_gfx[0][0]);
            // dfreg.Gfx[1].df_gfx_control_reg.val = DF_Read(dfreg_gfx[1][0]);

            // dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd = (cncfbinfo->fbs_lst[0].info->var.left_margin +
            //                                                  cncfbinfo->fbs_lst[0].info->var.xres)/display_bandwidth_factor - 1;

            // dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXStart = dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd +1;

            // dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXEnd = (dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXStart
            //                                                           + cncfbinfo->fbs_lst[0].info->var.xres / display_bandwidth_factor)  - 1;
            // dfreg.Gfx[0].df_gfx_buf_start_addr_reg.val = DF_Read(dfreg_gfx[0][6]);
            // dfreg.Gfx[1].df_gfx_buf_start_addr_reg.bits.iStartAddr = ((dfreg.Gfx[0].df_gfx_buf_start_addr_reg.bits.iStartAddr << 4) +
            //                                                           (cncfbinfo->fbs_lst[0].info->var.xres)/display_bandwidth_factor *
            //                                                           4 ) >> 4;



            // dfreg.Gfx[1].df_gfx_control_reg.val = dfreg.Gfx[0].df_gfx_control_reg.val;
            // DF_Write((dfreg_gfx[1][0]), dfreg.Gfx[1].df_gfx_control_reg.val);

            // DF_Write((dfreg_gfx[1][6]), dfreg.Gfx[1].df_gfx_buf_start_addr_reg.val);
            // DF_Write((dfreg_gfx[0][8]), dfreg.Gfx[0].df_gfx_x_position_reg.val);
            // DF_Write((dfreg_gfx[1][8]), dfreg.Gfx[1].df_gfx_x_position_reg.val);

            DF_Write((dfreg_gfx[info->node -1][1]), dfreg.Gfx[info->node -1].df_gfx_format_reg.val);

        }
#ifndef CONFIG_FORCE_TWO_OSD_LAYER
        else if (display_bandwidth_factor == 1 && info->node == 2 ) {
            if (cncfbinfo->fbs_lst[3].info != NULL ) {
                dfreg.Gfx[info->node -1].df_gfx_format_reg.val = DF_Read(dfreg_gfx[info->node -1][1]);
                if(cncfbinfo->fbs_lst[3].info->var.bits_per_pixel == 32 ){

#ifdef CONFIG_CNC1800H_GFX_ENABLE_ARGB
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i16BitEndian = 1;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i128BitEndian = 1;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.iByteEndian = 0;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.iNibbleEndian = 1;

#else
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i16BitEndian = 1;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i128BitEndian = 1;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.iByteEndian = 1;
                    dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian = 0;
#endif
                }
                else {

                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i16BitEndian = 0;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.i128BitEndian = 1;
                    dfreg.Gfx[info->node -1 ].df_gfx_format_reg.bits.iByteEndian = 1;
                    dfreg.Gfx[info->node -2].df_gfx_format_reg.bits.iNibbleEndian = 0;
                }
                DF_Write((dfreg_gfx[info->node -1][1]), dfreg.Gfx[info->node -1].df_gfx_format_reg.val); // roll back colorformat for layer 2
                DFGfxEna(1, 0, __Is_TV); // disable osd layer 2
                dfreg.Gfx[0].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[0][8]); // read osd 0 value
                dfreg.Gfx[1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[1][8]); // read osd 1 value
                if (dfreg.Gfx[0].df_gfx_x_position_reg.val == dfreg.Gfx[1].df_gfx_x_position_reg.val) {
                    dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd = (dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd + 1) * 2 - 1;
                    dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXEnd = 0;   // reset osd 1 x position to 0;
                }

            } else {
                //                DFGfxSetAlpha(2, 0xff, 0, 0);
                DFGfxEna(1, 0, __Is_TV);
                //                DFGfxSetColorKey(2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                dfreg.Gfx[0].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[0][8]); // read osd 0 value
                dfreg.Gfx[1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[1][8]); // read osd 1 value
                if (dfreg.Gfx[0].df_gfx_x_position_reg.val == dfreg.Gfx[1].df_gfx_x_position_reg.val) {
                    dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd = (dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd + 1) * 2 - 1;
                    dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXEnd = 0;   // reset osd 1 x position to 0;
                }
            }
            DF_Write((dfreg_gfx[0][8]), dfreg.Gfx[0].df_gfx_x_position_reg.val);
            DF_Write((dfreg_gfx[1][8]), dfreg.Gfx[1].df_gfx_x_position_reg.val);
        }
#endif // not define Two osd layer
        __df_update_end();
        return 0;
    }

	dfreg.Gfx[info->node].df_gfx_format_reg.val = DF_Read(dfreg_gfx[info->node][1]);
	dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.val = DF_Read(dfreg_gfx[info->node][6]);
	dfreg.Gfx[info->node].df_gfx_line_pitch_reg.val = DF_Read(dfreg_gfx[info->node][7]);
	dfreg.Gfx[info->node].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[info->node][8]);
	dfreg.Gfx[info->node].df_gfx_y_position_reg.val = DF_Read(dfreg_gfx[info->node][9]);
	dfreg.Gfx[info->node].df_gfx_scl_x_position_reg.val = DF_Read(dfreg_gfx[info->node][10]);

	dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat =
		(info->var.bits_per_pixel == 4) ? DF_GFX_CLUT4 :
		(info->var.bits_per_pixel == 8) ? DF_GFX_CLUT8 :
		(info->var.transp.length == 0) ? DF_GFX_RGB565 :
		(info->var.transp.length == 1) ? DF_GFX_ARGB1555 :
		(info->var.transp.length == 4) ? DF_GFX_ARGB4444 :
		(info->var.transp.length == 8) ? DF_GFX_ARGB8888 :
		(info->var.transp.length == 32) ? DF_GFX_A0 :
		dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat;

    if(dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat == DF_GFX_ARGB8888){
    //	if(info->var.bits_per_pixel == 32){
#ifdef CONFIG_CNC1800H_GFX_ENABLE_ARGB
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.i16BitEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.i128BitEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iByteEndian = 0;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iNibbleEndian = 1;

#else
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.i16BitEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.i128BitEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iByteEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iNibbleEndian = 0;
#endif
    }
	else {

		dfreg.Gfx[info->node].df_gfx_format_reg.bits.i16BitEndian = 0;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.i128BitEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iByteEndian = 1;
        dfreg.Gfx[info->node].df_gfx_format_reg.bits.iNibbleEndian = 0;
    }

	dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.bits.iStartAddr =
	(info->fix.smem_start +
	info->fix.line_length * info->var.yoffset +
	info->var.xoffset * info->var.bits_per_pixel / 8) >> 4;

	dfreg.Gfx[info->node].df_gfx_line_pitch_reg.bits.iLinePitch = info->fix.line_length >>4;
	dfreg.Gfx[info->node].df_gfx_line_pitch_reg.bits.iBlankPixel = 0;

	dfreg.Gfx[info->node].df_gfx_x_position_reg.bits.iXStart = info->var.left_margin;
	dfreg.Gfx[info->node].df_gfx_x_position_reg.bits.iXEnd = (info->var.left_margin + info->var.xres)/display_bandwidth_factor - 1;
	dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYStart	=
		info->var.upper_margin / (info->var.vmode & FB_VMODE_INTERLACED ? 2 : 1);
	dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYEnd =
		(info->var.upper_margin + info->var.yres) /(info->var.vmode & FB_VMODE_INTERLACED ? 2 : 1);


	if(init) {
		DFGfxSetAlpha(info->node, 0xff, 0, 0);
		DFGfxEna(info->node, 1, __Is_TV);
		DFGfxSetColorKey(info->node, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
		DFGfxColorKeyEna(info->node, 1);
	}

	DF_Write((dfreg_gfx[info->node][6]), dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.val);
	DF_Write((dfreg_gfx[info->node][7]), dfreg.Gfx[info->node].df_gfx_line_pitch_reg.val);
	DF_Write((dfreg_gfx[info->node][8]), dfreg.Gfx[info->node].df_gfx_x_position_reg.val);
	DF_Write((dfreg_gfx[info->node][9]), dfreg.Gfx[info->node].df_gfx_y_position_reg.val);
	DF_Write((dfreg_gfx[info->node][1]), dfreg.Gfx[info->node].df_gfx_format_reg.val);


    /* handle osd layer 2 as half osd layer 1 */
    if (display_bandwidth_factor == 2 && info->node == 0) {

//		 printk("we setting osd layer 2 following layer 1 as half display to down bandwidth requirement\n");
        dfreg.Gfx[info->node + 1].df_gfx_format_reg.val = DF_Read(dfreg_gfx[info->node][1]);
        dfreg.Gfx[info->node + 1].df_gfx_buf_start_addr_reg.val = DF_Read(dfreg_gfx[info->node][6]);
        dfreg.Gfx[info->node + 1].df_gfx_line_pitch_reg.val = DF_Read(dfreg_gfx[info->node][7]);
        dfreg.Gfx[info->node + 1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[info->node][8]);
        dfreg.Gfx[info->node + 1].df_gfx_y_position_reg.val = DF_Read(dfreg_gfx[info->node][9]);
        dfreg.Gfx[info->node + 1].df_gfx_scl_x_position_reg.val = DF_Read(dfreg_gfx[info->node][10]);
        dfreg.Gfx[info->node + 1].df_gfx_control_reg.val = DF_Read(dfreg_gfx[info->node][0]);

        dfreg.Gfx[info->node + 1].df_gfx_format_reg.bits.i16BitEndian = dfreg.Gfx[info->node ].df_gfx_format_reg.bits.i16BitEndian;
        dfreg.Gfx[info->node + 1].df_gfx_format_reg.bits.i128BitEndian = dfreg.Gfx[info->node ].df_gfx_format_reg.bits.i128BitEndian ;
        dfreg.Gfx[info->node + 1].df_gfx_format_reg.bits.iByteEndian = dfreg.Gfx[info->node ].df_gfx_format_reg.bits.iByteEndian;
        dfreg.Gfx[info->node + 1].df_gfx_format_reg.bits.iNibbleEndian = dfreg.Gfx[info->node ].df_gfx_format_reg.bits.iNibbleEndian;

        dfreg.Gfx[info->node + 1].df_gfx_buf_start_addr_reg.bits.iStartAddr = ((dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.bits.iStartAddr << 4) +
                                                                               info->var.xres/display_bandwidth_factor *
                                                                               info->var.bits_per_pixel / 8 ) >> 4;

 //       printk("node 0 baseaddr=0x%x[0x%x],\n xres=%d \n display_factor=%d \n node 1 baseaddr=0x%x\n",
 //              dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.bits.iStartAddr,((dfreg.Gfx[info->node].df_gfx_buf_start_addr_reg.bits.iStartAddr)<<4),
 //              info->var.xres, display_bandwidth_factor, (dfreg.Gfx[info->node + 1].df_gfx_buf_start_addr_reg.bits.iStartAddr));
        dfreg.Gfx[info->node + 1].df_gfx_line_pitch_reg.bits.iLinePitch = dfreg.Gfx[info->node ].df_gfx_line_pitch_reg.bits.iLinePitch;
        dfreg.Gfx[info->node + 1].df_gfx_line_pitch_reg.bits.iBlankPixel = dfreg.Gfx[info->node ].df_gfx_line_pitch_reg.bits.iBlankPixel;

        dfreg.Gfx[info->node + 1].df_gfx_x_position_reg.bits.iXStart = dfreg.Gfx[info->node].df_gfx_x_position_reg.bits.iXEnd +1;
        dfreg.Gfx[info->node + 1].df_gfx_x_position_reg.bits.iXEnd = (dfreg.Gfx[info->node + 1].df_gfx_x_position_reg.bits.iXStart
                                                                      + info->var.xres / display_bandwidth_factor)  - 1;
        dfreg.Gfx[info->node + 1].df_gfx_y_position_reg.bits.iYStart	= dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYStart;
        dfreg.Gfx[info->node + 1].df_gfx_y_position_reg.bits.iYEnd = dfreg.Gfx[info->node].df_gfx_y_position_reg.bits.iYEnd ;
        dfreg.Gfx[info->node + 1].df_gfx_format_reg.bits.iColorFormat = dfreg.Gfx[info->node].df_gfx_format_reg.bits.iColorFormat;


        DF_Write((dfreg_gfx[info->node + 1][6]), dfreg.Gfx[info->node+ 1].df_gfx_buf_start_addr_reg.val);
        DF_Write((dfreg_gfx[info->node + 1][7]), dfreg.Gfx[info->node+ 1].df_gfx_line_pitch_reg.val);
        DF_Write((dfreg_gfx[info->node + 1][8]), dfreg.Gfx[info->node+ 1].df_gfx_x_position_reg.val);
        DF_Write((dfreg_gfx[info->node + 1][9]), dfreg.Gfx[info->node+ 1].df_gfx_y_position_reg.val);
        DF_Write((dfreg_gfx[info->node + 1][1]), dfreg.Gfx[info->node+ 1].df_gfx_format_reg.val);
        DF_Write((dfreg_gfx[info->node + 1][0]), dfreg.Gfx[info->node+ 1].df_gfx_control_reg.val);


        if(init) {
            DFGfxGetAlpha(info->node, & defaultalpha, &argb1555a0, &argb1555a1);
            DFGfxSetAlpha(info->node +1, defaultalpha, argb1555a0, argb1555a1);

            dfreg.Gfx[info->node + 1].df_gfx_key_red_reg.val = dfreg.Gfx[info->node].df_gfx_key_red_reg.val;
            dfreg.Gfx[info->node + 1].df_gfx_key_blue_reg.val = dfreg.Gfx[info->node].df_gfx_key_blue_reg.val;
            dfreg.Gfx[info->node + 1].df_gfx_key_green_reg.val = dfreg.Gfx[info->node].df_gfx_key_green_reg.val;

            DF_Write((dfreg_gfx[info->node + 1][3]), dfreg.Gfx[info->node+ 1].df_gfx_key_red_reg.val);
            DF_Write((dfreg_gfx[info->node + 1][5]), dfreg.Gfx[info->node+ 1].df_gfx_key_green_reg.val);
            DF_Write((dfreg_gfx[info->node + 1][4]), dfreg.Gfx[info->node+ 1].df_gfx_key_blue_reg.val);
            dfreg.Gfx[info->node + 1].df_gfx_control_reg.val = dfreg.Gfx[info->node].df_gfx_control_reg.val;
            DF_Write((dfreg_gfx[info->node + 1][0]), dfreg.Gfx[info->node+ 1].df_gfx_control_reg.val);
        }
    }
#ifndef CONFIG_FORCE_TWO_OSD_LAYER
    else if (display_bandwidth_factor == 1 && info->node == 0) {
        if (cncfbinfo->fbs_lst[1].info->screen_base != cncfbinfo->fbs_lst[0].info->screen_base){ // there is gfx2 buffer
            //            DFGfxSetAlpha(2, 0xff, 0, 0);
            DFGfxEna(1, 0, __Is_TV);
            dfreg.Gfx[0].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[0][8]); // read osd 0 value
            dfreg.Gfx[1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[1][8]); // read osd 1 value
            if (dfreg.Gfx[0].df_gfx_x_position_reg.val == dfreg.Gfx[1].df_gfx_x_position_reg.val) {
                dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd =  (dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd +1 ) * 2 -1;
                dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXEnd = 0;   // reset osd 1 x position to 0;
            }

        } else { // there is no gfx2 buffer, we will disable osd layer 2
            //            DFGfxSetAlpha(2, 0xff, 0, 0);
            DFGfxEna(1, 0, __Is_TV);
            dfreg.Gfx[0].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[0][8]); // read osd 0 value
            dfreg.Gfx[1].df_gfx_x_position_reg.val = DF_Read(dfreg_gfx[1][8]); // read osd 1 value
            if (dfreg.Gfx[0].df_gfx_x_position_reg.val == dfreg.Gfx[1].df_gfx_x_position_reg.val) {
                dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd =  (dfreg.Gfx[0].df_gfx_x_position_reg.bits.iXEnd +1 ) * 2 -1;
                dfreg.Gfx[1].df_gfx_x_position_reg.bits.iXEnd = 0;   // reset osd 1 x position to 0;
            }

        }
        DF_Write((dfreg_gfx[0][8]), dfreg.Gfx[0].df_gfx_x_position_reg.val);
        DF_Write((dfreg_gfx[1][8]), dfreg.Gfx[1].df_gfx_x_position_reg.val);

    }
#endif
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
#ifndef CONFIG_FORCE_TWO_OSD_LAYER
	if (i_node == 0 && cncfbinfo->fbs_lst[i_node].info->var.bits_per_pixel == 32 )
        DFGfxEna(1, arg, __Is_TV);
#endif
    __df_update_end();

	return 0;
}

int cncfb_hw_gfx_alpha(struct fb_info *info, unsigned int arg,struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	arg &= 0xff;

    printk("in hw_gfx_alpha!\n");
	__df_update_start();

	DFGfxSetAlpha(i_node, arg, 0, 0xff);

#ifndef CONFIG_FORCE_TWO_OSD_LAYER
	if (i_node == 0 && cncfbinfo->fbs_lst[i_node].info->var.bits_per_pixel == 32 )
        DFGfxSetAlpha(1, arg, 0, 0xff);
#endif
	__df_update_end();

	return 0;
}

int cncfb_hw_colorkey_on(struct fb_info *info, unsigned int arg, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	__df_update_start();
	DFGfxColorKeyEna(i_node, arg);

#ifndef CONFIG_FORCE_TWO_OSD_LAYER
	if (i_node == 0 && cncfbinfo->fbs_lst[i_node].info->var.bits_per_pixel == 32 )
        DFGfxColorKeyEna(i_node + 1, arg);
#endif

	__df_update_end();

	return 0;
}

int cncfb_hw_colorkey_val(struct fb_info *info, gfx_colorkey *col_key, struct cnc_info * cncfbinfo)
{
	int i_node = info->node;
	if (i_node > 1) i_node -= 2;

	__df_update_start();
	DFGfxSetColorKey(i_node, col_key->r_min, col_key->r_max, col_key->g_min, col_key->g_max, col_key->b_min, col_key->b_max);

#ifndef CONFIG_FORCE_TWO_OSD_LAYER
	if (i_node == 0 && cncfbinfo->fbs_lst[i_node].info->var.bits_per_pixel == 32 )
        DFGfxSetColorKey(i_node + 1, col_key->r_min, col_key->r_max, col_key->g_min, col_key->g_max, col_key->b_min, col_key->b_max);
#endif

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
                         u_int transp, struct fb_info *info)
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
	return DF_Read(addr);
}
