#ifndef __CNC1800H_POWER_CLOCK_H__
#define __CNC1800H_POWER_CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_REG_SIZE 0x1000
typedef enum _CLOCK_REG_BANK_
{
	CLOCK_REG_BASE                    =0x42100000                 , 

	CLOCK_ID_BASE                     =(CLOCK_REG_BASE+(0<<2))    , 
	CLOCK_PLL_BASE                    =(CLOCK_REG_BASE+(0x40<<2)) , 
	CLOCK_MUX_BASE                    =(CLOCK_REG_BASE+(0x50<<2)) , 
	CLOCK_RESET_BASE                  =(CLOCK_REG_BASE+(0x80<<2)) , 

	CLOCK_ID_LO                       =(CLOCK_ID_BASE+(0<<2))     , 
	CLOCK_ID_HI                       =(CLOCK_ID_BASE+(1<<2))     , 

	CLOCK_PLL_DDR_DIV                 =(CLOCK_PLL_BASE+(0<<2))    , 
	CLOCK_PLL_AVS_DIV                 =(CLOCK_PLL_BASE+(1<<2))    , 
	CLOCK_PLL_TVE_DIV                 =(CLOCK_PLL_BASE+(2<<2))    , 
	CLOCK_AUD_HPD                     =(CLOCK_PLL_BASE+(3<<2))    , 
	CLOCK_AUD_FREQ                    =(CLOCK_PLL_BASE+(6<<2))    , 
	CLOCK_AUD_JITTER                  =(CLOCK_PLL_BASE+(7<<2))    , 
	CLOCK_CLK_GEN_DIV                 =(CLOCK_PLL_BASE+(8<<2))    , 


	CLOCK_PLL_CLK_EN                  =(CLOCK_MUX_BASE+(0<<2))    , 
	CLOCK_TVE_SEL                     =(CLOCK_MUX_BASE+(1<<2))    , 
	CLOCK_TVE_DIV_N                   =(CLOCK_MUX_BASE+(2<<2))    , 

	CLOCK_SOFT_RESET                  =(CLOCK_RESET_BASE+(0<<2))  , 
	CLOCK_CLK_RESET                   =(CLOCK_RESET_BASE+(1<<2))  , 

	CLOCK_PLL_SLEEP_N                 =(CLOCK_RESET_BASE+(3<<2))  , 
	CLOCK_DAC_POWER_DOWN              =(CLOCK_RESET_BASE+(4<<2))  , 
	CLOCK_XPORT_CLK_SEL               =(CLOCK_RESET_BASE+(5<<2))  , 
	CLOCK_USB_ULPI_BYPASS             =(CLOCK_RESET_BASE+(6<<2))  , 
}CLOCK_REG_BANK;

typedef enum _DISP_ID_
{
	_disp0 =0,
	_disp1 =1,
}DISP_ID;

typedef enum _disp_clock_source_
{
	_sys_pll =0,
	_tve_pll =1,
}DISP_CLOCK_SOURCE;

typedef enum _TMDS_CLOCK_MODE_
{
	_1x_disp_clk =0,
	_2x_disp_clk =1,
	_4x_disp_clk =3,
}TMDS_CLOCK_MODE;

/*
   typedef enum _HDMI_AUDIO_IN_MODE_
   {
   _hdmi_i2s_in    =0,
   _hdmi_spdif_in  =1,
   }HDMI_AUDIO_IN_MODE;
   int ClockHDMISetAudioInMode(HDMI_AUDIO_IN_MODE Mode);
   */
typedef enum _DDC_CLOCK_MODE_
{
	_ddc_clk_high 		=0,
	_ddc_clk_middle		=1,
	_ddc_clk_low		=2,
}DDC_CLOCK_MODE;

typedef enum _CLOCK_ENA_
{
	_clock_enable =0,
	_clock_disable =1,
}CLOCK_ENA;

//Video
typedef enum _VIDEO_CLOCK_MODE_
{
	_VIDEO_189M			=0, //756/4
	_VIDEO_151_2M		=1, //756/5
	_VIDEO_126M			=2,	//756/6
	_VIDEO_108M			=3,	//756/7
}VIDEO_CLOCK_MODE;

