/****************************************************************************
  * Copyright (C) 2008-2010 Celestial Semiconductor Inc.
  * All rights reserved
  *
  * [RELEASE HISTORY]                           
  * VERSION  DATE       AUTHOR                  DESCRIPTION
  * 0.1      10-03-30       Jia Ma           		Original
  ****************************************************************************
*/

#include <linux/device.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/cnc1800l.h>

#include "../xport/xport_drv1800.h"
#include "audio_reg_def.h"
#include "cnc1800_audio.h"

typedef enum _UART_ID_
{
	_uart_0 	=0,
	_uart_1		=1,
	_uart_2		=2,
	_uart_audio	=3,
	_uart_video	=4,
}UART_ID;

#ifdef CONFIG_CELESTIAL_AUDIO_DEBUG
#define DEBUG_PRINTF  printk
#else
#define DEBUG_PRINTF(fmt,args...)
#endif
extern int pinmux_enable_uart(UART_ID UART);
extern void clock_audio_setclock(AUDIO_CLOCK_MODE AudioClockMode);
extern void power_audio_on(POWER_MODE PowerMode);

static struct platform_device *audio_pdev;
static const struct firmware* audio_data_fw=(struct firmware*)NULL;
static const struct firmware* audio_text_fw=(struct firmware*)NULL;
static const struct firmware* audio_ext_fw=(struct firmware*)NULL;

volatile unsigned char *audmcu_base = NULL;
volatile unsigned char *cnc_audio_reg_base = NULL;

#define audio_writew(a,v)	writew(v,(cnc_audio_reg_base+((a)&0xfffff)))
#define audio_writel(a,v)	writel(v,(cnc_audio_reg_base+((a)&0xfffff)))
#define audio_readw(a)	readw(cnc_audio_reg_base+((a)&0xfffff))
#define audio_readl(a)	readl(cnc_audio_reg_base+((a)&0xfffff))

unsigned int irq_audio_samplerate_change = 0;
unsigned int irq_audio_silence_detection = 0;
AUDIO_CLOCK_MODE audio_clock_mode = _AUDIO_48KHz;

static int process_num = 0;
typedef struct CSM1800_AUDIO_DEV_t {
	int instance_num;
} csm1800_audio_dev;

DEFINE_SPINLOCK(cnc_audio_lock);
DECLARE_WAIT_QUEUE_HEAD(audio_notify_queue);
//static DECLARE_WAIT_QUEUE_HEAD(cnc_audio_queue);
//static DEFINE_SPINLOCK(cnc_audio_queue_lock);

static void __load_audio_firmware(volatile unsigned char *pDst, unsigned char *pSrc, int len_in_byte)
{
	unsigned long tmp_val;
	volatile unsigned long *tcm_addr = (volatile unsigned long*)pDst;
	int tcm_wr_size = len_in_byte>>2;
	
	while(tcm_wr_size-- > 0) {
		tmp_val  = (unsigned long)(*pSrc++);
		tmp_val |= (unsigned long)(*pSrc++) << 8;
		tmp_val |= (unsigned long)(*pSrc++) << 16;
		tmp_val |= (unsigned long)(*pSrc++) << 24;
			
		*tcm_addr++ = tmp_val;
	}
//	memcpy((unsigned char *)pDst, pSrc, len_in_byte);	
	return;
}

static void audio_load_firmware(void)
{

	/************* load 32K to ICACHE *************/
	__load_audio_firmware(cnc_audio_reg_base+(CSM1800_AUD_ICACHE_BASE&0xfffff), (unsigned char *)audio_text_fw->data, 0x8000);

	/************* load text to DDR ****************/
	__load_audio_firmware(audmcu_base+AUDIO_INS_OFFSET, (unsigned char *)audio_text_fw->data, audio_text_fw->size);

	/************* load data to DCACHE ************/
	__load_audio_firmware(cnc_audio_reg_base+(CSM1800_AUD_DSRAM_BASE&0xfffff) , (unsigned char *)audio_data_fw->data, audio_data_fw->size);

	/************* load ext data to DDR ************/
	__load_audio_firmware(audmcu_base + AUDIO_DATA_OFFSET, (unsigned char *)audio_ext_fw->data, audio_ext_fw->size);

	audio_writel(CSM1800_AUD_MAILBOX5, AUDIO_DATA_REGION);

	return;
}

