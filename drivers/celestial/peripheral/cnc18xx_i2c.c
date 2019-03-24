//#include <linux/config.h>
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/rwsem.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include "cnc18xx_i2c.h"


typedef enum _I2C_ID_
{
	_i2c_0      =0,
	_i2c_1      =1,
}I2C_ID;
extern int pinmux_enable_i2c(I2C_ID I2C);


#define CFG_I2C_CLK  PCLK_FREQ/1000000

#ifdef CONFIG_CNC18XX_I2C_EEPROM
#define DEV_NUM 0
#else
#define DEV_NUM 1
#endif

#define I2C_SET	       _IOW('I', 0, unsigned long)
#define I2C_GET        _IOR('I', 1, unsigned long)
#define I2C_SETSPEED   _IOW('I', 2, unsigned long)
#define I2C_GETSPEED   _IOR('I', 3, unsigned long)
#define I2C_SETLOOPADDR   _IOW('I', 4, unsigned long)
#define I2C_CLRLOOPADDR   _IOW('I', 5, unsigned long)
#define I2C_GETLOOPINFO     _IOR('I', 6, unsigned long)
#define I2C_SETCHIPADDRESS  _IOW('I', 7, unsigned long)
#define I2C_GETCHIPADDRESS  _IOR('I', 8, unsigned long)
#define I2C_SETADDRALEN     _IOW('I', 9, unsigned long)

#define I2C_SPEED_100K 1
#define I2C_SPEED_400K 2
#define i2c_set_speed_100k(x)  x = (void *)(((unsigned long)1<<20) + (unsigned long)((unsigned long)(x) & 0xff0fffff))
#define i2c_set_speed_400k(x)  x = (void *)(((unsigned long)1<<21) + (unsigned long)((unsigned long)(x) & 0xff0fffff))
#define i2c_get_speed(x)  ((unsigned long)x >> 20) & 0xf

#define i2c_set_loop_address(x) x = (void *)((1<< 24) + ((unsigned long)x & 0xf0ffffff))
#define i2c_clr_loop_address(x) x = (void *)((unsigned long)x & 0xf0ffffff)
#define i2c_get_loop_info(x) ((unsigned long)x>>24) & 0xf

#define i2c_set_chip_address(x,chipaddress) x = (void *)(((unsigned long)x & 0xffffff00) | (chipaddress & 0xff))
#define i2c_get_chip_address(x) (unsigned char) ((unsigned long)x & 0xff)

#define i2c_get_sleeptime(x) ((unsigned long)x >> 8) & 0xff
#define i2c_get_chipaddress(x) ((unsigned long)x) & 0xff;
#define i2c_get_address_alen(x) (((unsigned long)x) >> 16) & 0xff;
#define i2c_set_address_alen(x,alen) x = (void *)(((unsigned long)x & 0xfff0ffff) | (alen & 0xf)<<16)
#define i2c_set_default_privatedata(x) x=(void *) (1 << 16)

#define I2C_DEBUG_LEVEL0	(0)	/* Quiet   */
#define I2C_DEBUG_LEVEL1	(1)	/* Audible */
#define I2C_DEBUG_LEVEL2	(2)	/* Loud    */
#define I2C_DEBUG_LEVEL3	(3)	/* Noisy   */

#ifdef CONFIG_CELESTIAL_I2C_DEBUG
#define DEBUG(n, args...)				\
 	do {						\
		if (n <= CONFIG_I2C_DEBUG_VERBOSE)	\
			printk(KERN_INFO args);		\
	} while(0)
#else /* CONFIG_I2C_DEBUG */
#define DEBUG(n, args...) do { } while(0)
#endif

static void __iomem *cs_i2c_base;
static void __iomem *cs_i2c2_base;
static spinlock_t cs_i2c_lock;

typedef unsigned char uchar;
static uchar previous_chip;
static unsigned int previous_speed_cnt;


static unsigned int ss_ic_sclh_cnt;
static unsigned int ss_ic_scl_cnt;
static unsigned int fs_ic_sclh_cnt;
static unsigned int fs_ic_scl_cnt;
static unsigned int default_speed_cnt;

