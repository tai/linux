/*
 *  drivers/mtd/nand/cnc_nand.c
 *
 *  Copyrigth (C) 2010 Celestial Semiconductor
 *                2011 Cavium
 *  Author: xiaodong fan <xiaodong.fan@caviumnetworks.com>
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   Celestial CNC18xx SOC. 
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/platform_device.h>
#include <linux/err.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/nand.h>

//#define CONFIG_CELESTIAL_NAND_DEBUG

#ifdef CONFIG_CELESTIAL_NAND_DEBUG
#define CNC_DEBUG(args...)				\
 	do {						\
        printk(KERN_INFO args); \
	} while(0)
#else /* CONFIG_NAND_DEBUG */
#define CNC_DEBUG(args...) do { } while(0)

#endif /* CONFIG_NAND_DEBUG */


struct cnc_nand_info {
	struct mtd_info		mtd;
	struct nand_chip	chip;
	struct nand_ecclayout	ecclayout;

	struct device	*dev;
	struct clk		*clk;
	bool			partitioned;

	bool			is_readmode;

	void __iomem	*base;
	void __iomem	*vaddr;
    void __iomem    *paddr;

	uint32_t		ioaddr;
	uint32_t		current_cs;
    uint32_t        current_chip;
	uint32_t		maxchip;
	uint32_t		mask_ale;
	uint32_t		mask_cle;

	uint32_t		core_chipsel;
};

//static DEFINE_SPINLOCK(cnc_nand_lock);

#define to_cnc_nand(m) container_of(m, struct cnc_nand_info, mtd)

static inline unsigned int cnc_nand_readl(struct cnc_nand_info *info,
		int offset)
{
	return __raw_readl(info->base + offset);
}

static inline void cnc_nand_writel(struct cnc_nand_info *info,
		int offset, unsigned long value)
{
	__raw_writel(value, info->base + offset);
}


/*
 * 1-bit hardware ECC ... context maintained for each core chipselect
 */


static void cnc_nand_1bitecc_hwctl(struct mtd_info *mtd, int mode)
{

	struct nand_chip *chip = mtd->priv;
	struct cnc_nand_info *info = to_cnc_nand(mtd);
    unsigned long ecc_addr;


    if (mode == NAND_ECC_READ){
        ecc_addr = (unsigned long)((u32)info->paddr + ((u32)chip->IO_ADDR_R - (u32)info->ioaddr));
        cnc_nand_writel(info, ECC_A_OFFSET, ecc_addr);
        CNC_DEBUG("HW Read ECC enable--0x%x\n",(u32)ecc_addr);
    }
    else if (mode == NAND_ECC_WRITE) {
        ecc_addr = (unsigned long)((u32)info->paddr + ((u32)chip->IO_ADDR_W - (u32)info->ioaddr));
        cnc_nand_writel(info, ECC_A_OFFSET, ecc_addr);
        CNC_DEBUG("HW Write ECC enable--0x%x\n",(u32)ecc_addr);
    }
}

/*
 * Read hardware ECC value and pack into three bytes
 */
static int cnc_nand_1bitecc_calculate(struct mtd_info *mtd,
				      const u_char *dat, u_char *ecc_code)
{
	struct cnc_nand_info *info = to_cnc_nand(mtd);
	unsigned int ecc24 = cnc_nand_readl(info, ECC_CODE_OFFSET);

	ecc_code[0] = (u_char)(ecc24);
	ecc_code[1] = (u_char)(ecc24 >> 8);
	ecc_code[2] = (u_char)(ecc24 >> 16);

	return 0;
}

static int cnc_nand_1bitecc_correct(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
    //	struct nand_chip *chip = mtd->priv;
	uint32_t eccNand = read_ecc[0] | (read_ecc[1] << 8) |
					  (read_ecc[2] << 16);
	uint32_t eccCalc = calc_ecc[0] | (calc_ecc[1] << 8) |
					  (calc_ecc[2] << 16);
	uint32_t diff = eccCalc ^ eccNand;
	unsigned int bit, byte;

    
    CNC_DEBUG("eccNand=0x%x, eccCalc=0x%x, diff=0x%x\n", eccNand, eccCalc, diff);

	if (diff) {
		if ((((diff >> 1) ^ diff) & 0x555555) == 0x555555) {
			/* Correctable error */
            
            /* calculate the bit position of the error */
            bit  = ((diff >> 19) & 1) |
                ((diff >> 20) & 2) |
                ((diff >> 21) & 4);
            /* calculate the byte position of the error */

            byte = ((diff >> 9) & 0x100) |
                (diff  & 0x80)  |
                ((diff << 1) & 0x40)  |
                ((diff << 2) & 0x20)  |
                ((diff << 3) & 0x10)  |
                ((diff >> 12) & 0x08)  |
                ((diff >> 11) & 0x04)  |
                ((diff >> 10) & 0x02)  |
                ((diff >> 9) & 0x01);
            
            CNC_DEBUG("byte=%d, bit=%d, dat[byte]=0x%2x\n",byte,bit,dat[byte]);

            dat[byte] ^= (1 << bit);


            CNC_DEBUG("corrected dat[byte]=0x%x\n",dat[byte]);        
			
            return 1;
        } else if (!(diff & (diff - 1))) {
            /* Single bit ECC error in the ECC itself,
             * nothing to fix */
            return 1;
        } else {
            /* Uncorrectable error */
            return -1;
        }
    }
    return 0;
}



