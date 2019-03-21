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
 * @file  si_cpi_regs.h
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
 */

//***************************************************************************
//!file     si_cpi_regs.h
//!brief    SiI9387 Device CPI Registers Manifest Constants.
//
// No part of this work may be reproduced, modified, distributed,
// transmitted, transcribed, or translated into any language or computer
// format, in any form or by any means without written permission of
// Silicon Image, Inc., 1060 East Arques Avenue, Sunnyvale, California 94085
//
// Copyright 2007-2009, Silicon Image, Inc.  All rights reserved.
//***************************************************************************/
#ifndef __SI_CPI_REGS_H__
#define __SI_CPI_REGS_H__

//------------------------------------------------------------------------------
// NOTE: Register addresses are 16 bit values with page and offset combined.
//
// Examples:  0x005 = page 0, offset 0x05
//            0x1B6 = page 1, offset 0xB6
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Registers in Page 8
//------------------------------------------------------------------------------

#define REG_CEC_DEBUG_3                     (0x21c>>2)
#define BIT_SNOOP_EN                        0x01
#define BIT_FLUSH_TX_FIFO                   0x80

#define REG_CEC_TX_INIT                     (0x220>>2)
#define BIT_SEND_POLL                       0x80

#define REG_CEC_TX_DEST                     (0x224>>2)

#define REG_CEC_CONFIG_CPI                  0x88E

#define REG_CEC_TX_COMMAND                  (0x23C>>2)
#define REG_CEC_TX_OPERAND(x)               ((0x240+(x*4))>>2)

#define REG_CEC_TRANSMIT_DATA               (0x27C>>2)
#define BIT_TX_BFR_ACCESS                   0x40
#define BIT_TX_AUTO_CALC                    0x20
#define BIT_TRANSMIT_CMD                    0x10

#define REG_CEC_CAPTURE_ID0                 (0x288>>2)
#define REG_CEC_CAPTURE_ID1                 (0x28c>>2)

#define REG_CEC_INT_ENABLE_0                (0x290>>2)
#define BIT_TX_FIFO_FULL                    0x20

#define REG_CEC_INT_ENABLE_1                (0x294>>2)
#define BIT_RX_FIFO_OVERRUN                 0x08

// 0xA6 CPI Interrupt Status Register (R/W)
#define REG_CEC_INT_STATUS_0                (0x298>>2)
#define BIT_CEC_LINE_STATE                  0x80
#define BIT_TX_MESSAGE_SENT                 0x20
#define BIT_TX_HOTPLUG                      0x10
#define BIT_POWER_STAT_CHANGE               0x08
#define BIT_TX_FIFO_EMPTY                   0x04
#define BIT_RX_MSG_RECEIVED                 0x02
#define BIT_CMD_RECEIVED                    0x01

// 0xA7 CPI Interrupt Status Register (R/W)
#define REG_CEC_INT_STATUS_1                (0x29C>>2)
#define BIT_RX_FIFO_OVERRUN                 0x08
#define BIT_SHORT_PULSE_DET                 0x04
#define BIT_FRAME_RETRANSM_OV               0x02
#define BIT_START_IRREGULAR                 0x01

#define REG_CEC_RX_CONTROL                  (0x2B0>>2)
// CEC  CEC_RX_CONTROL bits
#define BIT_CLR_RX_FIFO_CUR                 0x01
#define BIT_CLR_RX_FIFO_ALL                 0x02

#define REG_CEC_RX_COUNT                    (0x2B4>>2)
#define BIT_MSG_ERROR                       0x80


#define REG_CEC_RX_CMD_HEADER               (0x2B8>>2)
#define REG_CEC_RX_OPCODE                   (0x2bc>>2) //where ?????
#define REG_CEC_RX_OPERAND_0                (0x2C0>>2)

#define CEC_OP_ABORT_0                      (0x300>>2)
#define CEC_OP_ABORT_31                     (0x37c>>2)


#endif  // __SI_CPI_REGS_H__
