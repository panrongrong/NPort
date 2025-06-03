/******************************************************************************
*
* Copyright (C) 2018 - 2028 FMSH, Inc.  All rights reserved.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file  fmsh_common_io.h
*
* This file contains
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 0.01   yl  12/20/2018  First Release
*</pre>
******************************************************************************/
#ifndef _FMSH_COMMON_IO_H_
#define _FMSH_COMMON_IO_H_

#ifdef __cplusplus
extern "C" {    /* allow C++ to use these headers */
#endif

/***************************** Include Files *********************************/

#include "stdio.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

#define FMSH_ReadReg(baseAddr, offSet)		*((volatile unsigned int *)(baseAddr + offSet))
#define FMSH_WriteReg(baseAddr, offSet, data)	*((volatile unsigned int *)(baseAddr + offSet)) = data

/* the following macro performs an 8-bit read */
#define FMSH_IN8_8(p)     ((uint8_t) *((volatile uint8_t *)(&p)))
/* the following macro performs an 8-bit write */
#define FMSH_OUT8_8(v,p)  *((volatile uint8_t *) (&p)) = (uint8_t) (v)
/* the following macro performs a 16-bit read */
#define FMSH_IN16_16(p)      ((uint16_t) *((volatile uint16_t *)(&p)))
/* the following macro performs a 16-bit write */
#define FMSH_OUT16_16(v,p)  *((volatile uint16_t *) (&p)) = (uint16_t)(v)

/* the following macro performs a 32-bit write */
#define FMSH_OUT32_32(v,p)  *((volatile UINT32 *)(&p)) = (v)

/* the following macro performs a 32-bit reads */
#define FMSH_IN32_32(p)   (UINT32)(*((UINT32 *)(&p)))

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _FMSH_COMMON_IO_H_ */

