/****************************************************************************
  * Copyright (C) 2008-2010 Celestial Semiconductor Inc.
  * All rights reserved
  *
  * [RELEASE HISTORY]
  * VERSION  DATE       AUTHOR                  DESCRIPTION
  * 0.1      10-04-?   Hao.Ran           			Original
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
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <mach/hardware.h>
#include <linux/poll.h>

#include <asm/mach/irq.h>


#define CMD_READ_GPIO         _IOR('G', 1, unsigned long)
#define CMD_WRITE_GPIO        _IOW('G', 2, unsigned long)
#define CMD_SET_DIRECTION     _IOW('G', 3, unsigned long)
#define CMD_ENA_GPIO_INTR     _IOW('G', 4, unsigned long)

#define GPIO_READ 0
#define GPIO_WRITE 1


struct gpio_cmd {
	int gpio;
	int value;
};


#define REG_GPIO_SWPORTA_DR             0x20
#define REG_GPIO_SWPORTA_DDR            0x24
#define REG_GPIO_EXT_PORTA              0x60


#define REG_GPIO_SWPORTB_DR             0x34
#define REG_GPIO_SWPORTB_DDR            0x38
#define REG_GPIO_EXT_PORTB              0x64

#define GPIO_MAJOR  	0
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
#define GPIO_NR_DEVS	64
#else
#define GPIO_NR_DEVS	41
#endif

#define GPIO_PINMUXA    0x30
#define GPIO_PINMUXB    0x44


static int gpio_major = GPIO_MAJOR;
static int gpio_nr_devs = GPIO_NR_DEVS;


module_param(gpio_major, int, GPIO_MAJOR);
module_param(gpio_nr_devs, int, GPIO_NR_DEVS);
MODULE_PARM_DESC(gpio_major, "GPIO major number");
MODULE_PARM_DESC(gpio_nr_devs, "GPIO number of lines");


typedef struct gpio_devs {
	u32 gpio_id;
	struct cdev cdev;
    wait_queue_head_t irq_wait_queue;
    u32 irq_requested : 8;
    u32 cur_irq_type  : 8;       // current irq type set by user, level_low or level_high
    u32 last_rcv_irq_type : 8;   // last receive irq, level_low or level_high
    u32 report_event  : 8;       // receive a irq and need to report
} gpio_devs_t;

typedef enum {
    GPIO_IRQ_TYPE_LEVEL_HIGH,
    GPIO_IRQ_TYPE_LEVEL_LOW
}CSGPIO_IRQ_TYPE;

typedef struct __gpio_intr_param
{
    u32 enable     : 1;       // 1 for enable, 0 for disable
    u32 irq_type   : 16;
    u32 gpio_index : 15;
}gpio_intr_param;

static struct class *cnc18xx_gpio_class;

/*
 * The devices
 */
struct gpio_devs *gpio_devices = NULL;	/* allocated in gpio_init_module */

/* function declaration ------------------------------------- */
static int __init cnc_gpio_init(void);
static void __exit cnc_gpio_exit(void);

static void gpio_setup_cdev(struct gpio_devs *dev, int index);

static ssize_t cnc_gpio_write(struct file *filp, const char __user * buf,
			  size_t count, loff_t * f_pos);
static ssize_t cnc_gpio_read(struct file *filp, char __user * buf, size_t count,
			 loff_t * f_pos);
static int cnc_gpio_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		      unsigned long arg);
static unsigned int cnc_gpio_poll(struct file *filp, poll_table *wait);
static int cnc_gpio_open(struct inode *inode, struct file *file);
static int cnc_gpio_close (struct inode *inode, struct file *file);
static int gpio_get_status_bit(unsigned int gpio,char is_pinmux);
void gpio_configure_status_bit(unsigned int bit, const char * configure_cmd,char is_pinmux);

//int cnc_gpio_register_module_set(const char * module, unsigned int  cnc_module_status);

static spinlock_t gpio_lock;
static volatile unsigned int *gpio_base;
static unsigned int gpio_status1 = 0;
static unsigned int gpio_status2 = 0;
static unsigned int gpio_pinmux_status1 =0;
static unsigned int gpio_pinmux_status2 =0;
static struct proc_dir_entry *gpio_proc_entry = NULL;
static struct proc_dir_entry *system_config_proc_entry=NULL;
/* ----------------------------------------------------------- */

/* gpio device struct */
struct file_operations gpio_fops = {
	.owner = THIS_MODULE,
	.read = cnc_gpio_read,
	.write = cnc_gpio_write,
	.ioctl = cnc_gpio_ioctl,
	.open = cnc_gpio_open,
	.release  = cnc_gpio_close,
	.poll = cnc_gpio_poll,
};

