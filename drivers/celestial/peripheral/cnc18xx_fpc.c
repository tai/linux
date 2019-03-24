/*
Rev Date        Author      Comments
--------------------------------------------------------------------------------

*/

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/system.h>
#include <linux/input.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <mach/hardware.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <mach/mux.h>
#include "cnc18xx_fpc.h"

MODULE_AUTHOR("Cavium Celestial SDK team");
MODULE_DESCRIPTION("Cavium Celestial Front Panel Controller driver");
MODULE_LICENSE("GPL");

//#define REPEAT_PERIOD		150	/* NEC: 110ms, RC5: 114ms */

/* Global variables */
static cs_rc_t *rc_dev;
static struct proc_dir_entry *fpc_proc_entry = NULL;
static void __iomem *cs_fpc_base;
static int rc_system_code = 0xffffffff;
DEFINE_SPINLOCK(cs_fpc_lock);

#define fpc_writew(v,a)    	iowrite16(v, (cs_fpc_base + (a)))
#define fpc_readw(a)       	ioread16((cs_fpc_base + (a)))

/* Predefined value */
typedef struct {
	unsigned int h_low;
	unsigned int h_high;
	unsigned int t_low;
	unsigned int t_high;
} pta_t;	/* protocal time ajust */

/* NEC State Machine */
#define NEC_HEAD   	    0
#define NEC_REPEAT  	1
#define NEC_ZERO    	2
#define NEC_ONE     	3
#define NEC_DROP   	    4

#define NEC_ENTRY 4
static pta_t pta_nec[NEC_ENTRY] = {
	{ 8500, 9500, 13000, 14000 },	/* head */
	{ 8500, 9500, 11000, 12000 },	/* repeat */
	{ 400, 700, 1000, 1250 },	/* zero */
	{ 400, 700, 2100, 2350 }	/* one */
};

/* RC5 State Machine */
#define RC5_ONE_BIT	0
#define RC5_ZO_BITS	1
#define RC5_OZ_BITS	2
#define RC5_OZO_BITS	3
#define RC5_DROP	4
#define RC_OVERFLOW_FLAG 7

static pta_t pta_rc5[] = {
	{ 750, 1050, 1600, 1900 },	/* normal */
	{ 750, 1050, 2400, 2720 },	/* zero one one */
	{ 1600, 1800, 2300, 2720 },	/* one zero zero */
	{ 1600, 1900, 3100, 3700 },	/* one zero one */
};

/* RC6 state machine */
#define RC6_STATE_LEARDER        0
#define RC6_STATE_ONE            1
#define RC6_STATE_OhZ            2
#define RC6_STATE_hZO            3
#define RC6_STATE_hZOhZ          4
#define RC6_STATE_hT0O           5
#define RC6_STATE_hT0OhZ         6
#define RC6_STATE_hO             7
#define RC6_STATE_hZhO           8
#define RC6_STATE_ERROR          10

/* There is no T1 in Mode 6A, so no status of T1 in this status machine */
/* In the second note '-' means v+ and '_' means v- */
static pta_t pta_rc6[] = {
	{ 2000, 2800, 3300, 3700 },	/* LEARDER 2667 3556 */ /* ------__ */
	{  100,  550,  600, 1000 },	/* One 444 889 */ /* -_ */
	{  100,  550, 1100, 1550 },	/* One & half Zero 444 1333 , also could be half Zero & half T0 */ /* -__ */
	{  560, 1000, 1100, 1550 },	/* half Zero & One 889 1333 */ /* --_ */
	{  560, 1000, 1600, 1900 },	/* half Zero & One & half Zero 889 1778 */ /* --__ */
	{ 1100, 1600, 1600, 1900 },	/* half T0 & One 1333 1778 */ /* ---_ */
	{ 1100, 1600, 2000, 2500 },	/* half T0 & One & half Zero 1333 2222 */ /* ---__ */
	{  100,  550,    0,    0 },	/* half One 444 0 */ /* - */
    {  560, 1000,    0,    0 }	/* half Zero & half One 889 0 */ /* -- */
                                /* if the last bit is one, maybe can only get the first half */
};

#define RC6_LEADER_BIT        0x11
#define RC6_ONE_BIT           0x12
#define RC6_ZERO_BIT          0x13
#define RC6_T0_BIT            0x14
#define RC6_T1_BIT            0x15
#define RC6_OZ_BIT            0x16
#define RC6_ZO_BIT            0x17
#define RC6_T0O_BIT           0x18
/* End of RC6 state machine */

