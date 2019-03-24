#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define CNC18XX_WDT_BASE 		0x801e1000
#define CNC18XX_WDT_SIZE 		0x100
#define CNC18XX_WDT_IRQ		0
#define CNC18XX_WDT_CLOSE_MAGIC	(0xbbffbbff)

#define WDT_CR			(0x00>>2)	/* Control register  */
#define WDT_TORR		(0x04>>2)	/* Timeout range register */
#define WDT_CCVR		(0x08>>2)	/* Current counter value register */
#define WDT_CRR			(0x0c>>2)	/* Counter restart register */
#define WDT_STAT		(0x10>>2)	/* Interrupt status register */
#define WDT_EOI			(0x14>>2)	/* Interrupt clear register */

#define WDT_1SEC		0xa
#define WDT_2SEC		0xb
#define WDT_5SEC		0xc
#define WDT_10SEC		0xd
#define WDT_20SEC		0xe
#define WDT_40SEC		0xf

static volatile u32 *wdt_base = NULL;
static struct proc_dir_entry *wdt_proc_entry = NULL;
static int margin = WDT_40SEC;		/* (secs) Default is the 40 sec */

static int expect_close;
#ifdef CONFIG_WATCHDOG_NOWAYOUT
static int nowayout = 1;
#else
static int nowayout = 0;
#endif

static int cnc18xx_wdt_open(struct inode *inode, struct file *file)
{
	nonseekable_open(inode, file);
	wdt_base[WDT_TORR] = margin;
	wdt_base[WDT_CRR] = 0x76;
	wdt_base[WDT_CR] = 0x1f;

	return 0;
}

static int cnc18xx_wdt_release(struct inode *inode, struct file *file)
{
	wdt_base[WDT_CRR] = 0x76;

	if (expect_close == CNC18XX_WDT_CLOSE_MAGIC) {
		wdt_base[WDT_CR] = 0x0;
	} else {
		printk(KERN_CRIT "WATCHDOG: WDT device closed unexpectedly.  WDT will not stop!\n");
	}

	expect_close = 0;

	return 0;
}

static ssize_t cnc18xx_wdt_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	if (len) {
		if (!nowayout) {
			size_t i;

			expect_close = 0;

			for (i = 0; i != len; i++) {
				char c;

				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					expect_close = CNC18XX_WDT_CLOSE_MAGIC;
			}
		}
		/* Refresh timer. */
		wdt_base[WDT_CRR] = 0x76;
	}

	return len;
}

static struct watchdog_info ident = {
	.options	= WDIOF_CARDRESET | WDIOF_MAGICCLOSE |
		WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity	= "Orion Watchdog",
};

static int cnc18xx_wdt_ioctl(struct inode *inode, struct file *file,
							 unsigned int cmd, unsigned long arg)
{
	int ret = -ENOIOCTLCMD;
	int time;

	switch (cmd) {
		case WDIOC_GETSUPPORT:
			ret = copy_to_user((struct watchdog_info *)arg, &ident,
							   sizeof(ident)) ? -EFAULT : 0;
			break;

		case WDIOC_GETSTATUS:
			ret = put_user(wdt_base[WDT_STAT] & 0x1, (int *)arg);
			break;

		case WDIOC_GETBOOTSTATUS:
			ret = put_user(0, (int *)arg);
			break;

		case WDIOC_SETTIMEOUT:
			ret = get_user(time, (int *)arg);
			if (ret)
				break;

#if 0
			if (time <= 0 || time > 16) {
				ret = -EINVAL;
				break;
			}
#endif
			if (time > 0 && time < 2) 
				margin = WDT_1SEC;
			else if (time >= 2 && time < 5) 
				margin = WDT_2SEC;
			else if (time >= 5 && time < 10) 
				margin = WDT_5SEC;
			else if (time >= 10 && time < 20) 
				margin = WDT_10SEC;
			else if (time >= 20 && time < 40) 
				margin = WDT_20SEC;
			else
				margin = WDT_40SEC;

			wdt_base[WDT_TORR] = margin;
			wdt_base[WDT_CRR] = 0x76;
			/*fall through*/

		case WDIOC_GETTIMEOUT:
			time = wdt_base[WDT_CCVR] * 1000 / CLOCK_TICK_RATE;
			ret = put_user(time, (int *)arg);
			break;

		case WDIOC_KEEPALIVE:
			wdt_base[WDT_CRR] = 0x76;
			ret = 0;
			break;
	}

	return ret;
}

