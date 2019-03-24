//#include <linux/config.h>
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <linux/semaphore.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

MODULE_AUTHOR("Celestial");
MODULE_DESCRIPTION("Cavium Celestial SoC Smart card driver");
MODULE_LICENSE("GPL");



#define SCI_MINOR			160
#define SCI_NUM				1

//#define SCI_BASE_ADDR0		0X101F0000   

//#define SCI_BASE_SIZE		0x1000

#define SCI_IRQ				IRQ_SMART_CART
#define SCI_MAX_FIFOSIZE    32

#define SCI_REF_CLK         PCLK_FREQ


/*
 * Debugging macro and defines
 */

#define SCI_DEBUG_LEVEL0	(0)	/* Quiet   */
#define SCI_DEBUG_LEVEL1	(1)	/* Audible */
#define SCI_DEBUG_LEVEL2	(2)	/* Loud    */
#define SCI_DEBUG_LEVEL3	(3)	/* Noisy   */

#ifdef CONFIG_CELESTIAL_SCI_DEBUG
#define DEBUG(n, args...)				\
 	do {						\
		if (n <= CONFIG_SCI_DEBUG_VERBOSE)	\
			printk(KERN_INFO args);		\
	} while(0)
#else /* CONFIG_SCI_DEBUG */
#define DEBUG(n, args...) do { } while(0)

#endif /* CONFIG_SCI_DEBUG */

static volatile unsigned short *sci_base = NULL;

//static int irq_sci_cardin = 0;	/*Flag to check the interrupt of card insert added by tata team*/
//static int irq_sci_cardout = 0;  /*Flag to check the interrupt of card remove added by tata team*/
#define sci_get_regs(reg, ch) \
	(*(volatile unsigned short*)(sci_base + (int)(ch)*(0x3000) + (int)(reg >> 1)))
#define sci_set_regs(reg, ch, v) \
	(*(volatile unsigned short*)(sci_base + (int)(ch)*(0x3000) + (int)(reg >> 1)) = (unsigned short)(v))

#define SCI_DATA			0x00 /* data register */

#define SCI_CR0				0x04 	/* control register 0 */
#define CR0_INIT_VALUE  		0x00 //0x28
#define CR0_SENSE			(1<<0) 	/* inverts sense of input/output line for data and parity bits */
									/* 0 - direct convention */
									/* 1 - inverse convention */
#define CR0_ORDER			(1<<1) 	/* specifies ordering of the data bits */
									/* 0 - LOW interpreted as logic 0, LSB (direct convention) */
									/* 1 - LOW interpreted as logic 1, MSB (inverse convention) */
#define CR0_TXPARITY		(1<<2)	/* transmit parity setting */
									/* 0 - even parity */
									/* 1 - odd parity */
#define CR0_TXNAK			(1<<3)	/* enables character transmission handshaking */
									/* 0 - the SCI does not check to see if the receiver 
									    has pulled the input/output line LOW to indicate a parity error
									    1 - the SCI checks, after each character has been transmitted, 
									    to see if the receiver has pulled the input/output line LOW to indicate a parity error */
#define CR0_RXPARITY		(1<<4)	/* receive parity setting 
									    0 - even parity 
									    1 - odd parity */
#define CR0_RXNAK			(1<<5)	/* enables character receipt handshaking
									    0 - the SCI does not pull the input/ouput line LOW if it detects a parity error
									    1 - the SCI pulls the input/output line LOW if it detects a parity error */
#define CR0_CLKDIS			(1<<6)	/* if the card supports clock stop mode, this bit ban be used to
									    stop and start the clock:
									    0 - clock start initiated
									    1 - clock stop initiated */
#define CR0_CLKVAL			(1<<7)	/* define the inactive state of the card clock when clock stop mode is supported:
									    0 - clock held LOW when inactive
									    1 - clock held HIGH when inactive */
									
#define SCI_CR1				0x08 	/* control register 1 */
#define CR1_INIT_VALUE  		0x1
#define CR1_VAL				0x001b	/* ... */
#define CR1_BLKEN           (1<<1) /* interface block time out 
                                       0 -disable
                                       1 -enable */

#define CR1_MOD				(1<<2)	/* interface direction of communication control:
									    0 -receive 
									    1 - transmit */
#define CR1_BGTEN           (1<<4) /* interface block guard timer enable 
                                       0 -disable
                                       1 -enable */


#define SCI_CR2				0x0c	/* control register 2 */
#define CR2_STARTUP			(1<<0)	/* writing a 1 to this bit starts the activation of the card */
#define CR2_FINISH			(1<<1)	/* writing a 1 to this bit deactivates the card */
#define CR2_WRESET			(1<<2)	/* writing a 1 to this bit initiates a warm reset */

#define SCI_CLKICC			0x10 	/* external smart card clock frequency */
#define CLKICC_INIT_VAL			0x1a 	/* define the smart card clock frequency */

#define SCI_VALUE			0x14 	/* Number of SCIBAUD cycles per etu */
#define VALUE_INIT_VAL			0x08	/* defines the number of SCIBAUD cycles per etu */

#define SCI_BAUD			0x18 	/* Baud rate clock divisor value */
#define BAUD_INIT_VAL			0x9ce	/* the divide valud used to define the baudrate clock frequency */

#define SCI_TIDE			0x1c 	/* Transmit and eceive FIFO water level marks */
#define TIDE_INIT_RX			0x10 	/* trigger point for SCIRXTIDEINTR */
#define TIDE_INIT_TX			0x01 //0x00	/* trigger point for SCITXTIDEINTR */
#define TIDE_INIT_VAL      TIDE_INIT_RX | TIDE_INIT_TX

#define SCI_DMACR			0x20 	/* Direct Memory Access control register */
#define DAMCR_VAL			0x00	/* no DMA support */

#define SCI_STABLE			0x24 	/* Card stable after insertion debounce duration */
#define STABLE_INIT_VAL			0x05 //0x54	/* stores the debounce time */

#define SCI_ATIME			0x28 	/* Card activation event time */
#define ATIME_INIT_VAL			0x9c40 //0xafc8	/* stores the time for each of the three stages of the card activation process time */
									    
									    
#define SCI_DTIME			0x2c	/* card deactivation event time */
#define DTIME_INIT_VAL			0x00c8	/*  stores the time for each of the three stages of the card 
									    deactivation process time */
									    
#define SCI_ATRSTIME		0x30 	/* Time to start of the ATR reception */
#define ATRSTIME_INIT_VAL		0xffff /*0xafc8*/ 	/*  ATR reception start timeout threshold from the assertion
									    or deassertion of nSCICARDRST */
									    
#define SCI_ATRDTIME		0x34 	/* Maximum allowed duration of the ATR character stream */
#define ATRDTIME_INIT_VAL		0xab00	/*0xffff*/	/* ATR reception duration timeout threshold from the start
									    bit of the first ATR character */
									    
#define SCI_STOPTIME		0x38 	/* Duration before the card clock can be stopped */
#define STOPTIME_VAL			0x00	/* FIXME! duration from card clock stop initiation to the card clock
									    becoming inactive, and the clock stopped interrupt SCICLKSTPINTR
									    becoming  asserted */

#define SCI_STARTTIME		0x3c 	/* Duration before transactions can commence after clock is restarted */
#define STARTTIME_VAL			0x00	/* FIXME! duration from the card clock start initiation to the card 
									    clock active interrupt SCICLKACTINTR becoming asserted */

#define SCI_RETRY			0x40 	/* Retry limit register */
#define RETRY_VAL				0x00	/* FIXME! [2:0] - specifies the maximum number of times that a 
									    char is retransmitted following the detection of a parity error by the card 
									    [5:3] - specifies the maximum number of retries to receive when a parity
									    error has occurred in reception */

