#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
//#include <linux/devfs_fs_kernel.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/signal.h>
#include <mach/mux.h>


#include <mach/hardware.h>
#include "xport_drv1800.h"
#include "xport_dma1800.h"
#include "xport_mips1800.h"
#include "xport_filter1800.h"


#define MAX_FW_DATA_SIZE 2048*4
#define MAX_FW_TEXT_SIZE  3072*4
#define  TS_SIZE      	188


#define SLEEP_TIMEOUT 2  /* sec*/

void __iomem *xport_regs_base;
void __iomem *xport_mem_base;

#define DEBUG_ON()


/* start of security-core code
 * this is for write descrambler key on the chip
 * that host can't access key register directly
 * added by xp, 2012-10-23
 */

/* Registers */
#define CNC_SEC_MB_BASE       0xfffff800
#define CNC_SEC_MB_SIZE       0x20

#define MAILBOX0              (sec_mb_base+0x00)
#define MAILBOX1              (sec_mb_base+0x04)
#define MAILBOX2              (sec_mb_base+0x08)
#define MAILBOX3              (sec_mb_base+0x0c)
#define MAILBOX4              (sec_mb_base+0x10)
#define MAILBOX5              (sec_mb_base+0x14)
#define MAILBOX6              (sec_mb_base+0x18)
#define MAILBOX7              (sec_mb_base+0x1c)

#define scwrite32(a,v) (*(unsigned long *)(a) = (v))
#define scread32(a)  (*(unsigned long *)(a))

#define TIMEOUT_MAX 200000
#define CHECK_WITH_TMO(condition) ({            \
		int timeout = 0;                    \
		while (condition) {                 \
		if (timeout++ > TIMEOUT_MAX)    \
		return -1;      \
		else                            \
		udelay(1);                  \
		}                                   \
		})

void __iomem *sec_mb_base;

/*
 * this function is used for transport descrambler key write
 * cmd=0x80000004 //write host key
 * key[9]={x,x,x,x,x,x,x,x,id};the last key is for keyid
 * size=8
 * desckey_write(0x80000004,key,8)
 */
static int desckey_write(unsigned int cmd, const unsigned char* key, int size)
{
	unsigned int val[6];
	unsigned int keyid;
	//int i;

	if (size & 0x3)
		return -1;

	CHECK_WITH_TMO(scread32(MAILBOX0) & 0x80000000);

	//every key is 32bit width
	val[0]=(unsigned int)key[0]|((unsigned int)key[1]<<8)|  ((unsigned int)key[2]<<16)|((unsigned int)key[3]<<24);
	val[1]=(unsigned int)key[4]|((unsigned int)key[5]<<8)|  ((unsigned int)key[6]<<16)|((unsigned int)key[7]<<24);
	/*descrambler key id 0~15 refer to channel0~7 ODD,EVEN key
	  for example,
	  if keyid =0, that means channel 0 ODD key
	  if keyid = 3, that means channel1 EVEN key
	  if keyid =15, that means channel8 EVEN key*/
	keyid=(unsigned long)key[8]&0x000000ff;

	scwrite32(MAILBOX1, 0x00000200|keyid);//0x00000200 means DVB

	// for(i = 0; i < size; i +=4)
	//scwrite32(MAILBOX2 + i, val[i/4]);
	scwrite32(MAILBOX2, val[0]);
	scwrite32(MAILBOX3, val[1]);

	scwrite32(MAILBOX7, 0x0);

	scwrite32(MAILBOX0, cmd);

	CHECK_WITH_TMO(!(scread32(MAILBOX7) & 0x80000000));

	if (scread32(MAILBOX7) & 0xffff)
		return -1;

	scwrite32(MAILBOX7, 0x0);

	return 0;
}

/* end of security-core code */

static struct class *cnc180x_xport_class;
static struct platform_device *xport_pdev;

static unsigned int irq0_dev_id = XPORT_IRQ0_ID;
static unsigned int irq1_dev_id = XPORT_IRQ1_ID;

static unsigned int crc_err_idx = 0x0000ffff;
static unsigned int last_crc_error_idx = -1;


