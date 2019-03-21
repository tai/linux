#ifndef _HDMI_SW_HW_IF_H_
#define _HDMI_SW_HW_IF_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef enum _CLOCK_RESET_
{
  _do_reset =0,
  _do_set   =1,
}CLOCK_RESET;

int HDMI_REG_Init(void);
void HW_REG_WRITE(unsigned int ByteAddr, unsigned int Data);
unsigned int  HW_REG_READ (unsigned int ByteAddr);
int  HW_REG_READ_CHECK (unsigned int ByteAddr, unsigned int Data);

void clock_hdmi_reset(CLOCK_RESET ResetOrSet);
// 1 for enable, 0 for disable
void clock_tms_clockena(unsigned char ena);
#ifdef __cplusplus
}
#endif

#endif