int gpio_get_status_bit(unsigned int gpio,char is_pinmux)
{

	int ret;
	if((gpio >= 0)&&(gpio < 32))
	{
		ret = ((gpio_status1 >> gpio) & 0x1);
		if (is_pinmux ==1)
			ret = ret & ((gpio_pinmux_status1 >> gpio) & 0x1);
	}
	else
	{
		gpio = gpio - 32;
		ret = ((gpio_status2 >> gpio) & 0x1);
		if (is_pinmux ==1)
			ret = ret & ((gpio_pinmux_status2 >> gpio) & 0x1);
	}
	return ret;
}

 void gpio_configure_status_bit(unsigned int bit, const char * configure_cmd,char is_pinmux)
{


	if((bit >= 0)&&(bit < 32))
	{
		if (strncmp("CLR", configure_cmd, 3) == 0){
			gpio_status1 &= ~(1<<bit);
			if (is_pinmux == 1)
				gpio_pinmux_status1 &= ~(1<<bit);
		}
		if (strncmp("SET", configure_cmd, 3) == 0){
			gpio_status1 |= (1<<bit);
			if (is_pinmux == 1)
				gpio_pinmux_status1 |= (1<<bit);
		}
	}
	else
	{
		bit = bit - 32;
		if (strncmp("CLR", configure_cmd, 3) == 0){
			gpio_status2 &= ~(1<<bit);
			if (is_pinmux == 1)
				gpio_pinmux_status2 &= ~(1<<bit);
		}
		if (strncmp("SET", configure_cmd, 3) == 0){
			gpio_status2 |= (1<<bit);
			if (is_pinmux == 1)
				gpio_pinmux_status2 |= (1<<bit);
		}
	}
}


int
gpio_hw_set_direct(int gpio_id, int dir)
{
	unsigned int flags;
	spin_lock(&gpio_lock);
	if((gpio_id < 32)&&(gpio_id >= 0))
	{

		flags = (1<<gpio_id);
		if (dir == GPIO_READ) {
			gpio_base[REG_GPIO_SWPORTA_DDR >> 2] |= flags;
		} else if (dir == GPIO_WRITE){
			gpio_base[REG_GPIO_SWPORTA_DDR >> 2] &= ~flags;
		} else{
			return -1;
		}

	}
	else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS))
	{
		gpio_id = gpio_id - 32;
		flags = (1<<gpio_id);
		if (dir == GPIO_READ) {
			gpio_base[REG_GPIO_SWPORTB_DDR >> 2] |= flags;
		} else if (dir == GPIO_WRITE){
			gpio_base[REG_GPIO_SWPORTB_DDR >> 2] &= ~flags;
		} else{
			return -1;
		}
	}

	spin_unlock(&gpio_lock);
	return 0;
}

static unsigned short
gpio_hw_read(unsigned char gpio_id)
{
	if((gpio_id < 32)&&(gpio_id >= 0))
	{
		return (gpio_base[REG_GPIO_EXT_PORTA >> 2] >> gpio_id) & 0x1;
	}
	else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS))
	{
		return (gpio_base[REG_GPIO_EXT_PORTB >> 2] >> gpio_id) & 0x1;
	}
	return 0;
}

int
gpio_hw_write(unsigned char gpio_id, unsigned char data)
{
	unsigned int flags ;
	if((gpio_id < 32)&&(gpio_id >= 0))
	{
		flags = (1<<gpio_id);
		if (!data) {
			gpio_base[REG_GPIO_SWPORTA_DR>>2 ] &= ~flags;
			gpio_status1 &= ~flags;
		} else {
			gpio_base[REG_GPIO_SWPORTA_DR>>2 ] |= flags;
			gpio_status1 |= flags;
		}
	}
	else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS))
	{
		gpio_id = gpio_id - 32;
		flags = (1<<gpio_id);
		if (!data) {
			gpio_base[REG_GPIO_SWPORTB_DR>>2 ] &= ~flags;
			gpio_status2 &= ~flags;
		} else {
			gpio_base[REG_GPIO_SWPORTB_DR>>2 ] |= flags;
			gpio_status2 |= flags;
		}
	}
	return 0;
}

