
/*
 *  arch/arm/mach-celestialsemi/core.c
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor
 *
 *  Author: Xaodong Fan <xiaodong.fan@celestialsemi.com>
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/io.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/irq.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/setup.h>

#include <mach/i2c.h>

extern int platform_register_uart(void);
extern int cnc18xx_platform_register_usb(void);
extern void __init cs_init_irq(void);
extern int cnc18xx_platform_register_sata(void);
extern struct sys_timer cs_timer;
extern void __init cnc18xx_platform_register_spi(void);
extern int platform_register_nand(void);
extern void __init cnc18xx_platform_register_pcmcia(void);
extern int __init cnc18xx_register_i2c(int instance,struct cnc18xx_i2c_platform_data *pdata);

static struct cnc18xx_i2c_platform_data cnc18xx_i2c_0_pdata = {
	.bus_freq	= 100,	/* kHz */
	.bus_delay	= 0,	/* usec */
};
static struct cnc18xx_i2c_platform_data cnc18xx_i2c_1_pdata = {
	.bus_freq	= 400,	/* kHz */
	.bus_delay	= 0,	/* usec */
};

static struct map_desc cs_io_desc[] __initdata = {
	{
		.virtual	=  VA_IO_REGS_BASE,
		.pfn		= __phys_to_pfn(PA_IO_REGS_BASE),
		.length		= IO_REGS_SIZE,
		.type		= MT_DEVICE
	}, {
		.virtual	=  VA_IO_GRAPHICS_BASE,
		.pfn		= __phys_to_pfn(PA_IO_GRAPHICS_BASE),
		.length		= IO_GRAPHICS_SIZE,
		.type		= MT_DEVICE
	}, {
		.virtual	=  VA_IO_MEDIA_PROCESSOR_BASE,
		.pfn		= __phys_to_pfn(PA_IO_MEDIA_PROCESSOR_BASE),
		.length		= IO_MEDIA_PROCESSOR_SIZE,
		.type		= MT_DEVICE
	},
};

void __init cs_map_io(void)
{
	iotable_init(cs_io_desc, ARRAY_SIZE(cs_io_desc));
}


static void __init cnc1800h_init(void)
{
	(*((volatile unsigned long *)(IO_ADDRESS(0x42100218)))) =0x3;
	platform_register_uart();
	cnc18xx_platform_register_spi();
    cnc18xx_platform_register_usb();
    cnc18xx_platform_register_sata();
    cnc18xx_platform_register_pcmcia();
    platform_register_nand();
	cnc18xx_register_i2c(0,&cnc18xx_i2c_0_pdata);
    printk("There is %dM for user!\n",XPORT_REGION / (1024 * 1024));
    return;

}

static void __init cnc1800h_fixup(struct machine_desc *mdesc, struct tag *tags,
		char **cmdline, struct meminfo *mi)
{
#if (CONFIG_CELESTIALSEMI_MEM_SIZE == 512)
	mi->nr_banks = 2;
	mi->bank[0].start = 0x80000000;
	mi->bank[0].node = 0;
	mi->bank[0].size = XPORT_REGION & 0xfff00000;
	mi->bank[1].start = 0x90000000;
	mi->bank[1].node = 0;
	mi->bank[1].size = SZ_256M;
#else
	mi->nr_banks = 1;
	mi->bank[0].start = 0x00000000;
	mi->bank[0].node = 0;
	mi->bank[0].size = XPORT_REGION & 0xfff00000;
#endif
}

MACHINE_START(CELESTIALSEMI, "Celestial CNC1800H")
	.phys_io	 = PA_UART0_BASE,
	.io_pg_offst = (VA_UART0_BASE >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x00000100,
	.map_io		 = cs_map_io,
	.init_irq	 = cs_init_irq,
	.timer		 = &cs_timer,
	.init_machine	= cnc1800h_init,
	.fixup       = cnc1800h_fixup,
MACHINE_END