static DECLARE_WAIT_QUEUE_HEAD(crc_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq0_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq1_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq0_dma0_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq0_dma1_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq0_dma2_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(irq0_dma3_wait_queue);

static wait_queue_head_t filter_wait_queue[MAX_FILTER_NUM];

void xport_writeb(int a,int v)
{
	writeb(v, xport_regs_base + __OFFSET_ADDR__(a)); udelay(5);
}


void xport_writew(int a,int v)
{
	writew(v, xport_regs_base + __OFFSET_ADDR__(a)); udelay(5);
}

void xport_writel(int a,int v)
{
	writel(v, xport_regs_base + __OFFSET_ADDR__(a)); udelay(5);
}

unsigned char xport_readb(int a)
{
	return readb( xport_regs_base    + __OFFSET_ADDR__(a) );
}

unsigned short xport_readw(int a)
{
	return readw( xport_regs_base    + __OFFSET_ADDR__(a) );
}

/*read data from mem mapped from usr space's device to kernel*/
unsigned int xport_readl(int a)
{
	return readl( xport_regs_base    + __OFFSET_ADDR__(a) );
}


EXPORT_SYMBOL(xport_readb);
EXPORT_SYMBOL(xport_readw);
EXPORT_SYMBOL(xport_readl);
EXPORT_SYMBOL(xport_writeb);
EXPORT_SYMBOL(xport_writew);
EXPORT_SYMBOL(xport_writel);


static int xport_load_firmware(void)
{
	int ret,i;
	const struct firmware *xport_data_fw=NULL;
	const struct firmware *xport_text_fw=NULL;
	unsigned int wr_data;

	printk("loading xport firmware\n ");

	ret = request_firmware(&xport_data_fw, "xport_fw_data.bin", &(xport_pdev->dev));
	if (ret != 0 || xport_data_fw == NULL ) {
		printk(KERN_ERR "Failed to load xport firmware data section\n");
		return -1;
	}
	if (xport_data_fw->size > MAX_FW_DATA_SIZE){
		printk(KERN_ERR "Firmware data senction is too larage!\n");
		return -1;
	}
	printk("readed xport fw data section size =%d\n", xport_data_fw->size);

	ret = request_firmware(&xport_text_fw, "xport_fw_text.bin", &(xport_pdev->dev));
	if (ret != 0 || xport_text_fw == NULL) {
		printk(KERN_ERR "Failed to load xport firmware code section\n");
		return -1;
	}
	if (xport_text_fw->size > MAX_FW_TEXT_SIZE){
		printk(KERN_ERR "Firmware text senction is too larage!\n");
		return -1;
	}
	printk("readed xport fw text section size =%d\n", xport_text_fw->size);

	xport_writel(MIPS_FW_EN, 1);
	for (i = 0; i < xport_data_fw->size; i+=4) {
		wr_data = (xport_data_fw->data)[i+3]<<24 | (xport_data_fw->data)[i+2]<<16 | (xport_data_fw->data)[i+1]<<8 | (xport_data_fw->data)[i];
		xport_writel(MIPS_FW_WRITE_DATA+i, wr_data);
	}
	for (; i < MAX_FW_DATA_SIZE; i+=4) {
		xport_writel(MIPS_FW_WRITE_DATA +i, 0);
	}


	for (i = 0; i < xport_text_fw->size; i+=4) {
		wr_data = (xport_text_fw->data)[i+3]<<24 | (xport_text_fw->data)[i+2]<<16 | (xport_text_fw->data)[i+1]<<8 | (xport_text_fw->data)[i];
		xport_writel(MIPS_FW_WRITE_INST+i, wr_data);
	}

	xport_writel(MIPS_FW_EN, 0);

	printk("xport firmware loaded.\n");

	if (xport_data_fw != NULL )
		release_firmware(xport_data_fw);
	if (xport_text_fw != NULL )
		release_firmware(xport_text_fw);

	return 0;
}

#if 0 // majia add these macro in mem_define.h
/**get buf from mips for xport psi component region**/
#define AUDIO_CAB_REGION	AUDIO_STUFF_REGION
#if (BASE_DDR_ADDR==0x8000000)
#define AUDIO_CAB_SIZE	(0x18000*1)
#define AUDIO_PTS_SIZE	(0x3000*1)
#else
#define AUDIO_CAB_SIZE	(0x18000*4)
#define AUDIO_PTS_SIZE	(0x3000*4)
#endif
#define AUDIO_PTS_REGION	(AUDIO_CAB_REGION + AUDIO_CAB_SIZE)
#endif

static int xport_config_buf(void)
{
	xport_mips_write(MIPS_CHL_BUF_LOW_ADDR(0), VIDEO_CPB_REGION);
	xport_mips_write(MIPS_CHL_BUF_UP_ADDR(0), VIDEO_CPB_REGION + VIDEO_CPB_SIZE/VIDEO_NUM);
	xport_mips_write(MIPS_CHL_DIR_LOW_ADDR(0), VIDEO_CPB_DIR_REGION);
	xport_mips_write(MIPS_CHL_DIR_UP_ADDR(0), VIDEO_CPB_DIR_REGION + VIDEO_CPB_DIR_SIZE/VIDEO_NUM);

	xport_mips_write(MIPS_CHL_BUF_LOW_ADDR(1), VIDEO_CPB_REGION + VIDEO_CPB_SIZE/VIDEO_NUM);
	xport_mips_write(MIPS_CHL_BUF_UP_ADDR(1), VIDEO_CPB_REGION + (VIDEO_CPB_SIZE/VIDEO_NUM)*2);
	xport_mips_write(MIPS_CHL_DIR_LOW_ADDR(1), VIDEO_CPB_DIR_REGION + VIDEO_CPB_DIR_SIZE/VIDEO_NUM);
	xport_mips_write(MIPS_CHL_DIR_UP_ADDR(1), VIDEO_CPB_DIR_REGION + (VIDEO_CPB_DIR_SIZE/VIDEO_NUM)*2);

	xport_mips_write(MIPS_CHL_DIR_LOW_ADDR(XPORT_AV_CHNL_NUM), AUDIO_CAB_REGION);
	xport_mips_write(MIPS_CHL_DIR_UP_ADDR(XPORT_AV_CHNL_NUM), AUDIO_CAB_REGION + (AUDIO_CAB_SIZE/AUDIO_NUM));

	xport_mips_write(MIPS_CHL_BUF_LOW_ADDR(XPORT_AV_CHNL_NUM), AUDIO_PTS_REGION);
	xport_mips_write(MIPS_CHL_BUF_UP_ADDR(XPORT_AV_CHNL_NUM), AUDIO_PTS_REGION + (AUDIO_PTS_SIZE/AUDIO_NUM));

	/* set external base address */
	printk("write to mips external base addr = 0x%08x\n", MIPS_EXTERNAL_BASE_ADDR);
	xport_mips_write(MIPS_CHL_EXTERNAL_BASE(0), MIPS_EXTERNAL_BASE_ADDR);


	return 0;
}


/**initialize xport input channel register status**/
static int xport_hw_init(void)
{
	xport_writel(IN_CHL0_BASE_ADDR, (XPORT_CHL0_BASE_ADDR_DEF >> 3));
	xport_writel(IN_CHL0_CFG_ADDR, IN_CHL0_CFG_DEF);
	xport_writel(IN_CHL0_CFG_ADDR, IN_CHL0_CFG_DEF);

	xport_writel(IN_CHL1_BASE_ADDR, (IN_CHL1_BASE_ADDR_DEF >> 3));
	xport_writel(IN_CHL1_CFG_ADDR, IN_CHL1_CFG_DEF);
	xport_writel(IN_CHL1_CFG_ADDR, IN_CHL1_CFG_DEF);

	xport_writel(XPORT_CFG_ADDR1,
			XPORT_CFG1_OUT_CHL0_LINE_SYNC |
			XPORT_CFG1_OUT_CHL1_LINE_SYNC |
			XPORT_CFG1_OUT_CHL2_LINE_SYNC |
			XPORT_CFG1_OUT_CHL3_LINE_SYNC);

	xport_writel(CLK0_PWM_CTRL_ADDR, 0x2);      // for high

	return 0;
}


/**initialize and activate irq0 signal of xport module**/
irqreturn_t xport_irq0(int irq, void *dev_id, struct pt_regs * egs)
{
	unsigned int irq_reg0;
	unsigned int idx;
	unsigned int i;

	irq_reg0 = xport_readl(XPORT_INT_REG_ADDR0);
	xport_writel(XPORT_INT_CLS_ADDR0, irq_reg0);

	idx = xport_readl(XPORT_INT_EXT_ADDR);
	xport_writel(XPORT_INT_EXT_ADDR, xport_readl(XPORT_INT_EXT_ADDR) ^ idx);

	if (irq_reg0 & XPORT_IRQ0_DMA0_EMPTY_MSK)
	{
		wake_up_interruptible(&irq0_dma0_wait_queue);
	}

	if (irq_reg0 & XPORT_IRQ0_DMA1_EMPTY_MSK)
	{
		wake_up_interruptible(&irq0_dma1_wait_queue);
	}

	if (irq_reg0 & XPORT_IRQ0_DMA2_EMPTY_MSK)
	{
		wake_up_interruptible(&irq0_dma2_wait_queue);
	}

	if (irq_reg0 & XPORT_IRQ0_DMA3_EMPTY_MSK)
	{
		wake_up_interruptible(&irq0_dma3_wait_queue);
	}

	if (irq_reg0 & XPORT_IRQ0_CRC_NOTIFY)
	{
		crc_err_idx = (irq_reg0 & XPORT_IRQ0_CRC_INDEX)>>XPORT_IRQ0_CRC_IDX_SHIFT;
		wake_up_interruptible(&crc_wait_queue);
	}

	if(irq_reg0 & XPORT_IRQ0_FILTER_NOTIFY)
	{
		for(i = 32; i < MAX_FILTER_NUM; i++)
		{
			if(idx & 0x1)
			{
				wake_up_interruptible(&filter_wait_queue[i]);
			}
			idx >>= 1;
		}
	}

	wake_up_interruptible(&irq0_wait_queue);
	return IRQ_HANDLED;
}


/**initialize and activate irq1 signal of xport module**/
irqreturn_t xport_irq1(int irq, void *dev_id, struct pt_regs * egs)
{
	unsigned int i;
	unsigned int irq_reg1;

	irq_reg1 = xport_readl(XPORT_INT_REG_ADDR1);
	xport_writel(XPORT_INT_CLS_ADDR1, irq_reg1);

	for (i = 0; i < 32; i++)
	{
		if (irq_reg1 & 1)
		{
			wake_up_interruptible(&filter_wait_queue[i]);
		}
		irq_reg1 >>= 1;
	}

	wake_up_interruptible(&irq1_wait_queue);

	return IRQ_HANDLED;
}

/***initialize xport device object***/
static int xport_open(struct inode *inode, struct file *file)
{
	unsigned int filter_index;
	XPORT_DEV *xport_dev = NULL;
	unsigned int chl_id = 0;
	unsigned int dma_id;

	xport_dev = kmalloc(sizeof(XPORT_DEV), GFP_KERNEL);

	if(xport_dev == NULL)
	{
		//printk("func:%s  line:%d chl_id:%d \n",__FUNCTION__,__LINE__,chl_id);
		return -EBUSY;
	}
	xport_dev->dev_minor = iminor(inode);

	chl_id = (int)(xport_dev->dev_minor-XPORT_MINOR_CHL(0));
	dma_id = 0;

	//printk("chl_id:%d,dma_id:%d \n",chl_id,dma_id);

	if(chl_id>=0 && chl_id <XPORT_MAX_CHL_NUM)
	{
		//printk("func:%s  line:%d chl_id:%d \n",__FUNCTION__,__LINE__,chl_id);
		//xport_dev->irq_wait_queue_ptr = (dma_id?(&irq0_dma1_wait_queue):(&irq0_dma0_wait_queue));
		xport_dev->irq_wait_queue_ptr = chl_id==0?&irq0_dma0_wait_queue:&irq0_dma1_wait_queue;

		xport_dev->stream_type = XPORT_INPUT_TS_TYPE;
		xport_dev->packet_size = TS_SIZE;
		xport_dma_reset(dma_id);
		xport_writel((dma_id?DMA_INPUT1_HEAD_ADDR: DMA_INPUT0_HEAD_ADDR),0);
		//xport_writel(IN_CHL_RP_ADDR(chl_id),xport_readl(IN_CHL_WP_ADDR(chl_id)));
		/* reset chl wp & rp here, 2011-05-03 by xp */
		xport_writel(IN_CHL_RP_ADDR(chl_id), 0x40000000);
		xport_writel(IN_CHL_WP_ADDR(chl_id), 0);
		xport_writel(XPORT_CHL_DMA_WP_ADDR(chl_id),0);

		memset(xport_mem_base+ (IN_CHL_BASE_DEF(chl_id)-XPORT_MEM_BASE),
				0,IN_CHL_UNIT_SIZE_DEF(chl_id)*IN_CHL_UNIT_NUM_DEF(chl_id));
	}
	else if ((xport_dev->dev_minor >= XPORT_MINOR_FT_BASE)
			&& (xport_dev->dev_minor < (XPORT_MINOR_FT_BASE + MAX_FILTER_NUM)))
	{
		filter_index = xport_dev->dev_minor - XPORT_MINOR_FT_BASE;
		xport_dev->filter_type = FILTER_TYPE_NUKOWN;
		xport_dev->irq_wait_queue_ptr = &filter_wait_queue[filter_index];
		xport_filter_reset(filter_index);
	}
	else
		xport_dev->irq_wait_queue_ptr = NULL;

	spin_lock_init(&(xport_dev->spin_lock));
	init_MUTEX(&(xport_dev->xport_sem));
	file->private_data = (void *) xport_dev;

	return 0;
}

static int xport_release(struct inode *inode, struct file *file)
{
	XPORT_DEV *xport_dev = file->private_data;
	unsigned int mnr =	xport_dev->dev_minor;
	unsigned int regs_val = 0;
	unsigned int tmp_val = 0;
	unsigned int mask_val = 0;
	unsigned int filter_index;

	if(xport_dev->dev_minor >= XPORT_MINOR_CHL(0) &&
			xport_dev->dev_minor < XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM))
	{
		// disable xport input channel
		tmp_val = xport_readl(IN_CHL_CFG_ADDR(xport_dev->dev_minor-XPORT_MINOR_CHL(0)));
		tmp_val &= ~0x80000000;
		xport_writel(IN_CHL_CFG_ADDR(xport_dev->dev_minor-XPORT_MINOR_CHL(0)),tmp_val);

		// disable tuner input channel
		mask_val = 0x1 << (mnr - XPORT_MINOR_CHL(0));
		tmp_val = xport_readl(XPORT_TUNER_EN);
		tmp_val &= ~mask_val;
		xport_writel(XPORT_TUNER_EN, tmp_val);

		// disable DMA
		mask_val = 0x3 << (8 * (mnr - XPORT_MINOR_CHL(0)));
		tmp_val = xport_readl(XPORT_CFG_ADDR0);
		tmp_val &= ~mask_val;
		xport_writel(XPORT_CFG_ADDR0, tmp_val);

		// clear channel buffer
		regs_val = (xport_dev->dev_minor-XPORT_MINOR_CHL(0)) ;
		xport_writel(IN_CHL_RP_ADDR(regs_val), 0x40000000);
		xport_writel(XPORT_CHL_DMA_WP_ADDR(regs_val),0);
		memset(xport_mem_base+(IN_CHL_BASE_DEF(regs_val)-XPORT_MEM_BASE),0,
				IN_CHL_UNIT_SIZE_DEF(regs_val)*IN_CHL_UNIT_NUM_DEF(regs_val));
		printk("xport channel %d has been released. \n", regs_val);
	}
	else if ((xport_dev->dev_minor >= XPORT_MINOR_FT_BASE)
			&& (xport_dev->dev_minor < (XPORT_MINOR_FT_BASE + MAX_FILTER_NUM)))
	{
		filter_index = xport_dev->dev_minor - XPORT_MINOR_FT_BASE;
		xport_filter_reset(filter_index);
		printk("xport filter %d has been released. \n", filter_index);
	}

	// free structure
	kfree(file->private_data);
	return 0;
}


