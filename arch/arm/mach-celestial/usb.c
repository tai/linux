/*
 * Platform device support for CNC 18xx SoCs.
 * Written by haoran<ran.hao@celestialsemi>, based on CSM1800 code.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/dma-mapping.h>
#include <mach/hardware.h>
#include <mach/irqs.h>


#define CNC18XX_USB_EHCI_PORTS_START PA_USB_EHCI_BASE
#define CNC18XX_USB_EHCI_PORTS_END   (PA_USB_EHCI_BASE + 0x1000)
#define CNC18XX_USB_OHCI_PORTS_START PA_USB_OHCI_BASE
#define CNC18XX_USB_OHCI_PORTS_END   (PA_USB_OHCI_BASE + 0x1000)
#define CNC18XX_EHCI_USB_INT IRQ_USB_EHCI
#define CNC18XX_OHCI_USB_INT IRQ_USB_OHCI


static u64 ehci_dmamask = DMA_BIT_MASK(32);

/* The dmamask must be set for OHCI to work */
static u64 ohci_dmamask = DMA_BIT_MASK(32);

static struct resource cnc18xx_usb_ehci_resources[] = {
	[0] = {
		.start	= CNC18XX_USB_EHCI_PORTS_START,
		.end	= CNC18XX_USB_EHCI_PORTS_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= CNC18XX_EHCI_USB_INT,
		.end	= CNC18XX_EHCI_USB_INT,
		.flags	= IORESOURCE_IRQ,
	},
};
static struct resource cnc18xx_usb_ohci_resources[] = {
	[0] = {
		.start	= CNC18XX_USB_OHCI_PORTS_START,
		.end	= CNC18XX_USB_OHCI_PORTS_END,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= CNC18XX_OHCI_USB_INT,
		.end	= CNC18XX_OHCI_USB_INT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cnc18xx_usb_ehci_device = {
	.name = "cnc18xx-ehci",
	.id = -1,
	.dev = {
		.dma_mask = &ehci_dmamask,
	    .coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(cnc18xx_usb_ehci_resources),
	.resource = cnc18xx_usb_ehci_resources,
};

static struct platform_device cnc18xx_usb_ohci_device = {
	.name = "cnc18xx-ohci",
	.id = -1,
	.dev = {
		.dma_mask = &ohci_dmamask,
	    .coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(cnc18xx_usb_ohci_resources),
	.resource = cnc18xx_usb_ohci_resources,
};

static struct platform_device *cnc18xx_platform_usb_devices[] __initdata = {
	&cnc18xx_usb_ohci_device,
	&cnc18xx_usb_ehci_device,
};

int __init cnc18xx_platform_register_usb(void)
{
	return platform_add_devices(cnc18xx_platform_usb_devices,
			            ARRAY_SIZE(cnc18xx_platform_usb_devices));
}


//arch_initcall(cnc18xx_platform_usb_init);
