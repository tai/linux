/*======================================================================

  Device driver for the PCMCIA control functionality of  CNC1800
  microprocessors.

    The contents of this file may be used under the
    terms of the GNU Public License version 2 (the "GPL")

    derived from sa11xx_base.c

     Portions created by John G. Dorsey are
     Copyright (C) 1999 John G. Dorsey.

  ======================================================================*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/ss.h>
#include <pcmcia/cistpl.h>

#include "soc_common.h"

#include "cnc18xx_socket.h"

#ifdef RESET_BY_GPIO12
#define SCS_BASEADDR      0x10171000
#define PIN_CTL_LO_ADDR   0x10171400
static void *sel_gpio12_pcmcia = NULL;
#endif

/*#define debugk(s) printk(s)*/
#define debugk(s)

extern int gpio_hw_set_direct(int gpio_id, int dir);
extern int gpio_hw_write(int gpio_id, unsigned short data);

static struct pcmcia_irqs irqs[] = {
    /*--------------------------------------------------
    * { 0, 31, "PCMCIA0 CD" },
    * { 0, 30, "PCMCIA0 TIMOUT" },
    * { 0, 29, "PCMCIA0 DATAEXCHG" },
    *--------------------------------------------------*/
};

static unsigned int timing_mrd = 0xE1;
static unsigned int timing_mwr = 0x04A1;
static unsigned int timing_io  = 0x7FFF;

static void cnc18xx_pcmcia_hw_set_cis_rmode(void)
{
    iowrite32(0x00, OPMODE);
    iowrite32(timing_mrd, TIMECFG);
}
static void cnc18xx_pcmcia_hw_set_cis_wmode(void)
{
    iowrite32(0x00, OPMODE);
    iowrite32(timing_mwr, TIMECFG);
}
static void cnc18xx_pcmcia_hw_set_comm_rmode(void)
{
    iowrite32(0x02, OPMODE);
    iowrite32(timing_mrd, TIMECFG);
}
static void cnc18xx_pcmcia_hw_set_comm_wmode(void)
{
    iowrite32(0x02, OPMODE);
    iowrite32(timing_mwr, TIMECFG);
}
static void cnc18xx_pcmcia_hw_set_io_mode(void)
{
    iowrite32(0x04, OPMODE);
    iowrite32(timing_io, TIMECFG);
}