static int cnc_nand_device_ready(struct mtd_info *mtd)
{
	struct cnc_nand_info *info = to_cnc_nand(mtd);
	
    return cnc_nand_readl(info, STATUS_OFFSET) & BIT(0);
}


/*
 * Minimal-overhead PIO for data access.
 */
static void cnc_nand_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
	struct nand_chip	*nand_chip = mtd->priv;

	__raw_readsb(nand_chip->IO_ADDR_R, buf, len);
}

static void cnc_nand_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	struct nand_chip	*nand_chip = mtd->priv;

	__raw_writesb(nand_chip->IO_ADDR_W, buf, len);
}


/*
 * Hardware specific access to control-lines
 */
static void cnc_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{

	struct cnc_nand_info	*info = to_cnc_nand(mtd);
	uint32_t			    addr = info->current_cs;
	struct nand_chip		*nand = mtd->priv;


	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE){
            (* (unsigned int *) ((int)info->base + (int)(info->current_chip * 0xC))) |= MASK_CE_FORCE_LOW;
        }
		else {
            (* (unsigned int *) ((int)(info->base) + (int)(info->current_chip * 0xC))) &= ~(MASK_CE_FORCE_LOW);
        }
        
        if (ctrl & NAND_CLE){
            addr |= info->mask_cle;
        }
        else if (ctrl & NAND_ALE) {
            addr |= info->mask_ale;
        }
		nand->IO_ADDR_W = (void __iomem __force *)addr;
	}


	if (cmd != NAND_CMD_NONE)
		iowrite8(cmd, nand->IO_ADDR_W);

}


static void cnc_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct cnc_nand_info	*info = to_cnc_nand(mtd);
	uint32_t			addr = info->ioaddr;


    if (chip > 0 && chip < info->maxchip) {
        addr += chip * BIT(26);

    } else if (chip == -1 && info->current_chip!= -1) {
        info->chip.cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
    }

    info->current_cs = addr;
    info->current_chip = chip;
    
    info->chip.IO_ADDR_W = (void __iomem __force *)addr;
    info->chip.IO_ADDR_R = info->chip.IO_ADDR_W;
}


