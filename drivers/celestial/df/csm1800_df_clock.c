/****************************************************************************
  * Copyright (C) 2008-2010 Celestial Semiconductor Inc.
  * All rights reserved
  *
  * [RELEASE HISTORY]                           
  * VERSION  DATE       AUTHOR                  DESCRIPTION
  * 0.1      10-03-10   	Jia Ma           			Original
  ****************************************************************************
*/

#include <linux/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "csm1800_df_clock.h"

volatile unsigned char  *df_clk_base_temp = NULL;
#define df_clk_write(a,v)	writel(v,df_clk_base_temp+((a)&0xfff))
#define df_clk_read(a)	readl(df_clk_base_temp+((a)&0xfff))

static DISP_PLL_CFG disp_pll_cfg[] = {
{_YUV_NTSC,27000000,7,4,15},
{_YUV_PAL,27000000,7,4,15},
{_YUV_480P,27000000,7,4,15},
{_YUV_576P,27000000,7,4,15},
{_YUV_720P_50Hz,74250000,0,2,21},
{_YUV_720P_60Hz,74250000,0,2,21},
{_YUV_1080I_50Hz,74250000,0,2,21},
{_YUV_1080I_60Hz,74250000,0,2,21},
{_YUV_1080I_50Hz_1250_Total,72000000,0,0,0},
{_YUV_1080P_24Hz,74250000,0,2,21},
{_YUV_1080P_25Hz,74250000,0,2,21},
{_YUV_1080P_30Hz,74250000,0,2,21},
{_YUV_1080P_50Hz,148500000,0,2,21},
{_YUV_1080P_60Hz,148500000,0,2,21},
{_RGB_DMT_640x350_85Hz,31500000,12,9,20},
{_RGB_DMT_640x400_85Hz,31500000,12,9,20},
{_RGB_DMT_720x400_85Hz,35500000,0,8,20},
{_RGB_DMT_640x480_60Hz,25175000,15,15,27},
{_RGB_DMT_640x480_72Hz,31500000,12,9,20},
{_RGB_DMT_640x480_75Hz,31500000,12,9,20},
{_RGB_DMT_640x480_85Hz,36000000,0,6,15},
{_RGB_DMT_800x600_56Hz,36000000,0,6,15},
{_RGB_DMT_800x600_60Hz,40000000,0,0,0},
{_RGB_DMT_800x600_72Hz,50000000,0,7,25},
{_RGB_DMT_800x600_75Hz,49500000,0,6,21},
{_RGB_DMT_800x600_85Hz,56250000,0,6,24},
{_RGB_DMT_848x480_60Hz,33750000,0,6,14},
{_RGB_DMT_1024x768_43Hz,44900000,0,6,19},
{_RGB_DMT_1024x768_60Hz,65000000,0,5,23},
{_RGB_DMT_1024x768_70Hz,75000000,0,0,0},
{_RGB_DMT_1024x768_75Hz,78750000,0,5,28},
{_RGB_DMT_1024x768_85Hz,94500000,4,3,20},
{_RGB_DMT_1152x864_75Hz,108000000,0,2,15},
{_RGB_DMT_1280x768_60Hz_RED,68250000,0,0,0},
{_RGB_DMT_1280x768_60Hz,79500000,0,0,0},
{_RGB_DMT_1280x768_75Hz,102250000,0,0,0},
{_RGB_DMT_1280x768_85Hz,157500000,0,0,0},
{_RGB_DMT_1280x960_60Hz,108000000,0,2,15},
{_RGB_DMT_1280x960_85Hz,148500000,0,2,21},
{_RGB_DMT_1280x1024_60Hz,108000000,0,2,15},
{_RGB_DMT_1280x1024_75Hz,135000000,0,2,19},
{_RGB_DMT_1280x1024_85Hz,157500000,0,0,0},
{_RGB_DMT_1360x768_60Hz,85500000,0,3,18},
{_RGB_DMT_1400x1050_60Hz_RED,101000000,0,2,14},
{_RGB_DMT_1400x1050_60Hz,121750000,0,2,17},
{_RGB_DMT_1400x1050_75Hz,156000000,0,0,0},
{_RGB_DMT_1400x1050_85Hz,179500000,0,0,0},
{_RGB_DMT_1440x900_60Hz_RED,88750000,0,0,0},
{_RGB_DMT_1440x900_60Hz,106500000,0,0,0},
{_RGB_DMT_1440x900_75Hz,136750000,0,0,0},
{_RGB_DMT_1440x900_85Hz,157000000,0,0,0},
{_RGB_DMT_1600x1200_60Hz,162000000,0,0,0},
{_RGB_DMT_1600x1200_65Hz,175500000,0,0,0},
{_RGB_DMT_1600x1200_70Hz,189000000,0,0,0},
{_RGB_DMT_1600x1200_75Hz,202500000,0,0,0},
{_RGB_DMT_1600x1200_85Hz,229500000,0,0,0},
{_RGB_DMT_1680x1050_60Hz_RED,119000000,0,0,0},
{_RGB_DMT_1680x1050_60Hz,146250000,0,0,0},
{_RGB_DMT_1680x1050_75Hz,187000000,0,0,0},
{_RGB_DMT_1680x1050_85Hz,214750000,0,0,0},
{_RGB_DMT_1792x1344_60Hz,204750000,0,0,0},
{_RGB_DMT_1792x1344_75Hz,261000000,0,0,0},
{_RGB_DMT_1856x1392_60Hz,218250000,0,0,0},
{_RGB_DMT_1856x1392_75Hz,288000000,0,0,0},
{_RGB_DMT_1920x1200_60Hz_RED,154000000,0,0,0},
{_RGB_DMT_1920x1200_60Hz,193250000,0,0,0},
{_RGB_DMT_1920x1200_75Hz,245250000,0,0,0},
{_RGB_DMT_1920x1200_85Hz,281250000,0,0,0},
{_RGB_DMT_1920x1440_60Hz,234000000,0,0,0},
{_RGB_DMT_1920x1440_75Hz,297000000,0,0,0},
{_RGB_CVT_640x480_50Hz,19750000,19,13,18},
{_RGB_CVT_640x480_60Hz,23875000,16,13,22},
{_RGB_CVT_640x480_75Hz,30625000,0,11,24},
{_RGB_CVT_640x480_85Hz,35625000,0,11,28},
{_RGB_CVT_800x600_50Hz,31125000,12,10,22},
{_RGB_CVT_800x600_60Hz,38125000,10,6,16},
{_RGB_CVT_800x600_75Hz,48875000,0,8,28},
{_RGB_CVT_800x600_85Hz,56500000,0,5,20},
{_RGB_CVT_1024x768_50Hz,51750000,0,6,22},
{_RGB_CVT_1024x768_60Hz,64125000,0,4,18},
{_RGB_CVT_1024x768_75Hz,81750000,0,0,0},
{_RGB_CVT_1024x768_85Hz,94250000,4,3,20},
{_RGB_CVT_1280x960_50Hz,83000000,0,0,0},
{_RGB_CVT_1280x960_60Hz,102000000,0,0,0},
{_RGB_CVT_1280x960_75Hz,129875000,0,0,0},
{_RGB_CVT_1280x960_85Hz,149375000,0,0,0},
{_RGB_CVT_1400x1050_50Hz,99750000,0,0,0},
{_RGB_CVT_1400x1050_60Hz,122500000,0,0,0},
{_RGB_CVT_1400x1050_75Hz,155875000,0,0,0},
{_RGB_CVT_1400x1050_85Hz,179125000,0,0,0},
{_RGB_CVT_1600x1200_50Hz,132375000,0,0,0},
{_RGB_CVT_1600x1200_60Hz,160875000,0,0,0},
{_RGB_CVT_1600x1200_75Hz,205875000,0,0,0},
{_RGB_CVT_1600x1200_85Hz,234625000,0,0,0},
{_RGB_CVT_1800x1350_50Hz,168250000,0,0,0},
{_RGB_CVT_1800x1350_60Hz,204500000,0,0,0},
{_RGB_CVT_1800x1350_75Hz,261250000,0,0,0},
{_RGB_CVT_1800x1350_85Hz,299625000,0,0,0},
{_RGB_CVT_2048x1536_50Hz,218625000,0,0,0},
{_RGB_CVT_2048x1536_60Hz,267000000,0,0,0},
{_RGB_CVT_2048x1536_75Hz,340500000,0,0,0},
{_RGB_CVT_2048x1536_85Hz,388125000,0,0,0},
{_RGB_CVT_2560x1920_50Hz,346000000,0,0,0},
{_RGB_CVT_2560x1920_60Hz,421375000,0,0,0},
{_RGB_CVT_2560x1920_75Hz,533750000,0,0,0},
{_RGB_CVT_2560x1920_85Hz,611125000,0,0,0},
{_RGB_CVT_1280x1024_50Hz,89375000,0,0,0},
{_RGB_CVT_1280x1024_60Hz,108875000,0,0,0},
{_RGB_CVT_1280x1024_75Hz,138500000,0,0,0},
{_RGB_CVT_1280x1024_85Hz,159375000,0,0,0},
{_RGB_CVT_640x360_50Hz,14750000,26,22,23},
{_RGB_CVT_640x360_60Hz,17875000,21,22,28},
{_RGB_CVT_640x360_75Hz,22500000,17,9,14},
{_RGB_CVT_640x360_85Hz,25750000,0,11,20},
{_RGB_CVT_848x480_50Hz,26000000,0,14,26},
{_RGB_CVT_848x480_60Hz,31500000,12,9,20},
{_RGB_CVT_848x480_75Hz,40875000,0,5,14},
{_RGB_CVT_848x480_85Hz,47250000,8,6,20},
{_RGB_CVT_1024x576_50Hz,37875000,10,10,27},
{_RGB_CVT_1024x576_60Hz,46875000,8,6,20},
{_RGB_CVT_1024x576_75Hz,60625000,0,4,17},
{_RGB_CVT_1024x576_85Hz,69125000,0,0,0},
{_RGB_CVT_1280x720_50Hz,60375000,0,4,17},
{_RGB_CVT_1280x720_60Hz,74375000,0,4,21},
{_RGB_CVT_1280x720_75Hz,95625000,0,0,0},
{_RGB_CVT_1280x720_85Hz,110000000,0,0,0},
{_RGB_CVT_1600x900_50Hz,97000000,0,0,0},
{_RGB_CVT_1600x900_60Hz,118875000,0,0,0},
{_RGB_CVT_1600x900_75Hz,152125000,0,0,0},
{_RGB_CVT_1600x900_85Hz,174750000,0,0,0},
{_RGB_CVT_1920x1080_50Hz,141375000,0,2,20},
{_RGB_CVT_1920x1080_60Hz,172750000,0,0,0},
{_RGB_CVT_1920x1080_75Hz,220500000,0,0,0},
{_RGB_CVT_1920x1080_85Hz,252875000,0,0,0},
{_RGB_CVT_2048x1152_50Hz,162125000,0,0,0},
{_RGB_CVT_2048x1152_60Hz,198000000,0,0,0},
{_RGB_CVT_2048x1152_75Hz,252500000,0,0,0},
{_RGB_CVT_2048x1152_85Hz,289500000,0,0,0},
{_RGB_CVT_2560x1440_50Hz,256000000,0,0,0},
{_RGB_CVT_2560x1440_60Hz,311750000,0,0,0},
{_RGB_CVT_2560x1440_75Hz,396750000,0,0,0},
{_RGB_CVT_2560x1440_85Hz,454250000,0,0,0},
{_RGB_CVT_640x400_50Hz,16375000,23,14,16},
{_RGB_CVT_640x400_60Hz,19875000,19,19,27},
{_RGB_CVT_640x400_75Hz,25000000,15,14,25},
{_RGB_CVT_640x400_85Hz,29125000,13,13,27},
{_RGB_CVT_768x480_50Hz,23625000,16,12,20},
{_RGB_CVT_768x480_60Hz,28625000,13,8,16},
{_RGB_CVT_768x480_75Hz,37250000,0,8,21},
{_RGB_CVT_768x480_85Hz,42500000,0,7,21},
{_RGB_CVT_1024x640_50Hz,42625000,0,6,18},
{_RGB_CVT_1024x640_60Hz,52750000,0,0,0},
{_RGB_CVT_1024x640_75Hz,67375000,0,3,14},
{_RGB_CVT_1024x640_85Hz,77625000,0,4,22},
{_RGB_CVT_1280x800_50Hz,68500000,0,0,0},
{_RGB_CVT_1280x800_60Hz,83375000,0,0,0},
{_RGB_CVT_1280x800_75Hz,107250000,0,0,0},
{_RGB_CVT_1280x800_85Hz,123375000,0,0,0},
{_RGB_CVT_1600x1000_50Hz,108625000,0,0,0},
{_RGB_CVT_1600x1000_60Hz,133125000,0,0,0},
{_RGB_CVT_1600x1000_75Hz,169125000,0,0,0},
{_RGB_CVT_1600x1000_85Hz,194125000,0,0,0},
{_RGB_CVT_1920x1200_50Hz,158000000,0,0,0},
{_RGB_CVT_1920x1200_60Hz,193125000,0,0,0},
{_RGB_CVT_1920x1200_75Hz,246500000,0,0,0},
{_RGB_CVT_1920x1200_85Hz,282625000,0,0,0},
{_RGB_CVT_2048x1280_50Hz,181125000,0,0,0},
{_RGB_CVT_2048x1280_60Hz,221250000,0,0,0},
{_RGB_CVT_2048x1280_75Hz,280500000,0,0,0},
{_RGB_CVT_2048x1280_85Hz,321625000,0,0,0},
{_RGB_CVT_2560x1600_50Hz,285750000,0,0,0},
{_RGB_CVT_2560x1600_60Hz,348000000,0,0,0},
{_RGB_CVT_2560x1600_75Hz,442750000,0,0,0},
{_RGB_CVT_2560x1600_85Hz,507000000,0,0,0},
{_RGB_CVT_640x480_60Hz_RED,22750000,0,16,26},
{_RGB_CVT_800x600_60Hz_RED,34375000,11,11,27},
{_RGB_CVT_1024x768_60Hz_RED,54500000,0,0,0},
{_RGB_CVT_1280x960_60Hz_RED,83375000,0,0,0},
{_RGB_CVT_1400x1050_60Hz_RED,99000000,0,3,21},
{_RGB_CVT_1600x1200_60Hz_RED,128000000,0,2,18},
{_RGB_CVT_1800x1350_60Hz_RED,160625000,0,0,0},
{_RGB_CVT_2048x1536_60Hz_RED,206250000,0,0,0},
{_RGB_CVT_2560x1920_60Hz_RED,318500000,0,0,0},
{_RGB_CVT_1280x1024_60Hz_RED,89000000,0,0,0},
{_RGB_CVT_640x360_60Hz_RED,17000000,22,23,28},
{_RGB_CVT_848x480_60Hz_RED,28875000,13,7,14},
{_RGB_CVT_1024x576_60Hz_RED,40875000,0,5,14},
{_RGB_CVT_1280x720_60Hz_RED,62500000,0,5,22},
{_RGB_CVT_1600x900_60Hz_RED,96000000,0,0,0},
{_RGB_CVT_1920x1080_60Hz_RED,136500000,0,0,0},
{_RGB_CVT_2048x1152_60Hz_RED,154625000,0,0,0},
{_RGB_CVT_2560x1440_60Hz_RED,238750000,0,0,0},
{_RGB_CVT_640x400_60Hz_RED,18875000,20,15,20},
{_RGB_CVT_768x480_60Hz_RED,26500000,0,15,28},
{_RGB_CVT_1024x640_60Hz_RED,45500000,0,8,26},
{_RGB_CVT_1280x800_60Hz_RED,69500000,0,0,0},
{_RGB_CVT_1600x1000_60Hz_RED,106625000,0,0,0},
{_RGB_CVT_1920x1200_60Hz_RED,151750000,0,0,0},
{_RGB_CVT_2048x1280_60Hz_RED,171875000,0,0,0},
{_RGB_CVT_2560x1600_60Hz_RED,265375000,0,0,0},
{_RGB_CVT_800x480_60Hz,295000000,13,11,23},
{_RGB_CVT_1366x768_60Hz,72000000,0,3,15}
};

