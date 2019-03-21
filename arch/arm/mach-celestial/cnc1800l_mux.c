/****************************************************************************
  * Copyright (C) 2008-2010 Celestial Semiconductor
  * Copyright (C) 2008-2011 Cavium
  * All rights reserved
  *
  * [RELEASE HISTORY]                           
  * VERSION  DATE       AUTHOR                  DESCRIPTION
  * 0.1      10-04-20   Hao.Ran           			Origina
  ****************************************************************************
*/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <mach/mux.h>

#define PINMUX_READ 0
#define PINMUX_WRITE 1

struct pinmux_cmd {
	int pinmux;
	int value;
};

#define  CNC18XX_PINMUX_BASE 0xB2110000
#define  CNC18XX_PINMUX_SIZE 0x100

#define REG_PINMUX_VER          0x000
#define REG_PINMUX_I2C          0x004
#define REG_PINMUX_UART			0x008
#define REG_PINMUX_SPI			0x00c
#define REG_PINMUX_SMI			0x010
#define REG_PINMUX_DISP			0x014
#define REG_PINMUX_TS  			0x018
#define REG_PINMUX_DVIO			0x01c
#define REG_PINMUX_ETHER		0x020
#define REG_PINMUX_SDIO         0x024

#define PINMUX_MAJOR  	0
#define PINMUX_NR_DEVS	16

 
static int pinmux_major = PINMUX_MAJOR;
static int pinmux_nr_devs = PINMUX_NR_DEVS;


module_param(pinmux_major, int, PINMUX_MAJOR);
module_param(pinmux_nr_devs, int, PINMUX_NR_DEVS);
MODULE_PARM_DESC(pinmux_major, "PINMUX major number");
MODULE_PARM_DESC(pinmux_nr_devs, "PINMUX number of lines");

typedef struct pinmux_devs {
	u32 pinmux_id;
	struct cdev cdev;
} pinmux_devs_t;

static struct class *cnc1800x_pinmux_class;
enum pinmux_class  mode;
// should use in higher kernel version:static struct class *cnc1800x_pinmux_class;
/*
 * The devices
 */
struct pinmux_devs *pinmux_devices = NULL;	/* allocated in pinmux_init_module */
static spinlock_t pinmux_lock;
static volatile unsigned short *pinmux_base;
static volatile unsigned short *rc_base;
static volatile unsigned short *fpc_base;
static volatile unsigned int * gpio_base;

#define REG_GPIO_SWPORTA_DDR            0x24
static struct proc_dir_entry *pinmux_proc_entry = NULL;
/* ----------------------------------------------------------- */

#define fpc_writew(v,a)    	do { *(fpc_base + (a)) = v; }while(0)
#define fpc_readw(a)       	*(fpc_base + (a))


static int __init cnc18xx_pinmux_init(void);
static void __exit cnc18xx_pinmux_exit(void);

static int cnc18xx_pinmux_open(struct inode *inode, struct file *file);
static int cnc18xx_pinmux_close (struct inode *inode, struct file *file);
static int pinmux_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int pinmux_proc_write(struct file *file, const char __user *buffer,unsigned long count, void *data);
extern void gpio_configure_status_bit(unsigned int bit, const char * configure_cmd,char is_pinmux);


/* pinmux device struct */
struct file_operations pinmux_fops = {
	.owner = THIS_MODULE,
	.open = cnc18xx_pinmux_open,
	.release  = cnc18xx_pinmux_close,
};



