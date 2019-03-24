#ifndef __CN100_DEFAULT_H__
#define __CN100_DEFAULT_H__

#include <mach/irqs.h>
#define _RESERVED_MEM_
#define _CN100_MII_

#define RX_BUFFER_ITEM_SIZE 		1600
#define RX_BUFFER_ITEM_NUM 		    255
#define TX_BUFFER_NOD_LEN			( 16 )
#define TX_BUFFER_DATA_LEN			( 2048 )
#define TX_BUFFER_LEN				( TX_BUFFER_NOD_LEN +  TX_BUFFER_DATA_LEN )
#define RX_BUFFER_LEN				( RX_BUFFER_ITEM_SIZE * RX_BUFFER_ITEM_NUM )

#define CN100_MEM_START 			( ETHERNET_REGION ) /* Reserved MEMORY start position is 123M 	*/
#define CN100_MEM_SIZE  			( ETHERNET_SIZE )  /* Reserved MEMORY length is 1M 			*/

#define CN100_IRQ  					IRQ_ETH_DMA

#endif /* __CN100_DEFAULT_H__ */
