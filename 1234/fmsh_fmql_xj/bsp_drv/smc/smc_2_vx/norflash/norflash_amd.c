/* norflash_amd.c - fmsh 7020/7045 pSmc driver */

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

#include "norflash_amd.h"

/*
defines 
*/



/* 
norflash_amd
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
* @return	NA.
*
* @note		NA.
*
*****************************************************************************/
static void amd_sndCmd8(UINT32 BaseAddr, UINT32 offset, UINT32 Cmd)
{
	UINT32 offset2 = 0;
	
	offset2 = (2 * offset) + (!(offset & 0x1));
	
	*((volatile UINT8*)(BaseAddr + offset2)) = Cmd;
	return;
}

/*****************************************************************************
*
* @description
* Implements a RESET command.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           address  Dummy data for AMD.
*
* @return	NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashAMD_Reset_Cmd(vxT_NORFLASH* pNorflash)
{
	amd_sndCmd8(pNorflash->baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(pNorflash->baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
	amd_sndCmd8(pNorflash->baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_RESET);
}

/*****************************************************************************
*
* @description
* Read specified manufactory id or device id.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*		    FMSH_SUCCESS if read id is performed.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashAMD_ReadIdentification(vxT_NORFLASH* pNorflash)
{
    UINT32 baseAddr = pNorflash->baseAddr;
    
    /* The amd_Read identification command sequence is initiated by first*/
    /* writing two unlock cycles. */
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
                 
    /* Followed by a third write cycle that contains the autoselect command.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_AUTO_SELECT);
    
    /* The device then enters the autoselect mode. It may read at any address any */
    /* number of times without initiating another autoselect command sequence. */
	ReadRawData((baseAddr + (AMD_MANU_ID << 1)), (UINT8*)&pNorflash->manufacture_id); 
	ReadRawData((baseAddr + (AMD_DEVIDE_ID << 1)), (UINT8*)&pNorflash->device_id1);

    /* The system must write the exit command to return to the read mode*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_RESET);
	
    return FMSH_SUCCESS;
}

/*****************************************************************************
*
* @description
* Toggle the DQ6 bit to check whether the embedded program or 
* erase algorithm is in process or complete.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           Offset is the offset into the device(s) address space on which
*		    command is required to be written.
*
* @return	
*		    NorCommon_ERROR_NONE if operation is complete.
*		    NorCommon_ERROR_TIMEOUT if operation is timeout.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashAMD_PollByToggle(vxT_NORFLASH* pNorflash, UINT32 offset)
{
	UINT8 StatusReg1;
	UINT8 StatusReg2;
	
	UINT32 busAddress;
  	UINT32 polling_counter = 0xFFFFFFFF;

	busAddress = pNorflash->baseAddr + offset;
	
	while (polling_counter--)
	{
		/* Read first status.*/
		ReadRawData(busAddress, &StatusReg1);
		
		/* Read second status*/
		ReadRawData(busAddress, &StatusReg2);
		
		/*If DQ6 did not toggle between two reads, operation is complete.*/
		if ((StatusReg1 & DQ6_MASK) == (StatusReg2 & DQ6_MASK))
		{
			/* DQ6 == NO Toggle.*/
			return (NorCommon_ERROR_NONE);
		}
		else
		{
			/* If DQ5 is zero then operation is not yet complete.*/
			if ((StatusReg2 & DQ5_MASK) != DQ5_MASK)
				continue;
			
			/* If DQ1 is zero then operation is not yet complete.*/
			if ((StatusReg2 & DQ1_MASK) != DQ1_MASK)
				continue;
		}
		
		/* Else (DQ5 == 1 or DQ1 == 1), read DQ6 again.*/
		ReadRawData(busAddress, &StatusReg1);
		ReadRawData(busAddress, &StatusReg2);

		/* If DQ6 did not toggle between two reads, operation is complete.*/
		if ((StatusReg1 & DQ6_MASK) == (StatusReg2 & DQ6_MASK))
		{
			/* DQ6 == NO Toggle.*/
			return (NorCommon_ERROR_NONE);
		}
		else
		{
			/* Else toggle fail, DQ5 == 1, operation is timeout.*/
			if ((StatusReg2 & DQ5_MASK) == DQ5_MASK)
				return (NorCommon_ERROR_TIMEOUT);
			
			/* Else toggle fail, DQ1 == 1, operation is timeout.*/
			if ((StatusReg2 & DQ1_MASK) == DQ1_MASK)
				return (NorCommon_ERROR_TIMEOUT);
		}
		
	}
	
	return (NorCommon_ERROR_TIMEOUT);
}