int cnc_gpio_register_module_set(const char * module, unsigned int  cnc_module_status) /* status: 0 disable, 1 enable */
{
    int gpionum;
	if(strncmp("RC", module, 2)==0){ 
					if (cnc_module_status == 1){
						gpio_configure_status_bit(11 ,"SET",1);
					}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(11 ,"CLR",1);
					}
 	}else if(strncmp("FPC-COMMON", module, 10)==0){
					if (cnc_module_status == 1){
						gpio_configure_status_bit(10, "SET",1);
						gpio_configure_status_bit(8,  "SET",1);
						gpio_configure_status_bit(9,  "SET",1);
						gpio_configure_status_bit(7,  "SET",1);
						gpio_configure_status_bit(2,  "SET",1);
					}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(10, "CLR",1);
						gpio_configure_status_bit(9,  "CLR",1);
						gpio_configure_status_bit(8,  "CLR",1);
						gpio_configure_status_bit(7,  "CLR",1);
						gpio_configure_status_bit(2,  "CLR",1);
					}

 	}else if(strncmp("FPC-LED-SPECIAL", module, 15)==0){
					if (cnc_module_status == 1){
						gpio_configure_status_bit(4, "SET",1);
						gpio_configure_status_bit(5,  "SET",1);
					}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(4,  "CLR",1);
						gpio_configure_status_bit(5,  "CLR",1);
					}

 	}else if(strncmp("PCMCIA", module, 6)==0){
					if (cnc_module_status == 1){
						gpio_configure_status_bit(14, "SET", 1);
						gpio_configure_status_bit(13, "SET", 1);
						gpio_configure_status_bit(12, "SET", 1);	
					}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(14, "CLR", 1);
						gpio_configure_status_bit(13, "CLR", 1);
						gpio_configure_status_bit(12, "CLR", 1);
					}
 	}else if (strncmp("I2C1", module, 4)==0){
	 				if (cnc_module_status == 1){
						gpio_configure_status_bit(0,"SET",1);
						gpio_configure_status_bit(1,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(0,"CLR",1);
						gpio_configure_status_bit(1,"CLR",1);
	 				}
 	}else if (strncmp("VGA", module, 4)==0){
	 				if (cnc_module_status == 1){
						gpio_configure_status_bit(0,"SET",1);
						gpio_configure_status_bit(1,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(0,"CLR",1);
						gpio_configure_status_bit(1,"CLR",1);
	 				}
 	}else if (strncmp("I2C0", module, 4)==0){
 					if (cnc_module_status == 1){
						gpio_configure_status_bit(16,"SET",1);
						gpio_configure_status_bit(17,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(16,"CLR",1);
						gpio_configure_status_bit(17,"CLR",1);
	 				}
 	}else if (strncmp("UART2", module, 5)==0){
 					if (cnc_module_status == 1){
						gpio_configure_status_bit(3,"SET",1);
						gpio_configure_status_bit(6,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(3,"CLR",1);
						gpio_configure_status_bit(6,"CLR",1);
	 				}
 	}else if (strncmp("UART0", module, 5)==0){
 					if (cnc_module_status == 1){
						gpio_configure_status_bit(18,"SET",1);
						gpio_configure_status_bit(19,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(18,"CLR",1);
						gpio_configure_status_bit(19,"CLR",1);
	 				}
 	}else if (strncmp("SPI", module, 3)==0){
 					if (cnc_module_status == 1){
						gpio_configure_status_bit(22,"SET",1);
						gpio_configure_status_bit(23,"SET",1);
						gpio_configure_status_bit(24,"SET",1);
						gpio_configure_status_bit(25,"SET",1);
						gpio_configure_status_bit(26,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(22,"CLR",1);
						gpio_configure_status_bit(23,"CLR",1);
						gpio_configure_status_bit(24,"CLR",1);
						gpio_configure_status_bit(25,"CLR",1);
						gpio_configure_status_bit(26,"CLR",1);
	 				}
 	}else if (strncmp("SMI", module, 3)==0){
 					if (cnc_module_status == 1){
						gpio_configure_status_bit(27,"SET",1);
						gpio_configure_status_bit(28,"SET",1);
						gpio_configure_status_bit(29,"SET",1);
						gpio_configure_status_bit(30,"SET",1);
						gpio_configure_status_bit(31,"SET",1);
	 				}
					if (cnc_module_status == 0){
						gpio_configure_status_bit(27,"CLR",1);
						gpio_configure_status_bit(28,"CLR",1);
						gpio_configure_status_bit(29,"CLR",1);
						gpio_configure_status_bit(30,"CLR",1);
						gpio_configure_status_bit(31,"CLR",1);
	 				}
 	}else if (strncmp("SDIO", module, 4)==0){
 					if (cnc_module_status == 1){
                        for (gpionum=32; gpionum<=39; gpionum ++) {
                            gpio_configure_status_bit(gpionum,"SET",1);
                        }
                    }
					if (cnc_module_status == 0){
                        for (gpionum=32; gpionum<=39; gpionum ++) {
                            gpio_configure_status_bit(gpionum,"CLR",1);
                        }
	 				}
 	}else if (strncmp("VIB", module, 3)==0 || strncmp("TTLOUT", module, 6)==0){
 					if (cnc_module_status == 1){
                        for (gpionum=40; gpionum<=63; gpionum ++) {
                            gpio_configure_status_bit(gpionum,"SET",1);

                        }
	 				}
					if (cnc_module_status == 0){

                        for (gpionum=40; gpionum<=63; gpionum ++) {
                            gpio_configure_status_bit(gpionum,"CLR",1);

                        }

	 				}
 	}else {
		return 0;
	}

 return 1;
}


int pinmux_set_status_bit(int mode, int bit, int set)
{
    printk("mode  = %0x ,bit = %0x,set = %0x\n",mode,bit,set);
	
    switch(mode){
        
    case PINMUX_I2C:
        if(set) pinmux_base[REG_PINMUX_I2C>>1] |= (1<<(bit-1)); 
        else pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<(bit-1));				
        break;
    case PINMUX_VGA:
        if(set){
            pinmux_base[REG_PINMUX_I2C>>1] |= (3<<1);
        }
        else {
            pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<2);				
            pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<(bit-1));				

        }
        break;
    case PINMUX_UART:
        if(set) pinmux_base[REG_PINMUX_UART>>1] |= (1<<(bit-1));
        else pinmux_base[REG_PINMUX_UART>>1] &= ~(1<<(bit-1));
        break;
    case PINMUX_DISP:
				
        if(bit <= 5){
            if(set ){
                if(bit == TVE0_YPbPr){
                    pinmux_base[REG_PINMUX_DISP>>1] &= 0;
                }
                else if(bit == OUTIF0_RGB){
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<<8;
                    pinmux_base[REG_PINMUX_DISP>>1] &= ~(1<<10);
                }
                else if(bit == TVE1_CVBS_S_video){
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<<10;
                    pinmux_base[REG_PINMUX_DISP>>1] &= ~(1<<9);
                    pinmux_base[REG_PINMUX_DISP>>1] &= ~(1<<8);
                }
                else if(bit == TVE1_YPbPr){
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<<10;
                    pinmux_base[REG_PINMUX_DISP>>1] &= ~(1<<9);
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<<8;
                }
                else if(bit == OUTIF1_RGB){
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<< 10;
                    pinmux_base[REG_PINMUX_DISP>>1] |= 1<< 9;
                }
								
            }
            else{
                printk("invalidated bit \n");
            }
        }
        else
            printk("invalidated bit \n");
        break;
    case PINMUX_SPI:
        if(bit == 1){
            if(set ) pinmux_base[REG_PINMUX_SPI>>1] |= (1<<(bit-1));
            else pinmux_base[REG_PINMUX_SPI>>1] &= ~(1<<(bit-1));
        }else
            printk("invalidated bit ");
        break;
    case PINMUX_SMI:
        if(bit == 1){
            if(set) pinmux_base[REG_PINMUX_SMI>>1] |= (1<<(bit-1));
            else pinmux_base[REG_PINMUX_SMI>>1] &= ~(1<<(bit-1));
        }else
            printk("invalidated bit ");
        break;
    case PINMUX_TS:
        if(bit == 14){
            if(set) pinmux_base[REG_PINMUX_TS>>1] |= (1<<(bit-1));
            else pinmux_base[REG_PINMUX_TS>>1] &= ~(1<<(bit-1));
        }else
            printk("invalidated bit ");
        break;
    case PINMUX_DVIO:
        if(bit == 2 || bit==1){
            if(set){
                pinmux_base[REG_PINMUX_DVIO>>1] |= (1<<(bit-1));
            }
            else pinmux_base[REG_PINMUX_DVIO>>1] &= ~(1<<(bit-1));
        }else
            printk("invalidated bit ");
        break;
    case PINMUX_Ether:
        if(bit == 1){
            if(set) pinmux_base[REG_PINMUX_ETHER>>1] |= (1<<(bit-1));
            else pinmux_base[REG_PINMUX_ETHER>>1] &= ~(1<<(bit-1));
        }else
            printk("invalidated bit ");
        break;
    case PINMUX_SDIO:
            if(bit == 1){
                if(set) pinmux_base[REG_PINMUX_SDIO>>1] |= (1<<(bit-1));
                else pinmux_base[REG_PINMUX_SDIO>>1] &= ~(1<<(bit-1));
            }else
                printk("invalidated bit ");
        break;
    default :
        printk("invalidated mode \n");		
    }
    return 0;
}


int pinmux_scan_status_bit(int regType)
{
	int ret;	
	switch(regType){
		case PINMUX_I2C:	
			ret = pinmux_base[REG_PINMUX_I2C>>1];
			if(ret & 0x1)
				printk("I2C0 ENA \n");
			else
				printk("GPIO17:16 ENA \n");
			if(ret & 0x2){
					if(ret & 0x6)
						printk(" VGA ENA \n");
					else
						printk("I2C1 ENA  \n");
				}
			else
				printk("GPIO 1:0 ENA  \n");
				if(ret & 0x8)
					printk("output disp1 hsync and vsync  \n");
				else
					printk("output disp2 hsync and vsync  \n");
			break;
		case PINMUX_UART:
			ret = pinmux_base[REG_PINMUX_UART>>1];
			if(ret & 0x1)
				printk("UART0 ENA  \n");
			else
				printk("GPIO 19:18 ENA  \n");
			if(ret & 0x20){
					if(ret & 0x2){
						if(ret & 0x8){
							if(ret & 0x4)
								printk("VIDEO UART ENA  \n");
							else
								printk("AUDIO UART ENA  \n");
						}
					else
						printk("UART1 ENA  \n");
						}
					else{
						if(ret & 0x10)
							printk("UART2 enable  \n");
						else
							printk("IRDA enable  \n");
						}
					
				}
			else
				printk("GPIO 3,6 ENA  \n");
			break;
			case PINMUX_DISP:
				ret = pinmux_base[REG_PINMUX_DISP>>1];
				if(ret & 0)
					printk("TVE0 YPbPr ENA  \n");
				if(ret & 0x100)
					printk("OUTIF0 RGB ENA   \n");
				if(ret & 0x400)
					printk("TVE1 CVBS+S-video ENA  \n");
				if(ret & 0x500)
					printk("TVE1 YPbPr ENA  \n");
				if(ret & 0x600)
					printk("OUTIF1 RGB ENA  \n");
				break;
 			case PINMUX_SPI:
				ret = pinmux_base[REG_PINMUX_SPI>>1];
				if(ret & 0x1)
					printk("SPI enable  \n");
				else
					printk("GPIO[26:22] enable  \n");
				break;
			case PINMUX_SMI:
				ret = pinmux_base[REG_PINMUX_SMI>>1];
				if(ret & 0x1)
					printk("SMI enable  \n");
				else
					printk("GPIO[31:27] enable  \n");
				break;
			case PINMUX_DVIO:
				ret = pinmux_base[REG_PINMUX_DVIO>>1];
				if(ret & 0x2)
					printk("digital video input enable  \n");
				else if (ret & 0x1) 
                    printk("digital video output(TTL) enable  \n");
                else
					printk("GPIO[63:40] enable  \n");
				break;
			case PINMUX_Ether:
				ret = pinmux_base[REG_PINMUX_ETHER>>1];
				if(ret & 0x1)
					printk("ehternet disabled and boot config parameter input enable  \n");
				else
					printk("Ethernet  enable  \n");
				break;
			case PINMUX_SDIO:
				ret = pinmux_base[REG_PINMUX_SDIO>>1];
				if(ret & 0x1)
					printk("SDIO enable  \n");
				else
					printk("GPIO[39:32] enable  \n");
				break;
			case PINMUX_TS:
				ret = pinmux_base[REG_PINMUX_TS>>1];
				if(ret & 0x4000)
					printk("xport 1 enable(xport 0 and xport 1 work under serial mode)  \n");
				else
					printk("xport 1 disalbe(xport 0 work under parallel mode)  \n");
				break;
				
			default:
				printk(" errorr  \n");
		
		}
			
		return 0;	
		
	
}
int pinmux_get_status_bit(int  regType,int bit)
{
	int ret = 0;
	switch(regType){
		case PINMUX_I2C:
			ret = pinmux_base[REG_PINMUX_I2C>>1]&(1<<(bit-1));
			printk("the REG_PINMUX_I2C's %d bit number is %d",bit,ret);
			break;
		case PINMUX_UART:
			ret = pinmux_base[REG_PINMUX_UART>>1]&(1<<(bit-1));
			printk("the PINMUX_UART's %d bit number is %d",bit,ret);
			break;
		case PINMUX_SPI:
			ret = pinmux_base[REG_PINMUX_SPI>>1]&(1<<(bit-1));
			printk("the PINMUX_SPI's %d bit number is %d",bit,ret);
			break;
		case PINMUX_SMI:
			ret = pinmux_base[REG_PINMUX_SMI>>1]&(1<<(bit-1));
			printk("the PINMUX_SMI's %d bit number is %d",bit,ret);
			break;
		case PINMUX_DISP:
			ret = pinmux_base[REG_PINMUX_DISP>>1]&(1<<(bit-1));
			printk("the PINMUX_DISP's %d bit number is %d",bit,ret);
			break;
		case PINMUX_TS:
			ret = pinmux_base[REG_PINMUX_TS>>1]&(1<<(bit-1));
			printk("the PINMUX_TS's %d bit number is %d",bit,ret);
			break;
		case PINMUX_DVIO:
			ret = pinmux_base[REG_PINMUX_DVIO>>1]&(1<<(bit-1));
			printk("the PINMUX_DVIO's %d bit number is %d",bit,ret);
			break;
		case PINMUX_Ether:
			ret = pinmux_base[REG_PINMUX_ETHER>>1]&(1<<(bit-1));
			printk("the PINMUX_Ether's %d bit number is %d",bit,ret);
			break;
		default :
			printk("errorr\n");
		}
	return 0;
}
int pinmux_enable_gpio(GPIO_ID GPIO)
{
	int rt=0;


    switch(GPIO)
        {

        case _gpio_0     :
        case _gpio_1     :
			pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<1);
			cnc_gpio_register_module_set("I2C1",0);
            break;
        case _gpio_2     :
            break;
        case _gpio_3     :
		case _gpio_6     :
			pinmux_base[REG_PINMUX_UART>>1] &= ~(1<<5);
			cnc_gpio_register_module_set("URAT2",0);
            break;
        case _gpio_4     :
            break;
        case _gpio_5     :
            break; 
        case _gpio_7     :
            break;
        case _gpio_8     :
            break;
        case _gpio_9     :
            break;
        case _gpio_10    :
            break;
        case _gpio_11    :
            break;
        case _gpio_12    :
            break;
        case _gpio_13    :
            break;
        case _gpio_14    :
            break;
        case _gpio_15    :
            break;
        case _gpio_16    :
        case _gpio_17    :
			pinmux_base[REG_PINMUX_I2C>>1] &= ~1;
			cnc_gpio_register_module_set("I2C0",0);
            break;	
        case _gpio_18    :
        case _gpio_19    :
			pinmux_base[REG_PINMUX_UART>>1] &= ~1;
			cnc_gpio_register_module_set("URAT0",0);
            break;
        case _gpio_20    :
			printk("has not used \n");
            break;
        case _gpio_21    :
			printk("has not used \n");
            break;
        case _gpio_22    :
        case _gpio_23    :
        case _gpio_24    :
        case _gpio_25    :
        case _gpio_26    :
			pinmux_base[REG_PINMUX_SPI>>1] &= ~1;
			cnc_gpio_register_module_set("SPI",0);
            break;
        case _gpio_27    :
        case _gpio_28    :
        case _gpio_29    :
        case _gpio_30    :
        case _gpio_31    :
			pinmux_base[REG_PINMUX_SMI>>1] &= ~1;
			cnc_gpio_register_module_set("SMI",0);
            break;
        case _gpio_32    :
        case _gpio_33    :
        case _gpio_34    :
        case _gpio_35    :
        case _gpio_36    :
        case _gpio_37    :
        case _gpio_38    :
        case _gpio_39    :
			pinmux_base[REG_PINMUX_SDIO>>1] &= ~(1<<1);
			cnc_gpio_register_module_set("SDIO",0);
            break;
        case _gpio_40    :
        case _gpio_41    :
        case _gpio_42    :
        case _gpio_43    :
        case _gpio_44    :
        case _gpio_45    :
        case _gpio_46    :
        case _gpio_47    :
        case _gpio_48    :
        case _gpio_49    :
        case _gpio_50    :
        case _gpio_51    :
        case _gpio_52    :
        case _gpio_53    :
        case _gpio_54    :
        case _gpio_55    :
        case _gpio_56    :
        case _gpio_57    :
        case _gpio_58    :
        case _gpio_59    :
        case _gpio_60    :
        case _gpio_61    :
        case _gpio_62    :
        case _gpio_63    :
			pinmux_base[REG_PINMUX_DVIO>>1] = 0;
			cnc_gpio_register_module_set("DVIO",0);
            break;
        default:
            ;

        }
	return rt;

}


int pinmux_enable_i2c(I2C_ID I2C)
{
	int rt=0;

	switch(I2C)
	{
		case _i2c_0:
			pinmux_base[REG_PINMUX_I2C>>1] |= 1;
			cnc_gpio_register_module_set("I2C0",1);
			break;
		case _i2c_1:
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<1);
			pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<2);
			cnc_gpio_register_module_set("I2C1",1);
			break;
		default:
			;
	}

	return rt;
}
int pinmux_enable_uart(UART_ID UART)
{
	int rt=0;

	switch(UART)
	{
		case _uart_0:
			pinmux_base[REG_PINMUX_UART>>1] |= 1;
			cnc_gpio_register_module_set("URAT0",1);
			break;
		case _uart_1:
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<5);
			pinmux_base[REG_PINMUX_UART>>1] &= ~(1<<3);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<1);
			break;
		case _uart_2:
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<5);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<4);
			cnc_gpio_register_module_set("URAT2",1);
			break;
		case _uart_audio:
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<1);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<3);
			pinmux_base[REG_PINMUX_UART>>1] &= ~(1<<2);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<5);
			break;
		case _uart_video:
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<1);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<3);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<2);
			pinmux_base[REG_PINMUX_UART>>1] |= (1<<5);
			break;
		default:
			;
	}

	return rt;
}

