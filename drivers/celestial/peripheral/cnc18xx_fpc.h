#ifndef _CS_FPC_H_
#define _CS_FPC_H_

#include <mach/hardware.h>

#define FPC_MAGIC 'z'

#define FPC_LED_EN	 	        _IOW(FPC_MAGIC, 0x06, int)
#define FPC_LED_DISP	 	    _IOW(FPC_MAGIC, 0x08, int)
#define FPC_RC_SET_SYSTEMCODE	_IOW(FPC_MAGIC, 0x09, int)
#define FPC_LED_GET		        _IOR(FPC_MAGIC, 0x0a, int)
#define FPC_KEYSCAN_EN          _IOW(FPC_MAGIC, 0x0b, int)
#define FPC_KSCAN_GET		    _IOR(FPC_MAGIC, 0x0c, int)
#define FPC_RC_SET_PROTOCAL     _IOR(FPC_MAGIC, 0x10, int)

/* Adjust bit time count outside for nonstandard rc protocal */
#define FPC_SET_BitTimeCnt	_IOR(FPC_MAGIC, 0x0d, int)
#define FPC_GET_BitTimeCnt	_IOR(FPC_MAGIC, 0x0e, int)

#define FPC_RC_EN      _IOW(FPC_MAGIC, 0x14, int)


/* Register Map */
#define FPC_INTS_L		         (0x000)    /*Interrupt Status Register*/
#define FPC_INTS_H		         (0x002)
#define FPC_INTM_L		         (0x004)    /*Interrupt Mask Register*/
#define FPC_INTM_H		         (0x006)
#define FPC_RC_CTL_L             (0x008)    /*FPC Control Register*/
#define FPC_RC_CTL_H             (0x00a)
#define FPC_RC_TIME_INTERVAL_L   (0x00c)    /*FPC Time Interval Register*/
#define FPC_RC_TIME_INTERVAL_H   (0x00e)
#define FPC_UHF_CTL_L            (0x010)    /*UHF control Register*/
#define FPC_UHF_CTL_H            (0x012)
#define FPC_UHF_TIME_INTERVAL_L  (0x014)    /*UHF Time Interval  Register*/
#define FPC_UHF_TIME_INTERVAL_H	 (0x016)
#define FPC_RC_INTR_CNT_L        (0x018)    /*FPC Interrupt counter Register*/
#define FPC_RC_INTR_CNT_H        (0x01a)
#define FPC_UHF_INTR_CNT_L       (0x01c)    /*UHF Interrupt counter Register*/
#define FPC_UHF_INTR_CNT_H       (0x01e)
#define FPC_RC_FIFO1_L           (0x020)    /*FPC FIFO1*/
#define FPC_RC_FIFO1_H           (0x022)
#define FPC_RC_FIFO1_CNT_L       (0x024)    /*FPC FIFO1 count*/
#define FPC_RC_FIFO1_CNT_H       (0x026)
#define FPC_RC_FIFO2_L           (0x028)    /*FPC FIFO2*/
#define FPC_RC_FIFO2_H           (0x02a)
#define FPC_RC_FIFO2_CNT_L       (0x02c)    /*FPC FIFO2 count*/
#define FPC_RC_FIFO2_CNT_H       (0x02e)
#define FPC_UHF_FIFO1_L          (0x030)    /*UHF FIFO1 */
#define FPC_UHF_FIFO1_H          (0x032)
#define FPC_UHF_FIFO1_CNT_L      (0x034)    /*UHF FIFO1 Count*/
#define FPC_UHF_FIFO1_CNT_H      (0x036)
#define FPC_UHF_FIFO2_L          (0x038)    /*UHF FIFO2 */
#define FPC_UHF_FIFO2_H          (0x03a)
#define FPC_UHF_FIFO2_CNT_L      (0x03c)    /*UHF FIFO2 Count*/
#define FPC_UHF_FIFO2_CNT_H      (0x03e)
#define FPC_FIR_L                (0x040)    /*FPC FIR Register*/
#define FPC_FIR_H                (0x042)
#define FPC_UHF_FIR_L            (0x044)    /*UHF FIR Register*/
#define FPC_UHF_FIR_H            (0x046)
#define FPC_LED_DATA_L           (0x200)    /*LED DATA Register*/
#define FPC_LED_DATA_H           (0x202)
#define FPC_LED_CNTL_L           (0x204)    /*LED Counter Register*/
#define FPC_LED_CNTL_H           (0x206)    /*LED Counter Register*/
#define FPC_KSCAN_DATA_L         (0x300)    /*KSCAN DATA Register*/
#define FPC_KSCAN_DATA_H         (0x302)
#define FPC_KSCAN_CNTL_L         (0x304)    /*KSCAN Counter Register*/
#define FPC_KSCAN_CNTL_H         (0x306)
#define FPC_KSCAN_ARB_CNTL_L     (0x308)    /*KSCAN Arbitrates Register*/
#define FPC_KSCAN_ARB_CNTL_H     (0x30a)

