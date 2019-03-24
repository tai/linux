/*
 *  Copyright (C) 2010 Celestial Corporation
 *  sdk@celestialsemi.cn 
 */

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <linux/io.h> 
#include <linux/semaphore.h>

#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include "xport_drv1800.h"
#include "xport_filter1800.h"
#include "xport_mips1800.h"

/*mips cmd */
#define  FLT_CMD_BASE                     (XPORT_AV_CHNL_NUM << 9)
#define SECTION_BUCKET_SIZE               0x1000

#define  FLT_EN_ADDR(x)                   (FLT_CMD_BASE+(x<<8))  
#define  FLT_FLT0_ADDR(x)                 (FLT_CMD_BASE+0x1+(x<<8))
#define  FLT_FLT1_ADDR(x)                 (FLT_CMD_BASE+0x2+(x<<8))
#define  FLT_FLT2_ADDR(x)                 (FLT_CMD_BASE+0x3+(x<<8))
#define  FLT_MASK0_ADDR(x)                (FLT_CMD_BASE+0x4+(x<<8))
#define  FLT_MASK1_ADDR(x)                (FLT_CMD_BASE+0x5+(x<<8))
#define  FLT_MASK2_ADDR(x)		          (FLT_CMD_BASE+0x6+(x<<8))
#define  FLT_BUF_LOW_ADDR(x)              (FLT_CMD_BASE+0x07+(x<<8))
#define  FLT_BUF_UP_ADDR(x)               (FLT_CMD_BASE+0x08+(x<<8))

#define  FLT_TC0_ERR_CNT_ADDR(x)          (FLT_CMD_BASE+0x0a+(x<<8))
#define  FLT_TC1_ERR_CNT_ADDR(x)          (FLT_CMD_BASE+0x0b+(x<<8))
#define  FLT_TC2_ERR_CNT_ADDR(x)          (FLT_CMD_BASE+0x0c+(x<<8))
#define  FLT_TC3_ERR_CNT_ADDR(x)          (FLT_CMD_BASE+0x0d+(x<<8))
#define  FLT_ERR_PACKET_CNT_ADDR(x)       (FLT_CMD_BASE+0x0e+(x<<8))
#define  FLT_OUT_PACKET_CNT_ADDR(x)       (FLT_CMD_BASE+0x0f+(x<<8))

#define  FLT_DISABLE_ADDR(x)		  		(FLT_CMD_BASE+0x36+(x<<8))
#define  PID_CMD_BASE                     (FLT_CMD_BASE+0x16)
/***********************n~(0,11)   x~(0,62)**********************/
#define  FLT_PID_ADDR(n,x)                (PID_CMD_BASE+n+(x<<8))
#define  FLT_BUF_WP_ADDR(x)               (XPORT_REG_ADDR_BASE+0x440+(x<<3))
#define  FLT_BUF_RP_ADDR(x)               (XPORT_REG_ADDR_BASE+0x444+(x<<3))


typedef enum
{
	RSV_OUTPUT = 0,
	TS_OUTPUT = 1,
	ES_OUTPUT,
	SEC_OUTPUT,
	PES_OUTPUT
}CHL_OUTPUT_TYPE;


inline unsigned int get_filter_buf_base_addr(unsigned int filter_index)
{
	unsigned int idx = filter_index;

	return (idx>= 0&&idx<MAX_FILTER_NUM ? FLT_BUF_BASE_ADDR(idx):-1);
}

inline unsigned int get_filter_buf_top_addr(unsigned int filter_index)
{
	unsigned int idx = filter_index;
	return (idx>= 0&&idx<MAX_FILTER_NUM ?(FLT_BUF_BASE_ADDR(idx + 1)):-1);
}

inline unsigned int get_filter_buf_size(unsigned int filter_index) 
{
	unsigned int idx = filter_index;
	return ((idx>= 0&&idx<MAX_FILTER_NUM) ? (FLT_BUF_SIZE(idx)): -1);
}


inline unsigned int get_xport_filter_wp(unsigned int filter_index)
{
	return xport_readl(FLT_BUF_WP_ADDR(filter_index));
}

