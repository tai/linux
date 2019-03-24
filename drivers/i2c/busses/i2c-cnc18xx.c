/*
*	Cavium CNC1800L Linux I2C compatible Driver
*	(C)Copyright Cavium Inc.
*
*	Originally ported by Hai Ran
*	Cleaned up by yguo@caviumnetworks.com
*
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>

#include <mach/i2c.h>

/* ----- global defines ----------------------------------------------- */


#define         IC_ENABLE                       0x00
#define         IC_SCLH_CNT                     0x04
#define         IC_SCLL_CNT                     0x08
#define         IC_DATA_CMD                     0x0c
#define         IC_INTR_STAT            0x10
#define         IC_RAW_INTR_STAT        0x14
#define         IC_INTR_MASK            0x18
#define         IC_INTR_CLR                     0x1c
#define         IC_STAT                         0x20
#define         IC_RX_TL                        0x24
#define         IC_TX_TL                        0x28
#define         IC_TXFLR                        0x2c
#define         IC_RXFLR                        0x30
#define         IC_FIFO_CLR                     0x34
#define         IC_RESET                        0x38
#define         IC_SLV_TIMEOUT_L        0x3c
#define         IC_SLV_TIMEOUT_H        0x3e
#define         IC_SLV_TIMEOUT_EN       0x40


#define CNC18XX_I2C_TIMEOUT	(1*HZ)
				 
#define i2c_set_chip_address(x,chipaddress) x = (void *)(((unsigned long)x & 0xffffff00) | (chipaddress & 0xff))


#define IC_RAW_INTR_STAT   0x14
#define I2C_SLV_TIMEOUT    0x80
#define I2C_ARBI_LOST	   0x40
#define I2C_SLAVE_NACK     0x20
#define I2C_TX_UNDER_TH    0x10
#define I2C_TX_OVERFLOW    0x08
#define I2C_RX_OVER_TH     0x04
#define I2C_RX_OVERFLOW    0x02
#define I2C_RX_UNDERFLOW   0x01

#define CNC18XX_I2C_MDR_STP	(1 << 11)
#define CNC18XX_I2C_MDR_TRX	(1 << 9)
#define CNC18XX_I2C_MDR_XA	(1 << 8)

static unsigned int ss_ic_sclh_cnt;
static unsigned int ss_ic_scl_cnt;
static unsigned int fs_ic_sclh_cnt;
static unsigned int fs_ic_scl_cnt;
static DECLARE_MUTEX(i2c_rw_mutex);
static unsigned char previous_chip;
static unsigned int previous_speed_cnt;

#define CFG_I2C_CLK  PCLK_FREQ/1000000
#define I2C_SPEED_100K 100
#define I2C_SPEED_400K 400

struct cnc18xx_i2c_dev {
	struct device           *dev;
	void __iomem		*base;
	struct completion	cmd_complete;
	struct clk              *clk;
	int			cmd_err;
	u8			*buf;
	size_t			buf_len;
	int			irq;
	u8			terminate;
	struct i2c_adapter	adapter;
	unsigned int slave_addr;
	unsigned int bus_frq;
	unsigned short flags;
};

/* default platform data to use if not supplied in the platform_device */
static struct cnc18xx_i2c_platform_data cnc18xx_i2c_platform_data_default = {
	.bus_freq	= 100,
	.bus_delay	= 0,
};

static inline void cnc18xx_i2c_write_reg(struct cnc18xx_i2c_dev *i2c_dev,
					  u16 val,int reg)
{
	__raw_writew(val, i2c_dev->base + reg);
}

static inline u16 cnc18xx_i2c_read_reg(struct cnc18xx_i2c_dev *i2c_dev, int reg)
{
	return __raw_readw(i2c_dev->base + reg);
}

/*
 * This functions configures I2C and brings I2C out of reset.
 * This function is called during I2C init function. This function
 * also gets called if I2C encounters any errors.
 */
static int i2c_cnc18xx_init(struct cnc18xx_i2c_dev *dev)
{

	ss_ic_sclh_cnt = (10 * CFG_I2C_CLK) / 4 ;
	ss_ic_scl_cnt = CFG_I2C_CLK * 5;

	fs_ic_sclh_cnt = (10 * CFG_I2C_CLK) / 24 ;
	fs_ic_scl_cnt = (CFG_I2C_CLK * 10) / 8 ;

		
	cnc18xx_i2c_write_reg(dev,0x0, IC_ENABLE);

	cnc18xx_i2c_write_reg(dev,ss_ic_sclh_cnt, IC_SCLH_CNT);
	cnc18xx_i2c_write_reg(dev,ss_ic_scl_cnt - ss_ic_sclh_cnt, IC_SCLL_CNT);
	previous_speed_cnt = ss_ic_scl_cnt;


	return 0;
	
}

