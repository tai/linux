#ifndef __CN100_MEM_H__
#define __CN100_MEM_H__

#define SWAP(x) ( ( ( x >> 24 ) & 0xFF ) | ( ( x >> 8 ) & 0xFF00 ) | ( ( x << 8 ) & 0xFF0000 ) | ( ( x << 24 ) & 0xFF000000 ) )

#define FPGA_START_ADDRESS 	0xfe000000

#define write_reg32(base, offset, value)	writel(value, (base + offset))
#define read_reg32(base, offset)		readl((base + offset))

int init_eth_mem 		( struct board_info *db );
void uninit_eth_mem 	( struct board_info *db );

int alloc_eth_mem 	( struct board_info *db );
int release_eth_mem 	( struct board_info *db );

#endif /* __CN100_MEM_H__ */
