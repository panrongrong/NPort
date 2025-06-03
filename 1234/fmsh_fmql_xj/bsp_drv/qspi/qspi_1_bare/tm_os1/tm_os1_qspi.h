/*
tm_os1_qspi.h -  the tianmai os1 funcitons header for qspiflash
*/

/*
 * Copyright (c) xxx, Corp..
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable QingSong license agreement.
 */

/*
DESCRIPTION:

This file contains the functions define: init, readid, erase, read, write etc.
*/

/*
modification history:
--------------------
01a,24Jun21, jc  create
*/


#ifndef  __TM_OS1_QSPIFLASH__
#define __TM_OS1_QSPIFLASH__

extern void qspiFlash_readId(void);
extern STATUS qspi_init(void);
extern STATUS qspiFlashWrite(UINT32 addr, char *buf, UINT32 byteLen);
extern STATUS qspiFlashRead(UINT32 offset, char * readBuffer, UINT32 byteLen);
extern int FQspiFlash_SectorErase(UINT32 start_offset);

#endif /* __TM_OS1_QSPIFLASH__ */