int CSM_DF_Clock_Init(void)
{
	int rtl_val = 0;

	if (NULL == df_clk_base_temp){
		if(!(df_clk_base_temp = (unsigned char *)ioremap(CLOCK_REG_BASE, CLOCK_REG_SIZE))){
			rtl_val = -1;;
		}
	}

	return (rtl_val);
}

void ClockDispReset(DISP_ID ID,CLOCK_RESET ResetOrSet)
{
	int reg_val = 0;

	reg_val = df_clk_read(CLOCK_SOFT_RESET);

	if(ResetOrSet==_do_reset){
		reg_val = reg_val & ((_disp1) ? 0xffffdfff : 0xffffefff);
	}
	else{
		reg_val = reg_val | ((_disp1) ? 0x2000 : 0x1000);
	}

	df_clk_write(CLOCK_SOFT_RESET,reg_val);

	return;
}

int ClockDispSetClockMode0(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
	int rt = 0;

	if(ID == _disp0){
		rt = ClockDispSetClockModeRaw(ID,DispName,_tve_pll);
	}
	else{
		rt = ClockDispSetClockModeRaw(ID,DispName,_sys_pll);
	}

	return (rt);
}

int ClockDispSetClockMode1(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
	int rt = 0;
	
	if(ID == _disp0){
		rt = ClockDispSetClockModeRaw(ID,DispName,_sys_pll);
	}
	else{
		rt = ClockDispSetClockModeRaw(ID,DispName,_tve_pll);
	}

	return (rt);
}

