/*
 * MTD SPI driver for ST M25Pxx (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 *
 * Copyright (c) 2005, Intec Automation Inc.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

 

#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/preempt.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include <linux/device.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/spi/cnc18xx_spi.h>

/* #define DEBUG(n, args...) printk(KERN_ERR args); */

#define CONFIG_SPI_DMA

#define FLASH_PAGESIZE    256

//#define SPI_PINMUX        0x4211000c

/* Flash opcodes. */
#define OPCODE_WREN       0x06       /* Write enable                     */
#define OPCODE_RDSR       0x05       /* Read status register             */
#define OPCODE_WRSR       0x01       /* Write status register 1 byte     */
#define OPCODE_NORM_READ  0x03       /* Read data bytes (low frequency)  */
#define OPCODE_FAST_READ  0x0b       /* Read data bytes (high frequency) */
#define OPCODE_PP         0x02       /* Page program (up to 256 bytes)   */
#define OPCODE_BE_4K      0x20       /* Erase 4KiB block                 */
#define OPCODE_BE_32K     0x52       /* Erase 32KiB block                */
#define OPCODE_CHIP_ERASE 0xc7       /* Erase whole flash chip           */
#define OPCODE_SE         0xd8       /* Sector erase (usually 64KiB)     */
#define OPCODE_RDID       0x9f       /* Read JEDEC ID                    */

#define MAX_RCV_SZ        0x1000
#define RD_SZ             MAX_RCV_SZ

#define DEF_R_SPEED       0x8
#define DEF_W_SPEED       0x8

#define DEF_DMA_R_SPEED   0x4
#define DEF_DMA_W_SPEED   0x2

/* Status Register bits. */
#define SR_WIP            1    /* Write in progress                */
#define SR_WEL            2    /* Write enable latch               */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0            4    /* Block protect 0                  */
#define SR_BP1            8    /* Block protect 1                  */
#define SR_BP2            0x10 /* Block protect 2                  */
#define SR_SRWD           0x80 /* SR write protect                 */

/* Define max times to check status register before we give up. */
#define	MAX_READY_WAIT_JIFFIES	(40 * HZ)	/* M25P16 specs 40s max chip erase */
#define	CMD_SIZE		4

#ifdef CONFIG_M25PXX_USE_FAST_READ
#define OPCODE_READ 	OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE 1
#else
#define OPCODE_READ 	OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE 0
#endif

typedef struct __SPI_REGS {
		u32 ctrlr0;    /*0x00           */
		u32 ctrlr1;    /*0x04           */
		u32 ssienr;    /*0x08           */
		u32 mwcr;      /*0x0C           */
		u32 ser;       /*0x10           */
		u32 baudr;     /*0x14           */
		u32 txftlr;    /*0x18           */
		u32 rxftlr;    /*0x1C           */
		u32 txflr;     /*0x20           */
		u32 rxflr;     /*0x24           */
		u32 sr;        /*0x28           */
		u32 imr;       /*0x2C           */
		u32 isr;       /*0x30           */
		u32 risr;      /*0x34           */
		u32 txoicr;    /*0x38           */
		u32 rxoicr;    /*0x3C           */
		u32 rxuicr;    /*0x40           */
		u32 msticr;    /*0x44           */
		u32 icr;       /*0x48           */
		u32 dmacr;     /*0x4c           */
		u32 dmatdlr;   /*0x50           */
		u32 dmardlr;   /*0x54           */
		u32 rsv1;      /*0x58, reserved */
		u32 rsv2;      /*0x5c, reserved */
		u32 dr;        /*0x60           */
} spi_regs_t;

/*******************************************************************
 * DMA */
#define DW_REG(name)		u32 name; u32 __pad_##name
/* Hardware register definitions. */
struct dw_dma_chan_regs {
    DW_REG(SAR);     /* Source Address Register      */
    DW_REG(DAR);     /* Destination Address Register */
    DW_REG(LLP);     /* Linked List Pointer          */
    u32 CTL_LO;      /* Control Register Low         */
    u32 CTL_HI;      /* Control Register High        */
    DW_REG(SSTAT);
    DW_REG(DSTAT);
    DW_REG(SSTATAR);
    DW_REG(DSTATAR);
    u32 CFG_LO;      /* Configuration Register Low   */
    u32 CFG_HI;      /* Configuration Register High  */
	DW_REG(SGR);
	DW_REG(DSR);
};

struct dw_dma_irq_regs {
	DW_REG(XFER);
	DW_REG(BLOCK);
	DW_REG(SRC_TRAN);
	DW_REG(DST_TRAN);
	DW_REG(ERROR);
};

#define DW_DMA_MAX_NR_CHANNELS 8
#define DW_SPI_REG_DMA         0x4000
typedef struct dw_dma_regs {
	/* per-channel registers */
	struct dw_dma_chan_regs	CHAN[DW_DMA_MAX_NR_CHANNELS];

	/* irq handling */
	struct dw_dma_irq_regs	RAW;		/* r */
	struct dw_dma_irq_regs	STATUS;		/* r (raw & mask) */
	struct dw_dma_irq_regs	MASK;		/* rw (set = irq enabled) */
	struct dw_dma_irq_regs	CLEAR;		/* w (ack, affects "raw") */

	DW_REG(STATUS_INT);			/* r */

	/* software handshaking */
	DW_REG(REQ_SRC);
	DW_REG(REQ_DST);
	DW_REG(SGL_REQ_SRC);
	DW_REG(SGL_REQ_DST);
	DW_REG(LAST_SRC);
	DW_REG(LAST_DST);

	/* miscellaneous */
	DW_REG(CFG);
	DW_REG(CH_EN);
	DW_REG(ID);
	DW_REG(TEST);

	/* optional encoded params, 0x3c8..0x3 */
} dma_regs_t;

struct dw_prd {
        __le32      sar;
        __le32      dar;
	    __le32		llp;
	    __le32		ctl_lo;
	    __le32		ctl_hi;
};

#define DW_PRD_SZ          20
#define DW_MAX_PRD         256
#define DW_PRD_TBL_SZ      (DW_MAX_PRD * DW_PRD_SZ)


/* DMA END *
*******************************************************************/

struct s25f {
	int                    irq;
	struct tasklet_struct  tasklet;

	void __iomem          *spi;
	struct mutex           lock;
	spinlock_t             slock;
	wait_queue_head_t      queue;
	struct mtd_info        mtd;
	unsigned               partitioned:1;
	u8                     erase_opcode;
	u8                     command[CMD_SIZE + FAST_READ_DUMMY_BYTE];

    
    u8                     chardev; /* Config for char device  */

	/* Dma */
	void __iomem          *dma;
	dma_addr_t             rx_dma;
	dma_addr_t             tx_dma;
    struct dw_prd         *prd;        /* our SG list     */
    dma_addr_t             prd_phy;

    int                    dma_inited;
    int                    txdma_done;
    int                    rxdma_done;

    u8                    *dma_buf;    /* dma buf for r/w */
	dma_addr_t             dma_buf_phy;
};

/* Register operation */
#define spi_writeb(b,a,v) iowrite8(v, &b->a)
#define spi_write(b,a,v)  iowrite16(v, &b->a)
#define spi_writel(b,a,v) iowrite32(v, &b->a)

#define spi_readb(b,a)    ioread8(&b->a)
#define spi_read(b,a)     ioread16(&b->a)
#define spi_readl(b,a)    ioread32(&b->a)

#define dma_readl(dw, name)                     \
    __raw_readl(&(dw->name))
#define dma_writel(dw, name, val)               \
    __raw_writel((val), &(dw->name))

#define channel_set_bit(dw, reg, mask)          \
    dma_writel(dw, reg, ((mask) << 8) | (mask))
#define channel_clear_bit(dw, reg, mask)        \
	dma_writel(dw, reg, ((mask) << 8) | 0)

/* Reload func */
#if defined(CONFIG_SPI_DMA) && !defined(CONFIG_SPI_CHARDEV)
#	define              flash_read  dma_flash_read
#	define              flash_write dma_flash_write
#else
#	define              flash_read  io_flash_read
#	define              flash_write io_flash_write
#endif

static int SPI_DEBUG = 0;
#ifdef CONFIG_SPI_DEBUG
# ifdef __KERNEL__
#  define SDEBUG(args...) printk(args)
# else
#  define SDEBUG(args...) printf(args)	
# endif  
#else
# define SDEBUG(args...) \
	if (SPI_DEBUG) printk(args)
#endif

/******************************************************************/
/* Varable definiation */
static struct s25f *flash = NULL;