inline unsigned int get_xport_filter_rp(unsigned int filter_index)
{
	return xport_readl(FLT_BUF_RP_ADDR(filter_index));
}


int xport_filter_reset(unsigned int filter_index)
{
	unsigned int idx = filter_index;
	xport_mips_write(FLT_DISABLE_ADDR(idx),0);
	xport_mips_write(FLT_BUF_LOW_ADDR(idx),get_filter_buf_base_addr(idx));
	xport_mips_write(FLT_BUF_UP_ADDR(idx),get_filter_buf_top_addr(idx));
	xport_writel(FLT_BUF_WP_ADDR(idx),get_filter_buf_base_addr(idx));
	xport_writel(FLT_BUF_RP_ADDR(idx),get_filter_buf_base_addr(idx));

	return 0;
}


/**set filter type cmd to I/D ram of MIPs**/
int xport_filter_set_type(unsigned int filter_index, XPORT_FILTER_TYPE filter_type)
{
	unsigned int en;
	int rt;
	unsigned int idx = filter_index;
	XPORT_FILTER_TYPE ft = filter_type;


	rt = xport_mips_read(FLT_EN_ADDR(idx), &en);

	/* bit31: enable flag: 1 - output channel enabled */
	en = xport_get_bit(en,31); 

	if (rt == 0  &&  en == 0) 
	{
		en = (ft==FILTER_TYPE_SECTION ? SEC_OUTPUT:
				(ft==FILTER_TYPE_TS ? TS_OUTPUT:
				 (ft==FILTER_TYPE_PES? PES_OUTPUT:
				  (ft==FILTER_TYPE_ES ? ES_OUTPUT : RSV_OUTPUT))));

		xport_mips_write(FLT_EN_ADDR(idx), en);

		return 0;
	}

	return -1;
}

int xport_filter_set_input(unsigned int filter_index, XPORT_INPUT_CHANNEL input_channel)
{
	return -1;
}

int xport_filter_set_crc(unsigned int filter_index, unsigned int crc_index)
{
	return -1;
}

int xport_filter_set_pidx(unsigned int filter_index, unsigned int pid, unsigned int slot)
{
	if(filter_index >=0 && filter_index <= 60 && slot>0)
	{
		printk("wrong operation.filter idx 0-61 shoule with only slot 0 work! \n");
		return -1;
	}

	return slot>=0&&slot<=11 ? xport_mips_write(FLT_PID_ADDR(slot,filter_index),pid):-1;
}

int xport_filter_set_filter(unsigned int filter_index, unsigned char *filter, unsigned char *mask)
{
	xport_mips_write(FLT_FLT0_ADDR(filter_index), *(unsigned int *) filter);
	xport_mips_write(FLT_FLT1_ADDR(filter_index), *(unsigned int *) (filter + 4));
	xport_mips_write(FLT_FLT2_ADDR(filter_index), *(unsigned int *) (filter + 8));
	xport_mips_write(FLT_MASK0_ADDR(filter_index), *(unsigned int *) mask);
	xport_mips_write(FLT_MASK1_ADDR(filter_index), *(unsigned int *) (mask + 4));
	xport_mips_write(FLT_MASK2_ADDR(filter_index), *(unsigned int *) (mask + 8));

	return 0;
}

int xport_filter_crc_enable(unsigned int filter_index)
{
	return -1;
}

int xport_filter_crc_disable(unsigned int filter_index)
{
	return -1;
}

int xport_filter_crc_is_enable(unsigned int filter_index)
{
	return -1;
}

int xport_filter_enable(unsigned int filter_index, spinlock_t * spin_lock_ptr)
{
	int rt;
	unsigned int en;
	unsigned long irq1_en;
	unsigned long spin_flags;

	rt = xport_mips_read(FLT_EN_ADDR(filter_index), &en);

	if(rt != 0)
	{
		return -1;
	}

	/**asign iomem addr into filter's register**/

	xport_writel(FLT_BUF_RP_ADDR(filter_index), get_filter_buf_base_addr(filter_index));
	spin_lock_irqsave(spin_lock_ptr, spin_flags);
	irq1_en = (filter_index <32 ? xport_readl(XPORT_INT_ENB_ADDR1): xport_readl(XPORT_INT_ENB_ADDR0));	 	
	irq1_en |=(filter_index<32 ? 1<<filter_index : XPORT_IRQ0_FILTER_NOTIFY);


	if(filter_index <32)
	{
		xport_writel(XPORT_INT_CLS_ADDR1, (1 << filter_index));
	}

	xport_writel((filter_index<32? XPORT_INT_ENB_ADDR1: XPORT_INT_ENB_ADDR0),irq1_en);
	spin_unlock_irqrestore(spin_lock_ptr, spin_flags);

	return xport_mips_write(FLT_EN_ADDR(filter_index), 0x80000000 | en);
}