/* Decode protocal */
static int cnc_nec_decode_bit(unsigned int high_val, unsigned int total_val)
{
	int ret = NEC_DROP;

	if((high_val >= pta_nec[NEC_HEAD].h_low) &&
			(high_val <= pta_nec[NEC_HEAD].h_high) &&
			(total_val >= pta_nec[NEC_HEAD].t_low) &&
			(total_val <= pta_nec[NEC_HEAD].t_high)) {

		PDEBUG("NEC_HEAD: high_val: %d, total_val: %d--NEC HEAD\n", high_val, total_val);
		ret = NEC_HEAD;

	} else if((high_val >= pta_nec[NEC_REPEAT].h_low) &&
			(high_val <= pta_nec[NEC_REPEAT].h_high) &&
			(total_val >= pta_nec[NEC_REPEAT].t_low) &&
			(total_val <= pta_nec[NEC_REPEAT].t_high)) {

		PDEBUG("NEC_REPEAT: high_val: %d, total_val: %d--NEC REPEAT\n", high_val, total_val);
		ret = NEC_REPEAT;

	} else if((high_val >= pta_nec[NEC_ZERO].h_low) &&
			(high_val <= pta_nec[NEC_ZERO].h_high) &&
			(total_val >= pta_nec[NEC_ZERO].t_low) &&
			(total_val <= pta_nec[NEC_ZERO].t_high)) {

		PDEBUG("NEC_ZERO: high_val: %d, total_val: %d--NEC ZERO\n", high_val, total_val);
		ret = NEC_ZERO;

	} else if ((high_val >= pta_nec[NEC_ONE].h_low) &&
			(high_val <= pta_nec[NEC_ONE].h_high) &&
			(total_val >= pta_nec[NEC_ONE].t_low) &&
			(total_val <= pta_nec[NEC_ONE].t_high)) {

		PDEBUG("NEC_ONE : high_val: %d, total_val: %d--NEC ONE\n", high_val, total_val);
		ret = NEC_ONE;

	} else {

		PDEBUG("NECXXXXXXXXX_DROP: high_val: %d, total_val: %d--NEC DROP\n", high_val, total_val);
		ret = NEC_DROP;
	}

	return ret;
}

static int cnc_nec_decode(int idx)
{
	int nec_st;

	PDEBUG("idx %2d ", idx);
	nec_st = cnc_nec_decode_bit(rc_dev->fifo1[idx], rc_dev->fifo2[idx]);

	switch(rc_dev->record.decode_period) {
		case HEAD_PERIOD:              /* Head Period */
			switch (nec_st) {
				case NEC_HEAD:
					rc_dev->record.bit = 0;
					rc_dev->record.rpt = 0;
					rc_dev->record.decode_period = ADDR_PERIOD;
					break;

				case NEC_REPEAT:
					rc_dev->record.bit = 0;

					// rc_dev->max_rpt = 2; /* FIXME: To be set outside */
#if 1
					if (rc_dev->record.rpt == (rc_dev->max_rpt-1)) {
						rc_dev->record.rpt = 0;
						return 1;
					} else {
						rc_dev->record.rpt++;
					}
#else	/* for test */
					rc_dev->record.rpt++;
					return 1;
#endif
					rc_dev->record.decode_period = HEAD_PERIOD;
					break;


				case NEC_DROP:
				case NEC_ZERO:
				case NEC_ONE:
				default:
					rc_dev->record.bit = 0;
					//					rc_dev->record.rpt =0;
					//                    rc_dev->record.decode_period = HEAD_PERIOD;
			}

			break;

		case ADDR_PERIOD:        /* Address Period */
			switch (nec_st) {
				case NEC_HEAD:
					rc_dev->record.bit = 0;
					rc_dev->record.rpt = 0;
					rc_dev->record.decode_period = HEAD_PERIOD;
					return 0;
				case NEC_REPEAT:
					rc_dev->record.rpt++;
					rc_dev->record.decode_period = HEAD_PERIOD;
					break;
				case NEC_DROP:
					rc_dev->record.bit = 0;
					rc_dev->record.rpt = 0;
					rc_dev->record.decode_period = HEAD_PERIOD;
					break;
				case NEC_ZERO:
					clear_bit(rc_dev->record.bit, (unsigned long *)&rc_dev->record.addr);
					break;
				case NEC_ONE:
					set_bit(rc_dev->record.bit, (unsigned long *)&rc_dev->record.addr);
					break;
			}

			if(rc_dev->record.bit == (16-1)) {
				rc_dev->record.bit = 0;
				rc_dev->record.addr = rc_dev->record.addr & 0xffff;
				rc_dev->record.decode_period = CMD_PERIOD;	/* FIXME: Sun: temparary enable cmd process */
			} else {
				rc_dev->record.bit++;
			}
#if 0
			if(rc_dev->addr == rc_dev->record.addr)
				rc_dev->record.decode_period = CMD_PERIOD;
			else
				rc_dev->record.decode_period = HEAD_PERIOD;
#endif

			break;

		case CMD_PERIOD:                                           /* Command Period */
			switch (nec_st) {
				case NEC_HEAD:
					rc_dev->record.bit = 0;
					rc_dev->record.rpt = 0;
					rc_dev->record.decode_period = HEAD_PERIOD;
					return 0;
				case NEC_REPEAT:
				case NEC_DROP:
					rc_dev->record.bit = 0;
					rc_dev->record.rpt = 0;
					rc_dev->record.decode_period = HEAD_PERIOD;
					break;
				case NEC_ZERO:
					clear_bit(rc_dev->record.bit, (unsigned long *)&rc_dev->record.cmd);
					break;
				case NEC_ONE:
					set_bit(rc_dev->record.bit, (unsigned long *)&rc_dev->record.cmd);
					break;
			}

			if(rc_dev->record.bit == (16-1)) {
				rc_dev->record.bit = 0;
				rc_dev->record.cmd = rc_dev->record.cmd & 0xff;
				rc_dev->record.decode_period = HEAD_PERIOD;
				return 1;
			} else {
				rc_dev->record.bit++;
			}

			break;

		default:
			;
	}

	return 0;
}

