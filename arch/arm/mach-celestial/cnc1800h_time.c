/*
 * arch/arm/mach-celestialsemi/time.c
 * Celestial Platform Timer System, 2010
 *
 * Author: Xiaodong Fan <xiaodong.fan@celestialsemi.com>
 *
 * This file is licensed unde the terms of the GNU General Public License 
 * version 2. This program is licensed "as is" without any warranty of 
 * any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/cnt32_to_63.h>
#include <linux/init.h> 

#include <asm/system.h>
#include <asm/irq.h>
#include <mach/time.h>
#include <asm/mach/time.h>
#include <asm/errno.h>
#include <mach/io.h>




/* Clocksource handling */
static cycle_t cs_clksrc_read(struct clocksource *cs)
{
    volatile unsigned int timeval_high,timeval_high2, timeval;
    unsigned long flags;

    raw_local_irq_save(flags);
    timeval_high = readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL + 2);
    timeval =  readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL) ;
    timeval_high2=readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL + 2);
    if ( timeval_high2 != timeval_high) {
        timeval = timeval_high2 << 16 | readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL);
    } 
    else
        timeval = timeval_high << 16 | timeval;
    
    raw_local_irq_restore(flags);

    return ((u32)0xffffffff - timeval);
   
}

static struct clocksource cs_clksrc = {
    .name= "cs_clocksource",
    .shift= 27,
    .rating= 300,
    .read= cs_clksrc_read,
    .mask= CLOCKSOURCE_MASK(32),
    .flags= CLOCK_SOURCE_IS_CONTINUOUS,
};



/*
 * Clockevent handling.
 */
static int
cs_clkevt_next_event(unsigned long delta, struct clock_event_device *dev)
{
    unsigned long flags;
    unsigned long ctrl;

    if (delta == 0)
        return -ETIME;

    raw_local_irq_save(flags);


    ctrl =0;
    writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL);     

    /* Clear and enable clockevent timer interrupt.*/
    readl(VA_TIMER0_BASE + TIMERS_EOI);
    readl(VA_TIMER0_BASE + TIMER_1_EOI);

    /* Setup new clockevent timer value.*/
    writew((u16)((delta >> 16) & 0xffff), VA_TIMER0_BASE+TIMER_1_LOADCOUNT+2);
    writew((u16)(delta & 0xffff), VA_TIMER0_BASE + TIMER_1_LOADCOUNT);


    /* Enable the timer, enable timer interrupt */
    ctrl = TIMER_CTL_ENABLE & ~(TIMER_CTL_INTMASK | TIMER_CTL_PERIODIC) ;
    writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL);     

    raw_local_irq_restore(flags);
    return 0;
}

static void cs_clkevt_set_mode(enum clock_event_mode mode,
                           struct clock_event_device *clk)
{
    unsigned long ctrl;
    unsigned long flags;

    raw_local_irq_save(flags);
    
    switch (mode) {
    case CLOCK_EVT_MODE_PERIODIC:
        printk("Clock event Set mode Periodic\n");
        ctrl = 0;
        writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL); /* disable it first */
        writew((u16)(TIMER_RELOAD >> 16), VA_TIMER0_BASE+TIMER_1_LOADCOUNT+2);
        writew((u16)(TIMER_RELOAD & 0xffff), VA_TIMER0_BASE + TIMER_1_LOADCOUNT);

     
        ctrl = TIMER_CTL_PERIODIC;
        ctrl |= TIMER_CTL_ENABLE;
        writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL);
        break;
    case CLOCK_EVT_MODE_ONESHOT:
        /* oneshot mode, disable interrupt and disable timer */
        printk("Clock event Set mode oneshot\n");
        ctrl = TIMER_CTL_INTMASK; 
        /* period set, and timer enabled in 'next_event' hook */

        readl(VA_TIMER0_BASE + TIMER_1_EOI);
        writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL);
        break;
    case CLOCK_EVT_MODE_UNUSED:
    case CLOCK_EVT_MODE_SHUTDOWN:
    default:
        ctrl = 0;
        writel(ctrl, VA_TIMER0_BASE + TIMER_1_CTRL);
    }

    raw_local_irq_restore(flags);
}

