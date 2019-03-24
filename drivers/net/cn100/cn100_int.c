
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "cn100_default.h"
#include "cn100.h"
#include "cn100_dbg.h"
#include "cn100_mem.h"
#include "cn100_hw.h"
#include "cn100_time.h"
#include "cn100_hash.h"
#include "cn100_phy.h"
#include "cn100_int.h"

extern struct net_device *cn100_devs;
static inline void cn100_tx_done(struct net_device *dev, board_info_t * db)
{
	/* One packet sent complete */
	db->tx_pkt_cnt--;
	db->stats.tx_packets++;
	netif_wake_queue(dev);
}

irqreturn_t cn100_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	board_info_t *db = netdev_priv(dev);
	u32 io_addr = db->io_addr;
	int int_status;


	spin_lock(&db->lock);

	int_status = read_reg32(io_addr, ETH_INT_REG_ADDR);
	if (unlikely(!(int_status & ETH_INT_ALL))) {
		printk(KERN_WARNING "Unexpected eth interrupts status=0x%08lx\n", (ulong)int_status);
		spin_unlock(&db->lock);
		return IRQ_NONE;
	}

    /* Receive packets are processed by poll routine.
       If not running start it now. */
	if (int_status & ETH_INT_RX) {
        PRINTK3("CN100 Int RX\n");
        if (likely(napi_schedule_prep(&db->napi))) {
            write_reg32(io_addr, ETH_INT_ENB_ADDR, ETH_INT_TX);
            __napi_schedule (&db->napi);
        }
	}
    
	/* Trnasmit interrupt check */
	if (int_status & ETH_INT_TX){
        PRINTK3("CN100 Int TX\n");
		write_reg32(io_addr, ETH_INT_CLS_ADDR, ETH_INT_TX);
		cn100_tx_done (dev, db);
	}

	spin_unlock(&db->lock);

	return IRQ_HANDLED;
}

static int cn100_rx(struct net_device *dev, struct board_info *np,
                    int budget)
{
    u32 io_addr = np->io_addr;
    int received = 0;
	u32 pkt_size = 0;
	u32 read_ptr = read_reg32(io_addr, ETH_RX_RD_ADDR);
	u8 *rdptr;

    while (netif_running(dev) && (received < budget)
           && ((read_ptr & ETH_FIFO_EMPTY) == 0)) {
        
        struct sk_buff *skb;
        PRINTK3("in cn100 rx while read_ptr=0x%x ;  bufferptr=0x%x\n",read_ptr, np->rx_buffer.rx_buffer_addr[read_ptr & 0x3FFFFFFF]);
		pkt_size = *(volatile u32 *)(np->rx_buffer.rx_buffer_addr[read_ptr & 0x3FFFFFFF] + 4);
		pkt_size = be32_to_cpu(pkt_size);
		/* Status check: pkt_size should always be valid */
		if(unlikely(pkt_size > RX_BUFFER_ITEM_SIZE)) 
            {
                printk(KERN_WARNING "Hardware ERROR? Packet 0x%08lx len 0x%08lx\n", (ulong)read_ptr, (ulong)pkt_size);
                break;
            }
		/* Copy packet to skb_buff */
        skb = dev_alloc_skb (pkt_size + 4);
        if (likely(skb)) {
            skb->dev = dev;
            skb_reserve (skb, 4);

			rdptr = skb_put(skb, pkt_size);
			memcpy (rdptr, (u8 *)(np->rx_buffer.rx_buffer_addr[read_ptr & 0x3FFFFFFF] + 8), pkt_size);

            skb->protocol = eth_type_trans (skb, dev);

            dev->last_rx = jiffies;
            np->stats.rx_bytes += pkt_size;
            np->stats.rx_packets++;

            netif_receive_skb (skb);
        } else {
            if (net_ratelimit())
                printk (KERN_WARNING
                        "%s: Memory squeeze, dropping packet.\n",
                        dev->name);
            np->stats.rx_dropped++;
        }
        received++;

		/* Ack the packet and update RX ring */
		write_reg32 (io_addr, ETH_INT_CLS_ADDR, ETH_INT_RX);

		if(unlikely((++read_ptr & 0x3FFFFFFF) == np->rx_buffer.buffer_num))
			read_ptr = ~read_ptr & ETH_FIFO_TOG;

		write_reg32(io_addr, ETH_RX_RD_ADDR, read_ptr);
		read_ptr = read_reg32(io_addr, ETH_RX_RD_ADDR);		/* Including the EMPTY bit */

    }

	if(unlikely(!received || pkt_size > RX_BUFFER_ITEM_SIZE))
		write_reg32 (io_addr, ETH_INT_CLS_ADDR, ETH_INT_RX);

    return received;
}

/*
 *
 * NAPI poll: optimized for high bandwidth inbound data
 *
 */
int cn100_poll(struct napi_struct *napi, int budget)
{
    board_info_t *np = container_of(napi, board_info_t, napi); 
	u32 io_addr = np->io_addr;
    //    int orig_budget = min(budget, cn100_devs->quota);
    int done = 1;
    PRINTK3("in cn100_poll calling\n");
    spin_lock(&np->rx_lock);
    if (likely(!(read_reg32(io_addr, ETH_RX_RD_ADDR) & ETH_FIFO_EMPTY))) {
        int work_done;

        work_done = cn100_rx(cn100_devs, np, budget);

        if (likely(work_done > 0)) {
            budget -= work_done;
            //dev->quota -= work_done;
            done = (work_done < budget);
        }
    }
    
    if (done) {
        /*
         * Order is important since data can get interrupted
         * again when we think we are done.
         */
        //napi_complete(napi);

        local_irq_disable();
        __napi_complete(napi);
        write_reg32(io_addr, ETH_INT_ENB_ADDR, ETH_INT_ALL);
        // __netif_rx_complete(dev);
        local_irq_enable();
    }
    spin_unlock(&np->rx_lock);
    
    return !done;
}

/*
 *  Hardware start transmission.
 *  Send a packet to media from the upper layer.
 */
int cn100_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	board_info_t *db = (board_info_t *) netdev_priv(dev);
	u32 value = 0;
	
	if (db->tx_pkt_cnt > 0)
	{
		printk (KERN_WARNING "ethernet driver bug: db->tx_pkt_cnt > 0\n");
		return 1;
	}
	netif_stop_queue(dev);
	memcpy ( (void *)(u32 *)(db->tx_buffer_t.vir_address ),
				 skb->data,
				 skb->len );

	/* buffer_len */
	value = skb->len;
	db->tx_buffer->buffer_len = cpu_to_be32 ( value );
	db->stats.tx_bytes += skb->len;
	db->tx_pkt_cnt++;
	
	dev->trans_start = jiffies;	/* save the time stamp */
	dev_kfree_skb(skb);

	spin_lock_irq(&db->lock);
	set_tx_buffer ( db );
	spin_unlock_irq(&db->lock);
	
	return 0;
}