/*RC5 Decorder*/
static unsigned int cnc_rc5_decode_bit(unsigned int high_val,unsigned int total_val)
{
	int ret = RC5_DROP;

	if(((high_val >= pta_rc5[RC5_ONE_BIT].h_low) &&
				(high_val <= pta_rc5[RC5_ONE_BIT].h_high)) &&

			(((total_val >= pta_rc5[RC5_ONE_BIT].t_low) &&
			  (total_val <= pta_rc5[RC5_ONE_BIT].t_high)) ||

			 rc_dev->record.bit == 13)) { /* total_val equal 0 , this means received last  one bit, (one or zero) */

		PDEBUG("RC5_ONE_BIT: bit: %d, high_val: %d, total_val: %d\n", rc_dev->record.bit, high_val, total_val);
		ret = RC5_ONE_BIT;

	} else if((high_val >= pta_rc5[RC5_ZO_BITS].h_low) &&
			(high_val <= pta_rc5[RC5_ZO_BITS].h_high) &&
			(total_val >= pta_rc5[RC5_ZO_BITS].t_low) &&
			(total_val <= pta_rc5[RC5_ZO_BITS].t_high)) {

		PDEBUG("RC5_ZO_BITS: bit: %d, high_val: %d, total_val: %d\n", rc_dev->record.bit, high_val, total_val);
		ret = RC5_ZO_BITS;

	} else if(((high_val >= pta_rc5[RC5_OZ_BITS].h_low) &&
				(high_val <= pta_rc5[RC5_OZ_BITS].h_high)) &&

			(((total_val >= pta_rc5[RC5_OZ_BITS].t_low) &&
			  (total_val <= pta_rc5[RC5_OZ_BITS].t_high)) ||

			 rc_dev->record.bit == 12)) { /* total_val equal 0, it means received both one and zero bits */

		PDEBUG("RC5_OZ_BITS: bit: %d, high_val: %d, total_val: %d\n", rc_dev->record.bit, high_val, total_val);
		ret = RC5_OZ_BITS;

	} else if((high_val >= pta_rc5[RC5_OZO_BITS].h_low) &&
			(high_val <= pta_rc5[RC5_OZO_BITS].h_high) &&
			(total_val >= pta_rc5[RC5_OZO_BITS].t_low) &&
			(total_val <= pta_rc5[RC5_OZO_BITS].t_high)) {

		PDEBUG("RC5_OZO_BITS: bit: %d, high_val: %d, total_val: %d\n", rc_dev->record.bit, high_val, total_val);
		ret = RC5_OZO_BITS;

	} else {

		PDEBUG("RC5XXXXXXXXX_DROP: bit: %d, high_val: %d, total_val: %d\n", rc_dev->record.bit, high_val, total_val);
		ret = RC5_DROP;
	}

	return ret;
}