/******************************************************************/
/* Function definiation */
static int controller_init(spi_regs_t *spi)
{
	unsigned short  ctrl0 = 0;

 //   writel(0x1, IO_ADDRESS(SPI_PINMUX)); /* Enable SPI */

    spi_write(spi, ssienr, 0x0);

	/* Config the CTRLR0 register */
    ctrl0 = 0x0 << 12  | /* CFS          */
            0x0 << 11  | /* SRL 0        */
            0x0 << 10  | /* SLV_OE       */
            0x0 << 8   | /* TMOD master  */
            0x1 << 7   | /* SCPOL clock  */
            0x1 << 6   | /* SCPH         */
            0x0 << 4   | /* frame format */
            0x7 << 0;    /* DFS          */

    spi_write(spi, ctrlr0,  ctrl0          );
    spi_write(spi, ctrlr1,  (MAX_RCV_SZ-1) ); /* only on EE mode             */
    spi_write(spi, imr,     0x0            ); /* Disable all spi interrupts  */
    spi_write(spi, baudr,   DEF_R_SPEED    ); /* rBAUDR 10M                  */
    spi_write(spi, ser,     0x1            ); /* rSER, select the device     */
    spi_write(spi, ssienr,  0x1            ); /* rSSIENR, enable the ssi bus */

    return 0;
}

static inline void write_enable(void)
{
	spi_regs_t *spi = flash->spi;

	/* Unprotect flash */     
	spi_write(spi, ssienr, 0x0     ); 
	spi_write(spi, ctrlr0, 0x1c7   ); 
	spi_write(spi, ssienr, 0x1     ); 
	spi_write(spi, dr, OPCODE_WREN ); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);
}

static void wait_for_finished(void)
{
	unsigned int status;
	spi_regs_t *spi = flash->spi;

	spi_write(spi, ssienr, 0x0   );
	spi_write(spi, ctrlr0, 0x3c7 );
	spi_write(spi, ctrlr1, 0x0   );
	spi_write(spi, ssienr, 0x1   );

	do {
		spi_write(spi, dr,  OPCODE_RDSR);
		while(spi_read(spi, rxflr) != 1);
		status = spi_read(spi, dr);
	} while(status & SR_WIP);
}


/* flash read */
static inline int io_flash_read(unsigned int caddr, unsigned char *buf, size_t len)
{
	unsigned long flags;
	spi_regs_t *spi = flash->spi;
	unsigned char *addr = (unsigned char *)&caddr;
	size_t size = len;

    //DEBUG(MTD_DEBUG_LEVEL2, "%s: flash offset: 0x%x, len: %u\n", __func__, caddr, len);
	spin_lock_irqsave(&flash->slock, flags);

	spi_write(spi, ssienr, 0x0);
	spi_write(spi, ctrlr0, 0x3c7);
	spi_write(spi, ctrlr1, (RD_SZ-1));
	spi_write(spi, baudr,  DEF_R_SPEED);
	spi_write(spi, ssienr, 0x1);
	

	spi_write(spi, dr, OPCODE_READ);
	spi_write(spi, dr, addr[2]);
	spi_write(spi, dr, addr[1]);
	spi_write(spi, dr, addr[0]);

	while (size > 0) {
		if (size >= 16) {
			while (spi_read(spi, rxflr) < 16);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			size   -= 16;
		} else if (size >= 8) {
			while (spi_read(spi, rxflr) < 8);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			size   -= 8;
		} else if (size >= 4) {
			while (spi_read(spi, rxflr) < 4);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			size   -= 4;
		} else if (size >= 2) {
			while (spi_read(spi, rxflr) < 2);
			*buf++ =  spi_read(spi, dr);
			*buf++ =  spi_read(spi, dr);
			size   -= 2;
		} else if (size > 0) {
			while (spi_read(spi, rxflr) == 0);
			*buf++ =  spi_read(spi, dr);
			size   -= 1;
		}
	}

	spin_unlock_irqrestore(&flash->slock, flags);

	return (len - size);
}

static inline int io_flash_write(unsigned int caddr, const u_char *buf, size_t len)
{
	unsigned long flags;
	spi_regs_t *spi = flash->spi;
	unsigned char *addr = (unsigned char *)&caddr;
	size_t size = len;

    //DEBUG(MTD_DEBUG_LEVEL2, "%s: flash offset: 0x%x, len: %u\n", __func__, caddr, len);
    //printk("%s: flash offset: 0x%x, len: %u\n", __func__, caddr, len);

	spin_lock_irqsave(&flash->slock, flags);

	/* Unprotect flash */     
	spi_write(spi, ssienr, 0x0);
	spi_write(spi, ctrlr0, 0x1c7); 
	spi_write(spi, ssienr, 0x1);
	spi_write(spi, dr, OPCODE_WREN); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);

	spi_write(spi, ssienr, 0x0);
	spi_write(spi, ctrlr0, 0x1c7);
	spi_write(spi, baudr,  DEF_W_SPEED);
	spi_write(spi, ssienr, 0x1);


	spi_write(spi, dr, OPCODE_PP); 
	spi_write(spi, dr, addr[2]);
	spi_write(spi, dr, addr[1]);
	spi_write(spi, dr, addr[0]);

	while (size) {
		if (size >= 16) {
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			while (spi_read(spi, txflr) > 8);
			size -= 16;
		} else if (size >= 8) {
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			while (spi_read(spi, txflr) > 8);
			size -= 8;
		} else if (size >= 4) {
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			while (spi_read(spi, txflr) > 8);
			size -= 4;
		} else if (size >= 2) {
			spi_write(spi, dr, *buf++);
			spi_write(spi, dr, *buf++);
			while (spi_read(spi, txflr) > 8);
			size -= 2;
		} else if (size > 0) {
			spi_write(spi, dr, *buf++);
			while (spi_read(spi, txflr) > 8);
			size -= 1;
		}
	}


	while ((spi_read(spi, sr) & 0x5) != 0x4);
	wait_for_finished ();
	spin_unlock_irqrestore(&flash->slock, flags);

	return (len - size);
}

static inline void dma_config_rx(unsigned char *buf, size_t len)
{
    unsigned int   ctl_lo;

    dma_regs_t    *dma    = flash->dma;
    struct dw_prd *prd    = flash->prd;

    /* Gloabl Enable DMA */
    dma_writel(dma, CFG,    0x1);

    ctl_lo =  0x0             /* CTL[31:0]                    */
              | (0x1  << 28)  /* LLP_SRC_EN                   */
              | (0x1  << 27)  /* LLP_DST_EN                   */
              | (0x0  << 25)  /* SRC Master                   */
              | (0x1  << 23)  /* DST Master                   */
              | (0x2  << 20)  /* P -> M, M as flow controller */
              | (0x0  << 18)  /* DSCATTER_EN                  */
              | (0x0  << 17)  /* SGATHER_EN                   */
              | (0x1  << 14)  /* SMSIZE:  4 * 1B              */
              | (0x3  << 11)  /* DMSIZE: 16 * 4B              */
              | (0x2  <<  9)  /* SINC: no change              */
              | (0x0  <<  7)  /* DINC: increment              */
              | (0x0  <<  4)  /* SRC_TR_WIDTH:  8b            */
              | (0x2  <<  1)  /* DST_TR_WIDTH: 32b            */
              | (0x0  <<  0); /* INT_EN                       */

    prd->sar    = PA_SPI_BASE + 0x60;                 /* SAR      */
    prd->dar    = flash->dma_buf_phy;                 /* DAR      */
    prd->llp    = (flash->prd_phy + DW_PRD_SZ) | 0x1; /* Next LLP */
    prd->ctl_lo = ctl_lo;
    prd->ctl_hi = (len > 2048)? 2048 : len;

    if (len > 2048) {
        prd++;
        prd->sar    = PA_SPI_BASE + 0x60;        /* SAR      */
        prd->dar    = flash->dma_buf_phy + 2048; /* DAR      */
        prd->llp    = 0;                         /* Next LLP */
        prd->ctl_lo = ctl_lo | 0x1;
        prd->ctl_hi = len - 2048;
    } else 
        prd->llp = 0;                                 /* Less than 2048, then no xfer */

    dma_writel(dma,      CHAN[0].CTL_LO, 0x3 << 27 );            /* LLP_SRC_EN/LLP_DST_EN                           */
    dma_writel(dma,      CHAN[0].CFG_HI, 0x1 <<  7 );            /* use RX handshaking signals for source periphral */
    dma_writel(dma,      CHAN[0].CFG_LO, 0x1 << 10 );            /* HS_SEL_DST = 0 (HW) HS_SEL_DST = 1 (SW )        */
    dma_writel(dma,      CHAN[0].LLP,    flash->prd_phy | 0x1 ); /* LMS = 1 external DRAM                           */
    channel_set_bit(dma, CH_EN,          0x1       );            /* Enable the only channel                         */

    dma_writel(dma,      CLEAR.XFER,     0x1       );
    channel_set_bit(dma, MASK.XFER,      0x1       );
}