int pinmux_enable_irda(void)
{

	pinmux_base[REG_PINMUX_UART>>1] |= (1<<5);
	pinmux_base[REG_PINMUX_UART>>1] &= ~(1<<4);
	return 0;
}
 int pinmux_enable_spi(void)
{
	pinmux_base[REG_PINMUX_SPI>>1] |= 1;
	cnc_gpio_register_module_set("SPI",1);
	return 0;
}

int pinmux_enable_smi(void)
{

	pinmux_base[REG_PINMUX_SMI>>1] |= 1;
	cnc_gpio_register_module_set("SMI",1);
	return 0;

}

int pinmux_enable_vib(void)
{

	pinmux_base[REG_PINMUX_DVIO>>1] = (1<<1);
    gpio_base[REG_GPIO_SWPORTA_DDR>>2] |= 0xffffff00; 
	cnc_gpio_register_module_set("VIB",1);
	return 0;
}


int pinmux_enable_ttlout(void)
{

	pinmux_base[REG_PINMUX_DVIO>>1] = 0x1;
	cnc_gpio_register_module_set("TTL",1);
	return 0;
}

int pinmux_enable_ethernet(void)
{

	pinmux_base[REG_PINMUX_ETHER>>1] &= ~1;
	return 0;
}

int pinmux_enable_booting(void)
{

	pinmux_base[REG_PINMUX_ETHER>>1] |= 1;
	return 0;
}