int xport_filter_disable(unsigned int filter_index, spinlock_t * spin_lock_ptr)
{
	int rt;
	unsigned int en;
	unsigned long irq1_en;
	unsigned long spin_flags;


	rt = xport_mips_read(FLT_EN_ADDR(filter_index), &en);

	if (rt != 0)
		return -1;


	spin_lock_irqsave(spin_lock_ptr, spin_flags);

	irq1_en = (filter_index <32 ? xport_readl(XPORT_INT_ENB_ADDR1) : xport_readl(XPORT_INT_ENB_ADDR0));
	irq1_en&=(~(filter_index<32?1<<filter_index: XPORT_IRQ0_FILTER_NOTIFY));
	xport_writel((filter_index<32?XPORT_INT_ENB_ADDR1: XPORT_INT_ENB_ADDR0),irq1_en);
	spin_unlock_irqrestore(spin_lock_ptr, spin_flags);

#if 1
	return xport_mips_write(FLT_EN_ADDR(filter_index), 0x7fffffff & en);
#else
	return xport_mips_write(FILTER_DISABLE_ADDR(filter_index), 0);
#endif


}

int xport_filter_is_enable(unsigned int filter_index)
{
	int rt;
	unsigned int en;

	rt = xport_mips_read(FLT_EN_ADDR(filter_index), &en);
	en >>= 31;

	if (rt == 0)
		return en;

	return -1;
}
/* seciton filter */
int xport_filter_check_section_number(unsigned int filter_index)
{
	unsigned int len;
	unsigned int wp, wp_tog;
	unsigned int rp, rp_tog;
	unsigned int up_addr, low_addr;


	up_addr = get_filter_buf_top_addr(filter_index);
	low_addr = get_filter_buf_base_addr(filter_index);
	wp = xport_readl(FLT_BUF_WP_ADDR(filter_index));
	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));


	wp_tog = wp & 0x80000000;
	rp_tog = rp & 0x80000000;

	wp = wp & 0x7fffffff;
	rp = rp & 0x7fffffff;

	if (wp_tog == rp_tog) {
		len = wp - rp;
	}
	else {
		len = (up_addr - low_addr) + wp - rp;
	}

	len = len & 0xfffff000;

	return len;
}

int xport_filter_check_section_size(unsigned int filter_index)
{
	return 0;
}

int xport_filter_clear_buffer(unsigned int filter_index)
{
	xport_writel(FLT_BUF_RP_ADDR(filter_index), get_filter_buf_base_addr(filter_index));
	xport_writel(FLT_BUF_WP_ADDR(filter_index), get_filter_buf_base_addr(filter_index));

	return 0;
}