#define SCI_CHTIMELS		0x44 	/* character to character timeout (least significant 16 bits) */
#define SCI_CHTIMEMS		0x48 	/* character to character timeout (most significant 16 bits) */
#define CHTIMELS_INIT_VAL		0x2580	/* FIXME! */
#define CHTIMEMS_INIT_VAL		0x00	/* FIXME! */

#define SCI_BLKTIMELS		0x4c 	/* Receive timeout between blocks (least significant 16 bits) */
#define SCI_BLKTIMEMS		0x50 	/* Receive timeout between blocks (most significant 16 bits) */
#define BLKTIMELS_VAL			0xfff0	/* FIXME! */
#define BLKTIMEMS_VAL			0x00	/* FIXME! */

#define SCI_CHGUARD			0x54 	/* character to character extra guard time */
/* 
    CHGUARD defines the extra guard time that is added to the minimum duration 
    between leading edges of the start bits of two consecutive characters, for subsequent 
    communication from the interface to the Smart Card.

    The CHGUARD valus is derived from the TC1 value that is extracted from the ATR character stream. 
    Writing a value to this register while the timeout is in progress causes the internal timer to be loaded
    with the new value, and  then cound down from this value.

    The TC1 value can be between 0 and 255. The software must read the TC1 value and program the 
    CHGUARD register as described in the follow table to provide the resultan guard time in etus 

               CHGUARD VALUE                 RESULTAN GUARD TIME (ETUS)
    --------------------------------------------------------------------------
                     T0           T1            T0                T1
    --------------------------------------------------------------------------
    0 =< TC1 < 255   TC1         TC1+1         TC1+12            TC1+11
    --------------------------------------------------------------------------
    255              0             0            12                11
    --------------------------------------------------------------------------
 */
#define CHGUARD_VAL			0x00	/* FIXME!*/

#define SCI_BLKGUARD		0x58 	/* block guard time */
/* 
   Defines the minimum time in etus between the leading edges of two consecutive characters sent 
   in opposite directions:
   1) For T0, the blkguard minimum time is 16 work etus.
   2) For T1, the blkguard minimum time is 22 work etus.

   NOTE:
   For T0, blkguard is effecitively offset by 12 etus internally, so the value to be programmed into the 
   blkguard register should be required time period in etus minus 12.
   For T1, blkguard is effecitively offset by 11 etus internally, so the value to be programmed into the 
   blkguard register should be required time period in etus minus 11.
*/
#define BLKGUARD_VAL		0x00	/* FIXME! */

#define SCI_RXTIME			0x5c 	/* receive read time-out register */
#define RXTIME_INIT_VAL			0xffff	/* receive read time out value */
									/* the following table list the range and resolution of the timeout 
									value for various smart card clock frequencies:
									FREQ/RANGE(APPROX)/RESOLUTION
									10MHZ/0 -6 ms/0.1us
									5 MHZ/0  -13ms/0.2us
									1 MHZ/0  -65ms/  1us
									500KHz/0-131ms/2us */

#define SCI_FIFOSTATUS		0x60 	/* transmit and receive FIFO status */
#define FIFOST_TXFF			(1<<0)	/* TXFIFO full status */
#define FIFOST_TXFE			(1<<1)	/* TXFIFO empty status */
#define FIFOST_RXFF			(1<<2)	/* RXFIFO full status */
#define FIFOST_RXFE			(1<<3)	/* RXFIFO empty status */

/* 
   SCITXCOUNT returns the number of characters (including any character currently being 
   transmitted) in the transmit FIFO when read, and flushes the transmit FIFO when written 
   (with any value)
   [15:4] reserved, do not modify, read as zero.
   [3:0] a read transmits the FIFO count. a write flushes the tranmit FIFO.
*/
#define SCI_TXCOUNT			0x64 	/* transmit FIFO fill level */

/* 
   SCIRXCOUNT returns the number of characters in the receive FIFO when read, 
   and flushes the receive FIFO when written (with any value)
   [15:4] reserved, do not modify, read as zero.
   [3:0] a read receive the FIFO count. a write flushes the receive FIFO.
*/
#define SCI_RXCOUNT			0x68 	/* receive FIFO fill level */

#define SCI_IMSC			0x6c 	/* interrupt mask set or clear mask */
#define SCI_RIS				0x70 	/* raw interrupt status register */
#define SCI_MIS				0x74 	/* masked interrupt status register */ 
#define SCI_ICR				0x78 	/* interrutp clear register */
#define INT_IN 				(1<<0)	/* card in interrupt */
#define INT_OUT 			(1<<1)	/* card out interrupt */
#define INT_PWUP 			(1<<2)	/* card power up interrupt */
#define INT_PWDN 			(1<<3)	/* card power down interrupt */
#define INT_TRANSERR 		(1<<4)	/* transmit error interrupt */
#define INT_ATRSTOUT 		(1<<5)	/* ATR start time out */
#define INT_ATRDTOUT 		(1<<6)	/* ATR data stream duration time out */
#define INT_BLKOUT 			(1<<7)	/* block time out interrupt */
#define INT_CHOUT 			(1<<8)	/* character time out interrupt */
#define INT_RDOUT 			(1<<9)	/* read time out interrupt */
#define INT_OVERRUN 		(1<<10)	/* overrun interrupt */
#define INT_CLKSTOP 		(1<<11)	/* clock stop interrupt */
#define INT_CLKACTIVE 		(1<<12)	/* clock active interrupt */
#define INT_RXTIDE 			(1<<13)	/* receive FIFO tide level interrupt */
#define INT_TXTIDE 			(1<<14)	/* transmit FIFO tide level interrupt */

/* 
  * the following 3 registers is not used for csm1201 chip 
 */
