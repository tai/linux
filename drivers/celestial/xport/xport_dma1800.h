#ifndef __XPORT_DMA1800_H__
#define __XPORT_DMA1800_H__

#include "xport_regs1800.h"
#include "xport_drv1800.h"


#define CLK27_MHZ                       27	/* 27MHZ */
#define MAX_DMA_MBPS                    20	/* 20MB/s */

#define XPORT_CHL0_MIN_SPACES           (IN_CHL0_UNIT_NUM_DEF >> 1)
#define XPORT_CHL1_MIN_SPACES           (IN_CHL1_UNIT_NUM_DEF >> 1)

#define XPORT_IRQ0_DMA0_MSK             0x00000001
#define XPORT_IRQ0_DMA1_MSK             0x00000002

#define XPORT_IRQ0_DMA0_EMPTY_MSK       0x00000010
#define XPORT_IRQ0_DMA1_EMPTY_MSK       0x00000020
#define XPORT_IRQ0_DMA2_EMPTY_MSK       0x00000040
#define XPORT_IRQ0_DMA3_EMPTY_MSK       0x00000080

void xport_dma_init(void);
int xport_dma_set(unsigned int chl_id);
int xport_dma_write(const char __user * buffer, size_t len, unsigned int chl_id, ssize_t packet_size);
int xport_dma_direct_write(const char __user * buffer, size_t len,unsigned int chl_id, ssize_t packet_size);
int xport_dma_input_check(unsigned int dma_id);
int xport_dma_reset(unsigned int dma_id);
int xport_dma_half_empty_check(unsigned int dma_id,unsigned int chl_id);
int xport_dma_enable(unsigned int dma_id);


#endif



