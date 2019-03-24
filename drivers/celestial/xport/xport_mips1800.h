
#ifndef __XPORT_MIPS1800_H__
#define __XPORT_MIPS1800_H__

#include "xport_regs1800.h"


/*x stand for channel index. here Max to 19*/
#define MIPS_CHL_EN(x)                  ((x<<8)|MIPS_OUTCHL_EN)

/****n~(0,11)  x~(0,60) .note: pid0 can be used in every filter.
  but  pid1-pid11 should be used only in PVR channel (filter 63)**/
#define MIPS_CHL_PID(n,x)               ((x<<8)|MIPS_OUTCHL_PID(n))
#define MIPS_CHL_FILTER0(x)             ((x<<8)|MIPS_OUTCHL_FILTER0)
#define MIPS_CHL_FILTER1(x)             ((x<<8)|MIPS_OUTCHL_FILTER1)
#define MIPS_CHL_FILTER2(x)             ((x<<8)|MIPS_OUTCHL_FILTER2)
#define MIPS_CHL_MASK0(x)               ((x<<8)|MIPS_OUTCHL_MASK0)
#define MIPS_CHL_MASK1(x)               ((x<<8)|MIPS_OUTCHL_MASK1)
#define MIPS_CHL_MASK2(x)               ((x<<8)|MIPS_OUTCHL_MASK2)
#define MIPS_CHL_DIR_LOW_ADDR(x)        ((x<<8)|MIPS_OUTCHL_DIR_LOW_ADRR)
#define MIPS_CHL_DIR_UP_ADDR(x)         ((x<<8)|MIPS_OUTCHL_DIR_UP_ADRR)
#define MIPS_CHL_WP(x)                  ((x<<8)|MIPS_OUTCHL_DIR_WP)
#define MIPS_CHL_TC0_ERR_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_TC0_ERR_CNT)
#define MIPS_CHL_TC1_ERR_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_TC1_ERR_CNT)
#define MIPS_CHL_TC2_ERR_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_TC2_ERR_CNT)
#define MIPS_CHL_TC3_ERR_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_TC3_ERR_CNT)
#define MIPS_CHL_ERR_PKT_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_ERR_PACKET_CNT)
#define MIPS_CHL_OUT_PKT_CNT(x)         ((x<<8)|MIPS_OUTCHL_BUF_OUT_PACKET_CNT)
#define MIPS_CHL_BUF_LOW_ADDR(x)        ((x<<8)|MIPS_OUTCHL_BUF_LOW_ADRR)
#define MIPS_CHL_BUF_UP_ADDR(x)         ((x<<8)|MIPS_OUTCHL_BUF_UP_ADRR)
#define MIPS_PCR_PID(x)                 ((x<<8)|MIPS_PCR_PID_ADDR)
#define MIPS_PCR_GET(x)                 ((x<<8)|MIPS_PCR_GET_ADDR)
#define MIPS_CHL_CRC_EN(x)              ((x<<8)|MIPS_OUTCHL_CRC_EN)
#define MIPS_CHL_CRC_NOTIFY_EN(x)       ((x<<8)|MIPS_OUTCHL_CRC_NOTIFY_EN)
#define MIPS_CHL_DISABLE(x)             ((x<<8)|MIPS_OUTCHL_DISABLE)
#define MIPS_CHL_SWITCH(x)              ((x<<8)|MIPS_OUTCHL_SWITCH)
#define MIPS_CHL_EXTERNAL_BASE(x)       (((x)<<8)|MIPS_EXTERNAL_BASE)
#define MIPS_CHL_BITWISE0(x)            (((x)<<8)|MIPS_OUTCHL_BITWISE0)
#define MIPS_CHL_BITWISE1(x)            (((x)<<8)|MIPS_OUTCHL_BITWISE1)
#define MIPS_CHL_BITWISE2(x)            (((x)<<8)|MIPS_OUTCHL_BITWISE2)
#define MIPS_CHL_FILTER_MAX_SIZE(x)     (((x)<<8)|MIPS_OUTCHL_FILTER_MAX_SIZE) 


/**I/D ram addr for storeing firmware message**/
#define MIPS_FW_WRITE_DATA              (0xc1400000+(0x4000<<2))
#define MIPS_FW_WRITE_INST              (0xc1400000+(0x2000<<2))
#define MIPS_FW_EN                      (0xc1400000+(0x6000<<2))

/*firmware appoint MBOX0/1/4 as its info exchange to host for communicate*/
#define MIPS_CMD_REQ 	                MAIL_BOX_ADDR(0)
#define MIPS_CMD_DATA	                MAIL_BOX_ADDR(1)
#define MIPS_CMD_DATA2	                MAIL_BOX_ADDR(4)


union firmware_req {

	struct {
		unsigned int req_type:8;
		unsigned int output_idx:8;
		unsigned int reserved:14;
		unsigned int rw:1;
		unsigned int enable:1;
	} bits;

	unsigned int val;
};

int xport_mips_write(unsigned int cmd, unsigned int req_dat);
int xport_mips_read(unsigned int cmd, unsigned int *req_dat);
int xport_mips_read_ex(unsigned int cmd, unsigned int *req_dat, unsigned int *req_dat2);

#endif

