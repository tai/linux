/*
 * DA8XX/OMAP L1XX platform device data
 *
 * Copyright (c) 2007-2009, MontaVista Software, Inc. <source@mvista.com>
 * Derived from code that was:
 *	Copyright (C) 2006 Komal Shah <komal_shah802003@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>


#include <linux/device.h>
#include <linux/resource.h>

#define SZ_4K 0x1000
#define CNC18XX_I2C0_BASE 0x80170000
#define CNC18XX_I2C1_BASE 0x80174000
#define IRQ_CNC18XX_I2CINT0 2
#define IRQ_CNC18XX_I2CINT1 3

static struct resource cnc18xx_i2c_resources0[] = {
	{
		.start	= CNC18XX_I2C0_BASE,
		.end	= CNC18XX_I2C0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_CNC18XX_I2CINT0,
		.end	= IRQ_CNC18XX_I2CINT0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cnc18xx_i2c_device0 = {
	.name		= "i2c_cnc18xx",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(cnc18xx_i2c_resources0),
	.resource	= cnc18xx_i2c_resources0,
};

static struct resource cnc18xx_i2c_resources1[] = {
	{
		.start	= CNC18XX_I2C1_BASE,
		.end	= CNC18XX_I2C1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_CNC18XX_I2CINT1,
		.end	= IRQ_CNC18XX_I2CINT1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cnc18xx_i2c_device1 = {
	.name		= "i2c_cnc18xx",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(cnc18xx_i2c_resources1),
	.resource	= cnc18xx_i2c_resources1,
};

int __init cnc18xx_register_i2c(int instance,
		struct cnc18xx_i2c_platform_data *pdata)
{
	struct platform_device *pdev;

	if (instance == 0)
		pdev = &cnc18xx_i2c_device0;
	else if (instance == 1)
		pdev = &cnc18xx_i2c_device1;
	else
		return -EINVAL;

	pdev->dev.platform_data = pdata;
	return platform_device_register(pdev);
}


