/****************************************************************************
  * Copyright (C) 2008-2010 Celestial Semiconductor Inc.
  * All rights reserved
  *
  * [RELEASE HISTORY]                           
  * VERSION  DATE       AUTHOR                  DESCRIPTION
  * 0.1      10-03-10   Jia Ma           			Original
  ****************************************************************************
*/

#include <linux/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <linux/fs.h>

#include <linux/clk.h>
#include <linux/ctype.h>

#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <mach/csm1800_power_clk.h>
#include <mach/hardware.h>
volatile unsigned char  *df_clk_base = NULL;


#define DFCLK_MAJOR  	0
#define DFCLK_NR_DEVS	16


//static int dfclk_major = DFCLK_MAJOR;


#define df_clk_write(a,v) writel(v,df_clk_base+((a)&0xfff))
#define df_clk_read(a)    readl(df_clk_base+((a)&0xfff))

static DISP_PLL_CFG disp_pll_cfg[] = {
	{_YUV_NTSC                   , 27000000  , 7  , 4  , 15} , 
	{_YUV_PAL                    , 27000000  , 7  , 4  , 15} , 
	{_YUV_480P                   , 27000000  , 7  , 4  , 15} , 
	{_YUV_576P                   , 27000000  , 7  , 4  , 15} , 
	{_YUV_720P_50Hz              , 74250000  , 0  , 2  , 21} , 
	{_YUV_720P_60Hz              , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080I_50Hz             , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080I_60Hz             , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080I_50Hz_1250_Total  , 72000000  , 0  , 0  , 0}  , 
	{_YUV_1080P_24Hz             , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080P_25Hz             , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080P_30Hz             , 74250000  , 0  , 2  , 21} , 
	{_YUV_1080P_50Hz             , 148500000 , 0  , 2  , 21} , 
	{_YUV_1080P_60Hz             , 148500000 , 0  , 2  , 21} , 
	{_RGB_DMT_640x350_85Hz       , 31500000  , 12 , 9  , 20} , 
	{_RGB_DMT_640x400_85Hz       , 31500000  , 12 , 9  , 20} , 
	{_RGB_DMT_720x400_85Hz       , 35500000  , 0  , 8  , 20} , 
	{_RGB_DMT_640x480_60Hz       , 25175000  , 15 , 15 , 27} , 
	{_RGB_DMT_640x480_72Hz       , 31500000  , 12 , 9  , 20} , 
	{_RGB_DMT_640x480_75Hz       , 31500000  , 12 , 9  , 20} , 
	{_RGB_DMT_640x480_85Hz       , 36000000  , 0  , 6  , 15} , 
	{_RGB_DMT_800x600_56Hz       , 36000000  , 0  , 6  , 15} , 
	{_RGB_DMT_800x600_60Hz       , 40000000  , 0  , 0  , 0}  , 
	{_RGB_DMT_800x600_72Hz       , 50000000  , 0  , 7  , 25} , 
	{_RGB_DMT_800x600_75Hz       , 49500000  , 0  , 6  , 21} , 
	{_RGB_DMT_800x600_85Hz       , 56250000  , 0  , 6  , 24} , 
	{_RGB_DMT_848x480_60Hz       , 33750000  , 0  , 6  , 14} , 
	{_RGB_DMT_1024x768_43Hz      , 44900000  , 0  , 6  , 19} , 
	{_RGB_DMT_1024x768_60Hz      , 65000000  , 0  , 5  , 23} , 
	{_RGB_DMT_1024x768_70Hz      , 75000000  , 0  , 0  , 0}  , 
	{_RGB_DMT_1024x768_75Hz      , 78750000  , 0  , 5  , 28} , 
	{_RGB_DMT_1024x768_85Hz      , 94500000  , 4  , 3  , 20} , 
	{_RGB_DMT_1152x864_75Hz      , 108000000 , 0  , 2  , 15} , 
	{_RGB_DMT_1280x768_60Hz_RED  , 68250000  , 0  , 0  , 0}  , 
	{_RGB_DMT_1280x768_60Hz      , 79500000  , 0  , 0  , 0}  , 
	{_RGB_DMT_1280x768_75Hz      , 102250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1280x768_85Hz      , 157500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1280x960_60Hz      , 108000000 , 0  , 2  , 15} , 
	{_RGB_DMT_1280x960_85Hz      , 148500000 , 0  , 2  , 21} , 
	{_RGB_DMT_1280x1024_60Hz     , 108000000 , 0  , 2  , 15} , 
	{_RGB_DMT_1280x1024_75Hz     , 135000000 , 0  , 2  , 19} , 
	{_RGB_DMT_1280x1024_85Hz     , 157500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1360x768_60Hz      , 85500000  , 0  , 3  , 18} , 
	{_RGB_DMT_1400x1050_60Hz_RED , 101000000 , 0  , 2  , 14} , 
	{_RGB_DMT_1400x1050_60Hz     , 121750000 , 0  , 2  , 17} , 
	{_RGB_DMT_1400x1050_75Hz     , 156000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1400x1050_85Hz     , 179500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1440x900_60Hz_RED  , 88750000  , 0  , 0  , 0}  , 
	{_RGB_DMT_1440x900_60Hz      , 106500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1440x900_75Hz      , 136750000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1440x900_85Hz      , 157000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1600x1200_60Hz     , 162000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1600x1200_65Hz     , 175500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1600x1200_70Hz     , 189000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1600x1200_75Hz     , 202500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1600x1200_85Hz     , 229500000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1680x1050_60Hz_RED , 119000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1680x1050_60Hz     , 146250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1680x1050_75Hz     , 187000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1680x1050_85Hz     , 214750000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1792x1344_60Hz     , 204750000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1792x1344_75Hz     , 261000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1856x1392_60Hz     , 218250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1856x1392_75Hz     , 288000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1200_60Hz_RED , 154000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1200_60Hz     , 193250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1200_75Hz     , 245250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1200_85Hz     , 281250000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1440_60Hz     , 234000000 , 0  , 0  , 0}  , 
	{_RGB_DMT_1920x1440_75Hz     , 297000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_640x480_50Hz       , 19750000  , 19 , 13 , 18} , 
	{_RGB_CVT_640x480_60Hz       , 23875000  , 16 , 13 , 22} , 
	{_RGB_CVT_640x480_75Hz       , 30625000  , 0  , 11 , 24} , 
	{_RGB_CVT_640x480_85Hz       , 35625000  , 0  , 11 , 28} , 
	{_RGB_CVT_800x600_50Hz       , 31125000  , 12 , 10 , 22} , 
	{_RGB_CVT_800x600_60Hz       , 38125000  , 10 , 6  , 16} , 
	{_RGB_CVT_800x600_75Hz       , 48875000  , 0  , 8  , 28} , 
	{_RGB_CVT_800x600_85Hz       , 56500000  , 0  , 5  , 20} , 
	{_RGB_CVT_1024x768_50Hz      , 51750000  , 0  , 6  , 22} , 
	{_RGB_CVT_1024x768_60Hz      , 64125000  , 0  , 4  , 18} , 
	{_RGB_CVT_1024x768_75Hz      , 81750000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1024x768_85Hz      , 94250000  , 4  , 3  , 20} , 
	{_RGB_CVT_1280x960_50Hz      , 83000000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x960_60Hz      , 102000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x960_75Hz      , 129875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x960_85Hz      , 149375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1400x1050_50Hz     , 99750000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1400x1050_60Hz     , 122500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1400x1050_75Hz     , 155875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1400x1050_85Hz     , 179125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1200_50Hz     , 132375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1200_60Hz     , 160875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1200_75Hz     , 205875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1200_85Hz     , 234625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1800x1350_50Hz     , 168250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1800x1350_60Hz     , 204500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1800x1350_75Hz     , 261250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1800x1350_85Hz     , 299625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1536_50Hz     , 218625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1536_60Hz     , 267000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1536_75Hz     , 340500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1536_85Hz     , 388125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1920_50Hz     , 346000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1920_60Hz     , 421375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1920_75Hz     , 533750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1920_85Hz     , 611125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x1024_50Hz     , 89375000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x1024_60Hz     , 108875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x1024_75Hz     , 138500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x1024_85Hz     , 159375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_640x360_50Hz       , 14750000  , 26 , 22 , 23} , 
	{_RGB_CVT_640x360_60Hz       , 17875000  , 21 , 22 , 28} , 
	{_RGB_CVT_640x360_75Hz       , 22500000  , 17 , 9  , 14} , 
	{_RGB_CVT_640x360_85Hz       , 25750000  , 0  , 11 , 20} , 
	{_RGB_CVT_848x480_50Hz       , 26000000  , 0  , 14 , 26} , 
	{_RGB_CVT_848x480_60Hz       , 31500000  , 12 , 9  , 20} , 
	{_RGB_CVT_848x480_75Hz       , 40875000  , 0  , 5  , 14} , 
	{_RGB_CVT_848x480_85Hz       , 47250000  , 8  , 6  , 20} , 
	{_RGB_CVT_1024x576_50Hz      , 37875000  , 10 , 10 , 27} , 
	{_RGB_CVT_1024x576_60Hz      , 46875000  , 8  , 6  , 20} , 
	{_RGB_CVT_1024x576_75Hz      , 60625000  , 0  , 4  , 17} , 
	{_RGB_CVT_1024x576_85Hz      , 69125000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x720_50Hz      , 60375000  , 0  , 4  , 17} , 
	{_RGB_CVT_1280x720_60Hz      , 74375000  , 0  , 4  , 21} , 
	{_RGB_CVT_1280x720_75Hz      , 95625000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x720_85Hz      , 110000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x900_50Hz      , 97000000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x900_60Hz      , 118875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x900_75Hz      , 152125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x900_85Hz      , 174750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1080_50Hz     , 141375000 , 0  , 2  , 20} , 
	{_RGB_CVT_1920x1080_60Hz     , 172750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1080_75Hz     , 220500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1080_85Hz     , 252875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1152_50Hz     , 162125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1152_60Hz     , 198000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1152_75Hz     , 252500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1152_85Hz     , 289500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1440_50Hz     , 256000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1440_60Hz     , 311750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1440_75Hz     , 396750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1440_85Hz     , 454250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_640x400_50Hz       , 16375000  , 23 , 14 , 16} , 
	{_RGB_CVT_640x400_60Hz       , 19875000  , 19 , 19 , 27} , 
	{_RGB_CVT_640x400_75Hz       , 25000000  , 15 , 14 , 25} , 
	{_RGB_CVT_640x400_85Hz       , 29125000  , 13 , 13 , 27} , 
	{_RGB_CVT_768x480_50Hz       , 23625000  , 16 , 12 , 20} , 
	{_RGB_CVT_768x480_60Hz       , 28625000  , 13 , 8  , 16} , 
	{_RGB_CVT_768x480_75Hz       , 37250000  , 0  , 8  , 21} , 
	{_RGB_CVT_768x480_85Hz       , 42500000  , 0  , 7  , 21} , 
	{_RGB_CVT_1024x640_50Hz      , 42625000  , 0  , 6  , 18} , 
	{_RGB_CVT_1024x640_60Hz      , 52750000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1024x640_75Hz      , 67375000  , 0  , 3  , 14} , 
	{_RGB_CVT_1024x640_85Hz      , 77625000  , 0  , 4  , 22} , 
	{_RGB_CVT_1280x800_50Hz      , 68500000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x800_60Hz      , 83375000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x800_75Hz      , 107250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x800_85Hz      , 123375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1000_50Hz     , 108625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1000_60Hz     , 133125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1000_75Hz     , 169125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1000_85Hz     , 194125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1200_50Hz     , 158000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1200_60Hz     , 193125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1200_75Hz     , 246500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1200_85Hz     , 282625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1280_50Hz     , 181125000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1280_60Hz     , 221250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1280_75Hz     , 280500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1280_85Hz     , 321625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1600_50Hz     , 285750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1600_60Hz     , 348000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1600_75Hz     , 442750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1600_85Hz     , 507000000 , 0  , 0  , 0}  , 
	{_RGB_CVT_640x480_60Hz_RED   , 22750000  , 0  , 16 , 26} , 
	{_RGB_CVT_800x600_60Hz_RED   , 34375000  , 11 , 11 , 27} , 
	{_RGB_CVT_1024x768_60Hz_RED  , 54500000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x960_60Hz_RED  , 83375000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1400x1050_60Hz_RED , 99000000  , 0  , 3  , 21} , 
	{_RGB_CVT_1600x1200_60Hz_RED , 128000000 , 0  , 2  , 18} , 
	{_RGB_CVT_1800x1350_60Hz_RED , 160625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1536_60Hz_RED , 206250000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1920_60Hz_RED , 318500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1280x1024_60Hz_RED , 89000000  , 0  , 0  , 0}  , 
	{_RGB_CVT_640x360_60Hz_RED   , 17000000  , 22 , 23 , 28} , 
	{_RGB_CVT_848x480_60Hz_RED   , 28875000  , 13 , 7  , 14} , 
	{_RGB_CVT_1024x576_60Hz_RED  , 40875000  , 0  , 5  , 14} , 
	{_RGB_CVT_1280x720_60Hz_RED  , 62500000  , 0  , 5  , 22} , 
	{_RGB_CVT_1600x900_60Hz_RED  , 96000000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1080_60Hz_RED , 136500000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1152_60Hz_RED , 154625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1440_60Hz_RED , 238750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_640x400_60Hz_RED   , 18875000  , 20 , 15 , 20} , 
	{_RGB_CVT_768x480_60Hz_RED   , 26500000  , 0  , 15 , 28} , 
	{_RGB_CVT_1024x640_60Hz_RED  , 45500000  , 0  , 8  , 26} , 
	{_RGB_CVT_1280x800_60Hz_RED  , 69500000  , 0  , 0  , 0}  , 
	{_RGB_CVT_1600x1000_60Hz_RED , 106625000 , 0  , 0  , 0}  , 
	{_RGB_CVT_1920x1200_60Hz_RED , 151750000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2048x1280_60Hz_RED , 171875000 , 0  , 0  , 0}  , 
	{_RGB_CVT_2560x1600_60Hz_RED , 265375000 , 0  , 0  , 0}  , 
	{_RGB_CVT_800x480_60Hz,295000000,13,11,23},
	{_RGB_CVT_1366x768_60Hz,72000000,0,3,15},
};