static unsigned char cnc18xx_pcmcia_hw_read_cis(unsigned int addr)
{
    unsigned char val = 0;
    iowrite32(((addr >> 8) & 0x3F), ATTRBASE);              // cnc18xx pcmcia have only 15 addr lines
    val = ioread8((void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
    return val;
}

static unsigned char cnc18xx_pcmcia_hw_read_comm(unsigned int addr)
{
    unsigned char val = 0;
    iowrite32(((addr >> 8) & 0x3F), COMMBASE);              // cnc18xx pcmcia have only 15 addr lines
    val = ioread8((void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
    return val;
}

static unsigned char cnc18xx_pcmcia_hw_read_io(unsigned int addr)
{
    unsigned char val = 0;
    iowrite32(((addr >> 8) & 0x3F), IOBASE);                // cnc18xx pcmcia have only 15 addr lines
    val = ioread8((void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
    return val;
}

static void cnc18xx_pcmcia_hw_write_cis(unsigned char val, unsigned int addr)
{
    iowrite32(((addr >> 8) & 0x3F), ATTRBASE);              // cnc18xx pcmcia have only 15 addr lines
    iowrite8(val, (void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
}

static void cnc18xx_pcmcia_hw_write_comm(unsigned char val, unsigned int addr)
{
    iowrite32(((addr >> 8) & 0x3F), COMMBASE);              // cnc18xx pcmcia have only 15 addr lines
    iowrite8(val, (void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
}

static void cnc18xx_pcmcia_hw_write_io(unsigned char val, unsigned int addr)
{
    iowrite32(((addr >> 8) & 0x3F), IOBASE);                // cnc18xx pcmcia have only 15 addr lines
    iowrite8(val, (void*)(VA_PCMCIA_BASE + (addr & 0xFF))); // device access window addr 0x00~0xFF
}

static int cnc18xx_pcmcia_hw_init(struct soc_pcmcia_socket *skt)
{
    /*
     * Setup default state of GPIO12 to PCMCIA_RESET_n
     */
    unsigned int reg;

    debugk("init in\n");

#if defined(RESET_BY_GPIO12)
    if(!request_mem_region(PIN_CTL_LO_ADDR, 0x100, "SEL PCMCIA RESET")) {
        printk(KERN_WARNING "Warning:PIN_CTL_LO request error.\n");
        return -EIO;
    }
    sel_gpio12_pcmcia = ioremap(PIN_CTL_LO_ADDR, 0x100);

    if(!sel_gpio12_pcmcia) {
        printk(KERN_WARNING "Warning:PIN_CTL_LO ioremap error.\n");
        release_mem_region(PIN_CTL_LO_ADDR, 0x100);
        return -EIO;
    }

    reg = ioread32(sel_gpio12_pcmcia);  /* enable IR mode for third UART */
    reg |= 0x40;
    iowrite32(reg, sel_gpio12_pcmcia);
#endif

    iowrite16(0x1000, (void*)WAITTMR);  //FIXME default time out error while reading & writing
    if (skt->irq == NO_IRQ) {
        //skt->irq = skt->nr ? IRQ_S1_READY_NINT : IRQ_S0_READY_NINT;
    }
    debugk("init out\n");
    return soc_pcmcia_request_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

static void cnc18xx_pcmcia_hw_shutdown(struct soc_pcmcia_socket *skt)
{
    debugk("shutdown in\n");

#if defined(RESET_BY_GPIO12)
    if(sel_gpio12_pcmcia)
        iounmap(sel_gpio12_pcmcia);
    sel_gpio12_pcmcia = NULL;
    release_mem_region(PIN_CTL_LO_ADDR, 0x100);
#endif

    debugk("shutdown out\n");
    soc_pcmcia_free_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

static void cnc18xx_pcmcia_socket_state(struct soc_pcmcia_socket *skt, struct pcmcia_state *state)
{
    unsigned char inserted;
    unsigned char vs;
    unsigned short rawstat, pinctl;

    state->ready = 0;
    state->detect = 0;
    state->vs_3v = 0;
    rawstat = ioread16((void*)RAWSTAT);
    pinctl = ioread16((void*)PINCTL);

    debugk("hw state in\n");

    switch (skt->nr) {
    case 0:
        //FIXME
        vs = 0x02;
        if((rawstat & INT_CDCHG) && ((PIN_CD1 | PIN_CD2) & pinctl))
            inserted = 1;
        else
            inserted = 0;
        break;
    default:    /* should never happen */
        return;
    }

    if (inserted) {
        switch (vs) {
        case 0:
        case 2: /* 3.3V */
            state->vs_3v = 1;
            //printk("pcmcia card detected!\n");
            break;
        case 3: /* 5V */
            break;
        default:
            /* return without setting 'detect' */
            printk(KERN_ERR "pcmcia bad VS (%d)\n", vs);
        }
        state->detect = 1;
        //if(pinctl & PIN_RDYIREQ){
        //printk("pcmcia card is ready!\n");
        state->ready = 1;
        //}
    } else {
        /* if the card was previously inserted and then ejected,
         * we should enable the card detect interrupt
         */
       /*--------------------------------------------------
        * enable_irq(31);
        *--------------------------------------------------*/
        /*printk(KERN_ERR "pcmcia not avaliable!\n");*/
    }

    state->bvd1 = 1;
    state->bvd2 = 1;
    state->wrprot = 0;
    state->vs_Xv = 0;
    /*--------------------------------------------------
    *     //clear CD interrupt ?  failed!!
    *     unsigned short tmp;
    *     tmp = ioread16((void*)RAWSTAT);
    *     iowrite16(tmp, (void*)RAWSTAT);
    *--------------------------------------------------*/
    debugk("hw state out\n");
}

static int cnc18xx_pcmcia_configure_socket(struct soc_pcmcia_socket *skt, const socket_state_t *state)
{
    unsigned short pinctl;

    debug("config_skt %d Vcc %dV Vpp %dV, reset %d\n", 
          skt->nr, state->Vcc, state->Vpp, state->flags & SS_RESET);

    if ((skt->nr == 0) && (state->flags & SS_RESET)) {
#if defined(RESET_BY_GPIO12)
        pinctl = ioread16((void*)PINCTL);
        pinctl |= PIN_RST;
        iowrite16(pinctl, (void*)PINCTL);
        msleep(1);
        pinctl &= ~PIN_RST;
        iowrite16(pinctl, (void*)PINCTL);
        msleep(1);
#elif defined(RESET_BY_GPIO15)
        gpio_hw_set_direct(15, 1); //set gpio15 to write mode
        gpio_hw_write(15, 1);
        msleep(1);
        gpio_hw_write(15, 0);
        msleep(1);
#endif
    }
    return 0;
}

/*
 * Enable card status IRQs on (re-)initialisation.  This can
 * be called at initialisation, power management event, or
 * pcmcia event.
 */
static void cnc18xx_pcmcia_socket_init(struct soc_pcmcia_socket *skt)
{
}

/*
 * Disable card status IRQs and PCMCIA bus on suspend.
 */
static void cnc18xx_pcmcia_socket_suspend(struct soc_pcmcia_socket *skt)
{
}

/*
 * Hardware specific timing routines.
 * If provided, the get_timing routine overrides the SOC default.
 */
static unsigned int cnc18xx_pcmcia_socket_get_timing(struct soc_pcmcia_socket *skt, unsigned int a, unsigned int b)
{
    return 0;
}

static int cnc18xx_pcmcia_socket_set_timing(struct soc_pcmcia_socket *skt)
{
    //iowrite16(0x00E1, (void*)TIMECFG);
    return 0;
}

static int cnc18xx_pcmcia_socket_show_timing(struct soc_pcmcia_socket *skt, char *buf)
{
    return 0;
}

static struct pcmcia_low_level cnc18xx_pcmcia_ops = {
    .owner            = THIS_MODULE,

    .nr               = 1,
    .first            = 0,
    .hw_init          = cnc18xx_pcmcia_hw_init,
    .hw_shutdown      = cnc18xx_pcmcia_hw_shutdown,

    .socket_state     = cnc18xx_pcmcia_socket_state,
    .configure_socket = cnc18xx_pcmcia_configure_socket,

    .socket_init      = cnc18xx_pcmcia_socket_init,
    .socket_suspend   = cnc18xx_pcmcia_socket_suspend,

    .get_timing       = NULL,
    .set_timing       = cnc18xx_pcmcia_socket_set_timing,
    .show_timing      = cnc18xx_pcmcia_socket_show_timing,

    .set_cis_rmode    = cnc18xx_pcmcia_hw_set_cis_rmode,
    .set_cis_wmode    = cnc18xx_pcmcia_hw_set_cis_wmode,
    .set_comm_rmode   = cnc18xx_pcmcia_hw_set_comm_rmode,
    .set_comm_wmode   = cnc18xx_pcmcia_hw_set_comm_wmode,
    .set_io_mode      = cnc18xx_pcmcia_hw_set_io_mode,
    .read_cis         = cnc18xx_pcmcia_hw_read_cis,
    .write_cis        = cnc18xx_pcmcia_hw_write_cis,
    .read_comm        = cnc18xx_pcmcia_hw_read_comm,
    .write_comm       = cnc18xx_pcmcia_hw_write_comm,
    .read_io          = cnc18xx_pcmcia_hw_read_io,
    .write_io         = cnc18xx_pcmcia_hw_write_io
};

static struct proc_dir_entry *pcmcia_proc_entry = NULL;

static unsigned int __stoi(const char* s)
{
    char *p = s;
    char c;
    unsigned int i = 0;
    while (c = *p++) {
        if(c >= '0' && c <= '9') {
            i  =  i * 16 + (c - '0');
        } else if (c >= 'a'  &&  c <= 'f') {
            i  =  i * 16 + (c - 'a' + 10);
        } else if (c >= 'A'  &&  c <= 'F') {
            i  =  i * 16 + (c - 'A' + 10);
        } else if (c == 0x0a) {
            break;
        } else {
            printk("illegal char: %c\n", c);
            return  0xffffffff;
        }
    }
    return  i;
}

static int cnc18xx_pcmcia_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned int addr;
    unsigned int val;
    unsigned int timing_val;
    const char *cmd_line = buffer;

    printk("cnc18xx_pcmcia_proc_write\n");
    if (strncmp("help", cmd_line, 4) == 0) {
        printk("command:\n");
        printk("  rst: reset the pc caard\n");
        printk("  reg: show pcmcia controller registers\n");
        printk("  cis: show cis of attribute space\n");
        printk("  tmr_xxx: set the timing for memory read    mode, xxx should be hexadecimal and none-0x headed\n");
        printk("  tmw_xxx: set the timing for memory write   mode, xxx should be hexadecimal and none-0x headed\n");
        printk("  tio_xxx: set the timing for IO (read/write)mode, xxx should be hexadecimal and none-0x headed\n");
    } else if (strncmp("rst", cmd_line, 3) == 0) {
        NULL;
    } else if (strncmp("tmr_", cmd_line, 4) == 0) {
        timing_val = __stoi(cmd_line + 4);
        if (0xffffffff == timing_val) {
            printk("Illegal command, timing_val = %x\n", timing_val);
        } else {
            timing_mrd = timing_val;
            printk("timing_mrd: %x\n", timing_mrd);
        }
    } else if (strncmp("tmw_", cmd_line, 4) == 0) {
        timing_val = __stoi(cmd_line + 4);
        if (0xffffffff == timing_val) {
            printk("Illegal command\n");
        } else {
            timing_mwr = timing_val;
            printk("timing_mwr: %x\n", timing_mwr);
        }
    } else if (strncmp("tio_", cmd_line, 4) == 0) {
        timing_val = __stoi(cmd_line + 4);
        if (0xffffffff == timing_val) {
            printk("Illegal command\n");
        } else {
            timing_io = timing_val;
            printk("timing_io: %x\n", timing_io);
        }
    } else if (strncmp("cis", cmd_line, 3) == 0) {
        int off = 0;
        char ch;
        cnc18xx_pcmcia_hw_set_cis_rmode();
        printk("reading cis:\n");
        printk("data cis:\n");
        while (1) {
            if (off > 0xff)  {
                break;
            }
            ch = cnc18xx_pcmcia_hw_read_cis(off << 1);
            printk("%02x ", ch);
            if (0xff == ch) {
                // break;
            }
            off++;
        }
        printk("\n");
    } else if (strncmp("reg", cmd_line, 3) == 0) {
        printk("Show pcmcia contrller registers:\n");
        printk("TIMECFG  (off = 0x100)      , val = 0x%08x\n", ioread16((void*)TIMECFG));
        printk("OPMODE   (off = 0x104)      , val = 0x%08x\n", ioread16((void*)OPMODE));
        printk("RAWSTAT  (off = 0x108)      , val = 0x%08x\n", ioread16((void*)RAWSTAT));
        printk("INTSTAT  (off = 0x10c)      , val = 0x%08x\n", ioread16((void*)INTSTAT));
        printk("INTENA   (off = 0x110)      , val = 0x%08x\n", ioread16((void*)INTENA));
        printk("PINCTL   (off = 0x114)      , val = 0x%08x\n", ioread16((void*)PINCTL));
        printk("ATTRBASE (off = 0x118)      , val = 0x%08x\n", ioread16((void*)ATTRBASE));
        printk("COMMBASE (off = 0x11c)      , val = 0x%08x\n", ioread16((void*)COMMBASE));
        printk("IOBASE   (off = 0x120)      , val = 0x%08x\n", ioread16((void*)IOBASE));
        printk("WAITTMR  (off = 0x124)      , val = 0x%08x\n", ioread16((void*)WAITTMR));
        printk("Timing   (for memory read)  , val = 0x%08x\n", timing_mrd);
        printk("Timing   (for memory write) , val = 0x%08x\n", timing_mwr);
        printk("Timing   (for io read/write), val = 0x%08x\n", timing_io);
    } else {
        printk("Illegal command\n");
    }
    return count;
}

#define SKT_DEV_INFO_SIZE(n) \
	(sizeof(struct skt_dev_info) + (n)*sizeof(struct soc_pcmcia_socket))

int __init cnc18xx_drv_pcmcia_probe(struct platform_device *pdev)
{
	struct skt_dev_info *sinfo;
    struct pcmcia_low_level *ops;
	struct soc_pcmcia_socket *skt;
	struct device *dev = &pdev->dev;
    struct resource *res;
    int first, nr;
    int i;

    if(!dev->platform_data)
        dev->platform_data = (void*)&cnc18xx_pcmcia_ops;

    ops = &cnc18xx_pcmcia_ops;
    nr = ops->nr;

#ifdef CONFIG_CPU_FREQ
    ops->frequency_change = cnc18xx_pcmcia_frequency_change;
#endif

    pcmcia_proc_entry = create_proc_entry("pcmcia_io", 0, NULL);
    if (NULL != pcmcia_proc_entry) {
        pcmcia_proc_entry->write_proc = &cnc18xx_pcmcia_proc_write;
        printk("Succesefully create pcmcia proc entry!\n");
    } else {
        printk("Failed to create pcmcia proc entry!\n");
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL)
        return -EINVAL;

    sinfo = kzalloc(SKT_DEV_INFO_SIZE(nr), GFP_KERNEL);
    if (!sinfo)
        return -ENOMEM;

    sinfo->nskt = nr;
    skt = sinfo->skt;
    memset(skt, 0, sizeof(*skt));

    skt->nr             = 0;
    skt->irq            = NO_IRQ;

    skt->res_skt.start  = res->start;
    skt->res_skt.end    = res->end;
    skt->res_skt.name   = "PCMCIA socket 0";
    skt->res_skt.flags  = IORESOURCE_MEM;

    skt->res_io.start   = res->start;
    skt->res_io.end     = res->end;
    skt->res_io.name    = "io";
    skt->res_io.flags   = IORESOURCE_MEM | IORESOURCE_BUSY;

    skt->res_mem.start  = _PCMCIAMem(skt->nr);
    skt->res_mem.end    = _PCMCIAMem(skt->nr) + PCMCIAMemSp - 1;
    skt->res_mem.name   = "memory";
    skt->res_mem.flags  = IORESOURCE_MEM;

    skt->res_attr.start = _PCMCIAAttr(skt->nr);
    skt->res_attr.end   = _PCMCIAAttr(skt->nr) + PCMCIAAttrSp - 1;
    skt->res_attr.name  = "attribute";
    skt->res_attr.flags = IORESOURCE_MEM;

#if 0
	for (i = 0; i < nr; i++) {
		skt = sinfo->skt + i;
		memset(skt, 0, sizeof(*skt));

		skt->nr		= i;
		skt->irq	= NO_IRQ;
		skt->dev	= dev;
		skt->ops	= ops;

        skt->res_skt.start  = res->start;
        skt->res_skt.end    = res->end;
        skt->res_skt.name   = skt_names[i];
        skt->res_skt.flags  = IORESOURCE_MEM;

        skt->res_io.start   = res->start;
        skt->res_io.end     = res->end - 1;
        skt->res_io.name    = "io";
        skt->res_io.flags   = IORESOURCE_MEM | IORESOURCE_BUSY;


        skt->res_mem.start  = _PCMCIAMem(skt->nr);
        skt->res_mem.end    = _PCMCIAMem(skt->nr) + PCMCIAMemSp - 1;
        skt->res_mem.name   = "memory";
        skt->res_mem.flags  = IORESOURCE_MEM;

        skt->res_attr.start = _PCMCIAAttr(skt->nr);
        skt->res_attr.end   = _PCMCIAAttr(skt->nr) + PCMCIAAttrSp - 1;
        skt->res_attr.name  = "attribute";
        skt->res_attr.flags = IORESOURCE_MEM;

	}
#endif

    return soc_common_drv_pcmcia_probe(dev, ops, sinfo);
}

static int cnc18xx_drv_pcmcia_suspend(struct platform_device *dev, pm_message_t state)
{
    return pcmcia_socket_dev_suspend(&dev->dev);
}

static int cnc18xx_drv_pcmcia_resume(struct platform_device *dev)
{
   /*--------------------------------------------------
    * struct pcmcia_low_level *ops = dev->platform_data;
    * int nr = ops ? ops->nr : 0;
    *--------------------------------------------------*/

   /*--------------------------------------------------
    * MECR = nr > 1 ? MECR_CIT | MECR_NOS : (nr > 0 ? MECR_CIT : 0);
    *--------------------------------------------------*/
    return pcmcia_socket_dev_resume(&dev->dev);
}

/**********************************************************************************/
static struct miscdevice cnc18xx_pcmcia_sci_miscdev;

static struct platform_driver cnc18xx_pcmcia_driver = {
    .driver = {
        .name    = "cnc18xx_pcmcia",
        .owner   = THIS_MODULE,
    },
    .probe   = cnc18xx_drv_pcmcia_probe,
    .remove  = soc_common_drv_pcmcia_remove,
    .suspend = cnc18xx_drv_pcmcia_suspend,
    .resume  = cnc18xx_drv_pcmcia_resume,
};

static int __init cnc18xx_pcmcia_init(void)
{
    printk("PCMCIA: %s called!\n", __func__);

#ifdef CONFIG_CNC18XX_PCMCIA_CI
    printk("cnc18xx socket ci module initializing!!!\n");

    iowrite16(0x1000, (void*)WAITTMR);  //FIXME default time out error while reading & writing

    if (misc_register(&cnc18xx_pcmcia_sci_miscdev)) {
        printk(KERN_INFO "Failed to register the PCMCIA CI misc device\n");
		return -EBUSY;
    }
#endif

    return platform_driver_register(&cnc18xx_pcmcia_driver);
}

static void __exit cnc18xx_pcmcia_exit(void)
{
#ifdef CONFIG_CNC18XX_PCMCIA_CI
    misc_deregister(&cnc18xx_pcmcia_sci_miscdev);
#endif
    platform_driver_unregister(&cnc18xx_pcmcia_driver);
}

module_init(cnc18xx_pcmcia_init);
module_exit(cnc18xx_pcmcia_exit);

MODULE_AUTHOR("ChenXiming <xm.chen@celestialsemi.com>, Sun He <he.sun@celestialsemi.cn>");
MODULE_DESCRIPTION("Linux PCMCIA Card Services: PALMCHIP core socket driver");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CNC18XX_PCMCIA_CI

#define DRV_READCOMM    0 // Read common space, not used
#define DRV_WRITECOMM   1 // Write common space, not used
#define DRV_READMEM     2 // Read attr memory
#define DRV_WRITEMEM    3 // Write arrt memory
#define DRV_READIO      4 // Read a I/O Register
#define DRV_WRITEIO     5 // Write a I/O Register
#define DRV_TSIGNAL     6 // Check a Signal
#define DRV_SSIGNAL     7 // Set / Clear a Signal
#define DRV_DBG_TOGGLE  100

/* Signal number for DRV_TSIGNAL command */
#define DRV_CARD_DETECT 1
#define DRV_READY_BUSY  2

/* Signal number for DRV_SSIGNAL command */
#define DRV_EMSTREAM    0
#define DRV_ENSLOT      1
#define DRV_RSTSLOT     2
#define DRV_IOMEM       6
#define DRV_SLOTSEL     7

#define GPIO_READ       0
#define GPIO_WRITE      1

static int SOCKET_CI_DEBUG = 0;
#define DBG(format, args...)\
do {\
if (SOCKET_CI_DEBUG) {\
    printk(format, ## args);}\
}while(0)

/*------ structures for ioctl--------------*/

typedef struct {
    unsigned short addr;    /* I/O Base Address                                    */
} DRV_stAddr;               /* structure for DRV_ADDR                              */

typedef struct {
    unsigned short addr;    /* address to read/write                               */
    unsigned short len;     /* number of bytes to read/write                       */
    unsigned char *pbytes;  /* pointer to bytes to read/write                      */
    unsigned short rlen;    /* number of bytes actually read/write                 */
} DRV_stMem;                /* structure for DRV_READMEM and DRV_WRITEMEM commands */


typedef struct {
    unsigned short registr; /* register addresse to read/write                     */
    unsigned char *pvalue;  /* pointer to the value to read/write                  */
} DRV_stIO;                 /* structure for DRV_READIO and DRV_WRITEIO commands   */


typedef struct {
    unsigned char sig;      /* signal number                                       */
    unsigned char value;    /* value(1 : signal present ; 0 missing)               */
} DRV_stSignal;             /* structure for DRV_TSIGNAL command                   */


typedef struct {
    unsigned char pin;      /* pin code                                            */
    unsigned char cs;       /* value(1 : Set ; 0 clear)                            */
} DRV_ssSignal;             /* structure for DRV_SSIGNAL command                   */

union Uiost {
    DRV_stAddr iobase;      /* structure for DRV_ADDR                              */
    DRV_stMem mem;          /* structure for DRV_READMEM and DRV_WRITEMEM commands */
    DRV_stIO io;            /* structure for DRV_READIO and DRV_WRITEIO commands   */
    DRV_stSignal rsig;      /* structure for DRV_TSIGNAL command                   */
    DRV_ssSignal wsig;      /* structure for DRV_SSIGNAL command                   */
};


static int cnc18xx_pcmcia_ci_open (struct inode *inode, struct file *filp)
{
    return 0;
}

static int cnc18xx_pcmcia_ci_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int cnc18xx_pcmcia_ci_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (char __user *)arg;
	union Uiost iost;
	int i = 0;
	unsigned char *buf;
	int ret = 0;
	switch (cmd)
	{
		case DRV_READMEM:
			{
				DRV_stMem* attr = &(iost.mem);
                cnc18xx_pcmcia_hw_set_cis_rmode();
				if (copy_from_user((char *)attr, uarg, sizeof(*attr)))
				{
					ret = -EFAULT;
					break;
				}
				buf = (unsigned char *)kmalloc(attr->len, GFP_KERNEL);
				if (NULL == buf)
				{
					ret = -ENOMEM;
					break;
				}
				for (i = 0; i < attr->len; i++)
				{
					buf[i] = cnc18xx_pcmcia_hw_read_cis(attr->addr + 2*i);
				}
				attr->rlen = attr->len;
				if (copy_to_user(attr->pbytes, buf, attr->rlen))
				{
					ret = -EFAULT;
				}
				if (copy_to_user(uarg, attr, sizeof(*attr)))
				{
					ret = -EFAULT;
				}
				kfree(buf);
			}
			break;
		case DRV_WRITEMEM:
			{
				DRV_stMem* attr = &(iost.mem);
				cnc18xx_pcmcia_hw_set_cis_wmode();
				if (copy_from_user(attr, uarg, sizeof(*attr)))
				{
					ret = -EFAULT;
					break;
				}
				buf = (unsigned char *)kmalloc(attr->len, GFP_KERNEL);
				if (NULL == buf)
				{
					ret = -ENOMEM;
					break;
				}
				if (copy_from_user(buf, attr->pbytes, attr->len))
				{
					kfree(buf);
					ret = -EFAULT;
					break;
				}
				for (i = 0; i < attr->len; i++)
				{
					cnc18xx_pcmcia_hw_write_cis(buf[i], (attr->addr + i));
				}
				kfree(buf);
			}
			break;
		case DRV_READCOMM:
			{
				DRV_stMem* comm = &(iost.mem);
				cnc18xx_pcmcia_hw_set_comm_rmode();
				if (copy_from_user(comm, uarg, sizeof(*comm)))
				{
					ret = -EFAULT;
					break;
				}
				buf = (unsigned char *)kmalloc(comm->len, GFP_KERNEL);
				if (NULL == buf)
				{
					ret = -ENOMEM;
					break;
				}
				for (i = 0; i < comm->len; i++)
				{
					buf[i] = cnc18xx_pcmcia_hw_read_comm(comm->addr + i);
				}
				comm->rlen = comm->len;
				if (copy_to_user(comm->pbytes, buf, comm->rlen))
				{
					ret = -EFAULT;
				}
				if (copy_to_user(uarg, comm, sizeof(*comm)))
				{
					ret = -EFAULT;
				}
				kfree(buf);
			}
			break;
		case DRV_WRITECOMM:
			{
				DRV_stMem* comm = &(iost.mem);
				cnc18xx_pcmcia_hw_set_comm_wmode();
				if (copy_from_user((char *)comm, uarg, sizeof(*comm)))
				{
					ret = -EFAULT;
					break;
				}
				buf = (unsigned char *)kmalloc(comm->len, GFP_KERNEL);
				if (NULL == buf)
				{
					ret = -ENOMEM;
					break;
				}
				if (copy_from_user(buf, comm->pbytes, comm->len))
				{
					kfree(buf);
					ret = -ENOMEM;
					break;
				}
				for (i = 0; i < comm->len; i++)
				{
					cnc18xx_pcmcia_hw_write_comm(buf[i], comm->addr);
				}
				kfree(buf);
			}
			break;
		case DRV_READIO:
			{
				DRV_stIO* io = &(iost.io);
				unsigned char val;
				cnc18xx_pcmcia_hw_set_io_mode();
				if (copy_from_user((void *)io, uarg, sizeof(*io)))
				{
					ret = -EFAULT;
					break;
				}
				val = cnc18xx_pcmcia_hw_read_io(io->registr);
				if (copy_to_user(io->pvalue, &val, 1))
				{
					ret = -EFAULT;
					break;
				}
			}
			break;
		case DRV_WRITEIO:
			{
				char val;
				DRV_stIO* io = &(iost.io);
				cnc18xx_pcmcia_hw_set_io_mode();
				if (copy_from_user((char *)io, uarg, sizeof(*io)))
				{
					ret = -EFAULT;
					break;
				}
				if (copy_from_user((char *)&val, io->pvalue, 1))
				{
					ret = -EFAULT;
					break;
				}
				cnc18xx_pcmcia_hw_write_io(val, io->registr);
			}
			break;
		case DRV_TSIGNAL:
			{
				DRV_stSignal* rsig = &(iost.rsig);
				if (copy_from_user((char *)rsig, uarg, sizeof(*rsig)))
				{
					ret = -EFAULT;
					break;
				}
				rsig->value = 0;
				switch (rsig->sig)
				{
					case DRV_CARD_DETECT:
						{
							unsigned short rawstat, pinctl;
							rawstat = ioread16((void*)RAWSTAT);
							pinctl = ioread16((void*)PINCTL);
							/*
							 * INT_CDCHG: This bit is set if either of the Card Detect signals, CD1_N or CD2_N, changes level and the
							 * corresponding bit in the INTENA register isset.
							 */
							if((rawstat & INT_CDCHG) && ((PIN_CD1 | PIN_CD2) & pinctl))
							{
								rsig->value = 1;
							}
						}
						break;
					case DRV_READY_BUSY:
						{
						}
						break;
					default:
						{
							return -EIO;
						}
						break;
				}
				if (copy_to_user(uarg, (char *)rsig, sizeof(*rsig)))
				{
					ret = -EFAULT;
				}
			}
			break;
		case DRV_SSIGNAL:
			{
				DRV_ssSignal* wsig = &(iost.wsig);
				if (copy_from_user((char *)wsig, uarg, sizeof(*wsig)))
				{
					ret = -EFAULT;
					break;
				}
				switch (wsig->pin)
				{
					case DRV_EMSTREAM:
						{
						}
						break;
					case DRV_RSTSLOT:
						{
						}
						break;
					case DRV_ENSLOT:
					case DRV_IOMEM:
					case DRV_SLOTSEL:
						break;
					default:
						{
							ret = -EIO;
						}
						break;
				}
			}
			break;
		case DRV_DBG_TOGGLE:
			{
				if (uarg == 0)
				{
					SOCKET_CI_DEBUG = 0;
				}
				else
				{
					SOCKET_CI_DEBUG = 1;
				}
			}
			break;
		default:
			{
				ret = -EIO;
				break;
			}
	}

	return ret;
}

static struct file_operations cnc18xx_pcmcia_ci_fops = {
    .owner   =  THIS_MODULE,
    .open    =  cnc18xx_pcmcia_ci_open,
    .release =  cnc18xx_pcmcia_ci_release,
    .ioctl   =  cnc18xx_pcmcia_ci_ioctl,
};

static struct miscdevice cnc18xx_pcmcia_sci_miscdev = {
    .name = "cnc_socket_ci",
    .minor = MISC_DYNAMIC_MINOR,
    .fops = &cnc18xx_pcmcia_ci_fops
};

#endif
