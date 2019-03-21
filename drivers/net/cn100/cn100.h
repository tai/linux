/*
 * cn100 Ethernet
 */

#ifndef _CN100X_H_
#define _CN100X_H_

/* Structure/enum declaration ------------------------------- */
typedef struct buffer_t
{
	u32 	size;
	u32 	phy_address;
	u32 	vir_address;
} buffer_f;

typedef struct tx_buffer_t
{
	u32 buffer_addr;
	u32 buffer_len;
	u32 next_link_item;
	u32 next_item_valid_flag;
} tx_buffer_f;

typedef struct rx_buffer_t
{
	u32 rx_buffer_addr[RX_BUFFER_ITEM_NUM];
	u32 buffer_num;
} rx_buffer_f;

/**************************************************************/
typedef struct board_info 
{
	u32 					  reset_counter;		/* counter: RESET */ 
	u32 					  reset_tx_timeout;	/* RESET caused by TX Timeout */ 
	u32 					  io_addr;				/* Register I/O base address */
	u16 					  irq;					/* IRQ */
	struct resource 	   *  irq_res;
	
	u16 	                  tx_pkt_cnt;
#ifdef _CN100_MII_	
	struct timer_list 		  timer;
#endif	
	struct timer_list 		   interrupt_timer;
	
	struct net_device_stats    stats;
	spinlock_t 				   lock;
	spinlock_t 				   rx_lock;

	u8 						   phy_op_mode; 			/* PHY operation mode */
	u8 						   phy_type; 				/* PHY operation mode */
//	u8 								device_wait_reset;	/* device state */
	
#ifdef _CN100_MII_
	struct mii_if_info 			mii;
#endif

	u32 						msg_enable;
	
	/* RX and TX Buffer Info */
	/* NOTE: This is tx data buffer address */
	struct buffer_t 			tx_buffer_t;
	/* NOTE: This is tx node struct address */
	struct tx_buffer_t 		*	tx_buffer;
	struct rx_buffer_t 			rx_buffer;	
	u32 						reserve_vir_addr;
    struct napi_struct napi;

} board_info_t;
/**************************************************************/

#endif /* _CN100X_H_ */

