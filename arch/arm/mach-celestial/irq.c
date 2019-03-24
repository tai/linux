/*
 * linux/arch/arm/mach-celestialsemi/irq.c
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor 
 *  Copyright (C) 2011 Cavium  
 *  
 *  Author: Xaodong Fan <xiaodong.fan@caviumnetworks.com>
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <mach/hardware.h>

/*
 * This contains the irq mask for Designware VIC irq controller,
 * 32 IRQ sources
 */


void cs_irq_enable(unsigned int irq)
{
    //    printk("cs_irq_enable %d\n",irq);
    volatile unsigned int *irq_inten_l=(unsigned int *)(VA_VIC_BASE+VIC_IRQ_ENABLE);
    *irq_inten_l|=(1<< (irq & 31));
}

void cs_irq_disable(unsigned int irq)
{

    volatile unsigned int *irq_inten_l=(unsigned int *)(VA_VIC_BASE+VIC_IRQ_ENABLE);
    *irq_inten_l&=~(1<< (irq & 31));
}
void cs_irq_ack(unsigned int irq)
{

    volatile unsigned int *irq_inten_l=(unsigned int *)(VA_VIC_BASE+VIC_IRQ_ENABLE);
    *irq_inten_l&=~(1<<(irq& 31));	
}

#if 0
static irqreturn_t bogus_int(int irq, void *dev_id, struct pt_regs *regs)
{
   
    return IRQ_NONE;
}
#endif 

static struct irq_chip cs_vic_chip = {
    .name   = "CSVIC",
    .mask_ack = cs_irq_ack,
    .ack = cs_irq_ack,
    .mask   = cs_irq_disable,
    .unmask = cs_irq_enable,
};


void __init cs_init_irq(void)
{
        unsigned int i;

    /* Clear All Interrupts*/
    writel(0, VA_VIC_BASE + VIC_IRQ_INTFORCE);

    /* Disable All Interrupts */
    writel(0, VA_VIC_BASE + VIC_IRQ_ENABLE);

     /*Un  Mask All Interrupts */
    writel(0, VA_VIC_BASE + VIC_IRQ_MASK);
    
    /* Regist handler */
    for (i = 0; i < NR_IRQS_VIC; i++) {
        set_irq_chip(i, &cs_vic_chip);
        set_irq_handler(i, handle_level_irq);
        set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
    }
}
