#ifndef __XPORT_DRV_H__
#define __XPORT_DRV_H__

#include "xport_regs1800.h"
#include "xport_filter1800.h"
#include <mach/hardware.h>

#define  AUD_TEST


#define XPORT_TIMEOUT           msecs_to_jiffies(5)

#define XPORT_MEM_BASE          XPORT_REGION

#define XPORT_MEM_SIZE          XPORT_SIZE

#define MAX_FILTER_NUM          63

#define XPORT_CHL0_BASE_ADDR_DEF    XPORT_MEM_BASE

#define IN_CHL_UNIT_SIZE_DEF(n)    ((n)%2==0?((n)==0?IN_CHL0_UNIT_SIZE_DEF: IN_CHL2_UNIT_SIZE_DEF):((n)==1?IN_CHL1_UNIT_SIZE_DEF: IN_CHL3_UNIT_SIZE_DEF))
#define IN_CHL_UNIT_NUM_DEF(n)     ((n)%2==0?((n)==0?IN_CHL0_UNIT_NUM_DEF: IN_CHL2_UNIT_NUM_DEF):((n)==1?IN_CHL1_UNIT_NUM_DEF: IN_CHL3_UNIT_NUM_DEF))
#define IN_CHL_CFG_DEF(n)          ((n)%2==0?((n)==0?IN_CHL0_CFG_DEF: IN_CHL2_CFG_DEF):((n)==1?IN_CHL1_CFG_DEF: IN_CHL3_CFG_DEF))
#define IN_CHL_BASE_DEF(n)         ((n)%2==0?((n)==0?IN_CHL0_BASE_ADDR_DEF: IN_CHL2_BASE_ADDR_DEF):((n)==1?IN_CHL1_BASE_ADDR_DEF: IN_CHL3_BASE_ADDR_DEF))
#define IN_CHL_BUF_SIZE(n)         ((n)%2==0?((n)==0?IN_CHL0_BUF_SIZE: IN_CHL2_BUF_SIZE):((n)==1?IN_CHL1_BUF_SIZE: IN_CHL3_BUF_SIZE))

#ifndef CONFIG_XPORT_BIG_DMABUFFER
#define IN_CHL0_UNIT_SIZE_DEF    512
#define IN_CHL0_UNIT_NUM_DEF     256
#define IN_CHL0_BUF_SIZE         0x20000 // should equal/greater than IN_CHL0_UNIT_SIZE_DEF * IN_CHL0_UNIT_NUM_DEF
#define IN_CHL0_CFG_DEF          ((IN_CHL0_UNIT_NUM_DEF<<8) | (IN_CHL0_UNIT_SIZE_DEF>>3))
#define IN_CHL0_BASE_ADDR_DEF     XPORT_CHL0_BASE_ADDR_DEF

#define IN_CHL1_UNIT_SIZE_DEF    512
#define IN_CHL1_UNIT_NUM_DEF     256
#define IN_CHL1_BUF_SIZE         0x20000 // should equal/greater than IN_CHL1_UNIT_SIZE_DEF * IN_CHL1_UNIT_NUM_DEF
#define IN_CHL1_CFG_DEF          ((IN_CHL1_UNIT_NUM_DEF<<8) | (IN_CHL1_UNIT_SIZE_DEF>>3))
#define IN_CHL1_BASE_ADDR_DEF     (IN_CHL0_BASE_ADDR_DEF+IN_CHL0_BUF_SIZE)

#if (BASE_DDR_ADDR==0x8000000)
#define IN_CHL2_UNIT_NUM_DEF     0
#define IN_CHL2_UNIT_SIZE_DEF    0
#define IN_CHL2_BUF_SIZE 0
#else
#define IN_CHL2_UNIT_NUM_DEF     256
#define IN_CHL2_UNIT_SIZE_DEF    512
#define IN_CHL2_BUF_SIZE         0x20000
#endif
#define IN_CHL2_BASE_ADDR_DEF    (IN_CHL1_BASE_ADDR_DEF+ IN_CHL1_BUF_SIZE)
#define IN_CHL2_CFG_DEF          ((IN_CHL2_UNIT_NUM_DEF<<8) | (IN_CHL2_UNIT_SIZE_DEF>>3))