int ClockDispSetClockMode2(DISP_ID ID,DISP_CLOCK_MODE DispName)
{
	int rt = 0;

	if(ID == _disp0){
		rt = ClockDispSetClockModeRaw(ID,DispName,_tve_pll);
	}
	else{
		rt = ClockDispSetClockModeRaw(ID,DispName,_tve_pll);
	}

	return (rt);
}

static void ClockDispClockGating(DISP_ID ID)
{
	int reg_val = 0;

	//0.1 Disable output	
	if (ID==_disp0){	
		reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
		df_clk_write(CLOCK_PLL_CLK_EN, reg_val&0xffffffaf);
	}
	else
	{
		reg_val = df_clk_read(CLOCK_PLL_CLK_EN);
		df_clk_write(CLOCK_PLL_CLK_EN, reg_val&0xffffffdf);
	}

	//0.2 Gate Clock 
	if (ID==_disp0){
		reg_val = df_clk_read(CLOCK_TVE_SEL);
		df_clk_write(CLOCK_TVE_SEL, reg_val&0xfffffffc);
	}
	else{
		reg_val = df_clk_read(CLOCK_TVE_SEL);
		df_clk_write(CLOCK_TVE_SEL, reg_val&0xfffcffff);
	}

	return;
}

void ClockDispTVEPLLMode(POWER_MODE PowerMode)
{
	unsigned int reg_val = 0;

	if(PowerMode == _clock_wake){
		reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
		df_clk_write(CLOCK_PLL_SLEEP_N, reg_val|0x1);

		udelay(50);

		reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
		df_clk_write(CLOCK_PLL_SLEEP_N, reg_val|0x2);
	}
	else{
		reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
		df_clk_write(CLOCK_PLL_SLEEP_N, reg_val&0xfffffffd);

		udelay(1);

		reg_val = df_clk_read(CLOCK_PLL_SLEEP_N);
		df_clk_write(CLOCK_PLL_SLEEP_N, reg_val&0xfffffffe);
	}

	return;
}

