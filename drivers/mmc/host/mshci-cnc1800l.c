/*
 * Mobile Storage Host Controller Interface driver
 * Scott Shu <sshu@caviumnetworks.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

#include <linux/mmc/host.h>

#include <mach/mshci.h>
#include "mshci.h"

#define MAX_BUS_CLK	(1)

struct mshci_cnc1800l {
	struct mshci_host	*host;		/* The MSHCI host created */
	struct platform_device	*pdev;		/* The platform device we where created from */
	struct resource		*ioarea;	/* The resource created when we claimed the IO area */
	struct cnc1800l_mshci_platdata *pdata;	/* The platform data for this controller */
	unsigned int		cur_clk;	/* The index of the current bus clock */
	struct clk		*clk_io;	/* The clock for the internal bus interface */
	struct clk		*clk_bus[MAX_BUS_CLK];	/* The clocks that are available for the SD/MMC bus clock */
};

extern int gpio_hw_write(unsigned char gpio_id, unsigned char data);
extern int gpio_hw_set_direct(int gpio_id, int dir);

static inline struct mshci_cnc1800l *to_cnc1800l(struct mshci_host *host)
{
	return mshci_priv(host);
}

static unsigned int mshci_cnc1800l_get_max_clk(struct mshci_host *host)
{
	int clk = 47250000;	/* 47.25M */

	return clk;
}

static unsigned int mshci_cnc1800l_consider_clock(struct mshci_cnc1800l *ourhost,
					     unsigned int src,
					     unsigned int wanted)
{
	unsigned long rate;
	struct clk *clksrc = ourhost->clk_bus[src];
	int div;

	if (!clksrc)
		return UINT_MAX;

	//rate = clk_get_rate(clksrc);
	rate = 400000;

	for (div = 1; div < 256; div *= 2) {
		if ((rate / div) <= wanted)
			break;
	}

	dev_dbg(&ourhost->pdev->dev, "clk %d: rate %ld, want %d, got %ld\n",
		src, rate, wanted, rate / div);

	return (wanted - (rate / div));
}

static void mshci_cnc1800l_set_clock(struct mshci_host *host, unsigned int clock)
{
	struct mshci_cnc1800l *ourhost = to_cnc1800l(host);
	unsigned int best = UINT_MAX;
	unsigned int delta;
	int best_src = 0;
	int src;

	/* don't bother if the clock is going off. */
	if (clock == 0)
		return;

	for (src = 0; src < MAX_BUS_CLK; src++) {
		delta = mshci_cnc1800l_consider_clock(ourhost, src, clock);
		if (delta < best) {
			best = delta;
			best_src = src;
		}
	}

	dev_dbg(&ourhost->pdev->dev, "selected source %d, clock %d, delta %d\n",
		 best_src, clock, best);

	/* select the new clock source */
	if (ourhost->cur_clk != best_src) {
		struct clk *clk = ourhost->clk_bus[best_src];

		//ourhost->cur_clk = best_src;
		ourhost->cur_clk = 47250000;
		//host->max_clk = clk_get_rate(clk);
		host->max_clk = 47250000;
	}

	/* reconfigure the hardware for new clock rate */
	{
		struct mmc_ios ios;

		ios.clock = clock;

		if (ourhost->pdata->cfg_card)
			(ourhost->pdata->cfg_card)(ourhost->pdev, host->ioaddr,
						   &ios, NULL);
	}
}

static void mshci_cnc1800l_set_ios(struct mshci_host *host, struct mmc_ios *ios)
{
	/* nothing to do */
}

static void mshci_cnc1800l_init_card(struct mshci_host *host)
{
	struct mshci_cnc1800l *ourhost = to_cnc1800l(host);

	/* set CHIP_PWD_L */
	/* TODO: GPIO_10 */

	if (ourhost->pdata->init_card)
		(ourhost->pdata->init_card)(ourhost->pdev);
}