#if (BASE_DDR_ADDR==0x8000000)
#define IN_CHL3_UNIT_NUM_DEF     0
#define IN_CHL3_UNIT_SIZE_DEF    0
#define IN_CHL3_BUF_SIZE         0
#else
#define IN_CHL3_UNIT_NUM_DEF     256
#define IN_CHL3_UNIT_SIZE_DEF    512
#define IN_CHL3_BUF_SIZE         0x20000
#endif
#define IN_CHL3_BASE_ADDR_DEF    (IN_CHL2_BASE_ADDR_DEF + IN_CHL2_BUF_SIZE)
#define IN_CHL3_CFG_DEF          ((IN_CHL3_UNIT_NUM_DEF<<8) | (IN_CHL3_UNIT_SIZE_DEF>>3))

#else // define BIG DMA BUFFER

#define IN_CHL0_UNIT_SIZE_DEF    2000
#define IN_CHL0_UNIT_NUM_DEF     128
#define IN_CHL0_CFG_DEF          ((0x60000000) | (IN_CHL0_UNIT_NUM_DEF<<8) | (IN_CHL0_UNIT_SIZE_DEF>>3))
#define IN_CHL0_BASE_ADDR_DEF     XPORT_CHL0_BASE_ADDR_DEF

#define IN_CHL1_UNIT_SIZE_DEF    512
#define IN_CHL1_UNIT_NUM_DEF     256
#define IN_CHL1_CFG_DEF          ((IN_CHL1_UNIT_NUM_DEF<<8) | (IN_CHL1_UNIT_SIZE_DEF>>3))
#define IN_CHL1_BASE_ADDR_DEF    (IN_CHL0_BASE_ADDR_DEF+0x20000)

#define IN_CHL2_UNIT_SIZE_DEF    512
#define IN_CHL2_UNIT_NUM_DEF     256
#define IN_CHL2_CFG_DEF          ((IN_CHL2_UNIT_NUM_DEF<<8) | (IN_CHL2_UNIT_SIZE_DEF>>3))
#define IN_CHL2_BASE_ADDR_DEF    (IN_CHL1_BASE_ADDR_DEF+0x20000)

#define IN_CHL3_UNIT_SIZE_DEF    512
#define IN_CHL3_UNIT_NUM_DEF     256
#define IN_CHL3_CFG_DEF          ((IN_CHL3_UNIT_NUM_DEF<<8) | (IN_CHL3_UNIT_SIZE_DEF>>3))
#define IN_CHL3_BASE_ADDR_DEF    (IN_CHL2_BASE_ADDR_DEF+0x20000)
#endif // #ifndef CONFIG_XPORT_BIG_DMABUFFER


#define DMA0_BUF0_BASE_ADDR         (IN_CHL3_BASE_ADDR_DEF + IN_CHL3_BUF_SIZE)  //(XPORT_MEM_BASE+0x40000)
#define DMA0_BUF0_SIZE              0x10000
#define DMA0_BUF1_BASE_ADDR         (DMA0_BUF0_BASE_ADDR + DMA0_BUF0_SIZE) //(XPORT_MEM_BASE+0x50000)
#define DMA0_BUF1_SIZE              0x10000
#define DMA0_MAX_BLOCK_SIZE         2048    // 2k  bytes
#define DMA0_MAX_BLOCK_NUM          16

#define DMA1_BUF0_BASE_ADDR         (DMA0_BUF1_BASE_ADDR + DMA0_BUF1_SIZE)  // (XPORT_MEM_BASE+0x60000)
#define DMA1_BUF0_SIZE              0x10000
#define DMA1_BUF1_BASE_ADDR         (DMA1_BUF0_BASE_ADDR + DMA1_BUF0_SIZE) //(XPORT_MEM_BASE+0x70000)
#define DMA1_BUF1_SIZE              0x10000
#define DMA1_MAX_BLOCK_SIZE         2048    // 2k  bytes
#define DMA1_MAX_BLOCK_NUM          16

#define DMA_LIST_HEAD_SIZE          0x800
#define DMA0_LIST0_HEAD_ADDR        (DMA1_BUF1_BASE_ADDR + DMA1_BUF1_SIZE)  //(XPORT_MEM_BASE+0x80000)
#define DMA0_LIST1_HEAD_ADDR        (DMA0_LIST0_HEAD_ADDR + DMA_LIST_HEAD_SIZE)    //(XPORT_MEM_BASE+0x80800)
#define DMA1_LIST0_HEAD_ADDR        (DMA0_LIST1_HEAD_ADDR + DMA_LIST_HEAD_SIZE)    //(XPORT_MEM_BASE+0x81000)
#define DMA1_LIST1_HEAD_ADDR        (DMA1_LIST0_HEAD_ADDR + DMA_LIST_HEAD_SIZE)    //(XPORT_MEM_BASE+0x81800)

