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
 * @file  si_cpCEC.c
 *
 * @brief Implementation of the Foo API.
 *
 *****************************************************************************
*/

//***************************************************************************
//!file     si_cpCEC.c
//!brief    CP 9387 Starter Kit CEC handler.
//
// No part of this work may be reproduced, modified, distributed,
// transmitted, transcribed, or translated into any language or computer
// format, in any form or by any means without written permission of
// Silicon Image, Inc., 1060 East Arques Avenue, Sunnyvale, California 94085
//
// Copyright 2009, Silicon Image, Inc.  All rights reserved.
//***************************************************************************/

#include "si_apiCpi.h"
#include "si_apiCEC.h"
#include "si_cec_enums.h"
//------------------------------------------------------------------------------
// Function:    CecViewOn
// Description: Take the HDMI switch and/or sink device out of standby and
//              enable it to display video.
//------------------------------------------------------------------------------

static void CecViewOn ( SI_CpiData_t *pCpi )
{
    pCpi = pCpi;

    SI_CecSetPowerState( CEC_POWERSTATUS_ON );
}

//------------------------------------------------------------------------------
// Function:    CpCecRxMsgHandler
// Description: Parse received messages and execute response as necessary
//              Only handle the messages needed at the top level to interact
//              with the Port Switch hardware.  The SI_API message handler
//              handles all messages not handled here.
//
// Warning:     This function is referenced by the Silicon Image CEC API library
//              and must be present for it to link properly.  If not used,
//              it should return 0 (false);
//
//              Returns true if message was processed by this handler
//------------------------------------------------------------------------------

unsigned char CpCecRxMsgHandler (SI_CpiData_t *pCpi)
{
    unsigned char            processedMsg, isDirectAddressed;

    isDirectAddressed = !((pCpi->srcDestAddr & 0x0F) == CEC_LOGADDR_UNREGORBC);
    processedMsg = true;
    if (isDirectAddressed)
    {
		// Respond to messages addressed to us
        switch (pCpi->opcode)
        {
            case CECOP_IMAGE_VIEW_ON:       // In our case, respond the same to both these messages
            case CECOP_TEXT_VIEW_ON:
                CecViewOn(pCpi);
                break;

            case CECOP_INACTIVE_SOURCE:
                break;

            default:
                processedMsg = false;
                break;
        }
    }
    else
    {
    	// Respond to broadcast messages.
        switch ( pCpi->opcode )
        {
            case CECOP_ACTIVE_SOURCE:
                break;

            default:
                processedMsg = false;
            break;
        }
    }

    return(processedMsg);
}