void audio_fw_reset(void)
{
#if 1
	// Base on 1800L datasheet, I change the reset sequence below.
	audio_writel(CSM1800_RISC_IO_PROT, 0x0);
	while(audio_readl(CSM1800_RISC_IO_PROT) & 0x100);
	audio_writel(CSM1800_AUDIO_RISC_CTRL, 1<<25);
	audio_load_firmware();
	audio_writel(CSM1800_RISC_IO_PROT, 1);
#else
	/************** risc stop **************/
	/* Due to that only two way mips access ddr, instruction cache and dma, which can't be stop by audio risc_ctrl*/
	audio_writel(CSM1800_AUDIO_RISC_CTRL, 1<<25);
	/* wait until cache and dma no action */
	while(audio_readl(CSM1800_RISC_IO_PROT) & 0x100);
	audio_writel(CSM1800_RISC_IO_PROT, 0x0);

	/* place those two line here to wait the suspending caching finish, if not, it may damage firmware. */
	/* because we "stop mips then wait io" in audio_mips_stop, so, this two line omitted */
	//write_ahb32(CSM1300_RISC_IO_PROT, 1); // enable IO
	//while(read_ahb32(CSM1300_RISC_IO_PROT) & 0x100);// Is it busy?
//	printk(KERN_INFO "%s : %s in %d\n",__FILE__,__FUNCTION__,__LINE__);
	audio_load_firmware();
//	printk(KERN_INFO "%s : %s in %d\n",__FILE__,__FUNCTION__,__LINE__);
	/*************** risc start **************/
	audio_writel(CSM1800_RISC_IO_PROT, 1); // enable IO
	//audio_writel(CSM1800_AUDIO_RISC_CTRL, 0 | (AUDIO_INS_REGION>>3)); // start
#endif
	return;
}

unsigned int audio_irq_type = 0;
#define AUDIO_IRQ_TYPE_SAMPLERATE_CHANGE 0x1
#define AUDIO_IRQ_TYPE_SILENCE_DETECTION 0x2

static int cnc_audio_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
	unsigned int minor_id = iminor(inode);
	unsigned int reg_val = 0;
	unsigned long flags;
	unsigned int set_dac_val,i2s_ctl_val;

	reg_val = reg_val;
	minor_id = minor_id;

	switch (cmd) {
		case CSAUD_IOC_GET_AUDIO_REG_ADDR:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(CSM_AUDIO_REG_BASE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_REG_SIZE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(CSM_AUDIO_REG_SIZE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_CAB_ADDR:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_CAB_REGION, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case  CSAUD_IOC_GET_AUDIO_CAB_SIZE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_CAB_SIZE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_HOST_IF_ADDR:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_HOST_FW_IF_REGION, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_HOST_IF_SIZE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_HOST_FW_IF_SIZE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_MIXER_ADDR:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_MIX_REGION, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_MIXER_IF_SIZE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_MIX_SIZE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_INS_ADDR	:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_INS_REGION, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;

		case CSAUD_IOC_SET_SAMPLE_RATE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			clock_audio_setclock(arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		
		case CSAUD_IOC_GET_AUDIO_PTS_ADDR:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_PTS_REGION, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_GET_AUDIO_PTS_SIZE:
 			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_PTS_SIZE, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;

		case CSAUD_IOC_GET_XPORT_AUDIO:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user((MIPS_EXTERNAL_BASE_ADDR+512), (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;

		case CSAUD_IOC_GET_IRQ_TYPE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			printk("CSAUD_IOC_GET_IRQ_TYPE : audio_irq_type = %d\n",audio_irq_type);
			__put_user(audio_irq_type, (unsigned int __user *) arg);
			audio_irq_type = 0;
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;

		case CSAUD_IOC_GET_SAMPLE_RATE:
			spin_lock_irqsave(&cnc_audio_lock, flags);
			printk("CSAUD_IOC_GET_SAMPLE_RATE : audio_clock_mode = %d\n",audio_clock_mode);
			__put_user(audio_clock_mode, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;

		case CSAUD_IOC_GET_AUDIO_NUM:
 			spin_lock_irqsave(&cnc_audio_lock, flags);
			__put_user(AUDIO_NUM, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_ENA_AUDIO_DAC:
 			spin_lock_irqsave(&cnc_audio_lock, flags);
			get_user(set_dac_val ,(unsigned int __user *) arg );
			power_audio_on(1);
			if(AUD_DAC_TYPE_RIGHT == set_dac_val)
				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val | (1<<30));
				}
			else if (AUD_DAC_TYPE_LEFT == set_dac_val)
				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val | (1<<29));
				}
            else 
				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val | (3<<29) );

				}
			
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		case CSAUD_IOC_DIS_AUDIO_DAC:
 			spin_lock_irqsave(&cnc_audio_lock, flags);
			get_user(set_dac_val ,(unsigned int __user *) arg );
			if(AUD_DAC_TYPE_RIGHT == set_dac_val)
				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val & (~(1<<30)));
				}
			else if (AUD_DAC_TYPE_LEFT == set_dac_val)

				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val & (~(1<<29)));
				}
            else 
				{
					i2s_ctl_val = audio_readl(CSM1800_AUD_I2S_CNTL);
					audio_writel(CSM1800_AUD_I2S_CNTL, i2s_ctl_val & (~(3<<29)) );

				}

			if(!(audio_readl(CSM1800_AUD_I2S_CNTL)&(1<<29)) && !(audio_readl(CSM1800_AUD_I2S_CNTL)&(1<<30)))
				power_audio_on(0);
			
			spin_unlock_irqrestore(&cnc_audio_lock, flags);
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int cnc_audio_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		int addr;
		addr = simple_strtol(&buffer[3], NULL, 16);
		printk(" addr[0x%08x] = 0x%08x \n", addr, audio_readl(addr));
	}
	else if(strncmp("print", cmd_line, 5) == 0){
		//pinmux_enable_uart(_uart_audio);		
	}

	return count;
}

