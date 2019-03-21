/*
 *  sata.c - Celestial SATA for CSM18xx series
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
#include <mach/hardware.h>
#include <mach/irqs.h>


static u64 sata_dmamask = DMA_BIT_MASK(32);

static struct resource csm18xx_sata_resources[] = {
	[0] = {
        .name   = "sata base",
		.start	= PA_SATA_BASE,
		.end	= PA_SATA_BASE + PA_SATA_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
        .name   = "sata irq",
		.start	= IRQ_SATA,
		.end	= IRQ_SATA,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device csm18xx_sata_device = {
	.name = "csm1800h-sata",
	.id = 0,
	.dev = {
		.dma_mask = &sata_dmamask,
	    .coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(csm18xx_sata_resources),
	.resource = csm18xx_sata_resources,
};

// static struct platform_device *csm18xx_sata_devices[] = {
// 	&csm18xx_sata_device,
// };

int __init csm18xx_platform_register_sata(void)
{
	return platform_device_register(&csm18xx_sata_device);
    
}
