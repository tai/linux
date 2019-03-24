/*******************************************************************************

File name   : orion_blit.h

Description : orion blitter driver head file

COPYRIGHT (C) Celestial Semiconductor 2007.

Date               Modification                                     Name
----               ------------                                     ----
14 Nov 2007        Created                                           XM.Chen
*******************************************************************************/


#ifndef	_ORION_BLIT_H_
#define _ORION_BLIT_H_

#include "blit_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLIT_MAGIC 'B'
#define CMD_FILL 	_IOW(BLIT_MAGIC, 1, int)
#define CMD_COPY 	_IOW(BLIT_MAGIC, 2, int)
#define CMD_SCALOR 	_IOW(BLIT_MAGIC, 3, int)
#define CMD_COMP 	_IOW(BLIT_MAGIC, 4, int)
#define BLIT_MAXNR 4

typedef struct CSBlit_Device 
{
	CSBlit_HW_Device_t	hwdev;
	struct list_head		buffer_head;
} blit_dev;

#ifdef __cplusplus
}
#endif

#endif //_ORION_BLIT_H_

