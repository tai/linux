/*
 * celestial platform ioremap implementation
 * Copyright (c) 2010, Celestial  
 * Copyright (c) 2011, Cavium  
 * Author: Xiaodong Fan <xiaodong.fan@caviumnetworks.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/tlb.h>
#define BETWEEN(p, st, sz)((p) >= (st) && (p) < ((st) + (sz)))
#define XLATE(p, pst, vst)((void __iomem *)((p) - (pst) + (vst)))

/*
 * Intercept ioremap() requests for addresses in our fixed mapping regions.
 */
void __iomem *cs_ioremap(unsigned long p, size_t size, unsigned int type)
{
    if (BETWEEN(p, PA_IO_REGS_BASE, IO_REGS_SIZE))
        return XLATE(p, PA_IO_REGS_BASE, VA_IO_REGS_BASE);

    else if (BETWEEN(p, PA_IO_GRAPHICS_BASE, IO_GRAPHICS_SIZE))
        return XLATE(p, PA_IO_GRAPHICS_BASE, VA_IO_GRAPHICS_BASE); 

    else if (BETWEEN(p, PA_IO_MEDIA_PROCESSOR_BASE, IO_MEDIA_PROCESSOR_SIZE))
        return XLATE(p, PA_IO_MEDIA_PROCESSOR_BASE, VA_IO_MEDIA_PROCESSOR_BASE);
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
	else if (BETWEEN(p, PA_IO_SYSTEM_CONTROL_BASE, IO_SYSTEM_CONTROL_SIZE))
        return XLATE(p, PA_IO_SYSTEM_CONTROL_BASE, VA_IO_SYSTEM_CONTROL_BASE);
#endif

	return __arm_ioremap(p, size, type);
}
EXPORT_SYMBOL(cs_ioremap);

void cs_iounmap(volatile void __iomem *addr)
{
    unsigned long virt = (unsigned long)addr;

    if (virt >= VMALLOC_START && virt < VMALLOC_END)
        __iounmap(addr);
}
EXPORT_SYMBOL(cs_iounmap);