static int csm18xx_df_clock_init(void)
{
    int rtl_val = 0;

    if (NULL == df_clk_base) {
        if (!(df_clk_base = (unsigned char *)ioremap(CLOCK_REG_BASE, CLOCK_REG_SIZE))) {
            rtl_val = -1;;
        } else
            printk("CSM18XXDFCLK init at %0x\n",CLOCK_REG_BASE);
    }

    return (rtl_val);
}


void clock_disp_reset(DISP_ID ID,CLOCK_RESET ResetOrSet)
{
    int reg_val = 0;

    reg_val = df_clk_read(CLOCK_SOFT_RESET);

    if (ResetOrSet==_do_reset) {
        reg_val = reg_val & ((_disp1) ? 0xffffdfff : 0xffffefff);
    } else {
        reg_val = reg_val | ((_disp1) ? 0x2000 : 0x1000);
    }

    df_clk_write(CLOCK_SOFT_RESET,reg_val);

    return;
}

int clock_dispset_clockmode0(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
    int rt = 0;

    if (ID == _disp0) {
        rt = clock_dispset_clockmoderaw(ID,DispName,_tve_pll);
    } else {
        rt = clock_dispset_clockmoderaw(ID,DispName,_sys_pll);
    }

    return (rt);
}

int clock_dispset_clockmode1(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
    int rt = 0;

    if (ID == _disp0) {
        rt = clock_dispset_clockmoderaw(ID,DispName,_sys_pll);
    } else {
        rt = clock_dispset_clockmoderaw(ID,DispName,_tve_pll);
    }

    return (rt);
}