#define MIPS_EXTERNAL_BASE_ADDR     (DMA1_LIST1_HEAD_ADDR + DMA_LIST_HEAD_SIZE)    // size = 0xE000
#define MIPS_EXTERNAL_SIZE          0xE000

/* Filter Buffer define ******/
#define FLT_BUF_SIZE_1              0x20000

#if (BASE_DDR_ADDR==0x8000000)
#define FLT_BUF_SIZE_2              0x10000
#define FLT_PVR_BUF_SIZE            0x190000
#else
#define FLT_BUF_SIZE_2              0x20000
#define FLT_PVR_BUF_SIZE            0x270000
#endif

#define FLT_BUF_SIZE(x)             ((x)>FLT_BUF_SIZE_SPLIT ? (((x) > 60)? FLT_PVR_BUF_SIZE:FLT_BUF_SIZE_2): FLT_BUF_SIZE_1)
#define FLT_BUF_SIZE_SPLIT          (36)
#define FLT_BUF_BASE                (MIPS_EXTERNAL_BASE_ADDR + MIPS_EXTERNAL_SIZE)//(XPORT_MEM_BASE+0x90000)

#define FLT_NORMAL_SIZE_SUM(x)      (((x)>FLT_BUF_SIZE_SPLIT) ? (FLT_BUF_SIZE_1 * FLT_BUF_SIZE_SPLIT + FLT_BUF_SIZE_2 * (x-FLT_BUF_SIZE_SPLIT)) : (FLT_BUF_SIZE_1 * (x)) )
#define FLT_NORMAL_BASE_ADDR(y)     (FLT_BUF_BASE + FLT_NORMAL_SIZE_SUM(y))
#define FLT_PVR_BUFF_ADDR(m)        (FLT_NORMAL_BASE_ADDR(61) + (m-61)*FLT_PVR_BUF_SIZE) // m should be 61,62
#define FLT_BUF_BASE_ADDR(n)        ((n)<61 ?(FLT_NORMAL_BASE_ADDR(n)): (FLT_PVR_BUFF_ADDR(n)))

#define XPORT_MEM_USED_SIZE         (FLT_PVR_BUFF_ADDR(63) - XPORT_MEM_BASE) //(FLT_BUF_BASE - XPORT_MEM_BASE + FLT_BUF_SIZE * 61 + FLT_PVR_BUF_SIZE * 2)


/***********Driver Constant Defines*********/
#define XPORT_MAJOR                 100
#define XPORT_MINOR                 0

/*VID now support to 8 channels. but  with no limit in theory*/
#define XPORT_MAX_CHL_NUM           4
#define XPORT_AV_CHNL_NUM           8
#define XPORT_MINOR_CHL(n)          (10+n)
#define XPORT_MINOR_FT_BASE         100


#define XPORT_CFG0_TUNER0_SER_MOD       0x00000001
#define XPORT_CFG0_TUNER1_SER_MOD       0x00000002
#define XPORT_CFG0_CHL0_DMA_EN          0x00000004
#define XPORT_CFG0_CHL1_DMA_EN          0x00000010
#define XPORT_CFG0_CHL0_ATSC_EN         0x00000100
#define XPORT_CFG0_CHL1_ATSC_EN         0x00000200

#define XPORT_CFG1_OUT_CHL0_LINE_SYNC   0x00000001
#define XPORT_CFG1_OUT_CHL1_LINE_SYNC   0x00000002
#define XPORT_CFG1_OUT_CHL2_LINE_SYNC   0x00000004
#define XPORT_CFG1_OUT_CHL3_LINE_SYNC   0x00000008

#define XPORT_TUNER0_EN                 0x00000001
#define XPORT_TUNER1_EN                 0x00000002

#define XPORT_IRQ0                      16
#define XPORT_IRQ1                      17

#define XPORT_IRQ0_ID                   100
#define XPORT_IRQ1_ID                   101

/************ Driver Macro Defines*********/
extern void __iomem *xport_regs_base;
extern void __iomem *xport_mem_base;

#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
#define __IS_HW_ADDR__(x)              (((x)&0xbff00000)==XPORT_REG_ADDR_BASE)
#define __IS_MIPS_ADDR__(x)            (((x)&0xbff00000)==(XPORT_REG_ADDR_BASE+0x100000))
#define __WR_FLAGS__(x)                (((x)>>30)&0x1)
#define __OFFSET_ADDR__(x)             ((x)&0x000fffff)
#else
#define __IS_HW_ADDR__(x)              (((x)&0x7ff00000)==XPORT_REG_ADDR_BASE)
#define __IS_MIPS_ADDR__(x)            (((x)&0x7ff00000)==(XPORT_REG_ADDR_BASE+0x100000))
#define __WR_FLAGS__(x)                ((x)>>31)
#define __OFFSET_ADDR__(x)             ((x)&0x000fffff)
#endif

