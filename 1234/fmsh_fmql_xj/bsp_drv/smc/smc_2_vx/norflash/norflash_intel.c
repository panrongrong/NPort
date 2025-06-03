/* norflash_intel.c - fmsh 7020/7045 pSmc driver */

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

#include "norflash_intel.h"

/*
defines 
*/



/* 
norflash_intel
*/

#if 1
/*****************************************************************************
*
* @description
* Writes a command using a 8-bit bus cycle.
*
* @param    
*           BaseAddr is the base address of device.
*           Offset is the offset into the device(s) address space on which
*		    command is required to be written.
*           Cmd is the command/data to write at BaseAddress + Offset.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashINTEL_sndCmd8(UINT32 BaseAddr, UINT32 Offset, UINT32 Cmd)
{
	NOR_WRITE_COMMAND8(((volatile u8*)BaseAddr) + Offset, Cmd);
}

/*****************************************************************************
*
* @description
* Implements a RESET command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashINTEL_Reset_Cmd(vxT_NORFLASH* pNorflash, UINT32 address)
{
    UINT32 baseAddr = pNorflash->baseAddr;
	
	norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_RESET);
}

/*****************************************************************************
*
* @description
* The Read Device Identifier command instructs the device to output manufacturer
* code, device identifier code, block-lock status, protection register data, 
* or configuration register data by giving offset.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           offset Identifier address offset.
*
* @return	
*           Indentification data.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashINTEL_Read_Identify(    vxT_NORFLASH* pNorflash,     UINT32 offset)
{
    UINT32 data;
    UINT32 baseAddr = pNorflash->baseAddr;
    
    /* Issue Read Array Command - just in case that the flash is not in Read Array mode */
    norFlashINTEL_Reset(pNorflash, 0);
	
    /* Issue the Read Device Identifier command at specified address.*/
    norFlashINTEL_sndCmd8(baseAddr, offset, INTEL_CMD_IDIN);
	
    ReadRawData(baseAddr + offset, (UINT8*)&data);
    ReadRawData(baseAddr + offset + 1, (UINT8*)&data);
	
    norFlashINTEL_Reset(pNorflash, 0);
	
    return data;
}

/*****************************************************************************
*
* @description
* Return the status register value.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address offset to be erase.
*
* @return	
*           Status value.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashINTEL_Read_Status(vxT_NORFLASH* pNorflash, UINT32 address)
{
    UINT32 status;
    UINT32 baseAddr = pNorflash->baseAddr;
    
    /* Issue the Read Status Register command at any address. */
    norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_READ_STATUS);
    ReadRawData(baseAddr + address, (UINT8*)&status);
	
    return status;
}

/*****************************************************************************
*
* @description
* Clear the status register.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashINTEL_ClearStatus(vxT_NORFLASH* pNorflash)
{
    UINT32 baseAddr = pNorflash->baseAddr;
    
    /* Issue the Clear Status Register command at any address*/
    norFlashINTEL_sndCmd8(baseAddr, 0, INTEL_CMD_CLEAR_STATUS);
}

/*****************************************************************************
*
* @description
* Unlocks the specified block of the device.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address in sector.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashINTEL_unLock_Sector(vxT_NORFLASH* pNorflash, UINT32 address)
{
    UINT32 baseAddr = pNorflash->baseAddr;
	
     /* Issue Read Array Command - just in case that the flash is not in Read Array mode */
    norFlashINTEL_Reset(pNorflash, 0);    
	 
    /* Clear the status register first.*/
    norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_BLOCK_LOCKSTART);
    norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_BLOCK_UNLOCK);
	
    norFlashINTEL_Reset(pNorflash, 0);
}

/*****************************************************************************
*
* @description
* The Read Device Identifier command instructs the device to output block-lock 
* status.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Address in sector/block.
*
* @return	
*           Indentification data.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashINTEL_Get_BlkLockStatus(vxT_NORFLASH* pNorflash, UINT32 address)
{
    return norFlashINTEL_Read_Identify(pNorflash, (address + INTEL_LOCKSTATUS));
}

/*****************************************************************************
*
* @description
* It implement a program word command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Start address offset to be wrote.
*           data word to be written.
*
* @return	
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashINTEL_Program(    vxT_NORFLASH* pNorflash, UINT32 address, UINT32 data)
{
    UINT32 status;
    UINT8 done = 0;
    UINT32 baseAddr = pNorflash->baseAddr;
	
     /* Issue Read Array Command - just in case that the flash is not in Read Array mode */
    norFlashINTEL_Reset(pNorflash, address);
    
    /*
    // Check if the data already have been erased.
    ReadRawData(busAddress, (UINT8*)&datain);
    if((datain & data)!= data) {
        return NorCommon_ERROR_CANNOTWRITE;
    }
    */
    
    /* Word programming operations are initiated by writing the Word Program Setup command to the device.*/
    norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_PROGRAM_WORD);
	
    /* This is followed by a second write to the device with the address and data to be programmed.*/
    WriteRawData(baseAddr + address, (UINT8*)&data);
    
    /* Status register polling */
    do
	{
        status = norFlashINTEL_Read_Status(pNorflash, address);
		
        /* Check if the device is ready.*/
        if ((status & INTEL_STATUS_DWS) == INTEL_STATUS_DWS )
		{
            /* check if VPP within acceptable limits during program or erase operation.*/
            if ((status & INTEL_STATUS_VPPS) == INTEL_STATUS_VPPS )
			{
                return NorCommon_ERROR_CANNOTWRITE;
            }
			
            /* Check if the erase block operation is completed. */
            if ((status & INTEL_STATUS_PS) == INTEL_STATUS_PS )
			{
                return NorCommon_ERROR_CANNOTWRITE;
            }
			
            /* check if Block locked during program or erase, operation aborted.*/
            else if ((status & INTEL_STATUS_BLS) == INTEL_STATUS_BLS )
			{
                return NorCommon_ERROR_CANNOTWRITE;
            }
            else
			{
                done = 1;
            }
        }
    } while (!done);
    
    norFlashINTEL_ClearStatus(pNorflash);
    norFlashINTEL_Reset(pNorflash, address);
	
    return 0;
}