static struct clock_event_device cs_clkevt = {
    .name= "cs_tick",
    .features= CLOCK_EVT_FEAT_PERIODIC, //| CLOCK_EVT_FEAT_ONESHOT ,
    .shift= 32,
    .rating= 300,
    .set_next_event= cs_clkevt_next_event,
    .set_mode= cs_clkevt_set_mode,
};

static irqreturn_t cs_timer_interrupt(int irq, void *dev_id)
{
    /*
     * Clear timer interrupt and call event handler.
     */

    readl(VA_TIMER0_BASE + TIMER_1_EOI);

    cs_clkevt.event_handler(&cs_clkevt);

    return IRQ_HANDLED;
}



/*
 * celestial's sched_clock implementation. It has a resolution of
 * at least 18.5ns (54MHz Timer CLK for csm1201) and a maximum value 
 * more than 2000 days.
 * 
 * The return value is guaranteed to be monotonic in that range as
 * long as there is always less than 39 seconds between successive
 * calls to this function.
*/
#if 1
unsigned long long sched_clock(void)
{
    unsigned long long v ;
   
    volatile unsigned int timeval_high,timeval_high2, timeval;

    //curr_val = readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL )  | (readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL + 2) << 16);
    timeval_high = readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL + 2);
    timeval =  readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL) ;
    timeval_high2=readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL + 2);
    if (timeval_high2  != timeval_high) {
        timeval = timeval_high2 << 16 | readw(VA_TIMER1_BASE + TIMER_1_CUR_VAL);
    } 
    else
        timeval = timeval_high << 16 | timeval;
    
    v = cnt32_to_63(((u32)0xffffffff - timeval));

    /*because of PCLK_MHZ is 47.25 of csm1800H, for precision
     * I have changed PCLK_MHZ macro to a number for 1800H 
    */
#if defined(CONFIG_MACH_CELESTIALSEMI_CSM1201) 
    v= (v* 1000 <<1);
    do_div(v,PCLK_MHZ <<1);
#elif defined(CONFIG_MACH_CELESTIALSEMI_CSM1800H)
    v= (v* 4000 <<1);
    do_div(v,189 <<1);
#endif
    return v;

}
#endif
static struct irqaction cs_timer_irq = {
    .name= "cs_tick",
    .flags= IRQF_DISABLED | IRQF_TIMER |IRQF_IRQPOLL,
    .handler= cs_timer_interrupt
};


static void __init cs_timer_init(void)
{
    u32 u = 0;

     /*
     * Setup free-running clocksource timer (interrupts disabled.)
     */

    /* fist disable timer */
    writel(u & (~TIMER_CTL_ENABLE), VA_TIMER1_BASE + TIMER_1_CTRL);

    writew(0xffff, VA_TIMER1_BASE + TIMER_1_LOADCOUNT);
    writew(0xffff, VA_TIMER1_BASE + TIMER_1_LOADCOUNT + 2);

    u = readl(VA_TIMER1_BASE + TIMER_1_CTRL);
    writel((u | TIMER_CTL_INTMASK) & (~TIMER_CTL_PERIODIC), 
           VA_TIMER1_BASE + TIMER_1_CTRL);
    cs_clksrc.mult = clocksource_hz2mult(PCLK_FREQ, cs_clksrc.shift);
    clocksource_register(&cs_clksrc);

    /* enable timer*/
    u = readl(VA_TIMER1_BASE + TIMER_1_CTRL);
    writel(u | TIMER_CTL_ENABLE | TIMER_CTL_INTMASK | TIMER_CTL_PERIODIC,  VA_TIMER1_BASE + TIMER_1_CTRL);
    

    /*
     * Setup clockevent timer (interrupt-driven.)
     */

    setup_irq(IRQ_TIMER0, &cs_timer_irq);
    
    cs_clkevt.irq = IRQ_TIMER0;
    cs_clkevt.mult = div_sc(PCLK_FREQ, NSEC_PER_SEC, cs_clkevt.shift);
    cs_clkevt.max_delta_ns = clockevent_delta2ns(0xfffffffe, &cs_clkevt);
    cs_clkevt.min_delta_ns = clockevent_delta2ns(PCLK_MHZ, &cs_clkevt); /* 1u */
    cs_clkevt.cpumask = cpumask_of(0);
    clockevents_register_device(&cs_clkevt);

}

struct sys_timer cs_timer = {
	.init = cs_timer_init,
};

