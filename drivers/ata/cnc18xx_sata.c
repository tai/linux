/*
 *  sata_csm.c - Celestial SATA for CSM18xx series
 *
 *  Copyright 2010 Celestial Semiconductor.  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *  libata documentation is available via 'make {ps|pdf}docs',
 *  as Documentation/DocBook/libata.*
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ata_platform.h>
#include <linux/bitops.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <linux/libata.h>

#define DRV_NAME            "cnc1800h-sata"
#define DRV_VERSION         "0.8"

enum {
    CNC_SATA_REG_DATA     = 0x00,
    CNC_SATA_REG_ERR      = 0x04,
    CNC_SATA_REG_NSECT    = 0x08,
    CNC_SATA_REG_LBAL     = 0x0C,
    CNC_SATA_REG_LBAM     = 0x10,
    CNC_SATA_REG_LBAH     = 0x14,
    CNC_SATA_REG_DEVICE   = 0x18,
    CNC_SATA_REG_STATUS   = 0x1C,
    CNC_SATA_REG_ASTATUS  = 0x20,
    CNC_SATA_REG_SCR      = 0x24,
    CNC_SATA_DMACR        = 0x70,
    CNC_SATA_DBTSR        = 0x74,
    CNC_SATA_REG_DMA      = 0x4000,

    CNC_SATA_REG_FEATURE  = CNC_SATA_REG_ERR,                /* and their aliases */
    CNC_SATA_REG_CMD      = CNC_SATA_REG_STATUS,
    CNC_SATA_REG_BYTEL    = CNC_SATA_REG_LBAM,
    CNC_SATA_REG_BYTEH    = CNC_SATA_REG_LBAH,
    CNC_SATA_REG_DEVSEL   = CNC_SATA_REG_DEVICE,
    CNC_SATA_REG_IRQ      = CNC_SATA_REG_NSECT,
    CNC_SATA_REG_CTL      = CNC_SATA_REG_ASTATUS,

    SAR0                  = 0x000,
    DAR0                  = 0x008,
    LLP0                  = 0x010,
    CTL0                  = 0x018,
    SSTAT0                = 0x020,
    DSTAT0                = 0x028,
    SSTATAR0              = 0x030,
    DSTATAR0              = 0x038,
    CFG0                  = 0x040,
    SGR0                  = 0x048,
    DSR0                  = 0x050,
    STATUS_TFR            = 0x2e8,
    STATUS_ERR            = 0x308,
    MASK_TFR              = 0x310,
    MASK_ERR              = 0x330,
    CLEAR_TFR             = 0x338,
    CLEAR_ERR             = 0x358,
    CHEN                  = 0x3a0,
    DMACFG                = 0x398,
    CNC_SATA_DMA_FIFO     = 0x10220400,
    CNC_SATA_DMA_BOUNDARY = 0x1fffUL,

    CNC_SATA_REG_PHYCR    = 0x88,
    CNC_SATA_VAL_PHYCR    = 0x810,

    CNC_SATA_PRD_SZ       = 20,
    CNC_SATA_PRD_TBL_SZ   = (ATA_MAX_PRD * CNC_SATA_PRD_SZ),
};


static void cnc_sata_dma_setup(struct ata_queued_cmd *qc);
static void cnc_sata_dma_start(struct ata_queued_cmd *qc);
static void cnc_sata_dma_stop(struct ata_queued_cmd *qc);
static u8   cnc_sata_dma_status(struct ata_port *ap);
static int cnc_check_atapi_dma(struct ata_queued_cmd *qc);

static void cnc_sata_qc_prep(struct ata_queued_cmd *qc);
static int cnc_dma_port_start(struct ata_port *ap);
static void cnc_dma_irq_clear(struct ata_port *ap);

static int cnc_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
static int cnc_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);

static int cnc18xx_sata_probe(struct platform_device *pdev);
static int __devexit cnc18xx_sata_remove(struct platform_device *pdev);

static struct platform_driver cnc18xx_sata_driver = {
    .probe     = cnc18xx_sata_probe,
    .remove    = __devexit_p(cnc18xx_sata_remove),
    .driver    = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
    },
};

static struct scsi_host_template cnc_sht = {
    ATA_BASE_SHT(DRV_NAME),
    .sg_tablesize           = ATA_MAX_PRD,
    .dma_boundary           = CNC_SATA_DMA_BOUNDARY
};