#define i2c_writeb( v,a)    writeb(v, (cs_i2c_base + (a)))
#define i2c_writew(v,a,n)    ((n==0)?writew(v, (cs_i2c_base + (a))):writew(v, (cs_i2c2_base + (a)))) 
#define i2c_writel(v,a)    writel(v, (cs_i2c_base + (a)))

#define i2c_readb(a)       readb((cs_i2c_base + (a)))

#define i2c_readw(a,n)    ((n==0)?readw((cs_i2c_base + (a))):readw((cs_i2c2_base + (a))))

		
#define i2c_readl(a)       readl((cs_i2c_base + (a)))

static int i2c_read(uchar chip,uint addr, unsigned int speed_cnt,int alen,
	 uchar * buffer, int len,unsigned num)
{
    int i;
	int cmd_cnt;
	int data_cnt;
    unsigned int time_cnt;
	unsigned char *byte_addr = (unsigned char *) &addr;
	unsigned char *p = buffer;

    time_cnt=0;
    while(i2c_readw(IC_STAT,num)&0x20)
    {
    	if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET,num);
    		i2c_writew(0x0,IC_RESET,num);
    		i2c_writew(0x1,IC_ENABLE,num);
    		i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}
    	time_cnt++;
    }
	if(speed_cnt!=previous_speed_cnt)
		{
			i2c_writew(0x0, IC_ENABLE,num);
			while(!(i2c_readw(IC_STAT,num)& 0x1));							//wait disabled
			if((speed_cnt==ss_ic_scl_cnt))									//Standard Speed
				{
					i2c_writew(ss_ic_sclh_cnt,IC_SCLH_CNT,num);
					i2c_writew(ss_ic_scl_cnt-ss_ic_sclh_cnt,IC_SCLL_CNT,num);
					previous_speed_cnt = ss_ic_scl_cnt;
				}
			else if(speed_cnt==fs_ic_scl_cnt)								//Fast Speed
				{
					
					i2c_writew(fs_ic_sclh_cnt,IC_SCLH_CNT,num);
					i2c_writew(fs_ic_scl_cnt-fs_ic_sclh_cnt,IC_SCLL_CNT,num);
					previous_speed_cnt =fs_ic_scl_cnt;
					
				}
		}
	i2c_writew(0x1, IC_ENABLE,num);
	
	if (alen > 0){
		i2c_writew(((chip<<1)&0xff)|0x100,IC_DATA_CMD,num);
	}
	for (i = alen - 1; i >= 0; i--)
	{
		i2c_writew(0x200|(byte_addr[i]& 0xff),IC_DATA_CMD,num);
	}
	i2c_writew(0x101|((chip<<1)&0xff),IC_DATA_CMD,num);
	data_cnt = 0;
	cmd_cnt = 0;
	time_cnt=0;
	while (data_cnt < len) {
		/*if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET);
    		i2c_writew(0x0,IC_RESET);
    		i2c_writew(0x1,IC_ENABLE);
    		i2c_writew(0x700,IC_DATA_CMD);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}*/
    	time_cnt++;
    	
		while ((cmd_cnt < len) && (i2c_readw(IC_STAT,num) & 0x2)) {
			
			if(cmd_cnt+1==len)
				{
				i2c_writew(0x600,IC_DATA_CMD,num);		//write stop bit
			  }
			else
				{
				i2c_writew(0x400, IC_DATA_CMD,num);
			  }
			cmd_cnt++;
		}

		if (i2c_readw(IC_STAT,num) & 0x8) {
			*p = i2c_readw(IC_DATA_CMD,num);
			p++;
			data_cnt++;
		}

		if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
			i2c_writew(0x03,IC_FIFO_CLR,num);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}
	time_cnt=0;
	while (!(i2c_readw(IC_STAT,num)&0x04))
	{
		if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET,num);
    		i2c_writew(0x0,IC_RESET,num);
    		i2c_writew(0x1,IC_ENABLE,num);
    		i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}
    	time_cnt++;
    	
		if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
			i2c_writew(0x03,IC_FIFO_CLR,num);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
			}
		}             //wait all data send
		
	time_cnt=0;
	while (i2c_readw(IC_STAT,num) & 0x20)
	{
		if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
			i2c_writew(0x03,IC_FIFO_CLR,num);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
		
		if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET,num);
    		i2c_writew(0x0,IC_RESET,num);
    		i2c_writew(0x1,IC_ENABLE,num);
    		i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}
    	time_cnt++;
		}
						//wait for stop signal
	return 0;
}

