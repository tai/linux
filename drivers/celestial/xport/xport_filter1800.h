#ifndef __XPORT_FILTER_H__
#define __XPORT_FILTER_H__

typedef enum {
	FILTER_TYPE_NUKOWN = -1,
	FILTER_TYPE_SECTION = 0,
	FILTER_TYPE_TS,
	FILTER_TYPE_PES,
	FILTER_TYPE_ES
} XPORT_FILTER_TYPE;

typedef enum {
	INPUT_CHANNEL_0 = 0,
	INPUT_CHANNEL_1,
	INPUT_CHANNEL_2,
	INPUT_CHANNEL_3	
} XPORT_INPUT_CHANNEL;

typedef enum {
    INPUT_DMA0 = 0,
	INPUT_DMA1
}XPORT_DMA_INPUT_TYPE ;


#define XPORT_IRQ0_CRC_NOTIFY		0x00000100
#define XPORT_IRQ0_CRC_INDEX		0x00007e00
#define XPORT_IRQ0_CRC_IDX_SHIFT	0x9
#define XPORT_IRQ0_FILTER_NOTIFY	0x00008000	/* filter index > 31 */
#define XPORT_IRQ0_FILTER_INDEX		0x003f0000
#define XPORT_IRQ0_FILTER_IDX_SHIFT	0x10


int xport_filter_reset(unsigned int filter_index);

int xport_filter_set_type(unsigned int filter_index, XPORT_FILTER_TYPE filter_type);
int xport_filter_set_input(unsigned int filter_index, XPORT_INPUT_CHANNEL input_channel);
int xport_filter_set_crc(unsigned int filter_index, unsigned int crc_index);

int xport_filter_set_pidx(unsigned int filter_index, unsigned int pid, unsigned int slot);

int xport_filter_set_filter(unsigned int filter_index, unsigned char *filter, unsigned char *mask);

int xport_filter_crc_enable(unsigned int filter_index);
int xport_filter_crc_disable(unsigned int filter_index);
int xport_filter_crc_is_enable(unsigned int filter_index);

int xport_filter_enable(unsigned int filter_index, spinlock_t * spin_lock_ptr);
int xport_filter_disable(unsigned int filter_index, spinlock_t * spin_lock_ptr);
int xport_filter_is_enable(unsigned int filter_index);

int xport_filter_check_section_number(unsigned int filter_index);
int xport_filter_check_section_size(unsigned int filter_index);
int xport_filter_read_section_data(unsigned int filter_index, char __user * buffer, size_t len);

int xport_filter_check_data_size(unsigned int section_filter_id);
int xport_filter_read_data(unsigned int section_filter_id, char __user * buffer, size_t len);

int xport_filter_check_pes_size(unsigned int filter_index);
int xport_filter_read_pes_data(unsigned int filter_index, char __user * buffer, size_t len);

int xport_filter_clear_buffer(unsigned int filter_index);

unsigned int get_filter_buf_base_addr(unsigned int filter_index);
unsigned int get_filter_buf_top_addr(unsigned int filter_index);
unsigned int get_filter_buf_size(unsigned int filter_index);
unsigned int get_xport_filter_rp(unsigned int filter_index);
unsigned int get_xport_filter_wp(unsigned int filter_index);

#endif