#define CS_FPC_BASE		        PA_FPC_BASE
#define CS_FPC_SIZE             0x1000
#define CS_RC_IRQ		        IRQ_PANEL_CTL

#define FILTER_PRECISION 	(450)

#define RC_CTL_OPTION 		( 7) 	/* rc_en | rc_pol | noise_filter_en */
#define BITTIME_CNT 		(PCLK_MHZ)
#define PANNEL_SELECT 		(11) 	/* GPIO input for remote controller */

#define DRIVER_DESC		    "CS Remote Controller"

/* Key number */
#define CS_FPC_KNUM 		KEY_MAX

/* Parse period */
#define HEAD_PERIOD		0
#define ADDR_PERIOD		1
#define CMD_PERIOD		2

#undef PDEBUG

//#define CONFIG_FPC_DEBUG
static int RC_DEBUG = 0;
#ifdef CONFIG_FPC_DEBUG
# ifdef __KERNEL__
#  define PDEBUG(fmt, args...) printk( fmt, ## args)
# else
#  define PDEBUG(fmt, args...) printf(fmt, ## args)
# endif
#else
# define PDEBUG(fmt, args...) \
	if (RC_DEBUG) printk( fmt, ## args)
#endif

/* Infrared Remote Control Protocol type */
typedef enum rc_protocal_ {
	NEC, RC5, RC6_6A,
#if 0
	FPCMM, RECS80, FPCA, XSAT, JVC, NFPC17, SHARP, SIFPC, ITT,
#endif
	P_TOTAL
} rc_protocal_t;

typedef struct rc_record_ {
	unsigned short addr;
	unsigned short cmd;
	unsigned short bit;
	unsigned short rpt;

	unsigned short decode_period;

	unsigned short rc5_data;
	unsigned short next_bit_is_zero;
	unsigned short pre_T;      /* Only FPC5 use */

    /* for rc6 mode 6a */
    unsigned char mode;
    unsigned char half_bit; // 0 no half bit left, 1 have half bit of ONE or ZERO, 2 have half bit of T0 or T1
} rc_record_t;

typedef struct cs_rc_ {
	spinlock_t lock;
	struct tasklet_struct tasklet;
	struct input_dev *dev;

	unsigned short keycode[CS_FPC_KNUM];
	rc_record_t record;

	/* To be set outside, used as condition restriction */
	unsigned short addr;
	unsigned short max_rpt;
	unsigned short protocal;

#define FIFO_LEN 80
	unsigned short fifo1[FIFO_LEN];
	unsigned short fifo2[FIFO_LEN];
	short f1_idx;
	short f2_idx;

    /* for FPC6 mode 6A */
    unsigned short last_report_cmd;
    unsigned long last_report_jiffies;
    unsigned repeat_ms;        /* can set by writing proc file */

#ifdef CONFIG_FPC_DEBUG
	unsigned short fc[32][2];	/* For debug only */
	short fc_idx;
#endif

	int bit_err;
} cs_rc_t;

//extern int cnc_gpio_register_module_status(const char * module, unsigned int  orion_module_status); /* status: 0 disable, 1 enable */

#endif

