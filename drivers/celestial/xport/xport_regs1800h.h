#ifndef _XPORT_REGS1800_H__
#define _XPORT_REGS1800_H__


#define xport_set_bit(v,n)                (v|(0x1<<(n)))
#define xport_cls_bit(v,n)                (v&~(0x1<<(n)))
#define xport_get_bit(v,n)                (((v)>>(n))&0x1)
#define xport_get_bits(v,m,n)/*m>=n*/     ((v<<(31-m))>>(31-m+n))



#define XPORT_REG_ADDR_BASE         PA_XPORT_REG_BASE

/***********************HW Register Define********************/

/************************Mail Box Addr0-7**********************/
#define XPORT_REG_OFST              0x4
#define MAIL_BOX_ADDR(n)            (XPORT_REG_ADDR_BASE+((n)*XPORT_REG_OFST))




/************************Xport Config Addr**********************/
#define XPORT_CFG_ADDR0             (XPORT_REG_ADDR_BASE+0x20)
#define XPORT_CFG_ADDR1             (XPORT_REG_ADDR_BASE+0x24)


/*********************Xport Config to Tuner Addr******************/
#define XPORT_TUN_STS_ADDR          (XPORT_REG_ADDR_BASE+0x28)
#define XPORT_TUN_STS_CFG_ADDR      (XPORT_REG_ADDR_BASE+0x2C)
#define DMA_INPUT0_HEAD_ADDR        (XPORT_REG_ADDR_BASE+0x30)
#define DMA_INPUT1_HEAD_ADDR        (XPORT_REG_ADDR_BASE+0x34)


/*******************Xport Raw Interrupt Reg Addr******************/
#define XPORT_INT_RAW_REG_ADDR0     (XPORT_REG_ADDR_BASE+0x38)
#define XPORT_INT_RAW_REG_ADDR1     (XPORT_REG_ADDR_BASE+0x3C)


/*******************Xport Interrupt Reg Addr0*********************/
#define XPORT_INT_REG_ADDR0         (XPORT_REG_ADDR_BASE+0x40)
#define XPORT_INT_ENB_ADDR0         (XPORT_REG_ADDR_BASE+0x44)
#define XPORT_INT_SET_ADDR0         (XPORT_REG_ADDR_BASE+0x48)
#define XPORT_INT_CLS_ADDR0         (XPORT_REG_ADDR_BASE+0x4C)


/*******************Xport Interrupt Reg Addr1*********************/
#define XPORT_INT_REG_ADDR1         (XPORT_REG_ADDR_BASE+0x50)
#define XPORT_INT_ENB_ADDR1         (XPORT_REG_ADDR_BASE+0x54)
#define XPORT_INT_SET_ADDR1         (XPORT_REG_ADDR_BASE+0x58)
#define XPORT_INT_CLS_ADDR1         (XPORT_REG_ADDR_BASE+0x5C)
#define XPORT_INT_EXT_ADDR          (MAIL_BOX_ADDR(5)) 




/********************Xport  Enable Tuner0/1**********************/
#define XPORT_TUNER_EN              (XPORT_REG_ADDR_BASE+0x60)


/*************Input channel update configure register****************/
#define XPORT_IN_CHL_UP_CFG         (XPORT_REG_ADDR_BASE+0x64)

#define CLK0_STC_HIGH_ADDR          (XPORT_REG_ADDR_BASE+0x80)
#define CLK0_STC_LOW_ADDR           (XPORT_REG_ADDR_BASE+0x84)

#define CLK0_PCR_HIGH_ADDR          (XPORT_REG_ADDR_BASE+0x88)
#define CLK0_PCR_LOW_ADDR           (XPORT_REG_ADDR_BASE+0x8C)

#define CLK0_PCR_CNT_ADDR           (XPORT_REG_ADDR_BASE+0x90)
#define CLK0_PCR_INTERVAL_ADDR      (XPORT_REG_ADDR_BASE+0x94)