static ssize_t
cnc_i2c_read(struct cnc18xx_i2c_dev *dev)
{
	unsigned int time_cnt=0;
	unsigned int cmd_cnt=0;
	unsigned int data_cnt=0;
	unsigned char *p=dev->buf;
	int i=0;
	
	data_cnt = 0;
	cmd_cnt = 0;
	time_cnt=0;

	while (data_cnt < dev->buf_len) {
		while ((cmd_cnt < dev->buf_len) && (cnc18xx_i2c_read_reg(dev,IC_STAT) & 0x2)) {
			if(dev->buf_len == cmd_cnt + 1 && dev->flags){
				cnc18xx_i2c_write_reg(dev,0x600,IC_DATA_CMD);		//write stop bit
			}else{
				cnc18xx_i2c_write_reg(dev,0x400, IC_DATA_CMD);
			}
			cmd_cnt++;
		}

		if (cnc18xx_i2c_read_reg(dev,IC_STAT) & 0x8) {
			*p = cnc18xx_i2c_read_reg(dev,IC_DATA_CMD);
			p++;
			data_cnt++;
		}
			
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     dev->slave_addr, i);
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS0 Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}
	time_cnt=0;
/* Messy error clean-up */
	while (!(cnc18xx_i2c_read_reg(dev,IC_STAT)&0x04))
	{
		if(unlikely(time_cnt>=100000))          //Bus Err
    		{
    			cnc18xx_i2c_write_reg(dev,0x1,IC_RESET);
	    		cnc18xx_i2c_write_reg(dev,0x0,IC_RESET);
    			cnc18xx_i2c_write_reg(dev,0x1,IC_ENABLE);
    			cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);     //write stop bit
	    		previous_chip = 0;
    			pr_err("BUS1 Error:Timeout!\n");
    			return -1;
    		}
    		time_cnt++;
    	
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     dev->slave_addr, i);
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS2 Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}             //wait all data send
		
	time_cnt=0;
	while (cnc18xx_i2c_read_reg(dev,IC_STAT) & 0x20)
	{
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     dev->slave_addr, i);
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);			//clear tx/rx fifo			
			if(i&0x40)								//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS3 Error:Timeout!\n");
				}
			if(i&0x20)								//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
		
		if(unlikely(time_cnt>=100000))          //Bus Err
    		{
    			cnc18xx_i2c_write_reg(dev,0x1,IC_RESET);
	    		cnc18xx_i2c_write_reg(dev,0x0,IC_RESET);
    			cnc18xx_i2c_write_reg(dev,0x1,IC_ENABLE);
    			cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);     //write stop bit
	    		previous_chip = 0;
    			pr_err("BUS4 Error:Timeout!\n");
    			return -1;
	    	}
    		time_cnt++;
	}
						//wait for stop signal
	return dev->buf_len;
}


static int
cnc_i2c_write(struct cnc18xx_i2c_dev *dev)
{
	int i;
	int data_cnt;
	unsigned int  time_cnt;
	unsigned char *p = dev->buf;
	unsigned int len = dev->buf_len;
	
 	time_cnt=0;
	data_cnt = 0;
	
	while (data_cnt < len) {
		if (cnc18xx_i2c_read_reg(dev,IC_STAT) & 0x2) {
			if(data_cnt != len - 1){
				cnc18xx_i2c_write_reg(dev,0x200|(*p), IC_DATA_CMD);
			}else{
				if (dev->flags)
					cnc18xx_i2c_write_reg(dev,0x300|(*p),IC_DATA_CMD);				//Stop
				else
					cnc18xx_i2c_write_reg(dev,0x200|(*p), IC_DATA_CMD);
			}
			p++;
			data_cnt++;
		}
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			    dev->slave_addr, i);
			
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS5 Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);		//Stop
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}

	if(!(dev->flags)){
			return 0;
	}

	time_cnt=0;
