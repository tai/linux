//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "TypeDefs.h"

#ifndef _SIIIIC_
#define _SIIIIC_

#ifdef __cplusplus
extern "C" {
#endif

#define TX_SLV0	0x39
#define TX_SLV1	0x3D

#define SET 1
#define CLR 0

#define IIC_CAPTURED  1
#define IIC_NOACK     2
#define IIC_OK 0

#define FLG_SHORT 0x01 // Used for Ri Short Read
#define FLG_NOSTOP 0x02 // Don't release IIC Bus
#define FLG_CONTD 0x04 // Continued from previous operation

typedef struct {

    BYTE SlaveAddr;
    BYTE Flags;
    BYTE NBytes;
    BYTE RegAddrL;
    BYTE RegAddrH;

} I2CShortCommandType;

BYTE hlReadByte_8BA ( BYTE, BYTE);
WORD hlReadWord_8BA ( BYTE , BYTE);
void hlWriteWord_8BA ( BYTE , BYTE, WORD );
BYTE hlWriteByte_8BA ( BYTE , BYTE, BYTE );

void siiReadBlockHDMI( BYTE bSlaveAddr, BYTE bAddr, BYTE bNBytes, BYTE * pbData );
void siiWriteBlockHDMI( BYTE bSlaveAddr, BYTE bRegAddr, BYTE bNBytes, BYTE * pbData );
void siiModifyBits( BYTE bSlaveAddr, BYTE bRegAddr, BYTE bModVal, BOOL qSet );

//add ......................................................................................................................
int hdmi_i2c_open(void);
int hdmi_i2c_read(unsigned char address, int subaddr, char *buffer, unsigned int num);
int hdmi_i2c_write(unsigned char address, unsigned int subaddr, unsigned char *buffer, unsigned int num);
int hdmi_write_mask(int addr, int subaddr, int mask, int data);
int hdmi_i2c_read_byte(unsigned char address, int subaddr, char *buffer);
int hdmi_i2c_write_byte(unsigned char address, unsigned int subaddr, unsigned char byte);
//end add.............................................................................................................................................................
#ifdef __cplusplus
}
#endif

#endif