int clock_dispset_clockmode2(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
    int rt = 0;

    if (ID == _disp0) {
        rt = clock_dispset_clockmoderaw(ID,DispName,_tve_pll);
    } else {
        rt = clock_dispset_clockmoderaw(ID,DispName,_tve_pll);
    }

    return (rt);
}

static void clock_dispclock_gating(DISP_ID ID)
{
    int reg_val = 0;

    //0.1 Disable output
    if (ID==_disp0) {
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val&0xffffffaf);
    } else {
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val&0xffffffdf);
    }

    //0.2 Gate Clock
    if (ID==_disp0) {
        reg_val = df_clk_read(CLOCK_TVE_SEL);
        df_clk_write(CLOCK_TVE_SEL, reg_val&0xfffffffc);
    } else {
        reg_val = df_clk_read(CLOCK_TVE_SEL);
        df_clk_write(CLOCK_TVE_SEL, reg_val&0xfffcffff);
    }

    return;
}

void clock_disptve_pllmode(POWER_MODE PowerMode)
{
    unsigned int reg_val = 0;

    if (PowerMode == _clock_wake) {
        reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
        df_clk_write(CLOCK_PLL_SLEEP_N, reg_val|0x1);

        udelay(100);

        reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
        df_clk_write(CLOCK_PLL_SLEEP_N, reg_val|0x2);
    } else {
        reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
        df_clk_write(CLOCK_PLL_SLEEP_N, reg_val&0xfffffffd);

        udelay(10);

        reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
        df_clk_write(CLOCK_PLL_SLEEP_N, reg_val&0xfffffffe);
    }

    return;
}

void clock_dispsys_pllmode(POWER_MODE PowerMode)
{
    //Not Implemented.
    return;
}

int clock_dispset_clockmoderaw(DISP_ID ID,DISP_CLOCK_MODE DispName,DISP_CLOCK_SOURCE ClockSource)
{
    int rt=0;
    int IsValid=0;
    unsigned int reg_val = 0;
    unsigned int Disp;
    unsigned int TveClk0;
    unsigned int TveClk6;
    unsigned int HdmiTmds;
    DISP_PLL_CFG *pdisp_pll_cfg=NULL;

    pdisp_pll_cfg=&disp_pll_cfg[DispName];

    //0.Check Valid or not
    if (ClockSource==_sys_pll) {
        IsValid = pdisp_pll_cfg->tve_sys_divider !=0;
    } else {
        IsValid=((pdisp_pll_cfg->tve_fb!=0) && (pdisp_pll_cfg->tve_m!=0));
    }

    //1.Config Clock
    if (IsValid) {
        //1.0 Gate Clock of Display source
        clock_dispclock_gating(ID);

        //1.1 Disable Power Down PLL
        if (ClockSource==_sys_pll) {
            clock_dispsys_pllmode(_clock_sleep);
        } else {
            //gate off DIVM;
            reg_val = df_clk_read(CLOCK_PLL_TVE_DIV);
            df_clk_write(CLOCK_PLL_TVE_DIV, reg_val&0xffff7fff);

            udelay(200);

            clock_disptve_pllmode(_clock_sleep);
        }

        //1.3 Config Mux and PLL Parameters
        //1.3.1 Config the PLL Parameters
        if (ClockSource == _sys_pll) {
            reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);

            if (ID==_disp0) {
                reg_val = ((reg_val&0xffffffe0) | pdisp_pll_cfg->tve_sys_divider);
            } else {
                reg_val = ((reg_val&0xffffe0ff) | (pdisp_pll_cfg->tve_sys_divider<<8));
            }

            df_clk_write(CLOCK_CLK_GEN_DIV, reg_val);
        } else {
            reg_val = df_clk_read(CLOCK_PLL_TVE_DIV);

            reg_val = (reg_val&0xffff0000)| pdisp_pll_cfg->tve_m;
            reg_val = reg_val|(pdisp_pll_cfg->tve_fb <<8);
            reg_val |= 0x8000;

            df_clk_write(CLOCK_PLL_TVE_DIV, reg_val);
        }

        //1.3.2 Wait 2us for PLL generating a stable clock
        udelay(500);

        //1.3.3 PLL Clock Wake
        if (ClockSource==_sys_pll) {
            clock_dispsys_pllmode(_clock_wake);
        } else {
            clock_disptve_pllmode(_clock_wake);
        }

        //1.3.4 Config Clock Switcher Level #1
        reg_val = df_clk_read(CLOCK_TVE_SEL);
        if (ID==_disp0) {
            if (ClockSource==_sys_pll) {
                reg_val = (reg_val & 0xfffffffc)|0x1;
            } else {
                reg_val = (reg_val & 0xfffffffc)|0x2;
            }

        } else {
            if (ClockSource==_sys_pll) {
                reg_val = (reg_val & 0xfffcffff)|0x10000;
            } else {
                reg_val = (reg_val & 0xfffcffff)|0x20000;
            }
        }
        df_clk_write(CLOCK_TVE_SEL,reg_val);

        //1.3.5 Config Divider
        reg_val = df_clk_read(CLOCK_TVE_DIV_N);
        switch (DispName) {
        case _YUV_NTSC:
        case _YUV_PAL:
            Disp  	=4;
            TveClk0	=2;
            TveClk6 =4;
            HdmiTmds=4;
            break;
        case _YUV_480P:
        case _YUV_576P:
            Disp  	=4;
            TveClk0	=2;
            TveClk6 =2;
            HdmiTmds=4;
            break;
        case _YUV_1080I_50Hz:
        case _YUV_1080I_60Hz:
        case _YUV_720P_50Hz:
        case _YUV_720P_60Hz:
        case _YUV_1080P_24Hz:
        case _YUV_1080P_25Hz:
        case _YUV_1080P_30Hz:
        case _YUV_1080I_50Hz_1250_Total:
            Disp  	=4;
            TveClk0	=2;
            TveClk6 =4;
            HdmiTmds=4;
            break;
        case _YUV_1080P_50Hz:
        case _YUV_1080P_60Hz:
            Disp  	=2;
            TveClk0	=2;
            TveClk6 =4;
            HdmiTmds=2;
            break;
        default:
            Disp  	=2;
            TveClk0	=2;
            TveClk6 =4;
            HdmiTmds=2;
            ;
        }

        if (ID==_disp0) {
            reg_val &= 0xffff0000;
            reg_val |= (HdmiTmds<<12);
            reg_val |= (Disp<<8);
            reg_val |= (TveClk6<<4);
            reg_val |= TveClk0;
        } else {
            reg_val &= 0xf000ffff;
            reg_val |= (Disp<<24);
            reg_val |= (TveClk6<<20);
            reg_val |= (TveClk0<<16);

        }
        df_clk_write(CLOCK_TVE_DIV_N,reg_val);

        //1.3.6 wait 1 us for stable
        udelay(50);//?

        //1.3.7 Enable
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);

        if (ID==_disp0) {
            reg_val |= 0x50;
        } else {
            reg_val |= 0x20;
        }

        df_clk_write(CLOCK_PLL_CLK_EN, reg_val);
    } else {
        rt = -1;
    }

    return (rt);
}