int pinmux_set_dac1mode(DAC1_MODE Mode)
{

	switch(Mode)
	{
		case _dac1_tve0_ypbpr:
			pinmux_base[REG_PINMUX_DISP>>1] &= (0<<8);
			break;
		case _dac1_tve1_ypbpr:
			pinmux_base[REG_PINMUX_DISP>>1] = (5<<8);		
			break;
		case _dac1_disp0_rgb:
			pinmux_base[REG_PINMUX_DISP>>1] = (1<<8);		
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<1);	
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<2);		
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<3);		
			break;
		case _dac1_disp1_rgb:
			pinmux_base[REG_PINMUX_DISP>>1] = (6<<8);
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<1);		
			pinmux_base[REG_PINMUX_I2C>>1] |= (1<<2);			
			pinmux_base[REG_PINMUX_I2C>>1] &= ~(1<<3);			
			break;
		case _dac1_tve1_cvbs:
			pinmux_base[REG_PINMUX_DISP>>1] = (4<<8);			
			break;
	}

	return 0;

}
int pinmux_set_xportmode(XPORT_MODE Mode)
{

	switch (Mode)
	{
		case _xport_serial_mode:
			pinmux_base[REG_PINMUX_TS>>1] |= (1<<14);		
			break;
		case _xport_paralell_mode:
			pinmux_base[REG_PINMUX_TS>>1] &= ~(1<<14);		
			break;
		default:
			printk("set wrong mode \n");
		}

	return 0;
}