/*************Xport Clock Component Addr************************/
#define CLK0_DELTA_CYL_ADDR         (XPORT_REG_ADDR_BASE+0x98)
#define CLK0_DELTA_CYL_THRS_ADDR    (XPORT_REG_ADDR_BASE+0x9C)
#define CLK0_PWM_CTRL_ADDR          (XPORT_REG_ADDR_BASE+0xA4)
#define CLK0_PWM_CWORD_ADDR         (XPORT_REG_ADDR_BASE+0xA8)
#define CLK0_PCR_VALID_ADDR         (XPORT_REG_ADDR_BASE+0xAC)
#define CLK0_REC_DELAY_TIME_ADDR    (XPORT_REG_ADDR_BASE+0xB0)
#define CLK0_CLK_CNT_ADDR           (XPORT_REG_ADDR_BASE+0xB4)


/**********Xport input channel generate macro Addr *****************/
#define IN_CHL_BASE_ADDR(n)         ((n)%2==0?((n)==0?IN_CHL0_BASE_ADDR: IN_CHL2_BASE_ADDR):((n)==1?IN_CHL1_BASE_ADDR: IN_CHL3_BASE_ADDR))
#define IN_CHL_CFG_ADDR(n)          ((n)%2==0?((n)==0?IN_CHL0_CFG_ADDR : IN_CHL2_CFG_ADDR) :((n)==1?IN_CHL1_CFG_ADDR : IN_CHL3_CFG_ADDR))
#define IN_CHL_RP_ADDR(n)           ((n)%2==0?((n)==0?IN_CHL0_RP_ADDR  : IN_CHL2_RP_ADDR)  :((n)==1?IN_CHL1_RP_ADDR  : IN_CHL3_RP_ADDR))
#define IN_CHL_WP_ADDR(n)           ((n)%2==0?((n)==0?IN_CHL0_WP_ADDR  : IN_CHL2_WP_ADDR)  :((n)==1?IN_CHL1_WP_ADDR  : IN_CHL3_WP_ADDR))


/*************Xport input Channel0 Option Addr ********************/
#define IN_CHL0_BASE_ADDR           (XPORT_REG_ADDR_BASE+0x100)
#define IN_CHL0_CFG_ADDR            (XPORT_REG_ADDR_BASE+0x104)
#define IN_CHL0_RP_ADDR             (XPORT_REG_ADDR_BASE+0x108)
#define IN_CHL0_WP_ADDR             (XPORT_REG_ADDR_BASE+0x10C)


/*************Xport input Channel1 Option Addr ********************/
#define IN_CHL1_BASE_ADDR           (XPORT_REG_ADDR_BASE+0x118)
#define IN_CHL1_CFG_ADDR            (XPORT_REG_ADDR_BASE+0x11C)
#define IN_CHL1_RP_ADDR             (XPORT_REG_ADDR_BASE+0x120)
#define IN_CHL1_WP_ADDR             (XPORT_REG_ADDR_BASE+0x124)



/*************Xport input Channel2 Option Addr ********************/
#define IN_CHL2_BASE_ADDR           (XPORT_REG_ADDR_BASE+0x130)
#define IN_CHL2_CFG_ADDR            (XPORT_REG_ADDR_BASE+0x134)
#define IN_CHL2_RP_ADDR             (XPORT_REG_ADDR_BASE+0x138)
#define IN_CHL2_WP_ADDR             (XPORT_REG_ADDR_BASE+0x13C)


/*************Xport input Channel3 Option Addr ********************/
#define IN_CHL3_BASE_ADDR           (XPORT_REG_ADDR_BASE+0x148)
#define IN_CHL3_CFG_ADDR            (XPORT_REG_ADDR_BASE+0x14C)
#define IN_CHL3_RP_ADDR             (XPORT_REG_ADDR_BASE+0x150)
#define IN_CHL3_WP_ADDR             (XPORT_REG_ADDR_BASE+0x154)


/****xunli: direct dma mode ****/
#define XPORT_CHL_DMA_WP_ADDR(n)    ((n)<2?MAIL_BOX_ADDR(n+2): MAIL_BOX_ADDR(n+4))
#define XPORT_CHL_DMA0_WP_ADDR      (MAIL_BOX_ADDR(2))
#define XPORT_CHL_DMA1_WP_ADDR      (MAIL_BOX_ADDR(3))
#define XPORT_CHL_DMA2_WP_ADDR      (MAIL_BOX_ADDR(6))
#define XPORT_CHL_DMA3_WP_ADDR      (MAIL_BOX_ADDR(7))


/********For Setting Xport filter Group0/1 (0-63/64-128)**************/