void ClockDispSYSPLLMode(POWER_MODE PowerMode)
{
	//Not Implemented.
	return;
}

int ClockDispSetClockModeRaw(DISP_ID ID,DISP_CLOCK_MODE DispName,DISP_CLOCK_SOURCE ClockSource)
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
	if (ClockSource==_sys_pll){
		IsValid = pdisp_pll_cfg->tve_sys_divider !=0;
	}
	else{
	    IsValid=((pdisp_pll_cfg->tve_fb!=0) && (pdisp_pll_cfg->tve_m!=0));
	}

	//1.Config Clock
	if (IsValid){
		//1.0 Gate Clock of Display source
		ClockDispClockGating(ID);

		//1.1 Disable Power Down PLL
		if (ClockSource==_sys_pll){
			ClockDispSYSPLLMode(_clock_sleep);
		}
		else{
			//gate off DIVM;
			reg_val = df_clk_read(CLOCK_PLL_TVE_DIV);
			df_clk_write(CLOCK_PLL_TVE_DIV, reg_val&0xffff7fff);

			udelay(200);

			ClockDispTVEPLLMode(_clock_sleep);
		}

		//1.3 Config Mux and PLL Parameters
		//1.3.1 Config the PLL Parameters
	    	if(ClockSource == _sys_pll){
			reg_val = df_clk_read(CLOCK_CLK_GEN_DIV);

			if(ID==_disp0){
				reg_val = ((reg_val&0xffffffe0) | pdisp_pll_cfg->tve_sys_divider);
			}
			else{
				reg_val = ((reg_val&0xffffe0ff) | (pdisp_pll_cfg->tve_sys_divider<<8));
			}

			df_clk_write(CLOCK_CLK_GEN_DIV, reg_val);
		}
		else{
			reg_val = df_clk_read(CLOCK_PLL_TVE_DIV);

			reg_val = (reg_val&0xffff0000)| pdisp_pll_cfg->tve_m;
			reg_val = reg_val|(pdisp_pll_cfg->tve_fb <<8);
			reg_val |= 0x8000; 

			df_clk_write(CLOCK_PLL_TVE_DIV, reg_val);
		}

		//1.3.2 Wait 2us for PLL generating a stable clock
		udelay(500);

		//1.3.3 PLL Clock Wake
		if (ClockSource==_sys_pll){
			ClockDispSYSPLLMode(_clock_wake);
		}
		else{
			ClockDispTVEPLLMode(_clock_wake);
		}

		//1.3.4 Config Clock Switcher Level #1
		reg_val = df_clk_read(CLOCK_TVE_SEL);
		if(ID==_disp0){
			if(ClockSource==_sys_pll){
				reg_val = (reg_val & 0xfffffffc)|0x1;
			}
			else{
				reg_val = (reg_val & 0xfffffffc)|0x2;
			}

		}
		else{
			if(ClockSource==_sys_pll){
				reg_val = (reg_val & 0xfffcffff)|0x10000;
			}
			else{
				reg_val = (reg_val & 0xfffcffff)|0x20000;
			}
		}
		df_clk_write(CLOCK_TVE_SEL,reg_val);

		//1.3.5 Config Divider
		reg_val = df_clk_read(CLOCK_TVE_DIV_N);
		switch(DispName){
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

		if(ID==_disp0)
		{
			reg_val &= 0xffff0000;
			reg_val |= (HdmiTmds<<12);
			reg_val |= (Disp<<8);
			reg_val |= (TveClk6<<4);
			reg_val |= TveClk0;
		}
		else
		{
			reg_val &= 0xf000ffff;
			reg_val |= (Disp<<24);
			reg_val |= (TveClk6<<20);
			reg_val |= (TveClk0<<16);
		
		}
		df_clk_write(CLOCK_TVE_DIV_N,reg_val);
		
		//1.3.6 wait 1 us for stable
		udelay(50);
		
		//1.3.7 Enable
		reg_val = df_clk_read(CLOCK_PLL_CLK_EN);

		if(ID==_disp0){
			reg_val |= 0x50; 
		}
		else{
			reg_val |= 0x20; 
		}

		df_clk_write(CLOCK_PLL_CLK_EN, reg_val);
	}
	else{
		rt = -1;
	}

	return (rt);
}

void ClockDispSetDAC0Mode(POWER_MODE PowerMode)
{
	unsigned int reg_val = 0;

	reg_val = df_clk_read(CLOCK_DAC_POWER_DOWN);

	if(PowerMode==_clock_sleep)
	{
		reg_val |= 0x1;
	}
	else
	{
		reg_val &= 0xfffffffe;
	}

	df_clk_write(CLOCK_DAC_POWER_DOWN, reg_val);

	return;
}

void ClockDispSetDAC1Mode(POWER_MODE PowerMode)
{
	unsigned int reg_val = 0;

	reg_val = df_clk_read(CLOCK_DAC_POWER_DOWN);
	
	if(PowerMode==_clock_sleep)
	{
		reg_val |= 0x38; 
	}
	else
	{
		reg_val &= 0xffffffc7;
	}

	df_clk_write(CLOCK_DAC_POWER_DOWN, reg_val);

	return;
}

void Clock_hdmi_reset(CLOCK_RESET ResetOrSet)
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