static unsigned int cnc_rc5_decode(int idx)
{
	int rc5_st;
	int toggle;

	PDEBUG("idx %2d ", idx);
	rc5_st = cnc_rc5_decode_bit(rc_dev->fifo1[idx], rc_dev->fifo2[idx]);

	switch (rc5_st) {
		case RC5_ONE_BIT:
			rc_dev->record.rc5_data <<= 1;
			if (! rc_dev->record.next_bit_is_zero)
				rc_dev->record.rc5_data |= 1;
			rc_dev->record.bit += 1;
			break;
		case RC5_ZO_BITS:
			rc_dev->record.rc5_data <<= 1;
			//rc_dev->record.rc5_data |= 1;
			rc_dev->record.next_bit_is_zero = 0;
			rc_dev->record.bit += 1;
			break;
		case RC5_OZ_BITS:
			rc_dev->record.rc5_data <<= 2;
			rc_dev->record.rc5_data |= 2;
			rc_dev->record.next_bit_is_zero = 1;
			rc_dev->record.bit += 2;
			break;
		case RC5_OZO_BITS:
			rc_dev->record.rc5_data <<= 2;
			rc_dev->record.rc5_data |= 2;
			rc_dev->record.next_bit_is_zero = 0;
			rc_dev->record.bit += 2;
			break;
		default:	/* Error occurs */
			rc_dev->record.bit = 0;
			rc_dev->record.next_bit_is_zero = 0;
			rc_dev->bit_err = 1;
	}

	if (rc_dev->record.bit == 14) {	/* Ok, we received all bits */

		rc_dev->record.addr = (rc_dev->record.rc5_data & 0x07c0) >> 6;
		rc_dev->record.cmd = rc_dev->record.rc5_data & 0x003f;
		toggle = (rc_dev->record.rc5_data & 0x0800) ? 1: 0;

		PDEBUG ("bit : %d, rc5_data: 0x%4x, toggle: %x, addr %x, cmd %x\n",
				rc_dev->record.bit, rc_dev->record.rc5_data, toggle, rc_dev->record.addr, rc_dev->record.cmd);

#if 0
		/* Handle repeat */
		if (rc_dev->record.pre_T == toggle && rc_dev->record.rpt++ != 0) {
			if (rc_dev->record.rpt < 3) {
				rc_dev->record.bit = 0;
				rc_dev->record.next_bit_is_zero = 0;
				return 0;
			} else {
				rc_dev->record.pre_T = toggle;
			}
		}
#endif

		rc_dev->record.rpt = 0;
		rc_dev->record.bit = 0;
		rc_dev->record.next_bit_is_zero = 0;
		return 1;
	}

	return 0;
}

static unsigned int cnc_rc6_mode6a_decode_bit(unsigned int high_val, unsigned int total_val)
{
	int ret = RC6_STATE_ERROR;

    return ret;
}

/* this part is defined by OEM */
static unsigned int cnc_rc6_mode6a_decode(int idx)
{
	return 0;
}

#if 0
static unsigned int cnc_rcmm_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}
static unsigned int cnc_recs80_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}
static unsigned int cnc_rca_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}
static unsigned int cnc_xsat_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}

static unsigned int cnc_jvc_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}

static unsigned int cnc_nrc17_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}

static unsigned int cnc_sharp_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}
static unsigned int cnc_sirc_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}

static unsigned int cnc_itt_decode(struct cnc_rc *rc)
{
	unsigned int ret;
	PDEBUG("The protocol type %d is not a valid type\n",cnc_rc_gbl->protocal);
	ret=0;
	return ret;
}
#endif