typedef enum _DISP_CLOCK_MODE_
{
	_YUV_NTSC					=0, 
	_YUV_PAL                    , 
	_YUV_480P                   , 
	_YUV_576P                   , 
	_YUV_720P_50Hz              , 
	_YUV_720P_60Hz              , 
	_YUV_1080I_50Hz             , 
	_YUV_1080I_60Hz             , 
	_YUV_1080I_50Hz_1250_Total  , 
	_YUV_1080P_24Hz             , 
	_YUV_1080P_25Hz             , 
	_YUV_1080P_30Hz             , 
	_YUV_1080P_50Hz             , 
	_YUV_1080P_60Hz             , 
	_RGB_DMT_640x350_85Hz       , 
	_RGB_DMT_640x400_85Hz       , 
	_RGB_DMT_720x400_85Hz       , 
	_RGB_DMT_640x480_60Hz       , 
	_RGB_DMT_640x480_72Hz       , 
	_RGB_DMT_640x480_75Hz       , 
	_RGB_DMT_640x480_85Hz       , 
	_RGB_DMT_800x600_56Hz       , 
	_RGB_DMT_800x600_60Hz       , 
	_RGB_DMT_800x600_72Hz       , 
	_RGB_DMT_800x600_75Hz       , 
	_RGB_DMT_800x600_85Hz       , 
	_RGB_DMT_848x480_60Hz       , 
	_RGB_DMT_1024x768_43Hz      , 
	_RGB_DMT_1024x768_60Hz      , 
	_RGB_DMT_1024x768_70Hz      , 
	_RGB_DMT_1024x768_75Hz      , 
	_RGB_DMT_1024x768_85Hz      , 
	_RGB_DMT_1152x864_75Hz      , 
	_RGB_DMT_1280x768_60Hz_RED  , 
	_RGB_DMT_1280x768_60Hz      , 
	_RGB_DMT_1280x768_75Hz      , 
	_RGB_DMT_1280x768_85Hz      , 
	_RGB_DMT_1280x960_60Hz      , 
	_RGB_DMT_1280x960_85Hz      , 
	_RGB_DMT_1280x1024_60Hz     , 
	_RGB_DMT_1280x1024_75Hz     , 
	_RGB_DMT_1280x1024_85Hz     , 
	_RGB_DMT_1360x768_60Hz      , 
	_RGB_DMT_1400x1050_60Hz_RED , 
	_RGB_DMT_1400x1050_60Hz     , 
	_RGB_DMT_1400x1050_75Hz     , 
	_RGB_DMT_1400x1050_85Hz     , 
	_RGB_DMT_1440x900_60Hz_RED  , 
	_RGB_DMT_1440x900_60Hz      , 
	_RGB_DMT_1440x900_75Hz      , 
	_RGB_DMT_1440x900_85Hz      , 
	_RGB_DMT_1600x1200_60Hz     , 
	_RGB_DMT_1600x1200_65Hz     , 
	_RGB_DMT_1600x1200_70Hz     , 
	_RGB_DMT_1600x1200_75Hz     , 
	_RGB_DMT_1600x1200_85Hz     , 
	_RGB_DMT_1680x1050_60Hz_RED , 
	_RGB_DMT_1680x1050_60Hz     , 
	_RGB_DMT_1680x1050_75Hz     , 
	_RGB_DMT_1680x1050_85Hz     , 
	_RGB_DMT_1792x1344_60Hz     , 
	_RGB_DMT_1792x1344_75Hz     , 
	_RGB_DMT_1856x1392_60Hz     , 
	_RGB_DMT_1856x1392_75Hz     , 
	_RGB_DMT_1920x1200_60Hz_RED , 
	_RGB_DMT_1920x1200_60Hz     , 
	_RGB_DMT_1920x1200_75Hz     , 
	_RGB_DMT_1920x1200_85Hz     , 
	_RGB_DMT_1920x1440_60Hz     , 
	_RGB_DMT_1920x1440_75Hz     , 
	_RGB_CVT_640x480_50Hz       , 
	_RGB_CVT_640x480_60Hz       , 
	_RGB_CVT_640x480_75Hz       , 
	_RGB_CVT_640x480_85Hz       , 
	_RGB_CVT_800x600_50Hz       , 
	_RGB_CVT_800x600_60Hz       , 
	_RGB_CVT_800x600_75Hz       , 
	_RGB_CVT_800x600_85Hz       , 
	_RGB_CVT_1024x768_50Hz      , 
	_RGB_CVT_1024x768_60Hz      , 
	_RGB_CVT_1024x768_75Hz      , 
	_RGB_CVT_1024x768_85Hz      , 
	_RGB_CVT_1280x960_50Hz      , 
	_RGB_CVT_1280x960_60Hz      , 
	_RGB_CVT_1280x960_75Hz      , 
	_RGB_CVT_1280x960_85Hz      , 
	_RGB_CVT_1400x1050_50Hz     , 
	_RGB_CVT_1400x1050_60Hz     , 
	_RGB_CVT_1400x1050_75Hz     , 
	_RGB_CVT_1400x1050_85Hz     , 
	_RGB_CVT_1600x1200_50Hz     , 
	_RGB_CVT_1600x1200_60Hz     , 
	_RGB_CVT_1600x1200_75Hz     , 
	_RGB_CVT_1600x1200_85Hz     , 
	_RGB_CVT_1800x1350_50Hz     , 
	_RGB_CVT_1800x1350_60Hz     , 
	_RGB_CVT_1800x1350_75Hz     , 
	_RGB_CVT_1800x1350_85Hz     , 
	_RGB_CVT_2048x1536_50Hz     , 
	_RGB_CVT_2048x1536_60Hz     , 
	_RGB_CVT_2048x1536_75Hz     , 
	_RGB_CVT_2048x1536_85Hz     , 
	_RGB_CVT_2560x1920_50Hz     , 
	_RGB_CVT_2560x1920_60Hz     , 
	_RGB_CVT_2560x1920_75Hz     , 
	_RGB_CVT_2560x1920_85Hz     , 
	_RGB_CVT_1280x1024_50Hz     , 
	_RGB_CVT_1280x1024_60Hz     , 
	_RGB_CVT_1280x1024_75Hz     , 
	_RGB_CVT_1280x1024_85Hz     , 
	_RGB_CVT_640x360_50Hz       , 
	_RGB_CVT_640x360_60Hz       , 
	_RGB_CVT_640x360_75Hz       , 
	_RGB_CVT_640x360_85Hz       , 
	_RGB_CVT_848x480_50Hz       , 
	_RGB_CVT_848x480_60Hz       , 
	_RGB_CVT_848x480_75Hz       , 
	_RGB_CVT_848x480_85Hz       , 
	_RGB_CVT_1024x576_50Hz      , 
	_RGB_CVT_1024x576_60Hz      , 
	_RGB_CVT_1024x576_75Hz      , 
	_RGB_CVT_1024x576_85Hz      , 
	_RGB_CVT_1280x720_50Hz      , 
	_RGB_CVT_1280x720_60Hz      , 
	_RGB_CVT_1280x720_75Hz      , 
	_RGB_CVT_1280x720_85Hz      , 
	_RGB_CVT_1600x900_50Hz      , 
	_RGB_CVT_1600x900_60Hz      , 
	_RGB_CVT_1600x900_75Hz      , 
	_RGB_CVT_1600x900_85Hz      , 
	_RGB_CVT_1920x1080_50Hz     , 
	_RGB_CVT_1920x1080_60Hz     , 
	_RGB_CVT_1920x1080_75Hz     , 
	_RGB_CVT_1920x1080_85Hz     , 
	_RGB_CVT_2048x1152_50Hz     , 
	_RGB_CVT_2048x1152_60Hz     , 
	_RGB_CVT_2048x1152_75Hz     , 
	_RGB_CVT_2048x1152_85Hz     , 
	_RGB_CVT_2560x1440_50Hz     , 
	_RGB_CVT_2560x1440_60Hz     , 
	_RGB_CVT_2560x1440_75Hz     , 
	_RGB_CVT_2560x1440_85Hz     , 
	_RGB_CVT_640x400_50Hz       , 
	_RGB_CVT_640x400_60Hz       , 
	_RGB_CVT_640x400_75Hz       , 
	_RGB_CVT_640x400_85Hz       , 
	_RGB_CVT_768x480_50Hz       , 
	_RGB_CVT_768x480_60Hz       , 
	_RGB_CVT_768x480_75Hz       , 
	_RGB_CVT_768x480_85Hz       , 
	_RGB_CVT_1024x640_50Hz      , 
	_RGB_CVT_1024x640_60Hz      , 
	_RGB_CVT_1024x640_75Hz      , 
	_RGB_CVT_1024x640_85Hz      , 
	_RGB_CVT_1280x800_50Hz      , 
	_RGB_CVT_1280x800_60Hz      , 
	_RGB_CVT_1280x800_75Hz      , 
	_RGB_CVT_1280x800_85Hz      , 
	_RGB_CVT_1600x1000_50Hz     , 
	_RGB_CVT_1600x1000_60Hz     , 
	_RGB_CVT_1600x1000_75Hz     , 
	_RGB_CVT_1600x1000_85Hz     , 
	_RGB_CVT_1920x1200_50Hz     , 
	_RGB_CVT_1920x1200_60Hz     , 
	_RGB_CVT_1920x1200_75Hz     , 
	_RGB_CVT_1920x1200_85Hz     , 
	_RGB_CVT_2048x1280_50Hz     , 
	_RGB_CVT_2048x1280_60Hz     , 
	_RGB_CVT_2048x1280_75Hz     , 
	_RGB_CVT_2048x1280_85Hz     , 
	_RGB_CVT_2560x1600_50Hz     , 
	_RGB_CVT_2560x1600_60Hz     , 
	_RGB_CVT_2560x1600_75Hz     , 
	_RGB_CVT_2560x1600_85Hz     , 
	_RGB_CVT_640x480_60Hz_RED   , 
	_RGB_CVT_800x600_60Hz_RED   , 
	_RGB_CVT_1024x768_60Hz_RED  , 
	_RGB_CVT_1280x960_60Hz_RED  , 
	_RGB_CVT_1400x1050_60Hz_RED , 
	_RGB_CVT_1600x1200_60Hz_RED , 
	_RGB_CVT_1800x1350_60Hz_RED , 
	_RGB_CVT_2048x1536_60Hz_RED , 
	_RGB_CVT_2560x1920_60Hz_RED , 
	_RGB_CVT_1280x1024_60Hz_RED , 
	_RGB_CVT_640x360_60Hz_RED   , 
	_RGB_CVT_848x480_60Hz_RED   , 
	_RGB_CVT_1024x576_60Hz_RED  , 
	_RGB_CVT_1280x720_60Hz_RED  , 
	_RGB_CVT_1600x900_60Hz_RED  , 
	_RGB_CVT_1920x1080_60Hz_RED , 
	_RGB_CVT_2048x1152_60Hz_RED , 
	_RGB_CVT_2560x1440_60Hz_RED , 
	_RGB_CVT_640x400_60Hz_RED   , 
	_RGB_CVT_768x480_60Hz_RED   , 
	_RGB_CVT_1024x640_60Hz_RED  , 
	_RGB_CVT_1280x800_60Hz_RED  , 
	_RGB_CVT_1600x1000_60Hz_RED , 
	_RGB_CVT_1920x1200_60Hz_RED , 
	_RGB_CVT_2048x1280_60Hz_RED , 
	_RGB_CVT_2560x1600_60Hz_RED , 
	_RGB_CVT_800x480_60Hz,
	_RGB_CVT_1366x768_60Hz,
	_DISP_FORMAT_NUM            , 
}DISP_CLOCK_MODE;

