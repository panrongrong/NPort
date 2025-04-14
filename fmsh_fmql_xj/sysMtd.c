/* sysMtd.c - MTD driver for numonyx N25Q00 flash */

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
01a,05may13,swu  created from xlnx_zynq7k/sysMtd.c rev 01a
*/

/*
DESCRIPTION
This file provides the TFFS MTD driver for the numonyx N25Q00 series flash
parts. The driver handles the low level operations like erase and program of
the flash. It also provides an identify routine to check if it is compatible
with the devices.

INCLUDE FILES:

SEE ALSO:
*/

/* includes */

#include <stdio.h>
#include <tffs/flflash.h>
#include <tffs/backgrnd.h>

/*#include "sysQspiFlashMem.h"*/
#include "config.h"

/* defines */

/* disable debugging */
#undef CFI_DEBUG

#ifdef CFI_DEBUG
#define DEBUG_PRINT(string, a, b, c, d, e, f) \
        printf(string, a, b, c, d, e, f)
#else
#define DEBUG_PRINT(string, a, b, c, d, e, f)
#endif /* CFI_DEBUG */

#define qspiFlashIdentify sysMtdIdentify

#define PAGE_SIZE  (0x100)  /* N25Q00 page size is 256bytes */

#define ALT_MODE_1     (1)   /* 1-alt_mode, 0-zynq_mode*/
#define SPEAR_MODE_1   (0)

/**/
/* base_addr: offset-0x800000, reserve low 8M area for uboot parameters */
/* for base_addr: 0x80_0000 flash_area as tffs_area*/
/**/
#undef FLASH_LOW_8M_RESERVD

#define FLASH_RSV_8M_SIZE    (0x800000)

#define TFFS_FLASH_8M_SIZE   (8)
#define TFFS_FLASH_16M_SIZE  (16)
#define TFFS_FLASH_32M_SIZE  (32)
#define TFFS_FLASH_64M_SIZE  (64)

#ifdef TFFS_FLASH_16M
#define TFFS_FLASH_SIZE TFFS_FLASH_16M_SIZE

#elif TFFS_FLASH_32M
#define TFFS_FLASH_SIZE TFFS_FLASH_32M_SIZE

#elif TFFS_FLASH_64M
#define TFFS_FLASH_SIZE TFFS_FLASH_64M_SIZE

else
#define TFFS_FLASH_SIZE TFFS_FLASH_8M_SIZE   // default
#endif


#define TFFS_ONE_LOGIC_DISK    // define: only one logic disk(default), undef: 2 or 4 logic disk


/* jc*/
#ifndef DRV_FM_QSPI  /* without qspi & tffs*/

STATUS tffs_qspiFlashPageWrite
    (
    UINT32         addr,           /* byte offset into flash memory */
    char *      buf,            /* buffer */
    int         byteLen         /* size of bytes */
    )
{
	return OK;
}

	
STATUS tffs_qspiFlashRead
    (
    UINT32         offset,         /* The address to read */
    char *      readBuffer,     /* A pointer to a location to read the data */
    int         byteLen         /* The size to read */
    )
{
	return OK;
}

STATUS tffs_qspiFlashSectorErase_Idx (UINT32 log_idx)
{
	return OK;
}
	
#endif


#include "./bsp_drv/qspi/qspi_2_vx/nor/vxQspi_Norflash.h"
extern vxT_QSPI * g_pQspi0;

/******************************************************************************
*
* f2xFlashMtdRead - MTD read routine (see TrueFFS Programmer's Guide)
*
* This routine is the Memory Technology Device read function.
*
* RETURNS: FLStatus
*
* ERRNO
*/

LOCAL FLStatus sysMtdRead
    (
    FLFlash *   pVol,
    CardAddress address,
    void FAR1 * buffer,
    int         length,
    int         dummy
    )
{
    STATUS  status;
	UINT32 unaligned;
	
#if 0
#ifdef FLASH_LOW_8M_RESERVD
	/* base_addr: offset-0x800000, reserve low 8M area for uboot parameters */
	/* for base_addr: 0x80_0000 flash_area as tffs_area*/
	unaligned = address + FLASH_RSV_8M_SIZE;
#else
	unaligned = address;
#endif

#else
	if (address < 0x1000000)  /* 16M*/
	{
		unaligned = address;
		qspiFlash_Set_Seg_0_16M(g_pQspi0);
	}
	else
	{
		unaligned = address - 0x1000000;	/* base_addr: offset-0		*/
		qspiFlash_Set_Seg_16_32M(g_pQspi0);
	}

#endif

    status = tffs_qspiFlashRead ((int)unaligned, (char *)buffer, length);
	
#ifdef DEBUG_PRINT
		DEBUG_PRINT("sysMtdRead address:0x%X len:%d-buf:0x%X.  \n\n",
				address, length, buffer, 0, 0, 0);
#endif

    return (status == OK ? flOK : flReadFault);
}