static inline void dma_config_tx(const u_char *buf, size_t len)
{
    unsigned int   ctl_lo;

    dma_regs_t    *dma    = flash->dma;
    struct dw_prd *prd    = flash->prd;

    /* Gloabl Enable DMA */
    dma_writel(dma, CFG,    0x1);

    ctl_lo = 0x0               /* CTL[31:0]                    */
             | ( 0x1  << 28 )  /* LLP_SRC_EN                   */
             | ( 0x1  << 27 )  /* LLP_DST_EN                   */
             | ( 0x1  << 25 )  /* SRC Master                   */
             | ( 0x0  << 23 )  /* DST Master                   */
             | ( 0x1  << 20 )  /* M -> P, M as flow controller */
             | ( 0x0  << 18 )  /* DSCATTER_EN                  */
             | ( 0x0  << 17 )  /* SGATHER_EN                   */
             | ( 0x3  << 14 )  /* SMSIZE: 16 * 4B              */
             | ( 0x1  << 11 )  /* DMSIZE:  4 * 1B              */
             | ( 0x0  <<  9 )  /* SINC: increment              */
             | ( 0x2  <<  7 )  /* DINC: no charge              */
             | ( 0x2  <<  4 )  /* SRC_TR_WIDTH: 32b            */
             | ( 0x0  <<  1 )  /* DST_TR_WIDTH:  8b            */
             | ( 0x1  <<  0 ); /* INT_EN                       */

    prd->sar    = flash->dma_buf_phy; /* SAR      */
    prd->dar    = PA_SPI_BASE + 0x60; /* DAR      */
    prd->llp    = 0;                  /* Next LLP */
    prd->ctl_lo = ctl_lo;
    prd->ctl_hi = (len + 4) / 4;

    dma_writel(dma,      CHAN[0].CTL_LO, 0x3 << 27 );            /* LLP_SRC_EN/LLP_DST_EN                           */
    dma_writel(dma,      CHAN[0].CFG_HI, 0x0 <<  7 );            /* use RX handshaking signals for source periphral */
    dma_writel(dma,      CHAN[0].CFG_LO, 0x2 << 10 );            /* HS_SEL_DST = 0 (HW) HS_SEL_DST = 1 (SW )        */
    dma_writel(dma,      CHAN[0].LLP,    flash->prd_phy | 0x1 ); /* LMS = 1 external DRAM                           */
    channel_set_bit(dma, CH_EN,          0x1       );            /* Enable the only channel                         */

    dma_writel(dma,      CLEAR.XFER,     0x1       );
    channel_set_bit(dma, MASK.XFER,      0x1       );
}

static inline int dma_flash_read(unsigned int caddr, unsigned char *buf, size_t len)
{
    unsigned long  flags;
    size_t         cnt;

    spi_regs_t    *spi    = flash->spi;
    dma_regs_t    *dma    = flash->dma;

    //DEBUG(MTD_DEBUG_LEVEL2, "%s: flash offset: 0x%x, len: %u\n", __func__, caddr, len);

    /* Force 4B align */
    cnt = (len & 0x3) ? ((len + 4) >> 2) << 2: len;

    /* Set Controller */
    spi_write(spi, ssienr,  0x0     );
    spi_write(spi, ctrlr0,  0x3c7   );
    spi_write(spi, ctrlr1,  (cnt-1) );
    spi_write(spi, baudr,   DEF_DMA_R_SPEED);
    spi_write(spi, dmardlr, 3       );
    spi_write(spi, dmacr,   1       );
    spi_write(spi, ssienr,  0x3     );

    /* Config DMA RX */
    dma_config_rx(buf, cnt);

    spi_write(spi, dr    , OPCODE_READ        ); 
    spi_write(spi, dr    , (caddr>>16) & 0xff ); 
    spi_write(spi, dr    , (caddr>>8)  & 0xff ); 
    spi_write(spi, dr    , (caddr)     & 0xff ); 
    spi_write(spi, ssienr, 0x1                ); 

    while (dma_readl(dma, CH_EN));
#if 0
    {
		u32 status_block;
		u32 status_xfer;
		u32 status_err;
		u32 st;
		st = dma_readl(dma, STATUS_INT);

		status_block = dma_readl(dma, RAW.BLOCK);
		status_xfer  = dma_readl(dma, RAW.XFER);
		status_err   = dma_readl(dma, RAW.ERROR);
		printk("block: %x, xfer: %x, err: %x, ctl_hi: %x -- st: %x -- len: %x\n", 
			   status_block, status_xfer, status_err, prd->ctl_hi, st, len);

    }
#endif
    memcpy(buf, flash->dma_buf, len);

    return (len);
}

static inline int dma_flash_write_aln(unsigned int caddr, const u_char *buf, size_t len)
{
    spi_regs_t    *spi    = flash->spi;
    dma_regs_t    *dma    = flash->dma;

    //DEBUG(MTD_DEBUG_LEVEL2, "%s: flash offset: 0x%x, len: %u\n", __func__, caddr, len);

	if (len & 0x3)
		return -EINVAL;

    /* Write_Enable();*/
    spi_write(spi, ssienr,  0x0         );
    spi_write(spi, ctrlr0,  0x1c7       );
    spi_write(spi, ssienr,  0x1         );
    spi_write(spi, dr,      OPCODE_WREN );
    while ((spi_read(spi, sr) & 0x5) != 0x4);

    /* Reorganize Data */
    flash->dma_buf[0] = OPCODE_PP;
    flash->dma_buf[1] = (caddr>>16) & 0xff;
    flash->dma_buf[2] = (caddr>>8)  & 0xff;
    flash->dma_buf[3] = (caddr)     & 0xff;
    memcpy(flash->dma_buf + 4, buf, len);

    /* Set Controller */
    spi_write(spi, ssienr,  0x0         );
    spi_write(spi, ctrlr0,  0x1c7       ); /* Transmit Only                                                */
    spi_write(spi, dmatdlr, 16          ); /* When FIFO half full, receive from DMA controller immediately */
    spi_write(spi, dmacr,   0x2         ); /* Transmit FIFO DMA request enabled                            */
    spi_write(spi, baudr,   DEF_DMA_W_SPEED);
    spi_write(spi, ssienr,  0x1         );

    /* Config DMA TX */
    dma_config_tx(buf, len);

    while (dma_readl(dma, CH_EN));
	while((spi_read(spi, sr) & 0x5) != 0x4);
    wait_for_finished();

    return (len);
}

static inline int dma_flash_write(unsigned int caddr, const u_char *buf, size_t len)
{
	size_t remain;
	size_t cnt;

	if (len == 0 || len > FLASH_PAGESIZE) {
		printk("%s: Write size is invalid, len: %u\n", __func__, len);
		return -EINVAL;
	}

	remain = len % 4;
	cnt = len - remain;

	DEBUG(MTD_DEBUG_LEVEL2, "remain: %d, cnt: %d len: %d\n", remain, cnt, len);

	while (cnt && 
		   (cnt != dma_flash_write_aln(caddr, buf, cnt)))
		printk("%s: DMA Write Error, Reread! ret: %d\n", __func__, cnt);

	while (remain && 
		   (remain != io_flash_write(caddr+cnt, buf+cnt, remain)))
		printk("%s: IO Write Error, Reread! ret: %d\n", __func__, cnt);

	return (len);
}

/*********************************************************************************/
/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(void)
{
	spi_regs_t *spi = flash->spi;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %lldKiB\n",
		  "spi-flash", __func__,
		  (long long)(flash->mtd.size >> 10));

	/* Unprotect flash */
	spi_write(spi, ssienr, 0x0         ); 
	spi_write(spi, ctrlr0, 0x107       ); 
	spi_write(spi, ssienr, 0x1         ); 
	spi_write(spi, dr,     OPCODE_WREN ); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);

	spi_write(spi, ssienr, 0x0         ); 
	spi_write(spi, ctrlr0, 0x1c7       ); 
	spi_write(spi, baudr,  DEF_R_SPEED ); 
	spi_write(spi, ssienr, 0x1         ); 

	spi_write(spi, dr, OPCODE_CHIP_ERASE); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);

	/* Wait until finished previous write command. */
	wait_for_finished();

	return 0;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(u32 offset)
{
	spi_regs_t *spi = flash->spi;
	unsigned char *addr = (unsigned char *)&offset;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %dKiB at 0x%08x\n",
			"spi-flash", __func__,
			flash->mtd.erasesize / 1024, offset);

	spi_write(spi, ssienr, 0x0         ); 
	spi_write(spi, ctrlr0, 0x107       ); 
	spi_write(spi, ssienr, 0x1         ); 
	spi_write(spi, dr,     OPCODE_WREN ); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);

	spi_write(spi, ssienr, 0x0         ); 
	spi_write(spi, ctrlr0, 0x1c7       ); 
	spi_write(spi, baudr,  DEF_R_SPEED ); 
	spi_write(spi, ssienr, 0x1         ); 

	spi_write(spi, dr,     flash->erase_opcode   ); 
	spi_write(spi, dr,     addr[2]     ); 
	spi_write(spi, dr,     addr[1]     ); 
	spi_write(spi, dr,     addr[0]     ); 
	while ((spi_read(spi, sr) & 0x5) != 0x4);

	wait_for_finished();

	return 0;
}