static struct mshci_ops mshci_cnc1800l_ops = {
	.get_max_clock  = mshci_cnc1800l_get_max_clk,
	.set_clock		= mshci_cnc1800l_set_clock,
	.set_ios		= mshci_cnc1800l_set_ios,
	.init_card		= mshci_cnc1800l_init_card,
};

static int __devinit mshci_cnc1800l_probe(struct platform_device *pdev)
{
	struct cnc1800l_mshci_platdata *pdata = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;
	struct mshci_host *host;
	struct mshci_cnc1800l *sc;
	struct resource *res;
	int ret, irq, ptr, clks;

	BUG_ON(pdev == NULL);

	if (!pdata) {
		dev_err(dev, "no device data specified\n");
		return -ENOENT;
	}

	(*((volatile unsigned long *)(IO_ADDRESS(0xB2100140)))) = 0xffffffff;
	(*((volatile unsigned long *)(IO_ADDRESS(0xB2110024)))) = 0x01;
	mdelay(100);

#ifdef CONFIG_CELESTIAL_TIGA_MINI
    /* this code is for AR6000 SDIO Wi-Fi, GPIO 10 is Wi-Fi reset */
    gpio_hw_set_direct(10, 1);
	udelay(10);
    gpio_hw_write(10, 1);
#endif

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "no irq specified\n");
		return irq;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "no memory specified\n");
		return -ENOMEM;
	}

	host = mshci_alloc_host(dev, sizeof(struct mshci_cnc1800l));
	if (IS_ERR(host)) {
		dev_err(dev, "mshci_alloc_host() failed\n");
		return PTR_ERR(host);
	}
	sc = mshci_priv(host);

	sc->host = host;
	sc->pdev = pdev;
	sc->pdata = pdata;

	platform_set_drvdata(pdev, host);

#if 0
	sc->clk_io = clk_get(dev, "mshc");
	if (IS_ERR(sc->clk_io)) {
		dev_err(dev, "failed to get io clock\n");
		ret = PTR_ERR(sc->clk_io);
		goto err_io_clk;
	}
	/* enable the local io clock and keep it running for the moment. */
	clk_enable(sc->clk_io);

	for (clks = 0, ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
		struct clk *clk;
		char *name = pdata->clocks[ptr];
		if (name == NULL)
			continue;
		clk = clk_get(dev, name);
		if (IS_ERR(clk)) {
			dev_err(dev, "failed to get clock %s\n", name);
			continue;
		}

		clks++;
		sc->clk_bus[ptr] = clk;
		clk_enable(clk);


		dev_info(dev, "clock source %d: %s (%ld Hz)\n",
			 ptr, name, clk_get_rate(clk));
	}

	if (clks == 0) {
		dev_err(dev, "failed to find any bus clocks\n");
		ret = -ENOENT;
		goto err_no_busclks;
	}
