#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/semaphore.h>

#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include "xport_drv1800h.h"
#include "xport_dma1800h.h"
#include "xport_regs1800h.h"



#define  ML_ALIGN(x)   	((x)<<24 | (x)>>24 | ((x)>>8 & 0xff00) | ((x)<<8 & 0xff0000))
#define	 MAX_INPUT_CHL_NUM   4
#define  DMA_ENB    1
static 	XPORT_DMA xport_dma[2];
static  XPORT_DIR dir_dma[XPORT_MAX_CHL_NUM];
#define  GET_DMA_HADDR(v)  (0xFFFFFFF&v)



typedef enum
{
	DMA0_INPUT = 0,
	DMA1_INPUT = 1
}DMA_INPUT_MODE;

/*
	actually ,the dir mode stand for the write pointer value should be got from
	dma mail box.
*/
typedef enum
{
	DMA0_DIRC  = 0,
	DMA1_DIRC = 1
}DMA_DIR_MODE;


void xport_dma_init(void)
{
	unsigned int i;
	unsigned int tmp_buf_addr;
	unsigned int tmp_list_addr;



	xport_dma[0].dma_list[0].head_node = xport_mem_base + (DMA0_LIST0_HEAD_ADDR - XPORT_MEM_BASE);
	xport_dma[0].dma_list[0].max_block_size = DMA0_MAX_BLOCK_SIZE;
	xport_dma[0].dma_list[0].max_block_num = DMA0_MAX_BLOCK_NUM;
	xport_dma[0].dma_list[0].cur_index = 0;
	xport_dma[0].dma_list[0].next_index = 0;
	xport_dma[0].dma_list[0].locked_flag = 0;

	tmp_buf_addr = DMA0_BUF0_BASE_ADDR;
	tmp_list_addr = DMA0_LIST0_HEAD_ADDR;


	for (i = 0; i < DMA0_MAX_BLOCK_NUM; i++)
	{
		tmp_list_addr += sizeof(DMA_LIST_NODE);

		xport_dma[0].dma_list[0].head_node[i].buf_addr = ML_ALIGN(tmp_buf_addr);
		xport_dma[0].dma_list[0].head_node[i].data_size = 0;
		xport_dma[0].dma_list[0].head_node[i].next_addr = (DMA_LIST_NODE *) (ML_ALIGN(tmp_list_addr));
		xport_dma[0].dma_list[0].head_node[i].next_valid = 0;

		tmp_buf_addr += DMA0_MAX_BLOCK_SIZE;
	}

	xport_dma[0].dma_list[1].head_node = xport_mem_base + (DMA0_LIST1_HEAD_ADDR - XPORT_MEM_BASE);
	xport_dma[0].dma_list[1].max_block_size = DMA0_MAX_BLOCK_SIZE;
	xport_dma[0].dma_list[1].max_block_num = DMA0_MAX_BLOCK_NUM;
	xport_dma[0].dma_list[1].cur_index = 0;
	xport_dma[0].dma_list[1].next_index = 0;
	xport_dma[0].dma_list[1].locked_flag = 0;

	tmp_buf_addr = DMA0_BUF1_BASE_ADDR;
	tmp_list_addr = DMA0_LIST1_HEAD_ADDR;

	for (i = 0; i < DMA0_MAX_BLOCK_NUM; i++) {
		tmp_list_addr += sizeof(DMA_LIST_NODE);

		xport_dma[0].dma_list[1].head_node[i].buf_addr = ML_ALIGN(tmp_buf_addr);
		xport_dma[0].dma_list[1].head_node[i].data_size = 0;
		xport_dma[0].dma_list[1].head_node[i].next_addr = (DMA_LIST_NODE *) (ML_ALIGN(tmp_list_addr));
		xport_dma[0].dma_list[1].head_node[i].next_valid = 0;

		tmp_buf_addr += DMA0_MAX_BLOCK_SIZE;
	}

	xport_dma[1].dma_list[0].head_node = xport_mem_base + (DMA1_LIST0_HEAD_ADDR - XPORT_MEM_BASE);
	xport_dma[1].dma_list[0].max_block_size = DMA1_MAX_BLOCK_SIZE;
	xport_dma[1].dma_list[0].max_block_num = DMA1_MAX_BLOCK_NUM;
	xport_dma[1].dma_list[0].cur_index = 0;
	xport_dma[1].dma_list[0].next_index = 0;
	xport_dma[1].dma_list[0].locked_flag = 0;

	tmp_buf_addr = DMA1_BUF0_BASE_ADDR;
	tmp_list_addr = DMA1_LIST0_HEAD_ADDR;

	for (i = 0; i < DMA1_MAX_BLOCK_NUM; i++) {
		tmp_list_addr += sizeof(DMA_LIST_NODE);

		xport_dma[1].dma_list[0].head_node[i].buf_addr = ML_ALIGN(tmp_buf_addr);
		xport_dma[1].dma_list[0].head_node[i].data_size = 0;
		xport_dma[1].dma_list[0].head_node[i].next_addr = (DMA_LIST_NODE *) (ML_ALIGN(tmp_list_addr));
		xport_dma[1].dma_list[0].head_node[i].next_valid = 0;

		tmp_buf_addr += DMA1_MAX_BLOCK_SIZE;
	}

	xport_dma[1].dma_list[1].head_node = xport_mem_base + (DMA1_LIST1_HEAD_ADDR - XPORT_MEM_BASE);
	xport_dma[1].dma_list[1].max_block_size = DMA1_MAX_BLOCK_SIZE;
	xport_dma[1].dma_list[1].max_block_num = DMA1_MAX_BLOCK_NUM;
	xport_dma[1].dma_list[1].cur_index = 0;
	xport_dma[1].dma_list[1].next_index = 0;
	xport_dma[1].dma_list[1].locked_flag = 0;

	tmp_buf_addr = DMA1_BUF1_BASE_ADDR;
	tmp_list_addr = DMA1_LIST1_HEAD_ADDR;

	for (i = 0; i < DMA1_MAX_BLOCK_NUM; i++) {
		tmp_list_addr += sizeof(DMA_LIST_NODE);

		xport_dma[1].dma_list[1].head_node[i].buf_addr = ML_ALIGN(tmp_buf_addr);
		xport_dma[1].dma_list[1].head_node[i].data_size = 0;
		xport_dma[1].dma_list[1].head_node[i].next_addr = (DMA_LIST_NODE *) (ML_ALIGN(tmp_list_addr));
		xport_dma[1].dma_list[1].head_node[i].next_valid = 0;

		tmp_buf_addr += DMA1_MAX_BLOCK_SIZE;
	}

	init_MUTEX(&(xport_dma[0].dma_sem));
	init_MUTEX(&(xport_dma[1].dma_sem));
	init_MUTEX(&(dir_dma[0].dir_sem));
	init_MUTEX(&(dir_dma[1].dir_sem));
	init_MUTEX(&(dir_dma[2].dir_sem));
	init_MUTEX(&(dir_dma[3].dir_sem));


	xport_dma[0].cur_index = 0;
	xport_dma[1].cur_index = 0;

	xport_dma[0].next_jiffies = 0;
	xport_dma[1].next_jiffies = 0;

	return;
}