/****************************************************************************/

/*
 * MTD implementation
 */

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int s25f_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	u32 addr,len;
	uint32_t rem;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%llx, len %lld\n",
	      "spi-flash", __func__, "at",
	      (long long)instr->addr, (long long)instr->len);

	/* sanity checks */
	if (instr->addr + instr->len > flash->mtd.size)
		return -EINVAL;

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	addr = instr->addr;
	len = instr->len;

	mutex_lock(&flash->lock);

	/* whole-chip erase? */
	if (len == flash->mtd.size) {
		if (erase_chip()) {
			instr->state = MTD_ERASE_FAILED;
			mutex_unlock(&flash->lock);
			return -EIO;
		}

	/* REVISIT in some cases we could speed up erasing large regions
	 * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
	 * to use "small sector erase", but that's not always optimal.
	 */

	/* "sector"-at-a-time erase */
	} else {
		while (len) {
			if (erase_sector(addr)) {
				instr->state = MTD_ERASE_FAILED;
				mutex_unlock(&flash->lock);
				return -EIO;
			}

			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	}

	mutex_unlock(&flash->lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int s25f_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	u_char *tbuf = buf;
	size_t remain = len;
	loff_t caddr = from;

	int size;
	int ret_cnt;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			"spi-flash", __func__, "from",
			(u32)from, len);

	/* sanity checks */
	if (!len)
		return 0;

	if (from + len > flash->mtd.size)
		return -EINVAL;

	/* Byte count starts at zero. */
	if (retlen)
		*retlen = 0;

	mutex_lock(&flash->lock);

	while (remain > 0) {
		size = (remain >= RD_SZ) ? RD_SZ : remain;

		ret_cnt = flash_read(caddr, tbuf, size); 
		if (ret_cnt != size) {
			printk("SPI FLASH Read ERROR!!! Continue\n");
			continue;
		}

		caddr  += size;
		tbuf   += size;
		remain -= size;
	}

	*retlen = len - remain;

	mutex_unlock(&flash->lock);

	return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int s25f_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	u32 page_offset, page_size;
	int ret_cnt;

	DEBUG(MTD_DEBUG_LEVEL2, "%s: %s %s 0x%08x, len %zd\n",
			"spi-flash", __func__, "to",
			(u32)to, len);

	if (retlen)
		*retlen = 0;

	/* sanity checks */
	if (!len)
		return(0);

	if (to + len > flash->mtd.size)
		return -EINVAL;

	mutex_lock(&flash->lock);

	/* what page do we start with? */
	page_offset = to % FLASH_PAGESIZE;

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= FLASH_PAGESIZE) {

		ret_cnt = flash_write(to, buf, len);
		*retlen = ret_cnt;
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = FLASH_PAGESIZE - page_offset;

		ret_cnt = flash_write(to, buf, page_size);
		*retlen = ret_cnt;

		/* write everything in PAGESIZE chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > FLASH_PAGESIZE)
				page_size = FLASH_PAGESIZE;

			/* write the next page to flash */
				ret_cnt = flash_write((to+i), (buf+i), page_size);

			if (retlen)
				*retlen += ret_cnt;
		}
	}

	mutex_unlock(&flash->lock);

	return 0;
}

/****************************************************************************/
/*
 * SPI device driver setup and teardown
 */

struct flash_info {
	char     *name;

	/* JEDEC id zero means "no ID" (most older chips); otherwise it has
	 * a high byte of zero plus three data bytes: the manufacturer id,
	 * then a two byte device id.
	 */
	u32       jedec_id;
	u16       ext_id;