/* CNC18xx SATA basically is SFF PIO + Synopsys DesignWare DMA */
static struct ata_port_operations cnc18xx_sata_ops = {
    .inherits        = &ata_bmdma_port_ops,
    .scr_read        = cnc_scr_read,
    .scr_write       = cnc_scr_write,

    // BMDMA emulation
    .port_start      = cnc_dma_port_start,
    .sff_irq_clear   = cnc_dma_irq_clear,
    .qc_prep         = cnc_sata_qc_prep,

    .bmdma_setup     = cnc_sata_dma_setup,
    .bmdma_start     = cnc_sata_dma_start,
    .bmdma_stop      = cnc_sata_dma_stop,
    .bmdma_status    = cnc_sata_dma_status,

    .check_atapi_dma = cnc_check_atapi_dma,
};



static const struct ata_port_info cnc_port_info[] = {
    {
        .flags      = ATA_FLAG_SATA | ATA_FLAG_NO_LEGACY,
        .pio_mask   = ATA_PIO4,
        .mwdma_mask = ATA_MWDMA2,
        .udma_mask  = ATA_UDMA6,
        .port_ops   = &cnc18xx_sata_ops,
    },
};

struct dw_prd {
    __le32      sar;
    __le32      dar;
    __le32      llp;
    __le32      ctl_lo;
    __le32      ctl_hi;
};


/* DesignWare DMA */

static void cnc_dma_irq_clear(struct ata_port *ap)
{
    void __iomem *mmio = ap->ioaddr.bmdma_addr;

    if (!mmio)
        return;

    iowrite8(0x1, mmio + CLEAR_TFR);
    iowrite8(0x1, mmio + CLEAR_ERR);
}

static int cnc_dma_port_start(struct ata_port *ap)
{
    struct device *dev = ap->dev;

    ap->prd = dmam_alloc_coherent(dev, CNC_SATA_PRD_TBL_SZ, &ap->prd_dma,
                                  GFP_KERNEL);

    if (!ap->prd)
        return -ENOMEM;

    if (ap->prd_dma & 0x3)
        printk(KERN_ERR "Unsupport prd_dma: 0x%08x\n", ap->prd_dma);

    return 0;
}

static void cnc_sata_fill_sg(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;
    struct scatterlist *sg;
    struct dw_prd *prd = (struct dw_prd *)ap->prd;
    unsigned int si, pi, next_llp, tbl_sz;
    unsigned int wr = (qc->tf.flags & ATA_TFLAG_WRITE);

    const unsigned int ctl_lo_tx =  0x0    /* CTL[31:0]        */
                                    | (0x1  << 28)  /* LLP_SRC_EN       */
                                    | (0x1  << 27)  /* LLP_DST_EN       */
                                    | (0x1  << 25)  /* SRC Master       */
                                    | (0x0  << 23)  /* DST Master       */
                                    | (0x1  << 20)  /* M -> P, M as flow controller */
                                    | (0x0  << 18)  /* DSCATTER_EN      */
                                    | (0x0  << 17)  /* SGATHER_EN       */
                                    | (0x3  << 14)  /* SMSIZE: 16 * 4B  */
                                    | (0x3  << 11)  /* DMSIZE: 16 * 4B  */
                                    | (0x0  <<  9)  /* SINC: Increment  */
                                    | (0x2  <<  7)  /* DINC: no change  */
                                    | (0x2  <<  4)  /* SRC_TR_WIDTH: 32b*/
                                    | (0x2  <<  1)  /* DST_TR_WIDTH: 32b*/
                                    | (0x0  <<  0); /* INT_EN           */

    const unsigned int ctl_lo_rx =  0x0    /* CTL[31:0]        */
                                    | (0x1  << 28)  /* LLP_SRC_EN       */
                                    | (0x1  << 27)  /* LLP_DST_EN       */
                                    | (0x0  << 25)  /* SRC Master       */
                                    | (0x1  << 23)  /* DST Master       */
                                    | (0x2  << 20)  /* M -> P, M as flow controller */
                                    | (0x0  << 18)  /* DSCATTER_EN      */
                                    | (0x0  << 17)  /* SGATHER_EN       */
                                    | (0x3  << 14)  /* SMSIZE: 16 * 4B  */
                                    | (0x3  << 11)  /* DMSIZE: 16 * 4B  */
                                    | (0x2  <<  9)  /* SINC: no change  */
                                    | (0x0  <<  7)  /* DINC: increment  */
                                    | (0x2  <<  4)  /* SRC_TR_WIDTH: 32b*/
                                    | (0x2  <<  1)  /* DST_TR_WIDTH: 32b*/
                                    | (0x0  <<  0); /* INT_EN           */
    pi = 0;
    tbl_sz = 0;
    next_llp = (ap->prd_dma  + CNC_SATA_PRD_SZ) | 0x1;
    for_each_sg(qc->sg, sg, qc->n_elem, si) {
        u32 addr;
        u32 sg_len;

        addr = (u32) sg_dma_address(sg);
        sg_len = sg_dma_len(sg);

        if(sg_len >= 0x4000 || (sg_len & 0x3))
            printk(KERN_ERR "Unsupport length: %d\n", sg_len);

        if(wr) {
            prd[pi].sar = cpu_to_le32(addr);
            prd[pi].dar = cpu_to_le32(CNC_SATA_DMA_FIFO);
            prd[pi].ctl_lo = cpu_to_le32(ctl_lo_tx);
        } else {
            prd[pi].sar = cpu_to_le32(CNC_SATA_DMA_FIFO);
            prd[pi].dar = cpu_to_le32(addr);
            prd[pi].ctl_lo = cpu_to_le32(ctl_lo_rx);
        }
        prd[pi].llp = cpu_to_le32(next_llp);
        prd[pi].ctl_hi = cpu_to_le32((sg_len >> 2) & 0xfff); 

        pi++;
        next_llp += CNC_SATA_PRD_SZ;
        tbl_sz += CNC_SATA_PRD_SZ;
    }

    prd[pi - 1].llp = 0x0;
    prd[pi - 1].ctl_lo = cpu_to_le32((wr ? ctl_lo_tx : ctl_lo_rx) | 0x1);
}