static inline void __kernel_load_audio_firmware(void)
{
	int ret;
	if(audio_text_fw == NULL){
		ret = request_firmware(&audio_text_fw, "audio_fw_text.bin", &(audio_pdev->dev));
		if (unlikely(ret != 0 || audio_text_fw== NULL)) {
			printk(KERN_ERR "Failed to load video firmware code section 1\n");
			return;
		}
		else{
			if (unlikely(audio_text_fw->size > AUDIO_INS_SIZE)){
					release_firmware(audio_text_fw);
					audio_text_fw=NULL;
					printk(KERN_ERR "Failed to load audio firmware code section 2\n");
					return;
			}
			if(audio_text_fw!=NULL){
				ret = request_firmware(&audio_data_fw, "audio_fw_data.bin", &(audio_pdev->dev));
				if (unlikely(ret != 0 || audio_data_fw== NULL)) {
					release_firmware(audio_text_fw);
					audio_text_fw=NULL;
					printk(KERN_ERR "Failed to load audio firmware data section 1\n");
					return;
				}
				else{
					if (unlikely(audio_data_fw->size > AUDIO_DATA_SIZE)){
				    		release_firmware(audio_data_fw);
							audio_data_fw=NULL;
							release_firmware(audio_text_fw);
							audio_text_fw=NULL;
							printk(KERN_ERR "Failed to load audio firmware data section 2\n");
							return;
					}
				}
			}
		}
		
		ret = request_firmware(&audio_ext_fw, "audio_fw_ext_data.bin", &(audio_pdev->dev));
		if (unlikely(ret != 0 || audio_ext_fw== NULL)) {
			printk(KERN_ERR "Failed to load audio external firmware 1\n");
			return;
		}
		else{
			if (unlikely(audio_ext_fw->size > AUDIO_DATA_SIZE)){
					release_firmware(audio_ext_fw);
					audio_ext_fw=NULL;
					printk(KERN_ERR "Failed to load audio external firmware 2\n");
					return;
			}
		}
	}
//	printk(KERN_INFO "%s : %s in %d\n",__FILE__,__FUNCTION__,__LINE__);
	return;
}

