/*
 *  arch/arm/mach-celestial/include/mach/time.h
 *
 *  This file contains the hardware definitions of the Celestial Platform.
 *
 *  Copyright (C) 2010 Celestial Semiconductor 
 *  Copyright (C) 2011 Cavium 
 
 *  Author: Xaodong Fan <xiaodong.fan@caviumneworks.com>
 */
#ifndef __ASM_ARCH_TIME_H
#define __ASM_ARCH_TIME_H

#include <mach/hardware.h>
/* This file defines MACROS needed by time.c */

#define TIMER_PRE_LOAD_0       0x0
#define TIMER_PRE_LOAD_1       0x4
#define TIMER_THRESHOLD_0      0x8
#define TIMER_THRESHOLD_1      0xc
#define TIMER_MODE             0x10
#define TIMER_INTR_EN          0x14
#define TIMER_INTR_CLR         0x18
#define TIMER_ENABLE           0x1c
#define TIMER_CURRENT_VAL_0    0x20
#define TIMER_CURRENT_VAL_1    0x24
#define TIMER_INTR_STATUS      0x28
#define TIMER_INTR_RAW_STATUS  0x2c





#define APB_TIMER_1_LOADCOUNT     0
#define APB_TIMER_1_CUR_VAL       0x04
#define APB_TIMER_1_CTRL          0x08
#define APB_TIMER_1_EOI           0x0c
#define APB_TIMER_1_INT_STATUS    0x10

#define APB_TIMER_2_LOADCOUNT     0x14
#define APB_TIMER_2_CUR_VAL       0x18
#define APB_TIMER_2_CTRL          0x1C
#define APB_TIMER_2_EOI           0x20
#define APB_TIMER_2_INT_STATUS    0x24

#define APB_TIMERS_INT_STATUS     0xA0
#define APB_TIMERS_EOI            0xA4
#define APB_TIMERS_RAW_INT_STATUS 0xA8


#define APB_TIMER_CTL_ENABLE      (1 <<0)
#define APB_TIMER_CTL_PERIODIC    (1<<1)
#define APB_TIMER_CTL_INTMASK     (1<<2)


#define TIMER_RELOAD (CLOCK_TICK_RATE / HZ)


/* 
 *  These are useconds NOT ticks.  
 * 
 */
#define mSEC_1                1000
#define mSEC_5                (mSEC_1 * 5)
#define mSEC_10               (mSEC_1 * 10)
#define mSEC_25               (mSEC_1 * 25)
#define SEC_1                 (mSEC_1 * 1000)







#endif
