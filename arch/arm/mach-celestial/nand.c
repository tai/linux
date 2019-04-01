/*
 * 
 * Nand devices definition for celestial platform
 *
 * Author: xiaodong fan, 2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/nand.h>

#define NAND_DEFAULT_BLOCK_SIZE		SZ_128K

static struct mtd_partition cnc_nand_partitions[] = {
	{
		.name		= "miniloader",
		.offset		= 0,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "u-boot1",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_512K,
		.mask_flags	= 0,/*Read only*/
	}, {
		.name		= "u-boot2",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_512K,
		.mask_flags	= 0,
	}, {
		.name		= "nvram_factory",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= 0,/*Read only*/
	}, {
		.name		= "marlin",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "factory_hdr",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= 0,
	}, {
		.name		= "factory_bak_hdr",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= 0,
	}, {
		.name		= "kernel_hdr",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_256K,
		.mask_flags	= 0,
	}, {
		.name		= "splash",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_2M,
		.mask_flags	= 0,
	}, {
		.name		= "factory",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_32M,
		.mask_flags	= 0,
	}, {
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_32M,
		.mask_flags	= 0,
	}, {
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_48M + SZ_8M + SZ_512K,
		.mask_flags	= 0,
	}, {
		.name		= "blob",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_2M + SZ_1M,
		.mask_flags	= MTD_WRITEABLE,
	}, {
		.name		= "reserve",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}, {
		.name		= "full",
		.offset		= 0,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= MTD_WRITEABLE,
	}

};

static struct cnc_nand_pdata cnc_nand_data = {
	.maxchip		= 1,
	.parts			= cnc_nand_partitions,
	.nr_parts		= ARRAY_SIZE(cnc_nand_partitions),
	.ecc_mode		= NAND_ECC_HW,
	.options		= NAND_USE_FLASH_BBT | NAND_BBT_LASTBLOCK | NAND_BBT_WRITE, 
};

static struct resource cnc_nand_resources[] = {
	{
		.start		= PA_CS0_BASE,
		.end		= PA_CS0_BASE + CS0_SIZE -1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= PA_SMC_IO_BASE,
		.end		= PA_SMC_IO_BASE + SMC_IO_SIZE,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device cnc_nand_device = {
	.name			= "cnc_nand",
	.id			= 0,
	.num_resources		= ARRAY_SIZE(cnc_nand_resources),
	.resource		= cnc_nand_resources,
	.dev			= {
		.platform_data	= &cnc_nand_data,
	},
};

int platform_register_nand(void)
{
    platform_device_register(&cnc_nand_device);
    return 0;
}