// DELME #define SCI_SYNCACT	0x7c 	/* synchronous mode activation register */
// DELME #define SCI_SYNCTX	0x80 	/* synchronous mode transmit clock and data stream register */
// DELME #define SCI_SYNCRX	0x84 	/* synchronous mode receive clock and data stream register */
#define SCI_IOC_MAGIC 'p'
#define SCI_IOC_WARMRESET			 _IOW(SCI_IOC_MAGIC, 0x01, int)
#define SCI_IOC_ISPRESENT			 _IOR(SCI_IOC_MAGIC, 0x02, int)
#define SCI_IOC_ACTIVE				 _IOW(SCI_IOC_MAGIC, 0x03, int)
#define SCI_IOC_DEACTIVE			 _IOW(SCI_IOC_MAGIC, 0x04, int)
#define SCI_IOC_CLKSTART			 _IOW(SCI_IOC_MAGIC, 0x05, int)
#define SCI_IOC_CLKSTOP				 _IOW(SCI_IOC_MAGIC, 0x06, int)
#define SCI_IOC_SELPROTOCOL 		 _IOW(SCI_IOC_MAGIC, 0x07, int)
#define SCI_IOC_RXINTTIGGER    		 _IOW(SCI_IOC_MAGIC, 0x08, int)
#define SCI_IOC_TXINTTIGGER    		 _IOW(SCI_IOC_MAGIC, 0x09, int)
#define SCI_IOC_SET_ETU     		 _IOW(SCI_IOC_MAGIC, 0x0a, int)
#define SCI_IOC_SET_SC_CLOCK     	 _IOW(SCI_IOC_MAGIC, 0x0b, int)
#define SCI_IOC_SET_TXRETRY    		 _IOW(SCI_IOC_MAGIC, 0x0c, int)
#define SCI_IOC_SET_RXRETRY    	     _IOW(SCI_IOC_MAGIC, 0x0d, int)
#define SCI_IOC_SET_CHGUARD 		 _IOW(SCI_IOC_MAGIC, 0x0e, int)
#define SCI_IOC_SET_BLOCKGUARD	 	 _IOW(SCI_IOC_MAGIC, 0x0f, int)
#define SCI_IOC_SET_RXTIMEOUT  		 _IOW(SCI_IOC_MAGIC, 0x10, int)
#define SCI_IOC_SET_CONVENTION 		 _IOW(SCI_IOC_MAGIC, 0x11, int)
#define SCI_IOC_SET_PARITY     		 _IOW(SCI_IOC_MAGIC, 0x12, int)
#define SCI_IOC_GET_RXRDYSIZE  	 _IOR(SCI_IOC_MAGIC, 0x13, int)
#define SCI_IOC_GET_TXRDYSIZE  	 _IOR(SCI_IOC_MAGIC, 0x14, int)
#define SCI_IOC_SET_RXFIFO_THRESHOLD _IOW(SCI_IOC_MAGIC, 0x15, int)
#define SCI_IOC_SET_TXFIFO_THRESHOLD _IOW(SCI_IOC_MAGIC, 0x16, int)
#define SCI_IOC_GET_RXFIFO_THRESHOLD _IOR(SCI_IOC_MAGIC, 0x17, int)
#define SCI_IOC_GET_TXFIFO_THRESHOLD _IOR(SCI_IOC_MAGIC, 0x18, int)
#define SCI_IOC_SET_RX_READREADY_REPORT _IOW(SCI_IOC_MAGIC, 0x19, int)
#define SCI_IOC_SET_TX_WRITEREADY_REPORT _IOW(SCI_IOC_MAGIC, 0x1a, int)
#define SCI_IOC_GET_BLOCKGUARD			_IOW(SCI_IOC_MAGIC, 0x1b, int)
#define SCI_IOC_GET_SC_CLOCK     	 _IOR(SCI_IOC_MAGIC, 0x1c, int)
#define SCI_IOC_GET_TXRETRY			_IOR(SCI_IOC_MAGIC, 0x1e, int)
#define SCI_IOC_GET_CHGUARD			_IOR(SCI_IOC_MAGIC, 0x1f, int)
#define SCI_IOC_GET_RXTIMEOUT			_IOR(SCI_IOC_MAGIC, 0x20, int)
#define SCI_IOC_GET_RXRETRY			_IOR(SCI_IOC_MAGIC, 0x22, int)
#define SCI_IOC_SET_TXRXNAK			_IOR(SCI_IOC_MAGIC, 0x23, int)

/*
 * loop queue definitions and implement 
 */
#define IFD_BUF_SIZE 512//8192

struct dat_queue {
		u32 head;
		u32 tail;
		u8  buf[IFD_BUF_SIZE];
};



#define INC(a) 			((a) = ((a)+1) & (IFD_BUF_SIZE-1))
#define DEC(a) 			((a) = ((a)-1) & (IFD_BUF_SIZE-1))
#define EMPTY(a) 		((a).head == (a).tail)
#define CHARS(a) 		(((a).head-(a).tail-1)&(IFD_BUF_SIZE-1))
#define LEFT(a) 		(((a).tail-(a).head)&(IFD_BUF_SIZE-1))
#define LAST(a) 		((a).buf[(IFD_BUF_SIZE-1)&((a).head-1)])
#define FULL(a) 		((a).head == (((a).tail+1) & (IFD_BUF_SIZE-1)))
#define GETCH(queue,c) 	(void)({c=(queue).buf[(queue).head];INC((queue).head);})
#define PUTCH(c,queue) 	(void)({(queue).buf[(queue).tail]=(c);INC((queue).tail);})

/* stats */
//static u16 frames_rx = 0;
//static u16 frames_tx = 0;
//static u16 frames_lost = 0;

/* flags */
static char card_inserted = 0;
static char card_noatr =0;
//static char clock_stopped = 0;
//static char flag_reverse = 0;
static char sci_channel = 0;
static char card_inout_change = 0;


#define RXTIMEOUT_DEFAULT 15
static char rxtimeout;

static int receive_exception =0;
static struct dat_queue rq;
static DECLARE_WAIT_QUEUE_HEAD(rq_queue);

static struct dat_queue tq;
static struct semaphore tq_sem;
static DECLARE_WAIT_QUEUE_HEAD(tq_queue);

//static DECLARE_WAIT_QUEUE_HEAD(CardInOut_queue);
/*
 * Feeding the output queue to the device is handled by a workqueue.
 */
//static void sci_do_work(void *);
//static DECLARE_WORK(sci_work, sci_do_work, NULL);
//static struct workqueue_struct *sci_workqueue;

/*
 * 
 */
static void sci_do_tasklet(unsigned long unused);
DECLARE_TASKLET(sci_tasklet, sci_do_tasklet, 0);

/*
 * The output "process" is controlled by a spin lock; decisions on
 * sci_output_active require that this lock be held.
 */
static spinlock_t sci_lock = SPIN_LOCK_UNLOCKED;
volatile static int sci_output_active;
volatile static int sci_input_active;

static unsigned char clkicc_value = 0;
static u16 baud_value = 0;

/* BUFFER status and event report releated*/
static spinlock_t sci_buffer_status_lock = SPIN_LOCK_UNLOCKED;
//static unsigned int sci_read_buffer_threshold = IFD_BUF_SIZE /2;
//static unsigned int sci_write_buffer_threshold =IFD_BUF_SIZE /2;
//static unsigned int sci_write_ready_report_en =0;
//static unsigned int sci_read_ready_report_en =0;
//static unsigned int sci_write_ready_report_flag =0;
//static unsigned int sci_read_ready_report_flag =0;

typedef struct{
int sci_buffer_threshold;
int sci_buffer_report_en;
int sci_buffer_report_flag;
} sci_buffer_t;
sci_buffer_t sci_rx_buffer={IFD_BUF_SIZE /2,0,1};
sci_buffer_t sci_tx_buffer={IFD_BUF_SIZE /2,0,1};
static DECLARE_WAIT_QUEUE_HEAD(sci_write_ready);
static DECLARE_WAIT_QUEUE_HEAD(sci_read_ready);

static DECLARE_WAIT_QUEUE_HEAD(sci_empty_queue); /* waked when queue empties */



static void _sci_clkstart(int ch)
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	sci_set_regs(SCI_CR0, ch, old_cr0_value & (~CR0_CLKDIS));
	return;
}


static void _sci_clkstop(int ch)
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_CLKDIS);
	return;
}


/*static void _sci_disable_blkguardtimer(int ch)
{
    u16 old_cr1_value;
	old_cr1_value = sci_get_regs(SCI_CR1, ch);
	sci_set_regs(SCI_CR1, ch, old_cr1_value & (~CR1_BGTEN));
	return;
}

static void _sci_enable_blkguardtimer(int ch)
{
	
    u16 old_cr1_value;
	old_cr1_value = sci_get_regs(SCI_CR1, ch);
	sci_set_regs(SCI_CR1, ch, old_cr1_value | CR1_BGTEN);
	return;
}*/

static unsigned char _sci_rxfifocount(int ch)
{
	return sci_get_regs(SCI_RXCOUNT, ch) & 0xff;  /* fixed by fxd from 0xf to 0xff. Because fifo size is 32 byte */ 
}

static void _sci_flush_rxfifo(int ch)
{
	sci_set_regs(SCI_RXCOUNT, ch, 0);
}