void clock_dispset_dac0mode(POWER_MODE PowerMode)
{
    unsigned int reg_val = 0;

    reg_val = df_clk_read(CLOCK_DAC_POWER_DOWN);

    if (PowerMode==_clock_sleep) {
        reg_val |= 0x1;
    } else {
        reg_val &= 0xfffffffe;
    }

    df_clk_write(CLOCK_DAC_POWER_DOWN, reg_val);

    return;
}

void clock_dispset_dac1mode(POWER_MODE PowerMode)
{
    unsigned int reg_val = 0;

    reg_val = df_clk_read(CLOCK_DAC_POWER_DOWN);

    if (PowerMode==_clock_sleep) {
        reg_val |= 0x38;
    } else {
        reg_val &= 0xffffffc7;
    }

    df_clk_write(CLOCK_DAC_POWER_DOWN, reg_val);

    return;
}

void clock_hdmiset_clockmode(DISP_CLOCK_MODE DispMode,TMDS_CLOCK_MODE ClockMode)
{
    int iHdmiTmds,iTMDSDivEna;
    unsigned int reg_val = 0;


    //0.1 Gating HDMI_TMDS_CLK
    reg_val = df_clk_read(CLOCK_TVE_DIV_N);
    df_clk_write(CLOCK_TVE_DIV_N, reg_val&(~(0xf<<12)));


    //0.2 Disable
    reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
    df_clk_write(CLOCK_PLL_CLK_EN, reg_val&(~(0x1<<6)));

    //0.2 Config DIV
    switch (DispMode) {
    case 	_YUV_NTSC:
    case	_YUV_PAL:
    case    _YUV_480P:
    case    _YUV_576P:
        switch (ClockMode) {
        case _1x_disp_clk:
            iHdmiTmds=4;
            reg_val = df_clk_read(CLOCK_TVE_DIV_N);
            df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0x4<<12));
            break;
        case _2x_disp_clk:
            iHdmiTmds =2;
            reg_val = df_clk_read(CLOCK_TVE_DIV_N);
            df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0x2<<12));
            break;
        case _4x_disp_clk:
            iHdmiTmds =1;
            reg_val = df_clk_read(CLOCK_TVE_DIV_N);
            df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0x1<<12));
            break;
        default:
            ;
        }
        break;
    case	_YUV_720P_50Hz:
    case	_YUV_720P_60Hz:
    case	_YUV_1080I_50Hz:
    case	_YUV_1080I_60Hz:
    case    _YUV_1080P_24Hz:
    case 	_YUV_1080P_25Hz:
    case 	_YUV_1080P_30Hz:
        iHdmiTmds =4;
        reg_val = df_clk_read(CLOCK_TVE_DIV_N);
        df_clk_write(CLOCK_TVE_DIV_N, reg_val&(~(0xf<<12)));
        df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0x4<<12));
        break;
    default:
        iHdmiTmds =2;
        reg_val = df_clk_read(CLOCK_TVE_DIV_N);
        df_clk_write(CLOCK_TVE_DIV_N, reg_val&(~(0xf<<12)));
        df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0x2<<12));

    }

    //0.3 Wait 1 us for stable
    udelay(1);
    //0.4 Enable
    iTMDSDivEna=1;//CLOCK_REG_FMT(clock->iTMDSDivEna,6,1);
    reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
    df_clk_write(CLOCK_PLL_CLK_EN, reg_val|(0x1<<6));


    return ;
}

void clock_ddcset_clockmode(DDC_CLOCK_MODE ClockMode)
{
    unsigned int reg_val=0;
    int iDDCDiv;
    //0 Gate Clock
    iDDCDiv =0;//16/5
    reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);
    df_clk_write(CLOCK_TVE_DIV_N, reg_val&(~(0x1f<<16)));


    switch (ClockMode) {
    case _ddc_clk_high:
        iDDCDiv =10;    //70K
        reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);
        df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0xA<<16));
        break;
    case _ddc_clk_middle:   //60K
        iDDCDiv =12;
        reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);
        df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0xC<<16));
        break;
    case _ddc_clk_low:     //50K
        iDDCDiv =14;
        reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);
        df_clk_write(CLOCK_TVE_DIV_N, reg_val|(0xE<<16));
        break;
    default:
        ;
    }

    return;
}

