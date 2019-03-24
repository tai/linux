#include <linux/version.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/io.h>

#include "cn100_default.h"
#include "cn100.h"
#include "cn100_dbg.h"
#include "cn100_mem.h"
#include "cn100_hw.h"
#include "cn100_time.h"
#include "cn100_hash.h"
#include "cn100_phy.h"
#include "cn100_int.h"

int init_eth_hw ( struct net_device *dev, struct board_info *db )
{
//	u32 value = 0;
	
	PRINTK3("entering %s\n",__FUNCTION__);
	reset_eth ( db );

	set_rx_int	  ( db, 0 );
	set_tx_int	  ( db, 0 );	
	set_tx_enable ( db, 0 );
	set_rx_enable ( db, 0 );

	/* set rx DMA buffer */
	set_rx_buffer ( db );

	write_reg32 ( db->io_addr, ETH_CFG_ADDR, 0x00005500 );
	write_reg32 ( db->io_addr, ETH_MAC_SUPP, 0x00000100 );
	
	/* IPGT */
	write_reg32 ( db->io_addr, ETH_MAC_IPGT, 0x00000015 );
	/* IPGR */
	write_reg32 ( db->io_addr, ETH_MAC_IPGR, 0x00000c12 );
	
	/* Hash Table */
	write_reg32 ( db->io_addr, ETH_MAC_SA_MASK, 0x3 );
	write_reg32 ( db->io_addr, ETH_MAC_HASH_TAB0, 0x0 );
	write_reg32 ( db->io_addr, ETH_MAC_HASH_TAB1, 0x0 );

	/* Enable */
    napi_enable(&db->napi);
	set_tx_enable ( db, 1 );
	set_rx_enable ( db, 1 );
	set_rx_int	  ( db, 1 );
	set_tx_int    ( db, 1 );
	
	set_hw_clock ( db );
	
	/* Set address filter table */
	cn100_hash_table ( dev );
	
	/* Init Driver variable */
	db->tx_pkt_cnt = 0;
	dev->trans_start = 0;
	
	return 0;
}

int 
release_eth_hw ( struct net_device *ndev, struct board_info *db )
{
	reset_eth ( db );
	return 0;
}

int 
init_phy_hw ( struct net_device *dev, struct board_info *db )
{
	PRINTK3 ( "entering init_phy_hw\n");
	return 0;
}

int 
release_phy_hw ( struct net_device *dev, struct board_info *db )
{
	PRINTK3 ( "entering release_phy_hw\n");
	return 0;
}

/* enable or disable tx */
void 
set_tx_enable ( struct board_info *db, int enable )
{
	return;
}

/* enable or disable tx interrupt */
void 
set_tx_int ( struct board_info *db, int enable )
{
	u32 value = 0;
	
	value = read_reg32 ( db->io_addr, ETH_INT_ENB_ADDR );
	if ( enable )
	{
		value |= ETH_INT_TX;
	}
	else
	{
		value &= ~ETH_INT_TX;
	}
	write_reg32 ( db->io_addr, ETH_INT_ENB_ADDR, value );
}

/* enable or disable rx */
void 
set_rx_enable ( struct board_info *db, int enable )
{
	u32 value = 0;
	value = read_reg32 ( db->io_addr, ETH_MAC_CFG0 );
	if ( enable )
	{
		value |= 0x01;
	}
	else
	{
		value &= 0xFFFFFFFE;
	}
	
	write_reg32 ( db->io_addr, ETH_MAC_CFG0, value );
	
	value = read_reg32 ( db->io_addr, ETH_RX_CFG_ADDR );
	if ( enable )
	{
		value |= 0x80000000;
	}
	else
	{
		value &= 0x7FFFFFFF;
	}
	write_reg32 ( db->io_addr, ETH_RX_CFG_ADDR, value );
	
}

/* enable or disable rx interrupt */
void 
set_rx_int ( struct board_info *db, int enable )
{
	u32 value = 0;
	value = read_reg32 ( db->io_addr, ETH_INT_ENB_ADDR );
	if ( enable )
	{
		value |= ETH_INT_RX;
	}
	else
	{
		value &= ~ETH_INT_RX;
	}
	write_reg32 ( db->io_addr, ETH_INT_ENB_ADDR, value );
}

void
reset_eth ( struct board_info * db)
{
	/* disable rx and tx */
	set_rx_int	  ( db, 0 );
	set_tx_int	  ( db, 0 );	
	set_tx_enable ( db, 0 );
	set_rx_enable ( db, 0 );
	
	write_reg32 ( db->io_addr, ETH_MAC_CFG0, 0x00008000 );
	udelay ( 800 );
	write_reg32 ( db->io_addr, ETH_MAC_CFG0, 0x00000001 );
	write_reg32 ( db->io_addr, ETH_INT_SET_ADDR, 0 );
	write_reg32 ( db->io_addr, ETH_MAC_CFG1, 0xb6 );
	udelay ( 200 );
}

/* set eth clock div */
void 
set_hw_clock ( struct board_info *db )
{
	write_reg32 ( db->io_addr, ETH_MAC_MCFG, 0x1c );
    udelay(100);
}