static void rc_do_tasklet(unsigned long data)
{
	unsigned long flags;
	int idx;
	int got_data = 0;

#ifdef CONFIG_RC_DEBUG	/* Print the fifo count */
	int i;
	for (i = 0; i < rc_dev->fc_idx; i++)
		PDEBUG("  FIFO COUNT fifo1_cnt: %d, fifo2_cnt: %d\n", rc_dev->fc[i][0], rc_dev->fc[i][1]);
	spin_lock_irqsave(&rc_dev->lock, flags);
	rc_dev->fc_idx = 0;
	spin_unlock_irqrestore(&rc_dev->lock, flags);
	PDEBUG("***************  f1_idx: %d, f2_idx: %d ***************\n", rc_dev->f1_idx, rc_dev->f2_idx);
#endif

	for (idx = 0; idx < rc_dev->f2_idx || idx< rc_dev->f1_idx; idx++) {
		if (rc_dev->bit_err)
			break;

		switch (rc_dev->protocal) {
			case NEC:
				got_data = cnc_nec_decode(idx);
				break;
			case RC5:
				got_data = cnc_rc5_decode(idx);
				break;
			case RC6_6A:
				got_data = cnc_rc6_mode6a_decode(idx);
				break;
#if 0
			case TYPE_RCMM:
				decoder_status=cnc_rcmm_decode(rc_dev);
				break;
			case TYPE_RECS80:
				decoder_status=cnc_recs80_decode(rc_dev);
				break;
			case TYPE_RCA:
				decoder_status=cnc_rca_decode(rc_dev);
				break;
			case TYPE_XSAT:
				decoder_status=cnc_xsat_decode(rc_dev);
				break;
			case TYPE_JVC:
				decoder_status=cnc_jvc_decode(rc_dev);
				break;
			case TYPE_NRC17:
				decoder_status=cnc_nrc17_decode(rc_dev);
				break;
			case TYPE_SHARP:
				decoder_status=cnc_sharp_decode(rc_dev);
				break;
			case TYPE_SIRC:
				decoder_status=cnc_sirc_decode(rc_dev);
				break;
			case TYPE_ITT:
				decoder_status=cnc_itt_decode(rc_dev);
				break;
#endif
			default:
				PDEBUG("The protocol type %d is not a valid type\n",rc_dev->protocal);
		}

		/* FIXME: this value should be determined outside */
		if(got_data) {
			PDEBUG("\nYES YES YES Got it! bit: %x rpt: %x addr: %x, cmd: %x\n\n",
					rc_dev->record.bit, rc_dev->record.rpt, rc_dev->record.addr, rc_dev->record.cmd);

			if ((0xffffffff == rc_system_code) ||
					((rc_dev->record.addr & 0xff)   == rc_system_code) ||
					((rc_dev->record.addr & 0xff00) == rc_system_code) ||
					(rc_dev->record.addr == rc_system_code)) {
				input_report_key(rc_dev->dev, rc_dev->record.cmd, 1);
				input_report_key(rc_dev->dev, rc_dev->record.cmd, 0);
				input_sync(rc_dev->dev);
			}
		}
	}

	spin_lock_irqsave(&rc_dev->lock, flags);
	/* When data is handled, clear the fifo, and reset err state */
	rc_dev->bit_err = 0;
	rc_dev->f1_idx = rc_dev->f2_idx = 0;
#if 0
	memset(rc_dev->fifo1, 0, ARRAY_SIZE(rc_dev->fifo1));
	memset(rc_dev->fifo2, 0, ARRAY_SIZE(rc_dev->fifo2));
#endif
	spin_unlock_irqrestore(&rc_dev->lock, flags);
}

static irqreturn_t rc_dev_interrupt(int irq, void *dev_id)
{
	unsigned long  flags;
	int intr_status;
	int fifo1_cnt, fifo2_cnt;

	intr_status = fpc_readw(FPC_INTS_L);
	if ((intr_status & 0x1) || (intr_status & 0x4)) {	/* rc available | timeout */
		spin_lock_irqsave(&rc_dev->lock, flags);

		fifo1_cnt = fpc_readw(FPC_RC_FIFO1_CNT_L) & 0xf;
		fifo2_cnt = fpc_readw(FPC_RC_FIFO2_CNT_L) & 0xf;

		if (fifo1_cnt >=  RC_OVERFLOW_FLAG || fifo2_cnt >= RC_OVERFLOW_FLAG) {
			while(fifo1_cnt --) {
				fpc_readw(FPC_RC_FIFO1_L);
			}

			while(fifo2_cnt --) {
				fpc_readw(FPC_RC_FIFO2_L);
			}
			rc_dev->f1_idx = 0;
			rc_dev->f2_idx = 0;
		} else {
			while(fifo1_cnt --) {
				rc_dev->fifo1[rc_dev->f1_idx++] = fpc_readw(FPC_RC_FIFO1_L);
				if (rc_dev->f1_idx == FIFO_LEN) rc_dev->f1_idx = 0;
			}
			while(fifo2_cnt --) {
				rc_dev->fifo2[rc_dev->f2_idx++] = fpc_readw(FPC_RC_FIFO2_L);
				if (rc_dev->f2_idx == FIFO_LEN) rc_dev->f2_idx = 0;
			}
		}

		rc_dev->tasklet.data = (unsigned int)0; /* FIXME: to be modified */
		tasklet_schedule(&rc_dev->tasklet);

		spin_unlock_irqrestore(&rc_dev->lock, flags);

	} else if (intr_status & 0x2) {
		static unsigned long ticks = 0;
		unsigned long curr_ticks = jiffies;

		if (curr_ticks - ticks > 23) {
			rc_dev->record.cmd = fpc_readw(FPC_KSCAN_DATA_L) & 0xff;
			rc_dev->record.cmd |= 0x100;

			if (rc_dev->record.cmd & 0x80) {
				spin_lock_irqsave(&rc_dev->lock, flags);

				input_report_key(rc_dev->dev, rc_dev->record.cmd, 1);
				input_report_key(rc_dev->dev, rc_dev->record.cmd, 0);
				input_sync(rc_dev->dev);

				spin_unlock_irqrestore(&rc_dev->lock, flags);
			}

			ticks = curr_ticks;
		}
	}

	return IRQ_HANDLED;
}