static void _sci_set_txint_trigger(int ch, unsigned char value ) /* invalid value is from 0 ~ 8 */
{
	unsigned char temp_value;
	if ( value > 8 )
		value = 8;
	temp_value = (sci_get_regs(SCI_TIDE, ch) & 0xf0) | value;
	sci_set_regs(SCI_TIDE, ch, temp_value);
}

static void _sci_set_rxint_trigger(int ch, unsigned char value ) /* invalid value is from 0 ~ 8 */
{
	unsigned char temp_value;
	if ( value > 8 )
		value = 8;
	temp_value = (sci_get_regs(SCI_TIDE, ch) & 0x0f) | (value << 4);
	sci_set_regs(SCI_TIDE, ch, temp_value);
}
static unsigned char _sci_txfifocount(int ch)
{
	return sci_get_regs(SCI_TXCOUNT, ch) & 0xff; /* fixed by fxd from 0xf to 0xff. Because fifo size is 32 byte */ 
}

static void _sci_flush_txfifo(int ch)
{
	sci_set_regs(SCI_TXCOUNT, ch, 0);
}

static void _sci_set_txrxmode(int ch,int mode) /*0: receive, 1:transmit */
{
	u16 old_cr1_value;
	old_cr1_value = sci_get_regs(SCI_CR1, ch);
	do {
		mode?sci_set_regs(SCI_CR1, ch, old_cr1_value | CR1_MOD):sci_set_regs(SCI_CR1, ch, old_cr1_value & (~CR1_MOD));
//		DEBUG(SCI_DEBUG_LEVEL3, "CR1 mode 0x%x \n", sci_get_regs(SCI_CR1, ch)& CR1_MOD);		
	}while((int)(sci_get_regs(SCI_CR1, ch)& CR1_MOD) != (mode << 2));
	
}

static inline unsigned char _sci_read_date(int ch)
{
	return sci_get_regs(SCI_DATA, 0) & 0xff;
}

static inline void _sci_write_date(int ch, unsigned char value)
{
	sci_set_regs(SCI_DATA, ch, value);	
	return;
}

static void _sci_set_rxretry(int ch, unsigned char retrytimes)
{
	u16 old_retry_value;
	if (retrytimes < 8){
		old_retry_value = sci_get_regs(SCI_RETRY, ch);	
		sci_set_regs(SCI_RETRY, ch, (old_retry_value & 0x7) | retrytimes << 3);
	}
}

static void _sci_set_txretry(int ch, unsigned char retrytimes)
{
	u16 old_retry_value;
	if (retrytimes < 8){
		old_retry_value = sci_get_regs(SCI_RETRY, ch);	
		sci_set_regs(SCI_RETRY, ch, (old_retry_value & 0x38) | retrytimes );
	}
}

static void _sci_get_rxretry(int ch, unsigned char *retrytimes)
{
	u16 retry_value;
	
	retry_value = sci_get_regs(SCI_RETRY, ch);	
	*retrytimes = (unsigned char)((retry_value & 0x38) >> 3);	
}

static void _sci_get_txretry(int ch, unsigned char *retrytimes)
{
	u16 retry_value;

	retry_value = sci_get_regs(SCI_RETRY, ch);	
	*retrytimes = (unsigned char)(retry_value & 0x7);
}

static void _sci_set_chguard(int ch, unsigned char value)
{
	sci_set_regs(SCI_CHGUARD, ch, value);	
	return;
}

static void _sci_set_blkguard(int ch, unsigned char value)
{
	sci_set_regs(SCI_BLKGUARD, ch, value);	
	return;
}

static void _sci_get_chguard(int ch, unsigned char *guard)
{
	u16 value;
	
	value = sci_get_regs(SCI_CHGUARD, ch);	
	*guard = (unsigned char)value;	
}

static void _sci_get_blockguard(int ch, unsigned char *guard)
{
	u16 value;

	value = sci_get_regs(SCI_BLKGUARD, ch);	
	*guard = (unsigned char)value;
}

static void _sci_set_rxtimeout(int ch, unsigned short value)
{
	sci_set_regs(SCI_RXTIME, ch, value);	
	return;
}

static void _sci_get_rxtimeout(int ch, unsigned short *timeout)
{
	u16 value;
	
	value = sci_get_regs(SCI_RXTIME, ch);	
	*timeout = value;	
}

static void _sci_set_rxparity(int ch, unsigned char value) /*value: 0 for even parity, 1 for odd parity */
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	value?sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_RXPARITY):sci_set_regs(SCI_CR0, ch, old_cr0_value & (~CR0_RXPARITY));
	return;
}


static void _sci_set_txparity(int ch, unsigned char value) /*value: 0 for even parity, 1 for odd parity */
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	value?sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_TXPARITY):sci_set_regs(SCI_CR0, ch, old_cr0_value & (~CR0_TXPARITY));
	return;
}

static void _sci_set_rxnak(int ch, unsigned char value) /*value: 0 disable, 1 enable */
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	value?sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_RXNAK):sci_set_regs(SCI_CR0, ch, old_cr0_value & (~CR0_RXNAK));
	return;
}

static void _sci_set_txnak(int ch, unsigned char value) /*value: 0 disable, 1 enable */
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	value?sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_TXNAK):sci_set_regs(SCI_CR0, ch, old_cr0_value & (~CR0_TXNAK));
	return;
}

static void _sci_direct_sense(int ch) /* high level for logic 1, lsb is fist bit */ 
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	sci_set_regs(SCI_CR0, ch, old_cr0_value & (~(CR0_SENSE | CR0_ORDER)));
	return;
}

static void _sci_indirect_sense(int ch) /* low level for logic 1, msb is fist bit */ 
{
	u16 old_cr0_value;
	old_cr0_value = sci_get_regs(SCI_CR0, ch);
	sci_set_regs(SCI_CR0, ch, old_cr0_value | CR0_SENSE | CR0_ORDER);
	return;
}

static void _sci_set_etu(int ch, unsigned int F, unsigned int D)
{
	u16 sci_value;
	u16 sci_baud;
	u16 sci_clkicc;
	sci_value = sci_get_regs(SCI_VALUE,ch);
	sci_clkicc = sci_get_regs(SCI_CLKICC,ch);
	sci_baud = ((u16)F * (sci_clkicc + 1) * 2)/((u16)D * sci_value) - 1;
	baud_value = sci_baud;

	sci_set_regs(SCI_BAUD, ch, baud_value);

}

static void _sci_set_sc_clock(int ch, unsigned int f) /* f is smart card clock (Hz)*/
{
	u32 sci_clkicc;

	sci_clkicc= (u32)(SCI_REF_CLK * 10/(f * 2))-10;		/* "*10","-10"  are used for rounding */
	if( (sci_clkicc - (sci_clkicc/10)*10) > 4 )
		sci_clkicc += 10;

	clkicc_value = (sci_clkicc/10)&0xff;
	
	sci_set_regs( SCI_CLKICC,ch,  (u16)((sci_clkicc/10)&0xff));
}

static unsigned int _sci_get_sc_clock(int ch) /* return value is smart card clock (Hz)*/
{
	u16 sci_clkicc;
	unsigned int ret;
	
	sci_clkicc = sci_get_regs(SCI_CLKICC,ch) & 0xff;
	ret = (unsigned int)(SCI_REF_CLK / ((sci_clkicc +1)*2));
	return ret;
}