static void
gpio_setup_cdev(struct gpio_devs *dev, int index)
{
	int errorcode = 0;
	char device_name[80];

	int devno = MKDEV(gpio_major, index);

	cdev_init(&dev->cdev, &gpio_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &gpio_fops;

	errorcode = cdev_add(&dev->cdev, devno, 1);
	if (errorcode) {
		printk(KERN_NOTICE "GPIO: Error %d adding gpio %d", errorcode,
		       index);
	}

	snprintf(device_name, sizeof (device_name), "gpio%d", index);
	device_create(cnc18xx_gpio_class, NULL, MKDEV(gpio_major, index), NULL, device_name);

}

#ifdef CONFIG_GPIO_INTERRUPT_SUPPORT

//#define MAX_GPIO_NUMBER         41
#define PORTA_MAX_NUMBER        32

#define CNC_GPIO_IRQ            6

#define REG_GPIO_INTMASK_A		0x28
#define REG_GPIO_INTMASK_B		0x3c

#define REG_GPIO_POLARITY_A		0x2c
#define REG_GPIO_POLARITY_B		0x40

#define REG_GPIO_INT_S_A		    0x68
#define REG_GPIO_INT_S_B		    0x6c

/*
 *  CNC GPIO IRQ Control
 */


static void cnc_gpio_irq_mask(unsigned int irq)
{
	unsigned long flags;
    unsigned int value;
    unsigned int gpio_id= irq_to_gpio(irq);
	spin_lock_irqsave(&gpio_lock, flags);
    //    printk("Entered gpio mask status: 0x%x, 0x%x, mask: 0x%x\n", gpio_base[REG_GPIO_INT_S_A>>2], gpio_base[REG_GPIO_INT_S_B>>2], gpio_base[REG_GPIO_INTMASK_A >> 2]);
	if(gpio_id < 32){
        value = (1<<gpio_id);
        gpio_base[REG_GPIO_INTMASK_A >> 2] |= value;

    } else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS)) {

        value = (1 << (gpio_id - 32));
        gpio_base[REG_GPIO_INTMASK_B >> 2] |= value;
    }

	spin_unlock_irqrestore(&gpio_lock, flags);
    //    printk("Out gpio intrrupt! status: 0x%x, 0x%x, mask: 0x%x\n", gpio_base[REG_GPIO_INT_S_A>>2], gpio_base[REG_GPIO_INT_S_B>>2], gpio_base[REG_GPIO_INTMASK_A >> 2]);
}

static void cnc_gpio_irq_unmask(unsigned int irq)
{
	unsigned long flags;
    unsigned int gpio_id= irq_to_gpio(irq);
    unsigned int value;

	spin_lock_irqsave(&gpio_lock, flags);

	if(gpio_id < 32){
        value = (1<<gpio_id);
        gpio_base[REG_GPIO_INTMASK_A >> 2] &= ~value;

    } else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS)) {

        value = (1 << (gpio_id - 32));
        gpio_base[REG_GPIO_INTMASK_B >> 2] &= ~value;

    }
	spin_unlock_irqrestore(&gpio_lock, flags);
}


static int cnc_gpio_set_irq_type(unsigned irq, unsigned int type)
{
    struct irq_desc *desc = irq_desc + irq;
    unsigned int gpio_id= irq_to_gpio(irq);
    unsigned int value;

    gpio_hw_set_direct(gpio_id, GPIO_READ);
    //printk("Set irq:%d, gpio:%d 's type : %d\n", irq,  gpio_id, type);
    switch (type) {
    case IRQ_TYPE_LEVEL_HIGH: /* Attentation Document is wrong */

        if(gpio_id < 32){
            value = (1<<gpio_id);
            gpio_base[REG_GPIO_POLARITY_A >> 2] &= ~value;

        } else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS)) {
            value = (1 << (gpio_id - 32));
            gpio_base[REG_GPIO_POLARITY_B >> 2] &= ~value;
        }
        desc->handle_irq = handle_level_irq;
        break;
    case IRQ_TYPE_LEVEL_LOW:
        if(gpio_id < 32){
            value = (1<<gpio_id);
            gpio_base[REG_GPIO_POLARITY_A >> 2] |= value;
        } else if((gpio_id >= 32)&&(gpio_id < GPIO_NR_DEVS)) {

            value = (1 << (gpio_id - 32));
            gpio_base[REG_GPIO_POLARITY_B >> 2] |= value;
        }
        desc->handle_irq = handle_level_irq;
        break;

    default :
        pr_err("cnc: failed to set irq type %d for gpio %d\n",
               type, gpio_id);
        return -EINVAL;
    }


    desc->status &= ~IRQ_TYPE_SENSE_MASK;
    desc->status |= type & IRQ_TYPE_SENSE_MASK;
    return 0;
}