	/* The size listed here is what works with OPCODE_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned  sector_size;
	u16       n_sectors;

	u16       flags;
#define	SECT_4K		0x01		/* OPCODE_BE_4K works uniformly */
};

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct flash_info __devinitdata s25f_data [] = {
	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ "at25fs010" , 0x1f6601, 0, 32 * 1024, 4  , SECT_4K },
	{ "at25fs040" , 0x1f6604, 0, 64 * 1024, 8  , SECT_4K },

	{ "at25df041a", 0x1f4401, 0, 64 * 1024, 8  , SECT_4K },
	{ "at25df641" , 0x1f4800, 0, 64 * 1024, 128, SECT_4K },

	{ "at26f004"  , 0x1f0400, 0, 64 * 1024, 8  , SECT_4K },
	{ "at26df081a", 0x1f4501, 0, 64 * 1024, 16 , SECT_4K },
	{ "at26df161a", 0x1f4601, 0, 64 * 1024, 32 , SECT_4K },
	{ "at26df321" , 0x1f4700, 0, 64 * 1024, 64 , SECT_4K },

	/* EON -- en25xxx */
	{ "en25f32", 0x1c3116, 0, 64 * 1024, 64 , SECT_4K },
	{ "en25p32", 0x1c2016, 0, 64 * 1024, 64 , 0 }      ,
	{ "en25p64", 0x1c2017, 0, 64 * 1024, 128, 0 }      ,

	/* Intel/Numonyx -- xxxs33b */
	{ "160s33b",  0x898911, 0, 64 * 1024,  32, 0 },
	{ "320s33b",  0x898912, 0, 64 * 1024,  64, 0 },
	{ "640s33b",  0x898913, 0, 64 * 1024, 128, 0 },

	/* Macronix */
        { "mx25l512e",   0xc22010, 0,  4 * 1024,  16, SECT_4K },
        { "mx25L1006e",  0xc22011, 0, 64 * 1024,   2, SECT_4K },
	{ "mx25l4005a",  0xc22013, 0, 64 * 1024,   8, SECT_4K },
	{ "mx25l8006e",   0xc22014, 0, 64 * 1024,  16, SECT_4K },
	{ "mx25l8005",   0xc22014, 0, 64 * 1024,  16, 0 },
	{ "mx25l1606e",  0xc22015, 0, 64 * 1024,  32, SECT_4K },
	{ "mx25l3205d",  0xc22016, 0, 64 * 1024,  64, 0 },
	{ "mx25l6405d",  0xc22017, 0, 64 * 1024, 128, SECT_4K },
	{ "mx25l6445e",  0xc22017, 0, 64 * 1024, 128, SECT_4K },
	{ "mx25l12805d", 0xc22018, 0, 64 * 1024, 256, 0 },
	{ "mx25l12855e", 0xc22618, 0, 64 * 1024, 256, 0 },
	{ "mx25l25635e", 0xc22019, 0, 64 * 1024, 512, 0 },
	{ "mx25l25655e", 0xc22619, 0, 64 * 1024, 512, 0 },

	/* Spansion -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ "s25sl004a",  0x010212,      0,  64 * 1024,   8, 0 },
	{ "s25sl008a",  0x010213,      0,  64 * 1024,  16, 0 },
	{ "s25sl016a",  0x010214,      0,  64 * 1024,  32, 0 },
	{ "s25sl032a",  0x010215,      0,  64 * 1024,  64, 0 },
	{ "s25sl032p",  0x010215, 0x4d00,  64 * 1024,  64, SECT_4K },
	{ "s25sl064a",  0x010216,      0,  64 * 1024, 128, 0 },
	{ "s25fl256s0", 0x010219, 0x4d00, 256 * 1024, 128, 0 },
	{ "s25fl256s1", 0x010219, 0x4d01,  64 * 1024, 512, 0 },
	{ "s25fl512s",  0x010220, 0x4d00, 256 * 1024, 256, 0 },
	{ "s70fl01gs",  0x010221, 0x4d00, 256 * 1024, 256, 0 },
	{ "s25sl12800", 0x012018, 0x0300, 256 * 1024,  64, 0 },
	{ "s25sl12801", 0x012018, 0x0301,  64 * 1024, 256, 0 },
	{ "s25fl129p0", 0x012018, 0x4d00, 256 * 1024,  64, 0 },
	{ "s25fl129p1", 0x012018, 0x4d01,  64 * 1024, 256, 0 },
	{ "s25fl016k",  0xef4015,      0,  64 * 1024,  32, SECT_4K },
	{ "s25fl064k",  0xef4017,      0,  64 * 1024, 128, SECT_4K },

	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ "sst25vf040b", 0xbf258d, 0, 64 * 1024,  8, SECT_4K },
	{ "sst25vf080b", 0xbf258e, 0, 64 * 1024, 16, SECT_4K },
	{ "sst25vf016b", 0xbf2541, 0, 64 * 1024, 32, SECT_4K },
	{ "sst25vf032b", 0xbf254a, 0, 64 * 1024, 64, SECT_4K },
	{ "sst25wf512",  0xbf2501, 0, 64 * 1024,  1, SECT_4K },
	{ "sst25wf010",  0xbf2502, 0, 64 * 1024,  2, SECT_4K },
	{ "sst25wf020",  0xbf2503, 0, 64 * 1024,  4, SECT_4K },
	{ "sst25wf040",  0xbf2504, 0, 64 * 1024,  8, SECT_4K },

	/* ST Microelectronics -- newer production may have feature updates */
	{ "m25p05",  0x202010,  0,  32 * 1024,   2, 0 },
	{ "m25p10",  0x202011,  0,  32 * 1024,   4, 0 },
	{ "m25p20",  0x202012,  0,  64 * 1024,   4, 0 },
	{ "m25p40",  0x202013,  0,  64 * 1024,   8, 0 },
	{ "m25p80",  0x202014,  0,  64 * 1024,  16, 0 },
	{ "m25p16",  0x202015,  0,  64 * 1024,  32, 0 },
	{ "m25p32",  0x202016,  0,  64 * 1024,  64, 0 },
	{ "m25p64",  0x202017,  0,  64 * 1024, 128, 0 },
	{ "m25p128", 0x202018,  0, 256 * 1024,  64, 0 },

	{ "m25p05-nonjedec",  0, 0,  32 * 1024,   2, 0 },
	{ "m25p10-nonjedec",  0, 0,  32 * 1024,   4, 0 },
	{ "m25p20-nonjedec",  0, 0,  64 * 1024,   4, 0 },
	{ "m25p40-nonjedec",  0, 0,  64 * 1024,   8, 0 },
	{ "m25p80-nonjedec",  0, 0,  64 * 1024,  16, 0 },
	{ "m25p16-nonjedec",  0, 0,  64 * 1024,  32, 0 },
	{ "m25p32-nonjedec",  0, 0,  64 * 1024,  64, 0 },
	{ "m25p64-nonjedec",  0, 0,  64 * 1024, 128, 0 },
	{ "m25p128-nonjedec", 0, 0, 256 * 1024,  64, 0 },

	{ "m45pe10", 0x204011,  0, 64 * 1024,    2, 0 },
	{ "m45pe80", 0x204014,  0, 64 * 1024,   16, 0 },
	{ "m45pe16", 0x204015,  0, 64 * 1024,   32, 0 },

	{ "m25pe80", 0x208014,  0, 64 * 1024, 16,       0 },
	{ "m25pe16", 0x208015,  0, 64 * 1024, 32, SECT_4K },

	{ "m25px32",    0x207116,  0, 64 * 1024, 64, SECT_4K },
	{ "m25px32-s0", 0x207316,  0, 64 * 1024, 64, SECT_4K },
	{ "m25px32-s1", 0x206316,  0, 64 * 1024, 64, SECT_4K },
	{ "m25px64",    0x207117,  0, 64 * 1024, 128, 0 },

	/* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	{ "w25x10", 0xef3011, 0, 64 * 1024,  2,  SECT_4K },
	{ "w25x20", 0xef3012, 0, 64 * 1024,  4,  SECT_4K },
	{ "w25x40", 0xef3013, 0, 64 * 1024,  8,  SECT_4K },
	{ "w25x80", 0xef3014, 0, 64 * 1024,  16, SECT_4K },
	{ "w25x16", 0xef3015, 0, 64 * 1024,  32, SECT_4K },
	{ "w25x32", 0xef3016, 0, 64 * 1024,  64, SECT_4K },
	{ "w25q80", 0xef4014, 0, 64 * 1024,  16, SECT_4K },
	{ "w25q16", 0xef4015, 0, 64 * 1024,  32, SECT_4K },
	{ "w25q32", 0xef4016, 0, 64 * 1024,  64, SECT_4K },
	{ "w25x64", 0xef3017, 0, 64 * 1024, 128, SECT_4K },
	{ "w25q64", 0xef4017, 0, 64 * 1024, 128, SECT_4K },
	{ "w25q64bv", 0xef4017 , 0, 64 * 1024 , 128, SECT_4K },  
	{ "w25q64-DIP8", 0x00ef4014, 0, 64 * 1024 , 128, SECT_4K },  
	{ "s25fl008a", 0x1c2017, 0, 64 * 1024, 128, },
	{ "q0bb02b", 0x1c3017, 0, 64 * 1024, 128, },
	{ "GD25Q80", 0xc84014, 0, 64 * 1024, 128, SECT_4K },
	{ "GD25Q512",0xc84010, 0, 32 * 1024, 2, SECT_4K },
};

static int get_manufacture_id(unsigned char *buf, int size)
{
	spi_regs_t *spi = flash->spi;

	spi_write(spi, ssienr, 0x0);
	spi_write(spi, ctrlr0, 0x3c7);
	spi_write(spi, ctrlr1, 0x4);
	spi_write(spi, ssienr, 0x1);
	
	spi_write(spi, dr, OPCODE_RDID);
	while (spi_read(spi, rxflr) < 5);

	*buf++ = spi_read(spi, dr);
	*buf++ = spi_read(spi, dr);
	*buf++ = spi_read(spi, dr);
	*buf++ = spi_read(spi, dr);
	*buf++ = spi_read(spi, dr);

	return 0;
}

static struct flash_info *__devinit jedec_probe(void)
{
	int                tmp;
	u8                 id[5];
	u32                jedec;
	u16                ext_jedec;
	struct flash_info *info;

	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	tmp = get_manufacture_id(id, 5);
	if (tmp < 0) {
		DEBUG(MTD_DEBUG_LEVEL0, "%s: error %d reading JEDEC ID\n",
			"spi-flash", tmp);
		return NULL;
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];

	ext_jedec = id[3] << 8 | id[4];

ComingFromDefault:

	for (tmp = 0, info = s25f_data; tmp < ARRAY_SIZE(s25f_data); tmp++, info++) 
    {
		if (info->jedec_id == jedec) 
        {
			if (info->ext_id != 0 && info->ext_id != ext_jedec)
            {
    			continue;
            }
			return info;
		}
	}
	DEBUG(MTD_DEBUG_LEVEL2, "unrecognized JEDEC id %06x\n", jedec);
    
    info=NULL;
    jedec = 0x00ef4014 ;
    ext_jedec = 0;
    goto ComingFromDefault;

	return info;
}

/***************************************************************************************************/

#define NB_OF(x) (sizeof (x) / sizeof (x[0]))

static struct mtd_erase_region_info erase_regions[] = {
	/* parameter blocks */
	{
		.offset		= 0x00000000,
		.erasesize	= 4 * 1024,
		.numblocks	= 2,
	},
	/* main blocks */
	{
		.offset	 	= 0x00002000,
		.erasesize	= 8 * 1024,
		.numblocks	= 1,
	},
	{
		.offset	 	= 0x00004000,
		.erasesize	= 16 * 1024,
		.numblocks	= 1,
	},
	{
		.offset	 	= 0x00008000,
		.erasesize	= 32 * 1024,
		.numblocks	= 1,
	},
	{
		.offset	 	= 0x00010000,
		.erasesize	= 64 * 1024,
		.numblocks	= 127,
	}
};

#ifdef CONFIG_MTD_PARTITIONS
/*
 * Define static partitions for flash device
 */
#ifdef CONFIG_CELESTIAL_TIGA_MINI
#define NUM_PARTITIONS_SMALL 3
static struct mtd_partition spi_partition_info_small[] = { /* for SPI flashes <= 64 KB */
    { .name   = "spi_cavm_miniloader",
      .offset = 0x0,
      .size   = 0x0000c000 },
   
    { .name   = "spi_cavm_bootloader_env1",
      .offset = 0x0000c000,
      .size   = 0x00002000 },
  
    { .name   = "spi_cavm_bootloader_env2",
      .offset = 0x0000e000,
      .size   = 0x00002000 }
};

#define NUM_PARTITIONS 5
static struct mtd_partition spi_partition_info_normal[] = {
    { .name   = "spi_cavm_miniloader",
      .offset = 0x0,
      .size   = 0x00060000 },

