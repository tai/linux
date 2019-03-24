/*
 * 
 * spi devices for celestial platform
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

#include <linux/mtd/partitions.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi.h>
#include <linux/spi/cnc18xx_spi.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

static u64 spi_dmamask = DMA_BIT_MASK(32);

static struct cnc18xx_spi_info cnc18xx_spi_plat_data = {
	.tclk				= AHB2_SPI_TCLK,
	.enable_clock_fix	= 1,
	.max_rcv_sz			= 65536,
};

static struct resource cnc18xx_spi_resources[] = {
	[0] = {
		.start	= PA_SPI_BASE,
		.end	= PA_SPI_BASE + PA_SPI_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
        .name   = "spi irq",
		.start	= IRQ_SPI,
		.end	= IRQ_SPI,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cnc18xx_spi = {
	.name		= "cnc18xx_spi",
	.id			= 0,
	.dev		= {
		.dma_mask = &spi_dmamask,
	    .coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data	= &cnc18xx_spi_plat_data,
	},
	.num_resources	= ARRAY_SIZE(cnc18xx_spi_resources),
	.resource	= cnc18xx_spi_resources,
};

#ifndef CONFIG_MTD_SPIDEV
/*
 * SPI devices.
 */
static struct mtd_partition __initdata cnc18xx_spi_partitions[] = {
	{
		.name	= "BOOT1",
		.offset	= 0,
		.size	= 4 * 1024,
	},
	{
		.name	= "BOOT2",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 256 * 1024,
	},
	{
		.name	= "kernel",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 2222 * 1024,
	},
	{
		.name	= "file system",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= MTDPART_SIZ_FULL,
	},
};

static struct flash_platform_data __initdata cnc18xx_spi_flash_platform_data = {
	.name		= "spi_flash",
	.parts		= cnc18xx_spi_partitions,
	.nr_parts	= ARRAY_SIZE(cnc18xx_spi_partitions)
};

static struct spi_board_info cnc18xx_spi_devices[] = {
	{	/* DataFlash chip */
		.modalias		= "m25p80",
		.max_speed_hz	= 45 * 1000 * 1000,
		.bus_num		= 0,
		.chip_select	= 0,
		.irq			= -1,
		.platform_data	= &cnc18xx_spi_flash_platform_data
	},
};

void __init cnc18xx_platform_register_spi(void)
{
	spi_register_board_info(cnc18xx_spi_devices, ARRAY_SIZE(cnc18xx_spi_devices));
	platform_device_register(&cnc18xx_spi);
}
#else
void __init cnc18xx_platform_register_spi(void) {
	platform_device_register(&cnc18xx_spi);
}
#endif