int xport_dma_set(unsigned int chl_id)
{
	int rt_val = -EFAULT;
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
    unsigned int dma_id = 0;
#else
    unsigned int dma_id = chl_id % 2;
#endif
	unsigned int regs_val = 0;
	DMA_LIST *dma_cur_list_ptr = NULL;
	DMA_LIST *dma_done_list_ptr = NULL;

	if (down_interruptible(&(xport_dma[dma_id].dma_sem)))
		return rt_val;

	dma_cur_list_ptr = &(xport_dma[dma_id].dma_list[xport_dma[dma_id].cur_index]);
	dma_done_list_ptr = &(xport_dma[dma_id].dma_list[1 - xport_dma[dma_id].cur_index]);

	regs_val = xport_readl((DMA_INPUT0_HEAD_ADDR + (dma_id << 2)));

#if 0
	{
		int i = 0;
		DMA_LIST_NODE *dma_list_node_ptr = NULL;

		dma_list_node_ptr = xport_dma[dma_id].dma_list[xport_dma[dma_id].cur_index].head_node;
		printk("dma_link: \n");
		do
		{
			unsigned int buf = dma_list_node_ptr->buf_addr;
			unsigned int next_buf = (unsigned int )(dma_list_node_ptr->next_addr);
			unsigned int offset = 0;

			buf = ML_ALIGN(buf);
			printk("\tnode %d: addr 0x%08x size 0x%08x next_addr 0x%08x next_valid 0x%08x\n", i, buf,
				dma_list_node_ptr->data_size,
				ML_ALIGN(next_buf),
				dma_list_node_ptr->next_valid);

			buf -= XPORT_MEM_BASE;
			buf += xport_mem_base;

			i++;

			while(offset < ML_ALIGN(dma_list_node_ptr->data_size))
			{
				if (((unsigned char *)buf)[offset+2] == 0xeb)
				{
					printk("data: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
						((unsigned char *)buf)[offset+0], ((unsigned char *)buf)[offset+1], ((unsigned char *)buf)[offset+2], ((unsigned char *)buf)[offset+3],
						((unsigned char *)buf)[offset+4], ((unsigned char *)buf)[offset+5], ((unsigned char *)buf)[offset+6], ((unsigned char *)buf)[offset+7]);

				}
				offset += 0xbc;
			}

		}while ((dma_list_node_ptr++)->next_valid);
	}
#endif
	if(regs_val == 0 && dma_cur_list_ptr->next_index > 0)
	{
		dma_cur_list_ptr->head_node[dma_cur_list_ptr->cur_index].next_valid = 0;

		if (dma_id == 0) {
            if (chl_id == 0)
                xport_writel(XPORT_INT_CLS_ADDR0, XPORT_IRQ0_DMA0_EMPTY_MSK);
            else
                xport_writel(XPORT_INT_CLS_ADDR0, XPORT_IRQ0_DMA2_EMPTY_MSK);

			if (xport_dma[dma_id].cur_index == 0)
			{
				xport_writel(DMA_INPUT0_HEAD_ADDR, (DMA0_LIST0_HEAD_ADDR|0x80000000));
			}
			else
			{
				xport_writel(DMA_INPUT0_HEAD_ADDR, (DMA0_LIST1_HEAD_ADDR|0x80000000));
			}
		}
		else
		{
			if (chl_id == 1)
                xport_writel(XPORT_INT_CLS_ADDR0, XPORT_IRQ0_DMA1_EMPTY_MSK);
            else
                xport_writel(XPORT_INT_CLS_ADDR0, XPORT_IRQ0_DMA3_EMPTY_MSK);
			if (xport_dma[dma_id].cur_index == 0)
			{
				xport_writel(DMA_INPUT1_HEAD_ADDR, DMA1_LIST0_HEAD_ADDR|0x80000000);
			}
			else
			{
				xport_writel(DMA_INPUT1_HEAD_ADDR, DMA1_LIST1_HEAD_ADDR|0x80000000);
			}
		}

		/* software bitrate control ,no need now.cur_delay_clk27 = ((dma_cur_list_ptr->block_size_sum * (CLK27_MHZ << 3)) / MAX_DMA_MBPS);

		if (dma_id)
			xport_writel(MAIL_BOX_ADDR(7), cur_delay_clk27);
		else
			xport_writel(MAIL_BOX_ADDR(6), cur_delay_clk27);

		 end of SW bitrate control */


		dma_done_list_ptr->cur_index = 0;
		dma_done_list_ptr->next_index = 0;
		dma_done_list_ptr->block_size_sum = 0;

		xport_dma[dma_id].cur_index = 1 - xport_dma[dma_id].cur_index;

		rt_val = 0;
	}

	up(&(xport_dma[dma_id].dma_sem));

	return rt_val;
}

