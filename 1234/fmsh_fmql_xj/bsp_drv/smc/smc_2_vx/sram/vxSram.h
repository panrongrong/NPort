/* vxSram.h - vxSram driver header */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a, 09Jun20, jc  written.
*/

#ifndef __INC_VX_SRAM_H__
#define __INC_VX_SRAM_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


extern UINT8 SRAM_Read(UINT32 baseAddr, UINT32 offSet);
extern void SRAM_Write(UINT32 baseAddr, UINT32 offSet, UINT8 data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_SRAM_H__ */