int xport_filter_read_section_data(unsigned int filter_index, char __user * buffer, size_t len)
{
	void __iomem *read_addr;	
	unsigned int rp, rp_tog;
	unsigned int section_len;

	if (xport_filter_check_section_number(filter_index) <= 0) {
		return -1;
	}

	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));

	rp_tog = rp & 0x80000000;
	rp = rp & 0x7fffffff;

	read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);

	section_len = ((unsigned char *) read_addr)[1];
	section_len <<= 8;
	section_len += ((unsigned char *) read_addr)[2];
	section_len &= 0xfff;
	section_len += 3;



	if(section_len > 4096 || section_len > len) {
		return -EFAULT;
	}

	if(copy_to_user(buffer, read_addr, section_len)) {
		return -EFAULT;
	}

	rp += 0x1000;

	if(rp >= get_filter_buf_top_addr(filter_index)) 
	{
		rp = get_filter_buf_base_addr(filter_index);
		rp_tog = rp_tog ^ 0x80000000;
	}	
	xport_writel(FLT_BUF_RP_ADDR(filter_index), rp_tog | rp);



	return section_len;
}
/* ts filter */
int xport_filter_check_data_size(unsigned int filter_index)
{
	unsigned int len = 0;
	unsigned int wp, wp_tog;
	unsigned int rp, rp_tog;
	unsigned int up_addr, low_addr;

	up_addr = get_filter_buf_top_addr(filter_index);
	low_addr = get_filter_buf_base_addr(filter_index);

	wp = xport_readl(FLT_BUF_WP_ADDR(filter_index));
	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));

	wp_tog = wp & 0x80000000;
	rp_tog = rp & 0x80000000;
	wp = wp & 0x7fffffff;
	rp = rp & 0x7fffffff;

	if (wp_tog == rp_tog) {
		len = wp - rp;
	}
	else {
		len = (up_addr - low_addr) + wp - rp;
	}

	len = (len / 188) * 188;  

	return len;
}


int xport_filter_read_data(unsigned int filter_index, char __user * buffer, size_t len)
{
	void __iomem *read_addr;

	unsigned int rp, rp_tog;
	unsigned int size_tmp;

	/*check available data in buffer for reading*/
	if (xport_filter_check_data_size(filter_index) < len)
		return -1;

	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));
	rp_tog = rp & 0x80000000;
	rp = rp & 0x7fffffff;

	size_tmp = get_filter_buf_top_addr(filter_index) - rp;

	if(size_tmp <= len) 
	{
		read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);
		if (copy_to_user(buffer, read_addr, size_tmp))
			return -EFAULT;

		len -= size_tmp;
		buffer += size_tmp;
		size_tmp += len;

		rp = get_filter_buf_base_addr(filter_index);
		rp_tog = rp_tog ^ 0x80000000;
	}
	else
		size_tmp = len;

	if (len > 0) {
		read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);
		if (copy_to_user(buffer, read_addr, len))
			return -EFAULT;

		rp += len;
	}

	xport_writel(FLT_BUF_RP_ADDR(filter_index), rp_tog | rp);


	return size_tmp;
}