static int wdt_proc_write(struct file *file, 
						  const char *buffer, unsigned long count, void *data)
{
	u32 addr;
	u16 val;

	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = wdt_base[addr>>2];
		printk(" wdt_base[0x%04x] = 0x%04x \n", addr, val);
	}
	else if (strncmp("wl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		val = simple_strtol(&cmd_line[6], NULL, 16);
		wdt_base[addr>>2] = val;
	}
	else if (strncmp("en", cmd_line, 2) == 0) {
		wdt_base[WDT_TORR] = margin;
		wdt_base[WDT_CRR] = 0x76;
		wdt_base[WDT_CR] = 0x1f;
	}
	else if (strncmp("dis", cmd_line, 3) == 0) {
		wdt_base[WDT_CR] = 0x0;
	}
	else if (strncmp("st", cmd_line, 2) == 0) {
		printk("WDT: Get state 0x%x\n", wdt_base[WDT_STAT]);
	}
	else if (strncmp("rm", cmd_line, 2) == 0) {
		printk("WDT: Get remain time  0x%x\n", wdt_base[WDT_CCVR]);
	}
	else if (strncmp("fe", cmd_line, 2) == 0) {
		val = simple_strtol(&cmd_line[3], NULL, 16);
		wdt_base[WDT_TORR] = val;
		wdt_base[WDT_CRR] = 0x76;
	}
	else if (strncmp("clear", cmd_line, 5) == 0) {
		wdt_base[WDT_EOI];
	}

	return count;
}

static struct file_operations cnc18xx_wdt_fops =
{
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= cnc18xx_wdt_write,
	.ioctl		= cnc18xx_wdt_ioctl,
	.open		= cnc18xx_wdt_open,
	.release	= cnc18xx_wdt_release,
};

static struct miscdevice cnc18xx_wdt_miscdev =
{
	.minor		= WATCHDOG_MINOR,
	.name		= "watchdog",
	.fops		= &cnc18xx_wdt_fops,
};

static int __init cnc18xx_wdt_init(void)
{
	int ret;

	ret = misc_register(&cnc18xx_wdt_miscdev);
	if (ret) {
		return -ENODEV;
	} else {
		printk("CNC18XX Watchdog Timer: timer margin %d sec\n",
			   (margin == WDT_1SEC ) ? 1 :
			   (margin == WDT_2SEC ) ? 2 :
			   (margin == WDT_5SEC ) ? 5 : 
			   (margin == WDT_10SEC) ? 10:
			   (margin == WDT_20SEC) ? 20:
			   (margin == WDT_40SEC) ? 40:0);
	}

	wdt_base = (u32 *)ioremap(CNC18XX_WDT_BASE, CNC18XX_WDT_SIZE);
	if (!wdt_base) {
		misc_deregister(&cnc18xx_wdt_miscdev);
		return -EBUSY;
	}

	wdt_proc_entry = create_proc_entry("wdt_io", 0, NULL);
	if (NULL != wdt_proc_entry) {
		wdt_proc_entry->write_proc = &wdt_proc_write;
	} else {
		iounmap((void*)wdt_base);
		misc_deregister(&cnc18xx_wdt_miscdev);
	}

	return ret;
}

static void __exit cnc18xx_wdt_exit(void)
{
	if (NULL != wdt_proc_entry) remove_proc_entry("wdt_io", NULL);
	iounmap((void*)wdt_base);
	misc_deregister(&cnc18xx_wdt_miscdev);
}

module_init(cnc18xx_wdt_init);
module_exit(cnc18xx_wdt_exit);

MODULE_AUTHOR("SunHe <he.sun@celestialsemi.cn>");
MODULE_DESCRIPTION("DesignWare APB Watchdog Timer");

module_param(margin, int, 0);
MODULE_PARM_DESC(margin, "Watchdog margin in seconds (default 40s)");

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
