/*
 *  arch/arm/mach-celestialsemi/include/mach/cnc1800h.h
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor 
 *  
 *  Author: Xaodong Fan <xiaodong.fan@celestialsemi.com>
 */

#ifndef __ASM_ARCH_CNC1800H_H
#define __ASM_ARCH_CNC1800H_H           

#include <mach/mem_define.h>


#define CPU_FREQ                   378000000


#define PCLK_FREQ                  47250000 //(CPU_FREQ/8)
#define PCLK_KHZ                   47250
#define PCLK_MHZ                   47

#define AHB2_SPI_TCLK              (CPU_FREQ/4)

#if (CONFIG_CELESTIALSEMI_MEM_SIZE == 512)
#define CS_PHY_RAM_BASE            0x80000000
#else
#define CS_PHY_RAM_BASE            0x00000000
#endif

#define CS_IRQ_UART0               12
#define CS_IRQ_UART1               13

#define PA_IO_REGS_BASE            0x10100000
#define IO_REGS_SIZE               0x500000
#define VA_IO_REGS_BASE            IO_ADDRESS(PA_IO_REGS_BASE)

#define PA_IO_GRAPHICS_BASE        0x40000000
#define IO_GRAPHICS_SIZE           0x200 // bigger than required (0x100)
#define VA_IO_GRAPHICS_BASE        IO_ADDRESS(PA_IO_GRAPHICS_BASE)

/* incl. Linebuffer, Audio, Ethernet, Transport,Video Processor,Display,Deint,Vib */
#define PA_IO_MEDIA_PROCESSOR_BASE 0x41000000
#define IO_MEDIA_PROCESSOR_SIZE    0x1200000
#define VA_IO_MEDIA_PROCESSOR_BASE IO_ADDRESS(PA_IO_MEDIA_PROCESSOR_BASE)


#define PA_SMC_IO_BASE             PA_IO_REGS_BASE
#define SMC_IO_SIZE                0x48

#define PA_ETH_MAC_BASE            0x41400000
#define PA_XPORT_REG_BASE          0x41400000
#define PA_DMAC_BASE               (PA_IO_REGS_BASE + 0x130000) // 0x10230000
#define PA_ATA_BASE                (PA_IO_REGS_BASE + 0x110000) // 0x10210000
#define PA_UART0_BASE              (PA_IO_REGS_BASE + 0xf1000)
#define PA_UART1_BASE              (PA_IO_REGS_BASE + 0xf2000)
#define PA_TIMER0_BASE             (PA_IO_REGS_BASE + 0xe2000)
#define PA_TIMER1_BASE             (PA_IO_REGS_BASE + 0xe3000)
#define PA_RTC_BASE                (PA_IO_REGS_BASE + 0xe8000)
#define PA_VIC_BASE                (PA_IO_REGS_BASE + 0x40000)
#define PA_I2C_BASE                (PA_IO_REGS_BASE + 0x70000)
#define PA_FPC_BASE                (PA_IO_REGS_BASE + 0x72000)
#define PA_GPIO1_BASE              (PA_IO_REGS_BASE + 0xe4000)
#define PA_GPIO2_BASE              (PA_IO_REGS_BASE + 0x160000)
#define PA_SCI_BASE                (PA_IO_REGS_BASE + 0xf0000)
#define PA_SATA_BASE               (PA_IO_REGS_BASE + 0x120000)

#define PA_SATA_SIZE               0x4400

#define VA_SMC_IO_BASE             IO_ADDRESS(PA_SMC_IO_BASE)
#define VA_UART0_BASE              IO_ADDRESS(PA_UART0_BASE)
#define VA_UART1_BASE              IO_ADDRESS(PA_UART1_BASE)
#define VA_TIMER0_BASE             IO_ADDRESS(PA_TIMER0_BASE)
#define VA_TIMER1_BASE             IO_ADDRESS(PA_TIMER1_BASE)
#define VA_VIC_BASE                IO_ADDRESS(PA_VIC_BASE)
#define VA_SCI_BASE                IO_ADDRESS(PA_SCI_BASE)
#define VA_FPC_BASE                IO_ADDRESS(PA_FPC_BASE)
#define VA_XPORT_BASE              IO_ADDRESS(PA_XPORT_REG_BASE)
#define VA_SATA_BASE               IO_ADDRESS(PA_SATA_BASE) 
#define VA_GPIO2_BASE              IO_ADDRESS(PA_GPIO2_BASE)
#define VA_ETH_MAC_BASE            IO_ADDRESS(PA_ETH_MAC_BASE)


#define PA_CS0_BASE                0x34000000
#define CS0_SIZE                   0x2000000

#define PA_CS1_BASE                0x36000000
#define CS1_SIZE                   0x2000000

#define PA_CS2_BASE                0x38000000
#define CS2_SIZE                   0x2000000

#define PA_CS3_BASE                0x3A000000
#define CS3_SIZE                   0x2000000

#define PA_SPI_BASE                (PA_IO_REGS_BASE + 0x50000)  // 0x10150000
#define PA_SPI_SIZE                0x4400
#define VA_SPI_BASE                IO_ADDRESS(PA_SPI_BASE)

#define PA_PCMCIA_BASE             (PA_IO_REGS_BASE + 0x150000) // 0x10250000
#define PCMCIA_SIZE                0x10000                      // 64KB
#define VA_PCMCIA_BASE             IO_ADDRESS(PA_IO_REGS_BASE)

#endif
