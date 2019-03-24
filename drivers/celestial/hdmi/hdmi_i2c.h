#ifndef _HDMI_I2C_H_
#define _HDMI_I2C_H_

#ifdef __cplusplus
extern "C"{
#endif

unsigned char        HDMI_I2C_Init(void);                                              //DDC Init 
unsigned char        HDMI_I2C_SeqRead(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum);   //DDC Sequential Read  
unsigned char        HDMI_I2C_SeqWrite(unsigned char SlvAddr,unsigned char DevOffset, unsigned char *Data,unsigned char ByteNum);  //DDC Sequential Write
#ifdef __cplusplus
}
#endif

#endif