#endif
	sc->ioarea = request_mem_region(res->start, resource_size(res),
					mmc_hostname(host->mmc));
	if (!sc->ioarea) {
		dev_err(dev, "failed to reserve register area\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	host->ioaddr = ioremap_nocache(res->start, resource_size(res));
	if (!host->ioaddr) {
		dev_err(dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_req_regs;
	}

	host->hw_name = "cavium-mshci";
	host->ops = &mshci_cnc1800l_ops;
	host->quirks = 0;
	host->irq = irq;

	if (pdata->host_caps)
		host->mmc->caps = pdata->host_caps;
	else
		host->mmc->caps = 0;

	if (pdata->cd_type == MSHCI_CD_PERMANENT){
		host->quirks |= MSHCI_QUIRK_BROKEN_PRESENT_BIT | MSHCI_QUIRK_1BIT_INTERRUPT;
		host->mmc->caps |= MMC_CAP_NONREMOVABLE;
	}

	ret = mshci_add_host(host);
	if (ret) {
		dev_err(dev, "mshci_add_host() failed\n");
		goto err_add_host;
	}

	return 0;

err_add_host:
	release_resource(sc->ioarea);
	kfree(sc->ioarea);

err_req_regs:
//	for (ptr = 0; ptr < MAX_BUS_CLK; ptr++) {
//		clk_disable(sc->clk_bus[ptr]);
//		clk_put(sc->clk_bus[ptr]);
//	}

err_no_busclks:
//	clk_disable(sc->clk_io);
//	clk_put(sc->clk_io);

err_io_clk:
	mshci_free_host(host);
	return ret;
}

static int __devexit mshci_cnc1800l_remove(struct platform_device *pdev)
{
    struct cnc1800l_mshci_platdata *pdata = pdev->dev.platform_data;
    struct mshci_host *host = platform_get_drvdata(pdev);
    struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    mshci_remove_host(host, 1);

    iounmap(host->ioaddr);
    release_mem_region(iomem->start, resource_size(iomem));
    mshci_free_host(host);
    platform_set_drvdata(pdev, NULL);
    (*((volatile unsigned long *)(IO_ADDRESS(0xB2110024)))) = 0x0;

#ifdef CONFIG_CELESTIAL_TIGA_MINI
    /* this code is for AR6000 SDIO Wi-Fi, GPIO 10 is Wi-Fi reset */
    gpio_hw_set_direct(10, 1);
	udelay(10);
    gpio_hw_write(10, 0);
#endif

    return 0;
}


#ifdef CONFIG_PM
static int mshci_cnc1800l_suspend(struct platform_device *dev, pm_message_t pm)
{
	struct mshci_host *host = platform_get_drvdata(dev);

	mshci_suspend_host(host, pm);
	return 0;
}

static int mshci_cnc1800l_resume(struct platform_device *dev)
{
	struct mshci_host *host = platform_get_drvdata(dev);

	mshci_resume_host(host);
	return 0;
}
#else
#define mshci_cnc1800l_suspend	NULL
#define mshci_cnc1800l_resume	NULL
#endif

static struct platform_driver mshci_cnc1800l_driver = {
	.driver		= {
		.name	= "cnc1800l-mshci",
		.owner	= THIS_MODULE,
	},
	.probe		= mshci_cnc1800l_probe,
	.remove		= __devexit_p(mshci_cnc1800l_remove),
	.suspend	= mshci_cnc1800l_suspend,
	.resume	        = mshci_cnc1800l_resume,
};

static struct proc_dir_entry *sdio_proc_entry = NULL;

static int sdio_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u32 addr;
	u32 val;

	const char *cmd_line = buffer;;

	if (strncmp("rl", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
		printk(" sdio read [0x%08x] = 0x%08x \n", addr, (*((volatile unsigned long *)(IO_ADDRESS(addr)))));
	} else if (strncmp("wl", cmd_line, 2) == 0) {
    	addr = simple_strtol(&cmd_line[3], NULL, 16);
        val = simple_strtol(&cmd_line[12], NULL, 16);
	    (*((volatile unsigned long *)(IO_ADDRESS(addr)))) = val;
	}
    else {
        printk("unknown command.\n");
    }

	return count;
}

static int __init mshci_cnc1800l_init(void)
{
	sdio_proc_entry = create_proc_entry("driver/sdio", 0, NULL);
	if (NULL != sdio_proc_entry) {
		sdio_proc_entry->write_proc = &sdio_proc_write;
	}

	return platform_driver_register(&mshci_cnc1800l_driver);
}

static void __exit mshci_cnc1800l_exit(void)
{
	platform_driver_unregister(&mshci_cnc1800l_driver);
}

module_init(mshci_cnc1800l_init);
module_exit(mshci_cnc1800l_exit);

MODULE_DESCRIPTION("Cavium Celestial MSHCI glue");
MODULE_AUTHOR("Scott Shu <sshu@caviumnetworks.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cnc1800l-mshci");