static struct irq_chip cnc_gpio_irq_chip = {
    .name= "GPIO",
    .ack = cnc_gpio_irq_mask,
    .mask_ack= cnc_gpio_irq_mask,
    .mask= cnc_gpio_irq_mask,
    .unmask= cnc_gpio_irq_unmask,
    .set_type= cnc_gpio_set_irq_type,
};


static void cnc_gpio_interrupt(unsigned irq, struct irq_desc *desc)
{
    unsigned long i_status_a, i_status_b;
    unsigned int dir_a, dir_b, gpio_id=0, value=0;
    int gpio_irq;

    //printk("Entered gpio intrrupt! status: 0x%x, 0x%x, mask: 0x%x, priority: 0x%x\n", gpio_base[REG_GPIO_INT_S_A>>2], gpio_base[REG_GPIO_INT_S_B>>2], gpio_base[REG_GPIO_INTMASK_A >> 2], gpio_base[REG_GPIO_POLARITY_A >> 2]);
    i_status_a = gpio_base[REG_GPIO_INT_S_A >> 2];
    i_status_b = gpio_base[REG_GPIO_INT_S_B >> 2];
    dir_a = gpio_base[REG_GPIO_SWPORTA_DDR >> 2];
    dir_b = gpio_base[REG_GPIO_SWPORTB_DDR >> 2];

    for_each_bit(gpio_id,&i_status_a, 32){

        value = 1 << gpio_id;
        //   printk("gpio_id = %d, value=0x%x\n, dir_a=0x%x", gpio_id, value, dir_a);
        if (value & dir_a){
            gpio_irq = gpio_to_irq(gpio_id);
            generic_handle_irq(gpio_irq);
        } else {
            continue;  /* means dir is output */
        }
    }

    for_each_bit(gpio_id,&i_status_b, GPIO_NR_DEVS-32){
        value = 1 << gpio_id;
        if (value & dir_b) {
            gpio_irq = gpio_to_irq(gpio_id + 32);
            generic_handle_irq(gpio_irq);
        } else {
            continue;  /* means dir is output */
        }
    }
    //printk("Out gpio intrrupt! status: 0x%x, 0x%x, mask: 0x%x\n", gpio_base[REG_GPIO_INT_S_A>>2], gpio_base[REG_GPIO_INT_S_B>>2], gpio_base[REG_GPIO_INTMASK_A >> 2]);
}

static irqreturn_t gpio_interrupt_handler(int irq, void *dev_id)
{
    int gpio_index = irq_to_gpio(irq);
    int polarity;

    // change the irq polarity to avoid continual irq
    polarity = gpio_base[(gpio_index<32?REG_GPIO_POLARITY_A:REG_GPIO_POLARITY_B)>>2]&(1<<gpio_index);

    if (polarity == 0) {
        gpio_devices[gpio_index].last_rcv_irq_type = GPIO_IRQ_TYPE_LEVEL_HIGH;
        set_irq_type(gpio_to_irq(gpio_index), IRQ_TYPE_LEVEL_LOW);
    } else {    // active-low
        gpio_devices[gpio_index].last_rcv_irq_type = GPIO_IRQ_TYPE_LEVEL_LOW;
        set_irq_type(gpio_to_irq(gpio_index), IRQ_TYPE_LEVEL_HIGH);
    }

    wake_up(&gpio_devices[gpio_index].irq_wait_queue);
    gpio_devices[gpio_index].report_event = 1;

    return IRQ_HANDLED;
}

#endif

EXPORT_SYMBOL(gpio_hw_write);
EXPORT_SYMBOL(gpio_hw_set_direct);
EXPORT_SYMBOL(gpio_configure_status_bit);
static int
cnc_gpio_open(struct inode *inode, struct file *file)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	if (gpio_get_status_bit(m,1) != 0 )
	{
		printk("\n EBUSY   \n");
		return -EBUSY;
	}
	gpio_configure_status_bit(m ,"SET",1);
    init_waitqueue_head(&gpio_devices[m].irq_wait_queue);
	return 0;
}

static int
cnc_gpio_close (struct inode *inode, struct file *file)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	if (gpio_get_status_bit(m,1) != 0 )
	gpio_configure_status_bit(m ,"CLR",0);

    if (gpio_devices[m].irq_requested)
    {
        gpio_devices[m].irq_requested = 0;
        free_irq(gpio_to_irq(m), NULL);
    }
	return 0;
}