/* Messy error clean-up */
	while (!(cnc18xx_i2c_read_reg(dev,IC_STAT)&0x04))
	{
		if(unlikely(time_cnt>=100000))          //Bus Err
    		{
    			cnc18xx_i2c_write_reg(dev,0x1,IC_RESET);
	    		cnc18xx_i2c_write_reg(dev,0x0,IC_RESET);
    			cnc18xx_i2c_write_reg(dev,0x1,IC_ENABLE);
    			cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);     //write stop bit
	    		previous_chip = 0;
    			pr_err("BUS6 Error:Timeout!\n");
    			return -1;
	    	}
    		time_cnt++;
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     dev->slave_addr, i);
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS7 Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);		//Stop
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
	}             //wait all data send
	time_cnt=0;
	while (cnc18xx_i2c_read_reg(dev,IC_STAT) & 0x20)
	{
		if(unlikely(time_cnt>=100000))          //Bus Err
    		{
    			cnc18xx_i2c_write_reg(dev,0x1,IC_RESET);
    			cnc18xx_i2c_write_reg(dev,0x0,IC_RESET);
    			cnc18xx_i2c_write_reg(dev,0x1,IC_ENABLE);
    			cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);     //write stop bit
    			previous_chip = 0;
    			pr_err("BUS8 Error:Timeout!\n");
    			return 1;
    		}
    		time_cnt++;
		if (unlikely((i=cnc18xx_i2c_read_reg(dev,IC_RAW_INTR_STAT)) & 0xe0)) {
			
			pr_devel("Chipaddress=0x%x, IC_RAW_INTR_STAT=0x%x\n",
			     dev->slave_addr, i);
				
			cnc18xx_i2c_write_reg(dev,0x03,IC_FIFO_CLR);				//clear tx/rx fifo			
			if(i&0x40)									//Arbitration Lost
				{
					cnc18xx_i2c_write_reg(dev,0x40,IC_INTR_CLR);
					pr_err("Error:Arbitration lost!\n");
				}
			if(i&0x80)
				{
					cnc18xx_i2c_write_reg(dev,0x80,IC_INTR_CLR);
					pr_err("BUS9 Error:Timeout!\n");
				}
			if(i&0x20)									//No ACK
				{
					cnc18xx_i2c_write_reg(dev,0x20,IC_INTR_CLR);
					cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);		//Stop
					pr_devel("Error:No Slave sends ack!\n");
				}
			previous_chip = 0;
			return -1;
		}
		
	}
	return 0;
}


/*
 * Low level master read/write transaction. This function is called
 * from i2c_cnc18xx_xfer.
 */
static int
i2c_cnc18xx_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg, int stop)
{
	struct cnc18xx_i2c_dev *dev = i2c_get_adapdata(adap);
	struct cnc18xx_i2c_platform_data *pdata = dev->dev->platform_data;
	unsigned int ret;
	
	if (msg->len == 0)
		return -EINVAL;

	if (!pdata)
		{
		pr_err("GOD  !!!!!!!!! pdata is NULL!!!!!!!!!!!!!\n");
		pdata = &cnc18xx_i2c_platform_data_default;
		}

	dev->buf_len = msg->len;
	dev->buf = msg->buf;
	dev->bus_frq = pdata->bus_freq;
	dev->flags = stop;
	dev->slave_addr = msg->addr;

	if (msg->flags & I2C_M_RD){
		ret = cnc_i2c_read(dev);
	}else{
		ret = cnc_i2c_write(dev);
		if (unlikely(ret)) {
			msleep(2);
			ret = -EIO;
		}else{
			ret = dev->buf_len;
		}
	}
	return ret;
}

/*
 * Prepare controller for a transaction and call i2c_cnc18xx_xfer_msg
 */
static int
i2c_cnc18xx_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct cnc18xx_i2c_dev *dev = i2c_get_adapdata(adap);
	struct cnc18xx_i2c_platform_data *pdata = dev->dev->platform_data;
	int i;
	int ret = 0;
	unsigned int time_cnt=0;
	unsigned int speed_cnt=0;

	dev_dbg(dev->dev, "%s: msgs: %d\n", __func__, num);
	
	dev->bus_frq = pdata->bus_freq;

	switch ((dev->bus_frq)) {
	case I2C_SPEED_100K:
		speed_cnt = ss_ic_scl_cnt;
		break;
	case I2C_SPEED_400K:
		speed_cnt = fs_ic_scl_cnt;
		break;
	default:
		speed_cnt = ss_ic_scl_cnt;
		break;
	}

	up(&i2c_rw_mutex); 	
	
	while(cnc18xx_i2c_read_reg(dev,IC_STAT)&0x20)
	{
	    	if(unlikely(time_cnt>=1000000))          //Bus Err
	    	{
	    		cnc18xx_i2c_write_reg(dev,0x1,IC_RESET);
	    		cnc18xx_i2c_write_reg(dev,0x0,IC_RESET);
	    		cnc18xx_i2c_write_reg(dev,0x1,IC_ENABLE);
	    		cnc18xx_i2c_write_reg(dev,0x700,IC_DATA_CMD);     //write stop bit
	    		previous_chip = 0;
	    		pr_err("I2C Error:Timeout!\n");
	    		down(&i2c_rw_mutex);
			return -EIO;
	    	}
	    	time_cnt++;
	 }
	

	if(speed_cnt!=previous_speed_cnt)
	{
			cnc18xx_i2c_write_reg(dev,0x0, IC_ENABLE);
			while(!(cnc18xx_i2c_read_reg(dev,IC_STAT)& 0x1));							//wait disabled
			if((speed_cnt==ss_ic_scl_cnt))									//Standard Speed
				{
					cnc18xx_i2c_write_reg(dev,ss_ic_sclh_cnt,IC_SCLH_CNT);
					cnc18xx_i2c_write_reg(dev,ss_ic_scl_cnt-ss_ic_sclh_cnt,IC_SCLL_CNT);
					previous_speed_cnt = ss_ic_scl_cnt;
				}
			else if(speed_cnt==fs_ic_scl_cnt)								//Fast Speed
				{
					
					cnc18xx_i2c_write_reg(dev,fs_ic_sclh_cnt,IC_SCLH_CNT);
					cnc18xx_i2c_write_reg(dev,fs_ic_scl_cnt-fs_ic_sclh_cnt,IC_SCLL_CNT);
					previous_speed_cnt =fs_ic_scl_cnt;
					
				}
	}
	//printk("HRWRITE1 slave_addr is %x,read len is %x ,read date is %x,data_cnt is %x \n",dev->slave_addr,dev->buf_len,*p,data_cnt);
					
	cnc18xx_i2c_write_reg(dev,0x1, IC_ENABLE);
	for (i = 0; i < num; i++) 
	{
		if(msgs[i].flags & I2C_M_RD){
			cnc18xx_i2c_write_reg(dev,(((msgs[i].addr)<<1)&0xff)|0x101,IC_DATA_CMD);
		}else{
			cnc18xx_i2c_write_reg(dev,(((msgs[i].addr)<<1)&0xff)|0x100,IC_DATA_CMD);
		}
		ret = i2c_cnc18xx_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if (ret < 0){
			pr_err("ERROR i2c_cnc18xx_xfer_msg EEROR \n");
			break;
		}
	}
	down(&i2c_rw_mutex);
	
	return (ret < 0)?-EIO:num;
}