static int cs_fpc_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	unsigned long flags,userbit,Key_Val;
	unsigned short rc_ctl, btc;
	//    unsigned long *arg_temp = (unsigned long *)arg;
	switch (cmd) {
		case FPC_LED_EN:
			spin_lock_irqsave(&cs_fpc_lock, flags);

            fpc_writew(0!=arg ? 0x8000:0x0000, FPC_LED_CNTL_H);
            fpc_writew(0xff, FPC_LED_CNTL_L);
            pinmux_update_fpc_status();

			spin_unlock_irqrestore(&cs_fpc_lock, flags);

			break;

		case FPC_LED_DISP:
			spin_lock_irqsave(&cs_fpc_lock, flags);

			fpc_writew((arg >> 16) & 0xffff, FPC_LED_DATA_H);
			fpc_writew(arg & 0xffff, FPC_LED_DATA_L);

			spin_unlock_irqrestore(&cs_fpc_lock, flags);

			break;
		case FPC_LED_GET:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			userbit = fpc_readw(FPC_LED_DATA_H);
			userbit = userbit << 16;
			userbit |= fpc_readw(FPC_LED_DATA_L);
            spin_unlock_irqrestore(&cs_fpc_lock, flags);

			copy_to_user((unsigned long *)arg, &userbit, 4);
			break;

		case FPC_RC_SET_SYSTEMCODE:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			rc_system_code = arg;
			spin_unlock_irqrestore(&cs_fpc_lock, flags);

			break;
		case FPC_KEYSCAN_EN:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			fpc_writew(arg & 0xffff,FPC_KSCAN_CNTL_L);
			fpc_writew((arg << 15) & 0xffff,FPC_LED_CNTL_H);
			spin_unlock_irqrestore(&cs_fpc_lock, flags);
            pinmux_update_fpc_status();
			break;
		case FPC_KSCAN_GET:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			Key_Val = fpc_readw(FPC_KSCAN_DATA_H);
			Key_Val = Key_Val << 16;
			Key_Val |= fpc_readw(FPC_KSCAN_DATA_L);
			spin_unlock_irqrestore(&cs_fpc_lock, flags);

			copy_to_user((unsigned long*)arg, &Key_Val, 4);
			break;
		case FPC_RC_EN:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			rc_ctl = fpc_readw(FPC_RC_CTL_H);
			if (arg == 1){
				fpc_writew(rc_ctl | 0x8000, FPC_RC_CTL_H);
                //			cnc_gpio_register_module_set("RC", 1);
   			}
			else if (arg == 0){
				fpc_writew(rc_ctl & 0x7fff,FPC_RC_CTL_H);
                //		cnc_gpio_register_module_set("RC", 0);
			}
            pinmux_update_rc_status();
			spin_unlock_irqrestore(&cs_fpc_lock, flags);
			break;
		case FPC_SET_BitTimeCnt:
			spin_lock_irqsave(&cs_fpc_lock, flags);

			rc_ctl = fpc_readw(FPC_RC_CTL_L);
			rc_ctl &= 0x00ff; rc_ctl |= (arg & 0xff) << 8;
			fpc_writew(rc_ctl, FPC_RC_CTL_L);

			rc_ctl = fpc_readw(FPC_RC_CTL_H);
			rc_ctl &= 0xff00; rc_ctl |= (arg >> 8) & 0xff;
			fpc_writew(rc_ctl, FPC_RC_CTL_H);

			spin_unlock_irqrestore(&cs_fpc_lock, flags);
			break;
		case FPC_GET_BitTimeCnt:
			spin_lock_irqsave(&cs_fpc_lock, flags);
			rc_ctl = fpc_readw(FPC_RC_CTL_L);
			btc = (rc_ctl & 0xff00) >> 8;

			rc_ctl = fpc_readw(FPC_RC_CTL_H);
			btc |= (rc_ctl & 0xff) << 8;
			spin_unlock_irqrestore(&cs_fpc_lock, flags);

			copy_to_user((unsigned long*)arg, &btc, 2);
			break;

        case FPC_RC_SET_PROTOCAL:
            spin_lock_irqsave(&rc_dev->lock, flags);
            rc_dev->protocal = arg;
        	spin_unlock_irqrestore(&rc_dev->lock, flags);
            break;
		default:
			break;
	}

	return 0;
}

static struct file_operations fpc_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= cs_fpc_ioctl,
};

static struct miscdevice fpc_miscdev = {
	.name = "cs_fpc",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &fpc_fops
};