void xport_writeb(int a,int v);
void xport_writew(int a,int v);
void xport_writel(int a,int v);
unsigned char xport_readb(int a);
unsigned short xport_readw(int a);
unsigned int xport_readl(int a);

/******Xport Data Object*********/
typedef enum
{
	XPORT_INPUT_TS_TYPE = 0,
	XPORT_INPUT_M2TS_TYPE,
	XPORT_INPUT_TS204_TYPE,
	XPORT_INPUT_OTHER_TYPE
} XPORT_STREAM_TYPE;

typedef struct XPORT_DEV_t {
	unsigned int dev_minor;
	XPORT_FILTER_TYPE filter_type;
	XPORT_STREAM_TYPE stream_type;
	ssize_t packet_size;
	wait_queue_head_t wait_queue;
	wait_queue_head_t *irq_wait_queue_ptr;
	spinlock_t spin_lock;
	struct semaphore xport_sem;

} XPORT_DEV;

typedef struct DMA_LIST_NODE_t {
	unsigned int buf_addr;
	unsigned int data_size;
	struct DMA_LIST_NODE_t *next_addr;
	unsigned int next_valid;
} DMA_LIST_NODE;

typedef struct DMA_LIST_t {
	DMA_LIST_NODE *head_node;
	unsigned int max_block_size;
	unsigned int max_block_num;
	unsigned int cur_index;
	unsigned int next_index;
	unsigned int locked_flag;
	unsigned int block_size_sum;
} DMA_LIST;

typedef struct XPORT_DMA_t {
	DMA_LIST dma_list[2];
	unsigned long next_jiffies;
	unsigned int cur_index;
	struct semaphore dma_sem;
} XPORT_DMA;

typedef struct XPORT_DIR_T{
	struct semaphore dir_sem;
}XPORT_DIR;


#define XPORT_FILTER_IOC_PID0           _IOW('k', 1, int)
#define XPORT_FILTER_IOC_PID1           _IOW('k', 2, int)
#define XPORT_FILTER_IOC_PID2           _IOW('k', 3, int)
#define XPORT_FILTER_IOC_PID3           _IOW('k', 4, int)
#define XPORT_FILTER_IOC_FILTER         _IOW('k', 5, int)

#define XPORT_FILTER_IOC_TYPE           _IOW('k', 6, int)
#define XPORT_FILTER_IOC_ENABLE         _IOW('k', 8, int)

#define XPORT_FILTER_IOC_QUERY_NUM      _IOR('k', 10, int)
#define XPORT_FILTER_IOC_QUERY_SIZE     _IOR('k', 11, int)

#define XPORT_FILTER_IOC_CRC_ENABLE     _IOW('k', 12, int)

#define XPORT_PIDFT_IOC_ENABLE          _IOW('k', 13, int)
#define XPORT_PIDFT_IOC_CHANNEL         _IOW('k', 14, int)
#define XPORT_PIDFT_IOC_PIDVAL          _IOW('k', 15, int)
#define XPORT_PIDFT_IOC_DESC_ODDKEY     _IOW('k', 16, int)
#define XPORT_PIDFT_IOC_DESC_EVENKEY    _IOW('k', 17, int)
#define XPORT_PIDFT_IOC_DESC_ENABLE     _IOW('k', 18, int)

#define XPORT_CHL_IOC_ENABLE            _IOW('k', 19, int)
#define XPORT_CHL_IOC_INPUT_MODE        _IOW('k', 20, int)
#define XPORT_CHL_IOC_RESET             _IOW('k', 21, int)
#define XPORT_CHL_IOC_DMA_RESET         _IOW('k', 22, int)

#define XPORT_VID_IOC_OUTPUT_MODE       _IOW('k', 23, int)
#define XPORT_VID_IOC_RESET             _IOW('k', 24, int)
#define XPORT_VID_IOC_ENABLE            _IOW('k', 25, int)
#define XPORT_VID_IOC_PIDVAL            _IOW('k', 26, int)

#define XPORT_AUD_IOC_OUTPUT_MODE       _IOW('k', 27, int)
#define XPORT_AUD_IOC_RESET             _IOW('k', 28, int)
#define XPORT_AUD_IOC_ENABLE            _IOW('k', 29, int)
#define XPORT_AUD_IOC_PIDVAL            _IOW('k', 30, int)