static u32 i2c_cnc18xx_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm i2c_cnc18xx_algo = {
	.master_xfer	= i2c_cnc18xx_xfer,
	.functionality	= i2c_cnc18xx_func,
};

static int cnc18xx_i2c_probe(struct platform_device *pdev)
{
	struct cnc18xx_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *irq;
	int r;

printk("HERE cnc18xx_i2c_probe \n");
	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}
#ifndef CONFIG_I2C
	ioarea = request_mem_region(mem->start, resource_size(mem),
				    pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}
#endif
	dev = kzalloc(sizeof(struct cnc18xx_i2c_dev), GFP_KERNEL);
	if (!dev) {
		r = -ENOMEM;
		goto err_release_region;
	}

	init_completion(&dev->cmd_complete);
	dev->dev = get_device(&pdev->dev);
	dev->irq = irq->start;
	platform_set_drvdata(pdev, dev);

	if (IS_ERR(dev->clk)) {
		r = -ENODEV;
		goto err_free_mem;
	}

	dev->base = (void __iomem *)IO_ADDRESS(mem->start);
	i2c_cnc18xx_init(dev);

#if 0
	r = request_irq(dev->irq, i2c_cnc18xx_isr, 0, pdev->name, dev);
	if (r) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}
#endif

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "CNC18xx I2C adapter", sizeof(adap->name));
	
	adap->algo = &i2c_cnc18xx_algo;
	adap->dev.parent = &pdev->dev;
	adap->timeout = CNC18XX_I2C_TIMEOUT;

	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(&pdev->dev, "failure adding adapter\n");
		goto err_free_irq;
	}

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	put_device(&pdev->dev);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, resource_size(mem));

	return r;
}

static int cnc18xx_i2c_remove(struct platform_device *pdev)
{
	struct cnc18xx_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&dev->adapter);
	put_device(&pdev->dev);
	
	//free_irq(IRQ_I2C, dev);
	kfree(dev);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, resource_size(mem));
	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:i2c_cnc18xx");

static struct platform_driver cnc18xx_i2c_driver = {
	.probe		= cnc18xx_i2c_probe,
	.remove		= cnc18xx_i2c_remove,
	.driver		= {
		.name	= "i2c_cnc18xx",
		.owner	= THIS_MODULE,
	},
};

/* I2C may be needed to bring up other drivers */
static int __init cnc18xx_i2c_init_driver(void)
{
	return platform_driver_register(&cnc18xx_i2c_driver);
}
subsys_initcall(cnc18xx_i2c_init_driver);

static void __exit cnc18xx_i2c_exit_driver(void)
{
	platform_driver_unregister(&cnc18xx_i2c_driver);
}
module_exit(cnc18xx_i2c_exit_driver);

MODULE_AUTHOR("Texas Instruments India");
MODULE_DESCRIPTION("TI DaVinci I2C bus adapter");
MODULE_LICENSE("GPL");