static ssize_t
cnc_gpio_write(struct file *file, const char __user * data,
	   size_t len, loff_t * ppos)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	size_t i;
	for (i = 0; i < len; ++i) {
		char c;
		if (get_user(c, data + i))
			return -EFAULT;
		switch (c) {
		case '0':
			gpio_hw_write(m, 0);
			break;
		case '1':
			gpio_hw_write(m, 1);
			break;
		case 'O':
			gpio_hw_set_direct(m, GPIO_WRITE);
			break;
		case 'o':
			gpio_hw_set_direct(m, GPIO_READ);
			break;
		}
	}

	return len;
}

static int system_config_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len=0;

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)
	len += sprintf(page+len,"Linux kernel is configured for CNC1800H. \n");
	len += sprintf(page+len,"System is configured for %d Total Memory.\n", CONFIG_CELESTIAL_MEM_SIZE);
#elif defined(CONFIG_MACH_CELESTIAL_CNC1800L)
	len += sprintf(page+len,"Linux kernel is configured for CNC1800L. \n");
	len += sprintf(page+len,"System is configured for %d Total Memory.\n", CONFIG_CELESTIAL_MEM_SIZE);
#else
    len += sprintf(page+len,"Linux kernel is configured for UNKNOWN system. \n");
#endif

#if defined(BASE_DDR_ADDR)
	len += sprintf(page+len,"Memsize is %dMB. \n",BASE_DDR_ADDR/(1024*1024));

	len += sprintf(page+len,"FB0 buffer size is %dMB.\n",FB0_SIZE/(1024*1024));
	len += sprintf(page+len,"FB0 buffer start is 0x%x.\n",FB0_REGION);
	len += sprintf(page+len,"FB1 buffer size is %dMB. \n",FB1_SIZE/(1024*1024));
	len += sprintf(page+len,"FB1 buffer start is 0x%x.\n",FB1_REGION);

	len += sprintf(page+len,"FB2 buffer size is %dMB. \n",FB2_SIZE/(1024*1024));
	len += sprintf(page+len,"FB2 buffer start is 0x%x.\n",FB2_REGION);
	len += sprintf(page+len,"FB3 buffer size is %dMB. \n",FB3_SIZE/(1024*1024));
	len += sprintf(page+len,"FB3 buffer start is 0x%x.\n",FB3_REGION);

        len += sprintf(page+len,"Blob start address is 0x%x. \n",BLOB_REGION);
        len += sprintf(page+len,"Blob size is %dKB. \n",BLOB_SIZE/1024);

	len += sprintf(page+len,"ETHERNET buffer size is %dKB. \n",ETHERNET_SIZE/1024);
	len += sprintf(page+len,"ETHERNET is 0x%x. \n",ETHERNET_REGION);

	len += sprintf(page+len,"HD2SD data buffer size is %dKB. \n",HD2SD_DATA_SIZE/1024);
	len += sprintf(page+len,"HD2SD start is 0x%x. \n",HD2SD_DATA_REGION);

	len += sprintf(page+len,"VIDEO STUFF buffer size is %dKB. \n",VIDEO_STUFF_SIZE/1024);
	len += sprintf(page+len,"VIDEO STUFF start is 0x%x. \n",VIDEO_STUFF_REGION);

	len += sprintf(page+len,"VIDEO Number: %d. \n",VIDEO_NUM);
	len += sprintf(page+len,"DBP start address is 0x%x. \n",VIDEO_DPB_REGION);
	len += sprintf(page+len,"DBP size is %dMB. \n",VIDEO_DPB_SIZE/(1024*1024));
	len += sprintf(page+len,"CBP start address is 0x%x. \n",VIDEO_CPB_REGION);
	len += sprintf(page+len,"CPB size is %dMB. \n",VIDEO_CPB_SIZE/(1024*1024));
	len += sprintf(page+len,"CBP Dir start address is 0x%x. \n",VIDEO_CPB_DIR_REGION);
	len += sprintf(page+len,"CPB Dir size is %dKB. \n",VIDEO_CPB_DIR_SIZE/1024);
	len += sprintf(page+len,"VIDEO User Data start address is 0x%x. \n",VIDEO_USER_DATA_REGION);
	len += sprintf(page+len,"VIDEO User Data size is %dKB. \n",VIDEO_USER_DATA_SIZE/1024);

	len += sprintf(page+len,"AUDIO Number: %d. \n",AUDIO_NUM );
	len += sprintf(page+len,"CAB start address is 0x%x. \n",AUDIO_CAB_REGION);
	len += sprintf(page+len,"CAB size is %dKB. \n",AUDIO_CAB_SIZE/1024);
	len += sprintf(page+len,"PTS start address is 0x%x. \n",AUDIO_PTS_REGION);
	len += sprintf(page+len,"PTS size is %dKB. \n",AUDIO_PTS_SIZE/1024);
	len += sprintf(page+len,"MIX start address is 0x%x. \n",AUDIO_MIX_REGION);
	len += sprintf(page+len,"MIX size is %dKB. \n",AUDIO_MIX_SIZE/1024);

	len += sprintf(page+len,"Audio buffer start address is 0x%x. \n",AUDIO_STUFF_REGION);
	len += sprintf(page+len,"Audio buffer size is %dMB. \n",AUDIO_STUFF_SIZE/(1024*1024));

	len += sprintf(page+len,"Demux buffer start address is 0x%x. \n",XPORT_REGION);
	len += sprintf(page+len,"Demux buffer size is %dKB. \n",XPORT_SIZE/1024);

	len += sprintf(page+len,"System/Firmwares used memory size is %dMB. \n",(BASE_DDR_ADDR-XPORT_REGION)/(1024*1024));