typedef struct _AUDIO_PLL_CFG_ {
    AUDIO_CLOCK_MODE ClockMode;
    unsigned int Freq;
    unsigned int HPD;
    unsigned int Jitter;
} AUDIO_PLL_CFG;
AUDIO_PLL_CFG audio_pll;

static AUDIO_PLL_CFG audio_pll_cfg []= {
    {_AUDIO_192KHz		,	0x67f	,	1	,	0x5f370+0x90+0xa48	},
    {_AUDIO_96KHz		,	1920	,	3	,	0x656d0	},
    {_AUDIO_88_2KHz		,	1764	,	4	,	0x146d0	},
    {_AUDIO_64KHz		,	1280	,	5	,	0x3d6d0	},
    {_AUDIO_44_1KHz		,	882		,	8	,	0x146d0	},
    {_AUDIO_48KHz		,	960		,	7	,	0x296d0	},
    {_AUDIO_32KHz		,	640		,	11	,	0x156d0	},
    {_AUDIO_24KHz		,	480		,	15	,	0x0b6d0	},
    {_AUDIO_22_05KHz 	,	441		,	16	,	0x146d0	},
    {_AUDIO_16KHz		,	320		,	23	,	0x16d0	},
    {_AUDIO_12KHz		,	240		,	30	,	0xb6d0	},
    {_AUDIO_11_025KHz	,	220		,	33	,	0x6a50	},
    {_AUDIO_8KHz 		,	160		,	46	,	0x16d0	},

};

void clock_audio_setclock(AUDIO_CLOCK_MODE AudioClockMode)
{
    int SleepOrNot =0;
    AUDIO_PLL_CFG *paudio_pll_cfg =NULL;
    paudio_pll_cfg =&audio_pll_cfg[AudioClockMode];
    //0.0 Get PLL Parameters

    audio_pll.Freq = df_clk_read(CLOCK_AUD_FREQ);
    audio_pll.HPD = df_clk_read(CLOCK_AUD_HPD);
    audio_pll.Jitter = df_clk_read(CLOCK_AUD_JITTER);
    if ((audio_pll.Freq==paudio_pll_cfg->Freq)&&
            (audio_pll.HPD==paudio_pll_cfg->HPD)&&
            (audio_pll.Jitter==paudio_pll_cfg->Jitter)) {
        SleepOrNot =0;
    } else {
        SleepOrNot =1;
    }

    //0.1 Config PLL if needed
    if (SleepOrNot) {
        //1.0 Disable PLL output
        //clock_audio_ena(_clock_disable);	// lixun: for audio noise

        //1.1 Config PLL
        audio_pll.Freq =paudio_pll_cfg->Freq;
        audio_pll.HPD=paudio_pll_cfg->HPD;
        audio_pll.Jitter=paudio_pll_cfg->Jitter;

        //1.2 Update The para to PLL
        df_clk_write(CLOCK_AUD_FREQ, audio_pll.Freq);
        df_clk_write(CLOCK_AUD_HPD, audio_pll.HPD);
        df_clk_write(CLOCK_AUD_JITTER, audio_pll.Jitter);

        //1.3 Sleep 200 us for Clock stable
        udelay(50);

        //1.4 Enable PLL output
        clock_audio_ena(_clock_enable);

    }
    return;

}
void clock_audio_ena(CLOCK_ENA ClockEna)
{
    int iAudioClkEna;
    unsigned int reg_val=0;

    if (ClockEna == _clock_enable) {
        iAudioClkEna = 1;//8/1
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val|(0x1<<8));
    } else {
        iAudioClkEna = 0;
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val&(~(0x1<<8)));
    }

    return;
}

//Xport
void clock_xportInput_clocksel(int ClockSel) //0:SYS_CLK27 1:AV_CLK27
{
    int iXportClkSel;
    unsigned int reg_val;

    iXportClkSel = ClockSel & 0x1;

    reg_val = df_clk_read(CLOCK_XPORT_CLK_SEL);
    df_clk_write(CLOCK_XPORT_CLK_SEL, reg_val=iXportClkSel?1:0);
    return;
}

//Video
void clock_video_setClock(VIDEO_CLOCK_MODE ClockMode)
{
    unsigned int reg_val;
    int iDivVideo;

    //0 Gate Clock
    iDivVideo =0;//CLOCK_PLL_AVS_DIV 0/4
    reg_val = df_clk_read(CLOCK_PLL_AVS_DIV);
    df_clk_write(CLOCK_PLL_AVS_DIV, reg_val&(~0xf));

    switch (ClockMode) {
    case _VIDEO_189M:
        iDivVideo	=4;
        reg_val = df_clk_read(CLOCK_PLL_AVS_DIV);
        df_clk_write(CLOCK_PLL_AVS_DIV, reg_val|0x4);
        break;
    case _VIDEO_151_2M:
        iDivVideo	=5;
        reg_val = df_clk_read(CLOCK_PLL_AVS_DIV);
        df_clk_write(CLOCK_PLL_AVS_DIV, reg_val|0x5);
        break;
    case _VIDEO_126M:
        iDivVideo	=6;
        reg_val = df_clk_read(CLOCK_PLL_AVS_DIV);
        df_clk_write(CLOCK_PLL_AVS_DIV, reg_val|0x6);
        break;
    case _VIDEO_108M:
        iDivVideo	=7;
        reg_val = df_clk_read(CLOCK_PLL_AVS_DIV);
        df_clk_write(CLOCK_PLL_AVS_DIV, reg_val|0x7);
        break;
    default:
        printk("wrong mode set \n");
    }

    return;
}

//USB

void clock_usb0phy_powerdown(POWER_MODE PowerMode)
{
    int iUSBPhyAnlogBlockPD;
    unsigned int reg_val;

    if (PowerMode==_clock_sleep) {
        iUSBPhyAnlogBlockPD |=0x1;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|(0x1<<2));
    } else {
        iUSBPhyAnlogBlockPD &=0x2;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val&(~(0x1<<2)));
    }

    return;

}
void clock_usb1phy_powerdown(POWER_MODE PowerMode)
{
    int iUSBPhyAnlogBlockPD;
    unsigned int reg_val;

    if (PowerMode==_clock_sleep) {
        iUSBPhyAnlogBlockPD |=0x2;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|(0x2<<2));
    } else {
        iUSBPhyAnlogBlockPD &=0x1;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val&(~(0x2<<2)));
    }

    return;

}
void clock_usb1phy_bypassenable(void)
{
    int iUSBPhyBypass;
    unsigned int reg_val;

    iUSBPhyBypass =3;
    reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
    df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|0x3);

    return;

}
void clock_usb1phy_bypassdisable(void)
{
    int iUSBPhyBypass;
    unsigned int reg_val;
    iUSBPhyBypass =1;
    reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
    df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|0x1);
    return;

}