#define XPORT_PCR_IOC_ENABLE            _IOW('k', 31, int)
#define XPORT_PCR_IOC_GETVAL            _IOW('k', 32, int)
#define XPORT_PCR_IOC_PIDVAL            _IOW('k', 33, int)

#define XPORT_FW_INIT                   _IOW('k', 34, int)
#define XPORT_FILTER_IOC_CRC_NTF_ENB    _IOW('k', 35, int)
#define XPORT_FILTER_IOC_SAVE_ENABLE    _IOW('k', 36, int)

#define XPORT_FILTER_IOC_PID4           _IOW('k', 37, int)
#define XPORT_FILTER_IOC_PID5           _IOW('k', 38, int)
#define XPORT_FILTER_IOC_PID6           _IOW('k', 39, int)
#define XPORT_FILTER_IOC_PID7           _IOW('k', 40, int)
#define XPORT_FILTER_IOC_PID8           _IOW('k', 41, int)
#define XPORT_FILTER_IOC_PID9           _IOW('k', 42, int)
#define XPORT_FILTER_IOC_PID10          _IOW('k', 43, int)
#define XPORT_FILTER_IOC_PID11          _IOW('k', 44, int)

#define XPORT_CHL_IOC_CLEAR             _IOW('k', 45, int)
#define XPORT_CHL_IOC_TUNER_MODE        _IOW('k', 46, int)

#define XPORT_VID_IOC_SWITCH            _IOW('k', 47, int)
#define XPORT_AUD_IOC_SWITCH            _IOW('k', 48, int)
#define XPORT_FILTER_IOC_SWITCH         _IOW('k', 49, int)
#define XPORT_CHL_IOC_DES_MODE          _IOW('k', 50, int)

#define XPORT_IOC_MEM_BASE_PHYADDR      _IOR('k', 51, int)
#define XPORT_IOC_MEM_SIZE              _IOR('k', 52, int)
#define XPORT_FILTER_IOC_BUFFER_UPADDR  _IOR('k', 53, int)
#define XPORT_FILTER_IOC_BUFFER_LOWADDR _IOR('k', 54, int)
#define XPORT_FILTER_IOC_BUFFER_RP_ADDR _IOR('k', 55, int)
#define XPORT_FILTER_IOC_BUFFER_WP_ADDR _IOR('k', 56, int)

#define XPORT_CHL_IOC_UNIT_NUM_DEF      _IOR('k', 57, int)
#define XPORT_CHL_IOC_UNIT_SIZE_DEF     _IOR('k', 58, int)
#define XPORT_CHL_IOC_MIN_SPACES        _IOR('k', 59, int)
#define XPORT_DMA_IOC_ENABLE            _IOR('k', 60, int)

#define XPORT_FLT_IOC_CLEAR             _IOR('k', 61, int)
#define XPORT_TUN_IOC_ENABLE            _IOR('k', 63, int)
#define XPORT_TUN_IOC_DISABLE           _IOR('k', 64, int)

#define XPORT_CHL_IOC_INPUT_TYPE        _IOR('k', 65, int)
#define XPORT_CHL_IOC_SERIAL_MODE       _IOW('k', 66, int)

#define XPORT_IOC_DMA0_LIST0_HEAD_ADDR  _IOR('k', 70, int)
#define XPORT_IOC_DMA0_BUF0_BASE_ADDR   _IOR('k', 71, int)
#define XPORT_IOC_DMA0_LIST1_HEAD_ADDR  _IOR('k', 72, int)
#define XPORT_IOC_DMA0_BUF1_BASE_ADDR   _IOR('k', 73, int)
#define XPORT_IOC_DMA0_MAX_BLOCK_SIZE   _IOR('k', 74, int)
#define XPORT_IOC_DMA0_MAX_BLOCK_NUM    _IOR('k', 75, int)
#define XPORT_IOC_DMA1_LIST0_HEAD_ADDR  _IOR('k', 76, int)
#define XPORT_IOC_DMA1_BUF0_BASE_ADDR   _IOR('k', 77, int)
#define XPORT_IOC_DMA1_LIST1_HEAD_ADDR  _IOR('k', 78, int)
#define XPORT_IOC_DMA1_BUF1_BASE_ADDR   _IOR('k', 79, int)
#define XPORT_IOC_DMA1_MAX_BLOCK_SIZE   _IOR('k', 80, int)
#define XPORT_IOC_DMA1_MAX_BLOCK_NUM    _IOR('k', 81, int)

#endif


