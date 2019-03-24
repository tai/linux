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
#include <asm/mach/irq.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>

#define TIGA_MAGIC   't'



#define POLL_TIMEOUT (2*HZ) 


static spinlock_t gpio_tiga_lock;
static int v_sync_is_coming = 0;
static void tiga_detect_v_sync(unsigned long data);
struct timer_list tiga_detect_v_sync_timer; // = TIMER_INITIALIZER(tiga_detect_v_sync, jiffies + POLL_TIMEOUT, 0);
static int detect_v_sync_status = 0; // 0: mask. 1: unmask.



#define GPIO_READ 0
#define GPIO_WRITE 1

#define VGA_INT_GPIO_NUM 12
#define CNL_GPIO_NUM 14
#define VGA_GPIO_IRQ  gpio_to_irq(VGA_INT_GPIO_NUM)

extern int gpio_hw_write(unsigned char gpio_id, unsigned char data);
extern int gpio_hw_set_direct(int gpio_id, int dir);


static struct file_operations vga_switching_fops = {
	.owner		= THIS_MODULE,
};

static struct miscdevice vga_switcher_miscdev = {
	.name = "vga_switcher",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &vga_switching_fops
};

static void tiga_detect_v_sync(unsigned long data)
{
	static int count = 0;
	unsigned long flags;

    //	printk("%s:%s in line %d\n",__FILE__,__FUNCTION__,__LINE__);

	spin_lock_irqsave(&gpio_tiga_lock, flags);

	if(detect_v_sync_status == 0){ // mask
        enable_irq(VGA_GPIO_IRQ);
        v_sync_is_coming = 0;
        detect_v_sync_status= 1;
        count = 0;
   
	}
	else if(detect_v_sync_status == 1){ // unmask

		if(v_sync_is_coming == 0){
			gpio_hw_write(CNL_GPIO_NUM, 1);
		}
	}

	spin_unlock_irqrestore(&gpio_tiga_lock, flags);
    tiga_detect_v_sync_timer.expires = jiffies + POLL_TIMEOUT;
    add_timer(&tiga_detect_v_sync_timer);

}


irqreturn_t csm_gpio_irq_for_vga_sync(int irq, void *dev_id)
{	
	unsigned long flags;

    //	printk("%s:%s in line %d\n",__FILE__,__FUNCTION__,__LINE__);

	spin_lock_irqsave(&gpio_tiga_lock, flags);
	v_sync_is_coming ++;

    if(v_sync_is_coming >= 3){
        disable_irq_nosync(VGA_GPIO_IRQ);
        gpio_hw_write(CNL_GPIO_NUM, 0);
        v_sync_is_coming= 0;
        detect_v_sync_status = 0;
    }
	spin_unlock_irqrestore(&gpio_tiga_lock, flags);
	return IRQ_HANDLED;
}

static int __init 
vga_switcher_init(void)
{
    int ret = 0;
    int data;
    
    
    if (misc_register(&vga_switcher_miscdev)) {
        ret = -EBUSY;
        printk("VGA swithcer device register failed\n");
        free_irq(VGA_GPIO_IRQ, &data);
        return ret;
    }
	spin_lock_init(&gpio_tiga_lock);


    printk("Enable V Sync Timer!\n");
#if 1
	detect_v_sync_status = 1;
    init_timer(&tiga_detect_v_sync_timer);
    tiga_detect_v_sync_timer.function = tiga_detect_v_sync;
    tiga_detect_v_sync_timer.expires = jiffies + POLL_TIMEOUT;
    tiga_detect_v_sync_timer.data = 0;
	add_timer(&tiga_detect_v_sync_timer);
#endif
    gpio_hw_set_direct(CNL_GPIO_NUM, GPIO_WRITE);

    printk("Register VGA Interrupt!\n");
    set_irq_type(VGA_GPIO_IRQ, IRQ_TYPE_LEVEL_HIGH);
    if (ret=request_irq(VGA_GPIO_IRQ, csm_gpio_irq_for_vga_sync, IRQF_DISABLED | IRQF_TRIGGER_HIGH, "vga_switcher_interrupt", &data)) {
        printk("Init GPIO VGA Switcher IRQ request is failed= %d\n", ret);
        return ret;
    }

    return 0;

}



static void __exit
vga_switcher_exit(void)
{
   misc_deregister(&vga_switcher_miscdev);
   free_irq(VGA_GPIO_IRQ, NULL);

}

module_init(vga_switcher_init);
module_exit(vga_switcher_exit);
MODULE_AUTHOR("Cavium Beijing");
MODULE_DESCRIPTION("VGA Switcher");
MODULE_LICENSE("GPL");
