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
 * @file  si_apiCpi.h
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
*/

//***************************************************************************
//!file     si_apiCpi.h
//!brief    SiI9387 CPI functions.
//
// No part of this work may be reproduced, modified, distributed,
// transmitted, transcribed, or translated into any language or computer
// format, in any form or by any means without written permission of
// Silicon Image, Inc., 1060 East Arques Avenue, Sunnyvale, California 94085
//
// Copyright 2002-2009, Silicon Image, Inc.  All rights reserved.
//***************************************************************************/

#ifndef __SI_APICPI_H__
#define __SI_APICPI_H__

#include "si_datatypes.h"

//-------------------------------------------------------------------------------
// CPI Enums and manifest constants
//-------------------------------------------------------------------------------

#define SII_MAX_CMD_SIZE 16

typedef enum
{
    SI_TX_WAITCMD,
    SI_TX_SENDING,
    SI_TX_SENDACKED,
    SI_TX_SENDFAILED
} SI_txState_t;

typedef enum
{
    SI_CEC_SHORTPULSE       = 0x80,
    SI_CEC_BADSTART         = 0x40,
    SI_CEC_RXOVERFLOW       = 0x20,
    SI_CEC_ERRORS           = (SI_CEC_SHORTPULSE | SI_CEC_BADSTART | SI_CEC_RXOVERFLOW)
} SI_cecError_t;

//-------------------------------------------------------------------------------
// CPI data structures
//-------------------------------------------------------------------------------

typedef struct
{
    uint8_t srcDestAddr;            // Source in upper nybble, dest in lower nybble
    uint8_t opcode;
    uint8_t args[SII_MAX_CMD_SIZE];
    uint8_t argCount;
    uint8_t nextFrameArgCount;
    uint8_t chn;
} SI_CpiData_t;

typedef struct
{
    uint8_t rxState;
    uint8_t txState;
    uint8_t cecError;
} SI_CpiStatus_t;


//-------------------------------------------------------------------------------
// CPI Function Prototypes
//-------------------------------------------------------------------------------

unsigned char SI_CpiRead( SI_CpiData_t *pCpi );
unsigned char SI_CpiWrite( SI_CpiData_t *pCpi );
unsigned char SI_CpiStatus( SI_CpiStatus_t *pCpiStat );

unsigned char SI_CpiInit( void );
unsigned char SI_CpiSetLogicalAddr( uint8_t logicalAddress );
unsigned char SI_CpiSetCaptureID ( uint8_t captureID[2] );
uint8_t	SI_CpiGetLogicalAddr( void );
void SI_CpiSendPing( uint8_t bCECLogAddr );

#endif // _SI_APICPI_H_



