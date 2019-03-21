/*
 * Cavium Celestial Platform IO address definitions
 *  
 *  
 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

#define IO_SPACE_LIMIT 0xffffffff

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define __io(a) __typesafe_io(a)
#define __mem_pci(a) (a)
#define __mem_isa(a) (a)

#ifndef __ASSEMBLER__
#define __arch_ioremap(p, s, t)  cs_ioremap(p, s, t)
#define __arch_iounmap(v)        cs_iounmap(v)

void __iomem *cs_ioremap(unsigned long phys, size_t size,
                              unsigned int type);
void cs_iounmap(volatile void __iomem *addr);
#endif
#endif /* __ASM_ARCH_IO_H */
