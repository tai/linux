
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

#include "xport_mips1800.h"
#include "xport_drv1800.h"


#ifdef CONFIG_ORION_XPORT_DEBUG
#define DEBUG(n, args...)				\
	do {						\
		if (n <= CONFIG_XPORT_DEBUG_VERBOSE)	\
		printk(KERN_INFO args);		\
	} while(0)
#else /* CONFIG_XPORT_DEBUG */
#define DEBUG(n, args...) do { } while(0)
#endif


typedef enum
{
	FW_NO_REPLAY = 0,
	FW_PREPARED = 1		
}FW_STATUS;



DECLARE_MUTEX(mips_cmd_sem);



/*get enable flag from  request addr in pull time*/
static unsigned int xport_pull_mips(int addr,unsigned int delay,unsigned int n, struct semaphore *sem)
{
	unsigned int ret = FW_PREPARED;
	unsigned int pull_cnt = n;
	unsigned int reg_v = 0;


	do
	{/*read enable flag of request info in mail box addr*/
		reg_v = xport_readl(addr);
		udelay(delay);
	}
	while((pull_cnt-- > 0) && xport_get_bit(reg_v,31));

	if (xport_get_bit(reg_v, 31))
	{
		ret = FW_NO_REPLAY;		
		up(sem);
	}


	return ret;
}


int xport_mips_write(unsigned int cmd, unsigned int req_dat)
{
	unsigned int cnt = 1000;

	union firmware_req req ;

	req.val = cmd;
	req.val = xport_set_bit(xport_set_bit(req.val,31),30);
	/* bit31 = 1, indicates that will have a request */
	/* bit30 = 1, indicates that a request will send to firmware */

	if (down_interruptible(&mips_cmd_sem)) {
		DEBUG(" mips_write error: down_interruptible () \n");
		return -1;
	}

	if(FW_NO_REPLAY == xport_pull_mips(MIPS_CMD_REQ,50,cnt,&mips_cmd_sem))
	{
		printk("[lixun] mips_write error: timeout \n");
		return -1;
	}


	//printk("[lixun]cmd = 0x%0x   data = 0x%08x\n", req.val, req_dat);

	xport_writel(MIPS_CMD_DATA, req_dat);	/* write a request to firmware */
	xport_writel(MIPS_CMD_REQ, req.val);

	up(&mips_cmd_sem);

	return 0;
}


int xport_mips_read(unsigned int cmd, unsigned int *req_dat)
{

	unsigned int cnt = 1000;
	union firmware_req req  ;


	req.val = cmd;
	req.val = xport_set_bit(req.val,31) ;
	/* bit31 = 1, indicates that will have a request */
	/* bit30 = 0, indicates that get information from firmware */

	if(down_interruptible(&mips_cmd_sem)) 
	{
		DEBUG(" mips_read error: down_interruptible () \n");
		return -1;
	}


	if(FW_NO_REPLAY == xport_pull_mips(MIPS_CMD_REQ,50,cnt,&mips_cmd_sem))
	{
		printk(" mips_read error: timeout0 \n");
		return -1;
	}


	xport_writel(MIPS_CMD_REQ, req.val);	/* send read-request to firmware */
	udelay(50);
	cnt = 1000;


	if(FW_NO_REPLAY == xport_pull_mips(MIPS_CMD_REQ,50,cnt,&mips_cmd_sem))
	{
		printk(" mips_read error: timeout0 \n");
		return -1;
	}


	*req_dat = xport_readl(MIPS_CMD_DATA);
	up(&mips_cmd_sem);

	return 0;
}

int xport_mips_read_ex(unsigned int cmd, unsigned int *req_dat, unsigned int *req_dat2)
{
	unsigned int cnt = 1000;
	union firmware_req req;

	req.val = cmd;
	req.val = xport_set_bit(req.val,31);
	/* bit31 = 1, indicates that will have a request */
	/* bit30 = 0, indicates that get information from firmware */

	if (down_interruptible(&mips_cmd_sem)) {
		DEBUG(" mips_read error: down_interruptible () \n");
		return -1;
	}

	if(FW_NO_REPLAY == xport_pull_mips(MIPS_CMD_REQ,50,cnt,&mips_cmd_sem))
	{
		DEBUG(KERN_INFO " mips_read error: timeout0 \n");
		return -1;   	  
	}


	xport_writel(MIPS_CMD_REQ, req.val);	/* send read-request to firmware */
	udelay(50);
	cnt = 1000;


	if(FW_NO_REPLAY == xport_pull_mips(MIPS_CMD_REQ,50,cnt,&mips_cmd_sem))
	{
		DEBUG(KERN_INFO " mips_read error: timeout1 \n");
		return -1;   	  
	}


	*req_dat = xport_readl(MIPS_CMD_DATA);
	*req_dat2 = xport_readl(MIPS_CMD_DATA2);
	up(&mips_cmd_sem);

	return 0;
}

EXPORT_SYMBOL(xport_mips_write);
EXPORT_SYMBOL(xport_mips_read);
EXPORT_SYMBOL(xport_mips_read_ex);


