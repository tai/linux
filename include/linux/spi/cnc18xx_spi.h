/*
 * orion_spi.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __LINUX_SPI_CNC18XX_SPI_H
#define __LINUX_SPI_CNC18XX_SPI_H

struct cnc18xx_spi_info {
	u32	tclk;		/* no <linux/clk.h> support yet */
	u32	enable_clock_fix;
	u32 max_rcv_sz;
};


#endif