#if (CONFIG_CELESTIAL_MEM_SIZE == 512)
	len += sprintf(page+len,"There is %dMB memory for kernel/user. \n",XPORT_REGION/(1024*1024)+256);
#elif (CONFIG_CELESTIAL_MEM_SIZE == 1024)
    len += sprintf(page+len,"There is %dMB memory for kernel/user. \n",XPORT_REGION/(1024*1024)+256+512);
#else
	len += sprintf(page+len,"There is %dMB memory for kernel/user. \n",XPORT_REGION/(1024*1024));
#endif
#else
	len += sprintf(page+len,"Cannot find Maxmium DDR Size. \n");
#endif

	return len;
}


static int gpio_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int gpioa,gpiob;
	int len=0;
	for (gpioa=0;gpioa<GPIO_NR_DEVS;gpioa++){
		if(gpioa < 32)
		{
			if ((gpio_status1>>gpioa) & 0x1)
			{
				if ((gpio_pinmux_status1>>gpioa) & 0x1)
					len += sprintf(page+len,"==> GPIOa %d is held by other component. \n",gpioa);
				else
					len += sprintf(page+len,"==> GPIOa %d is using. \n",gpioa);
			}
			else
			{
				len += sprintf(page+len,"==> GPIOa %d is free.\n",gpioa);
			}
		}
		else
		{
			gpiob = gpioa - 32;
			if ((gpio_status2>>gpiob) & 0x1)
			{
				if ((gpio_pinmux_status2>>gpiob) & 0x1)
					len += sprintf(page+len,"==> GPIOb %d(%d) is held by other component. \n",gpiob,gpioa);
				else
					len += sprintf(page+len,"==> GPIOb %d(%d) is using. \n",gpiob,gpioa);
			}
			else
			{
				len += sprintf(page+len,"==> GPIOb %d(%d) free.\n",gpiob,gpioa);
			}
		}
	}
	return len;
}

static int gpio_proc_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{
	unsigned char addr;
	unsigned char val;

	const char *cmd_line = buffer;;

	if (strncmp("wwgpio", cmd_line, 6) == 0) {
		addr = simple_strtol(&cmd_line[7], NULL, 16);
		val = simple_strtol(&cmd_line[9], NULL, 16);
		//printk("==> GPIO addr 0x%08x is set to 0x%04x \n",addr,val);
		//gpio_base[addr>>2] = val;
		gpio_hw_set_direct(addr, GPIO_WRITE);
		gpio_hw_write(addr,val);
	} else if (strncmp("rrgpio", cmd_line, 6) == 0) {
    	addr = simple_strtol(&cmd_line[7], NULL, 16);
        gpio_hw_set_direct(addr, GPIO_READ);
        val = gpio_hw_read(addr);
        printk("gpio %d = %d\n", addr, val);
	}

	return count;
}


static ssize_t
cnc_gpio_read(struct file *file, char __user * buf, size_t len, loff_t * ppos)
{
	unsigned m = iminor(file->f_dentry->d_inode);
	int value;
	value = gpio_hw_read(m);
	if (put_user(value ? '1' : '0', buf))
		return -EFAULT;

	return 1;
}