int pinmux_update_rc_status(void)
{
 	unsigned short rc_ctl; 
	
	rc_ctl = rc_base[RC_CTL_H>>1] & 0x8000;
    //    rc_ctl = rc_ctl >> 15;
    cnc_gpio_register_module_set("RC", rc_ctl?1:0);  
 	return 0;
}

#define FPC_LED_CNTL_H 		0x206 /* Led Display Control Register */
#define FPC_KEYSCAN_CNTL_L  0x304 
int pinmux_update_fpc_status(void)
{
    unsigned short keyscan_ctl, led_ctl;
    unsigned int arg;

    led_ctl = fpc_base[FPC_LED_CNTL_H >> 1] & 0x8000;
    keyscan_ctl = fpc_base[FPC_KEYSCAN_CNTL_L >>1] & 0x1;

    arg = (led_ctl | keyscan_ctl) ? 1: 0; 
    cnc_gpio_register_module_set("FPC-COMMON", arg);

    cnc_gpio_register_module_set("FPC-LED-SPECIAL", led_ctl? 1: 0);
            
	return 0;
}


EXPORT_SYMBOL(pinmux_update_fpc_status);
EXPORT_SYMBOL(pinmux_get_status_bit);
EXPORT_SYMBOL(pinmux_set_status_bit);
EXPORT_SYMBOL(pinmux_scan_status_bit);
EXPORT_SYMBOL(pinmux_set_xportmode);
EXPORT_SYMBOL(pinmux_enable_uart);
EXPORT_SYMBOL(pinmux_enable_i2c);
EXPORT_SYMBOL(pinmux_enable_irda);
EXPORT_SYMBOL(pinmux_enable_spi);
EXPORT_SYMBOL(pinmux_enable_smi);
EXPORT_SYMBOL(pinmux_enable_vib);
EXPORT_SYMBOL(pinmux_enable_ethernet);
EXPORT_SYMBOL(pinmux_enable_booting);
EXPORT_SYMBOL(pinmux_set_dac1mode);
EXPORT_SYMBOL(pinmux_enable_gpio);
EXPORT_SYMBOL(pinmux_update_rc_status);
//EXPORT_SYMBOL(cnc_gpio_register_module_set);