static int
i2c_write(uchar chip,uint addr, unsigned int speed_cnt, int alen,
	  uchar * buffer, int len,unsigned num)
{
	int i;
	int data_cnt;
    unsigned int  time_cnt;
	unsigned char *byte_addr = (unsigned char *) &addr;
	unsigned char *p = buffer;
    
    time_cnt=0;
    while(i2c_readw(IC_STAT,num)&0x20)
    {
    	if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET,num);
    		i2c_writew(0x0,IC_RESET,num);
    		i2c_writew(0x1,IC_ENABLE,num);
    		i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}
    	time_cnt++;
    }

if(speed_cnt!=previous_speed_cnt)
		{
			i2c_writew(0x0, IC_ENABLE,num);
			while(!(i2c_readw(IC_STAT,num)& 0x1));							//wait disabled
			if((speed_cnt==ss_ic_scl_cnt))									//Standard Speed
				{
					i2c_writew(ss_ic_sclh_cnt,IC_SCLH_CNT,num);
					i2c_writew(ss_ic_scl_cnt-ss_ic_sclh_cnt,IC_SCLL_CNT,num);
					previous_speed_cnt = ss_ic_scl_cnt;
				}
			else if(speed_cnt==fs_ic_scl_cnt)								//Fast Speed
				{
					
					i2c_writew(fs_ic_sclh_cnt,IC_SCLH_CNT,num);
					i2c_writew(fs_ic_scl_cnt-fs_ic_sclh_cnt,IC_SCLL_CNT,num);
					previous_speed_cnt =fs_ic_scl_cnt;
					
				}
		}
		
  i2c_writew(0x1, IC_ENABLE,num);
	i2c_writew(((chip<<1)&0xff)|0x100,IC_DATA_CMD,num);
	for (i = alen - 1; i >= 0; i--)
	{
		i2c_writew(0x200|byte_addr[i],IC_DATA_CMD,num);
	}
	
	data_cnt = 0;

	time_cnt=0;
	while (data_cnt < len) {
		
		/*if(unlikely(time_cnt>=6000000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET);
    		i2c_writew(0x0,IC_RESET);
    		i2c_writew(0x1,IC_ENABLE);
    		i2c_writew(0x700,IC_DATA_CMD);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}*/
    	time_cnt++;
    	
    	
		if (i2c_readw(IC_STAT,num) & 0x2) {
			if(data_cnt!=len-1)
				{
				i2c_writew(0x200|(*p), IC_DATA_CMD,num);
				}
			else
				{
				i2c_writew(0x300|(*p),IC_DATA_CMD,num);				//Stop
				
			}
			p++;
			data_cnt++;
		}

		if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
			
			i2c_writew(0x03,IC_FIFO_CLR,num);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);		//Stop
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}

	time_cnt=0;
	while (!(i2c_readw(IC_STAT,num)&0x04))
	{
		if(unlikely(time_cnt>=100000))          //Bus Err
    	{
    		i2c_writew(0x1,IC_RESET,num);
    		i2c_writew(0x0,IC_RESET,num);
    		i2c_writew(0x1,IC_ENABLE,num);
    		i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    		previous_chip = 0;
    		DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    		return 1;
    	}
    	time_cnt++;
		
		  if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
			i2c_writew(0x03,IC_FIFO_CLR,num);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);		//Stop
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
		}             //wait all data send
	time_cnt=0;
	while (i2c_readw(IC_STAT,num) & 0x20)
	{
			if(unlikely(time_cnt>=100000))          //Bus Err
    		{
    			i2c_writew(0x1,IC_RESET,num);
    			i2c_writew(0x0,IC_RESET,num);
    			i2c_writew(0x1,IC_ENABLE,num);
    			i2c_writew(0x700,IC_DATA_CMD,num);     //write stop bit
    			previous_chip = 0;
    			DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
    			return 1;
    		}
    		time_cnt++;
    	
			if (unlikely((i=i2c_readw(IC_RAW_INTR_STAT,num)) & 0xe0)) {
			
			DEBUG(I2C_DEBUG_LEVEL0,"Chipaddress=0x%x, subaddr=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     chip, addr, i);
				
			i2c_writew(0x03,IC_FIFO_CLR,num);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					i2c_writew(0x40,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					i2c_writew(0x80,IC_INTR_CLR,num);
					DEBUG(I2C_DEBUG_LEVEL0,"Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					i2c_writew(0x20,IC_INTR_CLR,num);
					i2c_writew(0x700,IC_DATA_CMD,num);		//Stop
					DEBUG(I2C_DEBUG_LEVEL0,"Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
		
		}
	return 0;
}

#ifdef CONFIG_CNC18XX_I2C_EEPROM
static int
cs_i2c_eeprom_open(struct inode *inode, struct file *file)
{
	file->private_data =
	    (void *) (((CONFIG_CNC18XX_I2C_EEPROM_PAGE_SIZE & 0xfff) << 20) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_SUBADDR_LEN & 0xf) << 16) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_SLEEP_TIME & 0xff) << 8) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_CHIP_1_ADDR & 0xff) << 0)
	    );
	return 0;
}
#endif

