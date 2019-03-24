/*
 *  arch/arm/mach-celestialsemi/include/mach/hardware.h
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor 
 *  
 *  Author: Xaodong Fan <xiaodong.fan@celestialsemi.com>
 */


#ifndef __MACH_CLKDEV_H
#define __MACH_CLKDEV_H

static inline int __clk_get(struct clk *clk)
{
	return 1;
}

static inline void __clk_put(struct clk *clk)
{
}

#endif
