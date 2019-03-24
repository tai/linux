/*
 * 
 * pcmcia devices for celestial platform
 *
 * Author: SunHe <he.sun@celestialsemi.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <mach/irqs.h>

static struct resource cnc18xx_pcmcia_resources[] = {
	[0] = {
		.start	= PA_PCMCIA_BASE,
		.end	= PA_PCMCIA_BASE + PCMCIA_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
        .name   = "pcmcia irq",
		.start	= IRQ_PCMCIA,
		.end	= IRQ_PCMCIA,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
        .name   = "pcmcia irq_io",
		.start	= IRQ_PCMCIA_IO,
		.end	= IRQ_PCMCIA_IO,
		.flags	= IORESOURCE_IRQ,
	},
	[3] = {
        .name   = "pcmcia irq_req",
		.start	= IRQ_PCMCIA_REQUEST,
		.end	= IRQ_PCMCIA_REQUEST,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cnc18xx_pcmcia = {
	.name		= "cnc18xx_pcmcia",
	.id			= 0,
	.num_resources	= ARRAY_SIZE(cnc18xx_pcmcia_resources),
	.resource	= cnc18xx_pcmcia_resources,
};

void __init cnc18xx_platform_register_pcmcia(void) {
	platform_device_register(&cnc18xx_pcmcia);
}