static int cnc_audio_open(struct inode *inode, struct file *file)
{
	csm1800_audio_dev *audio_dev = NULL;
	
	DEBUG_PRINTF("CSM AUDIO prepare open\n");
	if(NULL == file->private_data){
		printk("func:%s  line:%d \n",__FUNCTION__,__LINE__);
		audio_dev = kmalloc(sizeof(csm1800_audio_dev), GFP_KERNEL);
		if(NULL == audio_dev){
			printk("func:%s  line:%d \n",__FUNCTION__,__LINE__);
			return -EBUSY;
		}
		audio_dev->instance_num = 1;
		file->private_data = (void *)audio_dev;
		if(process_num > 0){
			process_num ++;
			goto CSM_AUDIO_OPEN_SUCCEED;
		}
		else{
			process_num ++;
		}
	}
	else{
		audio_dev = file->private_data;
		audio_dev->instance_num ++;
		printk("func:%s  line:%d instance_num = %d\n",__FUNCTION__,__LINE__,audio_dev->instance_num);
		goto CSM_AUDIO_OPEN_SUCCEED;
	}

	printk(KERN_INFO "CNC AUDIO has been opened\n");

	__kernel_load_audio_firmware();
	audio_fw_reset();
	if(!(audio_readl(CSM1800_AUD_I2S_CNTL)&(1<<29)) && !(audio_readl(CSM1800_AUD_I2S_CNTL)&(1<<30)))
	{
			power_audio_on(0);
	}

	CSM_AUDIO_OPEN_SUCCEED:
	return 0;
}

typedef enum _BIT_CMD_STATE
{
	DRV_NO_CMD = 0,
	DRV_NEW_CMD = 1,
}eBIT_CMD_STATE;

#define INIT_TRIGGER_BIT        0x1
#define INIT_TYPE_MASK_OFFSET   0x0
#define INIT_TYPE_MASK_WIDTH    0x5
#define INIT_TYPE_MASK          (((1<<INIT_TYPE_MASK_WIDTH)-1) << INIT_TYPE_MASK_OFFSET)

#define MUTE_TRIGGER_BIT	    0x2
#define MUTE_MASK_OFFSET	    (INIT_TYPE_MASK_OFFSET + INIT_TYPE_MASK_WIDTH)
#define MUTE_MASK_WIDTH         0x1
#define MUTE_MASK			    (((1<<MUTE_MASK_WIDTH)-1) << MUTE_MASK_OFFSET)

#define VOLUME_TRIGGER_BIT	    0x4
#define VOLUME_MASK_OFFSET	    (MUTE_MASK_OFFSET + MUTE_MASK_WIDTH)
#define VOLUME_MASK_WIDTH       0x7
#define VOLUME_MASK			    (((1<<VOLUME_MASK_WIDTH)-1) << VOLUME_MASK_OFFSET)

#define RUN_STATE_TRIGGER_BIT   0x8
#define RUN_STATE_MASK_OFFSET   (VOLUME_MASK_OFFSET + VOLUME_MASK_WIDTH)
#define RUN_STATE_MASK_WIDTH    0x2
#define RUN_STATE_MASK          (((1<<RUN_STATE_MASK_WIDTH)-1) << RUN_STATE_MASK_OFFSET)

typedef enum{
	CMD_STOP = 0,   // set by playmode trigger.
	CMD_PLAY = 1,   // set by playmode trigger.
	CMD_PAUSE = 2,  // set by playmode trigger.
	CMD_RESUME = 3, // set by playmode trigger.

	CMD_INIT = 4,   // got by init trigger.
	CMD_IDLE = 5,   // set by audio firmware.
}eDRIVER_CODER_STATE;

/* 
    register driver bit position
*/
#define TOP_INFO_WIDTH 4
#define TOP_INFO_MASK 0xf

/* mailbox2 */
#define TOP_INUSE_BIT_OFFSET    0
//#define TOP_HOST_LOCK_BIT_OFFSET     1
#define TOP_NEWCMD_BIT_OFFSET   2

unsigned int *audio_handshake_addr;
static int try_lock_host_driver(int channel, int wait_times)
{
	unsigned int holock = audio_readl(CSM1800_AUD_MAILBOX0);
	unsigned int mask = 1 << channel;;

	/* wait fw until unlock */
	while(audio_readl(CSM1800_AUD_MAILBOX1) & (1 << channel)){
		wait_times--;
		udelay(500);
		if (wait_times <= 0){
			return -1;
		}
	}

	/* lock host driver */
	audio_writel(CSM1800_AUD_MAILBOX0, holock | mask);

	/* wait fw lock until unlock */
	if (audio_readl(CSM1800_AUD_MAILBOX1) & (1 << channel)){
		/* release host driver */
		holock = audio_readl(CSM1800_AUD_MAILBOX0);
		mask = 1 << channel;
		mask = ~mask;
		audio_writel(CSM1800_AUD_MAILBOX0, holock & mask);
		return -1;
	}

	return 0;
}