    { .name   = "spi_cavm_bootloader_env1",
      .offset = 0x00060000,
      .size   = 0x00002000 },

    { .name   = "spi_cavm_free1",
      .offset = 0x00062000,
      .size   = 0x0000e000 },

    { .name   = "spi_cavm_bootloader_env2",
      .offset = 0x00070000,
      .size   = 0x00002000 },

    { .name   = "spi_cavm_free2",
      .offset = 0x00072000,
      .size   = 0x0078e000 },
};
#else
#define NUM_PARTITIONS 3
static struct mtd_partition spi_partition_info[] = {
    { .name   = "uboot",
      .offset = 0x0,
      .size   = 0xe0000 },
   
    { .name   = "uboot-env",
      .offset = 0xe0000,
      .size   = 0x10000 },
  
    { .name   = "spi spare",
      .offset = 0xf0000,
      .size   = 0x10000 }
};
#endif
#endif
#if 0
static void do_dma_tasklet(unsigned long data)
{
	struct s25f *flash = (struct s25f *)data;
/*    spi_regs_t * spi   = flash->spi;*/
	dma_regs_t * dma   = flash->dma;

	u32 status_block;
	u32 status_xfer;
	u32 status_err;

	status_block = dma_readl(dma, RAW.BLOCK);
	status_xfer  = dma_readl(dma, RAW.XFER);
	status_err   = dma_readl(dma, RAW.ERROR);

	printk("tasklet: status_block=%x status_err=%x\n",
			status_block, status_err);

	spin_lock(&flash->slock);

	if (status_block) {
		printk("block finished!\n");
		write_enable();
	} else if (status_xfer) {
		printk("transfer finished!\n");
/*        wake_up_interruptible();*/
	} else if (status_err)
		printk("XXXXXXXXXXXXXXX   SPI DMA TRANSFER ERROR!\n");

	spin_unlock(&flash->slock);

	/*
	 * Re-enable interrupts. Block Complete interrupts are only
	 * enabled if the INT_EN bit in the descriptor is set. This
	 * will trigger a scan before the whole list is done.
	 */
}

static irqreturn_t do_spi_irq(int irq, void *dev_id)
{
	u32 status;
	u32 dma_cfg;
	struct s25f *flash = dev_id;
	spi_regs_t * spi = flash->spi;
	dma_regs_t * dma = flash->dma;

	printk("interrupt: status=0x%x\n", dma_readl(dma, STATUS_INT));

	channel_clear_bit(dma, MASK.XFER,  1);
	channel_clear_bit(dma, MASK.BLOCK, 1);
	channel_clear_bit(dma, MASK.ERROR, 1);

	status = dma_readl(dma, STATUS_INT);
	if (status)
		printk( "BUG: Unexpected interrupts pending: 0x%x\n", status);

	u32 status_block;
	u32 status_xfer;
	u32 status_err;

	status_block = dma_readl(dma, RAW.BLOCK);
	status_xfer  = dma_readl(dma, RAW.XFER);
	status_err   = dma_readl(dma, RAW.ERROR);

	printk("tasklet: status_block=%x status_err=%x\n",
			status_block, status_err);

	if (status_block) {
		printk("block finished!\n");
		dma_cfg = dma_readl(dma, CHAN[0].CFG_LO);
		dma_writel(dma, CHAN[0].CFG_LO, dma_cfg&(1<<8));
		write_enable();
		dma_writel(dma, CHAN[0].CFG_LO, dma_cfg);
	} 
	else if (status_xfer) {
		printk("transfer finished!\n");
/*        wake_up_interruptible();*/
	} 
	else if (status_err)
		printk("XXXXXXXXXXXXXXX   SPI DMA TRANSFER ERROR!\n");

	channel_set_bit(dma, MASK.XFER, 1);
	channel_set_bit(dma, MASK.BLOCK, 1);
	channel_set_bit(dma, MASK.ERROR, 1);

	return IRQ_HANDLED;
}
#endif

/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */

static int spi_sys_register(struct platform_device *pdev);
static void spi_sys_unregister(struct platform_device *pdev);

static int __init s25f_probe(struct platform_device *pdev)
{
	struct flash_info *info;
	unsigned           i;
	int                ret;

    /*
     * System Register.
     */
    if ((ret = spi_sys_register(pdev)) < 0)
        goto out;

    /*
     * If configed for char device, then skip the mtd branch. 
     */
    if (ret > 0)
        return 0;

	info = jedec_probe();
	if (!info) {
		ret = -ENODEV;
        goto out;
    }

	/*
	 * Atmel serial flash tend to power up
	 * with the software protection bits set
	 */

	if (info->jedec_id >> 16 == 0x1f) {
        /*******************
         * write_enable(); *
         * write_sr(0);    *
         *******************/
		;
	}

	flash->mtd.name = "s25f";
	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 1;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = info->sector_size * info->n_sectors;

	flash->mtd.erase = s25f_erase;
	flash->mtd.read = s25f_read;
	flash->mtd.write = s25f_write;

#if 1
	if (!strcmp(info->name, "s25fl008a")) {
		flash->mtd.numeraseregions = NB_OF (erase_regions);
		flash->mtd.eraseregions = erase_regions;
	}
#endif

	/* prefer "small sector" erase if possible */
	if (info->flags & SECT_4K) {
		flash->erase_opcode = OPCODE_BE_4K;
		flash->mtd.erasesize = 4096;
	} else {
		flash->erase_opcode = OPCODE_SE;
		flash->mtd.erasesize = info->sector_size;
	}

	DEBUG(MTD_DEBUG_LEVEL2, "%s (%lld Kbytes)\n", info->name,
			(long long)flash->mtd.size >> 10);

	DEBUG(MTD_DEBUG_LEVEL2,
		"mtd .name = %s, .size = 0x%llx (%lldMiB) "
			".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
		flash->mtd.name,
		(long long)flash->mtd.size, (long long)(flash->mtd.size >> 20),
		flash->mtd.erasesize, flash->mtd.erasesize / 1024,
		flash->mtd.numeraseregions);

	if (flash->mtd.numeraseregions)
		for (i = 0; i < flash->mtd.numeraseregions; i++)
			DEBUG(MTD_DEBUG_LEVEL2,
				"mtd.eraseregions[%d] = { .offset = 0x%llx, "
				".erasesize = 0x%.8x (%uKiB), "
				".numblocks = %d }\n",
				i, (long long)flash->mtd.eraseregions[i].offset,
				flash->mtd.eraseregions[i].erasesize,
				flash->mtd.eraseregions[i].erasesize / 1024,
				flash->mtd.eraseregions[i].numblocks);


	/* partitions should match sector boundaries; and it may be good to
	 * use readonly partitions for writeprotected sectors (BP2..BP0).
	 */
	if (mtd_has_partitions()) {
		struct mtd_partition *parts    = NULL;
		int                   nr_parts = 0;


		if (mtd_has_cmdlinepart()) {
			static const char *part_probes[]
				= { "cmdlinepart", NULL, };

			nr_parts = parse_mtd_partitions(&flash->mtd,
					part_probes, &parts, 0);
		}

		if (nr_parts <= 0) {
#ifdef CONFIG_CELESTIAL_TIGA_MINI
			if (flash->mtd.size >= 0x100000) {
				/* for SPI Flash that is at least 1 megabyte big */
				parts = spi_partition_info_normal;
				nr_parts = NUM_PARTITIONS;
			} else {
				/* for SPI Flash that is smaller than 1 megabyte */
				nr_parts = NUM_PARTITIONS_SMALL;
				parts = spi_partition_info_small;
			}
#else
			parts = spi_partition_info;
			nr_parts = NUM_PARTITIONS;
#endif
		}

		if (nr_parts > 0) {
			for (i = 0; i < nr_parts; i++) {
				DEBUG(MTD_DEBUG_LEVEL2, "partitions[%d] = "
					"{.name = %s, .offset = 0x%llx, "
						".size = 0x%llx (%lldKiB) }\n",
					i, parts[i].name,
					(long long)parts[i].offset,
					(long long)parts[i].size,
					(long long)(parts[i].size >> 10));
			}
			flash->partitioned = 1;
			return add_mtd_partitions(&flash->mtd, parts, nr_parts);
		}
	} 

	return add_mtd_device(&flash->mtd) == 1 ? -ENODEV : 0;

out:
    return ret;
}

static int __exit s25f_remove(struct platform_device *pdev)
{
	int status = 0;

	if (flash == NULL) {
		DEBUG(MTD_DEBUG_LEVEL2, "%s: flash resources already freed!\n", __func__);
		return 0;
	}

    if (!flash->chardev) {
        /* Clean up MTD stuff. */
        if (flash->partitioned)
            status = del_mtd_partitions(&flash->mtd);
        else
            status = del_mtd_device(&flash->mtd);
    }

    spi_sys_unregister(pdev);

	return status;
}

