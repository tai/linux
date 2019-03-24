/*
 *  arch/arm/mach-celestial/include/mach/memory.h
 *
 *  Author: xiaodong fan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#include <mach/hardware.h>

/*
 * Physical DRAM offset.
 * possible for csm1800h high RAM base
 */
#define PHYS_OFFSET	UL(CS_PHY_RAM_BASE)

#ifdef CONFIG_ARCH_SPARSEMEM_ENABLE
/*
 *  * Sparsemem version of the above
 *   */
#define MAX_PHYSMEM_BITS        32
#define SECTION_SIZE_BITS       24
#endif

#endif