#define __PID_FILTER__(n) (n>63?PID_GRP1_FLTR(n): PID_GRP0_FLTR(n))
/***Set Pid filter Reg 0-63***/
#define FLTR_PID_ADDR_BASE0         (0x180+XPORT_REG_ADDR_BASE)
#define PID_GRP0_FLTR(n)            (FLTR_PID_ADDR_BASE0+(XPORT_REG_OFST*(n)))         


/***Set Pid filter Reg 64-127***/
#define FLTR_PID_ADDR_BASE1         (0xC00+XPORT_REG_ADDR_BASE)
#define PID_GRP1_FLTR(n)            (FLTR_PID_ADDR_BASE1+(XPORT_REG_OFST*(n-64)))         



/******************Xport Descramble Reg Addr Base***********************/
#define DSCRB_REG_ODD_BASE          (XPORT_REG_ADDR_BASE+0x280)
#define DSCRB_REG_EVN_BASE          (XPORT_REG_ADDR_BASE+0x298)

/**i~(0,5) for Key Cnt.   j~(0,7) for Group Cnt**/
#define __DSCRB_ODD_ADDR_(i, j)     (DSCRB_REG_ODD_BASE+(XPORT_REG_OFST*(i))+(0x30*(j)))
#define __DSCRB_EVN_ADDR_(i, j)     (DSCRB_REG_EVN_BASE+(XPORT_REG_OFST*(i))+(0x30*(j)))


/**32 output channel define ?**/
/******************MIPS software Register define************************/

#define MIPS_OUTCHL_EN                   0x0000
#define MIPS_OUTCHL_FILTER0              0x0001
#define MIPS_OUTCHL_FILTER1              0x0002
#define MIPS_OUTCHL_FILTER2              0x0003
#define MIPS_OUTCHL_MASK0                0x0004
#define MIPS_OUTCHL_MASK1                0x0005
#define MIPS_OUTCHL_MASK2                0x0006
#define MIPS_OUTCHL_DIR_LOW_ADRR         0x0007
#define MIPS_OUTCHL_DIR_UP_ADRR          0x0008
#define MIPS_OUTCHL_DIR_WP               0x0009
#define MIPS_OUTCHL_BUF_TC0_ERR_CNT      0x000a
#define MIPS_OUTCHL_BUF_TC1_ERR_CNT      0x000b
#define MIPS_OUTCHL_BUF_TC2_ERR_CNT      0x000c
#define MIPS_OUTCHL_BUF_TC3_ERR_CNT      0x000d
#define MIPS_OUTCHL_BUF_ERR_PACKET_CNT   0x000e
#define MIPS_OUTCHL_BUF_OUT_PACKET_CNT   0x000f
#define MIPS_OUTCHL_BUF_LOW_ADRR         0x0010
#define MIPS_OUTCHL_BUF_UP_ADRR          0x0011
#define MIPS_PCR_PID_ADDR                0x0012
#define MIPS_PCR_GET_ADDR                0x0013
#define MIPS_OUTCHL_CRC_EN               0x0014
#define MIPS_OUTCHL_CRC_NOTIFY_EN        0x0015
#define MIPS_OUTCHL_PID_BASE             0x0016

/*n~(0,16)*/
#define MIPS_OUTCHL_PID(n)               (MIPS_OUTCHL_PID_BASE+(n))
#define MIPS_OUTCHL_DISABLE              (MIPS_OUTCHL_PID_BASE + 0x20)
#define MIPS_OUTCHL_SWITCH               (MIPS_OUTCHL_PID_BASE + 0x21)
#define  MIPS_EXTERNAL_BASE              (MIPS_OUTCHL_PID_BASE + 0x22)
#define  MIPS_OUTCHL_BITWISE0            (MIPS_OUTCHL_PID_BASE + 0x23)  
#define  MIPS_OUTCHL_BITWISE1            (MIPS_OUTCHL_PID_BASE + 0x24)  
#define  MIPS_OUTCHL_BITWISE2            (MIPS_OUTCHL_PID_BASE + 0x25)  
#define  MIPS_OUTCHL_FILTER_MAX_SIZE     (MIPS_OUTCHL_PID_BASE + 0x26) 

#define MIPS_INCHL_TYPE                  (MIPS_OUTCHL_PID_BASE + 0x27)
#endif