#ifdef CONFIG_CNC18XX_I2C_MULTI_EEPROMS
static int
cs_i2c_multi_eeprom_open(struct inode *inode, struct file *file)
{
	file->private_data =
	    (void *) (((CONFIG_CNC18XX_I2C_EEPROM_2_PAGE_SIZE & 0xfff) << 20) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_2_SUBADDR_LEN & 0xf) << 16) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_2_SLEEP_TIME & 0xff) << 8) |
		      ((CONFIG_CNC18XX_I2C_EEPROM_CHIP_2_ADDR & 0xff) << 0)
	    );
	return 0;
}
#endif

static int
cs_i2c_open(struct inode *inode, struct file *file)
{
		unsigned m = 0;		
		i2c_writew(0x0, IC_ENABLE,m);
#ifdef CONFIG_CNC18XX_I2C_FASTSPEED
		i2c_writew(fs_ic_sclh_cnt, IC_SCLH_CNT,m);
		i2c_writew(fs_ic_scl_cnt - fs_ic_sclh_cnt, IC_SCLL_CNT,m);
		previous_speed_cnt = fs_ic_scl_cnt;
		default_speed_cnt = fs_ic_scl_cnt;
		printk(KERN_INFO "CNC I2C default 400KHZ\n");
#else
		i2c_writew(ss_ic_sclh_cnt, IC_SCLH_CNT,m);
		i2c_writew(ss_ic_scl_cnt - ss_ic_sclh_cnt, IC_SCLL_CNT,m);
		previous_speed_cnt = ss_ic_scl_cnt;
		default_speed_cnt = ss_ic_scl_cnt;

		printk(KERN_INFO "CNC I2C default 100KHZ\n");
#endif
	i2c_set_default_privatedata(file->private_data);

	if (default_speed_cnt == fs_ic_scl_cnt)
		i2c_set_speed_400k(file->private_data);
	else
		i2c_set_speed_100k(file->private_data);

	return 0;
}
#ifdef CONFIG_CNC18XX_I2C_DEV2
static int
cs_i2c2_open(struct inode *inode, struct file *file)
{
		unsigned m = 1;
		i2c_writew(0x0, IC_ENABLE,m);
#ifdef CONFIG_CNC18XX_I2C_FASTSPEED
		i2c_writew(fs_ic_sclh_cnt, IC_SCLH_CNT,m);
		i2c_writew(fs_ic_scl_cnt - fs_ic_sclh_cnt, IC_SCLL_CNT,m);
		previous_speed_cnt = fs_ic_scl_cnt;
		default_speed_cnt = fs_ic_scl_cnt;
		printk(KERN_INFO "CNC I2C2 initial: default 400KHZ\n");
#else
		i2c_writew(ss_ic_sclh_cnt, IC_SCLH_CNT,m);
		i2c_writew(ss_ic_scl_cnt - ss_ic_sclh_cnt, IC_SCLL_CNT,m);
		previous_speed_cnt = ss_ic_scl_cnt;
		default_speed_cnt = ss_ic_scl_cnt;

		printk(KERN_INFO "CNC I2C2 initial:default 100KHZ\n");
#endif
	i2c_set_default_privatedata(file->private_data);
	if (default_speed_cnt == fs_ic_scl_cnt)
		i2c_set_speed_400k(file->private_data);
	else
		i2c_set_speed_100k(file->private_data);

	return 0;
}
#endif



