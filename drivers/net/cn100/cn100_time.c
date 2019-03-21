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

#ifdef _CN100_MII_
static void
cn100_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *) data;
	board_info_t *db = (board_info_t *) netdev_priv(dev);;

	mii_check_media ( &db->mii, netif_msg_link(db), 0 );
	if(db->mii.full_duplex)
		write_reg32(db->io_addr, ETH_MAC_CFG1, read_reg32(db->io_addr, ETH_MAC_CFG1) |  0x1);
	else
		write_reg32(db->io_addr, ETH_MAC_CFG1, read_reg32(db->io_addr, ETH_MAC_CFG1) & ~0x1);

	/* Set timer again */
	db->timer.expires = CN100_TIMER_WUT;
	add_timer(&db->timer);
}
#endif

/* Our watchdog timed out. Called by the networking layer */
void 
cn100_timeout(struct net_device *dev)
{
	unsigned long flags;
	board_info_t *db = (board_info_t *) netdev_priv(dev);;

	spin_lock_irqsave(&db->lock, flags);
	netif_stop_queue(dev); 
	db->reset_tx_timeout++;
	db->reset_counter++;
	dev->trans_start = 0;
	init_eth_hw ( dev, db );
	netif_wake_queue(dev);
	/* We can accept TX packets again */
	dev->trans_start = jiffies;
	spin_unlock_irqrestore(&db->lock,flags);
}

#ifdef _CN100_MII_
int 
init_eth_timer ( struct net_device *dev )
{
	board_info_t *db 			= (board_info_t *) netdev_priv(dev);
	init_timer(&db->timer);
	db->timer.expires  		= CN100_TIMER_WUT;
	db->timer.data     		= (unsigned long) dev;
	db->timer.function 		= &cn100_timer;
	add_timer( &db->timer );

	return 0;
}

void 
release_eth_timer ( struct board_info * db )
{
	del_timer ( &db->timer );
}
#endif
