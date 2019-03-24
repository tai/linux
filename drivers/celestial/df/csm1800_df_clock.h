#ifndef __CSM1800_DF_CLOCK_H__
#define __CSM1800_DF_CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_REG_SIZE 0x1000
typedef enum _CLOCK_REG_BANK_
{
  CLOCK_REG_BASE                    =0x42100000,

  CLOCK_ID_BASE                     =(CLOCK_REG_BASE+(0<<2)),
  CLOCK_PLL_BASE                    =(CLOCK_REG_BASE+(0x40<<2)),
  CLOCK_MUX_BASE                    =(CLOCK_REG_BASE+(0x50<<2)),
  CLOCK_RESET_BASE                  =(CLOCK_REG_BASE+(0x80<<2)),

  CLOCK_ID_LO                       =(CLOCK_ID_BASE+(0<<2)),
  CLOCK_ID_HI                       =(CLOCK_ID_BASE+(1<<2)),

  CLOCK_PLL_DDR_DIV                 =(CLOCK_PLL_BASE+(0<<2)),
  CLOCK_PLL_AVS_DIV                 =(CLOCK_PLL_BASE+(1<<2)),
  CLOCK_PLL_TVE_DIV                 =(CLOCK_PLL_BASE+(2<<2)),
  CLOCK_AUD_HPD                     =(CLOCK_PLL_BASE+(3<<2)),
  CLOCK_AUD_FREQ                    =(CLOCK_PLL_BASE+(6<<2)),
  CLOCK_AUD_JITTER                  =(CLOCK_PLL_BASE+(7<<2)),
  CLOCK_CLK_GEN_DIV                 =(CLOCK_PLL_BASE+(8<<2)),


  CLOCK_PLL_CLK_EN                  =(CLOCK_MUX_BASE+(0<<2)),
  CLOCK_TVE_SEL                     =(CLOCK_MUX_BASE+(1<<2)),
  CLOCK_TVE_DIV_N					=(CLOCK_MUX_BASE+(2<<2)),

  CLOCK_SOFT_RESET                  =(CLOCK_RESET_BASE+(0<<2)),
  CLOCK_CLK_RESET                   =(CLOCK_RESET_BASE+(1<<2)),
  
  CLOCK_PLL_SLEEP_N                 =(CLOCK_RESET_BASE+(3<<2)),
  CLOCK_DAC_POWER_DOWN              =(CLOCK_RESET_BASE+(4<<2)),
  CLOCK_XPORT_CLK_SEL               =(CLOCK_RESET_BASE+(5<<2)),
  CLOCK_USB_ULPI_BYPASS             =(CLOCK_RESET_BASE+(6<<2)),
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

typedef enum _DISP_CLOCK_MODE_
{
  _YUV_NTSC							=0,
  _YUV_PAL							,
  _YUV_480P							,
  _YUV_576P							,
  _YUV_720P_50Hz					,
  _YUV_720P_60Hz					,
  _YUV_1080I_50Hz					,
  _YUV_1080I_60Hz					,
  _YUV_1080I_50Hz_1250_Total		,
  _YUV_1080P_24Hz					,
  _YUV_1080P_25Hz					,
  _YUV_1080P_30Hz                   ,
  _YUV_1080P_50Hz					,
  _YUV_1080P_60Hz					,
  _RGB_DMT_640x350_85Hz				,
  _RGB_DMT_640x400_85Hz				,
  _RGB_DMT_720x400_85Hz				,
  _RGB_DMT_640x480_60Hz				,
  _RGB_DMT_640x480_72Hz				,
  _RGB_DMT_640x480_75Hz				,
  _RGB_DMT_640x480_85Hz				,
  _RGB_DMT_800x600_56Hz				,
  _RGB_DMT_800x600_60Hz				,
  _RGB_DMT_800x600_72Hz				,
  _RGB_DMT_800x600_75Hz				,
  _RGB_DMT_800x600_85Hz				,
  _RGB_DMT_848x480_60Hz				,
  _RGB_DMT_1024x768_43Hz			,
  _RGB_DMT_1024x768_60Hz			,
  _RGB_DMT_1024x768_70Hz			,
  _RGB_DMT_1024x768_75Hz			,
  _RGB_DMT_1024x768_85Hz			,
  _RGB_DMT_1152x864_75Hz			,
  _RGB_DMT_1280x768_60Hz_RED		,
  _RGB_DMT_1280x768_60Hz			,
  _RGB_DMT_1280x768_75Hz			,
  _RGB_DMT_1280x768_85Hz			,
  _RGB_DMT_1280x960_60Hz			,
  _RGB_DMT_1280x960_85Hz			,
  _RGB_DMT_1280x1024_60Hz			,
  _RGB_DMT_1280x1024_75Hz			,
  _RGB_DMT_1280x1024_85Hz			,
  _RGB_DMT_1360x768_60Hz			,
  _RGB_DMT_1400x1050_60Hz_RED		,
  _RGB_DMT_1400x1050_60Hz			,
  _RGB_DMT_1400x1050_75Hz			,
  _RGB_DMT_1400x1050_85Hz			,
  _RGB_DMT_1440x900_60Hz_RED		,
  _RGB_DMT_1440x900_60Hz			,
  _RGB_DMT_1440x900_75Hz			,
  _RGB_DMT_1440x900_85Hz			,
  _RGB_DMT_1600x1200_60Hz			,
  _RGB_DMT_1600x1200_65Hz			,
  _RGB_DMT_1600x1200_70Hz			,
  _RGB_DMT_1600x1200_75Hz			,
  _RGB_DMT_1600x1200_85Hz			,
  _RGB_DMT_1680x1050_60Hz_RED		,
  _RGB_DMT_1680x1050_60Hz			,
  _RGB_DMT_1680x1050_75Hz			,
  _RGB_DMT_1680x1050_85Hz			,
  _RGB_DMT_1792x1344_60Hz			,
  _RGB_DMT_1792x1344_75Hz			,
  _RGB_DMT_1856x1392_60Hz			,
  _RGB_DMT_1856x1392_75Hz			,
  _RGB_DMT_1920x1200_60Hz_RED		,
  _RGB_DMT_1920x1200_60Hz			,
  _RGB_DMT_1920x1200_75Hz			,
  _RGB_DMT_1920x1200_85Hz			,
  _RGB_DMT_1920x1440_60Hz			,
  _RGB_DMT_1920x1440_75Hz			,
  _RGB_CVT_640x480_50Hz				,
  _RGB_CVT_640x480_60Hz				,
  _RGB_CVT_640x480_75Hz				,
  _RGB_CVT_640x480_85Hz				,
  _RGB_CVT_800x600_50Hz				,
  _RGB_CVT_800x600_60Hz				,
  _RGB_CVT_800x600_75Hz				,
  _RGB_CVT_800x600_85Hz				,
  _RGB_CVT_1024x768_50Hz			,
  _RGB_CVT_1024x768_60Hz			,
  _RGB_CVT_1024x768_75Hz			,
  _RGB_CVT_1024x768_85Hz			,
  _RGB_CVT_1280x960_50Hz			,
  _RGB_CVT_1280x960_60Hz			,
  _RGB_CVT_1280x960_75Hz			,
  _RGB_CVT_1280x960_85Hz			,
  _RGB_CVT_1400x1050_50Hz			,
  _RGB_CVT_1400x1050_60Hz			,
  _RGB_CVT_1400x1050_75Hz			,
  _RGB_CVT_1400x1050_85Hz			,
  _RGB_CVT_1600x1200_50Hz			,
  _RGB_CVT_1600x1200_60Hz			,
  _RGB_CVT_1600x1200_75Hz			,
  _RGB_CVT_1600x1200_85Hz			,
  _RGB_CVT_1800x1350_50Hz			,
  _RGB_CVT_1800x1350_60Hz			,
  _RGB_CVT_1800x1350_75Hz			,
  _RGB_CVT_1800x1350_85Hz			,
  _RGB_CVT_2048x1536_50Hz			,
  _RGB_CVT_2048x1536_60Hz			,
  _RGB_CVT_2048x1536_75Hz			,
  _RGB_CVT_2048x1536_85Hz			,
  _RGB_CVT_2560x1920_50Hz			,
  _RGB_CVT_2560x1920_60Hz			,
  _RGB_CVT_2560x1920_75Hz			,
  _RGB_CVT_2560x1920_85Hz			,
  _RGB_CVT_1280x1024_50Hz			,
  _RGB_CVT_1280x1024_60Hz			,
  _RGB_CVT_1280x1024_75Hz			,
  _RGB_CVT_1280x1024_85Hz			,
  _RGB_CVT_640x360_50Hz				,
  _RGB_CVT_640x360_60Hz				,
  _RGB_CVT_640x360_75Hz				,
  _RGB_CVT_640x360_85Hz				,
  _RGB_CVT_848x480_50Hz				,
  _RGB_CVT_848x480_60Hz				,
  _RGB_CVT_848x480_75Hz				,
  _RGB_CVT_848x480_85Hz				,
  _RGB_CVT_1024x576_50Hz			,
  _RGB_CVT_1024x576_60Hz			,
  _RGB_CVT_1024x576_75Hz			,
  _RGB_CVT_1024x576_85Hz			,
  _RGB_CVT_1280x720_50Hz			,
  _RGB_CVT_1280x720_60Hz			,
  _RGB_CVT_1280x720_75Hz			,
  _RGB_CVT_1280x720_85Hz			,
  _RGB_CVT_1600x900_50Hz			,
  _RGB_CVT_1600x900_60Hz			,
  _RGB_CVT_1600x900_75Hz			,
  _RGB_CVT_1600x900_85Hz			,
  _RGB_CVT_1920x1080_50Hz			,
  _RGB_CVT_1920x1080_60Hz			,
  _RGB_CVT_1920x1080_75Hz			,
  _RGB_CVT_1920x1080_85Hz			,
  _RGB_CVT_2048x1152_50Hz			,
  _RGB_CVT_2048x1152_60Hz			,
  _RGB_CVT_2048x1152_75Hz			,
  _RGB_CVT_2048x1152_85Hz			,
  _RGB_CVT_2560x1440_50Hz			,
  _RGB_CVT_2560x1440_60Hz			,
  _RGB_CVT_2560x1440_75Hz			,
  _RGB_CVT_2560x1440_85Hz			,
  _RGB_CVT_640x400_50Hz				,
  _RGB_CVT_640x400_60Hz				,
  _RGB_CVT_640x400_75Hz				,
  _RGB_CVT_640x400_85Hz				,
  _RGB_CVT_768x480_50Hz				,
  _RGB_CVT_768x480_60Hz				,
  _RGB_CVT_768x480_75Hz				,
  _RGB_CVT_768x480_85Hz				,
  _RGB_CVT_1024x640_50Hz			,
  _RGB_CVT_1024x640_60Hz			,
  _RGB_CVT_1024x640_75Hz			,
  _RGB_CVT_1024x640_85Hz			,
  _RGB_CVT_1280x800_50Hz			,
  _RGB_CVT_1280x800_60Hz			,
  _RGB_CVT_1280x800_75Hz			,
  _RGB_CVT_1280x800_85Hz			,
  _RGB_CVT_1600x1000_50Hz			,
  _RGB_CVT_1600x1000_60Hz			,
  _RGB_CVT_1600x1000_75Hz			,
  _RGB_CVT_1600x1000_85Hz			,
  _RGB_CVT_1920x1200_50Hz			,
  _RGB_CVT_1920x1200_60Hz			,
  _RGB_CVT_1920x1200_75Hz			,
  _RGB_CVT_1920x1200_85Hz			,
  _RGB_CVT_2048x1280_50Hz			,
  _RGB_CVT_2048x1280_60Hz			,
  _RGB_CVT_2048x1280_75Hz			,
  _RGB_CVT_2048x1280_85Hz			,
  _RGB_CVT_2560x1600_50Hz			,
  _RGB_CVT_2560x1600_60Hz			,
  _RGB_CVT_2560x1600_75Hz			,
  _RGB_CVT_2560x1600_85Hz			,
  _RGB_CVT_640x480_60Hz_RED			,
  _RGB_CVT_800x600_60Hz_RED			,
  _RGB_CVT_1024x768_60Hz_RED		,
  _RGB_CVT_1280x960_60Hz_RED		,
  _RGB_CVT_1400x1050_60Hz_RED		,
  _RGB_CVT_1600x1200_60Hz_RED		,
  _RGB_CVT_1800x1350_60Hz_RED		,
  _RGB_CVT_2048x1536_60Hz_RED		,
  _RGB_CVT_2560x1920_60Hz_RED		,
  _RGB_CVT_1280x1024_60Hz_RED		,
  _RGB_CVT_640x360_60Hz_RED			,
  _RGB_CVT_848x480_60Hz_RED			,
  _RGB_CVT_1024x576_60Hz_RED		,
  _RGB_CVT_1280x720_60Hz_RED		,
  _RGB_CVT_1600x900_60Hz_RED		,
  _RGB_CVT_1920x1080_60Hz_RED		,
  _RGB_CVT_2048x1152_60Hz_RED		,
  _RGB_CVT_2560x1440_60Hz_RED		,
  _RGB_CVT_640x400_60Hz_RED			,
  _RGB_CVT_768x480_60Hz_RED			,
  _RGB_CVT_1024x640_60Hz_RED		,
  _RGB_CVT_1280x800_60Hz_RED		,
  _RGB_CVT_1600x1000_60Hz_RED		,
  _RGB_CVT_1920x1200_60Hz_RED		,
  _RGB_CVT_2048x1280_60Hz_RED		,
  _RGB_CVT_2560x1600_60Hz_RED		,
  _RGB_CVT_800x480_60Hz,
  _RGB_CVT_1366x768_60Hz,
  _DISP_FORMAT_NUM,
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


unsigned int ClockDispGetFreq(DISP_CLOCK_MODE DispName);
void ClockDispReset(DISP_ID ID,CLOCK_RESET ResetOrSet);
int ClockDispSetClockMode0(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0 :HD+VGA+SD  OutIF1:SD
int ClockDispSetClockMode1(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0:VGA+SD         OutIF1:HD + SD
int ClockDispSetClockMode2(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0:HD               OutIF1:HD
int ClockDispSetClockModeRaw(DISP_ID ID,DISP_CLOCK_MODE DispName,DISP_CLOCK_SOURCE ClockSource);
void ClockDispSetDAC0Mode(POWER_MODE PowerMode);
void ClockDispSetDAC1Mode(POWER_MODE PowerMode); //DAC 0 Is the CVBS DAC DAC1 is the YPbPr and VGA DAC
void ClockDispTVEPLLMode(POWER_MODE PowerMode);
void ClockDispSYSPLLMode(POWER_MODE PowerMode);
void ClockDispSetDAC0Mode(POWER_MODE PowerMode);
void Clock_hdmi_reset(CLOCK_RESET ResetOrSet);

//----------------------------------------------------------------
//Note: The Display Can support five modes,These modes depends on three
//         different clock source select modes.
//----------------------------------------------------------------
//Disp #0     Disp#1               Disp0 Source          Disp1Source
//----------------------------------------------------------------
//HD            SD                     TVE_PLL                 SYS_PLL     ------Mode 0
//HD            HD                     TVE_PLL                 TVE_PLL     ------Mode 2
//VGA          SD                     TVE_PLL                 SYS_PLL     ------Mode 0
//VGA          HD                     SYS_PLL                 TVE_PLL     ------Mode 1
//SD            HD                     SYS_PLL                 TVE_PLL     ------Mode 1
//----------------------------------------------------------------

int CSM_DF_Clock_Init(void);

#ifdef __cplusplus
}
#endif

#endif
