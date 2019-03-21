//------------------------------------------------------------------------------
// Module Name: SiITXHDCP.h
// Module Description: SiITX HDCP header file
//
// Copyright ?2005-2008, SII, Inc.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef _SIITXHDCP_H_
#define _SIITXHDCP_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "TypeDefs.h"
#include "SiIIIC.h"

#define HDCPRX_SLV 0x74

//--------------------------------------------------------------
// HDCP DDC side (HDCP receiver or repearer)
#define DDC_BKSV_ADDR                   0x00  // 5 bytes, Read only (RO), receiver KSV
#define DDC_Ri_ADDR                     0x08    // Ack from receiver RO
#define DDC_AKSV_ADDR                   0x10    // 5 bytes WO, transmitter KSV
#define DDC_AN_ADDR                     0x18    // 8 bytes WO, random value.
#define DDC_V_ADDR                      0x20    // 20 bytes RO
#define DDC_RSVD_ADDR                   0x34    // 12 byte RO
#define DDC_BCAPS_ADDR                  0x40    // Capabilities Status byte RO

#define DDC_BIT_HDMI_CAP                0x80
#define DDC_BIT_REPEATER                0x40
#define DDC_BIT_FIFO_READY              0x20
#define DDC_BIT_FAST_I2C                0x10

#define DDC_BSTATUS1_ADDR               0x41 // B STATUS 1
#define DDC_BIT_BSTATUS1_DEV_EXC        0x80 // device count exceeded
#define DDC_BIT_BSTATUS1_DEV_COUNT      0x7F // device count

#define DDC_BSTATUS2_ADDR               0x42 // B STATUS 2
#define DDC_BIT_BSTATUS2_HDMI_MODE      0x10 // 1-device is in HDMI mode, 0-device is in DVI mode
#define DDC_BIT_BSTATUS2_CAS_EXC        0x08 // cascade depth exceeded
#define DDC_BIT_BSTATUS2_DEV_DEPTH      0x07 // device depth

#define DDC_KSV_FIFO_ADDR               0x43

#define RX_L_MAX_DEVICE_COUNT           16
#define RX_L_MAX_DEVICE_DEPTH           7
#define HDCP_DDC_L_VH                   20

#define SHA_UpstreamUse                 0
#define SHA_DownstreamUse               1

//-------------------------------------------------------------
#define NO_HDCP                         0x00
#define NO_DEVICE_WITH_HDCP_SLAVE_ADDR  0x01
#define BKSV_ERROR                      0x02
#define R0s_ARE_MISSMATCH               0x03
#define RIs_ARE_MISSMATCH               0x04
#define REAUTHENTATION_REQ              0x05
#define REQ_AUTHENTICATION              0x06
#define NO_ACK_FROM_HDCP_DEV            0x07
#define NO_RSEN                         0x08
#define AUTHENTICATED                   0x09
#define REPEATER_AUTH_REQ               0x0A
#define REQ_SHA_CALC                    0x0B
#define REQ_SHA_HW_CALC                 0x0C    // 9032 added
#define FAILED_ViERROR                  0x0D    // 9032 moved

#define HPD_NOT_CHANGED                 0x0E    // added by xp, 2011-12-29, this is not a state of HDCP State Machine,
                                                // but it must not equal any value in HDCP State Machine.

#define MASTER_BASE                     0xEC
#define MDDC_MANUAL_ADDR                0xEC                               // Register Offsets
#define MDDC_SLAVE_ADDR                 0xED
#define MDDC_SEGMENT_ADDR               0xEE
#define MDDC_OFFSET_ADDR                0xEF
#define MDDC_DIN_CNT_LSB_ADDR           0xF0
#define MDDC_DIN_CNT_MSB_ADDR           0xF1
#define MDDC_STATUS_ADDR                0xF2
#define MDDC_COMMAND_ADDR               0xF3

#define MDDC_FIFO_ADDR                  0xF4
#define MDDC_FIFO_CNT_ADDR              0xF5

#define BIT_MDDC_ST_IN_PROGR            0x10
#define BIT_MDDC_ST_I2C_LOW             0x40
#define BIT_MDDC_ST_NO_ACK              0x20

#define MASTER_CMD_ABORT                0x0f                    // Command Codes
#define MASTER_CMD_CLEAR_FIFO           0x09
#define MASTER_CMD_CLOCK                0x0a
#define MASTER_CMD_CUR_RD               0x00
#define MASTER_CMD_SEQ_RD               0x02
#define MASTER_CMD_ENH_RD               0x04
#define MASTER_CMD_SEQ_WR               0x06

