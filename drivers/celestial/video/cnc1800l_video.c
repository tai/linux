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

#include <mach/cnc1800l_power_clk.h>
#include "../xport/xport_drv1800.h"
#include "video_reg_def_cnc1800l.h"
#include "cnc1800l_video.h"

#ifdef CONFIG_CELESTIAL_VIDEO_DEBUG
#define DEBUG_PRINTF  printk
#else
#define DEBUG_PRINTF(fmt,args...)
#endif

//#define DRV_VIDEO_DEBUG

union video_flags {
	struct {
		unsigned int stillpicture_flag:1;
		unsigned int timecode_irq:1;
		unsigned int timecode_status:1;
		unsigned int decodeerror_irq:1;
		unsigned int decodeerror_status:1;
		unsigned int aspectratio_irq:1;
		unsigned int aspectratio_status:1;
		unsigned int pscancrop_irq:1;
		unsigned int pscancrop_status:1;
		unsigned int sync_irq:1;
		unsigned int sync_status:1;
		unsigned int sync_irq2:1;
		unsigned int underflow_report_irq:1;
		unsigned int underflow_report_status:1;
		unsigned int formatchange_report_irq:1;
		unsigned int formatchange_report_status:1;
		unsigned int srcformatchange_report_irq:1;
		unsigned int srcformatchange_report_status:1;
		unsigned int decode_finished_irq:1;
		unsigned int decode_finished_status:1;
		unsigned int reserved:12;
	} bits;

	unsigned int val;
};
static union video_flags vid_flags;

typedef enum _UART_ID_
{
	_uart_0 	=0,
	_uart_1		=1,
	_uart_2		=2,
	_uart_audio	=3,
	_uart_video	=4,
}UART_ID;
extern int pinmux_enable_uart(UART_ID UART);

extern void clock_coda_reset(CLOCK_RESET ResetOrSet);

typedef enum _I2C_ID_
{
	_i2c_0      =0,
	_i2c_1      =1,
}I2C_ID;
extern int pinmux_enable_i2c(I2C_ID I2C);
extern int pinmux_enable_vib(void);

static volatile unsigned int *vidmcu_base = NULL;
static volatile unsigned int *csm_video_base = NULL;
static volatile unsigned int *vidint_base = NULL;

#define video_write(v,a)    	do { *(csm_video_base + (a)) = (v) ;udelay(10);}while(0)
#define video_read(a)       	*(csm_video_base + (a))

static struct platform_device *video_pdev;
static const struct firmware* video_data_fw=(struct firmware*)NULL;
static const struct firmware* video_text_fw=(struct firmware*)NULL;
static const struct firmware* video_coda_fw=(struct firmware*)NULL;

static int process_num = 0;
typedef struct CSM1800_VIDEO_DEV_t {
	int instance_num;
} csm1800_video_dev;

DEFINE_SPINLOCK(csm_video_lock);
DEFINE_SPINLOCK(notify_lock);
DECLARE_WAIT_QUEUE_HEAD(notify_queue);

_HOST_FW_IF_t *host2videofw_if = NULL;
EXPORT_SYMBOL(host2videofw_if);

static int __csm_video_release_firmware(void);
static int __video_init(int load_coda_flag);
static inline void __kernel_load_firmware(int load_coda_flag);

unsigned int  __video_write(unsigned int val, unsigned int addr)
{
	video_write(val,addr);
	return video_read(addr);
}

unsigned int __video_read(unsigned int addr)
{
	return video_read(addr);
}

EXPORT_SYMBOL(__video_write);
EXPORT_SYMBOL(__video_read);

static int fix_bug_av_sync = 0;