/*****************************************************************************
*
* @description
* This function implements a RESET command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashINTEL_Reset(vxT_NORFLASH* pNorflash, UINT32 address)
{
    norFlashINTEL_Reset_Cmd(pNorflash, address);
}

/*****************************************************************************
*
* @description
* This function instructs the device to output device id
* by Read Device Identifier command.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           Device ID.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashINTEL_ReadDeviceID(vxT_NORFLASH* pNorflash)
{
    return norFlashINTEL_Read_Identify(pNorflash, INTEL_MANU_ID);
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
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashINTEL_EraseSector(    vxT_NORFLASH* pNorflash,  UINT32 address)
{
    UINT32 status;
    UINT8 done = 0;
    UINT32 baseAddr = pNorflash->baseAddr;
    
     /* Issue Read Array Command - just in case that the flash is not in Read Array mode */
    norFlashINTEL_Reset(pNorflash, address);
   
    /* Check the lock status is locked.*/
    status = norFlashINTEL_Get_BlkLockStatus(pNorflash, address);
	
    if (( status & INTEL_LOCKSTATUS_LOCKED ) == INTEL_LOCKSTATUS_LOCKED)
	{
        norFlashINTEL_unLock_Sector(pNorflash, address);
    }
	
    /* Clear the status register first.*/
    norFlashINTEL_ClearStatus(pNorflash);
	
    /* Block erase operations are initiated by writing the Block Erase Setup command to the address of the block to be erased.*/
	norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_BLOCK_ERASE_1);
	
    /* Next, the Block Erase Confirm command is written to the address of the block to be erased.*/
	norFlashINTEL_sndCmd8(baseAddr, address, INTEL_CMD_BLOCK_ERASE_2);
	
    /* Status register polling */
    do
	{
        status = norFlashINTEL_Read_Status(pNorflash, address);
		
        /* Check if the device is ready.*/
        if ((status & INTEL_STATUS_DWS) == INTEL_STATUS_DWS )
		{
            /* check if VPP within acceptable limits during program or erase operation.*/
            if ((status & INTEL_STATUS_VPPS) == INTEL_STATUS_VPPS )
			{
                norFlashINTEL_Reset(pNorflash, 0);
                return NorCommon_ERROR_CANNOTWRITE;
            }
			
            /* Check if the erase block operation is completed. */
            if ((status & INTEL_STATUS_PS) == INTEL_STATUS_PS )
			{
                norFlashINTEL_Reset(pNorflash, 0);
                return NorCommon_ERROR_CANNOTWRITE;
            }
			
            /* Check if the erase block operation is completed. */
            if ((status & INTEL_STATUS_ES) == INTEL_STATUS_ES )
			{
                norFlashINTEL_Reset(pNorflash, 0);
                return NorCommon_ERROR_CANNOTWRITE;
            }
			
            /* check if Block locked during program or erase, operation aborted.*/
            else if ((status & INTEL_STATUS_BLS) == INTEL_STATUS_BLS )
			{
                norFlashINTEL_Reset(pNorflash, 0);
                return NorCommon_ERROR_CANNOTWRITE;
            }
            else
			{
                done = 1;
            }
        }
    } while (!done);
		
    norFlashINTEL_Reset(pNorflash, address);
	
    return 0;
}

/*****************************************************************************
*
* @description
* This function erases all the block of the device.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*
* @return	
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashINTEL_EraseChip(vxT_NORFLASH* pNorflash)
{
    /* Interl flash have no independent Chip-erase command.*/
    UINT32 i;
    UINT32 sectors;
	
    sectors = norFlashCFI_Get_DevNumOfBlks(pNorflash);
	
    for (i=0; i<sectors; i++)
	{
        if (norFlashINTEL_EraseSector(pNorflash, norFlashCFI_Get_DevSectAddr(pNorflash, i)))
		{
            return NorCommon_ERROR_CANNOTERASE;
        }
    }
	
    return 0;
}

/*****************************************************************************
*
* @description
* This function sends data to the NorFlash chip from the provided buffer.
*
* @param    
*           pNorflash Pointer to an NorFlash instance.
*           address Start address offset to be wrote.
*           buffer Buffer where the data is stored.
*           size Number of bytes that will be written.
*
* @return	
*           returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashINTEL_Write_Data(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size)
{
    UINT32 i;
	
    for (i=0; i<size; i++)
	{
        if (norFlashINTEL_Program(pNorflash, address, buffer[i]))
		{
            return NorCommon_ERROR_CANNOTWRITE;
        }
        address ++;
    }
	
    return 0;
}

#endif


