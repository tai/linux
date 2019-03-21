/*
 *  arch/arm/mach-celestial/include/mach/mux.h
 *  Celestial Platform IO address definitions
 *   
 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __MACH_MUX_H
#define __MACH_MUX_H


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#define RC_CTL_L                (0x008)    /*RC Control Register*/
#define RC_CTL_H                (0x00a) //



enum pinmux_1{
	I2C0_EN = 0,
	I2C1_EN,
	VGA_OUT_EN,
	DISP1_OUT_EN,
};

enum pinmux_disp{
	TVE0_YPbPr = 1 ,
	OUTIF0_RGB,
	TVE1_CVBS_S_video,
	TVE1_YPbPr,
	OUTIF1_RGB,
};


typedef enum _GPIO_ID_{
	_gpio_0     =0,
	_gpio_1     =1,
	_gpio_2     =2,
	_gpio_3     =3,
	_gpio_4     =4,
	_gpio_5     =5,
	_gpio_6     =6,
	_gpio_7     =7,
	_gpio_8     =8,
	_gpio_9     =9,
	_gpio_10    =10,
	_gpio_11    =11,
	_gpio_12    =12,
	_gpio_13    =13,
	_gpio_14    =14,
	_gpio_15    =15,
	_gpio_16    =16,
	_gpio_17    =17,
	_gpio_18    =18,
	_gpio_19    =19,
	_gpio_20    =20,
	_gpio_21    =21,
	_gpio_22    =22,
	_gpio_23    =23,
	_gpio_24    =24,
	_gpio_25    =25,
	_gpio_26    =26,
	_gpio_27    =27,
	_gpio_28    =28,
	_gpio_29    =29,
	_gpio_30    =30,
	_gpio_31    =31,
	_gpio_32    =32,
	_gpio_33    =33,
	_gpio_34    =34,
	_gpio_35    =35,
	_gpio_36    =36,
	_gpio_37    =37,
	_gpio_38    =38,
	_gpio_39    =39,
	_gpio_40    =40,
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
    _gpio_41    =41,
	_gpio_42    =42,
    _gpio_43    =43,
	_gpio_44    =44,
    _gpio_45    =45,
	_gpio_46    =46,
    _gpio_47    =47,
	_gpio_48    =48,
    _gpio_49    =49,
	_gpio_50    =50,
    _gpio_51    =51,
	_gpio_52    =52,
    _gpio_53    =53,
	_gpio_54    =54,
    _gpio_55    =55,
	_gpio_56    =56,
    _gpio_57    =57,
	_gpio_58    =58,
    _gpio_59    =59,
	_gpio_60    =60,
    _gpio_61    =61,
	_gpio_62    =62,
    _gpio_63    =63,
#endif
}GPIO_ID;


//I2C

typedef enum _I2C_ID_
{
	_i2c_0      =0,
	_i2c_1      =1,
}I2C_ID;

int PinMuxEnableI2C(I2C_ID I2C);

//UART

typedef enum _UART_ID_
{
	_uart_0 	=0,
	_uart_1		=1,
	_uart_2		=2,
	_uart_audio	=3,
	_uart_video	=4,
}UART_ID;

//Analog Video

typedef enum _DAC1_MODE_
{
	_dac1_tve0_ypbpr =0,
	_dac1_tve1_ypbpr =1,
	_dac1_disp0_rgb  =2,
	_dac1_tve1_cvbs  =3,
	_dac1_disp1_rgb  =4, 
}DAC1_MODE;

//Xport

typedef enum _XPORT_MODE_
{
	_xport_serial_mode	    =0,
	_xport_paralell_mode    =1,
} XPORT_MODE;

enum pinmux_class{
	PINMUX_I2C = 0, // output disp1/2 hsync and vsync,VGA/I2C,GPIO[1:0] ,I2C0,GPIO[17:16]
    PINMUX_VGA,
	PINMUX_UART,    // UART1/Audio_uart/Video_uart/UART2/IRDA,GPIO[6],GPIO[3],UART0,GPIO[19:18]
	PINMUX_DISP,    // 3'b11x:OUTIF1 RGB,TVE1 YPbPr,TVE1 CVBS+S-video,OUTIF0 RGB,TVE0 YPbPr
	PINMUX_SPI,     // GPIO[26:22],SPI
	PINMUX_SMI,     // GPIO[31:27] ,SMI
	PINMUX_TS,      // xport 1
	PINMUX_DVIO,    // 1800H GPIO[40:32],digital video input / 1800L GPIO[63:40],digital video input
	PINMUX_Ether,   // ehternet disabled and boot config parameter input ,Ethernet
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
    PINMUX_SDIO,    // SDIO / GPIO [39:32]
#endif

};

/* function declaration ------------------------------------- */


int pinmux_scan_status_bit(int  mode);
int pinmux_set_status_bit(int  mode, int bit, int set);
int pinmux_get_status_bit(int  mode,int bit);
int pinmux_update_fpc_status(void);
int pinmux_update_rc_status(void);
int pinmux_set_xportmode(XPORT_MODE Mode);
int pinmux_enable_uart(UART_ID UART);
int pinmux_enable_i2c(I2C_ID I2C);
int pinmux_enable_irda(void);
int pinmux_enable_spi(void);
int pinmux_enable_smi(void);
int pinmux_enable_vib(void);
int pinmux_enable_ethernet(void);
int pinmux_enable_booting(void);
int pinmux_set_dac1mode(DAC1_MODE Mode);

#endif 