static int
cnc_gpio_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
    int ret;
    gpio_intr_param intr_param;
	struct gpio_cmd gpio_cmd;
	unsigned long direction;
	unsigned m = iminor(file->f_dentry->d_inode);

	switch (cmd) {
	case CMD_READ_GPIO:
	case CMD_WRITE_GPIO:
		if (copy_from_user
		    (&gpio_cmd, (void *) arg, sizeof (struct gpio_cmd)))
			return -EFAULT;

		switch (cmd) {
		case CMD_READ_GPIO:
			gpio_cmd.value = gpio_hw_read(gpio_cmd.gpio);
			if (copy_to_user
			    ((void *) arg, &gpio_cmd, sizeof (struct gpio_cmd)))
				return -EFAULT;
			else
				return 0;

		case CMD_WRITE_GPIO:
			gpio_hw_set_direct(gpio_cmd.gpio, GPIO_WRITE);
			gpio_hw_write(gpio_cmd.gpio, gpio_cmd.value);
			return 0;
		}
		break;
	case CMD_SET_DIRECTION:
		direction = arg;
		if (direction == GPIO_READ){
			gpio_hw_set_direct(m,GPIO_READ);
		}else if ( direction == GPIO_WRITE){
			gpio_hw_set_direct(m,GPIO_WRITE);
		}else {
			return -EINVAL;
		}
		break;

    case CMD_ENA_GPIO_INTR:
#ifdef CONFIG_GPIO_INTERRUPT_SUPPORT
        copy_from_user(&intr_param, (const void __user *)arg, sizeof(gpio_intr_param));
        if (intr_param.enable) {
            if (intr_param.irq_type == GPIO_IRQ_TYPE_LEVEL_HIGH) {
                if (gpio_devices[intr_param.gpio_index].irq_requested) {
                    if (gpio_devices[intr_param.gpio_index].cur_irq_type == GPIO_IRQ_TYPE_LEVEL_HIGH) {
                        break;
                    } else {
                        set_irq_type(gpio_to_irq(intr_param.gpio_index), IRQ_TYPE_LEVEL_HIGH);
                        gpio_devices[intr_param.gpio_index].cur_irq_type = GPIO_IRQ_TYPE_LEVEL_HIGH;
                    }
                } else {
                    gpio_devices[intr_param.gpio_index].cur_irq_type = GPIO_IRQ_TYPE_LEVEL_HIGH;
                    set_irq_type(gpio_to_irq(intr_param.gpio_index), IRQ_TYPE_LEVEL_HIGH);
                    ret = request_irq(gpio_to_irq(intr_param.gpio_index), gpio_interrupt_handler,
                                      IRQF_DISABLED | IRQF_TRIGGER_HIGH, "gpio_interrupt", NULL);
                    if (ret) {
                        printk("request gpio irq failed. gpio index : %d\n", intr_param.gpio_index);
                    }
                    gpio_devices[intr_param.gpio_index].irq_requested = 1;
                }
            } else if (intr_param.irq_type == GPIO_IRQ_TYPE_LEVEL_LOW) {
                if (gpio_devices[intr_param.gpio_index].irq_requested) {
                    if (gpio_devices[intr_param.gpio_index].cur_irq_type == GPIO_IRQ_TYPE_LEVEL_LOW) {
                        break;
                    } else {
                        set_irq_type(gpio_to_irq(intr_param.gpio_index), IRQ_TYPE_LEVEL_LOW);
                        gpio_devices[intr_param.gpio_index].cur_irq_type = GPIO_IRQ_TYPE_LEVEL_LOW;
                    }
                } else {
                    gpio_devices[intr_param.gpio_index].cur_irq_type = GPIO_IRQ_TYPE_LEVEL_LOW;
                    set_irq_type(gpio_to_irq(intr_param.gpio_index), IRQ_TYPE_LEVEL_LOW);
                    ret = request_irq(gpio_to_irq(intr_param.gpio_index), gpio_interrupt_handler,
                                      IRQF_DISABLED | IRQF_TRIGGER_LOW, "gpio_interrupt", NULL);
                    if (ret) {
                        printk("request gpio irq failed. gpio index : %d\n", intr_param.gpio_index);
                    }
                    gpio_devices[intr_param.gpio_index].irq_requested = 1;
                }
            }
        } else {
            if (gpio_devices[intr_param.gpio_index].irq_requested) {
                free_irq(gpio_to_irq(intr_param.gpio_index), NULL);
                gpio_devices[intr_param.gpio_index].irq_requested = 0;
            }
        }
#endif
        break;

	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned int cnc_gpio_poll(struct file *filp, poll_table *wait)
{
    unsigned m = iminor(filp->f_dentry->d_inode);

    poll_wait(filp, &gpio_devices[m].irq_wait_queue, wait);

    if (gpio_devices[m].report_event) {
        if (gpio_devices[m].cur_irq_type == gpio_devices[m].last_rcv_irq_type) {
            gpio_devices[m].report_event = 0;
            return POLLIN | POLLRDNORM;
        } else {
            gpio_devices[m].report_event = 0;
            return POLLOUT | POLLWRNORM;
        }
    }

    return 0;
}

#if defined(CONFIG_ARCH_CNC_DELAY)

#define GPIO_PINMUX() do {udelay(20);} while(0)

#else

#define GPIO_PINMUX() do {} while(0)

#endif

#define GPIO_REGINTR() do {} while(0)

static int __init
cnc_gpio_init(void)
{
	int result, i;
	dev_t dev = 0;
	int err=0;
	if (gpio_major) {
		dev = MKDEV(gpio_major, 0);
		result = register_chrdev_region(dev, gpio_nr_devs, "gpio");
	} else {
		result = alloc_chrdev_region(&dev, 0, gpio_nr_devs, "gpio");
		gpio_major = MAJOR(dev);
	}

	if (result < 0) {
		printk(KERN_WARNING "GPIO: can't get major %d\n", gpio_major);
		return result;
	}

    gpio_base =(volatile unsigned int *) VA_GPIO_BASE;

    if (!gpio_base) {
        unregister_chrdev_region(dev, gpio_nr_devs);
        printk(KERN_WARNING "GPIO: ioremap failed.\n");
        return -EIO;
    }

	cnc18xx_gpio_class = class_create(THIS_MODULE,"gpio");

    if (IS_ERR(cnc18xx_gpio_class)){
        err = PTR_ERR(cnc18xx_gpio_class);
		unregister_chrdev_region(dev, gpio_nr_devs);
        printk(KERN_WARNING "GPIO: class create failed.\n");
        return -EIO;
    }

	gpio_devices =
	    kzalloc(gpio_nr_devs * sizeof (struct gpio_devs), GFP_KERNEL);
	if (!gpio_devices) {
         unregister_chrdev_region(dev, gpio_nr_devs);
         printk(KERN_WARNING "GPIO cannot allocate device structure\n");
         return -ENOMEM;
	}
	//memset(gpio_devices, 0, gpio_nr_devs * sizeof (struct gpio_devs));

	/* Initialize each device. */
	for (i = 0; i < gpio_nr_devs; i++) {
		gpio_devices[i].gpio_id = i;
		gpio_setup_cdev(&gpio_devices[i], i);
	}

	spin_lock_init(&gpio_lock);

	printk(KERN_INFO "CNC GPIO at 0x%x, %d lines\n", VA_GPIO_BASE,
	       gpio_nr_devs);

	gpio_proc_entry = create_proc_entry("gpio", 0, NULL);
	if (NULL != gpio_proc_entry) {
		gpio_proc_entry->write_proc = &gpio_proc_write;
		gpio_proc_entry->read_proc = &gpio_proc_read;
	}

	system_config_proc_entry = create_proc_entry("systemconfig", 0, NULL);
	if (NULL != system_config_proc_entry) {
        system_config_proc_entry->read_proc = &system_config_proc_read;
	}

#ifdef CONFIG_GPIO_INTERRUPT_SUPPORT
    int gpio_irq;

    gpio_base[REG_GPIO_INTMASK_B >> 2] = 0xffffffff;
    gpio_base[REG_GPIO_INTMASK_A >> 2] = 0xffffffff;

    for (gpio_irq = gpio_to_irq(0);
          gpio_irq < gpio_to_irq(MAX_GPIO_NUMBER); ++gpio_irq) {
        set_irq_chip(gpio_irq, &cnc_gpio_irq_chip);
        set_irq_handler(gpio_irq, handle_level_irq);
        set_irq_flags(gpio_irq, IRQF_VALID | IRQF_DISABLED);
    }
    set_irq_chained_handler(CNC_GPIO_IRQ, cnc_gpio_interrupt);
#endif

	return 0;
}

static void __exit
cnc_gpio_exit(void)
{
	int i;
	dev_t devno = MKDEV(gpio_major, 0);

	if (gpio_devices) {
		for (i = 0; i < gpio_nr_devs; i++) {
			cdev_del(&gpio_devices[i].cdev);
		}
		kfree(gpio_devices);
		gpio_devices = NULL;
	}

	unregister_chrdev_region(devno, gpio_nr_devs);
}

// module_init(cnc_gpio_init);
// module_exit(cnc_gpio_exit);
postcore_initcall(cnc_gpio_init);
MODULE_AUTHOR("Cavium");
MODULE_DESCRIPTION("Cavium Celestial GPIO driver");
MODULE_LICENSE("GPL");