static int
cnc18xx_pinmux_open(struct inode *inode, struct file *file)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	printk("cnc18xx_pinmux_open %0x \n",m);
	return 0;
}

static int 
cnc18xx_pinmux_close (struct inode *inode, struct file *file)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	printk("cnc18xx_pinmux_close %0x \n",m);
	return 0;
}


static int  pinmux_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	
	int len=0;
	int i =1;
	int ret;

	ret = pinmux_base[REG_PINMUX_I2C>>1];
		if(ret & 0x1)
			len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by I2C0. \n",i++);
		else
			len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by GPIO[17:16]. \n",i++);
		
		if(ret & 0x2){
				//len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by I2C1/VGA sync output. \n",i++);
				if(ret & 0x4)
					len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by VGA sync output. \n",i++);
				else
					len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by I2C1 output. \n",i++);
			}
		else
			len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by GPIO[1:0]. \n",i++);

		if(ret & 0x8)
			len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by output disp1 hsync and vsync. \n",i++);
		else
			len += sprintf(page+len,"==> REG_PINMUX_I2C's PINMUX %d is held by output disp2 hsync and vsync. \n",i++);
	
	ret = pinmux_base[REG_PINMUX_UART>>1];
		if(ret & 0x1)
		len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by UART0. \n",i++);
		else
		len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by GPIO[19:18]. \n",i++);
		
		if(ret & 0x20){
			if(ret & 0x2)
				if(ret & 0x8)
					if(ret & 0x4)
						len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by Video UART. \n",i++);
					else
						len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by Audio UART. \n",i++);
				else
					len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by UART1. \n",i++);

			else
				if(ret & 0x10)
					len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by UART2. \n",i++);
				else
					len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by IRDA . \n",i++);
						
			}
		else
		len += sprintf(page+len,"==> REG_PINMUX_UART's PINMUX %d is held by GPIO[3,6]. \n",i++);

	
	ret = pinmux_base[REG_PINMUX_DISP>>1];
		if(!(ret&0x100) && !(ret&0x400))
			len += sprintf(page+len,"==> REG_PINMUX_DISP's PINMUX %d is held by TVE0 YPbPr. \n",i++);
		else if((ret & 0x100) && !(ret&0x400))
			len += sprintf(page+len,"==> REG_PINMUX_DISP's PINMUX %d is held by OUTIF0 RGB. \n",i++);
		else if ((ret & 0x400) && !(ret & 0x100) && !(ret & 0x200))
			len += sprintf(page+len,"==> REG_PINMUX_DISP's PINMUX %d is held by TVE1 CVBS+S-video. \n",i++);
		else if ((ret & 0x400) && !(ret & 0x200) && (ret & 0x100))
			len += sprintf(page+len,"==> REG_PINMUX_DISP's PINMUX %d is held by TVE1 YPbPr. \n",i++);
		else if ((ret & 0x400)&&(ret & 0x200))
			len += sprintf(page+len,"==> REG_PINMUX_DISP's PINMUX %d is held by OUTIF1 RGB. \n",i++);

	ret = pinmux_base[REG_PINMUX_SPI>>1];
		if(ret & 0x1)
			len += sprintf(page+len,"==> REG_PINMUX_SPI's PINMUX %d is held by SPI. \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_SPI's PINMUX %d is held by GPIO[26:22]. \n",i++);
	
	ret = pinmux_base[REG_PINMUX_SMI>>1];
		if(ret & 0x1)
			len += sprintf(page+len,"==> REG_PINMUX_SPI's PINMUX %d is held by SMI. \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_SPI's PINMUX %d is held by GPIO[31:27]. \n",i++);


	ret = pinmux_base[REG_PINMUX_TS>>1];
		if(ret & 0x4000)
			len += sprintf(page+len,"==> REG_PINMUX_TS's PINMUX %d xport 1 enable(xport 0 and xport 1 work under serial mode). \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_TS's PINMUX %d  xport 1 disalbe(xport 0 work under parallel mode). \n",i++);	

	ret = pinmux_base[REG_PINMUX_DVIO>>1];
		if(ret & 0x2)
			len += sprintf(page+len,"==> REG_PINMUX_DVIO's PINMUX %d is held by digital video input . \n",i);
        else if (ret & 0x1)
            len += sprintf(page+len,"==> REG_PINMUX_DVIO's PINMUX %d is held by digital video output(TTL) . \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_DVIO's PINMUX %d is held by GPIO[63:40]. \n",i++);

	ret = pinmux_base[REG_PINMUX_ETHER>>1];
		if(ret & 0x1)
			len += sprintf(page+len,"==> REG_PINMUX_ETH's PINMUX %d  ehternet disabled and boot config parameter input enable. \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_ETH's PINMUX %d is held by Ethernet . \n",i++);
	ret = pinmux_base[REG_PINMUX_SDIO>>1];
		if(ret & 0x1)
			len += sprintf(page+len,"==> REG_PINMUX_SDIO's PINMUX %d  SDIO enable. \n",i);
		else
			len += sprintf(page+len,"==> REG_PINMUX_SDIO's PINMUX %d is held by GPIO[39:32] . \n",i++);
	return len;
}