static ssize_t
cs_i2c_read(struct file *file, char __user * buffer, size_t len,
	       loff_t * offset)
{
	unsigned long flags;
	unsigned int ret;
	uchar chip;
	int alen;
	int rest;
	unsigned int burst_len;
	char *i2c_buffer;
	char __user *user_ptr;
	unsigned int speedcnt;
	unsigned char loop_info;
	unsigned m;
	chip = ((unsigned long) file->private_data) & 0xff;
	alen = (((unsigned long) file->private_data) >> 16) & 0xf;
	loop_info = (unsigned char) i2c_get_loop_info(file->private_data);

	if (strncmp("cs_i2c1", file->f_dentry->d_iname, 7) == 0)
		m = 0;
	else if(strncmp("cs_i2c2", file->f_dentry->d_iname, 7) == 0)
		m = 1;
	else{
			printk("Baffling Error !\n");
			return -1;	
		}
		
	
	
	switch (i2c_get_speed(file->private_data)) {
	case I2C_SPEED_100K:
		speedcnt = ss_ic_scl_cnt;
		break;
	case I2C_SPEED_400K:
		speedcnt = fs_ic_scl_cnt;
		break;
	default:
		speedcnt = default_speed_cnt;
		break;
	}

	if (loop_info)
		burst_len = 1;
	//else if (len > 256)
	//	burst_len = 256;
	else
		burst_len = len;

	i2c_buffer = (char *) kmalloc(burst_len, GFP_KERNEL);
	user_ptr = buffer;
	rest = len;
	DEBUG(I2C_DEBUG_LEVEL0, "burst_len =%d, len=%d,chip=%d,speedcnt=%d, alen=%d,offset=%d\n", burst_len, len,chip,speedcnt,alen,(int)*offset);

	while (rest > 0) {

		spin_lock_irqsave(&cs_i2c_lock, flags);
		ret =i2c_read(chip,*offset, speedcnt, alen, i2c_buffer, burst_len,m);
		spin_unlock_irqrestore(&cs_i2c_lock, flags);

		if (unlikely(ret)) {
			kfree(i2c_buffer);
			msleep(2);
			return -EIO;
		}
		DEBUG(I2C_DEBUG_LEVEL0, "burst_len =%d, len=%d,speedcnt=%d, alen=%d, chip=%d, offset=%d\n", burst_len, len,speedcnt,alen,chip,(int)*offset);

		if (unlikely(copy_to_user(user_ptr, i2c_buffer, burst_len))) {
			kfree(i2c_buffer);
			return -EFAULT;
		}
		*offset += burst_len;
		user_ptr += burst_len;
		rest -= burst_len;
		if (rest < burst_len){
			burst_len = rest;
		}
		
	}

	kfree(i2c_buffer);
	return len;
}


