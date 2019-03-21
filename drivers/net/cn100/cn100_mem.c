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

#include <mach/hardware.h>

#include "cn100_default.h"
#include "cn100.h"
#include "cn100_dbg.h"
#include "cn100_mem.h"
#include "cn100_hw.h"
#include "cn100_time.h"
#include "cn100_hash.h"
#include "cn100_phy.h"

static  ulong cn100_io_base = CN100_BASEADDRESS;

int init_eth_mem ( struct board_info *db )
{
	u32 value = 0;
	
	/* Alloc Reserved memory form HIGH mem */
  	if (!request_mem_region ( CN100_MEM_START, CN100_MEM_SIZE, "CN100 Reserved Mem"))
	{
		printk ( "Request CN100 High Mem Fail\n" );
		goto err_out_free_memregion;
	}
   	db->reserve_vir_addr = (u32)ioremap_nocache ( CN100_MEM_START, CN100_MEM_SIZE );
	if ( 0 == db->reserve_vir_addr ){
		printk ( "Get CN100 High Mem Fail\n" );
		goto err_out_free_memiomap;
	}
    else {
        PRINTK1("CN100: Maped Memory:0x%x in 0x%x, Size:0x%x\n",CN100_MEM_START, db->reserve_vir_addr, CN100_MEM_SIZE);
    }


	PRINTK3 (KERN_INFO "register io start = 0x%lx\n", (ulong)cn100_io_base);


	value = (u32) CN100_VA_BASEADDRESS; //(u32)ioremap(cn100_io_base, 0x1000);
	if ( 0 != value )
	{
		db->io_addr = value;
		PRINTK3 ( "io addr: 0x%08lx\n", (ulong)(db->io_addr) );
		return 0;
	}
	else
	{
		printk ( "Get CN100 Register Fail\n" );
		goto err_out_free_regiomap;
	}

err_out_free_regiomap:

err_out_free_memiomap:	
	release_mem_region ( CN100_MEM_START, CN100_MEM_SIZE );
err_out_free_memregion:
	return -EINVAL;
}

void uninit_eth_mem ( struct board_info *db )
{
	if ( NULL == db )
	{
		return;
	}

	if ( 0 != db->reserve_vir_addr )
	{
		iounmap ( (void *)( db->reserve_vir_addr ) );

		release_mem_region ( CN100_MEM_START, CN100_MEM_SIZE );
		db->reserve_vir_addr = 0;
	}

	if ( 0 != db->io_addr )
	{

		db->io_addr = 0;
	}
}

int alloc_eth_mem ( struct board_info *db )
{
	s32 i = 0;
	u32 value = 0;
	PRINTK3 ( "alloc_eth_mem ...\n" );

	if ( 0 == db->reserve_vir_addr )
	{
		return -EINVAL;
	}
	/* init tx data buffer struct */
	db->tx_buffer = (struct tx_buffer_t *)db->reserve_vir_addr;
	db->tx_buffer_t.size = TX_BUFFER_DATA_LEN;
	db->tx_buffer_t.vir_address = (u32)(((u8 *)db->reserve_vir_addr) + TX_BUFFER_NOD_LEN );
	db->tx_buffer_t.phy_address = CN100_MEM_START + TX_BUFFER_NOD_LEN;
	
	/* init tx node buffer struct */
	value = db->tx_buffer_t.phy_address;
	db->tx_buffer->buffer_addr = SWAP ( value );
	db->tx_buffer->buffer_len = 0;
	db->tx_buffer->next_link_item = 0;
	db->tx_buffer->next_item_valid_flag = 0;
	
	/* init rx buffer struct */
	db->rx_buffer.buffer_num = RX_BUFFER_ITEM_NUM;
	for ( i = 0; i < RX_BUFFER_ITEM_NUM; i ++ )
	{
		db->rx_buffer.rx_buffer_addr[i] = (u32)(((u8 *)db->reserve_vir_addr) + TX_BUFFER_LEN + RX_BUFFER_ITEM_SIZE * i );
        PRINTK3("Rx buffer item %d address is 0x%x\n", i, db->rx_buffer.rx_buffer_addr[i]);
	}
	return 0;
}

int release_eth_mem ( struct board_info *db )
{
	s32 i = 0;
	PRINTK3 ( "release_eth_mem ...\n" );

	if ( NULL != db->tx_buffer )
	{
		db->tx_buffer = NULL;
	}
	db->tx_buffer_t.size = 0;
	db->tx_buffer_t.phy_address = 0;
	db->tx_buffer_t.vir_address = 0;
	
	/* RX */
	for ( i = 0; i < db->rx_buffer.buffer_num; i ++ )
	{
		db->rx_buffer.rx_buffer_addr[i] = 0;
	}
	db->rx_buffer.buffer_num = 0;
	
	return 0;
}