static int pinmux_proc_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{
	unsigned short bit;
	unsigned short val;

	const char *cmd_line = buffer;

	if (strncmp("WW_I2C", cmd_line, 6) == 0) {
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_I2C, bit, val);
		
	} else if (strncmp("WW_UAR", cmd_line, 6) == 0) {
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_UART, bit, val);
	}else if(strncmp("WW_DIS", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_DISP, bit, val);
	}else if(strncmp("WW_SPI", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_SPI, bit, val);
	}else if(strncmp("WW_SMI", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_SMI, bit, val);
	}else if(strncmp("WW_TS1", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_TS, bit, val);
	}else if(strncmp("WW_ETH", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_Ether, bit, val);
	}else if(strncmp("WW_DVI", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_DVIO, bit, val);
	
	}else if(strncmp("WW_SDIO", cmd_line, 6) == 0){
		bit = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		pinmux_set_status_bit(PINMUX_SDIO, bit, val);
	}
	return count;
}

int cnc18xx_pinmux_init_setting(void)
{

#ifdef CONFIG_PINMUX_UART2_DEFAULT_ENABLE
    pinmux_enable_uart(_uart_1);    
#endif

    return 0;
}

static int __init
cnc18xx_pinmux_init(void)
{
	int result;
	dev_t dev = 0;
    int err=0;
	if (pinmux_major) {
		dev = MKDEV(pinmux_major, 0);
		result = register_chrdev_region(dev, pinmux_nr_devs, "pinmux");
	} else {
		result = alloc_chrdev_region(&dev, 0, pinmux_nr_devs, "pinmux");
		pinmux_major = MAJOR(dev);
	}

	if (result < 0) {
		printk(KERN_WARNING "PINMUX: can't get major %d\n", pinmux_major);
		return result;
	}

	if (!request_mem_region(CNC18XX_PINMUX_BASE, CNC18XX_PINMUX_SIZE, "CNC18XX PINMUX")) {
		unregister_chrdev_region(dev, pinmux_nr_devs);
		return -EIO;
	}

	pinmux_base =
	    (volatile unsigned short *) ioremap(CNC18XX_PINMUX_BASE,
						CNC18XX_PINMUX_SIZE);
	if (!pinmux_base) {
		unregister_chrdev_region(dev, pinmux_nr_devs);
		printk(KERN_WARNING "PINMUX: ioremap failed.\n");
		return -EIO;
	}

	rc_base = (u16 *)VA_FPC_BASE;
	fpc_base = (u16 *)VA_FPC_BASE;
	if (!fpc_base) {
		printk(KERN_WARNING "fpc: unknow address.\n");
		return -EIO;
	}

	gpio_base =(unsigned int *) VA_GPIO_BASE;

    cnc1800x_pinmux_class = class_create(THIS_MODULE,"pinmux");
	device_create(cnc1800x_pinmux_class, NULL, MKDEV(pinmux_major, 0) , NULL, "pinmux");

    if (IS_ERR(cnc1800x_pinmux_class)){
        err = PTR_ERR(cnc1800x_pinmux_class);
		unregister_chrdev_region(dev, pinmux_nr_devs);
        printk(KERN_WARNING "PINMUX: class create failed.\n");
        return -EIO;
    }

	pinmux_devices =
	    kmalloc(pinmux_nr_devs * sizeof (struct pinmux_devs), GFP_KERNEL);
	if (!pinmux_devices) {
		iounmap((void *) pinmux_base);
		release_mem_region(CNC18XX_PINMUX_BASE, CNC18XX_PINMUX_SIZE);
		unregister_chrdev_region(dev, pinmux_nr_devs);
		return -ENOMEM;
	}
	memset(pinmux_devices, 0, pinmux_nr_devs * sizeof (struct pinmux_devs));

	spin_lock_init(&pinmux_lock);

	printk(KERN_INFO "CNC18XX PINMUX at 0x%x, %d lines\n", CNC18XX_PINMUX_BASE,
	       pinmux_nr_devs);

	pinmux_proc_entry = create_proc_entry("pinmux", 0, NULL);
	if (NULL != pinmux_proc_entry) {
		pinmux_proc_entry->write_proc = &pinmux_proc_write;
		pinmux_proc_entry->read_proc = &pinmux_proc_read;
	}

    cnc18xx_pinmux_init_setting();

	return 0;
}

static void __exit
cnc18xx_pinmux_exit(void)
{
	int i;
	dev_t devno = MKDEV(pinmux_major, 0);

	if (pinmux_devices) {
		for (i = 0; i < pinmux_nr_devs; i++) {
			cdev_del(&pinmux_devices[i].cdev);
		}
		kfree(pinmux_devices);
		pinmux_devices = NULL;
	}

	iounmap((void *) pinmux_base);
	release_mem_region(CNC18XX_PINMUX_BASE, CNC18XX_PINMUX_SIZE);
	unregister_chrdev_region(devno, pinmux_nr_devs);
}

module_init(cnc18xx_pinmux_init);
module_exit(cnc18xx_pinmux_exit);

MODULE_AUTHOR("Cavium");
MODULE_DESCRIPTION("Cavium Celestial CNC1800L PINMUX driver");
MODULE_LICENSE("GPL");

