/*
 * Debugging macro for Celestial Semicondutor Platform
 * Author: xiaodong fan
 *
 * This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <mach/hardware.h>
        
		.macro	addruart,rx
		mrc	p15, 0, \rx, c1, c0
		tst	\rx, #1			@ MMU enabled?
		ldreq	\rx,  =PA_UART0_BASE
		ldrne	\rx,  =VA_UART0_BASE	@ virtual base
		.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>