static int csm_video_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
	unsigned long flags;

	switch (cmd){
		case CSVID_IOC_GET_VIDEO_NUM:
			__put_user(VIDEO_NUM, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_CPB_ADDR:
			__put_user(VIDEO_CPB_REGION, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_CPB_SIZE:
			__put_user(VIDEO_CPB_SIZE, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_CPB_DIR_ADDR:
			__put_user(VIDEO_CPB_DIR_REGION, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_CPB_DIR_SIZE:
			__put_user(VIDEO_CPB_DIR_SIZE, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_HOST_IF_ADDR:
			__put_user(VIDEO_HOST_IF_REGION, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_HOST_IF_SIZE:
			__put_user(VIDEO_HOST_IF_SIZE, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_REG_ADDR:
			__put_user(CSM_VIDEO_REG_BASE, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_REG_SIZE:
			__put_user(CSM_VIDEO_REG_SIZE, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_XPORT_VIDEO:
			__put_user(MIPS_EXTERNAL_BASE_ADDR, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_UD_ADDR:
			__put_user(VIDEO_USER_DATA_REGION, (unsigned int __user *) arg);
			break;
		case CSVID_IOC_GET_VIDEO_UD_SIZE:
			__put_user(VIDEO_USER_DATA_SIZE, (unsigned int __user *) arg);
			break;

		case CSVID_IOC_SET_VIDEO_START:
//			printk("mips start\n");
			video_write(VIDEO_MCU_REGION >> 3, MIPS_RESET_OFFSET);
			break;

		case CSVID_IOC_GET_NOTIFY_TYPE:
		{
			#ifdef DRV_VIDEO_TESTCTL
			printk("\n--CSVID_IOC_GET_NOTIFY_TYPE--\n");
			#endif
			int tempval = 0;
                     spin_lock_irqsave(&notify_lock, flags);
			if (vid_flags.bits.timecode_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_TIMECODE;
				vid_flags.bits.timecode_status = 0;
                            vid_flags.bits.timecode_irq = 0;
			}
			if (vid_flags.bits.decodeerror_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_DECODERERROR;
				vid_flags.bits.decodeerror_status = 1;
                            vid_flags.bits.decodeerror_irq = 0;
			}
                     if (vid_flags.bits.aspectratio_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_ASPECTRATIO;
				vid_flags.bits.aspectratio_status = 1;
                            vid_flags.bits.aspectratio_irq = 0;
			}
                     if (vid_flags.bits.pscancrop_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_PANSCANCROP;
				vid_flags.bits.pscancrop_status = 1;
                            vid_flags.bits.pscancrop_irq = 0;
			}
			if ((vid_flags.bits.sync_irq2 == 1)||(fix_bug_av_sync)) {
				tempval |= VIDEO_NOTIFY_TYPE_SYNC1;
				vid_flags.bits.sync_status = 1;
                            vid_flags.bits.sync_irq2 = 0;
				if(fix_bug_av_sync)
					fix_bug_av_sync = 0;
			}
			if(vid_flags.bits.sync_irq == 1){
				tempval |= VIDEO_NOTIFY_TYPE_SYNC2;
				vid_flags.bits.sync_status = 1;
                            vid_flags.bits.sync_irq = 0;
			}
			if (vid_flags.bits.underflow_report_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_UNDERFLOW;
				vid_flags.bits.underflow_report_status = 1;
				vid_flags.bits.underflow_report_irq = 0;
			}

			if (vid_flags.bits.formatchange_report_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_FORMATCHANGE;
				vid_flags.bits.formatchange_report_status = 1;
				vid_flags.bits.formatchange_report_irq = 0;
			}

			if (vid_flags.bits.srcformatchange_report_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_SRC_FORMATCHANGE;
				vid_flags.bits.srcformatchange_report_status = 1;
				vid_flags.bits.srcformatchange_report_irq = 0;
			}

			if (vid_flags.bits.decode_finished_irq) {
				tempval |= VIDEO_NOTIFY_TYPE_DECODE_FINISHED;
				vid_flags.bits.decode_finished_status = 1;
				vid_flags.bits.decode_finished_irq = 0;
			}

			__put_user(tempval, (unsigned int __user *) arg);
			spin_unlock_irqrestore(&notify_lock, flags);
			break;
		}

		case CSVID_IOC_SET_NOTIFY_TYPE:
		{
			int type_val = 0;
			int enable_bit = 0;
#ifdef DRV_VIDEO_DEBUG
			printk("CSVID_IOC_SET_NOTIFY_TYPE!\n");
#endif		
			spin_lock_irqsave(&notify_lock, flags);

			type_val = arg&(~VIDEO_NOTIFY_TYPE_ENABLE);
			enable_bit = arg>>31;
#ifdef DRV_VIDEO_DEBUG
			printk("type_val = 0x%x, enable_bit = %d\n",type_val,enable_bit);
#endif
			switch(type_val){
				case VIDEO_NOTIFY_TYPE_TIMECODE:
					vid_flags.bits.timecode_status = enable_bit;
        				break;

				case VIDEO_NOTIFY_TYPE_DECODERERROR:
					vid_flags.bits.decodeerror_status = enable_bit;
					break;

				case VIDEO_NOTIFY_TYPE_ASPECTRATIO:
					vid_flags.bits.aspectratio_status = enable_bit;
					vid_flags.bits.aspectratio_irq = 0;
			//		printk();
					break;

				case VIDEO_NOTIFY_TYPE_PANSCANCROP:
					vid_flags.bits.pscancrop_status = enable_bit;
					vid_flags.bits.pscancrop_irq = 0;
					break;

				case VIDEO_NOTIFY_TYPE_SYNC1:
					vid_flags.bits.sync_status = enable_bit;
					vid_flags.bits.sync_irq = 0;
		  			vid_flags.bits.sync_irq2 = 0;
					break;

				case VIDEO_NOTIFY_TYPE_SYNC2:
					break;

				case VIDEO_NOTIFY_TYPE_UNDERFLOW:
					vid_flags.bits.underflow_report_status = enable_bit;
					break;

				case VIDEO_NOTIFY_TYPE_FORMATCHANGE:
					vid_flags.bits.formatchange_report_status = enable_bit;
					vid_flags.bits.formatchange_report_irq = 0;
					break;

				case VIDEO_NOTIFY_TYPE_SRC_FORMATCHANGE:
					vid_flags.bits.srcformatchange_report_status = enable_bit;
					vid_flags.bits.srcformatchange_report_irq = 0;
					break;

				case VIDEO_NOTIFY_TYPE_DECODE_FINISHED:
					vid_flags.bits.decode_finished_status = enable_bit;
					vid_flags.bits.decode_finished_irq = 0;
					printk("Set VIDEO_NOTIFY_TYPE_DECODE_FINISHED!\n");
					break;
			}
			
			spin_unlock_irqrestore(&notify_lock, flags);
			break;
		}

		case CSVID_IOC_SET_VIB_PINMUX:
			if(arg == 1){
				pinmux_enable_i2c(_i2c_0);
				pinmux_enable_vib();
			}
			else{
				/* need do something here. */
			}

			clock_vib_reset(0);
			udelay(10);
			clock_vib_reset(1);
			break;

		case CSVID_IOC_RELOAD_FIRMWARE:
			//__csm_video_release_firmware();
			//udelay(100);
			__kernel_load_firmware(0);
            //udelay(100);
			//mdelay(1000);
			__video_init(0);
			//udelay(100);
			video_write(0x0, HOST_DPB_STATUS);
			video_write(0x0, MIPS_DPB_STATUS);
	//		video_write(VIDEO_HOST_IF_REGION, HOST_IF_VID_MIPS_MAILBOX_0);
			video_write(VIDEO_MCU_REGION >> 3, MIPS_RESET_OFFSET);
			//mdelay(10);
			break;

		default:
			return -EINVAL; 
	}

	return 0;
}

static int csm_video_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		int addr;
		addr = simple_strtol(&buffer[3], NULL, 16);
		printk(" addr[0x%08x] = 0x%08x \n", addr, video_read(addr));
	}

	return count;
}

static int __video_firmware_to_mips(int load_coda_flag)
{
	size_t firmware_size_txt = 0, firmware_size_dat = 0;
	unsigned int *p_src = NULL,*p_src_dat = NULL;
	volatile unsigned int *p_buf = NULL,*p_buf_dat = NULL;
	volatile unsigned int *start_addr = NULL;
	unsigned int i = 0;

	if (video_text_fw!=(struct firmware*)NULL){
		firmware_size_txt= video_text_fw->size>>2;
		p_src= (unsigned int *)video_text_fw->data;
		firmware_size_dat = (video_data_fw->size)>>2;
		p_src_dat= (unsigned int *)video_data_fw->data;
	}

	if(p_src!=NULL){
		start_addr = (volatile unsigned int *) vidmcu_base;
		memset((char *)start_addr, 0,VIDEO_MCU_SIZE);
		for (i = INST_OUT_MEM_SIZE / sizeof(unsigned int); i < firmware_size_txt; i++)
			*start_addr++ = p_src[i];
		for (i = 0; i < 4; i++)
			*start_addr++ = 0;

		p_buf = csm_video_base + MIPS_ISRAM_OFFSET;
		memset((char *)p_buf, 0,MIPS_ISRAM_MEM_SIZE);
		if (firmware_size_txt > (MIPS_ISRAM_MEM_SIZE) / sizeof(unsigned int))
			firmware_size_txt = (MIPS_ISRAM_MEM_SIZE) / sizeof(unsigned int);

		for (i = 0; i < firmware_size_txt; i++)
			p_buf[i] = p_src[i];

		p_buf_dat= csm_video_base + MIPS_DSRAM_OFFSET;
		memset((char *)p_buf_dat, 0,MIPS_DSRAM_MEM_SIZE);
		for (i = 0; i < firmware_size_dat; i++)
		{
			p_buf_dat[i] = p_src_dat[i];
		}

		for (; i < MIPS_DSRAM_MEM_SIZE / sizeof(unsigned int); i++)
			p_buf_dat[i] = 0x0;
	}

	if (load_coda_flag){
	    	if(video_coda_fw!=(struct firmware*)NULL){
	    		firmware_size_txt = video_coda_fw->size;
	    		p_src = (unsigned int *)video_coda_fw->data;
	    	}
	    	
	    	if(p_src!=NULL){
	    		volatile char *start_addr = NULL;
	    		volatile char *temp = NULL;

	    		start_addr = (volatile char *)(vidmcu_base + VIDEO_CODA_OFFSET);
	    		temp = (volatile char *)p_src;

	    		for (i = 0; i < firmware_size_txt; i++)
	    		{
	    			start_addr[i] = temp[i];
	    		}
	    	}
	}

	return 0;
}

static int __video_init(int load_coda_flag)
{
//	video_write(0x7f, 0x15C>>2);//video clock enable

	clock_video_clockena(_clock_enable);
	clock_vib_clockena(_clock_enable);
	video_write(0x0, MIPS_STATUS_REG);
#if 0
	mips_status = video_read(MIPS_RESET_OFFSET);
	if (!(mips_status & (1 << 25))) {	/* mcu is running, we must stop it */
		do {		/* we check whether the mcu stopped */
			if (0x0001004e == video_read(HOST_IF_VID_MIPS_STA0))
				break;
			else
				udelay(20);

			printk("loop1\n");
		} while (1);

		running = video_read(HOST_IF_VID_MIPS_MAILBOX_12);

		do{
			udelay(50);
			update_running = video_read(HOST_IF_VID_MIPS_MAILBOX_12);

			if (running == update_running){
				break;
			}
			else{
				running = update_running;
			}
			printk("loop2\n");
		}while(1);
	}
#endif

	/* reset mcu */
	video_write(0xffffffff, MIPS_RESET_OFFSET);
	udelay(5);

	__video_firmware_to_mips(load_coda_flag);

//	video_write(VIDEO_HOST_IF_REGION, HOST_IF_VID_MIPS_MAILBOX_0);//move to csvid_init()
	/* start mcu*/
//	video_write(VIDEO_MCU_REGION >> 3, MIPS_RESET_OFFSET);

	return 0;
}

static inline void __kernel_load_firmware(int load_coda_flag)
{
	int ret;
	if (video_text_fw == NULL) {
        ret = request_firmware(&video_text_fw, "video_fw_text.bin", &(video_pdev->dev));
        if (unlikely(ret != 0 || video_text_fw== NULL)) {
            printk(KERN_ERR "Failed to load video firmware code section 1\n");
            return;
        }
        else{
            if (unlikely(video_text_fw->size > VIDEO_MCU_SIZE)){
				release_firmware(video_text_fw);
				video_text_fw=NULL;
				printk(KERN_ERR "Failed to load video firmware code section 2\n");
				return;
            }
            if(video_text_fw!=NULL && video_data_fw == NULL){
                ret = request_firmware(&video_data_fw, "video_fw_data.bin", &(video_pdev->dev));
                if (unlikely(ret != 0 || video_data_fw== NULL)) {
                    release_firmware(video_text_fw);
                    video_text_fw=NULL;
                    printk(KERN_ERR "Failed to load video firmware data section 1\n");
                    return;
                }
                else{
                    if (unlikely(video_data_fw->size > MIPS_DSRAM_MEM_SIZE )){
			    		release_firmware(video_data_fw);
						video_data_fw=NULL;
						release_firmware(video_text_fw);
						video_text_fw=NULL;
						printk(KERN_ERR "Failed to load video firmware data section 2\n");
						return;
                    }
                }
            }
        }
	}

	if (load_coda_flag){
	    	if (video_coda_fw == NULL) {
	            ret = request_firmware(&video_coda_fw, "video_fw_coda.bin", &(video_pdev->dev));
	            if (unlikely(ret != 0 || video_coda_fw== NULL)) {
	                printk(KERN_ERR "Failed to load video coda firmware 1\n");
	                return;
	            }
	            else{
	                if (unlikely(video_coda_fw->size > BIT_CODE_SIZE)){
	    				release_firmware(video_coda_fw);
	    				video_coda_fw=NULL;
	    				printk(KERN_ERR "Failed to load video coda firmware 2\n");
	    				return;
	                }
	            }
	    	}
	}
    
	return;
}

static int csm_video_open(struct inode *inode, struct file *file)
{
	volatile char *start_addr = (volatile char *)vidmcu_base;
	csm1800_video_dev *video_dev = NULL;
	
	DEBUG_PRINTF("CSM VIDEO prepare open\n");
	if(NULL == file->private_data){
		printk("func:%s  line:%d process_num = %d\n",__FUNCTION__,__LINE__,process_num);
		video_dev = kmalloc(sizeof(csm1800_video_dev), GFP_KERNEL);
		if(NULL == video_dev){
			printk("func:%s  line:%d \n",__FUNCTION__,__LINE__);
			return -EBUSY;
		}
		video_dev->instance_num = 1;
		file->private_data = (void *)video_dev;
		if(process_num > 0){
			process_num ++;
			goto CSM_VIDEO_OPEN_SUCCEED;
		}
		else{
			process_num ++;
		}
	}
	else{
		video_dev = file->private_data;
		video_dev->instance_num ++;
		printk("func:%s  line:%d instance_num = %d\n",__FUNCTION__,__LINE__,video_dev->instance_num);
		goto CSM_VIDEO_OPEN_SUCCEED;
	}

	DEBUG_PRINTF("CSM VIDEO has been opened\n");
	printk("sizeof(_HOST_FW_IF_t) = %d\n",sizeof(_HOST_FW_IF_t));

	__kernel_load_firmware(1);
	memset((char *)start_addr,0xff,VIDEO_STUFF_SIZE);
	memset(host2videofw_if,0,sizeof(_HOST_FW_IF_t));

	host2videofw_if->channel_num = 0;
	host2videofw_if->coda_bit_code_addr = VIDEO_CODA_REGION;
	host2videofw_if->dpb_region = VIDEO_DPB_REGION;
	host2videofw_if->dpb_size = VIDEO_DPB_SIZE;

	__video_init(1);

	CSM_VIDEO_OPEN_SUCCEED:
	return 0;
}

static int __csm_video_release_firmware(void)
{
	int i = 0, ch = 0;
	unsigned int cmd = 0;
	unsigned int reg = 0;

	printk("CSM VIDEO has been released\n");

#if 1
	/* stop video */
	for (ch = 0; ch < MAX_CHANNEL_NUM; ch++){
		printk("channel %d \n", ch);
		if (host2videofw_if->channel_num & (1 << ch)){ // current channel enable, send stop cmd 
			i = 0;
			while (video_read(HOST_CMD_REG) >> 31){ // wait last cmd ack
				i++;
				udelay(100);

				if(1000 == i)
					break;
			}

			i = 0;
			cmd = (ch << 24) | 2; // 2: send stop cmd
			video_write(cmd|0x80000000, HOST_CMD_REG);

			do{
				if((video_read(HOST_CMD_REG) >> 31) == 0x0)
					break;

				udelay(100);
				i++;

				//printk("CSM VIDEO release failed %d!\n", i);

				if (1000 == i)
					break; 
			}while(1);

			printk("CSM VIDEO %d release success!\n", ch);
		}
	}

	// close interrupt
	reg = *(vidint_base + VID_RISC_ENA);
	//    printk("HOST_INT_ENABLE = 0x%08x\n", reg);
	reg |= (1 << 31);
	*(vidint_base + VID_RISC_ENA) = reg;
	//    printk("HOST_INT_ENABLE = 0x%08x\n", reg);
	*(vidint_base + VID_RISC_INT) = (1 << 31);

	i = 0;
	do{
		reg = *(vidint_base + VID_RISC_INT);
		//        printk("HOST_INT_FORCE = 0x%08x\n", reg);
		if(video_read(0x3d) == 0x1004e)
			break;

		i++;	
		if(1000 == i)
			break;

		udelay(100);

	}while (1);
#endif

	/* reset mcu */
	video_write(0xffffffff, MIPS_RESET_OFFSET);

	if (video_text_fw != NULL ){
		release_firmware(video_text_fw);
		video_text_fw = NULL;
	}
	if (video_data_fw != NULL){
		release_firmware(video_data_fw);
		video_data_fw = NULL;
	}
	if (video_coda_fw != NULL) {
		release_firmware(video_coda_fw);
		video_coda_fw = NULL;
	}	

	// coda reset
	printk("start code clock reset!\n");
//	clock_coda_reset(_do_reset);
//	mdelay(3);
	printk("reset code step 1 done\n");
//	clock_coda_reset(_do_set);
	printk("reset step 2 done\n");

	//video_write(0xf, 0x15C>>2);//video clock disable
//	clock_video_clockena(_clock_disable);
//	clock_vib_clockena(_clock_disable);
//	clock_coda_clockena(_clock_disable);

	return 0;
}

static int csm_video_release(struct inode *inode, struct file *file)
{
	csm1800_video_dev *video_dev = NULL;

	video_dev = file->private_data;
	video_dev->instance_num--;

	if(video_dev->instance_num > 0){
		printk("func:%s  line:%d instance_num = %d, process_num = %d\n",__FUNCTION__,__LINE__,video_dev->instance_num, process_num);
		return 0;
	}
	else{
printk("func:%s  line:%d instance_num = %d, process_num = %d\n",__FUNCTION__,__LINE__,video_dev->instance_num, process_num);
		kfree(file->private_data);
		process_num --;
		if(process_num <= 0){
			__csm_video_release_firmware();
		}
	}

	return 0;
}

static unsigned int csm_video_poll(struct file *filp, poll_table * wait)
{
	unsigned int mask = 0;

	if ((vid_flags.bits.timecode_irq) ||(vid_flags.bits.decodeerror_irq) ||(vid_flags.bits.aspectratio_irq) ||
	(vid_flags.bits.pscancrop_irq)||(vid_flags.bits.sync_irq)||(vid_flags.bits.sync_irq2)||
	(vid_flags.bits.underflow_report_irq)||(vid_flags.bits.formatchange_report_irq)||(vid_flags.bits.srcformatchange_report_irq)
	||(vid_flags.bits.decode_finished_irq)||(fix_bug_av_sync)){
		mask = POLLIN | POLLWRNORM;
		return mask;
	}
	poll_wait(filp, &notify_queue, wait);

	return mask;
}

static struct class *csm_video_class;
static struct proc_dir_entry *csm_video_proc_entry = NULL;
static struct file_operations csm_video_fops = {
	.owner	= THIS_MODULE,
	.open	=  csm_video_open,
	.release	= csm_video_release,
	.ioctl		= csm_video_ioctl,
	.poll		= csm_video_poll,
};

#define VID_INT_CMDACCEPT       0x1
#define VID_INT_NEWVIDEOFORMAT       0x2
#define VID_INT_NEWASPECTRATIO       0x4
#define VID_INT_STARTSENDCMDTODF       0x8
#define VID_INT_NEWTIMECODE       0x10
#define VID_INT_FINDUSERDATA       0x20
#define VID_INT_ADDFRMTOSWFIFO       0x40
#define VID_INT_FINDDECODERERROR       0x80
#define VID_INT_DPB_SIZE_SCANTY	0x100
#define VID_INT_M2VDRESET       0x200
#define VID_INT_VIBERROR       0x400
#define VID_INT_UD_EXCEED	0x800
#define VID_INT_DF0       0x1000
#define VID_INT_DF1       0x2000
#define VID_INT_SRCFORMAT	0x4000
#define VID_INT_UNDERFLOW      0x8000
#define VID_INT_YUVADDRALLOCATED      0x10000
#define VID_INT_FILEPLAYERROR      0x20000
#define VID_INT_CODA_RESET	   0x80000
#define VID_INT_DECODE_FINISH      0x100000

irqreturn_t csm_vid_irq(int irq, void *dev_id)
{
	unsigned long flag;
	unsigned int irq_reg = 0;
	unsigned int temp_val = 0;
	unsigned int channel_id = 0;

#ifdef DRV_VIDEO_DEBUG
		printk("csm_vid_irq!\n");
#endif
	irq_reg = video_read(VID_INT_TYPE_REG);
#ifdef DRV_VIDEO_DEBUG
		printk("irq_reg = 0x%x\n",irq_reg);
#endif
	channel_id = irq_reg>>28;

        if(irq_reg&VID_INT_CMDACCEPT){
#ifdef DRV_VIDEO_DEBUG
		printk("command accept!\n");
#endif
        }

	if(irq_reg&VID_INT_NEWVIDEOFORMAT){
#ifdef DRV_VIDEO_DEBUG
		printk("new video format!\n");
#endif
		spin_lock_irqsave(&notify_lock,flag);
		if(vid_flags.bits.formatchange_report_status){
			vid_flags.bits.formatchange_report_irq = 1;
			wake_up(&notify_queue);
		}
#ifdef DRV_VIDEO_DEBUG
		printk("vid_dint_format = 0x%x(stc = 0x%08x)\n",host2videofw_if->video_channel_para[channel_id].vid_dint_format, video_read(VID_STC0));
#endif
		spin_unlock_irqrestore(&notify_lock,flag);
       }

	if(irq_reg&VID_INT_NEWASPECTRATIO){
#ifdef DRV_VIDEO_DEBUG
		printk("new aspect ratio!\n");
#endif
                spin_lock_irqsave(&notify_lock,flag);
                if((vid_flags.bits.aspectratio_status)||(vid_flags.bits.pscancrop_status)){
                    if(vid_flags.bits.aspectratio_status)vid_flags.bits.aspectratio_irq = 1;
                    if(vid_flags.bits.pscancrop_status)vid_flags.bits.pscancrop_irq = 1;
		    wake_up(&notify_queue);
                }
#ifdef DRV_VIDEO_DEBUG
		printk("vid_aspect_ratio = 0x%x(stc = 0x%08x)\n",host2videofw_if->video_channel_para[channel_id].vid_aspect_ratio, video_read(VID_STC0));
#endif
                spin_unlock_irqrestore(&notify_lock,flag);
       }

	if(irq_reg&VID_INT_STARTSENDCMDTODF){
#ifdef DRV_VIDEO_DEBUG
	printk("host2videofw_if->video_channel_para[channel_id].vid_display_info = 0x%x\n",host2videofw_if->video_channel_para[channel_id].vid_display_info);
#endif

                spin_lock_irqsave(&notify_lock,flag);
		  if(vid_flags.bits.sync_status){
//			temp_val = video_read(HOST_IF_VID_MIPS_MAILBOX_3);
			temp_val = host2videofw_if->video_channel_para[channel_id].vid_display_info;
			if(temp_val >>16){
				fix_bug_av_sync = 1;
				vid_flags.bits.sync_irq2 = 1;
		       	wake_up(&notify_queue);
			}
			else if(temp_val){
				vid_flags.bits.sync_irq = 1;
		       	wake_up(&notify_queue);
			}
			else{
				vid_flags.bits.sync_irq = 1;
		       	wake_up(&notify_queue);
			}
                }
		spin_unlock_irqrestore(&notify_lock,flag);
#ifdef DRV_VIDEO_DEBUG
		printk("kernel: start send command to DF!\n");
		printk("kernel: syncirq = %d, sync_irq2 = %d\n",vid_flags.bits.sync_irq,vid_flags.bits.sync_irq2);
#endif
       }

	if(irq_reg&VID_INT_NEWTIMECODE){
		spin_lock_irqsave(&notify_lock,flag);
                if(vid_flags.bits.timecode_status){
		        vid_flags.bits.timecode_irq = 1;
		        wake_up(&notify_queue);
                }
		spin_unlock_irqrestore(&notify_lock,flag);
#ifdef DRV_VIDEO_DEBUG
		printk("new time code!\n");
#endif
       }

	if(irq_reg&VID_INT_FINDUSERDATA){
#ifdef DRV_VIDEO_DEBUG
		printk("found user data!\n");
#endif
       }

	if(irq_reg&VID_INT_ADDFRMTOSWFIFO){
#ifdef DRV_VIDEO_DEBUG
		printk("AddFrmToSWFIFO!\n");
#endif
       }
 
	if(irq_reg&VID_INT_FINDDECODERERROR){
		spin_lock_irqsave(&notify_lock,flag);
                if(vid_flags.bits.decodeerror_status){
		        vid_flags.bits.decodeerror_irq = 1;
		        wake_up(&notify_queue);
                }
		spin_unlock_irqrestore(&notify_lock,flag);
#ifdef DRV_VIDEO_DEBUG
		printk("found decode error!\n");
#endif

       }

        if (irq_reg&VID_INT_UNDERFLOW){
	    spin_lock_irqsave(&notify_lock,flag);
	    if(vid_flags.bits.underflow_report_status){
		vid_flags.bits.underflow_report_irq = 1;
		wake_up(&notify_queue);
	    }
	    spin_unlock_irqrestore(&notify_lock,flag);
#ifdef DRV_VIDEO_DEBUG
	    printk("found video data underflow!\n");
#endif
	}

        if(irq_reg&VID_INT_M2VDRESET){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_M2VDRESET IN!\n");
#endif

//        __video_init(vid_stream_types[CPB_CHANNEL_0]);
//     udelay(10);
//        __video_ctrl_cmd(VID_CMD_START, 0);
        
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_M2VDRESET OUT!\n");
#endif
       }

	if(irq_reg&VID_INT_VIBERROR){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_VIBERROR!\n");
#endif
//	__VIB_Reset();
//	__video_ctrl_cmd(VID_CMD_START, 0);
	}

	if(irq_reg&VID_INT_SRCFORMAT){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_SRCFORMAT!\n");
#endif
		spin_lock_irqsave(&notify_lock,flag);
		if(vid_flags.bits.srcformatchange_report_status){
			vid_flags.bits.srcformatchange_report_irq = 1;
			wake_up(&notify_queue);
		}
#ifdef DRV_VIDEO_DEBUG
		printk("vid_src_format = 0x%x(stc = 0x%08x)\n",host2videofw_if->video_channel_para[channel_id].vid_src_format, video_read(VID_STC0));
#endif
		spin_unlock_irqrestore(&notify_lock,flag);
	}

	if(irq_reg&VID_INT_YUVADDRALLOCATED){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_YUVADDRALLOCATED!\n");
#endif
	}
	if(irq_reg&VID_INT_FILEPLAYERROR){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_FILEPLAYERROR!\n");
#endif
	}

	if (irq_reg&VID_INT_CODA_RESET){
//		unsigned int temp_val;

#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_CODA_RESET!\n");
#endif

#if 0
		temp_val = *(vid_reset_base + HOST_IF_VID_RESET);
		temp_val = temp_val & (~(1<<18));
		*(vid_reset_base + HOST_IF_VID_RESET) = temp_val;
		mdelay(1);						// step 1, set reset bit to 0, and wait it stable

//		printk("reset code step 1 done\n");
		temp_val = *(vid_reset_base + HOST_IF_VID_RESET);
		temp_val = temp_val | (1<<18);
		*(vid_reset_base + HOST_IF_VID_RESET) = temp_val;	// step 2, set reset bit to 1, and wait to stable 
		mdelay(2);
//		printk("reset step 2 done\n");
#else
#if 1
		clock_coda_reset(_do_reset);
		mdelay(3);
		clock_coda_reset(_do_set);
#endif
#endif
		video_write(0xf0f0f0f0, MIPS_STATUS_REG);
		printk("reset coda done\n");
	}

	if(irq_reg&VID_INT_DECODE_FINISH){
#ifdef DRV_VIDEO_DEBUG
		printk("VID_INT_DECODE_FINISH!  0x%x\n",video_read(VID_MIPS_MAILBOX_56));
#endif
	}
	
	{
		unsigned int temp_val = 0;
		temp_val = video_read(VID_INT_TYPE_REG);
		temp_val = temp_val^irq_reg;
		video_write(temp_val, VID_INT_TYPE_REG);
	}

	*(vidint_base + (VID_HOST_INT)) = 0;
	udelay(10);

	return IRQ_HANDLED;
}

void __video_mem_initialize(void)
{
	if (!request_mem_region(CSM_VIDEO_REG_BASE, CSM_VIDEO_REG_SIZE, "CNC1800L Video"))
		goto ERR_MEM1;

	if (NULL == (csm_video_base = ioremap(CSM_VIDEO_REG_BASE, CSM_VIDEO_REG_SIZE)))
		goto ERR_MEM2;

	if (!request_mem_region(VIDEO_STUFF_REGION, VIDEO_STUFF_SIZE, "MCU Video"))
		goto ERR_MEM3;

	if (NULL == (vidmcu_base = ioremap(VIDEO_STUFF_REGION, VIDEO_STUFF_SIZE)))
		goto ERR_MEM4;

	if (!request_mem_region(RISC_INT_ADDR_BASE, RISC_INT_ADDR_SIZE, "Video INT"))
		goto ERR_MEM5;

	if (NULL == (vidint_base = ioremap(RISC_INT_ADDR_BASE, RISC_INT_ADDR_SIZE)))
		goto ERR_MEM6;

	return;
	
	ERR_MEM6:
	release_mem_region(RISC_INT_ADDR_BASE, RISC_INT_ADDR_SIZE);
	ERR_MEM5:
	iounmap((void *) vidmcu_base);	
	ERR_MEM4:
	release_mem_region(VIDEO_STUFF_REGION, VIDEO_STUFF_SIZE);
	ERR_MEM3:
	iounmap((void *) csm_video_base);	
	ERR_MEM2:
	release_mem_region(CSM_VIDEO_REG_BASE, CSM_VIDEO_REG_SIZE);
	ERR_MEM1:
	return;
}

void __video_mem_destroy(void)
{
	iounmap((void *)csm_video_base);
	release_mem_region(CSM_VIDEO_REG_BASE, CSM_VIDEO_REG_SIZE);
	iounmap((void *)vidmcu_base);
	release_mem_region(VIDEO_STUFF_REGION, VIDEO_STUFF_SIZE);
	iounmap((void *)vidint_base);
	release_mem_region(CSM_VIDEO_REG_BASE, RISC_INT_ADDR_SIZE);
}

#define BACKGROUND_SIZE (0x6000) //1024x16x1.5
#define DIRWPRP_SIZE (0) // 4*2*8
static int __init csm_video_init(void)
{
	if (register_chrdev(VIDEO_MAJOR, "cnc_1800l_video", &csm_video_fops)){
		return -ENODEV;
	}

	video_pdev = platform_device_register_simple("cnc_1800l_video", 0, NULL, 0);
    	if (IS_ERR(video_pdev)) {
        	return -ENODEV;
    	}

	csm_video_class = class_create(THIS_MODULE,"cnc_1800l_video");
	if (IS_ERR(csm_video_class)){
		printk(KERN_ERR "VIDEO: class create failed.\n");
		__video_mem_destroy();
		csm_video_class = NULL;
		return -ENODEV;
	}

	device_create(csm_video_class, NULL, MKDEV(VIDEO_MAJOR, 0), NULL, "cnc_1800l_video");

	csm_video_proc_entry = create_proc_entry("video_io", 0, NULL);
	if (NULL != csm_video_proc_entry) {
		csm_video_proc_entry->write_proc = &csm_video_proc_write;
	}

	__video_mem_initialize();

	if (request_irq(20, csm_vid_irq, 0, "cnc_video", NULL)) {
		printk(KERN_ERR "cnc_vid: cannot register IRQ \n");
		return -EIO;
	}

//	host2videofw_if = (_HOST_FW_IF_t *)(vidmcu_base + VIDEO_HOST_OFFSET);
	host2videofw_if = (_HOST_FW_IF_t *)(vidmcu_base + VIDEO_HOST_OFFSET+(BACKGROUND_SIZE+DIRWPRP_SIZE)/4);

	printk("CNC1800L Video System Version : 0.1\n");

	return 0;
}

static void __exit csm_video_exit(void)
{
	__video_mem_destroy();

	if (NULL != csm_video_proc_entry)
		remove_proc_entry("video_io", NULL);

	device_destroy(csm_video_class,MKDEV(VIDEO_MAJOR, 0));
	class_destroy(csm_video_class);
	unregister_chrdev(VIDEO_MAJOR, "cnc_1800l_video");

	printk(KERN_INFO " CNC 1800L VIDEO Exit ...... OK. \n");

	return;
}

module_init(csm_video_init);
module_exit(csm_video_exit);
MODULE_AUTHOR("Jia Ma, <jia.ma@caviumnetworks.com>");
MODULE_DESCRIPTION("Celestial Semiconductor Video sub-system");
MODULE_LICENSE("GPL");