/*******************************************************************************
*
* sysMtdWrite - write routine for flash
*
* This routine writes data to the flash.
*
* RETURNS: flOK or flWriteFault if write failed.
*
* ERRNO: N/A
*/

LOCAL FLStatus sysMtdWrite
    (
    FLFlash         vol,
    CardAddress     address,
    const void FAR1 *buffer,
    int             length,
    int             overwrite
    )
{
    UINT32  unaligned;
    UINT8  *buf = (UINT8 *)buffer;
    UINT8  *bufRead = NULL;
    UINT32  left = length;
    int     offset = 0;

#ifdef DEBUG_PRINT
DEBUG_PRINT("\nsysMtdWrite address:0x%x length:0x%x, overwrite(%d).\n",
            address, length, overwrite, 0, 0, 0);
#endif

	if (flWriteProtected(vol.socket))
    {    
    	return flWriteProtect;
    }
    /*
	*/

    /* calculate the program addr, make sure it's aligned */
#if 0

	#ifdef FLASH_LOW_8M_RESERVD
		/* base_addr: offset-0x800000, reserve low 8M area for uboot parameters */
		/* for base_addr: 0x80_0000 flash_area as tffs_area*/
		unaligned = address + FLASH_RSV_8M_SIZE;  
	#else
		unaligned = address;    /* base_addr: offset-0		*/
	#endif
#else
	if (address < 0x1000000)  /* 16M*/
	{
		unaligned = address;
		qspiFlash_Set_Seg_0_16M(g_pQspi0);
	}
	else
	{
		unaligned = address - 0x1000000;	/* base_addr: offset-0		*/
		qspiFlash_Set_Seg_16_32M(g_pQspi0);
	}
#endif
	

    /* address is not page align */
    if ((UINT32)unaligned % PAGE_SIZE)
    {
        if ((length + ((UINT32)unaligned % PAGE_SIZE)) < PAGE_SIZE)
        {
            if (tffs_qspiFlashPageWrite(unaligned, buf, length) != flOK)
            {
                return flWriteFault;
            }
			
            left = 0;
        }
        else
        {
            left = PAGE_SIZE - (UINT32)unaligned % PAGE_SIZE;
            if (tffs_qspiFlashPageWrite(unaligned, buf, left) != flOK)
            {
                return flWriteFault;
            }
			
            left = length - left;
            offset = length - left;
        }
    }

    /* continue page write */
    while (left / PAGE_SIZE)
    {
        if (tffs_qspiFlashPageWrite (unaligned + offset, buf + offset, PAGE_SIZE) != flOK)
            return flWriteFault;
		
        left -= PAGE_SIZE;
        offset = length - left;
    }

    /* write the last portion */
    if (left)
    {
        if (tffs_qspiFlashPageWrite(unaligned + offset, buf + offset, left) != flOK)
            return flWriteFault;
    }

#if 0 /* ALT_MODE_1*/
    bufRead = malloc (length);

    if (bufRead == NULL)
        return flWriteFault;

    if (tffs_qspiFlashRead ((int)address, (char *)bufRead, length) == ERROR)
        {
        free (bufRead);
        return flWriteFault;
        }

    if (tffscmp (bufRead, buffer, length))
        {

#ifdef DEBUG_PRINT
        DEBUG_PRINT("\nDebug: CFISCS write in verification failed!!! \n", 0, 0, 0,
                0, 0, 0);
        /*
        DEBUG_PRINT("\nvol.map (&vol, address, 0)@0x%x buffer@0x%x\n",
                vol.map (&vol, address, 0), buffer, 0, 0, 0, 0);
		*/
#endif
        free (bufRead);
        return flWriteFault;
        }

    free (bufRead);
#endif

#ifdef DEBUG_PRINT
    DEBUG_PRINT("sysMtdWrite address:0x%x length:0x%x, overwrite(%d)-buf:0x%X.\n\n",
            address, length, overwrite, buffer, 0, 0);
#endif

    return flOK;
}

/*******************************************************************************
*
* sysMtdErase - erase routine for flash
*
* This routine erases the specified blocks.
*
* RETURNS: flOK or flWriteFault if erase failed.
*
* ERRNO: N/A
*/