#define MASTER_FIFO_WR_USE              0x01
#define MASTER_FIFO_RD_USE              0x02
#define MASTER_FIFO_EMPTY               0x04
#define MASTER_FIFO_FULL                0x08
#define MASTER_DDC_BUSY                 0x10
#define MASTER_DDC_NOACK                0x20
#define MASTER_DDC_STUCK                0x40
#define MASTER_DDC_RSVD                 0x80

#define _MDDC_CAPTURED                  3
#define _MDDC_NOACK                     4
#define _MDDC_FIFO_FULL                 5
#define IS_EDID_ERROR                   1
#define NO_EDID_ERROR                   0

#define MDDCCommand( CMD )              hlWriteByte_8BA( TX_SLV0, MDDC_COMMAND_ADDR, CMD )
#define IsMDDCReadInProgress()          hlReadByte_8BA( TX_SLV0, MDDC_STATUS_ADDR ) & BIT_MDDC_ST_IN_PROGR
#define GetMDDCStatus()                 hlReadByte_8BA( TX_SLV0, MDDC_STATUS_ADDR )
#define ClrManualCtrlReg()              hlWriteByte_8BA( TX_SLV0,MDDC_MANUAL_ADDR, 0)
#define MDDCWriteOffset(SGM_OFFSET)     hlWriteByte_8BA( TX_SLV0, MDDC_SEGMENT_ADDR, SGM_OFFSET)
#define MDDCReadOffset()                hlReadByte_8BA( TX_SLV0, MDDC_SEGMENT_ADDR)

typedef struct {
	BYTE SlaveAddr;
	BYTE Offset;
	BYTE RegAddr;
	BYTE NBytesLSB;
	BYTE NBytesMSB;
	BYTE Dummy;
	BYTE Cmd;
	BYTE * PData;
	BYTE Data[6];
} MDDCType;

typedef union TmpData {
	BYTE FuncRes[16];
	MDDCType MDDC;
} TmpDType;

#ifdef REPEATER_SUPPORT
#define HDCP1X_KSV_SIZE 5
#define MAX_HDCP1X_KSV 31
 
typedef struct {
    unsigned char bksv[HDCP1X_KSV_SIZE];
    unsigned char rptr_dev_cnt;
    unsigned char rptr_dev_exd;
    unsigned char rptr_casc_exd;
    unsigned char rptr_depth;
    unsigned char ksv_list[HDCP1X_KSV_SIZE * MAX_HDCP1X_KSV];
} downstream_info_ksv_list_t;
#endif

/*********************************  SHA  ***************************************/
typedef struct{
	DWORD Digest[5];
	DWORD Data[16];
	WORD byteCounter;
	BYTE BStatusMXCounter;
} SHA_CTX;

#define KSV_PULLED                      0
#define KSV_FINISHED                    1

// Buffer status
#define PADED                           1
#define END_PLACED                      2

#define BSTATMX_PULLED                  0
#define BSTATMX_FINISHED                1
#define BSTATMX_SIZE                    10

#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )              // Rounds  0-19
#define f2(x,y,z)   ( x ^ y ^ z )                          // Rounds 20-39
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )      // Rounds 40-59
#define f4(x,y,z)   ( x ^ y ^ z )                          // Rounds 60-79

// The SHS Constants
#define K1  0x5A827999L                                    // Rounds  0-19
#define K2  0x6ED9EBA1L                                    // Rounds 20-39
#define K3  0x8F1BBCDCL                                    // Rounds 40-59
#define K4  0xCA62C1D6L                                    // Rounds 60-79

// SHS initial values
#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L
/*******************************************************************************/

void siiTXSetEncryption( BYTE OnOff );
void BlockRead_MDDC( MDDCType * MDDCCmd, BYTE * pData );
void BlockWrite_MDDC( MDDCType * MDDCCmd );
void MDDCBlockReadHDCPRX( BYTE NBytes, BYTE Addr, BYTE * pData );
void MDDCBlockWriteHDCPRX( BYTE NBytes, BYTE Addr, BYTE * pData );
BYTE CheckValidityHDCPDevice( BYTE * Data );
void ReleaseCPReset( void );
void AreR0sMatch( BYTE * Match );
void RiCheckInterruptMask( BYTE Enabled );
void printAuthState( void );
void siiTXHDCP_AuthenticationPart1( void );
BYTE SHAHandler( void );

#ifdef REPEATER_SUPPORT
int setHDCP1xAuthState (char state);
int getHDCP1xDownstreamInfo(unsigned char *receiver_id_list);
int si_hdmi_tx_hdcp_get_downstream_info(unsigned char *receiver_id_list);
#endif

extern BYTE DownStreamAuthState;
extern TmpDType TmpD;

#ifdef __cplusplus
}
#endif

#endif