static ssize_t xport_read(struct file *file, char __user * buffer, size_t len, loff_t * offset)
{
	unsigned int addr;
	unsigned int filter_index;
	ssize_t ret = -EFAULT;

	XPORT_DEV *xport_dev = file->private_data;

	if (likely((xport_dev->dev_minor >= XPORT_MINOR_FT_BASE)
				&& (xport_dev->dev_minor < (XPORT_MINOR_FT_BASE + MAX_FILTER_NUM)))) {

		if (unlikely(xport_dev->filter_type == FILTER_TYPE_NUKOWN)) {
			return -EFAULT;
		}

		filter_index = xport_dev->dev_minor - XPORT_MINOR_FT_BASE;

		if (down_interruptible(&xport_dev->xport_sem)) {
			return -ERESTARTSYS;
		}

		if(xport_dev->filter_type == FILTER_TYPE_SECTION) {

			ret = xport_filter_check_section_number(filter_index);
			//printk("---func:%s line:%d--ret:[%d]---\n",__FUNCTION__,__LINE__,ret);

			if (ret > 0) {
				ret = xport_filter_read_section_data(filter_index, buffer, len);
			}
		}
		else if (xport_dev->filter_type == FILTER_TYPE_PES) {
			ret = xport_filter_check_pes_size(filter_index);
			if (ret > 0) {
				ret = xport_filter_read_pes_data(filter_index, buffer, len);
			}
		}
		else {
			ret = xport_filter_check_data_size(filter_index);
			if (ret >0) {
				ret = xport_filter_read_data(filter_index, buffer, len);

			} else{
				ret = 0;
			}
		}
		up(&xport_dev->xport_sem);

		return ret;
	}
	else if (unlikely(xport_dev->dev_minor == XPORT_MINOR)) { //?
		/* It should be deleted branch*/
		void __iomem *read_addr;

		if (down_interruptible(&xport_dev->xport_sem)) {
			return -ERESTARTSYS;
		}

		copy_from_user(&addr, buffer, 4);	/* FIXME@debug used interface, can dump all date of xport buffer. It can be deleted */

		read_addr = xport_mem_base + (addr - XPORT_MEM_BASE);
		if (copy_to_user(buffer, read_addr, len)) {
			up(&xport_dev->xport_sem);
			return -EFAULT;
		}

		up(&xport_dev->xport_sem);

		return len;
	}
	return -EFAULT;
}
typedef  ssize_t(*WRITE_FN)(const char __user *buffer,size_t len,unsigned int chl_id, ssize_t packet_size);

static ssize_t xport_write(struct file *file, const char __user * buffer, size_t len, loff_t * offset)
{
	WRITE_FN fn;
	unsigned int chl_id = 0;
	unsigned int chl_type = 0;
	unsigned int dma_id = 0;

	ssize_t ret = -EFAULT;
	XPORT_DEV *xport_dev = file->private_data;
	ssize_t packet_size = xport_dev->packet_size;

	DECLARE_WAITQUEUE(wait, current);

	if(xport_dev->dev_minor >= XPORT_MINOR_CHL(0)
			&& xport_dev->dev_minor < XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM) )
	{
		chl_id = xport_dev->dev_minor - XPORT_MINOR_CHL(0);
	}
	else
	{
		//		printk("--func:%s line:%d dma_id:%d --	\n",__FUNCTION__,__LINE__,dma_id);
		return -EFAULT;
	}

	if (len < packet_size || ((len % packet_size) != 0))
	{
		printk(KERN_ERR"error 2 happend len = %d  packet_size = %d\n", len, packet_size);
		return -EFAULT;
	}

	/*xunli: for direct dma mode */
	chl_type = xport_readl(IN_CHL_CFG_ADDR(xport_dev->dev_minor - XPORT_MINOR_CHL(0)));
	chl_type = (chl_type >> 29) & 0x3;

	if(chl_type == 3)
	{
		fn = (WRITE_FN)xport_dma_direct_write;
	}
	else
	{
		if (xport_dev->stream_type != XPORT_INPUT_TS_TYPE)
		{
			printk(KERN_ERR"error 3 happend\n");
			return -EFAULT; // only support TS format in dma or tuner mode
		}
		fn = (WRITE_FN)xport_dma_write;
	}

	add_wait_queue(xport_dev->irq_wait_queue_ptr, &wait);

	while (1) {

		set_current_state(TASK_INTERRUPTIBLE);
		//		printk("--func:%s line:%d ret:%d dma_id:%d --  \n",__FUNCTION__,__LINE__,ret,dma_id);

		ret = xport_dma_half_empty_check(dma_id, chl_id);

		if (ret == -EFAULT)
		{
			//			printk("--func:%s line:%d ret:%d dma_id:%d --  \n",__FUNCTION__,__LINE__,ret,dma_id);
			ret = 0;
		}
		else
		{

			ret = ( chl_type!=3 ? fn(buffer, len, chl_id, packet_size):
					fn(buffer, len, xport_dev->dev_minor-XPORT_MINOR_CHL(0), packet_size) );
		}


		if (ret == -EFAULT || ret == len)
			break;

		else if (file->f_flags & O_NONBLOCK)
			break;

		else if (ret > 0 && ret < len) {
			buffer += ret;
			len -= ret;
		}

		if (signal_pending(current))
		{
			ret = -EFAULT;
			break;
		}

		schedule();
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(xport_dev->irq_wait_queue_ptr, &wait);

	return ret;
}


