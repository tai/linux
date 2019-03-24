#ifndef __CELESTIAL_I2C_H__
#define	__CELESTIAL_I2C_H__

#define		IC_ENABLE			0x00
#define		IC_SCLH_CNT			0x04
#define		IC_SCLL_CNT			0x08
#define		IC_DATA_CMD			0x0c
#define		IC_INTR_STAT		0x10
#define		IC_RAW_INTR_STAT	0x14
#define		IC_INTR_MASK		0x18
#define		IC_INTR_CLR			0x1c
#define		IC_STAT				0x20
#define		IC_RX_TL			0x24
#define		IC_TX_TL			0x28
#define		IC_TXFLR			0x2c
#define		IC_RXFLR			0x30
#define		IC_FIFO_CLR			0x34
#define		IC_RESET			0x38
#define		IC_SLV_TIMEOUT_L	0x3c
#define		IC_SLV_TIMEOUT_H	0x3e	
#define		IC_SLV_TIMEOUT_EN	0x40

#endif