static ssize_t
cs_i2c_write(struct file *file, const char __user * buffer, size_t len,
		loff_t * offset)
{
	unsigned long flags;
	unsigned int ret;
	uchar chip;
	int alen;
	int sleep_time;
	int rest;
	unsigned int burst_len;
	char *i2c_buffer;
	char *user_ptr;
	unsigned int speedcnt;
	unsigned char loop_info;
	unsigned m;

	chip = ((unsigned long) file->private_data) & 0xff;
	sleep_time = (((unsigned long) file->private_data) >> 8) & 0xff;
	alen = (((unsigned long) file->private_data) >> 16) & 0xf;
	loop_info = (unsigned char) i2c_get_loop_info(file->private_data);

	if (strncmp("cs_i2c1", file->f_dentry->d_iname, 7) == 0)
		m = 0;
	else if(strncmp("cs_i2c2", file->f_dentry->d_iname, 7) == 0)
		m = 1;
	else{
		printk("Baffling Error !\n");
		return -1;	
		}
	switch (i2c_get_speed(file->private_data)) {
	case I2C_SPEED_100K:
		speedcnt = ss_ic_scl_cnt;
		break;
	case I2C_SPEED_400K:
		speedcnt = fs_ic_scl_cnt;
		break;
	default:
		speedcnt = default_speed_cnt;
		break;
	}

	if (loop_info)
		burst_len = 1;
	//else if (len > 256)
		//burst_len = 256;
	else
		burst_len = len;
	i2c_buffer = (char *) kmalloc(burst_len, GFP_KERNEL);
	user_ptr = (char *) buffer;
	rest = len;

	DEBUG(I2C_DEBUG_LEVEL3, "burst_len =%d, len=%d,speedcnt=%d, alen=%d, chip=%d, offset=%d, loop_info=%d\n", burst_len, len,speedcnt,alen,chip,(int)*offset,loop_info);
	while (rest > 0) {

		if (unlikely(copy_from_user(i2c_buffer, user_ptr, burst_len))) {
			kfree(i2c_buffer);
			return -EFAULT;
		}
		DEBUG(I2C_DEBUG_LEVEL3, "i2c_buffer=%d\n",(*i2c_buffer) & 0xff);
	       DEBUG(I2C_DEBUG_LEVEL3, "burst_len =%d, len=%d,speedcnt=%d, alen=%d, chip=%d, offset=%d, loop_info=%d\n", burst_len, len,speedcnt,alen,chip,(int)*offset,loop_info);
		spin_lock_irqsave(&cs_i2c_lock, flags);
		ret = i2c_write(chip, *offset, speedcnt, alen, i2c_buffer,burst_len,m);
		spin_unlock_irqrestore(&cs_i2c_lock, flags);

		if (unlikely(ret)) {
			kfree(i2c_buffer);
			msleep(2);
			return -EIO;
		}
		*offset += burst_len;
		user_ptr += burst_len;
		rest -= burst_len;

		if (rest < burst_len)
			burst_len = rest;

		if (sleep_time > 0)
			msleep(sleep_time);

	}

	kfree(i2c_buffer);
	return len;
}


#ifdef CONFIG_CNC18XX_I2C_EEPROM
static ssize_t
cs_i2c_eeprom_write(struct file *file, const char __user * buffer,
		       size_t len, loff_t * offset)
{
	unsigned long flags;
	unsigned int ret;
	uchar chip;
	int alen;
	int sleep_time;
	int page_size;
	int rest;
	int burst_len;
	char *i2c_buffer;
	char *user_ptr;
	
	chip = ((unsigned long) file->private_data) & 0xff;
	sleep_time = (((unsigned long) file->private_data) >> 8) & 0xff;
	alen = (((unsigned long) file->private_data) >> 16) & 0xf;
	page_size = (((unsigned long) file->private_data) >> 20) & 0xfff;
	page_size = page_size ? page_size : 256;

	i2c_buffer = (char *) kmalloc(page_size, GFP_KERNEL);
	user_ptr = (char *) buffer;
	rest = len;

	while (rest > 0) {
		if (!sleep_time || !(*offset & (page_size - 1))) {
			burst_len = rest > page_size ? page_size : rest;
		} else {
			/* EEPROM page write */
			burst_len = page_size - (*offset & (page_size - 1));
			burst_len = burst_len > rest ? rest : burst_len;
		}

		if (copy_from_user(i2c_buffer, user_ptr, burst_len)) {
				kfree(i2c_buffer);
			return -EFAULT;
		}
		spin_lock_irqsave(&cs_i2c_lock, flags);
		ret =
			i2c_write(chip, *offset, default_speed_cnt, alen, i2c_buffer, burst_len,DEV_NUM);
		spin_unlock_irqrestore(&cs_i2c_lock, flags);

		if (ret) {
			kfree(i2c_buffer);
			return -EIO;
		}

		user_ptr += burst_len;
		*offset += burst_len;
		rest -= burst_len;
		if (sleep_time > 0)
			msleep(sleep_time);

	}

	kfree(i2c_buffer);
	return len;
}