//Reset

void clock_ddr_reset(CLOCK_RESET ResetOrSet)
{
    unsigned int reg_val;
    int reset=0;

    if (ResetOrSet==_do_reset) {
        reset =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~0x1));
    } else {
        reset=1;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|0x1);
    }

    return;

}

void clock_audio_reset(CLOCK_RESET ResetOrSet)
{
    unsigned int reg_val;
    int iUSBPhyBypass;

    if (ResetOrSet==_do_reset) {
        iUSBPhyBypass =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<3)));
    } else {
        iUSBPhyBypass=1;
        reg_val = df_clk_read(CLOCK_USB_ULPI_BYPASS);
        df_clk_write(CLOCK_USB_ULPI_BYPASS, reg_val|(0x1<<3));
    }

    return;

}

void clock_hdmi_reset(CLOCK_RESET ResetOrSet)
{
    unsigned int reg_val;
    int iHDMIRst;

    if (ResetOrSet==_do_reset) {
        iHDMIRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<19)));
    } else {
        iHDMIRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<19));
    }

    return;

}

void clock_coda_reset(CLOCK_RESET ResetOrSet)
{
    unsigned int reg_val;
    int iCodaRst;

    if (ResetOrSet==_do_reset) {
        iCodaRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<18)));
    } else {
        iCodaRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<18));
    }

    return;
}
void clock_usbphy_reset(CLOCK_RESET ResetOrSet)
{
    unsigned int reg_val;
    int iUSBPhyRst;

    if (ResetOrSet==_do_reset) {
        iUSBPhyRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<17)));
    } else {
        iUSBPhyRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<17));
    }

    return;

}
void clock_vib_reset(CLOCK_RESET ResetOrSet)
{
    int iVIBRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iVIBRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<16)));
    } else {
        iVIBRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<16));
    }

    return;


}
void clock_tve0_reset(CLOCK_RESET ResetOrSet)
{
    int iTVE0Rst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iTVE0Rst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<14)));
    } else {
        iTVE0Rst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<14));
    }

    return;


}
void clock_tve1_reset(CLOCK_RESET ResetOrSet)
{
    int iTVE1Rst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iTVE1Rst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<15)));
    } else {
        iTVE1Rst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<15));
    }

    return;


}
void clock_disp0_reset(CLOCK_RESET ResetOrSet)
{
    int iDisp0Rst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iDisp0Rst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<12)));
    } else {
        iDisp0Rst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<12));
    }

    return;

}
void clock_disp1_reset(CLOCK_RESET ResetOrSet)
{
    int iDisp1Rst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iDisp1Rst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<13)));
    } else {
        iDisp1Rst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<13));
    }

    return;

}
void clock_df_reset(CLOCK_RESET ResetOrSet)
{
    int iDFRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iDFRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<11)));
    } else {
        iDFRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<11));
    }

    return;

}
void clock_avs_reset(CLOCK_RESET ResetOrSet)
{
    int iAVSRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iAVSRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<10)));
    } else {
        iAVSRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<10));
    }

    return;

}
void clock_h264_reset(CLOCK_RESET ResetOrSet)
{
    int iH264Rst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iH264Rst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<9)));
    } else {
        iH264Rst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<9));
    }

    return;

}
void clock_m2vd_reset(CLOCK_RESET ResetOrSet)
{
    int iM2VDRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iM2VDRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<8)));
    } else {
        iM2VDRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<8));
    }

    return;

}
void clock_dint_reset(CLOCK_RESET ResetOrSet)
{
    int iDeintRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iDeintRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<7)));
    } else {
        iDeintRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<7));
    }

    return;

}
void clock_video_reset(CLOCK_RESET ResetOrSet)
{
    int iVideoRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iVideoRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<6)));
    } else {
        iVideoRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<6));
    }

    return;

}
void clock_sata_reset(CLOCK_RESET ResetOrSet)
{
    int iSATARst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iSATARst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<5)));
    } else {
        iSATARst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<5));
    }

    return;

}
void clock_usb_reset(CLOCK_RESET ResetOrSet)
{
    int iUSBRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iUSBRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<4)));
    } else {
        iUSBRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<4));
    }

    return;

}
void clock_ether_reset(CLOCK_RESET ResetOrSet)
{
    int iEtherRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iEtherRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<2)));
    } else {
        iEtherRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<2));
    }

    return;

}
void clock_xport_reset(CLOCK_RESET ResetOrSet)
{
    int iXportRst;
    unsigned int reg_val;
    if (ResetOrSet==_do_reset) {
        iXportRst =0;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val&(~(0x1<<1)));
    } else {
        iXportRst=1;
        reg_val = df_clk_read(CLOCK_SOFT_RESET);
        df_clk_write(CLOCK_SOFT_RESET, reg_val|(0x1<<1));
    }

    return;

}

void clock_video_clockena(CLOCK_ENA ClockEna)
{
    int iVideoClkEna;
    unsigned int reg_val;

    if (ClockEna == _clock_enable) {
        iVideoClkEna  =2;
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val&(~(0x3<<2)));
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val|(0x2<<2));
    } else {
        iVideoClkEna  =0;
        reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
        df_clk_write(CLOCK_PLL_CLK_EN, reg_val&(~(0x3<<2)));
    }
    return;

}

int clk_enable(struct clk *clk)
{
	return 0;
}

void clk_disable(struct clk *clk)
{
}

unsigned long clk_get_rate(struct clk *clk)
{
	return 0;
}

#define SATA_PHYCR 0x88
/* 0 for down; 1 for up */
static void sata_phy_powerctl(char power)
{
    if (power == 0 ) {
        writel(0x816,VA_SATA_BASE+ SATA_PHYCR);
    }else if (power == 1) {
        writel(0x810,VA_SATA_BASE+ SATA_PHYCR);       

    }

}