/* xunli: for direct dma mode */
int xport_dma_direct_write(const char __user * buffer, size_t len,unsigned int chl_id, ssize_t packet_size)
{
	ssize_t rt_val = -EFAULT;

	void __iomem *write_addr;
	void __iomem *base_addr;

	unsigned int tmp_addr = 0;
	unsigned int chl_buf_unit_num = 0, chl_buf_unit_size = 0, chl_buf_type = 0;
	unsigned int free_block_num = 0, chl_buf_wp = 0, chl_buf_rp = 0;
	unsigned int regs_val = 0;
	unsigned int write_len = 0;


	if(down_interruptible(&(dir_dma[chl_id].dir_sem)))
		return -EFAULT;


	regs_val = xport_readl(IN_CHL_CFG_ADDR(chl_id));
	chl_buf_type = ((regs_val >> 29) & 0x3);
	chl_buf_unit_num = (regs_val >> 8) & 0xfff;
	chl_buf_unit_size = (regs_val & 0xff) << 3;


	tmp_addr = xport_readl(IN_CHL_BASE_ADDR(chl_id)) << 3;
	if(chl_buf_type!=3 || (tmp_addr!=IN_CHL0_BASE_ADDR_DEF && tmp_addr!=IN_CHL1_BASE_ADDR_DEF
		&& tmp_addr!=IN_CHL2_BASE_ADDR_DEF && tmp_addr!=IN_CHL3_BASE_ADDR_DEF))
	{
		up(&(dir_dma[chl_id].dir_sem));
		return -EFAULT;
	}

	tmp_addr -= XPORT_MEM_BASE;
	base_addr = xport_mem_base + tmp_addr;

	chl_buf_wp = xport_readl(chl_id%2?(chl_id==3?XPORT_CHL_DMA3_WP_ADDR: XPORT_CHL_DMA1_WP_ADDR):
		(chl_id==2?XPORT_CHL_DMA2_WP_ADDR: XPORT_CHL_DMA0_WP_ADDR));

	chl_buf_rp = xport_readl(IN_CHL_RP_ADDR(chl_id));

	if ((chl_buf_wp ^ chl_buf_rp) >> 31)
		free_block_num = (chl_buf_rp & 0xfff) - (chl_buf_wp & 0xfff);
	else
		free_block_num = chl_buf_unit_num + (chl_buf_rp & 0xfff) - (chl_buf_wp & 0xfff);

	rt_val = 0;

	while (((int) len >= packet_size) && ((int) free_block_num > 0) && (free_block_num <= chl_buf_unit_num)) {

		write_addr = base_addr + ((chl_buf_wp & 0xfff) * chl_buf_unit_size);
		write_len = 0;

		while ((len >= packet_size) && (write_len + packet_size + 8 <= chl_buf_unit_size))
		{
			//printk("func:%s line:%d len:0x%08x write_len:0x%08x--\n",
				//__FUNCTION__,__LINE__,len,write_len);
			write_len += packet_size;
			len -= packet_size;
		}

		if (write_len == 0)
		{
			up(&(dir_dma[chl_id].dir_sem));
			return -EFAULT;
		}

		writel(0, write_addr);
		write_addr += 4;
		writel(ML_ALIGN(write_len), write_addr);
		write_addr += 4;

		copy_from_user(write_addr, buffer, write_len);

		buffer += write_len;
		rt_val += write_len;

		/* update wp pointer */
		chl_buf_wp++;

		if ((chl_buf_wp & 0xfff) >= chl_buf_unit_num)
		{chl_buf_wp = (~chl_buf_wp) & 0x80000000;}

		free_block_num--;
	}

	xport_writel((chl_id%2?(chl_id==3?XPORT_CHL_DMA3_WP_ADDR: XPORT_CHL_DMA1_WP_ADDR):
		(chl_id==2?XPORT_CHL_DMA2_WP_ADDR: XPORT_CHL_DMA0_WP_ADDR)), chl_buf_wp);
	up(&(dir_dma[chl_id].dir_sem));

	//printk("---func:%s line:%d rt_val:0x%08x  \n",__FUNCTION__,__LINE__,rt_val);

	return rt_val;
}