static ssize_t
cs_i2c_eeprom_read(struct file *file, char __user * buffer, size_t len,
		      loff_t * offset)
{
	unsigned long flags;
	unsigned int ret;
	uchar chip;
	int alen;
	int page_size;
	int rest;
	int burst_len;
	char *i2c_buffer;
	char *user_ptr;
	
	chip = ((unsigned long) file->private_data) & 0xff;
	alen = (((unsigned long) file->private_data) >> 16) & 0xf;
	page_size = (((unsigned long) file->private_data) >> 20) & 0xfff;
	page_size = page_size ? page_size : 256;

	i2c_buffer = (char *) kmalloc(page_size, GFP_KERNEL);
	user_ptr = buffer;
	rest = len;

	while (rest > 0) {
		burst_len = rest > page_size ? page_size : rest;

		spin_lock_irqsave(&cs_i2c_lock, flags);
		ret = i2c_read(chip, *offset, default_speed_cnt, alen, i2c_buffer,burst_len,DEV_NUM);
		spin_unlock_irqrestore(&cs_i2c_lock, flags);

		if (ret) {
			kfree(i2c_buffer);
			return -EIO;
		}

		if (copy_to_user(user_ptr, i2c_buffer, burst_len)) {
			kfree(i2c_buffer);
			return -EFAULT;
		}

		user_ptr += burst_len;
		*offset += burst_len;
		rest -= burst_len;
	}

	kfree(i2c_buffer);
	return len;
}
#endif

static loff_t
cs_i2c_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t ret;

	down_read(&file->f_dentry->d_inode->i_alloc_sem);
	switch (orig) {
	case 0: /* SEEK_SET*/
		file->f_pos = offset;
		ret = file->f_pos;
		break;
	case 1: /* SEEK_CUR*/
		file->f_pos += offset;
		ret = file->f_pos;
		break;
	default:
		ret = -EINVAL;
	}
	up_read(&file->f_dentry->d_inode->i_alloc_sem);
	return ret;
}

static int
cs_i2c_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	char speed;
	switch (cmd) {
	case I2C_SET:
		file->private_data = (void *) arg;
		return 0;
	case I2C_GET:
		return (int) file->private_data;

	case I2C_SETCHIPADDRESS:
		i2c_set_chip_address(file->private_data, arg);
		return 0;
	case I2C_GETCHIPADDRESS:
		return (int) i2c_get_chip_address(file->private_data);
	case I2C_SETSPEED:
		if (arg == I2C_SPEED_100K)
			i2c_set_speed_100k(file->private_data);
		else if (arg == I2C_SPEED_400K)
			i2c_set_speed_400k(file->private_data);
		else
			return -EINVAL;
	case I2C_GETSPEED:
		speed = i2c_get_speed(file->private_data);
		if (speed == I2C_SPEED_100K || speed == I2C_SPEED_400K)
			return (int) speed;
		else
			return 0;
	case I2C_SETLOOPADDR:
		i2c_set_loop_address(file->private_data);
		return 0;
	case I2C_CLRLOOPADDR:
		i2c_clr_loop_address(file->private_data);
		return 0;
	case I2C_GETLOOPINFO:
		return i2c_get_loop_info(file->private_data);

	case I2C_SETADDRALEN:
		if (arg >= 0) {
			i2c_set_address_alen(file->private_data, arg);
			return 0;
		} else
			return -EINVAL;
	default:
		return -EINVAL;
	}
}