#if 1
EXPORT_SYMBOL( clock_disp_reset);
EXPORT_SYMBOL( clock_dispset_clockmode0);
EXPORT_SYMBOL( clock_dispset_clockmode1);
EXPORT_SYMBOL( clock_dispset_clockmode2);
EXPORT_SYMBOL( clock_disptve_pllmode);
EXPORT_SYMBOL( clock_dispsys_pllmode);
EXPORT_SYMBOL( clock_dispset_clockmoderaw);
EXPORT_SYMBOL( clock_dispset_dac0mode);
EXPORT_SYMBOL( clock_dispset_dac1mode);
EXPORT_SYMBOL( clock_hdmiset_clockmode);
EXPORT_SYMBOL( clock_ddcset_clockmode);
EXPORT_SYMBOL( clock_audio_ena);
EXPORT_SYMBOL( clock_xportInput_clocksel);
EXPORT_SYMBOL( clock_video_setClock);
EXPORT_SYMBOL( clock_usb0phy_powerdown);
EXPORT_SYMBOL( clock_usb1phy_powerdown);
EXPORT_SYMBOL( clock_usb1phy_bypassenable);
EXPORT_SYMBOL( clock_usb1phy_bypassdisable);
EXPORT_SYMBOL( clock_ddr_reset);
EXPORT_SYMBOL( clock_audio_reset);
EXPORT_SYMBOL( clock_hdmi_reset);
EXPORT_SYMBOL( clock_coda_reset);
EXPORT_SYMBOL( clock_usbphy_reset);
EXPORT_SYMBOL( clock_vib_reset);
EXPORT_SYMBOL( clock_tve0_reset);
EXPORT_SYMBOL( clock_tve1_reset);
EXPORT_SYMBOL( clock_disp0_reset);
EXPORT_SYMBOL( clock_disp1_reset);
EXPORT_SYMBOL( clock_df_reset);
EXPORT_SYMBOL( clock_avs_reset);
EXPORT_SYMBOL( clock_h264_reset);
EXPORT_SYMBOL( clock_m2vd_reset);
EXPORT_SYMBOL( clock_dint_reset);
EXPORT_SYMBOL( clock_video_reset);
EXPORT_SYMBOL( clock_sata_reset);
EXPORT_SYMBOL( clock_usb_reset);
EXPORT_SYMBOL( clock_ether_reset);
EXPORT_SYMBOL( clock_xport_reset);
EXPORT_SYMBOL( clock_video_clockena);
EXPORT_SYMBOL( clock_audio_setclock);

EXPORT_SYMBOL( clk_enable);
EXPORT_SYMBOL( clk_disable);
EXPORT_SYMBOL( clk_get_rate);

#endif
#if 1

static struct proc_dir_entry *power_proc_entry = NULL;