/*****************************************************************************
*
* @description
* It implement a program word command.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           address Start address offset to be wrote.
*           data word to be written.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashAMD_Program(vxT_NORFLASH* pNorflash, UINT32 address, UINT32 data)
{
    UINT32 busAddress;
    UINT32 baseAddr = pNorflash->baseAddr;
	UINT8 status;
    
    /* The program command sequence is initiated by writing two unlock write cycles.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_PROGRAM);
                 
    /* The program address and data are written next, */
    /* which in turn initiate the Embedded Program algorithm.*/
    busAddress = baseAddr + address;
    WriteRawData(busAddress, (UINT8*)&data);

	status = norFlashAMD_PollByToggle(pNorflash, address);

	return status;
}
 
/*****************************************************************************
*
* @description
* This function implements a RESET command (amd command).
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           NA.
*
* @note		NA.
*
*****************************************************************************/
void norFlashAMD_Reset(vxT_NORFLASH* pNorflash)
{
    norFlashAMD_Reset_Cmd(pNorflash);
}

/*****************************************************************************
*
* @description
* This function instructs the device to output device id
* by Read Device Identifier command.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           Returns 0 if the operation was successful.
*
* @note		NA.
*
*****************************************************************************/
UINT32 norFlashAMD_Read_DevID(vxT_NORFLASH* pNorflash)
{
    return norFlashAMD_ReadIdentification(pNorflash);
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
UINT8 norFlashAMD_EraseSector(vxT_NORFLASH* pNorflash, UINT32 address)
{
    UINT32 busAddress;
    UINT32 baseAddr = pNorflash->baseAddr;
	UINT8 status;
    
    /*Programming is a six-bus-cycle operation. */
    /* The erase command sequence is initiated by writing two unlock write cycles.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
    /* Followed by the program set-up command.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_ERASE_SETUP);
    /* Two additional unlock cycles are written.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
        
    /* Followed by the address of the sector to be erased, and the sector erase command.*/
    busAddress = baseAddr + address;
    WriteCommand(busAddress, AMD_CMD_ERASE_SECTOR);

	status = norFlashAMD_PollByToggle(pNorflash, address);

	return status;
}

/*****************************************************************************
*
* @description
* This function erases all the block of the device.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashAMD_EraseChip(vxT_NORFLASH* pNorflash)
{
    UINT32 baseAddr = pNorflash->baseAddr;
	UINT8 status;
    
    /*Programming is a six-bus-cycle operation. */
    /* The erase command sequence is initiated by writing two unlock write cycles.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
    /* Followed by the program set-up command.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_ERASE_SETUP);
    /* Two additional unlock cycles are written.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_UNLOCK_1);
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_2, AMD_CMD_UNLOCK_2);
                 
    /* Then followed by the chip erase command.*/
	amd_sndCmd8(baseAddr, AMD_OFFSET_UNLOCK_1, AMD_CMD_ERASE_CHIP);

	status = norFlashAMD_PollByToggle(pNorflash, 0);

	return status;
}

/*****************************************************************************
*
* @description
* This function sends data to the NorFlash chip from the provided buffer.
*
* @param    
*           pNorflash  Pointer to an NorFlash instance.
*           address Start address offset to be wrote.
*		    buffer Buffer where the data is stored.
*           size Number of bytes that will be written.
*
* @return	
*           Returns 0 if the operation was successful.
*           otherwise returns an error code.
*
* @note		NA.
*
*****************************************************************************/
UINT8 norFlashAMD_Write_Data(vxT_NORFLASH* pNorflash, UINT32 address, UINT8 *buffer, UINT32 size)
{
    UINT32 i;
    
    for (i = 0; i < size; i++)
	{
        if (norFlashAMD_Program(pNorflash, address, buffer[i]))
		{
            return NorCommon_ERROR_CANNOTWRITE;
        }
		
        address ++;
    }

    return 0;
}

#endif



