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

/*
 *   Read a word from phyxcer
 */
int
cn100_phy_read(struct net_device *dev, int phy_id, int reg)
{
	board_info_t *db = (board_info_t *) netdev_priv(dev);
	u32 value = 0;

	write_reg32 ( db->io_addr, ETH_MAC_MADR, (phy_id<<8)|reg );
	do 
	{
		value = read_reg32  ( db->io_addr, ETH_MAC_MIND );
	} while ( value & 1 );	
	write_reg32 (db->io_addr, ETH_MAC_MCMD, 0x00000000);
	if(read_reg32 (db->io_addr, ETH_MAC_MCMD) != 0x00000000)
	{
		printk(KERN_WARNING "cn100_phy_read ERROR 1\n");
	}
	write_reg32 (db->io_addr, ETH_MAC_MCMD, 0x010000001);
	if (read_reg32 (db->io_addr, ETH_MAC_MCMD) != 0x00000001)
	{
		printk (KERN_WARNING "cn100_phy_read ERROR 2\n");
	}
	do
	{
		value = read_reg32 ( db->io_addr, ETH_MAC_MIND );
	} while ( value & 1 );
	value = read_reg32 ( db->io_addr, ETH_MAC_MRDD );
	return value;
}

/*
 *   Write a word to phyxcer
 */
void
cn100_phy_write(struct net_device *dev, int phy_id, int reg, int value_data )
{
	board_info_t *db = (board_info_t *) netdev_priv(dev);
	u32 value = 0;

	PRINTK3 ( "cn100_phy_write be call\n" );
	
	write_reg32 ( db->io_addr, ETH_MAC_MADR,  (phy_id<<8)|reg );
	do 
	{
		value = read_reg32 ( db->io_addr, ETH_MAC_MIND );
	} while ( value & 1 );
	
	write_reg32 ( db->io_addr, ETH_MAC_MWTD, value_data );
	
	do 
	{
		value = read_reg32  ( db->io_addr, ETH_MAC_MIND );
	} while ( value & 1 );

}
