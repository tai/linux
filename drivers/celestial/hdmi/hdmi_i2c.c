/*////////////////////////////////////////////////////////////////////////
// Copyright (C) 2008 Celestial Semiconductor Inc.
// All rights reserved
// ---------------------------------------------------------------------------
// Project NAME     : HDMI I2C
// AUTHOR           : Liguo Du
// AUTHOR'S EMAIL   : liguo.du@celestialsemicom
// ---------------------------------------------------------------------------
// [RELEASE HISTORY]                           
// VERSION  DATE       AUTHOR                  DESCRIPTION
// 1.0      08-11-25  Liguo Du                 Original
// ---------------------------------------------------------------------------
// [Description]
// */

#include <linux/types.h>

#include "hdmi_sw_hw_if.h"
#include "hdmi_i2c.h"
#define  HDMI_REG_BASE  (0x80400000)
#define  HDMI_CLK_BASE  (0xb2100000)

extern void volatile __iomem *hdmi_reg_addr;

unsigned char HDMI_I2C_SeqRead(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum)
{
   unsigned int BaseAddr =0;
   unsigned int RegAddr  =0;
   unsigned char  i        =0;

  if(SlvAddr==0x39)
   {
       BaseAddr =HDMI_REG_BASE>>2;
   }else if(SlvAddr==0xff)
   {
   	   BaseAddr  =(HDMI_REG_BASE>>2)+0x400;
   }
   else
   {
       BaseAddr =(HDMI_REG_BASE>>2)+0x100;
   }

  for(i=0;i<ByteNum;++i)
  {
     RegAddr  =(BaseAddr+DevOffset+i)<<2;
//	 printk("ByteNum == %x ,HW_REG_READ(RegAddr) == %x \n",ByteNum,HW_REG_READ(RegAddr));
     Data[i]  =HW_REG_READ(RegAddr)&0xFF;
  }

  return ByteNum; 		
}

unsigned char HDMI_I2C_SeqWrite(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum)
{
   unsigned int BaseAddr =0;
   unsigned int RegAddr  =0;
   unsigned char  i        =0;
   //assert(Data!=NULL);
   if(SlvAddr==0x39)
   {
       BaseAddr =HDMI_REG_BASE>>2;
   }else if(SlvAddr==0xff)
   {
   	   BaseAddr  =(HDMI_REG_BASE>>2)+0x400;
   }
   else
   {
       BaseAddr =(HDMI_REG_BASE>>2)+0x100;
   }

  for(i=0;i<ByteNum;++i)
  {
     RegAddr  =(BaseAddr+DevOffset+i)<<2;
     HW_REG_WRITE(RegAddr,Data[i]);
  }
  return ByteNum;
}

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

//unsigned int hdmi_reg_addr = -1;
//unsigned int hdmi_clock_addr = -1;

int HDMI_REG_Init()
{
#if 0
 	unsigned int phyaddr = 0;
	unsigned int memsize = 0;
	unsigned int addroffset = 0;
	unsigned char *mapped_addr = NULL;
	int fd = -1;
	unsigned int base_addr = HDMI_REG_BASE;
	unsigned int map_size = 0x10000;
 
	fd = open("/dev/mem",O_RDWR | O_SYNC);
	if (fd < 0) {
		return -1;
	}

	phyaddr = (base_addr >> PAGE_SHIFT) << PAGE_SHIFT;
	addroffset = base_addr - phyaddr;
	memsize = ((addroffset + map_size + PAGE_SIZE - 1) >> PAGE_SHIFT) << PAGE_SHIFT;
	mapped_addr = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyaddr);
	if (mapped_addr == MAP_FAILED) {
		printk("error : HDMI Register mmap\n");
		return -1;
	}

	hdmi_reg_addr = (unsigned int)(mapped_addr + addroffset);

	phyaddr = base_addr = addroffset = map_size = memsize = 0;

	mapped_addr = NULL;
	base_addr = HDMI_CLK_BASE;
	map_size = 0x1000;
	phyaddr = (base_addr >> PAGE_SHIFT) << PAGE_SHIFT;
	addroffset = base_addr - phyaddr;
	memsize = ((addroffset + map_size + PAGE_SIZE - 1) >> PAGE_SHIFT) << PAGE_SHIFT;
	mapped_addr = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyaddr);
	if (mapped_addr == MAP_FAILED) {
		printk("error : HDMI Clock Register mmap\n");
		return -1;
	}

	hdmi_clock_addr = (unsigned int)(mapped_addr + addroffset + 0x200);
#endif
	return 0;
}

void HW_REG_WRITE(unsigned int ByteAddr, unsigned int Data)
{
	unsigned int offset = ByteAddr - HDMI_REG_BASE;
//	printk("hdmi_reg_addr == %x ,Data == %x \n",hdmi_reg_addr,Data);
	*((unsigned int *)hdmi_reg_addr + (offset>>2)) = Data;
//	printk("hdmi_reg_addr == %x ,Data == %x \n",hdmi_reg_addr,Data);
}

unsigned int HW_REG_READ(unsigned int ByteAddr)
{
	unsigned int read_val;
	unsigned int offset = ByteAddr - HDMI_REG_BASE;
//	printk("hdmi_reg_addr == %x ,offset == %x \n",hdmi_reg_addr,offset);
	read_val = *((unsigned int *)hdmi_reg_addr + (offset>>2));
//	printk("hdmi_reg_addr == %x ,read_val == %x \n",hdmi_reg_addr,read_val);
	return read_val;
}

#if 0
void clock_hdmi_reset(CLOCK_RESET ResetOrSet)
{
	unsigned int reg_val;
	int iHDMIRst;
	
	if(ResetOrSet==_do_reset)
	{
		iHDMIRst =0;
		reg_val = *(unsigned int *)hdmi_clock_addr;
		*(unsigned int *)hdmi_clock_addr = reg_val&(~(0x1<<19));
	}
	else
	{
		iHDMIRst=1;
		reg_val = *(unsigned int *)hdmi_clock_addr;
		*(unsigned int *)hdmi_clock_addr = reg_val|(0x1<<19);
	}

	return;

}

// 1 for enable, 0 for disable
void clock_tms_clockena(unsigned char ena)
{
	unsigned int reg_val;
    if(ena == 1)
	{
        reg_val = *((unsigned int *)(hdmi_clock_addr - 0x200 + 0x140));
		*((unsigned int *)(hdmi_clock_addr - 0x200 + 0x140)) = reg_val | (1<<6);
	}
	else
	{
        reg_val = *((unsigned int *)(hdmi_clock_addr - 0x200 + 0x140));
		*((unsigned int *)(hdmi_clock_addr - 0x200 + 0x140)) = reg_val & ~(1<<6);
	}
    return;
}
#endif