LOCAL FLStatus sysMtdErase
    (
    FLFlash vol,
    int     firstEraseBlock,
    int     numberOfEraseBlocks
    )
{
    FLStatus status = flOK;
    int      iBlock;
    UINT32   sector;

#ifdef DEBUG_PRINT
    DEBUG_PRINT("\n sysMtdErase firstblock %d numberOfEraseBlocks %d.\n",
            firstEraseBlock, numberOfEraseBlocks, 0, 0, 0, 0);
#endif

    if (flWriteProtected(vol.socket))
    {
        return flWriteProtect;
    }
	
#ifdef FLASH_LOW_8M_RESERVD
	/* for base_addr: 0x80_0000 flash_area as tffs_area*/
	firstEraseBlock += FLASH_RSV_8M_SIZE / vol.erasableBlockSize;
#endif

#if ALT_MODE_1  /* alt*/
	sector = firstEraseBlock;
#endif


    for (iBlock = sector;
         (iBlock < (sector + numberOfEraseBlocks)) && status == flOK;
         iBlock++)
    {
		DEBUG_PRINT("sysMtdErase sector:%d / %d \n", iBlock, numberOfEraseBlocks, 0, 0, 0, 0);

		status = tffs_qspiFlashSectorErase_Idx(iBlock);
    }

#ifdef DEBUG_PRINT
    DEBUG_PRINT("sysMtdErase(%d) Done,status=%d \n\n", firstEraseBlock, status, 0, 0, 0, 0);
#endif

    return status;
}

/******************************************************************************
*
* n25Q00Map - MTD map routine
*
* RETURNS: FLStatus
*
*/

LOCAL void FAR0* qspiFlash_Map
    (
    FLFlash* pVol,
    CardAddress address,
    int length
    )
{
#if ALT_MODE_1

    UINT32 flashBaseAddr = 0;
    void FAR0* pFlash = NULL;

	#if 0 /* ok*/
	   /* flashBaseAddr = (pVol->socket->window.baseAddress << 12);*/
	   #if 0 	   	
	    pFlash = (void FAR0*)(FLASH_DATA_BASE_ADRS + address);
	   #else
	   		if (address > 0xff0000)
   			{
				pFlash = (void FAR0*)(FLASH_DATA_BASE_ADRS + address - 0x1000000);
   			}
			else
			{
				pFlash = (void FAR0*)(FLASH_DATA_BASE_ADRS + address);
			}
	   #endif

	#else 
	    /*flashBaseAddr = (pVol->socket->window.baseAddress << 12);*/
		if (address < 0x1000000)  /* 16M*/
		{
			pFlash = (void FAR0*)(FLASH_DATA_BASE_ADRS + address);
			qspiFlash_Set_Seg_0_16M(g_pQspi0);
		}
		else
		{
			pFlash = (void FAR0*)(FLASH_DATA_BASE_ADRS + address - 0x1000000);	/* 32M - 16M*/
			qspiFlash_Set_Seg_16_32M(g_pQspi0);
		}	
	#endif
	
#ifdef DEBUG_PRINT	
    DEBUG_PRINT("\n qspiFlash_Map address:0x%x length:0x%x\n", address, length, 0,0,0,0);	
#endif

#endif

    return(pFlash);
}

/*******************************************************************************
*
* sysMtdIdentify - identify routine for flash
*
* This routine probes the information of the flash part
*
* RETURNS: always flOK.
*
* ERRNO: N/A
*/

FLStatus sysMtdIdentify
    (
    FLFlash vol
    )
{
	int tffS_size_mode = 0;
	
#ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: entering CFISCS identification routine.\n", 0, 0, 0, 0, 0, 0);
#endif

    /* CFI INFO */
    vol.type = 0x1902; /* Vendor and Device ID : S25FL256S:0x4D190201,Spansion */
    vol.erasableBlockSize = 0x10000;  /* 64K FLASH_SECTOR_SIZE;*/

	/**/
	/* TFFS_FLASH_8M_SIZE,  TFFS_FLASH_16M_SIZE*/
	/**/	
	tffS_size_mode = TFFS_FLASH_SIZE;

	switch (tffS_size_mode)
	{
	case TFFS_FLASH_8M_SIZE:
		/* 8M for test - ok */
		vol.chipSize = 0x800000;
		break;
	
	case TFFS_FLASH_16M_SIZE:
		/* 16M for test - ok */
		vol.chipSize = 0x1000000; 
		break;
	
	case TFFS_FLASH_32M_SIZE:
		/* 32M for test */
		vol.chipSize = 0x2000000; 
		break;
	
	case TFFS_FLASH_64M_SIZE:
		/* 64M for test */
		vol.chipSize = 0x4000000; 
		break;		
	}

    vol.interleaving = 1;
    vol.noOfChips = 1;

    vol.erasableBlockSize *= vol.interleaving;

    vol.read = sysMtdRead;
    vol.write = sysMtdWrite;
    vol.erase = sysMtdErase;
    vol.map = qspiFlash_Map;

    return flOK;
}

	