int xport_dma_write(const char __user * buffer, size_t len, unsigned int chl_id, ssize_t packet_size)
{
	ssize_t rt_val = -EFAULT;

	void __iomem *write_addr;
	DMA_LIST *dma_list_ptr = NULL;

	unsigned int tmp_addr = 0;
	unsigned int regs_val = 0;
	unsigned int hw_set_req = 0;
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
    unsigned int dma_id = 0;
#else
    unsigned int dma_id = chl_id % 2;
#endif
	char *ptr = NULL;

    packet_size = packet_size;      // unused variable packet_size;

START_LAB:
	hw_set_req = 0;

	if(down_interruptible(&(xport_dma[dma_id].dma_sem)))
		return -EFAULT;
	dma_list_ptr = &(xport_dma[dma_id].dma_list[xport_dma[dma_id].cur_index]);

	if(len > dma_list_ptr->max_block_size)
	{
		up(&(xport_dma[dma_id].dma_sem));
		return -EFAULT;
	}

    //printk("xport_dma_write : chl id is %d, dma id is %d\n", chl_id, dma_id);

	if (dma_list_ptr->next_index < dma_list_ptr->max_block_num)
	{
		tmp_addr = ML_ALIGN(dma_list_ptr->head_node[dma_list_ptr->next_index].buf_addr);
		tmp_addr -= XPORT_MEM_BASE;
		write_addr = xport_mem_base + tmp_addr;

		copy_from_user(write_addr, buffer, len);
		ptr = write_addr;

		//printk(" --func:%s	line:%d --\n",__FUNCTION__,__LINE__);

		dma_list_ptr->head_node[dma_list_ptr->next_index].data_size = ML_ALIGN(len);
		dma_list_ptr->head_node[dma_list_ptr->next_index].next_valid = 0;


		if (dma_list_ptr->next_index != 0) {
			dma_list_ptr->head_node[dma_list_ptr->cur_index].next_valid = 0x01000000;	//
			dma_list_ptr->cur_index++;
		}

		dma_list_ptr->next_index++;
		dma_list_ptr->block_size_sum += len;

		rt_val = len;

	}
	else
	{
		rt_val = 0;
		regs_val = xport_readl((DMA_INPUT0_HEAD_ADDR + (dma_id << 2)));
		if (regs_val == 0)
			hw_set_req = 1;
	}

	up(&(xport_dma[dma_id].dma_sem));

	if (hw_set_req) {
		xport_dma_set(chl_id);
		goto START_LAB;
	}

	return rt_val;
}

