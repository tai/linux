/*
 *  linux/arch/arm/mach-celestialsemi/include/mach/irqs.h
 *
 *  Copyright (C) 2010 Celestial  
 *  Author: Xiaodong Fan <xiaodong.fan@celestialsemi.com>
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



#ifndef __ASM_ARM_ARCH_IRQ_H
#define __ASM_ARM_ARCH_IRQ_H



/* 
 *  IRQ interrupts definitions are the same the INT definitions
 *  held within platform.h
 */

#if defined(CONFIG_MACH_CELESTIAL_CNC1800H) 

#define IRQ_WDOGINT             0
#define IRQ_PCMCIA              1
#define IRQ_I2C1_COMMRRx        2
#define IRQ_I2C2_COMMRTx        3
#define IRQ_TIMER0              4
#define IRQ_TIMER1              5
#define IRQ_GPIO                6
#define IRQ_DF1                 7
#define IRQ_PANEL_CTL           8
#define IRQ_IR                  9
#define IRQ_RTC                 10
#define IRQ_SPI                 11
#define IRQ_UART0               12
#define IRQ_UART1               13
#define IRQ_SMART_CART          14    
#define IRQ_SECURITY            15
#define IRQ_TRANSPORT_1         16
#define IRQ_TRANSPORT_2         17
#define IRQ_NOUSED_18           18
#define IRQ_GFX_BLIT            19
#define IRQ_VIDEO_PROCESSOR     20
#define IRQ_AUDIO_PROCESSOR     21
#define IRQ_USB_EHCI            22
#define IRQ_SATA                23
#define IRQ_ETH_DMA             24
#define IRQ_DF2                 25 
#define IRQ_CRYPTO_ENG          26
#define IRQ_USB_OHCI            27
#define IRQ_PCMCIA_IO           28
#define IRQ_PCMCIA_REQUEST      29
#define IRQ_HDMI                30
#define IRQ_EXTERNAL            31


#define NR_IRQS_VIC 32
#define MAX_GPIO_NUMBER 41
#define NR_IRQS (NR_IRQS_VIC + MAX_GPIO_NUMBER)

#define VIC_IRQ_ENABLE          0
#define VIC_IRQ_MASK            0x8
#define VIC_IRQ_INTFORCE        0x10
#define VIC_IRQ_RAW_STATUS      0x18
#define VIC_IRQ_STATUS          0x20
#define VIC_IRQ_MASK_STATUS     0x28
#define VIC_IRQ_FINAL_STATUS    0x30

#endif /* for cnc1800h defines */

#if defined(CONFIG_MACH_CELESTIAL_CNC1800L) 

#define IRQ_WDOGINT             0
#define IRQ_PCMCIA              1
#define IRQ_I2C1_COMMRRx        2
#define IRQ_I2C2_COMMRTx        3
#define IRQ_TIMER1              4  /* AHB timer 1*/
#define IRQ_TIMER2              5  /* AHB timer 2 */
#define IRQ_GPIO                6
#define IRQ_DF1                 7
#define IRQ_PANEL_CTL           8
#define IRQ_IR                  9
#define IRQ_APB_TIMER           10
#define IRQ_SPI                 11
#define IRQ_UART0               12
#define IRQ_UART1               13
#define IRQ_SMART_CART          14    
#define IRQ_SECURITY            15
#define IRQ_TRANSPORT_1         16
#define IRQ_TRANSPORT_2         17
#define IRQ_SDIO                18
#define IRQ_MSHCI               18
#define IRQ_GFX_BLIT            19
#define IRQ_VIDEO_PROCESSOR     20
#define IRQ_AUDIO_PROCESSOR     21
#define IRQ_USB_EHCI            22

#define IRQ_ETH_DMA             24
#define IRQ_DF2                 25 
#define IRQ_CRYPTO_ENG          26
#define IRQ_USB_OHCI            27
#define IRQ_PCMCIA_IO           28
#define IRQ_PCMCIA_REQUEST      29
#define IRQ_HDMI                30
#define IRQ_EXTERNAL            31


#define NR_IRQS_VIC 32
#define MAX_GPIO_NUMBER 41
#define NR_IRQS (NR_IRQS_VIC + MAX_GPIO_NUMBER)

#define VIC_IRQ_ENABLE          0
#define VIC_IRQ_MASK            0x8
#define VIC_IRQ_INTFORCE        0x10
#define VIC_IRQ_RAW_STATUS      0x18
#define VIC_IRQ_STATUS          0x20
#define VIC_IRQ_MASK_STATUS     0x28
#define VIC_IRQ_FINAL_STATUS    0x30

#endif /* for cnc1800l defines */


#define gpio_to_irq(gpio) \
    (((gpio) <= MAX_GPIO_NUMBER) ? (NR_IRQS_VIC + (gpio)) : -EINVAL)

#define irq_to_gpio(irq) ((irq) - gpio_to_irq(0))


#endif
