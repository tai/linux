/*
 * 
 * uart devices definition for cavium celestial platform
 *
 * Author: xiaodong fan 2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/console.h>

#include <mach/irqs.h>
#include <mach/hardware.h>


static struct plat_serial8250_port serial_platform_data0[] = {
    {
        .membase= (void *)VA_UART0_BASE,
        .mapbase= PA_UART0_BASE,
        .irq= CS_IRQ_UART0,
        .uartclk= PCLK_FREQ,
        .regshift= 2,
        .iotype= UPIO_DWAPB,
        .type= PORT_16550A,
        .flags= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_FIXED_TYPE,
    },
    {        

        .flags= 0,
    },
};


static struct plat_serial8250_port serial_platform_data1[] = {
    {
        .membase= (void *)VA_UART1_BASE,
        .mapbase= PA_UART1_BASE,
        .irq= CS_IRQ_UART1,
        .uartclk= PCLK_FREQ,
        .regshift= 2,
        .iotype= UPIO_DWAPB,
        .type= PORT_16550A,
        .flags= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_FIXED_TYPE,
    },
    {        

        .flags= 0,
    },
};


static struct resource cs_uart0_resources[] = {
    {
        .start= PA_UART0_BASE,
        .end= PA_UART0_BASE + 0xff,
        .flags= IORESOURCE_MEM,
    }, {
        .start= CS_IRQ_UART0,
        .end= CS_IRQ_UART0,
        .flags= IORESOURCE_IRQ,
    },
};


static struct resource cs_uart1_resources[] = {
    {
        .start= PA_UART1_BASE,
        .end= PA_UART1_BASE + 0xff,
        .flags= IORESOURCE_MEM,
    }, {
        .start= CS_IRQ_UART1,
        .end= CS_IRQ_UART1,
        .flags= IORESOURCE_IRQ,
    },
};

static struct platform_device serial_device0 = {
    .name= "serial8250",
    .id= PLAT8250_DEV_PLATFORM,
    .dev= {
        .platform_data = serial_platform_data0,
    },
    .resource= cs_uart0_resources,
    .num_resources= ARRAY_SIZE(cs_uart0_resources),
};

static struct platform_device serial_device1 = {
    .name= "serial8250",
    .id= PLAT8250_DEV_PLATFORM1,
    .dev= {
        .platform_data = serial_platform_data1,
    },
    .resource= cs_uart1_resources,
    .num_resources= ARRAY_SIZE(cs_uart1_resources),
};

int platform_register_uart(void)
{
    platform_device_register(&serial_device0);
    platform_device_register(&serial_device1);
    return 0;
}


/* define a preferred console */

static int __init cs_console_init(void)
{
	return add_preferred_console("ttyS", 2, "115200");
}
console_initcall(cs_console_init);

