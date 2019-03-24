/*
 * Celestialsemi Platform vmalloc definitions
 *
 * Author: xiaodong.fan  <xiaodong.fan@caviumnetworks.com>
 *
 * 2010 (c) Celestial
 * 2011 (c) Cavium

 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <mach/hardware.h>

/* Allow vmalloc range  */
#if (CONFIG_CELESTIAL_MEM_SIZE == 1024)
#define VMALLOC_END	  (PAGE_OFFSET + 0x50000000)
#else
#define VMALLOC_END	  (PAGE_OFFSET + 0x30000000)
#endif