static int fpc_proc_write(struct file *file,
                const char *buffer, unsigned long count, void *data)
{
	u32 addr;
	u16 val;

    const char *cmd_line = buffer;;

    if (strncmp("rw", cmd_line, 2) == 0) {
		addr = simple_strtol(&cmd_line[3], NULL, 16);
        val = fpc_readw(addr);
        printk(" readl [0x%04x] = 0x%04x \n", addr, val);
    } else if (strncmp("ww", cmd_line, 2) == 0) {
        addr = simple_strtol(&cmd_line[3], NULL, 16);
        val = simple_strtol(&cmd_line[9], NULL, 16);
        fpc_writew(val, addr);
        pinmux_update_rc_status();
        pinmux_update_fpc_status();
    } else if (strncmp("reboot", cmd_line, 6) == 0) {
        printk("reboot the systme ....\n ");
        arch_reset(0,0);
    } else if (strncmp("st", cmd_line, 2) == 0) {	/* set time ajust value */
		int entry;
		int hl, hh, tl, th;
		entry = simple_strtol(&cmd_line[3], NULL, 10);

		hl = simple_strtol(&cmd_line[5], NULL, 10);
		hh = simple_strtol(&cmd_line[11], NULL, 10);
		tl = simple_strtol(&cmd_line[17], NULL, 10);
		th = simple_strtol(&cmd_line[23], NULL, 10);

		pta_nec[entry].h_low = hl; /* pta_nec: protocal time ajust */
		pta_nec[entry].h_high = hh;
		pta_nec[entry].t_low = tl;
		pta_nec[entry].t_high = th;
	} else if (strncmp("rt", cmd_line, 2) == 0) {	/* read time ajust value */
		int entry;
		entry = simple_strtol(&cmd_line[3], NULL, 10);
		printk (
				"pta: pta[%d].h_low  = %d\n"
				"pta: pta[%d].h_high = %d\n"
				"pta: pta[%d].t_low  = %d\n"
				"pta: pta[%d].t_high = %d\n",
				entry, pta_nec[entry].h_low,
				entry, pta_nec[entry].h_high,
				entry, pta_nec[entry].t_low,
				entry, pta_nec[entry].t_high
		       );
	} else if (strncmp("sp", cmd_line, 2) == 0) {	/* set remote controller protocals */
		int protocal;
		protocal = simple_strtol(&cmd_line[3], NULL, 10);
		rc_dev->protocal = protocal;
	} else if (strncmp("rpt", cmd_line, 3) == 0) {	/* set remote controller protocals */
		val = simple_strtol(&cmd_line[4], NULL, 10);
		rc_dev->max_rpt = val;
	} else if (strncmp("dbg", cmd_line, 3) == 0) {	/* set remote controller protocals */
		if (RC_DEBUG) 	RC_DEBUG = 0;
		else 		RC_DEBUG = 1;
	} else if (strncmp("syscode", cmd_line, 7) == 0) {
		printk("system code: 0x%x\n", rc_dev->record.addr);
	} else if (strncmp("rms", cmd_line, 3) == 0) {
        printk("repeat ms is %d.\n", rc_dev->repeat_ms);
	} else if (strncmp("wms", cmd_line, 3) == 0) {
    	val = simple_strtol(&cmd_line[4], NULL, 10);
        if (val > 0 && val < 1000) {
            printk("set repeat ms to %d.\n", val);
    	    rc_dev->repeat_ms = val;
        }
        else {
            printk("repeat ms must less than 1000.\n");
        }
	}
	else {
		printk(KERN_ERR "Illegal command\n");
	}

    return count;
}

