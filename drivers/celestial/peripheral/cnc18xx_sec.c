#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#define CNC18XX_SEC_BASE 0xfffff000
#define CNC18XX_SEC_SIZE 0x50

#define SEC_RD_OTP      13
#define SEC_RD_UNIID    14

#define WriteReg32(r, v) writel((v), cnc18xx_sec_base+(r))
#define ReadReg32(r) readl(cnc18xx_sec_base+(r))

static void __iomem *cnc18xx_sec_base;
DEFINE_SPINLOCK(sec_lock);


#if 0
#define MAILBOX0 (0x00)
#define MAILBOX1 (0x04)
#define MAILBOX2 (0x08)
#define MAILBOX3 (0x0C)
#define MAILBOX4 (0x10)
#define MAILBOX5 (0x14)
#define MAILBOX6 (0x18)
#define MAILBOX7 (0x1C)

typedef struct SEC_UNI_KEY {
    unsigned int ukey0;
    unsigned int ukey1;
    unsigned int ukey2;
    unsigned int ukey3;
    unsigned int pkey0;
    unsigned int pkey1;
    unsigned int pkey2;
    unsigned int pkey3;
} CSSEC_UNI_KEY;
#endif

static int
cnc18xx_sec_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned long flags;
    unsigned int ret = 0;

    switch (cmd) {
#if 0
    case SEC_RD_OTP:
        {
            CSSEC_UNI_KEY *key = (CSSEC_UNI_KEY *) arg;

            spin_lock_irqsave(&sec_lock, flags);
            WriteReg32(MAILBOX1, 0x0);
            WriteReg32(MAILBOX0, 0x80000010);
            while(!(ReadReg32(MAILBOX7) & 0x80000000));
            key->ukey0 = ReadReg32(MAILBOX2);  /* OTP0, Word 64 */
            key->ukey1 = ReadReg32(MAILBOX3);  /* OTP0, Word 65 */
            key->ukey2 = ReadReg32(MAILBOX4);  /* OTP0, Word 66 */
            key->ukey3 = ReadReg32(MAILBOX5);  /* OTP0, Word 67 */
            WriteReg32(MAILBOX7, 0x0);
            WriteReg32(MAILBOX1, 0x0);
            WriteReg32(MAILBOX0, 0x80000011);
            while(!(ReadReg32(MAILBOX7) & 0x80000000));
            key->pkey0 = ReadReg32(MAILBOX2);  /* OTP0, Word 68 */
            key->pkey1 = ReadReg32(MAILBOX3);  /* OTP0, Word 69 */
            key->pkey2 = ReadReg32(MAILBOX4);  /* OTP0, Word 70 */
            key->pkey3 = ReadReg32(MAILBOX5);  /* OTP0, Word 71 */
            WriteReg32(MAILBOX7, 0x0);
            spin_unlock_irqrestore(&sec_lock, flags);
        }
        break;
#endif

    case SEC_RD_UNIID:
        spin_lock_irqsave(&sec_lock, flags);
        *(unsigned int *)arg = ReadReg32(0x14);  /* read unique id from 0xfffff014 */
        spin_unlock_irqrestore(&sec_lock, flags);
        break;

    default:
        return -EINVAL;
    }

    return ret;
}

static int
cnc18xx_sec_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int
cnc18xx_sec_release(struct inode *inode, struct file *file)
{
    return 0;
}


static struct file_operations cnc18xx_sec_fops = {
    .owner  = THIS_MODULE,
    .ioctl  = cnc18xx_sec_ioctl,
    .open   = cnc18xx_sec_open,
    .release= cnc18xx_sec_release,
};

static struct miscdevice cnc18xx_sec_miscdev = {
    MISC_DYNAMIC_MINOR,
    "cnc18xx_sec",
    &cnc18xx_sec_fops
};

int __init cnc18xx_sec_init(void)
{
    int ret = 0;

    /* System init */
    if (misc_register(&cnc18xx_sec_miscdev))
        return -ENODEV;

    cnc18xx_sec_base = ioremap(CNC18XX_SEC_BASE, CNC18XX_SEC_SIZE);
    if (!cnc18xx_sec_base)
        return -EIO;

    return ret;
}

static void __exit cnc18xx_sec_exit(void)
{
    iounmap((void *)cnc18xx_sec_base);
    misc_deregister(&cnc18xx_sec_miscdev);
}

module_init(cnc18xx_sec_init);
module_exit(cnc18xx_sec_exit);

/*----------------------------------------------------------------------------*/


MODULE_AUTHOR("Sun He, <he.sun@celestialsemi.com>");
MODULE_DESCRIPTION("Celestial Semiconductor Secure driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");

