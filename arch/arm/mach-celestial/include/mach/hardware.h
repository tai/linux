/*
 *  arch/arm/mach-celestialsemi/include/mach/hardware.h
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor 
 *  Copyright (C) 2011 Cavium 
 *  Author: Xaodong Fan <xiaodong.fan@celestialsemi.com>
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H


#if defined(CONFIG_MACH_CELESTIAL_CNC1800H)
#include <mach/cnc1800h.h>
#elif defined(CONFIG_MACH_CELESTIAL_CNC1800L)
#include <mach/cnc1800l.h>
#endif 


/* macro to get at IO space when running virtually */
#define IO_ADDRESS(phys) (0xF0000000 + ((((phys) >> 4) & 0x0ff00000) + ((phys) & 0xFFFFFFF)))    
 

/* mapping size not bigger than 3dfffff */
/*#define IO_ADDRESS(phys) (0xF0000000 | ((((phys) >> 4) & 0x07000000) + ((phys) & 0xFFFFFFF))) 
  #define IO_ADDRESS(phys) (0xF0000000 | ((((phys) >> 4) & 0x08000000) | ((phys) & 0xFFFFFF))) */
#endif