static int __init cnc_nand_probe(struct platform_device *pdev)
{
	struct cnc_nand_pdata	*pdata = pdev->dev.platform_data;
	struct cnc_nand_info	*info;
	struct resource			*res1;
	struct resource			*res2;
	void __iomem			*vaddr;
	void __iomem			*base;
	int				        ret;

	nand_ecc_modes_t		ecc_mode;

	/* insist on board-specific configuration */
	if (!pdata)
		return -ENODEV;
    
	/* which external chipselect will we be managing? */
	if (pdev->id < 0 || pdev->id > 3)
		return -ENODEV;
    
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "unable to allocate memory\n");
		ret = -ENOMEM;
		goto err_nomem;
	}

	platform_set_drvdata(pdev, info);

	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res2 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res1 || !res2) {
		dev_err(&pdev->dev, "resource missing\n");
		ret = -EINVAL;
		goto err_nomem;
	}

	vaddr = ioremap(res1->start, res1->end - res1->start);
	base = ioremap(res2->start, res2->end - res2->start);
	if (!vaddr || !base) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -EINVAL;
		goto err_ioremap;
	}

	info->dev		= &pdev->dev;
	info->base		= base;
	info->vaddr		= vaddr;
    info->paddr     = (void *)res1->start;

	info->mtd.priv		= &info->chip;
	info->mtd.name		= dev_name(&pdev->dev);
	info->mtd.owner		= THIS_MODULE;

	info->mtd.dev.parent	= &pdev->dev;

	info->chip.IO_ADDR_R	= vaddr;
	info->chip.IO_ADDR_W	= vaddr;
	info->chip.chip_delay	= 0;
	info->chip.select_chip	= cnc_nand_select_chip;

	/* options such as NAND_USE_FLASH_BBT or 16-bit widths */
	info->chip.options	= pdata->options;

	info->ioaddr		= (uint32_t __force) vaddr;

	info->current_cs	= info->ioaddr;
	info->core_chipsel	= pdev->id;
	info->maxchip   	= pdata->maxchip;

	/* use nandboot-capable ALE/CLE masks by default */
	info->mask_ale		= pdata->mask_cle ? : MASK_ALE;
	info->mask_cle		= pdata->mask_cle ? : MASK_CLE;

	/* Set address of hardware control function */
	info->chip.cmd_ctrl	= cnc_nand_cmd_ctrl;
	info->chip.dev_ready	= cnc_nand_device_ready;

	/* Speed up buffer I/O */
	info->chip.read_buf     = cnc_nand_read_buf;
	info->chip.write_buf    = cnc_nand_write_buf;

	/* Use board-specific ECC config */
	ecc_mode		= pdata->ecc_mode;

	ret = -EINVAL;
	switch (ecc_mode) {
	case NAND_ECC_NONE:
	case NAND_ECC_SOFT:

        printk("Enable SOFT ECC for CNC18xx!\n");
		break;
	case NAND_ECC_HW:
        info->chip.ecc.calculate = cnc_nand_1bitecc_calculate;
        info->chip.ecc.correct = cnc_nand_1bitecc_correct;
        info->chip.ecc.hwctl = cnc_nand_1bitecc_hwctl;
        info->chip.ecc.bytes = 3;
		info->chip.ecc.size = 512;
        printk("Enable HW ECC for CNC18xx!\n");
		break;
	default:
		ret = -EINVAL;
		goto err_ecc;
	}
	info->chip.ecc.mode = ecc_mode;


    ret = nand_scan(&info->mtd, pdata->maxchip);
	if (ret < 0) {
		dev_dbg(&pdev->dev, "no NAND chip(s) found\n");
		goto err_scan;
	}


	if (mtd_has_partitions()) {
		struct mtd_partition	*mtd_parts = NULL;
		int			mtd_parts_nb = 0;

		if (mtd_has_cmdlinepart()) {
			static const char *probes[] __initconst =
				{ "cmdlinepart", NULL };

			mtd_parts_nb = parse_mtd_partitions(&info->mtd, probes,
							    &mtd_parts, 0);
		}

		if (mtd_parts_nb <= 0) {
			mtd_parts = pdata->parts;
			mtd_parts_nb = pdata->nr_parts;
		}

		/* Register any partitions */
		if (mtd_parts_nb > 0) {
			ret = add_mtd_partitions(&info->mtd,
					mtd_parts, mtd_parts_nb);
			if (ret == 0)
				info->partitioned = true;
		}

	} else if (pdata->nr_parts) {
		dev_warn(&pdev->dev, "ignoring %d default partitions on %s\n",
				pdata->nr_parts, info->mtd.name);
	}

	/* If there's no partition info, just package the whole chip
	 * as a single MTD device.
	 */
	if (!info->partitioned)
		ret = add_mtd_device(&info->mtd) ? -ENODEV : 0;

	if (ret < 0)
		goto err_scan;

	return 0;

err_scan:
err_ecc:
err_ioremap:
	if (base)
		iounmap(base);
	if (vaddr)
		iounmap(vaddr);

err_nomem:
	kfree(info);
	return ret;
}


static int __exit cnc_nand_remove(struct platform_device *pdev)
{
	struct cnc_nand_info *info = platform_get_drvdata(pdev);
	int status;

	if (mtd_has_partitions() && info->partitioned)
		status = del_mtd_partitions(&info->mtd);
	else
		status = del_mtd_device(&info->mtd);

	iounmap(info->base);
	iounmap(info->vaddr);

	nand_release(&info->mtd);


	kfree(info);

	return 0;
}

static struct platform_driver cnc_nand_driver = {
	.remove		= __exit_p(cnc_nand_remove),
	.driver		= {
		.name	= "cnc_nand",
		.owner	= THIS_MODULE,
	},
};

static int __init cnc_nand_init(void)
{
	return platform_driver_probe(&cnc_nand_driver, cnc_nand_probe);
}

static void __exit cnc_nand_exit(void)
{
	platform_driver_unregister(&cnc_nand_driver);
}

module_init(cnc_nand_init);
module_exit(cnc_nand_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("xiaodong fan <xiaodong.fan@caviumnetworks.com>");
MODULE_DESCRIPTION("NAND flash driver for Cavium celestial CNC18xx serials SOC");
MODULE_ALIAS("platform:cnc_nand");


