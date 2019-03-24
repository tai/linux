/*
 * Cavium Celestial Platform I2C controller platfrom_device info
 *
 *
*/

#ifndef __ASM_ARCH_I2C_H
#define __ASM_ARCH_I2C_H

/* All frequencies are expressed in kHz */
struct cnc18xx_i2c_platform_data {
	unsigned int	bus_freq;	/* standard bus frequency (kHz) */
	unsigned int	bus_delay;	/* post-transaction delay (usec) */
};


#endif /* __ASM_ARCH_I2C_H */

