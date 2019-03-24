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
 *  Set CN100 multicast address
 */
void
cn100_hash_table(struct net_device *dev)
{
	board_info_t *db = netdev_priv(dev);
	int mc_cnt = dev->mc_count;
	struct dev_mc_list *mcptr = dev->mc_list;
	unsigned long flags;
	int i;
	u32 rx_mode;
	u32 hash_table[2];
	u32 hash_val;

    if (dev->flags & IFF_PROMISC) {
            printk (KERN_NOTICE "%s: Promiscuous mode enabled.\n",
                    dev->name);
            rx_mode = 0x7;
            hash_table[1] = hash_table[0] = 0xffffffff;

    } else if ((dev->mc_count > 128)
               || (dev->flags & IFF_ALLMULTI)) {
            rx_mode = 0x3;
            hash_table[1] = hash_table[0] = 0xffffffff;

    } else {
	        rx_mode = 0x3;
	        hash_table[1] = hash_table[0] = 0x0;
	
	        for (i = 0; i < mc_cnt; i++, mcptr = mcptr->next) {
		        hash_val = ether_crc(6, mcptr->dmi_addr) >> (32 - 6);
		        hash_table[hash_val >> 5] |= 1 << (hash_val & 0x1f);
	        }

    }

	spin_lock_irqsave ( &db->lock, flags );

	write_reg32 ( db->io_addr, ETH_MAC_SA_MASK, rx_mode );
	write_reg32 ( db->io_addr, ETH_MAC_HASH_TAB1, hash_table[1]);
	write_reg32 ( db->io_addr, ETH_MAC_HASH_TAB0, hash_table[0]);
	
	spin_unlock_irqrestore(&db->lock,flags);
}