static void cnc_sata_qc_prep(struct ata_queued_cmd *qc)
{
    if (!(qc->flags & ATA_QCFLAG_DMAMAP))
        return;

    cnc_sata_fill_sg(qc);
}

static void cnc_sata_dma_setup(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;
    unsigned int wr = (qc->tf.flags & ATA_TFLAG_WRITE);

    if (wr) {
        iowrite32(0x3 << 27, ap->ioaddr.bmdma_addr + CTL0);
        iowrite32(0x0 << 11, ap->ioaddr.bmdma_addr + CFG0 + 4);
        iowrite32(0x2 << 10, ap->ioaddr.bmdma_addr + CFG0);
        iowrite32(ap->prd_dma | 0x1, ap->ioaddr.bmdma_addr + LLP0);
        iowrite32(0x5, ap->ioaddr.cmd_addr + CNC_SATA_DMACR);
    } else {
        iowrite32(0x3 << 27, ap->ioaddr.bmdma_addr + CTL0);
        iowrite32(0x1 <<  7, ap->ioaddr.bmdma_addr + CFG0 + 4);
        iowrite32(0x1 << 10, ap->ioaddr.bmdma_addr + CFG0);
        iowrite32(ap->prd_dma | 0x1, ap->ioaddr.bmdma_addr + LLP0);
        iowrite32(0x2, ap->ioaddr.cmd_addr + CNC_SATA_DMACR);
    }

    /* Disable IPF interrupt in DMA mode */
    ata_qc_set_polling(qc);

    /* issue r/w command */
    ap->ops->sff_exec_command(ap, &qc->tf);
}

static void cnc_sata_dma_start(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;

    iowrite32(0x101, ap->ioaddr.bmdma_addr + CHEN);
}

static void cnc_sata_dma_stop(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;

    iowrite32(0x0, ap->ioaddr.cmd_addr + CNC_SATA_DMACR);
    if (qc->tf.flags & ATA_TFLAG_WRITE)
        ata_wait_idle(ap);
}

static u8 cnc_sata_dma_status(struct ata_port *ap)
{
    unsigned int dma_active = ioread32(ap->ioaddr.bmdma_addr + CHEN) & 0x1;
    unsigned int dma_err    = ioread32(ap->ioaddr.bmdma_addr + STATUS_ERR) & 0x1;
    unsigned int dma_tfr    = ioread32(ap->ioaddr.bmdma_addr + STATUS_TFR) & 0x1;
    u8 status = (u8)(dma_active | (dma_err << 1) | ((dma_err | dma_tfr) << 2));

    return status;
}

static int cnc_check_atapi_dma(struct ata_queued_cmd *qc)
{

    if (atapi_cmd_type(qc->cdb[0]) == ATAPI_MISC)
        return -EOPNOTSUPP; 

    return 0;
}

/* Standard SFF */

static int cnc_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val)
{

    if (sc_reg > SCR_NOTIFICATION)
        return -EINVAL;

    *val = ioread32(link->ap->ioaddr.scr_addr + (sc_reg * 4));
    return 0;
}