static struct platform_driver s25f_driver = {
	.driver = {
		.name	= "cnc18xx_spi",
		.owner	= THIS_MODULE,
	},
	.remove		= __exit_p(s25f_remove),
};

static int __init s25f_init(void)
{
	return platform_driver_probe(&s25f_driver, s25f_probe);
}

static void __exit s25f_exit(void)
{
	platform_driver_unregister(&s25f_driver);
}

module_init(s25f_init);
module_exit(s25f_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sun He");
MODULE_DESCRIPTION("MTD SPI driver for Winbond S25Fxxx flash chips");

/* Ioctl */
#define SPI_MAGIC      'S'

#define SPI_SET_BUS    _IOW(SPI_MAGIC, 0, unsigned long)
#define SPI_SET_CS     _IOW(SPI_MAGIC, 1, unsigned long)
#define SPI_SET_BAUD   _IOW(SPI_MAGIC, 2, unsigned long)
#define SPI_SET_TMOD   _IOW(SPI_MAGIC, 3, unsigned long)
#define SPI_SET_POL    _IOW(SPI_MAGIC, 4, unsigned long)
#define SPI_SET_PH     _IOW(SPI_MAGIC, 5, unsigned long)
#define SPI_SET_TIMING _IOW(SPI_MAGIC, 6, unsigned long)
#define SPI_SET_NDF    _IOW(SPI_MAGIC, 7, unsigned long)

#define SPI_INTR_MASK  _IOW(SPI_MAGIC, 8, unsigned long)

#define SPI_READ_BYTE  _IOW(SPI_MAGIC, 9, unsigned long)
#define SPI_WRITE_BYTE _IOW(SPI_MAGIC, 10, unsigned long)

/* 
 * Bus state choise
 * @ enable
 * @ disable
 */
typedef enum CSSPI_SPI_{
	CSSPI_ENABLE = 0,
	CSSPI_DISABLE } CSSPI_SPI;

/*
 * Slave select
 * @ get cs
 * @ drop cs
 */
typedef enum CSSPI_SLAVE_{
	CSSPI_SENABLE = 0,
	CSSPI_SDISABLE } CSSPI_SLAVE;

typedef enum CSSPI_TIMING_ {
	CSSPI_MODE0 = 0,
	CSSPI_MODE1,
	CSSPI_MODE2,
	CSSPI_MODE3 } CSSPI_TIMING;

/*
 * Transfer mode
 * @ trans and receive
 * @ trans only
 * @ receive only
 * @ reservered
 */
typedef enum CSSPI_TMOD_ {
	CSSPI_TXRX = 0,
	CSSPI_TXO,
	CSSPI_RXO,
	CSSPI_EEPROM } CSSPI_TMOD;   

typedef enum CSSPI_SCPOL_ {
	CSSPI_LOWPOL= 0,
	CSSPI_HIGHPOL } CSSPI_SCPOL;

typedef enum CSSPI_SCPH_ {
	CSSPI_MIDDLE = 0,
	CSSPI_START } CSSPI_SCPH;

/*
 * Baud rate
 * @ 10MHz
 * @ 12MHz
 * @ 16MHz
 * @ 23MHz
 * @ 46MHz
 */
typedef enum CSSPI_SCBAUD_ {
	CSSPI_10MH = 0xa,
	CSSPI_12MH = 0x8,
	CSSPI_16MH = 0x6,
	CSSPI_23MH = 0x4,
	CSSPI_46MH = 0x2 } CSSPI_SCBAUD;

static void __spi_set_bus(CSSPI_SPI enable)
{
    spi_regs_t *spi = flash->spi;

    spi_write(spi, ssienr, enable);
}

static void __spi_set_cs(CSSPI_SLAVE cs)
{
    spi_regs_t *spi = flash->spi;

    /*
     * Force minimal delay between two transfers - in case two transfers
     * follow each other w/o delay, then we have to wait here in order for
     * the peripheral device to detect cs transition from inactive to active.
     */
    spi_write(spi, ser, cs);

    /* Give a wait time to ensure the state can be set */
    udelay(1);
}

static void __spi_set_ndf(u32 ndf)
{
    spi_regs_t *spi = flash->spi;

    spi_write(spi, ssienr, 0);
    spi_write(spi, ctrlr1, ndf-1);
    spi_write(spi, ssienr, 1);
}

static void __spi_set_baud(CSSPI_SCBAUD baud)
{
    spi_regs_t *spi = flash->spi;

    spi_write(spi, ssienr, 0);
    spi_write(spi, baudr, baud);
    spi_write(spi, ssienr, 1);

}

static void __spi_set_tmod(CSSPI_TMOD tmod)
{
    spi_regs_t *spi = flash->spi;

    u16 ctrl0 = spi_read(spi, ctrlr0);

    ctrl0 &= ~(0x3<<8);
    ctrl0 |= tmod << 8;

    spi_write(spi, ssienr, 0);
    spi_write(spi, ctrlr0, ctrl0);
    spi_write(spi, ssienr, 1);
}

static void __spi_set_timing(CSSPI_TIMING mode)
{
    spi_regs_t *spi = flash->spi;

    u16 ctrl0 = spi_read(spi, ctrlr0);

    ctrl0 &= ~(0x3 << 6);
    ctrl0 |= mode << 6;

    spi_write(spi, ssienr, 0);
    spi_write(spi, ctrlr0, ctrl0);
    spi_write(spi, ssienr, 1);
}

/* 
 * The following two interface is used mainly for convenience
 * for userspace
 */
static void __spi_set_phase(CSSPI_SCPH phase)
{
    spi_regs_t *spi = flash->spi;

    u16 ctrl0 = spi_read(spi, ctrlr0);

    ctrl0 &= ~(0x1 << 6);
    ctrl0 |= (phase << 6);

    spi_write(spi, ssienr, 0);
    spi_write(spi, ctrlr0, ctrl0);
    spi_write(spi, ssienr, 1);
}

static void __spi_set_polarity(CSSPI_SCPOL pol)
{
    spi_regs_t *spi = flash->spi;

    u16 ctrl0 = spi_read(spi, ctrlr0);

    ctrl0 &= ~(0x1 << 7);
    ctrl0 |= (pol << 7);

    spi_write(spi, ssienr, 0);
    spi_write(spi, ctrlr0, ctrl0);
    spi_write(spi, ssienr, 1);
}

static void __spi_intr_mask(u16 m)
{
    spi_regs_t *spi = flash->spi;

    spi_write(spi, imr, m);
}

static int
__proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	spi_regs_t *spi = flash->spi;

    u32 addr;
    u32 val;

    if (strncmp("rw", buffer, 2) == 0) {
        addr = simple_strtoul(&buffer[3], NULL, 16);
        val = ioread16((unsigned char *)spi + addr);
        printk(" readw [0x%04x] = 0x%08x \n", addr, val);
    } else if (strncmp("ww", buffer, 2) == 0) {
        addr = simple_strtoul(&buffer[3], NULL, 16);
        val = simple_strtoul(&buffer[7], NULL, 16);
        iowrite16((u16)val, (unsigned char *)spi + addr);
        printk(" writew [0x%04x] = 0x%08x \n", addr, val);
    }

    return count;
}

static void do_dma_tasklet(unsigned long data)
{
}

static irqreturn_t do_spi_irq(int irq, void *dev_id)
{
/*    spi_regs_t * spi = flash->spi;*/
/*    dma_regs_t * dma = flash->dma;*/

	return IRQ_HANDLED;
}

static int
cnc18xx_spi_open(struct inode *inode, struct file *file)
{
    SDEBUG("cnc18xx spi device opened!\n");

	nonseekable_open(inode, file);

/*    file->private_data = (void *)spi;*/
/*    cnc18xx_spi_t *spi = (cnc18xx_spi_t *)file->private_data;*/

    return 0;
}

static int
cnc18xx_spi_release(struct inode *inode, struct file *file)
{
    SDEBUG("Spi device closed!\n");

    return 0;
}

static ssize_t
cnc18xx_spi_read(struct file *file, char __user * buffer, size_t len, loff_t * f_pos)
{
/*    cnc18xx_spi_t *spi = (cnc18xx_spi_t *)file->private_data;*/

/*    if (len > 4096) {*/
/*        SDEBUG("%s: Read len should be less than 4096 (%d)!  \n", __func__, len);*/
/*        return -EINVAL;*/
/*    }*/

/*    mutex_lock(&spi->q_sem);*/

/*    do_dma_rx(spi->dma_buf, len);*/

/*    if (!wait_event_interruptible_timeout(spi->wqueue, spi->dma_finished, 10*HZ)) {*/
/*        SDEBUG("SPI read timeout! \n");*/
/*        return -ETIME; */
/*    }*/

/*    if (unlikely(copy_to_user(buffer, spi->dma_buf, len)))*/
/*        return -EFAULT;*/

/*    mutex_unlock(&spi->q_sem);*/

    return len;
}

