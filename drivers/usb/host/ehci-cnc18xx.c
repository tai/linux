/*
* linux/driver/usb/host/ehci-csm18xx.c
 *
 * Copyright (c) 2010  celestialsemi  corporation.
 *
 * Ran Hao <ran.hao@celestialsemi.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation
 *
 */


#include <linux/platform_device.h>
#include <mach/cnc1800l_power_clk.h>




extern int usb_disabled(void);

/*-------------------------------------------------------------------------*/
static int usb_hcd_cnc18xx_make_hc(struct usb_hcd *hcd)
{
	u32 *reg, val;
       
	reg = (u32 *)((char *)(hcd->regs) + 0x10);
	val = readl(reg);

	if ((val & 3) != 3 && (val & 3) != 0) {
		printk("usb_hcd_cnc18xx_make_hc(): unsane hardware state\n");
		return -EIO;
	}

	val |= 3;
	*reg = val;
	
	return 0;
}

static void cnc18xx_start_ehc(struct platform_device *dev)
{
	pr_debug(__FILE__ ": starting cnc18xx EHCI USB Controller\n");

	/* Reset USB core */
	clock_usb_reset(_do_reset);
	udelay(100);
	clock_usb_reset(_do_set);
	clock_usb1phy_bypassenable();
}

static void cnc18xx_stop_ehc(struct platform_device *dev)
{
	pr_debug(__FILE__ ": stopping cnc18xx EHCI USB Controller\n");
	clock_usb1phy_bypassdisable();
}

#ifdef CONFIG_PM

static int ehci_cnc18xx_bus_suspend(struct usb_hcd *hcd)
{
	int result;
	result = ehci_bus_suspend(hcd);
	if (!result)
	{
		//
	}
	return result;
}

static int ehci_cnc18xx_bus_resume(struct usb_hcd *hcd)
{
	//
	return ehci_bus_resume(hcd);
}

#else

#define ehci_cnc18xx_bus_suspend NULL
#define ehci_cnc18xx_bus_resume NULL

#endif

/*-------------------------------------------------------------------------*/

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * usb_ehci_cnc18xx_probe - initialize cnc18xx-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 *
 */
int usb_ehci_cnc18xx_probe(const struct hc_driver *driver,
			  struct usb_hcd **hcd_out, struct platform_device *dev)
{
	int retval;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;

	if (dev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		retval = -ENOMEM;
	}
	hcd = usb_create_hcd(driver, &dev->dev, "cnc18xx");
	if (!hcd)
		return -ENOMEM;
	hcd->rsrc_start = dev->resource[0].start;
	hcd->rsrc_len = dev->resource[0].end - dev->resource[0].start + 1;

	if (!request_mem_region((hcd->rsrc_start ), hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		retval = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap((hcd->rsrc_start), hcd->rsrc_len) + 0x100;
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}

	cnc18xx_start_ehc(dev);
	retval = usb_hcd_cnc18xx_make_hc(hcd);
	if (retval)
		return retval;

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	retval = usb_add_hcd(hcd, dev->resource[1].start, IRQF_DISABLED);
	if (retval == 0)
		return retval;

	cnc18xx_stop_ehc(dev);
	iounmap(hcd->regs - 0x100);
err2:
	release_mem_region((hcd->rsrc_start & 0x1fffffff), hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return retval;
}

/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_ehci_hcd_cnc18xx_remove - shutdown processing for ???-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_ehci_hcd_cnc18xx_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 *
 */
void usb_ehci_cnc18xx_remove(struct usb_hcd *hcd, struct platform_device *dev)
{
	usb_remove_hcd(hcd);
	cnc18xx_stop_ehc(dev);
	iounmap(hcd->regs - 0x100);
	release_mem_region((hcd->rsrc_start & 0x1fffffff), hcd->rsrc_len);
	usb_put_hcd(hcd);
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ehci_cnc18xx_hc_driver = {
	.description = hcd_name,
	.product_desc = "CNC18XX EHCI",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_init,
	.start = ehci_run,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	/*
	 * power management
	 */
	.bus_suspend = ehci_cnc18xx_bus_suspend,//??
	.bus_resume = ehci_cnc18xx_bus_resume,

};

/*-------------------------------------------------------------------------*/

static int ehci_hcd_cnc18xx_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd = NULL;
	int ret;

	pr_debug("In ehci_hcd_cnc18xx_drv_probe\n");

	if (usb_disabled())
		return -ENODEV;

	ret = usb_ehci_cnc18xx_probe(&ehci_cnc18xx_hc_driver, &hcd, pdev);
	return ret;
}

static int ehci_hcd_cnc18xx_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_ehci_cnc18xx_remove(hcd, pdev);
	return 0;
}
#if 0
static int ehci_hcd_cnc18xx_drv_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc;

	return 0;
	rc = 0;

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != 0x04) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	au1xxx_stop_ehc();

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	// could save FLADJ in case of Vaux power loss
	// ... we'd only use it to handle clock skew

	return rc;

	return 0;
}
static int ehci_hcd_cnc18xx_drv_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	return 0;
}
#endif
void
usb_hcd_cnc18xx_shutdown(struct platform_device* dev)
{
	struct usb_hcd *hcd = platform_get_drvdata(dev);

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}


MODULE_ALIAS("cnc18xx-ehci");
static struct platform_driver ehci_hcd_cnc18xx_driver = {
	.probe = ehci_hcd_cnc18xx_drv_probe,
	.remove = ehci_hcd_cnc18xx_drv_remove,
	.shutdown = usb_hcd_cnc18xx_shutdown,
	//.suspend      = ehci_hcd_cnc18xx_drv_suspend, 
	//.resume       = ehci_hcd_cnc18xx_drv_resume, 
	.driver = {
		.name = "cnc18xx-ehci",
		.bus = &platform_bus_type
	}
};
