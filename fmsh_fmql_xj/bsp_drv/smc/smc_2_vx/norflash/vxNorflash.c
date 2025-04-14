/* vxNorflash.c - fmsh 7020/7045 pSmc driver */

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

/*
DESCRIPTION
	no vxbus struct
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <string.h>
#include <vxbTimerLib.h>

#include "vxNorflash.h"

#include "norflash_cfi.h"
#include "norflash_amd.h"
#include "norflash_intel.h"

/*
defines 
*/
#if 1

#undef VX_DBG
#undef VX_DBG2

#define NORFLASH_DBG

#ifdef NORFLASH_DBG

#define VX_DBG(string, a, b, c, d, e, f)                    \
        if (_func_logMsg != NULL)                           \
           printf(string, a, b, c, d, e, f)  /* (* _func_logMsg)(string, a, b, c, d, e, f) */
#else
#define VX_DBG(string, a, b, c, d, e, f)
#endif

/* error info log */
#define VX_DBG2(string, a, b, c, d, e, f) printf(string, a, b, c, d, e, f)
#endif



/* 
norflash
*/
static vxT_NORFLASH vxNorFlash = {0};
vxT_NORFLASH * g_pNorflash = (vxT_NORFLASH *)(&vxNorFlash.cmdSet);

#if 1

void WriteCommand(UINT32 commandAddr, UINT32 command)
{
	*(volatile UINT8*)(commandAddr) = (UINT8)command;
}

/*****************************************************************************
*
* @description
* This function reads data from the NorFlash chip into the provided buffer.
*
* @param    
*           address  Address of data.
*           buffer  Buffer where the data will be stored.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void ReadRawData(UINT32      address, UINT8* buffer)
{
    *buffer = (*((volatile UINT8*)address));
}

/*****************************************************************************
*
* @description
* This function writes data to the NorFlash chip from the provided buffer.
*
* @param    
*           address  Address of data.
*           buffer  Buffer where the data will be stored.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void WriteRawData(UINT32       address, UINT8* buffer)
{
    (*((volatile UINT8*)address)) = (UINT8)(*buffer);
}

#endif


#if 1

/*****************************************************************************
*
* @description
* This function invokes different associate function to 
* implement a RESET command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address offset.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlash_Reset(vxT_NORFLASH* pNorflash, UINT32 address)
{
    if ((pNorflash->cmdSet) == CMD_SET_AMD) 
	{
        norFlashAMD_Reset(pNorflash);
    }
	else 
	{
        norFlashINTEL_Reset(pNorflash, address);
    }
}

/*****************************************************************************
*
* @description
* This function invokes associate function to 
* implement a read device ID command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           Returns 0 or data if the operation was successful.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlash_Read_DevID(vxT_NORFLASH* pNorflash)
{
    UINT32 ret;
	
    if ((pNorflash->cmdSet) == CMD_SET_AMD) 
	{
        ret = norFlashAMD_Read_DevID(pNorflash);
    } 
	else
	{
        ret = norFlashINTEL_ReadDeviceID(pNorflash);
    }
    return ret;
}

/*****************************************************************************
*
* @description
* This function erases the specified block of the device.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address offset to be erase.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlash_EraseSector(vxT_NORFLASH* pNorflash, UINT32 address)
{
    UINT8 ret;
	
    if ((pNorflash->cmdSet) == CMD_SET_AMD)
	{
        ret = norFlashAMD_EraseSector(pNorflash, address);
    }
	else 
	{
        ret = norFlashINTEL_EraseSector(pNorflash, address);
    }
	
    return ret;
}

/*****************************************************************************
*
* @description
* This function erases the specified block of the device.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address offset to be erase.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlash_EraseChip(vxT_NORFLASH* pNorflash)
{
    UINT8 ret;
	
    if ((pNorflash->cmdSet) == CMD_SET_AMD) 
	{
        ret = norFlashAMD_EraseChip(pNorflash);
    } 
	else 
	{
        ret = norFlashINTEL_EraseChip(pNorflash);
    }
	
    return ret;
}

/*****************************************************************************
*
* @description
* This function sends data to the pNorflash chip from the provided buffer.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Start address offset to be wrote.
*           buffer Buffer where the data is stored.
*           size Number of bytes that will be written.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlash_WriteData(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size)
{
    UINT8 ret;
	
    if ((pNorflash->cmdSet) == CMD_SET_AMD) 
	{
        ret = norFlashAMD_Write_Data(pNorflash,address, buffer, size);
    } 
	else 
	{
        ret = norFlashINTEL_Write_Data(pNorflash,address, buffer, size);
    }
	
    return ret;
}

/*****************************************************************************
*
* @description
* This function reads data from the NorFlash chip into the provided buffer.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address offset to be read.
*           buffer  Buffer where the data will be stored.
*           size  Number of bytes that will be read.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlash_ReadData(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size)
{
    UINT32 busAddress;
    UINT32 i;
    
    busAddress = pNorflash->baseAddr + address;
	
    for (i = 0; i < size; i++) 
	{
        ReadRawData(busAddress, buffer);
		
        buffer++;
        busAddress++;
    }

	return;
}

#endif

#if 1

int vxInit_Smc_Norflash(int bus_width, int chip_sel)
{
	int dev_mode = SMC_DEV_NORFLASH;
	
	vxInit_Smc(dev_mode, bus_width, chip_sel);	
	return 0;
}

#endif