static int cnc_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val)
{
    if (sc_reg > SCR_NOTIFICATION)
        return -EINVAL;

    iowrite32(val, link->ap->ioaddr.scr_addr + (sc_reg * 4));
    return 0;
}

/* init and exit */

static int cnc18xx_sata_probe(struct platform_device *pdev)
{
    static int printed_version=0;
    struct resource *res;
    const struct ata_port_info *ppi[] = { &cnc_port_info[0], NULL };
    struct ata_host *host;
    void __iomem * iomap;
    struct ata_ioports *ioaddr;

    if (!printed_version++)
        dev_printk(KERN_INFO, &pdev->dev, "version " DRV_VERSION "\n");

    /*
     * Simple resource validation ..
     */
    if (unlikely(pdev->num_resources != 2)) {
        dev_err(&pdev->dev, "invalid number of resources\n");
        return -EINVAL;
    }

    /*
     * Get the register base first
     */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL)
        return -EINVAL;

    /* allocate the host */
    host = ata_host_alloc_pinfo(&pdev->dev, ppi, 1);
    if (!host) {
        printk(KERN_INFO "cnc_init: ata_host_alloc_pinfo failed.\n");
        return -ENOMEM;
    }

    iomap = devm_ioremap(&pdev->dev, res->start, res->end - res->start + 1);

    ioaddr = &host->ports[0]->ioaddr;
    ioaddr->cmd_addr        = iomap                     ;
    ioaddr->data_addr       = ioaddr->cmd_addr +  CNC_SATA_REG_DATA     ;
    ioaddr->error_addr      = ioaddr->cmd_addr +  CNC_SATA_REG_ERR      ;
    ioaddr->feature_addr    = ioaddr->cmd_addr +  CNC_SATA_REG_FEATURE  ;
    ioaddr->nsect_addr      = ioaddr->cmd_addr +  CNC_SATA_REG_NSECT    ;
    ioaddr->lbal_addr       = ioaddr->cmd_addr +  CNC_SATA_REG_LBAL     ;
    ioaddr->lbam_addr       = ioaddr->cmd_addr +  CNC_SATA_REG_LBAM     ;
    ioaddr->lbah_addr       = ioaddr->cmd_addr +  CNC_SATA_REG_LBAH     ;
    ioaddr->device_addr     = ioaddr->cmd_addr +  CNC_SATA_REG_DEVICE   ;
    ioaddr->status_addr     = ioaddr->cmd_addr +  CNC_SATA_REG_STATUS   ;
    ioaddr->command_addr    = ioaddr->cmd_addr +  CNC_SATA_REG_CMD      ;
    ioaddr->altstatus_addr  = ioaddr->cmd_addr +  CNC_SATA_REG_ASTATUS  ;
    ioaddr->ctl_addr        = ioaddr->cmd_addr +  CNC_SATA_REG_CTL      ;
    ioaddr->bmdma_addr      = ioaddr->cmd_addr +  CNC_SATA_REG_DMA      ;
    ioaddr->scr_addr    = ioaddr->cmd_addr +  CNC_SATA_REG_SCR      ;

    /* Setup up SATA PHY correctly, maybe it should in BootROM/bootloader */
    iowrite32(CNC_SATA_VAL_PHYCR,  ioaddr->cmd_addr + CNC_SATA_REG_PHYCR);
    iowrite32(0x00100010, ioaddr->cmd_addr + CNC_SATA_DBTSR);
    iowrite32(0x1, ioaddr->bmdma_addr + DMACFG);
    iowrite32(0x101, ioaddr->bmdma_addr + MASK_TFR);
    iowrite32(0x101, ioaddr->bmdma_addr + MASK_ERR);

    return ata_host_activate(host, platform_get_irq(pdev, 0), ata_sff_interrupt,
                             IRQF_SHARED, &cnc_sht);
}

static int __devexit cnc18xx_sata_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct ata_host *host = dev_get_drvdata(dev);

    ata_host_detach(host);
    return 0;
}

static int __init cnc_sata_init(void)
{

    return platform_driver_register(&cnc18xx_sata_driver);
}

static void __exit cnc_sata_exit(void)
{
    platform_driver_unregister(&cnc18xx_sata_driver);
}

MODULE_AUTHOR("Celestial");
MODULE_DESCRIPTION("low-level driver for Celestial's CNC1800H");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

module_init(cnc_sata_init);
module_exit(cnc_sata_exit);