int xport_dma_reset(unsigned int dma_id)
{

	if(down_interruptible(&(xport_dma[dma_id].dma_sem)))
		return -EFAULT;

	xport_dma[dma_id].cur_index = 0;
	xport_dma[dma_id].next_jiffies = 0;
	xport_dma[dma_id].dma_list[0].cur_index = 0;
	xport_dma[dma_id].dma_list[0].next_index = 0;
	xport_dma[dma_id].dma_list[0].locked_flag = 0;
	xport_dma[dma_id].dma_list[1].cur_index = 0;
	xport_dma[dma_id].dma_list[1].next_index = 0;
	xport_dma[dma_id].dma_list[1].locked_flag = 0;

	up(&(xport_dma[dma_id].dma_sem));

	return 0;
}

int xport_dma_input_check(unsigned int dma_id)
{
	int rt_val = -EFAULT;
	DMA_LIST *dma_list_ptr = NULL;

	if (down_interruptible(&(xport_dma[dma_id].dma_sem)))
		return rt_val;

	dma_list_ptr = &(xport_dma[dma_id].dma_list[xport_dma[dma_id].cur_index]);
	if (dma_list_ptr->next_index < dma_list_ptr->max_block_num) {
		rt_val = 0;
	}

	up(&(xport_dma[dma_id].dma_sem));

	return rt_val;
}

int xport_dma_half_empty_check(unsigned int dma_id,unsigned int chl_id)
{
	int rt_val = -EFAULT;

	unsigned int chl_wp, chl_rp, chl_wp_addr, chl_rp_addr;
	unsigned int space_cnt = 0;
	unsigned int tmp = 0;
	unsigned int val = 0;
	struct semaphore * sp = NULL ;

	val = xport_readl(XPORT_CFG_ADDR0);
	tmp = xport_readl(IN_CHL_CFG_ADDR(chl_id));

	sp = (((tmp>>29)&0x3) ==3 ? (&dir_dma[chl_id].dir_sem) : (&xport_dma[dma_id].dma_sem));

	if(down_interruptible(sp))
	{return -EFAULT;}


	tmp = xport_readl(IN_CHL_CFG_ADDR(chl_id));
	chl_rp_addr = IN_CHL_RP_ADDR(chl_id);
	chl_wp_addr = IN_CHL_WP_ADDR(chl_id);


	if(0x3 == ((tmp>>29)&0x3))
	{
		chl_wp_addr = XPORT_CHL_DMA_WP_ADDR(chl_id);
	}

	if(DMA_ENB==xport_get_bit(val,(chl_id*8)) && dma_id == xport_get_bit(val,(1+chl_id*8)))
	{

		chl_wp = xport_readl(chl_wp_addr);
		chl_rp = xport_readl(chl_rp_addr);


		if ((chl_wp ^ chl_rp) >> 31)
			{space_cnt = (chl_rp&0xfff)-(chl_wp&0xfff);}
		else
			{space_cnt = IN_CHL_UNIT_NUM_DEF(chl_id)+(chl_rp&0xfff)-(chl_wp&0xfff);}
		if (space_cnt > IN_CHL_UNIT_NUM_DEF(chl_id)>>1)
			rt_val = 0;
	}
	else
	{return -EFAULT;}


	up(sp);

	return rt_val;
}


int xport_dma_enable(unsigned int dma_id)
{
	unsigned int reg = 0;


	if(down_interruptible(&(xport_dma[dma_id].dma_sem)))
		return -EFAULT;

	reg = xport_readl(dma_id ?DMA_INPUT1_HEAD_ADDR: DMA_INPUT0_HEAD_ADDR);
	//printk("dma_input_head%d reg value:[%04x]  line:%d \n",dma_id,reg,__LINE__);


	xport_writel((dma_id ?DMA_INPUT1_HEAD_ADDR: DMA_INPUT0_HEAD_ADDR),xport_set_bit(reg,31));
	up(&(xport_dma[dma_id].dma_sem));
	//printk(" set dma[%d] succ! line:%d \n",dma_id,__LINE__);


	return 1;

}