static void _sci_set_init_value(int ch)
{
	unsigned short direction;
	
	_sci_flush_rxfifo(ch);
	_sci_flush_txfifo(ch);
	sci_set_regs(SCI_ICR, ch, 0x7fff);
	
	direction = sci_get_regs(SCI_CR0,ch) & (unsigned short)(CR0_SENSE | CR0_ORDER);
	sci_set_regs(SCI_CR0, ch, CR0_INIT_VALUE | direction );
	
	sci_set_regs(SCI_CR1, ch, CR1_INIT_VALUE);
	sci_set_regs(SCI_STABLE, ch, STABLE_INIT_VAL);
	sci_set_regs(SCI_ATIME, ch, ATIME_INIT_VAL);
	sci_set_regs(SCI_DTIME, ch, DTIME_INIT_VAL);
	sci_set_regs(SCI_ATRSTIME, ch, ATRSTIME_INIT_VAL);
	sci_set_regs(SCI_ATRDTIME, ch, ATRDTIME_INIT_VAL);
	sci_set_regs(SCI_CHTIMEMS, ch, 0);	/* CHTIMEMS must be written first, followed by CHTIMELS */
	sci_set_regs(SCI_CHTIMELS, ch, CHTIMELS_INIT_VAL);
	sci_set_regs(SCI_TIDE, ch, TIDE_INIT_VAL);
	
	if( clkicc_value == 0 ){
		clkicc_value = CLKICC_INIT_VAL;
		baud_value = BAUD_INIT_VAL;
	}
	sci_set_regs(SCI_CLKICC, ch, clkicc_value);	
	sci_set_regs(SCI_BAUD, ch, baud_value);
	
	sci_set_regs(SCI_VALUE, ch, VALUE_INIT_VAL);
	
	sci_set_regs(SCI_RXTIME, ch, RXTIME_INIT_VAL);
	sci_set_regs(SCI_BLKTIMEMS, ch, 0);
	sci_set_regs(SCI_BLKTIMELS, ch, BLKTIMELS_VAL);
	return;
}

static void _sci_mask_interrupt(int ch, unsigned short intbits)
{
	u16 old_mask;
	old_mask = sci_get_regs(SCI_IMSC, ch);
	old_mask &= ~intbits;
	sci_set_regs(SCI_IMSC, 0, old_mask);
	return;
}

static void _sci_unmask_interrupt(int ch, unsigned short intbits)
{
	u16 old_mask;
	old_mask = sci_get_regs(SCI_IMSC, ch);
	old_mask |= intbits;
	sci_set_regs(SCI_IMSC, 0, old_mask);
	return;
}

static void _sci_active(int ch)
{
	sci_output_active = 0;
	sci_input_active = 0;
	card_noatr =0;
	_sci_set_rxint_trigger(ch, 1);
	_sci_set_init_value(ch);
	
/* each active will clear all rx/tx buffer */
	_sci_flush_txfifo(0);
	_sci_flush_rxfifo(0);
	rq.head = 0;
	rq.tail = 0;
	tq.head = 0;
	tq.tail = 0;
	sci_set_regs(SCI_CR2, ch, CR2_STARTUP);
	return;
}

static void _sci_warmreset(int ch)
{
	sci_output_active = 0;
	sci_input_active = 0;
	card_noatr =0;
	_sci_set_rxint_trigger(ch, 1);
	_sci_set_init_value(ch);
	
/* each active will clear all rx/tx buffer */
	_sci_flush_txfifo(0);
	_sci_flush_rxfifo(0);
	rq.head = 0;
	rq.tail = 0;
	tq.head = 0;
	tq.tail = 0;
	sci_set_regs(SCI_CR2, ch, CR2_WRESET);
	return;
}

static void _sci_deactive(int ch)
{
	card_noatr =0;
	_sci_flush_txfifo(0);
	_sci_flush_rxfifo(0);
	sci_set_regs(SCI_CR2, ch, CR2_FINISH);
	return;
}

/* --------------------------------------------------------------------------------- */
/*
 * Open the device.
 */
static int sci_open(struct inode *inode, struct file *filp)
{
	unsigned long flags;
	
/* set empty for receive/transmitt queue */
	rq.head = 0;
	rq.tail = 0;
	tq.head = 0;
	tq.tail = 0;

/* set init value for register to receive ATR*/
	spin_lock_irqsave(&sci_lock, flags);
	card_noatr =0;
	receive_exception =0;
	_sci_set_init_value(0);
	_sci_direct_sense(0);
	spin_unlock_irqrestore(&sci_lock, flags);
	_sci_unmask_interrupt(0, 0x3fff); /* enable all interrupts */
	return 0;
}


static int sci_release(struct inode *inode, struct file *filp)
{
	/* Wait for any pending output to complete */
	if (!EMPTY(tq))
		wait_event_interruptible(sci_empty_queue, sci_output_active == 0);

	_sci_deactive(0);
	_sci_mask_interrupt(0, 0xfffc); /* Only enable card in/out*/
	return 0;
}

static unsigned int sci_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
//	poll_wait(filp, &CardInOut_queue, wait);

	poll_wait(filp, &rq_queue, wait);
	poll_wait(filp, &tq_queue, wait);
	poll_wait(filp, &sci_read_ready, wait);
	poll_wait(filp, &sci_write_ready, wait);
	

	if (sci_rx_buffer.sci_buffer_report_flag ==1 && sci_rx_buffer.sci_buffer_report_en == 1){
		if (LEFT(rq) >= sci_rx_buffer.sci_buffer_threshold ){
			mask |= POLLIN | POLLRDNORM;
			return mask;
		}
	}		
	if (sci_tx_buffer.sci_buffer_report_flag ==1 && sci_tx_buffer.sci_buffer_report_en == 1){
		if (LEFT(tq) < sci_tx_buffer.sci_buffer_threshold ){
			mask |= POLLOUT | POLLWRNORM;
			return mask;
		}
	}
	
	if( card_inout_change == 1 ){
//		mdelay(4000);
		card_inout_change = 0;
		mask = POLLPRI;
		return mask;
	}
	
/*	if(irq_sci_cardin == 1){
		spin_lock_irqsave(&sci_lock, flags);
		irq_sci_cardin = 0;
		mask = POLLPRI;
		spin_unlock_irqrestore(&sci_lock, flags);
		return mask;
	}
	if(irq_sci_cardout == 1){
		spin_lock_irqsave(&sci_lock, flags);
		irq_sci_cardout = 0;
		mask = POLLPRI;
		spin_unlock_irqrestore(&sci_lock, flags);
		return mask;
	}*/	
	if (!EMPTY(rq)) mask |= POLLIN | POLLRDNORM; /* read enabled */
	if (!FULL(tq)) mask |= POLLOUT | POLLWRNORM; /* write enabled */

	return mask;
}

