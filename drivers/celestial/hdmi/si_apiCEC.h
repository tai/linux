/*
 *****************************************************************************
 *
 * Copyright 2010, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *****************************************************************************
 */
/*
 *****************************************************************************
 * @file  si_apiCEC.h
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
*/

//***************************************************************************
//!file     si_apicec.h
//!brief    Silicon Image CEC API
//
// No part of this work may be reproduced, modified, distributed,
// transmitted, transcribed, or translated into any language or computer
// format, in any form or by any means without written permission of
// Silicon Image, Inc., 1060 East Arques Avenue, Sunnyvale, California 94085
//
// Copyright 2009, Silicon Image, Inc.  All rights reserved.
//***************************************************************************/

#ifndef __SI_APICEC_H__
#define __SI_APICEC_H__
#include "si_apiCpi.h"
#include "si_cec_enums.h"

//-------------------------------------------------------------------------------
// CPI Enums, typedefs, and manifest constants
//-------------------------------------------------------------------------------

#define CSTVOUT_HDMI_CEC_MAX_ARGS   16

#define MAKE_SRCDEST( src, dest)    (( src << 4) | dest )

#define SII_NUMBER_OF_PORTS         5
#define SII_EHDMI_PORT              (1)

enum
{
    SI_CECTASK_IDLE,
    SI_CECTASK_ENUMERATE,
    SI_CECTASK_NEWSOURCE,
    SI_CECTASK_ONETOUCH,
    SI_CECTASK_SENDMSG
};

typedef struct
{
    uint8_t deviceType;     // 0 - Device is a TV.
                            // 1 - Device is a Recording device
                            // 2 - Device is a reserved device
                            // 3 - Device is a Tuner
                            // 4 - Device is a Playback device
                            // 5 - Device is an Audio System
			    // 6 - Pure CEC Switch
			    // 7 - Video Processer
    uint8_t  cecLA;         // CEC Logical address of the device.
    uint16_t cecPA;         // CEC Physical address of the device.
} CEC_DEVICE;

typedef enum {
	HDMI_CEC_TX_ACKED,	//<Message was ACKed.
	HDMI_CEC_TX_FAILED	//<Failed to send message.
}CSTVOUT_HDMI_CEC_TX_ACK;

typedef struct {
	CEC_LOG_ADDR_t src_la;///<Logical address of the source
	CEC_LOG_ADDR_t dst_la;
	unsigned char is_broadcast;///<is the message a broadcast message or a direct addressed message
	CEC_OPCODE_t opcode;///<is the message a broadcast message or a direct addressed message
	unsigned char arg_count;///<number of arguments
	unsigned char args[CSTVOUT_HDMI_CEC_MAX_ARGS];///<Arguments of sending message
}CSTVOUT_HDMI_CEC_RECEIVE_MESSAGE;

typedef struct {
    CEC_LOG_ADDR_t dst_la;///<Structure of HDMI CEC send message.
    CEC_OPCODE_t opcode;///<message number defined by HDMI specification
    unsigned char arg_count;///<number of arguments
    unsigned char args[CSTVOUT_HDMI_CEC_MAX_ARGS];///<Arguments of sending message
}CSTVOUT_HDMI_CEC_SEND_MESSAGE;

extern uint8_t  g_cecAddress;       // Initiator
extern uint16_t g_cecPhysical;      // For TV, the physical address is 0.0.0.0

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------
extern CEC_DEVICE   g_childPortList [SII_NUMBER_OF_PORTS];

//------------------------------------------------------------------------------
// API Function Templates
//------------------------------------------------------------------------------
void		si_CecSendMessage( uint8_t opCode, uint8_t dest );
void		SI_CecSendUserControlPressed( uint8_t keyCode );
void		SI_CecSendUserControlReleased( void );
unsigned char	SI_CecSwitchSources( uint8_t portIndex );
unsigned char	SI_CecEnumerate( void );
uint8_t		SI_CecGetPowerState( void );
void		SI_CecSetPowerState( uint8_t newPowerState );
void		SI_CecSourceRemoved( uint8_t portIndex );
uint16_t	SI_CecGetDevicePA( void );
void		SI_CecSetDevicePA( uint16_t devPa );
unsigned char		SI_CecInit( void );
uint8_t		SI_CecPortToLA(uint8_t portIndex);
uint8_t		SI_CecLaToPort(uint8_t logicalAddr);
void DisableCECSupport(void);
void EnableCECSupport(void);
int CECIsSupported(void);
void clear_capture_id(void);
uint8_t SI_CecGetManualSetCaptureIDFlag(void);
void SI_CecSetCaptureIDToHW(void );
uint8_t SI_CecGetManualSetLAFlag(void);
void SI_CecSetSrcLAToHW(void);
int si_CecCheckTaskStatus(void);
unsigned char SI_CecOneTouchPlay (void);
void SI_CecSetSrcLA(uint8_t src_la);
void SI_CecSetCaptureID(uint8_t captureID[2]);
CSTVOUT_HDMI_CEC_TX_ACK si_CecGetACKStatus(void);
void si_CecClearTxStatus(void);
void si_CecSendMessage_api ( uint8_t opCode, uint8_t dest, uint8_t argCount, uint8_t *args);

#endif // __SI_APICEC_H__

