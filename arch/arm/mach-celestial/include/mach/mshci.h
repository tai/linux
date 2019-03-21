/* 
 *
 * MSHCI platform data definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __MACH_MSHCI_H
#define __MACH_MSHCI_H

struct platform_device;
struct mmc_host;
struct mmc_card;
struct mmc_ios;

enum ms_cd_types {
	MSHCI_CD_INTERNAL,	/* use mmc internal CD line */
	MSHCI_CD_EXTERNAL,	/* use external callback */
	MSHCI_CD_GPIO,		/* use external gpio pin for CD line */
	MSHCI_CD_NONE,		/* no CD line, use polling to detect card */
	MSHCI_CD_PERMANENT,	/* no CD line, card permanently wired to host */
};

/*
 * Platform device data
 *
 * @max_width: The maximum number of data bits supported.
 * @host_caps: Standard MMC host capabilities bit field.
 * @cd_type: Type of Card Detection method (see cd_types enum above)
 * @wp_gpio: The gpio number using for WP.
 * @has_wp_gpio: Check using wp_gpio or not.
 * @ext_cd_init: Initialize external card detect subsystem.
 * @ext_cd_cleanup: Cleanup external card detect subsystem.
 * @ext_cd_gpio: gpio pin used for external CD line.
 * @ext_cd_gpio_invert: invert values for external CD gpio line.
 * @cfg_gpio: Configure the GPIO for a specific card bit-width
 * @cfg_card: Configure the interface for a specific card and speed. This
 *            is necessary the controllers and/or GPIO blocks require the
 *	      changing of driver-strength and other controls dependant on
 *	      the card and speed of operation.
 *
 * Initialisation data specific to either the machine or the platform
 * for the device driver to use or call-back when configuring gpio or
 * card speed information.
 */
struct cnc1800l_mshci_platdata {
	unsigned int max_width;
	unsigned int host_caps;

	enum ms_cd_types cd_type;

	int wp_gpio;
	int ext_cd_gpio;
	bool ext_cd_gpio_invert;
	bool has_wp_gpio;

	int (*ext_cd_init) (void (*notify_func)(struct platform_device *, int state));
	int (*ext_cd_cleanup) (void (*notify_func)(struct platform_device *, int state));

	void (*cfg_gpio) (struct platform_device *dev, int width);
	void (*init_card) (struct platform_device *dev);
	void (*cfg_card) (struct platform_device *dev, void __iomem *regbase,
			  struct mmc_ios *ios, struct mmc_card *card);
	void (*shutdown) (void);
};

/*
 * Set platform data
 */
extern void cnc1800l_mshci_set_platdata(struct cnc1800l_mshci_platdata *pd);

/*
 * Default platform data, exported so that per-cpu initialisation can
 * set the correct one when there are more than one cpu type selected.
 */
extern struct cnc1800l_mshci_platdata cnc1800l_mshci_platform_data;

#endif /* __MACH_MSHCI_H */
