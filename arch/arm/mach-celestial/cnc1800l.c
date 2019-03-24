
/*
 *  arch/arm/mach-celestialsemi/cnc1800l.c
 *
 *  This file contains the hardware definitions of the Cavium Celestial Platform.
 *
 *  Copyright (C) 2011 Cavium
 *
 *  Author: Xaodong Fan <xiaodong.fan@caviumnetworks.com>
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/io.h>
#include <linux/i2c.h>

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
extern void __init cnc1800l_platform_register_mshci(void);
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
	},{
		.virtual	=  VA_IO_SYSTEM_CONTROL_BASE,
		.pfn		= __phys_to_pfn(PA_IO_SYSTEM_CONTROL_BASE),
		.length		= IO_SYSTEM_CONTROL_SIZE,
		.type		= MT_DEVICE
	},
};

#ifdef CONFIG_RTC_DRV_RS5C372
static struct i2c_board_info cnc1800l_i2c_devs[] __initdata = {
        {
                I2C_BOARD_INFO("rs5c372a", 0x32),
        }
};
#endif
void __init cs_map_io(void)
{
	iotable_init(cs_io_desc, ARRAY_SIZE(cs_io_desc));

}

static void __init cnc1800l_init(void)
{
	platform_register_uart();
	cnc18xx_platform_register_spi();
	cnc18xx_platform_register_usb();
	cnc18xx_platform_register_pcmcia();
	cnc1800l_platform_register_mshci();
	platform_register_nand();
#ifdef CONFIG_RTC_DRV_RS5C372
	i2c_register_board_info(0, cnc1800l_i2c_devs, ARRAY_SIZE(cnc1800l_i2c_devs));
#endif
	cnc18xx_register_i2c(0,&cnc18xx_i2c_0_pdata);
#if (CONFIG_CELESTIAL_MEM_SIZE == 512)
	printk("There is %dM for user!\n",XPORT_REGION / (1024 * 1024) + 256);
#elif (CONFIG_CELESTIAL_MEM_SIZE == 1024)
    printk("There is %dM for user!\n",XPORT_REGION / (1024 * 1024) + 256 + 512);
#else
	printk("There is %dM for user!\n",XPORT_REGION / (1024 * 1024));
#endif
	return;
}

static void __init cnc1800l_fixup(struct machine_desc *mdesc, struct tag *tags,
		char **cmdline, struct meminfo *mi)
{
#if (CONFIG_CELESTIAL_MEM_SIZE == 512)
	mi->nr_banks = 2;
	mi->bank[0].start = 0x00000000;
	mi->bank[0].node = 0;
	mi->bank[0].size = XPORT_REGION & 0xfff00000;
	mi->bank[1].start = 0x10000000;
	mi->bank[1].node = 0;
	mi->bank[1].size = SZ_256M;
#elif (CONFIG_CELESTIAL_MEM_SIZE == 1024)
    mi->nr_banks = 2;
    mi->bank[0].start = 0x00000000;
    mi->bank[0].node = 0;
    mi->bank[0].size = XPORT_REGION & 0xfff00000;
    mi->bank[1].start = 0x10000000;
    mi->bank[1].node = 0;
    mi->bank[1].size = SZ_256M + SZ_512M;
#else
	mi->nr_banks = 1;
	mi->bank[0].start = 0x00000000;
	mi->bank[0].node = 0;
	mi->bank[0].size = XPORT_REGION & 0xfff00000;
#endif
}

MACHINE_START(CELESTIAL, "Celestial CNC1800L")
.phys_io	 = PA_UART0_BASE,
	.io_pg_offst = (VA_UART0_BASE >> 18) & 0xfffc,
	.boot_params = PHYS_OFFSET + 0x00000100,
	.map_io		 = cs_map_io,
	.init_irq	 = cs_init_irq,
	.timer		 = &cs_timer,
	.init_machine = cnc1800l_init,
	.fixup       = cnc1800l_fixup,
	MACHINE_END

