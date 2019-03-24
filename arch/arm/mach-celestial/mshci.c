/*
 * mshci devices for celestial platform
 * Scott Shu <sshu@caviumnetworks.com>
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

#include <linux/mmc/host.h>
#include <mach/hardware.h>
#include <mach/mshci.h>
#include <mach/irqs.h>

static struct cnc1800l_mshci_platdata cnc1800l_mshci_pdata __initdata = {
	.cd_type	= MSHCI_CD_PERMANENT,
};

static struct resource cnc1800l_mshci_resources[] = {
	[0] = {
		.start	= PA_MSHCI_BASE,
		.end	= PA_MSHCI_BASE + MSHCI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MSHCI,
		.end	= IRQ_MSHCI,
		.flags	= IORESOURCE_IRQ,
	},
};

struct cnc1800l_mshci_platdata cnc1800l_mshci_platform_data = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED |
			   MMC_CAP_SD_HIGHSPEED |
			   MMC_CAP_SDIO_IRQ),
};

static u64 cnc1800l_mshci_dma_mask = 0xffffffffUL;
static struct platform_device cnc1800l_mshci = {
	.name		= "cnc1800l-mshci",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(cnc1800l_mshci_resources),
	.resource	= cnc1800l_mshci_resources,
	.dev		= {
		.dma_mask		= &cnc1800l_mshci_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL,
		.platform_data		= &cnc1800l_mshci_platform_data,
	}
};

void cnc1800l_mshci_set_platdata(struct cnc1800l_mshci_platdata *pd)
{
	struct cnc1800l_mshci_platdata *set = &cnc1800l_mshci_platform_data;

	set->cd_type = pd->cd_type;
	set->wp_gpio = pd->wp_gpio;
	set->ext_cd_gpio = pd->ext_cd_gpio;
	set->ext_cd_gpio_invert = pd->ext_cd_gpio_invert;
	set->has_wp_gpio = pd->has_wp_gpio;
	set->ext_cd_init = pd->ext_cd_init;
	set->ext_cd_cleanup = pd->ext_cd_cleanup;

	if (pd->max_width)
		set->max_width = pd->max_width;
	if (pd->host_caps)
		set->host_caps |= pd->host_caps;

	if (pd->cfg_gpio)
		set->cfg_gpio = pd->cfg_gpio;
	if (pd->init_card)
		set->init_card= pd->init_card;
	if (pd->cfg_card)
		set->cfg_card = pd->cfg_card;
	if (pd->shutdown)
		set->shutdown = pd->shutdown;
}

void __init cnc1800l_platform_register_mshci(void) {
	cnc1800l_mshci_set_platdata(&cnc1800l_mshci_pdata);
	platform_device_register(&cnc1800l_mshci);
}