typedef struct _DISP_PLL_CFG_
{
	DISP_CLOCK_MODE mode;
	double          frequency;
	unsigned int    tve_sys_divider;
	unsigned int    tve_m;
	unsigned int    tve_fb;
}DISP_PLL_CFG;

typedef enum _POWER_MODE_
{
	_clock_sleep =0,
	_clock_wake  =1,
}POWER_MODE;

typedef enum _CLOCK_RESET_
{
	_do_reset =0,
	_do_set   =1,
}CLOCK_RESET;

typedef enum _AUDIO_CLOCK_MODE_
{
	_AUDIO_192KHz       =0,
	_AUDIO_96KHz 		=1,
	_AUDIO_88_2KHz		=2,
	_AUDIO_64KHz		=3,
	_AUDIO_44_1KHz		=4,
	_AUDIO_48KHz		=5,
	_AUDIO_32KHz		=6,
	_AUDIO_24KHz		=7,
	_AUDIO_22_05KHz		=8,
	_AUDIO_16KHz		=9,
	_AUDIO_12KHz		=10,
	_AUDIO_11_025KHz	=11,
	_AUDIO_8KHz			=12,
}AUDIO_CLOCK_MODE;

static int  csm18xx_df_clock_init(void);
void        clock_disp_reset(DISP_ID                 ID,CLOCK_RESET ResetOrSet);
int         clock_dispset_clockmode0(DISP_ID         ID,DISP_CLOCK_MODE DispName);
int         clock_dispset_clockmode1(DISP_ID         ID,DISP_CLOCK_MODE DispName);
int         clock_dispset_clockmode2(DISP_ID         ID,DISP_CLOCK_MODE DispName);
static void clock_dispclock_gating(DISP_ID           ID);
void        clock_disptve_pllmode(POWER_MODE         PowerMode);
void        clock_dispsys_pllmode(POWER_MODE         PowerMode);
int         clock_dispset_clockmoderaw(DISP_ID       ID,DISP_CLOCK_MODE DispName,DISP_CLOCK_SOURCE ClockSource);
void        clock_dispset_dac0mode(POWER_MODE        PowerMode);
void        clock_dispset_dac1mode(POWER_MODE        PowerMode);
void        clock_hdmiset_clockmode(DISP_CLOCK_MODE  DispMode,TMDS_CLOCK_MODE ClockMode);
void        clock_ddcset_clockmode(DDC_CLOCK_MODE    ClockMode);
void        clock_audio_setclock(AUDIO_CLOCK_MODE    AudioClockMode);
void        clock_audio_ena(CLOCK_ENA                ClockEna);
void        clock_xportInput_clocksel(int            ClockSel);
void        clock_video_setClock(VIDEO_CLOCK_MODE    ClockMode);
void        clock_usb0phy_powerdown(POWER_MODE       PowerMode);
void        clock_usb1phy_powerdown(POWER_MODE       PowerMode);
void        clock_usb1phy_bypassenable(void);
void        clock_usb1phy_bypassdisable(void);
void        clock_ddr_reset(CLOCK_RESET              ResetOrSet);
void        clock_audio_reset(CLOCK_RESET            ResetOrSet);
void        clock_hdmi_reset(CLOCK_RESET             ResetOrSet);
void        clock_coda_reset(CLOCK_RESET             ResetOrSet);
void        clock_usbphy_reset(CLOCK_RESET           ResetOrSet);
void        clock_vib_reset(CLOCK_RESET              ResetOrSet);
void        clock_tve0_reset(CLOCK_RESET             ResetOrSet);
void        clock_tve1_reset(CLOCK_RESET             ResetOrSet);
void        clock_disp0_reset(CLOCK_RESET            ResetOrSet);
void        clock_disp1_reset(CLOCK_RESET            ResetOrSet);
void        clock_df_reset(CLOCK_RESET               ResetOrSet);
void        clock_avs_reset(CLOCK_RESET              ResetOrSet);
void        clock_h264_reset(CLOCK_RESET             ResetOrSet);
void        clock_m2vd_reset(CLOCK_RESET             ResetOrSet);
void        clock_dint_reset(CLOCK_RESET             ResetOrSet);
void        clock_video_reset(CLOCK_RESET            ResetOrSet);
void        clock_sata_reset(CLOCK_RESET             ResetOrSet);
void        clock_usb_reset(CLOCK_RESET              ResetOrSet);
void        clock_ether_reset(CLOCK_RESET            ResetOrSet);
void        clock_xport_reset(CLOCK_RESET            ResetOrSet);
void        clock_video_clockena(CLOCK_ENA           ClockEna);

//----------------------------------------------------------------
//Note: The Display Can support five modes,These modes depends on three
//         different clock source select modes.
//----------------------------------------------------------------
//Disp #0 Disp#1  Disp0   Source     Disp1Source
//----------------------------------------------------------------
//HD   SD TVE_PLL SYS_PLL ------Mode 0
//HD   HD TVE_PLL TVE_PLL ------Mode 2
//VGA  SD TVE_PLL SYS_PLL ------Mode 0
//VGA  HD SYS_PLL TVE_PLL ------Mode 1
//SD   HD SYS_PLL TVE_PLL ------Mode 1
//----------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif
