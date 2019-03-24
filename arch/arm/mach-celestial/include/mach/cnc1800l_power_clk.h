#ifndef __CNC1800L_POWER_CLOCK_H__
#define __CNC1800L_POWER_CLOCK_H__

#ifdef __cplusplus
extern "C"{
#endif

#define POWER_BASE 'P'
#define USB_POWER_DOWN          _IOW (POWER_BASE, 0, int)
#define XPORT_CLK_DOWN          _IOW (POWER_BASE, 1, int)
#define VIB_CLK_DOWN            _IOW (POWER_BASE, 2, int)
#define AUD_CLK_DOWN            _IOW (POWER_BASE, 3, int)
#define CRYPTO_CLK_DOWN         _IOW (POWER_BASE, 4, int)
#define GRAPHICS_CLK_DOWN       _IOW (POWER_BASE, 5, int)
#define DF_CLK_DOWN             _IOW (POWER_BASE, 6, int)
#define VIDECOD_CLK_DOWN        _IOW (POWER_BASE, 7, int)
#define AUDDACIF_CLK_DOWN       _IOW (POWER_BASE, 8, int)
#define SD_CLK_DOWN             _IOW (POWER_BASE, 9, int)
#define TVE1_CLK_DOWN           _IOW (POWER_BASE, 10, int)
#define TVE0_CLK_DOWN           _IOW (POWER_BASE, 11, int)
#define AUD_POWER_DOWN          _IOW (POWER_BASE, 12, int)
#define VID_POWER_DOWN          _IOW (POWER_BASE, 13, int)
#define AUD_PLLCLK_DOWN         _IOW (POWER_BASE, 14, int)
#define VDAC0_POWER_DOWN	_IOW (POWER_BASE, 15, int)
#define VDAC1_POWER_DOWN	_IOW (POWER_BASE, 16, int)

typedef enum _CLOCK_REG_BANK_
{
	CLOCK_REG_BASE                    =0xb2100000                 , 

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
	CLOCK_AUD_POWER_ON				  =(CLOCK_RESET_BASE+(7<<2))  ,
	CLOCK_LOW_POWER_CTL               =(CLOCK_PLL_BASE+(0x10<<2)) ,

	CLOCK_REG_SIZE = 0x1000,
}CLOCK_REG_BANK;

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

typedef enum _CLOCK_ENA_
{
        _clock_enable =1,
        _clock_disable =0,
}CLOCK_ENA;

//Display
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

//double ClockDispFreq(DISP_CLOCK_MODE DispName);
void clock_disp_reset(DISP_ID ID,CLOCK_RESET ResetOrSet);
int clock_dispset_clockmode0(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0 :HD+VGA+SD  OutIF1:SD
int clock_dispset_clockmode1(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0:VGA+SD         OutIF1:HD + SD
int clock_dispset_clockmode2(DISP_ID ID,DISP_CLOCK_MODE DispName); //OutIF 0:HD               OutIF1:HD
int clock_dispset_clockmoderaw(DISP_ID ID,DISP_CLOCK_MODE DispName,DISP_CLOCK_SOURCE ClockSource);
void clock_dispset_dac0mode(POWER_MODE PowerMode);
void clock_dispset_dac1mode(POWER_MODE PowerMode); //DAC 0 Is the CVBS DAC DAC1 is the YPbPr and VGA DAC
void clock_disptve_pllmode(POWER_MODE PowerMode);
void clock_dispsys_pllmode(POWER_MODE PowerMode);

//----------------------------------------------------------------
//Note: The Display Can support five modes,These modes depends on three
//         different clock source select modes.
//Note: In 1800l, only mode0 are supported: OutIF0 :HD+VGA+SD  OutIF1:SD
//----------------------------------------------------------------
//Disp #0     Disp#1               Disp0 Source          Disp1Source
//----------------------------------------------------------------
//HD            SD                     TVE_PLL                 SYS_PLL     ------Mode 0
//HD            HD                     TVE_PLL                 TVE_PLL     ------Mode 2
//VGA          SD                     TVE_PLL                 SYS_PLL     ------Mode 0
//VGA          HD                     SYS_PLL                 TVE_PLL     ------Mode 1
//SD            HD                     SYS_PLL                 TVE_PLL     ------Mode 1
//----------------------------------------------------------------

typedef enum _TMDS_CLOCK_MODE_
{
	_1x_disp_clk =0,
	_2x_disp_clk =1,
	_4x_disp_clk =3,
}TMDS_CLOCK_MODE;
void clock_hdmiset_clockmode(DISP_CLOCK_MODE DispMode,TMDS_CLOCK_MODE ClockMode);

typedef enum _DDC_CLOCK_MODE_
{
	_ddc_clk_high 		=0,
	_ddc_clk_middle		=1,
	_ddc_clk_low		=2,
}DDC_CLOCK_MODE;

void clock_ddcset_clockmode(DDC_CLOCK_MODE ClockMode);
//!!!!!!!This function must be employed after set DF clock

//Audio
typedef enum _AUDIO_CLOCK_MODE_
{
	_AUDIO_192KHz           =0,
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
	_AUDIO_8KHz	        =12,
}AUDIO_CLOCK_MODE;

void clock_audio_setclock(AUDIO_CLOCK_MODE AudioClockMode);
//int ClockAudioPLLMode(POWER_MODE PowerMode);
//int ClockSPDIFSetClock(AUDIO_CLOCK_MODE ClockMode);

//USB
int ClockUSB0PhyPowerDown(POWER_MODE PowerMode);
int ClockUSB1PhyPowerDown(POWER_MODE PowerMode);
void clock_usb1phy_bypassenable(void);
void clock_usb1phy_bypassdisable(void);

//Xport
int ClockXportInputClockSel(int ClockSel); //0:SYS_CLK27 1:AV_CLK27

//Video
typedef enum _VIDEO_CLOCK_MODE_
{
	_VIDEO_189M			=0, //756/4
	_VIDEO_151_2M		        =1, //756/5
	_VIDEO_126M			=2, //756/6
	_VIDEO_108M			=3, //756/7
}VIDEO_CLOCK_MODE;
int ClockCodaSetClock(VIDEO_CLOCK_MODE ClockMode);


typedef enum _Cec_CLOCK_MODE_
{
        _Cec_94P5M                     =0, //378/4 (para should be set to N-1, 3)
        _Cec_2P1M                      =1, //378/180(para should be set to N-1, 179)
        _Cec_1P48M                     =2, //378/255(para should be set to N-1, 254)
}Cec_CLOCK_MODE;
void clock_cecset_clockmode(Cec_CLOCK_MODE ClockMode);

typedef enum _AV_CLOCK_MODE_
{
        _AV_189M                     =0, //756/4
        _AV_126M                     =1, //756/6
        _AV_50P4M                    =2, //756/15
}AV_CLOCK_MODE;
int ClockAVSetClock(AV_CLOCK_MODE ClockMode);

typedef enum _AHB_CLOCK_MODE_             //ClockGEN for AHB(AHB1 & CRYPTO/AHB2/APB Clock)
{
        _AHB_189M                     =0, //756/4
        _AHB_126M                     =1, //756/6
        _AHB_108M                     =2, //756/7
}AHB_CLOCK_MODE;
int ClockAHBSetClock(AHB_CLOCK_MODE ClockMode);

/*
typedef enum _AHB2_CLOCK_MODE_             
{
        _AHB2_189M                    =0, //756/4
        _AHB2_126M                    =1, //756/6
        _AHB2_108M                    =2, //756/7
        _AHB2_94P5M                   =3, //756/8
}AHB2_CLOCK_MODE;
int ClockAHB2SetClock(AHB2_CLOCK_MODE ClockMode);

typedef enum _APB_CLOCK_MODE_            
{
        _APB_94P5M                     =0, //756/8
        _APB_47P25M                    =1, //756/16
        _APB_24P38M                     =2, //756/31
}APB_CLOCK_MODE;
int ClockAPBSetClock(APB_CLOCK_MODE ClockMode);
*/

typedef enum _GFX_CLOCK_MODE_
{
        _GFX_189M                     =0, //756/4
        _GFX_126M                     =1, //756/6
        _GFX_50P4M                    =2, //756/15
}GFX_CLOCK_MODE;
int ClockGFXSetClock(GFX_CLOCK_MODE ClockMode);

typedef enum _DF_CLOCK_MODE_
{
        _DF_189M                     =0, //756/4
        _DF_126M                     =1, //756/6
        _DF_50P4M                    =2, //756/15
}DF_CLOCK_MODE;
int ClockDFSetClock(DF_CLOCK_MODE ClockMode);

typedef enum _SD_CLOCK_MODE_             
{
        _SD_108M                       =0, //756/7
        _SD_94P5M                      =1, //756/8
        _SD_47P25M                     =2, //756/16
        _SD_24P38M                     =3, //756/31
}SD_CLOCK_MODE;
int ClockSDSetClock(SD_CLOCK_MODE ClockMode);

//Reset
int ClockDramportAReset(CLOCK_RESET ResetOrSet);
void clock_hdmi_reset(CLOCK_RESET ResetOrSet);
void clock_coda_reset(CLOCK_RESET ResetOrSet);
int ClockUSBPhyReset(CLOCK_RESET ResetOrSet);
void clock_vib_reset(CLOCK_RESET ResetOrSet);
int ClockTVE0Reset(CLOCK_RESET ResetOrSet);
int ClockTVE1Reset(CLOCK_RESET ResetOrSet);
int ClockDisp0Reset(CLOCK_RESET ResetOrSet);
void clock_disp1_reset(CLOCK_RESET ResetOrSet);
void clock_df_reset(CLOCK_RESET ResetOrSet);
void  clock_gfx_reset(CLOCK_RESET ResetOrSet);
void clock_video_reset(CLOCK_RESET ResetOrSet);
int ClockAudioReset(CLOCK_RESET ResetOrSet);
void clock_usb_reset(CLOCK_RESET ResetOrSet);
int ClockEtherReset(CLOCK_RESET ResetOrSet);
int ClockXportReset(CLOCK_RESET ResetOrSet);
int ClockDDRReset(CLOCK_RESET ResetOrSet);
void clock_crypto_reset(CLOCK_RESET ResetOrSet);

//Clock Enable/disable
void clock_ddc_clockena(CLOCK_ENA ClockEna);
void clock_cec_clockena(CLOCK_ENA ClockEna);
int ClockDramportAClockEna(CLOCK_ENA ClockEna);
void clock_video_clockena(CLOCK_ENA ClockEna);
int ClockXportClockEna(CLOCK_ENA ClockEna);
void clock_vib_clockena(CLOCK_ENA ClockEna);
int ClockCryptoClockEna(CLOCK_ENA ClockEna);
int ClockGfxClockEna(CLOCK_ENA ClockEna);
int ClockDFClockEna(CLOCK_ENA ClockEna);
void clock_coda_clockena(CLOCK_ENA ClockEna);
int ClockAudpllClockEna(CLOCK_ENA ClockEna);
void clock_tms_clockena(CLOCK_ENA ClockEna);
int ClockTve0ClockEna(CLOCK_ENA ClockEna);
int ClockTve1ClockEna(CLOCK_ENA ClockEna);
int ClockDramcpuClockEna(CLOCK_ENA ClockEna);
int ClockDDRClockEna(CLOCK_ENA ClockEna);
int ClockAhb1ClockEna(CLOCK_ENA ClockEna);
int ClockAhbClockEna(CLOCK_ENA ClockEna);        //Ahb2/APB clock Ena
int ClockAudDACIFClockEna(CLOCK_ENA ClockEna);   //SigmaDAC clock Ena
int ClockSDClockEna(CLOCK_ENA ClockEna);      
void clock_audio_clockena(CLOCK_ENA ClockEna);
void clock_gfx_clockena(CLOCK_ENA ClockEna);
void clock_crypto_clockena(CLOCK_ENA ClockEna);
void clock_vib_clockena(CLOCK_ENA ClockEna);
void clock_video_clockena(CLOCK_ENA ClockEna);
void clock_coda_clockena(CLOCK_ENA ClockEna);
void clock_df_clockena(CLOCK_ENA ClockEna);

#ifdef __cplusplus
}
#endif

#endif