static void get_driver_cmd(int channel, unsigned int *trigger, unsigned int *status,unsigned int which_status)
{	
	if (-1 == try_lock_host_driver(channel, 1000)){
		printk("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);
		udelay(10);
	}

	*trigger = *(audio_handshake_addr + channel*4);
	*status = *(audio_handshake_addr + channel*4 + which_status);

	return;
}

static void send_driver_cmd(int channel, unsigned int trigger, unsigned int status, unsigned int which_status)
{
	unsigned int holock = audio_readl(CSM1800_AUD_MAILBOX0);
	unsigned int mask = 1 << channel;
	unsigned int top_cmd;

	/* write host to fw driver information*/
	*(audio_handshake_addr + channel*4) = trigger;
	*(audio_handshake_addr + channel*4 + which_status) = status;

	/* write new command */
	top_cmd = audio_readl(CSM1800_AUD_MAILBOX2);
	top_cmd = top_cmd | (DRV_NEW_CMD<<(TOP_NEWCMD_BIT_OFFSET+channel*TOP_INFO_WIDTH));
	audio_writel(CSM1800_AUD_MAILBOX2, top_cmd);

	/* release host driver */
	holock = audio_readl(CSM1800_AUD_MAILBOX0);
	mask = 1 << channel;
	mask = ~mask;
	audio_writel(CSM1800_AUD_MAILBOX0, holock & mask);

	return;
}

static void cnc_audio_send_stop_cmd(int channel)
{
	unsigned int trigger, status, counter = 0;

	printk(KERN_DEBUG "1trigger = 0x%x, status= 0x%x\n",*(audio_handshake_addr + channel*4), *(audio_handshake_addr + channel*4+1));
	get_driver_cmd(channel, &trigger, &status, 1);
	printk(KERN_DEBUG "2trigger = 0x%x, status= 0x%x\n",*(audio_handshake_addr + channel*4), *(audio_handshake_addr + channel*4+1));
	
	trigger = trigger | RUN_STATE_TRIGGER_BIT;
	status = (status & (~RUN_STATE_MASK)) | (CMD_STOP<< RUN_STATE_MASK_OFFSET);

	send_driver_cmd(channel, trigger, status, 1);
	printk(KERN_DEBUG "3trigger = 0x%x, status= 0x%x\n",*(audio_handshake_addr + channel*4), *(audio_handshake_addr + channel*4+1));
	
	while((*(audio_handshake_addr + channel*4))&RUN_STATE_TRIGGER_BIT){
		if(counter > 200){
			printk(KERN_DEBUG "4trigger = 0x%x, status= 0x%x\n",*(audio_handshake_addr + channel*4), *(audio_handshake_addr + channel*4+1));
	
			printk("%s : %s in line %d\n",__FILE__,__FUNCTION__,__LINE__);
			break;
		}
		counter ++;
		udelay(500);
	}

	return;
}

static int cnc_audio_release(struct inode *inode, struct file *file)
{
	csm1800_audio_dev *audio_dev = NULL;
	audio_handshake_addr = (unsigned int *)(audmcu_base + AUDIO_HOST_FW_IF_OFFSET);

	audio_dev = file->private_data;
	audio_dev->instance_num--;

	if(audio_dev->instance_num > 0){
		printk("func:%s  line:%d instance_num = %d, process_num = %d\n",__FUNCTION__,__LINE__,audio_dev->instance_num, process_num);
		return 0;
	}
	else{
		printk("func:%s  line:%d instance_num = %d, process_num = %d\n",__FUNCTION__,__LINE__,audio_dev->instance_num, process_num);
		kfree(file->private_data);
		process_num --;
		if(process_num <= 0){
			cnc_audio_send_stop_cmd(0);
			cnc_audio_send_stop_cmd(1);

			audio_writel(CSM1800_AUDIO_RISC_CTRL, 1<<25);
			release_firmware(audio_text_fw);
			audio_text_fw = NULL;
			release_firmware(audio_data_fw);
			audio_data_fw = NULL;
			release_firmware(audio_ext_fw);
			audio_ext_fw = NULL;
		}
	}
	printk(KERN_INFO "CNC AUDIO has been released\n");

	return 0;
}

static unsigned int cnc_audio_poll(struct file *filp, poll_table * wait)
{
	unsigned int mask = 0;

	if(irq_audio_samplerate_change ||irq_audio_silence_detection){
		mask = POLLIN | POLLWRNORM;
		if(irq_audio_samplerate_change)
			irq_audio_samplerate_change = 0;
		if(irq_audio_silence_detection)
			irq_audio_silence_detection = 0;
		return mask;
	}
	poll_wait(filp, &audio_notify_queue, wait);

	return mask;
}

static struct class *cnc_audio_class;
static struct proc_dir_entry *cnc_audio_proc_entry = NULL;
static struct file_operations cnc_audio_fops = {
	.owner	= THIS_MODULE,
	.open	=  cnc_audio_open,
	.release	= cnc_audio_release,
	.ioctl		= cnc_audio_ioctl,
	.poll 	= cnc_audio_poll,
};
static struct miscdevice csdrv_audio_miscdev = {
	        MISC_DYNAMIC_MINOR,
	        "cnc_audio",
	        &cnc_audio_fops
};

#if 1
#define AUD_SAMPLE_RATE_INT     		0xf0000002
#define AUD_SILENCE_DETECTION_INT     		0xf0000003
irqreturn_t cnc_audio_irq(int irq, void *dev_id)
{
	unsigned int flags = 0;
	unsigned long flags1;
	unsigned int irq_reg;

	irq_reg = audio_readl(CSM1800_AUD_MAILBOX4);
	printk(KERN_INFO "cnc_audio_irq : interrupt:  0x%x\n", irq_reg);

	if (AUD_SAMPLE_RATE_INT == irq_reg) {
		flags = audio_readl(CSM1800_AUD_MAILBOX5);
		printk(KERN_INFO "AUD_SAMPLE_RATE_INT:  0x%x\n", flags);
		switch(flags){
			case 192000:
				audio_clock_mode = _AUDIO_192KHz;
				break;
			case 96000:
				audio_clock_mode = _AUDIO_96KHz;
				break;
			case 88200:
				audio_clock_mode = _AUDIO_88_2KHz;
				break;
			case 64000:
				audio_clock_mode = _AUDIO_64KHz;
				break;
			case 48000:
				audio_clock_mode = _AUDIO_48KHz;
				break;
			case 44100:
				audio_clock_mode = _AUDIO_44_1KHz;
				break;
			case 32000:
				audio_clock_mode = _AUDIO_32KHz;
				break;
			case 24000:
				audio_clock_mode = _AUDIO_24KHz;
				break;
			case 22050:
				audio_clock_mode = _AUDIO_22_05KHz;
				break;
			case 16000:
				audio_clock_mode = _AUDIO_16KHz;
				break;
			case 12000:
				audio_clock_mode = _AUDIO_12KHz;
				break;
			case 11025:
				audio_clock_mode = _AUDIO_11_025KHz;
				break;
			case 8000:
				audio_clock_mode = _AUDIO_8KHz;
				break;
			default:
				audio_clock_mode = _AUDIO_48KHz;
				break;
		}
		clock_audio_setclock(audio_clock_mode);

		spin_lock_irqsave(&cnc_audio_lock, flags1);
		irq_audio_samplerate_change = 1;
		audio_irq_type|=AUDIO_IRQ_TYPE_SAMPLERATE_CHANGE;
		wake_up(&audio_notify_queue);
		spin_unlock_irqrestore(&cnc_audio_lock, flags1);
	}
	else if (AUD_SILENCE_DETECTION_INT & irq_reg){
		printk(KERN_INFO "AUD_SILENCE_DETECTION_INT:  0x%x\n", flags);
		spin_lock_irqsave(&cnc_audio_lock, flags1);
		irq_audio_silence_detection = 1;
		audio_irq_type|=AUDIO_IRQ_TYPE_SILENCE_DETECTION;
		wake_up(&audio_notify_queue);
		spin_unlock_irqrestore(&cnc_audio_lock, flags1);
	}
	else{
		printk("interrupt: Unknow type 0x%x\n", irq_reg);
		irq_reg = audio_readl(CSM1800_AUD_MAILBOX5);
		printk("interrupt:  0x%x\n", irq_reg);
	}

	irq_reg = audio_readl(CSM1800_AUD_INT);
	audio_writel(CSM1800_AUD_INT, irq_reg & 0xfffffffe);

	return IRQ_HANDLED;
}
#endif

void __audio_mem_initialize(void)
{
	if (!request_mem_region(CSM_AUDIO_REG_BASE, CSM_AUDIO_REG_SIZE, "CNC Audio"))
		goto ERR_MEM1;

	if (NULL == (cnc_audio_reg_base = ioremap(CSM_AUDIO_REG_BASE, CSM_AUDIO_REG_SIZE)))
		goto ERR_MEM2;
	
	if (!request_mem_region(AUDIO_STUFF_REGION, AUDIO_STUFF_SIZE, "MCU Audio"))
		goto ERR_MEM3;

	if (NULL == (audmcu_base = ioremap(AUDIO_STUFF_REGION, AUDIO_STUFF_SIZE)))
		goto ERR_MEM4;
	return;

	ERR_MEM4:
	printk("%s: %s ERR_MEM4\n",__FILE__,__FUNCTION__);
	release_mem_region(AUDIO_STUFF_REGION, AUDIO_STUFF_SIZE);
	ERR_MEM3:
	printk("%s: %s ERR_MEM3\n",__FILE__,__FUNCTION__);
	iounmap((void *) cnc_audio_reg_base);
	ERR_MEM2:
	printk("%s: %s ERR_MEM2\n",__FILE__,__FUNCTION__);
	release_mem_region(CSM_AUDIO_REG_BASE, CSM_AUDIO_REG_SIZE);
	ERR_MEM1:
	printk("%s: %s ERR_MEM1\n",__FILE__,__FUNCTION__);
	return;
}

void __audio_mem_destroy(void)
{
	iounmap((void *)cnc_audio_reg_base);
	release_mem_region(CSM_AUDIO_REG_BASE, CSM_AUDIO_REG_SIZE);
	iounmap((void *)audmcu_base);
	release_mem_region(AUDIO_STUFF_REGION, AUDIO_STUFF_SIZE);
}

static int __init cnc_audio_init(void)
{
	int ret = 0;

	ret = misc_register(&csdrv_audio_miscdev);
	if(ret){
		/* return if failure ... */
		printk(KERN_INFO "Failed to register the cnc_audio misc device\n");
		return ret;
	}

	audio_pdev = platform_device_register_simple("audio_device", 0, NULL, 0);
    	if (IS_ERR(audio_pdev)) {
        	return -ENODEV;
    	}

	cnc_audio_class = class_create(THIS_MODULE,"cnc_audio");
	if (IS_ERR(cnc_audio_class)){
		printk(KERN_ERR "AUDIO: class create failed.\n");
		__audio_mem_destroy();
		cnc_audio_class = NULL;
		return -ENODEV;
	}

	cnc_audio_proc_entry = create_proc_entry("audio_io", 0, NULL);
	if (NULL != cnc_audio_proc_entry) {
		cnc_audio_proc_entry->write_proc = &cnc_audio_proc_write;
	}

	__audio_mem_initialize();
#if 1
	if (request_irq(21, cnc_audio_irq, 0, "cs_audio", NULL)) {
		printk(KERN_ERR "csdrv_aud: cannot register IRQ \n");
		return -EIO;
	}
#endif

	printk("CNC1800 Audio System Version : 0.2\n");

	printk(KERN_INFO "%s: CNC Audio driver was initialized, at address@[phyical addr = %08x, size = %x] \n", 
		"cnc1800_audio", CSM_AUDIO_REG_BASE, CSM_AUDIO_REG_SIZE);
	printk(KERN_INFO "%s: CNC Audio stuff at address@[phyical addr = %08x, size = %x] \n", 
		"cnc1800_audio", AUDIO_STUFF_REGION, AUDIO_STUFF_SIZE);

	return 0;
}

static void __exit cnc_audio_exit(void)
{
	__audio_mem_destroy();

	if (NULL != cnc_audio_proc_entry)
		remove_proc_entry("audio_io", NULL);

	class_destroy(cnc_audio_class);
	unregister_chrdev(AUDIO_MAJOR, "cnc_audio");

	printk(KERN_INFO " CNC AUDIO Exit ...... OK. \n");

	return;
}

module_init(cnc_audio_init);
module_exit(cnc_audio_exit);
MODULE_AUTHOR("Jia Ma, <jia.ma@celestialsemi.com>");
MODULE_DESCRIPTION("Celestial Semiconductor Audio sub-system");
MODULE_LICENSE("GPL");
