#ifndef __CN100_HW_H__
#define __CN100_HW_H__

#include <mach/hardware.h>

#define CN100_BASEADDRESS  PA_ETH_MAC_BASE
#define CN100_VA_BASEADDRESS VA_ETH_MAC_BASE
/*************************************************/
#define	ETH_CFG_ADDR		    0x0800
#define	ETH_INT_REG_ADDR        0x0840
#define	ETH_INT_ENB_ADDR        0x0844
#define	ETH_INT_SET_ADDR        0x0848
#define	ETH_INT_CLS_ADDR        0x084c	
#define	ETH_TX_DMA_ADDR        	0x09c0	
#define	ETH_TX_STATES_ADDR     	0x09c4	
#define	ETH_RX_BASE_ADDR       	0x09c8	
#define	ETH_RX_CFG_ADDR        	0x09cc	
#define	ETH_RX_RD_ADDR         	0x09d0	
#define	ETH_RX_WR_ADDR         	0x09d4	
#define	ETH_RX_STATES_ADDR     	0x09dc	
#define	ETH_MAC_SA_MASK        	0x0a98	
#define	ETH_MAC_HASH_TAB0      	0x0a9c	
#define	ETH_MAC_HASH_TAB1      	0x0aa0
                                      
#define	ETH_MAC_CFG0          	0x0a40	
#define	ETH_MAC_CFG1          	0x0a44	
#define	ETH_MAC_IPGT          	0x0a48	
#define	ETH_MAC_IPGR          	0x0a4c	
#define	ETH_MAC_CLRT          	0x0a50	
#define	ETH_MAC_MAXF          	0x0a54	
#define	ETH_MAC_SUPP          	0x0a58	
#define	ETH_MAC_TEST          	0x0a5c	
#define	ETH_MAC_MCFG          	0x0a60	
#define	ETH_MAC_MCMD          	0x0a64	
#define	ETH_MAC_MADR          	0x0a68	
#define	ETH_MAC_MWTD          	0x0a6c	
#define	ETH_MAC_MRDD          	0x0a70	
#define	ETH_MAC_MIND          	0x0a74	
#define	ETH_MAC_SA0           	0x0a80	
#define	ETH_MAC_SA1           	0x0a84	
#define	ETH_MAC_SA2           	0x0a88	
#define	ETH_MAC_TSV0           	0x0a8c	
#define	ETH_MAC_TSV1           	0x0a90	
#define	ETH_MAC_RSV            	0x0a94

#define ETH_INT_TX		0x1
#define ETH_INT_RX		0x2
#define ETH_INT_RX_HF   0x4
#define ETH_INT_ALL		(ETH_INT_TX | ETH_INT_RX)	

#define ETH_FIFO_TOG		0x80000000
#define ETH_FIFO_EMPTY		0x40000000
/*************************************************/

// #define CN100_PKT_RDY			0x01	/* Packet ready to receive */
// #define CN100_PKT_MAX			1536	/* Received packet max size */

int init_eth_hw 		( struct net_device *dev, struct board_info *db );
int release_eth_hw 	( struct net_device *dev, struct board_info *db );
int init_phy_hw 		( struct net_device *dev, struct board_info *db );
int release_phy_hw 	( struct net_device *dev, struct board_info *db );

/* enable or disable tx */
void set_tx_enable 	( struct board_info *db, int enable );
/* enable or disable tx interrupt */
void set_tx_int 		( struct board_info *db, int enable );
/* enable or disable rx */
void set_rx_enable 	( struct board_info *db, int enable );
/* enable or disable rx interrupt */
void set_rx_int 		( struct board_info *db, int enable );
/* reset rx */
void reset_eth 		( struct board_info *db );
/* set tx buffer for DMA */
int set_tx_buffer 	( struct board_info *db );
/* set rx buffer for DMA */
int set_rx_buffer 	( struct board_info *db );
/* set eth clock */
void set_hw_clock 	( struct board_info *db );
/* set mac address */
int set_mac_addr 		( struct net_device *dev, char * );

void reset_eth 		( struct board_info *db );

void dump_reg 			( struct board_info *db );

#endif /* __CN100_HW_H__ */