static ssize_t
cnc18xx_spi_write(struct file *file, const char __user * buffer, size_t len, loff_t * f_pos)
{
/*    cnc18xx_spi_t *spi = (cnc18xx_spi_t *)file->private_data;*/

/*    if (len > 4096) {*/
/*        SDEBUG("%s: Read len should be less than 4096 (%d)!  \n", __func__, len);*/
/*        return -EINVAL;*/
/*    }*/

/*    mutex_lock(&spi->q_sem);*/

/*    if (unlikely(copy_from_user(spi->dma_buf, buffer, len)))*/
/*        return -EFAULT;*/

/*    do_dma_tx(spi->dma_buf, len);*/

/*    wait_event_interruptible(spi->wqueue, spi->dma_finished);*/

/*    mutex_unlock(&spi->q_sem);*/

    return len;
}

static loff_t
cnc18xx_spi_lseek(struct file *file, loff_t offset, int orig)
{
    down_read(&file->f_dentry->d_inode->i_alloc_sem);

    switch (orig) {
    case 0: /* SEEK_SET */
        file->f_pos = offset;
        break;
    case 1: /* SEEK_CUR */
        file->f_pos += offset;
        break;
    case 2: /* SEEK_END */
        break;
    default:
        SDEBUG("No such chioce!\n");
    }

    up_read(&file->f_dentry->d_inode->i_alloc_sem);

    return 0;
}

static unsigned int cnc18xx_spi_poll(struct file *filp, poll_table *wait)
{
    return 0;
}

static int
cnc18xx_spi_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    spi_regs_t *spi = flash->spi;
    unsigned long flags;

    switch (cmd) {
    case SPI_SET_BUS:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_bus(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_CS:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_cs(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_BAUD:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_baud(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_TMOD:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_tmod(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_POL:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_polarity(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_PH:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_phase(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_TIMING:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_timing(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_SET_NDF:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_set_ndf(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;

        /* Mask switch */
    case SPI_INTR_MASK:
        spin_lock_irqsave(&flash->slock, flags);
        __spi_intr_mask(arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;

        /* Simple interface */
    case SPI_READ_BYTE:
        spin_lock_irqsave(&flash->slock, flags);
        return spi_read(spi, dr);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;
    case SPI_WRITE_BYTE:
        spin_lock_irqsave(&flash->slock, flags);
        spi_write(spi, dr, (unsigned char)arg);
        spin_unlock_irqrestore(&flash->slock, flags);
        break;

         /* Read Secure Area */ 
    case 12:
        mutex_lock(&flash->lock);
        {
            int i;
            spi_regs_t *spi = flash->spi;
            struct {
                char *addr;
                char *buf;
                int size;
            } *msg = arg;

            spi_write(spi, ssienr, 0);
            spi_write(spi, ctrlr0, 0x0c7);
            spi_write(spi, baudr, 0xa);
            spi_write(spi, ssienr, 1);

            /* ENSA */
            spi_write(spi, dr, 0xb1);
            udelay(1000);

            /* Write read command */
            spi_write(spi, dr, 0x3);
            /* Write addr */
            spi_write(spi, dr, msg->addr[2]);
            spi_write(spi, dr, msg->addr[1]);
            spi_write(spi, dr, msg->addr[0]);

            #define DUMMY_CMD_SIZE 5
            for (i = 0; i < DUMMY_CMD_SIZE; i++) {
                spi_write(spi, dr, 0x0);
                while (!spi_read(spi, rxflr));
                spi_read(spi, dr);
            }
            i = 0;
            while (i < msg->size) {
                spi_write(spi, dr, 0x0);
                while (!spi_read(spi, rxflr));
                msg->buf[i++] = spi_read(spi, dr);
            }

            /* EXSA */
            udelay(1000);
            spi_write(spi, dr, 0xc1);
        }
        mutex_unlock(&flash->lock);

        break;
    default:
        return -EINVAL;
    }

    return 0;
}


static struct proc_dir_entry *cnc18xx_spi_proc;
static struct file_operations cnc18xx_spi_fops = {
    .owner  = THIS_MODULE,
    .open   = cnc18xx_spi_open,
    .release= cnc18xx_spi_release,
    .read   = cnc18xx_spi_read,
    .write  = cnc18xx_spi_write,
    .ioctl  = cnc18xx_spi_ioctl,
    .poll   = cnc18xx_spi_poll,
    .llseek = cnc18xx_spi_lseek,
};

static struct miscdevice cnc18xx_spi_miscdev = {
    MISC_DYNAMIC_MINOR,
    "cnc18xx_spi",
    &cnc18xx_spi_fops
};

static int spi_sys_register(struct platform_device *pdev)
{
	struct resource *r;
    int ret = 0;

	flash = kzalloc(sizeof *flash, GFP_KERNEL);
	if (!flash) {
		ret = -ENOMEM; 
        goto err0;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!request_mem_region(r->start, (r->end - r->start) + 1, 
				dev_name(&pdev->dev))) {
		ret = -EBUSY;
        goto err1;
	}
	flash->spi = ioremap(r->start, (r->end - r->start) + 1);
	flash->dma = flash->spi + DW_SPI_REG_DMA;

	flash->prd = 
		dmam_alloc_coherent(&pdev->dev, DW_PRD_TBL_SZ, &flash->prd_phy, GFP_KERNEL);
	if (!flash->prd) {
		ret = -ENOMEM;
        goto err2;
    }
	flash->dma_buf = 
		dmam_alloc_coherent(&pdev->dev, 4096, &flash->dma_buf_phy, GFP_KERNEL);
	if (!flash->dma_buf) {
		ret = -ENOMEM;
        goto err3;
    }

	mutex_init(&flash->lock);
	spin_lock_init(&flash->slock);

    /* init controller */
	controller_init(flash->spi);

    #if defined(CONFIG_SPI_CHARDEV)
    flash->chardev = 1;
    #else
        flash->chardev = 0;
    #endif

    if (flash->chardev) {
        if (misc_register(&cnc18xx_spi_miscdev)) {
            ret = -EBUSY;
            goto err4;
        }

        cnc18xx_spi_proc = create_proc_entry("spi_io", 0, NULL);
        if (!cnc18xx_spi_proc) {
            ret = -EFAULT;
            goto err5;
        } else {
            cnc18xx_spi_proc->write_proc = &__proc_write;
        }

        flash->irq = platform_get_irq(pdev, 0);
        if (flash->irq < 0) {
            ret = flash->irq;
            goto err6;
        }

#if 0
        request_irq(flash->irq, do_spi_irq, 0, dev_name(&pdev->dev), flash);
        tasklet_init(&flash->tasklet, do_dma_tasklet, (unsigned long)flash);
        init_waitqueue_head(&flash->queue);

        /* Tell probe to skip mtd branch */
        ret = 1;
#endif
    }

    goto out;
    goto err7;

err7:
    tasklet_kill(&flash->tasklet);
	free_irq(flash->irq, flash);
err6:
    remove_proc_entry("spi_io", NULL);
err5:
    misc_deregister(&cnc18xx_spi_miscdev);
err4:
    dmam_free_coherent(&pdev->dev, 4096, &flash->dma_buf_phy, GFP_KERNEL);
err3:
    dmam_free_coherent(&pdev->dev, DW_PRD_TBL_SZ, &flash->prd_phy, GFP_KERNEL);
err2:
	iounmap((void *)flash->spi);
	release_mem_region(r->start, (r->end - r->start) + 1);
err1:
    kfree(flash); flash = NULL;

err0:
out:
    return ret;
}

static void spi_sys_unregister(struct platform_device *pdev)
{
	struct resource *r;

    if (flash->chardev) {
        tasklet_kill(&flash->tasklet);
        free_irq(flash->irq, flash);
        remove_proc_entry("spi_io", NULL);
        misc_deregister(&cnc18xx_spi_miscdev);
    }

    dmam_free_coherent(&pdev->dev, 4096, &flash->dma_buf_phy, GFP_KERNEL);
    dmam_free_coherent(&pdev->dev, DW_PRD_TBL_SZ, &flash->prd_phy, GFP_KERNEL);

	iounmap((void *)flash->spi);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, (r->end - r->start) + 1);

    kfree(flash); flash = NULL;
}