/* set mac address */
int 
set_mac_addr ( struct net_device *ndev, char * mac_addr )
{
	struct board_info * db = netdev_priv(ndev);
	u32 val;

	if (!is_valid_ether_addr(mac_addr))
	{
		printk ("%s: Invalid ethernet MAC address. Please set using ifconfig\n", ndev->name);
		return -EINVAL;
	}

	/* Set Node Address */
    val = (u32)mac_addr[1];
    val = (val<<8) | mac_addr[0];
    write_reg32 ( db->io_addr, ETH_MAC_SA2, val );
    val = (u32)mac_addr[3];
    val = (val<<8) | mac_addr[2];
    write_reg32 ( db->io_addr, ETH_MAC_SA1, val );
    val = (u32)mac_addr[5];
    val = (val<<8) | mac_addr[4];
    write_reg32 ( db->io_addr, ETH_MAC_SA0, val );
    
    ndev->dev_addr[0] = (u8)mac_addr[0];	
    ndev->dev_addr[1] = (u8)mac_addr[1];	
    ndev->dev_addr[2] = (u8)mac_addr[2];	
    ndev->dev_addr[3] = (u8)mac_addr[3];	
    ndev->dev_addr[4] = (u8)mac_addr[4];	
    ndev->dev_addr[5] = (u8)mac_addr[5];	
	
	return 0;
}

int 
set_rx_buffer ( struct board_info *db )
{
	u32 value = 0;
	
	if ( 0 == db->rx_buffer.buffer_num )
	{
		return -EINVAL;
	}
	
	/* get virtual map physcial address */
	value = CN100_MEM_START + TX_BUFFER_LEN;
	write_reg32 ( db->io_addr, ETH_RX_BASE_ADDR, ( value >> 3 ) );
	value = db->rx_buffer.buffer_num;
	value = ( ( value << 8 ) & 0xFF00 ) | ( ( RX_BUFFER_ITEM_SIZE >> 3 ) & 0xFF ) ;
	write_reg32 ( db->io_addr, ETH_RX_CFG_ADDR, value );	
	/* set output ptr equal input ptr */
	value = read_reg32 ( db->io_addr, ETH_RX_WR_ADDR );
	write_reg32 ( db->io_addr, ETH_RX_RD_ADDR, value );
	
	return 0;
}

int 
set_tx_buffer 	( struct board_info *db )
{
	u32 value = 0;
	if ( NULL == db->tx_buffer )
	{
		return -EINVAL;
	}
	value = CN100_MEM_START;
	write_reg32 ( db->io_addr, ETH_TX_DMA_ADDR, value );
	return 0;
}

void 
dump_reg ( struct board_info *db )
{
#if 0
	printk ( "ETH_CFG_ADDR: 		0x%08x\n", 			read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0800 ) ) );
	printk ( "ETH_INT_REG_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0840 ) ) );
	printk ( "ETH_INT_ENB_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0844 ) ) );
//	printk ( "ETH_INT_SET_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0848 ) ) );
//	printk ( "ETH_INT_CLS_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x084c )	) );
	printk ( "ETH_TX_DMA_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09c0 ) ) );	
	printk ( "ETH_TX_STATES_ADDR: 0x%08x\n",      		read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09c4 ) ) );	
	printk ( "ETH_RX_BASE_ADDR: 	0x%08x\n",        	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09c8 ) ) );	
	printk ( "ETH_RX_CFG_ADDR: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09cc ) ) );	
	printk ( "ETH_RX_RD_ADDR: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09d0 ) ) );	
	printk ( "ETH_RX_WR_ADDR: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09d4 ) ) );	
	printk ( "ETH_RX_STATES_ADDR: 0x%08x\n",      		read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x09dc ) ) );	
	printk ( "ETH_MAC_SA_MASK: 	0x%08x\n",         	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a98 ) ) );	
	printk ( "ETH_MAC_HASH_TAB0: 	0x%08x\n",       	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a9c ) ) );	
	printk ( "ETH_MAC_HASH_TAB1: 	0x%08x\n",       	read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0aa0 ) ) );
	                                                            
	printk ( "ETH_MAC_CFG0: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a40 ) ) );	
	printk ( "ETH_MAC_CFG1: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a44 ) ) );	
	printk ( "ETH_MAC_IPGT: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a48 ) ) );	
	printk ( "ETH_MAC_IPGR: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a4c ) ) );	
	printk ( "ETH_MAC_CLRT: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a50 ) ) );	
	printk ( "ETH_MAC_MAXF: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a54 ) ) );	
	printk ( "ETH_MAC_SUPP: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a58 ) ) );	
	printk ( "ETH_MAC_TEST: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a5c ) ) );	
	printk ( "ETH_MAC_MCFG: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a60 ) ) );	
	printk ( "ETH_MAC_MCMD: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a64 ) ) );	
	printk ( "ETH_MAC_MADR: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a68 ) ) );	
	printk ( "ETH_MAC_MWTD: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a6c ) ) );	
	printk ( "ETH_MAC_MRDD: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a70 ) ) );	
	printk ( "ETH_MAC_MIND: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a74 ) ) );	
	printk ( "ETH_MAC_SA0: 			0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a80 ) ) );	
	printk ( "ETH_MAC_SA1: 			0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a84 ) ) );	
	printk ( "ETH_MAC_SA2: 			0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a88 ) ) );	
	printk ( "ETH_MAC_TSV0: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a8c ) ) );	
	printk ( "ETH_MAC_TSV1: 		0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a90 ) ) );	
	printk ( "ETH_MAC_RSV: 			0x%08x\n",          read_reg32 ( db->io_addr, ( CN100_BASEADDRESS + 0x0a94 ) ) );	
#endif
	return;
}