static int power_proc_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{

    int cmd_length;
    int fun, para1, para2;
    char * endptr;
    int it = 0;
    
	const char *cmd_line = buffer;;

    cmd_length = strlen(cmd_line);
              

    while (!isalpha((int)cmd_line[it]) && it < cmd_length)
        it ++;
	if (strncmp("usb0", &cmd_line[it], 4) == 0) {
        it +=4;
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        if (strncmp("up", &cmd_line[it], 2) == 0){
            it += 2;
            clock_usb0phy_powerdown(_clock_wake);        
            printk("USB0 PHY power is up!\n");
        }else if (strncmp("down", &cmd_line[it], 4) == 0){
            it += 4;
            clock_usb0phy_powerdown(_clock_sleep);        
            printk("USB0 PHY power is down!\n");
        }
	   
	} else if (strncmp("usb1", &cmd_line[it], 4) == 0) {
        it += 4;
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        if (strncmp("up", &cmd_line[it], 2) == 0){
            it += 4;
            clock_usb1phy_powerdown(_clock_wake);        
            printk("USB1 PHY power is up!\n");
        }else if (strncmp("down", &cmd_line[it], 4) == 0){
            it += 4;
            clock_usb1phy_powerdown(_clock_sleep);        
            printk("USB1 PHY power is down!\n");
        }

	}else if (strncmp("sata", &cmd_line[it], 4) == 0) {
        it += 4;
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        if (strncmp("up", &cmd_line[it], 2) == 0){
            it += 2;
            sata_phy_powerctl(1);            
            printk("SATA PHY power is up!\n");
        }else if (strncmp("down", &cmd_line[it], 4) == 0){
            it += 4;
            sata_phy_powerctl(0);
            printk("SATA PHY power is down!\n");
        }
	}else if (strncmp("dac0", &cmd_line[it], 4) == 0) {
        it += 4;
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        if (strncmp("up", &cmd_line[it], 2) == 0){
            it += 2;
            clock_dispset_dac0mode(_clock_wake);
            printk("DAC0 power is up!\n");
        }else if (strncmp("down", &cmd_line[it], 4) == 0){
            it += 4;
            clock_dispset_dac0mode(_clock_sleep);
            printk("DAC0 PHY power is down!\n");
        }
    }else if (strncmp("dac1", &cmd_line[it], 4) == 0) {
        it += 4;
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        if (strncmp("up", &cmd_line[it], 2) == 0){
            it += 2;
            clock_dispset_dac1mode(_clock_wake);
            printk("DAC1 PHY power is up!\n");
        }else if (strncmp("down", &cmd_line[it], 4) == 0){
            it += 4;
            clock_dispset_dac1mode(_clock_sleep);
            printk("DAC1 PHY power is down!\n");
        }
    }


    else if (strncmp("clockctl", &cmd_line[it], 8) ==0 ){
        
        char * funs_name[40];
        int index;
        it += 8;
        funs_name[1] = "clock_disp_reset(DISP_ID ID,CLOCK_RESET ResetOrSet)";
        funs_name[2] = "clock_dispset_clockmode0(DISP_ID ID,DISP_CLOCK_MODE DispName)";
        funs_name[3] = "clock_dispset_clockmode1(DISP_ID ID,DISP_CLOCK_MODE DispName)";
        funs_name[4] = "clock_dispset_clockmode2(DISP_ID ID,DISP_CLOCK_MODE DispName)";
    
        funs_name[5] = "clock_dispset_clockmoderaw(DISP_ID,DISP_CLOCK_MODE,DISP_CLOCK_SOURCE)";

        funs_name[6] = "clock_disptve_pllmode(POWER_MODE PowerMode)";
        funs_name[7] = "clock_dispsys_pllmode(POWER_MODE PowerMode)";
        funs_name[8] = "clock_dispset_dac0mode(POWER_MODE PowerMode)";
        funs_name[9] = "clock_dispset_dac1mode(POWER_MODE PowerMode)";

        funs_name[10] = "clock_hdmiset_clockmode(DISP_CLOCK_MODE,TMDS_CLOCK_MODE)";
        funs_name[11] = "clock_ddcset_clockmode(DDC_CLOCK_MODE ClockMode)";


        funs_name[12] = "clock_xportInput_clocksel(int ClockSel)";
        funs_name[13] = "clock_video_setClock(VIDEO_CLOCK_MODE ClockMode)";
        funs_name[14] = "clock_video_clockena(CLOCK_ENA ClockEna)";

        funs_name[15] = "clock_audio_ena(CLOCK_ENA ClockEna)";
        funs_name[16] = "clock_audio_setclock(AUDIO_CLOCK_MODE AudioClockMode)";
    

        funs_name[17] = "clock_usb1phy_bypassenable(void)";
        funs_name[18] = "clock_usb1phy_bypassdisable(void)";

        funs_name[19] = "clock_ddr_reset(CLOCK_RESET ResetOrSet)";
        funs_name[20] = "clock_audio_reset(CLOCK_RESET ResetOrSet)";
        funs_name[21] = "clock_hdmi_reset(CLOCK_RESET ResetOrSet)";
        funs_name[22] = "clock_coda_reset(CLOCK_RESET ResetOrSet)";
        funs_name[23] = "clock_usbphy_reset(CLOCK_RESET ResetOrSet)";
        funs_name[24] = "clock_vib_reset(CLOCK_RESET ResetOrSet)";
        funs_name[25] = "clock_tve0_reset(CLOCK_RESET ResetOrSet)";
        funs_name[26] = "clock_tve1_reset(CLOCK_RESET ResetOrSet)";
        funs_name[27] = "clock_df_reset(CLOCK_RESET ResetOrSet)";
        funs_name[28] = "clock_avs_reset(CLOCK_RESET ResetOrSet)";
        funs_name[29] = "clock_h264_reset(CLOCK_RESET ResetOrSet)";
        funs_name[30] = "clock_m2vd_reset(CLOCK_RESET ResetOrSet)";
        funs_name[31] = "clock_dint_reset(CLOCK_RESET ResetOrSet)";
        funs_name[32] = "clock_video_reset(CLOCK_RESET ResetOrSet)";
        funs_name[33] = "clock_sata_reset(CLOCK_RESET ResetOrSet)";
        funs_name[34] = "clock_usb_reset(CLOCK_RESET ResetOrSet)";
        funs_name[35] = "clock_ether_reset(CLOCK_RESET ResetOrSet)";
        funs_name[36] = "clock_xport_reset(CLOCK_RESET ResetOrSet)";
        funs_name[37] = "clock_disp0_reset(CLOCK_RESET ResetOrSet)";
        funs_name[38] = "clock_disp1_reset(CLOCK_RESET ResetOrSet)";
        
        while (!isalpha((int)cmd_line[it]) && it < cmd_length)
            it ++;
        
        if (strncmp("showfuns", &cmd_line[it], 8) == 0){
            for (index = 1; index < 39; index++)
                printk("%d: %s\n", index,funs_name[index]);
            printk("\n");
            it += 8;
        } else if(strncmp("run", &cmd_line[it], 3) == 0) {
            it += 3;
            while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                it ++;

            fun = simple_strtol(&cmd_line[it], & endptr, 10);
            it = (int) (endptr) - (int) (&cmd_line[it]);
            printk("runing functions: %d(%s)\n",fun, funs_name[fun]);
            switch(fun) {
            case 1 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);

                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;

                para1= simple_strtol(&cmd_line[it], &endptr, 0);

                it = (int) (endptr) - (int) (&cmd_line[0]);

                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para2= simple_strtol(&cmd_line[it], NULL, 0);
                printk("%s, %d, %d \n", funs_name[fun], para1, para2 );
                clock_disp_reset(para1,para2);
                break;
            case 2 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], &endptr, 0);
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para2= simple_strtol(&cmd_line[it], NULL, 0);
                printk("%s, %d, %d \n", funs_name[fun], para1, para2 );
                clock_dispset_clockmode0(para1,para2);
                break;
            case 3 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 0);

                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para2= simple_strtol(&cmd_line[it], NULL, 0);
                printk("%s, %d, %d \n", funs_name[fun], para1, para2 );
                clock_dispset_clockmode1(para1,para2);
                break;
            case 4 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 0);


                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para2= simple_strtol(&cmd_line[it], NULL, 0);
                printk("%s, %d, %d \n", funs_name[fun], para1, para2 );
                clock_dispset_clockmode2(para1,para2);
                break;
                
            case 5 : 
                printk("Interal functions not used!\n");
                break;

            case 6 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_disptve_pllmode(para1);
                break;
            case 7 :
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 0);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_dispsys_pllmode(para1);
                break;
            case 8 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_dispset_dac0mode(para1);
                break;
            case 9 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_dispset_dac1mode(para1);
                break;

            case 10 :
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;

                para1= simple_strtol(&cmd_line[it], &endptr, 0);
                
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para2=simple_strtol(&cmd_line[it], NULL, 0);                
                printk("%s, %d, %d \n", funs_name[fun], para1, para2 );
                clock_hdmiset_clockmode(para1, para2);
                break;
            case 11 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_ddcset_clockmode(para1);
                break;

            case 12 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_xportInput_clocksel(para1);
                break;
                
            case 13 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_video_setClock(para1);
                break;
                
            case 14 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_video_clockena(para1);
                break;
            case 15 :     
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_audio_ena(para1);
                break;
            case 16 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_audio_setclock(para1);
                break;

            case 17 : 
                printk("%s \n", funs_name[fun]);
                clock_usb1phy_bypassenable();
                break;

            case 18 : 

                clock_usb1phy_bypassdisable();
                break;
                
            case 19 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_ddr_reset(para1);
                break;
            case 20 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_audio_reset(para1);
                break;
            case 21 :
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_hdmi_reset(para1);
                break;
            case 22 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_coda_reset(para1);
                break; 
            case 23 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[16], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_usbphy_reset(para1);
                break; 
            case 24 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_vib_reset(para1);
                break; 
            case 25 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_tve0_reset(para1);
                break; 
            case 26 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_tve1_reset(para1);
                break; 
            case 27 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_df_reset(para1);
                break; 
            case 28 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_avs_reset(para1);
                break; 
            case 29 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_h264_reset(para1);
                break; 
            case 30 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_m2vd_reset(para1);
                break; 
            case 31 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_dint_reset(para1);
                break; 
            case 32 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_video_reset(para1);
                break; 
            case 33 :
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_sata_reset(para1);
                break; 
            case 34 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_usb_reset(para1);
                break; 
            case 35 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_ether_reset(para1);
                break; 
            case 36 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_xport_reset(para1);
                break; 
            case 37 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_disp0_reset(para1);
                break; 
            case 38 : 
                it = (int) (endptr) - (int) (&cmd_line[0]);
                if (it >= cmd_length) break;
                while (!isxdigit((int)cmd_line[it]) && it < cmd_length)
                    it ++;
                para1= simple_strtol(&cmd_line[it], NULL, 10);
                printk("%s, %d \n", funs_name[fun], para1);
                clock_disp1_reset(para1);
                

            }
        }
    }
	return count;
}


static int __init
csm18xx_dfclock_init(void)
{
    int ret;
    ret = csm18xx_df_clock_init();

	power_proc_entry = create_proc_entry("powerctl", 0, NULL);
	if (NULL != power_proc_entry) {
		power_proc_entry->write_proc = power_proc_write;
	}
    return 0;
}


static void __exit
csm18xx_dfclock_exit(void)
{
    iounmap((void *)df_clk_base);
	if (NULL != power_proc_entry) remove_proc_entry("powerctl", NULL);
    release_mem_region(CLOCK_REG_BASE, CLOCK_REG_SIZE);
    return;
}

module_init(csm18xx_dfclock_init);
module_exit(csm18xx_dfclock_exit);

MODULE_AUTHOR("Celestial Semiconductor");
MODULE_DESCRIPTION("Celestial Semiconductor DFCLK driver");
MODULE_LICENSE("GPL");
#endif