static struct file_operations cs_i2c_fops = {
	.owner = THIS_MODULE,
	.read = cs_i2c_read,
	.write = cs_i2c_write,
	.llseek = cs_i2c_lseek,
	.ioctl = cs_i2c_ioctl,
	.open = cs_i2c_open,
};

static struct miscdevice cs_i2c_miscdev = {
	MISC_DYNAMIC_MINOR,
	"cs_i2c1",
	&cs_i2c_fops
};

#ifdef CONFIG_CNC18XX_I2C_EEPROM
static struct file_operations cs_i2c_eeprom_fops = {
	.owner = THIS_MODULE,
	.open = cs_i2c_eeprom_open,
	.read = cs_i2c_eeprom_read,
	.write = cs_i2c_eeprom_write,
	.llseek = cs_i2c_lseek,
};

static struct miscdevice cs_i2c_eeprom_miscdev = {
	MISC_DYNAMIC_MINOR,
	"eeprom",
	&cs_i2c_eeprom_fops
};
#endif

#ifdef CONFIG_CNC18XX_I2C_MULTI_EEPROMS
static struct file_operations cs_i2c_multi_eeprom_fops = {
	.owner = THIS_MODULE,
	.open = cs_i2c_multi_eeprom_open,
	.read = cs_i2c_eeprom_read,
	.write = cs_i2c_eeprom_write,
	.llseek = cs_i2c_lseek,
};

static struct miscdevice cs_i2c_multi_eeprom_miscdev = {
	MISC_DYNAMIC_MINOR,
	"eeprom2",
	&cs_i2c_multi_eeprom_fops
};
#endif

#ifdef CONFIG_CNC18XX_I2C_DEV2
static struct file_operations cs_i2c_dev2_fops = {
	.owner = THIS_MODULE,
	.read = cs_i2c_read,
	.write = cs_i2c_write,
	.llseek = cs_i2c_lseek,
	.ioctl = cs_i2c_ioctl,
	.open = cs_i2c2_open,
};

static struct miscdevice cs_i2c_dev2_miscdev = {
	MISC_DYNAMIC_MINOR,
	"cs_i2c2",
	&cs_i2c_dev2_fops
};
#endif

int __init
cs_i2c_init(void)
{
	if (misc_register(&cs_i2c_miscdev))
		return -ENODEV;

#ifdef CONFIG_CNC18XX_I2C_EEPROM
	if (misc_register(&cs_i2c_eeprom_miscdev))
		return -ENODEV;
#endif
#ifdef CONFIG_CNC18XX_I2C_DEV2
	if (misc_register(&cs_i2c_dev2_miscdev))
		return -ENODEV;
#endif

#ifdef CONFIG_CNC18XX_I2C_MULTI_EEPROMS
	if (misc_register(&cs_i2c_multi_eeprom_miscdev))
		return -ENODEV;
#endif


	cs_i2c_base =(void *) VA_I2C_BASE;   

#ifdef CONFIG_CNC18XX_I2C_DEV2
	pinmux_enable_i2c(1);
	cs_i2c2_base =(void *) VA_I2C2_BASE;  
#endif
	{

		ss_ic_sclh_cnt = (10 * CFG_I2C_CLK) / 4 ;
		ss_ic_scl_cnt = CFG_I2C_CLK * 5;

		fs_ic_sclh_cnt = (10 * CFG_I2C_CLK) / 24 ;
		fs_ic_scl_cnt = (CFG_I2C_CLK * 10) / 8 ;


	}

	spin_lock_init(&cs_i2c_lock);

	return 0;
}

static void __exit
cs_i2c_exit(void)
{

	misc_deregister(&cs_i2c_miscdev);

}

module_init(cs_i2c_init);
module_exit(cs_i2c_exit);

MODULE_LICENSE("GPL");