/* pes filter */
int xport_filter_check_pes_size(unsigned int filter_index)
{
	int len;
	unsigned int pes_len = 0;

	unsigned int wp, wp_tog;
	unsigned int rp, rp_tog;
	unsigned int up_addr, low_addr;

	unsigned int section_size, data[3];
	unsigned int mask = SECTION_BUCKET_SIZE - 1;
	void __iomem *read_addr;

	up_addr = get_filter_buf_top_addr(filter_index);
	low_addr = get_filter_buf_base_addr(filter_index);

	wp = xport_readl(FLT_BUF_WP_ADDR(filter_index));
	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));

	wp_tog = wp & 0x80000000;
	rp_tog = rp & 0x80000000;
	wp = wp & (0x7fffffff & ~mask);
	rp = rp & (0x7fffffff & ~mask);

	if (wp_tog == rp_tog) {
		len = wp - rp;
	}
	else {//?
		len = (up_addr - low_addr) + wp - rp;
	}

	if (len < SECTION_BUCKET_SIZE)
		return 0;

	if (rp & mask) 	 /* rp is not aligned with 0x1000 size */
		return -EFAULT;

	section_size = 0;

	/* find the position that new pes packet starts */
	while ((len & ~mask) > 0) {
		read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);

		data[0] = *(unsigned int *) (read_addr);	 /* first 4 byte of bucket header */
		data[1] = *(unsigned int *) (read_addr + 4);
		data[2] = *(unsigned int *) (read_addr + 8);

		if ((data[0] & 0x010000) && ((data[1] & 0xffffff) == 0x010000)) {	 /* new pes packet */
			section_size += data[0] & 0xffff;
			section_size -= 4;
			pes_len = (data[2] & 0xff) << 8;
			pes_len |= (data[2] & 0xff00) >> 8;

			xport_writel(FLT_BUF_RP_ADDR(filter_index), rp | rp_tog);

			rp += SECTION_BUCKET_SIZE;
			len -= SECTION_BUCKET_SIZE;

			if (rp >= up_addr) {
				rp = low_addr;
				rp_tog ^= 0x80000000;
			}

			break;
		}

		rp += SECTION_BUCKET_SIZE;
		len -= SECTION_BUCKET_SIZE;

		if (rp >= up_addr) {
			rp = low_addr;
			rp_tog ^= 0x80000000;
		}
	}

	if (section_size == 0) {	 /* new pes packet is not found */
		xport_writel(FLT_BUF_RP_ADDR(filter_index), (wp & 0xfffff000) | wp_tog);
		return 0;
	}

	/* 
	 * find the the position of next new pes packet, to get the 
	 * size of current pes packet
	 */
	while (len > 0) {
		read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);
		data[0] = *(unsigned int *) (read_addr);	 /* first 4 byte of bucket header */
		data[1] = *(unsigned int *) (read_addr + 4);
		data[2] = *(unsigned int *) (read_addr + 8);

		if ((data[0] & 0x010000) && ((data[1] & 0xffffff) == 0x010000))
			break;
		else {
			section_size += (data[0] & 0xffff);
			section_size -= 4;
		}

		if (section_size > pes_len + 6)
			section_size = pes_len + 6;

		if ((section_size == pes_len + 6) && (pes_len != 0))
			break;  /* video pes is exception */

		rp += SECTION_BUCKET_SIZE;
		len -= SECTION_BUCKET_SIZE;

		if (rp >= up_addr) {
			rp = low_addr;
			rp_tog ^= 0x80000000;
		}
	}

	if (len <= 0) {
		unsigned int buffer_size = up_addr - low_addr;
		if (section_size >= (buffer_size - (buffer_size >> 10))) {
			printk(KERN_INFO
					"PES packet size exceed the size of filter buffer: filter id = %d, buffer size = %d section size = %d\n",
					filter_index, up_addr - low_addr, section_size);
			return -EFAULT;
		}
		else if ((section_size == pes_len + 6) && (pes_len != 0))
			return section_size;
		else
			/* not find the next pes packet, that means current packet is not ready */
			return 0;
	}

	return section_size;
}

int xport_filter_read_pes_data(unsigned int filter_index, char __user * buffer, size_t len)
{
	unsigned int rp, rp_tog;
	unsigned int up_addr, low_addr;
	unsigned int data, bucket_size;
	int pes_size, byte_written;
	unsigned int mask = SECTION_BUCKET_SIZE - 1;

	void __iomem *read_addr;

	pes_size = xport_filter_check_pes_size(filter_index);
	if (pes_size <= 0) {
		printk(KERN_INFO "pes size = %d, empty now \n", pes_size);
		return -EFAULT;
	}
	if (pes_size > len)
		pes_size = len;

	byte_written = pes_size;

	up_addr = get_filter_buf_top_addr(filter_index);
	low_addr = get_filter_buf_base_addr(filter_index);

	rp = xport_readl(FLT_BUF_RP_ADDR(filter_index));

	rp_tog = rp & 0x80000000;
	rp = rp & 0x7fffffff;
	if (rp & mask) {	 /* rp is not aligned with 0x1000 */
		printk(KERN_INFO "rp 0x%08x is not aligned with 0x1000\n", rp);
		return -EFAULT;
	}

	while (pes_size > 0) {
		read_addr = xport_mem_base + (rp - XPORT_MEM_BASE);
		data = *(unsigned int *) read_addr; /* first 4 byte of bucket header */
		read_addr += 4;
		bucket_size = (data & 0xffff);
		bucket_size -= 4;

		if (bucket_size > pes_size)
			bucket_size = pes_size;

		if (copy_to_user(buffer, read_addr, bucket_size) > 0) {
			printk(KERN_INFO " user buffer is not enough \n");
			return -EFAULT;
		}

		pes_size -= bucket_size;
		buffer += bucket_size;
		rp += SECTION_BUCKET_SIZE;

		if (rp >= up_addr) {
			rp = low_addr;
			rp_tog ^= 0x80000000;
		}
	}

	xport_writel(FLT_BUF_RP_ADDR(filter_index), rp_tog | rp);

	return byte_written;
}