static inline int uncached_access(struct file *file, unsigned long addr)
{
	/*
	 * Accessing memory above the top the kernel knows about or through a file pointer
	 * that was marked O_SYNC will be done non-cached.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	return addr >= __pa(high_memory);
}

static int xport_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if defined(__HAVE_PHYS_MEM_ACCESS_PROT)
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	vma->vm_page_prot = phys_mem_access_prot(file, offset,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);
#elif defined(pgprot_noncached)
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	int uncached;

	uncached = uncached_access(filp, offset);
	if (uncached)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	if (remap_pfn_range(vma,
				vma->vm_start,
				vma->vm_pgoff,
				vma->vm_end-vma->vm_start,
				vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}


static unsigned int xport_poll(struct file *file, poll_table * wait)
{


	unsigned int filter_index;
	XPORT_DEV *xport_dev = file->private_data;

	poll_wait(file, xport_dev->irq_wait_queue_ptr, wait);
	poll_wait(file, &crc_wait_queue, wait);

	if(xport_dev->dev_minor == XPORT_MINOR_CHL(0))
	{
		if (xport_dma_input_check(0))
			return POLLOUT | POLLWRNORM;
	}
	else if(xport_dev->dev_minor == XPORT_MINOR_CHL(1))
	{
		if (xport_dma_input_check(1))
			return POLLOUT | POLLWRNORM;
	}

	else if ((xport_dev->dev_minor >= XPORT_MINOR_FT_BASE)
			&& (xport_dev->dev_minor < (XPORT_MINOR_FT_BASE + MAX_FILTER_NUM))) {
		filter_index = xport_dev->dev_minor - XPORT_MINOR_FT_BASE;

		if (xport_dev->filter_type == FILTER_TYPE_NUKOWN)
			return 0;

		else if (xport_dev->filter_type == FILTER_TYPE_SECTION) {
			if (0x0000ffff != crc_err_idx)
			{
				last_crc_error_idx = crc_err_idx;
				crc_err_idx = 0x0000ffff;

				return POLLPRI;
			}

			if (xport_filter_check_section_number(filter_index) > 0)
				return POLLIN | POLLRDNORM;
		}

		else if (xport_dev->filter_type == FILTER_TYPE_PES) {
			if (xport_filter_check_pes_size(filter_index) > 0)
				return POLLIN | POLLRDNORM;
		}

		else {
			if (xport_filter_check_data_size(filter_index) > 0)
				return POLLIN | POLLRDNORM;
		}
	}

	return 0;
}


typedef struct __ioctl_params__
{
	unsigned int pid_idx;
	unsigned int pid_en;
	unsigned int pid_val;
	unsigned int pid_chl;
	unsigned int des_idx;
	unsigned int des_en;
	unsigned int des_len;
	unsigned int des_key[6];

	unsigned int avout_idx;
	unsigned int avout_en;
	unsigned int avout_pid;
	unsigned int avout_mode;
	unsigned int vid_low_latency;
	unsigned int avout_chl_switch;

	unsigned int filter_idx;
	unsigned int filter_en;
	unsigned int filter_crc_idx;
	unsigned int filter_crc_en;
	unsigned int filter_crc_save;
	unsigned int filter_crc_notify_en;

	unsigned int pcr_idx;
	unsigned int pcr_en;
	unsigned int pcr_pid;
	unsigned int pcr_hi_val;
	unsigned int pcr_lo_val;
} xport_ioctl_params;

/*here may integrate single set filter action into one seriel action?*/
static int xport_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int addr = 0;
	unsigned int regs_val = 0;
	unsigned int tmp_val = 0;
	unsigned int mask_val = 0;
	unsigned int filter_index = 0;
	unsigned int filter_data =0;
	unsigned char filter_mask[24];
	XPORT_DEV *xport_dev = file->private_data;
	unsigned int mnr =	xport_dev->dev_minor;
	xport_ioctl_params ioc_params = {0};

	/*filter operation*/
	if(mnr>=XPORT_MINOR_FT_BASE && mnr<(XPORT_MINOR_FT_BASE+MAX_FILTER_NUM))
	{
		filter_index = xport_dev->dev_minor - XPORT_MINOR_FT_BASE;

		switch (cmd) {
			case XPORT_FILTER_IOC_PID0:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 0);
				break;

			case XPORT_FILTER_IOC_PID1:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 1);
				break;

			case XPORT_FILTER_IOC_PID2:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 2);
				break;

			case XPORT_FILTER_IOC_PID3:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 3);
				break;

			case XPORT_FILTER_IOC_PID4:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 4);
				break;

			case XPORT_FILTER_IOC_PID5:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 5);
				break;

			case XPORT_FILTER_IOC_PID6:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 6);
				break;

			case XPORT_FILTER_IOC_PID7:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 7);
				break;

			case XPORT_FILTER_IOC_PID8:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 8);
				break;

			case XPORT_FILTER_IOC_PID9:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 9);
				break;

			case XPORT_FILTER_IOC_PID10:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 10);
				break;

			case XPORT_FILTER_IOC_PID11:
				__get_user(regs_val, (int __user *) arg);
				xport_filter_set_pidx(filter_index, regs_val, 11);
				break;

			case XPORT_FILTER_IOC_FILTER:
				copy_from_user(filter_mask, (int __user *) arg, 24);
				xport_filter_set_filter(filter_index, filter_mask, filter_mask + 12);
				break;

			case XPORT_FILTER_IOC_TYPE:
				__get_user(regs_val, (int __user *) arg);
				if (regs_val == FILTER_TYPE_SECTION || regs_val == FILTER_TYPE_TS || regs_val == FILTER_TYPE_PES
						|| regs_val == FILTER_TYPE_ES)
				{
					//printk(" FILTER_TYPE:[%d]  line:%d  \n",regs_val,__LINE__);
					if (xport_filter_set_type(filter_index, regs_val) == 0)
						xport_dev->filter_type = regs_val;
				}
				break;

			case XPORT_FILTER_IOC_ENABLE:
				__get_user(regs_val, (int __user *) arg);
				if (xport_dev->filter_type == FILTER_TYPE_SECTION
						|| xport_dev->filter_type == FILTER_TYPE_TS
						|| xport_dev->filter_type == FILTER_TYPE_PES || xport_dev->filter_type == FILTER_TYPE_ES) {
					if (regs_val)
						xport_filter_enable(filter_index, &(xport_dev->spin_lock));
					else
					{
						xport_filter_disable(filter_index, &(xport_dev->spin_lock));
					}

					xport_filter_clear_buffer(filter_index);
				}
				break;

			case XPORT_FILTER_IOC_QUERY_NUM:
				if (xport_dev->filter_type == FILTER_TYPE_SECTION) {
					regs_val = xport_filter_check_section_number(filter_index);
				}
				else if (xport_dev->filter_type == FILTER_TYPE_PES) {
					regs_val = xport_filter_check_pes_size(filter_index);
				}
				else if (xport_dev->filter_type == FILTER_TYPE_TS || xport_dev->filter_type == FILTER_TYPE_ES) {
					regs_val = xport_filter_check_data_size(filter_index);
				}
				else
					regs_val = 0;

				__put_user(regs_val, (int __user *) arg);

				break;

			case XPORT_FILTER_IOC_QUERY_SIZE:
				if (xport_dev->filter_type == FILTER_TYPE_SECTION)
					regs_val = xport_filter_check_section_size(filter_index);
				else
					regs_val = 0;

				__put_user(regs_val, (int __user *) arg);

				break;

			case XPORT_FILTER_IOC_CRC_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				tmp_val  = ioc_params.filter_crc_en << 7;
				tmp_val |= ioc_params.filter_crc_save << 6;
				tmp_val |= ioc_params.filter_crc_idx;
				/* the section filter index ~(0,127) but former*/
				xport_mips_write(MIPS_CHL_CRC_EN((filter_index+16)), tmp_val);
				break;

			case XPORT_FILTER_IOC_CRC_NTF_ENB:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				tmp_val  = ioc_params.filter_crc_notify_en << 31;
				xport_mips_write(MIPS_CHL_CRC_NOTIFY_EN((filter_index+16)), tmp_val);
				break;

			case XPORT_FILTER_IOC_SAVE_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				xport_mips_read(MIPS_CHL_CRC_EN((filter_index+16)), &tmp_val);
				tmp_val |= ioc_params.filter_crc_save << 6;
				xport_mips_write(MIPS_CHL_CRC_EN((filter_index+16)), tmp_val); /* the same reason, see the above */
				break;

			case XPORT_FILTER_IOC_SWITCH:
				xport_mips_write(MIPS_CHL_SWITCH((filter_index + 16)), (int)arg);
				break;

				/*get filter buffer end addr*/
			case XPORT_FILTER_IOC_BUFFER_UPADDR:
				if ((void *)arg != NULL){
					filter_data=get_filter_buf_top_addr(filter_index);
					copy_to_user((void __user *)arg, &filter_data, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

				/*get filter buffer start addr*/
			case XPORT_FILTER_IOC_BUFFER_LOWADDR:
				if ((void *)arg != NULL){
					filter_data=get_filter_buf_base_addr(filter_index);
					copy_to_user((void __user *)arg, &filter_data, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

			case XPORT_FILTER_IOC_BUFFER_RP_ADDR:
				if ((void *)arg != NULL){
					filter_data=get_xport_filter_rp(filter_index);
					copy_to_user((void __user *)arg, &filter_data, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

			case XPORT_FILTER_IOC_BUFFER_WP_ADDR:
				if ((void *)arg != NULL){
					filter_data=get_xport_filter_wp(filter_index);
					copy_to_user((void __user *)arg, &filter_data, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

			case XPORT_DMA_IOC_ENABLE:
				if((void *)arg != NULL)
				{
					//__get_user(regs_val, (int __user *) arg);
					regs_val = xport_dma_enable(0);
					regs_val = xport_readl(DMA_INPUT0_HEAD_ADDR);
					//printk("dma addr regs_val:[%04x]  line:%d  \n",regs_val,__LINE__);
					copy_to_user((void __user *)arg,&regs_val,sizeof(unsigned int));
				}
				break;

			case XPORT_FLT_IOC_CLEAR:
				/*get sec filter number */
				xport_filter_clear_buffer(filter_index);
				memset((char *)get_filter_buf_base_addr(filter_index),0,FLT_BUF_SIZE(filter_index));
				break;
		}

		return 0;
	}

	/*send or get cmd*/
	else if (__IS_HW_ADDR__(cmd))
	{
		if (__WR_FLAGS__(cmd))
		{
			regs_val = arg;
			xport_writel(cmd, regs_val);
		}
		else {/*kernel to usr space*/
			regs_val = xport_readl(cmd);
			__put_user(regs_val, (int __user *) arg);
		}

		return 0;
	}
	else if (__IS_MIPS_ADDR__(cmd))
	{
		if (__WR_FLAGS__(cmd))
		{
			regs_val = arg;
			if (xport_mips_write(__OFFSET_ADDR__(cmd), regs_val))
				return -EINVAL;
		}
		else {
			if (xport_mips_read(__OFFSET_ADDR__(cmd), &regs_val))
				return -EINVAL;
			__put_user(regs_val, (int __user *) arg);\
		}

		return 0;
	}

	else {
		int i = 0;

		switch (cmd)
		{
			case XPORT_PIDFT_IOC_ENABLE:
				/*get usr cmd from mips by*/
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				tmp_val = xport_readl(__PID_FILTER__(ioc_params.pid_idx));
				tmp_val = ioc_params.pid_en!=0 ? xport_set_bit(tmp_val,31): xport_cls_bit(tmp_val,31);
				addr =  ioc_params.pid_idx< 64 ?PID_GRP0_FLTR(ioc_params.pid_idx):
					PID_GRP1_FLTR(ioc_params.pid_idx);
				xport_writel(addr, tmp_val);
				break;


			case XPORT_PIDFT_IOC_CHANNEL:
				/*operation to channel by write regs*/
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				tmp_val = xport_readl(__PID_FILTER__(ioc_params.pid_idx));
				tmp_val = ioc_params.pid_chl%2 ? xport_set_bit(tmp_val,30): xport_cls_bit(tmp_val,30);
				addr =  ioc_params.pid_idx<64?PID_GRP0_FLTR(ioc_params.pid_idx):
					PID_GRP1_FLTR(ioc_params.pid_idx);
				xport_writel(addr, tmp_val);
				break;


			case XPORT_PIDFT_IOC_PIDVAL:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				//printk("get ioctl pid val:[%04x] function:%s line:%d \n",
				//ioc_params.pid_idx,__FUNCTION__,__LINE__);

				tmp_val = xport_readl(__PID_FILTER__(ioc_params.pid_idx));
				tmp_val &= ~0x1fff;
				tmp_val |= ioc_params.pid_val;
				addr =  ioc_params.pid_idx<64?PID_GRP0_FLTR(ioc_params.pid_idx):
					PID_GRP1_FLTR(ioc_params.pid_idx);
				xport_writel(addr, tmp_val);

				break;


			case XPORT_PIDFT_IOC_DESC_ODDKEY:

				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
#if 0
				for (i = 0; i < ioc_params.des_len; i++)
				{
					xport_writel(__DSCRB_ODD_ADDR_(i,ioc_params.des_idx),
							ioc_params.des_key[i]);
				}
#else
				{
					unsigned char key[9];
					if (ioc_params.des_len != 2)
					{
						printk("length of odd key is %d\n", ioc_params.des_len);
					}
					else
					{
						key[0] = (unsigned char)((ioc_params.des_key[1] >> 24) & 0xff);
						key[1] = (unsigned char)((ioc_params.des_key[1] >> 16) & 0xff);
						key[2] = (unsigned char)((ioc_params.des_key[1] >> 8) & 0xff);
						key[3] = (unsigned char)(ioc_params.des_key[1] & 0xff);
						key[4] = (unsigned char)((ioc_params.des_key[0] >> 24) & 0xff);
						key[5] = (unsigned char)((ioc_params.des_key[0] >> 16) & 0xff);
						key[6] = (unsigned char)((ioc_params.des_key[0] >> 8) & 0xff);
						key[7] = (unsigned char)(ioc_params.des_key[0] & 0xff);
						key[8] = ioc_params.des_idx * 2;
#if 0
						printk("odd key of des %d is :\n", ioc_params.des_idx);
						for(i = 0; i < 9; i++)
						{
							printk("0x%x,", key[i]);
						}
						printk("\n");
#endif
						if (0 > desckey_write(0x80000004, key, 8))
						{
							printk("failed to write desc key.\n");
						}
					}
				}
#endif
				break;

			case XPORT_PIDFT_IOC_DESC_EVENKEY:

				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
#if 0
				for (i = 0; i < ioc_params.des_len; i++)
					xport_writel(__DSCRB_EVN_ADDR_(i,ioc_params.des_idx),
							ioc_params.des_key[i]);
#else
				{
					unsigned char key[9];
					if (ioc_params.des_len != 2)
					{
						printk("length of even key is %d\n", ioc_params.des_len);
					}
					else
					{
						key[0] = (unsigned char)((ioc_params.des_key[1] >> 24) & 0xff);
						key[1] = (unsigned char)((ioc_params.des_key[1] >> 16) & 0xff);
						key[2] = (unsigned char)((ioc_params.des_key[1] >> 8) & 0xff);
						key[3] = (unsigned char)(ioc_params.des_key[1] & 0xff);
						key[4] = (unsigned char)((ioc_params.des_key[0] >> 24) & 0xff);
						key[5] = (unsigned char)((ioc_params.des_key[0] >> 16) & 0xff);
						key[6] = (unsigned char)((ioc_params.des_key[0] >> 8) & 0xff);
						key[7] = (unsigned char)(ioc_params.des_key[0] & 0xff);
						key[8] = ioc_params.des_idx * 2 + 1;
#if 0
						printk("even key of des %d is :\n", ioc_params.des_idx);
						for(i = 0; i < 9; i++)
						{
							printk("0x%x,", key[i]);
						}
						printk("\n");
#endif
						if (0 > desckey_write(0x80000004, key, 8))
						{
							printk("failed to write desc key.\n");
						}
					}
				}
#endif
				break;

			case XPORT_PIDFT_IOC_DESC_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				/* write bit26~29 of PID_FILTERx. */
				/* bit29: enable / disable DES */
				/* bit26~28: DES id */
				// regs_val = i;
				// regs_val |= 0x80000000; /* bit31 indicates enabling flags, 1 - enable, 0 = disable. */
				// regs_val |= (pidft_ptr->pid_filter_id << 30);

				addr =  ioc_params.pid_idx<=63?PID_GRP0_FLTR(ioc_params.pid_idx):
					PID_GRP1_FLTR(ioc_params.pid_idx);
				tmp_val = xport_readl(addr);
				tmp_val &= ~0x3c000000;

				if (ioc_params.des_en) {
					tmp_val |= 0x20000000;
					tmp_val |= (ioc_params.des_idx << 26);
				}
				xport_writel(addr, tmp_val);
				break;


			case XPORT_CHL_IOC_CLEAR:

				regs_val = (xport_dev->dev_minor-XPORT_MINOR_CHL(0)) ;
				xport_writel(IN_CHL_RP_ADDR(regs_val), 0x40000000);
				xport_writel(XPORT_CHL_DMA_WP_ADDR(regs_val),0);
				memset(xport_mem_base+(IN_CHL_BASE_DEF(regs_val)-XPORT_MEM_BASE),0,
						IN_CHL_UNIT_SIZE_DEF(regs_val)*IN_CHL_UNIT_NUM_DEF(regs_val));
				break;


				/*enable channel for tuner*/
			case XPORT_CHL_IOC_ENABLE:

				regs_val = (unsigned int) arg;
				//printk("func:%s line:%d \n",__FUNCTION__,__LINE__);

				if(xport_dev->dev_minor >= XPORT_MINOR_CHL(0) &&
						xport_dev->dev_minor < XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM))
				{
					tmp_val = xport_readl(IN_CHL_CFG_ADDR(xport_dev->dev_minor-XPORT_MINOR_CHL(0)));
				}
				else
				{
					//printk("func:%s line:%d wrong input device ! minor:[0x%08x] --\n",
					//__FUNCTION__,__LINE__,xport_dev->dev_minor);
					break;
				}

				tmp_val = (regs_val?(tmp_val|0x80000000):(tmp_val&~0x80000000));
				xport_writel(IN_CHL_CFG_ADDR(xport_dev->dev_minor-XPORT_MINOR_CHL(0)),tmp_val);

				mask_val = 0x1 << (mnr - XPORT_MINOR_CHL(0));
				tmp_val = xport_readl(XPORT_TUNER_EN);
				tmp_val = regs_val>0 ? tmp_val|mask_val : tmp_val&~mask_val;
				xport_writel(XPORT_TUNER_EN, tmp_val);

				break;

			case XPORT_CHL_IOC_DES_MODE:
				regs_val = (unsigned int) arg;
				tmp_val = xport_readl(XPORT_CFG_ADDR0);
				/*set io to alter to descriptor mode*/
				mnr = (mnr==XPORT_MINOR_CHL(0)?0:1);
				tmp_val = regs_val==0 ? tmp_val&mnr : tmp_val|mnr ;
				mnr = 0;
				xport_writel(XPORT_CFG_ADDR0, tmp_val);//?

				break;

			case XPORT_CHL_IOC_SERIAL_MODE:
				// 0 for MSB, 1 for LSB
				regs_val = (unsigned int) arg;
				tmp_val = xport_readl(XPORT_CFG_ADDR0);
				mnr = (mnr - XPORT_MINOR_CHL(0));
				tmp_val = (regs_val==0 ? tmp_val&~(8<<mnr*8) : tmp_val|(8<<mnr*8));
				mnr = 0;
				xport_writel(XPORT_CFG_ADDR0, tmp_val);

				break;


			case XPORT_CHL_IOC_TUNER_MODE:

				regs_val = (unsigned int) arg;
				if (regs_val == 1)  // serial mode
				{
					pinmux_set_xportmode(_xport_serial_mode);
				}
				else if (regs_val == 0)
				{
					pinmux_set_xportmode(_xport_paralell_mode);
				}

				tmp_val = xport_readl(XPORT_CFG_ADDR0);
				mnr = (mnr-XPORT_MINOR_CHL(0));
				//0 for Par.1 for Ser.
				tmp_val = (regs_val==0 ? tmp_val&~(4<<mnr*8) : tmp_val|(4<<mnr*8)) ;
				tmp_val = (regs_val==0 ? tmp_val : tmp_val|(0x10<<mnr*8)) ;
				mnr = 0;
				xport_writel(XPORT_CFG_ADDR0, tmp_val);

				break;

			case XPORT_CHL_IOC_INPUT_MODE:

				regs_val = (unsigned int) arg;
				tmp_val = xport_readl(XPORT_CFG_ADDR0);
				mask_val = (mnr-XPORT_MINOR_CHL(0))==0 ? 0x1 : 0x100;

				tmp_val = regs_val? tmp_val|mask_val: tmp_val&~mask_val ;
				xport_writel(XPORT_CFG_ADDR0, tmp_val);
				tmp_val = xport_readl(IN_CHL_CFG_ADDR(xport_dev->dev_minor-XPORT_MINOR_CHL(0)));
				tmp_val|= 0x80000000;

				switch (regs_val) {
					case 0:	/* TUNER */
						break;

					case 1:	/* DMA */
						tmp_val &= ~0x20000000;
						tmp_val |= 0x40000000;
						break;

					case 2:	/* DIR DMA */
						tmp_val |= 0x60000000;
						break;

					default:
						break;
				}

				xport_writel(IN_CHL_CFG_ADDR(mnr-XPORT_MINOR_CHL(0)),tmp_val);
				/*
				 * enable interrupt, according the following flow:
				 * INT0_ENB_ADDR | 0x11, for DEMUX_INPUT_MOD_TUNER.
				 * INT0_ENB_ADDR | 0x11, for DMA_MOD / CHL0.
				 * INT0_ENB_ADDR | 0x22, for DMA_MOD / CHL1.
				 */
				tmp_val = xport_readl(XPORT_INT_ENB_ADDR0);

				switch (regs_val)
				{
					case 0:	/* tuner */
						if(mnr == XPORT_MINOR_CHL(0))
						{tmp_val &= ~(XPORT_IRQ0_DMA0_EMPTY_MSK);}
						/* WARNING: this parameters are undocumented, pls don't change it. */
						if(mnr == XPORT_MINOR_CHL(1))
							tmp_val &= ~(XPORT_IRQ0_DMA1_EMPTY_MSK);
						/* WARNING: this parameters are undocumented, pls don't change it. */
						break;

					case 1:	/* DMA */
					case 2:	/* direct */
						if(mnr == XPORT_MINOR_CHL(0)) 		//?
							tmp_val |= (XPORT_IRQ0_DMA0_EMPTY_MSK);
						if(mnr == XPORT_MINOR_CHL(1))
							tmp_val |= (XPORT_IRQ0_DMA1_EMPTY_MSK);
						break;
				}
				xport_writel(XPORT_INT_ENB_ADDR0, tmp_val);
				break;


			case XPORT_CHL_IOC_RESET:
				break;

			case XPORT_CHL_IOC_DMA_RESET:

				xport_dma_reset(arg);//?

				break;

			case XPORT_CHL_IOC_INPUT_TYPE:
				__get_user(regs_val, (int __user *) arg);
				xport_dev->stream_type = regs_val;
				printk(KERN_INFO"stream type = %d\n", regs_val);
				switch(regs_val)
				{
					case XPORT_INPUT_M2TS_TYPE:
						xport_dev->packet_size = TS_SIZE + 4;
						break;
					case XPORT_INPUT_TS_TYPE:
						xport_dev->packet_size = TS_SIZE;
						break;
					case XPORT_INPUT_TS204_TYPE:
						xport_dev->packet_size = TS_SIZE + 16;
						break;
					default:
						printk(KERN_ERR"unsupported input stream (only m2ts, ts, ts204 supported\n");
						return -EFAULT;
				}
				printk(KERN_INFO"set input type for demux, packet size = %d\n", xport_dev->packet_size);
				if ((xport_dev->dev_minor<XPORT_MINOR_CHL(0)) ||
						(xport_dev->dev_minor>=XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM)))
				{
					printk(KERN_ERR"invalid xport channel handler\n");
					return -EFAULT;
				}
				tmp_val = xport_dev->dev_minor - XPORT_MINOR_CHL(0);
				if (xport_mips_write(MIPS_INCHL_TYPE | (tmp_val << 8), regs_val) < 0)
				{
					printk(KERN_ERR"xport mips write XPORT_CHL_IOC_INPUT_TYPE failed\n");
					return -EFAULT;
				}
				break;
			case XPORT_VID_IOC_OUTPUT_MODE:
				break;

			case XPORT_VID_IOC_RESET:
				break;

			case XPORT_VID_IOC_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				if (ioc_params.avout_en)
				{
					tmp_val = 0x80000000; /* enable flags */
					tmp_val |= ioc_params.avout_mode << 7; /* output mode, bit7 */
					tmp_val |= ioc_params.vid_low_latency << 3; /* low latency, bit3 */
					tmp_val |= 0x00000002; /* output type, ES */

					xport_mips_write(MIPS_CHL_EN(ioc_params.avout_idx), tmp_val);
				}
				else
					xport_mips_write(MIPS_CHL_DISABLE(ioc_params.avout_idx), 0);

				break;

			case XPORT_VID_IOC_PIDVAL:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				//printk("[lixun_kernel] xport_vid_ioc_pidval, pid = 0x%08x, avout_id 0x%x\n", ioc_params.avout_pid,ioc_params.avout_idx);
				xport_mips_write(MIPS_CHL_PID(0,ioc_params.avout_idx), ioc_params.avout_pid);

				break;

			case XPORT_AUD_IOC_OUTPUT_MODE:
				break;

			case XPORT_AUD_IOC_RESET:
				break;

			case XPORT_AUD_IOC_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				if (ioc_params.avout_en)
				{
					tmp_val  = 0x80000000; /* enable flags */
					tmp_val |= ioc_params.avout_mode << 7; /* output mode, bit7 */
					tmp_val |= 0x00000002; /* output type, ES */

					xport_mips_write(MIPS_CHL_EN(ioc_params.avout_idx), tmp_val);
				}
				else
					xport_mips_write(MIPS_CHL_DISABLE(ioc_params.avout_idx), 0);

				break;

			case XPORT_AUD_IOC_PIDVAL:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				//printk("[lixun_kernel] xport_aud_ioc_pidval, pid = 0x%08x, avout_id 0x%x\n", ioc_params.avout_pid,ioc_params.avout_idx);
				xport_mips_write(MIPS_CHL_PID(0,ioc_params.avout_idx), ioc_params.avout_pid);

				break;

			case XPORT_PCR_IOC_ENABLE:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				xport_mips_read(MIPS_PCR_PID(ioc_params.pcr_idx), &tmp_val);
				tmp_val &= ~0x80000000;
				tmp_val |= (ioc_params.pcr_en << 31);
				xport_mips_write(MIPS_PCR_PID(ioc_params.pcr_idx), tmp_val);

				break;

			case XPORT_PCR_IOC_GETVAL:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				xport_mips_read_ex(MIPS_PCR_GET(ioc_params.pcr_idx), &ioc_params.pcr_hi_val, &ioc_params.pcr_lo_val);
				copy_to_user((void*)arg, &ioc_params, sizeof(xport_ioctl_params));

				break;

			case XPORT_PCR_IOC_PIDVAL:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));

				xport_mips_read(MIPS_PCR_PID(ioc_params.pcr_idx), &tmp_val);
				tmp_val &= ~0x1fff;
				tmp_val |= ioc_params.pcr_pid;
				xport_mips_write(MIPS_PCR_PID(ioc_params.pcr_idx), tmp_val);

				break;

			case XPORT_FW_INIT:
				//			printk("load fw ... func:%s line:%d  \n",__FUNCTION__,__LINE__);
				xport_load_firmware();
				xport_config_buf();
				//			printk("load fw suc  func:%s line:%d  \n",__FUNCTION__,__LINE__);
				break;

			case XPORT_VID_IOC_SWITCH:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				xport_mips_write(MIPS_CHL_SWITCH(ioc_params.avout_idx), ioc_params.avout_chl_switch);
				break;

			case XPORT_AUD_IOC_SWITCH:
				copy_from_user(&ioc_params, (void*)arg, sizeof(xport_ioctl_params));
				xport_mips_write(MIPS_CHL_SWITCH(ioc_params.avout_idx/*only one audio output*/), ioc_params.avout_chl_switch);
				break;

			case XPORT_IOC_MEM_BASE_PHYADDR:
				if (NULL != (void *)arg){
					tmp_val = XPORT_MEM_BASE;
					copy_to_user((void __user *)arg, &tmp_val, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

			case XPORT_IOC_MEM_SIZE:
				if (NULL != (void *)arg){
					tmp_val = XPORT_MEM_SIZE;
					copy_to_user((void __user *)arg, &tmp_val, sizeof(unsigned int));
				}
				else
					return -EINVAL;

				break;
			case XPORT_CHL_IOC_UNIT_NUM_DEF:

				if (NULL != (void *)arg)
				{
					if(xport_dev->dev_minor >= XPORT_MINOR_CHL(0) &&
							xport_dev->dev_minor < XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM) )
					{tmp_val = IN_CHL_UNIT_NUM_DEF(mnr-XPORT_MINOR_CHL(0));}

					else
						return -EINVAL;
					copy_to_user((void __user *)arg, &tmp_val, sizeof(unsigned int));
				}

				else
					return -EINVAL;

				break;

			case XPORT_CHL_IOC_UNIT_SIZE_DEF:

				if (NULL != (void *)arg)
				{

					if(xport_dev->dev_minor >= XPORT_MINOR_CHL(0) &&
							xport_dev->dev_minor < XPORT_MINOR_CHL(XPORT_MAX_CHL_NUM) )
					{tmp_val = IN_CHL_UNIT_SIZE_DEF(mnr-XPORT_MINOR_CHL(0));}

					else
						return -EINVAL;
					copy_to_user((void __user *)arg, &tmp_val, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;


			case XPORT_CHL_IOC_MIN_SPACES:
				if (NULL != (void *)arg){
					if (xport_dev->dev_minor == XPORT_MINOR_CHL(0))
						tmp_val = XPORT_CHL0_MIN_SPACES;
					else if (xport_dev->dev_minor == XPORT_MINOR_CHL(1))
						tmp_val = XPORT_CHL1_MIN_SPACES;
					else
						return -EINVAL;
					copy_to_user((void __user *)arg, &tmp_val, sizeof(unsigned int));
				}
				else
					return -EINVAL;
				break;

			case XPORT_TUN_IOC_ENABLE:
				if(arg!= 1 && arg!= 0)
				{
					return -EINVAL;
				}

				tmp_val = xport_readl(XPORT_TUNER_EN);
				xport_writel(XPORT_TUNER_EN, xport_set_bit(tmp_val,arg));
				break;

			case XPORT_TUN_IOC_DISABLE:
				if(arg!= 1 && arg!= 0)
				{
					return -EINVAL;
				}

				tmp_val = xport_readl(XPORT_TUNER_EN);
				xport_writel(XPORT_TUNER_EN, xport_cls_bit(tmp_val,arg));
				break;

			case XPORT_IOC_DMA0_LIST0_HEAD_ADDR:
				__put_user(DMA0_LIST0_HEAD_ADDR, (int __user *)arg);
				break;
			case XPORT_IOC_DMA0_BUF0_BASE_ADDR:
				__put_user(DMA0_BUF0_BASE_ADDR, (int __user *)arg);
				break;
			case XPORT_IOC_DMA0_LIST1_HEAD_ADDR:
				__put_user(DMA0_LIST1_HEAD_ADDR, (int __user *)arg);
				break;
			case XPORT_IOC_DMA0_BUF1_BASE_ADDR:
				__put_user(DMA0_BUF1_BASE_ADDR, (int __user *)arg);
				break;
			case XPORT_IOC_DMA0_MAX_BLOCK_SIZE:
				__put_user(DMA0_MAX_BLOCK_SIZE, (int __user *)arg);
				break;
			case XPORT_IOC_DMA0_MAX_BLOCK_NUM:
				__put_user(DMA0_MAX_BLOCK_NUM, (int __user *)arg);
				break;
		}

		return 0;
	}

	return -EINVAL;
}


static struct file_operations xport_fops = {
	.owner = THIS_MODULE,
	.open = xport_open,
	.release = xport_release,
	.read = xport_read,
	.write = xport_write,
	.poll = xport_poll,
	.ioctl = xport_ioctl,
	.mmap = xport_mmap,
};

static struct proc_dir_entry *xport_proc_entry = NULL;


static void xport_dump_regs(void)
{
	unsigned int chl_id;
	unsigned int reg_val;

	printk("==============> Hardware Register <=====================\n");

	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0008 << 2));
	printk("xport_cfg0         = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0009 << 2));
	printk("xport_cfg1         = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0018 << 2));
	printk("tuner enable       = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0011 << 2));
	printk("IRQ0 En            = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0015 << 2));
	printk("IRQ1 En            = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0010 << 2));
	printk("IRQ0 ST            = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0014 << 2));
	printk("IRQ1 ST            = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0040 << 2));
	printk("channel0 base addr = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0041 << 2));
	printk("channel0 cfg       = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0042 << 2));
	printk("channel0 rp        = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0043 << 2));
	printk("channel0 wp        = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0045 << 2));
	printk("channel0 st        = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0060 << 2));
	printk("pid filter0        = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0061 << 2));
	printk("pid filter1        = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0003 << 2));
	printk("Mail Box3          = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0004 << 2));
	printk("Mail Box4          = 0x%08x\n", reg_val);
	reg_val = xport_readl(PA_XPORT_REG_BASE + (0x0005 << 2));
	printk("Mail Box5          = 0x%08x\n", reg_val);

	printk("\n==============>   Mips Register   <=====================\n");
	for (chl_id = 0; chl_id < 6; chl_id++) {
		printk("==============> Output Channel %d <=====================\n", chl_id);

		xport_mips_read(MIPS_CHL_EN(chl_id), &reg_val);
		printk("channel en           = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_PID(0,chl_id), &reg_val);
		printk("channel PID0         = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_PID(1,chl_id), &reg_val);
		printk("channel PID1         = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_PID(2,chl_id), &reg_val);
		printk("channel PID2         = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_PID(3,chl_id), &reg_val);
		printk("channel PID3         = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_DIR_LOW_ADDR(chl_id), &reg_val);
		printk("channel DIR_LOW_ADRR = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_DIR_UP_ADDR(chl_id), &reg_val);
		printk("channel DIR_UP_ADRR  = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_WP(chl_id), &reg_val);
		printk("channel WP           = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_BUF_LOW_ADDR(chl_id), &reg_val);
		printk("channel BUF_LOW_ADRR = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_BUF_UP_ADDR(chl_id), &reg_val);
		printk("channel BUF_UP_ADRR  = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_ERR_PKT_CNT(chl_id), &reg_val);
		printk("channel ERR Packet   = 0x%08x\n", reg_val);

		xport_mips_read(MIPS_CHL_OUT_PKT_CNT(chl_id), &reg_val);
		printk("channel Out Packet   = 0x%08x\n", reg_val);
	}

	return;
}

static void xport_show_buffer_define(void)
{
	unsigned int it;
	printk("Xport Mem Base: 0x%x\n", XPORT_MEM_BASE);
	printk("Xport Mem Size: 0x%x\n", XPORT_SIZE);

	for (it = 0; it < 4; it++){
		printk("Xport CHL%d BASE: 0x%x\n", it, IN_CHL_BASE_DEF(it));
		printk("Xport CHL%d SIZE: 0x%x\n", it, IN_CHL_BUF_SIZE(it));
	}
	printk("Xport DMA0 Buf0 Base: 0x%x\n", DMA0_BUF0_BASE_ADDR);
	printk("Xport DMA0 Buf0 Size: 0x%x\n", DMA0_BUF0_SIZE);
	printk("Xport DMA0 Buf1 Base: 0x%x\n", DMA0_BUF1_BASE_ADDR);
	printk("Xport DMA0 Buf1 Size: 0x%x\n", DMA0_BUF1_SIZE);
	printk("Xport DMA0 List0 Head Base: 0x%x\n", DMA0_LIST0_HEAD_ADDR);
	printk("Xport DMA0 List0 Head Size: 0x%x\n", DMA_LIST_HEAD_SIZE);
	printk("Xport DMA0 List1 Head Base: 0x%x\n", DMA0_LIST1_HEAD_ADDR);
	printk("Xport DMA0 List1 Head Size: 0x%x\n", DMA_LIST_HEAD_SIZE);
	printk("Xport MIPS_EXTERNAL_BASE_ADDR: 0x%x\n", MIPS_EXTERNAL_BASE_ADDR);
	printk("Xport  MIPS_EXTERNAL_SIZE: 0x%x\n",  MIPS_EXTERNAL_SIZE);

	for (it = 0; it < MAX_FILTER_NUM ; it++){
		printk("Xport Filter%d Buffer BASE: 0x%x\n", it, FLT_BUF_BASE_ADDR(it));
		printk("Xport Filter%d Buffer SIZE: 0x%x\n", it, FLT_BUF_SIZE(it));
	}

}


/*achieve serial write operation of xport module*/
static int xport_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u32 addr;
	u32 val;

	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = xport_readl(addr);
		printk(" xport_readl [0x%08x] = 0x%08x \n", addr, val);
	}
	else if (strncmp("wl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = simple_strtol(&cmd_line[12], NULL, 16);
		xport_writel(addr, val);
	}
	else if (strncmp("mips_rl", cmd_line, 7) == 0) {
		addr = simple_strtol(&cmd_line[8], NULL, 16);
		xport_mips_read(addr, &val);
		printk(" xport_readl [0x%08x] = 0x%08x \n", addr, val);
	}
	else if (strncmp("mips_wl", cmd_line, 7) == 0) {
		addr = simple_strtol(&cmd_line[8], NULL, 16);
		val = simple_strtol(&cmd_line[13], NULL, 16);
		xport_mips_write(addr, val);
	}
	else if (strncmp("dumpall", cmd_line, 7) == 0)
		xport_dump_regs();
	else if (strncmp("loadfw", cmd_line, 6) == 0)
		xport_load_firmware();
	else if (strncmp("allocbufs", cmd_line, 9) == 0)
		xport_config_buf();
	else if (strncmp("clearpids", cmd_line, 9) == 0)
	{
		int i;
		for (i = 0; i < 64; i ++)
			xport_writel(__PID_FILTER__(i), 0);
	}
	else if (strncmp("crcerr", cmd_line, 6) == 0)
		printk(" last_crc_error_idx = %d \n", last_crc_error_idx);
	else if (strncmp("getpcr", cmd_line, 6) == 0)
	{
		unsigned int pcr_hi, pcr_lo;
		xport_mips_write(MIPS_PCR_PID(0), 0x80000000);
		xport_mips_read_ex(MIPS_PCR_GET(0), &pcr_hi, &pcr_lo);
		printk(" pcr = %08x:%08x \n", pcr_hi, pcr_lo);
	}
	else if (strncmp("checkbufs", cmd_line, 9) == 0){
		xport_show_buffer_define();

	}
	else {
		printk("\nonly following commands are supported: \n");
		printk(" rl \n");
		printk(" wl \n");
		printk(" mips_rl \n");
		printk(" mips_wl \n");
		printk(" dumpall \n");
		printk(" allocbufs \n");
		printk(" loadfw \n");
		printk(" getpcr \n");
		printk(" clearpids \n");
		printk(" crcerr \n");
		printk(" checkbufs \n");
	}

	return count;
}

static void xport_setup_dev(dev_t xport_dev, char * dev_name)
{

	/*
	   replace class_simple_device_add() in pre version
	   by ZJ. 1st Apr 2010
	   */
	device_create(cnc180x_xport_class,NULL,xport_dev,NULL,dev_name);
	/*NULL might be alternated by some data array*/
}


static int __init xport_init(void)
{
	int i;
	char devname[100];

	printk("xport_init.\n");

	//(void)signal(SIGINT, &xport_signal_handler_clear);
	//signal(SIGINT,(xport_signal_handler_clear)) ;


	sec_mb_base = ioremap(CNC_SEC_MB_BASE, CNC_SEC_MB_SIZE);
	if (NULL == sec_mb_base) {
		goto outerr7;
	}

	xport_regs_base = ( __iomem char *)VA_XPORT_BASE;
	if (NULL == xport_regs_base) {
		goto outerr6;
	}

	if (NULL == request_mem_region(XPORT_MEM_BASE, XPORT_MEM_SIZE, "CS Xport Memory space")) {
		goto outerr5;
	}
	printk("xport region:0x%08x, used mem:0x%x, baseaddress:0x%x\n",XPORT_MEM_BASE, XPORT_MEM_USED_SIZE, xport_regs_base);

	xport_mem_base = ioremap(XPORT_MEM_BASE, XPORT_MEM_SIZE);
	//printk("xport mem base:[%04x] line:%d \n",xport_mem_base,__LINE__);

	if (NULL == xport_mem_base) {
		goto outerr4;
	}

	for (i = 0; i < MAX_FILTER_NUM; i++)
		init_waitqueue_head(&filter_wait_queue[i]);


	if (register_chrdev(XPORT_MAJOR, "cs_xport", &xport_fops)){
		goto outerr3;
	}
	/*
	   replace class_simple_create() in pre version
	   by ZJ .1st Apr 2010
	   */
	cnc180x_xport_class = class_create(THIS_MODULE,"cnc_xport");
	if (IS_ERR(cnc180x_xport_class)){
		printk(KERN_ERR "XPORT: class create failed.\n");
		goto outerr2;
		// return -EIO;
	}

	//create a platform device aim to obtain a device structure
	xport_pdev = platform_device_register_simple("xport_device", 0, NULL, 0);
	if (IS_ERR(xport_pdev)) {
		printk(KERN_ERR "XPORT: Failed to register device for \"%s\"\n",
				"xport");
		goto outerr1;
		//  return -EINVAL;
	}

	xport_setup_dev(MKDEV(XPORT_MAJOR, XPORT_MINOR), "cnc_xport");

	for(i = 0 ; i < XPORT_MAX_CHL_NUM; i++)
	{
		memset(devname,0,100);
		sprintf(devname,"channel_%d",i);
		xport_setup_dev(MKDEV(XPORT_MAJOR, XPORT_MINOR_CHL(i)),devname);
	}

	for (i = 0; i < MAX_FILTER_NUM; i++) {
		memset(devname,0,100);
		sprintf(devname, "xport_filter%d", i);
		xport_setup_dev(MKDEV(XPORT_MAJOR, XPORT_MINOR_FT_BASE + i), devname);
	}

	xport_dma_init();
	xport_hw_init();

	if (0 != request_irq(XPORT_IRQ0,(irq_handler_t)xport_irq0, IRQF_DISABLED, "cs_xport0", &irq0_dev_id)) {
		printk(KERN_ERR "XPORT: cannot register IRQ0 \n");
		goto outerr0;
		//	return -EIO;
	}

	if (0 != request_irq(XPORT_IRQ1, (irq_handler_t)xport_irq1, IRQF_DISABLED, "cs_xport1", &irq1_dev_id)) {
		printk(KERN_ERR "XPORT: cannot register IRQ1 \n");
		goto outerr0;
		//	return -EIO;
	}

	xport_proc_entry = create_proc_entry("xport_io", 0, NULL);
	if (NULL != xport_proc_entry) {
		xport_proc_entry->write_proc = &xport_proc_write;
	}

	// sema_init(&read_sem, 1);
	printk(KERN_INFO "Xport: Init OK [0x%08x]. \n", XPORT_MEM_BASE);
	return 0;

outerr0:
	platform_device_unregister(xport_pdev);
outerr1:  /*replace class_simple_destroy() in pre version . by ZhangJi . 1st Apr 2010*/
	class_destroy(cnc180x_xport_class);
outerr2:
	unregister_chrdev(XPORT_MAJOR, "cs_xport");
outerr3:
	iounmap(xport_mem_base);
outerr4:
	release_mem_region(XPORT_MEM_BASE, XPORT_MEM_SIZE);
outerr5:
outerr6:
	iounmap((void *)sec_mb_base);
outerr7:

	return -ENODEV;
}


static void __exit xport_exit(void)
{
	int i;
	char devname[100];

	for (i = 0; i < 48; i++)
	{
		sprintf(devname, "cs_xport/filter%d", i);
		devres_free(devname);
	}

	if (NULL != xport_proc_entry)
	{
		remove_proc_entry("xport_io", NULL);
	}

	/*
	   replace class_simple_device_remove()
	   by Zhang.Ji .1st Apr 2010
	   */
	device_destroy(cnc180x_xport_class,MKDEV(XPORT_MAJOR, XPORT_MINOR));
	for(i = 0 ; i < XPORT_MAX_CHL_NUM; i++)
	{
		device_destroy(cnc180x_xport_class,MKDEV(XPORT_MAJOR, XPORT_MINOR_CHL(i)));
	}

	for (i = 0; i < MAX_FILTER_NUM; i++)
	{
		device_destroy(cnc180x_xport_class,MKDEV(XPORT_MAJOR,XPORT_MINOR_FT_BASE+i));
	}

	platform_device_unregister(xport_pdev);
	class_destroy(cnc180x_xport_class);
	iounmap(xport_regs_base);
	//release_mem_region(VA_XPORT_BASE, XPORT_REGS_SIZE);

	iounmap(xport_mem_base);
	iounmap((void *)sec_mb_base);
	release_mem_region(XPORT_MEM_BASE, XPORT_MEM_SIZE);

	free_irq(XPORT_IRQ0, &irq0_dev_id);
	free_irq(XPORT_IRQ1, &irq1_dev_id);

	unregister_chrdev(XPORT_MAJOR, "cs_xport");
	printk(KERN_INFO " cs Xport Exit ...... OK. \n");

	return;
}



module_init(xport_init);
module_exit(xport_exit);

MODULE_AUTHOR("zhang.ji(ji.zhang@celestialsemi.com)");
MODULE_DESCRIPTION("cs 1.0.0 Xport Driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");