int __init cs_fpc_init(void)
{
    int i;
    int ret = 0;
    unsigned short dummy;
    unsigned short fir_psn;
    unsigned int rc_ctl;

    if (request_irq(CS_RC_IRQ, rc_dev_interrupt, IRQF_DISABLED, "cs_fpc", NULL)) {
        ret = -EBUSY;
        goto out;
    }

    if (!request_mem_region(CS_FPC_BASE, CS_FPC_SIZE, "CNC FPC")) {
        ret = -EBUSY;
        goto err1;
    }

    if (!(cs_fpc_base = (unsigned short *)ioremap(CS_FPC_BASE, CS_FPC_SIZE))) {
        ret = -EIO;
        goto err2;
    }

    if (misc_register(&fpc_miscdev)) {
        ret = -EBUSY;
        goto err3;
    }

	if (!(fpc_proc_entry = create_proc_entry("fpc_io", 0, NULL))) {
        ret = -EFAULT;
        goto err4;
	} else {
	    fpc_proc_entry->write_proc = fpc_proc_write;
	}

    if (!(rc_dev = kzalloc(sizeof(cs_rc_t), GFP_KERNEL))) {
        ret = -ENOMEM;
        goto err5;
    }

    //init_input_dev(&rc_dev->dev);
    rc_dev->dev = input_allocate_device();
    if (rc_dev->dev == NULL){
        return -ENOMEM;
        goto err6;
    }

    //rc_dev->dev = input;
    rc_dev->dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
    rc_dev->dev->keycode = rc_dev->keycode;
    rc_dev->dev->keycodesize = sizeof(unsigned short);
    rc_dev->dev->keycodemax = ARRAY_SIZE(rc_dev->keycode);
    //  rc_dev->dev->private = rc_dev;

    for (i = 0; i < CS_FPC_KNUM; i++)            /* Output RAW Code */
        rc_dev->keycode[i] = i;

    for (i = 0; i < CS_FPC_KNUM; i++)
        set_bit(rc_dev->keycode[i], rc_dev->dev->keybit);

    rc_dev->dev->name = DRIVER_DESC;
    rc_dev->dev->phys = "cs_rc";
    rc_dev->dev->id.bustype = BUS_HOST;
    rc_dev->dev->id.vendor = 0x0001;
    rc_dev->dev->id.product = 0x0001;
    rc_dev->dev->id.version = 0x0104;

    ret = input_register_device(rc_dev->dev);
    if (ret < 0) {
        goto err7;
    }

    /* default repeat is too fast */
    rc_dev->dev->rep[REP_DELAY]   = 500;
    rc_dev->dev->rep[REP_PERIOD]  = 100;
    rc_dev->max_rpt = 3;    /* real repeat every 2 repeat */

    /* init spin lock */
    spin_lock_init(&rc_dev->lock);

    /* init tasklet */
    rc_dev->tasklet.func = rc_do_tasklet;
    {
        rc_dev->protocal = NEC;

        rc_dev->repeat_ms = 150;    /* for RC6_6A */

        /* Remote Controller config */
        fpc_writew(0x0, FPC_INTM_L); /* Intr mask */

        /* Noise filter */
        fir_psn = FILTER_PRECISION * 27000 / PCLK_KHZ;
        fpc_writew(fir_psn, FPC_FIR_L);  /* Filter precision (0xe0*47.25)/27 = 450us */
        fpc_writew(0x0031, FPC_FIR_H);   /* Devider */

        /* Interrupt count */
        fpc_writew(0x4, FPC_RC_INTR_CNT_L); /* time_out << 4 | const_count */
        fpc_writew(0x3a94, FPC_RC_INTR_CNT_H); /* time out count */

        /* RC control */
        rc_ctl = (RC_CTL_OPTION<<29) | (BITTIME_CNT<<8) | (PANNEL_SELECT<<2);
        fpc_writew((rc_ctl<<16)>>16, FPC_RC_CTL_L); /* bittime_cnt & pannel_select ; 0x372c */
        /* noise filter must be enabled, or else no interrupt */
        fpc_writew(rc_ctl>>16, FPC_RC_CTL_H);   /* 00a, rc_en | rc_pol | noise_filter_en | bittime_cnt ; 0xe000 */

#if 0
        /* temparary not set */
        fpc_writew(0x1, RC_TIME_INTERVAL_L); /* RC_TIME_INTERVAL */
#endif
        dummy = fpc_readw(FPC_RC_FIFO1_L);
        PDEBUG("Throw out useless data: fifo1_cnt: %d, fifo2_cnt: %d , dummy: %d\n",
                fpc_readw(FPC_RC_FIFO1_CNT_L), fpc_readw(FPC_RC_FIFO2_CNT_L), dummy);

	/* KeyScan config  */
        //fpc_writew(0x0001, FPC_KSCAN_CNTL_L);    /* enable keyscan */
        fpc_writew(0 & 0xffff, FPC_KSCAN_CNTL_L);
        fpc_writew((0 << 15) & 0xffff, FPC_LED_CNTL_H);

        pinmux_update_rc_status();
        pinmux_update_fpc_status();
    }

    fpc_writew(0xff, FPC_LED_CNTL_L);

    goto out;

err7:
    input_free_device(rc_dev->dev);
err6:
    kfree(rc_dev);
err5:
    remove_proc_entry("rc_io", NULL);
err4:
    misc_deregister(&fpc_miscdev);
err3:
	iounmap((void*)cs_fpc_base);
err2:
    release_mem_region(CS_FPC_BASE, CS_FPC_SIZE);
err1:
    free_irq(CS_RC_IRQ, NULL);
out:
	return ret;

}

static void __exit cs_fpc_exit(void)
{
    input_free_device(rc_dev->dev);
    kfree(rc_dev);
    remove_proc_entry("rc_io", NULL);
    misc_deregister(&fpc_miscdev);
	iounmap((void*)cs_fpc_base);
    release_mem_region(CS_FPC_BASE, CS_FPC_SIZE);
    free_irq(CS_RC_IRQ, NULL);
}

module_init(cs_fpc_init);
module_exit(cs_fpc_exit);

EXPORT_SYMBOL(rc_system_code);