static int sci_ioctl(struct inode *inode, struct file *filp,
			   unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	char tmp_val = 0;
	unsigned int fifo_size =0;
	struct SCI_BAUD_t{
		unsigned int F;
		unsigned int D;
	}sci_etu;
	unsigned long lock_flags;
	unsigned int sci_clk;
	unsigned char	retry = 0, guard = 0;
	unsigned short rxfifotimeout;
	
	switch (cmd) {
		case SCI_IOC_WARMRESET: 
			if (card_inserted == 1)
				_sci_warmreset(sci_channel);
			/* else
				ret = -EPERM;
			*/
			break;
			
		case SCI_IOC_ISPRESENT: 
			tmp_val = card_inserted;
			if (copy_to_user((void*)arg, &tmp_val, sizeof(char))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_ACTIVE:
			if (card_inserted == 1){	
				_sci_active(sci_channel);
			   
            }
			/*else 
				ret = -EPERM;
			*/
			break;
			
		case SCI_IOC_DEACTIVE:
			if (card_inserted == 1){
                //state_atr = 0;
				_sci_deactive(sci_channel);
            }
			/* else
				ret = -EPERM;
			*/
			break;
			
		case SCI_IOC_CLKSTOP:
			if (card_inserted == 1)
				_sci_clkstart(sci_channel);
			/* else
				ret = -EPERM;
			*/
			break;
			
		case SCI_IOC_CLKSTART: 
			_sci_clkstop(sci_channel);
			break;

		case SCI_IOC_RXINTTIGGER:
			if (arg < 8)
				_sci_set_rxint_trigger(sci_channel, (unsigned char)arg);
			else
				return -EINVAL;
			break;
		case SCI_IOC_TXINTTIGGER:
			if (arg < 8)
				_sci_set_txint_trigger(sci_channel, (unsigned char)arg);
			else
				return -EINVAL;
			break;

		case SCI_IOC_SET_ETU:
			if (copy_from_user(&sci_etu, (void *)arg, sizeof(struct SCI_BAUD_t)))
				return -EFAULT;
			_sci_set_etu(sci_channel, sci_etu.F, sci_etu.D);
			
			break;
		case SCI_IOC_GET_SC_CLOCK:
			sci_clk = _sci_get_sc_clock(sci_channel);
			
			if (copy_to_user((void*)arg, &sci_clk, sizeof(unsigned int))) 
				ret = -EFAULT;
			break;
		case SCI_IOC_SET_SC_CLOCK:
			_sci_set_sc_clock(sci_channel,arg);
			break;

		case SCI_IOC_SET_TXRETRY:
			_sci_set_txretry(0, (unsigned char)arg);
			break;

		case SCI_IOC_SET_RXRETRY:
			_sci_set_rxretry(0, (unsigned char)arg);
			break;

		case SCI_IOC_GET_TXRETRY:
			_sci_get_txretry(0, &retry);
			if (copy_to_user((void*)arg, &retry, sizeof(unsigned char))) 
				ret = -EFAULT;
			break;

		case SCI_IOC_GET_RXRETRY:
			_sci_get_rxretry(0, &retry);
			if (copy_to_user((void*)arg, &retry, sizeof(unsigned char))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_SET_CHGUARD:
			_sci_set_chguard(0, (unsigned char)arg);
			break;
			
		case SCI_IOC_SET_BLOCKGUARD:
			_sci_set_blkguard(0, (unsigned char)arg);
			break;

		case SCI_IOC_GET_CHGUARD:
			_sci_get_chguard(0, &guard);
			if (copy_to_user((void*)arg, &guard, sizeof(unsigned char))) 
				ret = -EFAULT;
			break;

		case SCI_IOC_GET_BLOCKGUARD:
			_sci_get_blockguard(0, &guard);
			if (copy_to_user((void*)arg, &guard, sizeof(unsigned char))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_SET_CONVENTION:  
			if (arg == 0)
				_sci_direct_sense(0);
			else if (arg == 1)
				_sci_indirect_sense(0);
			else 
				ret = -EINVAL;
			break;
			
		case SCI_IOC_SET_PARITY:
			if (arg == 0 || arg ==1){
				_sci_set_rxparity(0, (unsigned char)arg);
				_sci_set_txparity(0, (unsigned char)arg);
			}
			else 
				ret = -EINVAL;
			break;

		case SCI_IOC_SET_TXRXNAK:
			if (arg == 0 || arg ==1){
				_sci_set_rxnak(0, (unsigned char)arg);
				_sci_set_txnak(0, (unsigned char)arg);
			}
			else 
				ret = -EINVAL;
			break;
			
		case SCI_IOC_SET_RXTIMEOUT:
			_sci_set_rxtimeout(0, (unsigned short)arg);
			break;

		case SCI_IOC_GET_RXTIMEOUT:
			_sci_get_rxtimeout(0, &rxfifotimeout);
			if (copy_to_user((void*)arg, &rxfifotimeout, sizeof(unsigned short))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_GET_RXRDYSIZE:
			fifo_size = LEFT(rq);
			if (copy_to_user((void*)arg, &fifo_size, sizeof(unsigned int))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_GET_TXRDYSIZE:
			fifo_size = CHARS(tq);
			if (copy_to_user((void*)arg, &fifo_size, sizeof(unsigned int))) 
				ret = -EFAULT;
			break;
			
		case SCI_IOC_SET_RXFIFO_THRESHOLD:
			if (arg < IFD_BUF_SIZE){
				spin_lock_irqsave(&sci_buffer_status_lock, lock_flags);
				sci_rx_buffer.sci_buffer_threshold = arg;
				spin_unlock_irqrestore(&sci_buffer_status_lock, lock_flags);
			}			
			break;
			
		case SCI_IOC_GET_RXFIFO_THRESHOLD:
				if (copy_to_user((void*)arg, &(sci_rx_buffer.sci_buffer_threshold), sizeof(int))) 
					ret = -EFAULT;
			break;
			
		case SCI_IOC_SET_TXFIFO_THRESHOLD:
			if (arg < IFD_BUF_SIZE){
				spin_lock_irqsave(&sci_buffer_status_lock, lock_flags);
				sci_tx_buffer.sci_buffer_threshold = arg;
				spin_unlock_irqrestore(&sci_buffer_status_lock, lock_flags);
			}
			break;
			
		case SCI_IOC_GET_TXFIFO_THRESHOLD:
				if (copy_to_user((void*)arg, &(sci_tx_buffer.sci_buffer_threshold), sizeof(int))) 
					ret = -EFAULT;
			break;
			
		case SCI_IOC_SET_RX_READREADY_REPORT:
			if (arg == 0 || arg == 1){
				spin_lock_irqsave(&sci_buffer_status_lock, lock_flags);
				sci_rx_buffer.sci_buffer_report_en = arg;
				spin_unlock_irqrestore(&sci_buffer_status_lock, lock_flags);
			}
			break;	
			
		case SCI_IOC_SET_TX_WRITEREADY_REPORT:	
			if (arg == 0 || arg == 1){
				spin_lock_irqsave(&sci_buffer_status_lock, lock_flags);
				sci_tx_buffer.sci_buffer_report_en = arg;
				spin_unlock_irqrestore(&sci_buffer_status_lock, lock_flags);
			}
			break;	
			
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

/*
 * Start input; call under lock.
 */
static void sci_start_input(void)
{
	if (sci_input_active) /* Should never happen */
		return;
	
	_sci_set_txrxmode(0, 0);
	_sci_mask_interrupt(0, INT_TXTIDE);
//	_sci_unmask_interrupt(0, INT_RXTIDE);
	

	sci_input_active = 1;
	sci_output_active = 0;
}


/*
 * Read from the device 
 */
static ssize_t sci_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned long flags;
	unsigned int i,count0 = 0;
	u8 read_buf[IFD_BUF_SIZE] = { 0 };
	
//	DEFINE_WAIT(wait);
	
	if ((card_inserted == 0) || (card_noatr == 1)){
		DEBUG(SCI_DEBUG_LEVEL0, "card status is wrong\n");
		return 0;
	}
	if (EMPTY(rq) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;
	
	
	spin_lock_irqsave(&sci_lock, flags);
	sci_start_input();
	spin_unlock_irqrestore(&sci_lock, flags);
	receive_exception =0;
	
	if (LEFT(rq) < count){
	//if (EMPTY(rq)){
		wait_event_interruptible_timeout(rq_queue, (LEFT(rq) >= count || receive_exception == 1), rxtimeout*HZ); /* 10 sec */
#if 0 
		prepare_to_wait(&rq_queue, &wait, TASK_INTERRUPTIBLE);
		if (LEFT(rq) < count){
			schedule_timeout(rxtimeout*HZ); /* max sleep time is 5 secs */
			finish_wait(&rq_queue, &wait);
			if (signal_pending(current))
				return -ERESTARTSYS;
		}
#endif
		}
	if(receive_exception == 1){
		if( EMPTY(rq) )
			return -EFAULT;
	}
	/* count0 is the number of readable data bytes */
	
	count0 = LEFT(rq);
	if (count0 < count) count = count0;

	for (i = 0; i<count && (!EMPTY(rq)); i++) { 
		GETCH(rq, read_buf[i]); 
//		DEBUG(SCI_DEBUG_LEVEL3, "Read ch is 0x%x\n",read_buf[i]);
	}

	if (count > 0){
		if (copy_to_user(buf, read_buf, count)) 
			return -EFAULT;
	}
	return count;
}

/*
 * Start output; call under lock.
 */
static inline void sci_start_output(void)
{
//	_sci_mask_interrupt(0, INT_RXTIDE);
	_sci_unmask_interrupt(0, INT_TXTIDE);
	if (!sci_output_active){
		_sci_set_txint_trigger(0, 4);
		_sci_set_txrxmode(0, 1);
		sci_output_active = 1;
		sci_input_active = 0;
	}
}


/*
 * Write to the device.
 */
static ssize_t sci_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned long flags;
	int i, space, written = 0;
	char write_buf[IFD_BUF_SIZE];

	if (card_inserted == 0 || card_noatr == 1){
		DEBUG(SCI_DEBUG_LEVEL0, "card status is wrong\n");
		return 0;
	}
	if (FULL(tq) && (filp->f_flags & O_NONBLOCK))
		return -EAGAIN;
	/*
	 * Take and hold the semaphore for the entire duration of the operation.  The
	 * consumer side ignores it, and it will keep other data from interleaving
	 * with ours.
	 */
	if (down_interruptible(&tq_sem)) return -ERESTARTSYS;

	/*
	 * Out with the data.
	 */
	while (written < count) {
		/* Hang out until some buffer space is available. */
		space = CHARS(tq);
		if (space == 0) {
			if (wait_event_interruptible(tq_queue,(space = CHARS(tq)) > 0)) /* the return value should be zero, otherwise go out*/
				goto out;
		}

		/* Move data into the buffer. */
		if ((space + written) > count) space = count - written;
		if (copy_from_user(write_buf, buf, space)) {
			up(&tq_sem);
			return -EFAULT;
		}
		
		for (i=0; i<space && !FULL(tq); i++) {
			PUTCH(write_buf[i], tq); 
			DEBUG(SCI_DEBUG_LEVEL3, "\nwirte_buf[%d]=0x%x",i,write_buf[i]);
		}
		
/*		buf += space;
		written += space;*/
		buf += i;
		written += i;

		/* If no output is active, make it active. */
		spin_lock_irqsave(&sci_lock, flags);
		sci_start_output();
		spin_unlock_irqrestore(&sci_lock, flags);
	}

	if (!EMPTY(tq)){
		if (wait_event_interruptible(sci_empty_queue, sci_output_active == 0))
			goto out;
		sci_start_input();
	}

out:
	*f_pos += written;
	up(&tq_sem);
	return written;
}


static void sci_do_tasklet(unsigned long rx_tx_flag)
{
	unsigned long flags;

	if (rx_tx_flag == INT_RXTIDE || rx_tx_flag == INT_RDOUT || rx_tx_flag == INT_CHOUT){    /* processing for receive data */

		spin_lock_irqsave(&sci_buffer_status_lock, flags);
		if (sci_rx_buffer.sci_buffer_report_flag ==1 && sci_rx_buffer.sci_buffer_report_en == 1){
			if (LEFT(rq) >= sci_rx_buffer.sci_buffer_threshold ){
				wake_up_interruptible(&sci_read_ready);
			}
		}		
		spin_unlock_irqrestore(&sci_buffer_status_lock, flags);
		if (rx_tx_flag == INT_CHOUT) {
			printk("SCI Attention info: Character timed out!\n");
		}
		DEBUG(SCI_DEBUG_LEVEL3, " rq.tail=%d, rq.head=%d\n",rq.tail,rq.head); 
		wake_up_interruptible(&rq_queue);
	}
	else if (rx_tx_flag == INT_TXTIDE) { /* processing for transmit data */
		spin_lock_irqsave(&sci_lock, flags);

		/* Have we written everything? */
		if (EMPTY(tq)) { /* empty */ /*The buffer size should be greater than 8 */
			sci_output_active = 0;
			wake_up_interruptible(&sci_empty_queue);
			if( _sci_txfifocount(0) == 0 )	/* tx fifo empty, ready to rx mode */
					sci_start_input();
		}
		else{
			_sci_unmask_interrupt(0, INT_TXTIDE);
		}
		
		wake_up_interruptible(&tq_queue);
		
		if (sci_tx_buffer.sci_buffer_report_flag ==1 && sci_tx_buffer.sci_buffer_report_en == 1){
			if (LEFT(tq) < sci_tx_buffer.sci_buffer_threshold ){
				wake_up_interruptible(&sci_write_ready);
			}
		}	
		spin_unlock_irqrestore(&sci_lock, flags);
	}
	else if (rx_tx_flag == INT_ATRSTOUT )
	{
		if ( sci_input_active ){
			receive_exception =1;
			wake_up_interruptible(&rq_queue);
			printk( "Information From Kernel: sci_do tasklet --> rx_tx_flag == 0x%x\n", (unsigned int)rx_tx_flag );	/* add for DeHe */
		}
	}


	return;
}

static irqreturn_t sci_interrupt(int irq, void *dev_id)
{
	u16 int_status = sci_get_regs(SCI_RIS, 0);
	unsigned char count_status;
	unsigned char ch;
	//int					timeout;
	
	if (int_status & INT_IN) { 
		sci_set_regs(SCI_ICR, 0, INT_IN);  
		card_inserted = 1; 		
		card_inout_change = 1;
/*		if(irq_sci_cardin == 0)
		{
			spin_lock_irqsave(&sci_lock, flags);
			irq_sci_cardin = 1;
			wake_up_interruptible(&CardInOut_queue);
			spin_unlock_irqrestore(&sci_lock, flags);
		}*/
		DEBUG(SCI_DEBUG_LEVEL3, " INTR_CARDIN \n"); 	
	}
	else if (int_status & INT_OUT) { 
		sci_set_regs(SCI_ICR, 0, INT_OUT); 
		card_inserted = 0; 
		clkicc_value = 0;
		receive_exception =1;
		wake_up_interruptible(&rq_queue);
		wake_up_interruptible(&sci_empty_queue);
		rq.head = 0;
		rq.tail = 0;
		tq.head = 0;
		tq.tail = 0;

		_sci_flush_txfifo(0);
		_sci_flush_rxfifo(0);
		sci_start_input();
		
		card_inout_change = 1;
		DEBUG(SCI_DEBUG_LEVEL3, " INTR_CARDOUT \n"); 
	}
	else if (int_status & INT_PWUP) { 
		sci_set_regs(SCI_ICR, 0, INT_PWUP);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_PWUP \n"); 
	}
	else if (int_status & INT_PWDN) { 
		sci_set_regs(SCI_ICR, 0, INT_PWDN);  
		sci_set_regs(SCI_TIDE, 0, 0x0011);
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_PWDN \n"); 
	}
	else if (int_status & INT_TRANSERR) { 
		sci_set_regs(SCI_ICR, 0, INT_TRANSERR);  
		_sci_flush_txfifo(0);
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_TRANSERR \n"); 
	}
	else if (int_status & INT_ATRSTOUT) { 
		sci_set_regs(SCI_ICR, 0, INT_ATRSTOUT);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_ATRSTOUT \n"); 
		card_noatr= 1;
		sci_tasklet.data = INT_ATRSTOUT; /* register for receive timeout*/
		tasklet_schedule(& sci_tasklet);  /* wakeup tasklet to execute*/
		
	}
	else if (int_status & INT_ATRDTOUT) { 
		sci_set_regs(SCI_ICR, 0, INT_ATRDTOUT);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_ATRDTOUT \n"); 
	}
	else if (int_status & INT_BLKOUT) { 
		sci_set_regs(SCI_ICR, 0, INT_BLKOUT);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_BLKOUT \n"); 
	}
	else if (int_status & INT_CHOUT) { 
		sci_set_regs(SCI_ICR, 0, INT_CHOUT);
		count_status =_sci_rxfifocount(0); /* ensure all data has been stored in receive buffer */
		while (count_status ){
			PUTCH(_sci_read_date(0),rq);
			count_status --;	
		}
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_CHOUT \n"); 
		sci_tasklet.data = INT_CHOUT; /* register for character timeout*/
		tasklet_schedule(& sci_tasklet);  /* wakeup tasklet to execute*/
	}
	else if (int_status & INT_RDOUT) { 
		sci_set_regs(SCI_ICR, 0, INT_RDOUT);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_RDOUT \n"); 
		count_status =_sci_rxfifocount(0);
		DEBUG( SCI_DEBUG_LEVEL3, "\nrxfifo-count[%02x], txfifo-count[%02x]\n", count_status,_sci_txfifocount(0));	
		while (count_status ){
			ch =_sci_read_date(0);
			PUTCH(ch,rq);
			DEBUG( SCI_DEBUG_LEVEL3, " [%02x]\n", ch);	
			count_status --;	
		}
		sci_tasklet.data = INT_RDOUT; /* register for receive timeout*/
		tasklet_schedule(& sci_tasklet);  /* wakeup tasklet to execute*/
	}
	else if (int_status & INT_OVERRUN) { 
		sci_set_regs(SCI_ICR, 0, INT_OVERRUN);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_OVERRUN \n"); 
	}
	else if (int_status & INT_CLKSTOP) { 
		sci_set_regs(SCI_ICR, 0, INT_CLKSTOP);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_CLKSTOP \n"); 
	}
	else if (int_status & INT_CLKACTIVE) { 
		sci_set_regs(SCI_ICR, 0, INT_CLKACTIVE);  
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_CLKACTIVE \n"); 
	}
	else if (int_status & INT_RXTIDE) { 
		sci_set_regs(SCI_ICR, 0, INT_RXTIDE); 
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_RXTIDE \n");
		count_status =_sci_rxfifocount(0);
		DEBUG( SCI_DEBUG_LEVEL3, "\nrxfifo-count[%02x], txfifo-count[%02x]\n", count_status,_sci_txfifocount(0));	
		if (count_status == 0) { // should not happen, but for speical case
			printk("count_status=%d, tide=0x%x\n", count_status, sci_get_regs(SCI_TIDE, 0));
			_sci_flush_rxfifo(0);
		}
		while (count_status ){
			ch =_sci_read_date(0);
			PUTCH(ch,rq);
			DEBUG( SCI_DEBUG_LEVEL3, " r[%02x]\n", ch);	
			count_status --;	
		}
		sci_tasklet.data = INT_RXTIDE; /* register for receive*/
		tasklet_schedule(& sci_tasklet);  /* wakeup tasklet to execute*/
		}
	else if (int_status & INT_TXTIDE) { 
		sci_set_regs(SCI_ICR, 0, INT_TXTIDE); /* clr interrupt */
		DEBUG( SCI_DEBUG_LEVEL3, " INTR_TXTIDE \n");
		count_status = SCI_MAX_FIFOSIZE - _sci_txfifocount(0);
		while (count_status && !EMPTY(tq)){
			GETCH(tq, ch);
			_sci_write_date(0, ch);
			DEBUG( SCI_DEBUG_LEVEL3, " w[%02x]  ", ch );	
			count_status --;	
		}

		if (EMPTY(tq)){
			_sci_mask_interrupt(0, INT_TXTIDE);
		/*timeout = 0x10000;
		count_status = SCI_MAX_FIFOSIZE - _sci_txfifocount(0);
		while( count_status && (timeout>0) ){
					timeout--;
					count_status = SCI_MAX_FIFOSIZE - _sci_txfifocount(0);
					timeout--;
		}*/
			sci_start_input();
		}
			
		sci_tasklet.data = INT_TXTIDE; /* register for transmation*/
		tasklet_schedule(& sci_tasklet);  /* wakeup tasklet to execute*/
		
	}

	return IRQ_HANDLED;
}

static struct file_operations sci_fops = {
	.read = sci_read,
	.write = sci_write,
	.open = sci_open,
	.release = sci_release,
	.poll = sci_poll,
	.ioctl = sci_ioctl,
	.owner = THIS_MODULE
};

static struct miscdevice sci_dev = {
	SCI_MINOR,
	"smartcard",
	&sci_fops,
};

static struct proc_dir_entry *sci_proc_entry = NULL;

static int sci_proc_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	sprintf(page, "No define\n");
	return strlen(page);
}

static int sci_proc_write(struct file *file, 
		const char __user*buffer, unsigned long count, void *data)
{
	char cmd_line[25];

	if (copy_from_user( cmd_line, buffer, count )) {
    return -EFAULT;
   }

	if (strncmp("get", cmd_line, 3) == 0) {
		unsigned short val = sci_get_regs(simple_strtol(&cmd_line[4], NULL, 16), 0);
		printk("Get [0x%08x] = 0x%04x \n", (unsigned int)simple_strtol(&cmd_line[4], NULL, 16), val);
	}
	else if (strncmp("set", cmd_line, 3) == 0) {
		unsigned short addr = simple_strtol(&cmd_line[4], NULL, 16);
		unsigned short val = simple_strtol(&cmd_line[7], NULL, 16);
		printk("Set [0x%04x] = 0x%02x \n", addr, val);
		sci_set_regs(addr, 0, val);
	}
	
	return count;
}

/*
 * Module initialization
 */
int __init cs_sci_init(void)
{
	int error = 0;

	error = misc_register(&sci_dev);
	if (error) {
		printk(KERN_ERR "smart card: unable to get misc minor\n");
		return error;
	}

	sci_base = (unsigned short *)VA_SCI_BASE;
	{
		sci_proc_entry = create_proc_entry("sci_io", (mode_t)0, NULL);
		if (NULL != sci_proc_entry) {
			sci_proc_entry->write_proc = &sci_proc_write;
			sci_proc_entry->read_proc = &sci_proc_read;
		}
	}

	sema_init(&tq_sem, 1);

	if ((sci_get_regs(SCI_RIS, 0) & INT_IN) != 0) {
		card_inserted = 1;
	}
	else {
		card_inserted = 0;
	}
	if ((error = request_irq(SCI_IRQ, sci_interrupt, IRQF_DISABLED, "smartcard", NULL)) != 0){
		printk(KERN_ERR "smart card: request irq fails!\n");
		
	}

	/* do something here */
	/* ... ... */
	rxtimeout = RXTIMEOUT_DEFAULT;
	printk( "Celestial Smart Card interface initialized!\n");

	return error;
}

static void __exit cs_sci_exit(void)
{
	misc_deregister(&sci_dev);
	remove_proc_entry("sci_io", NULL);
	free_irq(SCI_IRQ, NULL);
	
}

module_init(cs_sci_init);
module_exit(cs_sci_exit);

